#include "Audio.h"
#include "gbGlobals.h"
#include "Tape.h"
namespace {
int16_t blocks[3][512];
unsigned block=0,used=0,sampleClock=0;
bool tapeSound=true;
bool enabled=true;
uint32_t tones[3]={},noiseClock=0,envelopeClock=0,noise=0x1FFFF;
unsigned envelopeStep=0;bool envelopeHolding=false;
const int volumes[16]={0,30,44,63,93,135,193,283,405,586,848,1228,1778,2575,3729,5400};
int sample() {
  unsigned period=(psgregs[11]|(psgregs[12]<<8));if(!period) period=1;
  envelopeClock+=250;
  while(envelopeClock>=period*1024) {
    envelopeClock-=period*1024;
    if(!envelopeHolding) {
      ++envelopeStep;
      if(envelopeStep>=16) {
        unsigned shape=psgregs[13]&15;
        if(!(shape&8) || (shape&1)) {envelopeHolding=true;envelopeStep=16;}
      }
    }
  }
  unsigned shape=psgregs[13]&15, env;
  if(envelopeHolding) env=(shape&8) && (((shape>>2)^(shape>>1))&1)?15:0;
  else {bool up=shape&4;if((shape&2) && (envelopeStep&16)) up=!up;env=up?(envelopeStep&15):15-(envelopeStep&15);}
  unsigned np=(psgregs[6]&31);if(!np) np=1;
  noiseClock+=250;
  while(noiseClock>=np*64) {noiseClock-=np*64;noise=(noise>>1)|(((noise^(noise>>3))&1)<<16);}
  int out=0;
  for(unsigned ch=0;ch<3;++ch) {
    unsigned tp=psgregs[ch*2]|((psgregs[ch*2+1]&15)<<8);if(!tp) tp=1;
    tones[ch]=(tones[ch]+250)%(tp*64);
    bool gate=((psgregs[7]&(1<<ch)) || tones[ch]<tp*32) &&
              ((psgregs[7]&(8<<ch)) || (noise&1));
    unsigned v=psgregs[8+ch];out+=gate?volumes[v&16?env:v&15]:0;
  }
  if(tapeSound && tape::active()) out+=(tape::level()?6000:-6000);
  return out;
}
}
namespace cpc_audio {
void beginFrame() {block=(block+1)%3;used=0;}
void advance(unsigned tstates) {if(!enabled) return;sampleClock+=tstates;while(sampleClock>=250) {sampleClock-=250;int v=sample();if(used<512) blocks[block][used++]=v;}}
void triggerEnvelope() {envelopeStep=0;envelopeClock=0;envelopeHolding=false;}
const int16_t *data() {return blocks[block];}
size_t size() {return used;}
void setTapeSound(bool on) {tapeSound=on;}
void setEnabled(bool on) {enabled=on;}
}
