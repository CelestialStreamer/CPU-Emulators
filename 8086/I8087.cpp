#include "I8087.h"



I8087::I8087()
{
}


I8087::~I8087()
{
}

void I8087::run(byte opcode, byte ModRegRM) {
   int mod = (ModRegRM >> 5) & 0b11;
   int ext = (ModRegRM >> 2) & 0b111;
   int rm = (ModRegRM >> 0) & 0b111;

   if (opcode == 0x9b) {
      // CPU Wait for 807
   } else {
      opcode &= 0b111; // Only botton three bits are used
      switch (opcode & 1) {
      case 0: // - - 0 | - - - - - - - -
         switch (ext) { // if MOD=11, use ST(i) in place of R/M
         case 0b000: // - - 0 | - - 0 0 0 - - -
            // FADD = Addition
            // ESCAPE  MF 0 | MOD 0 0 0 R/M   | DISP     Integer/Real memory with ST(0)
            // ESCAPE d P 0 | 1 1 0 0 0 ST(i)            ST(i) and ST(0)
            break;
         case 0b001: // - - 0 | - - 0 0 1 - - -
            // FMUL = Multiplication
            // ESCAPE  MF 0 | MOD 0 0 1 R/M   | DISP     Integer/Real memory with ST(0)
            // ESCAPE d P 0 | 1 1 0 0 1 ST(i)            ST(i) and ST(0)
            break;
         case 0b010: // - - 0 | - - 0 1 0 - - -
            // FCOM = Compare
            // ESCAPE  MF 0 | MOD 0 1 0 R/M   | DISP     Integer/Real memory to ST(0)
            // ESCAPE 0 0 0 | 1 1 0 1 0 ST(i)            ST(i) to ST(0)
            break;
         case 0b011: // - - 0 | - - 0 1 1 - - -
            // FCOMPP = Compare ST(1) to ST(0) and Pop Twice
            // ESCAPE 1 1 0 | 1 1 0 1 1 0 0 1

            // FCOMP = Compare and Pop
            // ESCAPE  MF 0 | MOD 0 1 1 R/M   | DISP     Integer/Real memory to ST(0)
            // ESCAPE 0 0 0 | 1 1 0 1 1 ST(i)            ST(i) to ST(0)
            break;
         case 0b100: // - - 0 | - - 1 0 0 - - -
         case 0b101: // - - 0 | - - 1 0 1 - - -
            // FSUB = Subtraction
            // ESCAPE  MF 0 | MOD 1 0 R R/M   | DISP     Integer/Real memory with ST(0)
            // ESCAPE d P 0 | 1 1 1 0 R ST(i)            ST(i) and ST(0)
            break;
         case 0b110: // - - 0 | - - 1 1 0 - - -
         case 0b111: // - - 0 | - - 1 1 1 - - -
            // FDIV = Division
            // ESCAPE  MF 0 | MOD 1 1 R R/M   | DISP     Integer/Real memory with ST(0)
            // ESCAPE d P 0 | 1 1 1 1 R ST(i)            ST(i) and ST(0)
            break;
         }
         break;
      case 1: // - - 1 | - - - - - - - -
         switch (ext) {
         case 0b000: // - - 1 | - - 0 0 0 - - -
            if ((opcode == 0b101) && (mod == 0b11)) {
               // FFREE = Free ST(i)
               // ESCAPE 1 0 1 | 1 1 0 0 0 ST(i)
            } else {
               // FLD = LOAD
               // ESCAPE  MF 1 | MOD 0 0 0 R/M   | DISP     Integer/Real memory to ST(0)
               // ESCAPE 0 0 1 | 1 1 0 0 0 ST(i) |          ST(i) to ST(0)
            }
            break;
         case 0b001: // - - 1 | - - 0 0 1 - - -
            if ((opcode == 0b001) && (mod == 0b11)) {
               // FXCH = Exchange ST(i) and ST(0)
               // ESCAPE 0 0 1 | 1 1 0 0 1 ST(i)            ST(i) and ST(0)
            }
            // Unused
            // ESCAPE 0 0 1 | 0 0 0 0 1 - - -
            // ESCAPE 0 0 1 | 0 1 0 0 1 - - -
            // ESCAPE 0 0 1 | 1 0 0 0 1 - - -

            // Unused
            // ESCAPE 0 1 1 | - - 0 0 1 - - -
            // ESCAPE 1 0 1 | - - 0 0 1 - - -
            // ESCAPE 1 1 1 | - - 0 0 1 - - -
            break;
         case 0b010: // - - 1 | - - 0 1 0 - - -
            if ((opcode == 0b001) && (ModRegRM == 0b1101'0000)) {
               // FNOP = No Operation
               // ESCAPE 0 0 1 | 1 1 0 1 0 0 0 0
            } else {
               // FST = STORE
               // ESCAPE  MF 1 | MOD 0 1 0 R/M   | DISP     ST(0) to Integer/Real Memory
               // ESCAPE 1 0 1 | 1 1 0 1 0 ST(i)            ST(0) to ST(i)
            }
            break;
         case 0b011: // - - 1 | - - 0 1 1 - - -
            // FSTP = STORE AND POP
            // ESCAPE  MF 1 | MOD 0 1 1 R/M   | DISP     ST(0) to Integer/Real Memory
            // ESCAPE 1 0 1 | 1 1 0 1 1 ST(i)            ST(0) to ST(i)
            break;
         case 0b100: // - - 1 | - - 1 0 0 - - -
            switch (opcode) {
            case 0b001: // 0 0 1 | - - 1 0 0 - - -
               switch (ModRegRM) {
               case 0b11'100'000:
                  // FCHS = Change Sign of ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 0 0 0 0
                  break;
               case 0b11'100'001:
                  // FABS = Absolute Value of ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 0 0 0 1
                  break;
               case 0b11'100'100:
                  // FTST = Test ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 0 1 0 0
                  break;
               case 0b11'100'101:
                  // FXAM = Examine ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 0 1 0 1
                  break;
               default: // - - 1 | - - 1 0 0 - - -
                  // FLDENV = Load Environment
                  // ESCAPE 0 0 1 | MOD 1 0 0 R/M   | DISP
                  break;
               }
               break;
            case 0b011: // 0 1 1 | - - 1 0 0 - - -
               switch (ModRegRM) {
               case 0b11'100'000:
                  // FENI = Enable Interrupts
                  // ESCAPE 0 1 1 | 1 1 1 0 0 0 0 0
                  break;
               case 0b11'100'001:
                  // FDISI = Disable Interrupts
                  // ESCAPE 0 1 1 | 1 1 1 0 0 0 0 1
                  break;
               case 0b11'100'010:
                  // FCLEX = Clear Exceptions
                  // ESCAPE 0 1 1 | 1 1 1 0 0 0 1 0
                  break;
               case 0b11'100'011:
                  // FINIT = Initialized 8087
                  // ESCAPE 0 1 1 | 1 1 1 0 0 0 1 1
                  break;
               default: // 0 1 1 | - - 1 0 0 - - -
                  // Unused
                  // ESCAPE 0 1 1 | - - 1 0 0 1 - -
                  break;
               }
               break;
            case 0b101: // 1 0 1 | - - 1 0 0 - - -
               // FRSTOR = Restore State
               // ESCAPE 1 0 1 | MOD 1 0 0 R/M   | DISP
               break;
            case 0b111: // 1 1 1 | - - 1 0 0 - - -
               // FLD = LOAD
               // ESCAPE 1 1 1 | MOD 1 0 0 R/M   | DISP     BCD Memory to ST(0)
               break;
            }
            break;
         case 0b101: // - - 1 | - - 1 0 1 - - -
            switch (opcode) {
            case 0b001: // 0 0 1 | - - 1 0 1 - - -
               switch (ModRegRM) {
               case 0b11'101'000:
                  // FLD1 = LOAD +1.0 into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 0 0 0
                  break;
               case 0b11'101'001:
                  // FLDL2T = LOAD log2(10) into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 0 0 1
                  break;
               case 0b11'101'010:
                  // FLD2E = LOAD log2(e) into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 0 1 0
                  break;
               case 0b11'101'011:
                  // FLDPI = LOAD pi into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 0 1 1
                  break;
               case 0b11'101'100:
                  // FLDLG2 = LOAD log10(2) into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 1 0 0
                  break;
               case 0b11'101'101:
                  // FLDLN2 = LOAD loge(2) into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 1 0 1
                  break;
               case 0b11'101'110:
                  // FLDZ = LOAD +0.0 into ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 0 1 1 1 0
                  break;
               default: // 0 0 1 | - - 1 0 1 - - -
                  // FLDCW = Load Control Word
                  // ESCAPE 0 0 1 | MOD 1 0 1 R/M   | DISP
                  break;
               }
               break;
            case 0b011: // 0 1 1 | - - 1 0 1 - - -
               // FLD = LOAD
               // ESCAPE 0 1 1 | MOD 1 0 1 R/M   | DISP     Temporary Real Memory to ST(0)
               break;
            case 0b111: // 1 1 1 | - - 1 0 1 - - -
               // FLD = LOAD
               // ESCAPE 1 1 1 | MOD 1 0 1 R/M   | DISP     Long Integer Memory to ST(0)
               break;
            case 0b101: // 1 0 1 | - - 1 0 1 - - -
               // Unused
               // ESCAPE 1 0 1 | - - 1 0 1 - - -
               break;
            }
            break;
         case 0b110: // - - 1 | - - 1 1 0 - - -
            switch (opcode) {
            case 0b001: // 0 0 1 | - - 1 1 0 - - -
               switch (ModRegRM) {
               case 0b11'110'000:
                  // F2XM1 = 2^(ST(0)) - 1
                  // ESCAPE 0 0 1 | 1 1 1 1 0 0 0 0
                  break;
               case 0b11'110'001:
                  // FYL2X = ST(1) * Log2(|ST(0)|)
                  // ESCAPE 0 0 1 | 1 1 1 1 0 0 0 1
                  break;
               case 0b11'110'010:
                  // FPTAN = Partial Tangent of ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 1 0 0 1 0
                  break;
               case 0b11'110'011:
                  // FPATAN = Partial Arctangent of ST(0) / ST(1)
                  // ESCAPE 0 0 1 | 1 1 1 1 0 0 1 1
                  break;
               case 0b11'110'100:
                  // FXTRACT = Extract Components of ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 1 0 1 0 0
                  break;
               case 0b11'110'110:
                  // FDECSTP = Decrement Stack Pointer
                  // ESCAPE 0 0 1 | 1 1 1 1 0 1 1 0
                  break;
               case 0b11'110'111:
                  // FINCSTP = Increment Stack Pointer
                  // ESCAPE 0 0 1 | 1 1 1 1 0 1 1 1
                  break;
               default: // 0 0 1 | - - 1 1 0 - - -
                  // FSTENV = Store Environment
                  // ESCAPE 0 0 1 | MOD 1 1 0 R/M   | DISP
                  break;
               }
               break;
            case 0b101: // 1 0 1 | - - 1 1 0 - - -
               // FSAVE = Save State
               // ESCAPE 1 0 1 | MOD 1 1 0 R/M   | DISP
               break;
            case 0b111: // 1 1 1 | - - 1 1 0 - - -
               // FSTP = STORE AND POP
               // ESCAPE 1 1 1 | MOD 1 1 0 R/M   | DISP     ST(0) to BCD Memory
               break;
            case 0b011: // 0 1 1 | - - 1 1 0 - - -
               // Unused
               // ESCAPE 0 1 1 | - - 1 1 0 - - -
               break;
            }
            break;
         case 0b111: // - - 1 | - - 1 1 1 - - -
            switch (opcode) {
            case 0b001: // 0 0 1 | - - 1 1 1 - - -
               switch (ModRegRM) {
               case 0b11'111'000:
                  // FPREM = Partial Remainder of ST(0)+ST(1)
                  // ESCAPE 0 0 1 | 1 1 1 1 1 0 0 0
                  break;
               case 0b11'111'001:
                  // FYL2XP1 = ST(1) * Log2(ST(0)+1)
                  // ESCAPE 0 0 1 | 1 1 1 1 1 0 0 1
                  break;
               case 0b11'111'010:
                  // FSQRT = Square Root of ST(0)
                  // ESCAPE 0 0 1 | 1 1 1 1 1 0 1 0
                  break;
               case 0b11'111'100:
                  // FRNDINT = Rount ST(0) to Integer
                  // ESCAPE 0 0 1 | 1 1 1 1 1 1 0 0
                  break;
               case 0b11'111'101:
                  // FSCALE = Scale ST(0) by ST(1)
                  // ESCAPE 0 0 1 | 1 1 1 1 1 1 0 1
                  break;
               default: // 0 0 1 | - - 1 1 1 - - -
                  // FSTCW = Store Control Word
                  // ESCAPE 0 0 1 | MOD 1 1 1 R/M   | DISP
                  break;
               }
               break;
            case 0b011: // 0 1 1 | - - 1 1 1 - - -
               // FSTP = STORE AND POP
               // ESCAPE 0 1 1 | MOD 1 1 1 R/M   | DISP     ST(0) to Temporary Real Memory
               break;
            case 0b101: // 1 0 1 | - - 1 1 1 - - -
               // FSTSW = Store Status Word
               // ESCAPE 1 0 1 | MOD 1 1 1 R/M   | DISP
               break;
            case 0b111: // 1 1 1 | - - 1 1 1 - - -
               // FSTP = STORE AND POP
               // ESCAPE 1 1 1 | MOD 1 1 1 R/M   | DISP     ST(0) to Long Integer Memory
               break;
            }
            break;
         }
         break;
      }
   }
}