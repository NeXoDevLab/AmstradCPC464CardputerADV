#pragma once
#include <stdint.h>

constexpr uint8_t cpcDefaultVolume=(30*255+99)/100;
struct CpcGameInput {bool up,down,left,right,fire;};
inline CpcGameInput cpcGameInput(bool direct,bool fn,bool up,bool down,bool left,bool right,bool fire) {
  const bool enabled=direct || fn;
  return {enabled && up,enabled && down,enabled && left,enabled && right,enabled && fire};
}
inline bool cpcGameArrow(char ch,bool direct,bool fn) {
  if(!direct && !fn) return false;
  return ch==';' || ch==':' || ch=='.' || ch=='>' ||
         ch==',' || ch=='<' || ch=='/' || ch=='?';
}
template<class Press> inline void cpcApplyGameInput(CpcGameInput input,Press press) {
  // CPC cursors and joystick 0 use different positions in the keyboard matrix.
  if(input.up) {press(0,0);press(9,0);}
  if(input.down) {press(0,2);press(9,1);}
  if(input.left) {press(1,0);press(9,2);}
  if(input.right) {press(0,1);press(9,3);}
  if(input.fire) press(9,4);
}
// Reserve only Fn shortcuts; unmodified symbols keep their CPC meaning.
inline int cpcVolumeKey(char ch,bool fn) {
  if(!fn) return 0;
  if(ch=='+' || ch=='=') return 1;
  return ch=='-' || ch=='_' ? -1 : 0;
}
inline uint8_t cpcVolumeStep(uint8_t volume,int direction) {
  if(!direction) return volume;
  int percent=int(volume)*100/255;
  percent+=direction>0 ? 5 : -5;
  if(percent<0) percent=0;
  if(percent>100) percent=100;
  // Round upwards so the displayed integer percentage stays exact.
  return static_cast<uint8_t>((percent*255+99)/100);
}
// CPC matrix: bits of AY port A, selected by PPI port C low nibble.
inline bool cpcAsciiKey(char ch, uint8_t &row, uint8_t &bit, bool &shift) {
  shift = ch >= 'A' && ch <= 'Z';
  if (shift) ch += 'a'-'A';
  if (ch=='\n' || ch=='\r') {row=2;bit=2;return true;}
  if (ch=='\b') {row=9;bit=7;return true;}
  if (ch=='"') {row=8;bit=1;shift=true;return true;}
  if (ch=='|') {row=3;bit=2;shift=true;return true;}
  // Explicit pairs avoid applying the Cardputer's US layout to CPC symbols.
  switch(ch) {
    case '!':ch='1';shift=true;break;case '#':ch='3';shift=true;break;
    case '$':ch='4';shift=true;break;case '%':ch='5';shift=true;break;
    case '&':ch='6';shift=true;break;case '\'':ch='7';shift=true;break;
    case '(':ch='8';shift=true;break;case ')':ch='9';shift=true;break;
    case '_':ch='0';shift=true;break;case '=':ch='-';shift=true;break;
    case '?':ch='/';shift=true;break;case '<':ch=',';shift=true;break;
    case '>':ch='.';shift=true;break;case '~':ch='^';shift=true;break;
    case '{':ch='[';shift=true;break;case '}':ch=']';shift=true;break;
    case '+':ch=';';shift=true;break;case '*':ch=':';shift=true;break;
    case '`':ch='\\';shift=true;break;
  }
  static const char keys[10][9] = {
    "????????", "????????", "?[?]??\\?", "^-@p;:/.",
    "09oilkm,", "87uyhjn ", "65rtgfbv", "43ewsdcx", "12?q?a?z", "????????"
  };
  for(row=2;row<9;++row) for(bit=0;bit<8;++bit)
    if(ch!='?' && ch!=' ' && keys[row][bit]==ch) return true;
  if(ch==' ') {row=5;bit=7;return true;}
  return false;
}
