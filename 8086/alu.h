#pragma once

#include <cstdint>
#include <cassert>


template<typename T> inline uint8_t MSB(T x) { return ~((T)(-1) >> 1) & x ? 1 : 0; }
template<typename T> inline uint8_t LSB(T x) { return x & 1 ? 1 : 0; }

class ALU {
public:
   template<typename T>
   T add(T op1, T op2) {
      unsigned int res = (unsigned int)(op1)+(unsigned int)(op2);
      setArithemticFlags<T>(op1, op2, res);
      return (T)res;
   }

   template<typename T>
   T _or(T op1, T op2) {
      T res = op1 | op2;
      setLogicFlags<T>(res);
      return res;
   }

   template<typename T>
   T adc(T op1, T op2) {
      unsigned int res = (unsigned int)op1 + (unsigned int)op2 + (unsigned int)flags.C;
      setArithemticFlags(op1, op2, res);
      return (T)res;
   }

   template<typename T>
   T sbb(T op1, T op2) {
      unsigned int res = (unsigned int)op1 - (unsigned int)(op2 + flags.C);
      setArithemticFlags(op1, op2, res);
      return (T)res;
   }

   template<typename T>
   T _and(T op1, T op2) {
      T res = op1 & op2;
      setLogicFlags<T>(res);
      return res;
   }

   template<typename T>
   T sub(T op1, T op2) {
      unsigned int res = (unsigned int)op1 - (unsigned int)op2;
      setArithemticFlags<T>(op1, op2, res);
      return (T)res;
   }

   template<typename T>
   T _xor(T op1, T op2) {
      T res = op1 ^ op2;
      setLogicFlags<T>(res);
      return res;
   }

   template<typename T>
   T arithm(unsigned int i, T op1, T op2) {
      assert(i < 8);
      switch (i) {
      case 0: return add<T>(op1, op2);
      case 1: return _or<T>(op1, op2);
      case 2: return adc<T>(op1, op2);
      case 3: return sbb<T>(op1, op2);
      case 4: return _and<T>(op1, op2);
      case 5: return sub<T>(op1, op2);
      case 6: return _xor<T>(op1, op2);
      default:return sub<T>(op1, op2);
      }
   }

   bool condition(int test);

   template<typename T>
   T ROL(T dest, uint8_t cnt) {
      for (cnt %= 8 * sizeof(dest); cnt > 0; --cnt) {
         flags.C = MSB(dest);
         dest = (dest << 1) | (flags.C);
      }
      flags.O = MSB(dest); // valid only if cnt=1
      return dest;
   }

   template<typename T>
   T ROR(T dest, uint8_t cnt) {
      const int size = 8 * sizeof(dest);

      for (cnt %= size; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest = (dest >> 1) | (flags.C << (size - 1));
      }
      flags.O = (dest >> (size - 1)) ^ ((dest >> (size - 2)) & 1); // Valid only if cnt=1
      return dest;
   }

