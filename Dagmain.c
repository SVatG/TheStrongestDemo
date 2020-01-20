#include "DS3D.h"
#include "Tunnel.h"
#include "Worm.h"
#include "Feedback.h"
#include "ARM.h"
#include "Hardware.h"
#include "nitrofs.h"

#include <nds.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/dir.h>
#include <nds/fifocommon.h>
#include <maxmod9.h>
#include <fat.h>

#include <nds/registers_alt.h>

// Datafiles: Music.
#include "takeiteasy.h"

// Effects: halcy
#include "metaballs.h"
#include "rainbow.h"
#include "rad_blur.h"

// Kludge
#include "gfx/yukkuri_full.png.h"

extern uint8_t rivalschools_font[];
extern uint16_t rivalschools_pal[];

volatile int frame;
uint8_t DTCM_DATA dtcm_buffer[10240];

void vblank() {
	frame++;
}

u8 screen_flip = 0;
void flipLCD() {
	screen_flip ^= 1;
	if( screen_flip ) {
		POWER_CR &= ~POWER_SWAP_LCDS;
	}
	else {
		POWER_CR |= POWER_SWAP_LCDS;
	}
}

// Conclusion: GBATEK is stupid.
// 0 0000 fail, nothing
// 1 0001 fail, nothing
// 2 0010 fail, nothing
// 3 0011 fail, nothing
// 4 0100 brightens
// 5 0101 fail, all white
// 6 0110 brightens
// 7 0111 fail, all white
// 8 1000 darkens
// 9 1001 fail, all black
// A 1010 darkens
// B 1011 fail, all black
// C 1100 darkens
// D 1101 fail, all black
// E 1110 darkens
// F 1111 fail, all black

u16* master_bright = (u16*)(0x400006C);
u16* master_bright_sub = (u16*)(0x400106C);
void fade( s16 fade_val ) {
	memset( master_bright, (1<<7) | fade_val, 2 );
	memset( master_bright_sub, (1<<7) | fade_val, 2 );
}
void fadew( s16 fade_val ) {
	memset( master_bright, (1<<6) | fade_val, 2 );
	memset( master_bright_sub, (1<<6) | fade_val, 2 );
}

