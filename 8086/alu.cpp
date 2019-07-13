#include "alu.h"

void ALU::Flags::clear() {
   O = D = I = T = S = Z = A = P = C = 0;
}

bool ALU::condition(int test) {
   switch (test) {
   case 0x0: return flags.O == 1;                        // overflow (OF=1) (IA V2 p271)
   case 0x1: return flags.O == 0;                        // not overflow (OF=0) (IA V2 p271)
   case 0x2: return flags.C == 1;                        // below/carry/not above or equal (CF=1) (IA V2 p271)
   case 0x3: return flags.C == 0;                        // above or equal/not below/not carry (CF=0) (IA V2 p271)
   case 0x4: return flags.Z == 1;                        // equal/zero (ZF=1) (IA V2 p271)
   case 0x5: return flags.Z == 0;                        // not eqaul/not zero (ZF=0) (IA V2 p271)
   case 0x6: return flags.C == 1 || flags.Z == 1;        // below or equal/not above (CF=1 or ZF = 1) (IA V2 p271)
   case 0x7: return flags.C == 0 && flags.Z == 0;        // above/not below or equal (CF=0 and ZF=0) (IA V2 p271)
   case 0x8: return flags.S == 1;                        // sign (SF=1) (IA V2 p271)
   case 0x9: return flags.S == 0;                        // not sign (SF=0) (IA V2 p271)
   case 0xA: return flags.P == 1;                        // parity/parity even (PF=1) (IA V2 p271)
   case 0xB: return flags.P == 0;                        // not parity/parity odd (PF=0) (IA V2 p271)
   case 0xC: return flags.S != flags.O;                  // less/not greater or equal (SF<>OF) (IA V2 p271)
   case 0xD: return flags.S == flags.O;                  // greater or equal/not less (SF=OF) (IA V2 p271)
   case 0xE: return flags.Z == 1 || flags.S != flags.O;  // less or equal/not greater (ZF=1 or SF<>OF) (IA V2 p271)
   default:  return flags.Z == 0 && flags.S == flags.O;  // greater/not less or equal (ZF=0 and SF=OF) (IA V2 p271)
   }
}

template<> void ALU::Flags::set(uint8_t x) {
   C = x & (1 << 0x0) ? 1 : 0;
   P = x & (1 << 0x2) ? 1 : 0;
   A = x & (1 << 0x4) ? 1 : 0;
   Z = x & (1 << 0x6) ? 1 : 0;
   S = x & (1 << 0x7) ? 1 : 0;
}

template<> void ALU::Flags::set(uint16_t x) {
   C = x & (1 << 0x0) ? 1 : 0;
   P = x & (1 << 0x2) ? 1 : 0;
   A = x & (1 << 0x4) ? 1 : 0;
   Z = x & (1 << 0x6) ? 1 : 0;
   S = x & (1 << 0x7) ? 1 : 0;
   T = x & (1 << 0x8) ? 1 : 0;
   I = x & (1 << 0x9) ? 1 : 0;
   D = x & (1 << 0xA) ? 1 : 0;
   O = x & (1 << 0xB) ? 1 : 0;
}

template<> uint8_t ALU::Flags::get() {
   // bit 1 is always set in modern CPUs (IA V1 p51:Figure 3-7)
   // bit 1 is undefined in original (8086 Family p2-33:Figure 2-32)
   return (uint8_t)(
      2 // Use modern standard
      | (C << 0x0)
      | (P << 0x2)
      | (A << 0x4)
      | (Z << 0x6)
      | (S << 0x7));
}

template<> uint16_t ALU::Flags::get() {
   // bit 1 is always set in modern CPUs (IA V1 p51:Figure 3-7)
   // bit 1 is undefined in original (8086 Family p2-33:Figure 2-32)
   return (uint16_t)(
      2 // Use modern standard
      | (C << 0x0)
      | (P << 0x2)
      | (A << 0x4)
      | (Z << 0x6)
      | (S << 0x7)
      | (T << 0x8)
      | (I << 0x9)
      | (D << 0xA)
      | (O << 0xB));
}

