#include "ARM.h"
#include "Worm.h"
#include "DS3D.h"

#include <nds.h>
#include <math.h>





void ClaimWRAM()
{
	WRAM_CR=0;
	asm volatile ("mcr p15,0,%0,C6,C2,0"::"r" (0x300002f));
}




void ITCM_CODE RenderTunnelAsm(uint16 *table,uint16 *vram,uint16 *texture,int t)
{
	#if 0

/*	uint32 *table32=(uint32 *)table;
	uint16 *texture=(uint16 *)0x3000000;
	uint32 *vram32=(uint32 *)vram;

	for(int i=0;i<256*192/2;i++)
	{
		uint32 val=*table32++;
		uint32 a=texture[(val>>16)+t];
		uint32 b=texture[(val+t)&0xffff];
		*vram32++=(a<<16)|b;
	}*/
	uint32 *table32=(uint32 *)table;
	uint16 *textureptr=texture+(t&0x3fff);
	uint32 *vram32=(uint32 *)vram;

	for(int i=0;i<256*192/2;i++)
	{
		uint32 val=*table32++;
		uint32 a=textureptr[val>>17];
		uint32 b=textureptr[(val&0xffff)>>1];
		*vram32++=(a<<16)|b;
	}

	#else

	register uint32 r0 asm("r0")=(uint32)table;
	register uint32 r1 asm("r1")=(uint32)vram;
	register uint32 r2 asm("r2")=(uint32)(texture+(t&0x3fff));
	register uint32 r3 asm("r3")=0x7fff;
	register uint32 r14 asm("r14")=256*192/(2*8);
	// r0: table
	// r1: vram
	// r2: texture
	// r3: 0x7fff
	// r4: tmp
	// r5,r6,r7,r8,r9,r10: val
	// r11: 256*192/(2*6)

	asm volatile (
	".tunnelloop:\n"
	"	ldmia		r0!,{r5,r6,r7,r8,r9,r10,r11,r12}	\n"

	"	ldr		r4,[r2,r5,lsr #16]	\n"
	"	and		r5,r5,r3			\n"
	"	ldrh	r5,[r2,r5]			\n"
	"	orr		r5,r5,r4,lsl #16	\n"

	"	ldr		r4,[r2,r6,lsr #16]	\n"
	"	and		r6,r6,r3			\n"
	"	ldrh	r6,[r2,r6]			\n"
	"	orr		r6,r6,r4,lsl #16	\n"

	"	ldr		r4,[r2,r7,lsr #16]	\n"
	"	and		r7,r7,r3			\n"
	"	ldrh	r7,[r2,r7]			\n"
	"	orr		r7,r7,r4,lsl #16	\n"

	"	ldr		r4,[r2,r8,lsr #16]	\n"
	"	and		r8,r8,r3			\n"
	"	ldrh	r8,[r2,r8]			\n"
	"	orr		r8,r8,r4,lsl #16	\n"

	"	ldr		r4,[r2,r9,lsr #16]	\n"
	"	and		r9,r9,r3			\n"
	"	ldrh	r9,[r2,r9]			\n"
	"	orr		r9,r9,r4,lsl #16	\n"

	"	ldr		r4,[r2,r10,lsr #16]	\n"
	"	and		r10,r10,r3			\n"
	"	ldrh	r10,[r2,r10]		\n"
	"	orr		r10,r10,r4,lsl #16	\n"

	"	ldr		r4,[r2,r11,lsr #16]	\n"
	"	and		r11,r11,r3			\n"
	"	ldrh	r11,[r2,r11]		\n"
	"	orr		r11,r11,r4,lsl #16	\n"

	"	ldr		r4,[r2,r12,lsr #16]	\n"
	"	and		r12,r12,r3			\n"
	"	ldrh	r12,[r2,r12]		\n"
	"	orr		r12,r12,r4,lsl #16	\n"

	"	stmia	r1!,{r5,r6,r7,r8,r9,r10,r11,r12}		\n"

	"	subs	r14,r14,#1			\n"
	"	bne		.tunnelloop			\n"
	:
	:"r" (r0),"r" (r1),"r" (r2),"r" (r3),"r" (r14)
	:"r4","r5","r6","r7","r8","r9","r10","r11","r12"
	);

	#endif
}

static inline void draw_vline(uint16 *reardepth,uint32 *divtable,int x,int y1,int y2,int c1,int c2)
{
//	uint16 *ptr=((uint16 *)reardepth)+x+y1*256;
	uint32 *ptr=((uint32 *)reardepth)+x+y1*128;
	int n=y2-y1;
	int dc=(c2-c1)*divtable[n];
	int c=c1<<16;
	while(n--)
	{
		*ptr=(c&0xffff0000)|((c>>16)&0xffff);
//		*ptr=c>>16;
//		ptr+=256;
		ptr+=128;
		c+=dc;
	}
}

