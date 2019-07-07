#pragma once
#include <cstdint>

class Memory {
public:
   Memory();
   ~Memory();

   template<typename T> T getmem(int address) { return 0; }
   template<typename T> T getmem(int address, int offset) { return 0; }

   template<typename T> void putmem(int address, T value) {}
   template<typename T> void putmem(int address, int offset, T value) {}
private:
   uint8_t *mem;
};