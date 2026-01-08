#include "linux/types.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#define CLASS_NAME "dummy_i2c_class"
#define DRV_NAME "dummy_i2c"

struct dummy_i2c {
	struct device *dev;
	struct class *class;

	struct i2c_adapter adapter;
};

static struct dummy_i2c dummy_i2c = {
	.class = NULL,
	.dev = NULL,
};

static int dummy_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
			  int num)
{
	return num;
}

static u32 dummy_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm dummy_i2c_algo = {
	.master_xfer = dummy_i2c_xfer,
	.functionality = dummy_i2c_functionality,
};

static int dummy_i2c_register(struct dummy_i2c *i2c)
{
	struct class *class = i2c->class;
	struct device *dev = i2c->dev;

	class = class_create(CLASS_NAME);
	if (IS_ERR(class)) {
		pr_err("failed to create class\n");
		return -ENOMEM;
	}

	dev = device_create(class, NULL, MKDEV(0, 0), NULL, DRV_NAME);
	if (IS_ERR(dev)) {
		pr_err("failed to create device\n");
		goto err_free_class;
	}

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.algo = &dummy_i2c_algo;
	i2c->adapter.algo_data = NULL;
	i2c->adapter.retries = 3;
	i2c->adapter.timeout = msecs_to_jiffies(50);
	i2c->adapter.nr = 0;
	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
		 DRV_NAME " bus%d", i2c->adapter.nr);

	i2c_set_adapdata(&i2c->adapter, i2c);
	dev_set_drvdata(dev, i2c);

	i2c_add_numbered_adapter(&i2c->adapter);

	return 0;

// err_free_dev:
// 	device_destroy(i2c->class, MKDEV(0, 0));
err_free_class:
	class_destroy(i2c->class);
	return -ENODEV;
}

static int dummy_i2c_unregister(struct dummy_i2c *i2c)
{
	i2c_del_adapter(&i2c->adapter);

	if (i2c->dev)
		device_destroy(i2c->class, MKDEV(0, 0));

	if (i2c->class)
		class_destroy(i2c->class);
	return 0;
}

module_driver(dummy_i2c, dummy_i2c_register, dummy_i2c_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy I2C adapter driver");
MODULE_LICENSE("GPL");
