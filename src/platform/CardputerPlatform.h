#pragma once

#include <Arduino.h>

namespace cpc_platform {
constexpr uint16_t kCpcWidth = 320;
constexpr uint16_t kCpcHeight = 200;

void begin();
bool loadRoms();
bool ready();
void pollKeyboard();
bool openGameMenu();
void present();
void audioTick();
bool shouldRender();
void resetRequested();
}
