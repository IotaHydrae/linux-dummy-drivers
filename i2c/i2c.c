#include "linux/types.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/i2c.h>

#define CLASS_NAME "dummy_i2c_class"
#define DRV_NAME "dummy_i2c"

#define DUMMY_I2C_BUS_CLOCK_NORMAL 100000
#define DUMMY_I2C_BUS_CLOCK_FAST 400000
#define DUMMY_I2C_BUS_CLOCK_DEFAULT DUMMY_I2C_BUS_CLOCK_FAST

#define DUMMY_I2C_ADDR(addr) ((0xff & addr) << 1)
#define DUMMY_I2C_REG_CLOCK_M(clkm) ((0xf & clkm) << 3)
#define DUMMY_I2C_REG_CLOCK_N(clkn) (0x7 & clkn)

#define DUMMY_I2C_SPEED_100K \
	(DUMMY_I2C_REG_CLOCK_M(2) | DUMMY_I2C_REG_CLOCK_N(11))

/* Dummy I2C control register bits */
#define DUMMY_I2C_REG_CONTROL_INT_EN BIT(7) /* I2C Interrupt enable */
#define DUMMY_I2C_REG_CONTROL_BUS_EN BIT(6) /* I2C Bus enable */
#define DUMMY_I2C_REG_CONTROL_M_STA BIT(5) /* I2C Start signal flag */
#define DUMMY_I2C_REG_CONTROL_M_STP BIT(4) /* I2C Stop singal flag */
#define DUMMY_I2C_REG_CONTROL_INT_FLAG BIT(3) /* I2C Interrupt flag */
#define DUMMY_I2C_REG_CONTROL_A_ACK BIT(2) /* I2C ACK singal flag */

#define DUMMY_I2C_REG_SRST_SOFT_RESET BIT(0)

#define DUMMY_I2C_BUS_DIR_WR 0x00
#define DUMMY_I2C_BUS_DIR_RD 0x01

struct dummy_i2c_regs {
	u8 addr; /* I2C Slave Address Register */
	u8 xaddr; /* I2C Extend Address Register */
	u8 data; /* I2C Data Register */
	u8 cntr; /* I2C Control Register */
	u8 stat; /* I2C Status Register */
	u8 ccr; /* I2C Clock Register */
	u8 srst; /* I2C Soft Reset Register */
	u8 efr; /* I2C Enhance Feature Register */
	u8 lcr; /* I2C Line Control Register */
};

struct dummy_i2c {
	dev_t dev_num;

	struct device *dev;
	struct class *class;

	void __iomem *base;
	int irq;

	u32 dir; /* Direction of I2C msg */
	u32 addr; /* Addr of current device */
	u32 xaddr; /* Extend addr of current device */
	u32 cntr_bits; /* Control register value */
	u32 byte_left; /* Left byte need to be transfer */
	u32 byte_pos; /* Position of current I2C msg buf */
	u32 bus_freq; /* Current Bus Speed */

	struct dummy_i2c_regs reg_offsets;

	struct i2c_adapter adapter;
	struct i2c_msg *curr_msg;
	struct i2c_msg *msgs;
	int mum_msgs;

	struct clk *hclk;
	struct clk *mclk;

	struct completion complete;
	struct reset_control *rstc;
};

static struct dummy_i2c dummy_i2c;

struct dummy_i2c_regs dummy_i2c_regs_variant0 = {
	.addr = 0x00, /* addr and xaddr is used in slave mode */
	.xaddr = 0x04,
	.data = 0x08,
	.cntr = 0x0c,
	.stat = 0x10,
	.ccr = 0x14,
	.srst = 0x18,
	.efr = 0x1c,
	.lcr = 0x20,
};

static inline u32 dummy_i2c_read(struct dummy_i2c *i2c, u32 reg)
{
	// return readl(i2c->base + reg);
	return 0;
}

static inline void dummy_i2c_write(struct dummy_i2c *i2c, u32 reg, u32 val)
{
	// writel(val, i2c->base + reg);
}

static inline void dummy_i2c_soft_reset(struct dummy_i2c *i2c)
{
	dummy_i2c_write(i2c, i2c->reg_offsets.srst,
			DUMMY_I2C_REG_SRST_SOFT_RESET);
}

