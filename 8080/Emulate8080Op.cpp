#include "State8080.h"

#define FOR_CPUDIAG
#define DEBUG


void INR(State8080* state, uint8_t reg)
{
   // value = value + 1
   // Z, S, P, AC
   uint8_t value = state->reg(reg);
   uint8_t x = value + 1;

   state->Reg.f.z = (x == 0); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x); // Parity flag
   state->Reg.f.a = ((((value & 0x0f) + 1) & 0x10) == 0x10); // Auxiliary Carry flag

   state->reg(reg) = x;
}

void DCR(State8080* state, uint8_t reg)
{
   // value = value - 1
   // Z, S, P, AC
   uint8_t value = state->reg(reg);
   uint8_t x = value - 1;

   state->Reg.f.z = (x == 0); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x); // Parity flag
   state->Reg.f.a = ((value & 0x0f) < 1); // Half Carry flag

   state->reg(reg) = x;
}

void MOV(State8080* state, uint8_t opcode)
{
   uint8_t dst = (opcode >> 3) & 0x7; // 01DSTsrc
   uint8_t src = (opcode >> 0) & 0x7; // 01dstSRC

   state->reg(dst) = state->reg(src);
}

void ADD(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint16_t answer = (uint16_t)state->Reg.a + (uint16_t)value; // Emulate 8-bit addition using 16-bit numbers

   state->Reg.f.z = ((answer & 0xff) == 0); // Zero flag
   state->Reg.f.s = ((answer & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(answer & 0xff); // Parity flag
   state->Reg.f.c = ((answer & 0x100) == 0x100); // Carry flag
   state->Reg.f.a = ((((state->Reg.a & 0xf) + (value & 0xf)) & 0x10) == 0x10); // Auxiliary Carry flag

   state->Reg.a = answer & 0xff;
}
void ADC(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint16_t answer = (uint16_t)state->Reg.a + (uint16_t)value + (uint16_t)state->Reg.f.c; // Emulate 8-bit addition using 16-bit numbers

   state->Reg.f.z = ((answer & 0xff) == 0); // Zero flag
   state->Reg.f.s = ((answer & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(answer & 0xff); // Parity flag
   state->Reg.f.c = ((answer & 0x100) == 0x100); // Carry flag
   state->Reg.f.a = ((((state->Reg.a & 0xf) + (value & 0xf) + (state->Reg.f.c)) & 0x10) == 0x10); // Half Carry flag

   state->Reg.a = answer & 0xff;
}
void SUB(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint16_t x = (uint16_t)state->Reg.a + (uint16_t)(~value + 1); // Emulate 8-bit subtraction using 16-bit numbers

   state->Reg.f.z = ((x & 0xff) == 0x00); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x & 0xff); // Parity flag
   state->Reg.f.c = ((x & 0x100) == 0x100); // Carry flag
   state->Reg.f.a = ((((state->Reg.a & 0xf) + ((~value + 1) & 0xf)) & 0x10) == 0x10); // Auxiliary Carry flag

   state->Reg.a = x & 0xff;
}
void SBB(State8080* state, uint8_t value)
{
   /// The Carry bit is internally added to the contents of the specified byte.
   /// This value is then subtracted from the accumulator . . ..
   SUB(state, value + state->Reg.f.c);
}
void ANA(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint8_t x = state->Reg.a & value;

   state->Reg.f.z = (x == 0); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x); // Parity flag
   state->Reg.f.c = 0; // Carry flag
   state->Reg.f.a = 0; // Half Carry flag

   state->Reg.a = x;
}
void XRA(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint8_t x = state->Reg.a ^ value;

   state->Reg.f.z = (x == 0); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x); // Parity flag
   state->Reg.f.c = 0; // Carry flag
   state->Reg.f.a = 0; // Half Carry flag

   state->Reg.a = x;
}
void ORA(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint8_t x = state->Reg.a | value;

   state->Reg.f.z = (x == 0); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x); // Parity flag
   state->Reg.f.c = 0; // Carry flag
   state->Reg.f.a = 0; // Half Carry flag

   state->Reg.a = x;
}
void CMP(State8080* state, uint8_t value)
{
   //uint8_t value = state->reg(reg);
   uint16_t x = (uint16_t)state->Reg.a + (uint16_t)(~value + 1);

   state->Reg.f.z = ((x & 0xff) == 0x00); // Zero flag
   state->Reg.f.s = ((x & 0x80) == 0x80); // Sign flag
   state->Reg.f.p = parity(x & 0xff); // Parity flag
   state->Reg.f.c = ((x & 0x100) == 0x100); // Carry flag
   state->Reg.f.a = ((((state->Reg.a & 0xf) + ((~value + 1) & 0xf)) & 0x10) == 0x10); // Auxiliary Carry flag
}
void(*math[])(State8080* state, uint8_t reg) = { ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP };

void RET(State8080* state)
{
   state->Reg.pc
      = (((uint16_t)state->memory[state->Reg.sp + 0]) << 0)  // PC.lo <- (sp);
      | (((uint16_t)state->memory[state->Reg.sp + 1]) << 8); // PC.hi<-(sp+1);
   state->Reg.sp = state->Reg.sp + 2; // SP <- SP+2
}

void CALL(State8080* state, uint16_t value)
{
   uint16_t ret = state->Reg.pc + 3; // ret is address of next operation
   state->memory[state->Reg.sp - 1] = (ret >> 8) & 0xff; // (SP-1) = PC.hi
   state->memory[state->Reg.sp - 2] = (ret >> 0) & 0xff; // (SP-2) = PC.lo
   state->Reg.sp = state->Reg.sp - 2;                        // SP = SP - 2
   state->Reg.pc = value;
}

