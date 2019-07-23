#include "State8080.h"

// INR Increment Register or Memory (pg 15)
//
// Format: 00|REG|100
//            ^^^
//    000 for register B
//    001 for register C
//    010 for register D
//    011 for register E
//    100 for register H
//    101 for register L
//    110 for memory ref. M
//    111 for register A
//
// Description:
//    The specified register or memory byte is incremented by one.
//
// Condition bits affected:
//    Zero, Sign, Parity, Auxiliary Carry
void State8080::INR(uint8_t& x) {
   // Save value
   uint8_t value = x;

   // Perform and store operation
   x = x + 1;

   // Condition bits
   Reg.f.z = (x == 0 ? SET : RESET);                                // Zero flag
   Reg.f.s = ((x & 0x80) == 0x80 ? SET : RESET);                    // Sign flag
   Reg.f.p = parity(x);                                             // Parity flag
   Reg.f.a = ((((value & 0x0f) + 1) & 0x10) == 0x10 ? SET : RESET); // Auxiliary Carry flag
}

// DCR Decrement Register or Memory (pg 15)
//
// Format: 00|REG|100
//            ^^^
//    000 for register B
//    001 for register C
//    010 for register D
//    011 for register E
//    100 for register H
//    101 for register L
//    110 for memory ref. M
//    111 for register A
//
// Description:
//    The specified register or memory byte is decremented by one.
//
// Condition bits affected:
//    Zero, Sign, Parity, Auxiliary Carry
void State8080::DCR(uint8_t& x) {
   // Save value
   uint8_t value = x;

   // Perform and store operation
   x = x - 1;

   // Condition bits
   Reg.f.z = (x == 0);             // Zero flag
   Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   Reg.f.p = parity(x);            // Parity flag
   Reg.f.a = ((value & 0x0f) < 1); // Auxiliary Carry flag
}

// DAA Decimal Adjust Accumulator
//
// Format: 00100111
//
// Description:
//       The eight-bit hexadecimal number in the accumulator is
//    adjusted to form two four-bit binary-coded-decimal digits
//    by the following two step process:
//       (1) If the least significant four bits of the accumulator
//    represents a number greater than 9, or if the Auxiliary Carry
//    bit is equal to one, the accumulator is incremented by six.
//    Otherwise, no incrementing occurs.
//       (2) If the most significant four bits of the accumulator
//    now represent a number greater than 9, or if the normal carry
//    bit is equal to one, the most significant four bits of the
//    accumulator are incremented by six. Otherwise, no incrementing
///   occurs.
//       If a carry out of the least significant four bits occurs
//    during Step (1), the Auxiliary Carry bit is set; otherwise it
//    is reset. Likewise, if a carry out of the most significant four
//    bits occur during Step (2), the normal Carry bit is set;
//    otherwise, it is unaffected.
//
// NOTE:
//       This instruction is used when adding decimal numbers. It is
//    the only instruction whose operaiton is affected by the Auxiliary
//    Carry bit.
//
// Condition bits affected:
//    Zero, Sign, Parity, Carry, Auxiliary Carry
void State8080::DAA() {
   /// Step (1)
   /// If LSBits > 9 or AC is set ...
   if (((Reg.a & 0x0f) > 0x09) || (Reg.f.a == SET)) {
      // Compute carry out of bit 3
      uint8_t x = (Reg.a & 0x0f) + 0x06;

      // Auxiliary Carry flag set/reset if carry out on low 4 bits did/not occur
      if ((x & 0x10) == 0x10) Reg.f.a = SET;
      else                    Reg.f.a = RESET;

      /// ... increment accumulator by six, ...
      Reg.a = Reg.a + 0x06;
   } else { /// ... otherwise, no incrementing occurs.
      // TODO: Is the following line of code correct? Everything else is fine.
      Reg.f.a = RESET; // I guess no carry out occured, so AC is reset.
   }

   /// Step (2)
   /// Now if MSBits > 9 or normal carry is set ...
   if (((Reg.a & 0xf0) > 0x90) || (Reg.f.c == SET)) {
      // Compute carry out of bit 7
      uint16_t x = (uint16_t)(Reg.a & 0xf0) + (uint16_t)0x60;

      // Carry flag set/reset if carry out on high 4 bits did/not occur
      if ((x & 0x100) == 0x100) Reg.f.c = SET;
      else                      Reg.f.c = RESET;

      /// ... increment MSB of accumulator by six, ...
      Reg.a = Reg.a + 0x60;
   }
   /// ... otherwise, no incrementing occurs.

   // Condition bits
   Reg.f.z = (Reg.a == 0             ? SET : RESET); // Zero flag
   Reg.f.s = ((Reg.a & 0x80) == 0x80 ? SET : RESET); // Sign flag
   Reg.f.p = parity(Reg.a);                          // Parity flag
}


