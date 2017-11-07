#pragma once
#include "IO.h"
#include <cstdint> // uint8_t, uint16_t, uint32_t

#define SET 1
#define RESET 0

// From manual Parity Bit
// "The Parity bit is set to 1 for even parity, and is reset to 0 for odd parity."
#define EVEN SET
#define ODD RESET

uint8_t parity(uint8_t v);

class State8080 {
public:
   struct Reg
   {
      uint8_t a = 0;
      struct ConditionCodes {
         // 7 6 5 4 3 2 1 0
         // S Z 0 A 0 P 1 C
         uint8_t c; // Carry
         uint8_t p; // Parity
         uint8_t a; // Auxiliary Carry
         uint8_t z; // Zero
         uint8_t s; // Sign
      }f = { RESET, RESET, RESET, RESET, RESET };
      uint8_t b = 0, c = 0;
      uint8_t d = 0, e = 0;
      uint8_t h = 0, l = 0;
      uint16_t pc = 0, sp = 0;
   } Reg;

   uint8_t memory[0xffff] = {};

   void Emulate8080Op();
   int  Disassemble8080Op();
   void display();

   uint8_t immediate(uint8_t byte = 1) { return memory[Reg.pc + byte]; }

   uint16_t address() { return (immediate(2) << 8) | (immediate(1) << 0); }

   uint8_t &getRegister(uint8_t code)
   {
      switch (code)
      {
      case 0: return Reg.b;
      case 1: return Reg.c;
      case 2: return Reg.d;
      case 3: return Reg.e;
      case 4: return Reg.h;
      case 5: return Reg.l;
      case 6: return memory[(Reg.h << 8) | (Reg.l << 0)];
      default: return Reg.a;
      }
   }

   void generateInterrupt(uint8_t opcode);

private:
   IO io;
   bool interrupt_enabled = false;  // Are we ready to take interrupts?
   bool interruptRequested = false; // Is there an interrupt now?
   unsigned char interruptOpcode = 0;
   bool stopped = false;
};
