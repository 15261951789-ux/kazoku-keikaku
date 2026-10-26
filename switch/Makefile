#---------------------------------------------------------------------------------
# 家族計画 ～追憶～ Switch Port · Stage 2 引擎 Makefile（SDL2 + SDL2_ttf）
#---------------------------------------------------------------------------------
TARGET		:=	kazoku-keikaku

ifeq ($(strip $(DEVKITPRO)),)
DEVKITPRO	:=	/opt/devkitpro
endif

LIBNX		:=	$(DEVKITPRO)/libnx
DEVKITA64	:=	$(DEVKITPRO)/devkitA64
PORTLIBS	:=	$(DEVKITPRO)/portlibs/switch
CC			:=	$(DEVKITA64)/bin/aarch64-none-elf-gcc
NROTOOL		:=	elf2nro
NACPTOOL	:=	nacptool
ROMFSGEN	:=	build_romfs

APP_TITLE	:=	Kazoku Keikaku - Tsuioku (Switch Port)
APP_AUTHOR	:=	kzn-port
APP_VERSION	:=	0.2.0

ARCH		:=	-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS		:=	-g -Wall -O2 -ffunction-sections $(ARCH) -D__SWITCH__ \
				-I$(LIBNX)/include -I$(PORTLIBS)/include -I$(PORTLIBS)/include/SDL2
LDFLAGS		:=	-specs=$(LIBNX)/switch.specs -g $(ARCH) \
				-L$(LIBNX)/lib -L$(PORTLIBS)/lib
LIBS		:=	-lSDL2_ttf -lfreetype -lSDL2 -lglapi -ldrm_nouveau -lnx -lm

SRCS		:=	src/main.c
OBJS		:=	$(SRCS:.c=.o)
ROMFS_BIN	:=	romfs.bin

all: $(TARGET).nro

$(ROMFS_BIN): romfs/font.ttf romfs/script.txt
	@echo "打包 romfs ..."
	$(ROMFSGEN) romfs $@

$(TARGET).elf: $(OBJS)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).nacp: $(TARGET).elf
	$(NACPTOOL) --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@

$(TARGET).nro: $(TARGET).elf $(TARGET).nacp $(ROMFS_BIN) icon.jpg
	$(NROTOOL) $< $@ --icon=icon.jpg --nacp=$(TARGET).nacp --romfs=$(ROMFS_BIN)

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).nro $(TARGET).nacp $(ROMFS_BIN)

.PHONY: all clean
