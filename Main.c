#include <nds.h>
#include <nds/fifocommon.h>
#include <stdio.h>
#include <string.h>
#include <nds/arm9/trig_lut.h>
#include <nds/registers_alt.h>
#include <maxmod9.h>

#include "gfx/backdrop.png.h"

#include "DS3D.h"

// #include "glyphs.h"
// #include "metaballs.h"
// #include "rainbow.h"
// #include "rad_blur.h"

#include "Worm.h"
#include "Feedback.h"

#include "nitrofs.h"
#include "ARM.h"

#include "amigadisco2.h"

uint8_t DTCM_DATA dtcm_buffer[10240];
// uint8_t* dtcm_buffer = (uint8_t*)(0x027C0000);

volatile u32 frame;
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

// Good kludge

// int32_t sinFixed(int a) { return sinLerp(a<<6); }

int main(void){
	powerON(POWER_ALL);

//	consoleDemoInit();

// 	iprintf ("IRQ init\n" );
	
// 	irqInit();
// 	irqEnable( IRQ_ALL );

	irqSet( IRQ_VBLANK, vblank );
	
// 	iprintf ("IRQ GET, FIFO init\n" );
// 	fifoInit();
// 	iprintf ("FIFO GET\n" );
	
	s8 mode = 4;
	
// 	iprintf("NitroFS init\n");

	if(!nitroFSInitAdv(DEFAULT_FILENAME))
	{
// 		iprintf("NitroFS init failed.\n");
		for(;;);
	}

// 	iprintf ("claiming WRAM\n" );
	
// 	iprintf ("WRAM get\n" );

// 	mm_ds_system sys;
// 	sys.mod_count = MSL_NSONGS;
// 	sys.samp_count= MSL_NSAMPS;
// 	sys.mem_bank = malloc( MSL_BANKSIZE * 4 );
// 	sys.fifo_channel = FIFO_MAXMOD;
// 	mmInit( &sys );
// 	mmSoundBankInFiles( "nitro:/sounds.bin" );
	
// 	mmInitDefault( "nitro:/Mods.msl" );
// 	mmLoad( MOD_AMIGADISCO2 );
// 	mmStart( MOD_AMIGADISCO2, MM_PLAY_LOOP );


// iprintf ("Worm get\n" );
	

// iprintf ("Doo get\n" );
// 	for(int t=0;;t++) {
// 		swiWaitForVBlank();
// 		RenderWorm( t );
// 		DSFlush(0);
// 	}


// 	BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
// 	BLEND_Y = 0x1F;

	// StartMetaballs();

	frame = 0;
	u32 ccount;
	ClaimWRAM();
	StartWorm( "nitro:/test.map", dtcm_buffer );

	while( 1 ) {
		swiWaitForVBlank();
		RenderWorm( ccount );
		DSFlush(0);
		ccount++;
	}
	
	ccount = 0;
	for(int t=0;;t++) {
		switch( mode ) {
			case 0:
				if( frame <= 31 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = 31 - frame;
				} else if( frame >= 770 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = frame - 770;
				} else {
					BLEND_CR = BLEND_NONE;
				}
				
				swiWaitForVBlank();
				RenderMetaballs();
				
				if( frame >= 800 ) {
					flipLCD();
					StopMetaballs();
					StartRainbow();
					frame = 0;
					mode = 1;
				}
			break;
			case 1:
				if( frame <= 31 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = 31 - frame;
				} else if( frame >= 770 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = frame - 770;
				} else {
					BLEND_CR = BLEND_NONE;
				}
				
				swiWaitForVBlank();
				RenderRainbow();
				
				if( frame >= 800 ) {
					flipLCD();
					StopRainbow();
					StartRadBlur();
					frame = 0;
					mode = 2;
				}
			break;
			case 2:
				if( frame <= 31 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = 31 - frame;
				} else if( frame >= 770 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = frame - 770;
				} else {
					BLEND_CR = BLEND_NONE;
				}
				
				RenderRadBlur();
				swiWaitForVBlank();
				
				if( frame >= 800 ) {
					flipLCD();
					ccount = 0;
					StopRadBlur();
					StartFeedback();
					frame = 0;
					mode = 3;
				}
			break;
			case 3:
				if( frame <= 31 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = 31 - frame;
				} else if( frame >= 770 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = frame - 770;
				} else {
					BLEND_CR = BLEND_NONE;
				}
				
				swiWaitForVBlank();
				RenderFeedback( ccount, frame > 740 );
				DSFlush(DS_FLUSH_NO_SORTING);
				ccount++;
				
				if( frame >= 800 ) {
					flipLCD();
					StopFeedback();
					StartWorm( "nitro:/test.map", dtcm_buffer );
					ccount = 0;
					frame = 0;
					mode = 4;
				}
			break;
			case 4:
				if( frame <= 31 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = 31 - frame;
				} else if( frame >= 770 ) {
					BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG1 | BLEND_SRC_BG2 | BLEND_SRC_BG3;
					BLEND_Y = frame - 770;
				} else {
					BLEND_CR = BLEND_NONE;
				}
				
				swiWaitForVBlank();
				RenderWorm( ccount );
				DSFlush(0);
				ccount++;
				
				if( frame >= 800 ) {
					flipLCD();
					StopWorm();
					StartMetaballs();
					frame = 0;
					mode = 0;
				}
			break;
		}
	}
 
	return 0;
}
