#pragma once
#include "Common.h"

class Memory {
public:
   Memory();
   ~Memory();

   template<typename T> T& mem(int address) { return *(T*)(memory[address]); }
private:
   byte *memory;
};
