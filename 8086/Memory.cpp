#include "Memory.h"



Memory::Memory()
{
   mem = new uint8_t[0x10'0000]; // Megabyte of memory
}


Memory::~Memory()
{
   delete mem;
}
