#include <cstdint> // uint8_t, uint16_t, uint32_t
#include <iostream>
#include <fstream>
#include <iomanip>
#include <bitset>
#include <algorithm> // std::swap

#define FOR_CPUDIAG
#define DEBUG

uint8_t parity(uint8_t v)
{
   // From manual Parity Bit
   // "The Parity bit is set to 1 for even parity, and is reset to 0 for odd parity."
   if (std::bitset<8>(v).count() % 2 == 0) // If even parity
      return 1;
   else
      return 0;
}

struct ConditionCodes {
   // 7 6 5 4 3 2 1 0
   // S Z 0 A 0 P 1 C
   uint8_t c : 1; // Carry             An addition operation that results in a carry out of the high-order bit will set the Carry bit; an addition operation that could have resulted in a carry out but did not will reset the Carry bit.
   uint8_t p : 1; // Parity            The Parity bit is set to 1 for even parity, and is reset to 0 for odd parity.
   uint8_t a : 1; // Auxiliary Carry   The Auxiliary Carry bit indicates carry out of bit 3.
   uint8_t z : 1; // Zero              This conditioin bit is set if the result generated . . . is zero. The Zero bit is reset if the result is not zero.
   uint8_t s : 1; // Sign              At the conclusion of certain instructions, . . . the Sign bit will be set to the condition of the most significant bit of the answer (bit 7).
};

struct State8080 {
   State8080()
   {
      memory = new uint8_t[0xffff];
      for (int i = 0; i < 0xffff; i++)
         memory[i] = 0;
   }
   ~State8080() { delete memory; }

   uint8_t a = 0;
   ConditionCodes f = { 0,0,0,0,0 };

   uint8_t b = 0;
   uint8_t c = 0;

   uint8_t d = 0;
   uint8_t e = 0;

   uint8_t h = 0;
   uint8_t l = 0;

   uint16_t pc = 0;
   uint16_t sp = 0;

   uint8_t input[0xff];
   uint8_t output[0xff];

   uint8_t *memory;
   uint8_t int_enable;

   uint8_t& M()
   {
      return memory[(h << 8) | (l << 0)];
   }

   uint8_t* reg(uint8_t code)
   {
      switch (code)
      {
      case 0: return &b;
      case 1: return &c;
      case 2: return &d;
      case 3: return &e;
      case 4: return &h;
      case 5: return &l;
      case 6: return &M();
      default: return &a;
      }
   }

   bool stopped = false;
};

uint16_t address(State8080* state, uint8_t* opcode)
{
   uint16_t adr =
      (((uint16_t)opcode[2]) << 8) |
      (((uint16_t)opcode[1]) << 0);
   return adr;
}

void ADC(State8080* state, uint8_t value)
{
   uint16_t answer = (uint16_t)state->a + (uint16_t)value + (uint16_t)state->f.c; // Emulate 8-bit addition using 16-bit numbers

   state->f.z = ((answer & 0xff) == 0); // Zero flag
   state->f.s = ((answer & 0x80) == 0x80); // Sign flag
   state->f.p = parity(answer & 0xff); // Parity flag
   state->f.c = ((answer & 0x100) == 0x100); // Carry flag
   state->f.a = ((((state->a & 0xf) + (value & 0xf) + (state->f.c)) & 0x10) == 0x10); // Half Carry flag

   state->a = answer & 0xff;
}
void ADD(State8080* state, uint8_t value)
{
   uint16_t answer = (uint16_t)state->a + (uint16_t)value; // Emulate 8-bit addition using 16-bit numbers

   state->f.z = ((answer & 0xff) == 0); // Zero flag
   state->f.s = ((answer & 0x80) == 0x80); // Sign flag
   state->f.p = parity(answer & 0xff); // Parity flag
   state->f.c = ((answer & 0x100) == 0x100); // Carry flag
   state->f.a = ((((state->a & 0xf) + (value & 0xf)) & 0x10) == 0x10); // Auxiliary Carry flag

   state->a = answer & 0xff;
}
void ANA(State8080* state, uint8_t value)
{
   uint8_t x = state->a & value;

   state->f.z = (x == 0); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x); // Parity flag
   state->f.c = 0; // Carry flag
   state->f.a = 0; // Half Carry flag

   state->a = x;
}
void CALL(State8080* state, uint16_t value)
{
   uint16_t ret = state->pc + 3; // ret is address of next operation
   state->memory[state->sp - 1] = (ret >> 8) & 0xff; // (SP-1) = PC.hi
   state->memory[state->sp - 2] = (ret >> 0) & 0xff; // (SP-2) = PC.lo
   state->sp = state->sp - 2;                        // SP = SP - 2
   state->pc = value;
}
void RST(State8080* state, uint16_t value)
{
   uint16_t ret = state->pc + 1; // ret is address of next operation
   state->memory[state->sp - 1] = (ret >> 8) & 0xff; // (SP-1) = PC.hi
   state->memory[state->sp - 2] = (ret >> 0) & 0xff; // (SP-2) = PC.lo
   state->sp = state->sp - 2;                        // SP = SP - 2
   state->pc = value;
}
void CMP(State8080* state, uint8_t value)
{
   uint16_t x = (uint16_t)state->a + (uint16_t)(~value + 1);

   state->f.z = ((x & 0xff) == 0x00); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x & 0xff); // Parity flag
   state->f.c = ((x & 0x100) == 0x100); // Carry flag
   state->f.a = ((((state->a & 0xf) + ((~value + 1) & 0xf)) & 0x10) == 0x10); // Auxiliary Carry flag
}
void DCR(State8080* state, uint8_t &value)
{
   // value = value - 1
   // Z, S, P, AC
   uint8_t x = value - 1;

   state->f.z = (x == 0); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x); // Parity flag
   state->f.a = ((value & 0x0f) < 1); // Half Carry flag

   value = x;
}
void INR(State8080* state, uint8_t &value)
{
   // value = value + 1
   // Z, S, P, AC
   uint8_t x = value + 1;

   state->f.z = (x == 0); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x); // Parity flag
   state->f.a = ((((value & 0x0f) + 1) & 0x10) == 0x10); // Auxiliary Carry flag

   value = x;
}
void MOV(State8080* state, uint8_t &source, uint8_t &destination)
{
   destination = source;
}
void ORA(State8080* state, uint8_t value)
{
   uint8_t x = state->a | value;

   state->f.z = (x == 0); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x); // Parity flag
   state->f.c = 0; // Carry flag
   state->f.a = 0; // Half Carry flag

   state->a = x;
}
void RET(State8080* state)
{
   state->pc
      = (((uint16_t)state->memory[state->sp + 0]) << 0)  // PC.lo <- (sp);
      | (((uint16_t)state->memory[state->sp + 1]) << 8); // PC.hi<-(sp+1);
   state->sp = state->sp + 2; // SP <- SP+2
}
void SUB(State8080* state, uint8_t value)
{
   uint16_t x = (uint16_t)state->a + (uint16_t)(~value + 1); // Emulate 8-bit subtraction using 16-bit numbers

   state->f.z = ((x & 0xff) == 0x00); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x & 0xff); // Parity flag
   state->f.c = ((x & 0x100) == 0x100); // Carry flag
   state->f.a = ((((state->a & 0xf) + ((~value + 1) & 0xf)) & 0x10) == 0x10); // Auxiliary Carry flag

   state->a = x & 0xff;
}
void SBB(State8080* state, uint8_t value)
{
   /// The Carry bit is internally added to the contents of the specified byte.
   /// This value is then subtracted from the accumulator . . ..
   SUB(state, value + state->f.c);
}
void XRA(State8080* state, uint8_t value)
{
   uint8_t x = state->a ^ value;

   state->f.z = (x == 0); // Zero flag
   state->f.s = ((x & 0x80) == 0x80); // Sign flag
   state->f.p = parity(x); // Parity flag
   state->f.c = 0; // Carry flag
   state->f.a = 0; // Half Carry flag

   state->a = x;
}

