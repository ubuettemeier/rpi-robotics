/*! ---------------------------------------------------------------------
 * @file    test_i2c.c
 * @date    09.30.2018
 * @name    Ulrich Buettemeier
 * 
 * @brief   Program works with i2c-dev
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
/*
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
*/
#include <fcntl.h>

/*! --------------------------------------------------------------------
 *  @brief    print error text and thren programm exit.
 */
static void abort_by_error (const char *error_text, const char *func)
{
  char buf[1024];
  
  sprintf (buf, "%s <%s>", error_text, func);
  perror (buf);
  abort();
}
/*! --------------------------------------------------------------------
 * 
 */
static void scan_i2c (int device) 
{
  int i2c_addr;
  
  printf ("scan_i2c\n");
  for (i2c_addr=0; i2c_addr<128; i2c_addr++) {
    if (ioctl( device, I2C_SLAVE_FORCE, i2c_addr ) < 0) {
      abort_by_error ("I2C_SLAVE failed", __func__);
    } else {
      if (i2c_smbus_read_byte ( device ) >= 0) printf ("%3X", i2c_addr);  /* chip detected */
      else printf (" --");
    }
    if (((i2c_addr+1) % 16) == 0) printf ("\n");  /* stdout line feed */
  }
  printf ("\n");
}
/*! --------------------------------------------------------------------
 * 
 */
int main(int argc, char *argv[])
{	  
  int device;
  unsigned long funcs;
  
  if ((device = open("/dev/i2c-1", O_RDWR)) < 0) {  /* open device file */
    abort_by_error ("open i2c failed", __func__);
  }
  
  if (ioctl( device, I2C_FUNCS, &funcs) < 0) {      /* Get the adapter functionality mask */
    abort_by_error ("I2C_FUNCS failed", __func__);
  }
  
  /* To determine what functionality is present */
  if ( funcs & I2C_FUNC_I2C ) printf ("I2c funcs detected\n");
  if ( funcs & I2C_FUNC_SMBUS_BYTE ) printf ("SMBUS funcs detected\n");

  scan_i2c (device);
    
  close (device);
  
  return EXIT_SUCCESS;
}
