#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include "scull.h"
#include "scull_ioctl.h"
#include <linux/semaphore.h>

struct scull_qset
{
	void **data;
	struct scull_qset * next;
};

struct scull_dev 
{
	struct scull_qset * data;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int access_key;
	struct semaphore sem;
	struct cdev cdev;
	
};

void scull_trim(struct scull_dev *dev )
{
	int i = 0;
	int j = 0;
	struct scull_qset * qsetData;
	struct scull_qset * qsetDataTmp;
	printk(KERN_ALERT "scull_trim invoked!\n");
	if ( dev == NULL )
		return;

	qsetData = dev->data;
	while ( qsetData != NULL )
	{
		i = 0;	
		while( i < SCULL_QSET )
		{
			printk(KERN_ALERT "Inner while i[%d], j[%d], pointer[%u]\n", i,j,(unsigned int)qsetData->data[i]);
			kfree(qsetData->data[i]);
			i++;
		}
		printk(KERN_ALERT "outer while i[%d], j[%d], pointer[%u]\n", i,j,(unsigned int )qsetData->data);
		kfree(qsetData->data);
		qsetDataTmp = qsetData->next;
		printk(KERN_ALERT "outer while 2  i[%d], j[%d], pointer[%u]\n", i,j,(unsigned int)qsetData);
		kfree(qsetData);
		printk(KERN_ALERT " Freed qset[%d]", j++);	
		qsetData = qsetDataTmp;
		printk(KERN_ALERT "outer while 3  i[%d], j[%d], pointer[%u]\n", i,j,(unsigned int ) qsetData);
	}
	dev->size = 0;
	dev->data = NULL;
}

