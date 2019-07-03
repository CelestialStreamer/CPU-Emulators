#pragma once
#include <cstdint>
class IO {
public:
   IO();
   ~IO();

   template<typename T> T read(uint16_t port) { return 0; }
   template<typename T> void write(uint16_t port, T value) {}
};

