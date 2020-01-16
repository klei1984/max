AS = gcc
CC = gcc
LD = gcc
CXX = g++

RM = rm

PYTHON = python3

ASFLAGS := -m32 -O0 -no-pie -Wall -Wno-unused-parameter -g
CFLAGS := -m32 -O0 -no-pie -Wall -Wno-unused-parameter -g `pkg-config --cflags --libs sdl2`
CXXFLAGS := -m32 -O0 -no-pie -Wall -Wno-unused-parameter -g `pkg-config --cflags --libs sdl2`
LDFLAGS := -m32 -O0 -no-pie -g `pkg-config --cflags --libs sdl2`
DEPFLAGS = -MT $@ -MMD -MP -MF $*.d

# -g
MKWFLAGS =  
MKW = $(PYTHON) ./util/mkwrappers.py
MKW_RENAME	 = ./conf/fnmap_posix.conf

CHGC = $(PYTHON) ./util/chgcalls.py
CHGC_RENAME = ./conf/fnmap_chgcalls.conf

CONFIG_FILE = ./conf/wrappers.conf

TARGET = maxrun

LIBS := -L/usr/lib/i386-linux-gnu -lSDL2 -lm -lstdc++

OBJ := \
	$(TARGET).o \
	main.o \
	game.o \
	color.o \
	key.o \
	mouse.o \
	button.o \
	timer.o \
	sound.o \
	svga.o \
	wrappers.o \
	dos.o

export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

all: $(TARGET)

clean:
	@rm -f $(TARGET) *.log *.o *.S *.d

$(TARGET).S: $(TARGET).in $(CONFIG_FILE) $(CHGC_RENAME)
	$(CHGC) --rename-config=$(CHGC_RENAME) $(CONFIG_FILE) $< $@

wrappers.S: $(CONFIG_FILE) $(MKW_RENAME)
	$(MKW) $(MKWFLAGS) -o $@ -r $(MKW_RENAME) $<

.S.o: %.d
	$(AS) -c -o $@ $(ASFLAGS) $(DEPFLAGS) $<

.c.o: %.d
	$(CC) -c -o $@ $(CFLAGS) $(DEPFLAGS) $<

.cpp.o: %.d
	$(CXX) -c -o $@ $(CXXFLAGS) $(DEPFLAGS) $<

$(TARGET): $(OBJ)
	$(LD) -o $(TARGET) $(LDFLAGS) $(OBJ) $(LIBS)
#  -Wl,-Map=$@.map


DEPFILES := $(OBJ:%.o=%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
