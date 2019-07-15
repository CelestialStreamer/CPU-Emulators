#pragma once
#include "Common.h"
#include "alu.h"
#include "IO.h"
#include "Memory.h"

#include <cassert> /* assert */

// Reserved memory space:
// 0x00000 - 0x0007f (128 bytes)
// 0xffff0 - 0xfffff (16 bytes)
// Reserved IO space:
//  0x00f8 -  0x00ff (8 bytes)
class State8086 {
private:

   /// REGISTERS ///
   // The 8086 has eight 16-bit general purpose registers divided into two groups:
   // Data, and Pointer and Index group

   // 4 16-bit general purpose registers which allow high and low byte access
   struct DRegisters { // Data Registers
      // A register which allows access to high byte, low byte, or entire byte
      typedef union {
         word x; // word (2 bytes)
         struct {
            byte l; // low byte
            byte h; // high byte
         };
      } Register;
      Register a; // Accumulator
      Register b; // Base
      Register c; // Count
      Register d; // Data

   } d_regs;

   // 4 16-bit general purpose registers
   struct PIRegisters { // Pointer and Index Registers
      word SI; // Source Index
      word DI; // Destination Index
      word BP; // Base Pointer
      // Points to the top of the stack, or rather the offset from the top of the stack
      word SP; // Stack Pointer
   } pi_regs;

   // The megabyte of memory space is divided into logical segments of up to 64k bytes each.
   // 4 16-bit special purpose registers
   struct SegmentRegisters { // Segment Registers
      // Instructions are fetched from this segment
      word ES; // Extra segment
      // Points to the current stack segment; stack operations are performed on this segment
      word CS; // Code segment
      // Current extra segment; it also is typically used for data storage
      word SS; // Stack segment
      // Current data segment; it generally contains program variables
      word DS; // Data segment
   } segRegs;

   // The instruction pointer points to the next instruction.
   // This value is can be saved on the stack and later restored
   word IP; // Instruction Pointer

   /// ModR/M ///
   // Parts of the ModR/M byte
   int _mode;
   struct { int _rm, ext; };
   int _reg;
   // Not part of the ModR/M byte, but part of processing things related to the byte
   int disp16; // displacement of immediate value after ModR/M byte to be used for calculating the effective address
   int segment; // Segment of memory addressed
   bool segoverride;

   Memory* memory;
   IO * io;
   ALU alu;

   bool halted;

public:
   State8086(Memory*, IO*);
   ~State8086();

   // Reset/clear alu/segment flags, IP, and leave halt state if in halt state
   void reset();
   void externalInterrupt(unsigned int vector);
   void run(unsigned int runtime);

   bool INTR;

private:
   // Fetches the data pointed to by IP in current code segment
   template<typename T> T imm();

   /// ModR/M ///
   void fetchModRM(); // Read immediate byte and parse as ModR/M byte

   int ea(); // Effective address using address mode from mod field in ModR/M byte
   template<typename T> T& reg(); // register access (according to reg field in ModR/M byte)
   template<typename T> T& rm(); // register/memory access (according to r/m field in ModR/M byte)

   // access memory using default segment
   template<typename T> T& mem(int index) { return mem<T>(segment, index); }
   // access memory using specific segment
   template<typename T> T& mem(int segment, int index) { return memory->mem<T>((segment << 4) + index); }

   void push(word value);
   word pop();

   void callFar(word newIP, word newCS) {
      push(segRegs.CS); push(IP);
      jumpFar(newIP, newCS);
   }
   void callfar(unsigned int address) {
      push(segRegs.CS); push(IP);
      jumpFar(address);
   }
   void jumpFar(unsigned int address) {
      IP = mem<word>(address);
      segRegs.CS = mem<word>(address + 2);
   }
   void jumpFar(word newIP, word newCS) {
      IP = newIP;
      segRegs.CS = newCS;
   }
   void returnFar(word adjustSP = 0) {
      IP = pop(); segRegs.CS = pop() + adjustSP;
   }

   void callnear(word offset) {
      push(IP);
      jumpNear(offset);
   }
   void jumpShort(word jump) {
      IP += jump;
   }
   void jumpNear(word offset) {
      IP = offset;
   }
   void returnNear(word adjustSP = 0) {
      IP = pop();
      segRegs.CS += adjustSP;
   }


   void loadFarPointer(word& segment, word& index, int address) {
      index = mem<word>(address);
      segment = mem<word>(address + 2);
   }

   void interrupt(unsigned int vector);
};


template<typename T>
T State8086::imm() {
   T value = mem<T>(segRegs.CS, IP);
   IP += sizeof(T);
   return value;
}

template<typename T>
T& State8086::rm() {
   if (_mode == 3) return reg<T>();
   else            return memory->mem<T>(ea());
}