// ADD Add Register or Memory to Accumulator (pg 17)
//
// Format: 10|000|REG
//
// Description:
//       The specified byte is added to the contents of the accumulator using
//    two's complement arithmetic.
//
// Condition bits affected:
//    Carry, Sign, Zero, Parity, Auxiliary Carry
void State8080::ADD(uint8_t value) {
   // Emulate 8-bit addition using 16-bit numbers
   uint16_t answer = (uint16_t)Reg.a + (uint16_t)value;

   // Compute carry out of bottom four bits
   uint8_t x = (Reg.a & 0x0f) + (value & 0x0f);

   // Store result in Accumulator
   Reg.a = answer & 0xff;

   // Condition bits
   Reg.f.z = ((answer & 0xff) == 0      ? SET : RESET); // Zero   flag
   Reg.f.s = ((answer & 0x80) == 0x80   ? SET : RESET); // Sign   flag
   Reg.f.p = parity(answer & 0xff);                     // Parity flag
   Reg.f.c = ((answer & 0x100) == 0x100 ? SET : RESET); // Carry  flag
   Reg.f.a = ((x & 0x10) == 0x10        ? SET : RESET); // Auxiliary Carry flag
}

// ADC Add Register or Memory to Accumulator With Carry (pg 18)
//
// Format: 10|001|REG
//
// Description:
//       The specified byte plus the content of the Carry bit is added to the
//    contents of the accumulator using two's complement arithmetic.
//
// Condition bits affected:
//    Carry, Sign, Zero, Parity, Auxiliary Carry
void State8080::ADC(uint8_t value) {
   // Emulate 8-bit addition using 16-bit numbers
   uint16_t answer = (uint16_t)Reg.a + (uint16_t)value + (uint16_t)Reg.f.c;

   // Compute carry out of bottom four bits
   uint8_t x = (Reg.a & 0x0f) + (value & 0x0f) + (Reg.f.c == SET ? 1 : 0);

   // Store result in Accumulator
   Reg.a = answer & 0xff;

   // Condition bits
   Reg.f.z = ((answer & 0xff) == 0      ? SET : RESET); // Zero   flag
   Reg.f.s = ((answer & 0x80) == 0x80   ? SET : RESET); // Sign   flag
   Reg.f.p = parity(answer & 0xff);                     // Parity flag
   Reg.f.c = ((answer & 0x100) == 0x100 ? SET : RESET); // Carry  flag
   Reg.f.a = ((x & 0x10) == 0x10        ? SET : RESET); // Auxiliary Carry flag
}
// SUB Subtract Register or Memory From Accumulator (pg 18)
//
// Format: 10|010|REG
//
// Description:
//       The specified byte is subtracted from the accumulator using two's
//    complement arithmetic.
//       If there is no carry out of the high-order bit position, indicating
//    that a borrow occured, the Carry bit is set; otherwise it is reset.
//    (Note that this differs from an add operation, which resets the carry
//    if no overflow occurs).
//
// Condition bits affected:
//    Carry, Sign, Zero, Parity, Auxiliary Carry
void State8080::SUB(uint8_t value){
   // Emulate 8-bit subtraction using 16-bit numbers
   uint16_t answer = (uint16_t)Reg.a + (uint16_t)(~value + 1);

   // Compute carry out of bottom four bits
   uint8_t carry = (Reg.a & 0x0f) + ((~value + 1) & 0x0f);

   // Store result in Accumulator
   Reg.a = answer & 0xff;

   // Condition bits
   Reg.f.z = ((answer & 0xff) == 0      ? SET : RESET); // Zero   flag
   Reg.f.s = ((answer & 0x80) == 0x80   ? SET : RESET); // Sign   flag
   Reg.f.p = parity(answer & 0xff);                     // Parity flag
   Reg.f.c = ((answer & 0x100) == 0x100 ? SET : RESET); // Carry  flag
   Reg.f.a = ((carry & 0x10) == 0x10    ? SET : RESET); // Auxiliary Carry flag
}
// SBB Subtract Register or Memory From Accumulator With Borrow (pg 19)
//
// Format: 10|011|REG
//
// Description:
//       The Carry bit is internally added to the contents of the specified
//    byte. This value is then subtracted from the accumulator using two's
//    complement arithmetic.
//       This instruction is most usefull when performing subtractions. It
//    adjusts the result of subtracting two bytes when a previous subtraction
//    has produced a negative result (a borrow).
//
// Condition bits affected:
//    Carry, Sign, Zero, Parity, Auxiliary Carry
void State8080::SBB(uint8_t value) {
   /// The Carry bit is internally added to the contents of the specified byte.
   /// This value is then subtracted from the accumulator . . ..
   SUB(value + Reg.f.c);
}
// ANA Logical and Register or Memory With Accumulator (pg 19)
//
// Format: 10|100|REG
//
// Description:
//       The specified byte is logically ANDed bit by bit with the contents of
//    the accumulator. The Carry bit is reset to zero.
//       The logical AND function of two bits is 1 if and only if both bits
//    equal 1.
//
// Condition bits affected:
//    Carry, Zero, Sign, Parity
void State8080::ANA(uint8_t value) {
   // Perform operation
   uint8_t x = Reg.a & value;

   // Store result in Accumulator
   Reg.a = x;

   // Condition bits
   Reg.f.z = (x == 0             ? SET : RESET); // Zero flag
   Reg.f.s = ((x & 0x80) == 0x80 ? SET : RESET); // Sign flag
   Reg.f.p = parity(x);                          // Parity flag
   Reg.f.c = RESET;                              // Carry flag (Reset to zero)
                                                 // Documentation doesn't include this flag,
                                                 // but I'm adding it because all the rest of the math function set this flag.
   Reg.f.a = RESET;                              // Auxiliary Carry flag
}
// XRA Logical Exlusive-Or Register or Memory With Accumulator (Zero Accumulator) (pg 19)
//
// Format: 10|101|REG
//
// Description:
//       The specified byte is EXCLUSIVE-ORed bit by bit with the contents of
//    the accumulator. The Carry bit is reset to zero.
//       The EXCLUSIVE-OR function of two bits equals 1 if and only if the
//    values of the bits are different.
//
// Condition bits affected:
//    Carry, Zero, Sign, Parity, Auxiliary Carry
void State8080::XRA(uint8_t value) {
   // Perform operation
   uint8_t x = Reg.a ^ value;

   // Store result in Accumulator
   Reg.a = x;

   // Condition bits
   Reg.f.z = (x == 0             ? SET : RESET); // Zero flag
   Reg.f.s = ((x & 0x80) == 0x80 ? SET : RESET); // Sign flag
   Reg.f.p = parity(x);                          // Parity flag
   Reg.f.c = RESET;                              // Carry flag (Reset to zero)
   Reg.f.a = RESET;                              // Auxiliary Carry flag
}
// ORA Logical Or Register or Memory With Accumulator (pg 20)
//
// Format: 10|110|REG
//
// Description:
//       The specified byte is logically ORed bit by bit with the contents of
//    the accumulator. The Carry bit is reset to zero.
//       The logical OR function of two bits equals zero if and only if both
//    the bits equal zero.
//
// Condition bits affected:
//    Carry, Zero, Sign, Parity
void State8080::ORA(uint8_t value) {
   // Perform operation
   uint8_t x = Reg.a | value;

   // Store result in Accumulator
   Reg.a = x;

   // Condition bits
   Reg.f.z = (x == 0             ? SET : RESET); // Zero flag
   Reg.f.s = ((x & 0x80) == 0x80 ? SET : RESET); // Sign flag
   Reg.f.p = parity(x);                          // Parity flag
   Reg.f.c = RESET;                              // Carry flag (Reset to zero)
   Reg.f.a = RESET;                              // Auxiliary Carry flag
                                                 // (AC ncluded to match others)
}
// CMP Compare Register or Memory With Accumulator (pg 20)
//
// Format: 10|111|REG
//
// Description:
//       The specified byte is compared to the contents of the accumulator.
//    The comparison is performed by internally subtracting the contents of
//    REG from the accumulator (leaving both unchanged) and setting the
//    condition bits according to the result. In particular, the Zero bit is
//    set if the quantities are equal, and reset if they are unequal. Since a
//    subtraction operation is performed, the Carry bit will be set if there
//    is no carry bit of bit 7, indicating that the contents of REG are
//    greater than the contents of the accumulator, and reset otherwise.
//
// Note:
//       If the two quantities to be compared differ in sign, the sense of the
//    Carry bit is reversed.
//
// Condition bits affected:
//    Carry, Zero, Sign, Parity, Auxiliary Carry
void State8080::CMP(uint8_t value) {
   // Perform pseudo operation
   uint16_t answer = (uint16_t)Reg.a + (uint16_t)(~value + 1);

   // Compute carry out of bottom four bits
   uint8_t x = (Reg.a & 0x0f) + ((~value + 1) & 0x0f);

   // Nothing stored in Accumulator

   // Condition bits
   Reg.f.z = ((answer & 0xff) == 0x00   ? SET : RESET); // Zero flag
   Reg.f.s = ((answer & 0x80) == 0x80   ? SET : RESET); // Sign flag
   Reg.f.p = parity(answer & 0xff);                     // Parity flag
   Reg.f.c = ((answer & 0x100) == 0x100 ? SET : RESET); // Carry flag
   Reg.f.a = ((x & 0x10) == 0x10        ? SET : RESET); // Auxiliary Carry flag
}

