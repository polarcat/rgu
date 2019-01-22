LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lib3d
LOCAL_SRC_FILES := \
lib3d/jni.c \
lib3d/gl.c \
lib3d/gm.c \
lib3d/bg.c \
lib3d/img.c \
lib3d/font.c \
lib3d/draw.c \
lib3d/pip.c \
lib3d/cv.c \
lib3d/plotter.c \
utils/image.c \
utils/sensors.c \

ifneq (,$(wildcard $(LOCAL_PATH)/algo))
LOCAL_CFLAGS += "-DHAVE_ALGO"
endif

PNG_LIB := png/libs/$(NDK_APP_ABI)/libpng.so
PNG_INC := png

JPEG_LIB := jpeg/libs/$(NDK_APP_ABI)/libjpeg.so
JPEG_INC := jpeg

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(PNG_INC) $(JPEG_INC)
LOCAL_LDLIBS += -lm -llog -lGLESv2
LOCAL_LDLIBS += -landroid # assets handling
LOCAL_LDLIBS += -Wl,--start-group $(PNG_LIB) $(JPEG_LIB) -Wl,--end-group
LOCAL_CFLAGS += "-DLIB_TAG=\"lib3d\"" -DIMAGE_VIEWER

include $(BUILD_SHARED_LIBRARY)
