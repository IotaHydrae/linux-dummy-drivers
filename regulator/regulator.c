#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>

#define DUMMY_REGULATOR_LDO1_VOUT 0x01
#define DUMMY_REGULATOR_ENABLE	  0x08
#define DUMMY_REGULATOR_VOUT_M	  0x1F
#define DUMMY_REGULATOR_EN_LDO1_M BIT(0)

enum dummy_regulator_id {
	DUMMY_REGULATOR_ID_LDO1,
	DUMMY_REGULATOR_ID_LDO2,
};

struct dummy_regulator {
	dev_t dev_num;

	struct class *class;
	struct device *dev;

	bool enabled;
	struct gpio_desc *enable;

	int voltage;
	unsigned int mode;

	int num_regulators;
};

static struct dummy_regulator regulator;

static const unsigned int dummy_regulator_ldo_vtbl[] = {
	1200000, 1250000, 1300000, 1350000, 1400000, 1450000, 1500000, 1550000,
	1600000, 1650000, 1700000, 1750000, 1800000, 1850000, 1900000, 2000000,
	2100000, 2200000, 2300000, 2400000, 2500000, 2600000, 2650000, 2700000,
	2750000, 2800000, 2850000, 2900000, 2950000, 3000000, 3100000, 3300000,
};

static int dummy_regulator_enable(struct regulator_dev *rdev)
{
	struct dummy_regulator *regulator = rdev_get_drvdata(rdev);

	regulator->enabled = true;

	dev_info(&rdev->dev, "enable\n");

	return 0;
}

static int dummy_regulator_is_enabled(struct regulator_dev *rdev)
{
	struct dummy_regulator *regulator = rdev_get_drvdata(rdev);

	return regulator->enabled;
}

static int dummy_regulator_set_voltage(struct regulator_dev *rdev, int min_uV,
				       int max_uV, unsigned int *selector)
{
	struct dummy_regulator *regulator = rdev_get_drvdata(rdev);

	regulator->voltage = min_uV;

	dev_info(&rdev->dev, "set voltage %d\n", min_uV);

	return 0;
}

static int dummy_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct dummy_regulator *regulator = rdev_get_drvdata(rdev);

	return regulator->voltage;
}

static const struct regulator_ops dummy_regulator_ldo_ops = {
	// .list_voltage = regulator_list_voltage_table,
	.enable	     = dummy_regulator_enable,
	.is_enabled  = dummy_regulator_is_enabled,
	.set_voltage = dummy_regulator_set_voltage,
	.get_voltage = dummy_regulator_get_voltage,
};

static const struct regulator_desc dummy_regulator_desc = {
	.name	     = "dummy-ldo1",
	.ops	     = &dummy_regulator_ldo_ops,
	.type	     = REGULATOR_VOLTAGE,
	.owner	     = THIS_MODULE,
	.id	     = DUMMY_REGULATOR_ID_LDO1,
	.n_voltages  = ARRAY_SIZE(dummy_regulator_ldo_vtbl),
	.volt_table  = dummy_regulator_ldo_vtbl,
	.vsel_reg    = DUMMY_REGULATOR_LDO1_VOUT,
	.vsel_mask   = DUMMY_REGULATOR_VOUT_M,
	.enable_reg  = DUMMY_REGULATOR_ENABLE,
	.enable_mask = DUMMY_REGULATOR_EN_LDO1_M,
};

static struct regulator_consumer_supply dummy_regulator_supplies[] = {
	{
		.dev_name = "dummy-dev",
		.supply	  = "vdd",
	},
};

static struct regulator_init_data dummy_regulator_init_data = {
	.constraints = {
		.min_uV = 1200000,
		.max_uV = 3300000,
	},

	.num_consumer_supplies = ARRAY_SIZE(dummy_regulator_supplies),
	.consumer_supplies = dummy_regulator_supplies,
};

static int dummy_regulator_hw_enable(struct dummy_regulator *regulator)
{
	dev_info(regulator->dev, "%s\n", __func__);
	return 0;
}

static int dummy_regulator_register(struct dummy_regulator *regulator)
{
	const struct regulator_desc *desc;
	struct regulator_config cfg;
	struct regulator_dev *rdev;

	desc = &dummy_regulator_desc;

	cfg.dev		= regulator->dev;
	cfg.init_data	= &dummy_regulator_init_data;
	cfg.driver_data = regulator;

	rdev = devm_regulator_register(regulator->dev, desc, &cfg);
	if (IS_ERR(rdev)) {
		dev_err(regulator->dev, "regulator register err");
		return PTR_ERR(rdev);
	}

	return 0;
}

static int __init dummy_regulator_drv_register(struct dummy_regulator *regulator)
{
	struct device *dev;
	int ret;

	printk("%s\n", __func__);

	ret = alloc_chrdev_region(&regulator->dev_num, 0, 1, "dummy-regulator");

	regulator->class = class_create("dummy-regulator-class");
	if (IS_ERR(regulator->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	regulator->dev = device_create(regulator->class, NULL,
				       regulator->dev_num, NULL,
				       "dummy-regulator-dev");
	if (IS_ERR(regulator->dev)) {
		pr_err("%s, failed to create device\n", __func__);
		goto out_class;
	}
	dev = regulator->dev;

	/* subsys alloc & register */
	regulator->num_regulators = 4;

	dummy_regulator_hw_enable(regulator);

	return dummy_regulator_register(regulator);

/* Uncomment if subsys alloc & register failed, add goto out_dev */
// out_dev:
// 	device_destroy(regulator->class, regulator->dev_num);
out_class:
	class_destroy(regulator->class);
out_dev_num:
	unregister_chrdev_region(regulator->dev_num, 1);
	return -ENODEV;
}

static void __exit
dummy_regulator_drv_unregister(struct dummy_regulator *regulator)
{
	device_destroy(regulator->class, regulator->dev_num);
	class_destroy(regulator->class);
	unregister_chrdev_region(regulator->dev_num, 1);

	printk("%s, Goodbye.\n", __func__);
}

module_driver(regulator, dummy_regulator_drv_register,
	      dummy_regulator_drv_unregister);

MODULE_AUTHOR("XXX <xx.xx@xx.xx>");
MODULE_DESCRIPTION("Linux dummy regulator driver.");
MODULE_LICENSE("GPL");
