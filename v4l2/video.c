/*
 * Linux dummy video driver
 *
 * Copyright (C) 2026 Wooden Chair <hua.zheng@embeddedboys.com>
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

#include "frames.h"

#define DRV_NAME "dummy-video"

static int debug;
module_param(debug, int, 0644);

#define dprintk(level, fmt, arg...)                                    \
	do {                                                           \
		if (debug >= level)                                    \
			pr_info(DRV_NAME ": [%d], %s: " fmt, __LINE__, \
				__func__, ##arg);                      \
	} while (0)

#define debug(fmt, arg...) dprintk(1, fmt, ##arg)

#define DUMMY_VIDEO_WIDTH 600
#define DUMMY_VIDEO_HEIGHT 350
#define DUMMY_VIDEO_PLANE_SIZE (DUMMY_VIDEO_WIDTH * DUMMY_VIDEO_HEIGHT * 2)

struct dummy_video {
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	struct vb2_queue vb_queue;

	spinlock_t queued_bufs_slock;
	struct mutex vb_queue_mlock;
	struct mutex v4l2_mlock;

	struct list_head queued_bufs;

	struct timer_list timer;
	int sequence;
};

static struct dummy_video dummy_video;

struct dummy_video_frame_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

struct dummy_frame {
	const unsigned char *data;
	uint32_t len;
};
#define DUMMY_FRAME(num)                                                    \
	{                                                                   \
		.data = frame_##num##_jpg, .len = sizeof(frame_##num##_jpg) \
	}

static struct dummy_frame frames[] = {
	DUMMY_FRAME(1),	 DUMMY_FRAME(2),  DUMMY_FRAME(3),  DUMMY_FRAME(4),
	DUMMY_FRAME(5),	 DUMMY_FRAME(6),  DUMMY_FRAME(7),  DUMMY_FRAME(8),
	DUMMY_FRAME(9),	 DUMMY_FRAME(10), DUMMY_FRAME(11), DUMMY_FRAME(12),
	DUMMY_FRAME(13), DUMMY_FRAME(14), DUMMY_FRAME(15), DUMMY_FRAME(16),
	DUMMY_FRAME(17), DUMMY_FRAME(18), DUMMY_FRAME(19), DUMMY_FRAME(20),
	DUMMY_FRAME(21), DUMMY_FRAME(22), DUMMY_FRAME(23), DUMMY_FRAME(24),
	DUMMY_FRAME(25),
};
#define FRAME_COUNT ARRAY_SIZE(frames)

static void dummy_video_timer_func(struct timer_list *tl)
{
	struct dummy_video *dv = from_timer(dv, tl, timer);
	struct dummy_video_frame_buf *buf;
	struct dummy_frame *df;
	unsigned long flags;
	void *vaddr;

	spin_lock_irqsave(&dv->queued_bufs_slock, flags);
	if (list_empty(&dv->queued_bufs)) {
		spin_unlock_irqrestore(&dv->queued_bufs_slock, flags);
		goto resched;
	}

	buf = list_first_entry(&dv->queued_bufs, struct dummy_video_frame_buf,
			       list);
	list_del(&buf->list);
	spin_unlock_irqrestore(&dv->queued_bufs_slock, flags);

	debug("seq: %d\n", dv->sequence);

	vaddr = vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
	if (vaddr) {
		df = &frames[dv->sequence % FRAME_COUNT];
		memcpy(vaddr, df->data, df->len);
		vb2_set_plane_payload(&buf->vb.vb2_buf, 0, df->len);
	}

	buf->vb.sequence = dv->sequence++;
	buf->vb.vb2_buf.timestamp = ktime_get_ns();
	vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);

resched:
	mod_timer(&dv->timer, jiffies + msecs_to_jiffies(40));
}

/* Videobuf2 operations */
static int dummy_video_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				   unsigned int *nplanes, unsigned int sizes[],
				   struct device *alloc_devs[])
{
	debug("\n");

	if (*nplanes)
		return sizes[0] < (DUMMY_VIDEO_PLANE_SIZE) ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = DUMMY_VIDEO_PLANE_SIZE;

	return 0;
}

static int dummy_video_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct dummy_frame *df;

	if (vb2_plane_size(vb, 0) < (DUMMY_VIDEO_PLANE_SIZE))
		return -EINVAL;

	debug("\n");

	df = &frames[vbuf->sequence % FRAME_COUNT];
	// vb2_set_plane_payload(vb, 0, df->len);
	return 0;
}

