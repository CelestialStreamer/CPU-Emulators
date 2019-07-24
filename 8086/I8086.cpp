#include "I8086.h"

#include <cassert>



void I8086::push(word value) { mem<word>(segRegs.SS, (pi_regs.SP -= 2)) = value; }
word I8086::pop() { return mem<word>(segRegs.SS, (pi_regs.SP += 2) - 2); }

void I8086::reset() {
   /// [I]nitializes the system as shown in Table 2-4. (8086 Family p2-29:System Reset)
   alu.flags.clear();
   IP = 0;
   segRegs.CS = 0xffff;
   segRegs.DS = 0;
   segRegs.SS = 0;
   segRegs.ES = 0;

   /// The processor leaves the halt state upon activation of the RESET line (8086 Family p2-48:HLT)
   halted = false;
}

I8086::I8086(Memory* memory, IO* io) : memory(memory), io(io) { reset(); }
I8086::~I8086() { delete memory; delete io; }
void I8086::externalInterrupt(unsigned int vector) {
   if (alu.flags.I)
      interrupt(vector);
}
void I8086::interrupt(unsigned int vector) {
   push(alu.flags.get<word>());
   auto temp = alu.flags.T;
   alu.flags.I = alu.flags.T = 0;
   push(segRegs.CS); push(IP);
   /// Call interrupt service routine
   callfar(vector);
   if (false) // non maskable interrupt
      interrupt(0); // which vector?
   else if (temp)
      interrupt(0);
   /// Execute user interrupt procedure
   returnFar();
   alu.flags.set<word>(pop());
   // resume interrupted procedure
}

// r8 register mode
template<>
byte& I8086::reg() {
   switch (_reg) {
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
// r16 register mode
template<>
word& I8086::reg() {
   switch (_reg) {
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

void I8086::fetchModRM() {
   // (IA V2 2-1 Table 2-1 Intel Architecture Instruction Format)
   // (8086 Family 4-19 Figure 4-20 Typical 8086/8088 Machine Instruction Format)
   byte addrbyte = imm<byte>();

   _mode = (addrbyte >> 6) & 0b011; // 1100 0000
   _reg = (addrbyte >> 3) & 0b111; // 0011 1000
   _rm = (addrbyte >> 0) & 0b111; // 0000 0111

   bool BP = (_rm == 2) || (_rm == 3) || (_rm == 6); // If the BP index is used

   // Check to see if there is a displacement byte or word
   switch (_mode) {
   case 0: // No offset, but rm=6 uses disp16 instead of [BP] like the rest
   {
      if (_rm == 6) // No segment override, just uses an immediate displacement
         disp16 = imm<word>();
      else if (!segoverride && BP) // (See note 1)
         segment = segRegs.SS;
      break;
   }
   case 1: // 8 bit offset (see note 2)
   {
      disp16 = (int16_t)imm<byte>(); // Sign extend immediate byte to 16 bit
      if (!segoverride && BP) // (See note 1)
         segment = segRegs.SS;
      break;
   }
   case 2: // 16 bit offset (see note 3)
   {
      disp16 = imm<word>();
      if (!segoverride && BP) // (See note 1)
         segment = segRegs.SS;
      break;
   }
   default: // no displacement, register access
      disp16 = 0;
   }
   /// 1) The default segment register is SS for the effective addresses containing a BP index, DS for other effective addresses.
   /// 2) The "disp16" nomenclature denotes a 16-bit displacement following the ModR/M byte, to be added to the index.
   /// 3) The "disp8" nomenclature denotes an 8 bit displacement following the ModR/M byte, to be sign-extended and added to the index.
   /// (IA V2 2-4 Table 2-1)
}

int I8086::ea() {
   int tempea = 0;
   switch (_mode) {
   case 0: // no displacement, but case six is special
      switch (_rm) {
      case 0:  tempea = d_regs.b.x + pi_regs.SI; break;
      case 1:  tempea = d_regs.b.x + pi_regs.DI; break;
      case 2:  tempea = pi_regs.BP + pi_regs.SI; break;
      case 3:  tempea = pi_regs.BP + pi_regs.DI; break;
      case 4:  tempea = pi_regs.SI;              break;
      case 5:  tempea = pi_regs.DI;              break;
      case 6:  tempea = disp16;                  break;
      default: tempea = d_regs.b.x;              break;
      }
      break;
   case 1: // disp8 (already been sign extended to 16 bit)
   case 2: // disp16
      switch (_rm) {
      case 0:  tempea = d_regs.b.x + pi_regs.SI + disp16; break;
      case 1:  tempea = d_regs.b.x + pi_regs.DI + disp16; break;
      case 2:  tempea = pi_regs.BP + pi_regs.SI + disp16; break;
      case 3:  tempea = pi_regs.BP + pi_regs.DI + disp16; break;
      case 4:  tempea = pi_regs.SI + disp16;              break;
      case 5:  tempea = pi_regs.DI + disp16;              break;
      case 6:  tempea = pi_regs.BP + disp16;              break;
      default: tempea = d_regs.b.x + disp16;              break;
      }
      break;
   default: // register access, no displacement
      break;
   }

   return (segment << 4) + (tempea & 0xFFFF);
}