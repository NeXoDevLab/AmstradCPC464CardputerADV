#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../../src/core/FDC.h"

// Small synthetic AMSDOS images, without ROMs or copyrighted game data.
void fixture(const std::string &path,const char *name,const char *ext,
             bool system=false,bool extended=false,bool corrupt=false,
             unsigned type=2,unsigned entry=0x7f00,unsigned user=0,bool loader=false) {
  constexpr unsigned trackBytes=256+9*512;
  const unsigned directoryTrack=system?2:0,tracks=directoryTrack+1,base=system?0x41:0xc1;
  std::vector<unsigned char> image(256+tracks*trackBytes,0);
  memcpy(image.data(),extended?"EXTENDED CPC DSK":"MV - CPC",extended?16:8);
  image[0x30]=tracks;image[0x31]=1;
  image[0x32]=trackBytes&255;image[0x33]=trackBytes>>8;
  // Physical interleave deliberately differs from logical sector ordering.
  const unsigned order[9]={0,5,1,6,2,7,3,8,4};
  unsigned directory=0,header=0;
  for(unsigned t=0;t<tracks;++t) {
    image[0x34+t]=trackBytes/256;
    const unsigned off=256+t*trackBytes;
    memcpy(image.data()+off,"Track-Info",10);image[off+0x15]=9;
    for(unsigned i=0;i<9;++i) {
      auto *sector=image.data()+off+0x18+i*8;
      sector[0]=t;sector[2]=base+order[i];sector[3]=2;sector[7]=2;
      const unsigned data=off+256+i*512;
      if(t==directoryTrack && order[i]<4) {
        memset(image.data()+data,0xe5,512);
        if(!order[i]) directory=data;
      }
      if(t==directoryTrack && order[i]==4) header=data;
    }
  }
  auto *record=image.data()+directory;
  memset(record,0,32);memset(record+1,' ',11);record[0]=user;record[15]=1;record[16]=2;
  memcpy(record+1,name,strlen(name));memcpy(record+9,ext,strlen(ext));
  record[9]|=0x80; // Attribute bits do not belong to the extension.
  auto *data=image.data()+header;
  data[18]=type;data[24]=data[64]=1;data[26]=entry&255;data[27]=entry>>8;
  unsigned checksum=0;for(unsigned i=0;i<67;++i) checksum+=data[i];
  data[67]=(checksum&255)^(corrupt?1:0);data[68]=checksum>>8;
  if(loader) {
    auto *second=record+32;
    memset(second,0,32);memcpy(second+1,"DISC    BAS",11);second[15]=1;second[16]=3;
  }
  FILE *file=fopen(path.c_str(),"wb");assert(file);
  assert(fwrite(image.data(),1,image.size(),file)==image.size());fclose(file);
  assert(mountDsk(path.c_str()));
}
int main(int argc,char **argv) {
  assert(argc==2);const std::string path=std::string(argv[1])+"/boot.dsk";
  fixture(path,"PAC-MAN","");assert(!strcmp(dskBootCommand(),"run\"PAC-MAN.\n"));
  fixture(path,"MY GAME","X");assert(!strcmp(dskBootCommand(),"run\"MY GAME.X\n"));
  fixture(path,"LOADER","XYZ",true,true);assert(!strcmp(dskBootCommand(),"run\"LOADER.XYZ\n"));
  fixture(path,"GAME","X",false,false,true);assert(!strcmp(dskBootCommand(),"cat\n"));
  fixture(path,"DATA","",false,false,false,2,0);assert(!strcmp(dskBootCommand(),"cat\n"));
  fixture(path,"BASIC","X",false,false,false,0,0);assert(!strcmp(dskBootCommand(),"run\"BASIC.X\n"));
  fixture(path,"GAME","X",false,false,false,2,0x7f00,3);
  assert(!strcmp(dskBootCommand(),"|user,3\nrun\"GAME.X\n"));
  fixture(path,"SCREEN","SCR",false,false,false,2,0x7f00,0,true);
  assert(!strcmp(dskBootCommand(),"run\"DISC.BAS\n"));
  puts("DSK autoboot: extensionless files, custom extensions, headers, users, interleave and loader priority: OK");
}
