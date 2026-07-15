#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/cdev.h>

struct dummy_firmware {
	dev_t dev_num;

	struct class *class;
	struct device *dev;

	const struct firmware *fw;
};

static struct dummy_firmware dfw;

static int __init dummy_firmware_drv_register(struct dummy_firmware *dfw)
{
	const struct firmware *fw;
	struct device *dev;
	int ret;

	printk("%s\n", __func__);

	ret = alloc_chrdev_region(&dfw->dev_num, 0, 1, "dummy-firmware");

	dfw->class = class_create("dummy-firmware-class");
	if (IS_ERR(dfw->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	dfw->dev = device_create(dfw->class, NULL, dfw->dev_num, NULL,
				 "dummy-firmware-dev");
	if (IS_ERR(dfw->dev)) {
		pr_err("%s, failed to create device\n", __func__);
		goto out_class;
	}
	dev = dfw->dev;

	// struct file *fp;

	// fp = filp_open("/lib/firmware/dummy-firmware.bin", O_RDONLY, 0);

	// if (IS_ERR(fp)) {
	// 	pr_err("filp_open failed: %ld\n", PTR_ERR(fp));
	// } else {
	// 	pr_info("filp_open success!\n");
	// 	filp_close(fp, NULL);
	// }

	ret = request_firmware(&fw, "dummy-firmware.bin", dev);
	if (ret) {
		pr_info("request_firmware failed\n");
		goto out_dev;
	}
	dfw->fw = fw;

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, fw->data,
		       fw->size, true);

	release_firmware(fw);

	return 0;

out_dev:
	device_destroy(dfw->class, dfw->dev_num);
out_class:
	class_destroy(dfw->class);
out_dev_num:
	unregister_chrdev_region(dfw->dev_num, 1);
	return -ENODEV;
}

static void __exit dummy_firmware_drv_unregister(struct dummy_firmware *dfw)
{
	device_destroy(dfw->class, dfw->dev_num);
	class_destroy(dfw->class);
	unregister_chrdev_region(dfw->dev_num, 1);

	printk("%s, Goodbye.\n", __func__);
}

module_driver(dfw, dummy_firmware_drv_register, dummy_firmware_drv_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Linux dummy firmware load and print dump driver.");
MODULE_LICENSE("GPL");
