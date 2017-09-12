obj-m := myUSBdrive.o


KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.* *.symvers *.order *~
