/*! ---------------------------------------------------------------------
 * @file    rpi_tools.c
 * @date    10-28-2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include "rpi_tools.h"

/*! --------------------------------------------------------------------
 *  @brief    print error text and then programm exit.
 */
void abort_by_error (const char *error_text, const char *func_name)
{
    char buf[1024];
  
    sprintf (buf, "%s <%s>", error_text, func_name);
    perror (buf);
    abort();
}
/*!	--------------------------------------------------------------------
 * @brief   calculates time difference in us
 * @return  stop - start
 */
int64_t difference_micro (struct timeval *start, struct timeval *stop)
{
  return ((int64_t) stop->tv_sec * 1000000ll +
          (int64_t) stop->tv_usec) -	       
          ((int64_t) start->tv_sec * 1000000ll +
           (int64_t) start->tv_usec);
}
/*! --------------------------------------------------------------------
 * 
 */
int64_t current_difference_micro (struct timeval *start)
{
    struct timeval stop;
    
    gettimeofday (&stop, NULL);
    return difference_micro (start, &stop);
}
/*! --------------------------------------------------------------------
 * @brief   function displays progress bar
 * @param   t = sleep time [us]
 *           refresh_time [us]
 */
void show_usleep (uint64_t t, uint64_t refresh_time)
{
    uint64_t summe = 0;
    uint64_t delta = refresh_time;
    
    if (t == 0) 
        return;
        
    if (t < delta) delta = t-1;
    while (summe < t) {        
        usleep (delta);
        summe += delta;
        printf (".");        
        fflush(stdout);
    }
    printf ("\n");    
}
