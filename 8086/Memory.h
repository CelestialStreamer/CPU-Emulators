#pragma once
#include "Common.h"
#include <string>

class Memory {
public:
   Memory(std::string file);
   ~Memory();

   void memDump(const char* file);

   template<typename T> T& mem(int address) {
      return *(T*)(&memory[address]);
   }
private:
   byte *memory;
};