static void dummy_i2c_set_bus_freq(struct dummy_i2c *i2c, u32 bus_freq)
{
	switch (bus_freq) {
	case DUMMY_I2C_BUS_CLOCK_NORMAL:
		i2c->bus_freq = DUMMY_I2C_SPEED_100K;
		break;
	case DUMMY_I2C_BUS_CLOCK_FAST:
	default:
		dev_warn(i2c->dev, "given freq not supported!\n");
		break;
	}

	dummy_i2c_write(i2c, i2c->reg_offsets.ccr, i2c->bus_freq);
}

static void dummy_i2c_hw_init(struct dummy_i2c *i2c)
{
	dev_info(i2c->dev, "%s\n", __func__);

	dummy_i2c_soft_reset(i2c);

	dummy_i2c_set_bus_freq(i2c, DUMMY_I2C_BUS_CLOCK_NORMAL);

	/* Clear the necessary registers */
	dummy_i2c_write(i2c, i2c->reg_offsets.addr, 0);
	dummy_i2c_write(i2c, i2c->reg_offsets.xaddr, 0);
	dummy_i2c_write(i2c, i2c->reg_offsets.cntr, 0);

	/* Enable bus */
	i2c->cntr_bits |= DUMMY_I2C_REG_CONTROL_BUS_EN;
	dummy_i2c_write(i2c, i2c->reg_offsets.cntr,
			i2c->cntr_bits | DUMMY_I2C_REG_CONTROL_M_STP);
}

static inline void dummy_i2c_send_stop(struct dummy_i2c *i2c)
{
	dummy_i2c_write(i2c, i2c->reg_offsets.cntr,
			i2c->cntr_bits | DUMMY_I2C_REG_CONTROL_M_STP);

	complete(&i2c->complete);
}

static inline void dummy_i2c_clear_irq(struct dummy_i2c *i2c)
{
	i2c->cntr_bits &= ~(DUMMY_I2C_REG_CONTROL_INT_FLAG);
	dummy_i2c_write(i2c, i2c->reg_offsets.cntr, i2c->cntr_bits);
}

static __maybe_unused irqreturn_t dummy_i2c_isr(int irq, void *dev_id)
{
	struct dummy_i2c *i2c = dev_id;
	u8 *msg_buf = i2c->curr_msg->buf;

	/* NOTE: read status reg and do the corresponding action */

	for (; i2c->byte_left--; i2c->byte_pos++) {
		if (i2c->dir == DUMMY_I2C_BUS_DIR_WR)
			dummy_i2c_write(i2c, i2c->reg_offsets.data,
					msg_buf[i2c->byte_pos]);
		else
			msg_buf[i2c->byte_pos] = get_random_u8();

		dev_info(i2c->dev, "%s, %s: 0x%02x\n", __func__,
			 i2c->dir ? "rd" : "wr", msg_buf[i2c->byte_pos]);
	}

	dummy_i2c_send_stop(i2c);
	dummy_i2c_clear_irq(i2c);

	return IRQ_HANDLED;
}

static void dummy_i2c_send_start(struct dummy_i2c *i2c, struct i2c_msg *msg)
{
	i2c->byte_left = msg->len;
	i2c->curr_msg = msg;
	i2c->byte_pos = 0;

	dev_info(i2c->dev, "%s, addr: 0x%02x\n", __func__, msg->addr);

	if (msg->flags & I2C_M_RD)
		i2c->dir = DUMMY_I2C_BUS_DIR_RD;
	else
		i2c->dir = DUMMY_I2C_BUS_DIR_WR;

	if (msg->flags & I2C_M_TEN) {
		i2c->addr = DUMMY_I2C_ADDR(msg->addr) | i2c->dir;
		i2c->xaddr = (u32)msg->addr & 0xff;
	} else {
		i2c->addr = DUMMY_I2C_ADDR(msg->addr) | i2c->dir;
		i2c->xaddr = 0;
	}

	dummy_i2c_write(i2c, i2c->reg_offsets.cntr,
			i2c->cntr_bits | DUMMY_I2C_REG_CONTROL_M_STA);

	/* NOTE: Interrupt routines should be called externally. */
	dummy_i2c_isr(i2c->irq, i2c);
}