int Disassemble8080Op(unsigned char *codebuffer, int pc)
{
   unsigned char *code = &codebuffer[pc];
   int opbytes = 1;
   printf("%04x ", pc);
   switch (*code)
   {
   case 0x01: opbytes = 3; break;
   case 0x06: opbytes = 2; break;
   case 0x0e: opbytes = 2; break;
   case 0x11: opbytes = 3; break;
   case 0x16: opbytes = 2; break;
   case 0x1e: opbytes = 2; break;
   case 0x21: opbytes = 3; break;
   case 0x22: opbytes = 3; break;
   case 0x26: opbytes = 2; break;
   case 0x2a: opbytes = 3; break;
   case 0x2e: opbytes = 2; break;
   case 0x31: opbytes = 3; break;
   case 0x32: opbytes = 3; break;
   case 0x36: opbytes = 2; break;
   case 0x3a: opbytes = 3; break;
   case 0x3e: opbytes = 2; break;
   case 0xc2: opbytes = 3; break;
   case 0xc3: opbytes = 3; break;
   case 0xc4: opbytes = 3; break;
   case 0xc6: opbytes = 2; break;
   case 0xca: opbytes = 3; break;
   case 0xcb: opbytes = 3; break;
   case 0xcc: opbytes = 3; break;
   case 0xcd: opbytes = 3; break;
   case 0xce: opbytes = 2; break;
   case 0xd2: opbytes = 3; break;
   case 0xd3: opbytes = 2; break;
   case 0xd4: opbytes = 3; break;
   case 0xd6: opbytes = 2; break;
   case 0xda: opbytes = 3; break;
   case 0xdb: opbytes = 2; break;
   case 0xdc: opbytes = 3; break;
   case 0xdd: opbytes = 3; break;
   case 0xde: opbytes = 2; break;
   case 0xe2: opbytes = 3; break;
   case 0xe4: opbytes = 3; break;
   case 0xe6: opbytes = 2; break;
   case 0xea: opbytes = 3; break;
   case 0xec: opbytes = 3; break;
   case 0xed: opbytes = 3; break;
   case 0xee: opbytes = 2; break;
   case 0xf2: opbytes = 3; break;
   case 0xf4: opbytes = 3; break;
   case 0xf6: opbytes = 2; break;
   case 0xfa: opbytes = 3; break;
   case 0xfc: opbytes = 3; break;
   case 0xfd: opbytes = 3; break;
   case 0xfe: opbytes = 2; break;
   }
   if (opbytes == 1)
      printf("%02x       ", code[0]);
   else if (opbytes == 2)
      printf("%02x %02x    ", code[0], code[1]);
   else
      printf("%02x %02x %02x ", code[0], code[1], code[2]);

   switch (*code)
   {
   case 0x00: printf("NOP"); break;
   case 0x01: printf("LXI    B,#$%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x02: printf("STAX   B"); break;
   case 0x03: printf("INX    B"); break;
   case 0x04: printf("INR    B"); break;
   case 0x05: printf("DCR    B"); break;
   case 0x06: printf("MVI    B,#$%02x", code[1]); opbytes = 2; break;
   case 0x07: printf("RLC"); break;
   case 0x08: printf("NOP"); break;
   case 0x09: printf("DAD    B"); break;
   case 0x0a: printf("LDAX   B"); break;
   case 0x0b: printf("DCX    B"); break;
   case 0x0c: printf("INR    C"); break;
   case 0x0d: printf("DCR    C"); break;
   case 0x0e: printf("MVI    C,#$%02x", code[1]); opbytes = 2;	break;
   case 0x0f: printf("RRC"); break;

   case 0x10: printf("NOP"); break;
   case 0x11: printf("LXI    D,#$%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x12: printf("STAX   D"); break;
   case 0x13: printf("INX    D"); break;
   case 0x14: printf("INR    D"); break;
   case 0x15: printf("DCR    D"); break;
   case 0x16: printf("MVI    D,#$%02x", code[1]); opbytes = 2; break;
   case 0x17: printf("RAL"); break;
   case 0x18: printf("NOP"); break;
   case 0x19: printf("DAD    D"); break;
   case 0x1a: printf("LDAX   D"); break;
   case 0x1b: printf("DCX    D"); break;
   case 0x1c: printf("INR    E"); break;
   case 0x1d: printf("DCR    E"); break;
   case 0x1e: printf("MVI    E,#$%02x", code[1]); opbytes = 2; break;
   case 0x1f: printf("RAR"); break;

   case 0x20: printf("NOP"); break;
   case 0x21: printf("LXI    H,#$%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x22: printf("SHLD   $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x23: printf("INX    H"); break;
   case 0x24: printf("INR    H"); break;
   case 0x25: printf("DCR    H"); break;
   case 0x26: printf("MVI    H,#$%02x", code[1]); opbytes = 2; break;
   case 0x27: printf("DAA"); break;
   case 0x28: printf("NOP"); break;
   case 0x29: printf("DAD    H"); break;
   case 0x2a: printf("LHLD   $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x2b: printf("DCX    H"); break;
   case 0x2c: printf("INR    L"); break;
   case 0x2d: printf("DCR    L"); break;
   case 0x2e: printf("MVI    L,#$%02x", code[1]); opbytes = 2; break;
   case 0x2f: printf("CMA"); break;

   case 0x30: printf("NOP"); break;
   case 0x31: printf("LXI    SP,#$%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x32: printf("STA    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x33: printf("INX    SP"); break;
   case 0x34: printf("INR    M"); break;
   case 0x35: printf("DCR    M"); break;
   case 0x36: printf("MVI    M,#$%02x", code[1]); opbytes = 2; break;
   case 0x37: printf("STC"); break;
   case 0x38: printf("NOP"); break;
   case 0x39: printf("DAD    SP"); break;
   case 0x3a: printf("LDA    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0x3b: printf("DCX    SP"); break;
   case 0x3c: printf("INR    A"); break;
   case 0x3d: printf("DCR    A"); break;
   case 0x3e: printf("MVI    A,#$%02x", code[1]); opbytes = 2; break;
   case 0x3f: printf("CMC"); break;

   case 0x40: printf("MOV    B,B"); break;
   case 0x41: printf("MOV    B,C"); break;
   case 0x42: printf("MOV    B,D"); break;
   case 0x43: printf("MOV    B,E"); break;
   case 0x44: printf("MOV    B,H"); break;
   case 0x45: printf("MOV    B,L"); break;
   case 0x46: printf("MOV    B,M"); break;
   case 0x47: printf("MOV    B,A"); break;
   case 0x48: printf("MOV    C,B"); break;
   case 0x49: printf("MOV    C,C"); break;
   case 0x4a: printf("MOV    C,D"); break;
   case 0x4b: printf("MOV    C,E"); break;
   case 0x4c: printf("MOV    C,H"); break;
   case 0x4d: printf("MOV    C,L"); break;
   case 0x4e: printf("MOV    C,M"); break;
   case 0x4f: printf("MOV    C,A"); break;

   case 0x50: printf("MOV    D,B"); break;
   case 0x51: printf("MOV    D,C"); break;
   case 0x52: printf("MOV    D,D"); break;
   case 0x53: printf("MOV    D.E"); break;
   case 0x54: printf("MOV    D,H"); break;
   case 0x55: printf("MOV    D,L"); break;
   case 0x56: printf("MOV    D,M"); break;
   case 0x57: printf("MOV    D,A"); break;
   case 0x58: printf("MOV    E,B"); break;
   case 0x59: printf("MOV    E,C"); break;
   case 0x5a: printf("MOV    E,D"); break;
   case 0x5b: printf("MOV    E,E"); break;
   case 0x5c: printf("MOV    E,H"); break;
   case 0x5d: printf("MOV    E,L"); break;
   case 0x5e: printf("MOV    E,M"); break;
   case 0x5f: printf("MOV    E,A"); break;

   case 0x60: printf("MOV    H,B"); break;
   case 0x61: printf("MOV    H,C"); break;
   case 0x62: printf("MOV    H,D"); break;
   case 0x63: printf("MOV    H.E"); break;
   case 0x64: printf("MOV    H,H"); break;
   case 0x65: printf("MOV    H,L"); break;
   case 0x66: printf("MOV    H,M"); break;
   case 0x67: printf("MOV    H,A"); break;
   case 0x68: printf("MOV    L,B"); break;
   case 0x69: printf("MOV    L,C"); break;
   case 0x6a: printf("MOV    L,D"); break;
   case 0x6b: printf("MOV    L,E"); break;
   case 0x6c: printf("MOV    L,H"); break;
   case 0x6d: printf("MOV    L,L"); break;
   case 0x6e: printf("MOV    L,M"); break;
   case 0x6f: printf("MOV    L,A"); break;

   case 0x70: printf("MOV    M,B"); break;
   case 0x71: printf("MOV    M,C"); break;
   case 0x72: printf("MOV    M,D"); break;
   case 0x73: printf("MOV    M.E"); break;
   case 0x74: printf("MOV    M,H"); break;
   case 0x75: printf("MOV    M,L"); break;
   case 0x76: printf("HLT");        break;
   case 0x77: printf("MOV    M,A"); break;
   case 0x78: printf("MOV    A,B"); break;
   case 0x79: printf("MOV    A,C"); break;
   case 0x7a: printf("MOV    A,D"); break;
   case 0x7b: printf("MOV    A,E"); break;
   case 0x7c: printf("MOV    A,H"); break;
   case 0x7d: printf("MOV    A,L"); break;
   case 0x7e: printf("MOV    A,M"); break;
   case 0x7f: printf("MOV    A,A"); break;

   case 0x80: printf("ADD    B"); break;
   case 0x81: printf("ADD    C"); break;
   case 0x82: printf("ADD    D"); break;
   case 0x83: printf("ADD    E"); break;
   case 0x84: printf("ADD    H"); break;
   case 0x85: printf("ADD    L"); break;
   case 0x86: printf("ADD    M"); break;
   case 0x87: printf("ADD    A"); break;
   case 0x88: printf("ADC    B"); break;
   case 0x89: printf("ADC    C"); break;
   case 0x8a: printf("ADC    D"); break;
   case 0x8b: printf("ADC    E"); break;
   case 0x8c: printf("ADC    H"); break;
   case 0x8d: printf("ADC    L"); break;
   case 0x8e: printf("ADC    M"); break;
   case 0x8f: printf("ADC    A"); break;

   case 0x90: printf("SUB    B"); break;
   case 0x91: printf("SUB    C"); break;
   case 0x92: printf("SUB    D"); break;
   case 0x93: printf("SUB    E"); break;
   case 0x94: printf("SUB    H"); break;
   case 0x95: printf("SUB    L"); break;
   case 0x96: printf("SUB    M"); break;
   case 0x97: printf("SUB    A"); break;
   case 0x98: printf("SBB    B"); break;
   case 0x99: printf("SBB    C"); break;
   case 0x9a: printf("SBB    D"); break;
   case 0x9b: printf("SBB    E"); break;
   case 0x9c: printf("SBB    H"); break;
   case 0x9d: printf("SBB    L"); break;
   case 0x9e: printf("SBB    M"); break;
   case 0x9f: printf("SBB    A"); break;

   case 0xa0: printf("ANA    B"); break;
   case 0xa1: printf("ANA    C"); break;
   case 0xa2: printf("ANA    D"); break;
   case 0xa3: printf("ANA    E"); break;
   case 0xa4: printf("ANA    H"); break;
   case 0xa5: printf("ANA    L"); break;
   case 0xa6: printf("ANA    M"); break;
   case 0xa7: printf("ANA    A"); break;
   case 0xa8: printf("XRA    B"); break;
   case 0xa9: printf("XRA    C"); break;
   case 0xaa: printf("XRA    D"); break;
   case 0xab: printf("XRA    E"); break;
   case 0xac: printf("XRA    H"); break;
   case 0xad: printf("XRA    L"); break;
   case 0xae: printf("XRA    M"); break;
   case 0xaf: printf("XRA    A"); break;

   case 0xb0: printf("ORA    B"); break;
   case 0xb1: printf("ORA    C"); break;
   case 0xb2: printf("ORA    D"); break;
   case 0xb3: printf("ORA    E"); break;
   case 0xb4: printf("ORA    H"); break;
   case 0xb5: printf("ORA    L"); break;
   case 0xb6: printf("ORA    M"); break;
   case 0xb7: printf("ORA    A"); break;
   case 0xb8: printf("CMP    B"); break;
   case 0xb9: printf("CMP    C"); break;
   case 0xba: printf("CMP    D"); break;
   case 0xbb: printf("CMP    E"); break;
   case 0xbc: printf("CMP    H"); break;
   case 0xbd: printf("CMP    L"); break;
   case 0xbe: printf("CMP    M"); break;
   case 0xbf: printf("CMP    A"); break;

   case 0xc0: printf("RNZ"); break;
   case 0xc1: printf("POP    B"); break;
   case 0xc2: printf("JNZ    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xc3: printf("JMP    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xc4: printf("CNZ    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xc5: printf("PUSH   B"); break;
   case 0xc6: printf("ADI    #$%02x", code[1]); opbytes = 2; break;
   case 0xc7: printf("RST    0"); break;
   case 0xc8: printf("RZ"); break;
   case 0xc9: printf("RET"); break;
   case 0xca: printf("JZ     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xcb: printf("JMP    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xcc: printf("CZ     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xcd: printf("CALL   $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xce: printf("ACI    #$%02x", code[1]); opbytes = 2; break;
   case 0xcf: printf("RST    1"); break;

   case 0xd0: printf("RNC"); break;
   case 0xd1: printf("POP    D"); break;
   case 0xd2: printf("JNC    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xd3: printf("OUT    #$%02x", code[1]); opbytes = 2; break;
   case 0xd4: printf("CNC    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xd5: printf("PUSH   D"); break;
   case 0xd6: printf("SUI    #$%02x", code[1]); opbytes = 2; break;
   case 0xd7: printf("RST    2"); break;
   case 0xd8: printf("RC");  break;
   case 0xd9: printf("RET"); break;
   case 0xda: printf("JC     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xdb: printf("IN     #$%02x", code[1]); opbytes = 2; break;
   case 0xdc: printf("CC     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xdd: printf("CALL   $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xde: printf("SBI    #$%02x", code[1]); opbytes = 2; break;
   case 0xdf: printf("RST    3"); break;

   case 0xe0: printf("RPO"); break;
   case 0xe1: printf("POP    H"); break;
   case 0xe2: printf("JPO    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xe3: printf("XTHL"); break;
   case 0xe4: printf("CPO    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xe5: printf("PUSH   H"); break;
   case 0xe6: printf("ANI    #$%02x", code[1]); opbytes = 2; break;
   case 0xe7: printf("RST    4"); break;
   case 0xe8: printf("RPE"); break;
   case 0xe9: printf("PCHL"); break;
   case 0xea: printf("JPE    $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xeb: printf("XCHG"); break;
   case 0xec: printf("CPE     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xed: printf("CALL   $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xee: printf("XRI    #$%02x", code[1]); opbytes = 2; break;
   case 0xef: printf("RST    5"); break;

   case 0xf0: printf("RP");  break;
   case 0xf1: printf("POP    PSW"); break;
   case 0xf2: printf("JP     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xf3: printf("DI");  break;
   case 0xf4: printf("CP     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xf5: printf("PUSH   PSW"); break;
   case 0xf6: printf("ORI    #$%02x", code[1]); opbytes = 2; break;
   case 0xf7: printf("RST    6"); break;
   case 0xf8: printf("RM");  break;
   case 0xf9: printf("SPHL"); break;
   case 0xfa: printf("JM     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xfb: printf("EI");  break;
   case 0xfc: printf("CM     $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xfd: printf("CALL   $%02x%02x", code[2], code[1]); opbytes = 3; break;
   case 0xfe: printf("CPI    #$%02x", code[1]); opbytes = 2; break;
   case 0xff: printf("RST    7"); break;
   }

   return opbytes;
}

void display(State8080* state)
{
   std::bitset<8> ab, bb, cb, db, eb, hb, lb;
   int ad, bd, cd, dd, ed, hd, ld;
   ab = ad = state->a;
   bb = bd = state->b;
   cb = cd = state->c;
   db = dd = state->d;
   eb = ed = state->e;
   hb = hd = state->h;
   lb = ld = state->l;

   std::bitset<8> fb;
   int fd;
   fb = fd = *((uint8_t*)&state->f);

   std::bitset<16> spb, pcb;
   int spd, pcd;
   spb = spd = state->sp;
   pcb = pcd = state->pc;

   std::cout << std::hex << std::endl;

   std::cout << "    FEDCBA98    76543210" << std::endl;

   std::cout << "PSW ";
   std::cout << ab << "=" << std::setw(2) << ad << " ";
   std::cout << fb << " ";
   std::cout << "S=" << (state->f.s == 1 ? "1" : "0") << " ";
   std::cout << "Z=" << (state->f.z == 1 ? "1" : "0") << " ";
   std::cout << "A=" << (state->f.a == 1 ? "1" : "0") << " ";
   std::cout << "P=" << (state->f.p == 1 ? "1" : "0") << " ";
   std::cout << "C=" << (state->f.c == 1 ? "1" : "0") << std::endl;

   std::cout << "B   "
      << bb << "=" << std::setw(2) << bd << " "
      << cb << "=" << std::setw(2) << cd << std::endl;

   std::cout << "D   "
      << db << "=" << std::setw(2) << dd << " "
      << eb << "=" << std::setw(2) << ed << std::endl;

   std::cout << "H   "
      << hb << "=" << std::setw(2) << hd << " "
      << lb << "=" << std::setw(2) << ld << std::endl;


   std::cout << "    FEDCBA9876543210" << std::endl;
   std::cout << "SP  " << spb << "=" << std::setw(4) << spd << std::endl;
   std::cout << "PC  " << pcb << "=" << std::setw(4) << pcd << std::endl;

   std::cout << std::dec;
}

void Emulate8080Op(State8080* state)
{
   unsigned char *opcode = &state->memory[state->pc];

   Disassemble8080Op(state->memory, state->pc);

   if (state->stopped)
      return;

   switch (*opcode)
   {          // Opcode Instruction size  flags          function
// CARRY BIT INSTRUCTIONS: CMC, STC
   case 0x3F: // 0x3f   CMC         1     CY             CY=!CY
   {
      // 4 cycles
      state->f.c = !state->f.c;
      state->pc = state->pc + 1;
      break;
   }
   case 0x37: // 0x37   STC         1     CY             CY = 1
   {
      // 4 cycles
      state->f.c = 1;
      state->pc = state->pc + 1;
      break;
   }

   // SINGLE REGISTER INSTRUCTIONS: INR, DCR, CMA, DAA
   case 0x04: // 0x04   INR B       1     Z S P AC       B <- B+1
   {
      // 5 cycles
      INR(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0x0C: // 0x0c   INR C       1     Z S P AC       C <- C+1
   {
      // 5 cycles
      INR(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0x14: // 0x14   INR D       1     Z S P AC       D <- D+1
   {
      // 5 cycles
      INR(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0x1C: // 0x1c   INR E       1     Z S P AC       E <- E+1
   {
      // 5 cycles
      INR(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0x24: // 0x24   INR H       1     Z S P AC       H <- H+1
   {
      // 5 cycles
      INR(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0x2C: // 0x2c   INR L       1     Z S P AC       L <- L+1
   {
      // 5 cycles
      INR(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0x34: // 0x34   INR M       1     Z S P AC       (HL) <- (HL)+1
   {
      // 10 cycles
      INR(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0x3C: // 0x3c   INR A       1     Z S P AC       A <- A+1
   {
      // 5 cycles
      INR(state, state->a);
      state->pc = state->pc + 1;
      break;
   }

   case 0x05: // 0x05   DCR B       1     Z S P AC       B <- B-1
   {
      // 5 cycles
      DCR(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0x0D: // 0x0d   DCR C       1     Z S P AC       C <- C-1
   {
      // 5 cycles
      DCR(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0x15: // 0x15   DCR D       1     Z S P AC       D <- D-1
   {
      // 5 cycles
      DCR(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0x1D: // 0x1d   DCR E       1     Z S P AC       E <- E-1
   {
      // 5 cycles
      DCR(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0x25: // 0x25   DCR H       1     Z S P AC       H <- H-1
   {
      // 5 cycles
      DCR(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0x2D: // 0x2d   DCR L       1     Z S P AC       L <- L-1
   {
      // 5 cycles
      DCR(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0x35: // 0x35   DCR M       1     Z S P AC       (HL) <- (HL)-1
   {
      // 10 cycles
      DCR(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0x3D: // 0x3d   DCR A       1     Z S P AC       A <- A-1
   {
      // 5 cycles
      DCR(state, state->a);
      state->pc = state->pc + 1;
      break;
   }

   case 0x2F: // 0x2f   CMA         1                    A <- !A
   {
      // 4 cycles
      state->a = ~state->a;
      state->pc = state->pc + 1;
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
      if (((state->a & 0x0f) > 0x09) || (state->f.a == 1))
      {
         uint8_t x = (state->a & 0x0f) + 0x06;
         // Auxiliary Carry flag set/reset if carry out on low 4 bits did/not occur
         state->f.a = ((x & 0x10) == 0x10);
         state->a = state->a + 0x06;
      }
      else
         // TODO: Is the following line of code correct? Everything else is fine.
         state->f.a = 0; // I guess no carry out occured, so AC is reset.

      /// Step (2)
      /// If the (greater) four bits of the accumulator now (are) greater than 9,
      /// or if the normal carry bit is equal to one,
      /// the most significant four bits of the accumulator are incremented by six.
      /// Otherwise, no incrementing occurs.
      if (((state->a & 0xf0) > 0x90) || (state->f.c == 1))
      {
         uint16_t x = (uint16_t)(state->a & 0xf0) + (uint16_t)0x60;
         // Carry flag set/reset if carry out on high 4 bits did/not occur
         state->f.c = ((x & 0x100) == 0x100);
         state->a = state->a + 0x60;
      }

      /// If a carry out of the least significant four bits occurs during Step (1),
      /// the Auxiliary Carry bit is set; otherwise it is reset.
      /// Likewise, if a carry out of the most significant four bits occur during Step (2),
      /// the normal Carry bit is set; otherwise, it is unaffected.

      state->f.z = (state->a == 0); // Zero flag
      state->f.s = ((state->a & 0x80) == 0x80); // Sign flag
      state->f.p = parity(state->a); // Parity flag

      state->pc = state->pc + 1;
      break;
   }

   // NOP INSTRUCTION
   case 0x00: // 0x00   NOP         1
   {
      // 4 cycles
      state->pc = state->pc + 1;
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

      if (*opcode == 0x76) // dst and src both = 110B
         break;

      uint8_t* dst = state->reg((*opcode >> 3) & 0x7); // 01DDDSSS
      uint8_t* src = state->reg((*opcode >> 0) & 0x7); // 01DDDSSS
      *dst = *src;

      state->pc = state->pc + 1;
      break;
   }

   case 0x02: // 0x02   STAX B      1                    (BC) <- A
   {
      // 7 cycles
      state->memory[(state->b << 8) | (state->c << 0)] = state->a;
      state->pc = state->pc + 1;
      break;
   }
   case 0x12: // 0x12   STAX D      1                    (DE) <- A
   {
      // 7 cycles
      state->memory[(state->d << 8) | (state->e << 0)] = state->a;
      state->pc = state->pc + 1;
      break;
   }

   case 0x0A: // 0x0a   LDAX B      1                    A <- (BC)
   {
      // 7 cycles
      state->a = state->memory[(state->b << 8) | (state->c)];
      state->pc = state->pc + 1;
      break;
   }
   case 0x1A: // 0x1a   LDAX D      1                    A <- (DE)
   {
      // 7 cycles
      MOV(state, state->memory[(state->d << 8) | (state->e)], state->a);
      state->pc = state->pc + 1;
      break;
   }

   // REGISTER OR MEMORY TO ACCUMULATOR INSTRUCTIONS: ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP
   case 0x80: // 0x80   ADD B       1     Z S P CY AC    A <- A + B
   {
      // 4 cycles
      ADD(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0x81: // 0x81   ADD C       1     Z S P CY AC    A <- A + C
   {
      // 4 cycles
      ADD(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0x82: // 0x82   ADD D       1     Z S P CY AC    A <- A + D
   {
      // 4 cycles
      ADD(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0x83: // 0x83   ADD E       1     Z S P CY AC    A <- A + E
   {
      // 4 cycles
      ADD(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0x84: // 0x84   ADD H       1     Z S P CY AC    A <- A + H
   {
      // 4 cycles
      ADD(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0x85: // 0x85   ADD L       1     Z S P CY AC    A <- A + L
   {
      // 4 cycles
      ADD(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0x86: // 0x86   ADD M       1     Z S P CY AC    A <- A + (HL)
   {
      // 7 cycles
      ADD(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0x87: // 0x87   ADD A       1     Z S P CY AC    A <- A + A
   {
      // 4 cycles
      ADD(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0x88: // 0x88   ADC B       1     Z S P CY AC    A <- A + B + CY
   {
      // 4 cycles
      ADC(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0x89: // 0x89   ADC C       1     Z S P CY AC    A <- A + C + CY
   {
      // 4 cycles
      ADC(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0x8A: // 0x8a   ADC D       1     Z S P CY AC    A <- A + D + CY
   {
      // 4 cycles
      ADC(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0x8B: // 0x8b   ADC E       1     Z S P CY AC    A <- A + E + CY
   {
      // 4 cycles
      ADC(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0x8C: // 0x8c   ADC H       1     Z S P CY AC    A <- A + H + CY
   {
      // 4 cycles
      ADC(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0x8D: // 0x8d   ADC L       1     Z S P CY AC    A <- A + L + CY
   {
      // 4 cycles
      ADC(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0x8E: // 0x8e   ADC M       1     Z S P CY AC    A <- A + (HL) + CY
   {
      // 7 cycles
      ADC(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0x8F: // 0x8f   ADC A       1     Z S P CY AC    A <- A + A + CY
   {
      // 4 cycles
      ADC(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0x90: // 0x90   SUB B       1     Z S P CY AC    A <- A - B
   {
      // 4 cycles
      SUB(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0x91: // 0x91   SUB C       1     Z S P CY AC    A <- A - C
   {
      // 4 cycles
      SUB(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0x92: // 0x92   SUB D       1     Z S P CY AC    A <- A + D
   {
      // 4 cycles
      SUB(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0x93: // 0x93   SUB E       1     Z S P CY AC    A <- A - E
   {
      // 4 cycles
      SUB(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0x94: // 0x94   SUB H       1     Z S P CY AC    A <- A + H
   {
      // 4 cycles
      SUB(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0x95: // 0x95   SUB L       1     Z S P CY AC    A <- A - L
   {
      // 4 cycles
      SUB(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0x96: // 0x96   SUB M       1     Z S P CY AC    A <- A + (HL)
   {
      // 7 cycles
      SUB(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0x97: // 0x97   SUB A       1     Z S P CY AC    A <- A - A
   {
      // 4 cycles
      SUB(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0x98: // 0x98   SBB B       1     Z S P CY AC    A <- A - B - CY
   {
      // 4 cycles
      SBB(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0x99: // 0x99   SBB C       1     Z S P CY AC    A <- A - C - CY
   {
      // 4 cycles
      SBB(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0x9A: // 0x9a   SBB D       1     Z S P CY AC    A <- A - D - CY
   {
      // 4 cycles
      SBB(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0x9B: // 0x9b   SBB E       1     Z S P CY AC    A <- A - E - CY
   {
      // 4 cycles
      SBB(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0x9C: // 0x9c   SBB H       1     Z S P CY AC    A <- A - H - CY
   {
      // 4 cycles
      SBB(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0x9D: // 0x9d   SBB L       1     Z S P CY AC    A <- A - L - CY
   {
      // 4 cycles
      SBB(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0x9E: // 0x9e   SBB M       1     Z S P CY AC    A <- A - (HL) - CY
   {
      // 7 cycles
      SBB(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0x9F: // 0x9f   SBB A       1     Z S P CY AC    A <- A - A - CY
   {
      // 4 cycles
      SBB(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA0: // 0xa0   ANA B       1     Z S P CY AC    A <- A & B
   {
      // 4 cycles
      ANA(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA1: // 0xa1   ANA C       1     Z S P CY AC    A <- A & C
   {
      // 4 cycles
      ANA(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA2: // 0xa2   ANA D       1     Z S P CY AC    A <- A & D
   {
      // 4 cycles
      ANA(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA3: // 0xa3   ANA E       1     Z S P CY AC    A <- A & E
   {
      // 4 cycles
      ANA(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA4: // 0xa4   ANA H       1     Z S P CY AC    A <- A & H
   {
      // 4 cycles
      ANA(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA5: // 0xa5   ANA L       1     Z S P CY AC    A <- A & L
   {
      // 4 cycles
      ANA(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA6: // 0xa6   ANA M       1     Z S P CY AC    A <- A & (HL)
   {
      // 7 cycles
      ANA(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA7: // 0xa7   ANA A       1     Z S P CY AC    A <- A & A
   {
      // 4 cycles
      ANA(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA8: // 0xa8   XRA B       1     Z S P CY AC    A <- A ^ B
   {
      // 4 cycles
      XRA(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0xA9: // 0xa9   XRA C       1     Z S P CY AC    A <- A ^ C
   {
      // 4 cycles
      XRA(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0xAA: // 0xaa   XRA D       1     Z S P CY AC    A <- A ^ D
   {
      // 4 cycles
      XRA(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0xAB: // 0xab   XRA E       1     Z S P CY AC    A <- A ^ E
   {
      // 4 cycles
      XRA(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0xAC: // 0xac   XRA H       1     Z S P CY AC    A <- A ^ H
   {
      // 4 cycles
      XRA(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0xAD: // 0xad   XRA L       1     Z S P CY AC    A <- A ^ L
   {
      // 4 cycles
      XRA(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0xAE: // 0xae   XRA M       1     Z S P CY AC    A <- A ^ (HL)
   {
      // 7 cycles
      XRA(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0xAF: // 0xaf   XRA A       1     Z S P CY AC    A <- A ^ A
   {
      // 4 cycles
      XRA(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB0: // 0xb0   ORA B       1     Z S P CY AC    A <- A | B
   {
      // 4 cycles
      ORA(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB1: // 0xb1   ORA C       1     Z S P CY AC    A <- A | C
   {
      // 4 cycles
      ORA(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB2: // 0xb2   ORA D       1     Z S P CY AC    A <- A | D
   {
      // 4 cycles
      ORA(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB3: // 0xb3   ORA E       1     Z S P CY AC    A <- A | E
   {
      // 4 cycles
      ORA(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB4: // 0xb4   ORA H       1     Z S P CY AC    A <- A | H
   {
      // 4 cycles
      ORA(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB5: // 0xb5   ORA L       1     Z S P CY AC    A <- A | L
   {
      // 4 cycles
      ORA(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB6: // 0xb6   ORA M       1     Z S P CY AC    A <- A | (HL)
   {
      // 7 cycles
      ORA(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB7: // 0xb7   ORA A       1     Z S P CY AC    A <- A | A
   {
      // 4 cycles
      ORA(state, state->a);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB8: // 0xb8   CMP B       1     Z S P CY AC    A - B
   {
      // 4 cycles
      CMP(state, state->b);
      state->pc = state->pc + 1;
      break;
   }
   case 0xB9: // 0xb9   CMP C       1     Z S P CY AC    A - C
   {
      // 4 cycles
      CMP(state, state->c);
      state->pc = state->pc + 1;
      break;
   }
   case 0xBA: // 0xba   CMP D       1     Z S P CY AC    A - D
   {
      // 4 cycles
      CMP(state, state->d);
      state->pc = state->pc + 1;
      break;
   }
   case 0xBB: // 0xbb   CMP E       1     Z S P CY AC    A - E
   {
      // 4 cycles
      CMP(state, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0xBC: // 0xbc   CMP H       1     Z S P CY AC    A - H
   {
      // 4 cycles
      CMP(state, state->h);
      state->pc = state->pc + 1;
      break;
   }
   case 0xBD: // 0xbd   CMP L       1     Z S P CY AC    A - L
   {
      // 4 cycles
      CMP(state, state->l);
      state->pc = state->pc + 1;
      break;
   }
   case 0xBE: // 0xbe   CMP M       1     Z S P CY AC    A - (HL)
   {
      // 7 cycles
      CMP(state, state->memory[(state->h << 8) | (state->l)]);
      state->pc = state->pc + 1;
      break;
   }
   case 0xBF: // 0xbf   CMP A       1     Z S P CY AC    A - A
   {
      // 4 cycles
      CMP(state, state->a);
      state->pc = state->pc + 1;
      break;
   }

   // ROTATE ACCUMULATOR INSTRUCTIONS: RLC, RRC, RAL, RAR
   case 0x07: // 0x07   RLC         1     CY             A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
   {
      // 4 cycles
      state->f.c = ((state->a & 0x80) == 0x80); /// Carry bit is set equal to the high-order bit of the accumulator
      state->a = ((state->a << 1) & 0xfe) | ((state->a >> 7) & 0x01);
      state->pc = state->pc + 1;
      break;
   }
   case 0x0F: // 0x0f   RRC         1     CY             A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
   {
      // 4 cycles
      state->f.c = ((state->a & 0x01) == 0x01); /// Carry bit is set equal to the low-order bit of the accumulator
      state->a = ((state->a >> 1) & 0x7f) | ((state->a << 7) & 0x80);
      state->pc = state->pc + 1;
      break;
   }
   case 0x17: // 0x17   RAL         1     CY             A = A << 1; bit 0 = prev CY; CY = prev bit 7
   {
      // 4 cycles
      uint8_t temp = state->a;
      state->a = ((state->a << 1) & 0xfe) | (state->f.c);
      state->f.c = ((temp & 0x80) == 0x80);
      state->pc = state->pc + 1;
      break;
   }
   case 0x1F: // 0x1f   RAR         1     CY             A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
   {
      // 4 cycles
      uint8_t temp = state->a;
      state->a = ((state->a >> 1) & 0x7f) | (state->f.c << 7);
      state->f.c = ((temp & 0x01) == 0x01);
      state->pc = state->pc + 1;
      break;
   }

   // REGISTER PAIR INSTRUCTIONS: PUSH, POP, DAD INX, DCX, XCHG, XTHL, SPHL
   case 0xC5: // 0xc5   PUSH B      1                    (sp-2)<-C; (sp-1)<-B; sp <- sp - 2
   {
      // From manual STACK PUSH OPERATION

      // 11 cycles
      state->memory[state->sp - 1] = state->b;
      state->memory[state->sp - 2] = state->c;
      state->sp = state->sp - 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0xD5: // 0xd5   PUSH D      1                    (sp-2)<-E; (sp-1)<-D; sp <- sp - 2
   {
      // 11 cycles
      state->memory[state->sp - 1] = state->d;
      state->memory[state->sp - 2] = state->e;
      state->sp = state->sp - 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0xE5: // 0xe5   PUSH H      1                    (sp-2)<-L; (sp-1)<-H; sp <- sp - 2
   {
      // 11 cycles
      state->memory[state->sp - 1] = state->h;
      state->memory[state->sp - 2] = state->l;
      state->sp = state->sp - 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0xF5: // 0xf5   PUSH PSW    1                    (sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
   {
      // 11 cycles
      //ConditionCodes*
      //state->memory[state->sp - 2] = *((uint8_t*)&state->f);
      state->memory[state->sp - 2]
         = (state->f.s << 7)
         | (state->f.z << 6)
         | (state->f.a << 4)
         | (state->f.p << 2)
         | (state->f.c << 0);
      state->memory[state->sp - 1] = state->a;
      state->sp = state->sp - 2;
      state->pc = state->pc + 1;
      break;
   }

   case 0xC1: // 0xc1   POP B       1                    C <- (sp); B <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      state->c = state->memory[state->sp + 0];
      state->b = state->memory[state->sp + 1];
      state->sp = state->sp + 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0xD1: // 0xd1   POP D       1                    E <- (sp); D <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      state->d = state->memory[state->sp + 1];
      state->e = state->memory[state->sp + 0];
      state->sp = state->sp + 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0xE1: // 0xe1   POP H       1                    L <- (sp); H <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      state->h = state->memory[state->sp + 1];
      state->l = state->memory[state->sp + 0];
      state->sp = state->sp + 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0xF1: // 0xf1   POP PSW     1     Z S P CY AC    flags <- (sp); A <- (sp+1); sp <- sp+2
   {
      // 10 cycles
      //state->f = *((ConditionCodes*)&state->memory[state->sp + 0]); // Restore flags from memory
      uint8_t flags = state->memory[state->sp + 0];
      state->f.s = ((flags & (1 << 7)) == (1 << 7));
      state->f.z = ((flags & (1 << 6)) == (1 << 6));
      state->f.a = ((flags & (1 << 4)) == (1 << 4));
      state->f.p = ((flags & (1 << 2)) == (1 << 2));
      state->f.c = ((flags & (1 << 0)) == (1 << 0));

      state->a = state->memory[state->sp + 1];
      state->sp = state->sp + 2;
      state->pc = state->pc + 1;
      break;
   }
   case 0x09: // 0x09   DAD B       1     CY             HL = HL + BC
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(state->h << 8) | (uint32_t)state->l;
      uint32_t bc = (uint32_t)(state->b << 8) | (uint32_t)state->c;
      uint32_t res = hl + bc;
      state->h = (res & 0xff00) >> 8;
      state->l = (res & 0x00ff) >> 0;
      state->f.c = ((res & 0x10000) == 0x10000);
      state->pc = state->pc + 1;
      break;
   }
   case 0x19: // 0x19   DAD D       1     CY             HL = HL + DE
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(state->h << 8) | (uint32_t)state->l;
      uint32_t de = (uint32_t)(state->d << 8) | (uint32_t)state->e;
      uint32_t res = hl + de;
      state->h = (res & 0xff00) >> 8;
      state->l = (res & 0x00ff) >> 0;
      state->f.c = ((res & 0x10000) == 0x10000);
      state->pc = state->pc + 1;
      break;
   }
   case 0x29: // 0x29   DAD H       1     CY             HL = HL + HL
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(state->h << 8) | (uint32_t)state->l;
      uint32_t res = hl + hl;
      state->h = (res & 0xff00) >> 8;
      state->l = (res & 0x00ff) >> 0;
      state->f.c = ((res & 0x10000) == 0x10000);
      state->pc = state->pc + 1;
      break;
   }
   case 0x39: // 0x39   DAD SP      1     CY             HL = HL + SP
   {
      // 10 cycles
      uint32_t hl = (uint32_t)(state->h << 8) | (uint32_t)state->l;
      uint32_t res = hl + (uint32_t)state->sp;
      state->h = (res & 0xff00) >> 8;
      state->l = (res & 0x00ff) >> 0;
      state->f.c = ((res & 0x10000) == 0x10000);
      state->pc = state->pc + 1;
      break;
   }

   case 0x03: // 0x03   INX B       1                    BC <- BC+1
   {
      // 5 cycles
      state->c = state->c + 1;
      if (state->c == 0)
         state->b = state->b + 1;
      state->pc = state->pc + 1;
      break;
   }
   case 0x13: // 0x13   INX D       1                    DE <- DE + 1
   {
      // 5 cycles
      state->e = state->e + 1;
      if (state->e == 0)
         state->d = state->d + 1;
      state->pc = state->pc + 1;
      break;
   }
   case 0x23: // 0x23   INX H       1                    HL <- HL + 1
   {
      // 5 cycles
      state->l = state->l + 1;
      if (state->l == 0)
         state->h = state->h + 1;
      state->pc = state->pc + 1;
      break;
   }
   case 0x33: // 0x33   INX SP      1                    SP = SP + 1
   {
      // 5 cycles
      state->sp = state->sp + 1;
      state->pc = state->pc + 1;
      break;
   }

   case 0x0B: // 0x0b   DCX B       1                    BC = BC-1
   {
      // 5 cycles
      uint32_t bc = (state->b << 8) | (state->c);
      uint32_t res = bc - 1;
      state->b = (res & 0xff00) >> 8;
      state->c = (res & 0x00ff) >> 0;
      state->pc = state->pc + 1;
      break;
   }
   case 0x1B: // 0x1b   DCX D       1                    DE = DE-1
   {
      // 5 cycles
      uint32_t de = (state->d << 8) | (state->e);
      uint32_t res = de - 1;
      state->d = (res & 0xff00) >> 8;
      state->e = (res & 0x00ff) >> 0;
      state->pc = state->pc + 1;
      break;
   }
   case 0x2B: // 0x2b   DCX H       1                    HL = HL-1
   {
      // 5 cycles
      uint32_t hl = (state->h << 8) | (state->l);
      uint32_t res = hl - 1;
      state->h = (res & 0xff00) >> 8;
      state->l = (res & 0x00ff) >> 0;
      state->pc = state->pc + 1;
      break;
   }
   case 0x3B: // 0x3b   DCX SP      1                    SP = SP-1
   {
      // 5 cycles
      state->sp = state->sp - 1;
      state->pc = state->pc + 1;
      break;
   }

   case 0xEB: // 0xeb   XCHG        1                    H <-> D; L <-> E
   {
      // 5 cycles
      std::swap(state->h, state->d);
      std::swap(state->l, state->e);
      state->pc = state->pc + 1;
      break;
   }
   case 0xE3: // 0xe3   XTHL        1                    L <-> (SP); H <-> (SP+1)
   {
      // 18 cycles (Longest operation!)
      std::swap(state->l, state->memory[state->sp + 0]);
      std::swap(state->h, state->memory[state->sp + 1]);
      state->pc = state->pc + 1;
      break;
   }
   case 0xF9: // 0xf9   SPHL        1                    SP=HL
   {
      // 5 cycles
      state->sp = (state->h << 8) | (state->l);
      state->pc = state->pc + 1;
      break;
   }

   // IMMEDIATE INSTRUCTIONS: LXI, MVI, ADI, ACI, SUI, SBI, ANI, XRI, ORI, CPI
   case 0x01: // 0x01   LXI BD16    3                    B <- byte 3 C <- byte 2
   {
      // 10 cycles
      state->b = opcode[2];
      state->c = opcode[1];
      state->pc = state->pc + 3;
      break;
   }
   case 0x11: // 0x11   LXI DD16    3                    D <- byte 3 E <- byte 2
   {
      // 10 cycles
      state->d = opcode[2];
      state->e = opcode[1];
      state->pc = state->pc + 3;
      break;
   }
   case 0x21: // 0x21   LXI HD16    3                    H <- byte 3 L <- byte 2
   {
      // 10 cycles
      state->h = opcode[2];
      state->l = opcode[1];
      state->pc = state->pc + 3;
      break;
   }
   case 0x31: // 0x31   LXI SP D16  3                    SP.hi <- byte 3 SP.lo <- byte 2
   {
      // 10 cycles
      state->sp = address(state, opcode);
      state->pc = state->pc + 3;
      break;
   }

   case 0x06: // 0x06   MVI B D8    2                    B <- byte 2
   {
      // 7 cycles
      state->b = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x0E: // 0x0e   MVI C D8    2                    C <- byte 2
   {
      // 7 cycles
      state->c = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x16: // 0x16   MVI D D8    2                    D <- byte 2
   {
      // 7 cycles
      state->d = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x1E: // 0x1e   MVI E D8    2                    E <- byte 2
   {
      // 7 cycles
      state->e = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x26: // 0x26   MVI H D8    2                    H <- byte 2
   {
      // 7 cycles
      state->h = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x2E: // 0x2e   MVI L D8    2                    L <- byte 2
   {
      // 7 cycles
      state->l = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x36: // 0x36   MVI M D8    2                    (HL) <- byte 2
   {
      // 10 cycles
      state->memory[(state->h << 8) | (state->l)] = opcode[1];
      state->pc = state->pc + 2;
      break;
   }
   case 0x3E: // 0x3e   MVI A D8    2                    A <- byte 2
   {
      // 7 cycles
      state->a = opcode[1];
      state->pc = state->pc + 2;
      break;
   }

   case 0xC6: // 0xc6   ADI 8       2     Z S P CY AC    A <- A + byte
   {
      // 7 cycles
      ADD(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xCE: // 0xce   ACI D8      2     Z S P CY AC    A <- A + data + CY
   {
      // 7 cycles
      ADC(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xD6: // 0xd6   SUI D8      2     Z S P CY AC    A <- A - data
   {
      // 7 cycles
      SUB(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xDE: // 0xde   SBI D8      2     Z S P CY AC    A <- A - data - CY
   {
      // 7 cycles
      SBB(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xE6: // 0xe6   ANI D8      2     Z S P CY AC    A <- A & data
   {
      // 7 cycles
      ANA(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xEE: // 0xee   XRI D8      2     Z S P CY AC    A <- A ^ data
   {
      // 7 cycles
      XRA(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xF6: // 0xf6   ORI D8      2     Z S P CY AC    A <- A | data
   {
      // 7 cycles
      ORA(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }
   case 0xFE: // 0xfe   CPI D8      2     Z S P CY AC    A - data
   {
      // 7 cycles
      CMP(state, opcode[1]);
      state->pc = state->pc + 2;
      break;
   }

   // DIRECT ADDRESSING INSTRUCTIONS: STA, LDA, SHLD, LHLD
   case 0x32: // 0x32   STA adr     3                    (adr) <- A
   {
      // 13 cycles
      uint16_t adr = address(state, opcode);
      state->memory[adr] = state->a;
      state->pc = state->pc + 3;
      break;
   }
   case 0x3A: // 0x3a   LDA adr     3                    A <- (adr)
   {
      // 13 cycles
      uint16_t adr = address(state, opcode);
      state->a = state->memory[adr];
      state->pc = state->pc + 3;
      break;
   }
   case 0x22: // 0x22   SHLD adr    3                    (adr) <-L; (adr+1)<-H
   {
      // 16 cycles
      uint16_t adr = address(state, opcode);
      state->memory[adr + 0] = state->l;
      state->memory[adr + 1] = state->h;
      state->pc = state->pc + 3;
      break;
   }
   case 0x2A: // 0x2a   LHLD adr    3                    L <- (adr); H<-(adr+1)
   {
      // 16 cycles
      uint16_t adr = address(state, opcode);
      state->l = state->memory[adr + 0];
      state->h = state->memory[adr + 1];
      state->pc = state->pc + 3;
      break;
   }

   // JUMP INSTRUCTIONS: PCHL, JMP, JC, JNC, JZ, JNZ, JM, JP, JPE, JPO
   case 0xE9: // 0xe9   PCHL        1                    PC.hi <- H; PC.lo <- L
   {
      // 5 cycles
      state->pc
         = (((uint16_t)state->h) << 8)
         | (((uint16_t)state->l) << 0);
      break;
   }
   case 0xC3: // 0xc3   JMP adr     3                    PC <- adr
   {
      // 10 cycles
      state->pc = address(state, opcode);
      break;
   }
   case 0xDA: // 0xda   JC  adr     3                    if CY PC<-adr
   {
      // 10 cycles
      if (state->f.c == 1)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xD2: // 0xd2   JNC adr     3                    if NCY PC<-adr
   {
      // 10 cycles
      if (state->f.c == 0)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xCA: // 0xca   JZ  adr     3                    if Z PC <- adr
   {
      // 10 cycles
      if (state->f.z == 1)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xC2: // 0xc2   JNZ adr     3                    if NZ PC <- adr
   {
      // 10 cycles
      if (state->f.z == 0)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xFA: // 0xfa   JM  adr     3                    if S=1 PC <- adr
   {
      // Manual says:
      // "If the sign bit is one (indicating a negative result),
      // program execution continues at the memory adr."

      // 10 cycles
      if (state->f.s == 1)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xF2: // 0xf2   JP  adr     3                    if S=0 PC <- adr
   {
      // Manual says:
      // "If the sign bit is zero, (indicating a positive result),
      // program execution continues at the memory adr."

      // 10 cycles
      if (state->f.s == 0)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xEA: // 0xea   JPE adr     3                    if P=1 PC <- adr
   {
      // Manual says:
      // "If the parity bit is one (indicating a result with even parity),
      // program execution continues at the memory address adr.

      // 10 cycles
      if (state->f.p == 1)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xE2: // 0xe2   JPO adr     3                    if P=0 PC <- adr
   {
      // Manual says:
      // If the parity bit is zero (indicating a result with odd parity),
      // program execution continues at the memory address adr."

      // 10 cycles
      if (state->f.p == 0)
         state->pc = address(state, opcode);
      else
         state->pc = state->pc + 3;
      break;
   }

   // CALL SUBROUTINE INSTRUCTIONS: CALL, CC, CNC, CZ, CNZ, CM, CP, CPE, CPO
   case 0xCD: // 0xcd   CALL adr    3                    (SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP+2;PC=adr
   {
#ifdef FOR_CPUDIAG
      if (5 == ((opcode[2] << 8) | opcode[1]))
      {
         if (state->c == 9)
         {
            uint16_t offset = (state->d << 8) | (state->e);
            char *str = (char*)&state->memory[offset + 3]; // skip the prefix bytes
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
      CALL(state, address(state, opcode));
      break;
   }
   case 0xDC: // 0xdc   CC a dr     3                    if CY CALL adr
   {
      // 17/11 cycles
      if (state->f.c == 1)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xD4: // 0xd4   CNC adr     3                    if NCY CALL adr
   {
      // 17/11 cycles
      if (state->f.c == 0)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xCC: // 0xcc   CZ a dr     3                    if Z CALL adr
   {
      /// Description: If the Zero bit is *(one), a call operation is
      /// performed to subroutine sub.

      // 17/11 cycles
      if (state->f.z == 1)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xC4: // 0xc4   CNZ adr     3                    if NZ CALL adr
   {
      /// Description: If the Zero bit is *(zero), a call operation is
      /// performed to subroutine sub.

      // 17/11 cycles
      if (state->f.z == 0)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xFC: // 0xfc   CM a dr     3                    if S CALL adr
   {
      // 17/11 cycles
      if (state->f.s == 1)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xF4: // 0xf4   CP a dr     3                    if NS PC <- adr
   {
      // 17/11 cycles
      if (state->f.s == 0)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xEC: // 0xec   CPE adr     3                    if PE CALL adr
   {
      // 17/11 cycles
      if (state->f.p == 1)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }
   case 0xE4: // 0xe4   CPO adr     3                    if PO CALL adr
   {
      // 17/11 cycles
      if (state->f.p == 0)
         CALL(state, address(state, opcode));
      else
         state->pc = state->pc + 3;
      break;
   }

   // RETURN FROM SUBROUTINE INSTRUCTIONS: RET, RN, RNC, RZ, RNZ, RM, RP, RPE, RPO
   case 0xC9: // 0xc9   RET         1                    PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2
   {
      // 10 cycles
      RET(state);
      break;
   }
   case 0xD8: // 0xd8   RC          1                    if CY RET
   {
      // 11/5 cycles
      if (state->f.c == 1)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xD0: // 0xd0   RNC         1                    if NCY RET
   {
      // 11/5 cycles
      if (state->f.c == 0)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xC8: // 0xc8   RZ          1                    if Z RET
   {
      // 11/5 cycles
      if (state->f.z == 1)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xC0: // 0xc0   RNZ         1                    if NZ RET
   {
      // 11/5 cycles
      if (state->f.z == 0)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xF8: // 0xf8   RM          1                    if S RET
   {
      // 11/5 cycles
      if (state->f.s == 1)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xF0: // 0xf0   RP          1                    if NS RET
   {
      // 11/5 cycles
      if (state->f.s == 0)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xE8: // 0xe8   RPE         1                    if PE RET
   {
      // 11/5 cycles
      if (state->f.p == 1)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }
   case 0xE0: // 0xe0   RPO         1                    if PO RET
   {
      // 11/5 cycles
      if (state->f.p == 0)
         RET(state);
      else
         state->pc = state->pc + 1;
      break;
   }

   // RST INSTRUCTION
   case 0xC7: // 0xc7   RST 0       1                    CALL $0
   {
      // 11 cycles
      RST(state, 0 << 3);
      break;
   }
   case 0xCF: // 0xcf   RST 1       1                    CALL $8
   {
      // 11 cycles
      RST(state, 1 << 3);
      break;
   }
   case 0xD7: // 0xd7   RST 2       1                    CALL $10
   {
      // 11 cycles
      RST(state, 2 << 3);
      break;
   }
   case 0xDF: // 0xdf   RST 3       1                    CALL $18
   {
      // 11 cycles
      RST(state, 3 << 3);
      break;
   }
   case 0xE7: // 0xe7   RST 4       1                    CALL $20
   {
      // 11 cycles
      RST(state, 4 << 3);
      break;
   }
   case 0xEF: // 0xef   RST 5       1                    CALL $28
   {
      // 11 cycles
      RST(state, 5 << 3);
      break;
   }
   case 0xF7: // 0xf7   RST 6       1                    CALL $30
   {
      // 11 cycles
      RST(state, 6 << 3);
      break;
   }
   case 0xFF: // 0xff   RST 7       1                    CALL $38
   {
      // 11 cycles
      RST(state, 7 << 3);
      break;
   }

   // INTERRUPT FLIP-FLOP INSTRUCTIONS
   case 0xFB: // 0xfb   EI          1                    special
   {
      // 4 cycles
      // Enable Interrupts
      state->int_enable = true;
      state->pc = state->pc + 1;
      break;
   }
   case 0xF3: // 0xf3   DI          1                    special
   {
      // 4 cycles
      // Disable Interrupts
      state->int_enable = false;
      state->pc = state->pc + 1;
      break;
   }

   // INPUT/OUTPUT INSTRUCTIONS: IN, OUT
   case 0xDB: // 0xdb   IN D 8      2                    special
   {
      // 10 cycles
      // Read input port into A
      uint8_t port = state->memory[++state->pc];
      // state->a = state->port[port];
      state->pc = state->pc + 2;
      break;
   }
   case 0xD3: // 0xd3   OUT D8      2                    special
   {
      // 10 cycles
      // Write A to ouput port
      uint8_t port = state->memory[++state->pc];
      // state->port[port] = state->a;
      state->pc = state->pc + 2;
      break;
   }

   // HLT HALT INSTRUCTION
   case 0x76: // 0x76   HLT         1                    special
   {
      // Halt processor
      // 7 cycles
      state->pc = state->pc + 1;
      state->stopped = true;
      break;
   }

   // unused
   default: // 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0xcb, 0xd9, 0xdd, 0xed, 0xfd
   {
      // 4 cycles
      state->pc = state->pc + 1;
      break;
   }
   }
}

int main(int argc, char** argv)
{
   if (argc != 2)
      return 0;

   State8080* state = new State8080;

   std::ifstream file(argv[1], std::ios::binary);

   file.read((char*)state->memory, 0xffff);

   //int pc = 0;
   //while (pc < 1453)
   //{
   //   pc = pc + Disassemble8080Op(state->memory, pc);
   //   std::cout << std::endl;
   //}

   //state->memory[0x00] = 0xc3;
   //state->memory[0x01] = 0x00;
   //state->memory[0x02] = 0x01;

   //state->memory[0x170] = 0x07;

   //uint8_t program[] =
   //{
   //   0x3e, 0x01, // MVI A, $1
   //   0x06, 0x02, // MVI B, $2
   //   0x80        // ADD B
   //};

   //for (int opcode = 0; opcode < sizeof(program); opcode++)
   //   state->memory[opcode] = program[opcode];

   // Made it to 0x0588

   for (;;)
   {
      std::cout << std::endl;
      Emulate8080Op(state);
      display(state);
   }

   return 0;
}