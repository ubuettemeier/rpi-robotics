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

pthread_t thread_A4988 = NULL;                         /* glob thread handle */
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
    } else return (EXIT_FAILURE);
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  used by mot_run()
 */
static int64_t execute_step (struct _mot_ctl_ *mc, int64_t timediff)
{
    int64_t latency;
    
    gettimeofday (&mc->start, NULL);    /* set new time */   
    mot_step (mc);                      /* Execute step */
    mc->current_stepcount++;            /* Increase step counter */
    mc->runtime = difference_micro (&mc->run_start, &mc->stop); 

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
 * @brief  used by driver thread run_A4988()
 */
static int mot_run (struct _mot_ctl_ *mc)
{
    int64_t timediff;
    static int64_t latency; 
    
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
            if ((timediff = difference_micro (&mc->start, &mc->stop)) >= (mc->current_steptime - latency)) {    /* Execute step */
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
                mc->mc_mp->current_step++;   
                if (mc->mc_mp->current_step > mc->mc_mp->steps) {   /* end of MOT_RUN_MD */
                    mc->mode = MOT_START_MD;                    
                } else {                          
                    double t;
                    double new_omega;
                    if (mc->mc_mp->a == 0.0) {                     /* omega const. */
                        new_omega = mc->mc_mp->omega;
                        
                        if (new_omega < 0.0) mot_set_dir (mc, MOT_CCW);
                        else mot_set_dir (mc, MOT_CW);
                        
                        t = mc->phi_per_step / new_omega;
                    } else {                        
                        double faktor = 1.0;                        /* CW */
                        if ((mc->current_omega < 0.0) ||            /* CCW */
                            ((mc->current_omega == 0.0) && (mc->mc_mp->a < 0.0))) {
                            faktor = -1.0;   
                        }
                        new_omega = sqrt((mc->current_omega * mc->current_omega) + 2.0*(mc->mc_mp->a)*faktor*mc->phi_per_step) * faktor;
                        
                        if (new_omega < 0.0) mot_set_dir (mc, MOT_CCW);
                        else mot_set_dir (mc, MOT_CW);
                        
                        t = (2.0 * faktor*mc->phi_per_step) / (mc->current_omega + new_omega);
                    }                    
                        
                    mc->current_steptime = fabs(t) * 1000000.0;
                    mc->current_omega = new_omega;                    
                    latency = 0;
                    
                    gettimeofday (&mc->start, NULL);        
                    mc->mode = MOT_RUN_SPEED_MD;
                }
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
            printf ("-- max_latency=%lli us  current_stepcount=%llu  runtime=%lli us\n", mc->max_latency, mc->current_stepcount, mc->runtime);            
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
    
    while (!thread_state.kill) {
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
        
        if (all_mot_idle) {
            usleep (1000); 
        }
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
        printf ("wiringPiSetup failed !\n");
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
                printf("go() Thread CPU = CPU %d\n", i);
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
        printf ("parameter num_steps failed\n");
        return (EXIT_FAILURE);
    }
    
    if (mc->mode != MOT_IDLE) {
        printf ("Can't start motor\n");
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
 * @brief   Engine start. The motor follows the motion diagram.
 */ 
int mot_start_md (struct _motion_diagram_ *md)
{
    if (md) {
        if (grep_md(md) != EXIT_SUCCESS) {      /* check md */
            printf ("Data set not found\n");
            return EXIT_FAILURE;
        }
    }
    
    if (!md || !md->mc)
        return EXIT_FAILURE;
    
    if (md->data_set_is_incorrect) {
        printf ("Data set is incorrect. ERROR No.: %i\n", md->data_set_is_incorrect);
        return EXIT_FAILURE;
    }    
    
    if (md->mc->mode != MOT_IDLE) {
        printf ("Can't start motorprogram\n");
        return EXIT_FAILURE;
    }
    
    mot_enable (md->mc);
    md->mc->max_latency = 0;
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
 */
struct _motion_diagram_ *new_md (struct _mot_ctl_ *mc)
{
    struct _motion_diagram_ *md = (struct _motion_diagram_ *) malloc (sizeof(struct _motion_diagram_));
    struct _move_point_ *mp = (struct _move_point_ *) malloc (sizeof(struct _move_point_));
    
    mp->omega = mp->t = mp->delta_phi = 0.0;        /* the first move point with t=0 */
    mp->a = 0.0;
    mp->delta_omega = mp->delta_t = mp->delta_phi = 0.0;
    mp->steps = 0;
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
 * 
 */
extern int kill_md (struct _motion_diagram_ *md)
{
    if (md) {
        if (grep_md(md) != EXIT_SUCCESS) 
            return EXIT_FAILURE;
    }
    
    if (!md) 
        return EXIT_FAILURE;
        
    if (md->mc->mc_mp != NULL) {          /* Engine is running with this motion diagram */
        printf ("kill failure\n");
        return EXIT_FAILURE;
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
        if (md->mc->mode != MOT_IDLE) {
            printf ("Can't delete motion-diagram. Engine is running\n");
            return EXIT_FAILURE;
        }
        md = md->next;
    }
    
    while (first_md != NULL) 
        kill_md (first_md);
        
    return EXIT_SUCCESS;
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
    
    printf ("omega=%4.3f  delta_omega=%4.3f  delta_t=%2.4f  delta_phi = %2.3f  steps=%llu  a=%4.3f\n", 
             mp->omega,
             mp->delta_omega, 
             mp->delta_t,              
             mp->delta_phi,
             mp->steps,
             mp->a);
        
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   Checks whether a record exists.
 */
int grep_md (struct _motion_diagram_ *md)
{
    struct _motion_diagram_ *m = first_md;
    
    while (m) {
        if (m == md) 
            return EXIT_SUCCESS;
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
 * æbrief   write motion data to a file
 */
int gnuplot_write_graph_data_file (struct _motion_diagram_ *md, const char *fname)
{
    FILE *data;
    char buf[256];
    
    if ((data = fopen (fname, "w+t")) == 0) {
        printf ("Can't open %s\n", fname);
        return EXIT_FAILURE;
    }
    
    struct _move_point_ *mp = md->first_mp;
    
    sprintf (buf, "# x=t[s]   y=omega[s^-1]\n");    
    fwrite (buf, strlen(buf), 1, data);
    while (mp) {                    
        sprintf (buf, "%3.4f  %3.4f\n", mp->t, mp->omega);
        fwrite (buf, strlen(buf), 1, data);
        
        mp = mp->next;
    }
    fclose (data);
    
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   display motion diagram with gnupolt
 */
int gnuplot_md (struct _motion_diagram_ *md)
{
    FILE *gp;    
    
    if (gnuplot_write_graph_data_file (md, "graph.txt") != EXIT_SUCCESS) {      /* write motion data to a file */
        printf ("Can't write diagram data to graph.txt\n");
        return EXIT_FAILURE;
    }    
        
    if ((gp = popen("gnuplot -p" , "w")) == NULL) {
        printf ("Can't open gnuplot\n");
        return EXIT_FAILURE;
    }
        
    fprintf (gp, "reset\n");
    fprintf (gp, "set term x11\n");

    fprintf (gp, "set xlabel %ct[s]%c\n", '"', '"');
    fprintf (gp, "set ylabel %comega[s^-1]%c\n", '"', '"');
    
    fprintf (gp, "set xzeroaxis lt 2 lw 1 lc rgb %c#FF0000%c\n", '"', '"');
    fprintf (gp, "set yzeroaxis lt 2 lw 1 lc rgb %c#FF0000%c\n", '"', '"');
    
    fprintf (gp, "set yrange[%3.4f:%3.4f]\n",  md->min_omega * 1.2, md->max_omega * 1.2);    
    fprintf (gp, "set xrange[-0.5:%4.3f]\n", md->max_t * 1.2);
    
    fprintf (gp, "plot %cgraph.txt%c with linespoints lw 3 lc rgb %c#0000FF%c  pt 7 ps 3\n", '"', '"', '"', '"');
            
    pclose (gp);
    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief   add an item to the end of the list
 */
struct _move_point_ *add_mp (struct _motion_diagram_ *md, double Hz, double t)
{    
    if (md == NULL)
        return NULL;
        
    if (t < md->last_mp->t) {                               /* negative time */
        md->data_set_is_incorrect = 1;
        printf ("ERROR: negative time \n");
        return (NULL);
    }
        
    double omega = 2.0 * M_PI * Hz;
    if (((md->last_mp->omega > 0.0) && (omega < 0.0)) || 
        ((md->last_mp->omega < 0.0) && (omega > 0.0))) {    /* zero passage */           
            md->data_set_is_incorrect = 2;
            printf ("ERROR: zero passage \n");
            return (NULL);
    }
        
    struct _move_point_ *mp = (struct _move_point_ *) malloc (sizeof(struct _move_point_));    
    
    mp->omega = omega;
    mp->t = t;
        
    mp->delta_t = mp->t - md->last_mp->t;        
    mp->delta_omega = mp->omega - md->last_mp->omega;
    mp->a = (mp->delta_t > 0.0) ? mp->delta_omega / mp->delta_t : 0.0;    
    mp->delta_phi = (md->last_mp->omega + mp->omega) / 2.0 * mp->delta_t;
        
    mp->steps = fabs(mp->delta_phi) / md->mc->phi_per_step;
        
    mp->owner = md;
    mp->owner->phi_all += mp->delta_phi;
    mp->phi = mp->owner->phi_all;
    
    if (mp->omega > md->max_omega) 
        md->max_omega = mp->omega;
    if (mp->omega < md->min_omega) 
        md->min_omega = mp->omega;
    md->max_t = t;
    
    mp->next = mp->prev = NULL;
    if (md->first_mp == NULL) {        
        md->first_mp = md->last_mp = mp;
    } else {        
        md->last_mp->next = mp;
        mp->prev = md->last_mp;
        md->last_mp = mp;       
    }
    
    return mp;
}
/*! --------------------------------------------------------------------
 * @brief   add an item to the end of the list
 */
struct _move_point_ *add_mp_with_omega (struct _motion_diagram_ *md, double omega, double t)
{
    return add_mp (md, omega / 2.0 / M_PI, t);
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
