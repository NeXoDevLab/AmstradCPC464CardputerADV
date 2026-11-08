#include "CardputerPlatform.h"
#include "GameFiles.h"

#include <M5Cardputer.h>
#include <SD.h>
#include <atomic>
#include <memory>
#include <new>
#include <utility>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "../core/gbGlobals.h"
#include "../core/Z80.h"
#include "../core/CRTC.h"
#include "../core/GA.h"
#include "../core/FDC.h"
#include "../core/Tape.h"
#include "../core/Keyboard.h"
#include "../core/Audio.h"
#include "../core/Video.h"

namespace {
constexpr int kSdSck = 40;
constexpr int kSdMiso = 39;
constexpr int kSdMosi = 14;
constexpr int kSdCs = 12;
constexpr uint16_t kScreenWidth = 240;
constexpr uint16_t kScreenHeight = 135;
constexpr uint16_t kImageWidth = 240;
constexpr uint16_t kImageHeight = 135;
constexpr uint8_t kGameListRows = 6;
constexpr uint8_t kGameListTop = 34;
constexpr uint8_t kGameListRowHeight = 12;
constexpr uint16_t kGameListWidth = kScreenWidth - 16;
constexpr float kGameListTextScale = 10.0f / 8.0f;
constexpr float kLoadTextScale = 10.0f / 8.0f;
constexpr float kLoadHeadingScale = 18.0f / 16.0f;
constexpr float kBootTitleTextScale = 19.0f / 16.0f;

bool gReady = false;
bool gRomsLoaded = false;
bool gGameMenuRequested = false;
bool gAmsdosAvailable = false;
std::string gGameFolder=cpc_files::kRoot;
struct SdGameStorage final : cpc_files::Storage {
  bool scan(const char *path,cpc_files::Visitor visitor,void *context) override {
    File dir=SD.open(path);
    if(!dir || !dir.isDirectory()) return false;
    for(;;) {
      File item=dir.openNextFile();if(!item) break;
      const String name=item.name();const bool directory=item.isDirectory();
      item.close();visitor(context,name.c_str(),directory);
    }
    dir.close();return true;
  }
  bool exists(const char *path) override {return SD.exists(path);}
  bool directory(const char *path) override {File file=SD.open(path);return file && file.isDirectory();}
  bool mkdir(const char *path) override {return SD.mkdir(path);}
  bool move(const char *from,const char *to) override {return !SD.exists(to) && SD.rename(from,to);}
} gGameStorage;
String gAutoKeys;
unsigned gAutoFrame=0;
unsigned gAutoDelay=0;
bool gTabWasDown=false;
bool gFast=false,gTapeSound=true,gFastKey=false;
bool gOriginalLoad=true,gForceBorder=false,gBorderKey=false;
unsigned gFrameCount=0;
uint8_t gVolume=cpcDefaultVolume; // 30% on the speaker's 0..255 scale
uint32_t gVolumeAt=0,gVolumeUntil=0;
int gVolumeDirection=0;
// One immutable 8-bit snapshot costs 32 KiB. Skip refreshing while SPI is busy.
uint8_t gLcdFrame[kImageHeight][kImageWidth];
std::atomic<bool> gLcdBusy{false};
std::atomic<uint32_t> gLcdFrames{0};
TaskHandle_t gLcdTask=nullptr;
SemaphoreHandle_t gLcdMutex=nullptr;
uint8_t gLcdVolume=0;
uint32_t gLcdVolumeUntil=0;
uint32_t gAudioLastUs=0,gAudioMaxGapUs=0,gAudioLate=0,gAudioPackets=0;
struct DisplayLock {
  DisplayLock() {if(gLcdMutex) xSemaphoreTake(gLcdMutex,portMAX_DELAY);}
  ~DisplayLock() {if(gLcdMutex) xSemaphoreGive(gLcdMutex);}
};

void queueCommand(const String &text) {gAutoKeys=text;gAutoFrame=0;gAutoDelay=150;}
bool showCpcBorder() {
  return gForceBorder || cpcTapeBorderVisible(gOriginalLoad,gFast,tape::loaded(),tape::active(),
                                            tape::position(),tape::fileSize());
}
uint8_t gOsRom[16384];
uint8_t gBasicRom[16384];
uint8_t gAmsdosRom[16384];

uint16_t cpcColour(uint8_t c) {
  const uint8_t r = cpcColourComponent(c,0);
  const uint8_t g = cpcColourComponent(c,2);
  const uint8_t b = cpcColourComponent(c,4);
  return M5.Display.color565(r, g, b);
}

void drawLcdSnapshot() {
  DisplayLock lock;
  M5.Display.startWrite();
  for(unsigned y=0;y<kImageHeight;++y) {
    uint16_t line[kImageWidth];
    for(unsigned x=0;x<kImageWidth;++x) line[x]=cpcColour(gLcdFrame[y][x]);
    M5.Display.pushImage(0,y,kImageWidth,1,line);
  }
  M5.Display.endWrite();
  if(millis()<gLcdVolumeUntil) {
    M5.Display.fillRect(153,122,87,13,TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);
    M5.Display.setTextSize(kLoadTextScale);
    M5.Display.drawString(String("Vol ")+String(unsigned(gLcdVolume)*100/255)+"%",160,124,1);
    M5.Display.setTextSize(1);
  }
  ++gLcdFrames;
}

void lcdTask(void *) {
  for(;;) {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    if(!gLcdBusy.load(std::memory_order_acquire)) continue;
    drawLcdSnapshot();
    gLcdBusy.store(false,std::memory_order_release);
  }
}

void bootStatus(const char *message, const char *detail=nullptr, bool error=false) {
  Serial.printf("CPC BOOT %s%s%s\n",message,detail?" ":"",detail?detail:"");
  M5.Display.fillRect(0,34,kScreenWidth,kScreenHeight-34,TFT_BLACK);
  M5.Display.setTextSize(kLoadTextScale);
  M5.Display.setTextColor(error?TFT_RED:TFT_WHITE,TFT_BLACK);
  M5.Display.drawString(message,12,45,1);
  if(detail) M5.Display.drawString(detail,12,65,1);
  M5.Display.setTextSize(1);
}

bool readExact(const char *path, uint8_t *target) {
  Serial.printf("CPC BOOT open %s heap=%u max=%u\n",path,ESP.getFreeHeap(),ESP.getMaxAllocHeap());
  File file = SD.open(path, FILE_READ);
  if (!file || file.size() != 16384) {
    Serial.printf("CPC BOOT ROM missing or wrong size: %s size=%u (expected 16384)\n",path,file ? unsigned(file.size()) : 0);
    return false;
  }
  const bool ok = file.read(target, 16384) == 16384;
  file.close();
  Serial.printf("CPC BOOT read %s %s\n",path,ok?"OK":"ERROR");
  return ok;
}

void setKey(uint8_t line, uint8_t bit, bool down) {
  if (down) cpckeys[line][bit] = 0;
  else cpckeys[line][bit] = 1;
}

uint16_t word(const uint8_t *data, size_t offset) {
  return data[offset] | (static_cast<uint16_t>(data[offset + 1]) << 8);
}

bool isSnaName(const String &name) {
  return cpc_files::kindOf(name.c_str())==cpc_files::Kind::sna;
}

bool isDskName(const String &name) {
  return cpc_files::kindOf(name.c_str())==cpc_files::Kind::dsk;
}

bool isCdtName(const String &name) {
  return cpc_files::kindOf(name.c_str())==cpc_files::Kind::cdt;
}

String gameListLabel(const String &path,const lgfx::IFont *font=&fonts::Font0) {
  const String name = path.substring(path.lastIndexOf('/') + 1);
  if (M5.Display.textWidth(name, font) <= kGameListWidth) return name;
  // Keep the file type visible and remove complete UTF-8 characters only.
  const int dot = name.lastIndexOf('.');
  const String suffix = dot > 0 ? String("...") + name.substring(dot) : String("...");
  String stem = dot > 0 ? name.substring(0, dot) : name;
  while (stem.length() && M5.Display.textWidth(stem + suffix, font) > kGameListWidth) {
    unsigned end = stem.length() - 1;
    while (end && (static_cast<uint8_t>(stem[end]) & 0xC0) == 0x80) --end;
    stem.remove(end);
  }
  return stem + suffix;
}

void printGameFolder(const std::string &folder,const cpc_files::Listing &listing) {
  Serial.printf("GAMES %s\n",folder.c_str());
  for(std::size_t i=0;i<listing.count;++i)
    Serial.printf("%s%s\n",listing.entries[i].name.c_str(),listing.entries[i].directory?"/":"");
  if(listing.more) Serial.println("MORE (64 entries)");
  Serial.println("END");
}

struct VolumeInput {int direction;bool only;};
VolumeInput volumeInput() {
  const auto &keys=M5Cardputer.Keyboard.keysState();
  bool up=false,down=false,regular=false;
  for(char ch:keys.word) {
    const int key=cpcVolumeKey(ch,keys.fn);
    up|=key>0;down|=key<0;regular|=!key;
  }
  // The M5 library leaves word empty on the Fn layer.
  if(keys.fn) {
    up|=M5Cardputer.Keyboard.isKeyPressed('=') || M5Cardputer.Keyboard.isKeyPressed('+');
    down|=M5Cardputer.Keyboard.isKeyPressed('-') || M5Cardputer.Keyboard.isKeyPressed('_');
  }
  return {int(up)-int(down),(up || down) && !regular};
}
bool changeVolume(int direction) {
  const auto previous=gVolume;
  gVolume=cpcVolumeStep(gVolume,direction);
  M5.Speaker.setVolume(gVolume);gVolumeUntil=millis()+1200;
  return previous!=gVolume;
}
bool updateVolume(const VolumeInput &input) {
  if(!input.direction) {gVolumeDirection=0;return false;}
  const uint32_t now=millis();
  if(input.direction==gVolumeDirection && now-gVolumeAt<140) return false;
  gVolumeDirection=input.direction;gVolumeAt=now;
  return changeVolume(input.direction);
}

bool loadSna(const String &path) {
  File file = SD.open(path, FILE_READ);
  if (!file || file.size() < 0x10100) return false;
  uint8_t header[0x100];
  if (file.read(header, sizeof(header)) != sizeof(header) ||
      memcmp(header, "MV - SNA", 8) != 0 ||
      word(header,0x6B) != 64 ||
      file.read(ram, 0x10000) != 0x10000) {
    file.close();
    return false;
  }
  file.close();

  af.b.l = header[0x11]; af.b.h = header[0x12];
  bc.b.l = header[0x13]; bc.b.h = header[0x14];
  de.b.l = header[0x15]; de.b.h = header[0x16];
  hl.b.l = header[0x17]; hl.b.h = header[0x18];
  ir.b.l = header[0x19]; ir.b.h = header[0x1A];
  iff1 = header[0x1B]; iff2 = header[0x1C];
  ix.w = word(header, 0x1D); iy.w = word(header, 0x1F);
  sp = word(header, 0x21); pc = word(header, 0x23); im = header[0x25];
  saf.b.l = header[0x26]; saf.b.h = header[0x27];
  sbc.b.l = header[0x28]; sbc.b.h = header[0x29];
  sde.b.l = header[0x2A]; sde.b.h = header[0x2B];
  shl.b.l = header[0x2C]; shl.b.h = header[0x2D];
  for (uint8_t i = 0; i < 16; ++i) gapal[i] = (header[0x2F + i]&31)+1;
  gaborder = (header[0x3F]&31)+1;
  scrmode = header[0x40] & 3;
  ramconfig = 0; // CPC 464 has no expansion RAM banking.
  crtcreg = header[0x42];
  memcpy(crtcregs, header + 0x43, 18);
  curhrom = header[0x55]&15;
  psgreg = header[0x5A]&15;
  memcpy(psgregs, header + 0x5B, 16);
  loromena = !(header[0x40]&4);
  hiromena = !(header[0x40]&8);
  ramconfigBefore = 0xFF;
  curhromBefore = 0xFF;
  updaterambanking();
  remakelookup();
  return true;
}
}

