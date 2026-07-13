#include <linux/module.h>
#include <linux/device.h>

#include "bus.h"

static void dummy_device_release(struct device *dev)
{
	pr_info("%s, %s\n", __func__, dev_name(dev));
}

static struct device dummy_device = {
	.init_name = "dummy0",
	.bus = &dummy_bus,
	.release = dummy_device_release,
};

static int __init dummy_device_init(void)
{
	int ret;

	ret = device_register(&dummy_device);
	if (ret)
		return ret;

	pr_info("%s, device registered\n", __func__);

	return 0;
}

static void __exit dummy_device_exit(void)
{
	device_unregister(&dummy_device);

	pr_info("%s, device unregistered\n", __func__);
}

module_init(dummy_device_init);
module_exit(dummy_device_exit);

MODULE_LICENSE("GPL");
