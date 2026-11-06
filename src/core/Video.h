#pragma once
#include <stdint.h>

// The CPC has three analogue levels per RGB channel, not four VGA levels.
// Both intermediate values of the inherited 2-bit encoding mean half intensity.
inline uint8_t cpcColourComponent(unsigned packed, unsigned shift) {
  static constexpr uint8_t levels[4]={0,128,128,255};
  return levels[(packed>>shift)&3];
}

// Gate Array hardware colours, indexed by the core as hardware code + 1.
// Encoded BBGGRR for the platform-independent framebuffer. No VGA byte swap.
inline uint8_t cpcInkColour(unsigned ink) {
  static constexpr uint8_t colours[35] = {
    0,21,21,29,31,16,35,20,23,35,47,15,63,3,60,7,
    59,16,28,12,61,0,48,4,36,17,29,14,62,1,49,5,58,42,21
  };
  return colours[ink < 35 ? ink : 0];
}

inline uint8_t cpcPixelPen(uint8_t data, unsigned mode, unsigned pixel) {
  if (mode == 0 || mode == 3) {
    data <<= pixel;
    return ((data >> 7) & 1) | ((data >> 2) & 2) |
           ((data >> 3) & 4) | ((data << 2) & 8);
  }
  if (mode == 1) return ((data >> (7-pixel)) & 1) | (((data >> (3-pixel)) & 1) << 1);
  // Mode 2 has 640 pixels: preserve either lit bit when reducing to 320.
  return (data & (0xC0 >> (pixel*2))) ? 1 : 0;
}

// Same composition for LCD output and USB/test captures. A 384x240 viewport
// retains the full 320x200 picture plus the real CRTC border scanlines.
inline uint8_t cpcDisplayPixel(unsigned char *const *picture, const unsigned char *border,
                              unsigned x,unsigned y,bool showBorder) {
  if(!showBorder) return picture[y*200/135][x*320/240];
  unsigned vx=x*384/240,vy=y*240/135;
  if(vx>=32 && vx<352 && vy>=20 && vy<220) return picture[vy-20][vx-32];
  return border[vy+52];
}
