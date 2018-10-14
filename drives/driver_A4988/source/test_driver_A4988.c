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

struct _mot_ctl_ *m1 = NULL, *m2 = NULL;
/*! --------------------------------------------------------------------
 * 
 */
static void help()
{
    printf ("\n");
    printf ("h = this message\n");
    printf ("ESC = Exit\n");  
    printf ("1 = start m1 CW\n");  
    printf ("2 = start m1 CCW\n");  
    printf ("9 = Motor disenable\n");    
}
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{
    char c;
    int taste = 0; 
    uint8_t ende = 0;   
    uint8_t para = 0;
    
    init_mot_ctl ();
    sleep (1);
    m1 = new_mot (ENABLE_PIN_M1, DIR_PIN_M1, STEP_PIN_M1);
    m2 = new_mot (ENABLE_PIN_M2, DIR_PIN_M2, STEP_PIN_M2);

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
                    para = (c == '1') ? MOT_CW : MOT_CCW;
                    mot_setparam (m1, para, 400);
                    mot_start (m1);
                    break;                
                case '9':
                    mot_disenable (m1);
                    break;
            }
        }           
        usleep (1000);                                /* wait 1ms */
    }
    
    destroy_check_keypressed();                       /* destroy key-touch control */
    return EXIT_SUCCESS;
}
