#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../../src/core/Keyboard.h"
#include "../../src/core/Video.h"
#include "../../src/core/Tape.h"
#include "../../src/core/FDC.h"
#include "../../src/core/Audio.h"
unsigned char psgregs[16]={};
int main(int argc,char **argv) {
  assert(argc==2);
  uint8_t r,b;bool shift;
  for(char ch='a';ch<='z';++ch) {assert(cpcAsciiKey(ch,r,b,shift));assert(r<10 && b<8 && !shift);}
  for(char ch='0';ch<='9';++ch) assert(cpcAsciiKey(ch,r,b,shift));
  assert(cpcAsciiKey('a',r,b,shift) && r==8 && b==5);
  assert(cpcAsciiKey('i',r,b,shift) && r==4 && b==3);
  assert(cpcAsciiKey('y',r,b,shift) && r==5 && b==3);
  assert(cpcAsciiKey('\\',r,b,shift) && r==2 && b==6 && !shift);
  assert(cpcAsciiKey('|',r,b,shift) && r==3 && b==2 && shift);
  assert(cpcAsciiKey('"',r,b,shift) && r==8 && b==1 && shift);
  assert(cpcInkColour(5)==16 && cpcInkColour(11)==15);
  assert(cpcColourComponent(cpcInkColour(5),4)==128);
  assert(cpcColourComponent(cpcInkColour(11),0)==255 && cpcColourComponent(cpcInkColour(11),2)==255);
  assert(cpcPixelPen(0x80,1,0)==1 && cpcPixelPen(0x08,1,0)==2);
  assert(cpcPixelPen(0xAA,0,0)==15 && cpcPixelPen(0x55,0,1)==15);
  assert(cpcPixelPen(0x40,2,0)==1);
  std::string path=std::string(argv[1])+"/pulses.cdt";
  const uint8_t data[]={ 'Z','X','T','a','p','e','!',0x1A,1,20,
    0x30,3,'A','B','C',0x12,100,0,2,0,0x20,2,0 };
  FILE *f=fopen(path.c_str(),"wb");assert(f);fwrite(data,1,sizeof(data),f);fclose(f);
  assert(tape::mountCdt(path.c_str()));
  assert(!tape::active());bool level=tape::level();
  tape::advance(10000);assert(tape::level()==level); // stopped motor cannot consume tape
  tape::setMotor(true);assert(tape::active());
  tape::advance(113);assert(tape::level()==level);
  tape::advance(1);assert(tape::level()!=level); // 100 * 8/7 rounded -> 114 CPC clocks
  tape::advance(10000);assert(!tape::active() && !*tape::mountError());
  // A seek outside the physical medium followed by READ ID must fail safely.
  writefdc(0xFB7F,15);writefdc(0xFB7F,0);writefdc(0xFB7F,255);
  writefdc(0xFB7F,10);writefdc(0xFB7F,0);
  assert(readfdc(0xFB7E)==0xD0);
  assert(readfdc(0xFB7F)&0x40);
  for(unsigned i=0;i<6;++i) readfdc(0xFB7F);
  assert(readfdc(0xFB7E)==0x80);
  psgregs[0]=100;psgregs[7]=0x3E;psgregs[8]=15;
  cpc_audio::beginFrame();cpc_audio::advance(80000);
  assert(cpc_audio::size()==320);
  unsigned edges=0,high=0;const int16_t *pcm=cpc_audio::data();
  for(unsigned i=0;i<320;++i) {high+=pcm[i]>0;if(i) edges+=(pcm[i]>0)!=(pcm[i-1]>0);}
  assert(edges>=23 && edges<=26 && high>140 && high<180); // 625 Hz AY tone
  cpc_audio::setEnabled(false);cpc_audio::beginFrame();cpc_audio::advance(80000);assert(!cpc_audio::size());
  cpc_audio::setEnabled(true);
  assert(tape::mountCdt(path.c_str()));tape::setMotor(true);
  memset(psgregs,0,sizeof(psgregs));
  cpc_audio::beginFrame();cpc_audio::advance(80000);
  for(unsigned i=0;i<cpc_audio::size();++i) assert(cpc_audio::data()[i]==-6000);
  tape::eject();
  puts("Keyboard, palette, pixels, tape motor/timing, FDC errors and AY audio: OK");
}
