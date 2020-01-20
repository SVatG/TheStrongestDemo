// Pretty meta-balls.

#include <nds.h>

#include "DS3D.h"
#include "Hardware.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "gfx/balls.png.h"
#include "marching_cubes.h"
#include "gfx/greets.png.h"

#define SWITCH(x,y) x=((x)^(y));y=((x)^(y));x=((x)^(y));
#define abs(x) ((x)<0)?(-(x)):(x)

// A single metaball call.
// function:
// f(x,y,z) = 1 / ((x − x0)^2 + (y − y0)^2 + (z − z0)^2)
// It's slow, so we're going to need clever tricks.
// IDEA: cache field strengths for metaballs of one radius, then just copy
// memory for position changes and somehow interpolate.
static inline s32 metaball( s32 x, s32 y, s32 z, s32 cx, s32 cy, s32 cz ) {
	s32 dx = abs(x - cx);
	s32 dy = abs(y - dy);
	s32 dz = abs(z - dz);
	s32 xs = mulf32( dx, dx );
	s32 ys = mulf32( dy, dy );
	s32 zs = mulf32( dz, dz );
	s32 rsq = (xs + ys + zs);
	return( divf32( 1<<12, rsq ) );
}

// Nice precalculated grid for the balls.
PRECELL pre_grid[30][30][30];

// Polygon / isolevel storage
TRIANGLE triangles[1200];

// Get a single grid cell by looking up shit in the precalc'd table
static inline void cell_at( GRIDCELL* b, s32 x, s32 y, s32 z, s32 cx, s32 cy, s32 cz, u8 inf ) {
	s16 accx;
	s16 accy;
	s16 accz;
	
	accx = abs( x - cx );
	accy = abs( y - cy );
	accz = abs( z - cz );

	memcpy( &b->p, &pre_grid[ x ][ y ][ z ].p[ 0 ], 8 * sizeof( XYZ ) );
	memcpy( &b->val, &pre_grid[ accx ][ accy ][ accz ].val[ 0 ], 8 * sizeof( u16 ) );
	if( (x - cx) < 0 ) {
		SWITCH( b->val[ 0 ],  b->val[ 1 ] );
		SWITCH( b->val[ 4 ],  b->val[ 5 ] );
		SWITCH( b->val[ 7 ],  b->val[ 6 ] );
		SWITCH( b->val[ 3 ],  b->val[ 2 ] );
	}
	if( (y - cy) < 0 ) {
		SWITCH( b->val[ 0 ],  b->val[ 4 ] );
		SWITCH( b->val[ 1 ],  b->val[ 5 ] );
		SWITCH( b->val[ 3 ],  b->val[ 7 ] );
		SWITCH( b->val[ 2 ],  b->val[ 6 ] );
	}
		if( (z - cz) < 0 ) {
		SWITCH( b->val[ 1 ],  b->val[ 2 ] );
		SWITCH( b->val[ 0 ],  b->val[ 3 ] );
		SWITCH( b->val[ 5 ],  b->val[ 6 ] );
		SWITCH( b->val[ 4 ],  b->val[ 7 ] );
	}

	for( u8 i = 0; i < 8; i++ ) {
		b->p[ i ].inf[inf] = b->val[ i ];
	}
}

// Same, but add to a previously created cell.
static inline void cell_add( GRIDCELL* b, s32 x, s32 y, s32 z, s32 cx, s32 cy, s32 cz, u8 inf ) {
	GRIDCELL c;
	
	s16 accx = abs( x - cx );
	s16 accy = abs( y - cy );
	s16 accz = abs( z - cz );

	memcpy( &c.val, &pre_grid[ accx ][ accy ][ accz ].val[ 0 ], 8 * sizeof( u16 ) );
	if( (x - cx) < 0 ) {
		SWITCH( c.val[ 0 ],  c.val[ 1 ] );
		SWITCH( c.val[ 4 ],  c.val[ 5 ] );
		SWITCH( c.val[ 7 ],  c.val[ 6 ] );
		SWITCH( c.val[ 3 ],  c.val[ 2 ] );
	}
	if( (y - cy) < 0 ) {
		SWITCH( c.val[ 0 ],  c.val[ 4 ] );
		SWITCH( c.val[ 1 ],  c.val[ 5 ] );
		SWITCH( c.val[ 3 ],  c.val[ 7 ] );
		SWITCH( c.val[ 2 ],  c.val[ 6 ] );
	}
		if( (z - cz) < 0 ) {
		SWITCH( c.val[ 1 ],  c.val[ 2 ] );
		SWITCH( c.val[ 0 ],  c.val[ 3 ] );
		SWITCH( c.val[ 5 ],  c.val[ 6 ] );
		SWITCH( c.val[ 4 ],  c.val[ 7 ] );
	}

	for( int i = 0; i < 8; i++ ) {
		b->val[ i ] += c.val[ i ];
		b->p[ i ].inf[inf] = c.val[ i ];
	}
}

