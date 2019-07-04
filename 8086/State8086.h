#pragma once
#include "IO.h"
#include "Memory.h"

#include <cassert> /* assert */
#include <cstdint> /* uint8_t uint16_t */

// Reserved memory space:
// 0x00000 - 0x0007f (128 bytes)
// 0xffff0 - 0xfffff (16 bytes)
// Reserved IO space:
//  0x00f8 -  0x00ff (8 bytes)
class State8086 {
public:
   State8086() {
      reset();
   }
   ~State8086() {
      delete io;
      delete mem;
   }

   /// REGISTERS ///
   // The 8086 has eight 16-bit general purpose registers divided into two groups:
   // Data, and Pointer and Index group

   // 4 16-bit general purpose registers which allow high and low byte access
   struct DRegisters { // Data Registers
      // A register which allows access to high byte, low byte, or entire byte
      typedef union {
         uint16_t x; // word (2 bytes)
         struct {
            uint8_t l; // low byte
            uint8_t h; // high byte
         };
      } Register;
      Register a; // Accumulator
      Register b; // Base
      Register c; // Count
      Register d; // Data

   } d_regs;

   // 4 16-bit general purpose registers
   struct PIRegisters { // Pointer and Index Registers
      uint16_t SI; // Source Index
      uint16_t DI; // Destination Index
      uint16_t BP; // Base Pointer
      // Points to the top of the stack, or rather the offset from the top of the stack
      uint16_t SP; // Stack Pointer
   } pi_regs;

   // The megabyte of memory space is divided into logical segments of up to 64k bytes each.

   struct SegmentRegisters { // Segment Registers
      // Instructions are fetched from this segment
      uint16_t ES; // Extra segment
      // Points to the current stack segment; stack operations are performed on this segment
      uint16_t CS; // Code segment
      // Current extra segment; it also is typically used for data storage
      uint16_t SS; // Stack segment
      // Current data segment; it generally contains program variables
      uint16_t DS; // Data segment
      // 0 => ES
      // 1 => CS
      // 2 => SS
      // 3 => DS
      uint16_t& operator [] (uint8_t regid) {
         assert(regid <= 3);
         return ((uint16_t*)this)[regid];
      }
   } segRegs;

   // The instruction pointer points to the next instruction.
   // This value is can be saved on the stack and later restored
   uint16_t IP; // Instruction Pointer

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

   void reset() {
      IP = 0;
      halted = false;
   }

   void externalInterrupt(uint8_t vector) {
      if (flags.I)
         interrupt(vector);
   }

   void Emulate8086Op(int runtime);

   //private:
   IO * io;
   Memory * mem;

   uint8_t mode, reg, rm;
   uint16_t disp16;
   uint16_t segment;
   bool segoverride;

   bool halted;
   template<typename T> T    readrm(uint8_t rmval);
   template<typename T> void writerm(uint8_t rmval, T value);

   // register access
   template<typename T> T& fetchreg(uint8_t regid);

   // other
   uint16_t incrementIP(uint16_t inc) { return IP += inc; }
   void modregrm();
   void push(uint16_t value);
   uint16_t pop();
   void interrupt(uint8_t vector);
   int getea(uint8_t rmval);
};



template<typename T>
T State8086::readrm(uint8_t rmval) {
   if (mode == 3) return fetchreg<T>(rmval);
   else           return mem->getmem<T>(getea(rmval));
}

template<typename T>
void State8086::writerm(uint8_t rmval, T value) {
   if (mode == 3) fetchreg<T>(rmval) = value;
   else           mem->putmem(getea(rmval), value);
}

