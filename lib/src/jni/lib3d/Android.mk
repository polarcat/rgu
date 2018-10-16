LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lib3d
LOCAL_SRC_FILES := \
jni.c \
font.c \
gl.c \
gm.c \
bg.c \
point.c \
../utils/image.c \

ifeq (,$(wildcard ../libar))
LOCAL_SRC_FILES += \
../libar/tracking.c
LOCAL_CFLAGS += "-DHAVE_LIBAR"
endif

PNG_LIBS := png/libs/$(NDK_APP_ABI)/libpng.so
PNG_INC := png

JPEG_LIBS := jpeg/libs/$(NDK_APP_ABI)/libjpeg.so
JPEG_INC := jpeg

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../
LOCAL_C_INCLUDES += $(ARCORE_INC) $(PNG_INC) $(JPEG_INC)
LOCAL_LDLIBS += -lm -llog -lGLESv2
LOCAL_LDLIBS += -landroid # assets handling
LOCAL_LDLIBS += -Wl,--start-group $(ARCORE_LIBS) $(PNG_LIBS) $(JPEG_LIBS) -Wl,--end-group
LOCAL_CFLAGS += "-DLIB_TAG=\"lib3d\""

include $(BUILD_SHARED_LIBRARY)
