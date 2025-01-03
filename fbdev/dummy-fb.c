// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * Copyright (C) 2025 embeddedboys, Ltd.
 *
 * Author: Hua Zheng <hua.zheng@embeddedboys.com>
 */

#define pr_fmt(fmt) "dummy-fb: " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>

#define DRV_NAME "dummy-fb"

struct dummy_fb {
    struct class *class;
    struct device *dev;

    struct fb_deferred_io *fbdefio;
    struct fb_info *fbinfo;
    struct fb_ops *fbops;
};

struct dummy_display {
    u32     xres;
    u32     yres;
    u32     bpp;
    u32     fps;
    u32     rotate;
};

static struct dummy_fb *dfb;

static const struct dummy_display display = {
    .xres = 480,
    .yres = 320,
    .bpp = 16,
    .fps = 24,
};

static int dummy_fb_alloc(struct dummy_fb *dfb)
{
    struct fb_deferred_io *fbdefio;
    struct fb_ops *fbops;
    struct fb_info *fbinfo;
    int width, height, bpp, rotate;
    u8 *vmem = NULL;
    int vmem_size;
    int rc;

    pr_info("%s\n", __func__);

    width = display.xres;
    height = display.yres;
    bpp = display.bpp;
    rotate = display.rotate;

    vmem_size = (width * height * bpp) / BITS_PER_BYTE;
    pr_info("vmem_size: %d\n", vmem_size);
    vmem = vzalloc(vmem_size);
    if (!vmem) {
        pr_err("failed to allocate vmem\n");
        return -ENOMEM;
    }

    fbops = kzalloc(sizeof(*fbops), GFP_KERNEL);
    if (!fbops) {
        pr_err("failed to allocate fbops\n");
        goto err_free_vmem;
    }

    fbdefio = kzalloc(sizeof(*fbdefio), GFP_KERNEL);
    if (!fbdefio) {
        pr_err("failed to allocate fbdefio\n");
        goto err_free_fbops;
    }

    fbinfo = framebuffer_alloc(0, dfb->dev);
    if (!fbinfo) {
        pr_err("failed to allocate fbinfo\n");
        goto err_free_fbdefio;
    }

    fbops->owner = THIS_MODULE;
    fbinfo->screen_buffer = vmem;
    fbinfo->fbops = fbops;
    fbinfo->fbdefio = fbdefio;

    return 0;

err_free_fbdefio:
    kfree(fbdefio);
err_free_fbops:
    kfree(fbops);
err_free_vmem:
    vfree(vmem);
    return -ENOMEM;
}

static int __init dummy_fb_init(void)
{
    pr_info("%s\n", __func__);

    dfb = kzalloc(sizeof(*dfb), GFP_KERNEL);
    if (!dfb) {
        pr_err("failed to allocate dfb\n");
        return -ENOMEM;
    };

    dfb->class = class_create(DRV_NAME "class");
    if (IS_ERR(dfb->class)) {
        pr_err("failed to create class\n");
        goto err_free_dfb;
    }

    dfb->dev = device_create(dfb->class, NULL, MKDEV(0, 0), NULL, DRV_NAME "dev");
    if (IS_ERR(dfb->dev)) {
        pr_err("failed to create device\n");
        goto err_free_class;
    }

    dummy_fb_alloc(dfb);
    return 0;

err_free_class:
    class_destroy(dfb->class);
err_free_dfb:
    kfree(dfb);
    return -ENOMEM;
}

static void __exit dummy_fb_exit(void)
{
    pr_info("%s\n", __func__);

    if (dfb->dev)
        device_destroy(dfb->class, MKDEV(0, 0));

    if (dfb->class)
        class_destroy(dfb->class);

    if (dfb->fbinfo) {
        fb_deferred_io_cleanup(dfb->fbinfo);
        framebuffer_release(dfb->fbinfo);
    }

    kfree(dfb);
}

module_init(dummy_fb_init);
module_exit(dummy_fb_exit);
MODULE_AUTHOR("Hua Zheng <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
