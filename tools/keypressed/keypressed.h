/*! --------------------------------------------------------------------
 *
 */
#include <fcntl.h>
#include <termios.h> 
 
extern struct termios oldSettings, newSettings;	/* global Settings for stdin */
 
extern void init_check_keypressed (void);
extern void destroy_check_keypressed (void);
extern int check_keypressed (char *c);
