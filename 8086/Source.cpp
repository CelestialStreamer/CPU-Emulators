#include "Common.h"
#include "Memory.h"
#include "I8086.h"

#include <iostream>
#include <sstream>
#include <bitset>


int main() {
   I8086 state(new Memory("Microsoft DOS 6.0 (3.5)/Full.img"), new IO);
   state.run(77);
}