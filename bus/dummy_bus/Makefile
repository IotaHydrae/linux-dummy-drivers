
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# custom kernel build dir
# KERN_DIR:=~/WSL2-Linux-Kernel

all:
	make -C $(KERN_DIR) M=`pwd` modules

clean: rmmod
	make -C $(KERN_DIR) M=`pwd` clean

test: rmmod all insmod

.PHONY: insmod rmmod clean test
insmod:
	@sudo insmod bus.ko || true
	@sudo insmod device.ko || true
	@sudo insmod driver.ko || true
rmmod:
	@sudo rmmod device.ko || true
	@sudo rmmod driver.ko || true
	@sudo rmmod bus.ko || true

obj-m += bus.o device.o driver.o
