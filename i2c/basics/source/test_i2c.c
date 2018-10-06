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
// #include <bcm2835.h>
/*
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
*/
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

int dev_open( int bus_no, int slave_address, int force_slave)
{
	int dev_fd;
	char filename[40];

	sprintf(filename, "/dev/i2c-%d", bus_no);
	dev_fd = open(filename, O_RDWR);
	#ifdef DEBUG
		fprintf(stderr, "open: fd=%d, errno=%d\n", dev_fd, errno);
		perror("open");
	#endif /* DEBUG */

	if( dev_fd > 0 )
	{
		if (ioctl(dev_fd, force_slave ? I2C_SLAVE_FORCE : I2C_SLAVE, 
			slave_address) < 0) 
		{
			#ifdef DEBUG
				fprintf(stderr, "ioctl: errno=%d\n", errno);
				perror("ioctl");
			#endif /* DEBUG */
			close( dev_fd );
			dev_fd = -1;
		}
	}

	return( dev_fd );
}
// ***********************************************************************
// static int pcf8591_write_value(int client, unsigned char reg, 
//                               unsigned int value)
// where client is a handle returned by dev_open()
//       reg is the register to write 
//       value is the value to write to the register
// 
// ***********************************************************************
static int pcf8591_write_value(int client, unsigned char reg, unsigned short value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}
// ***********************************************************************
// static unsigned short pcf8591_read_value(int client, unsigned char reg)
// where client is a handle returned by dev_open()
//       reg is the register to read 
// 
// read content of a specific register of an I2C device
//
// ***********************************************************************
static unsigned short pcf8591_read_value(int client, unsigned char reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}
/*! --------------------------------------------------------------------
 *
 */
 
#define PCF8591_SLAVE_ADDR  0x48	// A0, A1, A2 = GND
#define REG_CTL             0x40
#define REG_DAC			    0x40    // analog out

int main(int argc, char *argv[])
{	  
    int device;  
    int i;
    // uint8_t foo = 0;
    unsigned short res;

    if ((device = open("/dev/i2c-1", O_RDWR)) < 0)   /* open device file for i2c-bus 1. Return the new file descriptor, or -1 if an error occurred  */
        abort_by_error ("open i2c failed", __func__);
  
    if (check_functionality (device) == 0) 
        abort_by_error ("no functionality", __func__);
    
    scan_i2c (device);    
    close (device);
  
    if ((device = dev_open(1, PCF8591_SLAVE_ADDR, 0)) >= 0 ) {
        // i2c_smbus_write_byte_data (device, REG_CTL, 0x43);  // ctl byte
        // pcf8591_write_value (device, REG_CTL, 0x43);  // ctl byte
        pcf8591_write_value (device, 0x00, 0x43);  // ctl byte
        for (i=0; i<10; i++) {
            res = pcf8591_read_value( device, 0x41 );
            printf ("%i\n", res);
            usleep (100000);
        }
        /*
        for (i=0; i<25; i++) {
            i2c_smbus_write_byte_data (device, REG_DAC, foo);
            foo += 10;
            usleep (100000);
        }
        i2c_smbus_write_byte_data (device, 0x40, 0);
        */
        close (device);
    }
  
    return EXIT_SUCCESS;
}
