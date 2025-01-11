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

#include <drm/drm_drv.h>
#include <drm/drm_device.h>
#include <drm/drm_managed.h>
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>

#define DRV_NAME "dummy-drm"

struct dummy_device {
    struct class *class;
    struct device *dev;
};

struct dummy_drm_dev {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct drm_connector connector;
    struct drm_display_mode mode;
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

static enum drm_mode_status dummy_drm_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
					      const struct drm_display_mode *mode)
{
    struct dummy_drm_dev *ddrm = drm_to_dummy_drm_dev(pipe->crtc.dev);
    pr_info("%s\n", __func__);
    return drm_crtc_helper_mode_valid_fixed(&pipe->crtc, mode, &ddrm->mode);
}

static void dummy_drm_pipe_enable(struct drm_simple_display_pipe *pipe,
				  struct drm_crtc_state *crtc_state,
				  struct drm_plane_state *plane_state)
{
    pr_info("%s\n", __func__);
}

static void dummy_drm_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    pr_info("%s\n", __func__);
}

static void dummy_drm_pipe_update(struct drm_simple_display_pipe *pipe,
                                struct drm_plane_state *old_state)
{
    pr_info("%s\n", __func__);
}

static int dummy_drm_pipe_begin_fb_access(struct drm_simple_display_pipe *pipe,
				  struct drm_plane_state *plane_state)
{
    return drm_gem_begin_shadow_fb_access(&pipe->plane, plane_state);
}

static void dummy_drm_pipe_end_fb_access(struct drm_simple_display_pipe *pipe,
				 struct drm_plane_state *plane_state)
{
	drm_gem_end_shadow_fb_access(&pipe->plane, plane_state);
}

static void dummy_drm_pipe_reset_plane(struct drm_simple_display_pipe *pipe)
{
	drm_gem_reset_shadow_plane(&pipe->plane);
}

static struct drm_plane_state *dummy_drm_pipe_duplicate_plane_state(struct drm_simple_display_pipe *pipe)
{
	return drm_gem_duplicate_shadow_plane_state(&pipe->plane);
}

static void dummy_drm_pipe_destroy_plane_state(struct drm_simple_display_pipe *pipe,
				       struct drm_plane_state *plane_state)
{
	drm_gem_destroy_shadow_plane_state(&pipe->plane, plane_state);
}

static const struct drm_simple_display_pipe_funcs dummy_display_pipe_funcs = {
    .mode_valid = dummy_drm_pipe_mode_valid,
    .enable = dummy_drm_pipe_enable,
    .disable = dummy_drm_pipe_disable,
    .update = dummy_drm_pipe_update,
    .begin_fb_access = dummy_drm_pipe_begin_fb_access,
    .end_fb_access = dummy_drm_pipe_end_fb_access,
    .reset_plane = dummy_drm_pipe_reset_plane,
    .duplicate_plane_state = dummy_drm_pipe_duplicate_plane_state,
    .destroy_plane_state = dummy_drm_pipe_destroy_plane_state,
};

static int dummy_connector_get_modes(struct drm_connector *connector)
{
	struct dummy_drm_dev *ddrm = drm_to_dummy_drm_dev(connector->dev);

	return drm_connector_helper_get_modes_fixed(connector, &ddrm->mode);
}

static const struct drm_connector_helper_funcs dummy_connector_hfuncs = {
    .get_modes = dummy_connector_get_modes,
};

static const struct drm_connector_funcs dummy_connector_funcs = {
    .reset = drm_atomic_helper_connector_reset,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_mode_config_funcs dummy_drm_mode_config_funcs = {
    .fb_create = drm_gem_fb_create_with_dirty,
    .atomic_check = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
};

static const uint32_t dummy_drm_formats[] = {
    DRM_FORMAT_RGB565,
    DRM_FORMAT_XRGB8888,
};

static const struct drm_display_mode dummy_disp_mode = {
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
    static const uint64_t modifiers[] = {
		DRM_FORMAT_MOD_LINEAR,
		DRM_FORMAT_MOD_INVALID
	};
    struct drm_device *drm = &ddrm->drm;
    int rc;

    pr_info("%s\n", __func__);

    rc = drm_mode_config_init(drm);
    if (rc) {
        pr_err("failed to init mode config\n");
        return rc;
    }

    drm_mode_copy(&ddrm->mode, mode);
    pr_info("mode: %ux%u\n", ddrm->mode.hdisplay, ddrm->mode.vdisplay);

    drm_connector_helper_add(&ddrm->connector, &dummy_connector_hfuncs);
    rc = drm_connector_init(drm, &ddrm->connector, &dummy_connector_funcs,
                            DRM_MODE_CONNECTOR_USB);
    if (rc) {
        pr_err("failed to init connector\n");
        return rc;
    }

    rc = drm_simple_display_pipe_init(drm, &ddrm->pipe, funcs, formats, formats_count, modifiers, &ddrm->connector);
    if (rc) {
        pr_err("failed to init pipe\n");
        return rc;
    }

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

    rc = dummy_drm_dev_init(ddrm, &dummy_display_pipe_funcs, &dummy_disp_mode);
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