int main(int argc,char **argv)
{
//defaultExceptionHandler();
	powerON(POWER_ALL);

// 	videoSetModeSub(MODE_0_2D|DISPLAY_BG0_ACTIVE);
// 
// 	vramSetBankI(VRAM_I_SUB_BG);
// 	SUB_BG0_CR=(2<<2)|(20<<8); // char base 32k, tile base 40k
// //	consoleInitDefault((uint16 *)0x620a000,(uint16 *)0x6208000,16);
// 	consoleInit((uint16 *)rivalschools_font,(uint16 *)0x6208000,128,0,(uint16 *)0x620a000,0,16);
// 	BG_PALETTE_SUB[0]=RGB15(0,0,0);
// 	for(int i=1;i<16;i++) BG_PALETTE_SUB[i]=rivalschools_pal[i];

// 	iprintf("IRQ init\n");
	irqInit();
	irqEnable(IRQ_VBLANK);
	irqSet(IRQ_VBLANK,vblank);

// 	iprintf("FIFO init\n");
	fifoInit();
// 	iprintf("NitroFS init\n");

	if(!nitroFSInitAdv(DEFAULT_FILENAME))
	{
// 		iprintf("NitroFS init failed.\n");
		for(;;);
	}

	ClaimWRAM();

// 	iprintf("Maxmod init\n");
	mmInitDefault( "nitro:/Mods.msl" );
// 	iprintf("Maxmod init worked.\n");
// 	iprintf("Maxmod load.\n");
	mmLoad( MOD_TAKEITEASY3_SMALL );
// 	iprintf("Maxmod load worked.\n");
// 	iprintf("Maxmod play.\n");
	mmStart( MOD_TAKEITEASY3_SMALL, MM_PLAY_LOOP );
// 	iprintf("Maxmod play worked.\n");

	// All dark
	fadew( 16 );

	StartFeedback();
//	StartMetaballs();
//	StartRadBlur();
//  	StartEnding();
//	StartWorm(  "nitro:/test.map", dtcm_buffer );
	u8 mode = 0;
	u32 t = 0;
	
	u8 running = 1;
	int blendval = 0;
	frame = 0;
	while( running ) {
		switch( mode ) {
			case 0:
				swiWaitForVBlank();
				RenderFeedback( t, t > 1530 );
				DSFlush( DS_FLUSH_NO_SORTING );
				t++;

				if( frame <= 31 ) {
					fadew( (31 - frame)>>1 );
				} else if( frame >= 1590 ) {
					fade( frame - 1590 );
				} else {
					fade( 0 );
				}

				if( frame > 1620 ) {
					StopFeedback();
					flipLCD();
					StartWorm( "nitro:/test.map", dtcm_buffer );
					frame = 0;
					t=0;
					mode = 1;
				}
			break;
			case 1:
				swiWaitForVBlank();
				RenderWorm(t);
				DSFlush(0);
				t++;
				t++;
				
				if( frame <= 31 ) {
					fade( (31 - frame)>>1 );
				} else if( frame >= 1800 ) {
					fadew( frame - 1800 );
				} else {
					fade( 0 );
				}
			
				if( frame > 1830 ) {
					StopWorm();
					flipLCD();
					StartRadBlur();
					frame = 0;
					t=0;
					mode = 4;
				}
			break;
			case 2:
				swiWaitForVBlank();
				RenderMetaballs();
				t++;

				if( frame <= 31 ) {
					fadew( (31 - frame)>>1 );
				} else if( frame >= 2640 ) {
					fade( frame - 2640 );
				} else {
					fade( 0 );
				}

				if( frame > 2670 ) {
					StopMetaballs();
					flipLCD();
					StartEnding();
					frame = 0;
					t=0;
					mode = 5;
				}
			break;
			case 3:
				swiWaitForVBlank();
				RenderRainbow( blendval );

				if( frame > 500 && frame <= 575 ) {
					blendval = (frame - 500) / 5;
				}
				if( frame > 575 ) {
					// Pallette bug fix kludge
					if( t == 0 ) {
						videoSetModeSub( MODE_5_2D | DISPLAY_BG3_ACTIVE );
						SUB_BLEND_CR = BLEND_NONE;
						SUB_BG3_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 0 ) | BG_PRIORITY_3;
						SUB_BG3_XDX = (1 << 8);
						SUB_BG3_XDY = 0;
						SUB_BG3_YDX = 0;
						SUB_BG3_YDY = (1 << 8);
						SUB_BG3_CX = 0;
						SUB_BG3_CY = 0;
						u16* sub_screen = (u16*)BG_BMP_RAM_SUB( 0 );
						dmaCopy( yukkuri_full_pngBitmap, sub_screen, 256 * 192 );
						dmaCopy( yukkuri_full_pngPal, BG_PALETTE_SUB, 256 * 2);	
					}
				}

				if( frame <= 31 ) {
					fade( (31 - frame)>>1 );
				} else if( frame >= 1800 ) {
					fadew( frame - 1800 );
				} else {
					fade( 0 );
				}

				if( frame > 1830 ) {
					StopRainbow();
					flipLCD();
					StartMetaballs();
					frame = 0;
					t=0;
					mode = 2;
				}
			break;
			case 4:
				swiWaitForVBlank();
				RenderRadBlur();
				t++;

				if( frame <= 31 ) {
					fadew( (31 - frame)>>1 );
				} else if( frame >= 900 ) {
					fade( frame - 900 );
				} else {
					fade( 0 );
				}

				if( frame > 930 ) {
					StopRadBlur();
					flipLCD();
					StartRainbow();
					frame = 0;
					t=0;
					mode = 3;
					blendval = 0;
				}
			break;
			case 5:
				swiWaitForVBlank();
				RenderEnding();
				t++;

				if( frame <= 31 ) {
					fade( (31 - frame)>>1 );
				} else if( frame >= 1270 ) {
					fadew( frame - 1270 );
				} else {
					fade( 0 );
				}

				if( frame > 1300 ) {
					StopEnding();
					mmStop();
					while( 1 ) {
						swiWaitForVBlank();
					}
					flipLCD();
					frame = 0;
					t=0;
					mode = 0;
				}
			break;
		}
	}

	return 0;
}


