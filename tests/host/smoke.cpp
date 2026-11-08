#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <initializer_list>
#include <cassert>
#include "../../src/core/gbGlobals.h"
#include "../../src/core/Z80.h"
#include "../../src/core/CRTC.h"
#include "../../src/core/GA.h"
#include "../../src/core/FDC.h"
#include "../../src/core/Tape.h"
#include "../../src/core/Keyboard.h"
#include "../../src/core/Video.h"
volatile unsigned char keymap[256]={},oldKeymap[256]={};
unsigned char *ram;
unsigned char **gb_buffer_vga;
unsigned int **gb_buffer_vga32;
unsigned char gb_sync_bits=0,gb_current_ms_poll_sound=1,gb_current_ms_poll_keyboard=10,
gb_current_ms_poll_mouse=10,gb_current_frame_crt_skip=0,gb_current_delay_emulate_ms=0,
gb_auto_delay_cpu=1,gb_sdl_blit=0,gb_screen_xOffset=0,gb_show_osd_main_menu=0,model=0,soundon=1;
static unsigned char os[16384],basic[16384],amsdos[16384],pixels[200][320];
static unsigned char *lines[200];
namespace cpc_platform { bool loadRoms() { return true; } }
void frames(int n) { while(n--) { cpuline=0; execz80(); } }
// Release Shift together with the key, matching the device's typing queue.
void key(int row,int bit,int n=5) { cpckeys[row][bit]=0; frames(n); cpckeys[row][bit]=1; cpckeys[2][5]=1; frames(5); }
void typeCommand(const char *s) {
  for (;*s;++s) {
    uint8_t row,bit; bool shift;
    bool found=cpcAsciiKey(*s,row,bit,shift);
    if(found) {if(shift) cpckeys[2][5]=0; key(row,bit); cpckeys[2][5]=1;}
    if(!found) fprintf(stderr,"unmapped %c\n",*s);
  }
}
void capture(const char *path,bool border=false) {
 FILE *f=fopen(path,"wb"); unsigned width=border?240:320,height=border?135:200;
 fprintf(f,"P6\n%u %u\n255\n",width,height);
 for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x) {
   uint8_t c=border?cpcDisplayPixel(lines,cpcBorderLines,x,y,true):pixels[y][x];
   unsigned char rgb[3]={cpcColourComponent(c,0),cpcColourComponent(c,2),cpcColourComponent(c,4)};fwrite(rgb,1,3,f);
 } fclose(f);
}
int main(int argc,char **argv) {
 if(argc<3) return 2;
 for(auto item : {std::pair<const char*,unsigned char*>{"OS.ROM",os},{"BASIC.ROM",basic},{"AMSDOS.ROM",amsdos}}) {
  char p[1024]; snprintf(p,sizeof(p),"%s/%s",argv[1],item.first); FILE *f=fopen(p,"rb"); if(!f || fread(item.second,1,16384,f)!=16384) return 3; fclose(f);
 }
 ram=(unsigned char*)calloc(1,65536);
 for(int y=0;y<200;y++) lines[y]=pixels[y]; gb_buffer_vga=lines; gb_buffer_vga32=(unsigned int**)lines;
 initz80(); lorom=os; hirom[0]=basic; hirom[7]=amsdos; resetz80(); resetcrtc(); frames(150);
 printf("BOOT pc=%04x inks=%d,%d border=%u mode=%u\n",pc,gapal[0],gapal[1],gaborder,scrmode);
 palchange=0;writega(0x7F00,0x10);writega(0x7F00,0x52);
 assert(gaborder==19 && !palchange); // border changes must not rebuild screen pixel lookup
 writega(0x7F00,0x44);writega(0x7F00,0);
 if(argc>3) {
  if(strstr(argv[3],".dsk")) { if(!mountDsk(argv[3])) { puts(dskMountError()); return 4; } const char *boot=dskBootCommand();assert(strcmp(boot,"cat\n"));printf("AUTOBOOT %s",boot);typeCommand(argc>4 && *argv[4]?argv[4]:boot); }
  else { if(!tape::mountCdt(argv[3])) return 5; typeCommand("|tape\nrun\"\n"); key(5,7); }
  frames(argc>5?atoi(argv[5]):1000);
  if(argc>6 && *argv[6]) {typeCommand(argv[6]);frames(1000);}
  if(argc>6 && *argv[6]) {
    assert(scrmode==0);assert(crtcregs[1]==32 && crtcregs[6]==16);assert(!*tape::mountError());
    unsigned rows=0,picture=0;
    for(auto &row:pixels) {unsigned count=0;for(auto pixel:row) count+=pixel!=0;rows+=count>20;picture+=count;}
    assert(rows>80 && picture>2000); // playing screen must survive geometry changes
  }
 } else {
   assert(scrmode==1 && gapal[0]==5 && gapal[1]==11);
   assert(pixels[100][100]==16); // blue paper, never an unconverted palette index
   unsigned yellow=0;
   for(int y=0;y<72;++y) for(int x=0;x<320;++x) {assert(pixels[y][x]==15 || pixels[y][x]==16);yellow+=pixels[y][x]==15;}
   assert(yellow>200); // complete banner glyphs use yellow only, with blue paper
 }
 capture(argv[2],argc>7); printf("FINAL pc=%04x mode=%u tape=%d error=%s crtc=%u,%u,%u,%u\n",pc,scrmode,tape::active(),tape::mountError(),crtcregs[1],crtcregs[6],crtcregs[9],crtcregs[12]);
 if(argc>7) {bool seen[64]={};unsigned colours=0;for(auto c:cpcBorderLines) if(!seen[c]) {seen[c]=true;++colours;}printf("Real border colours: %u\n",colours);assert(colours>1);}
 free(ram);
}
