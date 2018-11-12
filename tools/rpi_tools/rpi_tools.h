/*! ---------------------------------------------------------------------
 * @file    rpi_tools.h
 * @date    10-28-2018
 * @name    Ulrich Buettemeier
 */

#include <stdint.h>
#include <sys/time.h>

extern void abort_by_error (const char *error_text, const char *func_name);

extern int64_t difference_micro (struct timeval *start, struct timeval *stop);  /* calculates time difference in us */
extern int64_t current_difference_micro (struct timeval *start);

extern void show_usleep (uint64_t t, uint64_t refresh_time);                    /* function displays progress bar */
extern void show_scheduler_param (void);
