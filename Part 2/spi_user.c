
/********Included files********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <linux/input.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "rdtsc.h"

#define CONFIG 'k' /*IOCTL COMMAND*/

int distance=0; /*Global variable to store the the distance value calculated using the sensor*/ 
int fd_echo,fd_echoedge,fd_trigger,res; /* descriptors to poll the rising and falling edge of the echo pin*/
unsigned char ReadValue[2]; /*to read from the opened gpio file*/
uint64_t start,end,a; /*variables to calculate distance from the sensor*/
pthread_t spi_device; /*spi_device thread id*/
pthread_t sensor; /*sensor thread id*/
int direction_flag = 0; /*flag to check if object is moving forward or backward*/
char sequence[20]; /*array to store the write sequence*/
int fd_driver; /*file descriptor for the spi_driver*/

struct pollfd poll_echo = {0}; /*poll structure*/

/**functions to enable gpios**/
int gpio_export(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, unsigned int out_flag);
int gpio_set_value(unsigned int gpio, unsigned int value);

void *spi_device_write(); /*spi device write function*/
void *sensor_read(); /*sensor thread function*/

/*function to export gpios*/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[64];

	fd = open( "/sys/class/gpio/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

/*function to set gpio direction*/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[64];

	snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}

	if (out_flag)
		write(fd, "out", 3); //out when 1
	else
		write(fd, "in", 2); //in when 0

	close(fd);
	return 0;
}

/*function to set gpio value*/
int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd;
	char buf[64];

	 snprintf(buf, sizeof(buf),"/sys/class/gpio/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}

	if (value)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);

	close(fd);
	return 0;
}

/**MAIN**/
int main()
{
    int ret;  
    unsigned char pattern[10][8] = {{0x91,0xB9,0X18,0x38,0x10,0x81,0x81,0x00},
                                   {0x00,0x91,0xB9,0x18,0x38,0x10,0x81,0x81},
                                   {0x91,0xB9,0x18,0x38,0x10,0x81,0xA1,0x20},
                                   {0x00,0x91,0xB9,0x18,0x38,0x10,0xA1,0xA1},
                                   {0x89,0x9D,0x0C,0x1C,0x28,0xA1,0x80,0x00},
                                   {0x20,0xA1,0x91,0x38,0x18,0x38,0x91,0x81},}; //DISPLAY ANIMATION DATA VALUES 
                                 
   fd_driver = open("/dev/spidev1.0", O_RDWR); //opening driver

   if (fd_driver < 0 )
   { printf("Can not open device file.\n");		
     return 0;
   }
   else 
   {
    ioctl(fd_driver,CONFIG,pattern); //passing the display animation data values to kernel space.
   } 
   
     //trigger gpio
     gpio_export(13);
     gpio_set_dir(13,1);

     //echo gpio 
     gpio_export(14);
     gpio_set_dir(14,0);
   
     //gpio multiplexing logic
     gpio_export(34);
     gpio_set_dir(34,1);
     gpio_set_value(34,0);
     gpio_export(77);
     gpio_set_dir(77,1);
     gpio_set_value(77,0);
     gpio_export(16);
     gpio_set_dir(16,0);
     gpio_set_value(16,1);
     gpio_export(64);
     gpio_set_dir(64,1);
     gpio_set_value(64,0);
     gpio_export(76);
     gpio_set_dir(76,1);
     gpio_set_value(76,0);     

     //starting sensor and spi threads
     pthread_create(&sensor,NULL,sensor_read,NULL); //sensor thread creation  
     pthread_create(&spi_device,NULL,spi_device_write,NULL);
     
     pthread_join(spi_device,NULL);
     pthread_join(sensor,NULL);
     
   return 0;
}

void *sensor_read()
    {

     fd_echo = open("/sys/class/gpio/gpio14/value",O_RDONLY); 
     if (fd_echo < 0) {
        perror("fd1_open");}
     fd_trigger = open("/sys/class/gpio/gpio13/value", O_WRONLY);
     if (fd_trigger < 0) {
        perror("fd1_open");}
     fd_echoedge = open("/sys/class/gpio/gpio14/edge", O_WRONLY);
     if (fd_echoedge < 0) {
        perror("fd1_open");}
  
     poll_echo.fd = fd_echo;
     poll_echo.events = POLLPRI|POLLERR;
     poll_echo.revents = 0;
     
     lseek(fd_echo, 0, SEEK_SET);        // rest to the starting position of the file
     read(fd_echo,ReadValue,2);          // read in 2 bytes

     while(1)
          {
            write(fd_echoedge, "rising", 6);    //poll for rising edge

            lseek(fd_echo, 0, SEEK_SET);        // rest to the starting position of the file

            write(fd_trigger,"1",1);// trigger on
            usleep(12);
            write(fd_trigger,"0",1);// trigger off
            
            poll(&poll_echo,1,1000); //polling for 1000ms

            if(poll_echo.revents & POLLPRI)
            {  
               start = rdtsc(); //get rising edge clock ticks
               res = read(fd_echo,ReadValue,2); //read in two bytes
               write(fd_echoedge, "falling",7); //poll for falling edge
               lseek(fd_echo, 0, SEEK_SET);
               poll(&poll_echo,1,1000);
               end = rdtsc(); //get falling edge clock ticks
                      
             if(poll_echo.revents & POLLPRI)
              {
               a = (end-start);            
               distance=((a*2.5)/58000); //calculating distance
               if(distance > 70)//limiting distance values to 70
               {
                 distance = 70;
               } 
               printf("distance=%d\n",distance);
               usleep(500000);
             }
            }
          }
 return 0;
}  

void *spi_device_write()
{

 int current_distance =0 , past_distance = 0, diff_distance = 0,time_val =0; //variable to check if object is moving away or towards sensor
 
 while(1)
 {
   current_distance = distance;
   diff_distance = (current_distance)-(past_distance);
   if (((diff_distance > 0)||(diff_distance < 0)) && (current_distance < 70) && (current_distance > 12)) 
   {
      direction_flag = 1; //moving backward or forward flag = 1
   }
  else
   {
      direction_flag = 0; //if not moving flag = 0
   }
 
   
  if (direction_flag == 1) //if moving jump animation is displayed 
   {
     time_val = current_distance*10;
     sprintf(sequence,"%d 4 5", time_val); //the write statement of the form of " 20 0 1" i.e. switching b/w pattern 0 and pattern 1 with
                                           //delay (in ms) given by time_val (here it is 20 ms)  
     write(fd_driver,sequence,sizeof(sequence));
     usleep(500000);
   }
 else 
   {
    time_val = current_distance*10; //if not moving the general animation is dislayed
    sprintf(sequence,"%d 0 1", time_val);
    write(fd_driver,sequence,sizeof(sequence));
    usleep(500000);
    sprintf(sequence,"%d 2 3", time_val);
    write(fd_driver,sequence,sizeof(sequence));  
    usleep(500000);
   }   
  past_distance = current_distance; 
 }
return 0;
}      

/********END********/
