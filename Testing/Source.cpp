#include <thread>
#include <iostream>
#include <ctime>

#include <Windows.h>

bool interrupt;
bool halt;

void loop()
{
   while (!halt)
   {
      if (interrupt)
      {
         interrupt = false;
         std::cout << "Interrupted" << std::endl;
      }
      else
         std::cout << "Not interrupted" << std::endl;
      Sleep(100);
   }
}

void foo()
{
   while (!halt)
   {
      if (GetAsyncKeyState('I') & 0x8000)
         interrupt = true;
      else if (GetAsyncKeyState('H') & 0x8000)
         halt = true;
   }
}

int main(int argc, char** argv)
{
   std::thread t1(loop);
   std::thread t2(foo);

   t1.join();
   t2.join();

   return 0;
}