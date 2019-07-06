#pragma once
#include "alu.h"
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


   void reset() {
      IP = 0;
      halted = false;
   }

   void externalInterrupt(uint8_t vector) {
      if (alu.flags.I)
         interrupt(vector);
   }

   void run(int runtime);

private:
   IO * io;
   ALU alu;
   Memory * mem;

   struct ModRegRm {
      int mode, reg, rm;
      void operator()();
   } modregrm;

   uint16_t disp16;
   uint16_t segment;
   bool segoverride;

   bool halted;

   // register/memory access
   template<typename T> T    readrm();
   template<typename T> void writerm(T value);

   // register access
   template<typename T> T& fetchreg(uint8_t regid);

   // other
   template<typename T>
   T imm() {
      T value = mem->getmem<T>(segRegs.CS, IP);
      IP += sizeof(T);
      return value;
   }

   void push(uint16_t value);
   uint16_t pop();
   void interrupt(uint8_t vector);
   int getea();
};



template<typename T>
T State8086::readrm() {
   if (modregrm.mode == 3) return fetchreg<T>(modregrm.rm);
   else                    return mem->getmem<T>(getea());
}

template<typename T>
void State8086::writerm(T value) {
   if (modregrm.mode == 3) fetchreg<T>(modregrm.rm) = value;
   else                    mem->putmem(getea(), value);
}

