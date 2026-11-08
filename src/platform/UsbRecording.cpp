#include "UsbRecording.h"
#ifdef CPC_USB_RECORDING
#include "RecordingRle.h"
#include <Arduino.h>
#include "../core/Audio.h"
#include "../core/CRTC.h"
#include "../core/Tape.h"
#include "../core/Video.h"
#include "../core/DisplayScaler.h"
#include "../core/gbGlobals.h"

namespace cpc_recording {
namespace {
bool recording=false,collecting=false,continueKey=false;
char path[192];unsigned pathAt=0;
uint32_t sequence=0,samples=0;
cpc_display::Workspace scaling;
bool write(const uint8_t *data,unsigned size) {
  while(size) {
    const auto sent=Serial.write(data,size);
    if(!sent) {recording=false;return false;}
    data+=sent;size-=sent;
  }
  return true;
}
void little(uint8_t *p,uint32_t value,unsigned bytes=4) {
  for(unsigned i=0;i<bytes;++i) p[i]=value>>(i*8);
}
}
bool active() {return recording;}
bool videoFrame() {return !(sequence&1);}
bool takeContinueKey() {const bool key=continueKey;continueKey=false;return key;}
void pollSerial(bool (*prepare)(const char *)) {
  if(!recording && !collecting && Serial.peek()!='@') return;
  while(Serial.available()) {
    const char ch=Serial.read();
    if(recording) {
      if(ch=='!') {recording=false;Serial.print("CPCREC-END\n");}
      else if(ch=='3') continueKey=true;
      continue;
    }
    if(!collecting) {if(ch=='@') {collecting=true;pathAt=0;}continue;}
    if(ch=='\n') {
      path[pathAt]=0;collecting=false;
      if(!pathAt || pathAt>=sizeof(path)-1 || !prepare(path)) {
        Serial.println("CPCREC-ERROR: no se puede montar la cinta");continue;
      }
      sequence=samples=0;continueKey=false;recording=true;
      Serial.print("CPCREC2 240 135 16000 4000000 79872\n");
    } else if(ch!='\r' && pathAt<sizeof(path)-1) path[pathAt++]=ch;
  }
}
void frame() {
  if(!recording) return;
  const bool video=videoFrame();
  const bool border=cpcTapeBorderVisible(true,false,tape::loaded(),tape::active(),tape::position(),tape::fileSize());
  const auto view=cpcActiveViewport(crtcregs[1],crtcregs[6],unsigned(crtcregs[9])+1);
  auto scaler=cpc_display::scaler([](unsigned x,unsigned y) {return gb_buffer_vga[y][x];},cpcBorderLines,border,view,scaling,
                                 [](unsigned a,unsigned b) {return !memcmp(gb_buffer_vga[a],gb_buffer_vga[b],320);});
  uint16_t line[240];
  auto pixel=[&](unsigned n) {
    if(!(n%240)) scaler.render(n/240,line);
    return line[n%240];
  };
  unsigned bytes=0;
  if(video) runs(240*135,pixel,[&](uint8_t,uint16_t) {bytes+=3;return true;});
  const unsigned pcm=cpc_audio::size();
  uint8_t header[28]={'C','P','C','R'};
  little(header+4,sequence);little(header+8,samples);
  little(header+12,pcm,2);
  little(header+14,(tape::active()?1u:0u)|(tape::loaded()?2u:0u)|(*tape::mountError()?4u:0u),2);
  little(header+16,bytes);little(header+20,tape::position());little(header+24,tape::fileSize());
  if(!write(header,sizeof(header)) || !write(reinterpret_cast<const uint8_t *>(cpc_audio::data()),pcm*2)) return;
  if(video) {
    uint8_t buffer[510];unsigned used=0;
    const bool ok=runs(240*135,pixel,[&](uint8_t count,uint16_t colour) {
      buffer[used++]=count;buffer[used++]=colour&255;buffer[used++]=colour>>8;
      if(used<sizeof(buffer)) return true;
      const bool sent=write(buffer,used);used=0;return sent;
    });
    if(!ok || (used && !write(buffer,used))) return;
  }
  samples+=pcm;++sequence;
}
}
#endif
