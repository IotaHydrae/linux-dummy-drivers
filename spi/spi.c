#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>

#define CLASS_NAME "dummy_spi_class"
#define DRV_NAME "dummy_spi"

struct dummy_spi {
	dev_t dev_num;

	struct device *dev;
	struct class *class;
};

static struct dummy_spi dummy_spi;

static inline u32 dummy_spi_read(struct dummy_spi *spi, u32 reg)
{
	return 0;
}

static inline void dummy_spi_write(struct dummy_spi *spi, u32 reg, u32 val)
{

}

static inline void dummy_spi_soft_reset(struct dummy_spi *spi)
{

}

static void dummy_spi_set_bus_freq(struct dummy_spi *spi, u32 bus_freq)
{
	switch (bus_freq) {
	default:
		dev_warn(spi->dev, "given freq not supported!\n");
		break;
	}
}

static void dummy_spi_hw_init(struct dummy_spi *spi)
{
	dev_info(spi->dev, "%s\n", __func__);

	dummy_spi_soft_reset(spi);
	dummy_spi_set_bus_freq(spi, 1000000);
}

static int dummy_spi_register(struct dummy_spi *spi)
{
	struct device *dev;
	int ret;

	ret = alloc_chrdev_region(&spi->dev_num, 0, 1, DRV_NAME);
	if (ret) {
		pr_err("failed to allocate device number\n");
		return ret;
	}

	spi->class = class_create(CLASS_NAME);
	if (IS_ERR(spi->class)) {
		pr_err("failed to create class\n");
		goto err_free_dev_num;
	}

	spi->dev =
		device_create(spi->class, NULL, spi->dev_num, NULL, DRV_NAME);
	if (IS_ERR(spi->dev)) {
		pr_err("failed to create device\n");
		goto err_free_class;
	}
	dev = spi->dev;


	dev_set_drvdata(spi->dev, spi);
	dummy_spi_hw_init(spi);

	dev_info(dev, "new adapter - ready");

	return 0;

err_free_class:
	class_destroy(spi->class);
err_free_dev_num:
	unregister_chrdev_region(spi->dev_num, 1);

	return -ENODEV;
}

static int dummy_spi_unregister(struct dummy_spi *spi)
{
	device_destroy(spi->class, spi->dev_num);
	class_destroy(spi->class);
	unregister_chrdev_region(spi->dev_num, 1);

	return 0;
}

module_driver(dummy_spi, dummy_spi_register, dummy_spi_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy SPI controller driver");
MODULE_LICENSE("GPL");
