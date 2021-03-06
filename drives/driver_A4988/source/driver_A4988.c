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
 *          struct _mot_ctl_ *m1 = NULL;
 * 
 *          init_mot_ctl ();    
 *          sleep (1);
 *          m1 = new_mot (25, 23, 24, 400);              // param: GPIO_ENABLE PIN, GPIO_DIR PIN, GPIO_STEP PIN, steps_per_turn
 *          mot_setparam (m1, MOT_CW, 400, 20.0, 40.0);  // 400 steps, speed up=20 s⁻2, speed down=40 s⁻2
 *          mot_start (m1);
 * 
 *          while (m1->flag.aktive) sleep(1);
 * 
 *          mot_disenable (m1);
 *          kill_all_mot ();                    
 *          thread_state.kill = 1;
 *          while (!thread_state.run); 
 * 
 *          return ( 0 );
 *      }
 */

#define USE_GPIO
#define _GNU_SOURCE
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_GPIO
#include <wiringPi.h>
#endif

#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <math.h>
 
#include "../../../tools/rpi_tools/rpi_tools.h"
#include "driver_A4988.h"


struct _thread_state_ thread_state = {
    .run = 0,
    .kill = 0
};

pthread_t thread_A4988 = 0;                             /* glob thread handle */
struct _mot_ctl_ *first_mc = NULL, *last_mc = NULL;   /* motor control */
uint8_t is_init = 0;

struct _motion_diagram_ *first_md = NULL, *last_md = NULL;  /* motion diagram */

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
#ifdef USE_GPIO
        digitalWrite (mc->mp.step_pin, 0);
        digitalWrite (mc->mp.step_pin, 1);
        asm ("nop");
        asm ("nop");
        asm ("nop");
        asm ("nop");
        digitalWrite (mc->mp.step_pin, 0);
#endif
        if (mc->flag.dir)
            mc->real_stepcount--;
        else 
            mc->real_stepcount++;
            
    } else return (EXIT_FAILURE);
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  used by mot_run()
 */
static int64_t execute_step (struct _mot_ctl_ *mc, uint64_t timediff)
{
    uint64_t latency;
    
    gettimeofday (&mc->start, NULL);    /* set new time */   
    mot_step (mc);                      /* Execute step */
    mc->current_stepcount++;            /* Increase step counter */
    mc->runtime = (uint64_t) difference_micro (&mc->run_start, &mc->stop); 

    if ((latency = timediff - mc->current_steptime) > mc->max_latency)    /* check max latency */
        mc->max_latency = latency;   
        
    if ((!mc->flag.endless) || 
        (mc->flag.endless && (mc->mode == MOT_RUN_SPEED_DOWN))) {   /* check step counter */
            
        if (!(--mc->num_rest)) 
            mc->mode = MOT_JOB_READY;
    }
    
    return latency;
}
/*! --------------------------------------------------------------------
 *  @brief  used by mot_run(). see: state MOT_RUN_MD
 */
static inline void switch_dir (struct _mot_ctl_ *mc, double new_omega)
{
    if (new_omega < 0.0) {
        mot_set_dir (mc, MOT_CCW);
    } else if (new_omega > 0.0) {
        mot_set_dir (mc, MOT_CW);
    }
}
/*! --------------------------------------------------------------------
 * @brief  used by driver thread run_A4988()
 */
