#include "../../src/platform/RecordingRle.h"
#include <cassert>
#include <cstdio>
#include <vector>

int main() {
  for(unsigned kind=0;kind<3;++kind) {
    std::vector<uint8_t> original(240*135),encoded,decoded;
    for(unsigned i=0;i<original.size();++i)
      original[i]=kind==0 ? 16 : kind==1 ? i%64 : (i/240)%27;
    assert(cpc_recording::runs(original.size(),[&](unsigned i) {return original[i];},
      [&](uint8_t count,uint8_t colour) {encoded.push_back(count);encoded.push_back(colour);return true;}));
    assert(encoded.size()<=original.size()*2 && !(encoded.size()&1));
    for(unsigned i=0;i<encoded.size();i+=2) {
      assert(encoded[i]);decoded.insert(decoded.end(),encoded[i],encoded[i+1]);
    }
    assert(decoded==original);
  }
  unsigned calls=0;
  assert(cpc_recording::runs(0,[](unsigned) {return 0;},[&](uint8_t,uint8_t) {++calls;return false;}));
  assert(!calls);
  assert(!cpc_recording::runs(500,[](unsigned) {return 16;},[](uint8_t,uint8_t) {return false;}));
  std::vector<uint16_t> decoded;
  assert(cpc_recording::runs(640,[](unsigned i) {return uint16_t(i<320?0xffff:0x1234);},
    [&](uint8_t count,uint16_t colour) {decoded.insert(decoded.end(),count,colour);return true;}));
  assert(decoded.size()==640 && decoded[0]==0xffff && decoded[639]==0x1234);
  puts("Recording RLE round trips, maximum runs and transport cancellation: OK");
}
