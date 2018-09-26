
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "keypressed.h"

struct termios oldSettings, newSettings;	/* global Settings for stdin */

/*!	--------------------------------------------------------------------
 *	@brief	save aktuelle settings in oldSettings
 * 			see: destroy_check_keypressed()
 */
void init_check_keypressed()
{
  tcgetattr( fileno( stdin ), &oldSettings );
  newSettings = oldSettings;
  newSettings.c_lflag &= (~ICANON & ~ECHO);
  tcsetattr( fileno( stdin ), TCSANOW, &newSettings );    
}
/*!	--------------------------------------------------------------------
 *	@brief	function set oldSettings
 */
void destroy_check_keypressed()
{
  tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );
}
/*!	--------------------------------------------------------------------
 *	@brief	select for stdin.
 * 			10us TIMOUT for select
 */
int check_keypressed(char *c)
{
  fd_set set;
  struct timeval tv;
  int res;

  tv.tv_sec = 0;
  tv.tv_usec = 10;		/* 10us TIMEOUT for select */

  FD_ZERO( &set );
  FD_SET( fileno( stdin ), &set );

  if ((res = select( fileno( stdin )+1, &set, NULL, NULL, &tv )) > 0) {
    read( fileno( stdin ), c, 1 );
  }
  return (res);
}
