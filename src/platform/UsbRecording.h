#pragma once
#ifdef CPC_USB_RECORDING
namespace cpc_recording {
void pollSerial(bool (*prepare)(const char *));
bool active();
bool videoFrame();
bool takeContinueKey();
void frame();
}
#endif
