/*! --------------------------------------------------------------------
 *  @file    driver_A4988.c
 *  @date    10.13.2018
 *  @name    Ulrich Buettemeier
 */
 
#define _GNU_SOURCE
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <math.h>
 
#include "driver_A4988.h"


struct _thread_state_ thread_state = {
    .run = 0,
    .kill = 0
};

pthread_t thread_A4988 = NULL;                         /* glob thread handle */
struct _mot_ctl_ *first_mc = NULL, *last_mc = NULL;   /* motor control */
uint8_t is_init = 0;

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
static void printSchedulingPolicy(void)
{
    struct sched_param sp;
    int which;

    which = sched_getscheduler(0);    
    sched_getparam (0, &sp);
    printf ("-- priority = %i\n", sp.sched_priority);
    
    switch (which) {
        case SCHED_OTHER: 
            printf("-- default scheduling is being used\n");		
            break;
        case SCHED_FIFO:
            printf("-- fifo scheduling is being used\n");		
            break;
        case SCHED_RR:		
            printf("-- round robin scheduling is being used\n");		
            break;
	}		
}
/*! --------------------------------------------------------------------
 * @brief  Execute step
 */ 
static int mot_step (struct _mot_ctl_ *mc)
{
    if (mc) {
        digitalWrite (mc->mp.step_pin, 0);
        digitalWrite (mc->mp.step_pin, 1);
        asm ("nop");
        asm ("nop");
        asm ("nop");
        asm ("nop");
        digitalWrite (mc->mp.step_pin, 0);
    } else return (EXIT_FAILURE);
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */
static int mot_run (struct _mot_ctl_ *mc)
{
    int timediff;
    int delta;
    
    switch (mc->mode) {
        case MOT_STARTRUN:
            gettimeofday (&mc->start, NULL); 
            mc->mode = MOT_RUN;
            break;
        case MOT_RUN:
            gettimeofday (&mc->stop, NULL); 
            if ((timediff = difference_micro (&mc->start, &mc->stop)) >= mc->steptime) {
                gettimeofday (&mc->start, NULL);                 
                mot_step (mc);                 /* Execute step */
                
                if ((delta = timediff - mc->steptime) > mc->max_latency)
                    mc->max_latency = delta;                
                if (!(--mc->num_rest)) 
                    mc->mode = MOT_JOBREADY;
            }
            break;
        case MOT_JOBREADY:
            printf ("max_latency = %i\n", mc->max_latency);
            mc->mode = MOT_IDLE;
            break;
    }    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief  driver thread
 */

#define USE_HIGH_PRIORITY
 
void *run_A4988 (void *data)
{
    struct _mot_ctl_ *mc;

    thread_state.run = 1;
    printf ("-- <run_A4988> is started\n");
    
#ifdef USE_HIGH_PRIORITY
    struct sched_param sp = { .sched_priority = 95 };    /* priority > 95 you need permitted */       
    if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {    /* SCHED_FIFO  SCHED_RR   SCHED_OTHER  */
        perror("sched_setscheduler");
        return (NULL);
    }
    printSchedulingPolicy ();
#endif
    
    while (!thread_state.kill) {
        thread_state.mc_run = 0;
        if (!thread_state.mc_closed) {            
            mc = first_mc;
            while (mc) {
                if (mc->mode != MOT_IDLE) {
                    mot_run (mc);                
                    thread_state.mc_run = 1;
                }
                mc = mc->next;
            }
        }
        if (!thread_state.mc_run) usleep (1000);    
    }
    printf ("-- <run_A4988> is stoped\n");    
    thread_state.run = 0;
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief  Initializes the driver thread
 */  
#define USE_CORE_3_

int init_mot_ctl()
{    
    if( wiringPiSetup() < 0) {
        printf ("wiringPiSetup failed !\n");
        return EXIT_FAILURE;
    } 
    
    if (!thread_A4988) {         /* glob thread handle */
        thread_state.run = 0;
        thread_state.kill = 0;
        thread_state.mc_closed = 0;
        pthread_create (&thread_A4988, NULL, &run_A4988, NULL);  /* starts measuring routine */
        is_init = 1;
        
#ifdef USE_CORE_3        
        int i;
        cpu_set_t go_core;
        
        sleep (1);
        CPU_ZERO(&go_core);			// Initialisierung mit Nullen !
        CPU_SET(3, &go_core);		// CPU Core 3 eintragen
        pthread_setaffinity_np(thread_A4988, sizeof(cpu_set_t), &go_core);		// dem Thread ein CPU-Core zuweisen !
        for (i = 0; i < CPU_SETSIZE; i++)
            if (CPU_ISSET(i, &go_core))
                printf("go() Thread CPU = CPU %d\n", i);
        sleep (1);
#endif    
    }
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief  configures motor gpio
 */ 
void mot_initpins (struct _mot_ctl_ *mc)
{
    pinMode (mc->mp.enable_pin, OUTPUT);
    pinMode (mc->mp.dir_pin, OUTPUT);
    pinMode (mc->mp.step_pin, OUTPUT);
}
/*! --------------------------------------------------------------------
 * @brief  create dynamic memory for motor parameter
 */ 
struct _mot_ctl_ *new_mot (uint8_t pin_enable,
                             uint8_t pin_dir,
                             uint8_t pin_step,
                             uint32_t steps_per_turn)
{
    if (!is_init) 
        return (NULL);
    
    thread_state.mc_closed = 1;
    
    struct _mot_ctl_ *mc = (struct _mot_ctl_ *) malloc (sizeof(struct _mot_ctl_));
    mc->mode = MOT_IDLE;
    
    mc->mp.enable_pin = pin_enable;
    mc->mp.dir_pin = pin_dir;
    mc->mp.step_pin = pin_step;
        
    mot_disenable (mc);
    mot_set_dir (mc, MOT_CW);    
    digitalWrite (mc->mp.step_pin, 0);
    
    mc->steps_per_turn = steps_per_turn;
        
    mc->num_steps = 0;
    mc->num_rest = 0;
    
    mc->steptime = 1000;    /* default 1ms */
    
    mc->next = mc->prev = NULL;
    if (first_mc == NULL) first_mc = last_mc = mc;
    else {
        last_mc->next = mc;
        mc->prev = last_mc;
        last_mc = mc;
    }
    
    thread_state.mc_closed = 0;
    return (mc);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int kill_mot (struct _mot_ctl_ *mc)
{
    if (mc == NULL) return (EXIT_FAILURE);
    
    mot_set_dir (mc, MOT_CW);
    mot_disenable (mc);    
    
    if (mc->next != NULL) mc->next->prev = mc->prev;
    if (mc->prev != NULL) mc->prev->next = mc->next;
    if (mc == first_mc) first_mc = mc->next;
    if (mc == last_mc) last_mc = mc->prev;
    
    free (mc);
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int kill_all_mot ()
{
    int ret = EXIT_SUCCESS;
   
    while (first_mc) {
        if (kill_mot (first_mc) != EXIT_SUCCESS) 
            ret = EXIT_FAILURE;
    }
   
    return (ret);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int count_mot (void)
{
    struct _mot_ctl_ *mc = first_mc;
    int count = 0;
    
    while (mc) {
        count++;
        mc = mc->next;
    }
    return (count);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_setparam (struct _mot_ctl_ *mc,
                  uint8_t dir,
                  uint64_t steps)
{
    if (mc) {
        mot_set_dir (mc, dir);
        mc->num_steps = mc->num_rest = steps;
        mc->max_latency = 0;
    } else return (EXIT_FAILURE);
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_start (struct _mot_ctl_ *mc)
{
    if (mc) {
        mot_enable (mc);
        mc->mode = MOT_STARTRUN;        
    } else return (EXIT_FAILURE);
   
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_set_enable_pin (struct _mot_ctl_ *mc, uint8_t enable)
{
    if (mc) digitalWrite (mc->mp.enable_pin, (mc->enable = enable));
    else return (EXIT_FAILURE);
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_enable (struct _mot_ctl_ *mc)
{
    return (mot_set_enable_pin (mc, 0));
}

int mot_disenable (struct _mot_ctl_ *mc)
{
    return (mot_set_enable_pin (mc, 1));
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_set_dir (struct _mot_ctl_ *mc, uint8_t direction)
{
    if (mc) digitalWrite (mc->mp.dir_pin, (mc->dir = direction));
    else return (EXIT_FAILURE);
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief   set speed in [time per step]
 */ 
int mot_set_steptime (struct _mot_ctl_ *mc, int steptime)
{
   if (!mc) return (EXIT_FAILURE);
   mc->steptime = steptime;
   printf ("-- new steptime = %i us\n", mc->steptime);
   
   return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief  set speed in rpm
 */ 
int mot_set_rpm (struct _mot_ctl_ *mc, double rpm)
{
    if (!mc) return (EXIT_FAILURE);
    if ((rpm == 0.0) || (mc->steps_per_turn == 0)) return (EXIT_FAILURE);
        
    int ret = mot_set_steptime (mc, (int)(1000000.0 * 60.0 / (double)mc->steps_per_turn / rpm));
    
    return ( ret );
}
/*! --------------------------------------------------------------------
 * @brief  set speed in s^-1
 */
extern int mot_set_Hz (struct _mot_ctl_ *mc, double Hz)
{
    return (mot_set_rpm(mc, Hz * 60.0));
}