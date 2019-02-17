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

include comm.mk

CFLAGS += -D_GNU_SOURCE -lrt
CFLAGS += -std=c99

TARGET := duerospi

OBJFILES = src/duerapp.o
OBJFILES += src/duerapp_alert.o
OBJFILES += src/duerapp_event.o
OBJFILES += src/duerapp_media.o
OBJFILES += src/duerapp_profile_config.o
OBJFILES += src/duerapp_recorder.o

CFLAGS += $(shell pkg-config --cflags --libs gstreamer-1.0)
LDLIBS += -lm \
    -lrt \
    -lasound \
    $(shell pkg-config --cflags --libs gstreamer-1.0)

all: $(TARGET)

$(TARGET) : $(OBJFILES)
	$(CC) $(OBJFILES) $(CFLAGS) $(LDLIBS) -o $(TARGET)

clean:
	-rm -f *.o  $(TARGET) $(OBJFILES)

