// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * Copyright (C) 2026 embeddedboys, Ltd.
 *
 * Author: Wooden Chair <hua.zheng@embeddedboys.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <video/mipi_display.h>

#include <drm/drm_device.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_damage_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_format_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_mipi_dbi.h>
#include <drm/drm_modes.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_rect.h>
#include <drm/clients/drm_client_setup.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fbdev_dma.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_managed.h>
#include <video/mipi_display.h>

#define DRV_NAME "dummy-drm"

struct dummy_drm_dev {
	struct drm_device drm;
	struct drm_simple_display_pipe pipe;
	struct drm_connector connector;
	struct drm_display_mode mode;
};

struct dummy_drm {
	u64 dma_mask;
	dev_t dev_num;
	struct class *class;
	struct device *dev;

	struct dummy_drm_dev *ddev;
};

static struct dummy_drm dummy_drm;

static inline struct dummy_drm_dev *drm_to_dummy_drm_dev(struct drm_device *drm)
{
	return container_of(drm, struct dummy_drm_dev, drm);
}

static enum drm_mode_status
dummy_drm_pipe_mode_valid(struct drm_simple_display_pipe *pipe,
			  const struct drm_display_mode *mode)
{
	struct dummy_drm_dev *ddrm = drm_to_dummy_drm_dev(pipe->crtc.dev);
	int rc;
	rc = drm_crtc_helper_mode_valid_fixed(&pipe->crtc, mode, &ddrm->mode);
	pr_info("%s, rc: %d\n", __func__, rc);
	return rc;
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
	struct drm_plane_state *state = pipe->plane.state;
	// struct drm_shadow_plane_state *shadow_plane_state = to_drm_shadow_plane_state(state);
	struct drm_framebuffer *fb = state->fb;
	struct drm_rect rect;
	int idx;

	if (!pipe->crtc.state->active)
		return;

	if (WARN_ON(!fb))
		return;

	if (!drm_dev_enter(fb->dev, &idx))
		return;

	pr_info("%s\n", __func__);
	if (drm_atomic_helper_damage_merged(old_state, state, &rect))
		pr_info("x1: %u, y1: %u, x2: %u, y2: %u\n", rect.x1, rect.y1,
			rect.x2, rect.y2);

	drm_dev_exit(idx);
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

static struct drm_plane_state *
dummy_drm_pipe_duplicate_plane_state(struct drm_simple_display_pipe *pipe)
{
	return drm_gem_duplicate_shadow_plane_state(&pipe->plane);
}

static void
dummy_drm_pipe_destroy_plane_state(struct drm_simple_display_pipe *pipe,
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
	DRM_MODE_INIT(60, 480, 320, 85, 55),
};

DEFINE_DRM_GEM_DMA_FOPS(dummy_drm_fops);

static const struct drm_driver dummy_drm_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops = &dummy_drm_fops,
	DRM_GEM_DMA_DRIVER_OPS_VMAP,
	DRM_FBDEV_DMA_DRIVER_OPS,
	.name = "dummy-drm",
	.desc = "Dummy DRM driver",
	.major = 1,
	.minor = 0,
};

static int dummy_drm_dev_init_with_formats(
	struct dummy_drm_dev *ddev,
	const struct drm_simple_display_pipe_funcs *funcs,
	const uint32_t *formats, unsigned int formats_count,
	const struct drm_display_mode *mode, size_t buf_size)
{
	static const uint64_t modifiers[] = { DRM_FORMAT_MOD_LINEAR,
					      DRM_FORMAT_MOD_INVALID };
	struct drm_device *drm = &ddev->drm;
	int rc;

	pr_info("%s\n", __func__);

	rc = drm_mode_config_init(drm);
	if (rc) {
		pr_err("failed to init mode config\n");
		return rc;
	}

	drm_mode_copy(&ddev->mode, mode);
	pr_info("mode: %ux%u\n", ddev->mode.hdisplay, ddev->mode.vdisplay);

	drm_connector_helper_add(&ddev->connector, &dummy_connector_hfuncs);
	rc = drm_connector_init(drm, &ddev->connector, &dummy_connector_funcs,
				DRM_MODE_CONNECTOR_VIRTUAL);
	if (rc) {
		pr_err("failed to init connector\n");
		return rc;
	}

	rc = drm_simple_display_pipe_init(drm, &ddev->pipe, funcs, formats,
					  formats_count, modifiers,
					  &ddev->connector);
	if (rc) {
		pr_err("failed to init pipe\n");
		return rc;
	}

	drm_plane_enable_fb_damage_clips(&ddev->pipe.plane);

	drm->mode_config.funcs = &dummy_drm_mode_config_funcs;
	drm->mode_config.min_width = ddev->mode.hdisplay;
	drm->mode_config.max_width = ddev->mode.hdisplay;
	drm->mode_config.min_height = ddev->mode.vdisplay;
	drm->mode_config.max_height = ddev->mode.vdisplay;

	DRM_DEBUG_KMS("mode: %ux%u", ddev->mode.hdisplay, ddev->mode.vdisplay);

	return 0;
}

