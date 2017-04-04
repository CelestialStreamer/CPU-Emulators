#include "State8080.h"

#include <iostream>
#include <fstream>

int main(int argc, char** argv)
{
   if (argc != 2)
      return 0;

   State8080* state = new State8080;
   State8080::RReg* Reg = &state->Reg;

   std::ifstream file(argv[1], std::ios::binary);

   file.read((char*)state->memory, 0xffff);

   //int pc = 0;
   //while (pc < 1453)
   //{
   //   pc = pc + Disassemble8080Op(state->memory, pc);
   //   std::cout << std::endl;
   //}

   //state->memory[0x00] = 0xc3;
   //state->memory[0x01] = 0x00;
   //state->memory[0x02] = 0x01;

   //state->memory[0x170] = 0x07;

   //uint8_t program[] =
   //{
   //   0x3e, 0x01, // MVI A, $1
   //   0x06, 0x02, // MVI Reg.b, $2
   //   0x80        // ADD Reg.b
   //};

   //for (int opcode = 0; opcode < sizeof(program); opcode++)
   //   state->memory[opcode] = program[opcode];

   // Made it to 0x0588

   for (;;)
   {
      std::cout << std::endl;
      state->Emulate8080Op();
      state->display();
   }

   std::cout << std::endl;

   return 0;
}