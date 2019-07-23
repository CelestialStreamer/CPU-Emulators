#include "Common.h"
#include "Memory.h"
#include "State8086.h"

#include <iostream>


int main() {
   State8086 state(new Memory("Microsoft DOS 6.0 (3.5)/Full.img"), new IO);
   state.run(77);
}