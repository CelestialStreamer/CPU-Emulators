#include "IO.h"

uint8_t IO::read(uint8_t port) {
   switch (port) {
   case 1: return *((uint8_t*)&Read1);
   case 2: return *((uint8_t*)&Read2);
   case 3: return ((((shift1 << 8) | shift0) >> (8 - shift_offset)) & 0xff);
   default: return 0;
   }
}

void IO::write(uint8_t port, uint8_t value) {
   switch (port) {
   case 2:
   {
      // Shift register result offset (bits 0,1,2)
      shift_offset = value & 0x7;
      break;
   }
   case 3: break; // Sound related, unemulated
   case 4:
   {
      // Fill shift register
      shift0 = shift1;
      shift1 = value;
      break;
   }
   case 5: break; // Sound related, unemulated
   case 6: break; // strange 'debug' port? eg. it writes to this port when it writes text to the screen(0 = a, 1 = b, 2 = c, etc)
   default: break;
   }
}