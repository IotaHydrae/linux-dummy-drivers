// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * Copyright (C) 2025 embeddedboys, Ltd.
 *
 * Author: Hua Zheng <hua.zheng@embeddedboys.com>
 */

#define pr_fmt(fmt) "dummy-drm: " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>

#define DRV_NAME "dummy-drm"

struct dummy_drm {
    struct class *class;
    struct device *dev;
};

struct dirty_area {
    u32 x1;
    u32 y1;
    u32 x2;
    u32 y2;
};

struct dummy_display {
    u32     xres;
    u32     yres;
    u32     bpp;
    u32     fps;
    u32     rotate;
};

static struct dummy_drm *ddrm;

static const struct dummy_display display = {
    .xres = 480,
    .yres = 320,
    .bpp = 16,
    .fps = 24,
};

static int dummy_drm_alloc(struct dummy_drm *ddrm)
{
    int width, height, bpp, rotate;

    pr_info("%s\n", __func__);

    width  = display.xres;
    height = display.yres;
    bpp    = display.bpp;
    rotate = display.rotate;

    return 0;
}

static int __init dummy_drm_init(void)
{
    pr_info("%s\n", __func__);

    ddrm = kzalloc(sizeof(*ddrm), GFP_KERNEL);
    if (!ddrm) {
        pr_err("failed to allocate ddrm\n");
        return -ENOMEM;
    };

    ddrm->class = class_create(DRV_NAME "class");
    if (IS_ERR(ddrm->class)) {
        pr_err("failed to create class\n");
        goto err_free_ddrm;
    }

    ddrm->dev = device_create(ddrm->class, NULL, MKDEV(0, 0), NULL, DRV_NAME "dev");
    if (IS_ERR(ddrm->dev)) {
        pr_err("failed to create device\n");
        goto err_free_class;
    }

    dummy_drm_alloc(ddrm);
    return 0;

err_free_class:
    class_destroy(ddrm->class);
err_free_ddrm:
    kfree(ddrm);
    return -ENOMEM;
}

static void __exit dummy_drm_exit(void)
{
    pr_info("%s\n", __func__);

    if (ddrm->dev)
        device_destroy(ddrm->class, MKDEV(0, 0));

    if (ddrm->class)
        class_destroy(ddrm->class);

    kfree(ddrm);
}

module_init(dummy_drm_init);
module_exit(dummy_drm_exit);
MODULE_AUTHOR("Hua Zheng <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
