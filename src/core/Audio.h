#pragma once
#include <stddef.h>
#include <stdint.h>
namespace cpc_audio {
void beginFrame();
void advance(unsigned tstates);
void triggerEnvelope();
const int16_t *data();
size_t size();
void setTapeSound(bool on);
void setEnabled(bool on);
}
