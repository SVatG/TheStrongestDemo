// Pretty radial blur

#include <nds.h>

#include "DS3D.h"
#include "Hardware.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

u32 ptexture;
u32 rtexture;

#include "gfx/people.png.h"
#include "gfx/credits.png.h"
#include "gfx/balls.png.h"

void box( float x, float y, float z, float width, float height, float depth ) {
	DSBegin(DS_QUADS);
		DSColor3f( 1, 1, 1 );
		glBindTexture( 0, ptexture );
		DSTexCoord2f( 0, 127 ); DSVertex3f( x , y, z );
		DSTexCoord2f( 127, 127 ); DSVertex3f( x + width, y, z );
		DSTexCoord2f( 127, 0 ); DSVertex3f( x + width, y + height, z );
		DSTexCoord2f( 0, 0 ); DSVertex3f( x, y + height, z );

		DSTexCoord2f( 256, 256 ); DSVertex3f( x, y , z + depth );
		DSTexCoord2f( 256, 128 ); DSVertex3f( x, y + height, z + depth );
		DSTexCoord2f( 128, 128 ); DSVertex3f( x + width, y + height, z + depth );
		DSTexCoord2f( 128, 256 ); DSVertex3f( x + width, y, z + depth );
		DSSetTexture( 0 );
		
		// Orange
		DSColor3f( 1, 0.5, 0 );
		DSVertex3f( x, y , z );
		DSVertex3f( x, y + height, z );
		DSVertex3f( x, y + height, z + depth );
		DSVertex3f( x, y, z + depth );

		// Blue
		DSColor3f( 0, 0, 1 );
		DSVertex3f( x + width, y , z );
		DSVertex3f( x + width, y , z + depth);
		DSVertex3f( x + width, y + height, z + depth);
		DSVertex3f( x + width, y + height, z );

		DSColor3f( 1, 1, 1 );
		glBindTexture( 0, ptexture );
		DSTexCoord2f( 0, 128); DSVertex3f( x, y, z );
		DSTexCoord2f( 128, 128); DSVertex3f( x, y, z + depth );
		DSTexCoord2f( 128, 256); DSVertex3f( x + width, y, z + depth );
		DSTexCoord2f( 0, 256); DSVertex3f( x + width, y, z );

		DSTexCoord2f( 128, 0 ); DSVertex3f( x, y + height, z );
		DSTexCoord2f( 128, 128 ); DSVertex3f( x + width, y + height, z );
		DSTexCoord2f( 256, 128 ); DSVertex3f( x + width, y + height, z + depth );
		DSTexCoord2f( 256, 0 ); DSVertex3f( x, y + height, z + depth );
		DSSetTexture( 0 );
	DSEnd();
}

void StartRadBlur() {
	// Bonus kludge
	videoSetMode( MODE_5_2D | DISPLAY_BG0_ACTIVE );
	vramSetBankA( VRAM_A_MAIN_BG );
	
	videoSetMode( MODE_1_3D | DISPLAY_BG0_ACTIVE );
	
	glInit();
	DSInit3D();
	
	DSSetControl( DS_TEXTURING | DS_ANTIALIAS | DS_ALPHA_BLEND );
	DSClearParams( 0, 0, 0, 31, 63 );
	DSClearDepth( 0x7FFF );
	DSViewport( 0, 0, 255, 191 );

	DSMatrixMode( DS_PROJECTION );
	DSLoadIdentity();
	DSOrtho( -4, 4, -3, 3, 0.1, 10 );
	DSPolyFormat( DS_POLY_ALPHA( 31 ) | DS_POLY_CULL_NONE );

	// Faces
	vramSetBankB( VRAM_B_TEXTURE_SLOT1 );

	glGenTextures( 1, &ptexture );
	glBindTexture( 0, ptexture );
	glTexImage2D( 0, 0, GL_RGB, TEXTURE_SIZE_256 , TEXTURE_SIZE_256, 0, TEXGEN_TEXCOORD, (u8*)people_pngBitmap );
	
	// Subengine ready go
	vramSetBankC( VRAM_C_SUB_BG );
	SUB_BG3_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 0 );
	SUB_BG3_XDY = 0;
	SUB_BG3_XDX = 1 << 8;
	SUB_BG3_YDX = 0;
	SUB_BG3_YDY = 1 << 8;
	SUB_BG3_CX = 0;
	SUB_BG3_CY = 0;
	u16* sub_screen = (u16*)BG_BMP_RAM_SUB( 0 );
	dmaCopy( credits_pngBitmap, sub_screen, 256 * 192 );
	dmaCopy( credits_pngPal, BG_PALETTE_SUB, 256 * 2 );
	videoSetModeSub( MODE_3_2D  | DISPLAY_BG3_ACTIVE );
}

