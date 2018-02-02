#ifndef __scull_ioctl_h
#define __scull_ioctl_h
#include<linux/ioctl.h>
#define SCULL_IOC_TYPE 0XFE

#define SCULL_IOCGDEVSIZE _IOR(SCULL_IOC_TYPE, 0, unsigned int)
#define SCULL_IOCGAKey _IOW(SCULL_IOC_TYPE, 1, unsigned int)  
#define SCULL_IOCSAKey _IOW(SCULL_IOC_TYPE, 2, unsigned int)  

#endif
