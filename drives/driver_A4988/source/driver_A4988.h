/*! --------------------------------------------------------------------
 *  @file    driver_A4988.h
 *  @date    10-13-2018
 *  @name    Ulrich Buettemeier
 */

#include <math.h>
#include <stdint.h>
#include <sys/time.h>

enum DIRECTION {
    MOT_CW = 0,
    MOT_CCW = 1
};

enum MOT_STATE {
    MOT_IDLE = 0x00,
    MOT_START_RUN = 0x01,    
    MOT_SPEED_UP = 0x02,
    MOT_RUN_SPEED_UP = 0x03,
    MOT_RUN = 0x04,
    MOT_SPEED_DOWN = 0x08,
    MOT_RUN_SPEED_DOWN = 0x10,
    MOT_JOB_READY = 0x20
};

struct _thread_state_ {         /* thread state */
    unsigned run: 1;
    unsigned mc_closed: 1;
    unsigned kill: 1;
} extern thread_state;

struct _mot_pin_ {             /* motor gpio-pins */
    uint8_t dir_pin;
    uint8_t step_pin; 
    uint8_t enable_pin;
};

struct _mot_flags_ {
    unsigned dir : 1;           /* Direction of rotation. CW=0, CCW=1 */
    unsigned enable : 1;        /* chip enable */
    unsigned endless : 1;       /* motor runs forever, void mot_stop() stops the run. */
    unsigned aktiv : 1;         /* motor running */
};

struct _mot_ctl_ {             /* motor control */
    struct _mot_flags_ flag;   
    
    uint8_t mode;               /* used in mot_run function. */
    uint32_t steps_per_turn;    /* steps per revolution */
    
    int64_t max_latency;        
    int64_t num_steps;          /* num_step < 0 parameter failed, num_step == 0 the motor runs endless */
    uint64_t num_rest;
    uint64_t current_stepcount; /* Current number of steps */

    int64_t runtime;            /* full running time in us */
    
    uint32_t steptime;          /* steptime in us. Default 2000 us */ 
    uint32_t current_steptime;
    double phi_per_step;   
    double omega;               /* angle-speed{s⁻1] */
    double current_omega;       /* current angle-speed{s⁻1] */
    double a_start, a_stop;     /* spped-up[s⁻2], speed-down[s⁻2] */
    
    struct timeval start, stop, run_start;
    struct _mot_pin_ mp;        /* motor gpio-pins */
    
    struct _mot_ctl_ *next, *prev;
};

extern struct _mot_ctl_ *first_mc, *last_mc; 

/*! --------------------------------------------------------------------
 * 
 */
extern int init_mot_ctl(void);                            /* Initializes the driver thread */

extern struct _mot_ctl_ *new_mot (uint8_t pin_enable,   /* create dynamic memory for motor parameter */
                                     uint8_t pin_dir,
                                     uint8_t pin_step,
                                     uint32_t steps_per_turn);
                                     
extern int kill_mot (struct _mot_ctl_ *mc);
extern int kill_all_mot (void);
extern int count_mot (void);

extern void mot_initpins (struct _mot_ctl_ *mc);        /* configures motor gpio  */

extern int mot_setparam (struct _mot_ctl_ *mc,          /* mc = motor handle */
                          uint8_t dir,                  /* dir==0 CW, dir==1 CCW */
                          uint64_t steps,               /* steps == 0 motor runs endless */
                          double a_start,               /* alpha Start [s⁻2] */
                          double a_stop);               /* alpha stop [s⁻2] */
                          
extern int mot_start (struct _mot_ctl_ *mc);
extern int mot_stop (struct _mot_ctl_ *mc);

extern int mot_switch_enable (struct _mot_ctl_ *mc, uint8_t enable);    /* set chip enable/disenable */
extern int mot_enable (struct _mot_ctl_ *mc);
extern int mot_disenable (struct _mot_ctl_ *mc);

extern int mot_set_dir (struct _mot_ctl_ *mc, uint8_t direction);       /* set the direction */

extern int mot_set_steptime (struct _mot_ctl_ *mc, int steptime);       /* set speed in [time per step] */
extern int mot_set_rpm (struct _mot_ctl_ *mc, double rpm);              /* set speed in rpm */
extern int mot_set_Hz (struct _mot_ctl_ *mc, double Hz);                /* set speed in s^-1 (Omega) */

/*! --------------------------------------------------------------------
 * @brief   Help functions
 */
extern double calc_omega (uint32_t steps_per_turn, uint32_t steptime);  /* function for calculation of angular speed */
extern double calc_steps_for_step_down (struct _mot_ctl_ *mc);
