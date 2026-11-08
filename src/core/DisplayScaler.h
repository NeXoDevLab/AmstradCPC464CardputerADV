#pragma once
#include "Video.h"
#include <array>
#include <cstring>

namespace cpc_display {
#if defined(__GNUC__) && !defined(__clang__)
#define CPC_DISPLAY_FAST __attribute__((optimize("O3")))
#else
#define CPC_DISPLAY_FAST
#endif
constexpr unsigned kPackedSize=320*200*5/8+1;
constexpr std::array<uint8_t,64> makePackedColours() {
  std::array<uint8_t,64> out{};
  constexpr uint8_t level[4]={0,1,1,2};
  for(unsigned i=0;i<64;++i) out[i]=level[i&3]+3*level[(i>>2)&3]+9*level[(i>>4)&3];
  return out;
}
constexpr std::array<uint8_t,27> makeNativeColours() {
  std::array<uint8_t,27> out{};
  constexpr uint8_t level[3]={0,1,3};
  for(unsigned i=0;i<27;++i) out[i]=level[i%3]|(level[i/3%3]<<2)|(level[i/9]<<4);
  return out;
}
constexpr auto packedColours=makePackedColours();
constexpr auto nativeColours=makeNativeColours();

// Exact 27-colour snapshot: all 320x200 pixels fit in 40,001 bytes.
// The last byte is a sentinel for two-byte reads of the final packed pixel.
CPC_DISPLAY_FAST inline void pack(unsigned char *const *picture,uint8_t *out) {
  for(unsigned y=0;y<200;++y) for(unsigned x=0;x<320;x+=8) {
    const auto *p=picture[y]+x;
    const auto a=packedColours[p[0]&63],b=packedColours[p[1]&63];
    const auto c=packedColours[p[2]&63],d=packedColours[p[3]&63];
    const auto e=packedColours[p[4]&63],f=packedColours[p[5]&63];
    const auto g=packedColours[p[6]&63],h=packedColours[p[7]&63];
    *out++=a|(b<<5);*out++=(b>>3)|(c<<2)|(d<<7);
    *out++=(d>>1)|(e<<4);*out++=(e>>4)|(f<<1)|(g<<6);
    *out++=(g>>2)|(h<<3);
  }
  *out=0;
}
inline __attribute__((always_inline)) uint8_t unpack(const uint8_t *picture,unsigned x,unsigned y) {
  const unsigned bit=(y*320+x)*5,at=bit>>3;
  const unsigned colour=((picture[at]|(unsigned(picture[at+1])<<8))>>(bit&7))&31;
  return nativeColours[colour<27?colour:0];
}
constexpr std::array<uint32_t,64> makeLinearColours() {
  std::array<uint32_t,64> out{};
  constexpr uint32_t level[4]={0,64,64,255};
  for(unsigned i=0;i<64;++i) out[i]=level[i&3]|(level[(i>>2)&3]<<8)|(level[(i>>4)&3]<<16);
  return out;
}
constexpr std::array<uint8_t,256> makeGamma() {
  std::array<uint8_t,256> out{};
  for(unsigned i=0;i<256;++i) {
    const unsigned square=i*255;
    unsigned root=0;
    for(unsigned bit=128;bit;bit>>=1) if((root|bit)*(root|bit)<=square) root|=bit;
    if(root<255 && (root+1)*(root+1)-square<square-root*root) ++root;
    out[i]=root;
  }
  return out;
}
constexpr auto linearColours=makeLinearColours();
constexpr auto gamma=makeGamma();
inline uint16_t rgb565(unsigned r,unsigned g,unsigned b) {
  return ((r&248)<<8)|((g&252)<<3)|(b>>3);
}

// Area coverage keeps every source row/column. Gamma-aware blending keeps thin
// bright strokes visible while preserving uniform CPC colours exactly.
template<class Reader> class Row {
  Reader read;
  const uint8_t *border;
  CpcViewport view;
  bool showBorder;
  unsigned width,height,firstY,lastY,weightY[3],area;
  uint8_t pixel(unsigned x,unsigned y) const {
    if(!showBorder) return read(view.left+x,view.top+y);
    if(x>=32 && x<352 && y>=20 && y<220) return read(x-32,y-20);
    return border[y+52];
  }
 public:
  Row(Reader reader,const uint8_t *lines,unsigned y,bool show,CpcViewport active)
      :read(reader),border(lines),view(active),showBorder(show) {
    width=show?384:view.width;height=show?240:view.height;area=width*height;
    const unsigned from=y*height,to=from+height;
    firstY=from/135;lastY=(to-1)/135;
    for(unsigned sy=firstY;sy<=lastY;++sy) {
      const unsigned a=from>sy*135?from:sy*135,b=to<(sy+1)*135?to:(sy+1)*135;
      weightY[sy-firstY]=b-a;
    }
  }
  uint16_t at(unsigned x) const {
    const unsigned from=x*width,to=from+width,first=from/240,last=(to-1)/240;
    unsigned weights[3];
    for(unsigned sx=first;sx<=last;++sx) {
      const unsigned a=from>sx*240?from:sx*240,b=to<(sx+1)*240?to:(sx+1)*240;
      weights[sx-first]=b-a;
    }
    uint32_t r=0,g=0,b=0;
    for(unsigned sy=firstY;sy<=lastY;++sy) for(unsigned sx=first;sx<=last;++sx) {
      const unsigned weight=weights[sx-first]*weightY[sy-firstY];
      const auto colour=linearColours[pixel(sx,sy)&63];
      r+=(colour&255)*weight;g+=((colour>>8)&255)*weight;b+=(colour>>16)*weight;
    }
    return rgb565(gamma[(r+area/2)/area],gamma[(g+area/2)/area],gamma[(b+area/2)/area]);
  }
};
template<class Reader>
inline auto row(Reader read,const uint8_t *border,unsigned y,bool show,CpcViewport view) {
  return Row<Reader>(read,border,y,show,view);
}

struct LinearPixel {uint8_t r,g,b;};
struct Workspace {
  LinearPixel lines[3][240];
  unsigned tags[3];
};
class Normalise {
  unsigned half,multiplier;
 public:
  explicit Normalise(unsigned divisor):half(divisor/2),multiplier(divisor>1?uint32_t((uint64_t(1)<<32)/divisor)+1:0) {}
  inline __attribute__((always_inline)) unsigned operator()(unsigned value) const {
    if(!multiplier) return value;
    return (uint64_t(value+half)*multiplier)>>32;
  }
};

// Separable coverage filter. Each source pixel is decoded once, and three
// horizontally filtered rows are reused instead of sampling neighbours again
// for every destination pixel. No division occurs in the inner pixel loops.
template<class Reader> class Scaler {
  Reader read;
  const uint8_t *border;
  CpcViewport view;
  bool showBorder;
  unsigned width,height;
  Workspace &work;
  Normalise normalX,normalY;
  bool (*sameRows)(unsigned,unsigned);
  inline __attribute__((always_inline)) uint32_t pixel(unsigned x,unsigned y) const {
    uint8_t colour;
    if(!showBorder) colour=read(view.left+x,view.top+y);
    else if(x>=32 && x<352 && y>=20 && y<220) colour=read(x-32,y-20);
    else colour=border[y+52];
    return linearColours[colour&63];
  }
  CPC_DISPLAY_FAST void horizontal(unsigned y) {
    auto *output=work.lines[y%3];
    if(showBorder && (y<20 || y>=220)) {
      const auto c=linearColours[border[y+52]&63];
      for(unsigned x=0;x<240;++x) output[x]={uint8_t(c),uint8_t(c>>8),uint8_t(c>>16)};
      work.tags[y%3]=y;return;
    }
    if(sameRows && y && work.tags[(y-1)%3]==y-1 &&
       (!showBorder || (y>20 && border[y+52]==border[y+51]))) {
      const unsigned sourceY=showBorder?y-20:view.top+y;
      if(sameRows(sourceY,sourceY-1)) {
        memcpy(output,work.lines[(y-1)%3],sizeof(work.lines[0]));
        work.tags[y%3]=y;return;
      }
    }
    unsigned sourceX=0,available=240;
    auto colour=pixel(0,y);
    for(unsigned x=0;x<240;++x) {
      unsigned remaining=width,r=0,g=0,b=0;
      while(remaining) {
        const unsigned weight=remaining<available?remaining:available;
        r+=(colour&255)*weight;g+=((colour>>8)&255)*weight;b+=(colour>>16)*weight;
        remaining-=weight;available-=weight;
        if(!available) {available=240;if(++sourceX<width) colour=pixel(sourceX,y);}
      }
      output[x]={uint8_t(normalX(r)),uint8_t(normalX(g)),uint8_t(normalX(b))};
    }
    work.tags[y%3]=y;
  }
 public:
  Scaler(Reader reader,const uint8_t *lines,bool show,CpcViewport active,Workspace &storage,bool (*equal)(unsigned,unsigned))
      :read(reader),border(lines),view(active),showBorder(show),width(show?384:active.width),
       height(show?240:active.height),work(storage),normalX(width),normalY(height),sameRows(equal) {
    for(auto &tag:work.tags) tag=~0u;
  }
  CPC_DISPLAY_FAST void render(unsigned y,uint16_t *output) {
    const unsigned from=y*height,to=from+height,first=from/135,last=(to-1)/135;
    unsigned weights[3];const LinearPixel *input[3];
    for(unsigned sy=first;sy<=last;++sy) {
      if(work.tags[sy%3]!=sy) horizontal(sy);
      const unsigned a=from>sy*135?from:sy*135,b=to<(sy+1)*135?to:(sy+1)*135;
      weights[sy-first]=b-a;input[sy-first]=work.lines[sy%3];
    }
    for(unsigned x=0;x<240;++x) {
      unsigned r=0,g=0,b=0;
      for(unsigned i=0;i<=last-first;++i) {
        r+=input[i][x].r*weights[i];g+=input[i][x].g*weights[i];b+=input[i][x].b*weights[i];
      }
      output[x]=rgb565(gamma[normalY(r)],gamma[normalY(g)],gamma[normalY(b)]);
    }
  }
};
#undef CPC_DISPLAY_FAST
template<class Reader>
inline auto scaler(Reader read,const uint8_t *border,bool show,CpcViewport view,Workspace &work,bool (*equal)(unsigned,unsigned)=nullptr) {
  return Scaler<Reader>(read,border,show,view,work,equal);
}
}
