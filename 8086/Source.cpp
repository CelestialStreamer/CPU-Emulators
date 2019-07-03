#include <cstdint>
#include <iostream>

#include "alu.h"
#include "State8086.h"
#include "Memory.h"

template<typename T>
T f1() { return 0; }
template<typename T>
T f2() { return 1; }

template<typename T>
struct foo {
   T(*operator[](int i))() {
      return a[i];
   }
private:
   T(*a[2])() = {
      f1, f2
   };
};

int main() {
   foo<int>()[0]();
   foo<int>()[1]();
}