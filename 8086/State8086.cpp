#include "State8086.h"
#include "alu.h"
#include <algorithm>
#include <cassert>

bool continueFindLoop(int repeatType, uint8_t ZF, uint16_t &counter) {
   if (repeatType == 0)
      return false; // Not in a loop

   // Repeat prefix exists. Decrement counter
   counter--;

   // Break repeat loop if condition is met
   if ((repeatType == 1) && (ZF == 0))
      return false;
   if ((repeatType == 2) && (ZF == 1))
      return false;

   // Condition not met. Continue loop
   return true;
}

bool continueLoop(int repeatType, uint16_t &counter) {
   if (repeatType == 0)
      return false; // Not in a loop

   // Repeat prefix exists; decrement counter
   counter--;
   return true;
}

void State8086::interrupt(uint8_t vector) {

}

template<>
uint8_t& State8086::fetchreg(uint8_t rmval) {
   assert(rmval < 8);
   switch (rmval) {
   case 0: return d_regs.a.l;
   case 1: return d_regs.c.l;
   case 2: return d_regs.d.l;
   case 3: return d_regs.b.l;
   case 4: return d_regs.a.h;
   case 5: return d_regs.c.h;
   case 6: return d_regs.d.h;
   default:return d_regs.b.h;
   }
}
template<>
uint16_t& State8086::fetchreg(uint8_t rmval) {
   assert(rmval < 8);
   switch (rmval) {
   case 0: return d_regs.a.x;
   case 1: return d_regs.c.x;
   case 2: return d_regs.d.x;
   case 3: return d_regs.b.x;
   case 4: return pi_regs.SP;
   case 5: return pi_regs.BP;
   case 6: return pi_regs.SI;
   default:return pi_regs.DI;
   }
}

void State8086::modregrm() {

}

void State8086::push(uint16_t value) {

}

uint16_t State8086::pop() {
   return 0;
}

int State8086::getea(uint8_t rmval) {
   return 0;
}


template<> void State8086::Flags::set(uint8_t x) {
   C = x & (1 << 0x0) ? 1 : 0;
   P = x & (1 << 0x2) ? 1 : 0;
   A = x & (1 << 0x4) ? 1 : 0;
   Z = x & (1 << 0x6) ? 1 : 0;
   S = x & (1 << 0x7) ? 1 : 0;
}

template<> void State8086::Flags::set(uint16_t x) {
   C = x & (1 << 0x0) ? 1 : 0;
   P = x & (1 << 0x2) ? 1 : 0;
   A = x & (1 << 0x4) ? 1 : 0;
   Z = x & (1 << 0x6) ? 1 : 0;
   S = x & (1 << 0x7) ? 1 : 0;
   T = x & (1 << 0x8) ? 1 : 0;
   I = x & (1 << 0x9) ? 1 : 0;
   D = x & (1 << 0xA) ? 1 : 0;
   O = x & (1 << 0xB) ? 1 : 0;
}

template<> uint8_t State8086::Flags::get() {
   return 2 // Can't tell if bit 1 is supposed to be set or not.
      | ((uint16_t)C << 0x0)
      | ((uint16_t)P << 0x2)
      | ((uint16_t)A << 0x4)
      | ((uint16_t)Z << 0x6)
      | ((uint16_t)S << 0x7);
}

template<> uint16_t State8086::Flags::get() {
   return 2 // Can't tell if bit 1 is supposed to be set or not.
      | ((uint16_t)C << 0x0)
      | ((uint16_t)P << 0x2)
      | ((uint16_t)A << 0x4)
      | ((uint16_t)Z << 0x6)
      | ((uint16_t)S << 0x7)
      | ((uint16_t)T << 0x8)
      | ((uint16_t)I << 0x9)
      | ((uint16_t)D << 0xA)
      | ((uint16_t)O << 0xB);
}

