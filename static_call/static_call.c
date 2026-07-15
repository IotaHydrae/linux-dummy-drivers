#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/static_call.h>

struct dummy_static_call {
	dev_t dev_num;

	struct class *class;
	struct device *dev;
};

static struct dummy_static_call dummy_static_call;
static void (*normal_call)(void);

static int func_a(int x)
{
	pr_info("static_call -> %s, x : %d\n", __func__, x);

	return x;
}

static int func_b(int x)
{
	x += 10;

	pr_info("static_call -> %s, x + 10 : %d\n", __func__, x);

	return x;
}

static void func_c(void)
{
	pr_info("normal_call -> %s\n", __func__);
}

DEFINE_STATIC_CALL(dummy_static_call, func_a);

static int __init
dummy_static_call_drv_register(struct dummy_static_call *dummy_static_call)
{
	struct device *dev;
	int ret;

	printk("%s\n", __func__);

	ret = alloc_chrdev_region(&dummy_static_call->dev_num, 0, 1,
				  "dummy-static_call");

	dummy_static_call->class = class_create("dummy-static_call-class");
	if (IS_ERR(dummy_static_call->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	dummy_static_call->dev = device_create(dummy_static_call->class, NULL,
					       dummy_static_call->dev_num, NULL,
					       "dummy-static_call-dev");
	if (IS_ERR(dummy_static_call->dev)) {
		pr_err("%s, failed to create device\n", __func__);
		goto out_class;
	}
	dev = dummy_static_call->dev;

	static_call(dummy_static_call)(10);

	static_call_update(dummy_static_call, func_b);

	ret = static_call(dummy_static_call)(10);
	pr_info("%s, static_call -> func_b ret = %d\n", __func__, ret);

	normal_call = func_c;
	normal_call();

	return 0;

/* Uncomment if subsys alloc & register failed, add goto out_dev */
// out_dev:
// 	device_destroy(dummy_static_call->class, dummy_static_call->dev_num);
out_class:
	class_destroy(dummy_static_call->class);
out_dev_num:
	unregister_chrdev_region(dummy_static_call->dev_num, 1);
	return -ENODEV;
}

static void __exit
dummy_static_call_drv_unregister(struct dummy_static_call *dummy_static_call)
{
	device_destroy(dummy_static_call->class, dummy_static_call->dev_num);
	class_destroy(dummy_static_call->class);
	unregister_chrdev_region(dummy_static_call->dev_num, 1);

	printk("%s, Goodbye.\n", __func__);
}

module_driver(dummy_static_call, dummy_static_call_drv_register,
	      dummy_static_call_drv_unregister);

MODULE_AUTHOR("XXX <xx.xx@xx.xx>");
MODULE_DESCRIPTION("Linux dummy static_call driver.");
MODULE_LICENSE("GPL");
