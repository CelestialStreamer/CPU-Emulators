#include "Memory.h"



Memory::Memory()
{
   memory = new uint8_t[0x10'0000]; // Megabyte of memory
}


Memory::~Memory()
{
   delete memory;
}
