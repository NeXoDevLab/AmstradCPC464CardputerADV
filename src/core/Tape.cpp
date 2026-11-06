#include "Tape.h"
#include <Arduino.h>
#include <SD.h>
namespace {
File file;
uint32_t size=0,pos=0,dataAt=0,length=0,byteAt=0,cacheAt=0xFFFFFFFF;
uint8_t cache[512],bit=0,half=0,bits=8,stage=0;
uint32_t remaining=1,pulse=1,pilotCount=0,pauseMs=0;
uint16_t pilot=0,sync1=0,sync2=0,zero=0,one=0;
bool playing=false,motor=false,tapeLevel=false,pausing=false;
uint8_t pulseCount=0;
uint32_t pulseAt=0,loopAt=0;
uint16_t loops=0;
const char *error="";
uint8_t get(uint32_t at) {
  if(at>=size) {playing=false;error="CDT truncado";return 0;}
  uint32_t base=at&~uint32_t(511);
  if(base!=cacheAt) {
    unsigned count=min<uint32_t>(512,size-base);
    if(!file.seek(base) || file.read(cache,count)!=count) {playing=false;error="Error leyendo CDT de SD";return 0;}
    cacheAt=base;
  }
  return cache[at-base];
}
uint16_t word(uint32_t at) {return get(at)|(get(at+1)<<8);}
uint32_t triple(uint32_t at) {return word(at)|(uint32_t(get(at+2))<<16);}
uint32_t dword(uint32_t at) {return word(at)|(uint32_t(word(at+2))<<16);}
uint32_t ticks(uint32_t n) {return max<uint32_t>(1,(n*8+3)/7);} // 3.5 MHz TZX -> CPC 4 MHz
bool nextBlock() {
  for(unsigned guard=0;pos<size && guard<1024;++guard) {
    uint8_t id=get(pos++);pauseMs=0;bit=half=0;byteAt=0;stage=0;
    switch(id) {
      case 0x10:
        pauseMs=word(pos);length=word(pos+2);pos+=4;
        pilot=2168;sync1=667;sync2=735;zero=855;one=1710;bits=8;
        pilotCount=get(pos)<128?8063:3223;break;
      case 0x11:
        pilot=word(pos);sync1=word(pos+2);sync2=word(pos+4);zero=word(pos+6);one=word(pos+8);
        pilotCount=word(pos+10);bits=get(pos+12);pauseMs=word(pos+13);length=triple(pos+15);pos+=18;break;
      case 0x12:pilot=word(pos);pilotCount=word(pos+2);pos+=4;length=0;stage=5;break;
      case 0x13:pulseCount=get(pos++);pulseAt=pos;pos+=pulseCount*2;length=0;stage=6;break;
      case 0x14:
        zero=word(pos);one=word(pos+2);bits=get(pos+4);pauseMs=word(pos+5);length=triple(pos+7);pos+=10;stage=3;pilotCount=0;break;
      case 0x15:
        zero=word(pos);pauseMs=word(pos+2);bits=get(pos+4);length=triple(pos+5);pos+=8;stage=7;pilotCount=0;break;
      case 0x20:
        pauseMs=word(pos);pos+=2;if(!pauseMs) {playing=false;return false;}
        stage=4;length=0;return true;
      case 0x21:case 0x30:pos+=1+get(pos);continue;
      case 0x22:continue;
      case 0x24:loops=word(pos);pos+=2;loopAt=pos;continue;
      case 0x25:if(loops && --loops) pos=loopAt;continue;
      case 0x2A:pos+=4;continue;
      case 0x2B:tapeLevel=get(pos+4)!=0;pos+=4+dword(pos);continue;
      case 0x31:pos+=2+get(pos+1);continue;
      case 0x32:pos+=2+word(pos);continue;
      case 0x33:pos+=1+get(pos)*3;continue;
      case 0x35:pos+=20+dword(pos+16);continue;
      case 0x5A:pos+=9;continue;
      default:error="Bloque CDT no soportado";playing=false;return false;
    }
    dataAt=pos;pos+=length;
    if(pos>size || (length && (bits<1 || bits>8))) {error="Longitud CDT invalida";playing=false;return false;}
    return true;
  }
  playing=false;return false;
}
uint32_t nextPulse() {
  for(unsigned guard=0;playing && guard<1024;++guard) {
    pausing=false;
    if(stage==0 && pilotCount) {--pilotCount;return ticks(pilot);}
    if(stage==0) {stage=1;return ticks(sync1);}
    if(stage==1) {stage=3;return ticks(sync2);}
    if(stage==5 && pilotCount) {--pilotCount;return ticks(pilot);}
    if(stage==6 && pulseCount) {uint32_t n=ticks(word(pulseAt));pulseAt+=2;--pulseCount;return n;}
    if((stage==3 || stage==7) && byteAt<length) {
      bool high=get(dataAt+byteAt)&(0x80>>bit);
      uint32_t n=ticks(stage==7?zero:high?one:zero);
      if(stage==7) tapeLevel=high;
      if(stage==7 || ++half==2) {half=0;if(++bit==(byteAt+1==length?bits:8)) {bit=0;++byteAt;}}
      return n;
    }
    if(pauseMs) {uint32_t n=pauseMs*4000;pauseMs=0;pausing=true;tapeLevel=false;return n;}
    if(!nextBlock()) return 1;
  }
  return 1;
}
}
namespace tape {
bool mountCdt(const char *path) {
  playing=false;motor=false;tapeLevel=false;error="";
  if(file) file.close();file=SD.open(path,FILE_READ);
  if(!file || file.size()<10) {error="No se puede abrir CDT";return false;}
  uint8_t h[10];
  if(file.read(h,10)!=10 || memcmp(h,"ZXTape!\x1A",8) || h[8]!=1) {error="Cabecera CDT invalida";return false;}
  size=file.size();pos=10;cacheAt=0xFFFFFFFF;loops=0;
  playing=true;if(!nextBlock()) {if(!*error) error="CDT sin bloques reproducibles";return false;}
  remaining=pulse=nextPulse();return playing;
}
void setMotor(bool on) {motor=on;}
void advance(uint32_t tstates) {
  if(!motor) return;
  while(playing && tstates>=remaining) {
    tstates-=remaining;
    if(!pausing && stage!=7) tapeLevel=!tapeLevel;
    remaining=pulse=nextPulse();
  }
  if(playing) remaining-=tstates;
}
bool level() {return tapeLevel;}
bool active() {return playing && motor;}
uint16_t currentPulseTstates() {return min<uint32_t>(65535,pulse);}
const char *mountError() {return error;}
unsigned position() {return dataAt+byteAt;}
unsigned fileSize() {return size;}
bool loaded() {return playing;}
void eject() {playing=motor=tapeLevel=false;if(file) file.close();}
}
