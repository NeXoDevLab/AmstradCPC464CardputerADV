#include "gbConfig.h"
#include "string.h"
#include "CRTC.h"
#include "CPCem.h"
#include "gbGlobals.h"
#include "Video.h"

int crtcvsync;
int linessincevsync=0;
int galc=0;
//JJ int motoron=0; //no necesito estado motor disco
int vc;
int sc;
int crtcline;
int vsyncpulse;
//JJ int vols[3][512]; //no ncesito sonido
//JJ int pglatchs[5][512]; //no ncesito sonido
//unsigned char resolution=0;
int cpuline;
unsigned char crtctype=0;
unsigned short ma;
unsigned char crtcregs[32];
int crtcreg;
//JJ unsigned short crtctable[8][0x8000]; //precalculada
//static int frames=0,fps=0;
static unsigned char frames=0;
static bool videoEnabled=true;
unsigned char cpcBorderLines[312]={};
void setVideoEnabled(bool enabled) {videoEnabled=enabled;}



void writecrtc(unsigned short a, unsigned char v)
{
 switch (a&0xFF00)
 {
  case 0xBC00: crtcreg=v&31; return;
  case 0xBD00: crtcregs[crtcreg]=v; /*printf("R%i=%02X %i at vc %i %i\n",crtcreg,v,v,vc,linessincevsync); */return;
 }
}

unsigned char readcrtc(unsigned short a)
{
 switch (a&0xFF00)
 {
  case 0xBF00: return crtcregs[crtcreg];
 }
 return 0xFF;
}

void resetcrtc()
{
 vc=sc=crtcline=vsyncpulse=0;
 memset(cpcBorderLines,cpcInkColour(gaborder),sizeof(cpcBorderLines));
}


static unsigned int look2bpp[256];

void remakelookup()
{
 for(unsigned v=0;v<256;++v) {
  uint32_t packed=0;
  for(unsigned p=0;p<4;++p) {
   unsigned pen=cpcPixelPen(v,scrmode,(scrmode==0 || scrmode==3)?p/2:p);
   packed|=uint32_t(cpcInkColour(gapal[pen]))<<(p*8);
  }
  look2bpp[v]=packed;
 }
}

void pollline()
{
 unsigned int a0,a1,a2,a3,a32,pixel;
 int cur_y,cur_x;
 int aux_d;
 int aux_c;
 int aux_crtctable;

 int x,xoff;
 unsigned char v;
 unsigned char force_draw=1; //Dibuja o no dibuja
 //JJ vols[0][cpuline&511]=psgregs[8];//no ncesito sonido
 //JJ vols[1][cpuline&511]=psgregs[9];//no ncesito sonido
 //JJ vols[2][cpuline&511]=psgregs[10];//no ncesito sonido
 //JJ pglatchs[0][cpuline&511]=((psgregs[0]|((psgregs[1]<<8)&0xF00))>>1)+1;//no ncesito sonido
 //JJ pglatchs[1][cpuline&511]=((psgregs[2]|((psgregs[3]<<8)&0xF00))>>1)+1;//no ncesito sonido
 //JJ pglatchs[2][cpuline&511]=((psgregs[4]|((psgregs[5]<<8)&0xF00))>>1)+1;//no ncesito sonido
 //JJ pglatchs[3][cpuline&511]=(psgregs[6]>>3)+1;//no ncesito sonido
 //JJ pglatchs[4][cpuline&511]=((psgregs[11]|((psgregs[12]<<8)&0xFF00))<<18)+1;//no ncesito sonido
 if (gb_frame_crt_skip >= 1)
 {
  force_draw = ((frames & 1) == 1);
 }

 linessincevsync++;
 if(crtcline>=0 && crtcline<312) cpcBorderLines[crtcline]=cpcInkColour(gaborder);
 if (crtcline<(crtcregs[5]&31))
 {
  crtcline++;
  galines++;
  if (vsyncpulse)
  {
   vsyncpulse--;
   if (!vsyncpulse) crtcvsync=0;
  }
  if (galc)
  {
   galc--;
   if (!galc)
   {
    if (galines&32) intreq=1;
    galines=0;
   }
  }
  return;
 }                                 
 // Render the complete 320x200 active picture. The former VGA path packed
 // palette indices as RGB and permuted pixels with x^2, corrupting LCD output.
 if(palchange) {remakelookup();palchange=0;}
 const int activeY = vc * (crtcregs[9] + 1) + sc;
 if (videoEnabled && activeY >= 0 && activeY < 200 && vc < crtcregs[6]) {
   unsigned char *line = gb_buffer_vga[activeY];
   memset(line, cpcInkColour(gaborder), 320);
   const int bytes = crtcregs[1] * 2;
   const int left = bytes < 80 ? (320 - bytes * 4) / 2 : 0;
   for (int bx=0; bx<bytes && bx<80; ++bx) {
     const unsigned addr = (ma + bx) & 0x7FFF;
     const uint8_t data = ram[(addr & 0x7FF) | ((sc & 7) << 11) | ((addr & 0x6000) << 1)];
     const uint32_t packed=look2bpp[data];
     memcpy(line+left+bx*4,&packed,4);
   }
 }
 if(vc<crtcregs[6] && sc==crtcregs[9]) ma+=crtcregs[1]<<1;
 if(sc==crtcregs[9]) {sc=0;vc=(vc+1)&127;}
 else if(!(crtcline<(crtcregs[5]&31))) sc=(sc+1)&31;
 ++crtcline;++galines;
 if(galines==52) {galines=0;intreq=1;}
 if(vsyncpulse && !--vsyncpulse) crtcvsync=0;
 if(galc && !--galc) {if(galines&32) intreq=1;galines=0;}
 if(vc==crtcregs[4]+1) {
   sc=vc=0;
   ma=(crtcregs[13]|((crtcregs[12]&0x3F)<<8))<<1;
 }
 if(vc==crtcregs[7] && !sc) {
   ++frames;gb_sdl_blit=1;crtcline=0;
   vsyncpulse=(crtctype==0 ? crtcregs[3]>>4 : 16)+1;
   galc=2;crtcvsync=1;linessincevsync=0;
 }
 if(crtcline==313) {crtcline=0;gb_sdl_blit=1;}
}
