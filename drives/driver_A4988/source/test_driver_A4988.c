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

#include "../../../sensor/ultrasonic-HC-SR04/source/keypressed.h"

#define ENABLE_PIN 25     /* GPIO.25 */
#define STEP_PIN   24     /* GPIO.24 */
#define DIR_PIN    23     /* GPIO.23 */

#define one_step   digitalWrite (STEP_PIN, 1); \
                   asm ("nop"); \
                   asm ("nop"); \
                   asm ("nop"); \
                   asm ("nop"); \
                   digitalWrite (STEP_PIN, 0)

uint8_t enable = 1;
uint8_t dir = 0;


/*! --------------------------------------------------------------------
 * 
 */
static void help()
{
    printf ("h = this message\n");
    printf ("ESC = Exit\n");
    printf ("1 = toggle enable\n");
    printf ("2 = toggle dir\n");
    printf ("3 = one step\n");
    printf ("4 = 400 steps\n");
}
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{
    char c;
    int taste = 0;
    
    printf ("Exit program with ESC\n");
  
    if( wiringPiSetup() < 0) {
        printf ("wiringPiSetup failed !\n");
        return EXIT_FAILURE;
    } 

    pinMode (ENABLE_PIN, OUTPUT);
    pinMode (STEP_PIN, OUTPUT);
    pinMode (DIR_PIN, OUTPUT);
    
    digitalWrite (ENABLE_PIN, enable);
    digitalWrite (DIR_PIN, dir);
    digitalWrite (STEP_PIN, 0);

    help();
    init_check_keypressed();                           /* init key-touch control */
    
    while (1) {
        while ((taste = check_keypressed(&c)) <= 0);  /* wait for keypressed */
        if (c == 27) break;                           /* quit by ESC */
        if (c == 'h') help();
        if (c == '1') {
            enable = !enable;
            digitalWrite (ENABLE_PIN, enable);            
        }
        if (c == '2') {
            dir = !dir;
            digitalWrite (DIR_PIN, dir);
        }
        if (c == '3') {
            one_step;            
        }
        if (c == '4') {
            int i;
            for (i=0; i<400; i++) {                  /* 400 steps = 1 revolution */
                one_step;
                usleep (500);                         /* n = 5,0s⁻1 */
            }
        }
        usleep (1000);                                /* wait 1ms */
    }
    digitalWrite (ENABLE_PIN, (enable = 1));
    destroy_check_keypressed();                       /* destroy key-touch control */
    return EXIT_SUCCESS;
}
