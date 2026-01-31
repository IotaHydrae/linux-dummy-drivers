
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# user defined kernel dir
# KERN_DIR:=~/sources/WSL2-Linux-Kernel

MODULE_NAME:=dummy-spi

all:
	make -C $(KERN_DIR) M=`pwd` modules

clean:
	make -C $(KERN_DIR) M=`pwd` clean

test: all
	sudo rmmod $(MODULE_NAME).ko || true
	sudo insmod $(MODULE_NAME).ko || true
	echo spidev | sudo tee /sys/bus/spi/devices/spi0.0/driver_override
	echo spi0.0 | sudo tee /sys/bus/spi/drivers/spidev/bind
	sudo chmod 777 /dev/spidev0.0
	dmesg -w

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y += spi.o
