/*! ---------------------------------------------------------------------
 * @file    hc_sr04.c
 * @date    09.16.2018
 * @name    Ulrich Buettemeier
 */
 
#include <stdint.h> 
 
#define TIMEOUT 30000   /* timeout = 30ms */

struct _hc_sr04_ {
  int trig_pin, echo_pin;
  uint8_t on;                  /* default 1 = on */
  uint16_t id;                 /* sensor identifire number */
  uint32_t hc_sr04_run_time;   /* 0=fail measurement */ 
  uint32_t hc_sr04_dist_mm;    /* 0=fail measurement */
  uint32_t last_hc_sr04_run_time;   /* can used by owener application. default = 1 */
  struct _hc_sr04_ *next, *prev;
};

extern pthread_t hc_sr04_thread;                         /* thread handle */
extern struct _hc_sr04_ *first_hc_sr04, *end_hc_sr04;   /* glob sensor list pointer */
extern pthread_mutex_t hc_sr04_mutex;                   /* glob mutex */

/*! --------------------------------------------------------------------
 *  @param  pin_trig = wiringPi Pin No. for trigger signal
 *           pin_echo = wiringPi Pin No. for echo signal
 */
extern struct _hc_sr04_ *new_hc_sr04 (uint8_t pin_trig, uint8_t pin_echo);   /* Insert a new sensor. */
extern int delete_hc_sr04 (struct _hc_sr04_ *sensor);
extern int delete_all_hc_sr04 (void);
extern int count_hc_sr04 (void);

extern void start_hc_04_thread (void);                               /* start measurement thread */
extern void stop_hc_04_thread (void);                                /* stop measurement thread */

extern int get_dist (struct _hc_sr04_ *sen);
extern int while_echo (struct _hc_sr04_ *sen, uint8_t state);
extern void *run_hc_sr04 (void *data);                               /* measurement main thread */



