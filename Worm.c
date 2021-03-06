#include "Worm.h"
#include "DS3D.h"
#include "ARM.h"
#include "Hardware.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>



#define C1 0xffff
#define C2 0x8000

uint16_t texture1[8*8]=
{
	C1,C1,C1,C1,C2,C2,C2,C2,
	C2,C1,C1,C1,C1,C2,C2,C2,
	C2,C2,C1,C1,C1,C1,C2,C2,
	C2,C2,C2,C1,C1,C1,C1,C2,
	C2,C2,C2,C2,C1,C1,C1,C1,
	C1,C2,C2,C2,C2,C1,C1,C1,
	C1,C1,C2,C2,C2,C2,C1,C1,
	C1,C1,C1,C2,C2,C2,C2,C1,
};

uint16_t texture2[8*8]=
{
	C1,C2,C2,C2,C2,C1,C1,C1,
	C1,C1,C2,C2,C2,C1,C1,C2,
	C1,C1,C1,C2,C2,C1,C2,C2,
	C2,C2,C2,C1,C1,C2,C2,C2,
	C2,C2,C2,C1,C1,C2,C2,C2,
	C2,C2,C1,C2,C2,C1,C1,C1,
	C2,C1,C1,C2,C2,C2,C1,C1,
	C1,C1,C1,C2,C2,C2,C2,C1,
};

uint16_t texture3[8*8]=
{
	C1,C1,C1,C1,C2,C2,C2,C1,
	C2,C2,C2,C1,C1,C2,C2,C1,
	C2,C2,C1,C1,C1,C1,C2,C1,
	C2,C1,C1,C2,C2,C1,C1,C1,
	C1,C1,C1,C2,C2,C1,C1,C2,
	C1,C2,C1,C1,C1,C1,C2,C2,
	C1,C2,C2,C1,C1,C2,C2,C2,
	C1,C2,C2,C2,C1,C1,C1,C1,
};

static int flip;
static uint16 *reardepth;
static struct WormLookup *lookuptable;
static uint16 *depthtable;
static uint16 *divtable;
static uint32_t *scaletable;
static uint32_t tex1,tex2,tex3;
static int xrot,yrot;


static const float top=4;
static const float bottom=7;
static const float near=1;
static const float far=32;

static int32 WormFog(int32 z,int32 start,int32 end)
{
	float x=DSf32tofloat(divf32(z-start,end-start));
	return DSf32(x+x*x*(1-x)*(1-x)*(1-x)*(1-x)*20);
}

void StartWorm(char *mapfile,uint8_t *dtcmbuffer)
{
//__/)_\O/__ da sharks is evuhry weyah *<|:o]~ *<|:o]~ da clowns is evuhry weyah *<|:o]~ *<|:o]~ da clowns is evuhry weyah *<|:o]~ *<|:o]~ da clowns is evuhy weyah ^(_(__)####D(_(__)^ ^(_(__)####D(_(__)^ da gays is evuhy weyah { :() { :( da klingons is evuhy weyah ^o^ ^o^ da batz is evuhy weyah ^o^ ^o^ da batz is evuhy weyah ^o^ ^o^ da batz.

	int fd=open(mapfile,O_RDONLY);
	if(fd<0)
	{
		iprintf("Failed to load map file \"%s\"\n",mapfile);
		for(;;);
	}

	read(fd,(void *)0x3000000,MAP_W*MAP_H);
	close(fd);

	for(int i=0;i<MAP_W*MAP_H;i++) ((uint8_t *)0x3000000)[i]/=2;

	lookuptable=(void *)dtcmbuffer;

	for(int i=0;i<NUM_STRIPS;i++)
	{
		float x=(float)i/(float)(NUM_STRIPS-1);
//		float a=(1-(1-x)*(1-x))*M_PI/2;
		float a=x*M_PI/2;
		for(int j=0;j<128;j++)
		{
			float z=sinf(a)*(j+MIN_HEIGHT)*(top-bottom)/(128.0+MIN_HEIGHT)+bottom;
			lookuptable->strip[i].depth[j]=(int)(0x7fff*far*(near-z)/(z*(near-far)))|0x8000;
		}
		lookuptable->strip[i].scale=cosf(a)*2*0.33*0x10000;
	}

	for(int i=1;i<256;i++) lookuptable->div[i]=(0x10000)/i;

VRAMCNT_B=VRAMCNT_B_LCDC;
uint16 *ptr=VRAM_LCDC_B;
for(int i=0;i<192*256;i++) *ptr++=0xffff;

	VRAMCNT_A=VRAMCNT_A_TEXTURE_OFFS_0K;
	VRAMCNT_B=VRAMCNT_B_TEXTURE_OFFS_256K;
	VRAMCNT_C=VRAMCNT_C_LCDC;
	VRAMCNT_D=VRAMCNT_D_LCDC;

	DISPCNT_A=DISPCNT_MODE_0|DISPCNT_3D|DISPCNT_BG0_ON|DISPCNT_ON;

	DSInit3D();
	DSViewport(0,0,255,191);

	DSLight3b3f(0,31,31,31,0.5,-0.5,-1);

	DSSetControl(/*DS_TEXTURING|*/DS_ANTIALIAS|DS_CLEAR_BITMAP|DS_FOG);
	DSClearParams(0,0,8,31,63);

	DSMatrixMode(DS_PROJECTION);
	DSLoadIdentity();
	DSPerspective(45,256.0/192.0,near,far);

	DSLookAt(0.0, 0.0, 2.0, //camera possition 
	         0.0, 0.0, 0.0, //look at
	         0.0, 1.0, 0.0); //up	

//	DSSetFogWithCallback(0,0,0,31,DSf32(top),DSf32(bottom),DSf32(near),DSf32(far),WormFog);
	DSSetFogLinearf(0,0,0,31,top,bottom,near,far);
	DSMaterialShinyness();

// 	tex1=DSAllocAndCopyTexture(DS_TEX_FORMAT_RGB|DS_TEX_SIZE_S_8|DS_TEX_SIZE_T_8,texture1);
// 	tex2=DSAllocAndCopyTexture(DS_TEX_FORMAT_RGB|DS_TEX_SIZE_S_8|DS_TEX_SIZE_T_8,texture2);
// 	tex3=DSAllocAndCopyTexture(DS_TEX_FORMAT_RGB|DS_TEX_SIZE_S_8|DS_TEX_SIZE_T_8,texture3);

	flip=0;
	xrot=yrot=0;

	// Load shit into VRAM.
	fd=open("nitro:/screen_two.img.bin",O_RDONLY);
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
}