// mah balls.
extern uint8_t rivalschools_font[];
extern uint16_t rivalschools_pal[];

void StartMetaballs() {
	videoSetModeSub( MODE_3_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE );


	vramSetBankC( VRAM_C_SUB_BG );
	vramSetBankH( VRAM_H_LCD );
	vramSetBankI( VRAM_I_LCD );

	SUB_BG0_CR = BG_TILE_BASE( 0 ) | BG_MAP_BASE( 2 ) | BG_PRIORITY_0;
	SUB_BG3_CR = BG_PRIORITY_3;

	consoleInit(
		(uint16 *)rivalschools_font,
		(uint16 *)BG_TILE_RAM_SUB( 0 ),
		128,
		0,
		(uint16 *)BG_MAP_RAM_SUB( 2 ),
		0,
		16
	);

	// Greets bg
	SUB_BG3_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 5 ) | BG_PRIORITY_3;
	SUB_BG3_XDX = (1 << 8);
	SUB_BG3_XDY = 0;
	SUB_BG3_YDX = 0;
	SUB_BG3_YDY = (1 << 8);
	SUB_BG3_CX = 0;
	SUB_BG3_CY = 0;
	u16* sub_screen = (u16*)BG_BMP_RAM_SUB( 5 );
	dmaCopy( greets_pngBitmap, sub_screen, 256 * 192 );
	dmaCopy( greets_pngPal, BG_PALETTE_SUB, 256 * 2);
	
	BG_PALETTE_SUB[ 0 ] = RGB15( 0, 0, 0 );
 	for( int i = 1; i < 16; i++ ) {
 		BG_PALETTE_SUB[ i ] = rivalschools_pal[ i ];
 	}

 	// Init text
	iprintf( "\n The following people possess a\n  sufficient amount of balls:\n\n\n\n" );
	
	videoSetMode( MODE_3_3D | DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE );

	vramSetBankB( VRAM_B_MAIN_BG );
	BG3_CR = BG_BMP8_256x256 | BG_WRAP_ON | BG_BMP_BASE( 8 ) | BG_PRIORITY_3;
	BG3_XDY = 0;
	BG3_XDX = 1 << 8;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;
	BG3_CX = 0;
	BG3_CY = 0;
	u16* main_screen_bd = (u16*)BG_BMP_RAM( 8 );
	dmaCopy( balls_pngBitmap, main_screen_bd, 256 * 192 );
	dmaCopy( balls_pngPal, BG_PALETTE, 256 * 2);

	glInit();
	
	DSSetControl( DS_TEXTURING | DS_ANTIALIAS | DS_OUTLINE  );
	
	DSClearParams( 0, 0, 0, 0, 63 );
	DSClearDepth( 0x7FFF );
	DSViewport( 0, 0, 255, 191 );

	// Materials.
	glMaterialf( GL_AMBIENT, RGB15( 8, 8, 8 ) );
	glMaterialf( GL_DIFFUSE, RGB15( 24, 24, 24 ) );
	glMaterialf( GL_SPECULAR, RGB15( 0, 0, 0 ) );
	glMaterialf( GL_EMISSION, RGB15( 0, 0, 0 ) );

	DSMatrixMode( DS_PROJECTION );
	DSLoadIdentity();
	DSOrtho( -2, 2, -1.5, 1.5, -10, 10 );
	DSPolyFormat( POLY_ALPHA(31) | POLY_CULL_NONE );

