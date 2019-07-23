#include "Memory.h"
#include <fstream>
#include <iostream>
#include <iomanip>

Memory::Memory(std::string file) {
   memory = new byte[0x10'0000]; // Megabyte of memory
   std::ifstream stream(file, std::ios::binary);
   stream.read((char*)memory, 0xf'ffff);
   stream.close();

   int i = 0;
   for (int j = 0; j < 8; j++) {
      for (int k = 0; k < 8; k++) {
         std::cout << std::setw(2) << std::hex << (int)memory[i] << " ";
         i++;
      }
      std::cout << std::endl;
   }
}

Memory::~Memory() {
   delete memory;
}

void Memory::memDump(const char* file) {
   std::ofstream stream(file, std::ios::binary);

   int length = 0xf'ffff;
   while (memory[length] == 0 && length >= 0)
      length--;

   stream.write((char*)memory, length);
   stream.close();
}