static int dummy_drm_dev_init(struct dummy_drm_dev *ddev,
			      const struct drm_simple_display_pipe_funcs *funcs,
			      const struct drm_display_mode *mode)
{
	ssize_t bufsize = mode->vdisplay * mode->hdisplay * sizeof(u16);

	ddev->drm.mode_config.preferred_depth = 16;

	pr_info("%s\n", __func__);

	return dummy_drm_dev_init_with_formats(ddev, funcs, dummy_drm_formats,
					       ARRAY_SIZE(dummy_drm_formats),
					       mode, bufsize);
}

static int __init dummy_drm_register(struct dummy_drm *dummy_drm)
{
        struct dummy_drm_dev *ddev;
	struct drm_device *drm;
	struct device *dev;
	int ret;

	printk("%s\n", __func__);

	ret = alloc_chrdev_region(&dummy_drm->dev_num, 0, 1, "dummy-template");

	dummy_drm->class = class_create("dummy-template-class");
	if (IS_ERR(dummy_drm->class)) {
		pr_err("%s, failed to create class\n", __func__);
		goto out_dev_num;
	}

	dummy_drm->dev = device_create(dummy_drm->class, NULL,
				       dummy_drm->dev_num, NULL,
				       "dummy-template-dev");
	if (IS_ERR(dummy_drm->dev)) {
		pr_err("%s, failed to create device\n", __func__);
		goto out_class;
	}
	dev = dummy_drm->dev;

	dummy_drm->dma_mask = DMA_BIT_MASK(32);
	dummy_drm->dev->dma_mask = &dummy_drm->dma_mask;
	dummy_drm->dev->coherent_dma_mask = dummy_drm->dma_mask;

	dummy_drm->ddev = devm_drm_dev_alloc(dev, &dummy_drm_driver, struct dummy_drm_dev, drm);
	if (IS_ERR(dummy_drm->ddev))
	        goto out_dev;

	ddev = dummy_drm->ddev;
	drm = &ddev->drm;

	ret = dummy_drm_dev_init(dummy_drm->ddev, &dummy_display_pipe_funcs,
				&dummy_disp_mode);
	if (ret) {
		pr_err("failed to init drm dev\n");
		goto out_drm;
	}

	drm_mode_config_reset(drm);

	ret = drm_dev_register(drm, 0);
	if (ret) {
		pr_err("failed to register drm dev\n");
		goto out_drm;
	};

	dev_set_drvdata(dummy_drm->dev, &ddev->drm);

	drm_client_setup(drm, NULL);

	return 0;

out_drm:
	drm_dev_put(&ddev->drm);
out_dev:
	device_destroy(dummy_drm->class, dummy_drm->dev_num);
out_class:
	class_destroy(dummy_drm->class);
out_dev_num:
	unregister_chrdev_region(dummy_drm->dev_num, 1);
	return -ENODEV;
}

static void __exit dummy_drm_unregister(struct dummy_drm *dummy_drm)
{
	struct drm_device *drm = dev_get_drvdata(dummy_drm->dev);
	pr_info("%s\n", __func__);

	/*
	 * Unpublish the DRM device first (this also tears down the fbdev
	 * client), then shut down the display pipeline. The parent device
	 * must be destroyed last: devm_drm_dev_alloc() ties the DRM device
	 * lifetime to dummy_drm->dev, so device_destroy() would free it
	 * underneath drm_dev_unplug()/drm_atomic_helper_shutdown().
	 */
	drm_dev_unplug(drm);
	drm_atomic_helper_shutdown(drm);

	device_destroy(dummy_drm->class, dummy_drm->dev_num);
	class_destroy(dummy_drm->class);
	unregister_chrdev_region(dummy_drm->dev_num, 1);
}

module_driver(dummy_drm, dummy_drm_register, dummy_drm_unregister);

MODULE_DESCRIPTION("Dummy DRM simple display pipe driver");
MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_LICENSE("GPL");
