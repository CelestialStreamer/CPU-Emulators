#include "Common.h"
#include "Memory.h"
#include "State8086.h"

#include <iostream>


int main() {
   State8086 state(new Memory, new IO);
}