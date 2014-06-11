ifneq ($(KERNELRELEASE),)
obj-m := mymodule.o 
mymodule-objs := modmain.o ringbuffer.o fifodev.o
ccflags-y := -Wall -Wextra -Wno-unused-parameter -std=gnu99 -Wno-declaration-after-statement

else
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: default

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif
