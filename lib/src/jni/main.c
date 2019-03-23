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

#include "sb.h"
#include "gl.h"
#include "bg.h"
#include "img.h"
#include "font.h"

#ifdef STATIC_BG
#include "game.h"
#else
#include "cv.h"
#endif

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

static uint8_t rgba_;
static uint8_t done_;
static uint8_t pause_;

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
#ifdef STATIC_BG
	if (!rgba_)
		bg_render(0);
	else
		bg_render_color(.3, .5, .8, 1);

	game_render();
#else
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texid_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp_.w, bmp_.h, 0, GL_RGB,
	  GL_UNSIGNED_BYTE, bmp_.data);

	bg_render(0);
	cv_render();
#endif
}

static void handle_resize(xcb_resize_request_event_t *e)
{
	bg_resize(e->width, e->height);
#ifdef STATIC_BG
	game_resize(e->width, e->height);
#else
	cv_resize(e->width, e->height);
#endif
}

static void handle_button_press(xcb_button_press_event_t *e)
{
	switch (e->detail) {
	case MOUSE_BTN_LEFT:
		game_touch(e->event_x, e->event_y);
		break;
	case MOUSE_BTN_MID: /* fall through */
	case MOUSE_BTN_RIGHT:
		break;
	case MOUSE_BTN_FWD:
		break;
	case MOUSE_BTN_BACK:
		break;
	default:
		break;
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
		if (pause_) {
			dd("continue\n");
			pause_ = 0;
		} else {
			dd("pause\n");
			pause_ = 1;
		}
	} else if (sym == XK_BackSpace) {
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
	case XCB_BUTTON_RELEASE:
		game_touch(-1, -1);
		rc = 1;
		break;
	case XCB_RESIZE_REQUEST:
		handle_resize((xcb_resize_request_event_t *) e);
		break;
	default:
		dd("got message type %d", type);
	}

out:
	if (!pause_ && (refresh || poll)) {
		render();
		eglSwapBuffers(egl_, surf_);
	}

	xcb_flush(dpy_);
	free(e);

	return rc;
}

static uint8_t create_window(uint16_t w, uint16_t h)
{
	uint32_t mask;
	uint32_t val[2];
	int num;
	EGLConfig cfg;
	xcb_window_t old_win = win_;

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

	num = DefaultScreen(dpy_);

	if (!eglChooseConfig(egl_, attrs, &cfg, 1, &num)) {
		ee("eglChooseConfig() failed\n");
		return 0;
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
	  w, h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, scr_->root_visual,
	  mask, val);

	xcb_change_property(dpy_, XCB_PROP_MODE_REPLACE, win_, XCB_ATOM_WM_NAME,
	  XCB_ATOM_STRING, 8, sizeof("opengl") - 1, "opengl");

	xcb_change_property(dpy_, XCB_PROP_MODE_REPLACE, win_, XCB_ATOM_WM_CLASS,
	  XCB_ATOM_STRING, 8, sizeof("opengl") - 1, "opengl");

	xcb_map_window(dpy_, win_);
	xcb_flush(dpy_);

	eglBindAPI(EGL_OPENGL_ES_API);

	if (!ctx_ && !(ctx_ = eglCreateContext(egl_, cfg, EGL_NO_CONTEXT, ctx_attrs))) {
		ee("Error: eglCreateContext failed\n");
		return 0;
	}

	if (surf_)
		eglDestroySurface(egl_, surf_);

	if (!(surf_ = eglCreateWindowSurface(egl_, cfg, win_, NULL))) {
		ee("Error: eglCreateWindowSurface failed\n");
		return 0;
	}

	if (!eglMakeCurrent(egl_, surf_, surf_, ctx_)) {
		ee("eglMakeCurrent() failed\n");
		return 0;
	}

	if (old_win != XCB_WINDOW_NONE) {
		xcb_destroy_window(dpy_, old_win);
		xcb_flush(dpy_);
	}

	return 1;
}

static void resize_window(uint16_t w, uint16_t h)
{
	/* since xcb resize request does not work well with gl windows just
	 * re-create one with new size */

	create_window(w, h);
	ii("created win %#x with new size %u,%u\n", win_, w, h);
}

#include "tools.h"

static int init_scene(const char *path)
{
	if (!(font0_ = font_open(FONT_PATH, 36, NULL)))
		return -1;

	if (!(font1_ = font_open(FONT_PATH, 48, NULL)))
		return -1;

#ifdef STATIC_BG
	if (!path) {
		bg_open_color();
	} else {
		texid_ = bg_open(path, NULL, &bmp_.w, &bmp_.h);
		resize_window(bmp_.w, bmp_.h);
	}

	bg_resize(bmp_.w, bmp_.h);
	game_open(font0_, font1_, NULL);
	game_resize(bmp_.w, bmp_.h);
#else
	bg_resize(bmp_.w, bmp_.h);
	cv_open(font0_, font1_, CV_BLOCK);
	cv_resize(bmp_.w, bmp_.h);
	texid_ = bg_open();
#endif

	return 0;
}

static uint8_t init_display(void)
{
	if (!(dpy_ = xcb_connect(NULL, NULL))) {
		ee("xcb_connect() failed\n");
		return 0;
	}

	if (!(syms_ = xcb_key_symbols_alloc(dpy_)))
		ee("xcb_key_symbols_alloc() failed\n");

	scr_ = xcb_setup_roots_iterator(xcb_get_setup(dpy_)).data;
	return 1;
}

static void image_loop(void)
{
	while (!done_) {
		events(1);
		sleep_ms(16);
	}
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

#ifdef STATIC_BG
	if (argc == 2) {
		path = argv[1];
		bmp_.w = bmp_.h = 100;
	} else {
		ii("Usage: %s [image]\n", argv[0]);
		bmp_.w = 580;
		bmp_.h = bmp_.w * 1.7777;
		path = NULL;
		rgba_ = 1;
	}
#else
	if (argc == 2) {
		path = argv[1];
	} else {
		path = VIDEO_DEV;
		ii("Usage: %s <dev> (default %s)\n", argv[0], path);
	}
#endif

	if (path && stat(path, &st) < 0) {
		ee("failed to stat '%s'\n", path);
		return 1;
	}

#ifndef STATIC_BG
	if (!init_device(path, &st, &info))
		return 1;
#endif

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

	if (!init_display())
		return 1;

	if (!create_window(bmp_.w, bmp_.h))
		return 1;

	ii("created win %#x size %u,%u\n", win_, bmp_.w, bmp_.h);

	ii("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
	ii("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
	ii("GL_VENDOR = %s\n", (char *) glGetString(GL_VENDOR));
	ii("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));

	if (init_scene(path) < 0)
		return 1;

#ifdef STATIC_BG
	image_loop();
#else
	video_loop(argv, &info);
#endif

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
#ifdef STATIC_BG
	game_close();
#else
	cv_close();
#endif

	return 0;
}