void RST(State8080* state, uint16_t value)
{
   uint16_t ret = state->Reg.pc + 1; // ret is address of next operation
   state->memory[state->Reg.sp - 1] = (ret >> 8) & 0xff; // (SP-1) = PC.hi
   state->memory[state->Reg.sp - 2] = (ret >> 0) & 0xff; // (SP-2) = PC.lo
   state->Reg.sp = state->Reg.sp - 2;                        // SP = SP - 2
   state->Reg.pc = value;
}

void State8080::Emulate8080Op()
{
   unsigned char *opcode = &memory[Reg.pc];

   Disassemble8080Op();

   if (stopped)
      return;

   switch (*opcode)
   {          // Opcode Instruction size  flags          function
   // CARRY BIT INSTRUCTIONS: CMC, STC
   case 0x3F: // 0x3f   CMC         1     CY             CY=!CY
   {
      // 4 cycles
      Reg.f.c = !Reg.f.c;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x37: // 0x37   STC         1     CY             CY = 1
   {
      // 4 cycles
      Reg.f.c = 1;
      Reg.pc = Reg.pc + 1;
      break;
   }

   // SINGLE REGISTER INSTRUCTIONS: INR, DCR, CMA, DAA
   case 0x04: // 0x04   INR B       1     Z S P AC       B <- B+1
   case 0x0C: // 0x0c   INR C       1     Z S P AC       C <- C+1
   case 0x14: // 0x14   INR D       1     Z S P AC       D <- D+1
   case 0x1C: // 0x1c   INR E       1     Z S P AC       E <- E+1
   case 0x24: // 0x24   INR H       1     Z S P AC       H <- H+1
   case 0x2C: // 0x2c   INR L       1     Z S P AC       L <- L+1
   case 0x34: // 0x34   INR M       1     Z S P AC       (HL) <- (HL)+1
   case 0x3C: // 0x3c   INR A       1     Z S P AC       A <- A+1
   {
      // 5 cycles
      INR(this, (*opcode >> 3 ) & 7);
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0x05: // 0x05   DCR B       1     Z S P AC       B <- B-1
   case 0x0D: // 0x0d   DCR C       1     Z S P AC       C <- C-1
   case 0x15: // 0x15   DCR D       1     Z S P AC       D <- D-1
   case 0x1D: // 0x1d   DCR E       1     Z S P AC       E <- E-1
   case 0x25: // 0x25   DCR H       1     Z S P AC       H <- H-1
   case 0x2D: // 0x2d   DCR L       1     Z S P AC       L <- L-1
   case 0x35: // 0x35   DCR M       1     Z S P AC       (HL) <- (HL)-1
   case 0x3D: // 0x3d   DCR A       1     Z S P AC       A <- A-1
   {
      // 5 cycles
      DCR(this, (*opcode >> 3) & 7);
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0x2F: // 0x2f   CMA         1                    A <- !A
   {
      // 4 cycles
      Reg.a = ~Reg.a;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x27: // 0x27   DAA         1     Z S P CY AC    special
   {
      // 4 cycles

      /// DAA Decimal Adjust Accumulator

      ///      Description: The eight-bit hexadecimal number in the
      /// accumulator is adjusted to form two four-bit binary-coded-
      /// decimal digits by the following two step process:

      /// Step (1)
      /// If the (lower) four bits of the accumulator (are) greater than 9,
      /// or if the Auxiliary Carry bit is equal to one,
      /// the accumulator is incremented by six.
      /// Otherwise, no incrementing occurs.
      if (((Reg.a & 0x0f) > 0x09) || (Reg.f.a == 1))
      {
         uint8_t x = (Reg.a & 0x0f) + 0x06;
         // Auxiliary Carry flag set/reset if carry out on low 4 bits did/not occur
         Reg.f.a = ((x & 0x10) == 0x10);
         Reg.a = Reg.a + 0x06;
      }
      else
         // TODO: Is the following line of code correct? Everything else is fine.
         Reg.f.a = 0; // I guess no carry out occured, so AC is reset.

                         /// Step (2)
                         /// If the (greater) four bits of the accumulator now (are) greater than 9,
                         /// or if the normal carry bit is equal to one,
                         /// the most significant four bits of the accumulator are incremented by six.
                         /// Otherwise, no incrementing occurs.
      if (((Reg.a & 0xf0) > 0x90) || (Reg.f.c == 1))
      {
         uint16_t x = (uint16_t)(Reg.a & 0xf0) + (uint16_t)0x60;
         // Carry flag set/reset if carry out on high 4 bits did/not occur
         Reg.f.c = ((x & 0x100) == 0x100);
         Reg.a = Reg.a + 0x60;
      }

      /// If a carry out of the least significant four bits occurs during Step (1),
      /// the Auxiliary Carry bit is set; otherwise it is reset.
      /// Likewise, if a carry out of the most significant four bits occur during Step (2),
      /// the normal Carry bit is set; otherwise, it is unaffected.

      Reg.f.z = (Reg.a == 0); // Zero flag
      Reg.f.s = ((Reg.a & 0x80) == 0x80); // Sign flag
      Reg.f.p = parity(Reg.a); // Parity flag

      Reg.pc = Reg.pc + 1;
      break;
   }

   // NOP INSTRUCTION
   case 0x00: // 0x00   NOP         1
   {
      // 4 cycles
      Reg.pc = Reg.pc + 1;
      break;
   }

   // DATA TRANSFER INSTRUCTIONS: MOV, STAX, LDAX
   case 0x40: // 0x40   MOV BB      1                    B <- B
   case 0x41: // 0x41   MOV BC      1                    B <- C
   case 0x42: // 0x42   MOV BD      1                    B <- D
   case 0x43: // 0x43   MOV BE      1                    B <- E
   case 0x44: // 0x44   MOV BH      1                    B <- H
   case 0x45: // 0x45   MOV BL      1                    B <- L
   case 0x46: // 0x46   MOV BM      1                    B <- (HL)
   case 0x47: // 0x47   MOV BA      1                    B <- A
   case 0x48: // 0x48   MOV CB      1                    C <- B
   case 0x49: // 0x49   MOV CC      1                    C <- C
   case 0x4A: // 0x4a   MOV CD      1                    C <- D
   case 0x4B: // 0x4b   MOV CE      1                    C <- E
   case 0x4C: // 0x4c   MOV CH      1                    C <- H
   case 0x4D: // 0x4d   MOV CL      1                    C <- L
   case 0x4E: // 0x4e   MOV CM      1                    C <- (HL)
   case 0x4F: // 0x4f   MOV CA      1                    C <- A
   case 0x50: // 0x50   MOV DB      1                    D <- B
   case 0x51: // 0x51   MOV DC      1                    D <- C
   case 0x52: // 0x52   MOV DD      1                    D <- D
   case 0x53: // 0x53   MOV DE      1                    D <- E
   case 0x54: // 0x54   MOV DH      1                    D <- H
   case 0x55: // 0x55   MOV DL      1                    D <- L
   case 0x56: // 0x56   MOV DM      1                    D <- (HL)
   case 0x57: // 0x57   MOV DA      1                    D <- A
   case 0x58: // 0x58   MOV EB      1                    E <- B
   case 0x59: // 0x59   MOV EC      1                    E <- C
   case 0x5A: // 0x5a   MOV ED      1                    E <- D
   case 0x5B: // 0x5b   MOV EE      1                    E <- E
   case 0x5C: // 0x5c   MOV EH      1                    E <- H
   case 0x5D: // 0x5d   MOV EL      1                    E <- L
   case 0x5E: // 0x5e   MOV EM      1                    E <- (HL)
   case 0x5F: // 0x5f   MOV EA      1                    E <- A
   case 0x60: // 0x60   MOV HB      1                    H <- B
   case 0x61: // 0x61   MOV HC      1                    H <- C
   case 0x62: // 0x62   MOV HD      1                    H <- D
   case 0x63: // 0x63   MOV HE      1                    H <- E
   case 0x64: // 0x64   MOV HH      1                    H <- H
   case 0x65: // 0x65   MOV HL      1                    H <- L
   case 0x66: // 0x66   MOV HM      1                    H <- (HL)
   case 0x67: // 0x67   MOV HA      1                    H <- A
   case 0x68: // 0x68   MOV LB      1                    L <- B
   case 0x69: // 0x69   MOV LC      1                    L <- C
   case 0x6A: // 0x6a   MOV LD      1                    L <- D
   case 0x6B: // 0x6b   MOV LE      1                    L <- E
   case 0x6C: // 0x6c   MOV LH      1                    L <- H
   case 0x6D: // 0x6d   MOV LL      1                    L <- L
   case 0x6E: // 0x6e   MOV LM      1                    L <- (HL)
   case 0x6F: // 0x6f   MOV LA      1                    L <- A
   case 0x70: // 0x70   MOV MB      1                    (HL) <- B
   case 0x71: // 0x71   MOV MC      1                    (HL) <- C
   case 0x72: // 0x72   MOV MD      1                    (HL) <- D
   case 0x73: // 0x73   MOV ME      1                    (HL) <- E
   case 0x74: // 0x74   MOV MH      1                    (HL) <- H
   case 0x75: // 0x75   MOV ML      1                    (HL) <- L
   case 0x77: // 0x77   MOV MA      1                    (HL) <- A
   case 0x78: // 0x78   MOV AB      1                    A <- B
   case 0x79: // 0x79   MOV AC      1                    A <- C
   case 0x7A: // 0x7a   MOV AD      1                    A <- D
   case 0x7B: // 0x7b   MOV AE      1                    A <- E
   case 0x7C: // 0x7c   MOV AH      1                    A <- H
   case 0x7D: // 0x7d   MOV AL      1                    A <- L
   case 0x7E: // 0x7e   MOV AM      1                    A <- (HL)
   case 0x7F: // 0x7f   MOV AA      1                    A <- A
   {
      // ALL MOV opcodes
      // If dst or src = 110B, 7 cycles, else 5 cycles

      uint8_t dst = (*opcode >> 3) & 0x7; // 01DDDSSS
      uint8_t src = (*opcode >> 0) & 0x7; // 01DDDSSS
      reg(dst) = reg(src);

      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0x02: // 0x02   STAX B      1                    (BC) <- A
   {
      // 7 cycles
      memory[(Reg.b << 8) | (Reg.c << 0)] = Reg.a;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x12: // 0x12   STAX D      1                    (DE) <- A
   {
      // 7 cycles
      memory[(Reg.d << 8) | (Reg.e << 0)] = Reg.a;
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0x0A: // 0x0a   LDAX B      1                    A <- (BC)
   {
      // 7 cycles
      Reg.a = memory[(Reg.b << 8) | (Reg.c)];
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x1A: // 0x1a   LDAX D      1                    A <- (DE)
   {
      // 7 cycles
      Reg.a = memory[(Reg.d << 8) | (Reg.e)];
      Reg.pc = Reg.pc + 1;
      break;
   }

   // REGISTER OR MEMORY TO ACCUMULATOR INSTRUCTIONS: ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP
   case 0x80: // 0x80   ADD B       1     Z S P CY AC    A <- A + B
   case 0x81: // 0x81   ADD C       1     Z S P CY AC    A <- A + C
   case 0x82: // 0x82   ADD D       1     Z S P CY AC    A <- A + D
   case 0x83: // 0x83   ADD E       1     Z S P CY AC    A <- A + E
   case 0x84: // 0x84   ADD H       1     Z S P CY AC    A <- A + H
   case 0x85: // 0x85   ADD L       1     Z S P CY AC    A <- A + L
   case 0x86: // 0x86   ADD M       1     Z S P CY AC    A <- A + (HL)
   case 0x87: // 0x87   ADD A       1     Z S P CY AC    A <- A + A
   case 0x88: // 0x88   ADC B       1     Z S P CY AC    A <- A + B + CY
   case 0x89: // 0x89   ADC C       1     Z S P CY AC    A <- A + C + CY
   case 0x8A: // 0x8a   ADC D       1     Z S P CY AC    A <- A + D + CY
   case 0x8B: // 0x8b   ADC E       1     Z S P CY AC    A <- A + E + CY
   case 0x8C: // 0x8c   ADC H       1     Z S P CY AC    A <- A + H + CY
   case 0x8D: // 0x8d   ADC L       1     Z S P CY AC    A <- A + L + CY
   case 0x8E: // 0x8e   ADC M       1     Z S P CY AC    A <- A + (HL) + CY
   case 0x8F: // 0x8f   ADC A       1     Z S P CY AC    A <- A + A + CY
   case 0x90: // 0x90   SUB B       1     Z S P CY AC    A <- A - B
   case 0x91: // 0x91   SUB C       1     Z S P CY AC    A <- A - C
   case 0x92: // 0x92   SUB D       1     Z S P CY AC    A <- A + D
   case 0x93: // 0x93   SUB E       1     Z S P CY AC    A <- A - E
   case 0x94: // 0x94   SUB H       1     Z S P CY AC    A <- A + H
   case 0x95: // 0x95   SUB L       1     Z S P CY AC    A <- A - L
   case 0x96: // 0x96   SUB M       1     Z S P CY AC    A <- A + (HL)
   case 0x97: // 0x97   SUB A       1     Z S P CY AC    A <- A - A
   case 0x98: // 0x98   SBB B       1     Z S P CY AC    A <- A - B - CY
   case 0x99: // 0x99   SBB C       1     Z S P CY AC    A <- A - C - CY
   case 0x9A: // 0x9a   SBB D       1     Z S P CY AC    A <- A - D - CY
   case 0x9B: // 0x9b   SBB E       1     Z S P CY AC    A <- A - E - CY
   case 0x9C: // 0x9c   SBB H       1     Z S P CY AC    A <- A - H - CY
   case 0x9D: // 0x9d   SBB L       1     Z S P CY AC    A <- A - L - CY
   case 0x9E: // 0x9e   SBB M       1     Z S P CY AC    A <- A - (HL) - CY
   case 0x9F: // 0x9f   SBB A       1     Z S P CY AC    A <- A - A - CY
   case 0xA0: // 0xa0   ANA B       1     Z S P CY AC    A <- A & B
   case 0xA1: // 0xa1   ANA C       1     Z S P CY AC    A <- A & C
   case 0xA2: // 0xa2   ANA D       1     Z S P CY AC    A <- A & D
   case 0xA3: // 0xa3   ANA E       1     Z S P CY AC    A <- A & E
   case 0xA4: // 0xa4   ANA H       1     Z S P CY AC    A <- A & H
   case 0xA5: // 0xa5   ANA L       1     Z S P CY AC    A <- A & L
   case 0xA6: // 0xa6   ANA M       1     Z S P CY AC    A <- A & (HL)
   case 0xA7: // 0xa7   ANA A       1     Z S P CY AC    A <- A & A
   case 0xA8: // 0xa8   XRA B       1     Z S P CY AC    A <- A ^ B
   case 0xA9: // 0xa9   XRA C       1     Z S P CY AC    A <- A ^ C
   case 0xAA: // 0xaa   XRA D       1     Z S P CY AC    A <- A ^ D
   case 0xAB: // 0xab   XRA E       1     Z S P CY AC    A <- A ^ E
   case 0xAC: // 0xac   XRA H       1     Z S P CY AC    A <- A ^ H
   case 0xAD: // 0xad   XRA L       1     Z S P CY AC    A <- A ^ L
   case 0xAE: // 0xae   XRA M       1     Z S P CY AC    A <- A ^ (HL)
   case 0xAF: // 0xaf   XRA A       1     Z S P CY AC    A <- A ^ A
   case 0xB0: // 0xb0   ORA B       1     Z S P CY AC    A <- A | B
   case 0xB1: // 0xb1   ORA C       1     Z S P CY AC    A <- A | C
   case 0xB2: // 0xb2   ORA D       1     Z S P CY AC    A <- A | D
   case 0xB3: // 0xb3   ORA E       1     Z S P CY AC    A <- A | E
   case 0xB4: // 0xb4   ORA H       1     Z S P CY AC    A <- A | H
   case 0xB5: // 0xb5   ORA L       1     Z S P CY AC    A <- A | L
   case 0xB6: // 0xb6   ORA M       1     Z S P CY AC    A <- A | (HL)
   case 0xB7: // 0xb7   ORA A       1     Z S P CY AC    A <- A | A
   case 0xB8: // 0xb8   CMP B       1     Z S P CY AC    A - B
   case 0xB9: // 0xb9   CMP C       1     Z S P CY AC    A - C
   case 0xBA: // 0xba   CMP D       1     Z S P CY AC    A - D
   case 0xBB: // 0xbb   CMP E       1     Z S P CY AC    A - E
   case 0xBC: // 0xbc   CMP H       1     Z S P CY AC    A - H
   case 0xBD: // 0xbd   CMP L       1     Z S P CY AC    A - L
   case 0xBE: // 0xbe   CMP M       1     Z S P CY AC    A - (HL)
   case 0xBF: // 0xbf   CMP A       1     Z S P CY AC    A - A
   {
      uint8_t op = (*opcode >> 3) & 0x7;
      uint8_t reg = (*opcode >> 0) & 0x7;
      math[op](this, this->reg(reg));

      Reg.pc = Reg.pc + 1;
      break;
   }

   // ROTATE ACCUMULATOR INSTRUCTIONS: RLC, RRC, RAL, RAR
   case 0x07: // 0x07   RLC         1     CY             A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
   {
      // 4 cycles
      Reg.f.c = ((Reg.a & 0x80) == 0x80); /// Carry bit is set equal to the high-order bit of the accumulator
      Reg.a = ((Reg.a << 1) & 0xfe) | ((Reg.a >> 7) & 0x01);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x0F: // 0x0f   RRC         1     CY             A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
   {
      // 4 cycles
      Reg.f.c = ((Reg.a & 0x01) == 0x01); /// Carry bit is set equal to the low-order bit of the accumulator
      Reg.a = ((Reg.a >> 1) & 0x7f) | ((Reg.a << 7) & 0x80);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x17: // 0x17   RAL         1     CY             A = A << 1; bit 0 = prev CY; CY = prev bit 7
   {
      // 4 cycles
      uint8_t temp = Reg.a;
      Reg.a = ((Reg.a << 1) & 0xfe) | (Reg.f.c);
      Reg.f.c = ((temp & 0x80) == 0x80);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x1F: // 0x1f   RAR         1     CY             A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
   {
      // 4 cycles
      uint8_t temp = Reg.a;
      Reg.a = ((Reg.a >> 1) & 0x7f) | (Reg.f.c << 7);
      Reg.f.c = ((temp & 0x01) == 0x01);
      Reg.pc = Reg.pc + 1;
      break;
   }

   // REGISTER PAIR INSTRUCTIONS: PUSH, POP, DAD INX, DCX, XCHG, XTHL, SPHL
   case 0xC5: // 0xc5   PUSH B      1                    (sp-2)<-C; (sp-1)<-B; sp <- sp - 2
   {
      // From manual STACK PUSH OPERATION

      // 11 cycles
      memory[Reg.sp - 1] = Reg.b;
      memory[Reg.sp - 2] = Reg.c;
      Reg.sp = Reg.sp - 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xD5: // 0xd5   PUSH D      1                    (sp-2)<-E; (sp-1)<-D; sp <- sp - 2
   {
      // 11 cycles
      memory[Reg.sp - 1] = Reg.d;
      memory[Reg.sp - 2] = Reg.e;
      Reg.sp = Reg.sp - 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xE5: // 0xe5   PUSH H      1                    (sp-2)<-L; (sp-1)<-H; sp <- sp - 2
   {
      // 11 cycles
      memory[Reg.sp - 1] = Reg.h;
      memory[Reg.sp - 2] = Reg.l;
      Reg.sp = Reg.sp - 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xF5: // 0xf5   PUSH PSW    1                    (sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
   {
      // 11 cycles
      memory[Reg.sp - 1] = Reg.a;
      memory[Reg.sp - 2]
         = (Reg.f.s << 7)
         | (Reg.f.z << 6)
         | (0 << 5)
         | (Reg.f.a << 4)
         | (0 << 3)
         | (Reg.f.p << 2)
         | (1 << 1)
         | (Reg.f.c << 0);
      Reg.sp = Reg.sp - 2;
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0xC1: // 0xc1   POP B       1                    C <- (sp); B <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      Reg.c = memory[Reg.sp + 0];
      Reg.b = memory[Reg.sp + 1];
      Reg.sp = Reg.sp + 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xD1: // 0xd1   POP D       1                    E <- (sp); D <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      Reg.d = memory[Reg.sp + 1];
      Reg.e = memory[Reg.sp + 0];
      Reg.sp = Reg.sp + 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xE1: // 0xe1   POP H       1                    L <- (sp); H <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      Reg.h = memory[Reg.sp + 1];
      Reg.l = memory[Reg.sp + 0];
      Reg.sp = Reg.sp + 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xF1: // 0xf1   POP PSW     1     Z S P CY AC    flags <- (sp); A <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      //Reg.f = *((ConditionCodes*)&memory[Reg.sp + 0]); // Restore flags from memory
      uint8_t flags = memory[Reg.sp + 0];
      Reg.f.s = ((flags & (1 << 7)) == (1 << 7));
      Reg.f.z = ((flags & (1 << 6)) == (1 << 6));
      Reg.f.a = ((flags & (1 << 4)) == (1 << 4));
      Reg.f.p = ((flags & (1 << 2)) == (1 << 2));
      Reg.f.c = ((flags & (1 << 0)) == (1 << 0));

      Reg.a = memory[Reg.sp + 1];
      Reg.sp = Reg.sp + 2;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x09: // 0x09   DAD B       1     CY             HL = HL + BC
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(Reg.h << 8) | (uint32_t)Reg.l;
      uint32_t bc = (uint32_t)(Reg.b << 8) | (uint32_t)Reg.c;
      uint32_t res = hl + bc;
      Reg.h = (res & 0xff00) >> 8;
      Reg.l = (res & 0x00ff) >> 0;
      Reg.f.c = ((res & 0x10000) == 0x10000);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x19: // 0x19   DAD D       1     CY             HL = HL + DE
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(Reg.h << 8) | (uint32_t)Reg.l;
      uint32_t de = (uint32_t)(Reg.d << 8) | (uint32_t)Reg.e;
      uint32_t res = hl + de;
      Reg.h = (res & 0xff00) >> 8;
      Reg.l = (res & 0x00ff) >> 0;
      Reg.f.c = ((res & 0x10000) == 0x10000);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x29: // 0x29   DAD H       1     CY             HL = HL + HL
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(Reg.h << 8) | (uint32_t)Reg.l;
      uint32_t res = hl + hl;
      Reg.h = (res & 0xff00) >> 8;
      Reg.l = (res & 0x00ff) >> 0;
      Reg.f.c = ((res & 0x10000) == 0x10000);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x39: // 0x39   DAD SP      1     CY             HL = HL + SP
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(Reg.h << 8) | (uint32_t)Reg.l;
      uint32_t res = hl + (uint32_t)Reg.sp;
      Reg.h = (res & 0xff00) >> 8;
      Reg.l = (res & 0x00ff) >> 0;
      Reg.f.c = ((res & 0x10000) == 0x10000);
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0x03: // 0x03   INX B       1                    BC <- BC+1
   {
      // 5 cycles
      Reg.c = Reg.c + 1;
      if (Reg.c == 0)
         Reg.b = Reg.b + 1;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x13: // 0x13   INX D       1                    DE <- DE + 1
   {
      // 5 cycles
      Reg.e = Reg.e + 1;
      if (Reg.e == 0)
         Reg.d = Reg.d + 1;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x23: // 0x23   INX H       1                    HL <- HL + 1
   {
      // 5 cycles
      Reg.l = Reg.l + 1;
      if (Reg.l == 0)
         Reg.h = Reg.h + 1;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x33: // 0x33   INX SP      1                    SP = SP + 1
   {
      // 5 cycles
      Reg.sp = Reg.sp + 1;
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0x0B: // 0x0b   DCX B       1                    BC = BC-1
   {
      // 5 cycles
      uint32_t bc = (Reg.b << 8) | (Reg.c);
      uint32_t res = bc - 1;
      Reg.b = (res & 0xff00) >> 8;
      Reg.c = (res & 0x00ff) >> 0;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x1B: // 0x1b   DCX D       1                    DE = DE-1
   {
      // 5 cycles
      uint32_t de = (Reg.d << 8) | (Reg.e);
      uint32_t res = de - 1;
      Reg.d = (res & 0xff00) >> 8;
      Reg.e = (res & 0x00ff) >> 0;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x2B: // 0x2b   DCX H       1                    HL = HL-1
   {
      // 5 cycles
      uint32_t hl = (Reg.h << 8) | (Reg.l);
      uint32_t res = hl - 1;
      Reg.h = (res & 0xff00) >> 8;
      Reg.l = (res & 0x00ff) >> 0;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0x3B: // 0x3b   DCX SP      1                    SP = SP-1
   {
      // 5 cycles
      Reg.sp = Reg.sp - 1;
      Reg.pc = Reg.pc + 1;
      break;
   }

   case 0xEB: // 0xeb   XCHG        1                    H <-> D; L <-> E
   {
      // 5 cycles
      std::swap(Reg.h, Reg.d);
      std::swap(Reg.l, Reg.e);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xE3: // 0xe3   XTHL        1                    L <-> (SP); H <-> (SP+1)
   {
      // 18 cycles (Longest operation!)
      std::swap(Reg.l, memory[Reg.sp + 0]);
      std::swap(Reg.h, memory[Reg.sp + 1]);
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xF9: // 0xf9   SPHL        1                    SP=HL
   {
      // 5 cycles
      Reg.sp = (Reg.h << 8) | (Reg.l);
      Reg.pc = Reg.pc + 1;
      break;
   }

   // IMMEDIATE INSTRUCTIONS: LXI, MVI, ADI, ACI, SUI, SBI, ANI, XRI, ORI, CPI
   case 0x01: // 0x01   LXI BD16    3                    B <- byte 3 C <- byte 2
   {
      // 10 cycles
      Reg.b = opcode[2];
      Reg.c = opcode[1];
      Reg.pc = Reg.pc + 3;
      break;
   }
   case 0x11: // 0x11   LXI DD16    3                    D <- byte 3 E <- byte 2
   {
      // 10 cycles
      Reg.d = opcode[2];
      Reg.e = opcode[1];
      Reg.pc = Reg.pc + 3;
      break;
   }
   case 0x21: // 0x21   LXI HD16    3                    H <- byte 3 L <- byte 2
   {
      // 10 cycles
      Reg.h = opcode[2];
      Reg.l = opcode[1];
      Reg.pc = Reg.pc + 3;
      break;
   }
   case 0x31: // 0x31   LXI SP D16  3                    SP.hi <- byte 3 SP.lo <- byte 2
   {
      // 10 cycles
      Reg.sp = address();
      Reg.pc = Reg.pc + 3;
      break;
   }

   case 0x06: // 0x06   MVI b D8    2                    b <- byte 2
   case 0x0E: // 0x0e   MVI C D8    2                    C <- byte 2
   case 0x16: // 0x16   MVI D D8    2                    D <- byte 2
   case 0x1E: // 0x1e   MVI e D8    2                    E <- byte 2
   case 0x26: // 0x26   MVI H D8    2                    H <- byte 2
   case 0x2E: // 0x2e   MVI L D8    2                    L <- byte 2
   case 0x36: // 0x36   MVI M D8    2                    (HL) <- byte 2
   case 0x3E: // 0x3e   MVI A D8    2                    A <- byte 2
   {
      // 7 cycles
      reg((*opcode >> 3) & 0x7) = opcode[1];
      Reg.pc = Reg.pc + 2;
      break;
   }

   case 0xC6: // 0xc6   ADI D8      2     Z S P CY AC    A <- A + byte
   case 0xCE: // 0xce   ACI D8      2     Z S P CY AC    A <- A + data + CY
   case 0xD6: // 0xd6   SUI D8      2     Z S P CY AC    A <- A - data
   case 0xDE: // 0xde   SBI D8      2     Z S P CY AC    A <- A - data - CY
   case 0xE6: // 0xe6   ANI D8      2     Z S P CY AC    A <- A & data
   case 0xEE: // 0xee   XRI D8      2     Z S P CY AC    A <- A ^ data
   case 0xF6: // 0xf6   ORI D8      2     Z S P CY AC    A <- A | data
   case 0xFE: // 0xfe   CPI D8      2     Z S P CY AC    A - data
   {
      uint8_t op = (*opcode >> 3) & 0x7;
      math[op](this, immediate());

      Reg.pc = Reg.pc + 2;
      break;
   }

   // DIRECT ADDRESSING INSTRUCTIONS: STA, LDA, SHLD, LHLD
   case 0x32: // 0x32   STA adr     3                    (adr) <- A
   {
      // 13 cycles
      uint16_t adr = address();
      memory[adr] = Reg.a;
      Reg.pc = Reg.pc + 3;
      break;
   }
   case 0x3A: // 0x3a   LDA adr     3                    A <- (adr)
   {
      // 13 cycles
      uint16_t adr = address();
      Reg.a = memory[adr];
      Reg.pc = Reg.pc + 3;
      break;
   }
   case 0x22: // 0x22   SHLD adr    3                    (adr) <-L; (adr+1)<-H
   {
      // 16 cycles
      uint16_t adr = address();
      memory[adr + 0] = Reg.l;
      memory[adr + 1] = Reg.h;
      Reg.pc = Reg.pc + 3;
      break;
   }
   case 0x2A: // 0x2a   LHLD adr    3                    L <- (adr); H<-(adr+1)
   {
      // 16 cycles
      uint16_t adr = address();
      Reg.l = memory[adr + 0];
      Reg.h = memory[adr + 1];
      Reg.pc = Reg.pc + 3;
      break;
   }

   // JUMP INSTRUCTIONS: PCHL, JMP, JC, JNC, JZ, JNZ, JM, JP, JPE, JPO
   case 0xE9: // 0xe9   PCHL        1                    Reg.pc.hi <- H; Reg.pc.lo <- L
   {
      // 5 cycles
      Reg.pc
         = (((uint16_t)Reg.h) << 8)
         | (((uint16_t)Reg.l) << 0);
      break;
   }
   case 0xC3: // 0xc3   JMP adr     3                    Reg.pc <- adr
   {
      // 10 cycles
      Reg.pc = address();
      break;
   }
   case 0xDA: // 0xda   JC  adr     3                    if CY Reg.pc<-adr
   {
      // 10 cycles
      if (Reg.f.c == 1)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xD2: // 0xd2   JNC adr     3                    if NCY Reg.pc<-adr
   {
      // 10 cycles
      if (Reg.f.c == 0)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xCA: // 0xca   JZ  adr     3                    if Z Reg.pc <- adr
   {
      // 10 cycles
      if (Reg.f.z == 1)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xC2: // 0xc2   JNZ adr     3                    if NZ Reg.pc <- adr
   {
      // 10 cycles
      if (Reg.f.z == 0)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xFA: // 0xfa   JM  adr     3                    if S=1 Reg.pc <- adr
   {
      // Manual says:
      // "If the sign bit is one (indicating a negative result),
      // program execution continues at the memory adr."

      // 10 cycles
      if (Reg.f.s == 1)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xF2: // 0xf2   JP  adr     3                    if S=0 Reg.pc <- adr
   {
      // Manual says:
      // "If the sign bit is zero, (indicating a positive result),
      // program execution continues at the memory adr."

      // 10 cycles
      if (Reg.f.s == 0)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xEA: // 0xea   JPE adr     3                    if P=1 Reg.pc <- adr
   {
      // Manual says:
      // "If the parity bit is one (indicating a result with even parity),
      // program execution continues at the memory address adr.

      // 10 cycles
      if (Reg.f.p == 1)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xE2: // 0xe2   JPO adr     3                    if P=0 Reg.pc <- adr
   {
      // Manual says:
      // If the parity bit is zero (indicating a result with odd parity),
      // program execution continues at the memory address adr."

      // 10 cycles
      if (Reg.f.p == 0)
         Reg.pc = address();
      else
         Reg.pc = Reg.pc + 3;
      break;
   }

   // CALL SUBROUTINE INSTRUCTIONS: CALL, CC, CNC, CZ, CNZ, CM, CP, CPE, CPO
   case 0xCD: // 0xcd   CALL adr    3                    (SP-1)<-Reg.pc.hi;(SP-2)<-Reg.pc.lo;SP<-SP+2;Reg.pc=adr
   {
#ifdef FOR_CPUDIAG
      if (5 == ((opcode[2] << 8) | opcode[1]))
      {
         if (Reg.c == 9)
         {
            uint16_t offset = (Reg.d << 8) | (Reg.e);
            char *str = (char*)&memory[offset + 3]; // skip the prefix bytes
            while (*str != '$')
               printf("%c", *str++);
            printf("\n");
            exit(0);
         }
         else
         {
            printf("print char routine called\n");
         }
      }
#endif // FOR_CPUDIAG
      // 17 cycles
      CALL(this, address());
      break;
   }
   case 0xDC: // 0xdc   CC a dr     3                    if CY CALL adr
   {
      // 17/11 cycles
      if (Reg.f.c == 1)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xD4: // 0xd4   CNC adr     3                    if NCY CALL adr
   {
      // 17/11 cycles
      if (Reg.f.c == 0)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xCC: // 0xcc   CZ a dr     3                    if Z CALL adr
   {
      /// Description: If the Zero bit is *(one), a call operation is
      /// performed to subroutine sub.

      // 17/11 cycles
      if (Reg.f.z == 1)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xC4: // 0xc4   CNZ adr     3                    if NZ CALL adr
   {
      /// Description: If the Zero bit is *(zero), a call operation is
      /// performed to subroutine sub.

      // 17/11 cycles
      if (Reg.f.z == 0)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xFC: // 0xfc   CM a dr     3                    if S CALL adr
   {
      // 17/11 cycles
      if (Reg.f.s == 1)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xF4: // 0xf4   CP a dr     3                    if NS Reg.pc <- adr
   {
      // 17/11 cycles
      if (Reg.f.s == 0)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xEC: // 0xec   CPE adr     3                    if PE CALL adr
   {
      // 17/11 cycles
      if (Reg.f.p == 1)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }
   case 0xE4: // 0xe4   CPO adr     3                    if PO CALL adr
   {
      // 17/11 cycles
      if (Reg.f.p == 0)
         CALL(this, address());
      else
         Reg.pc = Reg.pc + 3;
      break;
   }

   // RETURN FROM SUBROUTINE INSTRUCTIONS: RET, RN, RNC, RZ, RNZ, RM, RP, RPE, RPO
   case 0xC9: // 0xc9   RET         1                    Reg.pc.lo <- (sp); Reg.pc.hi<-(sp+1); SP <- SP+2
   {
      // 10 cycles
      RET(this);
      break;
   }
   case 0xD8: // 0xd8   RC          1                    if CY RET
   {
      // 11/5 cycles
      if (Reg.f.c == 1)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xD0: // 0xd0   RNC         1                    if NCY RET
   {
      // 11/5 cycles
      if (Reg.f.c == 0)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xC8: // 0xc8   RZ          1                    if Z RET
   {
      // 11/5 cycles
      if (Reg.f.z == 1)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xC0: // 0xc0   RNZ         1                    if NZ RET
   {
      // 11/5 cycles
      if (Reg.f.z == 0)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xF8: // 0xf8   RM          1                    if S RET
   {
      // 11/5 cycles
      if (Reg.f.s == 1)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xF0: // 0xf0   RP          1                    if NS RET
   {
      // 11/5 cycles
      if (Reg.f.s == 0)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xE8: // 0xe8   RPE         1                    if PE RET
   {
      // 11/5 cycles
      if (Reg.f.p == 1)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xE0: // 0xe0   RPO         1                    if PO RET
   {
      // 11/5 cycles
      if (Reg.f.p == 0)
         RET(this);
      else
         Reg.pc = Reg.pc + 1;
      break;
   }

   // RST INSTRUCTION
   case 0xC7: // 0xc7   RST 0       1                    CALL $0
   case 0xCF: // 0xcf   RST 1       1                    CALL $8
   case 0xD7: // 0xd7   RST 2       1                    CALL $10
   case 0xDF: // 0xdf   RST 3       1                    CALL $18
   case 0xE7: // 0xe7   RST 4       1                    CALL $20
   case 0xEF: // 0xef   RST 5       1                    CALL $28
   case 0xF7: // 0xf7   RST 6       1                    CALL $30
   case 0xFF: // 0xff   RST 7       1                    CALL $38
   {
      // 11 cycles
      RST(this, *opcode & 0x38); // *opcode & 0x38 == ((*opcode >> 3) & 0x7) << 3
      break;
   }

   // INTERRUPT FLIP-FLOP INSTRUCTIONS
   case 0xFB: // 0xfb   EI          1                    special
   {
      // 4 cycles
      // Enable Interrupts
      int_enable = true;
      Reg.pc = Reg.pc + 1;
      break;
   }
   case 0xF3: // 0xf3   DI          1                    special
   {
      // 4 cycles
      // Disable Interrupts
      int_enable = false;
      Reg.pc = Reg.pc + 1;
      break;
   }

   // INPUT/OUTPUT INSTRUCTIONS: IN, OUT
   case 0xDB: // 0xdb   IN D 8      2                    special
   {
      // 10 cycles
      // Read input port into A
      uint8_t port = memory[++Reg.pc];
      // Reg.a = port[port];
      Reg.pc = Reg.pc + 2;
      break;
   }
   case 0xD3: // 0xd3   OUT D8      2                    special
   {
      // 10 cycles
      // Write A to ouput port
      uint8_t port = memory[++Reg.pc];
      // port[port] = Reg.a;
      Reg.pc = Reg.pc + 2;
      break;
   }

   // HLT HALT INSTRUCTION
   case 0x76: // 0x76   HLT         1                    special
   {
      // Halt processor
      // 7 cycles
      Reg.pc = Reg.pc + 1;
      stopped = true;
      break;
   }

   // unused
   default: // 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0xcb, 0xd9, 0xdd, 0xed, 0xfd
   {
      // 4 cycles
      Reg.pc = Reg.pc + 1;
      break;
   }
   }
}