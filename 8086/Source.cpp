#include <cstdint>
#include <iostream>

#include "State8086.h"
#include "Memory.h"

int main() {
   State8086 state(new Memory, new IO);
}