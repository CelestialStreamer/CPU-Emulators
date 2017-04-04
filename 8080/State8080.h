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
         uint8_t c : 1; // Carry             An addition operation that results in a carry out of the high-order bit will set the Carry bit; an addition operation that could have resulted in a carry out but did not will reset the Carry bit.
         uint8_t p : 1; // Parity            The Parity bit is set to 1 for even parity, and is reset to 0 for odd parity.
         uint8_t a : 1; // Auxiliary Carry   The Auxiliary Carry bit indicates carry out of bit 3.
         uint8_t z : 1; // Zero              This conditioin bit is set if the result generated . . . is zero. The Zero bit is reset if the result is not zero.
         uint8_t s : 1; // Sign              At the conclusion of certain instructions, . . . the Sign bit will be set to the condition of the most significant bit of the answer (bit 7).
      }f = { 0,0,0,0,0 };

      uint8_t b = 0;
      uint8_t c = 0;

      uint8_t d = 0;
      uint8_t e = 0;

      uint8_t h = 0;
      uint8_t l = 0;

      uint16_t pc = 0;
      uint16_t sp = 0;
   } Reg;

   State8080()
   {
      memory = new uint8_t[0xffff];
      for (int i = 0; i < 0xffff; i++)
         memory[i] = 0;
   }
   ~State8080() { delete memory; }

   uint8_t input[0xff];
   uint8_t output[0xff];

   uint8_t *memory;
   uint8_t int_enable;
   void Emulate8080Op();
   int Disassemble8080Op();
   void display();

   uint8_t immediate()
   {
      return memory[Reg.pc + 1];
   }

   uint16_t address()
   {
      uint8_t *opcode = &memory[Reg.pc];
      uint16_t adr =
         (((uint16_t)opcode[2]) << 8) |
         (((uint16_t)opcode[1]) << 0);
      return adr;
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
      case 6: return M();
      default: return Reg.a;
      }
   }

   uint16_t rp(uint8_t code)
   {
      switch (code)
      {
      case 0: // B
         return Reg.b << 8 | Reg.c;
      case 1: // D
         return Reg.d << 8 | Reg.e;
      case 2: // H
         return Reg.h << 8 | Reg.l;
      default: // SP
         return Reg.sp;
      }
   }

   uint8_t& M()
   {
      return memory[(Reg.h << 8) | (Reg.l << 0)];
   }

   bool stopped = false;

};