   template<typename T>
   T RCL(T dest, uint8_t cnt) {
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
   T RCR(T dest, uint8_t cnt) {
      const int size = 8 * sizeof(dest);

      int CF;
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      for (cnt %= size + 1; cnt > 0; --cnt) {
         CF = LSB(dest);
         dest = (dest >> 1) | (CF << (size - 1));
         flags.C = CF;
      }
      return dest;
   }

   template<typename T>
   T SHL(T dest, uint8_t cnt) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         flags.C = MSB(dest);
         dest <<= 1;
      }
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      setFlags(dest);
      return dest;
   }

   template<typename T>
   T SHR(T dest, uint8_t cnt) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest >>= 1; // Unsigned divide by 2
      }
      flags.O = MSB(dest); // Valid only if cnt=1
      setFlags<T>(dest);
      return dest;
   }

   template<typename T>
   T SAR(T dest, uint8_t cnt) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest = (int)dest >> 1; // Signed divide by 2
      }
      flags.O = 0; // Valid only if cnt=1
      setFlags(dest);
      return dest;
   }

   template<typename T>
   T rotate(unsigned int i, T dest, uint8_t cnt) {
      assert(i < 8);
      switch (i) {
      case 0: return ROL(dest, cnt);
      case 1: return ROR(dest, cnt);
      case 2: return RCL(dest, cnt);
      case 3: return RCR(dest, cnt);
      case 4: return SHL(dest, cnt);
      case 5: return SHR(dest, cnt);
      case 6: assert(false); return 0; // Not defined
      default: return SAR(dest, cnt);
      }
   }

   uint8_t DAA(uint8_t AL) {
      if (((AL & 0xF) > 9) || (flags.A == 1)) {
         uint16_t temp = (uint16_t)AL + 6;
         AL = temp & 0xff;
         flags.C = (flags.C || (temp & 0xF0)) ? 1 : 0; // detect carry out of lower 4 bits
      } else
         flags.A = 0;

      if (((AL & 0xF0) > 0x90) || (flags.C == 1)) {
         AL += 0x60;
         flags.C = 1;
      } else
         flags.C = 0;

      setFlags<uint8_t>(AL);
      return AL;
   }

   uint8_t AAA(uint8_t AL) {
      if (((AL & 0xF) > 9) || (flags.A == 1)) {

      }
      return 0;
   }

   template<typename T>
   T INC(T op) {
      auto C = flags.C; // save carry flag
      T result = add<T>(op, 1);
      flags.C = C; // restore carry flag
      return result;
   }

   template<typename T>
   T DEC(T op) {
      auto C = flags.C; // save carry flag
      T result = sub<T>(op, 1);
      flags.C = C; // restore carry flag
      return result;
   }

   template<typename T>
   void setFlags(T value) {
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
   void setArithemticFlags(T op1, T op2, unsigned int res) {
      const T signBit = ~((T)(-1) >> 1); // 0x80..0
      const unsigned int overflow = ((int)(-1) - (T)(-1)); // 0xFF..FF00..00 Should be as many zeros as bits in T

      setFlags<T>((T)res);
      flags.O = (res ^ op1) & (res ^ op2) & signBit ? 1 : 0;
      flags.C = (res & overflow) ? 1 : 0;
      flags.A = ((op1 ^ op2 ^ res) & 0x10) ? 1 : 0;
   }

   template<typename T>
   void setLogicFlags(T value) {
      setFlags<T>(value);
      flags.C = 0;
      flags.O = 0;
   }

   // Status Flags:
   // O, S, Z, A, P, C
   // Controll Flags:
   // I, D, T
   struct Flags {
      // F E D C B A 9 8 7 6 5 4 3 2 1 0
      // - - - - O D I T S Z - A - P - C

      // An arithmetic overflow has occurred.
      // An Interrupt On Overflow instruction is available (which?)
      uint8_t O = 0; // Overflow
      // When set, string instructions auto-decrement;
      // that is, to processs strings from high addresses to low addresses,
      // or from "right to left". Clearing causes "left to right"
      uint8_t D = 0; // Direction
      // Setting allows CPU to recognize external (maskable) interrupt requests.
      // This flag has no affect on either non-maskable external or internally generated interrupts.
      uint8_t I = 0; // Interrupt
      // Setting this puts the CPU into single-step mode for debugging.
      // In this mode, the CPU automatically generates an internal interrupt after each instruction.
      uint8_t T = 0; // Trap
      // If set, the high-order bit of the result is 1.
      // 0 => positive, 1 => negative
      uint8_t S = 0; // Sign
      // Result of operation is zero
      uint8_t Z = 0; // Zero
      // There has been a
      //    carry out of the low nibble into the high nibble
      // or a
      //    borrow from the high nibble into the low nibble
      // of an 8-bit quantity (low-order byte of a 16-bit quantity)
      uint8_t A = 0; // Auxiliary Carry
      // If set, the result has even parity, an even number of 1-bits
      uint8_t P = 0; // Parity
      // There has been a
      //    carry out of, or a borrow into,
      // the high-order bit of the result (8- or 16-bit)
      uint8_t C = 0; // Carry

      template<typename T> void set(T);
      template<typename T> T get();

   } flags;
};