static int dummy_i2c_do_msgs(struct dummy_i2c *i2c)
{
	struct i2c_msg *msgs = i2c->msgs;
	int num = i2c->mum_msgs;
	unsigned long time_left;
	int i;

	for (i = 0; i < num; i++) {
		reinit_completion(&i2c->complete);

		dummy_i2c_send_start(i2c, &msgs[i]);

		time_left = wait_for_completion_timeout(&i2c->complete,
							i2c->adapter.timeout);

		if (!time_left)
			dummy_i2c_hw_init(i2c);
	}

	return 0;
}

static int dummy_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
			  int num)
{
	struct dummy_i2c *i2c = i2c_get_adapdata(adap);

	dev_info(i2c->dev, "%s, msg len : %d\n", __func__, num);

	i2c->mum_msgs = num;
	i2c->msgs = msgs;

	dummy_i2c_do_msgs(i2c);

	return num;
}

static u32 dummy_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm dummy_i2c_algo = {
	.master_xfer = dummy_i2c_xfer,
	.functionality = dummy_i2c_functionality,
};

static int dummy_i2c_register(struct dummy_i2c *i2c)
{
	struct device *dev;
	int ret;

	ret = alloc_chrdev_region(&i2c->dev_num, 0, 1, DRV_NAME);
	if (ret) {
		pr_err("failed to allocate device number\n");
		return ret;
	}

	i2c->class = class_create(CLASS_NAME);
	if (IS_ERR(i2c->class)) {
		pr_err("failed to create class\n");
		goto err_free_dev_num;
	}

	i2c->dev =
		device_create(i2c->class, NULL, i2c->dev_num, NULL, DRV_NAME);
	if (IS_ERR(i2c->dev)) {
		pr_err("failed to create device\n");
		goto err_free_class;
	}
	dev = i2c->dev;

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.algo = &dummy_i2c_algo;
	i2c->adapter.algo_data = NULL;
	i2c->adapter.retries = 3;
	i2c->adapter.timeout = msecs_to_jiffies(50);
	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name), DRV_NAME " bus");

	i2c_set_adapdata(&i2c->adapter, i2c);
	dev_set_drvdata(i2c->dev, i2c);

	init_completion(&i2c->complete);

	dummy_i2c_hw_init(i2c);

	/*
	 * If you already know which I2C adapter this is, use this
	 * method instead.
	 */
	// i2c->adapter.nr = 0;
	// snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
	// 	 DRV_NAME " bus %d", i2c->adapter.nr);
	// ret = i2c_add_numbered_adapter(&i2c->adapter);

	/*
	 * If you have defined an alias for the i2c adapter node in the
	 * device tree, use `i2c_add_adapter` directly.
	 */
	ret = i2c_add_adapter(&i2c->adapter);
	if (ret) {
		dev_err(dev, "failed to add adapter\n");
		goto err_free_dev;
	}

	/*
	 * If we don't set adapter.nr or set with -1, `i2c_add_adapter` will
	 * attempt to assign a number automatically. In this case, you should
	 * manually append the number after the adapter name. This is not a
	 * required operation, but it can provide more infomation.
	 */
	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
		 DRV_NAME " bus %d", i2c->adapter.nr);

	dev_info(dev, "new adapter [%s] on <i2c-%d> - ready", i2c->adapter.name,
		 i2c->adapter.nr);

	return 0;

err_free_dev:
	i2c_del_adapter(&i2c->adapter);
	device_destroy(i2c->class, i2c->dev_num);
err_free_class:
	class_destroy(i2c->class);
err_free_dev_num:
	unregister_chrdev_region(i2c->dev_num, 1);

	return -ENODEV;
}

static int dummy_i2c_unregister(struct dummy_i2c *i2c)
{
	i2c_del_adapter(&i2c->adapter);
	device_destroy(i2c->class, i2c->dev_num);
	class_destroy(i2c->class);
	unregister_chrdev_region(i2c->dev_num, 1);

	return 0;
}

module_driver(dummy_i2c, dummy_i2c_register, dummy_i2c_unregister);

MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy I2C adapter driver");
MODULE_LICENSE("GPL");
