#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<asm/uaccess.h>

static bool devOpen = false;

static char drvMsg[100] = "This is print from my char driver\n";

static int msgIdx = 0;

static int mydev_open(struct inode *in, struct file * fp)
{
   if ( devOpen )
   {
      printk(KERN_INFO "Device already open !");
      return -EBUSY;
   }
   msgIdx = 0;
   devOpen = true;
   return 0;
}

static int mydev_release(struct inode *in, struct file * fp)
{
   devOpen = false;
   return 0;
}

static ssize_t mydev_read( struct file * fp, char *buff, size_t len, loff_t *offset)
{
   int bytesRead = 0;

   printk(KERN_INFO "offset is:%lld\n", *offset);
   printk(KERN_INFO "len  is:%d\n", len);

   while ( len > 0 && msgIdx < 100 )
   {
      put_user( drvMsg[msgIdx++], buff++);
      len--;
      bytesRead++; 
      if ( bytesRead >= 10 ) break;
   }

   printk(KERN_INFO "Bytes Read is:%d", bytesRead);
   return bytesRead;
}


static ssize_t mydev_write( struct file * fp, const char *buff, size_t len, loff_t *offset)
{
   printk(KERN_ALERT "Dev Write not supported !\n");
   if ( devOpen )
   {
      if ( msgIdx + len < 99 )
      {
         memcpy( drvMsg+msgIdx, buff, len );
         msgIdx += len;
         return len;
      }
   }
   return -1;
}

static struct file_operations mydev_fs = {
   .open    = mydev_open,
   .read    = mydev_read,
   .write   = mydev_write,
   .release = mydev_release
};

static int myMajorRev;

int init_module()
{
   printk(KERN_INFO "My char driver Entry \n");

   myMajorRev  = register_chrdev(0, "myCharDev", &mydev_fs); 
   if ( myMajorRev > 0 )
   {
      printk(KERN_INFO "myCharDev created with Major Rev:%d\n", myMajorRev);
      return 0;
   }
   return -1;
}

void cleanup_module()
{
   printk(KERN_INFO "My char driver Exit ");
   unregister_chrdev(myMajorRev, "myCharDev");
}

