#include "State8080.h"
#include "IO.h"
#include <iostream>
#include <fstream>



int main(int argc, char** argv)
{
   if (argc != 2)
      return 0;

   State8080* state = new State8080;

   std::ifstream file(argv[1], std::ios::binary);
   file.read((char*)state->memory, 0xffff);
   file.close();

   IO io;

   state->in = io.read;
   state->out = io.write;

   for (;;)
   {
      std::cout << std::endl;
      state->Disassemble8080Op();
      state->Emulate8080Op();
      state->display();
   }

   std::cout << std::endl;

   return 0;
}