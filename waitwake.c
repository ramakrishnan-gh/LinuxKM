#include<linux/module.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include<linux/fs.h>
#include<linux/sched.h>
#include <asm/uaccess.h>

/* Program to demonstrate blocking IO */
/* running the read bumps up the read counter and gets blocked on the read counter at that time ( notice the use of stack variable during wait_event call 
a write with appropriate char byte unblocks the reader */

/* cat /dev/waitwake will get blocked till a appropriate echo '<char>' > /dev/waitwake is done */

static atomic_t rCounter; 
static atomic_t wCounter; 
static struct cdev waitwake_dev;
static dev_t devId;

static wait_queue_head_t wqHead;

int waitwake_open( struct inode * inode, struct file * fp )
{
   printk(KERN_ALERT "waitwake_open called\n");
   if ( inode != NULL )
   {
	if ( fp != NULL )
	{
	    fp->private_data = inode->i_cdev;
	}
   }
   return 0;	
}

ssize_t waitwake_read(struct file *fp, char __user * buff, size_t size, loff_t * offset)
{
   int waitOn = atomic_inc_return(&rCounter);
   printk(KERN_ALERT "PID[%i] Read waiting on counter %d\n", current->pid, waitOn);
   wait_event_interruptible(wqHead, waitOn < atomic_read(&wCounter));
   printk(KERN_ALERT "PID[%i] Read unblocked with r %d and w %d!\n", current->pid, waitOn, atomic_read(&wCounter));
   return 0;
}  

ssize_t waitwake_write(struct file *fp, const char __user * buff, size_t size, loff_t * offset)
{
   char i = 0;
   printk(KERN_ALERT "waitwake_write called\n");
   if ( get_user(i, buff) == 0 )
   {
   	printk(KERN_ALERT "waitwake_write received char %c\n", i);
	if ( i >= 'a' && i <= 'z' )
	      i = i - 'a';
   	atomic_set(&wCounter, (int)i);
	printk(KERN_ALERT "Setting wCounter to %d\n", atomic_read(&wCounter));
   	wake_up_interruptible(&wqHead);
   }

   return size;
}  

static struct file_operations  f_ops = {
	.open = waitwake_open,
	.read = waitwake_read,
	.write = waitwake_write
};

static int __init waitwake_init(void)
{
   int retVal;
   
   devId  = MKDEV(0,0);
   retVal  = alloc_chrdev_region(&devId, 0, 1, "waitwake");

   init_waitqueue_head(&wqHead);

   atomic_set(&rCounter, 0);
   atomic_set(&wCounter, 0);
   if ( retVal == 0 )
   {
	cdev_init(&waitwake_dev, &f_ops);
	retVal = cdev_add(&waitwake_dev, devId, 1); 	 	 	
	if ( retVal == 0 )
	   printk(KERN_ALERT "Successfully added device with major rev %d\n", MAJOR(devId)); 
   }
   return 0;
}

static void __exit waitwake_exit(void)
{
   unregister_chrdev_region(devId,1);
}

module_init(waitwake_init);
module_exit(waitwake_exit);