// Status variables.
u32 rrot = 0;
u8 flip = 0;
	
void RenderRadBlur() {
	DSMatrixMode( DS_MODELVIEW );
	DSLoadIdentity();

	rrot += 100;
	DSTranslatef( 0, 0, -5 );
	DSRotateYi( rrot >> 6 );
	DSRotateXi( rrot >> 5 );
	box( -1, -1, -1, 2, 2, 2 );
		
	box( -2, -2, -2, 1, 1, 1 );
	box( -2, -2, 1, 1, 1, 1 );
	box( -2, 1, -2, 1, 1, 1 );
	box( -2, 1, 1, 1, 1, 1 );
	box( 1, -2, -2, 1, 1, 1 );
	box( 1, -2, 1, 1, 1, 1 );
	box( 1, 1, -2, 1, 1, 1 );
	box( 1, 1, 1, 1, 1, 1 );
	if( flip ) {
		vramSetBankA( VRAM_A_TEXTURE_SLOT2 );
		vramSetBankD( VRAM_D_LCD );
		REG_DISPCAPCNT =
			DCAP_BANK( 3 ) |
			DCAP_SIZE( 3 ) |
			DCAP_SRC( 1 ) |
			DCAP_MODE( 0 ) |
			DCAP_ENABLE;
		rtexture =
			DS_TEX_ADDRESS( VRAM_D ) |
			DS_TEX_SIZE_S_256 |
			DS_TEX_SIZE_T_256 |
			DS_TEX_FORMAT_RGB |
			DS_TEX_GEN_TEXCOORD;
	}
	else {
		vramSetBankA( VRAM_A_LCD );
		vramSetBankD( VRAM_D_TEXTURE_SLOT3 );
		REG_DISPCAPCNT =
			DCAP_BANK( 0 ) |
			DCAP_SIZE( 3 ) |
			DCAP_SRC( 1 ) |
			DCAP_MODE( 0 ) |
			DCAP_ENABLE;
			
		rtexture =
			DS_TEX_ADDRESS( VRAM_C ) | // It's actually VRAM A at slot 2.
			DS_TEX_SIZE_S_256 |
			DS_TEX_SIZE_T_256 |
			DS_TEX_FORMAT_RGB |
			DS_TEX_GEN_TEXCOORD;
	}
	flip^=1;

	DSColor3f( 31, 31, 31 );
	DSSetTexture( rtexture );
		
	DSMatrixMode( DS_MODELVIEW );
	DSLoadIdentity();

	DSPolyFormat( DS_POLY_ALPHA( 27 ) | DS_POLY_CULL_FRONT );
	DSBegin( DS_QUADS );
		DSTexCoord2f( 10, 192 ); DSVertex3f( -4, -3, -5 );
		DSTexCoord2f( 10, 10 ); DSVertex3f( -4, 3, -5 );
		DSTexCoord2f( 248, 10 ); DSVertex3f( 4, 3, -5 );
		DSTexCoord2f( 248, 192 ); DSVertex3f( 4, -4, -5 );
	DSEnd();
	DSPolyFormat( DS_POLY_ALPHA( 31 ) | DS_POLY_CULL_FRONT );

	DSSetTexture( 0 );

	DSFlush( 0 );

	dmaCopyWordsAsynch
}

void StopRadBlur() {
	REG_DISPCAPCNT = 0;
}
