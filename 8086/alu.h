#pragma once

#include <cstdint>
#include "State8086.h"

template<typename T>
void setFlags(T value, State8086::Flags& flags) {
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

   flags.Z = value == 0 ? 1 : 0;
   flags.S = value & signBit ? 1 : 0;
   flags.P = parity[value & 0xff]; // Calculate parity of LSByte
}

template<typename T>
void setArithemticFlags(T op1, T op2, unsigned int res, State8086::Flags& flags) {
   const T signBit = ~((T)(-1) >> 1); // 0x80..0
   const unsigned int overflow = ((int)(-1) - (T)(-1)); // 0xFF..FF00..00 Should be as many zeros as bits in T

   setFlags((T)res, flags);
   flags.O = (res ^ op1) & (res ^ op2) & signBit ? 1 : 0;
   flags.C = (res & overflow) ? 1 : 0;
   flags.A = ((op1 ^ op2 ^ res) & 0x10) ? 1 : 0;
}

template<typename T>
void setLogicFlags(T value, State8086::Flags& flags) {
   setFlags(value, flags);
   flags.C = 0;
   flags.O = 0;
}

template<typename T> inline uint8_t MSB(T x) { return ~((T)(-1) >> 1) & x ? 1 : 0; }
template<typename T> inline uint8_t LSB(T x) { return x & 1 ? 1 : 0; }

namespace ALU {

   template<typename T>
   T add(T op1, T op2, State8086::Flags& flags) {
      unsigned int res = (unsigned int)(op1)+(unsigned int)(op2);
      setArithemticFlags(op1, op2, res, flags);
      return (T)res;
   }

   template<typename T>
   T _or(T op1, T op2, State8086::Flags& flags) {
      T res = op1 | op2;
      setLogicFlags(res, flags);
      return res;
   }

   template<typename T>
   T adc(T op1, T op2, State8086::Flags& flags) {
      unsigned int res = (unsigned int)op1 + (unsigned int)op2 + (unsigned int)flags.C;
      setArithemticFlags(op1, op2, res, flags);
      return (T)res;
   }

   template<typename T>
   T sbb(T op1, T op2, State8086::Flags& flags) {
      unsigned int res = (unsigned int)op1 - (unsigned int)(op2 + flags.C);
      setArithemticFlags(op1, op2, res, flags);
      return (T)res;
   }

   template<typename T>
   T _and(T op1, T op2, State8086::Flags& flags) {
      T res = op1 & op2;
      setLogicFlags(res, flags);
      return res;
   }

   template<typename T>
   T sub(T op1, T op2, State8086::Flags& flags) {
      unsigned int res = (unsigned int)op1 - (unsigned int)op2;
      setArithemticFlags(op1, op2, res, flags);
      return (T)res;
   }

   template<typename T>
   T _xor(T op1, T op2, State8086::Flags& flags) {
      T res = op1 ^ op2;
      setLogicFlags(res, flags);
      return res;
   }

   template<typename T>
   struct arithm {
      T(*operator[](int i))(T, T, State8086::Flags&) { return a[i]; }
   private:
      T(*a[2])(T, T, State8086::Flags&) = { add,_or,adc,sbb,_and,sub,_xor,sub };
   };

   bool condition(int test, State8086::Flags flags) {
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

   template<typename T>
   T ROL(T dest, uint8_t cnt, State8086::Flags& flags) {
      for (cnt %= 8 * sizeof(dest); cnt > 0; --cnt) {
         flags.C = MSB(dest);
         dest = (dest << 1) | (flags.C);
      }
      flags.O = MSB(dest); // valid only if cnt=1
      return dest;
   }

   template<typename T>
   T ROR(T dest, uint8_t cnt, State8086::Flags& flags) {
      const int size = 8 * sizeof(dest);

      for (cnt %= size; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest = (dest >> 1) | (flags.C << size - 1);
      }
      flags.O = (dest >> size - 1) ^ ((dest >> (size - 2) & 1)); // Valid only if cnt=1
      return dest;
   }

   template<typename T>
   T RCL(T dest, uint8_t cnt, State8086::Flags& flags) {
      const int size = 8 * sizeof(dest);

      int CF;
      for (cnt %= size + 1; cnt > 0; --cnt) {
         CF = MSB(dest);
         dest = (dest << 1) | CF;
         flags.C = CF;
      }
      flags.O = MSB(dest) ^ CF; // Valid only if cnt=1
      return dest;
   }

   template<typename T>
   T RCR(T dest, uint8_t cnt, State8086::Flags& flags) {
      const int size = 8 * sizeof(dest);

      int CF;
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      for (cnt %= size + 1; cnt > 0; --cnt) {
         CF = LSB(dest);
         dest = (dest >> 1) | (CF << size - 1);
         flags.C = CF;
      }
      return dest;
   }

   template<typename T>
   T SHL(T dest, uint8_t cnt, State8086::Flags& flags) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         flags.C = MSB(dest);
         dest <<= 1;
      }
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      setFlags(dest, flags);
      return dest;
   }

   template<typename T>
   T SHR(T dest, uint8_t cnt, State8086::Flags& flags) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest >>= 1; // Unsigned divide by 2
      }
      flags.O = MSB(dest); // Valid only if cnt=1
      setFlags(dest, flags);
      return dest;
   }

   template<typename T>
   T SAR(T dest, uint8_t cnt, State8086::Flags& flags) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest = (int)dest >> 1; // Signed divide by 2
      }
      flags.O = 0; // Valid only if cnt=1
      setFlags(dest, flags);
      return dest;
   }

   template<typename T>
   struct rotate {
      T(*operator[](int i))(T, uint8_t, State8086::Flags&) { return a[i]; }
   private:
      T(*a[8])(T, uint8_t, State8086::Flags&) = { ROL,ROR,RCL,RCR,SHL,SHR,void,SAR };
   };

}
