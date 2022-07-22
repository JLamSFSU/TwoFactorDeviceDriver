/**************************************************************
* Name: Justin Lam
* GitHub Name: JLamSFSU
* Project: Two Factor Authentification Device Driver
*
* File: README.md
*
* Description: Write up of the corresponding program. The
* objective of the assignment is to develop a
* device driver which communicates with the
* kernel.
*
**************************************************************/

Overview:
The project was challenging due to the various options for what kind of device
driver could have been selected to be programmed. The overall purpose was to
experience how the Kernel and userspace communicate through device drivers. Various
selections were suggested, such as a calculator, but it was not of great personal
interest. It was decided that two-factor authentication was selected as a device
driver program for the assignment due to the increasing concern for security as a
personal interest.
It was not easy to decide where to start on such a challenge. Before
developing any code, it was essential to learn how a two-factor authenticator was and
how the device driver communicates with the Kernel and userspace. Watching the
lecture material, referencing the Linux manual, and paying attention to the class
discussion was enough to comprehend the kernel communication. As for the two-factor
authentication, part was mainly left to imagination and comprehension of experience
with prior two-factor programs to emulate. Ideally, it will be utilized as a security
measure against unauthorized access.
The objective was to have the device driver program request a user input based
on a randomly generated code by the Kernel within a time frame to authenticate the
user. Thus, the two-step authenticator kernel module will use read, write, and Input-Output Control System (ioctl) file operations displayed within the kernel log.
Unfortunately, this is not the ideal situation since it is preferably displayed in a
more visible location for the user.
Ideally, the device driver module can be utilized with any program that
requires proper access authentication. However, since only those with root access,
such as the sudo user, would access the kernel log, these users are the only users
qualified to have authentication access. Therefore, placing the authentication code
within the kernel log allows only authorized users to pass the two-step
authentication. Plausibly as a future feature, if any additional development is
desired would be to provide somehow access to the key to specific groups or users
aside from super users.
Many obstructions occurred during the development process; if not for social
collaboration on abstract concepts and resource sharing, it would have been quite
challenging to complete the assignment, precisely when utilizing the timer to expire
the key. Fortunately, a classmate was working on a similar subject, and we were able
to pull discovered resources and exchange ideas, mainly due to how esoteric the
subject is.
Attempting to generate a random key was also quite challenging, mainly due to
a random number generator is omitted without the inclusion of specific libraries.
After researching how random numbers are generated, it was decided to follow a simple
model of using time. It required the readings of a linear congruential generator and
the incorporation of time to create a seed for the random number generator. From the
current understanding, it is how the imported library random number generator works.
The two-factor authentication functions utilize three steps: opening the
module, process through the ioctl, and closing the module. The open is critical to
grabbing the file descriptor, which will be used ioctl. Then, making a call to the
iotcl to generate the initial key. Next, using write, it sends the user input to the
device driver within a cycle, where the next ioctl will validate the user input.
Then, using read, it will write to callers buffer the result. Finally, when closing,
it will release the device. The open, write, read, close, and iotcl are all ways the
Kernel and userspace communicate.
Overall the project was interesting due to the limitations when writing the
device drivers were implied. Such as the exclusions of loops, which are utilized
quite frequently outside of the device drivers. Nonetheless, it was a great
experience creating a device driver, and it can be seen how important it is for all
devices and can be an excellent addition when working on future projects.


Installation:
Once inside the project directory, change directory to the Module directory:
cd Module/
Once inside the Module directory, create the module utilizing the makefile:
make all
After making the device driver, insert the module using superuser permission:
sudo insmod twoStepAuth.ko

Testing:
To test the device driver, change directory to Test directory:
cd ../Test/
Using the provided makefile, run the program:
make run
Here we can see the key has been created in the previous image, but in the image
below, the key has expired.
Entering the expired key creates a new key. If the input is correct, the success
message will be displayed.
The wrong key has been entered to show an error message for invalid input in the
image below.
Enter 1 will exit the program.

Removal:
Remove the function using the makefile:
make clean
Change directory to the module:
cd ../Module/
Once inside the Module Directory super user remove the device driver:
sudo rmmod twoStepAuth
Now remove the files utilizing the makefile:
make clean