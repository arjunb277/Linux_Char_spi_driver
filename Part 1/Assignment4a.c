/********Included files*********/
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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

int i,fd_css,fd_spi,ret; //file descriptors and variables for the spi thread
int distance=0;//global variable to store claculated distance value from the sensor
int fd_echo,fd_echoedge,fd_trigger,res; //file descriptors and variables for the sensor thread
unsigned char ReadValue[2]; //to read for polling
uint64_t start,end,a; //variables to calculate the distance from the sensor
pthread_t spi_device; //spi thread id
pthread_t sensor;//sensor thread id
int direction_flag = 0; //flag to set direction of object in front of the sensor

struct pollfd poll_echo = {0};

/*Gpio functions*/
int gpio_export(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, unsigned int out_flag);
int gpio_set_value(unsigned int gpio, unsigned int value);

/*Thread functions*/
void *spi_device_write();
void *sensor_read();

typedef struct led1 {
uint8_t led[8];
} pattern;

/*Display animaton*/
pattern pattern1={{0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00}};
pattern pattern2={{0x00,0x00,0x00,0x00,0x0F,0x0F,0x0F,0x0F}};
pattern pattern3={{0x0F,0x0F,0x0F,0x0F,0x00,0x00,0x00,0x00}};
pattern pattern4={{0x00,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0}};

/*Funtion to export gpio*/
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

/*Funtion to set gpio direction*/
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

/*Funtion to set gpio value*/
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

/*MAIN*/
int main()
{ 

  /*configuring LEDs*/
     gpio_export(13);/*trigger*/
     gpio_set_dir(13,1);
 
     gpio_export(14);/*echo*/
     gpio_set_dir(14,0);
   
  /*Multiplexing logic*/
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
 
     gpio_export(15); // CS
     gpio_set_dir(15,1); //out

   //DIN
     gpio_export(24);
     gpio_set_dir(24,1);
     gpio_set_value(24,0);

     gpio_export(44);
     gpio_set_dir(44,1);
     gpio_set_value(44,1);
  
     gpio_export(72);
     gpio_set_dir(72,1);
     gpio_set_value(72,0);

  //CS
     gpio_export(42);
     gpio_set_dir(42,1);
     gpio_set_value(42,0);

  //CLK
     gpio_export(30); // CLK
     gpio_set_dir(30,1); //out
     gpio_set_value(30,0); 
     gpio_export(46);
     gpio_set_dir(46,1);
     gpio_set_value(46,1); 

     gpio_set_value(15,0);

     pthread_create(&sensor,NULL,sensor_read,NULL); //sensor thread creation  
     pthread_create(&spi_device,NULL,spi_device_write,NULL);// spi device thread creation
    
     pthread_join(spi_device,NULL);
     pthread_join(sensor,NULL);
     
 return 0;       
} 

//sensor thread
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
     read(fd_echo,ReadValue,2);   

     while(1)
          {
            write(fd_echoedge, "rising", 6); //polling on rising edge

            lseek(fd_echo, 0, SEEK_SET);        // rest to the starting position of the file

            write(fd_trigger,"1",1);// trigger on
            usleep(12);
            write(fd_trigger,"0",1);// trigger off
            
            poll(&poll_echo,1,1000);

            if(poll_echo.revents & POLLPRI)
            {  
               start = rdtsc(); //rising edge clock ticks               
               res = read(fd_echo,ReadValue,2);
               write(fd_echoedge, "falling",7);
               lseek(fd_echo, 0, SEEK_SET);
               poll(&poll_echo,1,1000);
               end = rdtsc(); //falling edge clock ticks
                      
             if(poll_echo.revents & POLLPRI)
              {
               a = (end-start);          
               distance=((a*2.5)/58000); //display calculation
               if(distance > 70)
               {
                 distance = 70;
               } 
               printf("distance=%d\n",distance);
               usleep(1000000);
             }
            }
          }
 return 0;
}  

