KERNELDIR=/lib/modules/$(shell uname -r)/build

obj-m :=smartWindow_dev.o

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
