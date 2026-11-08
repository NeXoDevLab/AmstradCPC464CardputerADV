#include <M5Cardputer.h>
#include <esp_ota_ops.h>

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
// Reserve the complete CPC address space before drivers fragment the heap.
// A late malloc(65536) can fail even with more than 100 KiB free in total.
static unsigned char cpcRam[0x10000];
unsigned char *ram = cpcRam;
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
  const esp_partition_t *partition = esp_ota_get_running_partition();
  if (partition) Serial.printf("CPC-ADV FIX9 partition=%s offset=0x%08x size=%u RAM=fixed-65536\n",
                              partition->label,partition->address,partition->size);
  for (uint16_t y = 0; y < cpc_platform::kCpcHeight; ++y)
    frameLines[y] = frameBuffer + y * cpc_platform::kCpcWidth;
  gb_buffer_vga = frameLines;
  gb_buffer_vga32 = reinterpret_cast<unsigned int **>(frameLines);
  cpc_platform::begin();
  if (!cpc_platform::ready()) return;
  initz80();
  // initz80 clears the ROM banks. Rebind the cached ROMs without reading SD again.
  if (!cpc_platform::loadRoms()) return;
  resetz80();
  resetcrtc();
  remakelookup();
  Serial.printf("CPC BOOT ready RAM=%p heap=%u max=%u\n",ram,ESP.getFreeHeap(),ESP.getMaxAllocHeap());
}

void loop() {
  cpc_platform::pollKeyboard();
  if (!cpc_platform::ready()) { delay(20); return; }
  if (cpc_platform::openGameMenu()) return;
  cpuline = 0;
  const bool draw=cpc_platform::shouldRender();
  setVideoEnabled(draw);
  execz80();
  cpc_platform::audioTick();
  if(draw) cpc_platform::present();
}
