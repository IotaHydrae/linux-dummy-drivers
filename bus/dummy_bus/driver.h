#ifndef __LINUX_DUMMY_BUS_DRIVER_H
#define __LINUX_DUMMY_BUS_DRIVER_H

#include <linux/device.h>

struct dummy_device;

struct dummy_device_id {
	char name[32];
	const void *data;
};

struct dummy_driver {
	int (*probe)(struct dummy_device *ddev);
	void (*remove)(struct dummy_device *ddev);

	const struct dummy_device_id *id_table;
	struct device_driver driver;
};

#define to_dummy_driver(drv) container_of(drv, struct dummy_driver, driver)

#endif
