##############################################################################################################################################
SPI DEVICE PROGRAMMING AND PULSE MEASUREMENT PART 1

The objective of this task is to develop an application of distance measurement and animation display in order to learn the basic programming techniques for LINUX SPI and GPIO kernel modules. The task consists of two parts of which this file explains Part 1 i.e. To develop a user application to enable distance control. Using a ultrasonic sensor HC-SR04 to measure the distance of an arbitary object and increase/decrease the speed and pattern of an animation according to the proximity of the object from the sensor. Please see notes to now how we are achieving the objective. ----------------------------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------

Please follow the following procedure to run our application:

Step 1: Please save "ESP-team21-assgn04.zip".

Step 2: Unzip the file you will find a directory "ESP-team21-assgn04" containing the folders: ESP-teamX-assgn04-Part1 and 
        ESP-teamX-assgn04-Part2. ESP-teamX-assgn04-Part1 contains the files: README, MAKEFILE, Assignment4.c and rdtsc.h

Step 3: Please save ESP-team21-assgn04 file in your home directory.

Step 4: Open terminal and change your directory to home and cd to ESP-team21-assgn04-Part1 and type "make" for cross compilation.
        The the file should compile without giving errors or warnings.

Step 5: Setup Galileo board and copy the files to its home directory by using : sudo scp /home/"insert_name"/ESP-team21-assgn04/ESP-team21
        assgn04/* root@"your_board_ip":/home/

Step 6: On your putty terminal cd to home and ls you will see the assign4a file. Run the the file by typing ./assign4a and press enter.

Step 7: The user program will run the animation. Place an object in front of the sensor to see the animation characteristics.
        You will observe that as one moves towards the sensor the animation will switch between a square on the left top side and 
        bottom right side. The closer you are to the sensor the faster the animation will switch between the squares. 

Step 8: If you move the object away from the sensor you will observe that the pattern switches to a square on the right top side and bottom 
        left side. The farther you move from the sensor the slower the animation will switch between the squares.

Step 9: To stop the program press ctrl+c. This will close the program. 
----------------------------------------------------------------------------------------------------------------------------------------------
Note:

1. The distance values calculated lie between 12 and 70. (Limited).
2. IOCTL calls are used to send data to respective adresses to configure the LED matrix and display the patterns.
3. To measure the distance we send a pulse on the trigger pin of the ultrasonic sensor and calculate the time between a risingedge and falling
   edge on the echo pin of the sensor. (Not generating accurate values as values have been limited between 12-70)
4. The PIN I02 and IO3 and trigger and echo respectively.
5. IO 11,12 and 13 are set as DIN CS and CLK.
 
