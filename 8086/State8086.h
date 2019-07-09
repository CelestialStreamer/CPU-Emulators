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
   State8086(Memory*, IO*);
   ~State8086();

   // Reset/clear alu/segment flags, IP, and leave halt state if in halt state
   void reset();
   void externalInterrupt(unsigned int vector);
   void run(unsigned int runtime);

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

   // 4 16-bit special purpose registers
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


private:
   IO * io;
   ALU alu;
   Memory * mem;

   bool segoverride;
   bool halted;

   struct BusInterfaceUnit {
      BusInterfaceUnit(State8086* state) : state(state) {}

      void fetchModRM(); // Read immediate byte and parse as ModR/M byte

      int ea(); // Effective address using address mode from mod field in ModR/M byte
      template<typename T> T& reg(); // register access (according to reg field in ModR/M byte)
      template<typename T> T& rm(); // register/memory access (according to r/m field in ModR/M byte)

      // access memory using default segment
      template<typename T> T& mem(int offset) { return mem<T>(segment, offset); }
      // access memory using specific segment
      template<typename T> T& mem(int segment, int offset) { return state->mem->mem<T>((segment << 4) + offset); }

      // Parts of the ModR/M byte
      int _mode;
      struct {
         int _rm, ext;
      };
      int _reg;

      int segment; // Segment of memory addressed

   private:
      State8086* state;
      int disp16; // displacement of immediate value after ModR/M byte to be used for calculating the effective address
   } BIU = BusInterfaceUnit(this);

   // Fetches the data pointed to by IP
   template<typename T> T imm();

   void push(uint16_t value);
   uint16_t pop();
   void interrupt(unsigned int vector);
};

template<typename T>
T State8086::imm() {
   T value = BIU.mem<T>(segRegs.CS, IP);
   IP += sizeof(T);
   return value;
}

template<typename T>
T& State8086::BusInterfaceUnit::rm() {
   if (_mode == 3) return reg<T>();
   else            return state->mem->mem<T>(ea());
}