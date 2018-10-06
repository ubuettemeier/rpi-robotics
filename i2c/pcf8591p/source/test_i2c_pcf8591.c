/*! ---------------------------------------------------------------------
 * @file    test_i2c_pcf8591.c
 * @date    05.10.2018
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

#define PCF8591_ADDR  0x48	// A0, A1, A2 = GND
#define ADC0 0x00
#define ADC1 0x01

int device = -1;
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
 *  @brief   Function sets the PCF8591 ADC output channel.
 *            ADC0 => channel = 0x00
 *            ADC0 => channel = 0x01
 *            ADC0 => channel = 0x02
 *            ADC0 => channel = 0x03
 *  @param   fd = file discriptor
 *            channel = ADC channel
 */
int set_ADC_channel (int fd, uint8_t channel)
{
    if (fd < 0) return EXIT_FAILURE;           /* check device */
    i2c_smbus_read_byte_data( fd, channel );    /* Set ADC channel and read out previous ADC value */
    
    return (EXIT_SUCCESS);
}
/*! --------------------------------------------------------------------
 *
 */
int main(int argc, char *argv[])
{	      
    int i;
    unsigned short res;
    char buf[256];
    
    if ((device = open("/dev/i2c-1", O_RDWR)) < 0)   /* open device file for i2c-bus 1. Return the new file descriptor, or -1 if an error occurred  */    
        abort_by_error ("open i2c-1 failed", __func__);
  
    if (ioctl(device, I2C_SLAVE, PCF8591_ADDR) < 0) {
        sprintf (buf, "ioctl addr=%i failed", PCF8591_ADDR); 
        abort_by_error (buf, __func__);
    }

    for (i=0; i<100; i++) {    
        set_ADC_channel ( device, ADC0 );     /* select ADC channel 0 */
        res = i2c_smbus_read_byte (device);   /* read ADC0 */
        printf ("%i ", res);
    
        set_ADC_channel ( device, ADC1 );     /* select ADC channel 1 */
        res = i2c_smbus_read_byte (device);   /* read ADC1 */
        printf ("%i\n", res);
        
        usleep (100000);
    }

    close (device);
    return EXIT_SUCCESS;
}
