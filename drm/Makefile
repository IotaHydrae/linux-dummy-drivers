
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# user defined kernel dir
# KERN_DIR:=~/sources/WSL2-Linux-Kernel

MODULE_NAME:=dummy-drm

all: fb_rectangle fb_str fbcat
	make -C $(KERN_DIR) M=`pwd` modules

fb_rectangle: tests/fb_rectangle.c
	$(CC) -o tests/$@ $^

fb_str: tests/fb_str.c
	$(CC) -o tests/$@ $^

fbcat: tests/fbcat.c
	$(CC) -o tests/$@ $^

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean

test: all
	sudo rmmod $(MODULE_NAME).ko || true
	sudo insmod $(MODULE_NAME).ko || true

dump: fbcat
	cd tests && sudo ./fbgrab -d /dev/fb0 ../dump.png

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y += drm.o dma_gem_dma_helper.o
