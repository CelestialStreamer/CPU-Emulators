#pragma once

#include <cstdint>
#include "State8086.h"

template<typename T>
void setFlags(T value, State8086::StatusRegister& sr) {
   // Lookup table for parity of a byte
   static const int parity[256] = {
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
   };
   const T signBit = ~((T)(-1) >> 1); // 0x80..0

   sr.Z = value == 0 ? 1 : 0;
   sr.S = value & signBit ? 1 : 0;
   sr.P = parity[value & 0xff]; // Calculate parity of LSByte
}

template<typename T>
void setArithemticFlags(T op1, T op2, unsigned int res, State8086::StatusRegister& sr) {
   const T signBit = ~((T)(-1) >> 1); // 0x80..0
   const unsigned int overflow = ((int)(-1) - (T)(-1)); // 0xFF..FF00..00 Should be as many zeros as bits in T

   setFlags((T)res, sr);
   sr.O = (res ^ op1) & (res ^ op2) & signBit ? 1 : 0;
   sr.C = (res & overflow) ? 1 : 0;
   sr.A = ((op1 ^ op2 ^ res) & 0x10) ? 1 : 0;
}

template<typename T>
void setLogicFlags(T value, State8086::StatusRegister& sr) {
   setFlags(value, sr);
   sr.C = 0;
   sr.O = 0;
}

template<typename T> inline uint8_t MSB(T x) { return ~((T)(-1) >> 1) & x ? 1 : 0; }
template<typename T> inline uint8_t LSB(T x) { return x & 1 ? 1 : 0; }

namespace ALU {

   template<typename T>
   T add(T op1, T op2, State8086::StatusRegister& sr) {
      unsigned int res = (unsigned int)(op1)+(unsigned int)(op2);
      setArithemticFlags(op1, op2, res, sr);
      return (T)res;
   }

   template<typename T>
   T _or(T op1, T op2, State8086::StatusRegister& sr) {
      T res = op1 | op2;
      setLogicFlags(res, sr);
      return res;
   }

   template<typename T>
   T adc(T op1, T op2, State8086::StatusRegister& sr) {
      unsigned int res = (unsigned int)op1 + (unsigned int)op2 + (unsigned int)sr.C;
      setArithemticFlags(op1, op2, res, sr);
      return (T)res;
   }

   template<typename T>
   T sbb(T op1, T op2, State8086::StatusRegister& sr) {
      unsigned int res = (unsigned int)op1 - (unsigned int)(op2 + sr.C);
      setArithemticFlags(op1, op2, res, sr);
      return (T)res;
   }

   template<typename T>
   T _and(T op1, T op2, State8086::StatusRegister& sr) {
      T res = op1 & op2;
      setLogicFlags(res, sr);
      return res;
   }

   template<typename T>
   T sub(T op1, T op2, State8086::StatusRegister& sr) {
      unsigned int res = (unsigned int)op1 - (unsigned int)op2;
      setArithemticFlags(op1, op2, res, sr);
      return (T)res;
   }

   template<typename T>
   T _xor(T op1, T op2, State8086::StatusRegister& sr) {
      T res = op1 ^ op2;
      setLogicFlags(res, sr);
      return res;
   }

   template<typename T>
   T arithm(int i, T op1, T op2, State8086::StatusRegister& sr) {
      switch (i) {
      case 0:  return add(op1, op2, sr);
      case 1:  return _or(op1, op2, sr);
      case 2:  return adc(op1, op2, sr);
      case 3:  return sbb(op1, op2, sr);
      case 4:  return _and(op1, op2, sr);
      case 5:  return sub(op1, op2, sr);
      case 6:  return _xor(op1, op2, sr);
      default: return sub(op1, op2, sr);
      }
   }

