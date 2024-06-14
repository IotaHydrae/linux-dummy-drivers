/*
 * Linux dummy video driver
 *
 * Copyright (C) 2024 embeddedboys <writeforever@foxmail.com>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

struct dummy_video {
    struct video_device vdev;
    struct v4l2_device v4l2_dev;
    struct vb2_queue vb_queue;

    struct mutex vb_queue_lock;
    struct mutex v4l2_lock;
} *g_dummy_video;

/* Videobuf2 operations */
static int dummy_video_queue_setup(struct vb2_queue *vq,
		unsigned int *nbuffers,
		unsigned int *nplanes, unsigned int sizes[], struct device *alloc_devs[])
{
	return 0;
}

static void dummy_video_buf_queue(struct vb2_buffer *vb)
{

}

// /*
//     * Sanity check
//     */
// if (WARN_ON(!q)			  ||
//     WARN_ON(!q->ops)		  ||
//     WARN_ON(!q->mem_ops)	  ||
//     WARN_ON(!q->type)		  ||
//     WARN_ON(!q->io_modes)	  ||
//     WARN_ON(!q->ops->queue_setup) ||
//     WARN_ON(!q->ops->buf_queue))
//     return -EINVAL;
static const struct vb2_ops dummy_video_vb2_ops = {
	.queue_setup            = dummy_video_queue_setup,
	.buf_queue              = dummy_video_buf_queue,
	// .start_streaming        = airspy_start_streaming,
	// .stop_streaming         = airspy_stop_streaming,
	.wait_prepare           = vb2_ops_wait_prepare,
	.wait_finish            = vb2_ops_wait_finish,
};

static const struct v4l2_ioctl_ops dummy_video_ioctl_ops =  {
	// .vidioc_querycap          = airspy_querycap,

	// .vidioc_enum_fmt_sdr_cap  = airspy_enum_fmt_sdr_cap,
	// .vidioc_g_fmt_sdr_cap     = airspy_g_fmt_sdr_cap,
	// .vidioc_s_fmt_sdr_cap     = airspy_s_fmt_sdr_cap,
	// .vidioc_try_fmt_sdr_cap   = airspy_try_fmt_sdr_cap,

	.vidioc_reqbufs           = vb2_ioctl_reqbufs,
	.vidioc_create_bufs       = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf       = vb2_ioctl_prepare_buf,
	.vidioc_querybuf          = vb2_ioctl_querybuf,
	.vidioc_qbuf              = vb2_ioctl_qbuf,
	.vidioc_dqbuf             = vb2_ioctl_dqbuf,

	.vidioc_streamon          = vb2_ioctl_streamon,
	.vidioc_streamoff         = vb2_ioctl_streamoff,

	// .vidioc_g_tuner           = airspy_g_tuner,
	// .vidioc_s_tuner           = airspy_s_tuner,

	// .vidioc_g_frequency       = airspy_g_frequency,
	// .vidioc_s_frequency       = airspy_s_frequency,
	// .vidioc_enum_freq_bands   = airspy_enum_freq_bands,

	.vidioc_subscribe_event   = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_log_status        = v4l2_ctrl_log_status,
};

static const struct v4l2_file_operations dummy_video_fops = {
    .owner                    = THIS_MODULE,
    .open                     = v4l2_fh_open,
    .release                  = vb2_fop_release,
    .read                     = vb2_fop_read,
    .poll                     = vb2_fop_poll,
    .mmap                     = vb2_fop_mmap,
    .unlocked_ioctl           = video_ioctl2,
};

struct video_device dummy_video_template = {
    .name                     = "dummy-video",
    .release                  = video_device_release_empty,
    .fops                     = &dummy_video_fops,
    .ioctl_ops                = &dummy_video_ioctl_ops,

    /* the device_caps field MUST be set for all but subdevs */
    .device_caps              = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING,
};

static void dummy_video_release(struct v4l2_device *v)
{
	struct dummy_video *s = container_of(v, struct dummy_video, v4l2_dev);

	// v4l2_ctrl_handler_free(&s->hdl);
	v4l2_device_unregister(&s->v4l2_dev);
	kfree(s);
}

static __init int dummy_video_drv_init(void)
{
    int ret;

    printk("%s, Hello, world!\n", __func__);

    g_dummy_video = kzalloc(sizeof(*g_dummy_video), GFP_KERNEL);
    if (IS_ERR(g_dummy_video)) {
        printk("failed to alloc memory!\n");
        return -ENOMEM;
    }

    g_dummy_video->vdev = dummy_video_template;

    /* Setup vb queue */
    printk("%s, initializing vb2 queue...\n", __func__);
    g_dummy_video->vb_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    g_dummy_video->vb_queue.io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
    // g_dummy_video->vb_queue.drv_priv = NULL;
    // g_dummy_video->vb_queue.buf_struct_size = sizeof(struct vb2_buffer);
    g_dummy_video->vb_queue.ops = &dummy_video_vb2_ops;
    g_dummy_video->vb_queue.mem_ops = &vb2_vmalloc_memops;
    g_dummy_video->vb_queue.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    ret = vb2_queue_init(&g_dummy_video->vb_queue); // will initialize vb2_buf_ops
    if (ret) {
        printk("failed to init vb queue!\n");
        return ret;
    }
    g_dummy_video->vdev.queue = &g_dummy_video->vb_queue;
    g_dummy_video->vdev.queue->lock = &g_dummy_video->vb_queue_lock;

    /* Setup v4l2 device */
    printk("%s, registering v4l2 device...\n", __func__);
    /* If dev == NULL, then name must be filled in by the caller */
    snprintf(g_dummy_video->v4l2_dev.name, sizeof(g_dummy_video->v4l2_dev.name), "%s %s",
			KBUILD_MODNAME, KBUILD_BASENAME);
    g_dummy_video->v4l2_dev.release = dummy_video_release;
    ret = v4l2_device_register(NULL, &g_dummy_video->v4l2_dev);
    if (ret) {
        printk("failed to register v4l2 device!\n");
        goto err_free_vb_queue;
    }
    g_dummy_video->vdev.v4l2_dev = &g_dummy_video->v4l2_dev;
    g_dummy_video->vdev.lock = &g_dummy_video->v4l2_lock;

    printk("%s, registering video device...\n", __func__);
    ret = video_register_device(&g_dummy_video->vdev, VFL_TYPE_SDR, -1);
    if (ret) {
        printk("failed to register video device!\n");
        goto err_free_v4l2_dev;
    }

    return 0;

err_free_v4l2_dev:
    v4l2_device_unregister(&g_dummy_video->v4l2_dev);
err_free_vb_queue:
    vb2_queue_release(&g_dummy_video->vb_queue);
    return -1;
}

static __exit void dummy_video_drv_exit(void)
{
    printk("%s, Good bye, friend.\n", __func__);

    printk("%s, unregistering things...\n", __func__);
    v4l2_device_unregister(&g_dummy_video->v4l2_dev);
    vb2_video_unregister_device(&g_dummy_video->vdev);

    printk("%s, free dummy video instance...\n", __func__);
    if (g_dummy_video) {
        kfree(g_dummy_video);
        g_dummy_video = NULL;
    }
}

module_init(dummy_video_drv_init);
module_exit(dummy_video_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("embeddedboys <writeforever@foxmail.com>");
MODULE_DESCRIPTION("A dummy linux v4l2 driver.");
