#include <linux/module.h>
#include <linux/device.h>

#include "bus.h"

static int dummy_driver_probe(struct device *dev)
{
	pr_info("%s\n", __func__);

	return 0;
}

static int dummy_driver_remove(struct device *dev)
{
	pr_info("%s\n", __func__);

	return 0;
}

static struct device_driver dummy_driver = {
	.name = "dummy0",
	.bus = &dummy_bus,
	.probe = dummy_driver_probe,
	.remove = dummy_driver_remove,
};

static int __init dummy_driver_init(void)
{
	int ret;

	ret = driver_register(&dummy_driver);
	if (ret)
		return ret;

	pr_info("%s, driver registered\n", __func__);

	return 0;
}

static void __exit dummy_driver_exit(void)
{
	driver_unregister(&dummy_driver);

	pr_info("%s, driver unregistered\n", __func__);
}

module_init(dummy_driver_init);
module_exit(dummy_driver_exit);

MODULE_LICENSE("GPL");
