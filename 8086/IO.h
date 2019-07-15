#pragma once
#include "Common.h"

class IO {
public:
   IO();
   ~IO();

   template<typename T> T read(word port) { return 0; }
   template<typename T> void write(word port, T value) {}
};

