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
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_managed.h>

#define DRV_NAME "dummy-drm"

struct dummy_device {
    struct class *class;
    struct device *dev;
};

struct dummy_drm_dev {
    const struct drm_display_mode mode;
    struct drm_device drm;
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

static inline struct dummy_drm_dev *drm_to_dummy_drm_dev(struct drm_device *drm)
{
    return container_of(drm, struct dummy_drm_dev, drm);
}

enum drm_mode_status dummy_drm_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
					      const struct drm_display_mode *mode)
{
    // struct
    return 0;
}

static const struct drm_simple_display_pipe_funcs dummy_display_pipe_funcs = {
    .mode_valid = NULL,
    .enable = NULL,
    .disable = NULL,
    .update = NULL,
    .begin_fb_access = NULL,
    .end_fb_access = NULL,
    .reset_plane = NULL,
    .duplicate_plane_state = NULL,
    .destroy_plane_state = NULL,
};

static const struct drm_display_mode dummy_mode = {
    DRM_SIMPLE_MODE(480, 320, 85, 55),
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

static int dummy_drm_dev_init_with_formats(struct dummy_drm_dev *ddrm,
                const struct drm_simple_display_pipe_funcs *funcs,
                const uint32_t *formats, unsigned int formats_count,
                const struct drm_display_mode *mode, size_t buf_size)
{
    pr_info("%s\n", __func__);
    return 0;
}

static int dummy_drm_dev_init(struct dummy_drm_dev *ddrm,
                const struct drm_simple_display_pipe_funcs *funcs,
                const struct drm_display_mode *mode)
{
    ssize_t bufsize = mode->vdisplay * mode->hdisplay * sizeof(u16);

    ddrm->drm.mode_config.preferred_depth = 16;

    pr_info("%s\n", __func__);

    return dummy_drm_dev_init_with_formats(ddrm, funcs, NULL, 0, mode, bufsize);
}

static int __init dummy_drm_init(void)
{
    struct dummy_drm_dev *ddrm;
    struct drm_device *drm;
    int rc;

    pr_info("\n\n\n%s\n", __func__);

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

    ddrm = devm_drm_dev_alloc(ddev->dev, &dummy_drm_driver,
                    struct dummy_drm_dev, drm);
    if (IS_ERR(ddrm)) {
        pr_err("failed to allocate drm\n");
        goto err_free_dev;
    }
    drm = &ddrm->drm;
    dev_set_drvdata(ddev->dev, drm);

    rc = dummy_drm_dev_init(ddrm, &dummy_display_pipe_funcs, &dummy_mode);
    if (rc) {
        pr_err("failed to init drm dev\n");
        goto err_free_drm;
    }

    return 0;

err_free_drm:
    drm_dev_put(&ddrm->drm);
err_free_dev:
    device_destroy(ddev->class, MKDEV(0, 0));
err_free_class:
    class_destroy(ddev->class);
err_free_ddev:
    kfree(ddev);
    return -ENOMEM;
}

static void __exit dummy_drm_exit(void)
{
    struct drm_device *drm = dev_get_drvdata(ddev->dev);
    pr_info("%s\n", __func__);

    if (ddev->dev)
        device_destroy(ddev->class, MKDEV(0, 0));

    if (ddev->class)
        class_destroy(ddev->class);

    if (drm)
        drm_dev_put(drm);

    kfree(ddev);
}

module_init(dummy_drm_init);
module_exit(dummy_drm_exit);
MODULE_AUTHOR("Hua Zheng <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