int scull_open(struct inode * inode, struct file * filp)
{
	if ( inode == NULL || filp == NULL )
		return -EFAULT;
	struct scull_dev * dev;
	dev = container_of(inode->i_cdev, struct scull_dev, cdev ); 
	filp->private_data = dev;
	if ( down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (( filp->f_flags & O_ACCMODE )== O_WRONLY )
	{
		printk(KERN_ALERT "scull_trim invoked from open!\n");
		scull_trim(dev);
	}
	up(&dev->sem);
	return 0;	
}

int scull_release(struct inode * inode, struct file * filp)
{
	return 0;	
}
ssize_t scull_read(struct file * filp, char __user * buff, size_t count, loff_t * off )
{
	struct scull_dev * dev;
	struct scull_qset * qset;
	int i;
	int qsetIndex, qsetLocalOffset, quantumIndex, quantumLocalOffset;
	int qsetSize;
	void* quantumData;
	int copyCount;
        ssize_t retVal;
	if ( filp == NULL )
		return -EFAULT;
	dev = filp->private_data;
	if ( dev == NULL )
		return -EFAULT;
	if ( off == NULL )
		return -EFAULT;

	if ( down_interruptible(&dev->sem));
		return -ERESTARTSYS;

	if ( *off >= dev->size ) 
	{
		printk( KERN_ALERT "Requested offset %lld is greater than size %lu\n", *off, dev->size );
		retVal = -EFAULT;
	}
	else
	{
		qsetSize = SCULL_QUANTUM * SCULL_QSET;	
		qsetIndex = (int)(*off)/ qsetSize;
		qsetLocalOffset = (int)(*off) % qsetSize;
		quantumIndex = qsetLocalOffset / SCULL_QUANTUM;
		quantumLocalOffset = qsetLocalOffset % SCULL_QUANTUM;
		printk(KERN_ALERT "*off [%d], count [%d],  qsetIndex [%d], qsetLocalOffset [%d], quantumIndex[%d], quantumLocalOffset[%d]\n", 
							(int)(*off), count, qsetIndex , qsetLocalOffset , quantumIndex, quantumLocalOffset);
		qset = dev->data;
		i = qsetIndex;
		while ( qset != NULL && i > 0)
		{
			qset = qset->next;
			i--;
		}
		if ( qset == NULL )
		{
			printk( KERN_ALERT "dev->data NULL !\n ");
			retVal =  -EFAULT;
		}	
		else if ( qset->data == NULL )
		{
			printk(KERN_ALERT "qset->data NULL !!\n");
			retVal = -EFAULT;
		}
		else
		{
			quantumData = qset->data[quantumIndex];
			copyCount = ( count <= (SCULL_QUANTUM - quantumLocalOffset) ? count :(SCULL_QUANTUM - quantumLocalOffset));
			copy_to_user ( buff, quantumData + quantumLocalOffset, copyCount);
			*off += copyCount;
			printk( KERN_ALERT "Copied %d bytes to user \n", copyCount);	
			retVal = copyCount;
		}
	}
	up(&dev->sem);
	return retVal;
}

struct scull_qset * scull_allocate_qset(void)
{
	struct scull_qset * qset;
	int i;
	qset = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
	qset->next = NULL;
	qset->data = kmalloc(sizeof(void*) * SCULL_QSET, GFP_KERNEL);
	for ( i = 0; i < SCULL_QSET; i++ )
	{
		qset->data[i] = kmalloc(SCULL_QUANTUM, GFP_KERNEL);
		memset(qset->data[i], 0 , SCULL_QUANTUM);
	}
	return qset;
}

ssize_t scull_write(struct file * filp, const char __user * buff, size_t count, loff_t * off )
{
	struct scull_dev * dev;
	struct scull_qset ** prev;
	struct scull_qset ** curr;
	int i;
	int copyCount;
	int qsetIndex, qsetLocalOffset, quantumIndex, quantumLocalOffset;
	int qsetSize;
	ssize_t retVal;
	if ( filp == NULL )
		return -EFAULT;
	dev = filp->private_data;
	if ( dev == NULL )
		return -EFAULT;
	if ( off == NULL )
		return -EFAULT;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if ( *off > dev->size ) 
	{
		printk( KERN_ALERT "Requested offset %lld is greater than size %lu\n", *off, dev->size );
		retVal = -EFAULT;
	}
        else
        {
		qsetSize = SCULL_QUANTUM * SCULL_QSET;	
		qsetIndex = (int)(*off)/ qsetSize;
		qsetLocalOffset = (int)(*off) % qsetSize;
		quantumIndex = qsetLocalOffset / SCULL_QUANTUM;
		quantumLocalOffset = qsetLocalOffset % SCULL_QUANTUM;
		printk(KERN_ALERT "*off [%d], count [%d],  qsetIndex [%d], qsetLocalOffset [%d], quantumIndex[%d], quantumLocalOffset[%d]\n", 
							(int)(*off), count, qsetIndex , qsetLocalOffset , quantumIndex, quantumLocalOffset);
		i = qsetIndex;
		prev = NULL;	
		curr = &dev->data;
		while ( *curr != NULL && i > 0)
		{
			prev = curr;
			curr = &((*curr)->next);
			i--;
		}
		if ( *curr == NULL )
		{
			printk(KERN_ALERT "Allocating a new scull_qset!\n");
			*curr = scull_allocate_qset();
			if ( prev != NULL )
			{		
				(*prev)->next = *curr;
			}
			else
			{
			   printk(KERN_ALERT "Allocated a new scull data chain !\n");		
			}	
			if ( quantumIndex != 0 || quantumLocalOffset != 0 )
			{
				printk(KERN_ALERT " Inconsistent state !!, new qset created but quantumIndex is %d and quantumLocalOffset is %d", quantumIndex, quantumLocalOffset );
			}
		}
		copyCount = ( count < ( SCULL_QUANTUM - quantumLocalOffset ) ? count : ( SCULL_QUANTUM - quantumLocalOffset ));
		copy_from_user ( (*curr)->data[quantumIndex] + quantumLocalOffset, buff, copyCount );
		*off += copyCount;
		dev->size += copyCount;
		printk( KERN_ALERT "Copied %d bytes from user \n", copyCount);
		retVal = copyCount;
	}
        up(&dev->sem);
	return copyCount;
}

long scull_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
        printk(KERN_ALERT "Received IOCTL call %u\n", cmd);
	long retVal;
	struct scull_dev *dev;
	if ( _IOC_TYPE(cmd) != SCULL_IOC_TYPE ) {
        	printk(KERN_ALERT "IOC type not matching %u\n", _IOC_TYPE(cmd));
		return -ENOTTY;
	}
	if ( filp == NULL ) {
        	printk(KERN_ALERT "filep NULL\n");
		return -EFAULT;
	}
	dev = filp->private_data;
	if ( dev == NULL ) {
		printk(KERN_ALERT "dev is nULL!\n");
		return -EFAULT;
	}
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	switch(cmd)
	{
	    case SCULL_IOCGDEVSIZE:
		printk(KERN_ALERT "Got IOCTL 0\n");
		retVal = put_user(dev->size, (unsigned int __user *)(arg));
		break;
	    case SCULL_IOCGAKey:
		printk(KERN_ALERT "Got IOCTL 1\n");
		retVal = put_user(dev->access_key, (unsigned int __user *)(arg));
		break;
	    case SCULL_IOCSAKey:
		printk(KERN_ALERT "Got IOCTL 2\n");
		retVal = get_user(dev->access_key, (unsigned int __user *)(arg));
		break;
	    default:		
		printk(KERN_ALERT "Unknown IOCTL number:%u\n", _IOC_NR(cmd));	 
	} 
	up(&dev->sem);
	return retVal;
}