namespace cpc_platform {
void begin() {
  gReady = false;
  auto config = M5.config();
  config.clear_display = true;
  M5Cardputer.begin(config, true);
  M5.Speaker.end();
  auto speaker=M5.Speaker.config();
  speaker.task_priority=4;
  speaker.task_pinned_core=0;
  M5.Speaker.config(speaker);
  M5.Speaker.setVolume(gVolume);
  M5.Speaker.setChannelVolume(0,255);
  M5.Display.setRotation(1);
  // Los píxeles RGB565 de pushImage están en orden nativo little-endian.
  // El ST7789 del Cardputer necesita el intercambio para conservar la paleta
  // CPC: fondo azul y tinta amarilla del BASIC original.
  M5.Display.setSwapBytes(true);
  M5.Display.setBrightness(180);
  M5.Display.setTextSize(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.setTextSize(kBootTitleTextScale);
  M5.Display.drawString("Amstrad CPC 464", 12, 10, 2);
  M5.Display.setTextSize(1);
  bootStatus("Montando tarjeta SD...");
  Serial.printf("CPC BOOT SPI.begin heap=%u max=%u\n",ESP.getFreeHeap(),ESP.getMaxAllocHeap());
  SPI.begin(kSdSck, kSdMiso, kSdMosi, kSdCs);
  Serial.println("CPC BOOT SD.begin");
  if (!SD.begin(kSdCs, SPI, 25000000)) {
    bootStatus("SD no disponible","Comprueba la tarjeta FAT32",true);
    return;
  }
  Serial.println("CPC BOOT SD mounted");
  if (!loadRoms()) return;
  bootStatus("Iniciando CPC 464...");
  gLcdMutex=xSemaphoreCreateMutex();
  if(gLcdMutex) xTaskCreatePinnedToCore(lcdTask,"cpc_lcd",4096,nullptr,1,&gLcdTask,0);
  if(!gLcdTask) Serial.println("CPC BOOT LCD task unavailable; using synchronous display");
  gReady = true;
  Serial.printf("CPC BOOT platform ready=%u heap=%u max=%u\n",gReady,ESP.getFreeHeap(),ESP.getMaxAllocHeap());
}

bool loadRoms() {
  if (!gRomsLoaded) {
    bootStatus("Leyendo OS.ROM...");
    if (!readExact("/CPC464/OS.ROM", gOsRom)) {
      bootStatus("ROM ausente o invalida","/CPC464/OS.ROM: 16384 bytes",true);
      return false;
    }
    bootStatus("Leyendo BASIC.ROM...");
    if (!readExact("/CPC464/BASIC.ROM", gBasicRom)) {
      bootStatus("ROM ausente o invalida","/CPC464/BASIC.ROM: 16384 bytes",true);
      return false;
    }
    bootStatus("Leyendo AMSDOS.ROM...");
    gAmsdosAvailable=readExact("/CPC464/AMSDOS.ROM",gAmsdosRom);
    gRomsLoaded = true;
  }
  lorom = gOsRom;
  hirom[0] = gBasicRom;
  for (uint8_t i = 1; i < 16; ++i) hirom[i] = nullptr;
  if(gAmsdosAvailable) hirom[7] = gAmsdosRom;
  return true;
}

bool ready() { return gReady; }

void pollKeyboard() {
  ++gFrameCount;
  M5Cardputer.update();
  while(Serial.available()) {
    char ch=Serial.read();
    if(ch=='?') Serial.printf("CPC-ADV FIX9 pc=%04x mode=%u heap=%u psram=%u tape=%u frames=%u ms=%u tape_bytes=%u/%u fast=%u audio_packets=%u late=%u max_gap_us=%u lcd=%u volume=%u ay=%u,%u,%u error=%s\n",
      pc,scrmode,ESP.getFreeHeap(),ESP.getPsramSize(),tape::active(),gFrameCount,millis(),tape::position(),tape::fileSize(),gFast,gAudioPackets,gAudioLate,gAudioMaxGapUs,gLcdFrames.load(),unsigned(M5.Speaker.getVolume())*100/255,psgregs[8],psgregs[9],psgregs[10],tape::mountError());
    if(!gReady) continue;
    if(ch=='+' || ch=='-') changeVolume(ch=='+'?1:-1);
    if(ch=='m') gGameMenuRequested=true;
    if(ch=='o') {
      M5.Speaker.stop();gAudioLastUs=0;
      const auto result=cpc_files::organize(gGameStorage);
      Serial.printf("ORGANIZE ready=%u cdt=%u dsk=%u skipped=%u failed=%u\n",
                    result.ready,result.cdt,result.dsk,result.skipped,result.failed);
    }
    if(ch=='c') {gAudioLastUs=gAudioMaxGapUs=gAudioLate=gAudioPackets=0;}
    if(ch=='f') {gFast=!gFast;M5.Speaker.stop();}
    if(ch=='s') {
      Serial.print("CPCFRAME 320 200\n");
      for(unsigned y=0;y<200;++y) Serial.write(gb_buffer_vga[y],320);
      Serial.flush();
    }
    if(ch=='v') {
      Serial.print("CPCFRAME 240 135\n");
      const bool border=showCpcBorder();
      const auto active=cpcActiveViewport(crtcregs[1],crtcregs[6],unsigned(crtcregs[9])+1);
      for(unsigned y=0;y<135;++y) for(unsigned x=0;x<240;++x) Serial.write(cpcDisplayPixel(gb_buffer_vga,cpcBorderLines,x,y,border,active));
      Serial.flush();
    }
    if(ch=='l') {
      const auto listing=cpc_files::list(gGameStorage,gGameFolder);
      printGameFolder(gGameFolder,listing);
    }
    if(ch=='d' || ch=='t') {
      const String selected=cpc_files::findMedia(gGameStorage,ch=='d'?cpc_files::Kind::dsk:cpc_files::Kind::cdt).c_str();
      bool ok=selected.length() && (ch=='d'?mountDsk(selected.c_str()):tape::mountCdt(selected.c_str()));
      Serial.printf("MEDIA %s %s\n",selected.c_str(),ok?"OK":ch=='d'?dskMountError():tape::mountError());
      if(ok) {if(ch=='d') tape::eject();resetz80();resetcrtc();queueCommand(ch=='d'?String("|disc\n")+dskBootCommand():String("|tape\nrun\"\n "));}
    }
    if(ch=='3') {gAutoKeys="3";gAutoDelay=0;gAutoFrame=0;}
  }
  const auto &keys=M5Cardputer.Keyboard.keysState();
  const auto volume=volumeInput();
  updateVolume(volume);
  bool fastKey=keys.fn && M5Cardputer.Keyboard.isKeyPressed('f');
  if(fastKey && !gFastKey) {gFast=!gFast;M5.Speaker.stop();}
  gFastKey=fastKey;
  bool borderKey=keys.fn && M5Cardputer.Keyboard.isKeyPressed('b');
  if(borderKey && !gBorderKey) gForceBorder=!gForceBorder;
  gBorderKey=borderKey;
  if(M5.BtnA.wasPressed() || (keys.tab && !gTabWasDown)) gGameMenuRequested=true;
  gTabWasDown=keys.tab;
  memset(cpckeys,1,sizeof(cpckeys));
  if(gAutoKeys.length()) {
    if(gAutoDelay) {--gAutoDelay;return;}
    unsigned at=gAutoFrame/10;
    if(at>=gAutoKeys.length()) {gAutoKeys="";return;}
    if(gAutoFrame%10<5) {
      uint8_t row,bit;bool shift;
      if(cpcAsciiKey(gAutoKeys[at],row,bit,shift)) {setKey(row,bit,true);if(shift) setKey(2,5,true);}
    }
    ++gAutoFrame;return;
  }
  for(char ch:keys.word) {
    if(cpcVolumeKey(ch,keys.fn)) continue;
    uint8_t row,bit;bool shift;
    // Fn + the printed arrow keys controls both CPC cursors and joystick 0.
    if(keys.fn && (ch==';' || ch==',' || ch=='.' || ch=='/')) {
      unsigned direction=ch==';'?0:ch==','?2:ch=='.'?1:3;
      setKey(direction==1?1:0,direction==1?0:direction,true);
      setKey(9,ch==';'?0:ch==','?1:ch=='.'?2:3,true);
    } else if(cpcAsciiKey(ch,row,bit,shift)) {
      setKey(row,bit,true);if(shift) setKey(2,5,true);
    }
  }
  if(keys.enter) setKey(2,2,true);
  if(keys.del || keys.backspace) setKey(9,7,true);
  if(keys.esc) setKey(8,2,true);
  if(keys.up) {setKey(0,0,true);setKey(9,0,true);}
  if(keys.down) {setKey(0,2,true);setKey(9,1,true);}
  if(keys.left) {setKey(1,0,true);setKey(9,2,true);}
  if(keys.right) {setKey(0,1,true);setKey(9,3,true);}
  if(keys.shift && !volume.only) setKey(2,5,true);
  if(keys.ctrl && !volume.only) setKey(2,7,true);
  const bool functionKeys[9]={keys.f1,keys.f2,keys.f3,keys.f4,keys.f5,keys.f6,keys.f7,keys.f8,keys.f9};
  const uint8_t functionRows[9]={1,1,0,2,1,0,1,1,0};
  const uint8_t functionBits[9]={5,6,5,4,4,4,2,3,3};
  for(unsigned i=0;i<9;++i) if(functionKeys[i]) setKey(functionRows[i],functionBits[i],true);
  if(keys.fn && keys.space) setKey(9,4,true);
}

bool openGameMenu() {
  if (!gGameMenuRequested) return false;
  DisplayLock lock;
  gGameMenuRequested = false;
  gAutoKeys="";
  M5.Speaker.stop();
  gAudioLastUs=0;
  std::unique_ptr<cpc_files::Listing> storage(new(std::nothrow) cpc_files::Listing);
  if(!storage) {
    M5.Display.fillScreen(TFT_BLACK);M5.Display.setTextSize(1);M5.Display.setTextColor(TFT_RED,TFT_BLACK);
    M5.Display.drawString("Sin memoria para la lista",8,45,1);delay(1200);return true;
  }
  auto &games=*storage;
  cpc_files::list(gGameStorage,gGameFolder,games);
  if(!games.ok && gGameFolder!=cpc_files::kRoot) {
    gGameFolder=cpc_files::kRoot;cpc_files::list(gGameStorage,gGameFolder,games);
  }
  std::size_t selected=0;
  const char *folderError=nullptr;
  auto changeFolder=[&](const std::string &path) {
    if(!gGameStorage.directory(path.c_str())) {folderError="No se puede abrir carpeta";return false;}
    // Release old filenames before reading the next directory on a small heap.
    if(!cpc_files::list(gGameStorage,path,games)) {
      cpc_files::list(gGameStorage,gGameFolder,games);selected=0;
      folderError="No se puede abrir carpeta";return false;
    }
    gGameFolder=path;selected=0;folderError=nullptr;return true;
  };
  bool previous = false;
  bool next = false;
  bool enter = false;
  bool back = false;
  bool redraw = true;
  bool soundKey=false;
  while (true) {
    M5Cardputer.update();
    if(updateVolume(volumeInput())) redraw=true;
    const bool hasParent=gGameFolder!=cpc_files::kRoot;
    const std::size_t count=games.count+(hasParent?1:0);
    if (redraw) {
      M5.Display.fillScreen(TFT_BLACK);
      M5.Display.setTextColor(TFT_YELLOW);
      M5.Display.setTextSize(1);
      const String title=hasParent?String("Juegos - ")+cpc_files::baseName(gGameFolder).c_str():String("Juegos CPC - FIX9");
      M5.Display.drawString(gameListLabel(title,&fonts::Font2), 8, 4, 2);
      M5.Display.setTextSize(kLoadTextScale);
      M5.Display.drawString(gOriginalLoad?"O:original ON  R:rapida":"O:original  R:rapida ON",8,22,1);
      M5.Display.setTextColor(TFT_WHITE);
      if (count) {
        // Original 8-pixel font, enlarged by just 2 pixels; keep six rows.
        M5.Display.setTextSize(kGameListTextScale);
        const std::size_t first = selected >= kGameListRows ? selected - kGameListRows + 1 : 0;
        for (auto i = first; i < count && i < first + kGameListRows; ++i) {
          String label;
          if(hasParent && !i) label="[ .. ] Volver";
          else {
            const auto &entry=games.entries[i-(hasParent?1:0)];
            label=gameListLabel(entry.directory?String("[ ")+entry.name.c_str()+" ]":String(entry.name.c_str()));
          }
          const uint16_t y = kGameListTop + (i - first) * kGameListRowHeight;
          if (i == selected) M5.Display.fillRect(8, y, kGameListWidth, kGameListRowHeight, TFT_YELLOW);
          M5.Display.setTextColor(i == selected ? TFT_BLACK : TFT_WHITE,
                                  i == selected ? TFT_YELLOW : TFT_BLACK);
          M5.Display.drawString(label, 8, y, 1);
        }
        M5.Display.setTextSize(1);
      }
      M5.Display.setTextSize(kLoadTextScale);
      if(!games.count) {
        M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);
        M5.Display.drawString(games.ok?"Carpeta vacia":"No se puede leer SD",8,kGameListTop+(hasParent?18:0),1);
      }
      M5.Display.setTextColor(TFT_DARKGREY);
      M5.Display.drawString(String(gTapeSound?"S:ON ":"S:OFF ")+"Vol:"+String(unsigned(gVolume)*100/255)+"%",8,108,1);
      if(hasParent) M5.Display.drawString("Del:volver",150,108,1);
      if(folderError) {
        M5.Display.fillRect(0,107,kScreenWidth,13,TFT_BLACK);
        M5.Display.setTextColor(TFT_RED,TFT_BLACK);
        M5.Display.drawString(folderError,8,108,1);
      }
      M5.Display.drawString(", . mover Enter abrir ` salir", 3, 122, 1);
      M5.Display.setTextSize(1);
      redraw = false;
    }

    const auto &state = M5Cardputer.Keyboard.keysState();
    bool nowPrevious = state.up, nowNext = state.down;
    bool nowEnter=state.enter,nowBack=state.backspace || state.del;
    bool nowSound=false;
    while(Serial.available()) {
      const char command=Serial.read();
      if(command=='+' || command=='-') {changeVolume(command=='+'?1:-1);redraw=true;}
      if(command=='l') printGameFolder(gGameFolder,games);
      if(command=='?') Serial.printf("MENU %s selected=%u entries=%u volume=%u\n",gGameFolder.c_str(),unsigned(selected),unsigned(count),unsigned(gVolume)*100/255);
      if(command=='j') nowNext=true;
      if(command=='k') nowPrevious=true;
      if(command=='e') nowEnter=true;
      if(command=='b') nowBack=true;
      if(command=='q') return true;
    }
    for (char key : state.word) {
      nowSound |= key=='s' || key=='S';
      if(key=='o' || key=='O') {gOriginalLoad=true;gFast=false;gTapeSound=true;cpc_audio::setTapeSound(true);redraw=true;}
      if(key=='r' || key=='R') {gOriginalLoad=false;gFast=true;redraw=true;}
      nowPrevious |= key == ';' || key == ':' || key == ',' || key == '<';
      nowNext |= key == '.' || key == '>' || key == '/';
      if (key == '`' || key == '~') return true;
    }
    if(nowSound && !soundKey) {gTapeSound=!gTapeSound;cpc_audio::setTapeSound(gTapeSound);redraw=true;}
    soundKey=nowSound;
    if(nowBack && !back && hasParent) {
      changeFolder(cpc_files::parent(gGameFolder));
      redraw=true;back=true;enter=nowEnter;previous=nowPrevious;next=nowNext;
      continue;
    }
    if (count && nowPrevious && !previous) { selected = (selected + count - 1) % count; redraw = true; }
    if (count && nowNext && !next) { selected = (selected + 1) % count; redraw = true; }
    if (count && nowEnter && !enter) {
      if(hasParent && !selected) {
        changeFolder(cpc_files::parent(gGameFolder));redraw=true;enter=true;continue;
      }
      const auto &entry=games.entries[selected-(hasParent?1:0)];
      const auto path=cpc_files::child(gGameFolder,entry.name);
      if(entry.directory) {
        changeFolder(path);redraw=true;enter=true;continue;
      }
      const String selectedFile=path.c_str();
      if(isDskName(selectedFile) && !gAmsdosAvailable) {
        M5.Display.setTextColor(TFT_RED,TFT_BLACK);
        M5.Display.setTextSize(kLoadTextScale);
        M5.Display.drawString("Falta /CPC464/AMSDOS.ROM",8,108,1);
        M5.Display.setTextSize(1);
        delay(1800);enter=true;redraw=true;continue;
      }
      M5.Display.fillScreen(TFT_BLACK);
      M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);
      M5.Display.setTextSize(kLoadHeadingScale);
      M5.Display.drawString("Montando imagen...",8,40,2);
      M5.Display.setTextSize(1);
      const bool loaded = isSnaName(selectedFile) ? loadSna(selectedFile) :
                          isDskName(selectedFile) ? mountDsk(selectedFile.c_str()) :
                          tape::mountCdt(selectedFile.c_str());
      if (!loaded) {
        M5.Display.setTextColor(TFT_RED);
        M5.Display.setTextSize(kLoadTextScale);
        M5.Display.drawString(isSnaName(selectedFile) ? "SNA invalido" :
                              isDskName(selectedFile) ? dskMountError() : tape::mountError(),
                              8, 104, 1);
        M5.Display.setTextSize(1);
        delay(1800);
        enter=true;
        redraw = true;
        continue;
      }
      if (isDskName(selectedFile) || isCdtName(selectedFile)) {
        if(isDskName(selectedFile)) tape::eject();
        resetz80();
        resetcrtc();
        if(isDskName(selectedFile)) queueCommand(String("|disc\n") + dskBootCommand());
        else queueCommand("|tape\nrun\"\n ");
      }
      return false;
    }
    previous = nowPrevious;
    next = nowNext;
    enter = nowEnter;
    back = nowBack;
    delay(20);
  }
}

