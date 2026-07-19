
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# users kernel dir
# KERN_DIR:=/home/user/linux

all:
	make -C $(KERN_DIR) M=`pwd` modules

clean:
	make -C $(KERN_DIR) M=`pwd` clean

clangd: clean
	unset http_proxy https_proxy
	bear -- make -C $(KERN_DIR) M=`pwd` modules

test: all
	@sudo rmmod dummy-video.ko || true
	@sudo insmod dummy-video.ko debug=2 || true

obj-m+=dummy-video.o

dummy-video-y += video.o