void StopWorm()
{
//	free(lookup);
}



void RenderWorm(int t)
{
	if(flip)
	{
		VRAMCNT_A=VRAMCNT_A_TEXTURE_OFFS_384K;
//		VRAMCNT_C=VRAMCNT_C_TEXTURE_OFFS_256K;
		VRAMCNT_D=VRAMCNT_D_LCDC;
		reardepth=VRAM_LCDC_D;
	}
	else
	{
		VRAMCNT_A=VRAMCNT_A_LCDC;
		VRAMCNT_D=VRAMCNT_D_TEXTURE_OFFS_384K;
//		VRAMCNT_D=VRAMCNT_D_TEXTURE_OFFS_256K;
		reardepth=VRAM_LCDC_A;
	}
	flip^=1;



	DSMatrixMode(DS_MODELVIEW);

	DSMaterialDiffuseAndAmbient6b(23,11,0,15,7,0);
	DSMaterialSpecularAndEmission6b(10,1,1,1,0,0,0);

	DSPolyFormat(DS_POLY_ALPHA(31)|DS_POLY_CULL_BACK|DS_POLY_LIGHT0|DS_POLY_FOG);

	DSBegin(DS_QUADS);

	int twist=isin(t/3);
	int pos0=(int)(t*0.001*0x10000)+128*128;

static int extended=0;
if(rand()%(extended*70+30)==0) extended^=1;
static int length=0;

int targetlength=extended*8192+4096;
length+=(targetlength-length)/30;

//	int length=isin(t/4+384)+4096+4096;

	for(int j=-3;j<=3;j++)
	{
		int pos=(pos0+j*128*32)&~(128*32-1);

		int u0=(mulf32(isin(pos>>8),twist)*40);
		int mainangle=u0>>10;

		for(int i=0;i<10;i++)
		{
			DSLoadIdentity();

//			DSTranslatef((pos-pos0)/-,0,-bottom);
			DSTranslatef((pos-pos0)/128.0f/16.0f,0,-bottom);

			int angle=128-(512*i)/20+mainangle%(512/20);
		
			DSRotateXi(angle);
		
			DSScalef(0.3,0.3,length/2048.0f);
			DSTranslatef(0,0,0.5);
		
			DSNormal3f(0,0,1);
			DSVertex3f(-0.5,-0.5,0.5);
			DSVertex3f(0.5,-0.5,0.5);
			DSVertex3f(0.5,0.5,0.5);
			DSVertex3f(-0.5,0.5,0.5);
		
			DSNormal3f(0,0,-1);
			DSVertex3f(0.5,-0.5,-0.5);
			DSVertex3f(-0.5,-0.5,-0.5);
			DSVertex3f(-0.5,0.5,-0.5);
			DSVertex3f(0.5,0.5,-0.5);
		
			DSNormal3f(1,0,0);
			DSVertex3f(0.5,-0.5,0.5);
			DSVertex3f(0.5,-0.5,-0.5);
			DSVertex3f(0.5,0.5,-0.5);
			DSVertex3f(0.5,0.5,0.5);
		
			DSNormal3f(-1,0,0);
			DSVertex3f(-0.5,-0.5,-0.5);
			DSVertex3f(-0.5,-0.5,0.5);
			DSVertex3f(-0.5,0.5,0.5);
			DSVertex3f(-0.5,0.5,-0.5);
		
			DSNormal3f(0,1,0);
			DSVertex3f(-0.5,0.5,0.5);
			DSVertex3f(0.5,0.5,0.5);
			DSVertex3f(0.5,0.5,-0.5);
			DSVertex3f(-0.5,0.5,-0.5);
		
			DSNormal3f(0,-1,0);
			DSVertex3f(-0.5,-0.5,-0.5);
			DSVertex3f(0.5,-0.5,-0.5);
			DSVertex3f(0.5,-0.5,0.5);
			DSVertex3f(-0.5,-0.5,0.5);
		
			DSEnd();
		}
	}

	RenderWormAsm(reardepth,lookuptable,t);
}

