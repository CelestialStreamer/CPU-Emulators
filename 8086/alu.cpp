#include "alu.h"

bool ALU::condition(int test, State8086::Flags flags) {
   switch (test) {
   case 0x0: return flags.O == 1;                        // overflow (OF=1) (pg 271)
   case 0x1: return flags.O == 0;                        // not overflow (OF=0) (pg 271)
   case 0x2: return flags.C == 1;                        // below/carry/not above or equal (CF=1) (pg 271)
   case 0x3: return flags.C == 0;                        // above or equal/not below/not carry (CF=0) (pg 271)
   case 0x4: return flags.Z == 1;                        // equal/zero (ZF=1) (pg 271)
   case 0x5: return flags.Z == 0;                        // not eqaul/not zero (ZF=0) (pg 271)
   case 0x6: return flags.C == 1 || flags.Z == 1;        // below or equal/not above (CF=1 or ZF = 1) (pg 271)
   case 0x7: return flags.C == 0 && flags.Z == 0;        // above/not below or equal (CF=0 and ZF=0) (pg 271)
   case 0x8: return flags.S == 1;                        // sign (SF=1) (pg 271)
   case 0x9: return flags.S == 0;                        // not sign (SF=0) (pg 271)
   case 0xA: return flags.P == 1;                        // parity/parity even (PF=1) (pg 271)
   case 0xB: return flags.P == 0;                        // not parity/parity odd (PF=0) (pg 271)
   case 0xC: return flags.S != flags.O;                  // less/not greater or equal (SF<>OF) (pg 271)
   case 0xD: return flags.S == flags.O;                  // greater or equal/not less (SF=OF) (pg 271)
   case 0xE: return flags.Z == 1 || flags.S != flags.O;  // less or equal/not greater (ZF=1 or SF<>OF) (pg 271)
   default:  return flags.Z == 0 && flags.S == flags.O;  // greater/not less or equal (ZF=0 and SF=OF) (pg 271)
   }
}