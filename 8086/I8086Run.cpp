#include "I8086.h"

#include <algorithm>
#include <functional>


void I8086::run(unsigned int runtime)
{
   byte opcode;
   bool trap_toggle = false;
   enum RepeatType { None, Equal, NEqual } repeatType;

   auto REP = [&](std::function<void()> instruction) {
      switch (repeatType) {
      case None:
         instruction();
         return;

      case Equal:
         while (d_regs.c.x != 0) {
            // handle interrupts, if any
            instruction();
            d_regs.c.x--;
            if (alu.flags.Z == 0) break;
         };
         return;

      case NEqual:
         while (d_regs.c.x != 0) {
            // handle interrupts, if any
            instruction();
            d_regs.c.x--;
            if (alu.flags.Z == 1) break;
         };
         return;
      }
   };

   for (unsigned int count = 0; count < runtime;) {
      if (halted) // Halt state
         return;

      // 8086 Family Figure 2-29 : Interrupt Processing Sequence (upper left)
      if (false) // internal interrupt
         interrupt(0); // no idea which code it is
      else if (false) // non maskable interrupt
         interrupt(0); // again, no idea which
      else if (INTR && alu.flags.I) { // maskable interrupt and interrupt flag set
         // read type code
         interrupt(0); // I suppose this is where the type code goes?
      } else if (alu.flags.T) //
         interrupt(0); // which fucking vector is it?

      disassembleOp();

      repeatType = None;
      // The default segment register is SS for the effective addresses
      // containing a BP index, DS for other effective addresses (IA V2 p29).
      // An override prefix overrides this.
      // A call to modregrm() may override too.
      segment = segRegs.DS;

      // Process all prefix byte(s), if there are any

      for (bool prefix = true; prefix;) {
         opcode = imm<byte>(); // Grab the next instruction
         count++;

         switch (opcode) {
            // These are prefix bytes. Causes memory access to use specified segment instead of default segment designated for instruction operand
            // SEGMENT = Override prefix:         [001 reg 110]
         case 0x26: segment = segRegs.ES; break; // SEG =ES
         case 0x2E: segment = segRegs.CS; break; // SEG =CS
         case 0x36: segment = segRegs.SS; break; // SEG =SS
         case 0x3E: segment = segRegs.DS; break; // SEG =DS

            // LOCK = Bus lock prefix
            // [11110000]
         case 0xF0: // LOCK               asserts LOCK# signal for duration of the accompanying instruction (IA V2 p303)
            break; // This is a prefix opcode. This one can probably be ignored since this is not a hardware emulator.

            // REP = Repeat
            // [1111001 z]
         case 0xF2: // REPNE (IA V2 p434)
            repeatType = NEqual;
            break;
         case 0xF3: // REP REPE (IA V2 p434)
            repeatType = Equal;
            break;

         default: // Normal opcode
            prefix = false;
            break;
         }

      };

      // Process all non-prefix instructions
      // The order of the case statements follow Table 4-12: 8086 Instruction Encoding (8086 Family)
      switch (opcode) {

         ///******       Data Transfer      ******///
         ///****** MOV PUSH POP XCHG IN OUT ******///

      /// MOV = Move

      /// Register/memory to/from register
      /// [100010 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x88: fetchModRM(); rm<byte>() = reg<byte>(); break; // MOV r/m8,r8        move r8 to r/m8 (IA V2 p316)
      case 0x89: fetchModRM(); rm<word>() = reg<word>(); break; // MOV r/m16,r16      move r16 to r/m16 (IA V2 p316)
      case 0x8A: fetchModRM(); reg<byte>() = rm<byte>(); break; // MOV r8,r/m8        move r/m8 to r8 (IA V2 p316)
      case 0x8B: fetchModRM(); reg<word>() = rm<word>(); break; // MOV r16,r/m16      move r/m16 to r16 (IA V2 p316)

      /// Immediate to register/memory
      /// [1100011 w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]
      case 0xC6: // Move group
                 // /0 MOV r/m8,imm8   move imm8 to r/m8 (IA V2 p316) (8086 Family Table 4-13 page 4-35)
      {
         fetchModRM(); assert(ext == 0); // /1-7 -unused- (8086 Family Table 4-13 page 4-33)
         rm<byte>() = imm<byte>();
         break;
      }
      case 0xC7: // Move group
                 // /0 MOV r/m16,imm16 move imm16 to r/m16 (IA V2 p316) (8086 Family Table 4-13 page 4-35)
      {
         fetchModRM(); assert(ext == 0); // /1-7 -unused- (8086 Family Table 4-13 page 4-35)
         rm<word>() = imm<word>();
         break;
      }

      // Immediate to register
      // [1011 w reg] [data] [data if w=1]
      case 0xB0: d_regs.a.l = imm<byte>(); break; // MOV AL,imm8       move imm8 to r8 (IA V2 p316)
      case 0xB1: d_regs.c.l = imm<byte>(); break; // MOV CL,imm8       move imm8 to r8 (IA V2 p316)
      case 0xB2: d_regs.d.l = imm<byte>(); break; // MOV DL,imm8       move imm8 to r8 (IA V2 p316)
      case 0xB3: d_regs.b.l = imm<byte>(); break; // MOV BL,imm8       move imm8 to r8 (IA V2 p316)

      case 0xB4: d_regs.a.h = imm<byte>(); break; // MOV AH,imm8       move imm8 to r8 (IA V2 p316)
      case 0xB5: d_regs.c.h = imm<byte>(); break; // MOV CH,imm8       move imm8 to r8 (IA V2 p316)
      case 0xB6: d_regs.d.h = imm<byte>(); break; // MOV DH,imm8       move imm8 to r8 (IA V2 p316)
      case 0xB7: d_regs.b.h = imm<byte>(); break; // MOV BH,imm8       move imm8 to r8 (IA V2 p316)

      case 0xB8: d_regs.a.x = imm<word>(); break; // MOV AX,imm16       move imm16 to r16 (IA V2 p316)
      case 0xB9: d_regs.c.x = imm<word>(); break; // MOV CX,imm16       move imm16 to r16 (IA V2 p316)
      case 0xBA: d_regs.d.x = imm<word>(); break; // MOV DX,imm16       move imm16 to r16 (IA V2 p316)
      case 0xBB: d_regs.b.x = imm<word>(); break; // MOV BX,imm16       move imm16 to r16 (IA V2 p316)

      case 0xBC: pi_regs.SP = imm<word>(); break; // MOV SP,imm16       move imm16 to r16 (IA V2 p316)
      case 0xBD: pi_regs.BP = imm<word>(); break; // MOV BP,imm16       move imm16 to r16 (IA V2 p316)
      case 0xBE: pi_regs.SI = imm<word>(); break; // MOV SI,imm16       move imm16 to r16 (IA V2 p316)
      case 0xBF: pi_regs.DI = imm<word>(); break; // MOV DI,imm16       move imm16 to r16 (IA V2 p316)

      // Memory to accumulator
      // [1010000 w] [addr-lo] [addr-hi]
      case 0xA0: d_regs.a.l = mem<byte>(imm<word>()); break; // MOV AL,moffs8      move byte at (seg:offset) to AL (IA V2 p316)
      case 0xA1: d_regs.a.x = mem<word>(imm<word>()); break; // MOV AX,moffs16     move word at (seg:offset) to AX (IA V2 p316)

      // Accumulator to memory
      // [1010001 w] [addr-lo] [addr-hi]
      case 0xA2: mem<byte>(imm<word>()) = d_regs.a.l; break; // MOV moffs8,AL      move AL to (seg:offset) (IA V2 p316)
      case 0xA3: mem<word>(imm<word>()) = d_regs.a.x; break; // MOV moffs16,AX     move AX to (seg:offset) (IA V2 p316)

      // Register/memory to segment register
      // [10001110] [mod 0 SR r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8E: // MOV Sreg,r/m16     move r/m16 to segment register (IA V2 p316)
      {
         fetchModRM();
         switch (ext) {
         case 0: segRegs.ES = rm<word>(); break;
         case 1: segRegs.CS = rm<word>(); break;
         case 2: segRegs.SS = rm<word>(); break;
         case 3: segRegs.DS = rm<word>(); break;
         default: assert(false); // invalid segment register (8086 Family Table 4-13 page 4-31)
         }
         break;
      }
      // Segment register to register/memory
      // [10001100] [mod 0 SR r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8C: // MOV r/m16,Sreg     move segment register to r/m16 (IA V2 p316)
      {
         fetchModRM();
         switch (ext) {
         case 0: rm<word>() = segRegs.ES; break;
         case 1: rm<word>() = segRegs.CS; break;
         case 2: rm<word>() = segRegs.SS; break;
         case 3: rm<word>() = segRegs.DS; break;
         default: assert(false); // invalid segment register (8086 Family Table 4-13 page 4-31)
         }
         break;
      }


      // PUSH = Push

      // Register/memory
      // [11111111] [mod 110 r/m] [(DISP-LO)] [(DISP-HI)]
      // See case 0xff group 5

      // Register
      // [01010 reg]
      case 0x50: push(d_regs.a.x); break; // PUSH AX            push AX (IA V2 p415)
      case 0x51: push(d_regs.c.x); break; // PUSH CX            push CX (IA V2 p415)
      case 0x52: push(d_regs.d.x); break; // PUSH DX            push DX (IA V2 p415)
      case 0x53: push(d_regs.b.x); break; // PUSH BX            push BX (IA V2 p415)

      case 0x54: push(pi_regs.SP); break; // PUSH SP            push SP (IA V2 p415)
      case 0x55: push(pi_regs.BP); break; // PUSH BP            push BP (IA V2 p415)
      case 0x56: push(pi_regs.SI); break; // PUSH SI            push SI (IA V2 p415)
      case 0x57: push(pi_regs.DI); break; // PUSH DI            push DI (IA V2 p415)

      // Segment register
      // [000 reg 110]
      case 0x06: push(segRegs.ES); break; // PUSH ES            push ES (IA V2 p415)
      case 0x0E: push(segRegs.CS); break; // PUSH CS            push CS (IA V2 p415)
      case 0x16: push(segRegs.SS); break; // PUSH SS            push SS (IA V2 p415)
      case 0x1E: push(segRegs.DS); break; // PUSH DS            push DS (IA V2 p415)


      // POP = Pop

      // Register/memory
      // [10001111] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8F: // /0 POP m16            pop top of stack into m16; increment stack pointer (IA V2 p380)
      {
         fetchModRM(); assert(ext == 0); // /1-7 -unused- (8086 Family Table 4-13 page 4-(31-32))
         rm<word>() = pop();
         break;
      }

      // Register
      // [01011 reg]
      case 0x58: d_regs.a.x = pop(); break; // POP AX             pop top of stack into AX; increment stack pointer (IA V2 p380)
      case 0x59: d_regs.c.x = pop(); break; // POP CX             pop top of stack into CX; increment stack pointer (IA V2 p380)
      case 0x5A: d_regs.d.x = pop(); break; // POP DX             pop top of stack into DX; increment stack pointer (IA V2 p380)
      case 0x5B: d_regs.b.x = pop(); break; // POP BX             pop top of stack into BX; increment stack pointer (IA V2 p380)

      case 0x5C: pi_regs.SP = pop(); break; // POP SP             pop top of stack into SP; increment stack pointer (IA V2 p380)
      case 0x5D: pi_regs.BP = pop(); break; // POP BP             pop top of stack into BP; increment stack pointer (IA V2 p380)
      case 0x5E: pi_regs.SI = pop(); break; // POP SI             pop top of stack into SI; increment stack pointer (IA V2 p380)
      case 0x5F: pi_regs.DI = pop(); break; // POP DI             pop top of stack into DI; increment stack pointer (IA V2 p380)

      // Segment register
      // [000 reg 111]
      case 0x07: segRegs.ES = pop(); break; // POP ES             pop top of stack into ES; increment stack pointer (IA V2 p380)
      // case 0x0F: // 8086 does not use this (8086 Family, Table 4-13). Later CPUs use this as an escape character
      case 0x17: segRegs.SS = pop(); break; // POP SS             pop top of stack into SS; increment stack pointer (IA V2 p380)
      case 0x1F: segRegs.DS = pop(); break; // POP DS             pop top of stack into DS; increment stack pointer (IA V2 p380)


      // XCHG = Exchange

      // Register/memory with register
      // [1000011 w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x86: fetchModRM(); std::swap(reg<byte>(), rm<byte>()); break; // XCHG r/m8,r8       exchange r8 (byte register) with byte from r/m8 (IA V2 p492)
      case 0x87: fetchModRM(); std::swap(reg<word>(), rm<word>()); break; // XCHG r/m16,r16     exchange r16 with word from r/m16 (IA V2 p492)

      // Register with accumulator
      // [10010 reg]
      case 0x90:/*std::swap(d_regs.a.x,d_regs.a.x);*/break;// NOP                no operation (IA V2 p340)
      case 0x91: std::swap(d_regs.a.x, d_regs.c.x); break; // XCHG CX            exchange CX with AX (IA V2 p492)
      case 0x92: std::swap(d_regs.a.x, d_regs.d.x); break; // XCHG DX            exchange DX with AX (IA V2 p492)
      case 0x93: std::swap(d_regs.a.x, d_regs.b.x); break; // XCHG BX            exchange BX with AX (IA V2 p492)
      case 0x94: std::swap(d_regs.a.x, pi_regs.SP); break; // XCHG SP            exchange SP with AX (IA V2 p492)
      case 0x95: std::swap(d_regs.a.x, pi_regs.BP); break; // XCHG BP            exchange BP with AX (IA V2 p492)
      case 0x96: std::swap(d_regs.a.x, pi_regs.SI); break; // XCHG SI            exchange SI with AX (IA V2 p492)
      case 0x97: std::swap(d_regs.a.x, pi_regs.DI); break; // XCHG DI            exchange DI with AX (IA V2 p492)


      // IN = Input from

      // Fixed port
      // [1110010 w] [DATA-8]
      case 0xE4: d_regs.a.l = io->read<byte>(imm<byte>()); break; // IN AL,imm8         input byte from imm8 I/O port address into AL (IA V2 p241)
      case 0xE5: d_regs.a.x = io->read<word>(imm<byte>()); break; // IN AX,imm8         input *word from imm8 I/O port address into AX (IA V2 p241) *change to match pattern of OUT (IA V2 p345)

      // Variable port
      // [1110110 w]
      case 0xEC: d_regs.a.l = io->read<byte>(d_regs.d.x); break; // IN AL,DX           input byte from I/O port in DX into AL (IA V2 p241)
      case 0xED: d_regs.a.x = io->read<word>(d_regs.d.x); break; // IN AX,DX           input word from I/O port in DX into AX (IA V2 p241)


      // OUT = Output to

      // Fixed port
      // [1110011 w] [DATA-8]
      case 0xE6: io->write<byte>(imm<byte>(), d_regs.a.l); break; // OUT imm8,AL        output byte in AL to I/O port address imm8 (IA V2 p345)
      case 0xE7: io->write<word>(imm<byte>(), d_regs.a.x); break; // OUT imm8,AX        output word in AX to I/O port address imm8 (IA V2 p345)

      // Variable port
      // [1110111 w]
      case 0xEE: io->write<byte>(d_regs.d.x, d_regs.a.l); break; // OUT DX,AL          output byte in AL to I/O port address in DX (IA V2 p345)
      case 0xEF: io->write<word>(d_regs.d.x, d_regs.a.x); break; // OUT DX,AX          output word in AX to I/O port address in DX (IA V2 p345)


      // Miscellaneous Data Transfer

      // XLAT = Translate byte to AL
      // [11010111]
      case 0xD7: // XLAT m8            set AL to memory byte DS:[(E)BX+unsigned AL] (IA V2 p494)
      {
         d_regs.a.l = mem<byte>(d_regs.b.x + d_regs.a.l);
         break;
      }

      // LEA = Load EA to register
      // [10001101] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x8D: // LEA r16,m          store effective address for m in register r16 (IA V2 p289)
                 // r16 is from the REG field and m is from the r/m field, right?
      {
         fetchModRM();
         reg<word>() = ea();
         break;
      }

      // LES = Load pointer to ES
      // [11000100] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xC4: fetchModRM(); loadFarPointer(segRegs.ES, reg<word>(), ea()); break; // LES r16,m16:16     load ES:r16 with far pointer from memory (IA V2 p286)

      // LDS = Load pointer to DS
      // [11000101] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xC5: fetchModRM(); loadFarPointer(segRegs.DS, reg<word>(), ea()); break; // LDS r16,m16:16     load DS:r16 with far pointer from memory (IA V2 p286)

      // PUSHF = Push flags
      // [10011100]
      case 0x9C: push(alu.flags.get<word>()); break; // PUSHF              push lower 16 bits of EFLAGS (IA V2 p420)

      // POPF = Pop flags
      // [10011101]
      case 0x9D: alu.flags.set<word>(pop()); break; // POPF               pop top of stack into lower 16 bits of EFLAGS (IA V2 p386)

      // SAHF = Store AH into flags
      // [10011110]
      case 0x9E: alu.flags.set<byte>(d_regs.a.h); break; // SAHF               loads SF,ZF,AF,PF, and CF from AH into EFLAGS register (IA V2 p445)

      // LAHF = Load AH with flags
      // [10011111]
      case 0x9F: d_regs.a.h = alu.flags.get<byte>(); break; // LAHF               load: AH=EFLAGS(SF:ZF:0:AF:0:PF:1:CF) (IA V2 p282)



         //******************************************************************************************//
         //******                               Arithmetic/Logic                               ******//
         //****** ADD ADC INC AA DAA SUB SBB DEC CMP AAS DAS MUL IMUL AAM DIV IDIV AAD CBW CWD ******//
         //******************************************************************************************//



      // Reg/memory with register to either
      // [00 000 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] ADD = Add
      // [00 001 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] OR  = Or
      // [00 010 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] ADC = Add with carry
      // [00 011 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] SBB = Subtract with borrow
      // [00 100 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] AND = And
      // [00 101 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] SUB = Subtract
      // [00 110 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] XOR = Exclusive or
      // [00 111 0 d w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)] CMP = Compare
      case 0x00: fetchModRM(); [&](byte& dest) {dest = alu.add <byte>(dest, reg<byte>()); }(rm<byte>()); break; // ADD r/m8,r8        add r8 to r/m8                       (IA V2 p47)
      case 0x01: fetchModRM(); [&](word& dest) {dest = alu.add <word>(dest, reg<word>()); }(rm<word>()); break; // ADD r/m16,r16      add r16 to r/m16                    (IA V2 p47)
      case 0x02: fetchModRM(); [&](byte& dest) {dest = alu.add <byte>(dest, rm<byte>()); }(reg<byte>()); break; // ADD r8,r/m8        add r/m8 to r8                       (IA V2 p47)
      case 0x03: fetchModRM(); [&](word& dest) {dest = alu.add <word>(dest, rm<word>()); }(reg<word>()); break; // ADD r16,r/m16      add r/m16 to r16                    (IA V2 p47)

      case 0x08: fetchModRM(); [&](byte& dest) {dest = alu._or <byte>(dest, reg<byte>()); }(rm<byte>()); break; // OR  r/m8,r8        r/m8 OR r8                           (IA V2 p343)
      case 0x09: fetchModRM(); [&](word& dest) {dest = alu._or <word>(dest, reg<word>()); }(rm<word>()); break; // OR  r/m16,r16      r/m16 OR r16                        (IA V2 p343)
      case 0x0A: fetchModRM(); [&](byte& dest) {dest = alu._or <byte>(dest, rm<byte>()); }(reg<byte>()); break; // OR  r8,r/m8        r8 OR r/m8                           (IA V2 p343)
      case 0x0B: fetchModRM(); [&](word& dest) {dest = alu._or <word>(dest, rm<word>()); }(reg<word>()); break; // OR  r16,r/m16      r16 OR r/m16                        (IA V2 p343)

      case 0x10: fetchModRM(); [&](byte& dest) {dest = alu.adc <byte>(dest, reg<byte>()); }(rm<byte>()); break; // ADC r/m8,r8        add with carry byte register to r/m8 (IA V2 p45)
      case 0x11: fetchModRM(); [&](word& dest) {dest = alu.adc <word>(dest, reg<word>()); }(rm<word>()); break; // ADC r/m16,r16      add with carry r16 to r/m16         (IA V2 p45)
      case 0x12: fetchModRM(); [&](byte& dest) {dest = alu.adc <byte>(dest, rm<byte>()); }(reg<byte>()); break; // ADC r8,r/m8        add with carry r/m8 to byte register (IA V2 p45)
      case 0x13: fetchModRM(); [&](word& dest) {dest = alu.adc <word>(dest, rm<word>()); }(reg<word>()); break; // ADC r16,r/m16      add with carry r/m16 to r16         (IA V2 p45)

      case 0x18: fetchModRM(); [&](byte& dest) {dest = alu.sbb <byte>(dest, reg<byte>()); }(rm<byte>()); break; // SBB r/m8,r8        subtract with borrow r8 from r/m8    (IA V2 p450)
      case 0x19: fetchModRM(); [&](word& dest) {dest = alu.sbb <word>(dest, reg<word>()); }(rm<word>()); break; // SBB r/m16,r16      subtract with borrow r16 from r/m16 (IA V2 p450)
      case 0x1A: fetchModRM(); [&](byte& dest) {dest = alu.sbb <byte>(dest, rm<byte>()); }(reg<byte>()); break; // SBB r8,r/m8        subtract with borrow r/m8 from r8    (IA V2 p450)
      case 0x1B: fetchModRM(); [&](word& dest) {dest = alu.sbb <word>(dest, rm<word>()); }(reg<word>()); break; // SBB r16,r/m16      subtract with borrow r/m16 from r16 (IA V2 p450)

      case 0x20: fetchModRM(); [&](byte& dest) {dest = alu._and<byte>(dest, reg<byte>()); }(rm<byte>()); break; // AND r/m8,r8        r/m8 AND r8                          (IA V2 p49)
      case 0x21: fetchModRM(); [&](word& dest) {dest = alu._and<word>(dest, reg<word>()); }(rm<word>()); break; // AND r/m16,r16      r/m16 AND r16                       (IA V2 p49)
      case 0x22: fetchModRM(); [&](byte& dest) {dest = alu._and<byte>(dest, rm<byte>()); }(reg<byte>()); break; // AND r8,r/m8        r8 AND r/m8                          (IA V2 p49)
      case 0x23: fetchModRM(); [&](word& dest) {dest = alu._and<word>(dest, rm<word>()); }(reg<word>()); break; // AND r16,r/m16      r16 AND r/m16                       (IA V2 p49)

      case 0x28: fetchModRM(); [&](byte& dest) {dest = alu.sub <byte>(dest, reg<byte>()); }(rm<byte>()); break; // SUB r/m8,r8        subtract r8 from r/m8                (IA V2 p478)
      case 0x29: fetchModRM(); [&](word& dest) {dest = alu.sub <word>(dest, reg<word>()); }(rm<word>()); break; // SUB r/m16,r16      subtract r16 from r/m16             (IA V2 p478)
      case 0x2A: fetchModRM(); [&](byte& dest) {dest = alu.sub <byte>(dest, rm<byte>()); }(reg<byte>()); break; // SUB r8,r/m8        subtract r/m8 from r8                (IA V2 p478)
      case 0x2B: fetchModRM(); [&](word& dest) {dest = alu.sub <word>(dest, rm<word>()); }(reg<word>()); break; // SUB r16,r/m16      subtract r/m16 from r16             (IA V2 p478)

      case 0x30: fetchModRM(); [&](byte& dest) {dest = alu._xor<byte>(dest, reg<byte>()); }(rm<byte>()); break; // XOR r/m8,r8        r/m8 XOR r8                          (IA V2 p496)
      case 0x31: fetchModRM(); [&](word& dest) {dest = alu._xor<word>(dest, reg<word>()); }(rm<word>()); break; // XOR r/m16,r16      r/m16 XOR r16                       (IA V2 p496)
      case 0x32: fetchModRM(); [&](byte& dest) {dest = alu._xor<byte>(dest, rm<byte>()); }(reg<byte>()); break; // XOR r8,r/m8        r8 XOR r/m8                          (IA V2 p496)
      case 0x33: fetchModRM(); [&](word& dest) {dest = alu._xor<word>(dest, rm<word>()); }(reg<word>()); break; // XOR r16,r/m16      r16 XOR r/m16                       (IA V2 p496)

      case 0x38: fetchModRM(); alu.sub<byte>(rm<byte>(), reg<byte>()); break; // CMP r/m8,r8        compare r8 with r/m8                 (IA V2 p91)
      case 0x39: fetchModRM(); alu.sub<word>(rm<word>(), reg<word>()); break; // CMP r/m16,r16      compare r16 with r/m16               (IA V2 p91)
      case 0x3A: fetchModRM(); alu.sub<byte>(reg<byte>(), rm<byte>()); break; // CMP r8,r/m8        compare r/m8 with r8                 (IA V2 p91)
      case 0x3B: fetchModRM(); alu.sub<word>(reg<word>(), rm<word>()); break; // CMP r16,r/m16      compare r/m16 with r16               (IA V2 p91)

      // Immediate to accumulator
      // [00 000 1 0 w] [data] [data if w=1] ADD = Add
      // [00 001 1 0 w] [data] [data if w=1] OR  = Or
      // [00 010 1 0 w] [data] [data if w=1] ADC = Add with carry
      // [00 011 1 0 w] [data] [data if w=1] SBB = Subtract with borrow
      // [00 100 1 0 w] [data] [data if w=1] AND = And
      // [00 101 1 0 w] [data] [data if w=1] SUB = Subtract
      // [00 110 1 0 w] [data] [data if w=1] XOR = Exclusive or
      // [00 111 1 0 w] [data] [data if w=1] CMP = Compare
      case 0x04: fetchModRM(); d_regs.a.l = alu.add<byte>(d_regs.a.l, imm<byte>());  break; // ADD AL,imm8        add imm8 to AL                     (IA V2 p47)
      case 0x05: fetchModRM(); d_regs.a.x = alu.add<word>(d_regs.a.x, imm<word>());  break; // ADD AX,imm16       add imm16 to AX                    (IA V2 p47)

      case 0x0C: fetchModRM(); d_regs.a.l = alu._or<byte>(d_regs.a.l, imm<byte>());  break; // OR  AL,imm8        AL OR imm8                         (IA V2 p343)
      case 0x0D: fetchModRM(); d_regs.a.x = alu._or<word>(d_regs.a.x, imm<word>());  break; // OR  AX,imm16       AX OR imm16                        (IA V2 p343)

      case 0x14: fetchModRM(); d_regs.a.l = alu.adc<byte>(d_regs.a.l, imm<byte>());  break; // ADC AL,imm8        add with carry imm8 to AL          (IA V2 p45)
      case 0x15: fetchModRM(); d_regs.a.x = alu.adc<word>(d_regs.a.x, imm<word>());  break; // ADC AX,imm16       add with carry imm16 to AX         (IA V2 p45)

      case 0x1C: fetchModRM(); d_regs.a.l = alu.sbb<byte>(d_regs.a.l, imm<byte>());  break; // SBB AL,imm8        subtract with borrow imm8 from AL  (IA V2 p450)
      case 0x1D: fetchModRM(); d_regs.a.x = alu.sbb<word>(d_regs.a.x, imm<word>());  break; // SBB AX,imm16       subtract with borrow imm16 from AX (IA V2 p450)

      case 0x24: fetchModRM(); d_regs.a.l = alu._and<byte>(d_regs.a.l, imm<byte>()); break; // AND AL,imm8        AL AND imm8                        (IA V2 p49)
      case 0x25: fetchModRM(); d_regs.a.x = alu._and<word>(d_regs.a.x, imm<word>()); break; // AND AX,imm16       AX AND imm16                       (IA V2 p49)

      case 0x2C: fetchModRM(); d_regs.a.l = alu.sub<byte>(d_regs.a.l, imm<byte>());  break; // SUB AL,imm8        subtract imm8 from AL              (IA V2 p478)
      case 0x2D: fetchModRM(); d_regs.a.x = alu.sub<word>(d_regs.a.x, imm<word>());  break; // SUB AX,imm16       subtract imm16 from AX             (IA V2 p478)

      case 0x34: fetchModRM(); d_regs.a.l = alu._xor<byte>(d_regs.a.l, imm<byte>()); break; // XOR AL,imm8        AL XOR imm8                        (IA V2 p496)
      case 0x35: fetchModRM(); d_regs.a.x = alu._xor<word>(d_regs.a.x, imm<word>()); break; // XOR AX,imm16       AX XOR imm16                       (IA V2 p496)

      case 0x3C: fetchModRM(); alu.sub<byte>(d_regs.a.l, imm<byte>()); break; // CMP AL,imm8        compare imm8 with AL              (IA V2 p91)
      case 0x3D: fetchModRM(); alu.sub<word>(d_regs.a.x, imm<word>()); break; // CMP AX,imm16       compare imm16 with AX              (IA V2 p91)

      // Immediate to register/memory
      // [100000 s w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] ADD = Add
      // [100000 0 w] [mod 001 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]    OR  = Or
      // [100000 s w] [mod 010 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] ADC = Add with carry
      // [100000 s w] [mod 011 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] SBB = Subtract with borrow
      // [100000 0 w] [mod 100 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]    AND = And
      // [100000 s w] [mod 101 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] SUB = Subtract
      // [100000 0 w] [mod 110 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]    XOR = Exclusive or
      // [100000 s w] [mod 111 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if s:w=01] CMP = Compare
      case 0x80: // Immediate Group r/m8,imm8
      {
         fetchModRM();
         [&](byte& dest, byte src) {
            switch (ext) {
            case 0: dest = alu.add<byte>(dest, src);  break; // /0 ADD r/m8,imm8   add imm8 to r/m8                    (IA V2 p47)
            case 1: dest = alu._or<byte>(dest, src);  break; // /1 OR  r/m8,imm8   r/m8 OR imm8                        (IA V2 p343)
            case 2: dest = alu.adc<byte>(dest, src);  break; // /2 ADC r/m8,imm8   add with carry imm8 to r/m8         (IA V2 p45)
            case 3: dest = alu.sbb<byte>(dest, src);  break; // /3 SBB r/m8,imm8   subtract with borrow imm8 from r/m8 (IA V2 p450)
            case 4: dest = alu._and<byte>(dest, src); break; // /4 AND r/m8,imm8   r/m8 AND imm8                       (IA V2 p49)
            case 5: dest = alu.sub<byte>(dest, src);  break; // /5 SUB r/m8,imm8   subtract imm8 from r/m8             (IA V2 p478)
            case 6: dest = alu._xor<byte>(dest, src); break; // /6 XOR r/m8,imm8   r/m8 XOR imm8                       (IA V2 p496)
            case 7:        alu.sub<byte>(dest, src);  break; // /7 CMP r/m8,imm8   compare imm8 with r/m8              (IA V2 p91)
            }
         }(rm<byte>(), imm<byte>());
         break;
      }
      // This particular opcode is not at all referenced in IA V2. The only assurance that it is in fact used
      // is because of Table 4-13 found in 8086 Family page 4-31.
      // This does seem to be exactly the same as 0x80 except that only arithmetic operations are used.
      case 0x82: // Immediate Group r/m8,imm8 (not referenced in modern documentation, but follows the pattern)
      {
         fetchModRM();
         [&](byte& dest, byte src) {
            switch (ext) {
            case 0: dest = alu.add<byte>(dest, src); break; // /0 ADD r/m8,imm8 (8086 Table 4-13 page 4-31)
            case 1: assert(false);                          // /1    (not used) (8086 Table 4-13 page 4-31)
            case 2: dest = alu.adc<byte>(dest, src); break; // /2 ADC r/m8,imm8 (8086 Table 4-13 page 4-31)
            case 3: dest = alu.sbb<byte>(dest, src); break; // /3 SBB r/m8,imm8 (8086 Table 4-13 page 4-31)
            case 4: assert(false);                          // /4    (not used) (8086 Table 4-13 page 4-31)
            case 5: dest = alu.sub<byte>(dest, src); break; // /5 SUB r/m8,imm8 (8086 Table 4-13 page 4-31)
            case 6: assert(false);                          // /6    (not used) (8086 Table 4-13 page 4-31)
            case 7:        alu.sub<byte>(dest, src); break; // /7 CMP r/m8,imm8 (8086 Table 4-13 page 4-31)
            }
         }(rm<byte>(), imm<byte>());
         break;
      }
      case 0x81: // Immediate Group r/m16,imm16
      {
         fetchModRM();
         [&](word& dest, word src) {
            switch (ext) {
            case 0: dest = alu.add<word>(dest, src);  break; // /0 ADD r/m16,imm16 add imm16 to r/m16                    (IA V2 p47)
            case 1: dest = alu._or<word>(dest, src);  break; // /1 OR  r/m16,imm16 r/m16 OR imm16                        (IA V2 p343)
            case 2: dest = alu.adc<word>(dest, src);  break; // /2 ADC r/m16,imm16 add with carry imm16 to r/m16         (IA V2 p45)
            case 3: dest = alu.sbb<word>(dest, src);  break; // /3 SBB r/m16,imm16 subtract with borrow imm16 from r/m16 (IA V2 p450)
            case 4: dest = alu._and<word>(dest, src); break; // /4 AND r/m16,imm16 r/m16 AND imm16                       (IA V2 p49)
            case 5: dest = alu.sub<word>(dest, src);  break; // /5 SUB r/m16,imm16 subtract imm16 from r/m16             (IA V2 p478)
            case 6: dest = alu._xor<word>(dest, src); break; // /6 XOR r/m16,imm16 r/m16 XOR imm16                       (IA V2 p496)
            case 7:        alu.sub<word>(dest, src);  break; // /7 CMP r/m16,imm16 compare imm16 with r/m16              (IA V2 p91)
            }
         }(rm<word>(), imm<word>());
         break;
      }
      case 0x83: // Immediate Group r/m16,imm8
                 // [0x83] [mod /# r/m] [(DISP-LO)] [(DISP-HI)] [DATA-SX]
      {
         fetchModRM();
         [&](word& dest, word src) {
            switch (ext) {
            case 0: dest = alu.add<word>(dest, src); break; // /0 ADD r/m16,imm8  add sign-extended imm8 to r/m16                    (IA V2 p47)
            case 1: assert(false);                          // /1 (not used)                                                         (8086 Family Table 4-13 page 4-31)
            case 2: dest = alu.adc<word>(dest, src); break; // /2 ADC r/m16,imm8  add with CF sign-extended imm8 to r/m16            (IA V2 p45)
            case 3: dest = alu.sbb<word>(dest, src); break; // /3 SBB r/m16,imm8  subtract with borrow sign-extended imm8 from r/m16 (IA V2 p450)
            case 4: assert(false);                          // /4 (not used)                                                         (8086 Family Table 4-13 page 4-31)
            case 5: dest = alu.sub<word>(dest, src); break; // /5 SUB r/m16,imm8  subtract sign-extended imm8 from r/m16             (IA V2 p478)
            case 6: assert(false);                          // /6 (not used)                                                         (8086 Family Table 4-13 page 4-31)
            case 7:        alu.sub<word>(dest, src); break; // /7 CMP r/m16,imm8  compare imm8 with r/m16                            (IA V2 p91)
            }
         }(rm<word>(), (int16_t)(int8_t)imm<byte>());
         break;
      }


      // INC = Increment


      // [11111111] [mod xxx r/m] [(DISP-LO)] [(DISP-HI)]
      case 0xFF: // Group 5
      {
         fetchModRM();
         switch (ext) {
         case 0: [&](word& dest) {dest = alu.INC<word>(dest); }(rm<word>()); break; // /0 INC  r/m16      increment r/m word by 1                               (IA V2 p243) (8086 Family Table 4-13 page 4-35)
         case 1: [&](word& dest) {dest = alu.DEC<word>(dest); }(rm<word>()); break; // /1 DEC  r/m16      decrement r/m16    by 1                               (IA V2 p112) (8086 Family Table 4-13 page 4-35)
         case 2: callnear(rm<word>());                                       break; // /2 CALL r/m16      call near, absolute indirect, address given in r/m16  (IA V2 p68)  (8086 Family Table 4-13 page 4-35)
         case 3: callfar(ea());                                              break; // /3 CALL m16:16     call far,  absolute indirect, address given in m16:16 (IA V2 p68)  (8086 Family Table 4-13 page 4-35)
         case 4: jumpNear(rm<word>());                                       break; // /4 JMP  r/m16      jump near, absolute,          address given in r/m16  (IA V2 p275) (8086 Family Table 4-13 page 4-35)
         case 5: jumpFar(ea());                                              break; // /5 JMP  ptr16:16   jump far,  absolute,          address given in m16:16 (IA V2 p275) (8086 Family Table 4-13 page 4-35)
         case 6: push(rm<word>());                                           break; // /6 PUSH r/m16      push r/m16                                            (IA V2 p415) (8086 Family Table 4-13 page 4-35)
         case 7: assert(false);                                                     // /7 (not used)                                                                         (8086 Family Table 4-13 page 4-35)
         }
         break;
      }

      // Register/memory
      // [1111111 w] [mod 001 r/m] [(DISP-LO)] [(DISP-HI)]
      /* see 'case 0xFE' and 'case 0xFF' */
      case 0xFE: // INC/DEC Group
      {
         fetchModRM();
         [&](byte& dest) {
            switch (ext) {
            case 0: dest = alu.INC<byte>(dest); break; // /0 INC r/m8        increment r/m byte by 1 (IA V2 p243)
            case 1: dest = alu.DEC<byte>(dest); break; // /1 DEC r/m8        decrement r/m8 by 1     (IA V2 p112)
            default: assert(false);                    // /2-7 (not used)       (8086 Table 4-13 page 4-35)
            }
         }(rm<byte>());
         break;
      }
      // Register
      // [01000 reg]
      case 0x40: d_regs.a.x = alu.INC<word>(d_regs.a.x); break; // INC AX             increment AX by 1 (IA V2 p243)
      case 0x41: d_regs.c.x = alu.INC<word>(d_regs.c.x); break; // INC CX             increment CX by 1 (IA V2 p243)
      case 0x42: d_regs.d.x = alu.INC<word>(d_regs.d.x); break; // INC DX             increment DX by 1 (IA V2 p243)
      case 0x43: d_regs.b.x = alu.INC<word>(d_regs.b.x); break; // INC BX             increment BX by 1 (IA V2 p243)
      case 0x44: pi_regs.SP = alu.INC<word>(pi_regs.SP); break; // INC SP             increment SP by 1 (IA V2 p243)
      case 0x45: pi_regs.BP = alu.INC<word>(pi_regs.BP); break; // INC BP             increment BP by 1 (IA V2 p243)
      case 0x46: pi_regs.SI = alu.INC<word>(pi_regs.SI); break; // INC SI             increment SI by 1 (IA V2 p243)
      case 0x47: pi_regs.DI = alu.INC<word>(pi_regs.DI); break; // INC DI             increment DI by 1 (IA V2 p243)

      // AAA = ASCII adjust for add
      // [00110111]
      case 0x37: alu.AAA(d_regs.a.l, d_regs.a.h); break; // AAA                ASCII adjust AL after addition (IA V2 p41)

      // DAA = Decimal adjust for add
      // [00100111]
      case 0x27: d_regs.a.l = alu.DAA(d_regs.a.l); break; // DAA                Decimal adjust AL after addition (IA V2 p109)


      // DEC = Decrement

      // Register
      // [01001 reg]
      case 0x48: d_regs.a.x = alu.DEC<word>(d_regs.a.x); break; // DEC AX             decrement AX by 1 (IA V2 p112)
      case 0x49: d_regs.c.x = alu.DEC<word>(d_regs.c.x); break; // DEC CX             decrement CX by 1 (IA V2 p112)
      case 0x4A: d_regs.d.x = alu.DEC<word>(d_regs.d.x); break; // DEC DX             decrement DX by 1 (IA V2 p112)
      case 0x4B: d_regs.b.x = alu.DEC<word>(d_regs.b.x); break; // DEC BX             decrement BX by 1 (IA V2 p112)
      case 0x4C: pi_regs.SP = alu.DEC<word>(pi_regs.SP); break; // DEC SP             decrement SP by 1 (IA V2 p112)
      case 0x4D: pi_regs.BP = alu.DEC<word>(pi_regs.BP); break; // DEC BP             decrement BP by 1 (IA V2 p112)
      case 0x4E: pi_regs.SI = alu.DEC<word>(pi_regs.SI); break; // DEC SI             decrement SI by 1 (IA V2 p112)
      case 0x4F: pi_regs.DI = alu.DEC<word>(pi_regs.DI); break; // DEC DI             decrement DI by 1 (IA V2 p112)

      // [1111011 w] [mod 000 r/m] [(DISP-LO)] [(DISP-HI)] [data] [data if w=1]
      // [1111011 w] [mod 010 r/m] [(DISP-LO)] [(DISP-HI)] NOT = Invert
      // [1111011 w] [mod 011 r/m] [(DISP-LO)] [(DISP-HI)] NEG = Change sign
      // [1111011 w] [mod 100 r/m] [(DISP-LO)] [(DISP-HI)] MUL = Multiply (unsigned)
      // [1111011 w] [mod 101 r/m] [(DISP-LO)] [(DISP-HI)] IMUL = Integer multiply (signed)
      // [1111011 w] [mod 110 r/m] [(DISP-LO)] [(DISP-HI)] DIV = Divide (unsigned)
      // [1111011 w] [mod 111 r/m] [(DISP-LO)] [(DISP-HI)] IDIV = Integer divide (signed)
      case 0xF6: // Unary Group imm8
      {
         fetchModRM();
         [&](byte& val) {
            switch (ext) {
            case 0: alu._and(val, imm<byte>());                  break; // /0 TEST r/m8,imm8     AND imm8 with r/m8; set SF,ZF,PF according to result                                                               (IA V2 p480)
            case 1: assert(false);                                      // /1 (not used)                                                                                                                            (8086 Table 4-13 page 4-35)
            case 2: alu.NOT<byte>(val);                          break; // /2 NOT  r/m8          reverse each bit of r/m8                                                                                           (IA V2 p341)
            case 3: alu.NEG<byte>(val);                          break; // /3 NEG  r/m8          two's complement negate r/m8                                                                                       (IA V2 p338)
            case 4: alu.MUL <byte>(d_regs.a.h, d_regs.a.l, val); break; // /4 MUL  r/m8          unsigned multiply (AX<-AL*r/m8)                                                                                    (IA V2 p336)
            case 5: alu.IMUL<byte>(d_regs.a.h, d_regs.a.l, val); break; // /5 IMUL r/m8          AX<-AL*r/m byte                                                                                                    (IA V2 p238)
            case 6: alu.DIV <byte>(d_regs.a.h, d_regs.a.l, val); break; // /6 DIV  r/m8          unsigned divide AX by r/m8; AL <- quotient, AH <- remainder                                                        (IA V2 p114)
            case 7: alu.IDIV<byte>(d_regs.a.h, d_regs.a.l, val); break; // /7 IDIV r/m8          signed divide AX (where AH must contain sign-extended of AL) by r/m byte. (Results: AL=quotient, AH=remainder)     (IA V2 p235)
            }
         }(rm<byte>());
         break;
      }
      case 0xF7: // Unary Group 3^2 Ev
      {
         fetchModRM();
         [&](word& val) {
            switch (ext) {
            case 0: alu._and(val, imm<word>());                  break; // /0 TEST r/m16,imm16   AND imm16 with r/m16; set SF,ZF,PF according to result                                                             (IA V2 p480)
            case 1: assert(false);                                      // /1 (not used)                                                                                                                            (8086 Table 4-13 page 4-35)
            case 2: alu.NOT<word>(val);                          break; // /2 NOT  r/m16         reverse each bit of r/m16                                                                                          (IA V2 p341)
            case 3: alu.NEG<word>(val);                          break; // /3 NEG  r/m16         two's complement negate r/m16                                                                                      (IA V2 p338)
            case 4: alu.MUL <word>(d_regs.d.x, d_regs.a.x, val); break; // /4 MUL  r/m16         unsigned multiply (DX:AX<-AX*r/m16)                                                                                (IA V2 p336)
            case 5: alu.IMUL<word>(d_regs.d.x, d_regs.a.x, val); break; // /5 IMUL r/m16         DX:AX<-AX*r/m word                                                                                                 (IA V2 p238)
            case 6: alu.DIV <word>(d_regs.d.x, d_regs.a.x, val); break; // /6 DIV  r/m16         unsigned divide D:AX by r/m16; AX <- quotient, DX <- remainder                                                     (IA V2 p114)
            case 7: alu.IDIV<word>(d_regs.d.x, d_regs.a.x, val); break; // /7 IDIV r/m16         signed divide DX:AX (where DX must contain sign-extension of AX) by r/m word. (Results: AX=quotient, DX=remainder) (IA V2 p235)
            }
         }(rm<word>());
         break;
      }

      // AAS = ASCII adjust for subtract
      // [00111111]
      case 0x3F: alu.AAS(d_regs.a.h, d_regs.a.l); break; // AAS                ASCII adjust AL after subtraction (IA V2 p44)

      // DAS = Decimal adjust for subtract
      // [00101111]
      case 0x2F: alu.DAS(d_regs.a.l); break; // DAS                Decimal adjust AL after subtraction (IA V2 p111)

      // AAM = ASCII adjust for multiply
      // [11010100] [00001010]                       NOTE: Second byte selects number base. 0x8 for octal, 0xA for decimal, 0xC for base 12
      case 0xD4: // AAM                ASCII adjust AX after multiply (IA V2 p43)
      {
         byte base = imm<byte>();

         if (base == 0) // divide by 0 error
            break; // TODO: how to handle this? Do I care? No.

         d_regs.a.h = d_regs.a.l / base;
         d_regs.a.l = d_regs.a.l % base;

         alu.setFlags<word>(d_regs.a.x);

         break;
      }
      // AAD = ASCII adjust for divide
      // [11010101] [00001010]                       NOTE: Second byte selects number base. 0x8 for octal, 0xA for decimal, 0xC for base 12
      case 0xD5: // AAD                ASCII adjust AX before division (IA V2 p42)
      {
         byte base = imm<byte>();

         //if (base == 0) // base 0 is stupid
         //   break; // TODO: how to handle this? Do I care? No.

         d_regs.a.l = (d_regs.a.l + (d_regs.a.h * base)) & 0xFF;
         d_regs.a.h = 0;

         alu.setFlags<word>(d_regs.a.x);

         break;
      }
      // CBW = Convert byte to word
      // [10011000]
      case 0x98: // CBW                AX <- sign-extended of AL (IA V2 p79)
      {
         if (d_regs.a.l & 0x80) d_regs.a.h = 0xFF;
         else                   d_regs.a.h = 0;
         break;
      }
      // CWD = Convert word to double word
      // [10011001]
      case 0x99: // CWD                DX:AX <- sign-extend of AX (IA V2 p107)
      {
         if (d_regs.a.x & 0x8000) d_regs.d.x = 0xFFFF;
         else                     d_regs.d.x = 0;
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
      {
         fetchModRM();
         [&](byte& dest) {
            switch (ext) {
            case 0: dest = alu.ROL<byte>(dest, 1); break; // /0 ROL r/m8,1      Rotate 8 bits r/m8 left once       (IA V2 p424)
            case 1: dest = alu.ROR<byte>(dest, 1); break; // /1 ROR r/m8,1      Rotate 8 bits r/m8 right once      (IA V2 p424)
            case 2: dest = alu.RCL<byte>(dest, 1); break; // /2 RCL r/m8,1      Rotate 9 bits (CF,r/m8) left once  (IA V2 p424)
            case 3: dest = alu.RCR<byte>(dest, 1); break; // /3 RCR r/m8,1      Rotate 9 bits (CF,r/m8) right once (IA V2 p424)
            case 4: dest = alu.SHL<byte>(dest, 1); break; // /4 SHL/SAL r/m8,1  Multiply r/m8 by 2, once           (IA V2 p446)
            case 5: dest = alu.SHR<byte>(dest, 1); break; // /5 SHR r/m8,1      Unsigned divide r/m8 by 2, once    (IA V2 p446)
            case 6: assert(false);                        // /6 (not used)                                         (8086 Table 4-13 page 4-33)
            case 7: dest = alu.SAR<byte>(dest, 1); break; // /7 SAR r/m8,1      Signed divide* r/m8 by 2, once     (IA V2 p446)
            }
         }(rm<byte>());
         break;
      }
      case 0xD1: // Shift Group 2^2 Ev,1
      {
         fetchModRM();
         [&](word& dest) {
            switch (ext) {
            case 0: dest = alu.ROL<word>(dest, 1); break; // /0 ROL r/m16,1     Rotate 16 bits r/m16 left once       (IA V2 p424)
            case 1: dest = alu.ROR<word>(dest, 1); break; // /1 ROR r/m16,1     Rotate 16 bits r/m16 right once      (IA V2 p424)
            case 2: dest = alu.RCL<word>(dest, 1); break; // /2 RCL r/m16,1     Rotate 17 bits (CF,r/m16) left once  (IA V2 p424)
            case 3: dest = alu.RCR<word>(dest, 1); break; // /3 RCR r/m16,1     Rotate 17 bits (CF,r/m16) right once (IA V2 p424)
            case 4: dest = alu.SHL<word>(dest, 1); break; // /4 SHL/SAL r/m16,1 Multiply r/m16 by 2, once            (IA V2 p446)
            case 5: dest = alu.SHR<word>(dest, 1); break; // /5 SHR r/m16,1     Unsigned divide r/m16 by 2, once     (IA V2 p446)
            case 6: assert(false);                        // /6 (not used)                                           (8086 Table 4-13 page 4-34)
            case 7: dest = alu.SAR<word>(dest, 1); break; // /7 SAR r/m16,1     Signed divide* r/m16 by 2, once      (IA V2 p446)
            }
         }(rm<word>());
         break;
      }
      case 0xD2: // Shift Group 2^2 Eb,CL
      {
         fetchModRM();
         [&](byte& dest) {
            switch (ext) {
            case 0: dest = alu.ROL<byte>(dest, d_regs.c.l); break; // /0 ROL r/m8,CL     Rotate 8 bits r/m8 left CL times       (IA V2 p424)
            case 1: dest = alu.ROR<byte>(dest, d_regs.c.l); break; // /1 ROR r/m8,CL     Rotate 8 bits r/m8 right CL times      (IA V2 p424)
            case 2: dest = alu.RCL<byte>(dest, d_regs.c.l); break; // /2 RCL r/m8,CL     Rotate 9 bits (CF,r/m8) left CL times  (IA V2 p424)
            case 3: dest = alu.RCR<byte>(dest, d_regs.c.l); break; // /3 RCR r/m8,CL     Rotate 9 bits (CF,r/m8) right CL times (IA V2 p424)
            case 4: dest = alu.SHL<byte>(dest, d_regs.c.l); break; // /4 SHL/SAL r/m8,CL Multiply r/m8 by 2, CL times           (IA V2 p446)
            case 5: dest = alu.SHR<byte>(dest, d_regs.c.l); break; // /5 SHR r/m8,CL     Unsigned divide r/m8 by 2, CL times    (IA V2 p446)
            case 6: assert(false);                                 // /6 (not used)                                             (8086 Table 4-13 page 4-34)
            case 7: dest = alu.SAR<byte>(dest, d_regs.c.l); break; // /7 SAR r/m8,CL     Signed divide* r/m8 by 2, CL times     (IA V2 p446)
            }
         }(rm<byte>());
         break;
      }
      case 0xD3: // Shift Group 2^2 Ev,CL
      {
         fetchModRM();
         [&](word& dest) {
            switch (ext) {
            case 0: dest = alu.ROL<word>(dest, d_regs.c.l); break; // /0 ROL r/m16,CL    Rotate 16 bits r/m16 left CL times       (IA V2 p424)
            case 1: dest = alu.ROR<word>(dest, d_regs.c.l); break; // /1 ROR r/m16,CL    Rotate 16 bits r/m16 right CL times      (IA V2 p424)
            case 2: dest = alu.RCL<word>(dest, d_regs.c.l); break; // /2 RCL r/m16,CL    Rotate 17 bits (CF,r/m16) left CL times  (IA V2 p424)
            case 3: dest = alu.RCR<word>(dest, d_regs.c.l); break; // /3 RCR r/m16,CL    Rotate 17 bits (CF,r/m16) right CL times (IA V2 p424)
            case 4: dest = alu.SHL<word>(dest, d_regs.c.l); break; // /4 SHL/SAL r/m16,CL Multiply r/m16 by 2, CL times           (IA V2 p446)
            case 5: dest = alu.SHR<word>(dest, d_regs.c.l); break; // /5 SHR r/m16,CL    Unsigned divide r/m16 by 2, CL times     (IA V2 p446)
            case 6: assert(false);                                 // /6 (not used)                                               (8086 Table 4-13 page 4-34)
            case 7: dest = alu.SAR<word>(dest, d_regs.c.l); break; // /7 SAR r/m16,CL    Signed divide* r/m16 by 2, CL times      (IA V2 p446)
            }
         }(rm<word>());
         break;
      }

      // TEST = And function to flags no result

      // Register/memory and register
      // [1000 010 w] [mod reg r/m] [(DISP-LO)] [(DISP-HI)]
      case 0x84: fetchModRM(); alu._and<byte>(reg<byte>(), rm<byte>()); break; // TEST r/m8,r8       AND r8 with r/m8; set SF,ZF,PF according to result (IA V2 p480)
      case 0x85: fetchModRM(); alu._and<word>(reg<word>(), rm<word>()); break; // TEST r/m16,r16     AND r16 with r/m16; set SF,ZF,PF according to result (IA V2 p480)

      // Immediate data and accumulator
      // [1010100 w] [data] *[data if w=1] *this was not included in documentation, but I believe that it should be there.
      case 0xA8: alu._and<byte>(d_regs.a.l, imm<byte>()); break; // TEST AL,imm8       AND imm8 with AL; set SF,ZF,PF according to result (IA V2 p480)
      case 0xA9: alu._and<word>(d_regs.a.x, imm<word>()); break; // TEST AX,imm16      AND imm16 with AX; set SF,ZF,PF according to result (IA V2 p480)



         //*******************************************//
         //******      String Manipulation      ******//
         //****** REP MOVS CMPS SCAS LODS STDS  ******//
         //*******************************************//



      // MOVS = Move byte/word
      // [1010010 w]
      case 0xA4: // MOVS m8,m8         move byte at address DS:(E)SI to address ES:(E)DI (IA V2 p329)
      {
         // F3 A4    REP MOVS m8,m8       Move (E)CX bytes from DS:[(E)SI] to ES:[(E)DI] (IA V2 p434)
         REP([&] {
            mem<byte>(segRegs.ES, pi_regs.DI) = mem<byte>(segRegs.DS, pi_regs.SI);
            pi_regs.SI += (alu.flags.D == 0) ? 1 : -1;
            pi_regs.DI += (alu.flags.D == 0) ? 1 : -1;
         });
         break;
      }
      case 0xA5: // MOVS m16,m16       move word at address DS:(E)SI to address ES:(E)DI (IA V2 p329)
      {
         // F3 A5    REP MOVS m16,m16     Move (E)CX words from DS:[(E)SI] to ES:[(E)DI] (IA V2 p434)
         REP([&] {
            mem<word>(segRegs.ES, pi_regs.DI) = mem<word>(segRegs.DS, pi_regs.SI);
            pi_regs.SI += (alu.flags.D == 0) ? 2 : -2;
            pi_regs.DI += (alu.flags.D == 0) ? 2 : -2;
         });
         break;
      }
      // CMPS = Compare byte/word
      // [1010011 w]
      case 0xA6: // CMPS m8,m8         compares byte at address DS:(E)SI with byte at address ES:(E)DI and sets the status flags accordingly (IA V2 p93)
      {
         // F2 A6    REPNE CMPS m8,m8     Find matching bytes in ES:[(E)DI] and DS:[(E)SI]    (IA V2 p434)
         // F3 A6    REPE CMPS m8,m8      Find nonmatching bytes in ES:[(E)DI] and DS:[(E)SI] (IA V2 p434)
         REP([&] {
            alu.sub<byte>(
               mem<byte>(segRegs.DS, pi_regs.SI),
               mem<byte>(segRegs.ES, pi_regs.DI));
            pi_regs.SI += (alu.flags.D == 0) ? 1 : -1;
            pi_regs.DI += (alu.flags.D == 0) ? 1 : -1;
         });
         break;
      }
      case 0xA7: // CMPS m16,m16       compares word at address DS:(E)SI with word at address ES:(E)DI and sets the status flags accordingly (IA V2 p93)
      {
         // F2 A7    REPNE CMPS m16,m16   Find matching words in ES:[(E)DI] and DS:[(E)SI]    (IA V2 p434)
         // F3 A7    REPE CMPS m16,m16    Find nonmatching words in ES:[(E)DI] and DS:[(E)SI] (IA V2 p434)
         REP([&] {
            alu.sub<word>(
               mem<word>(segRegs.DS, pi_regs.SI),
               mem<word>(segRegs.ES, pi_regs.DI));
            pi_regs.SI += (alu.flags.D == 0) ? 2 : -2;
            pi_regs.DI += (alu.flags.D == 0) ? 2 : -2;
         });
         break;
      }
      // SCAS = Scan byte/word
      // [1010111 w]
      case 0xAE: // SCAS m8            compare AL with byte at ES:(E)DI and set status flags (IA V2 p452)
      {
         // F2 AE    REPNE SCAS m8        Find AL, starting at ES:[(E)DI]         (IA V2 p434)
         // F3 AE    REPE SCAS m8         Find non-AL byte starting at ES:[(E)DI] (IA V2 p434)
         REP([&] {
            alu.sub<byte>(d_regs.a.l, mem<byte>(segRegs.ES, pi_regs.DI));
            pi_regs.DI += (alu.flags.D == 0) ? 1 : -1;
         });
         break;
      }
      case 0xAF: // SCAS m16           compare AX with word at ES:(E)DI and set status flags (IA V2 p452)
      {
         // F2 AF    REPNE SCAS m16       Find AX, starting at ES:[(E)DI]         (IA V2 p434)
         // F3 AF    REPE SCAS m16        Find non-AX word starting at ES:[(E)DI] (IA V2 p434)
         REP([&] {
            alu.sub<word>(d_regs.a.x, mem<word>(segRegs.ES, pi_regs.DI));
            pi_regs.DI += (alu.flags.D == 0) ? 2 : -2;
         });
         break;
      }
      // LODS = Load byte/wd to AL/AX
      // [1010110 w]
      case 0xAC: // LODS m8            load byte at address DS:(E)SI into AL (IA V2 p305)
      {
         // F3 AC    REP LODS AL          Load (E)CX bytes from DS:[(E)SI] to AL (IA V2 p434)
         REP([&] {
            d_regs.a.l = mem<byte>(segRegs.DS, pi_regs.SI);
            pi_regs.SI += (alu.flags.D == 0) ? 1 : -1;
         });
         break;
      }
      case 0xAD: // LODS m16           load word at address DS:(E)SI into AX (IA V2 p305)
      {
         // F3 AD    REP LODS AX          Load (E)CX words from DS:[(E)SI] to AX (IA V2 p434)
         REP([&] {
            d_regs.a.x = mem<word>(segRegs.DS, pi_regs.SI);
            pi_regs.SI += (alu.flags.D == 0) ? 2 : -2;
         });
         break;
      }
      // STDS = Stor byte/wd from AL/A
      // [1010101 w]
      case 0xAA: // STOS m8            store AL at address ES:(E)DI (IA V2 p473)
      {
         // F3 AA    REP STOS m8          Fill (E)CX bytes at ES:[(E)DI] with AL (IA V2 p434)
         REP([&] {
            mem<byte>(segRegs.DS, pi_regs.SI) = d_regs.a.l;
            pi_regs.SI += (alu.flags.D == 0) ? 1 : -1;
         });
         break;
      }
      case 0xAB: // STOS m16           store AX at address ES:(E)DI (IA V2 p473)
      {
         // F3 AB    REP STOS m16         Fill (E)CX words at ES:[(E)DI] with AX (IA V2 p434)
         REP([&] {
            mem<word>(segRegs.DS, pi_regs.SI) = d_regs.a.x;
            pi_regs.SI += (alu.flags.D == 0) ? 2 : -2;
         });
         break;
      }



      //**************************************************//
      //******           Control Transfer           ******//
      //****** CALL JMP RET J__ LOOP_ INT INTO IRET ******//
      //**************************************************//



   // CALL = Call

   // Direct within segment
   // [11101000] [IP-INC-LO] [IP-INC-HI]
      case 0xE8: callnear(imm<word>()); break; // CALL rel16         Call near, relative, displacement relative to next instruction (IA V2 p68)
      // Direct intersegment
      // [10011010] [IP-lo] [IP-hi] [CS-lo] [CS-hi]
      case 0x9A: callFar(imm<word>(), imm<word>()); break; // CALL ptr16:16      Call far, absolute, address given in operand (IA V2 p68)

      // JMP = Unconditional Jump

      // Direct within segment
      // [11101001] [IP-INC-LO] [IP-INC-HI]
      case 0xE9: jumpShort(imm<word>()); /* short does relative */ break; // JMP rel16          jump near, relative, displacement relative to next instruction (IA V2 p275)
      // Direct within segment-short
      // [11101011] [IP-INC8]
      case 0xEB: jumpShort((int16_t)(int8_t)imm<byte>()); break; // JMP rel8           jump short, relative, displacement relative to next instruction (IA V2 p275)
      // Indirect within segment
      // [11101010] [IP-lo] [IP-hi] [CS-lo] [CS-hi]
      case 0xEA: jumpFar(imm<word>(), imm<word>()); break; // JMP ptr16:16       jump far, absolute, address given in operand (IA V2 p275)

      // RET = Return from CALL

      // Within segment
      // [11000011]
      case 0xC3: returnNear(); break; // RET                near return to calling procedure (IA V2 p437)
      // Within seg adding immed to SP
      // [11000010] [data-lo] [data-hi]
      case 0xC2: returnNear(imm<word>()); break; // RET imm16          near return to calling procedure and pop imm16 bytes from stack (IA V2 p437)
      // Intersegment
      // [11001011]
      case 0xCB: returnFar(); break; // RET                far return to calling procedure (IA V2 p437)
      // Intersegment adding immediate to SP
      // [11001010] [data-lo] [data-hi]
      case 0xCA: returnFar(imm<word>()); break; // RET imm16          far return to calling procedure and pop imm16 bytes from stack (IA V2 p437)

      case 0x70: // JO                 jump short if overflow (OF=1)                            (IA V2 p271)
      case 0x71: // JNO                jump short if not overflow (OF=0)                        (IA V2 p271)
      case 0x72: // JB/JNAE/JC         jump short if below/carry/not above or equal (CF=1)      (IA V2 p271)
      case 0x73: // JNB/JAE/JNC        jump short if above or equal/not below/not carry (CF=0)  (IA V2 p271)
      case 0x74: // JZ/JE              jump short if equal/zero (ZF=1)                          (IA V2 p271)
      case 0x75: // JNE/JNZ            jump short if not eqaul/not zero (ZF=0)                  (IA V2 p271)
      case 0x76: // JBE                jump short if below or equal/not above (CF=1 or ZF = 1)  (IA V2 p271)
      case 0x77: // JA/JNBE            jump short if above/not below or equal (CF=0 and ZF=0)   (IA V2 p271)
      case 0x78: // JS                 jump short if sign (SF=1)                                (IA V2 p271)
      case 0x79: // JNS                jump short if not sign (SF=0)                            (IA V2 p271)
      case 0x7A: // JP/JPE             jump short if parity/parity even (PF=1)                  (IA V2 p271)
      case 0x7B: // JNP/JPO            jump short if not parity/parity odd (PF=0)               (IA V2 p271)
      case 0x7C: // JL/JNGE            jump short if less/not greater or equal (SF<>OF)         (IA V2 p271)
      case 0x7D: // JGE/JNL            jump short if greater or equal/not less (SF=OF)          (IA V2 p271)
      case 0x7E: // JLE/JNG            jump short if less or equal/not greater (ZF=1 or SF<>OF) (IA V2 p271)
      case 0x7F: // JNLE/JG            jump short if greater/not less or equal (ZF=0 and SF=OF) (IA V2 p271)
      {
         int disp16 = (int16_t)(int8_t)imm<byte>();

         if (alu.condition(opcode & 0xF))
            jumpShort(disp16);

         break;
      }


      // Loop = Loop CX times:                         [11100010]
      case 0xE2: // LOOP rel8          decrement count; jump short if count!=0 (IA V2 p308)
      {
         int disp16 = (int16_t)(int8_t)imm<byte>();
         d_regs.c.x--;
         if (d_regs.c.x != 0)
            jumpShort(disp16);
         break;
      }
      // LOOPZ/LOOPE = Loop while zero/equal:          [11100001]
      case 0xE1: // LOOPE/LOOPZ rel8   decrement count; jump short if count!=0 and ZF=1 (IA V2 p308)
      {
         int disp16 = (int16_t)(int8_t)imm<byte>();
         d_regs.c.x--;
         if ((d_regs.c.x != 0) && (alu.flags.Z == 1))
            jumpShort(disp16);
         break;
      }
      // LOOPNZ/LOOPNE = Loop while not zero/equal:    [11100000]
      case 0xE0: // LOOPNE/LOOPNZ rel8 decrement count; jump short if count!=0 and ZF=0 (IA V2 p308)
      {
         int disp16 = (int16_t)(int8_t)imm<byte>();
         d_regs.c.x--;
         if ((d_regs.c.x != 0) && (alu.flags.Z == 0))
            jumpShort(disp16);
         break;
      }
      // JCXZ = Jump on CX zero:                       [11100011]
      case 0xE3: // JCXZ rel8          jump short if CX register is 0 (IA V2 p271)
      {
         int disp16 = (int16_t)(int8_t)imm<byte>();
         if (d_regs.c.x == 0)
            jumpShort(disp16);
         break;
      }

      // INT = Interrupt

      // Type specified
      // [11001101] [DATA-8]
      case 0xCD: // INT imm8           interrupt vector number specified by immediate byte (IA V2 p248)
      {
         interrupt(imm<byte>());
         break;
      }
      // Type 3
      // [11001100]
      case 0xCC: // INT 3              interrupt 3-trap to debugger (IA V2 p248)
      {
         interrupt(3);
         break;
      }
      // INTO = Interrupt on overflow
      // [11001110]
      case 0xCE: // INTO               interrupt 4-if overflow flag is 1 (IA V2 p248)
      {
         if (alu.flags.O == 1)
            interrupt(4);
         break;
      }
      // IRET=Interrupt return
      // [11001111]
      case 0xCF: // IRET               interrupt return (IA V2 p263)
      {
         returnFar();
         alu.flags.set(pop());
         break;
      }



      //*******************************************************************//
      //******                   Processor Controll                  ******//
      //****** CLC CMC STC CLD STD CLI STI HLT WAIT ESC LOCK SEGMENT ******//
      //*******************************************************************//



      case 0xF4: halted = true;                       break; // HLT                Halt (IA V2 p234)
      case 0xF5: alu.flags.C = (alu.flags.C ? 0 : 1); break; // CMC                Complement CF flag (IA V2 p86)
      case 0xF8: alu.flags.C = 0;                     break; // CLC                clear CF flag (IA V2 p81)
      case 0xF9: alu.flags.C = 1;                     break; // STC                set CF flag (IA V2 p469)
      case 0xFA: alu.flags.I = 0;                     break; // CLI                clear interrupt flag; interrupts disabled when interrupt flag cleared (IA V2 p83)
      case 0xFB: alu.flags.I = 1;                     break; // STI                set interrupt flag; external, maskable interrupts enabled at the end of the next instruction (IA V2 p471)
      case 0xFC: alu.flags.D = 0;                     break; // CLD                clear DF flag (IA V2 p82)
      case 0xFD: alu.flags.D = 1;                     break; // STD                set DF flag (IA V2 p470)

      // WAIT = Wait:                       [10011011]
      case 0x9B: // WAIT               check pending unmasked floating-point exceptions (IA V2 p485)
                 // FCLEX              clear floating-point exception flags after checking for pending unmasked floating-point exceptions (IA V2 p135)
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
      {
         fetchModRM();
         break; // escape to x87 FPU (unsupported)
      }
      // Input from Port to String
      case 0x6C: // INS m8,DX          Input byte from I/O port specified in DX into memory location specified in ES:(E)DI (IA V2 p245)
      {
         // F3 6C    REP INS r/m8, DX     Input (E)CX bytes from port DX into ES:[(E)DI] (IA V2 p434)
         REP([&] {
            mem<byte>(segRegs.ES, pi_regs.DI) = io->read<byte>(d_regs.d.x);
            if (alu.flags.D == 0) pi_regs.DI += 1;
            else                  pi_regs.DI -= 1;
         });
         break;
      }
      case 0x6D: // INS m16,DX         Input word from I/O port specified in DX into memory location specified in ES:(E)DI (IA V2 p245)
      {
         // F3 6D    REP INS r/m16,DX     Input (E)CX words from port DX into ES:[(E)DI] (IA V2 p434)
         REP([&] {
            mem<word>(segRegs.ES, pi_regs.DI) = io->read<word>(d_regs.d.x);
            if (alu.flags.D == 0) pi_regs.DI += 2;
            else                  pi_regs.DI -= 2;
         });
         break;
      }

      // unused opcodes (8086 Family Table 4-13)
      // 0x0F Later CPUs use this as an escape character
      // 0x60-6F, Later CPUs use 0x66 as operand size override and 0x67 as address override
      // 0xC0-C1,
      // 0xC8 C9,
      // 0xD6,
      // 0xF1
      default: // The 8086/8088 treated these like NOPs. Future CPUs trip invalid opcode exception
         break;
      }
   }
}