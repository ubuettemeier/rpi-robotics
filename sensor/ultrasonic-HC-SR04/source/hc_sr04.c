/*! --------------------------------------------------------------------
 * @file    hc_sr04.c
 * @date    09-16-2018
 * @name    Ulrich Buettemeier
 * @brief   program use wiringPi
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <pthread.h>

#include "hc_sr04.h"

struct _hc_sr04_ *first_hc_sr04 = NULL, *end_hc_sr04 = NULL;   /* pointer to sensor list */

pthread_mutex_t hc_sr04_mutex;            /* glob mutex */
pthread_t hc_sr04_thread = NULL;          /* glob thread handle */

uint8_t hc_sr04_end = 0;     
uint16_t akt_id = 0;                       /* counter for sensor identifire number */

/*! --------------------------------------------------------------------
 *  @param  pin_trig = wiringPi Pin No. for trigger signal
 *           pin_echo = wiringPi Pin No. for echo signal
 */
struct _hc_sr04_ *new_hc_sr04 (uint8_t pin_trig, uint8_t pin_echo)
{
  if( wiringPiSetup() < 0) {
    printf ("wiringPiSetup failed !\n");
    return NULL;
  }    
    
  struct _hc_sr04_ *sen = (struct _hc_sr04_ *) malloc (sizeof(struct _hc_sr04_));
  
  sen->on = 1;
  sen->id = ++akt_id;
  sen->trig_pin = pin_trig;
  sen->echo_pin = pin_echo;
  
  sen->hc_sr04_run_time = 0;
  sen->hc_sr04_dist_mm = 0;
  sen->last_hc_sr04_run_time = 1;
  
  pinMode (sen->trig_pin, OUTPUT);    /* init gpio pins for this hc_sr04 */
  pinMode (sen->echo_pin, INPUT);
  pullUpDnControl (sen->echo_pin, PUD_UP);  /* pull-up resistor */
  digitalWrite (sen->trig_pin, 0);	
  
  sen->next = sen->prev = NULL;
  if (first_hc_sr04 == NULL) first_hc_sr04 = end_hc_sr04 = sen;
  else {
    end_hc_sr04->next = sen;
    sen->prev = end_hc_sr04;
    end_hc_sr04 = sen;
  }  
    
  return sen;
}
/*! --------------------------------------------------------------------
 * 
 */
int delete_hc_sr04 (struct _hc_sr04_ *sensor)
{
    if (sensor == NULL) 
        return EXIT_FAILURE;

    if (sensor->next != NULL) sensor->next->prev = sensor->prev;
    if (sensor->prev != NULL) sensor->prev->next = sensor->next;
    if (sensor == first_hc_sr04) first_hc_sr04 = sensor->next;  
    if (sensor == end_hc_sr04) end_hc_sr04 = sensor->prev;
    
    free (sensor);

    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */
int delete_all_hc_sr04 ()
{
    while (first_hc_sr04 != NULL) 
        delete_hc_sr04 (first_hc_sr04);

    return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * 
 */
int count_hc_sr04 ()
{
  int amount = 0;
  struct _hc_sr04_ *sensor = first_hc_sr04;  
  
  while (sensor != NULL) {
    amount++;
    sensor = sensor->next;
  }
  return amount;
}
/*! --------------------------------------------------------------------
 * @brief   start the measure thread
 *           see also: void *run_hc_sr04 (void *data)
 */
void start_hc_04_thread ()
{
  if ((void *)hc_sr04_thread == NULL) {    
    pthread_mutex_init(&hc_sr04_mutex, NULL);
    hc_sr04_end = 0;
    pthread_create (&hc_sr04_thread, NULL, &run_hc_sr04, NULL);  /* starts measuring routine */
    usleep (250000);    
  } 
}
/*! --------------------------------------------------------------------
 * @brief  finished the measure thread
 */
void stop_hc_04_thread ()
{
  if ((void *)hc_sr04_thread != NULL) {
    hc_sr04_end = 1;
    while (hc_sr04_end == 1);	               /* wait for exit */
    pthread_mutex_destroy(&hc_sr04_mutex); 
    hc_sr04_thread = (pthread_t)NULL;                  /* thread handle */
  }
}
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
/*!	--------------------------------------------------------------------
 * @brief  wait until the echo signal has reached the value "state".
 *          If the waiting time is >= TIMEOUT, the function is ended.
 * @return  waittime
 */
int while_echo (struct _hc_sr04_ *sen, uint8_t state)
{
  int timediff;
  struct timeval start, stop;
  uint8_t end = 0;

  gettimeofday (&start, NULL);
  while (!end) {
    gettimeofday (&stop, NULL); 
    if ((timediff = difference_micro (&start, &stop)) >= TIMEOUT) end = 2;
    else if (digitalRead (sen->echo_pin) != state) end = 1;        
  }    
  
  return timediff;
}
/*! --------------------------------------------------------------------
 * @brief  starts the measurement and calculates the distance
 */
int get_dist (struct _hc_sr04_ *sen)
{
  int timediff;
  
  if (sen == NULL) 
    return EXIT_FAILURE;
  
  if (digitalRead (sen->echo_pin) == 0) {      /* check echo */
    digitalWrite (sen->trig_pin, 0);
    usleep (2);
    digitalWrite (sen->trig_pin, 1);
    usleep (10);
    digitalWrite (sen->trig_pin, 0);
  
    if (while_echo (sen, 0) < TIMEOUT) {
      if ((timediff = while_echo (sen, 1)) >= TIMEOUT) 
        timediff = 0;
      
      pthread_mutex_lock(&hc_sr04_mutex);                  /* enter critical section */
      sen->hc_sr04_run_time = timediff;
      sen->hc_sr04_dist_mm = timediff * 34300 / 200000;    /* distance in mm */
      pthread_mutex_unlock(&hc_sr04_mutex);                /* leave critical section */
    }
  }
  
  return EXIT_SUCCESS;
}
/*! --------------------------------------------------------------------
 * @brief  sensor read thread
 */
void *run_hc_sr04 (void *data)
{
  struct _hc_sr04_ *sen;

  printf ("-- thread run_hc_sr04 is running\n");
  while (!hc_sr04_end) {    
    sen = first_hc_sr04;
    while (sen != NULL) {
      if (sen->on) get_dist (sen);
      sen = sen->next;
      usleep (1000);
    }    
  }
  hc_sr04_end = 2;
  printf ("-- thread run_hc_sr04 killed\n");
  
  return EXIT_SUCCESS;
}