void present() {
  if(gLcdBusy.load(std::memory_order_acquire)) return;
  const bool border=showCpcBorder();
  const auto active=cpcActiveViewport(crtcregs[1],crtcregs[6],unsigned(crtcregs[9])+1);
  for(unsigned y=0;y<kImageHeight;++y) for(unsigned x=0;x<kImageWidth;++x)
    gLcdFrame[y][x]=cpcDisplayPixel(gb_buffer_vga,cpcBorderLines,x,y,border,active);
  gLcdVolume=gVolume;gLcdVolumeUntil=gVolumeUntil;
  if(gLcdTask) {
    gLcdBusy.store(true,std::memory_order_release);
    xTaskNotifyGive(gLcdTask);
  } else drawLcdSnapshot();
}

bool shouldRender() {
  static uint32_t last=0;
  cpc_audio::setEnabled(!gFast);
  uint32_t now=millis();
  if(gLcdBusy.load(std::memory_order_acquire) || now-last<(gFast?250u:40u)) return false;
  last=now;return true;
}

void audioTick() {
  if(gFast) {gAudioLastUs=0;return;}
  if(cpc_audio::size()) {
    M5.Speaker.playRaw(cpc_audio::data(),cpc_audio::size(),16000,false,1,0,false);
    const uint32_t now=micros();
    if(gAudioLastUs) {
      const uint32_t gap=now-gAudioLastUs;
      if(gap>gAudioMaxGapUs) gAudioMaxGapUs=gap;
      if(gap>26000) ++gAudioLate;
    }
    gAudioLastUs=now;++gAudioPackets;
  }
}

void resetRequested() { resetz80(); resetcrtc(); }
}  // namespace cpc_platform
