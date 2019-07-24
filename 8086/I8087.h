#pragma once
#include "Common.h"

class I8087 {
public:
   I8087();
   ~I8087();
   void run(byte opcode, byte ModRegRM);
};