static int mot_run (struct _mot_ctl_ *mc)
{
    uint64_t timediff;
    static uint64_t latency; 
    
    switch (mc->mode) {        
        case MOT_START_RUN:
            mc->max_latency = 0;
            mc->current_steptime = mc->steptime;
            mc->num_rest = (mc->num_steps >= 0) ? mc->num_steps : 0;
            mc->current_stepcount = 0;              /* Current number of steps = 0 */
            mc->current_omega = 0.0;
            latency = 0;
            gettimeofday (&mc->start, NULL);        /* get start time */
            mc->run_start = mc->start;              /* memory start time */    
            mc->mode = (mc->a_start <= 0.0) ? MOT_RUN : MOT_SPEED_UP;            
            break;
            
        case MOT_SPEED_UP:             
            if (mc->num_steps && (mc->current_stepcount >= mc->num_steps)) mc->mode = MOT_JOB_READY;         /* target number of steps reached */
            else {                
                double current_phi = mc->phi_per_step * ((double)mc->current_stepcount);   /* current angle from acceleration */                
                double new_omega = sqrt(2.0 * mc->a_start * (current_phi + mc->phi_per_step));
                if (new_omega >= mc->omega) {                                               /* target speed reached */ 
                    mc->current_steptime = mc->steptime;
                    mc->current_omega = mc->omega; 
                    mc->mode = MOT_RUN;                                                     /* constant speed */
                } else {
                    double t = 2.0 * mc->phi_per_step / (new_omega + mc->current_omega);    /* new time for step */
                    mc->current_steptime = (int64_t) (t * 1000000.0);                       /* steptime in us */
                    mc->current_omega = new_omega;
                    mc->mode = MOT_RUN_SPEED_UP;
                }
            }
            break;
            
        case MOT_RUN:
        case MOT_RUN_SPEED_UP:
        case MOT_RUN_SPEED_DOWN:
        case MOT_RUN_SPEED_MD:
            gettimeofday (&mc->stop, NULL); 
            if ((timediff = (uint64_t)difference_micro (&mc->start, &mc->stop)) >= (mc->current_steptime - latency)) {    /* Execute step */
                latency = execute_step (mc, timediff);                  
                
                if (((mc->mode == MOT_RUN) || (mc->mode == MOT_RUN_SPEED_UP)) && (mc->a_stop > 0.0)) {          /* See if you need to brake. */
                    if (mc->num_steps != 0) {
                        uint64_t rest_steps = calc_steps_for_step_down(mc);
                        if (mc->num_rest <= rest_steps) {
                            mc->mode = MOT_SPEED_DOWN;                            
                        }
                    }
                }
                
                if (mc->mode == MOT_RUN_SPEED_UP) 
                    mc->mode = MOT_SPEED_UP;
                if (mc->mode == MOT_RUN_SPEED_DOWN) 
                    mc->mode = MOT_SPEED_DOWN;
                if (mc->mode == MOT_RUN_SPEED_MD)
                    mc->mode = MOT_RUN_MD;
            }
            break;
            
        case MOT_SPEED_DOWN: 
            if (mc->num_rest) {
                double phi1 = (double)mc->num_rest * mc->phi_per_step;
                double phi0 = phi1 - mc->phi_per_step;
                double t1 = sqrt(phi1 * 2.0 / mc->a_stop);
                double t0 = sqrt(phi0 * 2.0 / mc->a_stop);                
                mc->current_steptime = (int64_t) ((t1 - t0) * 1000000.0);       /* steptime in us */
                mc->current_omega = calc_omega (mc->steps_per_turn, mc->current_steptime);
                mc->mode = MOT_RUN_SPEED_DOWN;
            }
            break;
            
        case MOT_RUN_MD: {                
                if (mc->mc_mp->current_step >= mc->mc_mp->steps) {   /* end of MOT_RUN_MD */
                    mc->mode = MOT_START_MD;                    
                } else {                          
                    double t;
                    double new_omega;
                    if (mc->mc_mp->a == 0.0) {                     /* omega const. */
                        new_omega = mc->mc_mp->omega;                        
                        switch_dir (mc, new_omega);                 /* inline function */
                        
                        t = mc->phi_per_step / new_omega;
                    } else {                        
                        double faktor = 1.0;                        /* CW */
                        if ((mc->current_omega < 0.0) ||            /* CCW */
                            ((mc->current_omega == 0.0) && (mc->mc_mp->a < 0.0))) {
                            faktor = -1.0;   
                        }
                        new_omega = sqrt((mc->current_omega * mc->current_omega) + 2.0*(mc->mc_mp->a)*faktor*mc->phi_per_step) * faktor;
                        switch_dir (mc, new_omega);                 /* inline function */
                        
                        t = (2.0 * faktor*mc->phi_per_step) / (mc->current_omega + new_omega);
                    }                    
                        
                    mc->current_steptime = fabs(t) * 1000000.0;
                    mc->current_omega = new_omega;                    
                    latency = 0;
                    
                    gettimeofday (&mc->start, NULL);        
                    mc->mode = MOT_RUN_SPEED_MD;
                }
                mc->mc_mp->current_step++;
            }
            break;
        case MOT_START_MD: {                                 
                mc->mc_mp = mc->mc_mp->next;       /* set the next motion-point in the struct _mot_ctl_ */
                if (!mc->mc_mp) {
                    mc->mode = MOT_JOB_READY;                    
                } else {                          
                    if (mc->mc_mp->delta_t != 0.0) {
                        mc->mc_mp->current_step = 0;
                        mc->current_omega = mc->mc_mp->prev->omega;
                        mc->mode = MOT_RUN_MD;
                    }
                }                                
            }
            break;
        case MOT_JOB_READY:             
            mc->mode = MOT_IDLE;
            mc->flag.aktiv = 0;
            printf ("-- max_latency=%lli us  current_stepcount=%llu  runtime=%lli us   real_stepcout=%lli\n", 
                     (long long int) mc->max_latency, 
                     (long long unsigned) mc->current_stepcount, 
                     (long long int) mc->runtime,
                     (long long int) mc->real_stepcount);
            break;
    }    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  driver thread
 */

#define USE_HIGH_PRIORITY
 
void *run_A4988 (void *data)
{
    struct _mot_ctl_ *mc;

    thread_state.kill = 0;
    thread_state.run = 1;
    printf ("-- <run_A4988> is started\n");
    
#ifdef USE_HIGH_PRIORITY
    struct sched_param sp = { .sched_priority = 95 };       /* priority > 95 you need permitted ??? */       
    if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {     /* SCHED_FIFO  SCHED_RR   SCHED_OTHER  */
        perror("sched_setscheduler");
        return (NULL);
    }
    printSchedulingPolicy ();
#endif
    
    uint8_t all_mot_idle;
    
    while (!thread_state.kill) {                /* thread main loop */
        all_mot_idle = 1;
        if (!thread_state.mc_closed) {            
            mc = first_mc;
            while (mc) {
                if (mc->mode != MOT_IDLE) {
                    mot_run (mc);                
                    all_mot_idle = 0;                    
                }
                mc = mc->next;
            }
        }
        
        if (all_mot_idle) 
            usleep (1000); 
    }
    printf ("-- <run_A4988> is stoped\n");    
    thread_state.run = 0;
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  Initializes the driver thread
 */  
#define USE_CORE_3_

int init_mot_ctl()
{    
#ifdef USE_GPIO
    if( wiringPiSetup() < 0) {
        printf ("-- wiringPiSetup failed !\n");
        return EXIT_FAILURE;
    } else printf ("-- wiringPi initialisiert\n");
#endif
    
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
                printf("-- thread run_A4988() -> CPU = CPU %d\n", i);
        sleep (1);
#endif    
    }
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  configures motor gpio
 */ 
void mot_initpins (struct _mot_ctl_ *mc)
{
    if (!mc) 
        return;
   
#ifdef USE_GPIO
    pinMode (mc->mp.enable_pin, OUTPUT);
    pinMode (mc->mp.dir_pin, OUTPUT);
    pinMode (mc->mp.step_pin, OUTPUT);
#endif    
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
        return NULL;
    
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
#ifdef USE_GPIO
    digitalWrite (mc->mp.step_pin, 0);
#endif
    
    mc->steps_per_turn = steps_per_turn;  
    mc->real_stepcount = 0; 
    mc->phi_per_step = 2.0 * M_PI / (double)mc->steps_per_turn;    
        
    mc->num_steps = -1;
    mc->num_rest = 0;
    mc->current_stepcount = 0;
    
    mc->steptime = 2000;                                        /* steptime in us. Default 2ms */
    mc->omega = calc_omega (mc->steps_per_turn, mc->steptime);
    mc->a_start = mc->a_stop = 0.0;                             /* speed-up, speed-down */
    
    mc->mc_mp = NULL;                       /* moition point; for define use function mot_start_md()  */
    
    mc->next = mc->prev = NULL;
    if (first_mc == NULL) {
        first_mc = last_mc = mc;
    } else {
        last_mc->next = mc;
        mc->prev = last_mc;
        last_mc = mc;
    }
    
    thread_state.mc_closed = 0;
    return mc;
}
/*! --------------------------------------------------------------------
 * @brief  The motor enable pin is deactivated and 
 *          the parameters are removed from the list.
 */ 
int kill_mot (struct _mot_ctl_ *mc)
{
    if (mc == NULL) 
        return EXIT_FAILURE;
    
    thread_state.mc_closed = 1;
    
    mot_set_dir (mc, MOT_CW);
    mot_disenable (mc);    
    
    if (mc->next != NULL) mc->next->prev = mc->prev;
    if (mc->prev != NULL) mc->prev->next = mc->next;
    if (mc == first_mc) first_mc = mc->next;
    if (mc == last_mc) last_mc = mc->prev;
    
    clear_mc_in_md (mc);
    
    free (mc);
    
    thread_state.mc_closed = 0;
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */ 
int kill_all_mot ()
{   
    while (first_mc) {
        if (kill_mot (first_mc) != EXIT_SUCCESS) 
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
    
    return count;
}
/*! --------------------------------------------------------------------
 * 
 */ 
int check_mc_pointer (struct _mot_ctl_ *mc)
{
    struct _mot_ctl_ *m = first_mc;
    
    while (m) {
        if (m == mc) 
            return EXIT_SUCCESS;
        m = m->next;
    }
    
    return EXIT_FAILURE;
}
/*! --------------------------------------------------------------------
 * 
 */ 
void show_mot_ctl (struct _mot_ctl_ *mc)
{
    if (check_mc_pointer(mc) == EXIT_FAILURE) {
        printf ("-- ERROR in func show_mot_ctl(). Parameter is incorrect\n");
        return;
    }
    
    printf ("max_latency=%lli\n", (long long int)mc->max_latency);
    printf ("current_stepcount=%llu\n", (unsigned long long)mc->current_stepcount);
    printf ("real_stepcount=%lli\n", (long long int)mc->real_stepcount);
    
}
/*! --------------------------------------------------------------------
 * @param  *mc  motor handle
 *          dir  0=CW  1=CCW
 *          steps>0 Number of steps; steps==0 endless steps
 */ 
int mot_setparam (struct _mot_ctl_ *mc,
                  uint8_t dir,
                  uint64_t num_steps,
                  double a_start,           /* alpha Start */
                  double a_stop)            /* alpha Stop */
{
    if (!mc) 
        return EXIT_FAILURE;
    
    mot_set_dir (mc, dir);        
    mc->num_steps = mc->num_rest = num_steps;
    mc->flag.endless = (num_steps == 0) ? 1 : 0;
    mc->max_latency = 0;
    mc->a_start = a_start;
    mc->a_stop = a_stop;
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_start (struct _mot_ctl_ *mc)
{
    if (!mc) 
        return EXIT_FAILURE;

    if (mc->num_steps < 0) {
        printf ("-- parameter num_steps failed\n");
        return (EXIT_FAILURE);
    }
    
    if (mc->mode != MOT_IDLE) {
        printf ("-- Can't start motor\n");
        return EXIT_FAILURE;
    }
    
    mot_enable (mc);
    mc->mode = MOT_START_RUN;        
    mc->flag.aktiv = 1;
   
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_stop (struct _mot_ctl_ *mc)
{
    if (!mc) 
        return EXIT_FAILURE;
    
    if (mc->mode != MOT_IDLE) {
        if (mc->a_stop > 0.0) {                             /* stop with speed-down */
            mc->num_rest = calc_steps_for_step_down (mc);
            mc->mode = MOT_SPEED_DOWN;
        }
        else mc->mode = MOT_JOB_READY;
    }
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   Engine stopping without ramp.
 */ 
int mot_fast_stop (struct _mot_ctl_ *mc)
{
    if (!mc) 
        return EXIT_FAILURE;
    
    if (mc->mode != MOT_IDLE)
        mc->mode = MOT_JOB_READY;    
        
    return EXIT_SUCCESS;
} 
/*! --------------------------------------------------------------------
 * @param   dir==0 CW, dir==1 CCW
 */
int mot_on_step (struct _mot_ctl_ *mc, uint8_t dir)
{
    if (!mc) 
        return EXIT_FAILURE;
        
    if (mc->mode != MOT_IDLE) {
        printf ("-- Can't step. Motor is not idle\n");
        return EXIT_FAILURE;
    }
        
    mot_enable (mc);
    mot_set_dir (mc, dir);
    mot_step (mc);              /* Execute step */
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   Engine start. The motor follows the motion diagram.
 */ 
int mot_start_md (struct _motion_diagram_ *md)
{
    if (md) {
        if (check_md_pointer(md) != EXIT_SUCCESS) {      /* check md */
            printf ("-- Data set not found\n");
            return EXIT_FAILURE;
        }
    }
    
    if (!md || !md->mc)
        return EXIT_FAILURE;
    
    if (md->data_set_is_incorrect) {
        printf ("-- Data set is incorrect. ERROR No.: %i\n", md->data_set_is_incorrect);
        return EXIT_FAILURE;
    }    
    
    if (md->mc->mode != MOT_IDLE) {
        printf ("-- Can't start motor-program\n");
        return EXIT_FAILURE;
    }
    
    mot_enable (md->mc);                            /* switch motor ON */
    md->mc->max_latency = 0;
    md->mc->current_stepcount = 0;
    md->mc->mc_mp = md->first_mp;                   /* set first moition-point */
    md->mc->current_omega = md->first_mp->omega;
    gettimeofday (&md->mc->run_start, NULL);
    md->mc->mode = MOT_START_MD;
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  set chip enable/disenable
 *          chip enable-pin is low aktiv
 */ 
int mot_switch_enable (struct _mot_ctl_ *mc, uint8_t enable)
{
    if (mc) {
#ifdef USE_GPIO
        digitalWrite (mc->mp.enable_pin, (mc->flag.enable = enable));
#endif    
    } else return EXIT_FAILURE;
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */ 
int mot_enable (struct _mot_ctl_ *mc)
{
    return mot_switch_enable (mc, 0);        /* chip is low aktiv */
}

int mot_disenable (struct _mot_ctl_ *mc)
{
    return mot_switch_enable (mc, 1);        /* chip is low aktiv */
}
/*! --------------------------------------------------------------------
 * @brief  set the direction
 */ 
int mot_set_dir (struct _mot_ctl_ *mc, uint8_t direction)
{
    if (!mc) 
        return EXIT_FAILURE;
#ifdef USE_GPIO
    digitalWrite (mc->mp.dir_pin, (mc->flag.dir = direction));
#endif    
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   set speed in [time per step]
 * @param   mc = motor handle
 *          steptime = steptime in us
 */ 
int mot_set_steptime (struct _mot_ctl_ *mc, int steptime)
{
    if (!mc) 
        return (EXIT_FAILURE);
   
    if (steptime != mc->steptime) {
        mc->steptime = steptime;
        mc->omega = calc_omega (mc->steps_per_turn, mc->steptime);    
        printf ("-- new steptime=%u us  Omega=%2.3f s⁻1\n", mc->steptime, mc->omega);
    }
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  set speed rpm [min⁻1]
 */ 
int mot_set_rpm (struct _mot_ctl_ *mc, double rpm)
{
    if (!mc) 
        return (EXIT_FAILURE);
        
    if ((rpm == 0.0) || (mc->steps_per_turn == 0)) 
        return (EXIT_FAILURE);
        
    int ret = mot_set_steptime (mc, (int)(1000000.0 * 60.0 / (double)mc->steps_per_turn / rpm));
    
    return ret;
}
/*! --------------------------------------------------------------------
 * @brief  set speed f [s⁻1]
 */
extern int mot_set_Hz (struct _mot_ctl_ *mc, double Hz)
{    
    return mot_set_rpm(mc, Hz * 60.0);
}
/*! --------------------------------------------------------------------
 * @brief   omega = 2*Pi/T [s⁻1]
 * @param   steptime in us
 * @return  omega in s⁻1
 */
double calc_omega (uint32_t steps_per_turn, uint32_t steptime)
{
    if ((steps_per_turn == 0) || (steptime == 0)) 
        return (0.0);
   
    return 2.0 * M_PI / ((double)steptime / 1000000.0 * (double)steps_per_turn);
}
/*! --------------------------------------------------------------------
 * 
 */
double calc_steps_for_step_down (struct _mot_ctl_ *mc)
{
    double omega = calc_omega (mc->steps_per_turn, mc->current_steptime); 
    double phi = omega * omega / 2.0 / mc->a_stop;         
    
    return phi / mc->phi_per_step;
}
/*! --------------------------------------------------------------------
 * @brief   motion diagram
 *          a new diagram is created
 */
struct _motion_diagram_ *new_md (struct _mot_ctl_ *mc)
{
    struct _motion_diagram_ *md = (struct _motion_diagram_ *) malloc (sizeof(struct _motion_diagram_));
    struct _move_point_ *mp = (struct _move_point_ *) malloc (sizeof(struct _move_point_));
    
    mp->omega = mp->t = mp->delta_phi = 0.0;        /* the first move point with t=0 */
    mp->a = 0.0;
    mp->delta_omega = mp->delta_t = mp->delta_phi = 0.0;
    mp->steps = mp->sum_steps = 0;
    mp->owner = md;

    mp->next = mp->prev = NULL;
    md->first_mp = md->last_mp = mp;

    md->max_omega = 0.5;
    md->min_omega = -0.5;
    md->max_t = 0.5;
    
    md->data_set_is_incorrect = 0;
    md->mc = mc;
    md->phi_all = 0.0;
    md->next = md->prev = NULL;    
    if (first_md == NULL) {
        first_md = last_md = md;    
    } else {
        last_md->next = md;
        md->prev = last_md;
        last_md = md;
    }
    
    return md;
}
/*! --------------------------------------------------------------------
 * @brief   a new motion diagram is created from file
 * @param   speedformat = [speed, FREQ, RPM]; see: enum SPEEDFORMAT 
 * @example     file (RPM Format / Line 1:  180 1/min  1,5s):
 *                  # Hz  s    Kommentar
 *                  180  1.5
 *                  240  3
 *                  50   5.5
 * @param   if speedformat == STEP the file format is FREQ and STEPS
 */ 
#define MAX_CHAR 1024
 
struct _motion_diagram_ *new_md_from_file (struct _mot_ctl_ *mc, const char *fname, uint8_t speedformat)
{
    FILE *f;
    struct _motion_diagram_ *md = NULL;
    float_t speed, t;    
    char str[MAX_CHAR];
    uint16_t n;
    
    if ((f = fopen (fname, "r+t")) == NULL) {
        printf ("-- File <%s> not found\n", fname);
        return NULL;
    }
    
    md = new_md (mc);
    while (fgets (str, MAX_CHAR, f)) {
        n=0;
        while (str[n] == ' ') n++;
        if (str[n] != '#') {
            int anz_arg;
            if ((anz_arg = sscanf (str, "%f %f\n", &speed, &t)) != 2) {
                if (anz_arg > 0) 
                    printf ("param count incorrekt: %s\n", str);
            } else {
                switch (speedformat) {
                    case OMEGA:
                        add_mp_omega (md, speed, t);    /* file format =  OMEGA[1/rad] and t[s] */
                        break;
                    case FREQ:
                        add_mp_Hz (md, speed, t);       /* file format =  FREQ[1/s] and t[s] */
                        break;
                    case RPM:
                        add_mp_rpm (md, speed, t);      /* file format =  RPM[1/min] and t[s] */
                        break;
                    case STEP:                        
                        add_mp_steps (md, speed, t);    /* file format =  FREQ[1/s] and STEPS */
                        break;
                }
            }                                    
        }
    }
    fclose (f);
    
    return md;
}
/*! --------------------------------------------------------------------
 * 
 */
extern int kill_md (struct _motion_diagram_ *md)
{
    if (md) {
        if (check_md_pointer(md) != EXIT_SUCCESS) 
            return EXIT_FAILURE;
    }
    
    if (!md) 
        return EXIT_FAILURE;
       
    if (md->mc != NULL) {
        if (md->mc->mc_mp != NULL) {          /* Engine is running with this motion diagram */
            printf ("-- kill failure\n");
            return EXIT_FAILURE;
        }
    }
        
    kill_all_mp (md);
    
    if (md->next != NULL) md->next->prev = md->prev;
    if (md->prev != NULL) md->prev->next = md->next;
    if (md == first_md) first_md = md->next;
    if (md == last_md) last_md = md->prev; 
    
    free (md);
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */
extern int kill_all_md (void)
{
    struct _motion_diagram_ *md = first_md;
    
    while (md != NULL) {
        if (md->mc != NULL) {
            if (md->mc->mode != MOT_IDLE) {
                printf ("-- Can't delete motion-diagram. Engine is running\n");
                return EXIT_FAILURE;
            }
        }
        md = md->next;
    }
    
    while (first_md != NULL) 
        kill_md (first_md);
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */
int count_md ()
{
    struct _motion_diagram_ *md = first_md;
    int n = 0;
    
    while (md != NULL) {
        n++;
        md = md->next;
    }
    return n;
}
/*! --------------------------------------------------------------------
 * @brief   show diagram point
 */
int show_mp (struct _move_point_ *mp)
{
    if (!mp)
        return EXIT_FAILURE;

    if (mp->prev) printf ("prev->omega_0=%4.3f  ", mp->prev->omega);
    else printf ("omega_0=NIL     ");
    
    printf ("omega=%4.3f  delta_omega=%4.3f  delta_t=%2.4f  delta_phi = %2.3f  steps=%llu  sum_steps=%llu  a=%4.3f\n", 
             mp->omega,
             mp->delta_omega, 
             mp->delta_t,              
             mp->delta_phi,
             (long long unsigned) mp->steps,
             (long long unsigned) mp->sum_steps,
             mp->a);
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   Checks whether a record exists.
 */
int check_md_pointer (struct _motion_diagram_ *md)
{
    struct _motion_diagram_ *m = first_md;
    
    while (m) {
        if (m == md) 
            return EXIT_SUCCESS;
        m = m->next;
    }
    
    return EXIT_FAILURE;
}
/*! --------------------------------------------------------------------
 * @brief   show diagram points
 */
int show_md (struct _motion_diagram_ *md)
{
    if (!md)
        return EXIT_FAILURE;
        
    struct _move_point_ *mp = md->first_mp;
    
    while (mp) {
        show_mp (mp);       /* show diagram point */
        mp = mp->next;
    }
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  Delete the motor pointer in the motion_diagram dataset.
 *          Used by kill_mc()
 */
void clear_mc_in_md (struct _mot_ctl_ *mc)
{
    struct _motion_diagram_ *md = first_md;
    
    while (md != NULL) {
        if (md->mc == mc) 
            md->mc = NULL;
        md = md->next;
    }
}
/*! --------------------------------------------------------------------
 * æbrief   write motion data to a file
 */
int gnuplot_write_graph_data_file (struct _motion_diagram_ *md, const char *fname)
{
    FILE *data;
    char buf[256];
    uint64_t sum_step = 0;
    
    if ((data = fopen (fname, "w+t")) == 0) {
        printf ("-- Can't open %s\n", fname);
        return EXIT_FAILURE;
    }
    
    struct _move_point_ *mp = md->first_mp;
    
    sprintf (buf, "# x=t[s]   y=omega[s^-1]   steps\n");    
    fwrite (buf, strlen(buf), 1, data);
    while (mp) {          
        sum_step += mp->steps;
        sprintf (buf, "%3.4f  %3.4f   %llu-Steps\n", 
                      mp->t, 
                      mp->omega/(2.0*M_PI), 
                      (long long unsigned) sum_step);
        fwrite (buf, strlen(buf), 1, data);
        
        mp = mp->next;
    }
    fclose (data);
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   display motion diagram with gnupolt
 *           install gnuplot with: 
 *           sudo apt-get install gnuplot gnuplot-x11 gnuplot-doc 
 */
 
#define STRETCH_FAKTOR 1.3

int gnuplot_md (struct _motion_diagram_ *md)
{
    FILE *gp;    
        
    if (md != NULL) {        
        if (check_md_pointer(md) != EXIT_SUCCESS) {              /* check parameter md */
            printf ("-- Can't find motion-diagram\n");
            return EXIT_FAILURE;
        }
    } else {
        printf ("-- diagram pointer is empty\n");
        return EXIT_FAILURE;
    }
    
        
    if (gnuplot_write_graph_data_file (md, "graph.txt") != EXIT_SUCCESS) {      /* write motion data to a file */
        printf ("-- Can't write diagram data to graph.txt\n");
        return EXIT_FAILURE;
    }    
        
    if ((gp = popen("gnuplot -p" , "w")) == NULL) {
        printf ("-- Can't open gnuplot\n");
        return EXIT_FAILURE;
    }
        
    fprintf (gp, "reset\n");
    fprintf (gp, "set term x11\n");
    
    fprintf (gp, "set title \"motion diagram\" font \", 16\"\n");

    fprintf (gp, "set xlabel \"t[s]\"\n");
    fprintf (gp, "set ylabel \"f[s^-1]\"\n");
    
    fprintf (gp, "set xzeroaxis lt 2 lw 1 lc rgb \"#FF0000\"\n");
    fprintf (gp, "set yzeroaxis lt 2 lw 1 lc rgb \"#FF0000\"\n");
    
    fprintf (gp, "set yrange[%3.4f:%3.4f]\n",  md->min_omega/(2.0*M_PI) * STRETCH_FAKTOR, md->max_omega/(2.0*M_PI) * STRETCH_FAKTOR);    
    fprintf (gp, "set xrange[-0.5:%4.3f]\n", md->max_t * STRETCH_FAKTOR);
    
    fprintf (gp, "set label 1 \"CW\" at graph -0.05, 1.05, 0 left\n");
    fprintf (gp, "set label 2 \"CCW\" at graph -0.05, -0.05, 0 left\n");
        
    fprintf (gp, "plot \"graph.txt\" w linespoints lw 3 lc rgb \"#0000FF\"  pt 7 ps 3, \
                  '' w labels left offset 2.0, 1.0 notitle\n");
            
    pclose (gp);
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  add an item to the end of the list
 */
struct _move_point_ *add_mp_Hz (struct _motion_diagram_ *md, double Hz, double t)
{    
    if (md == NULL)
        return NULL;
        
    if (t < md->last_mp->t) {                               /* negative time */
        md->data_set_is_incorrect = 1;
        printf ("-- ERROR: negative time \n");
        return (NULL);
    }
        
    double omega = 2.0 * M_PI * Hz;
    if (((md->last_mp->omega > 0.0) && (omega < 0.0)) || 
        ((md->last_mp->omega < 0.0) && (omega > 0.0))) {    /* zero crossing */
            double dw = fabs(omega - md->last_mp->omega);
			double dt = t - md->last_mp->t;
            double nt = fabs(md->last_mp->omega) * (dt / dw);
            
			add_mp_omega (md, 0.0, md->last_mp->t + nt);    /* include zero crossing motion point */
    }
        
    struct _move_point_ *mp = (struct _move_point_ *) malloc (sizeof(struct _move_point_));    
    
    mp->omega = omega;
    mp->t = t;
        
    mp->delta_t = mp->t - md->last_mp->t;        
    mp->delta_omega = mp->omega - md->last_mp->omega;
    mp->a = (mp->delta_t > 0.0) ? mp->delta_omega / mp->delta_t : 0.0;    
    mp->delta_phi = (md->last_mp->omega + mp->omega) / 2.0 * mp->delta_t;
        
    mp->steps = round(fabs(mp->delta_phi) / md->mc->phi_per_step);      
        
    mp->owner = md;
    mp->owner->phi_all += mp->delta_phi;
    mp->phi = mp->owner->phi_all;
    
    if (mp->omega > md->max_omega)      
        md->max_omega = mp->omega;
    if (mp->omega < md->min_omega) 
        md->min_omega = mp->omega;
    if (t > md->max_t) md->max_t = t;
    
    mp->next = mp->prev = NULL;
    if (md->first_mp == NULL) {        
        md->first_mp = md->last_mp = mp;
    } else {        
        md->last_mp->next = mp;
        mp->prev = md->last_mp;
        md->last_mp = mp;       
    }
    
    mp->sum_steps = mp->prev->sum_steps + mp->steps;
    
    return mp;
}
/*! --------------------------------------------------------------------
 * @brief   add an item to the end of the list
 */
struct _move_point_ *add_mp_omega (struct _motion_diagram_ *md, double omega, double t)
{
    return add_mp_Hz (md, omega / 2.0 / M_PI, t);
}
/*! --------------------------------------------------------------------
 * @brief   add an item to the end of the list
 */
struct _move_point_ *add_mp_rpm (struct _motion_diagram_ *md, double rpm, double t)
{
    return add_mp_Hz (md, rpm / 60.0, t);
}
/*! --------------------------------------------------------------------
 * @brief   add an item to the end of the list
 * @param   Hz = speed [1/s]
 *           steps = number of steps
 */
struct _move_point_ *add_mp_steps (struct _motion_diagram_ *md, double Hz, double steps)
{
    if (md == NULL)
        return NULL;
    
    double omega = 2.0*M_PI*Hz;
    if (((omega > 0.0) && (md->last_mp->omega < 0.0)) || 
        ((omega < 0.0) && (md->last_mp->omega > 0.0))) {
        double dw = omega - md->last_mp->omega;
        double ds = steps - md->last_mp->sum_steps;
        double ns = ds / fabs(dw) * md->last_mp->omega;
        
        add_mp_steps (md, 0.0, md->last_mp->sum_steps + ns);
    }
        
    double phi = md->mc->phi_per_step * (steps - md->last_mp->sum_steps);
    double sum_omega = md->last_mp->omega + omega;
    double t = (sum_omega == 0.0) ? 0.0 : fabs(2.0 * phi / sum_omega);
    t += md->last_mp->t;
    
    if (t < md->last_mp->t) {                               /* negative time */
        md->data_set_is_incorrect = 1;
        printf ("-- ERROR: negative time \n");
        return (NULL);
    }
    
    return add_mp_omega (md, omega, t);
}
/*! --------------------------------------------------------------------
 * @brief   delete move point in motion diagram
 */
int kill_mp (struct _move_point_ *mp)
{
    if (!mp) 
        return EXIT_FAILURE;
        
    if (mp->next != NULL) mp->next->prev = mp->prev;
    if (mp->prev != NULL) mp->prev->next = mp->next;    
    if (mp == mp->owner->first_mp) mp->owner->first_mp = mp->next;
    if (mp == mp->owner->last_mp) mp->owner->last_mp = mp->prev;
        
    free (mp);    
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   delete all move points off motion diagram
 */
int kill_all_mp (struct _motion_diagram_ *md)
{
    if (!md) 
        return EXIT_FAILURE;
        
    while (md->first_mp != NULL) {        
        kill_mp (md->first_mp);
    }
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */
extern int count_mp (struct _motion_diagram_ *md)
{
    int n = 0;
    
    if (!md)
        return EXIT_FAILURE;    
        
    struct _move_point_ *mp = md->first_mp;
    while (mp) {
        n++;
        mp = mp->next;
    }
    return n;
}
