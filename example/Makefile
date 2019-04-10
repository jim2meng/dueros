#
# Copyright (2019) Yundeaiot Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TOPDIR := ../
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
 
# Set optimization level.
CFLAGS += -O3
CXXFLAGS += -O3

CFLAGS += -D_GNU_SOURCE -lrt
CFLAGS += -std=c99

TARGET := record

OBJFILES = record.o

CFLAGS += $(shell pkg-config --cflags --libs gstreamer-1.0)
LDLIBS += -lm \
    -lrt \
    -lasound \
    $(shell pkg-config --cflags --libs gstreamer-1.0)

all: $(TARGET)

$(TARGET) : $(OBJFILES)
	$(CC) $(OBJFILES) $(CFLAGS) $(LDLIBS) -o $(TARGET)

clean:
	-rm -f *.o *.wav $(TARGET) $(OBJFILES)

