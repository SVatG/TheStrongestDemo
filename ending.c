// Royal rainbow.

#include <nds.h>

#include "DS3D.h"
#include "Hardware.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

void RenderEnding() {
	// It's a static image, so uh... do nothing.
}

void StartEnding() {
	// Load shit into VRAM.
	int fd=open("nitro:/outro_1.img.bin",O_RDONLY);
	if(fd<0)
	{
		iprintf("Failed to load texture\n");
		for(;;);
	}
	VRAMCNT_C = VRAMCNT_C_LCDC;
	read(fd,VRAM_LCDC_C,256*192*2);
	close(fd);
	vramSetBankC( VRAM_C_SUB_BG );

	// Set up subengine to just display.
	videoSetModeSub( MODE_5_2D | DISPLAY_BG2_ACTIVE  );
	SUB_BG2_CR = BG_BMP16_256x256 | BG_WRAP_ON | BG_BMP_BASE( 0 );
	SUB_BG2_XDX = (1 << 8);
	SUB_BG2_XDY = 0;
	SUB_BG2_YDX = 0;
	SUB_BG2_YDY = (1 << 8);
	SUB_BG2_CX = 0;
	SUB_BG2_CY = 0;
	u16* sub_screen = (u16*)BG_BMP_RAM_SUB( 0 );

		fd=open("nitro:/outro_2.img.bin",O_RDONLY);
	if(fd<0)
	{
		iprintf("Failed to load texture\n");
		for(;;);
	}

	VRAMCNT_D=VRAMCNT_D_LCDC;
	read(fd,VRAM_LCDC_D,256*192*2);
	close(fd);

	VRAMCNT_A=VRAMCNT_A_LCDC;
	VRAMCNT_B=VRAMCNT_B_LCDC;
	VRAMCNT_D=VRAMCNT_D_BG_VRAM_A_OFFS_0K;
	// VRAMCNT_C=VRAMCNT_C_LCDC;

	DISPCNT_A=DISPCNT_MODE_4|DISPCNT_3D|DISPCNT_BG0_ON|DISPCNT_BG3_ON|DISPCNT_ON;
	BG0CNT_A=BGxCNT_PRIORITY_2;
	BG3CNT_A=BGxCNT_BITMAP_BASE_0K|BGxCNT_EXTENDED_BITMAP_16
			|BGxCNT_BITMAP_SIZE_256x256|BGxCNT_PRIORITY_1; // RGB bitmap mode
	BG3PA_A=0x100;
	BG3PB_A=0;
	BG3PC_A=0;
	BG3PD_A=0x100;
	BG3HOFS_A=0;
	BG3VOFS_A=0;
}

// Resets CX/CY et cetera
void StopEnding() {
	
}
