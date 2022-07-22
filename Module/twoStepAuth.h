/**************************************************************
* Name: Justin Lam
* GitHub UserID:  JLamSFSU
* Project: Two Factor Authentification Device Driver
*
* File: twoStepAuth.h
*
* Description: h file for two-step authentication
*              Device Driver module
*
**************************************************************/
#include <linux/device.h>

#define NAME "twoStepAuth"

static int myOpen(struct inode *iNP, struct file *fP);
static int myRelease(struct inode *iNP, struct file *fP);

static ssize_t myRead(struct file *fP, char *buf, size_t buffLen, loff_t *offset);
static ssize_t myWrite(struct file *fP, const char __user *buf, size_t buffLen, loff_t *offset);

static long myIOCtl(struct file *fP, unsigned int cmd, unsigned long data);
