/**
 * @file main.c
 * @author Arash Golgol (arash.golgol@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-03-22
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note This code is combination of following examples
 *       capture_raw_frames.c (https://gist.github.com/maxlapshin/1253534)
 *       gst-appsrc.c (https://gist.github.com/floe/e35100f091315b86a5bf)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

struct cam_pipeline {
    GstElement* gst_pipeline;
    GstElement* appsrc;
    GstElement* queue;
    GstElement* rawvideo;
    GstElement* scale;
    GstElement* scale_capsfilter;
    GstElement* conv;
    GstElement* videosink;
};

struct buffer {
    void   *start;
    size_t  length;
};

/* global variables */
struct buffer*          buffers  = NULL;
static unsigned int     n_buffers = 0;
int                     want = 1;

void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int open_device(const char* dev) {
    int fd = -1;
    struct stat st;

    if (-1 == stat(dev, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev);
        exit(EXIT_FAILURE);
    }

    fd = open(dev, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

void close_device(int fd) {
    if (-1 == close(fd))
        errno_exit("close");
}

void init_mmap(int fd) {
    struct v4l2_requestbuffers req = {0};

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr,"device does not support memory mapping\n");
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory\n");
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf = {0};

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
        mmap(NULL /* start anywhere */,
              buf.length,
              PROT_READ | PROT_WRITE /* required */,
              MAP_SHARED /* recommended */,
              fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

int init_device(int fd) {
    struct v4l2_format fmt = {0};
    unsigned int min;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 1920;
    fmt.fmt.pix.height      = 1080;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR10;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    init_mmap(fd);

    return 0;
}

void uninit_device(void) {
    unsigned int i;

    for (i = 0; i < n_buffers; ++i)
        if (-1 == munmap(buffers[i].start, buffers[i].length))
            errno_exit("munmap");

    free(buffers);
}

int start_capturing(int fd) {
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf = {0};

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
    }
                
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        errno_exit("VIDIOC_STREAMON");

    return 0;
}

void stop_capturing(int fd) {
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
        errno_exit("VIDIOC_STREAMOFF");
}

int stream_frame(GstAppSrc* appsrc, void* start, size_t length) {
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;

    if (!want) 
        return -1;
    want = 0;

    buffer = gst_buffer_new_wrapped_full(0, start, length, 0, length, NULL, NULL );
    GST_BUFFER_PTS (buffer) = timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 30);
    timestamp += GST_BUFFER_DURATION (buffer);

    ret = gst_app_src_push_buffer(appsrc, buffer);

    if (ret != GST_FLOW_OK) {
        /* something wrong, stop pushing */
        fprintf(stderr, "push buffer error: %d\n", ret);
        // g_main_loop_quit (loop);
    }

    return 0;
}

int prepare_to_read(int fd, GstAppSrc* appsrc) {
    struct v4l2_buffer buf = {0};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
            return 0;

        case EIO:
            /* Could ignore EIO, see spec. */
            /* fall through */

        default:
            errno_exit("VIDIOC_DQBUF");
        }
    }

    assert(buf.index < n_buffers);

    // process_image(buffers[buf.index].start, buf.bytesused);
    stream_frame(appsrc, buffers[buf.index].start, buf.bytesused);

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");

    return 1;
}

void wait_frame(int fd, GstAppSrc* appsrc) {
    for (;;) {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
                if (EINTR == errno)
                        continue;
                errno_exit("select");
        }

        if (0 == r) {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
        }
                
        if (prepare_to_read(fd, appsrc))
            break;
    }
}

void appsrc_need_data (GstElement *appsrc, guint unused_size, gpointer user_data) {
    (void)appsrc;
    (void)unused_size,
    (void)user_data;
    want = 1;
}

int init_pipeline(struct cam_pipeline* pipe) {

    /* setup pipeline */
    pipe->gst_pipeline = gst_pipeline_new ("pipeline");
    pipe->appsrc = gst_element_factory_make ("appsrc", "source");
    pipe->queue = gst_element_factory_make ("queue", "queue");
    pipe->rawvideo = gst_element_factory_make ("rawvideoparse", "rawvideo");
    pipe->scale = gst_element_factory_make("videoscale", "scale");
    pipe->scale_capsfilter = gst_element_factory_make("capsfilter", "caps");
    pipe->conv = gst_element_factory_make ("videoconvert", "conv");
    pipe->videosink = gst_element_factory_make ("fbdevsink", "videosink");

    /* setup appsrc */
    g_object_set (G_OBJECT (pipe->appsrc), "caps",
  		gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, "NV12",
			"width", G_TYPE_INT, 1920,
			"height", G_TYPE_INT, 1080,
			"framerate", GST_TYPE_FRACTION, 30, 1,
		NULL), NULL);

    /* setup voidescale */
    g_object_set (G_OBJECT (pipe->scale_capsfilter), "caps",
  		gst_caps_new_simple ("video/x-raw",
			"width", G_TYPE_INT, 800,
			"height", G_TYPE_INT, 480,
		NULL), NULL);

    /* setup videosink */
    g_object_set (G_OBJECT(pipe->videosink), "sync", false, NULL);

    gst_bin_add_many (
        GST_BIN (pipe->gst_pipeline), 
        pipe->appsrc, 
        pipe->scale, 
        pipe->scale_capsfilter,
        pipe->conv, 
        pipe->videosink, NULL);
    
    gst_element_link_many (
        pipe->appsrc, 
        pipe->scale, 
        pipe->scale_capsfilter,
        pipe->conv, 
        pipe->videosink, NULL);

    /* setup appsrc */
    g_object_set (G_OBJECT (pipe->appsrc),
		"stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
		"format", GST_FORMAT_TIME,
        "is-live", TRUE, NULL);

    g_signal_connect (pipe->appsrc, "need-data", G_CALLBACK (appsrc_need_data), NULL);

    return 0;
}

int main(int argc, char* argv[]) {

    int fd = -1;
    struct cam_pipeline pipeline = {0};

    /* initialize camera */
    const char* dev = (2 == argc) ? argv[1] : "/dev/video2";
    fd = open_device(dev);
    init_device(fd);
    start_capturing(fd);

    /* initialize GStreamer */
    gst_init(&argc, &argv);
    init_pipeline(&pipeline);

    /* play */
    gst_element_set_state (pipeline.gst_pipeline, GST_STATE_PLAYING);
  
    while (1) {
        wait_frame(fd, (GstAppSrc*)pipeline.appsrc);
        g_main_context_iteration(g_main_context_default(),FALSE);
    }

    /* clean up GStreamer */
    gst_element_set_state (pipeline.gst_pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline.gst_pipeline));

    /* clean up v4l2 */
    stop_capturing(fd);
    uninit_device();
    close_device(fd);

    return EXIT_SUCCESS;
}