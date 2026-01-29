#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>

#define CLASS_NAME "dummy_spi_class"
#define DRV_NAME "dummy_spi"

#define DUMMY_SPI_MAX_XFER_SIZE 256
#define DUMMY_SPI_BUF_SIZE (DUMMY_SPI_MAX_XFER_SIZE + 16)

struct dummy_spi {
	dev_t dev_num;

	struct device *dev;
	struct class *class;

	struct spi_controller *host;

	void *buf;

	u8 bpw;
	u32 speed;
	u16 mode;
	u8 cs;
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
	struct spi_controller *host;
	struct dummy_spi *dspi;
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
		goto exit_free_dev_num;
	}

	spi->dev =
		device_create(spi->class, NULL, spi->dev_num, NULL, DRV_NAME);
	if (IS_ERR(spi->dev)) {
		pr_err("failed to create device\n");
		goto exit_free_class;
	}
	dev = spi->dev;

	host = devm_spi_alloc_host(dev, sizeof(struct dummy_spi));
	if (!host)
		goto exit_free_dev;

	device_set_node(&host->dev, dev_fwnode(dev));
	dev_set_drvdata(spi->dev, host);

	/* this is an example */
	dspi = spi_controller_get_devdata(host);

	spi->buf = devm_kmalloc(dev, DUMMY_SPI_BUF_SIZE, GFP_KERNEL);
	if (!spi->buf)
		return -ENOMEM;

	spi->host = host;
	/* cs/mode can never be 0xff, so the first transfer will set them */
	spi->cs = 0xff;
	spi->mode = 0xff;

	/* TODO: setup spi hardware */
	dummy_spi_hw_init(spi);

	host->bus_num = -1;
	host->mode_bits = SPI_CPOL | SPI_CPHA;
	host->prepare_message = NULL;
	host->transfer_one = NULL;
	host->auto_runtime_pm = false;

	/* TODO: enable SPI module */

	ret = devm_spi_register_controller(dev, host);
	if (ret < 0) {
		dev_err(dev, "Failed to register host\n");
		goto exit_free_dev;
	}

	dev_info(dev, "new adapter - ready");

	return 0;

exit_free_dev:
	device_destroy(spi->class, spi->dev_num);
exit_free_class:
	class_destroy(spi->class);
exit_free_dev_num:
	unregister_chrdev_region(spi->dev_num, 1);

	return ret;
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
MODULE_ALIAS("dummy-spi");
