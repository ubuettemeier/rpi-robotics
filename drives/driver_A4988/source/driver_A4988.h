/*! --------------------------------------------------------------------
 *  @file    driver_A4988.h
 *  @date    10.13.2018
 *  @name    Ulrich Buettemeier
 */

#include <stdint.h>
#include <sys/time.h>
// #include <linux/types.h>

enum {
    MOT_CW = 0,
    MOT_CCW = 1
};

enum {
    MOT_IDLE = 0x00,
    MOT_STARTRUN = 0x01,
    MOT_RUN = 0x02,
    MOT_JOBREADY = 0x04
};

struct _thread_state_ {          /* thread state */
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
    unsigned endless : 1;       /* Motor runs forever, void mot_stop() stops the run. */
};

struct _mot_ctl_ {             /* motor control */
    struct _mot_flags_ flag;   
    
    uint8_t mode;               /* used in mot_run function. */
    uint32_t steps_per_turn;    /* step per revolution */
    
    int64_t max_latency;        
    uint64_t num_steps;         /* if num_step == 0 the motor runs endless */
    uint64_t num_rest;
    uint64_t current_stepcount;
    int64_t runtime; 
    
    int steptime;                /* time in us */
    
    struct timeval start, stop;    
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

extern void mot_initpins (struct _mot_ctl_ *mc);         /* configures motor gpio  */
extern int mot_setparam (struct _mot_ctl_ *mc,
                          uint8_t dir,
                          uint64_t steps);
extern int mot_start (struct _mot_ctl_ *mc);
extern int mot_stop (struct _mot_ctl_ *mc);

extern int mot_switch_enable (struct _mot_ctl_ *mc, uint8_t enable);    /* set chip enable/disenable */
extern int mot_enable (struct _mot_ctl_ *mc);
extern int mot_disenable (struct _mot_ctl_ *mc);

extern int mot_set_dir (struct _mot_ctl_ *mc, uint8_t direction);       /* set the direction */

extern int mot_set_steptime (struct _mot_ctl_ *mc, int steptime);       /* set speed in [time per step] */
extern int mot_set_rpm (struct _mot_ctl_ *mc, double rpm);              /* set speed in rpm */
extern int mot_set_Hz (struct _mot_ctl_ *mc, double Hz);                /* set speed in s^-1 */
