# local kernel build dir
KERN_DIR ?= /lib/modules/$(shell uname -r)/build

# user defined kernel dir
# KERN_DIR ?= ~/sources/WSL2-Linux-Kernel

MODULE_NAME := dummy-fb

# userspace test tools
FBDEV  ?= /dev/fb1
OUT    ?= output.png
CFLAGS ?= -O2

.PHONY: all module tests test dump rmmod clean

all: module tests

module:
	$(MAKE) -C $(KERN_DIR) M=$(CURDIR) modules

tests: tests/fb_rectangle tests/fb_str

tests/fb_rectangle: tests/fb_rectangle.c
	$(CC) $(CFLAGS) -o $@ $<

tests/fb_str: tests/fb_str.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(MAKE) -C $(KERN_DIR) M=$(CURDIR) clean
	rm -f tests/fb_rectangle tests/fb_str

test: all
	$(MAKE) rmmod
	sudo insmod $(MODULE_NAME).ko

dump:
	sudo ./tests/fbgrab -d $(FBDEV) $(OUT)

rmmod:
	-sudo sh -c 'echo 0 > /sys/class/vtconsole/vtcon1/bind'
	-sudo rmmod $(MODULE_NAME)

obj-m += $(MODULE_NAME).o
