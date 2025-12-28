LOCAL_PATH := $(call my-dir)
BASE_DIR   := $(LOCAL_PATH)/../../..
CORE_DIR   := $(BASE_DIR)/src

INCFLAGS := -I$(CORE_DIR) \
   -I$(CORE_DIR) \
   -I$(CORE_DIR)/common \
   -I$(CORE_DIR)/libretro

SRCS := $(CORE_DIR) $(CORE_DIR)/common $(CORE_DIR)/libretro
SOURCES_CXX := $(foreach dir,$(SRCS),$(wildcard $(dir)/*.cpp))

#ROMDIR = rom/
#DISKDIR = disk/
#TAPEDIR = tape/

GIT_VERSION := $(shell git describe --abbrev=7 --always)
COREFLAGS := -D__LIBRETRO__ -DVERSION=\"$(GIT_VERSION)\" -fexceptions -std=c++11

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_DISABLE_FORMAT_STRING_CHECKS := true
LOCAL_SRC_FILES    := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_CPPFLAGS     := $(COREFLAGS)
LOCAL_CPP_FEATURES := exceptions
LOCAL_CXXFLAGS     := $(COREFLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(BASE_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
