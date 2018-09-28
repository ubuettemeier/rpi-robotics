/*! ---------------------------------------------------------------------
 * @file    test_laser_sensor.c
 * @date    09.28.2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>

/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{	
  uint8_t state, last_state; 
  
  printf ("Exit program with Ctrl+C\n");
  
  if( wiringPiSetup() < 0) {
    printf ("wiringPiSetup failed !\n");
    return EXIT_FAILURE;
  } 
  
  pinMode (26, INPUT);                        /* laser sensor DOUT-Pin */
  // pullUpDnControl (26, PUD_UP);               /* pull-up resistor */   
  last_state = state = digitalRead (26);
  printf ("state = %i\n", state);
  
  while (1) {
    state = digitalRead (26);
    if (state != last_state) {
      last_state = state;
      printf ("state = %i\n", state);
    }
    usleep (1000);                           /* wait 1ms */
  }
  
  return EXIT_SUCCESS;
}
