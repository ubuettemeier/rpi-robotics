/*! ---------------------------------------------------------------------
 * @file    rpi_tools.c
 * @date    10-28-2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "rpi_tools.h"

/*! --------------------------------------------------------------------
 * @brief   function displays progress bar
 */
void show_usleep (uint64_t t)
{
    uint64_t summe = 0;
    uint64_t delta = 100000;    /* 0,1 s */
    
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
