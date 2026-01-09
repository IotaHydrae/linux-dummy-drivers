#include "linux/types.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#define CLASS_NAME "dummy_i2c_class"
#define DRV_NAME "dummy_i2c"

struct dummy_i2c {
	dev_t dev_num;

	struct device *dev;
	struct class *class;

	struct i2c_adapter adapter;
};

static struct dummy_i2c dummy_i2c;

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
	struct device *dev;
	int ret;

	ret = alloc_chrdev_region(&i2c->dev_num, 0, 1, DRV_NAME);
	if (ret) {
		pr_err("failed to allocate device number\n");
		return ret;
	}

	i2c->class = class_create(CLASS_NAME);
	if (IS_ERR(i2c->class)) {
		pr_err("failed to create class\n");
		goto err_free_dev_num;
	}

	i2c->dev =
		device_create(i2c->class, NULL, i2c->dev_num, NULL, DRV_NAME);
	if (IS_ERR(i2c->dev)) {
		pr_err("failed to create device\n");
		goto err_free_class;
	}
	dev = i2c->dev;

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.algo = &dummy_i2c_algo;
	i2c->adapter.algo_data = NULL;
	i2c->adapter.retries = 3;
	i2c->adapter.timeout = msecs_to_jiffies(50);
	i2c->adapter.nr = 0;
	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
		 DRV_NAME " bus%d", i2c->adapter.nr);

	i2c_set_adapdata(&i2c->adapter, i2c);
	dev_set_drvdata(i2c->dev, i2c);

	ret = i2c_add_numbered_adapter(&i2c->adapter);
	if (ret) {
		dev_err(dev, "failed to add adapter\n");
		goto err_free_dev;
	}

	dev_info(dev, "adapter i2c-%d ready", i2c->adapter.nr);

	return 0;

err_free_dev:
	i2c_del_adapter(&i2c->adapter);
	device_destroy(i2c->class, i2c->dev_num);
err_free_class:
	class_destroy(i2c->class);
err_free_dev_num:
	unregister_chrdev_region(i2c->dev_num, 1);

	return -ENODEV;
}

static int dummy_i2c_unregister(struct dummy_i2c *i2c)
{
	i2c_del_adapter(&i2c->adapter);
	device_destroy(i2c->class, i2c->dev_num);
	class_destroy(i2c->class);
	unregister_chrdev_region(i2c->dev_num, 1);

	return 0;
}

module_driver(dummy_i2c, dummy_i2c_register, dummy_i2c_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy I2C adapter driver");
MODULE_LICENSE("GPL");
