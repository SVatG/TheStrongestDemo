// Royal rainbow.

#include <nds.h>

#include "DS3D.h"
#include "Hardware.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "glyphs.h"
#include "gfx/backdrop.png.h"
#include "gfx/yukkuri_full.png.h"
#include "gfx/yukkuri_outline.png.h"

#define WHITE RGB15( 31, 31, 31 )
#define BLACK RGB15( 0, 0, 0 )
#define PINK RGB15( 31, 00, 15 )
#define RED RGB15( 31, 00, 00 )
#define ORANGE RGB15( 31, 15, 00 )
#define YELLOW RGB15( 31, 31, 00 )
#define GREEN RGB15( 0, 31, 0 )
#define BLUE RGB15( 0, 0, 31 )
#define PURPLE RGB15( 31, 0, 31 )

#define sinLerp(x) (sinFixed((x)>>6))
#define cosLerp(x) (cosFixed((x)>>6))

// Rainbow status variables
u16 royal_rainbow[ 7 ] = { PINK, RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE };
u16 rainbow_off[ 7 ] = { 0, 2000, 6000, 13000, 15000, 21000, 29000 };
u8 rainbow_bw[ 7 ] = { 0, 10, 14, 3, 20, 15, 10 };
s16 rainbow_bwinc[ 7 ] = { 200, -200, 200, -200, -200, 200, -200 };
u32 offset = 0;

void rainbow_relayer( u8 index ) {
	u16 color_temp = royal_rainbow[ index ];
	u16 off_temp = rainbow_off[ index ];
	u8 bw_temp = rainbow_bw[ index ];
	s16 bwinc_temp = rainbow_bwinc[ index ];
	for( int i = index; i > 0; i-- ) {
		royal_rainbow[ i ] = royal_rainbow[ i - 1 ];
		rainbow_off[ i ] = rainbow_off[ i - 1 ];
		rainbow_bw[ i ] = rainbow_bw[ i - 1 ];
		rainbow_bwinc[ i ] = rainbow_bwinc[ i - 1 ];
	}
	royal_rainbow[ 0 ] = color_temp;
	rainbow_off[ 0 ] = off_temp;
	rainbow_bw[ 0 ] = bw_temp;
	rainbow_bwinc[ 0 ] = bwinc_temp;
}

u16* main_screen;

void RenderRainbow( u8 blend_val ) {
	// Some layer changes
	for( int c = 0; c < 7; c++ ) {
		if( rainbow_bw[ c ] == 0 ) {
			rainbow_relayer( c );
		}
	}

	// Some action.
	offset = offset + 100;
	u16 offset_sin[ 7 ];
	for( int c = 0; c < 7; c++ ) {
		offset_sin[ c ] = (sinLerp( offset + rainbow_off[ c ] ) >> 6) + 86;
		rainbow_bwinc[ c ]++;
		if( rainbow_bw[ c ] == 20 ) {
			rainbow_bwinc[ c ] = -2000;
		}
		if( rainbow_bw[ c ] == 0 ) {
			rainbow_bwinc[ c ] = 0;
		}
		if( rainbow_bwinc[ c ] % 20 == 0 ) {
			if( rainbow_bwinc[ c ] >= 0 ) {
				rainbow_bw[ c ]++;
			}
			else {
				rainbow_bw[ c ]--;
			}
		}
	}

	BG2_CX = (offset / 40) << 8;
	BG2_XDY = cosLerp(offset/200) >> 2;
	BG2_XDX = -sinLerp(offset/200) >> 2;
	BG2_YDX = sinLerp(offset/200) >> 2;
	BG2_YDY = cosLerp(offset/200) >> 2;

	s8 scrollval = (offset / 20) % 256;
	BG3_CX = scrollval + 8 << 8;
	
	// Royal rainbow
	for( int i = 0; i < 192; i++ ) {
		int ci = i * 256;
		//main_screen[ci+1+offset/40] =
		main_screen[ci+scrollval] =
		main_screen[ci+scrollval+1] =
		main_screen[ci+scrollval+2] =
		main_screen[ci+scrollval+4] =
		main_screen[ci+scrollval+5] =
		main_screen[ci+scrollval+6] =
		main_screen[ci+scrollval+7] =
		main_screen[ci+scrollval+8] =
			BLACK & !BIT(15);
		for( int c = 0; c < 7; c++ ) {
			if( i > offset_sin[ c ] && i < offset_sin[ c ] + rainbow_bw[ c ] ) {
				main_screen[ci+scrollval] =
				main_screen[ci+1+scrollval] =
				main_screen[ci+2+scrollval] =
				main_screen[ci+3+scrollval] =
				main_screen[ci+4+scrollval] =
				main_screen[ci+5+scrollval] =
				main_screen[ci+6+scrollval] =
					royal_rainbow[ c ] | BIT(15);
			}

		}
	}
	SUB_BLEND_AB = (blend_val<<8) | (16-blend_val);
	// dmaCopyWordsAsynch(2, &main_screen[0], &sub_screen[0], 256 * 192 * 2 );
}

