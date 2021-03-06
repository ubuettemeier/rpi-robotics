/*! ---------------------------------------------------------------------
 * @file    test_driver_A4988.c
 * @date    10-12-2018
 * @name    Ulrich Buettemeier
 */

#define G 9.81

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "driver_A4988.h"
#include "../../../tools/rpi_tools/rpi_tools.h"
#include "../../../tools/keypressed/keypressed.h"

#define ENABLE_PIN_M1 25     /* GPIO.25  PIN 37 */
#define STEP_PIN_M1   24     /* GPIO.24  PIN 35 */
#define DIR_PIN_M1    23     /* GPIO.23  PIN 33 */

#define ENABLE_PIN_M2 29     /* GPIO.29  PIN 40 */
#define STEP_PIN_M2   28     /* GPIO.28  PIN 38 */
#define DIR_PIN_M2    27     /* GPIO.27  PIN 36 */

#define STEPS_PER_TURN 400

struct _mot_ctl_ *m1 = NULL;
/*! --------------------------------------------------------------------
 * 
 */
 
#define USE_MOTION_PROFIL_5

static inline void init_motion_diagram (struct _motion_diagram_ *md)
{
    #ifdef USE_MOTION_PROFIL_1
        add_mp_omega (md, 10.0, 0.5);    /* the first moition-point: omega=10 s⁻1; t=0.5s */
        add_mp_omega (md, 10.0, 2);
        add_mp_omega (md, 0, 3);
        add_mp_omega (md, -10.0, 4);
        add_mp_omega (md, -10.0, 5);
        add_mp_omega (md, 0.0, 7.5);
    #endif
    
    #ifdef USE_MOTION_PROFIL_2
        add_mp_Hz (md, 1.0, 0.0);
        add_mp_Hz (md, 1.0, 2.5);
        add_mp_Hz (md, 0.25, 2.5);
        add_mp_Hz (md, 0.25, 5.0-0.25);
        add_mp_Hz (md, -0.25, 5.0+0.25);
        add_mp_Hz (md, -0.25, 7.5);
        add_mp_Hz (md, -1.0, 7.5);
        add_mp_Hz (md, -1.0, 10.0);
    #endif
    
    #ifdef USE_MOTION_PROFIL_3
        add_mp_Hz (md, -0.5, 0.0);
        add_mp_Hz (md, -0.5, 0.005);
    #endif
    
    #ifdef USE_MOTION_PROFIL_4
        add_mp_rpm (md, -100, 1.2);
        add_mp_rpm (md, 100, 4);
        add_mp_rpm (md, 0, 5.2);
    #endif
            
    #ifdef USE_MOTION_PROFIL_5
        add_mp_steps (md, 2.0, 300);
        add_mp_steps (md, 2.0, 800);
        add_mp_steps (md, 0.0, 1200);
        add_mp_steps (md, -1.5, 1200);
        add_mp_steps (md, -2.0, 1600);
        add_mp_steps (md, 0.0, 1800);
    #endif
    
    #ifdef USE_MOTION_PROFIL_6
        add_mp_steps (md, 2.0, 300);
        add_mp_steps (md, 2.0, 800);                
        add_mp_steps (md, -2.0, 1200);
        add_mp_steps (md, -2.0, 1700);
        add_mp_steps (md, 0.0, 2000);
    #endif
    
    #ifdef USE_MOTION_PROFIL_7
        add_mp_Hz (md, 1.0, 0.0);
        add_mp_Hz (md, -1.0, 1.0);
        add_mp_Hz (md, 0.5, 1.0);
        add_mp_Hz (md, 0.5, 2.0);
        add_mp_Hz (md, -0.7, 2.0);
    #endif
}
/*! --------------------------------------------------------------------
 *  @brief  shows the key's
 */
