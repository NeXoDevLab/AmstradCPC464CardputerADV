#include "../../src/core/DisplayScaler.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static uint8_t pixels[200][320],border[312];
static uint8_t *lines[200];
static uint8_t packed[cpc_display::kPackedSize];
static cpc_display::Workspace work,otherWork;
bool sameRaw(unsigned a,unsigned b) {return !memcmp(pixels[a],pixels[b],320);}
bool samePacked(unsigned a,unsigned b) {return !memcmp(packed+a*200,packed+b*200,200);}
uint16_t native(uint8_t c) {
  return cpc_display::rgb565(cpcColourComponent(c,0),cpcColourComponent(c,2),cpcColourComponent(c,4));
}
void preview(const char *input,const char *prefix) {
  FILE *in=fopen(input,"rb");assert(in);
  char tag[3];unsigned w,h,max;
  assert(fscanf(in,"%2s%u%u%u",tag,&w,&h,&max)==4 && w==320 && h==200 && max==255);fgetc(in);
  for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) {
    unsigned char rgb[3];assert(fread(rgb,1,3,in)==3);
    auto component=[](unsigned c) {return c<64?0:c<192?1:3;};
    pixels[y][x]=component(rgb[0])|(component(rgb[1])<<2)|(component(rgb[2])<<4);
  }
  fclose(in);
  for(unsigned filter=0;filter<2;++filter) {
    const auto name=std::string(prefix)+(filter?"-despues.ppm":"-antes.ppm");
    FILE *out=fopen(name.c_str(),"wb");assert(out);fprintf(out,"P6\n960 540\n255\n");
    auto scaler=cpc_display::scaler([](unsigned x,unsigned y) {return pixels[y][x];},border,false,CpcViewport{0,0,320,200},work,sameRaw);
    uint16_t line[240];
    for(unsigned y=0;y<540;++y) {
      if(!(y%4) && filter) scaler.render(y/4,line);
      for(unsigned x=0;x<960;++x) {
        const auto c=filter?line[x/4]:native(cpcDisplayPixel(lines,border,x/4,y/4,false));
        const uint8_t rgb[3]={uint8_t((c>>11)*255/31),uint8_t(((c>>5)&63)*255/63),uint8_t((c&31)*255/31)};
        fwrite(rgb,1,3,out);
      }
    }
    fclose(out);printf("%s\n",name.c_str());
  }
}
int main(int argc,char **argv) {
  for(unsigned y=0;y<200;++y) lines[y]=pixels[y];
  if(argc==3) {preview(argv[1],argv[2]);return 0;}
  for(unsigned divisor=1;divisor<=384;++divisor) {
    cpc_display::Normalise normal(divisor);
    for(unsigned colour=0;colour<256;++colour) assert(normal(colour*divisor)==colour);
  }
  for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x) pixels[y][x]=(x*17+y*11+(x*y>>3))%64;
  cpc_display::pack(lines,packed);
  for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;++x)
    assert(native(pixels[y][x])==native(cpc_display::unpack(packed,x,y)));
  auto direct=cpc_display::scaler([](unsigned x,unsigned y) {return pixels[y][x];},border,false,CpcViewport{0,0,320,200},work,sameRaw);
  auto snapshot=cpc_display::scaler([](unsigned x,unsigned y) {return cpc_display::unpack(packed,x,y);},border,false,CpcViewport{0,0,320,200},otherWork,samePacked);
  uint16_t output[240],other[240];
  for(unsigned y=0;y<135;++y) {direct.render(y,output);snapshot.render(y,other);assert(!memcmp(output,other,sizeof(output)));}
  for(unsigned c=0;c<64;++c) {
    memset(pixels,c,sizeof(pixels));memset(border,c,sizeof(border));
    for(bool show : {false,true}) {
      auto scaler=cpc_display::scaler([](unsigned x,unsigned y) {return pixels[y][x];},border,show,CpcViewport{0,0,320,200},work,sameRaw);
      for(unsigned y : {0u,1u,67u,133u,134u}) {
        scaler.render(y,output);
        for(unsigned x : {0u,1u,119u,238u,239u}) assert(output[x]==native(c));
      }
    }
  }
  // A one-pixel stroke must survive at every source position, including rows
  // and columns the old nearest-neighbour sampling discarded completely.
  memset(pixels,16,sizeof(pixels));
  for(unsigned column=0;column<320;++column) {
    for(auto &line:pixels) line[column]=15;
    auto scaler=cpc_display::scaler([](unsigned x,unsigned y) {return pixels[y][x];},border,false,CpcViewport{0,0,320,200},work,sameRaw);
    scaler.render(50,output);
    bool visible=false;for(unsigned x=0;x<240;++x) visible|=(output[x]&0xf800)!=0;
    assert(visible);for(auto &line:pixels) line[column]=16;
  }
  for(unsigned sourceY=0;sourceY<200;++sourceY) {
    memset(pixels[sourceY],15,320);bool visible=false;
    auto scaler=cpc_display::scaler([](unsigned x,unsigned y) {return pixels[y][x];},border,false,CpcViewport{0,0,320,200},work,sameRaw);
    for(unsigned y=0;y<135;++y) {
      scaler.render(y,output);visible|=(output[50]&0xf800)!=0;
    }
    assert(visible);memset(pixels[sourceY],16,320);
  }
  // Shrinking a game preserves its declared viewport and all four corners.
  memset(pixels,7,sizeof(pixels));const CpcViewport view{32,0,256,128};
  const uint8_t corners[4]={1,3,12,48};
  for(unsigned i=0;i<4;++i) for(unsigned y=0;y<2;++y) for(unsigned x=0;x<2;++x)
    pixels[(i&2?126:0)+y][(i&1?286:32)+x]=corners[i];
  auto scaler=cpc_display::scaler([](unsigned x,unsigned y) {return pixels[y][x];},border,false,view,work,sameRaw);
  for(unsigned i=0;i<4;++i) {
    scaler.render(i&2?134:0,output);assert(output[i&1?239:0]==native(corners[i]));
  }
  puts("Display scaler: lossless snapshot, exact palette, every thin stroke, borders and viewport corners OK");
}