// PUSH Push Data Onto Stack (pg 22)
//
// Format: 11|RP|0101
//            ^^
//    00 for registers B and C
//    01 for registers D and E
//    10 for registers H and L
//    11 for flags and register A
//
// Description:
//       The contents of the specified register pair are saved in two bytes of
//    memory indicated by the stack pointer SP.
//       The contents of the first register are saved at the memory address
//    one less than the address indicated by the stack pointer; the contents
//    of the second register are saved at the address two less than the
//    address indicated by the stack pointer. If register pair PSW is
//    specified, the first byte of information saved holds the contents of the
//    A register; the second byte holds the settings of the five condition
//    bits, i.e., Carry, Zero, Sign, Parity, and Auxiliary Carry. The format
//    of this byte is:
//
//          Bit Symbol Note
//          7   S      State of Sign bit
//          6   Z      State of Zero bit
//          5   0      Always 0
//          4   AC     State of auxiliary Carry bit
//          3   0      Always zero
//          2   P      State of Parity bit
//          1   1      Always one
//          0   C      State of Carry bit
//
//       In any case, after the data has been saved, the stack pointer is
//    decremented by two.
//
// Condition bits affected:
//    None
void State8080::PUSH(uint16_t val) {
   Reg.sp -= 2;
   mem<uint16_t>(Reg.sp) = val;
}

