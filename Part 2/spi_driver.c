/********Included Files********/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/uaccess.h>

#define SPIDEV_MAJOR			153	
#define N_SPI_MINORS			32	
#define CONFIG 'k'  /*IOCTL COMMAND*/

static DECLARE_BITMAP(minors, N_SPI_MINORS);

/*Per device structure*/
struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;
	
	struct mutex		buf_lock;
	unsigned		users;
        u32                     speed_hz;
       
};

struct spidev_data *spidev;

unsigned char pattern[10][8]; //pattern matrix to store the display animation data.
char in_string[20]; //input string for storing the written sequence from userspace.
char dtm[20];
unsigned char tx_buffer[2]; //transfer buffer to send to spi bus
unsigned char rx_buffer[2];

/*Spi tranfer structure used to access spi bus*/
struct spi_transfer	t = {                 
		.tx_buf		= &tx_buffer[0],
                .rx_buf         = &rx_buffer[0],
		.len		= 2,
                .cs_change      = 1,
                .bits_per_word  = 8,
		.speed_hz	= 500000,
	};

	struct spi_message	m;

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*Function to tranfer address and data through spi bus*/
static void spi_tx_function(unsigned char addr, unsigned char data)
{
    int ret=0;
    tx_buffer[0] = addr;
    tx_buffer[1] = data;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    gpio_set_value(15,0); //chip select low 
    ret = spi_sync(spidev->spi, &m);
    gpio_set_value(15,1); //chip select high 
    return;
}

/*Open function to access the driver*/
static int spidev_open(struct inode *inode, struct file *filp)
{
  int i=0;

  /*Configuring GPIOS for spi*/
  //DIN 11
  gpio_request(24,"sysfs");
  gpio_export(24,false);
  gpio_direction_output(24,0);
  
  gpio_request(44,"sysfs");
  gpio_export(44,false);
  gpio_direction_output(44,1);

  gpio_request(72,"sysfs");
  gpio_export(72,false);
  gpio_set_value_cansleep(72,0);

  //CS 12

  gpio_request(15,"sysfs");
  gpio_export(15,false);
  gpio_direction_output(15,0);

  gpio_request(42,"sysfs");
  gpio_export(42,false);
  gpio_direction_output(42,0);

  //CLK 13

  gpio_request(30,"sysfs");
  gpio_export(30,false);
  gpio_direction_output(30,0);

  gpio_request(46,"sysfs");
  gpio_export(46,false);
  gpio_direction_output(46,1);

  /*configuring the SPI led matrix*/	
  spi_tx_function(0x0F, 0x01);
  spi_tx_function(0x0F, 0x00);
  spi_tx_function(0x09, 0x00);
  spi_tx_function(0x0A, 0x04);
  spi_tx_function(0x0B, 0x07);
  spi_tx_function(0x0C, 0x01);

  /*clearing led matrix*/
  for(i=1; i < 9; i++)
  {
   spi_tx_function(i, 0x00);
  }
	
  return 0;
}

/*Driver release function*/
static int spidev_release(struct inode *inode, struct file *filp)
{
    int status = 0;
    unsigned char i=0;

	for(i=1; i < 9; i++)
	{
         spi_tx_function(i, 0x00);
	}
	//free all gpios
        gpio_free(15);
	gpio_free(24);
	gpio_free(44);
	gpio_free(42);
	gpio_free(72);
        gpio_free(46);
        gpio_free(30);
	
	printk("spi_led_release -- spidev is closing\n");
	return status;
}

/*Driver IOCTL function*/
static long spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
switch (cmd)
   {
    case CONFIG:
     {  
        int ret_val,i,j;
        unsigned char input_buffer[10][8];
         
        ret_val = copy_from_user((void *)&input_buffer,(void __user *) arg,sizeof(input_buffer));
        if(ret_val != 0){
        printk("error");}

       for(i=0;i<10;i++)
       {
        for(j=0;j<8;j++)	
        {	
         pattern[i][j] = input_buffer[i][j]; //storing the values of the display animations in pattern buffer
        }
       }
      break;
     }
    default:
     {
        printk("Error IOCTL CONFIG");
        break;
     }
   }
    return 0;
}

