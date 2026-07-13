#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#define DRV_NAME "dummy-bus"

static int dummy_bus_match(struct device *dev, struct device_driver *drv)
{
	pr_info("%s: match(%s <-> %s)\n", DRV_NAME, dev_name(dev), drv->name);

	return strcmp(dev_name(dev), drv->name) == 0;
}

struct bus_type dummy_bus = {
	.name = DRV_NAME,
	.match = dummy_bus_match,
};
EXPORT_SYMBOL_GPL(dummy_bus);

static int __init dummy_bus_init(void)
{
	int ret;

	/* Will create /sys/bus/dummy-bus and devices, drivers dirs */
	ret = bus_register(&dummy_bus);
	if (ret)
		return ret;

	pr_info("%s, %s: bus registered\n", DRV_NAME, __func__);

	return 0;
}

static void __exit dummy_bus_exit(void)
{
	bus_unregister(&dummy_bus);

	pr_info("%s, %s: bus unregistered\n", DRV_NAME, __func__);
}

module_init(dummy_bus_init);
module_exit(dummy_bus_exit);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy Bus driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("dummy-bus");
