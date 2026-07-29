#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/regulator/consumer.h>

struct dummy_consumer {
	dev_t dev_num;

	struct class *class;
	struct device *dev;

	struct regulator *vdd;
};

static struct dummy_consumer consumer;

static int __init dummy_consumer_drv_register(struct dummy_consumer *consumer)
{
	struct device *dev;
	int ret;

	printk("%s\n", __func__);

	ret = alloc_chrdev_region(&consumer->dev_num, 0, 1, "dummy-consumer");

	consumer->class = class_create("dummy-consumer-class");
	if (IS_ERR(consumer->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	consumer->dev = device_create(consumer->class, NULL, consumer->dev_num,
				      NULL, "dummy-consumer-dev");
	if (IS_ERR(consumer->dev)) {
		pr_err("%s, failed to create device\n", __func__);
		goto out_class;
	}
	dev = consumer->dev;
	dev_info(dev, "%s\n", dev_name(dev));

	/* subsys alloc & register */
	consumer->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(consumer->vdd))
		return dev_err_probe(dev, PTR_ERR(consumer->vdd),
				     "failed to get regulator\n");

	return 0;

/* Uncomment if subsys alloc & register failed, add goto out_dev */
// out_dev:
// 	device_destroy(consumer->class, consumer->dev_num);
out_class:
	class_destroy(consumer->class);
out_dev_num:
	unregister_chrdev_region(consumer->dev_num, 1);
	return -ENODEV;
}

static void __exit dummy_consumer_drv_unregister(struct dummy_consumer *consumer)
{
	device_destroy(consumer->class, consumer->dev_num);
	class_destroy(consumer->class);
	unregister_chrdev_region(consumer->dev_num, 1);

	printk("%s, Goodbye.\n", __func__);
}

module_driver(consumer, dummy_consumer_drv_register,
	      dummy_consumer_drv_unregister);

MODULE_AUTHOR("XXX <xx.xx@xx.xx>");
MODULE_DESCRIPTION("Linux dummy consumer driver.");
MODULE_LICENSE("GPL");