/*Driver write function*/
static ssize_t spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
  int time_val,p1,p2,k;
  
  copy_from_user((void *)&in_string,(void __user *) buf,sizeof(in_string));
  strcpy( dtm, in_string);
  sscanf(dtm, "%d %d %d",&time_val,&p1,&p2);

  for(k=1;k<9;k++)
  {
   spi_tx_function(k,pattern[p1][k-1]);  
  }
   
  msleep(time_val);

  for(k=1;k<9;k++)
  {
   spi_tx_function(k,pattern[p2][k-1]);  
  }
  
  msleep(time_val);

  
 return 0;
}

/*file operation strucure*/
static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	.unlocked_ioctl = spidev_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
};

static struct class *spidev_class; 

/*Device probe function*/
static int spidev_probe(struct spi_device *spi)
{
   int	status;
   unsigned long minor;
	
   spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
   if (!spidev)
   return -ENOMEM;
	
   spidev->spi = spi;
   spin_lock_init(&spidev->spi_lock);
   mutex_init(&spidev->buf_lock);
	
   INIT_LIST_HEAD(&spidev->device_entry);
	
   mutex_lock(&device_list_lock);
   minor = find_first_zero_bit(minors, N_SPI_MINORS);
   if (minor < N_SPI_MINORS) {
	struct device *dev;
		
	spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
	dev = device_create(spidev_class, &spi->dev, spidev->devt,
	spidev, "spidev%d.%d",
	spi->master->bus_num, spi->chip_select);
	status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}

  if (status == 0) {
	set_bit(minor, minors);
	list_add(&spidev->device_entry, &device_list);
        }
  mutex_unlock(&device_list_lock);
		
  spidev->speed_hz = spi->max_speed_hz;
		
  if (status == 0)
  spi_set_drvdata(spi, spidev);
  else
  kfree(spidev);		
  return status;
}

/*device remove function*/	
static int spidev_remove(struct spi_device *spi)
{
  struct spidev_data	*spidev = spi_get_drvdata(spi);
		
  spin_lock_irq(&spidev->spi_lock);
  spidev->spi = NULL;
  spin_unlock_irq(&spidev->spi_lock);

  mutex_lock(&device_list_lock);
  list_del(&spidev->device_entry);
  device_destroy(spidev_class, spidev->devt);
  clear_bit(MINOR(spidev->devt), minors);
  if (spidev->users == 0)
  kfree(spidev);
  mutex_unlock(&device_list_lock);
		
	return 0;
}
		
static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{},
};
	
MODULE_DEVICE_TABLE(of, spidev_dt_ids);
	
static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.owner =	THIS_MODULE,
		.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,
	
};

/*Driver init function*/	
static int __init spidev_init(void)
{
   int status;
   BUILD_BUG_ON(N_SPI_MINORS > 256);
   status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);  
   if (status < 0)
   return status;
	
   spidev_class = class_create(THIS_MODULE, "spidev");
   if (IS_ERR(spidev_class)) {
   unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
   return PTR_ERR(spidev_class); 
   }
		
   status = spi_register_driver(&spidev_spi_driver);
   if (status < 0) {
   class_destroy(spidev_class);
   unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
}

   printk("SPI driver is available\n");
   return status;
}
module_init(spidev_init);
	
/*Driver exit function*/
static void __exit spidev_exit(void)
{
  spi_unregister_driver(&spidev_spi_driver);
  class_destroy(spidev_class);
  unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
  printk("SPI driver is unavailable\n");
}
module_exit(spidev_exit);
	
MODULE_AUTHOR("Team 21");
MODULE_DESCRIPTION("SPI driver for LED matrix");
MODULE_LICENSE("GPL");
/********END********/ 