static void dummy_video_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct dummy_video *dv = vb2_get_drv_priv(vb->vb2_queue);
	struct dummy_video_frame_buf *buf =
		container_of(vbuf, struct dummy_video_frame_buf, vb);
	unsigned long flags;

	debug("\n");

	spin_lock_irqsave(&dv->queued_bufs_slock, flags);
	list_add_tail(&buf->list, &dv->queued_bufs);
	spin_unlock_irqrestore(&dv->queued_bufs_slock, flags);
}

static int dummy_video_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct dummy_video *dv = vb2_get_drv_priv(vq);

	debug("\n");

	dv->sequence = 0;
	// dv->timer.expires = jiffies + HZ / 30;
	mod_timer(&dv->timer, jiffies + msecs_to_jiffies(40));

	// add_timer(&dv->timer);
	return 0;
}

static void dummy_video_stop_streaming(struct vb2_queue *vq)
{
	struct dummy_video *dv = vb2_get_drv_priv(vq);
	struct dummy_video_frame_buf *buf;
	unsigned long flags;

	debug("\n");

	del_timer_sync(&dv->timer);

	spin_lock_irqsave(&dv->queued_bufs_slock, flags);
	while (!list_empty(&dv->queued_bufs)) {
		buf = list_first_entry(&dv->queued_bufs,
				       struct dummy_video_frame_buf, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&dv->queued_bufs_slock, flags);
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
	.queue_setup = dummy_video_queue_setup,
	.buf_prepare = dummy_video_buf_prepare,
	.buf_queue = dummy_video_buf_queue,
	.start_streaming = dummy_video_start_streaming,
	.stop_streaming = dummy_video_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int dummy_video_querycap(struct file *file, void *fh,
				struct v4l2_capability *cap)
{
	struct dummy_video *dv = video_drvdata(file);

	debug("\n");

	strscpy(cap->driver, KBUILD_MODNAME, sizeof(cap->driver));
	strscpy(cap->card, dv->vdev.name, sizeof(cap->card));

	cap->device_caps = dv->vdev.device_caps;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int dummy_video_enum_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_fmtdesc *f)
{
	debug("\n");

	if (f->index > 0)
		return -EINVAL;

	strscpy(f->description, "motion jpeg", sizeof(f->description));
	f->pixelformat = V4L2_PIX_FMT_MJPEG;

	return 0;
}

static int dummy_video_enum_framesizes(struct file *file, void *fh,
				       struct v4l2_frmsizeenum *fsize)
{
	debug("\n");

	if (fsize->index > 0)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = DUMMY_VIDEO_WIDTH;
	fsize->discrete.height = DUMMY_VIDEO_HEIGHT;
	return 0;
}

static int dummy_video_enum_frameintervals(struct file *file, void *fh,
					   struct v4l2_frmivalenum *fival)
{
	debug("\n");

	if (fival->index > 0)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1;
	fival->discrete.denominator = 25;

	return 0;
}

static int dummy_video_g_fmt_vid_cap(struct file *file, void *fh,
				     struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;

	debug("\n");

	pix->width = DUMMY_VIDEO_WIDTH;
	pix->height = DUMMY_VIDEO_HEIGHT;
	pix->pixelformat = V4L2_PIX_FMT_MJPEG;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = DUMMY_VIDEO_WIDTH * 2;
	pix->sizeimage = DUMMY_VIDEO_WIDTH * DUMMY_VIDEO_HEIGHT * 2;
	pix->colorspace = V4L2_COLORSPACE_SRGB;

	return 0;
}

static int dummy_video_try_fmt_vid_cap(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	debug("\n");

	return dummy_video_g_fmt_vid_cap(file, priv, f);
}

static int dummy_video_s_fmt_vid_cap(struct file *file, void *priv,
				     struct v4l2_format *f)
{
	debug("\n");

	return dummy_video_g_fmt_vid_cap(file, priv, f);
}

static int dummy_video_g_parm(struct file *file, void *fh,
			      struct v4l2_streamparm *a)
{
	debug("\n");
	return 0;
}

static int dummy_video_s_parm(struct file *file, void *fh,
			      struct v4l2_streamparm *a)
{
	debug("\n");
	return 0;
}

static const struct v4l2_ioctl_ops dummy_video_ioctl_ops = {
	.vidioc_querycap = dummy_video_querycap,

	.vidioc_enum_fmt_vid_cap = dummy_video_enum_fmt_vid_cap,
	.vidioc_enum_framesizes = dummy_video_enum_framesizes,
	.vidioc_enum_frameintervals = dummy_video_enum_frameintervals,

	.vidioc_g_fmt_vid_cap = dummy_video_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap = dummy_video_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap = dummy_video_s_fmt_vid_cap,

	.vidioc_g_parm = dummy_video_g_parm,
	.vidioc_s_parm = dummy_video_s_parm,

	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,

	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,

	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_log_status = v4l2_ctrl_log_status,
};

static const struct v4l2_file_operations dummy_video_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.read = vb2_fop_read,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

struct video_device dummy_video_template = {
	.name = "dummy-video",
	.release = video_device_release_empty,
	.fops = &dummy_video_fops,
	.ioctl_ops = &dummy_video_ioctl_ops,

	/* the device_caps field MUST be set for all but subdevs */
	.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
		       V4L2_CAP_READWRITE,
};

static void dummy_video_release(struct v4l2_device *v)
{
	struct dummy_video *s = container_of(v, struct dummy_video, v4l2_dev);

	// v4l2_ctrl_handler_free(&s->hdl);
	v4l2_device_unregister(&s->v4l2_dev);
	kfree(s);
}

static int __init dummy_video_drv_register(struct dummy_video *dv)
{
	int ret;

	debug("init\n");

	/* TODO: Alloc memory for your hardware structure */

	mutex_init(&dv->v4l2_mlock);
	mutex_init(&dv->vb_queue_mlock);
	spin_lock_init(&dv->queued_bufs_slock);
	INIT_LIST_HEAD(&dv->queued_bufs);
	timer_setup(&dv->timer, dummy_video_timer_func, 0);

	/* Init videobuf2 queue structure */
	dv->vb_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dv->vb_queue.io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
	dv->vb_queue.drv_priv = dv;
	dv->vb_queue.buf_struct_size = sizeof(struct dummy_video_frame_buf);
	dv->vb_queue.ops = &dummy_video_vb2_ops;
	dv->vb_queue.mem_ops = &vb2_vmalloc_memops;
	dv->vb_queue.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	ret = vb2_queue_init(&dv->vb_queue);
	if (ret) {
		pr_err("Could not initialize vb2 queue\n");
	}

	/* Init video_device structure */
	dv->vdev = dummy_video_template;
	dv->vdev.queue = &dv->vb_queue;
	dv->vdev.queue->lock = &dv->vb_queue_mlock;
	video_set_drvdata(&dv->vdev, dv);

	/* Register the v4l2_device structure */
	dv->v4l2_dev.release = dummy_video_release;
	snprintf(dv->v4l2_dev.name, sizeof(dv->v4l2_dev.name), "%s",
		 KBUILD_MODNAME);
	debug("%s\n", dv->v4l2_dev.name);
	ret = v4l2_device_register(NULL, &dv->v4l2_dev);
	if (ret) {
		pr_err("Failed to register v4l2-device (%d)\n", ret);
	}

	/* TODO: Register controls */

	dv->vdev.v4l2_dev = &dv->v4l2_dev;
	dv->vdev.lock = &dv->v4l2_mlock;

	ret = video_register_device(&dv->vdev, VFL_TYPE_VIDEO, -1);
	if (ret) {
		pr_err("Failed to register as video device (%d)\n", ret);
	}

	pr_info("Registered as %s\n", video_device_node_name(&dv->vdev));

	return 0;
}

static void __exit dummy_video_drv_unregister(struct dummy_video *dv)
{
	debug("\n");

	del_timer_sync(&dv->timer);

	mutex_lock(&dv->vb_queue_mlock);
	mutex_lock(&dv->v4l2_mlock);
	v4l2_device_unregister(&dv->v4l2_dev);
	video_unregister_device(&dv->vdev);
	mutex_unlock(&dv->v4l2_mlock);
	mutex_unlock(&dv->vb_queue_mlock);

	v4l2_device_put(&dv->v4l2_dev);
}

module_driver(dummy_video, dummy_video_drv_register,
	      dummy_video_drv_unregister);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wooden Chair <hua.zheng@embeddedboys.com>");
MODULE_DESCRIPTION("Dummy linux v4l2 video driver.");
