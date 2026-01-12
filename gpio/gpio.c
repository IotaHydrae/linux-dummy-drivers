#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>

#define DRV_NAME "dummy_gpio"

struct dummy_gpio {
	dev_t dev_num;

	struct class *class;
	struct device *dev;

	struct gpio_chip gc;

	u16 ngpio;
	u16 gpio_val_bits;
	u16 gpio_dir_bits;
};

static struct dummy_gpio dummy_gpio = {
	.ngpio = 16,
};

static int dummy_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);

	return 0;
}

static void dummy_gpio_set(struct gpio_chip *chip, unsigned int offset,
			   int value)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);
}

static int dummy_gpio_request(struct gpio_chip *chip, unsigned int offset)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);

	return 0;
}

static void dummy_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);
}

static int dummy_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);

	return GPIO_LINE_DIRECTION_OUT;
}

static int dummy_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);

	return 0;
}

static int dummy_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
				       int value)
{
	struct dummy_gpio *gpio = gpiochip_get_data(chip);

	dev_info(gpio->dev, "%s\n", __func__);

	return 0;
}

static int dummy_gpio_register(struct dummy_gpio *gpio)
{
	struct device *dev;
	int ret;

	ret = alloc_chrdev_region(&gpio->dev_num, 0, 1, DRV_NAME);
	if (ret)
		return ret;

	gpio->class = class_create("dummy_gpio_class");
	if (IS_ERR(gpio->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	gpio->dev =
		device_create(gpio->class, NULL, gpio->dev_num, NULL, DRV_NAME);
	if (IS_ERR(gpio->dev)) {
		goto out_class;
	}
	dev = gpio->dev;

	gpio->gc.label = DRV_NAME;
	gpio->gc.parent = dev;
	gpio->gc.owner = THIS_MODULE;
	gpio->gc.base = -1;
	gpio->gc.ngpio = gpio->ngpio;
	gpio->gc.can_sleep = true;
	gpio->gc.set = dummy_gpio_set;
	gpio->gc.get = dummy_gpio_get;
	gpio->gc.request = dummy_gpio_request;
	gpio->gc.free = dummy_gpio_free;
	gpio->gc.get_direction = dummy_gpio_get_direction;
	gpio->gc.direction_input = dummy_gpio_direction_input;
	gpio->gc.direction_output = dummy_gpio_direction_output;

	dev_set_drvdata(dev, gpio);

	devm_gpiochip_add_data(dev, &gpio->gc, gpio);
	if (ret < 0) {
		dev_err(dev, "failed to add gpio chip: %d\n", ret);
		goto out_dev;
	}

	return 0;

out_dev:
	device_destroy(gpio->class, gpio->dev_num);
out_class:
	class_destroy(gpio->class);
out_dev_num:
	unregister_chrdev_region(gpio->dev_num, 1);

	return -ENODEV;
}

static int dummy_gpio_unregister(struct dummy_gpio *gpio)
{
	device_destroy(gpio->class, gpio->dev_num);
	class_destroy(gpio->class);
	unregister_chrdev_region(gpio->dev_num, 1);

	return 0;
}

module_driver(dummy_gpio, dummy_gpio_register, dummy_gpio_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy GPIO chip driver");
MODULE_LICENSE("GPL");
