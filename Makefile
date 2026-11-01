.SUFFIXES:

TOPDIR := $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET  := cotl-3ds
SOURCES := source

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -O2 -Wall -mword-relocations -ffunction-sections $(ARCH) -I$(CTRULIB)/include -D__3DS__
CXXFLAGS := -g -O2 -Wall -mword-relocations -ffunction-sections $(ARCH) -I$(CTRULIB)/include -D__3DS__ -fno-rtti -fno-exceptions -std=gnu++11
LDFLAGS := -specs=3dsx.specs -g $(ARCH)
LIBS := -lcitro2d -lcitro3d -lctru -lm

OBJ := main.o

all: $(TARGET).cia

$(TARGET).elf: $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) -L$(CTRULIB)/lib $(LIBS) -o $@

$(TARGET).smdh: $(TARGET).elf
	smdhtool --create "CultLamb" "Cult of the Lamb demo" "Homebrew" icon.png $@

$(TARGET).cia: $(TARGET).elf $(TARGET).smdh
	makerom -f cia -o $@ -elf $(TARGET).elf -smdh $(TARGET).smdh -target t

main.o: $(SOURCES)/main.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f *.o *.elf *.smdh *.cia
