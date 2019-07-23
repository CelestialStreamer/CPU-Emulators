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
   struct Reg {
      union {
         uint16_t psw; // Used for push/pop
         struct {
            uint8_t flagByte; // Used for push/pop
            uint8_t a = 0;
         };
      };
      struct ConditionCodes {
         // 7 6 5 4 3 2 1 0
         // S Z 0 A 0 P 1 C
         uint8_t c; // Carry
         uint8_t p; // Parity
         uint8_t a; // Auxiliary Carry
         uint8_t z; // Zero
         uint8_t s; // Sign
      }f = { RESET, RESET, RESET, RESET, RESET };
      union { uint16_t bc = 0; struct { uint8_t b, c; }; };
      union { uint16_t de = 0; struct { uint8_t d, e; }; };
      union { uint16_t hl = 0; struct { uint8_t h, l; }; };
      uint16_t pc = 0, sp = 0;
   } Reg;

   uint8_t memory[0xffff] = {};
   template<typename T> T& mem(int address) {
      return *(*T)(&memory[address]);
   }

   void Emulate8080Op();
   int  Disassemble8080Op();
   void display();

   template<typename T> T imm() {
      // I don't know if this is correct for when interrupts happen
      T value = mem<T>(Reg.pc);
      Reg.pc += sizeof(T);
      return value;
   }

   void generateInterrupt(uint8_t opcode);

private:
   IO io;
   bool interrupt_enabled = false;  // Are we ready to take interrupts?
   bool interruptRequested = false; // Is there an interrupt now?
   unsigned char interruptOpcode = 0;
   bool stopped = false;

   void INR(uint8_t& x);
   void DCR(uint8_t& x);

   void DAA();

   void ADD(uint8_t value);
   void ADC(uint8_t value);
   void SUB(uint8_t value);
   void SBB(uint8_t value);
   void ANA(uint8_t value);
   void XRA(uint8_t value);
   void ORA(uint8_t value);
   void CMP(uint8_t value);

   void PUSH(uint16_t val);
   uint16_t POP();

   void DAD(uint32_t rp);

   void CALL(uint16_t address);
   void RET();
};