// 	DSLight3f( 0, RGB15(0,0,31) , 0, -1.0, 0 );
// 	DSLight3f( 1, RGB15(31,0,0), -1.0, 0, 0 );

// 	glSetToonTableRange( 0, 15, RGB15( 31, 0, 0 ) );
// 	glSetToonTableRange( 16, 31, RGB15( 24, 24, 24 ) );

	// Set up vertex position change variables.
	u32 cell_width = divf32( 2 << 12, 20 << 12 );
	u32 cell_size = mulf32( cell_width, 2 << 12 );

	// Precalculate field strengths.
	s32 tx = 0;
	s32 ty = 0;
	s32 tz = 0;
	for( int x = 0; x < 30; x++ ) {
		ty = 0;
		for( int y = 0; y < 30; y++ ) {
			tz = 0;
			for( int z = 0; z < 30; z++ ) {
				pre_grid[ x ][ y ][ z ].p[ 0 ].x = tx - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 0 ].y = ty - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 0 ].z = tz + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 1 ].x = tx + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 1 ].y = ty - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 1 ].z = tz + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 2 ].x = tx + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 2 ].y = ty - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 2 ].z = tz - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 3 ].x = tx - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 3 ].y = ty - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 3 ].z = tz - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 4 ].x = tx - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 4 ].y = ty + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 4 ].z = tz + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 5 ].x = tx + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 5 ].y = ty + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 5 ].z = tz + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 6 ].x = tx + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 6 ].y = ty + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 6 ].z = tz - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 7 ].x = tx - cell_width;
				pre_grid[ x ][ y ][ z ].p[ 7 ].y = ty + cell_width;
				pre_grid[ x ][ y ][ z ].p[ 7 ].z = tz - cell_width;

				for( int i = 0; i < 8; i++ ) {
					pre_grid[ x ][ y ][ z ].val[ i ] = metaball(
						pre_grid[ x ][ y ][ z ].p[ i ].x,
						pre_grid[ x ][ y ][ z ].p[ i ].y,
						pre_grid[ x ][ y ][ z ].p[ i ].z,
					0, 0, 0);
				}
				
				tz += cell_size;
			}
			ty += cell_size;
		}
		tx += cell_size;
	}
	
	copyTables();
}

s16 g_tris = 0;
s8 mark_grid[30][30][30];
// static inline void ball_recurse( s32 x, s32 y, s32 z, s32 xpos, s32 ypos, s32 zpos, TRIANGLE* tris ) {
// 
// 	// Been here before, eh.
// 	if( mark_grid[ x ][ y ][ z ] == 1 ) {
// 		return;
// 	}
// 	mark_grid[ x ][ y ][ z ] = 1;
// 
// 	iprintf( "%d, %d, %d, %d\n", x, y, z, g_tris );
// 	
// 	GRIDCELL b;
// 	cell_at( &b, x, y, z, xpos, ypos, zpos );
// 	cell_add( &b, x, y, z, 10, 10, 10 );
// 	s8 tri_add = polygonise( &b, 1 << 14, &tris[ g_tris ] );
// 	if( tri_add != 0 ) {
// 		// Commit triangles.
// 		g_tris += tri_add;
// 		
// 		// Recurse!
// 		ball_recurse( x + 1, y, z, xpos, ypos, zpos, tris );
// 		ball_recurse( x - 1, y, z, xpos, ypos, zpos, tris );
// 		ball_recurse( x, y + 1, z, xpos, ypos, zpos, tris );
// 		ball_recurse( x, y - 1, z, xpos, ypos, zpos, tris );
// 		ball_recurse( x, y, z + 1, xpos, ypos, zpos, tris );
// 		ball_recurse( x, y, z - 1, xpos, ypos, zpos, tris );
// 	}
// 	return;
// }

// Let's recurse but not!
s16 pc[400][3];
s16 cp = 0;

