#include "State8086.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <string>

enum RepeatType {
   None, Equal, NEqual
};

std::string intToHexSigned(int x, bool usePlus = false) {
   std::ostringstream stream;
   std::string sign = x < 0 ? "-0x" : (usePlus ? "+0x" : "0x");
   stream << sign << std::hex << x;
   return stream.str();
};
std::string intToHexUnsigned(int x) {
   std::ostringstream stream;
   stream << "0x" << std::hex << std::abs(x);
   return stream.str();
}

void State8086::disassembleOp() {
   enum OperationSize {
      BYTE, WORD
   };

   std::string rm, reg, seg;
   std::string UNSUPPORTED = "Unused Opcode";

   word saveIP = IP;
   word saveSegment = segment;
   std::ostringstream bytes;
   std::string lock = "";
   std::string prefix;

   RepeatType repeatType;
   byte opcode;

   auto imm8 = [&]() -> byte {
      byte temp = imm<byte>();
      bytes << std::setfill('0') << std::setw(2) << std::hex << (int)temp << " ";
      std::cout << bytes.str() << std::endl;
      return temp;
   };

   auto imm16 = [&]() -> word {
      union { word x; struct { byte h; byte l; }; } temp;
      temp.x = imm<word>();
      bytes << std::setfill('0') << std::setw(2) << std::hex << (int)temp.h << " ";
      bytes << std::setfill('0') << std::setw(2) << std::hex << (int)temp.l << " ";
      std::cout << bytes.str() << std::endl;
      return temp.x;
   };
   prefix = [&]() -> std::string {
      std::ostringstream stream;
      while (true)
         switch (opcode = imm8()) {
         case 0x26: segment = segRegs.ES; seg = "ES"; break;
         case 0x2E: segment = segRegs.CS; seg = "CS"; break;
         case 0x36: segment = segRegs.SS; seg = "SS"; break;
         case 0x3E: segment = segRegs.DS; seg = "DS"; break;
         case 0xf0: lock = "LOCK "; break;
         case 0xf2: repeatType = NEqual; stream << "REPNZ ";  break;
         case 0xf3: repeatType = Equal; stream << "REPZ"; break;
         default: return stream.str();
         }
   }();

   auto memRef = [&](std::string address, std::string defaultSegment = "ds") -> std::string {
      return (seg == "" ? defaultSegment : seg) + ":[" + address + "]";
   };
   auto memAdd = [&](std::string address, std::string defaultSegment = "ds") -> std::string {
      return (seg == "" ? defaultSegment : seg) + ":" + address;
   };

   auto print = [&](std::string mnemonic, std::string dest = "", std::string src = "") {
      std::cout << std::setw(16) << std::left << bytes.str();
      std::cout << lock << prefix << mnemonic << " " << dest << (src == "" ? "" : "," + src) << std::endl;
   };


   auto regString = [&](OperationSize size) -> std::string {
      switch (_reg) {
      case 0: return size == BYTE ? "AL" : "AX";
      case 1: return size == BYTE ? "CL" : "CX";
      case 2: return size == BYTE ? "DL" : "DX";
      case 3: return size == BYTE ? "BL" : "BX";
      case 4: return size == BYTE ? "AH" : "SP";
      case 5: return size == BYTE ? "CH" : "BP";
      case 6: return size == BYTE ? "DH" : "SI";
      case 7: return size == BYTE ? "BH" : "DI";
      }
      return "Bad Register";
   };

   auto rmString = [&](OperationSize size) -> std::string {
      std::ostringstream stream;
      switch (_mode) {
      case 0:
         switch (_rm) {
         case 0: stream << (seg == "" ? "" : seg + ":") << "[BX+SI]"; break;
         case 1: stream << (seg == "" ? "" : seg + ":") << "[BX+DI]"; break;
         case 2: stream << (seg == "" ? "ss" : seg) << ":[BP+SI]"; break;
         case 3: stream << (seg == "" ? "ss" : seg) << ":[BP+DI]"; break;
         case 4: stream << (seg == "" ? "" : seg + ":") << "[SI]"; break;
         case 5: stream << (seg == "" ? "" : seg + ":") << "[DI]"; break;
         case 6: stream << (seg == "" ? "ds" : seg) << ":" << intToHexSigned(imm16()); break;
         case 7: stream << (seg == "" ? "" : seg + ":") << "[BX]"; break;
         }
         return stream.str();
      case 1:
         switch (_rm) {
         case 0: stream << (seg == "" ? "" : seg + ":") << "[BX+SI]"; break;
         case 1: stream << (seg == "" ? "" : seg + ":") << "[BX+DI]"; break;
         case 2: stream << (seg == "" ? "ss" : seg) << ":[BP+SI]"; break;
         case 3: stream << (seg == "" ? "ss" : seg) << ":[BP+DI]"; break;
         case 4: stream << (seg == "" ? "" : seg + ":") << "[SI]"; break;
         case 5: stream << (seg == "" ? "" : seg + ":") << "[DI]"; break;
         case 6: stream << (seg == "" ? "" : seg + ":") << "[BP]"; break;
         case 7: stream << (seg == "" ? "" : seg + ":") << "[BX]"; break;
         }
         stream << intToHexSigned((int8_t)imm8(), true);
         return stream.str();
      case 2:
         switch (_rm) {
         case 0: stream << (seg == "" ? "" : seg + ":") << "[BX+SI]"; break;
         case 1: stream << (seg == "" ? "" : seg + ":") << "[BX+DI]"; break;
         case 2: stream << (seg == "" ? "ss" : seg) << ":[BP+SI]"; break;
         case 3: stream << (seg == "" ? "ss" : seg) << ":[BP+DI]"; break;
         case 4: stream << (seg == "" ? "" : seg + ":") << "[SI]"; break;
         case 5: stream << (seg == "" ? "" : seg + ":") << "[DI]"; break;
         case 6: stream << (seg == "" ? "" : seg + ":") << "[BP]"; break;
         case 7: stream << (seg == "" ? "" : seg + ":") << "[BX]"; break;
         }
         stream << intToHexSigned((int16_t)imm16(), true);
         return stream.str();
      case 3:
         switch (_rm) {
         case 0: stream << (size == BYTE ? "AL" : "AX"); break;
         case 1: stream << (size == BYTE ? "CL" : "CX"); break;
         case 2: stream << (size == BYTE ? "DL" : "DX"); break;
         case 3: stream << (size == BYTE ? "BL" : "BX"); break;
         case 4: stream << (size == BYTE ? "AH" : "SP"); break;
         case 5: stream << (size == BYTE ? "CH" : "BP"); break;
         case 6: stream << (size == BYTE ? "DH" : "SI"); break;
         case 7: stream << (size == BYTE ? "BH" : "DI"); break;
         }
      }
      return "Bad ModRegR/M byte";
   };



   switch (opcode) {
   case 0x88: fetchModRM(); print("MOV", rmString(BYTE), regString(BYTE)); break;
   case 0x89: fetchModRM(); print("MOV", rmString(WORD), regString(WORD)); break;
   case 0x8a: fetchModRM(); print("MOV", regString(BYTE), rmString(BYTE)); break;
   case 0x8b: fetchModRM(); print("MOV", regString(WORD), rmString(WORD)); break;
   case 0xc6: fetchModRM(); print("MOV", rmString(BYTE), intToHexUnsigned(imm8())); break;
   case 0xc7: fetchModRM(); print("MOV", rmString(WORD), intToHexUnsigned(imm16())); break;
   case 0xb0: print("MOV", "AL", intToHexUnsigned(imm8())); break;
   case 0xB1: print("MOV", "CL", intToHexUnsigned(imm8())); break;
   case 0xB2: print("MOV", "DL", intToHexUnsigned(imm8())); break;
   case 0xB3: print("MOV", "BL", intToHexUnsigned(imm8())); break;
   case 0xB4: print("MOV", "AH", intToHexUnsigned(imm8())); break;
   case 0xB5: print("MOV", "CH", intToHexUnsigned(imm8())); break;
   case 0xB6: print("MOV", "DH", intToHexUnsigned(imm8())); break;
   case 0xB7: print("MOV", "BH", intToHexUnsigned(imm8())); break;
   case 0xB8: print("MOV", "AX", intToHexUnsigned(imm16())); break;
   case 0xB9: print("MOV", "CX", intToHexUnsigned(imm16())); break;
   case 0xBA: print("MOV", "DX", intToHexUnsigned(imm16())); break;
   case 0xBB: print("MOV", "BX", intToHexUnsigned(imm16())); break;
   case 0xBC: print("MOV", "SP", intToHexUnsigned(imm16())); break;
   case 0xBD: print("MOV", "BP", intToHexUnsigned(imm16())); break;
   case 0xBE: print("MOV", "SI", intToHexUnsigned(imm16())); break;
   case 0xBF: print("MOV", "DI", intToHexUnsigned(imm16())); break;
   case 0xa0: print("MOV", "AL", memAdd(intToHexUnsigned(imm8()))); break;
   case 0xa1: print("MOV", "AX", memAdd(intToHexUnsigned(imm16()))); break;
   case 0xa2: print("MOV", memAdd(intToHexUnsigned(imm8()), "AL")); break;
   case 0xa3: print("MOV", memAdd(intToHexUnsigned(imm16()), "AX")); break;
   case 0x8e:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("MOV", "ES", rmString(WORD)); break;
      case 1: print("MOV", "CS", rmString(WORD)); break;
      case 2: print("MOV", "SS", rmString(WORD)); break;
      case 3: print("MOV", "DS", rmString(WORD)); break;
      } break;
   }
   case 0x8c:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("MOV", rmString(WORD), "ES"); break;
      case 1: print("MOV", rmString(WORD), "CS"); break;
      case 2: print("MOV", rmString(WORD), "SS"); break;
      case 3: print("MOV", rmString(WORD), "DS"); break;
      } break;
   }

   case 0x50: print("PUSH", "AX"); break;
   case 0x51: print("PUSH", "CX"); break;
   case 0x52: print("PUSH", "DX"); break;
   case 0x53: print("PUSH", "BX"); break;
   case 0x54: print("PUSH", "SP"); break;
   case 0x55: print("PUSH", "BP"); break;
   case 0x56: print("PUSH", "SI"); break;
   case 0x57: print("PUSH", "DI"); break;
   case 0x06: print("PUSH", "ES"); break;
   case 0x0E: print("PUSH", "CS"); break;
   case 0x16: print("PUSH", "SS"); break;
   case 0x1E: print("PUSH", "DS"); break;

   case 0x8f: fetchModRM(); print("POP", memAdd(rmString(WORD), "cs")); break;
   case 0x58: print("POP", "AX"); break;
   case 0x59: print("POP", "CX"); break;
   case 0x5A: print("POP", "DX"); break;
   case 0x5B: print("POP", "BX"); break;
   case 0x5C: print("POP", "SP"); break;
   case 0x5D: print("POP", "BP"); break;
   case 0x5E: print("POP", "SI"); break;
   case 0x5F: print("POP", "DI"); break;
   case 0x07: print("POP", "ES"); break;
   case 0x17: print("POP", "SS"); break;
   case 0x1F: print("POP", "DS"); break;

   case 0x86: fetchModRM(); print("XCHG", rmString(BYTE), regString(BYTE)); break;
   case 0x87: fetchModRM(); print("XCHG", rmString(WORD), regString(WORD)); break;
   case 0x90: print("NOP");
   case 0x91: print("XCHG", "AX", "CX"); break;
   case 0x92: print("XCHG", "AX", "DX"); break;
   case 0x93: print("XCHG", "AX", "BX"); break;
   case 0x94: print("XCHG", "AX", "SP"); break;
   case 0x95: print("XCHG", "AX", "BP"); break;
   case 0x96: print("XCHG", "AX", "SI"); break;
   case 0x97: print("XCHG", "AX", "DI"); break;

   case 0xE4: print("IN", "AL", intToHexUnsigned(imm8())); break;
   case 0xE5: print("IN", "AX", intToHexUnsigned(imm8())); break;
   case 0xEC: print("IN", "AL", "DX"); break; d_regs.a.l = io->read<byte>(d_regs.d.x); break;
   case 0xED: print("IN", "AX", "DX"); break; d_regs.a.x = io->read<word>(d_regs.d.x); break;

   case 0xE6: print("OUT", intToHexUnsigned(imm8()), "AL"); break;
   case 0xE7: print("OUT", intToHexUnsigned(imm8()), "AX"); break;
   case 0xEe: print("OUT", "DX", "AL"); break;
   case 0xEf: print("OUT", "DX", "AX"); break;

   case 0xd7: print("XLAT", memRef("BX+AL")); break;

   case 0x8d: print("LEA", regString(WORD), memRef(rmString(WORD))); break;

   case 0xc4: fetchModRM(); print("LES", regString(WORD), memRef(rmString(WORD))); break;
   case 0xc5: fetchModRM(); print("LDS", regString(WORD), memRef(rmString(WORD))); break;

   case 0x9c: print("PUSHF"); break;
   case 0x9D: print("POPF"); break;
   case 0x9E: print("SAHF"); break;
   case 0x9F: print("LAHF"); break;

   case 0x00: fetchModRM(); print("ADD", rmString(BYTE), regString(BYTE)); break;
   case 0x08: fetchModRM(); print("OR", rmString(BYTE), regString(BYTE)); break;
   case 0x10: fetchModRM(); print("ADC", rmString(BYTE), regString(BYTE)); break;
   case 0x18: fetchModRM(); print("SBB", rmString(BYTE), regString(BYTE)); break;
   case 0x20: fetchModRM(); print("AND", rmString(BYTE), regString(BYTE)); break;
   case 0x28: fetchModRM(); print("SUB", rmString(BYTE), regString(BYTE)); break;
   case 0x30: fetchModRM(); print("XOR", rmString(BYTE), regString(BYTE)); break;
   case 0x38: fetchModRM(); print("CMP", rmString(BYTE), regString(BYTE)); break;
   case 0x01: fetchModRM(); print("ADD", rmString(WORD), regString(WORD)); break;
   case 0x09: fetchModRM(); print("OR", rmString(WORD), regString(WORD)); break;
   case 0x11: fetchModRM(); print("ADC", rmString(WORD), regString(WORD)); break;
   case 0x19: fetchModRM(); print("SBB", rmString(WORD), regString(WORD)); break;
   case 0x21: fetchModRM(); print("AND", rmString(WORD), regString(WORD)); break;
   case 0x29: fetchModRM(); print("SUB", rmString(WORD), regString(WORD)); break;
   case 0x31: fetchModRM(); print("XOR", rmString(WORD), regString(WORD)); break;
   case 0x39: fetchModRM(); print("CMP", rmString(WORD), regString(WORD)); break;

   case 0x02: fetchModRM(); print("ADD", regString(BYTE), rmString(BYTE)); break;
   case 0x0A: fetchModRM(); print("OR", regString(BYTE), rmString(BYTE)); break;
   case 0x12: fetchModRM(); print("ADC", regString(BYTE), rmString(BYTE)); break;
   case 0x1A: fetchModRM(); print("SBB", regString(BYTE), rmString(BYTE)); break;
   case 0x22: fetchModRM(); print("AND", regString(BYTE), rmString(BYTE)); break;
   case 0x2A: fetchModRM(); print("SUB", regString(BYTE), rmString(BYTE)); break;
   case 0x32: fetchModRM(); print("XOR", regString(BYTE), rmString(BYTE)); break;
   case 0x3A: fetchModRM(); print("CMP", regString(BYTE), rmString(BYTE)); break;

   case 0x03: fetchModRM(); print("ADD", regString(WORD), rmString(WORD)); break;
   case 0x0B: fetchModRM(); print("OR", regString(WORD), rmString(WORD)); break;
   case 0x13: fetchModRM(); print("ADC", regString(WORD), rmString(WORD)); break;
   case 0x1B: fetchModRM(); print("SBB", regString(WORD), rmString(WORD)); break;
   case 0x23: fetchModRM(); print("AND", regString(WORD), rmString(WORD)); break;
   case 0x2B: fetchModRM(); print("SUB", regString(WORD), rmString(WORD)); break;
   case 0x33: fetchModRM(); print("XOR", regString(WORD), rmString(WORD)); break;
   case 0x3B: fetchModRM(); print("CMP", regString(WORD), rmString(WORD)); break;

   case 0x04: fetchModRM(); print("ADD", "AL", intToHexUnsigned(imm8())); break;
   case 0x0C: fetchModRM(); print("OR", "AL", intToHexUnsigned(imm8())); break;
   case 0x14: fetchModRM(); print("ADC", "AL", intToHexUnsigned(imm8())); break;
   case 0x1C: fetchModRM(); print("SBB", "AL", intToHexUnsigned(imm8())); break;
   case 0x24: fetchModRM(); print("AND", "AL", intToHexUnsigned(imm8())); break;
   case 0x2C: fetchModRM(); print("SUB", "AL", intToHexUnsigned(imm8())); break;
   case 0x34: fetchModRM(); print("XOR", "AL", intToHexUnsigned(imm8())); break;
   case 0x3C: fetchModRM(); print("CMP", "AL", intToHexUnsigned(imm8())); break;

   case 0x05: fetchModRM(); print("ADD", "AX", intToHexUnsigned(imm16())); break;
   case 0x0D: fetchModRM(); print("OR", "AX", intToHexUnsigned(imm16())); break;
   case 0x15: fetchModRM(); print("ADC", "AX", intToHexUnsigned(imm16())); break;
   case 0x1D: fetchModRM(); print("SBB", "AX", intToHexUnsigned(imm16())); break;
   case 0x25: fetchModRM(); print("AND", "AX", intToHexUnsigned(imm16())); break;
   case 0x2D: fetchModRM(); print("SUB", "AX", intToHexUnsigned(imm16())); break;
   case 0x35: fetchModRM(); print("XOR", "AX", intToHexUnsigned(imm16())); break;
   case 0x3D: fetchModRM(); print("CMP", "AX", intToHexUnsigned(imm16())); break;

   case 0x80:
   {
      fetchModRM();
      switch (ext) {
      case 1: print("ADD", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 0: print("OR", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 2: print("ADC", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 3: print("SBB", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 4: print("AND", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 5: print("SUB", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 6: print("XOR", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 7: print("CMP", rmString(BYTE), intToHexUnsigned(imm8())); break;
      }
      break;
   }
   case 0x82:
   {
      fetchModRM();
      switch (ext) {
      case 1: print("ADD", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 0: print("UNUSED"); break;
      case 2: print("ADC", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 3: print("SBB", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 4: print("UNUSED"); break;
      case 5: print("SUB", rmString(BYTE), intToHexUnsigned(imm8())); break;
      case 6: print("UNUSED"); break;
      case 7: print("CMP", rmString(BYTE), intToHexUnsigned(imm8())); break;
      }
      break;
   }
   case 0x81:
   {
      fetchModRM();
      switch (ext) {
      case 1: print("ADD", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 0: print("OR", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 2: print("ADC", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 3: print("SBB", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 4: print("AND", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 5: print("SUB", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 6: print("XOR", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 7: print("CMP", rmString(WORD), intToHexUnsigned(imm16())); break;
      }
      break;
   }
   case 0x83:
   {
      fetchModRM();
      switch (ext) {
      case 1: print("ADD", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 0: print(UNSUPPORTED); break;
      case 2: print("ADC", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 3: print("SBB", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 4: print(UNSUPPORTED); break;
      case 5: print("SUB", rmString(WORD), intToHexUnsigned(imm16())); break;
      case 6: print(UNSUPPORTED); break;
      case 7: print("CMP", rmString(WORD), intToHexUnsigned(imm16())); break;
      }
      break;
   }
   case 0xFF:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("INC", rmString(WORD)); break;
      case 1: print("DEC", rmString(WORD)); break;
      case 2: print("CALL", rmString(WORD)); break;
      case 3: print("CALL", rmString(WORD)); break;
      case 4: print("JMP", rmString(WORD)); break;
      case 5: print("JMP", rmString(WORD)); break;
      case 6: print("PUSH", rmString(WORD)); break;
      default: print(UNSUPPORTED);
      }
      break;
   }
   case 0xFE:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("INC", rmString(BYTE)); break;
      case 1: print("DEC", rmString(BYTE)); break;
      default: print(UNSUPPORTED);
      }
      break;
   }
   case 0x40: print("INC", "AX"); break;
   case 0x41: print("INC", "CX"); break;
   case 0x42: print("INC", "DX"); break;
   case 0x43: print("INC", "BX"); break;
   case 0x44: print("INC", "SP"); break;
   case 0x45: print("INC", "BP"); break;
   case 0x46: print("INC", "SI"); break;
   case 0x47: print("INC", "DI"); break;

   case 0x37: print("AAA"); break;
   case 0x27: print("DAA"); break;

   case 0x48: print("DEC", "AX"); break;
   case 0x49: print("DEC", "CX"); break;
   case 0x4A: print("DEC", "DX"); break;
   case 0x4B: print("DEC", "BX"); break;
   case 0x4C: print("DEC", "SP"); break;
   case 0x4D: print("DEC", "BP"); break;
   case 0x4E: print("DEC", "SI"); break;
   case 0x4F: print("DEC", "DI"); break;

   case 0xF6:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("TEST", rmString(BYTE)); break;
      case 2: print("NOT", rmString(BYTE)); break;
      case 3: print("NEG", rmString(BYTE)); break;
      case 4: print("MUL", rmString(BYTE)); break;
      case 5: print("IMUL", rmString(BYTE)); break;
      case 6: print("DIV", rmString(BYTE)); break;
      case 7: print("IDIV", rmString(BYTE)); break;
      default: print(UNSUPPORTED); break;
      }
      break;
   }
   case 0xF7:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("TEST", rmString(WORD)); break;
      case 2: print("NOT", rmString(WORD)); break;
      case 3: print("NEG", rmString(WORD)); break;
      case 4: print("MUL", rmString(WORD)); break;
      case 5: print("IMUL", rmString(WORD)); break;
      case 6: print("DIV", rmString(WORD)); break;
      case 7: print("IDIV", rmString(WORD)); break;
      case 1: print(UNSUPPORTED);
      }
      break;
   }

   case 0x3F: print("AAS"); break;
   case 0x2F: print("DAS"); break;
   case 0xD4: print("AAM"); break;
   case 0xD5: print("AAD"); break;
   case 0x98: print("CBW"); break;
   case 0x99: print("CWD"); break;

   case 0xD0:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("ROL", rmString(BYTE), "1"); break;
      case 1: print("ROR", rmString(BYTE), "1"); break;
      case 2: print("RCL", rmString(BYTE), "1"); break;
      case 3: print("RCR", rmString(BYTE), "1"); break;
      case 4: print("SHL", rmString(BYTE), "1"); break;
      case 5: print("SHR", rmString(BYTE), "1"); break;
      case 7: print("SAR", rmString(BYTE), "1"); break;
      default: print(UNSUPPORTED);
      }
      break;
   }
   case 0xD1:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("ROL", rmString(WORD), "1"); break;
      case 1: print("ROR", rmString(WORD), "1"); break;
      case 2: print("RCL", rmString(WORD), "1"); break;
      case 3: print("RCR", rmString(WORD), "1"); break;
      case 4: print("SHL", rmString(WORD), "1"); break;
      case 5: print("SHR", rmString(WORD), "1"); break;
      case 7: print("SAR", rmString(WORD), "1"); break;
      default: print(UNSUPPORTED);
      }
      break;
   }
   case 0xD2:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("ROL", rmString(BYTE), "CL"); break;
      case 1: print("ROR", rmString(BYTE), "CL"); break;
      case 2: print("RCL", rmString(BYTE), "CL"); break;
      case 3: print("RCR", rmString(BYTE), "CL"); break;
      case 4: print("SHL", rmString(BYTE), "CL"); break;
      case 5: print("SHR", rmString(BYTE), "CL"); break;
      case 7: print("SAR", rmString(BYTE), "CL"); break;
      default: print(UNSUPPORTED);
      }
      break;
   }
   case 0xD3:
   {
      fetchModRM();
      switch (ext) {
      case 0: print("ROL", rmString(WORD), "CL"); break;
      case 1: print("ROR", rmString(WORD), "CL"); break;
      case 2: print("RCL", rmString(WORD), "CL"); break;
      case 3: print("RCR", rmString(WORD), "CL"); break;
      case 4: print("SHL", rmString(WORD), "CL"); break;
      case 5: print("SHR", rmString(WORD), "CL"); break;
      case 7: print("SAR", rmString(WORD), "CL"); break;
      default: print(UNSUPPORTED);
      }
      break;
   }

   case 0x84: fetchModRM(); print("TEST", rmString(BYTE), regString(BYTE)); break;
   case 0x85: fetchModRM(); print("TEST", rmString(WORD), regString(WORD)); break;

   case 0xA8: print("TEST", "AL", intToHexUnsigned(imm8())); break;
   case 0xA9: print("TEST", "AX", intToHexUnsigned(imm16())); break;

   case 0xA4: print("MOVS", "BYTE ES:[DI]", "BYTE " + memAdd("SI", "DS")); break;
   case 0xA5: print("MOVS", "WORD ES:[DI]", "WORD " + memAdd("SI", "DS")); break;

   case 0xA6: print("CMPS", "BYTE ES:[DI]", "BYTE " + memAdd("SI", "DS")); break;
   case 0xA7: print("CMPS", "WORD ES:[DI]", "WORD " + memAdd("SI", "DS")); break;

   case 0xAE: print("SCAS", "AL", "BYTE " + memAdd("SI", "DS")); break;
   case 0xAF: print("SCAS", "AX", "WORD " + memAdd("SI", "DS")); break;

   case 0xAC: print("LODS", "AL", "BYTE " + memAdd("SI", "DS")); break;
   case 0xAD: print("LODS", "AX", "WORD " + memAdd("SI", "DS")); break;

   case 0xAA: print("STOS", "AL", "BYTE " + memAdd("SI", "DS")); break;
   case 0xAB: print("STOS", "AX", "WORD " + memAdd("SI", "DS")); break;


   case 0xE8: print("CALL", intToHexUnsigned(imm16() + 5)); break;
   case 0x9A: print("CALL", intToHexUnsigned(imm16()) + ":[" + intToHexUnsigned(imm16()) + "]"); break;
   case 0xE9: print("JMP", intToHexUnsigned(imm16())); break;
   case 0xEB: print("JMP", intToHexUnsigned(imm8())); break;
   case 0xEA: print("JMP", intToHexUnsigned(imm16()) + ":[" + intToHexUnsigned(imm16()) + "]"); break;
   case 0xC3: print("RET"); break;
   case 0xC2: print("RET", intToHexUnsigned(imm16())); break;
   case 0xCB: print("RETF"); break;
   case 0xCA: print("RETF", intToHexUnsigned(imm16())); break;

   case 0x70: print("JO"); break;
   case 0x71: print("JNO"); break;
   case 0x72: print("JB"); break;
   case 0x73: print("JNB"); break;
   case 0x74: print("JZ"); break;
   case 0x75: print("JNE"); break;
   case 0x76: print("JBE"); break;
   case 0x77: print("JA"); break;
   case 0x78: print("JS"); break;
   case 0x79: print("JNS"); break;
   case 0x7A: print("JP"); break;
   case 0x7B: print("JNP"); break;
   case 0x7C: print("JL"); break;
   case 0x7D: print("JGE"); break;
   case 0x7E: print("JLE"); break;
   case 0x7F: print("JNLE"); break;

   case 0xE2: print("LOOP", intToHexUnsigned(imm8())); break;
   case 0xE1: print("LOOPE", intToHexUnsigned(imm8())); break;
   case 0xE0: print("LOOPNE", intToHexUnsigned(imm8())); break;

   case 0xE3: print("JCXZ", intToHexUnsigned(imm8())); break;

   case 0xCD: print("INT", intToHexUnsigned(imm8())); break;
   case 0xCC: print("INT", "3"); break;
   case 0xCE: print("INTO"); break;
   case 0xCF: print("IRET"); break;

   case 0xF8: print("CLC"); break;
   case 0xF5: print("CMC"); break;
   case 0xF9: print("STC"); break;
   case 0xFC: print("CLD"); break;
   case 0xFD: print("STD"); break;
   case 0xFA: print("CLI"); break;
   case 0xFB: print("STI"); break;
   case 0xF4: print("HLT"); break;

   case 0x9B: print("WAIT"); break;

   case 0xD8: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xD9: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xDA: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xDB: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xDC: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xDD: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xDE: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;
   case 0xDF: fetchModRM(); print("ESCAPE TO COPROSSESOR"); break;

   case 0x6C: print("INS", "BYTE", memRef(intToHexUnsigned(imm8()), "ds")); break;
   case 0x6D: print("INS", "WORD", memRef(intToHexUnsigned(imm16()), "ds")); break;

   default: print(UNSUPPORTED); break;
   }

   segment = saveSegment;
   IP = saveIP;
}