// POP Pop Data Off Stack (pg 23)
//
// Format: 11|RP|0001
//            ^^
//    00 for registers B and C
//    01 for registers D and E
//    10 for registers H and L
//    11 for flags and register A
//
// Description:
//       The contents of the specified register pair are restored from two
//    bytes of memory indicated by the stack pointer SP. The byte of data at
//    the memory address indicated by the stack pointer is loaded into the
//    second register of the register pair; the byte of data at the address
//    one greater than the address indicated by the stack pointer is loaded
//    into the first register of the pair. If register pair PSW is specified,
//    the byte of data indicated by the contents of the stack pointer plus one
//    is used to restore the values of the five condition bits (Carry, Zero,
//    Sign, Parity, and Auxiliary Carry) using the format described in the
//    last section.
//       In any case, after the data has been restored, the stack pointer is
//    incremented by two.
//
// Condition bits affected:
//       If register pair PSW is specified, Carry, Sign, Zero, Parity, and
//    Zuxiliary Carry may be changed. Otherwise, none are affected.
uint16_t State8080::POP() {
   uint16_t value = mem<uint16_t>(Reg.sp);
   Reg.sp -= 2;
   return value;
   // Condition bits are not set by this function.
   // Calling function should set condition bits if register pair PSW is specified.
}

// DAD Double Add (pg 24)
//
// Format: 00|RP|1001
//            ^^
//    00 for registers B and C
//    01 for registers D and E
//    10 for registers H and L
//    11 for register SP
//
// Description:
//       The 16-bit number in the specified register pair is added to the
//    16-bit number held in the H and L registers using two's complement
//    arithmetic. The result replaces the contents of the H and L registers.
//
// Condition bits affected:
//    Carry
void State8080::DAD(uint32_t rp) {
   // Perform operation
   uint32_t res = (uint32_t)Reg.hl + rp;

   // Store result
   Reg.hl = res;

   // Condition flags
   Reg.f.c = ((res & 0x10000) == 0x10000 ? SET : RESET); // Carry flag
}

// CALL Call (pg 34)
//
// Format: [11|001|10|1] [low add] [hi add]
//
// Description:
//    A call operation is unconditionally performed to subroutine sub.
//
// Condition bits affected:
//    None
void State8080::CALL(uint16_t address) {
   PUSH(Reg.pc); // Push program counter onto stack
   Reg.pc = address; // Jump to address
}

// RET Return (pg 36)
//
// Format: 11|001|00|1
//
// Description:
//       A return operation is unconditionally performed.
//       Thus, execution proceeds with the instruction immediately following
//    the last call instruction.
//
// Condition bits affected:
//    None
void State8080::RET() {
   Reg.pc = POP();
}