template<>
void ALU::MUL(uint16_t& overflow, uint16_t& op1, uint16_t op2) {
   union { uint32_t full; struct { uint16_t high; uint16_t low; }; } temp;
   temp.full = (int)op1 * (int)op2;
   overflow = temp.high;
   op1 = temp.low;
   flags.C = flags.O = temp.high == 0 ? 0 : 1;
   //// S,Z,A,P flags are undefined
}

template<>
void ALU::MUL(uint8_t& overflow, uint8_t& op1, uint8_t op2) {
   union { uint16_t full; struct { uint8_t high; uint8_t low; }; } temp;
   temp.full = (int)op1 * (int)op2;
   overflow = temp.high;
   op1 = temp.low;
   flags.C = flags.O = temp.high == 0 ? 0 : 1;
   //// S,Z,A,P flags are undefined
}

template<>
void ALU::IMUL(uint16_t& overflow, uint16_t& op1, uint16_t op2) {
   union { uint32_t full; struct { uint16_t high; uint16_t low; }; } temp;
   temp.full = (int)op1 * (int)op2;
   overflow = temp.high;
   op1 = temp.low;
   flags.C = flags.O = temp.high == 0 || temp.high == 0xffff ? 0 : 1;
   // S,Z,A,P flags are undefined
}

template<>
void ALU::IMUL(uint8_t& overflow, uint8_t& op1, uint8_t op2) {
   union { uint16_t full; struct { uint8_t high; uint8_t low; }; } temp;
   temp.full = (int)op1 * (int)op2;
   overflow = temp.high;
   op1 = temp.low;
   flags.C = flags.O = temp.high == 0 || temp.high == 0xff ? 0 : 1;
   // S,Z,A,P flags are undefined
}

template<>
bool ALU::DIV(uint16_t& overflow, uint16_t& op1, uint16_t op2) {
   union { uint32_t full; struct { uint16_t high; uint16_t low; }; } temp;
   temp.high = overflow;
   temp.low = op1;
   if (op2 == 0) return false;
   temp.full /= op2;
   if (temp.full > 0xffff) return false;
   overflow = temp.full % op2;
   op1 = temp.low;
   // all flags are undefined. so fuck 'em
   return true;
}

template<>
bool ALU::DIV(uint8_t& overflow, uint8_t& op1, uint8_t op2) {
   union { uint16_t full; struct { uint8_t high; uint8_t low; }; } temp;
   temp.high = overflow;
   temp.low = op1;
   if (op2 == 0) return false;
   temp.full /= op2;
   if (temp.full > 0xff) return false;
   overflow = temp.full % op2;
   op1 = temp.low;
   // all flags are undefined. so fuck 'em
   return true;
}

template<>
bool ALU::IDIV(uint16_t& overflow, uint16_t& op1, uint16_t op2) {
   union { uint32_t full; struct { uint16_t high; uint16_t low; }; } temp;
   temp.high = overflow;
   temp.low = op1;
   if (op2 == 0) return false;
   temp.full /= op2;
   if (temp.full > 0xffff || temp.full < 0x8000) return false;
   overflow = temp.full % op2;
   op1 = temp.low;
   // all flags are undefined. so fuck 'em
   return true;
}

template<>
bool ALU::IDIV(uint8_t& overflow, uint8_t& op1, uint8_t op2) {
   union { uint16_t full; struct { uint8_t high; uint8_t low; }; } temp;
   temp.high = overflow;
   temp.low = op1;
   if (op2 == 0) return false;
   temp.full /= op2;
   if (temp.full > 0xff || temp.full < 0x80) return false;
   overflow = temp.full % op2;
   op1 = temp.low;
   // all flags are undefined. so fuck 'em
   return true;
}