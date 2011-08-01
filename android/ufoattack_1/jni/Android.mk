LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := ufoattack

LOCAL_CFLAGS := -DANDROID_NDK \
                -DDISABLE_IMPORTGL \
				-DGRINLIZ_NO_STL

LOCAL_SRC_FILES := \
    app-android.c \

include $(LOCAL_PATH)/../../../gamui/sources.mk
include $(LOCAL_PATH)/../../../shared/sources.mk
include $(LOCAL_PATH)/../../../grinliz/sources.mk
include $(LOCAL_PATH)/../../../tinyxml/sources.mk
include $(LOCAL_PATH)/../../../micropather/sources.mk
include $(LOCAL_PATH)/../../../engine/sources.mk
include $(LOCAL_PATH)/../../../game/sources.mk
include $(LOCAL_PATH)/../../../faces/sources.mk

LOCAL_LDLIBS := -lGLESv1_CM -ldl -llog -lz

include $(BUILD_SHARED_LIBRARY)
