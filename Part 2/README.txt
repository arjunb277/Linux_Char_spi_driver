##############################################################################################################################################
SPI DEVICE PROGRAMMING AND PULSE MEASUREMENT PART 2

The objective of this task is to develop an application of distance measurement and animation display in order to learn the basic programming techniques for LINUX SPI and GPIO kernel modules. The task consists of two parts of which this file explains Part 2 i.e. To develop a user application to enable distance controlled animation. Using a ultrasonic sensor HC-SR04 to measure the distance of an arbitary object and increase/decrease the speed and pattern of an animation according to the proximity of the object from the sensor. We have to develop a device driver that provides. An IOCTL command to pass 10 patterns to the a display buffer in kernel space and a write function which specifies a sequence of patterns and the switching time between them. We used this to create a small game. ----------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------

Please follow the following procedure to run our application:

Step 1: Please save "ESP-team21-assgn04.zip".

Step 2: Unzip the file you will find a directory "ESP-team21-assgn04" containing the folders: ESP-teamX-assgn04-Part1 and 
        ESP-teamX-assgn04-Part2. ESP-teamX-assgn04-Part2 contains the files: README, MAKEFILE, spi_driver.c,spi_usr.c and rdtsc.h

Step 3: Please save ESP-team21-assgn04 file in your home directory.

Step 4: Open terminal and change your directory to home and cd to ESP-team21-assgn04-Part1 and type "make" for cross compilation.
        The the file should compile without giving errors or warnings.

Step 5: Setup Galileo board and copy the files to its home directory by using : sudo scp /home/"insert_name"/ESP-team21-assgn04/ESP-team21
        assgn04/* root@"your_board_ip":/home/

Step 6: On your putty terminal cd to home and ls you will see the spi_driver.ko file. First delete the original driver by doing rmmod spidev 
        and then insmod spi_driver.ko, it will show that it is now available to use. You can run the user application by running the spiusr 
        file. Run the file by typing ./spiusr

Step 7: The user program will run the animation. Place an object in front of the sensor to see the animation characteristics.
        You will observe a car animation moving at a pace proportional to the distance from the sensor. If the object is still 
        the animation will show an obstacle coming towards the car. You can make the car jump over the obstacle by either moving 
        the object closer to the sensor or away from it.  
         
Step 8: If you move the object away from the sensor you will observe that the difficulty of the game becomes easier as the animation 
        speed slows, if you move closer to the sensor the  difficulty of the game increases.

Step 9: To stop the program press ctrl+c. This will close the program and clears the display.
----------------------------------------------------------------------------------------------------------------------------------------------
Note:

1. The distance values calculated lie between 12 and 70. (Limited).
2. The IOCTL call can pass 10 patterns to the kernel at once.
3. The write call passes the distance value from user space to kernel space and specifies which two patterns need to run.
4. To measure the distance we send a pulse on the trigger pin of the ultrsonic sensor and calculate the time between a rising edge and falling
   edge on the echo pin of the sensor. (Not generating accurate values as values have been limited between 12-70)
5. The PIN I02 and IO3 and trigger and echo respectively.
6. IO 11,12 and 13 are set as DIN CS and CLK.
7. Th game is sensitive to movement.
8. Link to animation for clarity: https://xantorohara.github.io/led-matrix-editor/#63000a1f0e000063|c600143e1c0000c6|6300ca1f0e000063|c600d43e1c0000c6|6300300a1f0e0063|c6002b7c380000c6
