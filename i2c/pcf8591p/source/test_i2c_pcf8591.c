/*! ---------------------------------------------------------------------
 * @file    test_i2c_pcf8591.c
 * @date    10-05-2018
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

#include "../../../tools/keypressed/keypressed.h"
#include "../../../tools/rpi_tools/rpi_tools.h"

#define PCF8591_ADDR  0x48	/* A0, A1, A2 = GND */

enum ADC {
    ADC0,
    ADC1,
    ADC2,
    ADC3
};

int device = -1;

/*! --------------------------------------------------------------------
 *  @brief   Function select the PCF8591 ADC channel.
 *            ADC0 => channel = 0x00
 *            ADC1 => channel = 0x01
 *            ADC2 => channel = 0x02
 *            ADC3 => channel = 0x03
 *
 *  @param   fd = file discriptor
 *            channel = ADC channel
 * 
 *  @return  returning negative errno else a data byte received from the device.
 */
int select_ADC_channel (int fd, uint8_t channel)
{
    if (fd < 0) 
        return ( -1 );                              /* check device */
        
    return i2c_smbus_read_byte_data( fd, channel );   /* Select ADC channel and read previous ADC value */
}
/*! --------------------------------------------------------------------
 *  @brief  Function sets new DAC value
 * 
 *  @param   fd = file discriptor
 *            value = DAC Value
 *
 *  @return Zero on success. Negative errno by failure
 */
int set_DAC_value (int fd, uint8_t value)
{
    if (fd < 0) 
        return ( -1 );                                  /* check device */
        
    return i2c_smbus_write_byte_data (fd, 0x40, value);
}
/*! --------------------------------------------------------------------
 *
 */
int main(int argc, char *argv[])
{	      
    int res;
    char buf[256];
    char c;
    int taste = 0;
    uint8_t dac_value = 0x00;
    
    printf ("Exit with ESC\nNext measurement with any key\n");
    
    init_check_keypressed();                    /* init key-touch control */
    
    if ((device = open("/dev/i2c-1", O_RDWR)) < 0)   /* open device file for i2c-bus 1. Return the new file descriptor, or -1 if an error occurred  */    
        abort_by_error ("open i2c-1 failed", __func__);
  
    if (ioctl(device, I2C_SLAVE, PCF8591_ADDR) < 0) {
        sprintf (buf, "ioctl addr=%i failed", PCF8591_ADDR); 
        abort_by_error (buf, __func__);
    }


    while (1) {        
        select_ADC_channel ( device, ADC0 );    /* select ADC channel 0 */
        res = i2c_smbus_read_byte (device);     /* read ADC0 */
        printf ("ADC0=%3i  ", res);
        
        select_ADC_channel ( device, ADC1 );    /* select ADC channel 1 */
        res = i2c_smbus_read_byte (device);     /* read ADC1 */
        printf ("ADC1=%3i  ", res);
        
        res = set_DAC_value (device, (dac_value += 0x10));    /* set new DAC value */
        printf ("DAC=%i\n", (res >= 0) ? dac_value : res); 
        
        usleep (10000);                         /* wait 10 ms */
        
        while ((taste = check_keypressed(&c)) <= 0);  /* wait for keypressed */
        if (c == 27) break;                           /* quit by ESC */
    }
    set_DAC_value (device, 0);
    close (device);
    
    destroy_check_keypressed();              /* destroy key-touch control */
    return EXIT_SUCCESS;
}
