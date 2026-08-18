
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# user defined kernel dir
# KERN_DIR:=~/sources/WSL2-Linux-Kernel

MODULE_NAME:=dummy-drm

TEST_BINS := $(wildcard tests/*.c)
TEST_BINS := $(TEST_BINS:.c=)

all: $(TEST_BINS)
	make -C $(KERN_DIR) M=`pwd` modules

tests/%: tests/%.c
	$(CC) -o $@ $<

clean:
	make -C $(KERN_DIR) M=`pwd` clean
	rm -f $(TEST_BINS)

test: all
	sudo rmmod $(MODULE_NAME).ko || true
	sudo insmod $(MODULE_NAME).ko || true

install: all
	sudo make -C $(KERN_DIR) M=`pwd` modules_install
	sudo depmod -a
	echo $(MODULE_NAME) | sudo tee /etc/modules-load.d/$(MODULE_NAME).conf > /dev/null
	sudo modprobe $(MODULE_NAME)

preview:
	sudo python3 tools/fbview.py

dump:
	sudo fbgrab -d /dev/fb0 dump.png

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y += drm.o

.PHONY: all clean test install preview dump