struct file_operations f_ops = {
	.open    = scull_open,
	.release = scull_release,
	.read  = scull_read,
	.write = scull_write,
	.unlocked_ioctl = scull_ioctl
};

static int scull_setup_chardev_regions(void)
{
	int err;
	dev_t dev;
	dev = MKDEV(SCULL_MAJOR, SCULL_MINOR);
	if ( MAJOR(dev) )
	{
		err = register_chrdev_region ( dev, SCULL_NUM_DEVICES, "SCULL");
	}
	else
	{
		err = alloc_chrdev_region( &dev, SCULL_MINOR, SCULL_NUM_DEVICES, "SCULL");
		SCULL_MAJOR = MAJOR(dev);
	}
	if ( err )
	{
		printk(KERN_ALERT " Unable to allocate/register char dev region with dev MAJOR %d\n", SCULL_MAJOR );
	}
	else
	{
		printk(KERN_ALERT " Allocated/Registered char dev region with dev MAJOR %d and dev MINOR %d\n", SCULL_MAJOR, SCULL_MINOR );
	}
	return err;
}

struct scull_dev scull0;

static int setup_scull(struct scull_dev * dev, int scullNum)
{
	int err;
	dev_t devNo;
	scull0.data = NULL;
	scull0.quantum = SCULL_QUANTUM;
	scull0.qset = SCULL_QSET;
	scull0.size = 0;
	sema_init(&scull0.sem, 1);
	cdev_init(&dev->cdev, &f_ops);
	devNo = MKDEV(SCULL_MAJOR, SCULL_MINOR+scullNum);
	err = cdev_add(&dev->cdev, devNo, 1);
 	if ( err )
	{
		printk( KERN_ALERT "Unable to setup scull_dev devNo %d", scullNum);	
	}
	return err;
}
static int __init scull_init(void)
{
	int err = scull_setup_chardev_regions();
	if ( !err)
	{
		err = setup_scull(&scull0, 0);		
	}
	return err;
}	

static void __exit scull_exit(void)
{
	dev_t dev = MKDEV( SCULL_MAJOR, SCULL_MINOR );
	unregister_chrdev_region(dev, SCULL_NUM_DEVICES);
	printk(KERN_ALERT "scull_trim invoked from exit!\n");
	scull_trim(&scull0);
}

module_init(scull_init);
module_exit(scull_exit);
