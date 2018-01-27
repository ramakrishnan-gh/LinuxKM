#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>

int a=20;

int init_module()
{
   printk(KERN_INFO "My char driver Entry %d=\n", a);
   a++; 
   return 0;
}

void cleanup_module()
{
   printk(KERN_INFO "My char driver Exit a= %d \n", a);
}