inline void step_to_pc( s32 x, s32 y, s32 z ) {
	pc[ cp + 1 ][ 0 ] = x + 1;
	pc[ cp + 1 ][ 1 ] = y;
	pc[ cp + 1 ][ 2 ] = z;
	
	pc[ cp + 2 ][ 0 ] = x - 1;
	pc[ cp + 2 ][ 1 ] = y;
	pc[ cp + 2 ][ 2 ] = z;
	
	pc[ cp + 3 ][ 0 ] = x;
	pc[ cp + 3 ][ 1 ] = y + 1;
	pc[ cp + 3 ][ 2 ] = z;
	
	pc[ cp + 4 ][ 0 ] = x;
	pc[ cp + 4 ][ 1 ] = y - 1;
	pc[ cp + 4 ][ 2 ] = z;

	pc[ cp + 5 ][ 0 ] = x;
	pc[ cp + 5 ][ 1 ] = y;
	pc[ cp + 5 ][ 2 ] = z + 1;
	
	pc[ cp + 6 ][ 0 ] = x;
	pc[ cp + 6 ][ 1 ] = y;
	pc[ cp + 6 ][ 2 ] = z - 1;

	cp += 6;
}

u32 balls( s32* x, s32* y, s32* z, s8 ball_count, TRIANGLE* tris ) {
	g_tris = 0;

	// Reset status
	memset( mark_grid, 0, 30 * 30 * 30 * sizeof( u8 ) );

	for( s8 c = 0; c < ball_count; c++ ) {
		s32 cx = x[ c ];
		s32 cy = y[ c ];
		s32 cz = z[ c ];
		
		// Find first cell
		GRIDCELL b;
		s8 tri_add = 0;
		
		while( tri_add == 0 ) {
			cell_at( &b, cx, cy, cz, x[ 0 ], y[ 0 ], z[ 0 ], c );
			for( u8 i = 1; i < ball_count; i++ ) {
				cell_add( &b, cx, cy, cz, x[ i ], y[ i ], z[ i ], i );
			}
			tri_add = polygonise( &b, 1 << 14, &tris[ g_tris ] );
			cx++;
		}
		
		// Had that one already.
		if( mark_grid[ cx ][ cy ][ cz ] == 1 ) {
			continue;
		}

		// Commit triangles.
		g_tris += tri_add;
		
		// Found triangle, now recurse.
		mark_grid[ cx ][ cy ][ cz ] = 1;

		// Push first step.
		cp = 0;
		pc[ cp ][ 0 ] = cx;
		pc[ cp ][ 1 ] = cy;
		pc[ cp ][ 2 ] = cz;
		step_to_pc( pc[ cp ][ 0 ], pc[ cp ][ 1 ], pc[ cp ][ 2 ] );

		do {
			// Pop one step.
			cp--;
			
			// Visitation check.
			if( mark_grid[pc[ cp ][ 0 ]][pc[ cp ][ 1 ]][pc[ cp ][ 2 ]] == 1 ) {
				continue;
			}
			mark_grid[pc[ cp ][ 0 ]][pc[ cp ][ 1 ]][pc[ cp ][ 2 ]] = 1;

			// Tri check.
			cell_at( &b, pc[cp][0], pc[cp][1], pc[cp][2], x[ 0 ], y[ 0 ], z[ 0 ], 0 );
			for( u8 i = 1; i < ball_count; i++ ) {
				cell_add( &b, pc[cp][0], pc[cp][1], pc[cp][2], x[ i ], y[ i ], z[ i ], i );
			}
			tri_add = polygonise( &b, 1 << 14, &tris[ g_tris ] );

			if( tri_add != 0 ) {
				g_tris += tri_add;
				step_to_pc( pc[ cp ][ 0 ], pc[ cp ][ 1 ], pc[ cp ][ 2 ] );
			}
			
		} while( cp != 0 );
		
	// 	ball_recurse( cx + 1, cy, cz, x, y, z, tris );
	// 	ball_recurse( cx - 1, cy, cz, x, y, z, tris );
	// 	ball_recurse( cx, cy + 1, cz, x, y, z, tris );
	// 	ball_recurse( cx, cy - 1, cz, x, y, z, tris );
	// 	ball_recurse( cx, cy, cz + 1, x, y, z, tris );
	// 	ball_recurse( cx, cy, cz - 1, x, y, z, tris );
	}
	
	return( g_tris );
}

