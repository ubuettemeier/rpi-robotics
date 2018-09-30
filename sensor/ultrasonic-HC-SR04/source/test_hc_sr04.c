/*! ---------------------------------------------------------------------
 * @file    test_hc_sr04.c
 * @date    09.16.2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>

#include "keypressed.h"
#include "hc_sr04.h"

#define MAX_SENSOR 2
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{	
  uint8_t ende = 0;
  int res;              /* check_keypressed return */
  char c;               /* key of kepressed */  
  struct _hc_sr04_ *sen;

  init_check_keypressed();                    /* init key-touch control */
  printf ("quit with ESC\nplease touch the sensor\n");
  
  sen = new_hc_sr04 (0, 1);      /* new sensor trig=0, echo=1 */  
  sen = new_hc_sr04 (21, 22);    /* new sensor trig=21, echo=22 */
  start_hc_04_thread();          /* start measurement */
  
  while (!ende) {
    if ((res = check_keypressed(&c)) > 0) {
      if (c == 27) ende = 1;                  /* Exit with ESC */
      if (c == 's') stop_hc_04_thread ();
      if (c == 'g') start_hc_04_thread ();
    }
        
    sen = first_hc_sr04;
    while (sen != NULL) {
      pthread_mutex_lock(&hc_sr04_mutex);       /* enter critical section */        
      if (sen->hc_sr04_run_time != sen->last_hc_sr04_run_time) {
        sen->last_hc_sr04_run_time = sen->hc_sr04_run_time;
        printf ("Sensor[%i] %u us  %u mm     \n", sen->id, sen->hc_sr04_run_time, sen->hc_sr04_dist_mm);
      }
      pthread_mutex_unlock(&hc_sr04_mutex);     /* leave critical section */      
      sen = sen->next;
    }
            
    usleep (1000);                            /* wait 1 ms */
  }
  stop_hc_04_thread ();                       /* stop measurement */
  delete_all_hc_sr04 ();                      /* delete sensor list */
  destroy_check_keypressed();
  return EXIT_SUCCESS;
}
