# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

ifneq ($(KERNELRELEASE),)
	obj-m := matmul.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
