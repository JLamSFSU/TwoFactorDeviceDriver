/**************************************************************
* Name: Justin Lam
* GitHub UserID:  JLamSFSU
* Project: Two Factor Authentification Device Driver
*
* File: twoStepAuth.c
*
* Description: A two-step authentication
*              Device Driver module
**************************************************************/
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "twoStepAuth.h"

#define MAX_RANDOM_SEED 0x7FFFFFF
#define KEY_MAX_VALUE 99999999
#define KEY_MIN_VALUE 1000000
#define TIME_MAX_VALUE 18000 // 5 minutes
#define RW_PERM 0666
#define VALIDATE_KEY 666
#define GENERATE_KEY 420

typedef struct twoStep
{
    // boolean value to determine if device has been created
    int dInit;
    // number for the device
    int major;
    // two-step key
    int key;
    // user key
    int userKey;
    // for the character driver
    struct cdev myCDev;
    // holds the class object/driver
    struct class *myclass;
    // two-step timer
    struct timer_list myTimer;
    // boolean for two step expiration
    int timeExpired;
    // boolean for two step pass
    int twoStepPass;
} twoStep;

static twoStep myTwoStep;
static int rSeed = -1;

static int generateSeed(void);
static int rand(void);
static int rNGen(void);

static int initModule(void);
static void exitModule(void);
static void shutDownModule(void);

static char *devnode(struct device *dev, umode_t *mode);

static void twoStepAuthGenerator(void);
static void twoStepAuthStartTimer(void);
static void twoStepAuthEndTimer(struct timer_list *data);
static int twoStepAuthValidator(void);

/**
 * Uses current time to generate a seed
 * Found how to get time for kernel from link:
 * http://practicepeople.blogspot.com/2013/10/kernel-programming-7-ktimeget-and.html
 * @return value for seed
 */
static int generateSeed(void)
{
    ktime_t ct;
    ct = ktime_get();
    return ct;
}

/**
 * Linear congruential generator for random numbers
 * Refernce:
 * https://rosettacode.org/wiki/Linear_congruential_generator#C
 */
static int rand(void)
{
    if (rSeed == -1)
        rSeed = generateSeed();
    return rSeed = (rSeed * 1103515245 + 12345) & MAX_RANDOM_SEED;
}

/**
 * Further randomize number for the key
 * and ensures 0 is not returned
 * @return key value 
 */
static int rNGen(void)
{
    int randomNumber = rand();
    randomNumber = (randomNumber % 10000) * (rand() % 10000);
    if (randomNumber < KEY_MIN_VALUE)
        randomNumber = randomNumber * (rand() % 10);
    if (randomNumber > KEY_MAX_VALUE)
        randomNumber = randomNumber / 10;
    if (randomNumber < KEY_MIN_VALUE)
        randomNumber = rNGen();
    return randomNumber;
}

/**
 * Only creating the functions by the names
 * needed based on the right hand side (fileOps).
 */
static const struct file_operations fileOps = {
    .open = myOpen,
    .release = myRelease,
    .read = myRead,
    .write = myWrite,
    .unlocked_ioctl = myIOCtl,
    .owner = THIS_MODULE,
};

/**
 * Initialize the device driver
 */
static int initModule(void)
{
    // Initialze inital values
    myTwoStep.dInit = 0;
    myTwoStep.major = -1;
    myTwoStep.myclass = NULL;
    myTwoStep.key = -1;
    myTwoStep.userKey = -1;
    myTwoStep.twoStepPass = -1;

    printk("Creating %s Device Driver\n", NAME);
    // Dynamically allocate the major number to the device
    if (alloc_chrdev_region(&myTwoStep.major, 0, 1, NAME) < 0)
    {
        printk("Error Allocating region for %s Device Driver\n", NAME);
        shutDownModule();
        return -1;
    }

    // Creates the file
    if (!(myTwoStep.myclass = class_create(THIS_MODULE, NAME)))
    {
        printk("Error Creating Class for %s Device Driver\n", NAME);
        shutDownModule();
        return -1;
    }

    // Set the permissions if not set
    myTwoStep.myclass->devnode = devnode;

    // Create the device
    if (!(device_create(myTwoStep.myclass, NULL, myTwoStep.major, NULL, NAME)))
    {
        printk("Error Device Creation for %s Device Driver\n", NAME);
        shutDownModule();
        return -1;
    }

    // Device was created.
    myTwoStep.dInit = 1;

    cdev_init(&myTwoStep.myCDev, &fileOps);

    if (cdev_add(&myTwoStep.myCDev, myTwoStep.major, 1) == -1)
    {
        printk("Error Adding Character Device for %s Device Driver\n", NAME);
        shutDownModule();
        return -1;
    }

    timer_setup(&myTwoStep.myTimer, twoStepAuthEndTimer, 0);
    printk("%s Device Driver Succesfully Created\n", NAME);
    return 0;
}

/**
 * Upon exit call shutDownModule.
 */
static void exitModule(void)
{
    printk("Exiting %s Device Driver\n", NAME);
    shutDownModule();
    return;
}

/**
 * Cleans up the device driver by freeing and destoying
 */
static void shutDownModule(void)
{
    if (myTwoStep.dInit)
    {
        device_destroy(myTwoStep.myclass, myTwoStep.major);
        cdev_del(&myTwoStep.myCDev);
        myTwoStep.dInit = 0;
    }
    if (myTwoStep.myclass)
        class_destroy(myTwoStep.myclass);
    if (myTwoStep.major != -1)
    {
        del_timer(&myTwoStep.myTimer);
        unregister_chrdev_region(myTwoStep.major, 1);
    }
    return;
}

/**
 * Opens the file
 * @param iNP inode pointer
 * @param fP file pointer
 * @return error code
 */
static int myOpen(struct inode *iNP, struct file *fP)
{
    printk(KERN_ERR "2FA_Status: %s's File Open\n", NAME);
    return 0;
}

/**
 * Release the file
 * @param iNP inode pointer
 * @param fP file pointer
 * @return error code
 */
static int myRelease(struct inode *iNP, struct file *fP)
{
    printk(KERN_INFO "2FA_Status: %s device closed\n", NAME);
    return 0;
}

/**
 * Reads the file to the caller
 * @param fP the file pointer
 * @param buf callers buffer
 * @param buffLen length of callers buffer
 * @param offset offset into the buffer
 * @return error code
 */
static ssize_t myRead(struct file *fP, char *buf, size_t buffLen, loff_t *offset)
{
    // system buffer
    char buffer[256] = {0};
    int messageSize;
    if (myTwoStep.key == -1)
    {
        printk(KERN_ALERT "2FA_Status: Does not have a key for the user\n");
        return 1;
    }

    switch (myTwoStep.twoStepPass)
    {
    default:
    case 0:
        sprintf(buffer, "%s", "Invalid Key!\n");
        break;
    case 1:
        sprintf(buffer, "%s", "Succesful two-step authentication!\n");
        break;
    case 2:
        sprintf(buffer, "%s", "Expired Key!\n");
        twoStepAuthGenerator();
        break;
    }

    messageSize = strlen(buffer);

    copy_to_user(buf, buffer, messageSize);

    return 0;
}

/**
 * Prints to kernel alerting the user if invalid key format 
 * @param fP file pointer
 * @param buf caller buffer
 * @param buffLen length of caller buffer
 * @param offset offset into the buffer
 * @return error code
 */
static ssize_t myWrite(struct file *fP, const char *buf, size_t buffLen, loff_t *offset)
{
    myTwoStep.userKey = buffLen;
    if (myTwoStep.userKey < KEY_MIN_VALUE || myTwoStep.userKey > KEY_MAX_VALUE)
    {
        printk(KERN_ALERT "2FA_Status: User entered invalid key format\n");
        myTwoStep.userKey = -1;
        return 1;
    }
    return 0;
}

/**
 * The i/o control, validates or gerenate keys for the caller
 * @param fP file pointer
 * @param cmd command to determine what to do
 * @param data
 * @return error code
 */
static long myIOCtl(struct file *fP, unsigned int cmd, unsigned long data)
{
    int twoStep;
    printk(KERN_INFO "2FA_Status: Entering IOCTL");
    switch (cmd)
    {
    case VALIDATE_KEY:
        printk(KERN_INFO "2FA_Status: Validating user key\n");
        twoStep = twoStepAuthValidator();
        // Copies data from kernel space to user space
        myTwoStep.twoStepPass = twoStep;
        copy_to_user((int32_t *)data, &twoStep, sizeof(int));
        return 0;
    case GENERATE_KEY:
        printk(KERN_INFO "2FA_Status: Generating key\n");
        twoStepAuthGenerator();
        return 0;
    }
    return 1;
}

/**
 * Checks to see if the mode is assigned, if not assign set the permissions.
 * This way the sudo is not required to run the program. The permissions are 
 * _rw_rw_rw
 * @param dev
 * @param mode
 */
static char *devnode(struct device *dev, umode_t *mode)
{
    if (!mode)
        return NULL;
    *mode = RW_PERM;
    return NULL;
}

/**
 * Generates a key for the caller, setting the timer to determine
 * how long they have to submit the correct input.
 */
static void twoStepAuthGenerator(void)
{
    printk(KERN_INFO "2FA_Status: Creating key..\n");

    myTwoStep.key = rNGen();

    printk(KERN_INFO "2FA_Status: Key %d created\n", myTwoStep.key);

    twoStepAuthStartTimer();
    return;
}

/**
 * Starts the timer for how long the caller has to input the key
 * https://embetronicx.com/tutorials/linux/device-drivers/using-kernel-timer-in-linux-device-driver/
 * if time exceeds value locks out
 */
static void twoStepAuthStartTimer(void)
{
    mod_timer(&myTwoStep.myTimer, jiffies + msecs_to_jiffies(TIME_MAX_VALUE));
    printk(KERN_INFO "2FA_Status: Timer successfully started\n");
    myTwoStep.timeExpired = 0;
    return;
}

/**
 * Expires the key
 */
static void twoStepAuthEndTimer(struct timer_list *data)
{
    printk(KERN_INFO "2FA_Status: Key expired\n");
    myTwoStep.timeExpired = 1;
    return;
}

/**
 * Compares caller input with generated key, it will expire the key.
 * @return error code
 */
static int twoStepAuthValidator(void)
{
    int keyBoolean = 0;
    if (myTwoStep.key == -1)
    {
        printk(KERN_ALERT "2FA_Status: Key is missing\n");
        keyBoolean = 1;
    }
    if (myTwoStep.userKey == -1)
    {
        printk(KERN_ALERT "2FA_Status: User Key is missing\n");
        keyBoolean = 1;
    }
    if (keyBoolean)
        return 0;

    printk(KERN_INFO "2FA_Status: Key: %d\n2FA_Status: User Key: %d\n",
           myTwoStep.key, myTwoStep.userKey);

    if (myTwoStep.timeExpired)
        return 2;

    if (myTwoStep.userKey == myTwoStep.key)
        return 1;

    return 0;
}

module_init(initModule);
module_exit(exitModule);
MODULE_LICENSE("GPL");
