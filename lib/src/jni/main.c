#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_keysyms.h>

#include <EGL/egl.h>

#include <utils/time.h>
#include <utils/log.h>
#include <utils/sensors.h>

#include "cv.h"
#include "sb.h"
#include "gl.h"
#include "bg.h"
#include "img.h"
#include "font.h"

#define MOUSE_BTN_LEFT 1
#define MOUSE_BTN_MID 2
#define MOUSE_BTN_RIGHT 3
#define MOUSE_BTN_FWD 4
#define MOUSE_BTN_BACK 5

#define VIDEO_W 640
#define VIDEO_H 480

#ifndef VIDEO_DEV
#define VIDEO_DEV "/dev/video0"
#endif

#define FONT_PATH ASSETS"fonts/Shure-Tech-Mono-Nerd-Font-Complete.ttf"

static struct font *font0_;
static struct font *font1_;

static GLuint texid_;

struct buffer {
	void *ptr;
	size_t len;
};

struct fileinfo {
	const char *prefix;
	const char *ext;
	struct buffer *buf;
	size_t num; /* number of buffers or files */
	uint8_t dir;
};

struct bitmap {
	void *data;
	uint16_t w;
	uint16_t h;
};

struct bitmap bmp_;

static xcb_connection_t *dpy_;
static xcb_screen_t *scr_;
static xcb_drawable_t win_;
static xcb_key_symbols_t *syms_;

static uint16_t w_ = 768;
static uint16_t h_ = 576;
static uint8_t back_;
static uint8_t done_;
static uint8_t stop_;

static EGLSurface surf_;
static EGLContext ctx_;
static EGLDisplay egl_;

static int fd_;

static uint8_t devreq(int req, void *arg)
{
	int rc;

	do {
		rc = v4l2_ioctl(fd_, req, arg);
	} while (rc == -1 && ((errno == EINTR) || (errno == EAGAIN)));

	if (rc < 0) {
		ee("v4l2_ioctl(%d) failed\n", fd_);
		return 0;
	}

	return 1;
}

static void render(void)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texid_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp_.w, bmp_.h, 0, GL_RGB,
	  GL_UNSIGNED_BYTE, bmp_.data);

	bg_render(0);
	cv_render();
}

static void handle_resize(xcb_resize_request_event_t *e)
{
	bg_resize(e->width, e->height);
	cv_resize(e->width, e->height);
}

static void handle_button_press(xcb_button_press_event_t *e)
{
	switch (e->detail) {
	case MOUSE_BTN_LEFT:
		back_ = 0;
		break;
	case MOUSE_BTN_MID: /* fall through */
	case MOUSE_BTN_RIGHT:
		back_ = 1;
		break;
	case MOUSE_BTN_FWD:
		break;
	case MOUSE_BTN_BACK:
		break;
	default:
		break;
	}
}

static void pause_sampling(void)
{
	if (stop_) {
		ii("cont sampling\n");
		stop_ = 0;
	} else {
		ii("stop sampling\n");
		stop_ = 1;
	}
}

static void handle_key_press(xcb_key_press_event_t *e)
{
	xcb_keysym_t sym;

	if (!syms_) {
		ee("key symbols not available\n");
		return;
	}

	sym = xcb_key_press_lookup_keysym(syms_, e, 0);

	if (sym == XK_Escape) {
		done_ = 1;
	} else if (sym == XK_Up) {
	} else if (sym == XK_Down) {
	} else if (sym == XK_Left) {
	} else if (sym == XK_Right) {
	} else if (sym == XK_Next) {
	} else if (sym == XK_Prior) {
	} else if (sym == XK_Return) {
	} else if (sym == XK_space) {
		back_ = 0;
		pause_sampling();
	} else if (sym == XK_BackSpace) {
		back_ = 1;
	} else {
		return;
	}
}

