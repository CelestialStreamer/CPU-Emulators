#include "State8080.h"
#include "IO.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <Windows.h>

State8080* state = new State8080;
IO io;

void VideoInterrupts()
{
   for (;;)
   {
      Sleep(16); // 1/60 second
      //state->generateInterrupt(0);
   }
}

void KeyPresses()
{
   for (;;)
   {
      io.Read1.player1Start = (GetAsyncKeyState(VK_RETURN) & 0x8000 ? 1 : 0);
      io.Read1.player1joystickLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000 ? 1 : 0);
      io.Read1.player1joystickRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000 ? 1 : 0);
      io.Read1.player1Shoot = (GetAsyncKeyState(VK_SPACE) & 0x8000 ? 1 : 0);


      io.Read1.player2Start = (GetAsyncKeyState('Z') & 0x8000 ? 1 : 0);
      io.Read2.player2joystickLeft = (GetAsyncKeyState('A') & 0x8000 ? 1 : 0);
      io.Read2.player2joystickRight = (GetAsyncKeyState('D') & 0x8000 ? 1 : 0);
      io.Read2.player2Shoot = (GetAsyncKeyState('S') & 0x8000 ? 1 : 0);

      if (GetAsyncKeyState('C') & 0x8000)
         io.Read1.coin = 1;
   }
}

void CPU_Cycles()
{
   int cycles = 0;
   for (;;)
   {
      int pc = state->Reg.pc;
      state->Disassemble8080Op();
      state->Emulate8080Op();
      state->display();
   }

   std::cout << std::endl;
}

void init(char** argv)
{
   std::ifstream file(argv[1], std::ios::binary);
   file.read((char*)state->memory, 0xffff);
   file.close();
}

int main(int argc, char** argv)
{
   if (argc != 2)
      return 0;

   init(argv);

   //std::thread videoInterrupts(VideoInterrupts);
   //std::thread keyPresses(KeyPresses);

   CPU_Cycles();

   //videoInterrupts.join();
   //keyPresses.join();

   return 0;
}