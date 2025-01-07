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
    struct fb_info *info;
    struct fb_ops *fbops;

    u32             pseudo_palette[16];
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

static ssize_t dummy_fb_read(struct fb_info *info, char __user *buf,
			   size_t count, loff_t *ppos)
{
    pr_info("%s\n", __func__);
    return fb_sys_read(info, buf, count, ppos);
}

static ssize_t dummy_fb_write(struct fb_info *info, const char __user *buf,
			    size_t count, loff_t *ppos)
{
    pr_info("%s: count=%zd, ppos=%llu\n", __func__,  count, *ppos);
    return fb_sys_write(info, buf, count, ppos);
}

static void dummy_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
    pr_info("%s\n", __func__);
    sys_fillrect(info, rect);
}

static void dummy_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
    pr_info("%s\n", __func__);
    sys_copyarea(info, area);
}

static void dummy_fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
    pr_info("%s\n", __func__);
    sys_imageblit(info, image);
}

/* from pxafb.c */
static unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

static int dummy_fb_setcolreg(unsigned int regno, unsigned int red,
                                unsigned int green, unsigned int blue,
                                unsigned int transp, struct fb_info *info)
{
    unsigned int val;
    int ret = 1;

    printk("%s(regno=%u, red=0x%X, green=0x%X, blue=0x%X, trans=0x%X)\n",
           __func__, regno, red, green, blue, transp);

    if (regno >= 256)   /* no. of hw registers */
        return 1;
    /*
    * Program hardware... do anything you want with transp
    */

    switch (info->fix.visual) {
    case FB_VISUAL_TRUECOLOR:
        if (regno < 16) {
            val  = chan_to_field(red, &info->var.red);
            val |= chan_to_field(green, &info->var.green);
            val |= chan_to_field(blue, &info->var.blue);

            ((u32 *)(info->pseudo_palette))[regno] = val;
            ret = 0;
        }
        break;
    }

    return ret;
}

static int dummy_fb_blank(int blank, struct fb_info *info)
{
    int ret = -EINVAL;

    switch (blank) {
    case FB_BLANK_POWERDOWN:
    case FB_BLANK_VSYNC_SUSPEND:
    case FB_BLANK_HSYNC_SUSPEND:
    case FB_BLANK_NORMAL:
        pr_info("%s, normal\n", __func__);
        break;
    case FB_BLANK_UNBLANK:
        pr_info("%s, unblank\n", __func__);
        break;
    }
    return ret;
}

static void dummy_fb_deferred_io(struct fb_info *info, struct list_head *pagelist)
{
    pr_info("%s\n", __func__);
}

static int dummy_fb_alloc(struct dummy_fb *dfb)
{
    struct fb_deferred_io *fbdefio;
    struct fb_ops *fbops;
    struct fb_info *info;
    int width, height, bpp, rotate;
    u8 *vmem = NULL;
    int vmem_size;
    int rc;

    pr_info("%s\n", __func__);

    width  = display.xres;
    height = display.yres;
    bpp    = display.bpp;
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

    info = framebuffer_alloc(0, dfb->dev);
    if (!info) {
        pr_err("failed to allocate info\n");
        goto err_free_fbdefio;
    }

    dfb->info = info;
    info->screen_buffer = vmem;
    info->fbops = fbops;
    info->fbdefio = fbdefio;

    fbops->owner        = THIS_MODULE;
    fbops->fb_read      = dummy_fb_read,
    fbops->fb_write     = dummy_fb_write;
    fbops->fb_fillrect  = dummy_fb_fillrect;
    fbops->fb_copyarea  = dummy_fb_copyarea;
    fbops->fb_imageblit = dummy_fb_imageblit;
    fbops->fb_setcolreg = dummy_fb_setcolreg;
    fbops->fb_blank     = dummy_fb_blank;

    snprintf(info->fix.id, sizeof(info->fix.id), "%s", DRV_NAME);
    info->fix.type        = FB_TYPE_PACKED_PIXELS;
    info->fix.visual      = FB_VISUAL_TRUECOLOR;
    info->fix.xpanstep    = 0;
    info->fix.ypanstep    = 0;
    info->fix.ywrapstep   = 0;
    info->fix.line_length = width * bpp / BITS_PER_BYTE;
    info->fix.accel       = FB_ACCEL_NONE;
    info->fix.smem_len    = vmem_size;

    info->var.rotate         = rotate;
    info->var.xres           = width;
    info->var.yres           = height;
    info->var.xres_virtual   = width;
    info->var.yres_virtual   = height;
    info->var.bits_per_pixel = bpp;
    info->var.nonstd         = 1;
    info->var.grayscale      = 0;

    info->var.red.offset    = 11;
    info->var.red.length    = 5;
    info->var.green.offset  = 5;
    info->var.green.length  = 6;
    info->var.blue.offset   = 0;
    info->var.blue.length   = 5;
    info->var.transp.offset = 0;
    info->var.transp.length = 0;

    info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;
    info->pseudo_palette = &dfb->pseudo_palette;

    fbdefio->delay = HZ;
    // fbdefio->sort_pagereflist = true;
    fbdefio->deferred_io = dummy_fb_deferred_io;
    fb_deferred_io_init(info);

    rc = register_framebuffer(info);
    if (rc < 0) {
        pr_err("framebuffer register failed with %d!", rc);
        return -1;
    }

    pr_info("%d KB video memory\n", info->fix.smem_len >> 10);

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

    dfb->class = class_create(THIS_MODULE, DRV_NAME "class");
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

    if (dfb->info) {
        fb_deferred_io_cleanup(dfb->info);
        unregister_framebuffer(dfb->info);
        framebuffer_release(dfb->info);
    }

    kfree(dfb);
}

module_init(dummy_fb_init);
module_exit(dummy_fb_exit);
MODULE_AUTHOR("Hua Zheng <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
