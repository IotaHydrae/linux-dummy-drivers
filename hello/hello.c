#include <linux/module.h>

static int __init hello_drv_init(void)
{
	printk("Hello, world!\n");

	return 0;
}

static void __exit hello_drv_exit(void)
{
	printk("Goodbye.\n");
}

module_init(hello_drv_init);
module_exit(hello_drv_exit);

MODULE_AUTHOR("XXX <xxx@xxx.com>");
MODULE_DESCRIPTION("Linux hello world driver.");
MODULE_LICENSE("GPL");
