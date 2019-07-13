#pragma once

#include <cstdint>
#include <cassert>


template<typename T> inline uint8_t MSB(T x) { return ~((T)(-1) >> 1) & x ? 1 : 0; }
template<typename T> inline uint8_t LSB(T x) { return x & 1 ? 1 : 0; }

class ALU {
public:
   // ADD: (IA V2 3-17) (8086 Family 2-35)
   //    Adds the destination operand (first) and the source operand (second) 
   // Operation:
   //    DEST <- DEST + SRC
   // Flags:
   //    O, S, Z, A, C, P
   template<typename T>
   T add(T op1, T op2) {
      /// The ADD instruction does not distinguish between signed or unsigned operands (IA V2 3-17:Description)
      /// Both operands may be signed or unsigned binary numbers (8086 Family 2-35:ADD)
      unsigned int res = (unsigned int)(op1)+(unsigned int)(op2);
      setArithemticFlags<T>(op1, op2, res);
      return (T)res;
   }

   // OR: (IA V2 3-313) (8086 Family 2-38)
   //    Performs a bitwise inclusive OR operation between the destination (first) and source (second)
   // Operation:
   //    DEST <- DEST OR SRC
   // Flags:
   //    S, Z, P
   //    O, C cleared
   template<typename T>
   T _or(T op1, T op2) {
      T res = op1 | op2;
      setLogicFlags<T>(res);
      return res;
   }

   // ADC: (IA V2 3-15) (8086 Family 2-35)
   //    Adds the destination operand (first), the source operand (second), and the carry flag
   // Operation:
   //    DEST <- DEST + SRC + CF
   // Flags:
   //    O, S, Z, A, C, P
   template<typename T>
   T adc(T op1, T op2) {
      /// The ADC instruction does not distiguish between signed or unsigned operands (IA V2 3-15:Description)
      /// Both operands may be signed or unsigned binary numbers (8086 Family 2-35:ADC)
      unsigned int res = (unsigned int)op1 + (unsigned int)op2 + (unsigned int)flags.C;
      setArithemticFlags(op1, op2, res);
      return (T)res;
   }

   // SBB: (IA V2 3-420) (8086 Family 2-36)
   //    Adds the source operand (second) and the carry flag, and subtracts the result from the destination operand (first)
   // Operation:
   //    DEST <- DEST - (SRC + CF)
   // Flags:
   //    O, S, Z, A, P, C
   template<typename T>
   T sbb(T op1, T op2) {
      /// The SBB instruction does not distinguish between signed or unsigned operands (IA V2 3-420:Description)
      /// Both operands may be signed or unsigned binary numbers (8086 Family 2-36:SBB)
      unsigned int res = (unsigned int)op1 - (unsigned int)(op2 + flags.C);
      setArithemticFlags(op1, op2, res);
      return (T)res;
   }

   // AND: (IA V2 3-19) (8086 Family 2-38)
   //    Performs a bitwise AND operation on the destination (first) and source (second) operands
   // Operation:
   //    DEST <- DEST AND SRC
   // Flags:
   //    S, Z, P
   //    O, C cleared
   template<typename T>
   T _and(T op1, T op2) {
      T res = op1 & op2;
      setLogicFlags<T>(res);
      return res;
   }

   // SUB: (IA V2 3-448) (8086 Family 2-36)
   //    Subtracts the source operand (second) from the destination operand (first)
   // Operation:
   //    DEST <- DEST - SRC
   // Flags:
   //    O, S, Z, A, P, C
   template<typename T>
   T sub(T op1, T op2) {
      /// The SUB instruction does not distinguish between signed or unsigned operands (IA V2 3-448:Description)
      /// Both operands may be signed or unsigned binary numbers (8086 Family 2-36:SUB)
      unsigned int res = (unsigned int)op1 - (unsigned int)op2;
      setArithemticFlags<T>(op1, op2, res);
      return (T)res;
   }

