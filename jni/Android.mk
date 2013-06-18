LOCAL_PATH := $(call my-dir)

SRC_DIR := $(APP_PROJECT_PATH)/src

include $(CLEAR_VARS)
LOCAL_MODULE    := proxy
LOCAL_CFLAGS := \
	-Wall -g -O2 -pthread  \
	-DWITHMAIN -DNOPORTMAP -DANONYMOUS -DGETHOSTBYNAME_R -D_THREAD_SAFE \
	-D_REENTRANT -DNOODBC -DWITH_STD_MALLOC -DFD_SETSIZE=4096 -DWITH_POLL
LOCAL_SRC_FILES := \
	$(SRC_DIR)/proxy.c \
	$(SRC_DIR)/sockmap.c \
	$(SRC_DIR)/sockgetchar.c \
	$(SRC_DIR)/myalloc.c \
	$(SRC_DIR)/common.c \
	$(SRC_DIR)/base64.c \
	$(SRC_DIR)/ftp.c \

LOCAL_C_INCLUDES := 

include $(BUILD_EXECUTABLE)