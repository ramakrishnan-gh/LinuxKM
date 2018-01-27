#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>

static struct proc_dir_entry * my_proc_entry;


static void *
mydrv_seq_start( struct seq_file *sf, loff_t *pos)
{

   static int counter = 0;

   if ( *pos == 0)
   {
      printk(KERN_INFO "Seq Start called returning pos 0\n");
      return &counter;
   }
   else
   {
      printk(KERN_INFO "Seq Start called with pos = %lld\n", *pos);
      return NULL;
   }
}

static int
mydrv_seq_show( struct seq_file *sf, void *v)
{
   seq_printf(sf, "Seq Show with pos = %d\n", (*(int*)v));
   return 0;
}


static void *
mydrv_seq_next( struct seq_file *sf, void *v, loff_t *pos )
{
    int *p = (int*)v;
    if ( *pos < 5 )
    {
         *pos = *pos + 1;
         (*p)++;
         printk(KERN_INFO "Seq next return  with pos = %lld\n", *pos);
         return v;
    }  
    else
    {
       
         printk(KERN_INFO "Seq next done with pos = %lld\n", *pos);
          return NULL;
    }

}

static void mydrv_seq_stop( struct seq_file *sf, void *v)
{
   printk(KERN_INFO "Seq Stop called");
}

/*
static int
procfile_show(struct seq_file *sf, void *v)
{
   seq_printf(sf, "Hello Ramki Proc !\n");
   return 0;
}*/


static struct seq_operations seq_ops = {
   .start = mydrv_seq_start,
   .next = mydrv_seq_next,
   .show = mydrv_seq_show,
   .stop = mydrv_seq_stop
};

static int
procfile_open( struct inode *in, struct file * file )
{
   return seq_open(file, &seq_ops );
}

static struct file_operations proc_fops = {
   .open =  procfile_open,
   .release = seq_release,
   .read = seq_read,
   .owner = THIS_MODULE,
   .llseek = seq_lseek,
};

int init_module(void)
{
   printk(KERN_INFO "Inside Procfs Hello\n");
   my_proc_entry = proc_create("testProc", 0, NULL, &proc_fops );
   if ( my_proc_entry == NULL )
   {
      printk(KERN_ALERT "testProc couun't be created!!\n");
      remove_proc_entry("testProc", NULL);
      return -ENOMEM;
   } 

   printk(KERN_ALERT "testProc created succesfully!");   
   return 0;	
}

void cleanup_module(void)
{
//   remove_proc_entry("testProc", &proc_root);
      remove_proc_entry("testProc", NULL);
   printk(KERN_INFO " Exiting Procfs\n");
}
