/* gl.h: graphics helpers
 *
 * Copyright (c) 2018, Aliaksei Katovich <aliaksei.katovich at gmail.com>
 *
 * Released under the GNU General Public License, version 2
 */

#pragma once

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define gl_disable_features() {\
    glDisable(GL_DEPTH_TEST);\
    glDisable(GL_CULL_FACE);\
    glDisable(GL_SCISSOR_TEST);\
    glDisable(GL_STENCIL_TEST);\
    glDisable(GL_BLEND);\
    glDepthMask(0);\
}

#define gl_enable_features() {\
    glEnable(GL_DEPTH_TEST);\
    glDepthMask(1);\
}

#define gl_error(msg) {\
	for (GLint err = glGetError(); err; err = glGetError()) {\
		ee("%s error '0x%x'\n", msg, err);\
	}\
}

GLuint gl_make_prog(const char *vsrc, const char *fsrc);
