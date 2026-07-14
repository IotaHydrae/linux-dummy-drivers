#ifndef __LINUX_DUMMY_BUS_DEVICE_H
#define __LINUX_DUMMY_BUS_DEVICE_H

#include <linux/device.h>

struct dummy_device {
	int id;
	struct device dev;
};

#define to_dummy_device(dev) container_of(dev, struct dummy_device, dev)

#endif
