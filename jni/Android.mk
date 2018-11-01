LOCAL_PATH := $(call my-dir)

MINIGZIP_LOCAL_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= main.c \
                  libmincrypt/sha.c \
                  libmincrypt/rsa.c\
                  libmincrypt/dsa_sig.c\
                  libmincrypt/p256.c\
                  libmincrypt/p256_ec.c\
                  libmincrypt/p256_ecdsa.c \
                  libmincrypt/sha256.c \
                  mkbootimg/mkbootimg.c \
                  utils/device_vendor.c \
                  mkbootimg/unmkbootimg.c \
                  utils/fs_utils.c \
                  cpio/mkbootfs.c \
                  utils/fs_config.c \
                  cpio/uncpio.c \
                  utils/fs_mtd.c \
                  utils/minigzip.c


LOCAL_MODULE:= repack-img

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/utils

LOCAL_CFLAGS := -Wall -Wextra -Wno-unused-parameter -pedantic -Wno-parentheses -pipe -Wno-pointer-arith -Wno-unused-function -Wno-pedantic -std=c99 -D_GNU_SOURCE -rdynamic -DANDROID -marm -pie -fPIE

LOCAL_LDFLAGS := -pie -fPIE

LOCAL_LDLIBS := -L$(LOCAL_PATH)/../../lib -lz -llog

include $(BUILD_EXECUTABLE)