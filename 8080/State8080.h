#pragma once

#include <cstdint> // uint8_t, uint16_t, uint32_t
#include <bitset>

uint8_t parity(uint8_t v);

class State8080 {
public:
   struct RReg
   {
      uint8_t a = 0;
      struct ConditionCodes {
         // 7 6 5 4 3 2 1 0
         // S Z 0 A 0 P 1 C
         uint8_t c : 1; // Carry             Set to 1 if carry out of bit 7
         uint8_t p : 1; // Parity            Set to 1 if even parity
         uint8_t a : 1; // Auxiliary Carry   Set to 1 if carry out of bit 3
         uint8_t z : 1; // Zero              Set to 1 if zero
         uint8_t s : 1; // Sign              Set to 1 if negative (bit 7)
      }f = { 0, 0, 0, 0, 0 };
      uint8_t b = 0, c = 0;
      uint8_t d = 0, e = 0;
      uint8_t h = 0, l = 0;
      uint16_t pc = 0, sp = 0;
   } Reg;

   uint8_t input[0xff];
   uint8_t output[0xff];

   uint8_t memory[0xffff] = {};

   void Emulate8080Op();
   int Disassemble8080Op();
   void display();

   uint8_t immediate(uint8_t byte = 1)
   {
      return memory[Reg.pc + byte];
   }

   uint16_t address()
   {
      return (immediate(2) << 8) | (immediate(1) << 0);
   }

   uint8_t &reg(uint8_t code)
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

private:
   bool interrupt_enabled;
   bool stopped = false;
};