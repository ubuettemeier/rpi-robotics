/*! ---------------------------------------------------------------------
 * @file    test_driver_A4988.c
 * @date    10.12.2018
 * @name    Ulrich Buettemeier
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <sys/time.h>
#include <pthread.h>

#include "../../../sensor/ultrasonic-HC-SR04/source/keypressed.h"

#define STEP_TIME  750

#define ENABLE_PIN 25     /* GPIO.25 */
#define STEP_PIN   24     /* GPIO.24 */
#define DIR_PIN    23     /* GPIO.23 */

#define one_step   digitalWrite (STEP_PIN, 0); \
                   digitalWrite (STEP_PIN, 1); \
                   asm ("nop"); \
                   asm ("nop"); \
                   asm ("nop"); \
                   asm ("nop"); \
                   digitalWrite (STEP_PIN, 0)

uint8_t enable = !0;      /* Signal is low active */
uint8_t dir = 0;

/*!	--------------------------------------------------------------------
 * @brief   calculates time difference in us
 * @return  stop - start
 */
static int difference_micro (struct timeval *start, struct timeval *stop)
{
  return ((signed long long) stop->tv_sec * 1000000ll +
          (signed long long) stop->tv_usec) -	       
          ((signed long long) start->tv_sec * 1000000ll +
           (signed long long) start->tv_usec);
}
/*! --------------------------------------------------------------------
 * 
 */
static void help()
{
    printf ("\n");
    printf ("h = this message\n");
    printf ("ESC = Exit\n");
    printf ("1 = toggle enable\n");
    printf ("2 = toggle dir\n");
    printf ("3 = one step\n");
    printf ("4 = 400 steps\n");
    printf ("r = run\n");
    printf ("s = stop run\n");
}
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{
    char c;
    int taste = 0;
    struct timeval start, stop;
    uint8_t run = 0;
    int timediff;
    int max_delta = 0;
    int max_latency = 0;
    int n = 0;
    int state = 0;
  
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
        if ((taste = check_keypressed(&c)) > 0) {          /* look for keypressed */        
            if (c == 27) break;                           /* quit by ESC */
            if (c == 'h') help();
            if (!run) {
                if (c == '1') {                                /* toggle enable */
                    enable = !enable;
                    digitalWrite (ENABLE_PIN, enable);
                }
                if (c == '2') {                                /* toggle dir */
                    dir = !dir;
                    digitalWrite (DIR_PIN, dir);
                }
                if (c == '3') {
                    one_step;            
                }
                if (c == '4') {
                    int i;
                    for (i=0; i<400; i++) {                   /* 400 steps = 1 revolution */
                        one_step;
                        usleep (1000);                         /* n = 2,5s⁻1 */
                    }
                }
            }
            
            if (c == 'r') {
                enable = !1;
                digitalWrite (ENABLE_PIN, enable);                
                run = 1;
                max_delta = 0;
                gettimeofday (&start, NULL);
            }
            if (c == 's') {
                run = 0;   
                printf ("max latency = %i us\n", max_latency);
            }
        }   
        if (run) {
            gettimeofday (&stop, NULL); 
            if ((timediff = difference_micro (&start, &stop)) >= STEP_TIME) {                
                gettimeofday (&start, NULL);                
                one_step;
                if (timediff > max_delta) {
                    max_delta = timediff;
                    max_latency = timediff - STEP_TIME;
                }
                if (n > 600) {
                    n = 0;
                    if (state == 0) printf ("/\r");
                    if (state == 1) printf ("-\r");
                    if (state == 2) printf ("%c\r", '\\');
                    fflush(stdout);
                    state = (state < 2) ? state+1 : 0;
                }
                n++;
            }
        }
        // usleep (1000);                                /* wait 1ms */
    }
    digitalWrite (ENABLE_PIN, (enable = !0));         /* Signal is low aktiv */
    
    destroy_check_keypressed();                       /* destroy key-touch control */
    return EXIT_SUCCESS;
}
