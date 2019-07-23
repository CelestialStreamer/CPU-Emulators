#pragma once
#include <cstdint>

class IO {
public:
   uint8_t read(uint8_t port);
   void write(uint8_t port, uint8_t value);

   struct Read1 {
      uint8_t coin : 1; // Coin (0 when active)   
      uint8_t player2Start : 1;
      uint8_t player1Start : 1;
      uint8_t fill1 : 1; // ?
      uint8_t player1Shoot : 1;
      uint8_t player1joystickLeft : 1;
      uint8_t player1joystickRight : 1;
      uint8_t fill2 : 1; // ?
   } Read1;
   struct Read2 {
      uint8_t lives : 2; // Dipswitch number of lives (0:3,1:4,2:5,3:6)
      uint8_t tilt : 1; // Tilt 'button'
      uint8_t bonusLife : 1; // Dipswitch bonus life at 1:1000,0:1500    
      uint8_t player2Shoot : 1;
      uint8_t player2joystickLeft : 1;
      uint8_t player2joystickRight : 1;
      uint8_t coinInfo : 1; // Dipswitch coin info 1:off,0:on  
   } Read2;

private:
   uint8_t shift_offset;

   uint8_t shift0;
   uint8_t shift1;
};