void StartRainbow() {
	// Set up main engine for extended rotation mode.
	videoSetMode( MODE_5_2D | DISPLAY_BG1_ACTIVE );
	vramSetBankA( VRAM_A_MAIN_BG );
	BG3_CR = BG_BMP16_256x256 | BG_WRAP_ON | BG_BMP_BASE( 0 ) | BG_PRIORITY_0;
	BG3_XDY = 0;
	BG3_XDX = 1 << 8;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;
	main_screen = (u16*)BG_BMP_RAM(0);

	// Backdrop
	vramSetBankB( VRAM_B_MAIN_BG );
	BG2_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 8 ) | BG_PRIORITY_3;
	BG2_XDY = 0;
	BG2_XDX = 1 << 8;
	BG2_YDX = 0;
	BG2_YDY = 1 << 8;
	u16* main_screen_bd = (u16*)BG_BMP_RAM( 8 );
	dmaCopy( backdrop_pngBitmap, main_screen_bd, 256 * 256 );

	// Set up sub engine for extended rotation mode.
	videoSetModeSub( MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE );
	vramSetBankC( VRAM_C_SUB_BG );
	vramSetBankH( VRAM_H_SUB_BG );
	vramSetBankI( VRAM_I_SUB_BG );

	// A nice blend
	SUB_BLEND_CR = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG3;
	SUB_BLEND_AB = (0x0F << 8) | (0x0F);

	// Full yukkuri
	SUB_BG3_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 0 ) | BG_PRIORITY_3;
	SUB_BG3_XDX = (1 << 8);
	SUB_BG3_XDY = 0;
	SUB_BG3_YDX = 0;
	SUB_BG3_YDY = (1 << 8);
	SUB_BG3_CX = 0;
	SUB_BG3_CY = 0;
	u16* sub_screen = (u16*)BG_BMP_RAM_SUB( 0 );
	dmaCopy( yukkuri_full_pngBitmap, sub_screen, 256 * 192 );

	// Outline yukkuri
	SUB_BG2_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 4 ) | BG_PRIORITY_0;
	SUB_BG2_XDX = (1 << 8);
	SUB_BG2_XDY = 0;
	SUB_BG2_YDX = 0;
	SUB_BG2_YDY = (1 << 8);
	SUB_BG2_CX = 0;
	SUB_BG2_CY = 0;
	sub_screen = (u16*)BG_BMP_RAM_SUB( 4 );
	dmaCopy( yukkuri_outline_pngBitmap, sub_screen, 256 * 192 );
	dmaCopy( yukkuri_full_pngPal, BG_PALETTE_SUB, 256 * 2);

	// Initial screen
	for( int i = 0; i < 256; i++ ) {
		RenderRainbow( 0 );
	}

	// Copy palette only here, because.
	dmaCopy( backdrop_pngPal, BG_PALETTE, 256 * 2);
	// dmaCopy( main_screen, sub_screen, 256 * 192 * 2 );
	
	// Main screen turn on
	videoSetMode( MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG3_ACTIVE );
}

// Resets CX/CY et cetera
void StopRainbow() {
	SUB_BG3_XDX = 0;
	SUB_BG3_XDY = 0;
	SUB_BG3_YDX = 0;
	SUB_BG3_YDY = 0;
	SUB_BG3_CX = 0;
	SUB_BG3_CY = 0;
	
	BG2_XDY = 0;
	BG2_XDX = 0;
	BG2_YDX = 0;
	BG2_YDY = 0;
	BG2_CX = 0;
	
	s8 scrollval = 0;
	BG3_CX = 0;

// 	SUB_BLEND_CR = BLEND_NONE;
}
