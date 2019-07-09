#pragma once
#include <cstdint>

class Memory {
public:
   Memory();
   ~Memory();

   template<typename T> T& mem(int address) { return *(T*)(memory[address]); }
private:
   uint8_t *memory;
};
