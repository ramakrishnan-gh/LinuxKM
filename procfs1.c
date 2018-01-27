#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>

static struct proc_dir_entry * my_proc_entry;



static int
procfile_show(struct seq_file *sf, void *v)
{
   seq_printf(sf, "Hello Ramki Proc !\n");
   return 0;
}

static int
procfile_open( struct inode *in, struct file * file )
{
   return single_open(file, procfile_show, NULL );
}

static struct file_operations proc_fops = {
   .open =  procfile_open,
   .release = single_release,
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
      printk(KERN_ALERT "testProc couldn't be created!!\n");
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
