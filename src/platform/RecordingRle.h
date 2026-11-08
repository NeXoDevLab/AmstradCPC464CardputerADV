#pragma once
#include <stdint.h>

namespace cpc_recording {
// Emit runs without narrowing the colour type (indexed CPC or RGB565).
template<class Pixel,class Emit>
bool runs(unsigned pixels,Pixel pixel,Emit emit) {
  if(!pixels) return true;
  auto colour=pixel(0);uint8_t count=1;
  for(unsigned i=1;i<pixels;++i) {
    const auto next=pixel(i);
    if(next==colour && count<255) {++count;continue;}
    if(!emit(count,colour)) return false;
    colour=next;count=1;
  }
  return emit(count,colour);
}
}
