
# local kernel build dir
KERN_DIR:=/lib/modules/$(shell uname -r)/build

# users kernel dir
# KERN_DIR:=/home/user/linux

all:
	bear -- make -C $(KERN_DIR) M=`pwd` modules

clean:
	bear -- make -C $(KERN_DIR) M=`pwd` clean

obj-m+=dummy-video.o

dummy-video-y += video.o
dummy-video-y += red.o
dummy-video-y += green.o
dummy-video-y += blue.o