   // XOR: (IA V2 3-466) (8086 Family 2-38)
   //    Peforms a bitwise exclusive OR (XOR) operation on the destination (first) and source (second) operands
   // Operation:
   //    DEST <- DEST XOR SrC
   // Flags:
   //    S, Z, P
   //    O, C cleared
   template<typename T>
   T _xor(T op1, T op2) {
      T res = op1 ^ op2;
      setLogicFlags<T>(res);
      return res;
   }

   bool condition(int test);

   // ROL: (IA V2 3-394) (8086 Family 2-40)
   //    Shift all the bits toward more-significant bit positions, except for the most-significant bit,
   //    which is rotated to the least-significant bit location (see IA V1 Fig 6-10)
   // Operation:
   //    SIZE <- OperandSize
   //    CASE (determine count) OF
   //       SIZE=8:  tempCOUNT <- COUNT MOD 8
   //       SIZE=16: tempCOUNT <- COUNT MOD 16
   //    ESAC
   //    WHILE (tempCOUNT != 0)
   //       DO
   //          tempCF <- MSB(DEST)
   //          DEST <- (DEST * 2) + tempCF
   //          tempCOUNT <- tempCOUNT - 1
   //       OD
   //    ELIHW
   //    CF <- LSB(DEST)
   //    IF COUNT=1
   //       THEN OF <- MSB(DEST) XOR CF
   //       ELSE OF is undefined
   //    FI
   // Flags:
   //    C contains the value of the bit shifted into it
   //    O affected only for single bit rotates; it is undefined for multi-bit rotates
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T ROL(T dest, unsigned int cnt) {
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         flags.C = MSB(dest);
         dest = (dest << 1) | (flags.C);
      }
      flags.O = MSB(dest) ^ flags.C; // valid only if cnt=1
      return dest;
   }

   // ROR: (IA V2 3-395) (8086 Family 2-40)
   //    Shift allthe bits toward less significant bit positions, except for the least-significant bit,
   //    which is rotated to the most-significant bit location (see IA V1 Fig 6-10)
   // Operation:
   //    SIZE <- OperandSize
   //    CASE (determine count) OF
   //       SIZE=8:  tempCOUNT <- COUNT MOD 8
   //       SIZE=16: tempCOUNT <- COUNT MOD 16
   //    ESAC
   //    WHILE (tempCOUNT != 0)
   //       DO
   //          tempCF <- LSB(SRC)
   //          DEST <- (DEST / 2) + (tempCF * 2^SIZE)
   //          tempCOUNT <- tempCOUNT - 1
   //       OD
   //    ELIHW
   //    CF <- MSB(DEST)
   //    IF COUNT = 1
   //       THEN OF <- MSB(DEST) XOR MSB - 1(DEST)
   //       ELSE OF is undefined
   //    FI
   // Flags:
   //    C contains the value of the bit shifted into it
   //    O affected only for single bit rotates; it is undefined for multi-bit rotates
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T ROR(T dest, unsigned int cnt) {
      const T highBit = ~((T)(-1) >> 1); // 0x80..00
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest = (dest >> 1) | (flags.C ? highBit : 0);
      }
      // (MSB(DEST) XOR MSB - 1(DEST)) works out to be the parity of the top two bits
      flags.O = parity[dest & 0b1100'0000];
      return dest;
   }

   // RCL: (IA V2 3-395) (8086 Family 2-40)
   //    Shift all the bits toward more-significant bit positions, except for the most-significant bit,
   //    which is rotate to the lest significant bit location (see IA V1 Fig 6-10)
   // Operation:
   //    SIZE <- OperandSize
   //    CASE (determine count) OF
   //       SIZE=8:  tempCOUNT <- (COUNT AND 1FH) MOD 9
   //       SIZE=16: tempCOUNT <- (COUNT AND 1FH) MOD 17
   //    ESAC
   //    WHILE (tempCOUNt != 0)
   //       DO
   //          tempCF <- MSB(DEST)
   //          DEST <- (DEST * 2) + CF
   //          CF <- tempCF
   //          tempCOUNT <- tempCOUNT - 1
   //       OD
   //    ELIHW
   //    IF COUNT = 1
   //       THEN OF <- MSB(DEST) XOR CF
   //       ELSE OF is undefined
   //    FI
   // Flags:
   //    C contains the value of the bit shifted into it
   //    O affected only for single bit rotates; it is undefined for multi-bit rotates
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T RCL(T dest, unsigned int cnt) {
      int CF;
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         CF = MSB(dest);
         dest = (dest << 1) | flags.C;
         flags.C = CF;
      }
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      return dest;
   }

   // RCR: (IA V2 3-395) (8086 Family 2-40)
   //    Shift all the bits toward less significant bit posotions, except for the least-significant bit,
   //    which is rotated to the most-significant bit location. The instructions shifts the C flag into the
   //    most-significant bit and shifts the least-significant bit into the C flag (see IA V1 Fig 6-10).
   // Operation:
   //    SIZE <- OperandSize
   //    CASE (determine count) OF
   //       SIZE=8:  tempCOUNT <- (COUNT AND 1FH) MOD 9
   //       SIZE=16: tempCOUNT <- (COUNT AND 1FH) MOD 17
   //    ESAC
   //    IF COUNT = 1
   //       THEN OF <- MSB(DEST) XOR CF
   //       ELSE OF is undefined
   //    FI
   //    WHILE (tempCOUNT != 0)
   //       DO
   //          tempCF <- LSB(SRC)
   //          DEST <- (DEST / 2) + (CF * 2^SIZE)
   //          CF <- tempCF
   //          tempCOUNT <- tempCOUNT - 1
   //       OD
   //    ELIWH
   // Flags:
   //    C contains the value of the bit shifted into it
   //    O affected only for single bit rotates; it is undefined for multi-bit rotates
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T RCR(T dest, unsigned int cnt) {
      const T highBit = ~((T)(-1) >> 1); // 0x80..00

      int CF;
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         CF = LSB(dest);
         dest = (dest >> 1) | (flags.C ? highBit : 0);
         flags.C = CF;
      }
      return dest;
   }

   // SHL and SAL perform the same operation and are physically the same instruction. (8086 Family 2-39:SHL/SAL)
   // SHL/SAL: (IA V2 3-416) (8086 Family 2-39)
   //    Shift the bits in the destination operand to the left (toward more significant bit locations).
   //    For each shift count, the most significant bit of the destination operand is shifted into the C flag,
   //    and the least significant bit is cleared (see IA V1 Fig 6-6)
   // Operation:
   //    tempCOUNT <- COUNT AND 1FH
   //    tempDEST <- DEST
   //    WHILE (tempCOUNT != 0)
   //       DO
   //          CF <- MSB(DEST)
   //          DEST <- DEST * 2
   //       OD
   //    IF COUNT = 1
   //       THEN
   //          OF <- MSB(DEST) XOR CF
   //       ELSE IF COUNT = 0
   //          THEN
   //             All flags remain unchanged
   //          ELSE (* COUNT neither 1 or 0 *)
   //             OF is undefined
   //       FI
   //    FI
   // Flags:
   //    C contains the value of the last bit shifted out of the destination operand; it is unedfined where cnt>numbitsof(dest)
   //    O valid for 1-bit shifts, otherwise undefined
   //    A is undefined
   //    For cnt=0, no flags are touched
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T SHL(T dest, unsigned int cnt) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         flags.C = MSB(dest); // Valid only if cnt < numbits(dest)
         dest <<= 1;
      }
      flags.O = MSB(dest) ^ flags.C; // Valid only if cnt=1
      setFlags<T>(dest);
      return dest;
   }

   // SHR: (IA V2 3-416) (8086 Family 2-39)
   //    Shift the bits of the destination operand to the right(toward less significant bit locations).
   //    For each shift count, the least significant bit of the destination operand is shifted into the C flag,
   //    and the most significant bit is either set or cleared depending on the instruction type.
   //    Clears the most significant bit (see IA V1 Fig 6-7)
   // Operation:
   //    tempCOUNT <- COUNT AND 1FH
   //    tempDEST <- DEST
   //    WHILE (tempCOUNT != 0)
   //       DO
   //          CF <- LSB(DEST)
   //          DEST <- DEST / 2 (* Unsigned divide *)
   //       OD
   //    IF COUNT = 1
   //       THEN
   //          OF <- MSB(tempDEST)
   //       ELSE IF COUNT = 0
   //          THEN
   //             All flags remain unchanged
   //          ELSE
   //             OF is undefined
   //       FI
   //    FI
   // Flags:
   //    C O A S Z P
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T SHR(T dest, unsigned int cnt) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest >>= 1; // Unsigned divide by 2
      }
      flags.O = MSB(dest); // Valid only if cnt=1
      setFlags<T>(dest); // Take care of Z,S,P flags
      return dest;
   }

   // SAR: (IA V2 3-416) (8086 Family 2-39)
   //    Shift the bits of the destination operand to the right(toward less significant bit locations).
   //    For each shift count, the least significant bit of the destination operand is shifted into the C flag,
   //    and the most significant bit is either set or cleared depending on the instruction type.
   //    Sets or clears the most significant bit to correspond to the sign (most significant bit)
   //    of the original value in the destination operand. If effect, the instruction fills the empty bit
   //    positions's shifted value with the sign of the unshifted value (see IA V1 Fig 6-8)
   // Operation:
   //    tempCOUNT <- COUNT AND 1FH
   //    tempDEST <- DEST
   //    WHILE (tempCOUNT != 0)
   //       DO
   //          CF <- LSB(DEST)
   //          DEST <- DEST / 2 (* Signed divide, rounding toward negative infinity *)
   //       OD
   //    IF COUNT = 1
   //       THEN
   //          OF <- 0
   //       ELSE IF COUNT = 0
   //          THEN
   //             All flags remain unchanged
   //          ELSE
   //             OF is undefined
   //       FI
   //    FI
   // Flags:
   //    C O A S Z P
   // Note:
   //    The 8086 does not mask the rotation count. However, all other Intel Architecture processors
   //    (starting with the Intel 286 processor) do mask the rotation count to 5 bits, resulting int a
   //    maximum count of 31.
   template<typename T>
   T SAR(T dest, unsigned int cnt) {
      if (cnt == 0) return dest; // All flags remain unchanged if cnt=0
      // This emulator, staying true to the original 8086, will not mask the rotation count
      for (; cnt > 0; --cnt) {
         flags.C = LSB(dest);
         dest = (int)dest >> 1; // Signed divide by 2
      }
      flags.O = 0; // Valid only if cnt=1
      setFlags<T>(dest); // Take care of Z,S,P flags
      return dest;
   }

   // DAA (IA V2 3-79) (8086 Family 2-36)
   //    Adjusts the sum of two packed BCD values to create a packed BCD result
   // Operation:
   //    IF (((AL AND 0FH)>9) or AF=1)
   //       THEN
   //          AL <- AL + 6
   //          CF <- CF OR CarryFromLastAddition (* CF OR carry from AL <- AL + 6 *)
   //          AF <- 1
   //       ELSE
   //          AF <- 0
   //    FI
   //    IF ((AL AND F0H) >90H) or CF=1)
   //       THEN
   //          AL <- AL + 60H
   //          CF <- 1
   //       ELSE
   //          CF <- 0
   //    FI
   // Flags:
   //    C, A, S, Z, P, O
   uint8_t DAA(uint8_t AL) {
      if (((AL & 0xF) > 9) || (flags.A == 1)) {
         uint16_t temp = (uint16_t)AL + 6;
         AL = temp & 0xff;
         flags.C = (flags.C || (temp & 0xFF00)) ? 1 : 0; // detect carry out from AL + 6
         flags.A = 1;
      } else
         flags.A = 0;

      if (((AL & 0xF0) > 0x90) || (flags.C == 1)) {
         AL += 0x60;
         flags.C = 1;
      } else
         flags.C = 0;

      setFlags<uint8_t>(AL); // Take care of Z,S,P flags
      return AL;
   }

   // AAA: (IA V2 3-11) (8086 Family 2-35)
   //    Adjusts the sum of two unpacked BCD values to create an unpacked BCD result.
   // Operation:
   //    IF ((AL AND 0FH)>9) OR (AF=1)
   //       THEN
   //          AL <- (AL + 6)
   //          AH <- AH + 1
   //          AF <- 1
   //          CF <- 1
   //       ELSE
   //          AF <- 0
   //          CF <- 0
   //    FI
   //    AL <- AL AND 0FH
   // Flags:
   //    A, C, O, S, Z, P
   void AAA(uint8_t &AL, uint8_t &AH) {
      if (((AL & 0xF) > 9) || (flags.A == 1)) {
         AL += 6;
         ++AH;
         flags.A = 1;
         flags.C = 1;
      } else {
         flags.A = 0;
         flags.C = 0;
      }
      AL &= 0xF;

      setFlags<uint8_t>(AL); // Z, S, P  flags will be undefined (per spec)
   }

   void AAS(uint8_t &AH, uint8_t &AL) {
      if (((AL & 0xF) > 9) || (flags.A == 1)) {
         AL -= 6;
         AH -= 1;
         flags.A = 1;
         flags.C = 1;
      } else {
         flags.A = 0;
         flags.C = 0;
      }

      AL &= 0x0F;
   }

   void DAS(uint8_t &AL) {
      if (((AL & 0xF) > 9) || (flags.A == 1)) {
         uint16_t temp = (uint16_t)AL - 6;
         AL = temp & 0xFF;
         flags.C = (flags.C || (temp & 0xFF00)) ? 1 : 0;
         flags.A = 1;
      } else
         flags.A = 0;

      if (((AL & 0xF0) > 0x90) || (flags.C == 1)) {
         AL -= 0x60;
         flags.C = 1;
      } else
         flags.C = 0;
   }

   // INC: (IA V2 3-213) (8086 Family 2-35)
   //    Adds 1 to the destination operand, while preserving the state of the CF flag.
   // Operation:
   //    DEST <- DEST + 1
   // Flags:
   //    O, S, Z, A, P
   template<typename T>
   T INC(T op) {
      auto C = flags.C; // save carry flag
      T result = add<T>(op, 1);
      flags.C = C; // restore carry flag
      return result;
   }

   // DEC: (IA V2 3-82) (8086 Family 2-36)
   //    Subtracts 1 from the destination operand, while preserving the state of the CF flag.
   // Operation:
   //    DEST <- DEST - 1
   // Flags:
   //    O, S, Z, A, P
   template<typename T>
   T DEC(T op) {
      auto C = flags.C; // save carry flag
      T result = sub<T>(op, 1);
      flags.C = C; // restore carry flag
      return result;
   }

   // NOT: (IA V2 3-311)
   //    Performs a bitwise NOT operation (each 1 is cleared to 0, and each 0 is set to 1) on the destination operand and stores the result in the destination operand location.
   // Operation:
   //    DEST <- NOT DEST
   // Flags:
   //    None
   template<typename T>
   void NOT(T& op) {
      op = ~op;
   }

   template<typename T>
   void NEG(T& op) {
      op = -op;
      setFlags<uint16_t>(op);
      setLogicFlags<uint16_t>(op);
      flags.C = op == 0 ? 0 : 1;
   }

   template<typename T>
   void MUL(T& overflow, T& op1, T op2);

   template<typename T>
   void IMUL(T& overflow, T& op1, T op2);

   template<typename T>
   bool DIV(T& overflow, T& op1, T op2);

   template<typename T>
   bool IDIV(T& overflow, T& op1, T op2);

   // Compute Z,S,P flags for arithmetic or logical operations
   template<typename T>
   void setFlags(T value) {
      const T signBit = ~((T)(-1) >> 1); // 0x80..0 (high-order bit mask)

      /// If the result of an arithmetic or logical operation is zero,...
      flags.Z = value == 0 ? 1 : 0; /// ...then Z is set; otherwise Z is cleared.

      /// Arithmetic and logical instructions set the sign flag equal to the high-order bit (bit 7 or 15) of the result.
      /// For signed binary numbers, the sign flag will be 0 for positive results and 1 for negative results (so long as overflow does not occur).
      flags.S = value & signBit ? 1 : 0;

      /// If the low-order eight bits of an arithmetic or logical result contain an even number of 1-bits,...
      flags.P = parity[value & 0xff]; /// ...then the parity flag is set; otherwise it is cleared. 

      /// Source: 8086 Family 2-35
   }

   // Compute C,O,A flags for arithmetic operations (along with Z,S,P flags)
   template<typename T>
   void setArithemticFlags(T op1, T op2, unsigned int res) {
      const T signBit = ~((T)(-1) >> 1); // 0x80..0 (high-order bit mask)
      const unsigned int overflow = ((int)(-1) - (T)(-1)); // 0xFF..FF00..00 Should be as many zeros as bits in T

      // Take care of Z,S,P flags
      setFlags<T>((T)res);

      // TODO: Test all of this

      /// If an addition results in a carry out of the high-order bit of the result,...
      /// If a subtraction results in a borrow into the high-order bit of the result,...
      flags.C = (res & overflow) ? 1 : 0; /// ...then C is set; otherwise C is cleared.

      /// If the result on an operation is too large a positive number, or too small a negative number to fit in the destination operand (exluding the sign bit),...
      flags.O = (res ^ op1) & (res ^ op2) & signBit ? 1 : 0; /// ...then O is set otherwise O is cleared.

      /// If an addition results in a carry out of the low-order half-byte of the result,...
      /// If a subtraction results in a borrow into the low-order half-byte of the result,...
      flags.A = ((op1 ^ op2 ^ res) & 0x10) ? 1 : 0; // ...then A is set; otherwise A is cleared.

      /// Source: 8086 Family 2-35
   }

   // Clear O, C flags. Compute S, Z, P flags for logical operations
   template<typename T>
   void setLogicFlags(T value) {
      /// AND, OR, XOR, and TEST affect the flags as follows:
      /// The overflow (OF) and carry (CF) flags are always cleared by logical instructions,
      /// and the contents of the auxiliary carry (AF) is always undefined following execution of a logical instruction.
      flags.C = flags.O = 0;
      /// The sign (SF), zero (ZF)  and parity (PF) flags are always posted to refect the result of the operation.
      setFlags<T>(value); // Set Z,S,P flags
      // (8086 Family 2-38:Logical)
   }

   // Status Flags:
   // O, S, Z, A, P, C
   // Controll Flags:
   // I, D, T
   struct Flags {
      // F E D C B A 9 8 7 6 5 4 3 2 1 0
      // - - - - O D I T S Z - A - P - C
      // (8086 Family p2-33:Figure 2-32)

      // An arithmetic overflow has occurred.
      // An Interrupt On Overflow instruction is available (which?)
      int O = 0; // Overflow
      // When set, string instructions auto-decrement;
      // that is, to processs strings from high addresses to low addresses,
      // or from "right to left". Clearing causes "left to right"
      int D = 0; // Direction
      // Setting allows CPU to recognize external (maskable) interrupt requests.
      // This flag has no affect on either non-maskable external or internally generated interrupts.
      int I = 0; // Interrupt
      // Setting this puts the CPU into single-step mode for debugging.
      // In this mode, the CPU automatically generates an internal interrupt after each instruction.
      int T = 0; // Trap
      // If set, the high-order bit of the result is 1.
      // 0 => positive, 1 => negative
      int S = 0; // Sign
      // Result of operation is zero
      int Z = 0; // Zero
      // There has been a
      //    carry out of the low nibble into the high nibble
      // or a
      //    borrow from the high nibble into the low nibble
      // of an 8-bit quantity (low-order byte of a 16-bit quantity)
      int A = 0; // Auxiliary Carry
      // If set, the result has even parity, an even number of 1-bits
      int P = 0; // Parity
      // There has been a
      //    carry out of, or a borrow into,
      // the high-order bit of the result (8- or 16-bit)
      int C = 0; // Carry

      template<typename T> void set(T); // Set flags
      template<typename T> T get(); // Get flags in byte/word format
      void clear();
   } flags;
private:

   // Lookup table for parity of a byte
   const int parity[256] = {
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
};
