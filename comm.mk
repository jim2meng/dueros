TOPDIR := ./
DYNAMIC := False
CC :=
CXX :=
LDFLAGS :=
LDLIBS :=

CFLAGS :=
CXXFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0

ifeq ($(DYNAMIC), True)
  CFLAGS += -fPIC
  CXXFLAGS += -fPIC
  CFLAGS += BUILD_DUEROS_DYNAMIC
endif

CC := gcc
CXX := g++
CFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/include/libduer-device/include -I$(TOPDIR)/include/snowboy/include -Wall -L ./libs/
CXXFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/include/libduer-device/include -I$(TOPDIR)/include/snowboy/include -std=c++0x -Wall -Wno-sign-compare \
      -Wno-unused-local-typedefs -Winit-self -rdynamic
      
LDLIBS +=  -Wl,-rpath=./libs -lduer-device -lsnowboy-detect-c

# Set optimization level.
CFLAGS += -O3
CXXFLAGS += -O3

