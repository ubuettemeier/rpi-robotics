/*! --------------------------------------------------------------------
 *  @file    driver_A4988.c
 *  @date    10-13-2018
 *  @name    Ulrich Buettemeier
 *  @todo    insert error flags
 *              step error
 *              latency error
 * 
 *  @example   
 *      #include <stdio.h>
 *      #include <unistd.h>
 *      #include "driver_A4988.h"
 *      
 *      int main() {
 *          struct _mot_ctl_ *m1 = NULL 
 * 
 *          init_mot_ctl ();    
 *          sleep (1);
 *          m1 = new_mot (25, 23, 24, 400);
 *          mot_setparam (m1, MOT_CW, 400, 20.0, 40.0);  // 400 steps, speed up=20 s⁻2, speed down=40 s⁻2
 *          mot_start (m1);
 *          while (m1->flag.aktive) sleep(1);
 *          mot_disenable (m1);
 *          return ( 0 );
 *      }
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
static int64_t difference_micro (struct timeval *start, struct timeval *stop)
{
  return ((int64_t) stop->tv_sec * 1000000ll +
          (int64_t) stop->tv_usec) -	       
          ((int64_t) start->tv_sec * 1000000ll +
           (int64_t) start->tv_usec);
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
 *          used by execute_step()
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
 * @brief  used by mot_run()
 */
static int64_t execute_step (struct _mot_ctl_ *mc, int64_t timediff)
{
    int64_t delta;
    
    gettimeofday (&mc->start, NULL);    /* set new time */   
    mot_step (mc);                      /* Execute step */
    mc->current_stepcount++;            /* Increase step counter */
    mc->runtime = difference_micro (&mc->run_start, &mc->stop); 

    if ((delta = timediff - mc->current_steptime) > mc->max_latency)    /* check max latency */
        mc->max_latency = delta;   
        
    if ((!mc->flag.endless) || 
        (mc->flag.endless && (mc->mode == MOT_MAKE_SPEED_DOWN))) {   /* check step counter */
            
        if (!(--mc->num_rest)) mc->mode = MOT_JOBREADY;
    }
    
    return (delta);
}
/*! --------------------------------------------------------------------
 * @brief  used by run_A4988()
 */
