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
#include <linux/timer.h>
#include <linux/version.h>
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

    spinlock_t queued_bufs_lock;
    struct mutex vb_queue_lock;
    struct mutex v4l2_lock;

    struct list_head queued_bufs;

    struct timer_list timer;
    int frame_count;
} *g_dummy_video;

struct dummy_video_frame_buf {
    struct vb2_v4l2_buffer vb;
    struct list_head list;
};

extern unsigned char red[8229];
extern unsigned char blue[8229];
extern unsigned char green[8228];

static struct dummy_video_frame_buf *dummy_video_get_next_buf(void)
{
	unsigned long flags;
	struct dummy_video_frame_buf *buf = NULL;

	spin_lock_irqsave(&g_dummy_video->queued_bufs_lock, flags);
	if (list_empty(&g_dummy_video->queued_bufs))
		goto leave;

	buf = list_entry(g_dummy_video->queued_bufs.next,
			struct dummy_video_frame_buf, list);
	list_del(&buf->list);
leave:
	spin_unlock_irqrestore(&g_dummy_video->queued_bufs_lock, flags);
	return buf;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 9, 191)
static void dummy_video_timer_func(struct timer_list *tl)
#else
static void dummy_video_timer_func(unsigned long data)
#endif
{
    void *ptr;
    /* get first free buffer */
    struct dummy_video_frame_buf *buf = dummy_video_get_next_buf();

    /* write into buffer */
    if (buf) {
        ptr = vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
        if (g_dummy_video->frame_count <= 60) {
            memcpy(ptr, red, sizeof(red));
            vb2_set_plane_payload(&buf->vb.vb2_buf, 0, sizeof(red));
        } else if (g_dummy_video->frame_count > 60 && g_dummy_video->frame_count <= 120) {
            memcpy(ptr, blue, sizeof(blue));
            vb2_set_plane_payload(&buf->vb.vb2_buf, 0, sizeof(blue));
        } else {
            memcpy(ptr, green, sizeof(green));
            vb2_set_plane_payload(&buf->vb.vb2_buf, 0, sizeof(green));
        }
    }

    g_dummy_video->frame_count++;
    if (g_dummy_video->frame_count > 180)
        g_dummy_video->frame_count = 0;

    /* call vb2_buffer_done */
    vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);

    /* reset timer expire time */
    mod_timer(&g_dummy_video->timer, jiffies + HZ / 30);
}

/* Videobuf2 operations */
static int dummy_video_queue_setup(struct vb2_queue *vq,
		unsigned int *nbuffers,
		unsigned int *nplanes, unsigned int sizes[], struct device *alloc_devs[])
{
	/* Need at least 8 buffers */
	if (vq->num_buffers + *nbuffers < 8)
		*nbuffers = 8 - vq->num_buffers;
	*nplanes = 1;
	sizes[0] = PAGE_ALIGN(800*600*2);

	return 0;
}

static void dummy_video_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct dummy_video_frame_buf *buf =
			container_of(vbuf, struct dummy_video_frame_buf, vb);
	unsigned long flags;

	spin_lock_irqsave(&g_dummy_video->queued_bufs_lock, flags);
	list_add_tail(&buf->list, &g_dummy_video->queued_bufs);
	spin_unlock_irqrestore(&g_dummy_video->queued_bufs_lock, flags);
}

static int dummy_video_start_streaming(struct vb2_queue *vq, unsigned int count)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 9, 191)
    timer_setup(&g_dummy_video->timer, dummy_video_timer_func, 0);
    g_dummy_video->timer.expires = jiffies + HZ / 30;
    add_timer(&g_dummy_video->timer);
#else
    setup_timer(&g_dummy_video->timer, dummy_video_timer_func, 0);
#endif
    return 0;
}

static void dummy_video_stop_streaming(struct vb2_queue *vq)
{
    del_timer(&g_dummy_video->timer);
}

