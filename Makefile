ifneq ($(KERNELRELEASE),)
obj-m := mymodule.o 
mymodule-objs := modmain.o
ccflags-y := -Wall -Wextra -Wno-unused-parameter

else
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif
