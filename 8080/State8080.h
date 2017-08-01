#pragma once

#include <cstdint> // uint8_t, uint16_t, uint32_t
//#include <bitset>

uint8_t parity(uint8_t v);

class State8080 {
public:
   struct Reg
   {
      uint8_t a = 0;
      struct ConditionCodes {
         // 7 6 5 4 3 2 1 0
         // S Z 0 A 0 P 1 C
         uint8_t c; // Carry             Set to 1 if carry out of bit 7
         uint8_t p; // Parity            Set to 1 if even parity
         uint8_t a; // Auxiliary Carry   Set to 1 if carry out of bit 3
         uint8_t z; // Zero              Set to 1 if zero
         uint8_t s; // Sign              Set to 1 if negative (bit 7)
      }f = { 0, 0, 0, 0, 0 };
      uint8_t b = 0, c = 0;
      uint8_t d = 0, e = 0;
      uint8_t h = 0, l = 0;
      uint16_t pc = 0, sp = 0;
   } Reg;

   uint8_t (*in)(uint8_t port);
   void (*out)(uint8_t port, uint8_t value);

   uint8_t memory[0xffff] = {};

   void Emulate8080Op();
   int Disassemble8080Op();
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
   bool interrupt_enabled;  // Are we ready to take interrupts?
   bool interruptRequested; // Is there an interrupt now?
   unsigned char interruptOpcode;
   bool stopped = false;
};