#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define DEVICE "/dev/video0"
#define BUFFER_COUNT 32

struct video_buffer {
    unsigned int length;
    unsigned int offset;
    unsigned char *start;
};

int main()
{
    int fd;
    int                        m_rb_count;
    int                        m_rb_current;
    int                        m_total_bytes;
    unsigned int video_buff_size;
    enum v4l2_buf_type type;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_fmtdesc fmt_desc;
    struct v4l2_frmsizeenum frmsize;
    struct video_buffer *buffers;

    // Open the device
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Query the device capabilities
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("VIDIOC_QUERYCAP");
        return 1;
    }

    memset(&fmt_desc, 0x0, sizeof(fmt_desc));
    fmt_desc.index = 0;
    fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Format supportted:\n");

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt_desc) != -1) {
        printf("\t%d. Type: %s\n", fmt_desc.index, fmt_desc.description);

        frmsize.index = 0;
        frmsize.pixel_format = fmt_desc.pixelformat;
        printf("\tResolutions:\n");
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0) {
            printf("\t\t%d x %d\n", frmsize.discrete.width,
                   frmsize.discrete.height);
            frmsize.index++;
        }
        fmt_desc.index++;
    }

    // Set the format
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 800;
    fmt.fmt.pix.height = 600;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("ioctl VIDIOC_S_FMT failed");
        return -1;
    }

    // Request buffers
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("ioctl VIDIOC_REQBUFS failed");
        return -1;
    }

    buffers = (struct video_buffer *)calloc(req.count, sizeof(struct video_buffer));

    for (int i = 0; i < req.count; i++) {
        memset(&buf, 0x0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("ioctl VIDIOC_QUERYBUF failed!");
            return -1;
        }

        video_buff_size = buf.length;
        buffers[i].start = (unsigned char *)mmap(NULL, video_buff_size, PROT_READ | PROT_WRITE,
                                                 MAP_SHARED, fd, buf.m.offset);

        if (buffers[i].start == MAP_FAILED) {
            perror("mmap failed!");
            return -1;
        }

        memset(&buf, 0x0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            return -1;
        }
    }

    printf("v4l2_buf size : %d\n", buf.length);

    /* start streaming */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("ioctl VIDIOC_STREAMON failed!");
        return -1;
    }

    unsigned frame_count = 120;
    /* Get frames */
    while (frame_count--) {
        struct v4l2_buffer v4lbuffer;

        memset(&v4lbuffer, 0x0, sizeof(v4lbuffer));
        v4lbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4lbuffer.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &v4lbuffer) < 0) {
            perror("ioctl VIDIOC_DQBUF failed!");
            return -1;
        }

        m_rb_current = v4lbuffer.index;
        m_total_bytes = v4lbuffer.bytesused;

        int out_fd;
        unsigned char *file_base;
        char out_fname[50];
        sprintf(out_fname, "./abc_%04d.jpeg", frame_count);
        printf("writing %s by mmap...\n", out_fname);

        out_fd = open(out_fname, O_RDWR | O_CREAT | O_TRUNC, 0755);
        if (out_fd < 0) {
            perror("open path failed!");
            return out_fd;
        }

        lseek(out_fd, m_total_bytes - 1, SEEK_END);
        write(out_fd, "", 1);   /* because of COW? */
        printf("total_bytes:%d\n", m_total_bytes);
        printf("current:%d\n", m_rb_current);

        /* write operation here */
        file_base = (unsigned char *)mmap(NULL, m_total_bytes,
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED,
                                          out_fd, 0);

        if (file_base == MAP_FAILED) {
            perror("mmap file failed!");
            return -1;
        }

        memcpy(file_base, buffers[m_rb_current].start, m_total_bytes);
        munmap(file_base, m_total_bytes);
        close(out_fd);

        /* push back fb to queue */
        memset(&v4lbuffer, 0x0, sizeof(v4lbuffer));
        v4lbuffer.index = m_rb_current;
        v4lbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4lbuffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_QBUF, &v4lbuffer) < 0) {
            perror("ioctl VIDIOC_QBUF failed!");
            return -1;
        }

    }

    /* stop streaming */
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        perror("ioctl VIDIOC_STREAMOFF failed!");
        return -1;
    }

    close(fd);
    return 0;
}