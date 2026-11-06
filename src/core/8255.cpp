#include "gbConfig.h"
#include "CPCem.h"
#include "PSG.h"
#include "8255.h"
#include "gbGlobals.h"
#include "Tape.h"

//JJ int crtcline,sc,vc;
struct
{
        unsigned char portc;
        unsigned char ctrl;
} PIA;

void write8255(unsigned short a, unsigned char v)
{
        switch (a&0xFF00)
        {
                case 0xF400: psgdat=v; updatepsg(); return;
                case 0xF500: return;
                case 0xF600: PIA.portc=v; psgmode=v>>6; tape::setMotor(v&0x10); updatekeys(v&15); updatepsg(); return;
                case 0xF700:
                  if (v&0x80) {PIA.ctrl=v;PIA.portc=0;}
                  else {unsigned mask=1<<((v>>1)&7);if(v&1) PIA.portc|=mask;else PIA.portc&=~mask;}
                  psgmode=PIA.portc>>6;tape::setMotor(PIA.portc&0x10);updatekeys(PIA.portc&15);updatepsg();return;
        }
}

unsigned char read8255(unsigned short a)
{
        switch (a&0xFF00)
        {
                case 0xF400: updatekeys(PIA.portc&15); if(psgmode==1) return psgregs[psgreg&15]; return psgdat;
                case 0xF500: return 0x3E | crtcvsync | (tape::level() ? 0x80 : 0);
                case 0xF600: return PIA.portc;
                case 0xF700: return PIA.ctrl;
        }
        return 0xFF;
}
