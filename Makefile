.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET		:=	cotl-3ds
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	include

ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS	:=	-g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS	+=	$(INCLUDE) -D__3DS__
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS	:= -lcitro2d -lcitro3d -lctru -lm
LIBDIRS	:= $(CTRULIB)

# иконка
ifeq ($(strip $(ICON)),)
icons := $(wildcard *.png)
ifneq (,$(findstring icon.png,$(icons)))
export APP_ICON := $(TOPDIR)/icon.png
endif
else
export APP_ICON := $(TOPDIR)/$(ICON)
endif

APP_TITLE		:=	CultLamb
APP_DESCRIPTION	:=	Cult of the Lamb style demo
APP_AUTHOR		:=	Homebrew

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

ifeq ($(strip $(CPPFILES)),)
export LD	:=	$(CC)
else
export LD	:=	$(CXX)
endif

export OFILES_SOURCES :=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 		:=	$(OFILES_SOURCES)
export HFILES :=

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS := --smdh=$(CURDIR)/$(TARGET).smdh

.PHONY: all clean
all: $(BUILD) $(DEPSDIR)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(TARGET).cia

else

DESTDIR := $(TOPDIR)

.PHONY: all
all: $(DESTDIR)/$(TARGET).cia

$(DESTDIR)/$(TARGET).elf: $(OFILES)
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(DESTDIR)/$(TARGET).3dsx: $(DESTDIR)/$(TARGET).elf $(_3DSXDEPS)
	@echo creating $(notdir $@)
	@3dsxtool $< $@ $(_3DSXFLAGS)

$(DESTDIR)/$(TARGET).smdh: $(DESTDIR)/$(TARGET).elf
	@echo creating $(notdir $@)
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(APP_ICON) $@

$(DESTDIR)/$(TARGET).cia: $(DESTDIR)/$(TARGET).elf 
