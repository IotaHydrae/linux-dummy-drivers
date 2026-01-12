
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# user defined kernel dir
# KERN_DIR:=~/sources/WSL2-Linux-Kernel

MODULE_NAME:=dummy-gpio

all:
	make -C $(KERN_DIR) M=`pwd` modules

clean:
	make -C $(KERN_DIR) M=`pwd` clean

test: all
	sudo rmmod $(MODULE_NAME).ko || true
	sudo insmod $(MODULE_NAME).ko || true

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y += gpio.o