// Position variables
u32 rot = 0;
s16 xpos = 8;
s16 ypos = 10;
s16 zpos = 4;
s8 xdir = 1;
s8 ydir = 1;
s8 zdir = 1;

s16 xpos2 = 4;
s16 ypos2 = 3;
s16 zpos2 = 3;
s8 xdir2 = 1;
s8 ydir2 = 1;
s8 zdir2 = 1;

u16 cpos = 0;
#define T_SPACE "                                    "
#define H_SPACE "                     "
#define DEKA_1 "                 #####                              "
#define DEKA_2 "           #   # #   # ####                   ##### "
#define DEKA_3 "      #### #  #  ##### #   # ####       ##### #     "
#define DEKA_4 "####  #    ###   #   # #   # #    ##  # #     ###   "
#define DEKA_5 "#   # ###  #  #  #   # #   # ###  # # # #     #     "
#define DEKA_6 "#   # #    #   # #   # #   # #    # # # #     #     "
#define DEKA_7 "####  #### #   # #   # ####  #### #  ## ##### ##### "
#define DEKA_8 "                                                    "
#define ALPHA_1 "                  #   #                                           "
#define ALPHA_2 "            ##### #   # #####     ####                      ##  # "
#define ALPHA_3 "      #     #   # #   # #   #     #   # #####         ##### # # # "
#define ALPHA_4 "##### #     ##### ##### #####     #   # #     ##### # #     # # # "
#define ALPHA_5 "#   # #     #     #   # #   #     #   # ####  #     # #  ## # # # "
#define ALPHA_6 "##### #     #     #   # #   #     #   # #     ##### # #   # # # # "
#define ALPHA_7 "#   # ##### #     #   # #   #     ####  #####     # # ##### #  ## "
#define ALPHA_8 "                                              #####               "
#define SATF_1 "                         "
#define SATF_2 "      #####              "
#define SATF_3 "##### #   # ###### ##### "
#define SATF_4 "#     #####   #    #     "
#define SATF_5 "##### #   #   #    ####  "
#define SATF_6 "    # #   #   #    #     "
#define SATF_7 "##### #   #   #    #     "
#define SATF_8 "              #          "
#define MODS_1 "            #####                         #####               "
#define MODS_2 "      #   # #   # ####        ##### #   # #   # #             "  
#define MODS_3 " # #  ## ## #   # #   #       #     #   # ##### # ##  #       "
#define MODS_4 "##### # # # #   # #   #       #     ##### ##    # # # # ##### "
#define MODS_5 " # #  #   # #   # #   #       ##### #   # # #   # # # # #     "
#define MODS_6 "##### #   # #   # #   #           # #   # #  #  # # # # ##### "
#define MODS_7 " # #  #   # ##### #   #           # #   # #   # # #  ## #     "
#define MODS_8 "                  ####  ##### #####                     ##### "
#define CUPE_1 "                        "
#define CUPE_2 "                        "
#define CUPE_3 "##### #   # ##### ##### "
#define CUPE_4 "#     #   # #   # #     "
#define CUPE_5 "#     #   # ##### ####  "
#define CUPE_6 "##### #   # #     #     "
#define CUPE_7 "      ##### #     ##### "
#define CUPE_8 "            #           "
#define BASTI_1 "                                       "
#define BASTI_2 "                  #####                "
#define BASTI_3 "      ##### #####   #    #             "
#define BASTI_4 "####  #   # #       #    # ##### ##### "
#define BASTI_5 "#   # ##### #####   #    # #     #     "
#define BASTI_6 "####  #   #     #   #    # #     ####  "
#define BASTI_7 "#   # #   # #####   #    # ##### #     "
#define BASTI_8 "####                             ##### "
#define BKUB_1 "            #   #       "
#define BKUB_2 "      #  #  #   # ####  "
#define BKUB_3 "####  # #   #   # #   # "
#define BKUB_4 "#   # ##    #   # #   # "
#define BKUB_5 "####  # #   #   # ####  "
#define BKUB_6 "#   # #  #  #   # #   # "
#define BKUB_7 "####  #   # ##### #   # "
#define BKUB_8 "                  ####  "
#define E_SPACE T_SPACE T_SPACE T_SPACE T_SPACE T_SPACE T_SPACE
char* lines[8] = {
	T_SPACE DEKA_1 H_SPACE ALPHA_1 H_SPACE SATF_1 H_SPACE CUPE_1 H_SPACE BASTI_1 H_SPACE MODS_1 H_SPACE BKUB_1 E_SPACE,
	T_SPACE DEKA_2 H_SPACE ALPHA_2 H_SPACE SATF_2 H_SPACE CUPE_2 H_SPACE BASTI_2 H_SPACE MODS_2 H_SPACE BKUB_2 E_SPACE,
	T_SPACE DEKA_3 H_SPACE ALPHA_3 H_SPACE SATF_3 H_SPACE CUPE_3 H_SPACE BASTI_3 H_SPACE MODS_3 H_SPACE BKUB_3 E_SPACE,
	T_SPACE DEKA_4 H_SPACE ALPHA_4 H_SPACE SATF_4 H_SPACE CUPE_4 H_SPACE BASTI_4 H_SPACE MODS_4 H_SPACE BKUB_4 E_SPACE,
	T_SPACE DEKA_5 H_SPACE ALPHA_5 H_SPACE SATF_5 H_SPACE CUPE_5 H_SPACE BASTI_5 H_SPACE MODS_5 H_SPACE BKUB_5 E_SPACE,
	T_SPACE DEKA_6 H_SPACE ALPHA_6 H_SPACE SATF_6 H_SPACE CUPE_6 H_SPACE BASTI_6 H_SPACE MODS_6 H_SPACE BKUB_6 E_SPACE,
	T_SPACE DEKA_7 H_SPACE ALPHA_7 H_SPACE SATF_7 H_SPACE CUPE_7 H_SPACE BASTI_7 H_SPACE MODS_7 H_SPACE BKUB_7 E_SPACE,
	T_SPACE DEKA_8 H_SPACE ALPHA_8 H_SPACE SATF_8 H_SPACE CUPE_8 H_SPACE BASTI_8 H_SPACE MODS_8 H_SPACE BKUB_8 E_SPACE
};