static uint8_t events(uint8_t poll)
{
	uint8_t rc;
	uint8_t refresh = 0;
	uint8_t type;
	xcb_generic_event_t *e;

	if (poll)
		e = xcb_poll_for_event(dpy_);
	else
		e = xcb_wait_for_event(dpy_);

	if (!e)
		goto out;

	rc = 0;
	type = XCB_EVENT_RESPONSE_TYPE(e);

	switch (type) {
	case XCB_VISIBILITY_NOTIFY:
		switch (((xcb_visibility_notify_event_t *) e)->state) {
		case XCB_VISIBILITY_PARTIALLY_OBSCURED: /* fall through */
		case XCB_VISIBILITY_UNOBSCURED:
			refresh = 1;
			break;
		}
		break;
	case XCB_EXPOSE:
		refresh = 1;
		break;
	case XCB_KEY_PRESS:
		handle_key_press((xcb_key_press_event_t *) e);
		rc = 1;
		break;
	case XCB_BUTTON_PRESS:
		handle_button_press((xcb_button_press_event_t *) e);
		rc = 1;
		break;
	case XCB_RESIZE_REQUEST:
		handle_resize((xcb_resize_request_event_t *) e);
		break;
	default:
		dd("got message type %d", type);
	}

out:

	if (refresh || poll) {
		render();
		eglSwapBuffers(egl_, surf_);
	}

	xcb_flush(dpy_);
	free(e);

	return rc;
}

static int init_scene(void)
{
	if (!(font0_ = font_open(FONT_PATH, 64, NULL)))
		return -1;

	if (!(font1_ = font_open(FONT_PATH, 128, NULL)))
		return -1;

	cv_open(font0_, font1_, CV_BLOCK);
	texid_ = bg_open();
}

static int init_context(void)
{
	uint32_t mask;
	uint32_t val[2];
	int num;
	EGLConfig cfg;

	static const EGLint attrs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_DEPTH_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	static const EGLint ctx_attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	if (!(dpy_ = xcb_connect(NULL, NULL))) {
		ee("xcb_connect() failed\n");
		return 1;
	}

	if (!(syms_ = xcb_key_symbols_alloc(dpy_)))
		ee("xcb_key_symbols_alloc() failed\n");

	scr_ = xcb_setup_roots_iterator(xcb_get_setup(dpy_)).data;
	num = DefaultScreen(dpy_);

	if (!eglChooseConfig(egl_, attrs, &cfg, 1, &num)) {
		ee("eglChooseConfig() failed\n");
		return -1;
	}

	win_ = xcb_generate_id(dpy_);

	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	val[0] = 0;
	val[1] = XCB_EVENT_MASK_EXPOSURE;
	val[1] |= XCB_EVENT_MASK_VISIBILITY_CHANGE;
	val[1] |= XCB_EVENT_MASK_KEY_PRESS;
	val[1] |= XCB_EVENT_MASK_KEY_RELEASE;
	val[1] |= XCB_EVENT_MASK_BUTTON_PRESS;
	val[1] |= XCB_EVENT_MASK_BUTTON_RELEASE;
	val[1] |= XCB_EVENT_MASK_POINTER_MOTION;
	val[1] |= XCB_EVENT_MASK_RESIZE_REDIRECT;

	xcb_create_window(dpy_, XCB_COPY_FROM_PARENT, win_, scr_->root, 0, 0,
	  w_, h_, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, scr_->root_visual, mask,
	  val);

	xcb_change_property(dpy_, XCB_PROP_MODE_REPLACE, win_, XCB_ATOM_WM_NAME,
	  XCB_ATOM_STRING, 8, sizeof("opengl") - 1, "opengl");

	xcb_change_property(dpy_, XCB_PROP_MODE_REPLACE, win_, XCB_ATOM_WM_CLASS,
	  XCB_ATOM_STRING, 8, sizeof("opengl") - 1, "opengl");

	xcb_map_window(dpy_, win_);
	xcb_flush(dpy_);

	eglBindAPI(EGL_OPENGL_ES_API);

	if (!(ctx_ = eglCreateContext(egl_, cfg, EGL_NO_CONTEXT, ctx_attrs))) {
		printf("Error: eglCreateContext failed\n");
		return -1;
	}

	if (!(surf_ = eglCreateWindowSurface(egl_, cfg, win_, NULL))) {
		printf("Error: eglCreateWindowSurface failed\n");
		return -1;
	}

	if (!eglMakeCurrent(egl_, surf_, surf_, ctx_)) {
		ee("eglMakeCurrent() failed\n");
		return 1;
	}

	return 0;
}

static void video_loop(const char *argv[], struct fileinfo *info)
{
	struct v4l2_buffer buf;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct pollfd fds;

	if (!devreq(VIDIOC_STREAMON, &type))
		return;

	fds.fd = fd_;

	while (!done_) {
		fds.events = POLLIN;
		fds.revents = 0;

		if (poll(&fds, 1, -1) < 0) {
			if (errno == EINTR)
				continue;

			ee("poll() failed\n");
			break;
		}

		if (!(fds.revents & POLLIN))
			continue;

		events(1);

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (!devreq(VIDIOC_DQBUF, &buf))
			break;

		memcpy(bmp_.data, info->buf[buf.index].ptr, buf.bytesused);

		if (!devreq(VIDIOC_QBUF, &buf))
			break;
	}
}

