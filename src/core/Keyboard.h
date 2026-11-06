#pragma once
#include <stdint.h>
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
