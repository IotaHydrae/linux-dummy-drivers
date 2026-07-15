#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

struct dummy_template {
	dev_t dev_num;

	struct class *class;
	struct device *dev;
};

static struct dummy_template template;

static int __init dummy_template_drv_register(struct dummy_template *template)
{
	struct device *dev;
	int ret;

	printk("%s\n", __func__);

	ret = alloc_chrdev_region(&template->dev_num, 0, 1, "dummy-template");

	template->class = class_create("dummy-template-class");
	if (IS_ERR(template->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	template->dev = device_create(template->class, NULL, template->dev_num,
				      NULL, "dummy-template-dev");
	if (IS_ERR(template->dev)) {
		pr_err("%s, failed to create device\n", __func__);
		goto out_class;
	}
	dev = template->dev;

	/* subsys alloc & register */

	return 0;

/* Uncomment if subsys alloc & register failed, add goto out_dev */
// out_dev:
// 	device_destroy(template->class, template->dev_num);
out_class:
	class_destroy(template->class);
out_dev_num:
	unregister_chrdev_region(template->dev_num, 1);
	return -ENODEV;
}

static void __exit dummy_template_drv_unregister(struct dummy_template *template)
{
	device_destroy(template->class, template->dev_num);
	class_destroy(template->class);
	unregister_chrdev_region(template->dev_num, 1);

	printk("%s, Goodbye.\n", __func__);
}

module_driver(template, dummy_template_drv_register,
	      dummy_template_drv_unregister);

MODULE_AUTHOR("XXX <xx.xx@xx.xx>");
MODULE_DESCRIPTION("Linux dummy template driver.");
MODULE_LICENSE("GPL");
