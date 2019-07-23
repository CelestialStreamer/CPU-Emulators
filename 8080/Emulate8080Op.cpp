#include "State8080.h"
#include <algorithm>

#define FOR_CPUDIAG
#define DEBUG

void State8080::generateInterrupt(uint8_t opcode) {
   interruptOpcode = opcode;
   interruptRequested = true;
}

void State8080::Emulate8080Op() {
   if (stopped) return;

   unsigned char opcode;

   // Handle interrupts first
   if (interruptRequested) {
      interrupt_enabled = false;
      interruptRequested = false;
      opcode = interruptOpcode;
   } else {
      opcode = imm<uint8_t>();
   }

   switch (opcode)
   {          // Instruction size  flags          function
   // CARRY BIT INSTRUCTIONS: CMC, STC
   case 0x3F: // CMC         1     CY             CY <- !CY
      Reg.f.c = !Reg.f.c; break;
   
   case 0x37: // STC         1     CY             CY <- 1
      Reg.f.c = 1; break;

   // SINGLE REGISTER INSTRUCTIONS: INR, DCR, CMA, DAA
   case 0x04: // INR B       1     Z S P AC       B <- B+1
      INR(Reg.b); break;
   case 0x0C: // INR C       1     Z S P AC       C <- C+1
      INR(Reg.c); break;
   case 0x14: // INR D       1     Z S P AC       D <- D+1
      INR(Reg.d); break;
   case 0x1C: // INR E       1     Z S P AC       E <- E+1
      INR(Reg.e); break;
   case 0x24: // INR H       1     Z S P AC       H <- H+1
      INR(Reg.h); break;
   case 0x2C: // INR L       1     Z S P AC       L <- L+1
      INR(Reg.l); break;
   case 0x34: // INR M       1     Z S P AC       (HL) <- (HL)+1
      INR(mem<uint8_t>(Reg.hl)); break;
   case 0x3C: // INR A       1     Z S P AC       A <- A+1
      INR(Reg.a); break;

   case 0x05: // DCR B       1     Z S P AC       B <- B-1
      DCR(Reg.b); break;
   case 0x0D: // DCR C       1     Z S P AC       C <- C-1
      DCR(Reg.c); break;
   case 0x15: // DCR D       1     Z S P AC       D <- D-1
      DCR(Reg.d); break;
   case 0x1D: // DCR E       1     Z S P AC       E <- E-1
      DCR(Reg.e); break;
   case 0x25: // DCR H       1     Z S P AC       H <- H-1
      DCR(Reg.h); break;
   case 0x2D: // DCR L       1     Z S P AC       L <- L-1
      DCR(Reg.l); break;
   case 0x35: // DCR M       1     Z S P AC       (HL) <- (HL)-1
      DCR(mem<uint8_t>(Reg.hl)); break;
   case 0x3D: // DCR A       1     Z S P AC       A <- A-1
      DCR(Reg.a); break;

   case 0x2F: // CMA         1                    A <- !A
      Reg.a = ~Reg.a; break;

   case 0x27: // DAA         1     Z S P CY AC    special
      DAA(); break;

   // NOP INSTRUCTION
   case 0x00: // NOP         1
      break;

   // DATA TRANSFER INSTRUCTIONS: MOV, STAX, LDAX
   case 0x40: // MOV BB      1                    B <- B
      break;
   case 0x41: // MOV BC      1                    B <- C
      Reg.b = Reg.c; break;
   case 0x42: // MOV BD      1                    B <- D
      Reg.b = Reg.d; break;
   case 0x43: // MOV BE      1                    B <- E
      Reg.b = Reg.e; break;
   case 0x44: // MOV BH      1                    B <- H
      Reg.b = Reg.h; break;
   case 0x45: // MOV BL      1                    B <- L
      Reg.b = Reg.l; break;
   case 0x46: // MOV BM      1                    B <- (HL)
      Reg.b = mem<uint8_t>(Reg.hl); break;
   case 0x47: // MOV BA      1                    B <- A
      Reg.b = Reg.a; break;

   case 0x48: // MOV CB      1                    C <- B
      Reg.c = Reg.b; break;
   case 0x49: // MOV CC      1                    C <- C
      break;
   case 0x4A: // MOV CD      1                    C <- D
      Reg.c = Reg.d; break;
   case 0x4B: // MOV CE      1                    C <- E
      Reg.c = Reg.e; break;
   case 0x4C: // MOV CH      1                    C <- H
      Reg.c = Reg.h; break;
   case 0x4D: // MOV CL      1                    C <- L
      Reg.c = Reg.l; break;
   case 0x4E: // MOV CM      1                    C <- (HL)
      Reg.c = mem<uint8_t>(Reg.hl); break;
   case 0x4F: // MOV CA      1                    C <- A
      Reg.c = Reg.a; break;

   case 0x50: // MOV DB      1                    D <- B
      Reg.d = Reg.b; break;
   case 0x51: // MOV DC      1                    D <- C
      Reg.d = Reg.c; break;
   case 0x52: // MOV DD      1                    D <- D
      break;
   case 0x53: // MOV DE      1                    D <- E
      Reg.d = Reg.e; break;
   case 0x54: // MOV DH      1                    D <- H
      Reg.d = Reg.h; break;
   case 0x55: // MOV DL      1                    D <- L
      Reg.d = Reg.l; break;
   case 0x56: // MOV DM      1                    D <- (HL)
      Reg.d = mem<uint8_t>(Reg.hl); break;
   case 0x57: // MOV DA      1                    D <- A
      Reg.d = Reg.a; break;

   case 0x58: // MOV EB      1                    E <- B
      Reg.e = Reg.b; break;
   case 0x59: // MOV EC      1                    E <- C
      Reg.e = Reg.c; break;
   case 0x5A: // MOV ED      1                    E <- D
      Reg.e = Reg.d; break;
   case 0x5B: // MOV EE      1                    E <- E
      break;
   case 0x5C: // MOV EH      1                    E <- H
      Reg.e = Reg.h; break;
   case 0x5D: // MOV EL      1                    E <- L
      Reg.e = Reg.l; break;
   case 0x5E: // MOV EM      1                    E <- (HL)
      Reg.e = mem<uint8_t>(Reg.hl); break;
   case 0x5F: // MOV EA      1                    E <- A
      Reg.e = Reg.a; break;

   case 0x60: // MOV HB      1                    H <- B
      Reg.h = Reg.b; break;
   case 0x61: // MOV HC      1                    H <- C
      Reg.h = Reg.c; break;
   case 0x62: // MOV HD      1                    H <- D
      Reg.h = Reg.d; break;
   case 0x63: // MOV HE      1                    H <- E
      Reg.h = Reg.e; break;
   case 0x64: // MOV HH      1                    H <- H
      break;
   case 0x65: // MOV HL      1                    H <- L
      Reg.h = Reg.l; break;
   case 0x66: // MOV HM      1                    H <- (HL)
      Reg.h = mem<uint8_t>(Reg.hl); break;
   case 0x67: // MOV HA      1                    H <- A
      Reg.h = Reg.a; break;

   case 0x68: // MOV LB      1                    L <- B
      Reg.l = Reg.b; break;
   case 0x69: // MOV LC      1                    L <- C
      Reg.l = Reg.c; break;
   case 0x6A: // MOV LD      1                    L <- D
      Reg.l = Reg.d; break;
   case 0x6B: // MOV LE      1                    L <- E
      Reg.l = Reg.e; break;
   case 0x6C: // MOV LH      1                    L <- H
      Reg.l = Reg.h; break;
   case 0x6D: // MOV LL      1                    L <- L
      break;
   case 0x6E: // MOV LM      1                    L <- (HL)
      Reg.l = mem<uint8_t>(Reg.hl); break;
   case 0x6F: // MOV LA      1                    L <- A
      Reg.l = Reg.a; break;

   case 0x70: // MOV MB      1                    (HL) <- B
      mem<uint8_t>(Reg.hl) = Reg. b; break;
   case 0x71: // MOV MC      1                    (HL) <- C
      mem<uint8_t>(Reg.hl) = Reg. c; break;
   case 0x72: // MOV MD      1                    (HL) <- D
      mem<uint8_t>(Reg.hl) = Reg. d; break;
   case 0x73: // MOV ME      1                    (HL) <- E
      mem<uint8_t>(Reg.hl) = Reg. e; break;
   case 0x74: // MOV MH      1                    (HL) <- H
      mem<uint8_t>(Reg.hl) = Reg. h; break;
   case 0x75: // MOV ML      1                    (HL) <- L
      mem<uint8_t>(Reg.hl) = Reg. l; break;
   case 0x77: // MOV MA      1                    (HL) <- A
      mem<uint8_t>(Reg.hl) = Reg. a; break;

   case 0x78: // MOV AB      1                    A <- B
      Reg.a = Reg.b; break;
   case 0x79: // MOV AC      1                    A <- C
      Reg.a = Reg.c; break;
   case 0x7A: // MOV AD      1                    A <- D
      Reg.a = Reg.d; break;
   case 0x7B: // MOV AE      1                    A <- E
      Reg.a = Reg.e; break;
   case 0x7C: // MOV AH      1                    A <- H
      Reg.a = Reg.h; break;
   case 0x7D: // MOV AL      1                    A <- L
      Reg.a = Reg.l; break;
   case 0x7E: // MOV AM      1                    A <- (HL)
      Reg.a = mem<uint8_t>(Reg.hl); break;
   case 0x7F: // MOV AA      1                    A <- A
      break;

   case 0x02: // STAX B      1                    (BC) <- A
      mem<uint8_t>(Reg.bc) = Reg.a; break;
   case 0x12: // STAX D      1                    (DE) <- A
      mem<uint8_t>(Reg.de) = Reg.a; break;

   case 0x0A: // LDAX B      1                    A <- (BC)
      Reg.a = mem<uint8_t>(Reg.bc); break;
   case 0x1A: // LDAX D      1                    A <- (DE)
      Reg.a = mem<uint8_t>(Reg.de); break;

   // REGISTER OR MEMORY TO ACCUMULATOR INSTRUCTIONS: ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP
   case 0x80: // ADD B       1     Z S P CY AC    A <- A + B
      ADD(Reg.b); break;
   case 0x81: // ADD C       1     Z S P CY AC    A <- A + C
      ADD(Reg.c); break;
   case 0x82: // ADD D       1     Z S P CY AC    A <- A + D
      ADD(Reg.d); break;
   case 0x83: // ADD E       1     Z S P CY AC    A <- A + E
      ADD(Reg.e); break;
   case 0x84: // ADD H       1     Z S P CY AC    A <- A + H
      ADD(Reg.h); break;
   case 0x85: // ADD L       1     Z S P CY AC    A <- A + L
      ADD(Reg.l); break;
   case 0x86: // ADD M       1     Z S P CY AC    A <- A + (HL)
      ADD(mem<uint8_t>(Reg.hl)); break;
   case 0x87: // ADD A       1     Z S P CY AC    A <- A + A
      ADD(Reg.a); break;

   case 0x88: // ADC B       1     Z S P CY AC    A <- A + B + CY
      ADC(Reg.b); break;
   case 0x89: // ADC C       1     Z S P CY AC    A <- A + C + CY
      ADC(Reg.c); break;
   case 0x8A: // ADC D       1     Z S P CY AC    A <- A + D + CY
      ADC(Reg.d); break;
   case 0x8B: // ADC E       1     Z S P CY AC    A <- A + E + CY
      ADC(Reg.e); break;
   case 0x8C: // ADC H       1     Z S P CY AC    A <- A + H + CY
      ADC(Reg.h); break;
   case 0x8D: // ADC L       1     Z S P CY AC    A <- A + L + CY
      ADC(Reg.l); break;
   case 0x8E: // ADC M       1     Z S P CY AC    A <- A + (HL) + CY
      ADC(mem<uint8_t>(Reg.hl)); break;
   case 0x8F: // ADC A       1     Z S P CY AC    A <- A + A + CY
      ADC(Reg.a); break;

   case 0x90: // SUB B       1     Z S P CY AC    A <- A - B
      SUB(Reg.b); break;
   case 0x91: // SUB C       1     Z S P CY AC    A <- A - C
      SUB(Reg.c); break;
   case 0x92: // SUB D       1     Z S P CY AC    A <- A + D
      SUB(Reg.d); break;
   case 0x93: // SUB E       1     Z S P CY AC    A <- A - E
      SUB(Reg.e); break;
   case 0x94: // SUB H       1     Z S P CY AC    A <- A + H
      SUB(Reg.h); break;
   case 0x95: // SUB L       1     Z S P CY AC    A <- A - L
      SUB(Reg.l); break;
   case 0x96: // SUB M       1     Z S P CY AC    A <- A + (HL)
      SUB(mem<uint8_t>(Reg.hl)); break;
   case 0x97: // SUB A       1     Z S P CY AC    A <- A - A
      SUB(Reg.a); break;

   case 0x98: // SBB B       1     Z S P CY AC    A <- A - B - CY
      SBB(Reg.b); break;
   case 0x99: // SBB C       1     Z S P CY AC    A <- A - C - CY
      SBB(Reg.c); break;
   case 0x9A: // SBB D       1     Z S P CY AC    A <- A - D - CY
      SBB(Reg.d); break;
   case 0x9B: // SBB E       1     Z S P CY AC    A <- A - E - CY
      SBB(Reg.e); break;
   case 0x9C: // SBB H       1     Z S P CY AC    A <- A - H - CY
      SBB(Reg.h); break;
   case 0x9D: // SBB L       1     Z S P CY AC    A <- A - L - CY
      SBB(Reg.l); break;
   case 0x9E: // SBB M       1     Z S P CY AC    A <- A - (HL) - CY
      SBB(mem<uint8_t>(Reg.hl)); break;
   case 0x9F: // SBB A       1     Z S P CY AC    A <- A - A - CY
      SBB(Reg.a); break;

   case 0xA0: // ANA B       1     Z S P CY AC    A <- A & B
      ANA(Reg.b); break;
   case 0xA1: // ANA C       1     Z S P CY AC    A <- A & C
      ANA(Reg.c); break;
   case 0xA2: // ANA D       1     Z S P CY AC    A <- A & D
      ANA(Reg.d); break;
   case 0xA3: // ANA E       1     Z S P CY AC    A <- A & E
      ANA(Reg.e); break;
   case 0xA4: // ANA H       1     Z S P CY AC    A <- A & H
      ANA(Reg.h); break;
   case 0xA5: // ANA L       1     Z S P CY AC    A <- A & L
      ANA(Reg.l); break;
   case 0xA6: // ANA M       1     Z S P CY AC    A <- A & (HL)
      ANA(mem<uint8_t>(Reg.hl)); break;
   case 0xA7: // ANA A       1     Z S P CY AC    A <- A & A
      ANA(Reg.a); break;

   case 0xA8: // XRA B       1     Z S P CY AC    A <- A ^ B
      XRA(Reg.b); break;
   case 0xA9: // XRA C       1     Z S P CY AC    A <- A ^ C
      XRA(Reg.c); break;
   case 0xAA: // XRA D       1     Z S P CY AC    A <- A ^ D
      XRA(Reg.d); break;
   case 0xAB: // XRA E       1     Z S P CY AC    A <- A ^ E
      XRA(Reg.e); break;
   case 0xAC: // XRA H       1     Z S P CY AC    A <- A ^ H
      XRA(Reg.h); break;
   case 0xAD: // XRA L       1     Z S P CY AC    A <- A ^ L
      XRA(Reg.l); break;
   case 0xAE: // XRA M       1     Z S P CY AC    A <- A ^ (HL)
      XRA(mem<uint8_t>(Reg.hl)); break;
   case 0xAF: // XRA A       1     Z S P CY AC    A <- A ^ A
      XRA(Reg.a); break;

   case 0xB0: // ORA B       1     Z S P CY AC    A <- A | B
      ORA(Reg.b); break;
   case 0xB1: // ORA C       1     Z S P CY AC    A <- A | C
      ORA(Reg.c); break;
   case 0xB2: // ORA D       1     Z S P CY AC    A <- A | D
      ORA(Reg.d); break;
   case 0xB3: // ORA E       1     Z S P CY AC    A <- A | E
      ORA(Reg.e); break;
   case 0xB4: // ORA H       1     Z S P CY AC    A <- A | H
      ORA(Reg.h); break;
   case 0xB5: // ORA L       1     Z S P CY AC    A <- A | L
      ORA(Reg.l); break;
   case 0xB6: // ORA M       1     Z S P CY AC    A <- A | (HL)
      ORA(mem<uint8_t>(Reg.hl)); break;
   case 0xB7: // ORA A       1     Z S P CY AC    A <- A | A
      ORA(Reg.a); break;

   case 0xB8: // CMP B       1     Z S P CY AC    A - B
      CMP(Reg.b); break;
   case 0xB9: // CMP C       1     Z S P CY AC    A - C
      CMP(Reg.c); break;
   case 0xBA: // CMP D       1     Z S P CY AC    A - D
      CMP(Reg.d); break;
   case 0xBB: // CMP E       1     Z S P CY AC    A - E
      CMP(Reg.e); break;
   case 0xBC: // CMP H       1     Z S P CY AC    A - H
      CMP(Reg.h); break;
   case 0xBD: // CMP L       1     Z S P CY AC    A - L
      CMP(Reg.l); break;
   case 0xBE: // CMP M       1     Z S P CY AC    A - (HL)
      CMP(mem<uint8_t>(Reg.hl)); break;
   case 0xBF: // CMP A       1     Z S P CY AC    A - A
      CMP(Reg.a); break;

   // ROTATE ACCUMULATOR INSTRUCTIONS: RLC, RRC, RAL, RAR
   case 0x07: // RLC         1     CY             A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
   {
      Reg.f.c = ((Reg.a & 0x80) == 0x80); /// Carry bit is set equal to the high-order bit of the accumulator
      Reg.a = (Reg.a << 1) & (Reg.a >> 7); // Rotate to the right while wrapping first bit (7) to the last bit (0)
      break;
   }
   case 0x0F: // RRC         1     CY             A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
   {
      Reg.f.c = ((Reg.a & 0x01) == 0x01); /// Carry bit is set equal to the low-order bit of the accumulator
      Reg.a = (Reg.a >> 1) & (Reg.a << 7); // Rotate to the left while wrapping last bit (0) to first bit (7)
      break;
   }
   case 0x17: // RAL         1     CY             A = A << 1; bit 0 = prev CY; CY = prev bit 7
   {
      uint8_t carry = Reg.f.c; // Copy of carry bit
      Reg.f.c = ((Reg.a & 0x80) == 0x80); /// High-order bit of the accumulator replaces the Carry bit
      Reg.a = (Reg.a << 1) & carry; /// Rotate left, Carry bit replaces the *low-order bit of the accumulator
      break; // * Originally high-order, but I followed low-order to match diagram depicted.
   }
   case 0x1F: // RAR         1     CY             A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
   {
      uint8_t carry = Reg.f.c; // Copy of carry bit
      Reg.f.c = ((Reg.a & 0x01) == 0x01); /// Low-order bit of the accumulator replaces the Carry bit
      Reg.a = (Reg.a >> 1) & (carry << 7); // Rotate right, Carry bit replaces the high-order bit of the accumulator
      break;
   }

   // REGISTER PAIR INSTRUCTIONS: PUSH, POP, DAD INX, DCX, XCHG, XTHL, SPHL
   case 0xC5: // PUSH B      1                    (sp-2)<-C; (sp-1)<-B; sp <- sp - 2
      PUSH(Reg.bc); break;
   case 0xD5: // PUSH D      1                    (sp-2)<-E; (sp-1)<-D; sp <- sp - 2
      PUSH(Reg.de); break;
   case 0xE5: // PUSH H      1                    (sp-2)<-L; (sp-1)<-H; sp <- sp - 2
      PUSH(Reg.hl); break;
   case 0xF5: // PUSH PSW    1                    (sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
   {
      Reg.flagByte = // Convert flags to byte
         (Reg.f.s << 7) |
         (Reg.f.z << 6) |
         (0 << 5)       |
         (Reg.f.a << 4) |
         (0 << 3)       |
         (Reg.f.p << 2) |
         (1 << 1)       |
         (Reg.f.c << 0);

      PUSH(Reg.psw);

      break;
   }

   case 0xC1: // POP B       1                    C <- (sp); B <- (sp+1); sp <- sp+2
      Reg.bc = POP(); break;
   case 0xD1: // POP D       1                    E <- (sp); D <- (sp+1); sp <- sp+2
      Reg.de = POP(); break;
   case 0xE1: // POP H       1                    L <- (sp); H <- (sp+1); sp <- sp+2
      Reg.hl = POP(); break;
   case 0xF1: // POP PSW     1     Z S P CY AC    flags <- (sp); A <- (sp+1); sp <- sp+2
   {
      Reg.psw = POP();

      // Extract flags from byte
      Reg.f.s = ((Reg.flagByte & (1 << 7)) == (1 << 7)); // Sign bit
      Reg.f.z = ((Reg.flagByte & (1 << 6)) == (1 << 6)); // Zero bit
      // Ignore ((Reg.flagByte & (1 << 5)) == (1 << 5));
      Reg.f.a = ((Reg.flagByte & (1 << 4)) == (1 << 4)); // Auxiliary Carry bit
      // Ignore ((Reg.flagByte & (1 << 3)) == (1 << 3));
      Reg.f.p = ((Reg.flagByte & (1 << 2)) == (1 << 2)); // Parity bit
      // Ignore ((Reg.flagByte & (1 << 1)) == (1 << 1));
      Reg.f.c = ((Reg.flagByte & (1 << 0)) == (1 << 0)); // Carry bit

      break;
   }
   
   case 0x09: // DAD B       1     CY             HL = HL + BC
      DAD(Reg.bc); break;
   case 0x19: // DAD D       1     CY             HL = HL + DE
      DAD(Reg.de); break;
   case 0x29: // DAD H       1     CY             HL = HL + HL
      DAD(Reg.hl); break;
   case 0x39: // DAD SP      1     CY             HL = HL + SP
      DAD(Reg.sp); break;

   case 0x03: // INX B       1                    BC <- BC + 1
      Reg.bc += 1; break;
   case 0x13: // INX D       1                    DE <- DE + 1
      Reg.de += 1; break;
   case 0x23: // INX H       1                    HL <- HL + 1
      Reg.hl += 1; break;
   case 0x33: // INX SP      1                    SP = SP + 1
      Reg.sp += 1; break;

   case 0x0B: // DCX B       1                    BC = BC - 1
      Reg.bc -= 1; break;
   case 0x1B: // DCX D       1                    DE = DE - 1
      Reg.de -= 1; break;
   case 0x2B: // DCX H       1                    HL = HL - 1
      Reg.hl -= 1; break;
   case 0x3B: // DCX SP      1                    SP = SP - 1
      Reg.sp -= 1; break;

   case 0xEB: // XCHG        1                    H <-> D; L <-> E
      std::swap(Reg.hl, Reg.de); break;
   
   case 0xE3: // XTHL        1                    L <-> (SP); H <-> (SP+1)
      std::swap(Reg.hl, mem<uint16_t>(Reg.sp)); break;
   
   case 0xF9: // SPHL        1                    SP=HL
      Reg.sp = Reg.hl; break;
   

   // IMMEDIATE INSTRUCTIONS: LXI, MVI, ADI, ACI, SUI, SBI, ANI, XRI, ORI, CPI
   case 0x01: // LXI BD16    3                    B <- byte 3 C <- byte 2
      Reg.bc = imm<uint16_t>(); break;
   case 0x11: // LXI DD16    3                    D <- byte 3 E <- byte 2
      Reg.de = imm<uint16_t>(); break;
   case 0x21: // LXI HD16    3                    H <- byte 3 L <- byte 2
      Reg.hl = imm<uint16_t>(); break;
   case 0x31: // LXI SP D16  3                    SP.hi <- byte 3 SP.lo <- byte 2
      Reg.sp = imm<uint16_t>(); break;

   case 0x06: // MVI B D8    2                    B <- byte 2
      Reg.b = imm<uint8_t>(); break;
   case 0x0E: // MVI C D8    2                    C <- byte 2
      Reg.c = imm<uint8_t>(); break;
   case 0x16: // MVI D D8    2                    D <- byte 2
      Reg.d = imm<uint8_t>(); break;
   case 0x1E: // MVI E D8    2                    E <- byte 2
      Reg.e = imm<uint8_t>(); break;
   case 0x26: // MVI H D8    2                    H <- byte 2
      Reg.h = imm<uint8_t>(); break;
   case 0x2E: // MVI L D8    2                    L <- byte 2
      Reg.l = imm<uint8_t>(); break;
   case 0x36: // MVI M D8    2                    (HL) <- byte 2
      mem<uint8_t>(Reg.hl) = imm<uint8_t>(); break;
   case 0x3E: // MVI A D8    2                    A <- byte 2
      Reg.a = imm<uint8_t>(); break;

   case 0xC6: // ADI D8      2     Z S P CY AC    A <- A + byte
      ADD(imm<uint8_t>()); break;
   case 0xCE: // ACI D8      2     Z S P CY AC    A <- A + data + CY
      ADC(imm<uint8_t>()); break;
   case 0xD6: // SUI D8      2     Z S P CY AC    A <- A - data
      SUB(imm<uint8_t>()); break;
   case 0xDE: // SBI D8      2     Z S P CY AC    A <- A - data - CY
      SBB(imm<uint8_t>()); break;
   case 0xE6: // ANI D8      2     Z S P CY AC    A <- A & data
      ANA(imm<uint8_t>()); break;
   case 0xEE: // XRI D8      2     Z S P CY AC    A <- A ^ data
      XRA(imm<uint8_t>()); break;
   case 0xF6: // ORI D8      2     Z S P CY AC    A <- A | data
      ORA(imm<uint8_t>()); break;
   case 0xFE: // CPI D8      2     Z S P CY AC    A - data
      CMP(imm<uint8_t>()); break;

   // DIRECT ADDRESSING INSTRUCTIONS: STA, LDA, SHLD, LHLD
   case 0x32: // STA adr     3                    (adr) <- A
      mem<uint8_t>(imm<uint16_t>()) = Reg.a; break;
   case 0x3A: // LDA adr     3                    A <- (adr)
      Reg.a = mem<uint8_t>(imm<uint16_t>()); break;

   case 0x22: // SHLD adr    3                    (adr) <-L; (adr+1)<-H
      mem<uint16_t>(imm<uint16_t>()) = Reg.hl; break;
   case 0x2A: // LHLD adr    3                    L <- (adr); H<-(adr+1)
      Reg.hl = mem<uint16_t>(imm<uint16_t>()); break;

   // JUMP INSTRUCTIONS: PCHL, JMP, JC, JNC, JZ, JNZ, JM, JP, JPE, JPO
   case 0xE9: // PCHL        1                    pc.hi <- H; pc.lo <- L
      Reg.pc = Reg.hl; break;

   case 0xC3: // JMP adr     3                    pc <- adr
      Reg.pc = imm<uint16_t>(); break;

   case 0xC2: // JNZ adr     3                    if NZ pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.z != SET) Reg.pc = jump; break;
   case 0xCA: // JZ  adr     3                    if Z  pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.z == SET) Reg.pc = jump; break;
   case 0xD2: // JNC adr     3                    if NC pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.c != SET) Reg.pc = jump; break;
   case 0xDA: // JC  adr     3                    if C  pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.c == SET) Reg.pc = jump; break;
   case 0xE2: // JPO adr     3                    if PO pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.p != SET) Reg.pc = jump; break;
   case 0xEA: // JPE adr     3                    if PE pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.p == SET) Reg.pc = jump; break;
   case 0xF2: // JP  adr     3                    if P  pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.s != SET) Reg.pc = jump; break;
   case 0xFA: // JM  adr     3                    if M  pc <- adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.s == SET) Reg.pc = jump; break;

   // CALL SUBROUTINE INSTRUCTIONS: CALL, CC, CNC, CZ, CNZ, CM, CP, CPE, CPO
   case 0xCD: // CALL adr    3                    (SP-1) <- pc.hi; (SP-2) <- pc.lo; SP <- SP + 2; pc = adr
   {
#ifdef FOR_CPUDIAG
      if (imm<uint16_t>() == 5) {
         if (Reg.c == 9) {
            for (char* str = (char*)&memory[Reg.de + 3]; *str != '$'; str++)
               printf("%c", *str);
            printf("\n");
            exit(0);
         } else printf("print char routine called\n");
      }
#endif // FOR_CPUDIAG
      CALL(imm<uint16_t>());
      break;
   }
   case 0xC4: // CNZ adr     3                    if NZ CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.z != SET) CALL(jump); break;
   case 0xCC: // CZ  adr     3                    if Z  CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.z == SET) CALL(jump); break;
   case 0xD4: // CNC adr     3                    if NC CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.c != SET) CALL(jump); break;
   case 0xDC: // CC  adr     3                    if C  CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.c == SET) CALL(jump); break;
   case 0xE4: // CPO adr     3                    if PO CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.p != SET) CALL(jump); break;
   case 0xEC: // CPE adr     3                    if PE CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.p == SET) CALL(jump); break;
   case 0xF4: // CP  adr     3                    if P  CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.s != SET) CALL(jump); break;
   case 0xFC: // CM  adr     3                    if M  CALL adr
      uint16_t jump = imm<uint16_t>(); if (Reg.f.s == SET) CALL(jump); break;

   // RETURN FROM SUBROUTINE INSTRUCTIONS: RET, RN, RNC, RZ, RNZ, RM, RP, RPE, RPO
   case 0xC9: // RET         1                    pc.lo <- (sp); pc.hi <- (sp + 1); SP <- SP + 2
      RET(); break;
   
   case 0xC0: // RNZ         1                    if NZ RET
      if (Reg.f.z != SET) RET(); break;
   case 0xC8: // RZ          1                    if Z  RET
      if (Reg.f.z == SET) RET(); break;
   case 0xD0: // RNC         1                    if NC RET
      if (Reg.f.c != SET) RET(); break;
   case 0xD8: // RC          1                    if C  RET
      if (Reg.f.c == SET) RET(); break;
   case 0xE0: // RPO         1                    if PO RET
      if (Reg.f.p != SET) RET(); break;
   case 0xE8: // RPE         1                    if PE RET
      if (Reg.f.p == SET) RET(); break;
   case 0xF0: // RP          1                    if P  RET
      if (Reg.f.s != SET) RET(); break;
   case 0xF8: // RM          1                    if M  RET
      if (Reg.f.s == SET) RET(); break;

   // RST INSTRUCTION
   case 0xC7: // RST 0       1                    CALL $0
      CALL(0x0); break;
   case 0xCF: // RST 1       1                    CALL $8
      CALL(0x8); break;
   case 0xD7: // RST 2       1                    CALL $10
      CALL(0x10); break;
   case 0xDF: // RST 3       1                    CALL $18
      CALL(0x18); break;
   case 0xE7: // RST 4       1                    CALL $20
      CALL(0x20); break;
   case 0xEF: // RST 5       1                    CALL $28
      CALL(0x28); break;
   case 0xF7: // RST 6       1                    CALL $30
      CALL(0x30); break;
   case 0xFF: // RST 7       1                    CALL $38
      CALL(0x38); break;

   // INTERRUPT FLIP-FLOP INSTRUCTIONS
   case 0xFB: // EI          1                    special
      interrupt_enabled = true;  break;
   case 0xF3: // DI          1                    special
      interrupt_enabled = false; break;

   // INPUT/OUTPUT INSTRUCTIONS: IN, OUT
   case 0xDB: // IN  D8      2                    special
      Reg.a = io.read(imm<uint8_t>()); break;
   case 0xD3: // OUT D8      2                    special
      io.write(imm<uint8_t>(), Reg.a); break;

   // HLT HALT INSTRUCTION
   case 0x76: // HLT         1                    special
      stopped = true; break;

   // unused
   default: // 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0xcb, 0xd9, 0xdd, 0xed, 0xfd
      break;
   }
}
