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
#include <fcntl.h>

/*! --------------------------------------------------------------------
 *  @brief    print error text and then programm exit.
 */
static void abort_by_error (const char *error_text, const char *func_name)
{
    char buf[1024];
  
    sprintf (buf, "%s <%s>", error_text, func_name);
    perror (buf);
    abort();
}
/*! --------------------------------------------------------------------
 *  @brief  function works like 
 *            sudo i2cdetect -y 1
 */
static void scan_i2c (int device) 
{
    int i2c_addr;
    int i;
  
    if (device < 0) return;
    
    printf ("\n   ");
    for (i=0; i<=0xF; i++) printf ("%3X", i);                           /* print the low nibble of the i2c_addr */
    printf ("\n");
    
    for (i2c_addr=0; i2c_addr<128; i2c_addr++) {
        if (((i2c_addr+1) % 16) == 1) printf ("%2X:", (i2c_addr));      /* First print the high nibble of the i2c_addr */
        
        if ((i2c_addr > 2) && (i2c_addr < 0x78)) {                      /* scan betwewn 0x02 to 0x78 */
            if (ioctl( device, I2C_SLAVE_FORCE, i2c_addr ) < 0) {
                abort_by_error ("I2C_SLAVE failed", __func__);
            } else {
                if (i2c_smbus_read_byte ( device ) >= 0) printf ("%3X", i2c_addr);  /* chip detected */
                else printf (" --");
            }
        } else printf ("   ");
        if (((i2c_addr+1) % 16) == 0) printf ("\n");         /* stdout line feed */
    }
    
    printf ("\n");
}
/*! --------------------------------------------------------------------
 *  @brief  function works like 
 *               sudo i2cdetect -F 1
 *  @return adapter functionality mask
 */
static unsigned long check_functionality (int device)
{
  unsigned long funcs = 0;
  
  if (device < 0) 
      return 0;  
  if (ioctl( device, I2C_FUNCS, &funcs) < 0)       /* Get the adapter functionality mask */
      return 0;

  printf ("\n");
  printf ("%32s = %s\n", "I2C_FUNC_I2C", (funcs & I2C_FUNC_I2C) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_10BIT_ADDR", (funcs & I2C_FUNC_10BIT_ADDR) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_PROTOCOL_MANGLING", (funcs & I2C_FUNC_PROTOCOL_MANGLING) ? "YES" : "NO");  
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_PEC", (funcs & I2C_FUNC_SMBUS_PEC) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_BLOCK_PROC_CALL", (funcs & I2C_FUNC_SMBUS_BLOCK_PROC_CALL) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_QUICK", (funcs & I2C_FUNC_SMBUS_QUICK) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_READ_BYTE", (funcs & I2C_FUNC_SMBUS_READ_BYTE) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_WRITE_BYTE", (funcs & I2C_FUNC_SMBUS_WRITE_BYTE) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_READ_BYTE_DATA", (funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_WRITE_BYTE_DATA", (funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_READ_WORD_DATA", (funcs & I2C_FUNC_SMBUS_READ_WORD_DATA) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_WRITE_WORD_DATA", (funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_PROC_CALL", (funcs & I2C_FUNC_SMBUS_PROC_CALL) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_READ_BLOCK_DATA", (funcs & I2C_FUNC_SMBUS_READ_BLOCK_DATA) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_WRITE_BLOCK_DATA", (funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_READ_I2C_BLOCK", (funcs & I2C_FUNC_SMBUS_READ_I2C_BLOCK) ? "YES" : "NO");
  printf ("%32s = %s\n", "I2C_FUNC_SMBUS_WRITE_I2C_BLOCK", (funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK) ? "YES" : "NO");
  
  return funcs;
}
/*! --------------------------------------------------------------------
 *
 */
int main(int argc, char *argv[])
{	  
    int device;  

    if ((device = open("/dev/i2c-1", O_RDWR)) < 0)   /* open device file for i2c-bus 1. Return the new file descriptor, or -1 if an error occurred  */
        abort_by_error ("open i2c failed", __func__);
  
    if (check_functionality (device) == 0) 
        abort_by_error ("no functionality", __func__);
    
    scan_i2c (device);    
    close (device);
  
    return EXIT_SUCCESS;
}
