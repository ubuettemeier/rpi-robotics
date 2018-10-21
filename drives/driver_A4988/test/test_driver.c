/*! --------------------------------------------------------------------
 * @file    test_driver.c
 * @date    10-21-2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <unistd.h>
#include "../source/driver_A4988.h"

int main() {
    struct _mot_ctl_ *m1 = NULL; 
    
    init_mot_ctl ();    
    sleep (1);
    m1 = new_mot (25, 23, 24, 400);
    mot_setparam (m1, MOT_CW, 400, 20.0, 40.0);  /* 400 steps, speed up=20 s⁻2, speed down=40 s⁻2 */
    mot_start (m1);
    while (m1->flag.aktiv) sleep(1);        
    mot_disenable (m1);
    return (0);
}
