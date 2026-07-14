#ifndef __LINUX_DUMMY_BUS_H
#define __LINUX_DUMMY_BUS_H

#include <linux/device.h>

#include "driver.h"

extern struct bus_type dummy_bus_type;

int dummy_bus_register_driver(struct module *owner,
			      struct dummy_driver *driver);
void dummy_bus_del_driver(struct dummy_driver *driver);

#define dummy_bus_add_driver(drv) dummy_bus_register_driver(THIS_MODULE, drv)

#define module_dummy_bus_driver(__dummy_driver)             \
	module_driver(__dummy_driver, dummy_bus_add_driver, \
		      dummy_bus_del_driver)

#endif
