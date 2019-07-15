#include "Memory.h"


Memory::Memory()
{
   memory = new byte[0x10'0000]; // Megabyte of memory
}


Memory::~Memory()
{
   delete memory;
}
