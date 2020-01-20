#ifndef __WORM_H__
#define __WORM_H__

#include <nds.h>

#define MAP_U_SHIFT 7
#define MAP_V_SHIFT 8
#define MAP_W (1<<MAP_U_SHIFT)
#define MAP_H (1<<MAP_V_SHIFT)
#define MAP_U_MASK (MAP_W-1)
#define MAP_V_MASK (MAP_H-1)
#define NUM_STRIPS 24
#define MIN_HEIGHT 64

struct WormLookup
{
	uint32_t div[256];

	struct WormStripLookup
	{
		uint16_t depth[128];
		uint32_t scale;
	} strip[NUM_STRIPS];
};

void StartWorm(char *mapfile,uint8 *dtcmbuffer);
void StopWorm();
void RenderWorm(int offs);


#endif