static void help()
{
    printf ("\n");
    printf ("h = this message\n");
    printf ("ESC = Exit\n");  
    printf ("\n");  
    printf ("x = one step CW\n");
    printf ("y = one step CCW\n");
    printf ("\n");  
    printf ("1 = start m1 CW 400 steps\n");  
    printf ("2 = start m1 CCW 400 steps\n"); 
    printf ("8 = new speed\n"); 
    printf ("9 = toggle Motor disenable/enable\n");
    printf ("\n");  
    printf ("e = start m1 CW endless steps\n");  
    printf ("s = Motor STOP\n"); 
    printf ("f = motor fast stop\n"); 
    printf ("r = repeat motor sequence\n");
    printf ("\n");
    printf ("t = test motion diagram\n");
    printf ("c = read motion diagram from file <curve_1.dat>\n");
    printf ("v = read motion diagram from file <curve_2.dat>\n");
    printf ("g = draw motion diagram with gnuplot\n");
    printf ("k = kill motion diagram pointer\n");
}
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{
    char c;
    int key = 0; 
    uint8_t ende = 0;   
    int sn = 0;
    double speed_rpm[3] = {150.0, 53.5, 250.0};
    struct _motion_diagram_ *md = NULL;
    
    init_mot_ctl ();
    show_usleep (1000000, 100000/2);       /* see: rpi_tools.h */
    
    m1 = new_mot (ENABLE_PIN_M1, DIR_PIN_M1, STEP_PIN_M1, STEPS_PER_TURN);  /* create motor 1 */

    md = new_md(m1);                     /* new motion diagram for motor 1 */
    init_motion_diagram (md);
            
    help();
    init_check_keypressed();                            /* init key-touch control */
    show_md (md);
    
    while (!ende) {        
        if ((key = check_keypressed(&c)) > 0) {         /* look for keypressed */        
            switch ( c ) {
                case 27:                                /* quit by ESC */                
                    kill_all_mot ();                    /* make motors disenabled */
                    thread_state.kill = 1;              /* set terminat flag */
                    while (!thread_state.run);          /* wait for thread ending */
                    ende = 1;
                    break;
                case 'h':                   /* help */
                    help();
                    break;
                case 'x':                   /* one step CW */
                    mot_on_step (m1, MOT_CW);
                    printf ("real_stepcount=%lli\n", (long long int) m1->real_stepcount);
                    break;
                case 'y':                   /* one step CCW */
                    mot_on_step (m1, MOT_CCW);
                    printf ("real_stepcount=%lli\n", (long long int) m1->real_stepcount);
                    break;
                case '1':
                case '2':                  /* set motor sequence */
                    if (m1->mode == MOT_IDLE) {
                        mot_setparam (m1, (c == '1') ? MOT_CW : MOT_CCW, 400, 3*G, 5*G);
                        mot_start (m1);
                    } else 
                        printf ("Can't start motor. Motor is running\n");
                    break;   
                case '8':
                    sn = (sn < 2) ? sn+1 : 0;
                    mot_set_rpm (m1, speed_rpm[sn]);                                      
                    break;
                case '9':
                    if (m1->flag.enable == 1) 
                        mot_enable (m1);
                    else 
                        mot_disenable (m1);                        
                    break;                
                case 'e':                   /* set endless motor sequence (steps = 0) */
                    if (m1->mode == MOT_IDLE) {
                        mot_setparam (m1, MOT_CW, 0, 3*G, 5*G);
                        mot_set_rpm (m1, 200.0);
                        mot_start (m1);
                    } else 
                        printf ("Can't start motor. Motor is running\n");
                    break;                   
                case 's':
                    mot_stop (m1);
                    break;
                case 'f':                   /* motor fast stop */
                    mot_fast_stop (m1);
                    break;
                case 'r':                   /* repeat motor sequence */
                    if (m1->mode == MOT_IDLE) 
                        mot_start (m1);
                    else 
                        printf ("Can't start motor. Motor is running\n");
                    break;
                case 't':                   /* test moition diagramm */                                                
                    mot_start_md (md);                                            
                    break;                    
                case 'g':              /* draw motion diagram with gnuplot */
                    gnuplot_md (md);                        
                    break;
                case 'c':                
                    if ((md = new_md_from_file (m1, "curve_1.dat", RPM)) == NULL)
                        printf ("Read ERROR\n");
                    else
                        printf ("data set has %i motion points\n", count_mp(md));
                    break;
                case 'v':
                    if ((md = new_md_from_file (m1, "curve_2.dat", STEP)) == NULL)
                        printf ("Read ERROR\n");
                    else
                        printf ("data set has %i motion points\n", count_mp(md));
                    break;
                case 'k':               /* test the kill function */
                    if (kill_all_md() == EXIT_SUCCESS) {
                        md = NULL;
                        printf ("all diagram killed\n");
                    }
                    break;
            }
        }           
        usleep (1000);                                /* wait 1ms */
    }
    
    kill_all_md ();    
    destroy_check_keypressed();                       /* destroy key-touch control */
    return EXIT_SUCCESS;
}
