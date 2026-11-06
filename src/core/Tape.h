#pragma once

#include <stdint.h>

namespace tape {
bool mountCdt(const char *path);
void setMotor(bool on);
const char *mountError();
unsigned position();
unsigned fileSize();
bool loaded();
void eject();
void advance(uint32_t tstates);
bool level();
bool active();
uint16_t currentPulseTstates();
}
