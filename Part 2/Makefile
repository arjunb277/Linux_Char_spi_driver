IOT_HOME = /opt/iot-devkit/1.7.2/sysroots

KDIR:=$(IOT_HOME)/i586-poky-linux/usr/src/kernel

CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-

#WARN:= -W -Wall -Wstrict-prototypes -Wmissing-prototypes

PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

SROOT=$(IOT_HOME)/i586-poky-linux

obj-m += spi_driver.o

all:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- $(WARN) -C $(KDIR) M=$(PWD) modules
	i586-poky-linux-gcc $(WARN) -o spiusr spi_user.c -lpthread --sysroot=$(SROOT)
clean:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -Wall -C $(KDIR) M=$(PWD) clean
	rm red
