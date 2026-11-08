#include "FDC.h"
#include <Arduino.h>
#include <SD.h>

namespace {
constexpr unsigned kTracks=84, kSides=2, kMaxSectors=16;
struct Sector { uint32_t offset; uint16_t size; uint8_t c,h,r,n; };
Sector sectors[kTracks][kSides][kMaxSectors];
uint8_t counts[kTracks][kSides]={};
File disk;
bool mounted=false, motor=false;
const char *error="Sin disco";
uint8_t cmd=0, args[9], argAt=0, argCount=0;
uint8_t result[7], resultAt=0, resultCount=0;
uint8_t cylinder[4]={}, interruptStatus=0x80;
uint8_t selected=0,head=0,sectorId=0,endId=0, readId=0;
uint8_t buffer[8192];
uint16_t dataAt=0,dataSize=0;
bool transferring=false;
Sector *current=nullptr;
uint16_t word(const uint8_t *p) {return p[0]|(p[1]<<8);}
bool runnableHeader(unsigned directoryTrack,unsigned base,unsigned block) {
  if(block<2) return false;
  // AMSDOS Data/System formats: 1 KiB allocation blocks, 9 x 512-byte sectors.
  const unsigned logical=block*2,track=directoryTrack+logical/9,id=base+logical%9;
  if(track>=kTracks) return false;
  for(unsigned i=0;i<counts[track][0];++i) {
    const auto &s=sectors[track][0][i];
    if(s.r!=id || s.size<128) continue;
    uint8_t header[128];
    if(!disk.seek(s.offset) || disk.read(header,sizeof(header))!=sizeof(header)) return false;
    unsigned checksum=0;for(unsigned j=0;j<67;++j) checksum+=header[j];
    if(checksum!=word(header+67) || !(word(header+24) || word(header+64) || header[66])) return false;
    return header[18]<=1 || (header[18]==2 && word(header+26)!=0);
  }
  return false;
}
void resultPhase(uint8_t st0,uint8_t st1=0,uint8_t st2=0) {
  transferring=false;
  result[0]=st0;result[1]=st1;result[2]=st2;
  result[3]=args[1];result[4]=args[2];result[5]=sectorId;result[6]=args[4];
  resultAt=0;resultCount=7;
}
bool loadSector() {
  if(!mounted || selected || head>=kSides || cylinder[selected]>=kTracks) return false;
  for(unsigned i=0;i<counts[cylinder[selected]][head];++i) {
    Sector &s=sectors[cylinder[selected]][head][i];
    if(s.r!=sectorId || s.c!=args[1] || s.h!=args[2] || s.n!=args[4]) continue;
    if(!s.size || s.size>sizeof(buffer) || !disk.seek(s.offset) || disk.read(buffer,s.size)!=s.size) return false;
    current=&s;dataAt=0;dataSize=s.size;return true;
  }
  return false;
}
void execute() {
  selected=args[0]&3;head=(args[0]>>2)&1;
  switch(cmd) {
    case 3: break; // SPECIFY
    case 4: // SENSE DRIVE STATUS
      result[0]=selected|(head<<2)|0x48|(mounted?0x20:0)|(cylinder[selected]==0?0x10:0);
      resultAt=0;resultCount=1;break;
    case 7: cylinder[selected]=0;interruptStatus=0x20|selected;break;
    case 15: cylinder[selected]=args[1];interruptStatus=0x20|selected;break;
    case 10: { // READ ID
      if(!mounted || selected || cylinder[0]>=kTracks || !counts[cylinder[0]][head]) {resultPhase(0x48,4);break;}
      Sector &s=sectors[cylinder[0]][head][readId++ % counts[cylinder[0]][head]];
      args[1]=s.c;args[2]=s.h;args[4]=s.n;sectorId=s.r;
      resultPhase(head<<2);break;
    }
    case 6: case 12: // READ DATA / READ DELETED DATA
      sectorId=args[3];endId=args[5];
      if(loadSector()) transferring=true;
      else resultPhase(0x40|selected|(head<<2),4);
      break;
    case 5: case 9: case 13: resultPhase(0x40,2);break; // write-protected SD image
    default: break;
  }
}
}

