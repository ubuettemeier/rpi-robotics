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
    m1 = new_mot (25, 23, 24, 400);              /* GPIO_ENABLE PIN, GPIO_DIR PIN, GPIO_STEP PIN, steps_per_turn */
    mot_setparam (m1, MOT_CW, 400, 20.0, 40.0);  /* 400 steps, speed up=20 s⁻2, speed down=40 s⁻2 */    
    mot_start (m1);
    
    while (m1->flag.aktiv) sleep(1);        
    
    mot_disenable (m1);
    kill_all_mot ();                    
    thread_state.kill = 1;
    while (!thread_state.run); 
    
    return 0;
}