#define STR(x) #x
#define XSTR(x) STR(x)
#define C(x) "#" STR(x)

void ITCM_CODE RenderWormAsm(uint16 *reardepth,struct WormLookup *lookuptable,int t)
{
	int w=256/2;
	int mid=192/2;

	uint32 *ptr=(uint32 *)reardepth;
//	for(int i=0;i<256*192/2;i++) *ptr++=0;
	for(int i=0;i<256*192/2;i++) *ptr++=0xffffffff;

	//int u0=(int)(0*0x10000)&0xffff;
	//int v0=(int)(t*0.003*0x10000)&0xffff;

//	int du0=0.006*sinf(t*0.01)*0x10000;
	//int du0=0.01*sinf(t*0.005)*0x10000;
	//int dv0=0.001*0x10000;
	int du=0x10000/(2*NUM_STRIPS-1);
	int twist=isin(t/3);

	reardepth+=mid*256;

	int pos0=(int)(t*0.001*0x10000);

	for(int x=0;x<w;x++)
	{
		//u0+=du0; v0+=dv0;
		int pos=pos0+x*128;
		int u0=(mulf32(isin(pos>>8),twist)*40)&0xffff;
		int v0=(pos)&0xffff;

		#if 1

		register uint32 r2 asm("r2")=u0;
		register uint32 r3 asm("r3")=du;
		register uint32 r5 asm("r5")=(uint32)reardepth+x*4-512;
		register uint32 r6 asm("r6")=(uint32)lookuptable;
		register uint32 r10 asm("r10")=0x3000000+((v0>>1)&0x7f80);
	
		// r0: 
		// r1: i
		// r2: u
		// r3: du
		// r5: reardepth+x*2-512
		// r6: lookuptable
		// r7: lookuptable strips
		// r8: prev_y
		// r9: prev_c
		// r10: 0x3000000+(v>>1)&0x7f80
		// r11:
		// r12:
		// r14:
	
		asm volatile(
		"	push	{r2}				\n"
		"	mov 	r1,#0				\n"
		"	add		r7,r6,#1024			\n"
		"	mov		r8,#1000			\n"
	
		"upperloop:						\n"
		"	and		r14,r2,#0xfe00		\n"
		"	ldrb	r14,[r10,r14,lsr #9]\n" // r14=h

		"	mov		r12,r14,lsl #1		\n"
		"	ldrh	r12,[r7,r12]		\n" // r12=c=lookuptable->strip[i].depth[h]
		"	add		r14,r14," C(MIN_HEIGHT) "\n"
		"	ldr		r11,[r7,#256]		\n"
		"	smulwb	r14,r11,r14			\n"
		"	rsc		r14,r14,#0			\n" // r14=y=-((h+MIN_HEIGHT)*lookuptable->strip[i].scale>>16)
		"	subs	r11,r14,r8			\n"	// r11=n=y-prev_y
		"	ble		skipupper			\n"

		"	push	{r1,r2}				\n"
		"	mov		r1,r12				\n"
		"	orr		r1,r1,lsl #16		\n"
		"	ldr		r0,[r6,r11,lsl #2]	\n"
		"	subs	r2,r9,r12			\n"
		"	bmi		drawtopdownupper	\n"

		"	smulwb	r0,r0,r2			\n" // r0=-dc=(prev_c-c)*divtable[n]>>16
		"	orr		r0,r0,lsl #16		\n"
		"	add		r2,r5,r14,lsl #9	\n" // r3=reardepth+x*2+y*512-256
		"	add		r2,r2,#512	\n"

		"	rsc		r11,r11,#32			\n"
		"	add		pc,pc,r11,lsl #3	\n"
		"	nop							\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	b		enddrawloopupper	\n"

		"drawtopdownupper:				\n"
		"	rsc		r2,r2,#0			\n"
		"	smulwb	r0,r0,r2			\n" // r0=dc=(c-prev_c)*divtable[n]>>16
		"	orr		r0,r0,lsl #16		\n"
		"	add		r2,r5,r8,lsl #9		\n" // r3=reardepth+x*2+prev_y*512-256

		"	rsc		r11,r11,#33			\n"
		"	add		pc,pc,r11,lsl #3	\n"
		"	nop							\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"enddrawloopupper:				\n"
		"	pop		{r1,r2}				\n"

		"skipupper:						\n"
		"	mov		r8,r14				\n" // r8=prev_y=-(-y)
		"	mov		r9,r12				\n" // r9=prev_c=c
		"	add		r2,r2,r3			\n"
		"	add		r7,r7,#260			\n"
		"	add		r1,r1,#1			\n"
		"	cmp		r1," C(NUM_STRIPS) "\n"
		"	bne		upperloop			\n"



		// lower half

		"	pop		{r2}				\n"
		"	mov 	r1,#0				\n"
		"	add		r7,r6,#1024			\n"
		"	mvn		r8,#1000			\n"
	
		"lowerloop:						\n"
		"	and		r14,r2,#0xfe00		\n"
		"	ldrb	r14,[r10,r14,lsr #9]\n" // r14=h

		"	mov		r12,r14,lsl #1		\n"
		"	ldrh	r12,[r7,r12]		\n" // r12=c=lookuptable->strip[i].depth[h]
		"	add		r14,r14," C(MIN_HEIGHT) "\n"
		"	ldr		r11,[r7,#256]		\n"
		"	smulwb	r14,r11,r14			\n" // r14=y=((h+MIN_HEIGHT)*lookuptable->strip[i].scale>>16)
		"	subs	r11,r8,r14			\n"	// r11=n=prev_y-y
		"	ble		skiplower			\n"

		"	push	{r1,r2}				\n"
		"	mov		r1,r12				\n"
		"	orr		r1,r1,lsl #16		\n"
		"	ldr		r0,[r6,r11,lsl #2]	\n"

		"	subs	r2,r12,r9			\n"
		"	bmi		drawtopdownlower	\n"

		"	smulwb	r0,r0,r2			\n" // r0=-dc=(prev_c-c)*divtable[n]>>16
		"	orr		r0,r0,lsl #16		\n"
		"	add		r2,r5,r8,lsl #9	\n" // r3=reardepth+x*2+prev_y*512-256

		"	rsc		r11,r11,#32			\n"
		"	add		pc,pc,r11,lsl #3	\n"
		"	nop							\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#-512]!		\n	add		r1,r1,r0	\n"
		"	b		enddrawlooplower	\n"

		"drawtopdownlower:				\n"
		"	rsc		r2,r2,#0			\n"
		"	smulwb	r0,r0,r2			\n" // r0=dc=(c-prev_c)*divtable[n]>>16
		"	orr		r0,r0,lsl #16		\n"
		"	add		r2,r5,r14,lsl #9		\n" // r3=reardepth+x*2+y*512-256
		"	sub		r2,r2,#512	\n"

		"	rsc		r11,r11,#33			\n"
		"	add		pc,pc,r11,lsl #3	\n"
		"	nop							\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"
		"	str		r1,[r2,#512]!		\n	add		r1,r1,r0	\n"

		"enddrawlooplower:				\n"
		"	pop		{r1,r2}				\n"

		"skiplower:						\n"
		"	mov		r8,r14				\n" // r8=prev_y=-(-y)
		"	mov		r9,r12				\n" // r9=prev_c=c
		"	sub		r2,r2,r3			\n"
		"	add		r7,r7,#260			\n"
		"	add		r1,r1,#1			\n"
		"	cmp		r1," C(NUM_STRIPS) "\n"
		"	bne		lowerloop			\n"

		:"=r" (r2)
		:"r" (r2),"r" (r3),"r" (r5),"r" (r6),"r" (r10)
		:"r0","r1","r4","r7","r8","r9","r11","r12","r14","cc"
		);

		#else

		int u=u0;
		int prev_y=1000,prev_c=0;

		uint8 *map=(uint8 *)(0x3000000+((v0>>(16-MAP_V_SHIFT-MAP_U_SHIFT))&(MAP_V_MASK<<MAP_U_SHIFT)));
		struct WormStripLookup *lookupstrip=lookuptable->strip;

		// draw upper half
		for(int i=0;i<NUM_STRIPS;i++)
		{
			int h=map[((u>>(16-MAP_U_SHIFT))&MAP_U_MASK)];
			int y=-((h+MIN_HEIGHT)*lookupstrip->scale>>16);
			int c=lookupstrip->depth[h];

			if(y>prev_y)
			{
				draw_vline(reardepth,lookuptable->div,x,prev_y,y,prev_c,c);
			}

			prev_c=c; prev_y=y;
			lookupstrip++;
			u+=du;
		}

		prev_y=-1000;
		u=u0;
		lookupstrip=lookuptable->strip;

		// draw lower half
		for(int i=0;i<NUM_STRIPS;i++)
		{
			int h=map[((u>>(16-MAP_U_SHIFT))&MAP_U_MASK)];
			int y=((h+MIN_HEIGHT)*lookupstrip->scale>>16);
			int c=lookupstrip->depth[h];

			if(y<prev_y)
			{
				draw_vline(reardepth,lookuptable->div,x,y,prev_y,c,prev_c);
			}

			prev_c=c; prev_y=y;
			lookupstrip++;
			u-=du;
		}
		#endif
	}
}
