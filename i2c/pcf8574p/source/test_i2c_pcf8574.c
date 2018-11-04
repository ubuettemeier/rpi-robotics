/*! ---------------------------------------------------------------------
 * @file    test_i2c_pcf8574.c
 * @date    11-04-2018
 * @name    Ulrich Buettemeier
 * 
 * @brief   Program works with i2c-dev
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>

#include "../../../tools/keypressed/keypressed.h"

#define PCF8574_ADDR  0x20	            /* A0, A1, A2 = GND */

int device = -1;
const uint8_t config_mask = 0x0F;     /* P0-P3 input; p4-p7 output */

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
 *
 */
void help ()
{
    printf ("\n");
    printf ("ESC = exit program\n");
    printf ("h = this message\n");
    printf ("0 = set all port's = 0\n");
    printf ("1 = set all port's = 1\n");
    printf ("r = get all port's\n");
}
/*! --------------------------------------------------------------------
 *
 */
int main(int argc, char *argv[])
{	      
    char buf[256];
    char c;
    int taste = 0;
                    
    if ((device = open("/dev/i2c-1", O_RDWR)) < 0)   /* open device file for i2c-bus 1. Return the new file descriptor, or -1 if an error occurred  */    
        abort_by_error ("open i2c-1 failed", __func__);
  
    if (ioctl(device, I2C_SLAVE, PCF8574_ADDR) < 0) {        
        sprintf (buf, "ioctl addr=%i failed", PCF8574_ADDR); 
        abort_by_error (buf, __func__);
    }
    
    if (i2c_smbus_read_byte ( device ) < 0) 
        abort_by_error ("pcf8574 failed", __func__);
        
    if (i2c_smbus_write_byte (device, config_mask) != 0)    /* write config mask */
        printf ("write ERROR\n");

    init_check_keypressed();                            /* init key-touch control */        
    help();
    
    while (1) {                                
        while ((taste = check_keypressed(&c)) <= 0);  /* wait for keypressed */
        if (c == 27) break;                           /* quit by ESC */
        if (c == 'h') help();
        
        if ((c == '0') || (c == '1')) {                /* set output-pin's */
            uint8_t foo = (c == '0') ? 0x00 | config_mask : 0xf0 | config_mask;
            
            int n = i2c_smbus_write_byte (device, foo);
            printf ("-- %s\n", (n==0) ? "1 Byte ausgegeben" : "write ERROR");
        }        
        
        if (c == 'r') {                                 /* get input-pin's */
            int32_t foo;

            foo = i2c_smbus_read_byte (device);
            if (foo < 0) 
                printf ("read ERROR\n");
            else 
                printf ("data = 0x%2X\n", (uint8_t)foo & config_mask);
        }
        
        usleep (1000);                       /* wait 1 ms */
    }
    
    close (device);  
    destroy_check_keypressed();              /* destroy key-touch control */
    return EXIT_SUCCESS;
}