void State8086::Emulate8086Op(int runtime)
{
   uint8_t opcode = 0;
   bool finished = false;
   int repeatType = 0;
   uint16_t saveIP;
   bool prefix, trap_toggle = false;


   for (int count = 0; count < runtime;) {
      if (trap_toggle)
         interrupt(1);

      trap_toggle = flags.T;

      //if (!trap_toggle && flags.I)
      //   ;


      if (halted) // Halt state
         return;


      saveIP = IP; // Used if repeat prefix is present (to go back and execute instruction again)
      prefix = true; // Assume next byte is a prefix for an opcode
      repeatType = 0; // 0 if no repeat, 1 if NOT EQUAL repeat, 2 if EQUAL
      // The default segment register is SS for the effective addresses
      // containing a BP index, DS for other effective addresses (pg 29).
      // An override prefix overrides this.
      // A call to modregrm() may override too.
      uint16_t segment = segRegs.DS;
      bool segoverride = false; // 

      do { // Grab next byte. If it's a prefix byte, process it and grab another byte and repeat, else move on
         //opcode = mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         count++;

         switch (opcode) {
            // These are prefix bytes. Causes memory access to use specified segment instead of default segment designated for instruction operand
            // SEGMENT = Override prefix:         [001 reg 110]
         case 0x26: // SEG =ES
         case 0x2E: // SEG =CS
         case 0x36: // SEG =SS
         case 0x3E: // SEG =DS
            segment = segRegs[(opcode >> 3) & 3];
            segoverride = true;
            break;

            // REP = Repeat:
            // [1111001 z]
         case 0xF2: // REPNE
                    // This is a prefix byte
                    // Opcode   Instruction          Description (pg 434)
                    // F2 A6    REPNE CMPS m8,m8     Find matching bytes in ES:[(E)DI] and DS:[(E)SI]
                    // F2 A7    REPNE CMPS m16,m16   Find matching words in ES:[(E)DI] and DS:[(E)SI]
                    // F2 AE    REPNE SCAS m8        Find AL, starting at ES:[(E)DI]
                    // F2 AF    REPNE SCAS m16       Find AX, starting at ES:[(E)DI]
            repeatType = 1;
            break;
         case 0xF3: // REP REPE
                    // This is a prefix byte
                    // Opcode   Instruction          Description (pg 434)
                    // F3 6C    REP INS r/m8, DX     Input (E)CX bytes from port DX into ES:[(E)DI]
                    // F3 6D    REP INS r/m16,DX     Input (E)CX words from port DX into ES:[(E)DI]
                    // F3 6E    REP OUTS DX,r/m8     Output (E)CX bytes from DS:[(E)SI] to port DX
                    // F3 6F    REP OUTS DX,r/m16    Output (E)CX words from DS:[(E)SI] to port DX
                    // F3 A4    REP MOVS m8,m8       Move (E)CX bytes from DS:[(E)SI] to ES:[(E)DI]
                    // F3 A5    REP MOVS m16,m16     Move (E)CX words from DS:[(E)SI] to ES:[(E)DI]
                    // F3 AA    REP STOS m8          Fill (E)CX bytes at ES:[(E)DI] with AL
                    // F3 AB    REP STOS m16         Fill (E)CX words at ES:[(E)DI] with AX
                    // F3 AC    REP LODS AL          Load (E)CX bytes from DS:[(E)SI] to AL
                    // F3 AD    REP LODS AX          Load (E)CX words from DS:[(E)SI] to AX
                    // F3 A6    REPE CMPS m8,m8      Find nonmatching bytes in ES:[(E)DI] and DS:[(E)SI]
                    // F3 A7    REPE CMPS m16,m16    Find nonmatching words in ES:[(E)DI] and DS:[(E)SI]
                    // F3 AE    REPE SCAS m8         Find non-AL byte starting at ES:[(E)DI]
                    // F3 AF    REPE SCAS m16        Find non-AX word starting at ES:[(E)DI]
            repeatType = 2;
            break;

         default: // Normal opcode with no repeat or segment override
            prefix = false;
            break;
         }

      } while (prefix);


      switch (opcode) {
         //******       Data Transfer      ******//
         //****** MOV PUSH POP XCHG IN OUT ******//

      // MOV = Move:

      // Register/memory to/from register
      // [100010 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      // 
      case 0x88: // MOV r/m8,r8        move r8 to r/m8 (pg 316)
      {
         modregrm(); // fetch the ModR/M byte
         writerm<uint8_t>(rm, fetchreg<uint8_t>(reg));
         break;
      }
      case 0x89: // MOV r/m16,r16      move r16 to r/m16 (pg 316)
      {
         modregrm();
         writerm<uint8_t>(rm, (uint8_t)fetchreg<uint16_t>(reg));
         break;
      }
      case 0x8A: // MOV r8,r/m8        move r/m8 to r8 (pg 316)
      {
         modregrm();
         fetchreg<uint8_t>(reg) = readrm<uint8_t>(rm);
         break;
      }
      case 0x8B: // MOV r16,r/m16      move r/m16 to r16 (pg 316)
      {
         modregrm();
         fetchreg<uint16_t>(reg) = readrm<uint16_t>(rm);
         break;
      }
      // Immediate to register/memory
      // [1100011 w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]
      case 0xC6: // Move group
                 // /0 MOV r/m8,imm8   move imm8 to r/m8 (pg 316)
                 // /1-7 -unused-
      {
         modregrm();
         writerm<uint8_t>(rm, mem->getmem<uint8_t>(segRegs.CS, IP));
         incrementIP(1);
         break;
      }
      case 0xC7: // Move group
                 // /0 MOV r/m16,imm16 move imm16 to r/m16 (pg 316)
                 // /1-7 -unused-
      {
         modregrm();
         writerm<uint16_t>(rm, mem->getmem<uint16_t>(segRegs.CS, IP));
         incrementIP(2);
         break;
      }
      // Immediate to register
      // [1011 w reg] [data] [data if w=1]
      case 0xB0: // MOV AL             move imm8 to r8 (pg 316)
      case 0xB1: // MOV CL             move imm8 to r8 (pg 316)
      case 0xB2: // MOV DL             move imm8 to r8 (pg 316)
      case 0xB3: // MOV BL             move imm8 to r8 (pg 316)
      case 0xB4: // MOV AH             move imm8 to r8 (pg 316)
      case 0xB5: // MOV CH             move imm8 to r8 (pg 316)
      case 0xB6: // MOV DH             move imm8 to r8 (pg 316)
      case 0xB7: // MOV BH             move imm8 to r8 (pg 316)
      { // fall through for cases 0xB0-0xB7
         reg = opcode & 7; // reg in lower 3 bits
         fetchreg<uint8_t>(reg) = mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         break;
      }
      case 0xB8: // MOV AX             move imm16 to r16 (pg 316)
      case 0xB9: // MOV CX             move imm16 to r16 (pg 316)
      case 0xBA: // MOV DX             move imm16 to r16 (pg 316)
      case 0xBB: // MOV BX             move imm16 to r16 (pg 316)
      case 0xBC: // MOV SP             move imm16 to r16 (pg 316)
      case 0xBD: // MOV BP             move imm16 to r16 (pg 316)
      case 0xBE: // MOV SI             move imm16 to r16 (pg 316)
      case 0xBF: // MOV DI             move imm16 to r16 (pg 316)
      { // fall through for cases 0xB8-0xBF
         reg = opcode & 7; // reg in lower 3 bits
         fetchreg<uint16_t>(reg) = mem->getmem<uint16_t>(segRegs.CS, IP);
         incrementIP(2);
         break;
      }
      // Memory to accumulator
      // [1010000 w] [addr-lo] [addr-hi]
      case 0xA0: // MOV AL,moffs8      move byte at (seg:offset) to AL (pg 316)
      {
         // I don't know what segment is being used here
         d_regs.a.l = mem->getmem<uint8_t>(segment, mem->getmem<uint16_t>(segRegs.CS, IP));
         incrementIP(2);
         break;
      }
      case 0xA1: // MOV AX,moffs16     move word at (seg:offset) to AX (pg 316)
      {
         d_regs.a.x = mem->getmem<uint16_t>(segment, mem->getmem<uint16_t>(segRegs.CS, IP));
         incrementIP(2);
         break;
      }
      // Accumulator to memory
      // [1010001 w] [addr-lo] [addr-hi]
      case 0xA2: // MOV moffs8,AL      move AL to (seg:offset) (pg 316)
      {
         mem->putmem<uint8_t>(segment, mem->getmem<uint16_t>(segRegs.CS, IP), d_regs.a.l);
         incrementIP(2);
         break;
      }
      case 0xA3: // MOV moffs16,AX     move AX to (seg:offset) (pg 316)
      {
         mem->putmem<uint16_t>(segment, mem->getmem<uint16_t>(segRegs.CS, IP), d_regs.a.x);
         incrementIP(2);
         break;
      }
      // Register/memory to segment register
      // [10001110] [mod 0 SR r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8E: // MOV Sreg,r/m16     move r/m16 to segment register (pg 316)
      {
         modregrm();
         segRegs[reg] = readrm<uint16_t>(rm);
         break;
      }
      // Segment register to register/memory
      case 0x8C: // MOV r/m16,Sreg     move segment register to r/m16 (pg 316)
      {
         modregrm();
         writerm<uint16_t>(rm, segRegs[reg]);
         break;
      }

      // PUSH = Push:

      // Register
      // [01010 reg]
      case 0x50: // PUSH AX            push AX (page 415)
      case 0x51: // PUSH CX            push CX (page 415)
      case 0x52: // PUSH DX            push DX (page 415)
      case 0x53: // PUSH BX            push BX (page 415)
      case 0x54: // PUSH SP            push SP (page 415)
      case 0x55: // PUSH BP            push BP (page 415)
      case 0x56: // PUSH SI            push SI (page 415)
      case 0x57: // PUSH DI            push DI (page 415)
      {
         reg = opcode & 7;
         push(fetchreg<uint16_t>(reg));
         break;
      }
      // Segment register
      // [000 reg 110]
      case 0x06: // PUSH ES            push ES (pg 415)
      case 0x0E: // PUSH CS            push CS (pg 415)
      case 0x16: // PUSH SS            push SS (pg 415)
      case 0x1E: // PUSH DS            push DS (pg 415)
      {
         reg = (opcode >> 3) & 3;
         push(segRegs[reg]);
         break;
      }

      // POP = Pop:

      // Register/memory
      // [10001111] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8F: // /0 POP m16            pop top of stack into m16; increment stack pointer (pg 380)
      {
         modregrm();
         writerm<uint16_t>(rm, pop());
         break;
      }
      // Register
      // [01011 reg]
      case 0x58: // POP AX             pop top of stack into AX; increment stack pointer (pg 380)
      case 0x59: // POP CX             pop top of stack into CX; increment stack pointer (pg 380)
      case 0x5A: // POP DX             pop top of stack into DX; increment stack pointer (pg 380)
      case 0x5B: // POP BX             pop top of stack into BX; increment stack pointer (pg 380)
      case 0x5C: // POP SP             pop top of stack into SP; increment stack pointer (pg 380)
      case 0x5D: // POP BP             pop top of stack into BP; increment stack pointer (pg 380)
      case 0x5E: // POP SI             pop top of stack into SI; increment stack pointer (pg 380)
      case 0x5F: // POP DI             pop top of stack into DI; increment stack pointer (pg 380)
      {
         reg = opcode & 7;
         fetchreg<uint16_t>(reg) = pop();
         break;
      }
      // Segment register
      // [000 reg 111]
      case 0x07: // POP ES             pop top of stack into ES; increment stack pointer (pg 380)
      case 0x0F: // POP CS             pop top of stack into CS; increment stack pointer (only the 8086/8088 does this) *future cpu's use this as an escape character
      case 0x17: // POP SS             pop top of stack into SS; increment stack pointer (pg 380)
      case 0x1F: // POP DS             pop top of stack into DS; increment stack pointer (pg 380)
      {
         reg = (opcode >> 3) & 3;
         segRegs[reg] = pop();

         break;
      }

      // XCHG = Exchange:

      // Register/memory with register
      // [1000011 w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x86: // XCHG r/m8,r8       exchange r8 (byte register) with byte from r/m8 (pg 492)
      {
         modregrm();

         uint8_t temp = fetchreg<uint8_t>(reg);
         fetchreg<uint8_t>(reg) = readrm<uint8_t>(rm);
         writerm<uint8_t>(rm, temp);

         break;
      }
      case 0x87: // XCHG r/m16,r16     exchange r16 with word from r/m16 (pg 492)
      {
         modregrm();

         uint16_t temp = fetchreg<uint16_t>(reg);
         fetchreg<uint16_t>(reg) = readrm<uint16_t>(rm);
         writerm<uint16_t>(rm, temp);

         break;
      }
      // Register with accumulator
      // [10010 reg]
      case 0x90: // NOP                no operation (pg 340)
      case 0x91: // XCHG CS            exchange CX with AX (pg 492)
      case 0x92: // XCHG DX            exchange DX with AX (pg 492)
      case 0x93: // XCHG BS            exchange BX with AX (pg 492)
      case 0x94: // XCHG SP            exchange SP with AX (pg 492)
      case 0x95: // XCHG BP            exchange BP with AX (pg 492)
      case 0x96: // XCHG SI            exchange SI with AX (pg 492)
      case 0x97: // XCHG DI            exchange DI with AX (pg 492)
      { // fall through for case 0x90-0x97
         reg = opcode & 7; // reg in lower 3 bits

         std::swap(d_regs.a.x, fetchreg<uint16_t>(reg));

         break;
      }

      // IN = Input from:

      // Fixed port
      // [1110010 w] [DATA-8]
      case 0xE4: // IN AL,imm8         input byte from imm8 I/O port address into AL (pg 241)
      {
         d_regs.a.l = io->read<uint8_t>(mem->getmem<uint8_t>(segRegs.CS, IP));
         incrementIP(1);
         break;
      }
      case 0xE5: // IN AX,imm8         input *word from imm8 I/O port address into AX (pg 241) *change to match pattern of OUT (pg 345)
      {
         d_regs.a.x = io->read<uint16_t>(mem->getmem<uint8_t>(segRegs.CS, IP));
         incrementIP(1);
         break;
      }
      // Variable port
      // [1110110 w]
      case 0xEC: // IN AL,DX           input byte from I/O port in DX into AL (pg 241)
      {
         d_regs.a.l = io->read<uint8_t>(d_regs.d.x);
         break;
      }
      case 0xED: // IN AX,DX           input word from I/O port in DX into AX (pg 241)
      {
         d_regs.a.x = io->read<uint16_t>(d_regs.d.x);
         break;
      }

      // OUT = Output to:

      // Fixed port
      // [1110011 w] [DATA-8]
      case 0xE6: // OUT imm8,AL        output byte in AL to I/O port address imm8 (pg 345)
      {
         io->write<uint8_t>(mem->getmem<uint8_t>(segRegs.CS, IP), d_regs.a.l);
         incrementIP(1);
         break;
      }
      case 0xE7: // OUT imm8,AX        output word in AX to I/O port address imm8 (pg 345)
      {
         io->write<uint16_t>(mem->getmem<uint8_t>(segRegs.CS, IP), d_regs.a.x);
         incrementIP(1);
         break;
      }
      // Variable port
      // [1110111 w]
      case 0xEE: // OUT DX,AL          output byte in AL to I/O port address in DX (pg 345)
      {
         io->write<uint8_t>(d_regs.d.x, d_regs.a.l);
         break;
      }
      case 0xEF: // OUT DX,AX          output word in AX to I/O port address in DX (pg 345)
      {
         io->write<uint16_t>(d_regs.d.x, d_regs.a.x);
         break;
      }
      // XLAT = Translate byte to AL:
      // [11010111]
      case 0xD7: // XLAT m8            set AL to memory byte DS:[(E)BX+unsigned AL] (pg 494)
      {
         d_regs.a.l = mem->getmem<uint8_t>(segment, d_regs.b.x + d_regs.a.l);
         break;
      }
      // LEA = Load EA to register:
      // [10001101] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8D: // LEA r16,m          store effective address for m in register r16 (pg 289)
      {
         modregrm();
         //putreg16(reg, getea(rm)); // TODO: other guy used "ea - segbase(useseg)" instead of just "ea"
         fetchreg<uint16_t>(reg) = getea(rm);
         break;
      }
      // LES = Load pointer to ES:
      // [11000100] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xC4: // LES r16,m16:16     load ES:r16 with far pointer from memory (pg 286)
      {
         modregrm();
         int ea = getea(rm);
         fetchreg<uint16_t>(reg) = mem->getmem<uint16_t>(ea);
         segRegs.ES = mem->getmem<uint16_t>(ea + 2);
         break;
      }
      // LDS = Load pointer to DS:
      // [11000101] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xC5: // LDS r16,m16:16     load DS:r16 with far pointer from memory (pg 286)
      {
         modregrm();
         int ea = getea(rm);
         fetchreg<uint16_t>(reg) = mem->getmem<uint16_t>(ea);
         segRegs.DS = mem->getmem<uint16_t>(ea + 2);
         break;
      }
      // PUSHF = Push flags:
      // [10011100]
      case 0x9C: // PUSHF              push lower 16 bits of EFLAGS (pg 420)
      {
         push(flags.get<uint16_t>());
         break;
      }
      // POPF = Pop flags:
      // [10011101]
      case 0x9D: // POPF               pop top of stack into lower 16 bits of EFLAGS (pg 386)
      {
         flags.set<uint16_t>(pop());
         break;
      }
      // SAHF = Store AH into flags:
      // [10011110]
      case 0x9E: // SAHF               loads SF,ZF,AF,PF, and CF from AH into EFLAGS register (pg 445)
      {
         flags.set<uint8_t>(d_regs.a.h);
         break;
      }
      // LAHF = Load AH with flags:
      // [10011111]
      case 0x9F: // LAHF               load: AH=EFLAGS(SF:ZF:0:AF:0:PF:1:CF) (pg 282)
      {
         d_regs.a.h = flags.get<uint8_t>();
         break;
      }

      //******************************************************************************************//
      //******                                  Arithmetic                                  ******//
      //****** ADD ADC INC AA DAA SUB SBB DEC CMP AAS DAS MUL IMUL AAM DIV IDIV AAD CBW CWD ******//
      //******************************************************************************************//

   // Reg/memory with register to either
   // [000000 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] ADD = Add:
   // [000010 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] OR  = Or:
   // [000100 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] ADC = Add with carry:
   // [000110 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] SBB = Subtract with borrow:
   // [001000 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] AND = And:
   // [001010 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] SUB = Subtract:
   // [001100 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] XOR = Exclusive or:
   // [001110 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] CMP = Compare:
      case 0x00: // ADD r/m8,r8        add r8 to r/m8 (pg 47)
      case 0x08: // OR  r/m8,r8        r/m8 OR r8 (pg 343)
      case 0x10: // ADC r/m8,r8        add with carry byte register to r/m8 (pg 45)
      case 0x18: // SBB r/m8,r8        subtract with borrow r8 from r/m8 (pg 450)
      case 0x20: // AND r/m8,r8        r/m8 AND r8 (pg 49)
      case 0x28: // SUB r/m8,r8        subtract r8 from r/m8 (pg 478)
      case 0x30: // XOR r/m8,r8        r/m8 XOR r8 (pg 496)
      case 0x38: // CMP r/m8,r8        compare r8 with r/m8 (pg 91)
      {
         auto op = (opcode >> 3) & 7;

         modregrm();
         uint8_t op1 = readrm<uint8_t>(rm);
         uint8_t op2 = fetchreg<uint8_t>(reg);
         uint8_t res = ALU::arithm<uint8_t>()[op](op1, op2, flags);

         if (op < 7) writerm<uint8_t>(rm, res);
         break;
      }

      case 0x01: // ADD r/m16,r16      add r16 to r/m16 (pg 47)
      case 0x09: // OR  r/m16,r16      r/m16 OR r16 (pg 343)
      case 0x11: // ADC r/m16,r16      add with carry r16 to r/m16 (pg 45)
      case 0x19: // SBB r/m16,r16      subtract with borrow r16 from r/m16 (pg 450)
      case 0x21: // AND r/m16,r16      r/m16 AND r16 (pg 49)
      case 0x29: // SUB r/m16,r16      subtract r16 from r/m16 (pg 478)
      case 0x31: // XOR r/m16,r16      r/m16 XOR r16 (pg 496)
      case 0x39: // CMP r/m16,r16      compare r16 with r/m16 (pg 91)
      {
         auto op = (opcode >> 3) & 7; // operation is bits 3-5

         modregrm();
         uint16_t op1 = readrm<uint16_t>(rm);
         uint16_t op2 = fetchreg<uint16_t>(reg);
         uint16_t res = ALU::arithm<uint16_t>()[op](op1, op2, flags);

         if (op < 7) writerm<uint16_t>(rm, res);
         break;
      }

      case 0x02: // ADD r8,r/m8        add r/m8 to r8 (pg 47)
      case 0x0A: // OR  r8,r/m8        r8 OR r/m8 (pg 343)
      case 0x12: // ADC r8,r/m8        add with carry r/m8 to byte register (pg 45)
      case 0x1A: // SBB r8,r/m8        subtract with borrow r/m8 from r8 (pg 450)
      case 0x22: // AND r8,r/m8        r8 AND r/m8 (pg 49)
      case 0x2A: // SUB r8,r/m8        subtract r/m8 from r8 (pg 478)
      case 0x32: // XOR r8,r/m8        r8 XOR r/m8 (pg 496)
      case 0x3A: // CMP r8,r/m8        compare r/m8 with r8 (pg 91)
      {
         auto op = (opcode >> 3) & 7;

         modregrm();
         uint8_t op1 = fetchreg<uint8_t>(reg);
         uint8_t op2 = readrm<uint8_t>(rm);
         uint8_t res = ALU::arithm<uint8_t>()[op](op1, op2, flags);

         if (op < 7)
            fetchreg<uint8_t>(reg) = res;

         break;
      }

      case 0x03: // ADD r16,r/m16      add r/m16 to r16 (pg 47)
      case 0x0B: // OR  r16,r/m16      r16 OR r/m16 (pg 343)
      case 0x13: // ADC r16,r/m16      add with carry r/m16 to r16 (pg 45)
      case 0x1B: // SBB r16,r/m16      subtract with borrow r/m16 from r16 (pg 450)
      case 0x23: // AND r16,r/m16      r16 AND r/m16 (pg 49)
      case 0x2B: // SUB r16,r/m16      subtract r/m16 from r16 (pg 478)
      case 0x33: // XOR r16,r/m16      r16 XOR r/m16 (pg 496)
      case 0x3B: // CMP r16,r/m16      compare r/m16 with r16 (pg 91)
      {
         auto op = (opcode >> 3) & 7;

         modregrm();
         uint16_t op1 = fetchreg<uint16_t>(reg);
         uint16_t op2 = readrm<uint16_t>(rm);
         uint16_t res = ALU::arithm<uint16_t>()[op](op1, op2, flags);

         if (op < 7)
            fetchreg<uint16_t>(reg) = res;
         break;
      }

      // Immediate to accumulator
      // [00000 10 w] [data] [data if w=1] ADD = Add:
      // [00001 10 w] [data] [data if w=1] OR  = Or:
      // [00010 10 w] [data] [data if w=1] ADC = Add with carry:
      // [00011 10 w] [data] [data if w=1] SBB = Subtract with borrow:
      // [00100 10 w] [data] [data if w=1] AND = And:
      // [00101 10 w] [data] [data if w=1] SUB = Subtract:
      // [00110 10 w] [data] [data if w=1] XOR = Exclusive or:
      // [00111 10 w] [data] [data if w=1] CMP = Compare:
      case 0x04: // ADD AL,imm8        add imm8 to AL (pg 47)
      case 0x0C: // OR  AL,imm8        AL OR imm8 (pg 343)
      case 0x14: // ADC AL,imm8        add with carry imm8 to AL (pg 45)
      case 0x1C: // SBB AL,imm8        subtract with borrow imm8 from AL (pg 450)
      case 0x24: // AND AL,imm8        AL AND imm8 (pg 49)
      case 0x2C: // SUB AL,imm8        subtract imm8 from AL (pg 478)
      case 0x34: // XOR AL,imm8        AL XOR imm8 (pg 496)
      case 0x3C: // CMP AL,imm8        compare imm8 with AL (pg 91)
      {
         auto op = (opcode >> 3) & 7;

         uint8_t op1 = d_regs.a.l;
         uint8_t op2 = mem->getmem<uint8_t>(segRegs.CS, IP); incrementIP(1);
         uint8_t res = ALU::arithm<uint8_t>()[op](op1, op2, flags);

         if (op < 7)
            d_regs.a.l = res;

         break;
      }

      case 0x05: // ADD AX,imm16       add imm16 to AX (pg 47)
      case 0x0D: // OR  AX,imm16       AX OR imm16 (pg 343)
      case 0x15: // ADC AX,imm16       add with carry imm16 to AX (pg 45)
      case 0x1D: // SBB AX,imm16       subtract with borrow imm16 from AX (pg 450)
      case 0x25: // AND AX,imm16       AX AND imm16 (pg 49)
      case 0x2D: // SUB AX,imm16       subtract imm16 from AX (pg 478)
      case 0x35: // XOR AX,imm16       AX XOR imm16 (pg 496)
      case 0x3D: // CMP AX,imm16       compare imm16 with AX (pg 91)
      {
         auto op = (opcode >> 3) & 7;

         uint16_t op1 = d_regs.a.x;
         uint16_t op2 = mem->getmem<uint16_t>(segRegs.CS, IP); incrementIP(2);
         uint16_t res = ALU::arithm<uint16_t>()[op](op1, op2, flags);

         if (op < 7)
            d_regs.a.x = res;
         break;
      }

      // Immediate to register/memory
      // [100000 s w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] ADD = Add:
      // [100000 0 w] [mod 001 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]    OR  = Or:
      // [100000 s w] [mod 010 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] ADC = Add with carry:
      // [100000 s w] [mod 011 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] SBB = Subtract with borrow:
      // [100000 0 w] [mod 100 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]    AND = And:
      // [100000 s w] [mod 101 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] SUB = Subtract:
      // [100000 0 w] [mod 110 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]    XOR = Exclusive or:
      // [100000 s w] [mod 111 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] CMP = Compare:
      case 0x80: // Immediate Group r/m8,imm8
                 // [0x80] [mod /# r/m] [(DISP-LO)] [(DISP-HI)] [DATA-8]
                 // /0 ADD r/m8,imm8   add imm8 to r/m8 (pg 47)
                 // /1 OR  r/m8,imm8   r/m8 OR imm8 (pg 343)
                 // /2 ADC r/m8,imm8   add with carry imm8 to r/m8 (pg 45)
                 // /3 SBB r/m8,imm8   subtract with borrow imm8 from r/m8 (pg 450)
                 // /4 AND r/m8,imm8   r/m8 AND imm8 (pg 49)
                 // /5 SUB r/m8,imm8   subtract imm8 from r/m8 (pg 478)
                 // /6 XOR r/m8,imm8   r/m8 XOR imm8 (pg 496)
                 // /7 CMP r/m8,imm8   compare imm8 with r/m8 (pg 91)
      case 0x82: // Immediate Group r/m8,imm8 (not referenced in modern documentation)
                 // [0x82] [mod /# r/m] [(DISP-LO)] [(DISP-HI)] [DATA-8]
                 // /0 ADD r/m8,imm8
                 // /1
                 // /2 ADC r/m8,imm8
                 // /3 SBB r/m8,imm8
                 // /4
                 // /5 SUB r/m8,imm8
                 // /6
                 // /7 CMP r/m8,imm8
      {
         modregrm();

         uint8_t op1 = readrm<uint8_t>(rm);
         uint8_t op2 = mem->getmem<uint8_t>(segRegs.CS, IP); incrementIP(1);

         uint8_t res = ALU::arithm<uint8_t>()[reg](op1, op2, flags);

         if (reg != 7) // compare does not write back
            writerm<uint8_t>(rm, res);
         break;
      }
      case 0x81: // Immediate Group r/m16,imm16
                 // [0x81] [mod /# r/m] [(DISP-LO)] [(DISP-HI)] [DATA-LO] [DATA-HI]
                 // /0 ADD r/m16,imm16 add imm16 to r/m16 (pg 47)
                 // /1 OR  r/m16,imm16 r/m16 OR imm16 (pg 343)
                 // /2 ADC r/m16,imm16 add with carry imm16 to r/m16 (pg 45)
                 // /3 SBB r/m16,imm16 subtract with borrow imm16 from r/m16 (pg 450)
                 // /4 AND r/m16,imm16 r/m16 AND imm16 (pg 49)
                 // /5 SUB r/m16,imm16 subtract imm16 from r/m16 (pg 478)
                 // /6 XOR r/m16,imm16 r/m16 XOR imm16 (pg 496)
                 // /7 CMP r/m16,imm16 compare imm16 with r/m16 (pg 91)
      case 0x83: // Immediate Group r/m16,imm8
                 // [0x83] [mod /# r/m] [(DISP-LO)] [(DISP-HI)] [DATA-SX]
                 // /0 ADD r/m16,imm8  add sign-extended imm8 to r/m16 (pg 47)
                 // /1
                 // /2 ADC r/m16,imm8  add with CF sign-extended imm8 to r/m16 (pg 45)
                 // /3 SBB r/m16,imm8  subtract with borrow sign-extended imm8 from r/m16 (pg 450)
                 // /4
                 // /5 SUB r/m16,imm8  subtract sign-extended imm8 from r/m16 (pg 478)
                 // /6
                 // /7 CMP r/m16,imm8  compare imm8 with r/m16 (pg 91)
      {
         modregrm();

         uint16_t op1 = readrm<uint16_t>(rm);
         uint16_t op2;
         if (opcode == 0x81) {
            op2 = mem->getmem<uint16_t>(segRegs.CS, IP);
            incrementIP(2);
         } else { // sign extend byte into word
            op2 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
            incrementIP(1);
         }

         uint16_t res = ALU::arithm<uint16_t>()[reg](op1, op2, flags);

         if (reg != 7)
            writerm<uint16_t>(rm, res);
         break;
      }


      // INC = Increment:


      // [11111111] [mod xxx r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xFF: // Group 5
                 // /0 INC  r/m16      increment r/m word by 1 (pg 243)
                 // /1 DEC  r/m16      decrement r/m16    by 1 (pg 112)
                 // /2 CALL r/m16      call near, absolute indirect, address given in r/m16  (pg 68)
                 // /3 CALL m16:16     call far,  absolute indirect, address given in m16:16 (pg 68)
                 // /4 JMP  r/m16      jump near, absolute,          address given in r/m16  (pg 275)
                 // /5 JMP  ptr16:16   jump far,  absolute,          address given in m16:16 (pg 275)
                 // /6 PUSH r/m16      push r/m16 (pg 415)
                 // /7 -unused-
      {
         modregrm();
         uint16_t rm16 = readrm<uint16_t>(rm);
         auto ea = getea(rm); // far CALL/JUMP
         auto C = flags.C; // INC/DEC

         switch (reg) {
         case 0:  // /0 INC  r/m16      increment r/m word by 1 (pg 243)
            writerm<uint16_t>(rm, ALU::add<uint16_t>(rm16, 1, flags));
            flags.C = C; // restore carry flag
            break;
         case 1:  // /1 DEC  r/m16      decrement r/m16    by 1 (pg 112)
            writerm<uint16_t>(rm, ALU::sub<uint16_t>(rm16, 1, flags));
            flags.C = C; // restore carry flag
            break;
         case 2:  // /2 CALL r/m16      call near, absolute indirect, address given in r/m16  (pg 68)
            push(IP);
            IP = rm16;
            break;
         case 3:  // /3 CALL m16:16     call far,  absolute indirect, address given in m16:16 (pg 68)
            push(segRegs.CS);
            push(IP);
            IP = mem->getmem<uint16_t>(ea);
            segRegs.CS = mem->getmem<uint16_t>(ea + 2);
            break;
         case 4:  // /4 JMP  r/m16      jump near, absolute,          address given in r/m16  (pg 275)
            IP = rm16;
            break;
         case 5:  // /5 JMP  ptr16:16   jump far,  absolute,          address given in m16:16 (pg 275)
            IP = mem->getmem<uint16_t>(ea);
            segRegs.CS = mem->getmem<uint16_t>(ea + 2);
            break;
         case 6:  // /6 PUSH r/m16      push r/m16 (pg 415)
            push(rm16);
            break;
         }
         break;
      }

      // Register/memory
      // [1111111 w] [mod 001 r/m] [(DISP-LO)] [(DISP-HI)]
      /* see 'case 0xFE' and 'case 0xFF' */
      case 0xFE: // INC/DEC Group
                 // /0 INC r/m8        increment r/m byte by 1 (pg 243)
                 // /1 DEC r/m8        decrement r/m8 by 1 (pg 112)
      {
         modregrm();

         auto C = flags.C; // save carry flag

         switch (reg) {
         case 0: writerm<uint16_t>(rm, ALU::add<uint8_t>(readrm<uint8_t>(rm), 1, flags)); break;
         case 1: writerm<uint8_t>(rm, ALU::sub<uint8_t>(readrm<uint8_t>(rm), 1, flags)); break;
         }

         flags.C = C; // restore carry flag

         break;
      }
      // Register
      // [01000 reg]
      case 0x40: // INC AX             increment AX by 1 (pg 243)
      case 0x41: // INC CX             increment CX by 1 (pg 243)
      case 0x42: // INC DX             increment DX by 1 (pg 243)
      case 0x43: // INC BX             increment BX by 1 (pg 243)
      case 0x44: // INC SP             increment SP by 1 (pg 243)
      case 0x45: // INC BP             increment BP by 1 (pg 243)
      case 0x46: // INC SI             increment SI by 1 (pg 243)
      case 0x47: // INC DI             increment DI by 1 (pg 243)
      {  // case 0x40-0x47 fall through
         reg = opcode & 7;
         auto C = flags.C; // save carry flag
         fetchreg<uint16_t>(reg) = ALU::add<uint16_t>(fetchreg<uint16_t>(reg), 1, flags);
         flags.C = C; // restore carry flag
         break;
      }
      // AAA = ASCII adjust for add:
      // [00110111]
      case 0x37: // AAA                ASCII adjust AL after addition (pg 41)
      {
         if (((d_regs.a.l & 0xF) > 9) || (flags.A == 1)) {
            d_regs.a.l += 6;
            d_regs.a.h += 1;
            flags.A = 1;
            flags.C = 1;
         } else {
            flags.A = 0;
            flags.C = 0;
         }
         d_regs.a.l &= 0xF;
         break;
      }
      // DAA = Decimal adjust for add:
      // [00100111]
      case 0x27: // DAA                Decimal adjust AL after addition (pg 109)
      {
         if (((d_regs.a.l & 0xF) > 9) || (flags.A == 1)) {
            uint16_t temp = (uint16_t)d_regs.a.l + 6;
            d_regs.a.l = temp & 0xFF;
            flags.C = (flags.C || (temp & 0xF0)) ? 1 : 0; // detect carry out of lower 4 bits, or old carry
            flags.A = 1;
         } else
            flags.A = 0;

         if (((d_regs.a.l & 0xF0) > 0x90) || (flags.C == 1)) {
            d_regs.a.l += 0x60;
            flags.C = 1;
         } else
            flags.C = 0;

         //ALU::setFlags(d_regs.a.l, flags);
         setFlags<uint8_t>(d_regs.a.l, flags); // TODO: put this in the right spot

         break;
      }

      // DEC = Decrement:

      // Register
      // [01001 reg]
      case 0x48: // DEC AX             decrement AX by 1 (pg 112)
      case 0x49: // DEC CX             decrement CX by 1 (pg 112)
      case 0x4A: // DEC DX             decrement DX by 1 (pg 112)
      case 0x4B: // DEC BX             decrement BX by 1 (pg 112)
      case 0x4C: // DEC SP             decrement SP by 1 (pg 112)
      case 0x4D: // DEC BP             decrement BP by 1 (pg 112)
      case 0x4E: // DEC SI             decrement SI by 1 (pg 112)
      case 0x4F: // DEC DI             decrement DI by 1 (pg 112)
      {
         reg = opcode & 7;
         auto C = flags.C; // save carry flag
         fetchreg<uint16_t>(reg) = ALU::sub<uint16_t>(fetchreg<uint16_t>(reg), 1, flags);
         flags.C = C; // restore carry flag
         break;
      }

      // [1111011 w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]
      // [1111011 w] [mod 010 r/m] [(DISP-LO)] [(DISP-HI)] NOT = Invert
      // [1111011 w] [mod 011 r/m] [(DISP-LO)] [(DISP-HI)] NEG = Change sign
      // [1111011 w] [mod 100 r/m] [(DISP-LO)] [(DISP-HI)] MUL = Multiply (unsigned)
      // [1111011 w] [mod 101 r/m] [(DISP-LO)] [(DISP-HI)] IMUL = Integer multiply (signed)
      // [1111011 w] [mod 110 r/m] [(DISP-LO)] [(DISP-HI)] DIV = Divide (unsigned)
      // [1111011 w] [mod 111 r/m] [(DISP-LO)] [(DISP-HI)] IDIV = Integer divide (signed)
      case 0xF6: // Unary Group imm8
                 // /0 TEST r/m8,imm8     AND imm8 with r/m8; set SF,ZF,PF according to result (pg 480)
                 // /1 -unused-
                 // /2 NOT  r/m8          reverse each bit of r/m8 (pg 341)
                 // /3 NEG  r/m8          two's complement negate r/m8 (pg 338)
                 // /4 MUL  r/m8          unsigned multiply (AX<-AL*r/m8) (pg 336)
                 // /5 IMUL r/m8          AX<-AL*r/m byte (pg 238)
                 // /6 DIV  r/m8          unsigned divide AX by r/m8; AL <- quotient, AH <- remainder (pg 114)
                 // /7 IDIV r/m8          signed divide AX (where AH must contain sign-extended of AL) by r/m byte. (Results: AL=quotient, AH=remainder) (pg 235)
      case 0xF7: // Unary Group 3^2 Ev
                 // /0 TEST r/m16,imm16   AND imm16 with r/m16; set SF,ZF,PF according to result (pg 480)
                 // /1 -unused-
                 // /2 NOT  r/m16         reverse each bit of r/m16 (pg 341)
                 // /3 NEG  r/m16         two's complement negate r/m16 (pg 338)
                 // /4 MUL  r/m16         unsigned multiply (DX:AX<-AX*r/m16) (pg 336)
                 // /5 IMUL r/m16         DX:AX<-AX*r/m word (pg 238)
                 // /6 DIV  r/m16         unsigned divide D:AX by r/m16; AX <- quotient, DX <- remainder (pg 114)
                 // /7 IDIV r/m16         signed divide DX:AX (where DX must contain sign-extension of AX) by r/m word. (Results: AX=quotient, DX=remainder) (pg 235)

      // AAS = ASCII adjust for subtract:
      // [00111111]
      case 0x3F: // AAS                ASCII adjust AL after subtraction (pg 44)
      {
         if (((d_regs.a.l & 0xF) > 9) || (flags.A == 1)) {
            d_regs.a.l -= 6;
            d_regs.a.h -= 1;
            flags.A = 1;
            flags.C = 1;
         } else {
            flags.A = 0;
            flags.C = 0;
         }

         d_regs.a.l &= 0x0F;
         break;
      }
      // DAS = Decimal adjust for subtract:
      // [00101111]
      case 0x2F: // DAS                Decimal adjust AL after subtraction (pg 111)
      {
         if (((d_regs.a.l & 0xF) > 9) || (flags.A == 1)) {
            uint16_t temp = (uint16_t)d_regs.a.l - 6;
            d_regs.a.l = temp & 0xFF;
            flags.C = (flags.C || (temp & 0xFF00)) ? 1 : 0;
            flags.A = 1;
         } else
            flags.A = 0;

         if (((d_regs.a.l & 0xF0) > 0x90) || (flags.C == 1)) {
            d_regs.a.l -= 0x60;
            flags.C = 1;
         } else
            flags.C = 0;
      }
      // AAM = ASCII adjust for multiply
      // [11010100] [00001010]                       NOTE: Second byte selects number base. 0x8 for octal, 0xA for decimal, 0xC for base 12
      case 0xD4: // AAM                ASCII adjust AX after multiply (pg 43)
      {
         uint8_t base = mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);

         if (base == 0) // divide by 0 error
            break; // TODO: how to handle this? Do I care? No.

         uint8_t temp = d_regs.a.l;
         d_regs.a.h = temp / base;
         d_regs.a.l = temp % base;

         //ALU::setFlags(d_regs.a.x, flag);
         setFlags<uint16_t>(d_regs.a.x, flags);

         break;
      }
      // AAD = ASCII adjust for divide
      // [11010101] [00001010]                       NOTE: Second byte selects number base. 0x8 for octal, 0xA for decimal, 0xC for base 12
      case 0xD5: // AAD                ASCII adjust AX before division (pg 42)
      {
         uint8_t base = mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);

         //if (base == 0) // base 0 is stupid
         //   break; // TODO: how to handle this? Do I care? No.

         d_regs.a.l = (d_regs.a.l + (d_regs.a.h * base)) & 0xFF;
         d_regs.a.h = 0;

         //ALU::setFlags(d_regs.a.x, flag);
         setFlags<uint16_t>(d_regs.a.x, flags);

         break;
      }
      // CBW = Convert byte to word:
      // [10011000]
      case 0x98: // CBW                AX <- sign-extended of AL (pg 79)
      {
         if (d_regs.a.l & 0x80)
            d_regs.a.h = 0xFF;
         else
            d_regs.a.h = 0;
         break;
      }
      // CWD = Convert word to double word:
      // [10011001]
      case 0x99: // CWD                DX:AX <- sign-extend of AX (pg 107)
      {
         if (d_regs.a.x & 0x8000)
            d_regs.d.x = 0xFFFF;
         else
            d_regs.d.x = 0;
         break;
      }

      //******************************************************************//
      //******                        Logic                         ******//
      //****** NOT SHL/SAL SHR SAR ROL ROR RCL RCR AND TEST OR XOR  ******//
      //******************************************************************//


   // ROL = Rotate left:                        // [110100 v w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)]
   // ROR = Rotate right:                       // [110100 v w] [mod 001 r/m] [(DISP-LO)] [(DISP-HI)]
   // RCL = Rotate through carry flag left:     // [110100 v w] [mod 010 r/m] [(DISP-LO)] [(DISP-HI)]
   // RCR = Rotate through carry right:         // [110100 v w] [mod 011 r/m] [(DISP-LO)] [(DISP-HI)]
   // SHL/SAL = Shif logical/arithmetic left:   // [110100 v w] [mod 100 r/m] [(DISP-LO)] [(DISP-HI)]
   // SHR = Shift logical right:                // [110100 v w] [mod 101 r/m] [(DISP-LO)] [(DISP-HI)]
   // SAR = Shift arithmetic right:             // [110100 v w] [mod 111 r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xD0: // Shift Group 2^2 Eb,1
                 // /0 ROL r/m8,1      Rotate 8 bits r/m8 left once (pg 424)
                 // /1 ROR r/m8,1      Rotate 8 bits r/m8 right once (pg 424)
                 // /2 RCL r/m8,1      Rotate 9 bits (CF,r/m8) left once (pg 424)
                 // /3 RCR r/m8,1      Rotate 9 bits (CF,r/m8) right once (pg 424)
                 // /4 SHL/SAL r/m8,1  Multiply r/m8 by 2, once (pg 446)
                 // /5 SHR r/m8,1      Unsigned divide r/m8 by 2, once (pg 446)
                 // /6 -unused-
                 // /7 SAR r/m8,1      Signed divide* r/m8 by 2, once (pg 446)
      {
         modregrm();
         if (reg != 6)
            writerm<uint8_t>(rm, ALU::rotate<uint8_t>()[reg](readrm<uint8_t>(rm), 1, flags));
         break;
      }
      case 0xD1: // Shift Group 2^2 Ev,1
                 // /0 ROL r/m16,1     Rotate 16 bits r/m16 left once (pg 424)
                 // /1 ROR r/m16,1     Rotate 16 bits r/m16 right once (pg 424)
                 // /2 RCL r/m16,1     Rotate 17 bits (CF,r/m16) left once (pg 424)
                 // /3 RCR r/m16,1     Rotate 17 bits (CF,r/m16) right once (pg 424)
                 // /4 SHL/SAL r/m16,1 Multiply r/m16 by 2, once (pg 446)
                 // /5 SHR r/m16,1     Unsigned divide r/m16 by 2, once (pg 446)
                 // /6 -unused-
                 // /7 SAR r/m16,1     Signed divide* r/m16 by 2, once (pg 446)
      {
         modregrm();
         if (reg != 6)
            writerm<uint16_t>(rm, ALU::rotate<uint16_t>()[reg](readrm<uint16_t>(rm), 1, flags));
         break;
      }
      case 0xD2: // Shift Group 2^2 Eb,CL
                 // /0 ROL r/m8,CL     Rotate 8 bits r/m8 left CL times (pg 424)
                 // /1 ROR r/m8,CL     Rotate 8 bits r/m8 right CL times (pg 424)
                 // /2 RCL r/m8,CL     Rotate 9 bits (CF,r/m8) left CL times (pg 424)
                 // /3 RCR r/m8,CL     Rotate 9 bits (CF,r/m8) right CL times (pg 424)
                 // /4 SHL/SAL r/m8,CL Multiply r/m8 by 2, CL times (pg 446)
                 // /5 SHR r/m8,CL     Unsigned divide r/m8 by 2, CL times (pg 446)
                 // /6 -unused-
                 // /7 SAR r/m8,CL     Signed divide* r/m8 by 2, CL times (pg 446)
      {
         modregrm();
         if (reg != 6)
            writerm<uint8_t>(rm, ALU::rotate<uint8_t>()[reg](readrm<uint8_t>(rm), d_regs.c.l, flags));
         break;
      }
      case 0xD3: // Shift Group 2^2 Ev,CL
                 // /0 ROL r/m16,CL    Rotate 16 bits r/m16 left CL times (pg 424)
                 // /1 ROR r/m16,CL    Rotate 16 bits r/m16 right CL times (pg 424)
                 // /2 RCL r/m16,CL    Rotate 17 bits (CF,r/m16) left CL times (pg 424)
                 // /3 RCR r/m16,CL    Rotate 17 bits (CF,r/m16) right CL times (pg 424)
                 // /4 SHL/SAL r/m16,CL Multiply r/m16 by 2, CL times (pg 446)
                 // /5 SHR r/m16,CL    Unsigned divide r/m16 by 2, CL times (pg 446)
                 // /6 -unused-
                 // /7 SAR r/m16,CL    Signed divide* r/m16 by 2, CL times (pg 446)
      {
         modregrm();
         if (reg != 6)
            writerm<uint16_t>(rm, ALU::rotate<uint16_t>()[reg](readrm<uint16_t>(rm), d_regs.c.l, flags));
         break;
      }

      // TEST = And function to flags no result:

      // Register/memory and register
      // [1000 010 w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x84: // TEST r/m8,r8       AND r8 with r/m8; set SF,ZF,PF according to result (pg 480)
      {
         modregrm();
         uint8_t op1 = fetchreg<uint8_t>(reg);
         uint8_t op2 = readrm<uint8_t>(rm);
         ALU::_and<uint8_t>(op1, op2, flags);
         break;
      }
      case 0x85: // TEST r/m16,r16     AND r16 with r/m16; set SF,ZF,PF according to result (pg 480)
      {
         modregrm();
         uint16_t op1 = fetchreg<uint16_t>(reg);
         uint16_t op2 = readrm<uint16_t>(rm);
         ALU::_and<uint16_t>(op1, op2, flags);
         break;
      }
      // Immediate data and accumulator
      // [1010100 w] [data] *[data if w=1] *this was not included in documentation, but I believe that it should be there.
      case 0xA8: // TEST AL,imm8       AND imm8 with AL; set SF,ZF,PF according to result (pg 480)
      {
         uint8_t op1 = d_regs.a.l;
         uint8_t op2 = mem->getmem<uint8_t>(segRegs.CS, IP); incrementIP(1);
         ALU::_and<uint8_t>(op1, op2, flags);
         break;
      }
      case 0xA9: // TEST AX,imm16      AND imm16 with AX; set SF,ZF,PF according to result (pg 480)
      {
         uint16_t op1 = d_regs.a.x;
         uint16_t op2 = mem->getmem<uint16_t>(segRegs.CS, IP); incrementIP(2);
         ALU::_and<uint16_t>(op1, op2, flags);
         break;
      }


      //*******************************************//
      //******      String Manipulation      ******//
      //****** REP MOVS CMPS SCAS LODS STDS  ******//
      //*******************************************//

      // MOVS = Move byte/word:
      // [1010010 w]
      case 0xA4: // MOVS m8,m8         move byte at address DS:(E)SI to address ES:(E)DI (pg 329)
      {
         mem->putmem<uint8_t>(segRegs.ES, pi_regs.DI, mem->getmem<uint8_t>(segRegs.DS, pi_regs.SI));

         if (flags.D == 0) {
            pi_regs.SI += 1;
            pi_regs.DI += 1;
         } else {
            pi_regs.SI -= 1;
            pi_regs.DI -= 1;
         }

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      case 0xA5: // MOVS m16,m16       move word at address DS:(E)SI to address ES:(E)DI (pg 329)
      {
         mem->putmem<uint16_t>(segRegs.ES, pi_regs.DI, mem->getmem<uint16_t>(segRegs.DS, pi_regs.SI));

         if (flags.D == 0) {
            pi_regs.SI += 2;
            pi_regs.DI += 2;
         } else {
            pi_regs.SI -= 2;
            pi_regs.DI -= 2;
         }

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      // CMPS = Compare byte/word:
      // [1010011 w]
      case 0xA6: // CMPS m8,m8         compares byte at address DS:(E)SI with byte at address ES:(E)DI and sets the status flags accordingly (pg 93)
      {
         // Do normal operation
         uint8_t op1 = mem->getmem<uint8_t>(segment, pi_regs.SI);
         uint8_t op2 = mem->getmem<uint8_t>(segRegs.ES, pi_regs.DI);
         ALU::sub<uint8_t>(op1, op2, flags);

         if (flags.D == 0) {
            pi_regs.SI += 1;
            pi_regs.DI += 1;
         } else {
            pi_regs.SI -= 1;
            pi_regs.DI -= 1;
         }

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      case 0xA7: // CMPS m16,m16       compares word at address DS:(E)SI with word at address ES:(E)DI and sets the status flags accordingly (pg 93)
      {
         // Do normal operation
         uint16_t op1 = mem->getmem<uint16_t>(segRegs.DS, pi_regs.SI);
         uint16_t op2 = mem->getmem<uint16_t>(segRegs.ES, pi_regs.DI);
         ALU::sub<uint16_t>(op1, op2, flags);

         if (flags.D == 0) {
            pi_regs.SI += 2;
            pi_regs.DI += 2;
         } else {
            pi_regs.SI -= 2;
            pi_regs.DI -= 2;
         }

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      // SCAS = Scan byte/word:
      // [1010111 w]
      case 0xAE: // SCAS m8            compare AL with byte at ES:(E)DI and set status flags (pg 452)
      {
         uint8_t op1 = d_regs.a.l;
         uint8_t op2 = mem->getmem<uint8_t>(segRegs.ES, pi_regs.DI);
         ALU::sub<uint8_t>(op1, op2, flags);

         if (flags.D == 0)
            pi_regs.DI += 1;
         else
            pi_regs.DI -= 1;

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      case 0xAF: // SCAS m16           compare AX with word at ES:(E)DI and set status flags (pg 452)
      {
         uint16_t op1 = d_regs.a.x;
         uint16_t op2 = mem->getmem<uint16_t>(segRegs.ES, pi_regs.DI);
         ALU::sub<uint16_t>(op1, op2, flags);

         if (flags.D == 0)
            pi_regs.DI += 2;
         else
            pi_regs.DI -= 2;

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      // LODS = Load byte/wd to AL/AX:
      // [1010110 w]
      case 0xAC: // LODS m8            load byte at address DS:(E)SI into AL (pg 305)
      {
         d_regs.a.l = mem->getmem<uint8_t>(segRegs.DS, pi_regs.SI);

         if (flags.D == 0)
            pi_regs.SI += 1;
         else
            pi_regs.SI -= 1;

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      case 0xAD: // LODS m16           load word at address DS:(E)SI into AX (pg 305)
      {
         d_regs.a.x = mem->getmem<uint16_t>(segRegs.DS, pi_regs.SI);

         if (flags.D == 0)
            pi_regs.SI += 2;
         else
            pi_regs.SI -= 2;

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      // STDS = Stor byte/wd from AL/A
      // [1010101 w]
      case 0xAA: // STOS m8            store AL at address ES:(E)DI (pg 473)
      {
         mem->putmem<uint8_t>(segRegs.ES, pi_regs.DI, d_regs.a.l);

         if (flags.D == 0)
            pi_regs.DI += 1;
         else
            pi_regs.DI -= 1;

         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }
      case 0xAB: // STOS m16           store AX at address ES:(E)DI (pg 473)
      {
         mem->putmem<uint16_t>(segRegs.ES, pi_regs.DI, d_regs.a.x);

         if (flags.D == 0)
            pi_regs.DI += 2;
         else
            pi_regs.DI -= 2;
         if (continueFindLoop(repeatType, flags.Z, d_regs.c.x))
            IP = saveIP;

         break;
      }

      //**************************************************//
      //******           Control Transfer           ******//
      //****** CALL JMP RET J__ LOOP_ INT INTO IRET ******//
      //**************************************************//

      // CALL = Call:

      // Direct within segment
      // [11101000] [IP-INC-LO] [IP-INC-HI]
      case 0xE8: // CALL rel16         Call near, relative, displacement relative to next instruction (pg 68)
      {
         disp16 = mem->getmem<uint16_t>(segRegs.CS, IP);
         incrementIP(2);
         push(IP);
         IP += disp16;
         break;
      }
      // Direct intersegment
      // [10011010] [IP-lo] [IP-hi] [CS-lo] [CS-hi]
      case 0x9A: // CALL ptr16:16      Call far, absolute, address given in operand (pg 68)
      {
         uint16_t newIP = mem->getmem<uint16_t>(segRegs.CS, IP);
         incrementIP(2);
         uint16_t newCS = mem->getmem<uint16_t>(segRegs.CS, IP);
         incrementIP(2);

         push(segRegs.CS);
         push(IP);

         segRegs.CS = newCS;
         IP = newIP;

         break;
      }

      // JMP = Unconditional Jump:

      // Direct within segment
      // [11101001] [IP-INC-LO] [IP-INC-HI]
      case 0xE9: // JMP rel16          jump near, relative, displacement relative to next instruction (pg 275)
      {
         disp16 = mem->getmem<uint16_t>(segRegs.CS, IP);
         incrementIP(2);
         IP += disp16;
         break;
      }
      // Direct within segment-short
      // [11101011] [IP-INC8]
      case 0xEB: // JMP rel8           jump short, relative, displacement relative to next instruction (pg 275)
      {
         disp16 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         IP += disp16;
         break;
      }
      // Indirect within segment
      // [11101010] [IP-lo] [IP-hi] [CS-lo] [CS-hi]
      case 0xEA: // JMP ptr16:16       jump far, absolute, address given in operand (pg 275)
      {
         uint16_t newIP = mem->getmem<uint16_t>(segRegs.CS, IP);
         incrementIP(2);
         uint16_t newCS = mem->getmem<uint16_t>(segRegs.CS, IP);
         //incrementIP(2); // pointless since this is an absolute jump

         segRegs.CS = newCS;
         IP = newIP;

         break;
      }

      // RET = Return from CALL:

      // Within segment
      // [11000011]
      case 0xC3: // RET                near return to calling procedure (pg 437)
      {
         IP = pop();
         break;
      }
      // Within seg adding immed to SP
      // [11000010] [data-lo] [data-hi]
      case 0xC2: // RET imm16          near return to calling procedure and pop imm16 bytes from stack (pg 437)
      {
         uint16_t adjustSP = mem->getmem<uint16_t>(segRegs.CS, IP);
         IP = pop(); // pop IP off stack
         segRegs.CS += adjustSP; // pop n bytes from stack
         break;
      }
      // Intersegment
      // [11001011]
      case 0xCB: // RET                far return to calling procedure (pg 437)
      {
         IP = pop();
         segRegs.CS = pop();
         break;
      }
      // Intersegment adding immediate to SP
      // [11001010] [data-lo] [data-hi]
      case 0xCA: // RET imm16          far return to calling procedure and pop imm16 bytes from stack (pg 437)
      {
         uint16_t adjustSP = mem->getmem<uint16_t>(segRegs.CS, IP);
         IP = pop();
         segRegs.CS = pop() + adjustSP;
         break;
      }

      // JO      = Jump on overflow:                   [01110000] [IP-INC8]
      // JNO     = Jump on not overflow:               [01110001] [IP-INC8]
      // JB/JNAE = Jump on below/not above or equal:   [01110010] [IP-INC8]
      // JNB/JAE = Jump on not below/above or equal:   [01110011] [IP-INC8]
      // JE/JZ   = Jump on equal/zero:                 [01110100] [IP-INC8]
      // JNE/JNZ = Jump on not equal/not zero:         [01110101] [IP-INC8]
      // JBE/JNA = Jump on below or equal/not above:   [01110110] [IP-INC8]
      // JNBE/JA = Jump on not below equal/above:      [01110111] [IP-INC8]
      // JS      = Jump on sign:                       [01111000] [IP-INC8]
      // JNS     = Jump on not sign-extend:            [01111001] [IP-INC8]
      // JP/JPE  = Jump on parity/parity even:         [01111010] [IP-INC8]
      // JNP/JPO = Jump on not par/par odd:            [01111011] [IP-INC8]
      // JL/JNGE = Jump on less/not greater or equal:  [01111100] [IP-INC8]
      // JNL/JGE = Jump on not less/greater or equal:  [01111101] [IP-INC8]
      // JLE/JNG = Jump on less or equal/not greater:  [01111110] [IP-INC8]
      // JNLE/JG = Jump on not less or equal/greater:
      case 0x70: // JO                 jump short if overflow (OF=1) (pg 271)
      case 0x71: // JNO                jump short if not overflow (OF=0) (pg 271)
      case 0x72: // JB/JNAE/JC         jump short if below/carry/not above or equal (CF=1) (pg 271)
      case 0x73: // JNB/JAE/JNC        jump short if above or equal/not below/not carry (CF=0) (pg 271)
      case 0x74: // JZ/JE              jump short if equal/zero (ZF=1) (pg 271)
      case 0x75: // JNE/JNZ            jump short if not eqaul/not zero (ZF=0) (pg 271)
      case 0x76: // JBE                jump short if below or equal/not above (CF=1 or ZF = 1) (pg 271)
      case 0x77: // JA/JNBE            jump short if above/not below or equal (CF=0 and ZF=0) (pg 271)
      case 0x78: // JS                 jump short if sign (SF=1) (pg 271)
      case 0x79: // JNS                jump short if not sign (SF=0) (pg 271)
      case 0x7A: // JP/JPE             jump short if parity/parity even (PF=1) (pg 271)
      case 0x7B: // JNP/JPO            jump short if not parity/parity odd (PF=0) (pg 271)
      case 0x7C: // JL/JNGE            jump short if less/not greater or equal (SF<>OF) (pg 271)
      case 0x7D: // JGE/JNL            jump short if greater or equal/not less (SF=OF) (pg 271)
      case 0x7E: // JLE/JNG            jump short if less or equal/not greater (ZF=1 or SF<>OF) (pg 271)
      case 0x7F: // JNLE/JG            jump short if greater/not less or equal (ZF=0 and SF=OF) (pg 271)
      {
         auto test = opcode & 0xF;

         disp16 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);

         if (ALU::condition(test, flags))
            IP += disp16;

         break;
      }


      // Loop = Loop CX times:                         [11100010]
      case 0xE2: // LOOP rel8          decrement count; jump short if count!=0 (pg 308)
      {
         disp16 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         d_regs.c.x--;
         if (d_regs.c.x != 0)
            IP += disp16;
         break;
      }
      // LOOPZ/LOOPE = Loop while zero/equal:          [11100001]
      case 0xE1: // LOOPE/LOOPZ rel8   decrement count; jump short if count!=0 and ZF=1 (pg 308)
      {
         disp16 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         d_regs.c.x--;
         if ((d_regs.c.x != 0) && (flags.Z == 1))
            IP += disp16;
         break;
      }
      // LOOPNZ/LOOPNE = Loop while not zero/equal:    [11100000]
      case 0xE0: // LOOPNE/LOOPNZ rel8 decrement count; jump short if count!=0 and ZF=0 (pg 308)
      {
         disp16 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         d_regs.c.x--;
         if ((d_regs.c.x != 0) && (flags.Z == 0))
            IP += disp16;
         break;
      }
      // JCXZ = Jump on CX zero:                       [11100011]
      case 0xE3: // JCXZ rel8          jump short if CX register is 0 (pg 271)
      {
         disp16 = (int16_t)(int8_t)mem->getmem<uint8_t>(segRegs.CS, IP);
         incrementIP(1);
         if (d_regs.c.x == 0)
            IP += disp16;
         break;
      }

      // INT = Interrupt:

      // Type specified
      // [11001101] [DATA-8]
      case 0xCD: // INT imm8           interrupt vector number specified by immediate byte (pg 248)
      {
         uint8_t vector = mem->getmem<uint8_t>(segRegs.CS, IP); incrementIP(1);
         interrupt(vector);
         break;
      }
      // Type 3
      // [11001100]
      case 0xCC: // INT 3              interrupt 3-trap to debugger (pg 248)
      {
         interrupt(3);
         break;
      }
      // INTO = Interrupt on overflow:
      // [11001110]
      case 0xCE: // INTO               interrupt 4-if overflow flag is 1 (pg 248)
      {
         if (flags.O == 1)
            interrupt(4);
         break;
      }
      // IRET=Interrupt return:
      // [11001111]
      case 0xCF: // IRET               interrupt return (pg 263)
      {
         IP = pop();
         segRegs.CS = pop();
         flags.set(pop());
      }


      //*******************************************************************//
      //******                   Processor Controll                  ******//
      //****** CLC CMC STC CLD STD CLI STI HLT WAIT ESC LOCK SEGMENT ******//
      //*******************************************************************//

   // CLC = Clear carry:                 [11111000]
      case 0xF8: // CLC                clear CF flag (pg 81)
      { flags.C = 0; break; }
      // CMC = Complement carry:            [11110101]
      case 0xF5: // CMC                Complement CF flag (pg 86)
      { flags.C = (flags.C ? 0 : 1); break; }
      // STC = Set carry:                   [11111001]
      case 0xF9: // STC                set CF flag (pg 469)
      { flags.C = 1; break; }
      // CLD = Clear direction:             [11111100]
      case 0xFC: // CLD                clear DF flag (pg 82)
      { flags.D = 0; break; }
      // STD = Set direction:               [11111101]
      case 0xFD: // STD                set DF flag (pg 470)
      { flags.D = 1; break; }
      // CLI = Clear interrupt:             [11111010]
      case 0xFA: // CLI                clear interrupt flag; interrupts disabled when interrupt flag cleared (pg 83)
      { flags.I = 0; break; }
      // STI = Set interrupt:               [11111011]
      case 0xFB: // STI                set interrupt flag; external, maskable interrupts enabled at the end of the next instruction (pg 471)
      { flags.I = 1; break; }
      // HLT = Halt:                        [11110100]
      case 0xF4: // HLT                Halt (pg 234)
      { halted = true; break; }
      // WAIT = Wait:                       [10011011]
      case 0x9B: // WAIT               check pending unmasked floating-point exceptions (pg 485)
                 // FCLEX              clear floating-point exception flags after checking for pending unmasked floating-point exceptions (pg 135)
         break; // Don't do anything because I haven't implemented an x87 fpu yet
      // ESC = Escape (to external device): [11011 xxx] [mod yyy r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xD8: // (Escape to Coprecessor Instruction Set)
      case 0xD9: // (Escape to Coprecessor Instruction Set)
      case 0xDA: // (Escape to Coprecessor Instruction Set)
      case 0xDB: // (Escape to Coprecessor Instruction Set)
      case 0xDC: // (Escape to Coprecessor Instruction Set)
      case 0xDD: // (Escape to Coprecessor Instruction Set)
      case 0xDE: // (Escape to Coprecessor Instruction Set)
      case 0xDF: // (Escape to Coprecessor Instruction Set)
         modregrm(); break; // escape to x87 FPU (unsupported)
      // LOCK = Bus lock prefix:            [11110000]
      case 0xF0: // LOCK               asserts LOCK# signal for duration of the accompanying instruction (pg 303)
         break; // This is a prefix opcode. This one can probably be ignored since this is not a hardware emulator.

      // Input from Port to String
      case 0x6C: // INS m8,DX          Input byte from I/O port specified in DX into memory location specified in ES:(E)DI (pg 245)
      {
         mem->putmem<uint8_t>(segRegs.ES, pi_regs.DI, io->read<uint8_t>(d_regs.d.x));
         if (flags.D == 0)
            pi_regs.DI += 1;
         else
            pi_regs.DI -= 1;
         if (continueLoop(repeatType, d_regs.c.x))
            IP = saveIP;
         break;
      }
      case 0x6D: // INS m16,DX         Input word from I/O port specified in DX into memory location specified in ES:(E)DI (pg 245)
      {
         mem->putmem<uint16_t>(segRegs.ES, pi_regs.DI, io->read<uint16_t>(d_regs.d.x));
         if (flags.D == 0)
            pi_regs.DI += 2;
         else
            pi_regs.DI -= 2;
         if (continueLoop(repeatType, d_regs.c.x))
            IP = saveIP;
         break;
      }
      // Output String to Port
      case 0x6E: // OUTS DX,m8         Output byte from memory location specified in DS:(E)SI to I/O port specified in DX (pg 347)
      {
         io->write<uint8_t>(d_regs.d.x, mem->getmem<uint8_t>(segRegs.ES, pi_regs.DI));
         if (flags.D == 0)
            pi_regs.SI += 1;
         else
            pi_regs.SI -= 1;
         if (continueLoop(repeatType, d_regs.c.x))
            IP = saveIP;
         break;
      }
      case 0x6F: // OUTS DX,m16        Output word from memory location specified in DS:(E)SI to I/O port specified in DX (pg 347)
      {
         io->write<uint16_t>(d_regs.d.x, mem->getmem<uint16_t>(segRegs.ES, pi_regs.DI));
         if (flags.D == 0)
            pi_regs.SI += 2;
         else
            pi_regs.SI -= 2;
         if (continueLoop(repeatType, d_regs.c.x))
            IP = saveIP;
         break;
      }

      // -unused- 0x60-6B, 0xC0, 0xC1, 0xC8, 0xC9, 0xD6, 0xF1
      default: // The 8086/8088 treated these like NOPs. Future CPUs trip invalid opcode exception
         break;
      }
   }
}
