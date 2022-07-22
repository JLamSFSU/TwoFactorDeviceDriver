/**************************************************************
Name: Justin Lam
* GitHub UserID:  JLamSFSU
* Project: Two Factor Authentification Device Driver
*
* File: device_driver_test.c
*
* Description: file Device Driver
*
**************************************************************/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define BUF_LENGTH 256
#define VALIDATE_KEY 666
#define GENERATE_KEY 420

typedef union Auth
{
    int fd;
} Auth;

int main()
{
    char userInput[BUF_LENGTH];
    char userOutput[BUF_LENGTH];
    int code, system_code, ret;

    /**
     * 1. cd ../Module 
     * 2. make all
     * 3. sudo insmod twoStepAuth.ko
     * 4. devnode() will handle chmod
     */
    // Opens the device driver
    Auth auth = {open("/dev/twoStepAuth", O_RDWR)};
    if (auth.fd == -1)
        return -1;

    // I/O control to generate the key
    ret = ioctl(auth.fd, GENERATE_KEY, 0);

    if (ret != 0)
    {
        printf("Error occured with I/O control, EXITING!\n");
        exit(1);
    }

    printf("Please check the kernel log for the key\ntail -f /var/log/kern.log\nEnter 1 to exit\n");

    // Cycle through a loop till the user successfully enters the key or quits. 
    do
    {
        printf("Enter Key: ");
        // Takes user input and convert it to an integer value
        scanf("%s", userInput);
        code = atoi(userInput);
        // Exit condition, when user gives us entering a key.
        if (code == 1)
        {
            printf("Exiting without key input\n");
            break;
        }

        // send the user input to the device driver
        ret = write(auth.fd, NULL, code);

        if (ret != 0)
            printf("Invalid Input\n");

        // I/O control to validate the user entry.
        ioctl(auth.fd, VALIDATE_KEY, &system_code);

        // returns the validation as a text entry
        ret = read(auth.fd, userOutput, BUF_LENGTH);
        printf("%s", userOutput);
    } while (system_code != 1);

    // release the device driver.
    close(auth.fd);
    return 0;
}