bool mountDsk(const char *path) {
  mounted=false;transferring=false;resultCount=0;argCount=0;
  if(disk) disk.close();disk=SD.open(path,FILE_READ);
  if(!disk) {error="No se puede abrir DSK";return false;}
  uint8_t header[256];
  if(disk.read(header,256)!=256) {error="DSK truncado";return false;}
  bool extended=!memcmp(header,"EXTENDED CPC DSK",16);
  if(!extended && memcmp(header,"MV - CPC",8)) {error="Cabecera DSK invalida";return false;}
  unsigned tracks=header[0x30],sides=header[0x31];
  if(!tracks || tracks>kTracks || !sides || sides>2) {error="Geometria DSK invalida";return false;}
  memset(counts,0,sizeof(counts));
  uint32_t offset=256;
  for(unsigned t=0;t<tracks;++t) for(unsigned h=0;h<sides;++h) {
    unsigned size=extended ? unsigned(header[0x34+t*sides+h])*256 : word(header+0x32);
    if(!size) continue;
    uint8_t th[256];
    if(size<256 || offset+size>disk.size() || !disk.seek(offset) || disk.read(th,256)!=256 || memcmp(th,"Track-Info",10)) {error="Pista DSK truncada";return false;}
    unsigned count=th[0x15];
    if(count>kMaxSectors) {error="Demasiados sectores/pista";return false;}
    uint32_t data=offset+256;
    for(unsigned i=0;i<count;++i) {
      uint8_t *d=th+0x18+i*8;
      unsigned bytes=extended ? word(d+6) : (d[3]<7 ? 128u<<d[3] : 0);
      if(!bytes || bytes>sizeof(buffer) || data+bytes>offset+size) {error="Sector DSK truncado";return false;}
      sectors[t][h][i]={data,(uint16_t)bytes,d[0],d[1],d[2],d[3]};data+=bytes;
    }
    counts[t][h]=count;offset+=size;
  }
  mounted=true;memset(cylinder,0,sizeof(cylinder));error="";return true;
}
const char *dskMountError() {return error;}
const char *dskBootCommand() {
  static char boot[48]; strcpy(boot,"cat\n");
  if(!mounted) return boot;
  char best[13]={};unsigned rank=0,bestUser=0;
  // CP/M directory: data format on track 0 (C1...), system format on track 2 (41...).
  for(unsigned t : {0u,2u}) for(unsigned i=0;i<counts[t][0];++i) {
    const Sector &s=sectors[t][0][i];
    bool dir=t==0 ? s.r>=0xC1 && s.r<=0xC4 : s.r>=0x41 && s.r<=0x44;
    if(!dir || s.size>sizeof(buffer) || !disk.seek(s.offset) || disk.read(buffer,s.size)!=s.size) continue;
    for(unsigned at=0;at+32<=s.size;at+=32) {
      const uint8_t *e=buffer+at;if(e[0]>15 || e[12]) continue;
      char name[9]={},ext[4]={};
      for(unsigned j=0;j<8;++j) name[j]=e[j+1]&127;
      for(int j=7;j>=0 && name[j]==' ';--j) name[j]=0;
      for(unsigned j=0;j<3;++j) ext[j]=e[j+9]&127;
      for(int j=2;j>=0 && ext[j]==' ';--j) ext[j]=0;
      if(!*name) continue;
      unsigned score=!strcmp(ext,"BAS")?3:!strcmp(ext,"BIN")?2:0;
      if(!score && runnableHeader(t,t==0?0xC1:0x41,e[16])) score=1;
      if(score && (!strcmp(name,"DISC") || !strcmp(name,"DISK") || !strcmp(name,"LOADER") || !strcmp(name,"MENU"))) score+=10;
      if(score>rank) {
        rank=score;bestUser=e[0];
        snprintf(best,sizeof(best),"%s.%s",name,ext);
      }
    }
  }
  // A trailing dot explicitly selects a file without an extension.
  if(rank) {
    if(bestUser) snprintf(boot,sizeof(boot),"|user,%u\nrun\"%s\n",bestUser,best);
    else snprintf(boot,sizeof(boot),"run\"%s\n",best);
  }
  return boot;
}
void loaddsk2Flash(unsigned char) {}
void loaddsk() {}
unsigned char readfdc(unsigned short address) {
  if(!(address&0x100)) return 0xFF;
  if(!(address&1)) return resultCount ? 0xD0 : transferring ? 0xF0 : argCount ? 0x90 : 0x80;
  if(resultCount) {
    uint8_t v=result[resultAt++];--resultCount;return v;
  }
  if(transferring) {
    uint8_t v=buffer[dataAt++];
    if(dataAt==dataSize) {
      if(sectorId==endId) resultPhase(selected|(head<<2));
      else {++sectorId;if(!loadSector()) resultPhase(0x40|selected|(head<<2),4);}
    }
    return v;
  }
  return 0xFF;
}
void writefdc(unsigned short address,unsigned char value) {
  if(!(address&0x100)) {motor=value&1;return;}
  if(!(address&1) || transferring || resultCount) return;
  if(argCount) {args[argAt++]=value;if(!--argCount) execute();return;}
  cmd=value&31;argAt=0;
  switch(cmd) {
    case 3:argCount=2;break;
    case 4:case 7:case 10:argCount=1;break;
    case 15:argCount=2;break;
    case 5:case 6:case 9:case 12:argCount=8;break;
    case 13:argCount=5;break;
    case 8:result[0]=interruptStatus;result[1]=cylinder[interruptStatus&3];
      resultAt=0;resultCount=interruptStatus==0x80?1:2;interruptStatus=0x80;break;
    default:result[0]=0x80;resultAt=0;resultCount=1;break;
  }
}
