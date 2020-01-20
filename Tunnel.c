#include "Tunnel.h"
#include "DS3D.h"
#include "ARM.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

extern uint16_t texture1[8*8];
extern uint16_t texture2[8*8];
extern uint16_t texture3[8*8];

static uint16_t *lookup=NULL;

static int flip;
static uint16_t *rearbitmap;
static uint32_t tex1,tex2,tex3;
static int xrot,yrot;

void StartTunnel(char *tablefile,char *texturefile)
{
//__/)_\O/__ da sharks is evuhry weyah *<|:o]~ *<|:o]~ da clowns is evuhry weyah *<|:o]~ *<|:o]~ da clowns is evuhry weyah *<|:o]~ *<|:o]~ da clowns is evuhy weyah ^(_(__)####D(_(__)^ ^(_(__)####D(_(__)^ da gays is evuhy weyah { :() { :( da klingons is evuhy weyah ^o^ ^o^ da batz is evuhy weyah ^o^ ^o^ da batz is evuhy weyah ^o^ ^o^ da batz.

	int fd=open(tablefile,O_RDONLY);
	if(fd<0)
	{
		iprintf("Failed to load lookup table file \"%s\"\n",texturefile);
		for(;;);
	}

	lookup=malloc(256*192*2);
	read(fd,lookup,256*192*2);

	vramSetBankB(VRAM_B_LCD);
	read(fd,VRAM_B,256*192*2);

	close(fd);

	fd=open(texturefile,O_RDONLY);
	if(fd<0)
	{
		iprintf("Failed to load texture file \"%s\"\n",texturefile);
		for(;;);
	}

	read(fd,(void *)0x3000000,128*128*2);
	close(fd);

	DSInit3D();
	DSViewport(0,0,255,191);

	DSSetControl(DS_TEXTURING|DS_ANTIALIAS|DS_CLEAR_BITMAP|DS_FOG);
	DSLight3b3f(0,31,31,31,0.5,-0.5,-1);

//	iprintf("Setup banks\n");
	vramSetBankA(VRAM_A_TEXTURE_SLOT0);
	vramSetBankB(VRAM_B_TEXTURE_SLOT3);
	vramSetBankC(VRAM_C_LCD);
	vramSetBankD(VRAM_D_LCD);

//	iprintf("3D init\n");

	DSMatrixMode(DS_PROJECTION);
	DSLoadIdentity();
	DSPerspective(70,256.0/192.0,1,32);

	DSLookAt(0.0, 0.0, 2.0, //camera possition 
	         0.0, 0.0, 0.0, //look at
	         0.0, 1.0, 0.0); //up	

	DSSetFogLinearf(0,0,0,31,2,6,1,32);
	DSMaterialShinyness();

	tex1=DSAllocAndCopyTexture(DS_TEX_FORMAT_RGB|DS_TEX_SIZE_S_8|DS_TEX_SIZE_T_8,texture1);
	tex2=DSAllocAndCopyTexture(DS_TEX_FORMAT_RGB|DS_TEX_SIZE_S_8|DS_TEX_SIZE_T_8,texture2);
	tex3=DSAllocAndCopyTexture(DS_TEX_FORMAT_RGB|DS_TEX_SIZE_S_8|DS_TEX_SIZE_T_8,texture3);

	flip=0;
	xrot=yrot=0;
}

void StopTunnel()
{
	free(lookup);
}



void RenderTunnel(int t)
{
	if(flip)
	{
		vramSetBankC(VRAM_C_TEXTURE_SLOT2);
		vramSetBankD(VRAM_D_LCD);
		rearbitmap=VRAM_D;
	}
	else
	{
		vramSetBankC(VRAM_C_LCD);
		vramSetBankD(VRAM_D_TEXTURE_SLOT2);
		rearbitmap=VRAM_C;
	}
	flip^=1;

	DSMatrixMode(DS_MODELVIEW);
	DSLoadIdentity();

	DSTranslatef(0,0,-2.3);
	DSRotateYi(t);
	DSTranslatef(0,0,-2.3);
//DSTranslatef32(128,96,0);
//DSScalef(100.0/4096,100.0/4096,1);
	DSRotateXi(xrot);
	DSRotateYi(yrot);

	DSMaterialDiffuseAndAmbient6b(20,20,20,12,12,12);
	DSMaterialSpecularAndEmission6b(0,0,0,0,0,0,0);

	DSPolyFormat(DS_POLY_ALPHA(31)|DS_POLY_CULL_BACK|DS_POLY_LIGHT0|DS_POLY_FOG);

	scanKeys();
	uint16_t keys=keysHeld();
	if((keys&KEY_UP)) xrot+=6;
	if((keys&KEY_DOWN)) xrot-=6;
	if((keys&KEY_LEFT)) yrot+=6;
	if((keys&KEY_RIGHT)) yrot-=6;

	DSBegin(DS_QUADS);

	DSSetTexture(tex1);

	DSNormal3f(0,0,1);
	DSTexCoord2f(0,8); DSVertex3f(-0.5,-0.5,0.5);
	DSTexCoord2f(8,8); DSVertex3f(0.5,-0.5,0.5);
	DSTexCoord2f(8,0); DSVertex3f(0.5,0.5,0.5);
	DSTexCoord2f(0,0); DSVertex3f(-0.5,0.5,0.5);

	DSNormal3f(0,0,-1);
	DSTexCoord2f(0,8); DSVertex3f(0.5,-0.5,-0.5);
	DSTexCoord2f(8,8); DSVertex3f(-0.5,-0.5,-0.5);
	DSTexCoord2f(8,0); DSVertex3f(-0.5,0.5,-0.5);
	DSTexCoord2f(0,0); DSVertex3f(0.5,0.5,-0.5);

	DSNormal3f(1,0,0);
	DSTexCoord2f(0,8); DSVertex3f(0.5,-0.5,0.5);
	DSTexCoord2f(8,8); DSVertex3f(0.5,-0.5,-0.5);
	DSTexCoord2f(8,0); DSVertex3f(0.5,0.5,-0.5);
	DSTexCoord2f(0,0); DSVertex3f(0.5,0.5,0.5);

	DSNormal3f(-1,0,0);
	DSTexCoord2f(0,8); DSVertex3f(-0.5,-0.5,-0.5);
	DSTexCoord2f(8,8); DSVertex3f(-0.5,-0.5,0.5);
	DSTexCoord2f(8,0); DSVertex3f(-0.5,0.5,0.5);
	DSTexCoord2f(0,0); DSVertex3f(-0.5,0.5,-0.5);

	DSSetTexture(tex2);

	DSNormal3f(0,1,0);
	DSTexCoord2f(0,8); DSVertex3f(-0.5,0.5,0.5);
	DSTexCoord2f(8,8); DSVertex3f(0.5,0.5,0.5);
	DSTexCoord2f(8,0); DSVertex3f(0.5,0.5,-0.5);
	DSTexCoord2f(0,0); DSVertex3f(-0.5,0.5,-0.5);

	DSSetTexture(tex3);

	DSNormal3f(0,-1,0);
	DSTexCoord2f(0,8); DSVertex3f(-0.5,-0.5,-0.5);
	DSTexCoord2f(8,8); DSVertex3f(0.5,-0.5,-0.5);
	DSTexCoord2f(8,0); DSVertex3f(0.5,-0.5,0.5);
	DSTexCoord2f(0,0); DSVertex3f(-0.5,-0.5,0.5);

	DSEnd();

	int offs=t*0x100;
	RenderTunnelAsm(lookup,rearbitmap,(uint16_t *)0x3000000,offs);
}
