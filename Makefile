BITMAPS=gfx/people.o gfx/backdrop.o gfx/balls.o gfx/credits.o gfx/yukkuri_full.o \
gfx/yukkuri_outline.o gfx/greets.o ending.o

OBJS=Dagmain.o DS3D.o Worm.o Tunnel.o Feedback.o ARM.o Font.o nitrofs.o \
metaballs.o rainbow.o rad_blur.o $(BITMAPS)
OBJS7=Main.arm7.o

LIBS=-L$(DEVKITPRO)/libnds/lib -lmm9 -lfat -lnds9 -lm
LIBS7=-L$(DEVKITPRO)/libnds/lib -lmm7 -lnds7

NAME=Demo1
DEFINES=-DDEFAULT_FILENAME='"$(NAME).nds"'

CC=$(DEVKITARM)/bin/arm-eabi-gcc
AS=$(DEVKITARM)/bin/arm-eabi-as
LD=$(DEVKITARM)/bin/arm-eabi-gcc
CFLAGS=-std=gnu99 -O3 -mcpu=arm9e -mtune=arm9e -fomit-frame-pointer -ffast-math \
-mthumb -mthumb-interwork -I$(DEVKITPRO)/libnds/include -DARM9 $(DEFINES)
CFLAGSARM=-std=gnu99 -O3 -mcpu=arm9e -mtune=arm9e -fomit-frame-pointer -ffast-math \
-mthumb-interwork -I$(DEVKITPRO)/libnds/include -DARM9 $(DEFINES)
CFLAGS7=-std=gnu99 -O3 -mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer -ffast-math \
-mthumb -mthumb-interwork -I$(DEVKITPRO)/libnds/include -DARM7 $(DEFINES)
CFLAGSS=-std=gnu99 -Os -mcpu=arm9e -mtune=arm9e -fomit-frame-pointer -ffast-math \
-mthumb -mthumb-interwork -I$(DEVKITPRO)/libnds/include -DARM9 $(DEFINES)
LDFLAGS=-specs=ds_arm9.specs -mthumb -mthumb-interwork -mno-fpu
LDFLAGS7=-specs=ds_arm7_no32k.specs -mthumb -mthumb-interwork -mno-fpu

.SUFFIXES: .o .png
.png.o :
	$(DEVKITARM)/bin/grit $< -ftc -o$<.c
	$(CC) -c $<.c -o $@

$(NAME).nds: $(NAME).arm9 $(NAME).arm7
	$(DEVKITARM)/bin/ndstool -c $@ -9 $(NAME).arm9 -7 $(NAME).arm7 -r7 0x3800000 -e7 0x3800000 -d Datafiles

$(NAME).arm9: $(NAME).arm9.elf
	$(DEVKITARM)/bin/arm-eabi-objcopy -O binary $< $@

$(NAME).arm7: $(NAME).arm7.elf
	$(DEVKITARM)/bin/arm-eabi-objcopy -O binary $< $@

$(NAME).arm9.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(NAME).arm7.elf: $(OBJS7)
	$(LD) $(LDFLAGS7) -o $@ $(OBJS7) $(LIBS7)

clean:
	rm -f $(NAME).nds $(NAME).arm9 $(NAME).arm7 $(NAME).arm9.elf $(NAME).arm7.elf $(OBJS) $(OBJS7)

install: $(NAME).nds
	cp $(NAME).nds /Volumes/KINGSTON/
	hdiutil eject /Volumes/KINGSTON/

Main.o: Main.c DS3D.h Tunnel.h Worm.h Feedback.h ARM.h nitrofs.h

Main.arm7.o: Main.arm7.c
	$(CC) $(CFLAGS7) -c -o $@ $< 

DS3D.o: DS3D.c DS3D.h

Tunnel.o: Tunnel.c Tunnel.h DS3D.h ARM.h

Worm.o: Worm.c Worm.h DS3D.h ARM.h

Feedback.o: Feedback.c Feedback.h DS3D.h

ARM.o: ARM.c ARM.h
	$(CC) $(CFLAGSARM) -c -o $@ $< 
metaballs.o: metaballs.c metaballs.h
	$(CC) $(CFLAGSS) -c -o $@ $<

Font.o: Font.c

nitrofs.o: nitrofs.c

images: $(BITMAPS)

todisk: $(NAME).nds
	/bin/mkdir disk
	/usr/bin/sudo mount /dev/mmcblk0p1 disk
	/usr/bin/sudo cp $(NAME).nds disk/Homebrew/Development/
	/usr/bin/sudo umount disk
	/bin/rm -rf disk

test: $(NAME).nds
	/usr/bin/wine /home/halcyon/Desktop/src/DS/devkitpro/nocash/NOCASH.EXE $(NAME).nds