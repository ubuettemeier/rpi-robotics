/*! --------------------------------------------------------------------
 *  @file    driver_A4988.h
 *  @date    10-13-2018
 *  @name    Ulrich Buettemeier
 */

#include <math.h>
#include <stdint.h>
#include <sys/time.h>

enum SPEEDFORMAT {
    OMEGA = 0,          /* rad/s */
    FREQ = 1,           /* s⁻1 */
    RPM = 2,            /* min⁻1 */
    STEP = 3            /* number of steps */
};

enum DIRECTION {
    MOT_CW = 0,
    MOT_CCW = 1
};

enum MOT_STATE {                /* see: function mot_run() */
    MOT_IDLE = 0x00,
    MOT_START_RUN = 0x01,    
    MOT_SPEED_UP = 0x02,
    MOT_RUN_SPEED_UP = 0x03,
    MOT_RUN = 0x04,
    MOT_SPEED_DOWN = 0x08,
    MOT_RUN_SPEED_DOWN = 0x10,
    
    MOT_START_MD = 0x20,
    MOT_RUN_MD = 0x021,
    MOT_RUN_SPEED_MD = 0x22,
    
    MOT_JOB_READY = 0x80
};

struct _thread_state_ {         /* thread state => see: void *run_A4988() */
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
    
    uint8_t mode;               /* used in mot_run function. see: enum MOT_STATE */
    uint32_t steps_per_turn;    /* steps per revolution */
    
    uint64_t max_latency;        
    int64_t num_steps;          /* num_step < 0 parameter failed, num_step == 0 the motor runs endless */
    uint64_t num_rest;
    uint64_t current_stepcount; /* Current number of steps */
    int64_t real_stepcount;     /* if CW inc(), if CCW dec() */

    uint64_t runtime;           /* full running time in us */
    
    uint32_t steptime;          /* steptime in us. Default 2000 us */ 
    uint32_t current_steptime;
    double phi_per_step;        /* angle per step [rad] */
    double omega;               /* angle-speed{rad/s] */
    double current_omega;       /* current angle-speed{rad/s] */
    double a_start, a_stop;     /* spped-up[s⁻2], speed-down[s⁻2] */
    
    struct _move_point_ *mc_mp;     /* Motion Point default = NULL; for define use function mot_start_md()  */
    
    struct timeval start, stop, run_start;
    struct _mot_pin_ mp;       /* motor gpio-pins */
    
    struct _mot_ctl_ *next, *prev;
};

extern struct _mot_ctl_ *first_mc, *last_mc; 
/*! --------------------------------------------------------------------
 * Motion Diagram
 */
struct _motion_diagram_ {
    struct _mot_ctl_ *mc;                       /* motor-pointer */
    double phi_all;
    uint8_t data_set_is_incorrect;               /* default = 0. See: add_mp() */
    double max_omega, min_omega;
    double max_t;
    struct _move_point_ *first_mp, *last_mp;    /* first and last move point of motion diagramm */
    struct _motion_diagram_ *next, *prev;
};

extern struct _motion_diagram_ *first_md, *last_md;

struct _move_point_ {
    double omega;           /* angle-speed[rad/s] */
    double t;               /* [s] t >=  prev->t */
    double a;               /* angle acceleration */
    double phi;
    uint64_t steps;
    uint64_t current_step;
    uint64_t sum_steps;
    
    double delta_omega;     /* omega - prev->omega */
    double delta_t;         /* t - prev->t */
    double delta_phi;       /* angle phi ??? */      
    
    struct _motion_diagram_ *owner;
    struct _move_point_ *next, *prev;
};
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
extern int check_mc_pointer (struct _mot_ctl_ *mc);
extern void show_mot_ctl (struct _mot_ctl_ *mc);

extern void mot_initpins (struct _mot_ctl_ *mc);        /* configures motor gpio  */

extern int mot_setparam (struct _mot_ctl_ *mc,        /* mc = motor handle */
                          uint8_t dir,                  /* dir==0 CW, dir==1 CCW */
                          uint64_t num_steps,           /* steps == 0 motor runs endless */
                          double a_start,               /* alpha Start [s⁻2] */
                          double a_stop);               /* alpha stop [s⁻2] */
                          
extern int mot_start (struct _mot_ctl_ *mc);
extern int mot_stop (struct _mot_ctl_ *mc);
extern int mot_fast_stop (struct _mot_ctl_ *mc);                 /* Engine stopping without ramp. */
extern int mot_on_step (struct _mot_ctl_ *mc, uint8_t dir);     /* dir==0 CW, dir==1 CCW */

extern int mot_start_md (struct _motion_diagram_ *md);                  /* Engine start. The motor follows the motion diagram. */

extern int mot_switch_enable (struct _mot_ctl_ *mc, uint8_t enable);    /* set chip enable/disenable */
extern int mot_enable (struct _mot_ctl_ *mc);
extern int mot_disenable (struct _mot_ctl_ *mc);

extern int mot_set_dir (struct _mot_ctl_ *mc, uint8_t direction);       /* set the direction */

extern int mot_set_steptime (struct _mot_ctl_ *mc, int steptime);       /* set speed in time per step [us] */
extern int mot_set_rpm (struct _mot_ctl_ *mc, double rpm);              /* set speed rpm [min⁻1] */
extern int mot_set_Hz (struct _mot_ctl_ *mc, double Hz);                /* set speed f [s⁻1] */

/*! --------------------------------------------------------------------
 * @brief   calculation functions
 */
extern double calc_omega (uint32_t steps_per_turn, uint32_t steptime);  /* function for calculation of angle speed */
extern double calc_steps_for_step_down (struct _mot_ctl_ *mc);

/*! --------------------------------------------------------------------
 * @brief   motion diagram
 */
extern struct _motion_diagram_ *new_md (struct _mot_ctl_ *mc);      /* A new diagram is created. */
extern struct _motion_diagram_ *new_md_from_file (struct _mot_ctl_ *mc, const char *fname, uint8_t speedformat);    /* speedformat see: enum SPEEDFORMAT */
extern int kill_md (struct _motion_diagram_ *md);
extern int kill_all_md (void);
extern int count_md (void);
extern int check_md_pointer (struct _motion_diagram_ *md);          /* Checks whether a record exists. */
extern int show_md (struct _motion_diagram_ *md);                   /* Terminal output. Show all point's */
extern void clear_mc_in_md (struct _mot_ctl_ *mc);                   /* Delete the motor pointer in the motion_diagram dataset. Used by kill_mc */

/* ---- motion points ---- */
extern struct _move_point_ *add_mp_Hz (struct _motion_diagram_ *md, double Hz, double t);          /* add an item to the end of the list */
extern struct _move_point_ *add_mp_omega (struct _motion_diagram_ *md, double omega, double t);
extern struct _move_point_ *add_mp_rpm (struct _motion_diagram_ *md, double rpm, double t);

extern struct _move_point_ *add_mp_steps (struct _motion_diagram_ *md, double Hz, double steps);

extern int kill_mp (struct _move_point_ *mp);                        /* delete move point in motion diagram */
extern int kill_all_mp (struct _motion_diagram_ *md);                /* delete all move points off motion diagram */
extern int count_mp (struct _motion_diagram_ *md);
extern int show_mp (struct _move_point_ *mp);                        /* terminal output */

/* ---- draw motion diagram ---- */
extern int gnuplot_md (struct _motion_diagram_ *md);                /* display motion diagram with gnupolt */
extern int gnuplot_write_graph_data_file (struct _motion_diagram_ *md, const char *fname);  /* write motion data to a file */
