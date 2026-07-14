#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#include "bus.h"
#include "device.h"
#include "driver.h"

int dummy_bus_register_driver(struct module *owner, struct dummy_driver *driver)
{
	int ret;

	driver->driver.owner = owner;
	driver->driver.bus = &dummy_bus_type;

	ret = driver_register(&driver->driver);
	if (ret)
		return ret;

	pr_info("driver [%s] registered\n", driver->driver.name);

	return 0;
}
EXPORT_SYMBOL_GPL(dummy_bus_register_driver);

void dummy_bus_del_driver(struct dummy_driver *driver)
{
	driver_unregister(&driver->driver);
	pr_info("driver [%s] unregistered\n", driver->driver.name);
}
EXPORT_SYMBOL_GPL(dummy_bus_del_driver);

static int dummy_bus_match(struct device *dev, struct device_driver *drv)
{
	const struct dummy_device_id *ids;
	struct dummy_driver *ddrv;

	ddrv = to_dummy_driver(drv);
	ids = ddrv->id_table;

	while (ids && ids->name[0]) {
		pr_info("%s, dev_name: %s, drv->ids: %s\n", __func__,
			dev_name(dev), ids->name);
		if (!strcmp(dev_name(dev), ids->name)) {
			pr_info("%s: match(%s <-> %s)\n", __func__,
				dev_name(dev), drv->name);
			return 1;
		}

		ids++;
	}

	return 0;
}

static int dummy_bus_probe(struct device *dev)
{
	struct dummy_device *ddev;
	struct dummy_driver *ddrv;

	ddev = to_dummy_device(dev);
	ddrv = to_dummy_driver(dev->driver);

	if (ddrv->probe) {
		pr_info("%s, probe %s\n", __func__, dev_name(dev));
		return ddrv->probe(ddev);
	}

	return 0;
}

static void dummy_bus_remove(struct device *dev)
{
	struct dummy_device *ddev = to_dummy_device(dev);
	struct dummy_driver *ddrv = to_dummy_driver(dev->driver);

	if (ddrv->remove) {
		pr_info("%s, remove %s\n", __func__, dev_name(dev));
		ddrv->remove(ddev);
	}
}

struct bus_type dummy_bus_type = {
	.name = "dummy-bus",
	.match = dummy_bus_match,
	.probe = dummy_bus_probe,
	.remove = dummy_bus_remove,
};
EXPORT_SYMBOL_GPL(dummy_bus_type);

static int __init dummy_bus_init(void)
{
	int ret;

	/* Will create /sys/bus/dummy-bus and devices, drivers dirs */
	ret = bus_register(&dummy_bus_type);
	if (ret)
		return ret;

	pr_info("%s, bus registered\n", __func__);

	return 0;
}

static void __exit dummy_bus_exit(void)
{
	bus_unregister(&dummy_bus_type);

	pr_info("%s, bus unregistered\n", __func__);
}

module_init(dummy_bus_init);
module_exit(dummy_bus_exit);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy Bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("dummy-bus");
