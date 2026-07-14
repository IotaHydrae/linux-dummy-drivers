#include <linux/module.h>
#include <linux/device.h>

#include "bus.h"
#include "device.h"

static void dummy_device_release(struct device *dev)
{
	pr_info("%s, %s\n", __func__, dev_name(dev));
}

static struct dummy_device ddev = {
	.id = 0x12345678,
};

static int __init dummy_device_init(struct dummy_device *ddev)
{
	int ret;

	ddev->dev.init_name = "dummy-device";
	ddev->dev.bus = &dummy_bus_type;
	ddev->dev.release = dummy_device_release,

	ret = device_register(&ddev->dev);
	if (ret)
		return ret;

	pr_info("%s, device registered\n", __func__);

	return 0;
}

static void __exit dummy_device_exit(struct dummy_device *ddev)
{
	device_unregister(&ddev->dev);

	pr_info("%s, device unregistered\n", __func__);
}

module_driver(ddev, dummy_device_init, dummy_device_exit);

MODULE_LICENSE("GPL");
