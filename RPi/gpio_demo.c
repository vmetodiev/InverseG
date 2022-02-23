//
// Compile with: gcc -Wall -o gpiodemo gpiodemo.c -lwiringPi
//

#include <wiringPi.h>
#include <stdio.h>

int main (void)
{
  wiringPiSetupGpio();

  pinMode (17, OUTPUT);
  pinMode (27, INPUT);
  
  int pinStatus = 0;
  
  for (;;)
  {
    digitalWrite (17, HIGH) ; delay (500);

    pinStatus = digitalRead(27);
    printf("\nGPIO27: %d", pinStatus);

    digitalWrite (17,  LOW) ; delay (500) ;

    pinStatus = digitalRead(27);
    printf("\nGPIO27: %d", pinStatus);
  }

  return 0 ;
}