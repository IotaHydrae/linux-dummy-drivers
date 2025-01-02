#include <linux/kernel.h>
#include <linux/module.h>

static int __init dummy_fb_init(void)
{
    printk("%s\n", __func__);
    return 0;
}

static void __exit dummy_fb_exit(void)
{
    printk("%s\n", __func__);
}

module_init(dummy_fb_init);
module_exit(dummy_fb_exit);
MODULE_AUTHOR("Hua Zheng <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