void RenderMetaballs() {

	// Greets
	cpos++;

	consoleClear();
	iprintf( "\n The following people possess a\n  sufficient amount of balls:\n\n\n\n" );
	for( u8 i = 0; i < 8; i++ ) {
		char* line = strndup( lines[ i ] + (cpos), 32 );
		iprintf( line );
		// iprintf( "\n" );
		free( line );
	}
	
	//swiWaitForVBlank();
	DSMatrixMode( DS_MODELVIEW );
	DSLoadIdentity();
		
	DSRotateZi( rot >> 4 );
	DSTranslatef( -2, -2, 0);

	// Movement.
	xpos = xpos + xdir;
	ypos = ypos + ydir;
	zpos = zpos + zdir;
	if( xpos == 17 ) {
		xdir = -1;
	}
	if( xpos == 3 ) {
		xdir = 1;
	}
	if( ypos == 15) {
		ydir = -1;
	}
	if( ypos == 4 ) {
		ydir = 1;
	}
	if( zpos == 14 ) {
		zdir = -1;
	}
	if( zpos == 3 ) {
		zdir = 1;
	}

	xpos2 = xpos2 + xdir2;
	ypos2 = ypos2 + ydir2;
	zpos2 = zpos2 + zdir2;
	if( xpos2 == 18 ) {
		xdir2 = -1;
	}
	if( xpos2 == 4 ) {
		xdir2 = 1;
	}
	if( ypos2 == 17) {
		ydir2 = -1;
	}
	if( ypos2 == 2 ) {
		ydir2 = 1;
	}
	if( zpos2 == 18 ) {
		zdir2 = -1;
	}
	if( zpos2 == 2 ) {
		zdir2 = 1;
	}
	
	GRIDCELL b;
	u16 tri_count = 0;

	// Calculate grid isolevel strengths and polygonise.
	s32 xa[ 3 ] = { xpos, 10, xpos2 };
	s32 ya[ 3 ] = { ypos, 10 + isin( rot << 8 ) >> 1, ypos2 };
	s32 za[ 3 ] = { zpos, 10, zpos2 };

	tri_count = balls( xa, ya, za, 3, &triangles[ 0 ] );
	
// 	for( int x = 1; x < 20; x++ ) {
// 		for( int y = 1; y < 19; y++ ) {
// 			for( int z = 2; z < 18; z++ ) {
// 				cell_at( &b, x, y, z, xpos, ypos, zpos );
// 				cell_add( &b, x, y, z, 10, 10, 10 );
// 				tri_count += polygonise( &b, 1 << 14, &triangles[ tri_count ] );
// 			}
// 		}
// 	}
	
	// Draw the created mesh.
	DSBegin( DS_TRIANGLES );
// 	s32 normal[3];
	for( u16 i = 0; i < tri_count; i++ ) {
		for( s8 j = 0; j < 3; j++ ) {
// 			s32 normal[ 3 ];
// 			for( u8 v = 0; v < 2; v++ ) {
// 				normal[ 0 ] -= metaball(
// 					triangles[ i ].p[ j ].x + (1<<12),
// 					triangles[ i ].p[ j ].y,
// 					triangles[ i ].p[ j ].z,
// 					xa[ v ], ya[ v ], za[ v ]
// 				);
// 				normal[ 0 ] += triangles[ i ].p[ j ].inf[ v ];
// // 				normal[ 0 ] += metaball(
// // 					triangles[ i ].p[ j ].x - (1<<12),
// // 					triangles[ i ].p[ j ].y,
// // 					triangles[ i ].p[ j ].z,
// // 					xa[ v ], ya[ v ], za[ v ]
// // 				);
// 				normal[ 1 ] -= metaball(
// 					triangles[ i ].p[ j ].x,
// 					triangles[ i ].p[ j ].y + (1<<12),
// 					triangles[ i ].p[ j ].z,
// 					xa[ v ], ya[ v ], za[ v ]
// 				);
// 				normal[ 1 ] += triangles[ i ].p[ j ].inf[ v ];
// // 				normal[ 1 ] += metaball(
// // 					triangles[ i ].p[ j ].x,
// // 					triangles[ i ].p[ j ].y - (1<<12),
// // 					triangles[ i ].p[ j ].z,
// // 					xa[ v ], ya[ v ], za[ v ]
// // 				);
// 				normal[ 2 ] -= metaball(
// 					triangles[ i ].p[ j ].x,
// 					triangles[ i ].p[ j ].y,
// 					triangles[ i ].p[ j ].z + (1<<12),
// 					xa[ v ], ya[ v ], za[ v ]
// 				);
// 				normal[ 0 ] += triangles[ i ].p[ j ].inf[ v ];
// // 				normal[ 2 ] += metaball(
// // 					triangles[ i ].p[ j ].x,
// // 					triangles[ i ].p[ j ].y,
// // 					triangles[ i ].p[ j ].z - (1<<12),
// // 					xa[ v ], ya[ v ], za[ v ]
// // 				);
// 			}
// 			normalizef32( normal );
// 			DSNormal3n10( normal[ 0 ]<<2, normal[ 1 ]<<2, normal[ 2 ]<<2 );
			DSColor3b( triangles[ i ].p[ j ].inf[0]>>9, triangles[ i ].p[ j ].inf[1]>>9, triangles[ i ].p[ j ].inf[2]>>9 );
// 			DSColor3b( triangles[ i ].p[ j ].inf[0]>>9, 0, 31 - (triangles[ i ].p[ j ].inf[0]>>9) );
			DSVertex3v16( triangles[ i ].p[ j ].x, triangles[ i ].p[ j ].y, triangles[ i ].p[ j ].z );
		}
	}
	DSEnd();
		
	rot += 100;
		
	DSFlush( 0 );
}

// Empty, for sake of completeness.
void StopMetaballs() {

}