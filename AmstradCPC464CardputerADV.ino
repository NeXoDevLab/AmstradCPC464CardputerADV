#include <M5Cardputer.h>

#include "src/CPCem.h"
#include "src/core/CRTC.h"
#include "src/core/FDC.h"
#include "src/core/GA.h"
#include "src/core/PSG.h"
#include "src/core/Z80.h"
#include "src/core/gbGlobals.h"
#include "src/platform/CardputerPlatform.h"

// Estado que originalmente estaba ligado al ejecutable VGA.  Se mantiene
// separado para que el núcleo CPC no dependa del hardware del Cardputer.
volatile unsigned char keymap[256] = {};
volatile unsigned char oldKeymap[256] = {};
unsigned char *ram = nullptr;
unsigned char **gb_buffer_vga = nullptr;
unsigned int **gb_buffer_vga32 = nullptr;
unsigned char gb_sync_bits = 0;
unsigned char gb_current_ms_poll_sound = 1;
unsigned char gb_current_ms_poll_keyboard = 10;
unsigned char gb_current_ms_poll_mouse = 10;
unsigned char gb_current_frame_crt_skip = 0;
unsigned char gb_current_delay_emulate_ms = 0;
unsigned char gb_auto_delay_cpu = 1;
unsigned char gb_sdl_blit = 0;
unsigned char gb_screen_xOffset = 0;
unsigned char gb_show_osd_main_menu = 0;
unsigned char model = 0;
unsigned char soundon = 1;

static unsigned char frameBuffer[cpc_platform::kCpcWidth * cpc_platform::kCpcHeight];
static unsigned char *frameLines[cpc_platform::kCpcHeight];

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("CPC PSRAM: %s (%u/%u bytes libres/totales)\n", psramFound() ? "activa" : "no disponible", ESP.getFreePsram(), ESP.getPsramSize());
  cpc_platform::begin();
  for (uint16_t y = 0; y < cpc_platform::kCpcHeight; ++y)
    frameLines[y] = frameBuffer + y * cpc_platform::kCpcWidth;
  gb_buffer_vga = frameLines;
  gb_buffer_vga32 = reinterpret_cast<unsigned int **>(frameLines);
  // La RAM base del CPC cabe en SRAM interna. No se debe depender de PSRAM:
  // el perfil estándar del Cardputer ADV puede iniciarla desactivada.
  ram = static_cast<unsigned char *>(malloc(0x10000));
  if (!ram) {
    M5.Display.println("Sin memoria CPC");
    while (true) delay(1000);
  }
  initz80();
  // initz80 inicializa los bancos a nulo; las ROM deben montarse después.
  if (!cpc_platform::loadRoms()) return;
  resetz80();
  resetcrtc();
  remakelookup();
}

void loop() {
  cpc_platform::pollKeyboard();
  if (!cpc_platform::ready()) return;
  if (cpc_platform::openGameMenu()) return;
  cpuline = 0;
  const bool draw=cpc_platform::shouldRender();
  setVideoEnabled(draw);
  execz80();
  cpc_platform::audioTick();
  if(draw) cpc_platform::present();
}
