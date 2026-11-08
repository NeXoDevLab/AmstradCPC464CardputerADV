#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
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
  assert(cpcVolumeKey('+',true)==1 && cpcVolumeKey('-',true)==-1);
  assert(cpcVolumeKey('=',true)==1 && cpcVolumeKey('_',true)==-1);
  for(char ch : std::string("+-=_")) assert(!cpcVolumeKey(ch,false));
  assert(!cpcVolumeKey('a',true));
  assert(cpcAsciiKey('+',r,b,shift) && shift);
  assert(cpcAsciiKey('-',r,b,shift) && !shift);
  assert(cpcAsciiKey('=',r,b,shift) && shift);
  uint8_t volume=cpcDefaultVolume;assert(unsigned(volume)*100/255==30);
  volume=cpcVolumeStep(volume,1);assert(unsigned(volume)*100/255==35);
  volume=cpcVolumeStep(volume,-1);assert(volume==cpcDefaultVolume);
  for(unsigned i=0;i<30;++i) volume=cpcVolumeStep(volume,-1);
  assert(volume==0 && cpcVolumeStep(volume,-1)==0);
  for(unsigned i=0;i<30;++i) volume=cpcVolumeStep(volume,1);
  assert(volume==255 && cpcVolumeStep(volume,1)==255);
  for(bool direct : {false,true}) for(bool fn : {false,true}) {
    const auto input=cpcGameInput(direct,fn,true,true,true,true,true);
    bool matrix[10][8]={};
    cpcApplyGameInput(input,[&](unsigned row,unsigned bit) {assert(row<10 && bit<8);matrix[row][bit]=true;});
    const bool enabled=direct || fn;
    for(auto key : {std::pair<unsigned,unsigned>{0,0},{0,2},{1,0},{0,1},
                    {9,0},{9,1},{9,2},{9,3},{9,4}}) assert(matrix[key.first][key.second]==enabled);
    assert(!matrix[0][3] && !matrix[5][7]); // No F9 or synthetic space from directions.
    for(char ch : std::string(";:.,<>/?")) assert(cpcGameArrow(ch,direct,fn)==enabled);
    assert(!cpcGameArrow('a',direct,fn) && !cpcGameArrow(' ',direct,fn));
  }
  // Each physical direction reaches the matching cursor and joystick bit.
  const unsigned cursorRow[4]={0,0,1,0},cursorBit[4]={0,2,0,1};
  for(unsigned direction=0;direction<4;++direction) {
    bool matrix[10][8]={};
    auto input=cpcGameInput(true,false,direction==0,direction==1,direction==2,direction==3,false);
    cpcApplyGameInput(input,[&](unsigned row,unsigned bit) {matrix[row][bit]=true;});
    unsigned count=0;for(auto &row : matrix) for(bool pressed : row) count+=pressed;
    assert(count==2 && matrix[cursorRow[direction]][cursorBit[direction]] && matrix[9][direction]);
  }
  // BASIC keeps punctuation and space; gameplay adds FIRE without losing SPACE.
  for(char ch : std::string(";.,/ ")) assert(cpcAsciiKey(ch,r,b,shift) && !cpcGameArrow(ch,false,false));
  assert(cpcInkColour(5)==16 && cpcInkColour(11)==15);
  assert(cpcColourComponent(cpcInkColour(5),4)==128);
  assert(cpcColourComponent(cpcInkColour(11),0)==255 && cpcColourComponent(cpcInkColour(11),2)==255);
  assert(cpcPixelPen(0x80,1,0)==1 && cpcPixelPen(0x08,1,0)==2);
  assert(cpcPixelPen(0xAA,0,0)==15 && cpcPixelPen(0x55,0,1)==15);
  assert(cpcPixelPen(0x40,2,0)==1);
  assert(cpcTapeBorderVisible(true,false,true,false,10,100)); // pause between blocks
  assert(cpcTapeBorderVisible(true,false,true,true,100,100)); // final data still playing
  assert(!cpcTapeBorderVisible(true,false,true,false,100,100)); // game stopped motor at EOF
  assert(!cpcTapeBorderVisible(true,true,true,true,10,100)); // fast loading
  assert(!cpcTapeBorderVisible(false,false,true,true,10,100));
  assert(!cpcTapeBorderVisible(true,false,false,false,0,0));
  // Reduced game geometry fills the LCD and keeps every corner visible.
  static unsigned char screen[200][320],border[312];
  unsigned char *lines[200];
  for(unsigned y=0;y<200;++y) lines[y]=screen[y];
  memset(screen,0xFF,sizeof(screen));memset(border,16,sizeof(border));
  const auto view=cpcActiveViewport(32,16,8);
  assert(view.left==32 && view.top==0 && view.width==256 && view.height==128);
  for(unsigned y=0;y<128;++y) memset(screen[y]+32,7,256);
  screen[0][32]=1;screen[0][287]=2;screen[127][32]=3;screen[127][287]=4;
  assert(cpcDisplayPixel(lines,border,0,0,false,view)==1);
  assert(cpcDisplayPixel(lines,border,239,0,false,view)==2);
  assert(cpcDisplayPixel(lines,border,0,134,false,view)==3);
  assert(cpcDisplayPixel(lines,border,239,134,false,view)==4);
  for(unsigned y=0;y<135;++y) for(unsigned x=0;x<240;++x) {
    assert(cpcDisplayPixel(lines,border,x,y,false,view)!=0xFF);
    assert(cpcDisplayPixel(lines,border,x,y,true,view)==cpcDisplayPixel(lines,border,x,y,true));
  }
  for(auto full : {cpcActiveViewport(40,25,8),cpcActiveViewport(0,0,0),cpcActiveViewport(255,255,256)}) {
    assert(full.left==0 && full.top==0 && full.width==320 && full.height==200);
    assert(cpcDisplayPixel(lines,border,239,134,false,full)==screen[134*200/135][239*320/240]);
  }
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
  puts("Keyboard, palette, pixels, fullscreen viewport, tape motor/timing, FDC errors and AY audio: OK");
}
