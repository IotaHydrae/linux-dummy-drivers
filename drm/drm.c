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
#include <video/mipi_display.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_managed.h>

#define DRV_NAME "dummy-drm"

struct dummy_device {
    struct class *class;
    struct device *dev;
};

struct dummy_drm_priv {
    struct drm_device *drm;
    struct dummy_dev *dev;
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

static struct dummy_device *ddev;

static const struct dummy_display display = {
    .xres = 480,
    .yres = 320,
    .bpp = 16,
    .fps = 24,
};

DEFINE_DRM_GEM_DMA_FOPS(dummy_drm_fops);

static const struct drm_driver dummy_drm_driver = {
    .driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
    .fops = &dummy_drm_fops,
    DRM_GEM_DMA_DRIVER_OPS_VMAP,
    .name = "dummy-drm",
    .desc = "Dummy DRM driver",
    .date = "20250111",
    .major = 1,
    .minor = 0,
};

static int dummy_drm_alloc(struct dummy_device *ddev)
{
    struct dummy_drm_priv *priv;
    int width, height, bpp, rotate;

    pr_info("%s\n", __func__);

    width  = display.xres;
    height = display.yres;
    bpp    = display.bpp;
    rotate = display.rotate;

    priv = devm_drm_dev_alloc(ddev->dev, &dummy_drm_driver,
        struct dummy_drm_priv, drm);
    if (IS_ERR(priv))
        return PTR_ERR(priv);

    drm_dev_register(priv->drm, 0);
    // drm_fbdev_generic_setup(priv->drm, 0);

    return 0;
}

static int __init dummy_drm_init(void)
{
    pr_info("%s\n", __func__);

    ddev = kzalloc(sizeof(*ddev), GFP_KERNEL);
    if (!ddev) {
        pr_err("failed to allocate ddev\n");
        return -ENOMEM;
    };

    ddev->class = class_create(DRV_NAME "class");
    if (IS_ERR(ddev->class)) {
        pr_err("failed to create class\n");
        goto err_free_ddev;
    }

    ddev->dev = device_create(ddev->class, NULL, MKDEV(0, 0), NULL, DRV_NAME "dev");
    if (IS_ERR(ddev->dev)) {
        pr_err("failed to create device\n");
        goto err_free_class;
    }

    dummy_drm_alloc(ddev);
    return 0;

err_free_class:
    class_destroy(ddev->class);
err_free_ddev:
    kfree(ddev);
    return -ENOMEM;
}

static void __exit dummy_drm_exit(void)
{
    pr_info("%s\n", __func__);

    if (ddev->dev)
        device_destroy(ddev->class, MKDEV(0, 0));

    if (ddev->class)
        class_destroy(ddev->class);

    kfree(ddev);
}

module_init(dummy_drm_init);
module_exit(dummy_drm_exit);
MODULE_AUTHOR("Hua Zheng <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