//spi thread
void *spi_device_write()
{
   unsigned char txmessage[2] = {0}, rxmessage[2] = {0,0};
  
   struct spi_ioc_transfer spi_tx_structure = {
      
      .tx_buf = (unsigned long)txmessage,
      .rx_buf = (unsigned long)rxmessage,
      .len = ARRAY_SIZE(txmessage),
      .speed_hz = 500000,
      .bits_per_word = 8,
     };
  
  printf("GPIO SET\n"); 
 
  fd_spi = open("/dev/spidev1.0", O_WRONLY);

  printf("driver_open\n");  

  if (fd_spi<0)
   {
     printf ("Error opening SPI DEV");
   }
  
  printf("sending data\n");

  fd_css = open("/sys/class/gpio/gpio15/value", O_WRONLY);  
  printf("gpio 15 opened succesfully\n");

  /*configuring spi device*/
  txmessage[0] = 0x0F;
  txmessage[1] = 0x01;
  write(fd_css,"0",1);
  ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
  usleep(10000);
  write(fd_css,"1",1);

  txmessage[0] = 0x0F;
  txmessage[1] = 0x00;
  write(fd_css,"0",1);
  ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
  usleep(10000);
  write(fd_css,"1",1);

  txmessage[0] = 0x09;
  txmessage[1] = 0x00;
  write(fd_css,"0",1);
  ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
  usleep(10000);
  write(fd_css,"1",1);

  txmessage[0] = 0x0B;
  txmessage[1] = 0x07;
  write(fd_css,"0",1);
  ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
  usleep(10000);
  write(fd_css,"1",1);

  txmessage[0] = 0x0C;
  txmessage[1] = 0x01;
  write(fd_css,"0",1);
  ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
  usleep(10000);
  write(fd_css,"1",1);

  txmessage[0] = 0x0A;
  txmessage[1] = 0x00;
  write(fd_css,"0",1); 
  ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
  usleep(10000);
  write(fd_css,"1",1);

  for(i=1;i<9;i++)
  {
    txmessage[0] = i;
    txmessage[1] = 0;
    write(fd_css,"0",1);  
    ret  = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &spi_tx_structure);
    usleep(10000);
    write(fd_css,"1",1);  
  }

 int current_distance =0 , past_distance = 0, diff_distance = 0;

while(1)
{

  current_distance = distance;
  diff_distance = (current_distance) - (past_distance); 

  if ((diff_distance > 0) && (current_distance < 70) && (current_distance > 12)) //checking direction
   {
      direction_flag = 1; //if direction away from sensor 
   }
  else if((diff_distance < 0) && (current_distance < 70) && (current_distance > 12))
   {
      direction_flag = 0; //if direction towards sensor
   }
  else
   {
     //if still
   }
  
if( direction_flag == 1)
{    
 for( i=1;i<9;i++)
               {//display two square
                write(fd_css,"0",1);
                txmessage[0] = i;
                txmessage[1] = pattern2.led[i-1];  
                ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1),&spi_tx_structure);
                usleep(10000);
                write(fd_css,"1",1);
               }

      usleep(current_distance*9000);

     for( i=1;i<9;i++)
               {
                write(fd_css,"0",1);
                txmessage[0] = i;
                txmessage[1] = pattern1.led[i-1];  
                ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1),&spi_tx_structure);
                usleep(10000);
                write(fd_css,"1",1);
               }

      usleep(current_distance*9000);//switching speed
    }
 else 
   {
     for( i=1;i<9;i++)
               {//display two squares
                write(fd_css,"0",1);
                txmessage[0] = i;
                txmessage[1] = pattern4.led[i-1];  
                ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1),&spi_tx_structure);
                usleep(10000);
                write(fd_css,"1",1);
               }

      usleep(current_distance*9000);

     for( i=1;i<9;i++)
               {
                write(fd_css,"0",1);
                txmessage[0] = i;
                txmessage[1] = pattern3.led[i-1];  
                ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1),&spi_tx_structure);
                usleep(10000);
                write(fd_css,"1",1);
               }

      usleep(current_distance*9000);
            
    }   
   past_distance = current_distance;
}
return 0;
}      
