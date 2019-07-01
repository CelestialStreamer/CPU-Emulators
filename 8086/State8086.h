#pragma once
#include <cstdint>

class State8086 {
public:
   State8086() {
      reset();
   }
   ~State8086() {}

   /// REGISTERS ///

   // 4 16-bit general purpose registers which allow high and low byte access
   struct MainRegisters {
      // A register which allows access to high byte, low byte, or entire byte
      typedef union {
         uint16_t x; // word (2 bytes)
         struct {
            uint8_t l; // low byte
            uint8_t h; // high byte
         };
      } Register;
      Register a; // Primary accumulator
      Register b; // Base, accumulator
      Register c; // Counter, accumulator
      Register d; // Accumulator, extended accumulator
   } regs;

   // 4 16-bit general purpose registers
   struct IndexRegisters {
      uint16_t SI; // Source Index
      uint16_t DI; // Destination Index
      uint16_t BP; // Base Pointer
      uint16_t SP; // Stack Pointer
   } indRegs;

   // Instruction Pointer
   uint16_t IP;

   // Segment Registers
   struct SegmentRegisters {
      uint16_t CS; // Code segment
      uint16_t DS; // Data segment
      uint16_t ES; // Extra segment
      uint16_t SS; // Stack segment
      uint16_t& operator [] (uint8_t regid) {
         return ((uint16_t*)this)[regid > 3 ? 3 : regid];
      }
   } segRegs;

   // Status register
   struct StatusRegister {
      // F E D C B A 9 8 7 6 5 4 3 2 1 0
      // - - - - O D I T S Z - A - P - C
      uint8_t O = 0; // Overflow
      uint8_t D = 0; // Direction
      uint8_t I = 0; // Interrupt
      uint8_t T = 0; // Trap
      uint8_t S = 0; // Sign
      uint8_t Z = 0; // Zero
      uint8_t A = 0; // Auxiliary Carry
      uint8_t P = 0; // Parity
      uint8_t C = 0; // Carry
      void set(uint16_t x) {
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
      void set8(uint8_t x) {
         C = x & (1 << 0x0) ? 1 : 0;
         P = x & (1 << 0x2) ? 1 : 0;
         A = x & (1 << 0x4) ? 1 : 0;
         Z = x & (1 << 0x6) ? 1 : 0;
         S = x & (1 << 0x7) ? 1 : 0;
      }
      uint16_t get() {
         return 2 // Can't tell if bit 1 is supposed to be set or not.
            | ((uint16_t)C << 0x0)
            | ((uint16_t)P << 0x2)
            | ((uint16_t)A << 0x4)
            | ((uint16_t)Z << 0x6)
            | ((uint16_t)S << 0x7)
            | ((uint16_t)T << 0x8)
            | ((uint16_t)I << 0x9)
            | ((uint16_t)D << 0xA)
            | ((uint16_t)O << 0xB);
      }
      uint8_t get8() {
         return 2 // Can't tell if bit 1 is supposed to be set or not.
            | ((uint16_t)C << 0x0)
            | ((uint16_t)P << 0x2)
            | ((uint16_t)A << 0x4)
            | ((uint16_t)Z << 0x6)
            | ((uint16_t)S << 0x7);
      }
   } flags;

   void foo() {
      segRegs[0];
   }

   void reset() {
      IP = 0;
      halted = false;
   }
   void externalInterrupt(uint8_t vector) {
      if (flags.I)
         interrupt(vector);
   }

private:
   bool halted;
   void interrupt(uint8_t vector);
};
