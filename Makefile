# NoDS NDS/GBA emulator
# Copyright (C) 2016 Frederic Meyer


#CC = clang
CFLAGS  += -Wall -g -DDEBUG
LDFLAGS += -lSDL

TARGET = nods

MODULES  := platform/sdl arm arm/gdb nds common gba .
SRC_DIR  := $(addprefix src/, $(MODULES))

SOURCES  := $(foreach sdir, $(SRC_DIR), $(wildcard $(sdir)/*.c))
INCLUDES := $(addprefix -I, $(SRC_DIR))
OBJECTS  := $(addsuffix .o, $(basename $(SOURCES)))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean

