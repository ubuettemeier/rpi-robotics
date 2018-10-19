/*! ---------------------------------------------------------------------
 * @file    test_driver_A4988.c
 * @date    10.12.2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include "driver_A4988.h"
#include "../../../sensor/ultrasonic-HC-SR04/source/keypressed.h"

#define ENABLE_PIN_M1 25     /* GPIO.25  PIN 37 */
#define STEP_PIN_M1   24     /* GPIO.24  PIN 35 */
#define DIR_PIN_M1    23     /* GPIO.23  PIN 33 */

#define ENABLE_PIN_M2 29     /* GPIO.29  PIN 40 */
#define STEP_PIN_M2   28     /* GPIO.28  PIN 38 */
#define DIR_PIN_M2    27     /* GPIO.27  PIN 36 */

#define STEPS_PER_TURN 400

struct _mot_ctl_ *m1 = NULL, *m2 = NULL;
/*! --------------------------------------------------------------------
 * 
 */
static void help()
{
    printf ("\n");
    printf ("h = this message\n");
    printf ("ESC = Exit\n");  
    printf ("1 = start m1 CW 400 steps\n");  
    printf ("2 = start m1 CCW 400 steps\n"); 
    printf ("8 = new speed\n"); 
    printf ("9 = Motor disenable\n\n");  
    printf ("e = start m1 CW endless steps\n");  
    printf ("s = Motor STOP\n");  
}
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{
    char c;
    int taste = 0; 
    uint8_t ende = 0;   
    int sn = 0;
    double speed_rpm[3] = {150.0, 53.5, 200.0};
    
    init_mot_ctl ();    
    sleep (1);
    m1 = new_mot (ENABLE_PIN_M1, DIR_PIN_M1, STEP_PIN_M1, STEPS_PER_TURN);
    m2 = new_mot (ENABLE_PIN_M2, DIR_PIN_M2, STEP_PIN_M2, STEPS_PER_TURN);
    mot_set_rpm (m1, 100.0);

    help();
    init_check_keypressed();                           /* init key-touch control */

    while (!ende) {        
        if ((taste = check_keypressed(&c)) > 0) {       /* look for keypressed */        
            switch ( c ) {
                case 27:                                /* quit by ESC */                
                    kill_all_mot ();                    /* make motors disenabled */
                    thread_state.kill = 1;
                    while (!thread_state.run); 
                    ende = 1;
                    break;
                case 'h':
                    help();
                    break;
                case '1':
                case '2':                    
                    mot_setparam (m1, (c == '1') ? MOT_CW : MOT_CCW, 400);
                    mot_start (m1);
                    break;   
                case '8':
                    sn = (sn < 2) ? sn+1 : 0;
                    mot_set_rpm (m1, speed_rpm[sn]);                                      
                    break;
                case '9':
                    mot_disenable (m1);
                    break;
                case 'e':
                    mot_setparam (m1, MOT_CW, 0);
                    mot_set_Hz (m1, 2.5);
                    mot_start (m1);
                    break;                   
                case 's':
                    mot_stop (m1);
                    break;
            }
        }           
        usleep (1000);                                /* wait 1ms */
    }
    
    destroy_check_keypressed();                       /* destroy key-touch control */
    return EXIT_SUCCESS;
}
