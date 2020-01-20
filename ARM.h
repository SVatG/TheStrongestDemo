#ifndef __ARM_H__
#define __ARM_H__

#include <nds.h>
#include "Worm.h"

void ClaimWRAM();

void ITCM_CODE RenderTunnelAsm(uint16 *table,uint16 *vram,uint16 *texture,int t);
void ITCM_CODE RenderWormAsm(uint16 *reardepth,struct WormLookup *lookuptable,int t);

#endif