   bool condition(int test, State8086::StatusRegister sr) {
      switch (test) {
      case 0x0: return sr.O == 1;                  // overflow (OF=1) (pg 271)
      case 0x1: return sr.O == 0;                  // not overflow (OF=0) (pg 271)
      case 0x2: return sr.C == 1;                  // below/carry/not above or equal (CF=1) (pg 271)
      case 0x3: return sr.C == 0;                  // above or equal/not below/not carry (CF=0) (pg 271)
      case 0x4: return sr.Z == 1;                  // equal/zero (ZF=1) (pg 271)
      case 0x5: return sr.Z == 0;                  // not eqaul/not zero (ZF=0) (pg 271)
      case 0x6: return sr.C == 1 || sr.Z == 1;     // below or equal/not above (CF=1 or ZF = 1) (pg 271)
      case 0x7: return sr.C == 0 && sr.Z == 0;     // above/not below or equal (CF=0 and ZF=0) (pg 271)
      case 0x8: return sr.S == 1;                  // sign (SF=1) (pg 271)
      case 0x9: return sr.S == 0;                  // not sign (SF=0) (pg 271)
      case 0xA: return sr.P == 1;                  // parity/parity even (PF=1) (pg 271)
      case 0xB: return sr.P == 0;                  // not parity/parity odd (PF=0) (pg 271)
      case 0xC: return sr.S != sr.O;               // less/not greater or equal (SF<>OF) (pg 271)
      case 0xD: return sr.S == sr.O;               // greater or equal/not less (SF=OF) (pg 271)
      case 0xE: return sr.Z == 1 || sr.S != sr.O;  // less or equal/not greater (ZF=1 or SF<>OF) (pg 271)
      default:  return sr.Z == 0 && sr.S == sr.O;  // greater/not less or equal (ZF=0 and SF=OF) (pg 271)
      }
   }

   template<typename T>
   T ROL(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      for (cnt %= 8 * sizeof(dest); cnt > 0; --cnt) {
         sr.C = MSB(dest);
         dest = (dest << 1) | (sr.C);
      }
      sr.O = MSB(dest); // valid only if cnt=1
      return dest;
   }

   template<typename T>
   T ROR(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      const int size = 8 * sizeof(dest);

      for (cnt %= size; cnt > 0; --cnt) {
         sr.C = LSB(dest);
         dest = (dest >> 1) | (sr.C << size - 1);
      }
      sr.O = (dest >> size - 1) ^ ((dest >> (size - 2) & 1)); // Valid only if cnt=1
      return dest;
   }

   template<typename T>
   T RCL(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      const int size = 8 * sizeof(dest);

      int CF;
      for (cnt %= size + 1; cnt > 0; --cnt) {
         CF = MSB(dest);
         dest = (dest << 1) | CF;
         sr.C = CF;
      }
      sr.O = MSB(dest) ^ CF; // Valid only if cnt=1
      return dest;
   }

   template<typename T>
   T RCR(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      const int size = 8 * sizeof(dest);

      int CF;
      sr.O = MSB(dest) ^ sr.C; // Valid only if cnt=1
      for (cnt %= size + 1; cnt > 0; --cnt) {
         CF = LSB(dest);
         dest = (dest >> 1) | (CF << size - 1);
         sr.C = CF;
      }
      return dest;
   }

   template<typename T>
   T SHL(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         sr.C = MSB(dest);
         dest <<= 1;
      }
      sr.O = MSB(dest) ^ sr.C; // Valid only if cnt=1
      setFlags(dest, sr);
      return dest;
   }

   template<typename T>
   T SHR(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         sr.C = LSB(dest);
         dest >>= 1; // Unsigned divide by 2
      }
      sr.O = MSB(dest); // Valid only if cnt=1
      setFlags(dest, sr);
      return dest;
   }

   template<typename T>
   T SAR(T dest, uint8_t cnt, State8086::StatusRegister& sr) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         sr.C = LSB(dest);
         dest = (int)dest >> 1; // Signed divide by 2
      }
      sr.O = 0; // Valid only if cnt=1
      setFlags(dest, sr);
      return dest;
   }

   template<typename T>
   T rotate(int i, T op1, T cnt, State8086::StatusRegister& sr) {
      switch (i) {
      case 0: return ROL(op1, cnt, sr);
      case 1: return ROR(op1, cnt, sr);
      case 2: return RCL(op1, cnt, sr);
      case 3: return RCR(op1, cnt, sr);
      case 4: return SHL(op1, cnt, sr);
      case 5: return SHR(op1, cnt, sr);
      case 6: return (T)(-1);
      default: return SAR(op1, cnt, sr);
      }
   }

}