static int dummy_video_querycap(struct file *file, void *fh,
		struct v4l2_capability *cap)
{
	strlcpy(cap->driver, KBUILD_MODNAME, sizeof(cap->driver));
	strlcpy(cap->card, g_dummy_video->vdev.name, sizeof(cap->card));
	cap->device_caps = g_dummy_video->vdev.device_caps;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int dummy_video_enum_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_fmtdesc *f)
{
	if (f->index > 0)
		return -EINVAL;

	strlcpy(f->description, "motion jpeg", sizeof(f->description));
	f->pixelformat = V4L2_PIX_FMT_MJPEG;

	return 0;
}

static int dummy_video_s_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
    printk("%s\n", __func__);
    if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        printk("%s, unsupported buffer type!\n", __func__);
        return -EINVAL;
    }

    if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG) {
        printk("%s, unsupported pixel format!\n", __func__);
        return -EINVAL;
    }

    f->fmt.pix.width = 800;
    f->fmt.pix.height = 600;
    f->fmt.pix.field = V4L2_FIELD_ANY;

	return 0;
}

static int dummy_video_enum_framesizes(struct file *file, void *fh,
                    struct v4l2_frmsizeenum *fsize)
{
    printk("%s\n", __func__);

	if (fsize->index > 0)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = 800;
	fsize->discrete.height = 600;
	return 0;
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
	.start_streaming        = dummy_video_start_streaming,
	.stop_streaming         = dummy_video_stop_streaming,
	.wait_prepare           = vb2_ops_wait_prepare,
	.wait_finish            = vb2_ops_wait_finish,
};

static const struct v4l2_ioctl_ops dummy_video_ioctl_ops =  {
	.vidioc_querycap          = dummy_video_querycap,

	.vidioc_enum_fmt_vid_cap  = dummy_video_enum_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = dummy_video_s_fmt_vid_cap,
    .vidioc_enum_framesizes   = dummy_video_enum_framesizes,

	.vidioc_reqbufs           = vb2_ioctl_reqbufs,
	.vidioc_create_bufs       = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf       = vb2_ioctl_prepare_buf,
	.vidioc_querybuf          = vb2_ioctl_querybuf,
	.vidioc_qbuf              = vb2_ioctl_qbuf,
	.vidioc_dqbuf             = vb2_ioctl_dqbuf,

	.vidioc_streamon          = vb2_ioctl_streamon,
	.vidioc_streamoff         = vb2_ioctl_streamoff,

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
    .device_caps              = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE,
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

    mutex_init(&g_dummy_video->vb_queue_lock);
    mutex_init(&g_dummy_video->v4l2_lock);
    spin_lock_init(&g_dummy_video->queued_bufs_lock);
    INIT_LIST_HEAD(&g_dummy_video->queued_bufs);

    g_dummy_video->vdev = dummy_video_template;
    printk("%s, video device name : %s\n", __func__, g_dummy_video->vdev.name);

    /* Setup vb queue */
    printk("%s, initializing vb2 queue...\n", __func__);
    g_dummy_video->vb_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    g_dummy_video->vb_queue.io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
    g_dummy_video->vb_queue.drv_priv = NULL;
    g_dummy_video->vb_queue.buf_struct_size = sizeof(struct dummy_video_frame_buf);
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
    printk("%s, v4l2 device name : %s\n", __func__, g_dummy_video->v4l2_dev.name);
    g_dummy_video->v4l2_dev.release = dummy_video_release;
    ret = v4l2_device_register(NULL, &g_dummy_video->v4l2_dev);
    if (ret) {
        printk("failed to register v4l2 device!\n");
        goto err_free_vb_queue;
    }
    g_dummy_video->vdev.v4l2_dev = &g_dummy_video->v4l2_dev;
    g_dummy_video->vdev.lock = &g_dummy_video->v4l2_lock;

    printk("%s, registering video device...\n", __func__);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 9, 191)
    ret = video_register_device(&g_dummy_video->vdev, VFL_TYPE_VIDEO, -1);
#else
    ret = video_register_device(&g_dummy_video->vdev, VFL_TYPE_GRABBER, -1);
#endif
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
    video_unregister_device(&g_dummy_video->vdev);

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
