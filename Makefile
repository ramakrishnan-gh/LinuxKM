#obj-m += hello1.o
#obj-m += procfs1.o
#obj-m += procfs2.o
#obj-m += chardev.o
#obj-m += chardev1.o
obj-m += scull.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules	

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
