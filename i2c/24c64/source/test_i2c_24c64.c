/*! ---------------------------------------------------------------------
 * @file    test_i2c_24c64.c
 * @date    09-10-2018
 * @name    Ulrich Buettemeier
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       
#include <time.h>
#include <fcntl.h>
#include <netinet/in.h>      /* htons */
#include <linux/i2c-dev.h>

#include "../../../sensor/ultrasonic-HC-SR04/source/keypressed.h"

#define ADDR_24CXX  0x50   /* 0x50...0x57 */

#define C32_
#define C64

#define PAGESIZE 32
#ifdef C64
    #define NPAGES  256
#endif
#ifdef C32
    #define NPAGES  128
#endif
#define NBYTES (NPAGES*PAGESIZE)


typedef struct _data_block_ {
    uint16_t addr;
    uint8_t buf[34];
} data_block;

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
 *
 */
static void help ()
{
    printf ("ESC = Exit\n");
    printf ("  h = this message\n");
    printf ("  1 = write byte\n");
    printf ("  2 = read  byte\n");
    
    printf ("\n");
}
/*! --------------------------------------------------------------------
 *  @brief   Write data to page of address.
 *  @return  If successful, the number of written bytes is returned.
 *            In the event of an error, the return value is negative.
 */
static int write_page (int fd, uint16_t addr, uint16_t len, uint8_t *value)
{
    data_block db;
    int ret;
    int timeout = 0;

    if (len > PAGESIZE) 
        return -2;
        
    db.addr = htons(addr);    
    memcpy (&db.buf[0], value, len);
    
    if ((ret = write (fd, &db, len+2)) != len+2) 
        return -1;
        
    while (timeout < 20) {     //   wait on write's end 
        char ap[4];  
        if (read(fd, &ap, 1) != 1) {
            usleep(500); 
            timeout++;
        }
        else break; 
    }
    if (timeout >= 20) ret = -3;
    
    return (ret); 
}
/*! --------------------------------------------------------------------
 *  @return  If an error occurs, the return value is negative.
 *            If successful, the return value is 0.
 */
static int write_bytes (int fd, uint16_t addr, uint16_t len, uint8_t *value)
{    
    uint8_t rest_of_block;
    uint8_t write_len; 
    uint16_t rest = len;
    uint16_t act_addr = addr;
    uint16_t ret = 0;
    
    rest_of_block = PAGESIZE -(act_addr % PAGESIZE);
    write_len = (rest > rest_of_block) ? rest_of_block : rest;
    
    write_page (fd, act_addr, write_len, &value[act_addr - addr]);
    
    while (rest > rest_of_block) {
        rest -= rest_of_block;
        act_addr += rest_of_block;
        rest_of_block = PAGESIZE -(act_addr % PAGESIZE);
        write_len = (rest > PAGESIZE) ? PAGESIZE : rest;
        
        if ((act_addr + write_len) > NBYTES) {             /* overflow */
            write_page (fd, act_addr, NBYTES - act_addr, &value[act_addr - addr]);
            return (-1);                      
        }
            
        write_page (fd, act_addr, write_len, &value[act_addr - addr]);
    }
    
    return ret;
}
/*! --------------------------------------------------------------------
 *  @return  If an error occurs, the return value is negative.
 *            If successful, the return value is 0
 */
static int read_bytes (int fd, uint16_t addr, uint16_t len, uint8_t *buf)
{
    uint16_t ad = htons(addr);
       
    if (addr > NBYTES-1)              /* out of memory */
        return -3;
        
    if ((addr + len -1) >= NBYTES)    /* overflow */
        len = NBYTES - addr;
        
    if (write (fd, &ad, 2) != 2)      /* select addresse */
        return -1;
        
    if (read (fd, buf, len) != len)   /* read data */
        return -2;
        
    return (0);
}
/*! --------------------------------------------------------------------
 *
 */
 
#define RW_ADDR 0x0101
 
int main(int argc, char *argv[])
{	      
    char buf[256] = {0};
    char c;
    int taste = 0;
    int akt_len = 32;
    time_t t;
    
    if ((device = open("/dev/i2c-1", O_RDWR)) < 0)     /* open device file for i2c-bus 1. Return the new file descriptor, or -1 if an error occurred  */    
        abort_by_error ("open i2c-1 failed", __func__);
  
    if (ioctl(device, I2C_SLAVE, ADDR_24CXX) < 0) {    /* check i2c addr */
        sprintf (buf, "ioctl addr=%i failed", ADDR_24CXX); 
        abort_by_error (buf, __func__);
    } 
    
    if (i2c_smbus_read_byte ( device ) < 0) {          /* check read */
        sprintf (buf, "read from addr=0x%X failed", ADDR_24CXX); 
        abort_by_error (buf, __func__);
    }
    
    help();    
    init_check_keypressed();                           /* init key-touch control */
        
    while (1) {        
        while ((taste = check_keypressed(&c)) <= 0);  /* wait for keypressed */
        if (c == 27) break;                           /* quit by ESC */
        if (c == 'h') help();
        if (c == '1') {            
            time (&t);
            sprintf (buf, "Timestemp of this Test: %s\n", ctime(&t));
            write_bytes (device, RW_ADDR, (akt_len = strlen(buf)), (uint8_t*)buf);
            printf ("%i bytes written from address %i.\n", strlen(buf), RW_ADDR);
        }
        if (c == '2') {
            memset (buf, '\0', 256);
            printf ("read %i Bytes from address %i\n", akt_len, RW_ADDR);
            read_bytes (device, RW_ADDR, akt_len, (uint8_t*)buf);
            printf ("%s\n", buf);            
        }        
    }
    
    close (device);
    destroy_check_keypressed();              /* destroy key-touch control */
    return (EXIT_SUCCESS);
}