static int mot_run (struct _mot_ctl_ *mc)
{
    int64_t timediff;
    static int64_t delta;    
    
    switch (mc->mode) {        
        case MOT_STARTRUN:
            mc->max_latency = 0;
            mc->current_steptime = mc->steptime;
            mc->current_stepcount = 0;              /* Current number of steps = 0 */
            delta = 0;                              /* latency */
            gettimeofday (&mc->start, NULL);        /* get start time */
            mc->run_start = mc->start;                  
            mc->mode = (mc->a_start <= 0.0) ? MOT_RUN : MOT_SPEED_UP;            
            break;
        case MOT_SPEED_UP:             
            if (mc->num_steps && (mc->current_stepcount >= mc->num_steps)) mc->mode = MOT_JOBREADY;         /* target number of steps reached */
            else {
                
                double phi0 = mc->phi_per_step * ((double)mc->current_stepcount);   /* current angle from acceleration */
                double phi1 = phi0 + mc->phi_per_step; 
                double omega = sqrt(2.0 * mc->a_start * phi1);                
                if (omega >= mc->omega) {                       /* target speed reached */ 
                    mc->current_steptime = mc->steptime;
                    mc->mode = MOT_RUN;                         /* constant speed */
                } else {                                        /* further speed-up */                    
                    double t0 = sqrt(phi0 * 2.0 / mc->a_start);
                    double t1 = sqrt(phi1 * 2.0 / mc->a_start);
                    mc->current_steptime = (int64_t) ((t1 - t0) * 1000000.0);
                    mc->mode = MOT_MAKE_SPEED_UP;
                }
            }
            break;
        case MOT_RUN:
        case MOT_MAKE_SPEED_UP:
        case MOT_MAKE_SPEED_DOWN:
            gettimeofday (&mc->stop, NULL); 
            if ((timediff = difference_micro (&mc->start, &mc->stop)) >= (mc->current_steptime - delta)) {    /* Execute step */
                delta = execute_step (mc, timediff);                  
                
                if (((mc->mode == MOT_RUN) || (mc->mode == MOT_MAKE_SPEED_UP)) && (mc->a_stop > 0.0)) { 
                    if (mc->num_steps != 0) {
                        uint64_t rest_steps = calc_steps_for_step_down(mc);
                        if (mc->num_rest <= rest_steps) {
                            mc->mode = MOT_SPEED_DOWN;                            
                        }
                    }
                }
                
                if (mc->mode == MOT_MAKE_SPEED_UP) mc->mode = MOT_SPEED_UP;
                if (mc->mode == MOT_MAKE_SPEED_DOWN) mc->mode = MOT_SPEED_DOWN;
            }
            break;
        case MOT_SPEED_DOWN: 
            if (mc->num_rest) {
                double phi1 = (double)mc->num_rest * mc->phi_per_step;
                double phi0 = phi1 - mc->phi_per_step;
                double t1 = sqrt(phi1 * 2.0 / mc->a_stop);
                double t0 = sqrt(phi0 * 2.0 / mc->a_stop);                
                mc->current_steptime = (int64_t) ((t1 - t0) * 1000000.0);
                mc->mode = MOT_MAKE_SPEED_DOWN;
            }
            break;
        case MOT_JOBREADY: 
            mc->mode = MOT_IDLE;
            mc->flag.aktiv = 0;
            printf ("-- max_latency=%lli us  current_stepcount=%llu  runtime=%lli us\n", mc->max_latency, mc->current_stepcount, mc->runtime);            
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
    struct sched_param sp = { .sched_priority = 95 };       /* priority > 95 you need permitted */       
    if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {     /* SCHED_FIFO  SCHED_RR   SCHED_OTHER  */
        perror("sched_setscheduler");
        return (NULL);
    }
    printSchedulingPolicy ();
#endif
    
    uint8_t mc_was_aktiv;
    
    while (!thread_state.kill) {
        mc_was_aktiv = 0;
        if (!thread_state.mc_closed) {            
            mc = first_mc;
            while (mc) {
                if (mc->mode != MOT_IDLE) {
                    mot_run (mc);                
                    mc_was_aktiv = 1;
                }
                mc = mc->next;
            }
        }
        if (!mc_was_aktiv) usleep (1000);    
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
    } else printf ("-- wiringPi initialisiert\n");
    
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
        CPU_ZERO(&go_core);			/* Initialize with 0 */
        CPU_SET(3, &go_core);		/* enter Core 3 */
        pthread_setaffinity_np(thread_A4988, sizeof(cpu_set_t), &go_core);		/* Assign a CPU core to the thread */
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
    if (!mc) return;
    
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
    mc->flag.aktiv = 0;
    mc->flag.endless = 0;
    
    mc->mp.enable_pin = pin_enable;
    mc->mp.dir_pin = pin_dir;
    mc->mp.step_pin = pin_step;
    
    mot_initpins (mc);
        
    mot_disenable (mc);                     /* flag enable is updated */
    mot_set_dir (mc, MOT_CW);               /* flag dir is updated */
    digitalWrite (mc->mp.step_pin, 0);
    
    mc->steps_per_turn = steps_per_turn;   
    mc->phi_per_step = 2.0 * M_PI / (double)mc->steps_per_turn;    
        
    mc->num_steps = 0;
    mc->num_rest = 0;
    mc->current_stepcount = 0;
    
    mc->steptime = 1000;                                        /* steptime in us. Default 1ms */
    mc->omega = calc_omega (mc->steps_per_turn, mc->steptime);
    mc->a_start = mc->a_stop = 0.0;                             /* speed-up, speed-down */
    
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
 * @brief  The motor enable pin is deactivated and 
 *          the parameters are removed from the list.
 */ 
int kill_mot (struct _mot_ctl_ *mc)
{
    if (mc == NULL) return (EXIT_FAILURE);
    
    thread_state.mc_closed = 1;
    
    mot_set_dir (mc, MOT_CW);
    mot_disenable (mc);    
    
    if (mc->next != NULL) mc->next->prev = mc->prev;
    if (mc->prev != NULL) mc->prev->next = mc->next;
    if (mc == first_mc) first_mc = mc->next;
    if (mc == last_mc) last_mc = mc->prev;
    
    free (mc);
    
    thread_state.mc_closed = 0;
    
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
 * @param  *mc  motor handle
 *          dir  0=CW  1=CCW
 *          steps>0 Number of steps; steps==0 endless steps
 */ 
int mot_setparam (struct _mot_ctl_ *mc,
                  uint8_t dir,
                  uint64_t steps,
                  double a_start,           /* alpha Start */
                  double a_stop)            /* alpha Stop */
{
    if (!mc) return (EXIT_FAILURE);
    
    mot_set_dir (mc, dir);        
    mc->num_steps = mc->num_rest = steps;
    mc->flag.endless = (steps == 0) ? 1 : 0;
    mc->max_latency = 0;
    mc->a_start = a_start;
    mc->a_stop = a_stop;
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_start (struct _mot_ctl_ *mc)
{
    if (!mc) return (EXIT_FAILURE);

    mot_enable (mc);
    mc->mode = MOT_STARTRUN;        
    mc->flag.aktiv = 1;
   
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_stop (struct _mot_ctl_ *mc)
{
    if (!mc) return (EXIT_FAILURE);
    
    if (mc->mode != MOT_IDLE) {
        if (mc->a_stop > 0.0) {
            mc->num_rest = calc_steps_for_step_down (mc);
            mc->mode = MOT_SPEED_DOWN;
        }
        else mc->mode = MOT_JOBREADY;
    }
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief  set chip enable/disenable
 *          chip enable-pin is low aktiv
 */ 
int mot_switch_enable (struct _mot_ctl_ *mc, uint8_t enable)
{
    if (mc) digitalWrite (mc->mp.enable_pin, (mc->flag.enable = enable));
    else return (EXIT_FAILURE);
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_enable (struct _mot_ctl_ *mc)
{
    return (mot_switch_enable (mc, 0));        /* chip is low aktiv */
}

int mot_disenable (struct _mot_ctl_ *mc)
{
    return (mot_switch_enable (mc, 1));        /* chip is low aktiv */
}
/*! --------------------------------------------------------------------
 * @brief  set the direction
 */ 
int mot_set_dir (struct _mot_ctl_ *mc, uint8_t direction)
{
    if (mc) digitalWrite (mc->mp.dir_pin, (mc->flag.dir = direction));
    else return (EXIT_FAILURE);
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 * @brief   set speed in [time per step]
 * @param   mc = motor handle
 *          steptime = steptime in us
 */ 
int mot_set_steptime (struct _mot_ctl_ *mc, int steptime)
{
    if (!mc) return (EXIT_FAILURE);
   
    if (steptime != mc->steptime) {
        mc->steptime = steptime;
        mc->omega = calc_omega (mc->steps_per_turn, mc->steptime);    
        printf ("-- new steptime=%u us  Omega=%2.3f s⁻1\n", mc->steptime, mc->omega);
    }
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
/*! --------------------------------------------------------------------
 * @brief   omega = 2*Pi/T [s⁻1]
 * @param   steptime in us
 * @return  omega in s⁻1
 */
double calc_omega (uint32_t steps_per_turn, uint32_t steptime)
{
   if ((steps_per_turn == 0) || (steptime == 0)) return (0.0);
   
   return (2.0 * M_PI / ((double)steptime / 1000000.0 * (double)steps_per_turn));
}
/*! --------------------------------------------------------------------
 * 
 */
double calc_steps_for_step_down (struct _mot_ctl_ *mc)
{
    double omega = calc_omega (mc->steps_per_turn, mc->current_steptime); 
    double phi = omega * omega / 2.0 / mc->a_stop;         
    return (phi / mc->phi_per_step);
}