static uint8_t init_device(const char *arg, struct stat *st,
  struct fileinfo *info)
{
	struct v4l2_format fmt;
	struct v4l2_buffer buf;
	struct v4l2_requestbuffers req;
	fd_set fds;
	struct timeval tv;
	int rc;
	size_t i;

	if (!S_ISCHR(st->st_mode))
		return 0;

	if ((fd_ = v4l2_open(arg, O_RDWR | O_NONBLOCK, 0)) < 0) {
		ee("failed to open v4l2_device '%s'\n", arg);
		return 0;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = VIDEO_W; /* hint */
	fmt.fmt.pix.height = VIDEO_H; /* hint */
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (!devreq(VIDIOC_S_FMT, &fmt))
		return 0;

	if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
		ee("v4l does not support RGB24\n");
		return 0;
	}

	ii("texture %ux%u\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

	bmp_.w = fmt.fmt.pix.width;
	bmp_.h = fmt.fmt.pix.height;

	size_t size = bmp_.w * bmp_.h * 3;

	if (!(bmp_.data = malloc(size))) {
		ee("failed to allocate %zu bytes\n", size);
		return 0;
	}

	memset(&req, 0, sizeof(req));
	req.count = 2;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (!devreq(VIDIOC_REQBUFS, &req))
		return 0;

	if (!(info->buf = calloc(req.count, sizeof(*info->buf))))
		return 0;

	for (info->num = 0; info->num < req.count; ++info->num) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = info->num;

		if (!devreq(VIDIOC_QUERYBUF, &buf))
			return 0;

		info->buf[info->num].len = buf.length;
		info->buf[info->num].ptr = v4l2_mmap(NULL, buf.length,
						     PROT_READ | PROT_WRITE,
						     MAP_SHARED, fd_,
						     buf.m.offset);

		if (MAP_FAILED == info->buf[info->num].ptr) {
			ee("v4l2_mmap() failed\n");
			return 0;
		}
	}

	for (i = 0; i < info->num; ++i) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (!devreq(VIDIOC_QBUF, &buf))
			return 0;
	}

	return 1;
}

int main(int argc, const char *argv[])
{
	EGLint major;
	EGLint minor;
	struct stat st;
	struct fileinfo info;
	const char *path = VIDEO_DEV;

	if (argc == 2) {
		path = argv[1];
	} else {
		path = VIDEO_DEV;
		ii("Usage: %s <dev> (default %s)\n", argv[0], path);
	}

	if (stat(path, &st) < 0) {
		ee("failed to stat '%s'\n", path);
		return 1;
	}

	if (!init_device(path, &st, &info))
		return 1;

	if (!(egl_ = eglGetDisplay(EGL_DEFAULT_DISPLAY))) {
		ee("eglGetDisplay() failed\n");
		return 1;
	}

	if (!eglInitialize(egl_, &major, &minor)) {
		ee("eglInitialize() failed\n");
		return 1;
	}

	ii("EGL_VERSION = %s\n", eglQueryString(egl_, EGL_VERSION));
	ii("EGL_VENDOR = %s\n", eglQueryString(egl_, EGL_VENDOR));
	ii("EGL_EXTENSIONS = %s\n", eglQueryString(egl_, EGL_EXTENSIONS));
	ii("EGL_CLIENT_APIS = %s\n", eglQueryString(egl_, EGL_CLIENT_APIS));

	if (init_context() < 0)
		return 1;

	ii("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
	ii("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
	ii("GL_VENDOR = %s\n", (char *) glGetString(GL_VENDOR));
	ii("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));

	if (init_scene() < 0)
		return 1;

	video_loop(argv, &info);

	eglDestroyContext(egl_, ctx_);
	eglDestroySurface(egl_, surf_);
	eglTerminate(egl_);
	xcb_destroy_window(dpy_, win_);

	if (fd_) {
		size_t i;
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (devreq(VIDIOC_STREAMOFF, &type)) {
			for (i = 0; i < info.num; ++i)
				v4l2_munmap(info.buf[i].ptr, info.buf[i].len);
		}

		v4l2_close(fd_);
	}

	font_close(&font0_);
	font_close(&font1_);

	bg_close();
	cv_close();

	return 0;
}
