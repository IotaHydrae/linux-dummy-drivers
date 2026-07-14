#include <linux/module.h>
#include <linux/device.h>

#include "bus.h"
#include "device.h"
#include "driver.h"

static int dummy_driver_probe(struct dummy_device *ddev)
{
	pr_info("%s, id: 0x%08x\n", __func__, ddev->id);

	return 0;
}

static void dummy_driver_remove(struct dummy_device *ddev)
{
	pr_info("%s, id: 0x%08x\n", __func__, ddev->id);
}

static const struct dummy_device_id dummy_ids[] = {
	/* See device.c `init_name` and bus.c match_func */
	{ "dummy-device" },
	{ /* sentinel */ }
};

static struct dummy_driver dummy_driver = { 
	.driver = {
		.name = "dummy0",
	},
	.probe = dummy_driver_probe,
	.remove = dummy_driver_remove,
	.id_table = dummy_ids,
};
module_dummy_bus_driver(dummy_driver);

MODULE_LICENSE("GPL");
