#include "Feedback.h"
#include "DS3D.h"
#include "Hardware.h"

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

static int flip;
static uint32_t whitetexture;
static bool fadingout;
static int fadestart;

// PRNG
// uint16_t rainbow_rand_2 = 0xD00D;
// uint16_t rand() {
// 	uint16_t next =
// 		(rainbow_rand_2 & 0x0001) ^
// 		((rainbow_rand_2 & 0x0004) >> 2) ^
// 		((rainbow_rand_2 & 0x0008) >> 3) ^
// 		((rainbow_rand_2 & 0x0020) >> 5);
// 	rainbow_rand_2 = (rainbow_rand_2 >> 1) | (next << 15);
// 	return( rainbow_rand_2 );
// }

void hblank() {
	
}

void StartFeedback()
{
	// Load shit into VRAM.
	int fd=open("nitro:/screen_one.img.bin",O_RDONLY);
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
	
	fd=open("nitro:/overlay1.texture",O_RDONLY);
	if(fd<0)
	{
		iprintf("Failed to load texture\n");
		for(;;);
	}

	VRAMCNT_D=VRAMCNT_D_LCDC;
	read(fd,VRAM_LCDC_D,256*192*2);
	close(fd);

	VRAMCNT_A=VRAMCNT_A_LCDC;
	VRAMCNT_B=VRAMCNT_B_LCDC;
	VRAMCNT_D=VRAMCNT_D_BG_VRAM_A_OFFS_0K;
	// VRAMCNT_C=VRAMCNT_C_LCDC;

	DISPCNT_A=DISPCNT_MODE_4|DISPCNT_3D|DISPCNT_BG0_ON|DISPCNT_BG3_ON|DISPCNT_ON;
	BG0CNT_A=BGxCNT_PRIORITY_2;
	BG3CNT_A=BGxCNT_BITMAP_BASE_0K|BGxCNT_EXTENDED_BITMAP_16
			|BGxCNT_BITMAP_SIZE_256x256|BGxCNT_PRIORITY_1; // RGB bitmap mode
	BG3PA_A=0x100;
	BG3PB_A=0;
	BG3PC_A=0;
	BG3PD_A=0x100;
	BG3HOFS_A=0;
	BG3VOFS_A=0;

	DSInit3D();
	DSViewport(0,0,255,191);

	DSSetControl(DS_TEXTURING|DS_ANTIALIAS|DS_ALPHA_BLEND);
	DSClearParams(0,0,0,31,63);

	for(int i=0;i<256*256;i++)
	{
		VRAM_LCDC_A[i]=0x8000;
		VRAM_LCDC_B[i]=0x8000;
	}

	DSCopyColorTexture(DS_TEX_ADDRESS(VRAM_LCDC_A+256*204),0x7fff);
	DSCopyColorTexture(DS_TEX_ADDRESS(VRAM_LCDC_B+256*204),0x7fff);
	whitetexture=DS_TEX_ADDRESS(VRAM_LCDC_A+256*204);

	flip=0;
	fadingout=false;
	//whitetexture=DSMakeWhiteTexture();
//DSTranslatef32(128,96,0);
//DSScalef(100.0/4096,100.0/4096,1);

	// Interrupt for pretty
	irqEnable( IRQ_HBLANK );
	irqSet( IRQ_HBLANK, hblank );
}

void StopFeedback()
{
	REG_DISPCAPCNT=0;
}

void RenderFeedback(int t,bool fadeout)
{
	int capsrc;

	uint16_t *ptr;
	if(flip) ptr=VRAM_LCDC_A;
	else ptr=VRAM_LCDC_B;
	for(int i=0;i<256*10;i++)
	{
		ptr[256*192+i]=ptr[256*191+i];
	}

	if(fadeout&&!fadingout)
	{
		fadingout=true;
		fadestart=t;

		DISPCNT_A=DISPCNT_MODE_4|DISPCNT_3D|DISPCNT_BG0_ON|DISPCNT_BG3_ON|DISPCNT_ON;
		capsrc=0;
	}
	else if(fadingout)
	{
		DISPCNT_A=DISPCNT_MODE_4|DISPCNT_3D|DISPCNT_BG0_ON|DISPCNT_ON;
		capsrc=1;
	}
	else
	{
		DISPCNT_A=DISPCNT_MODE_4|DISPCNT_3D|DISPCNT_BG0_ON|DISPCNT_BG3_ON|DISPCNT_ON;
		capsrc=1;
	}

	if(flip)
	{
		VRAMCNT_A=VRAMCNT_A_TEXTURE_OFFS_0K;
		VRAMCNT_B=VRAMCNT_B_LCDC;
		REG_DISPCAPCNT=DCAP_BANK(1)|DCAP_SIZE(3)|DCAP_SRC(capsrc)|DCAP_MODE(0)|DCAP_ENABLE;
	}
	else
	{
		VRAMCNT_A=VRAMCNT_A_LCDC;
		VRAMCNT_B=VRAMCNT_B_TEXTURE_OFFS_0K;
		if(t!=0) REG_DISPCAPCNT=DCAP_BANK(0)|DCAP_SIZE(3)|DCAP_SRC(capsrc)|DCAP_MODE(0)|DCAP_ENABLE;
	}
	flip^=1;

	DSMatrixMode(DS_PROJECTION);
	DSLoadIdentity();
	DS2DProjection(0);

	DSMatrixMode(DS_MODELVIEW);
	DSLoadIdentity();

	DSSetTexture(0|DS_TEX_SIZE_S_256|DS_TEX_SIZE_T_256|DS_TEX_FORMAT_RGB|DS_TEX_GEN_TEXCOORD);

/*for(int i=0;i<6;i++)
for(int j=0;j<256;j+=2<<i)
{
	DSVertex3v16(j,i*20,DSf32(-1));
	DSVertex3v16(j+(1<<i),i*20,DSf32(-1));
	DSVertex3v16(j+(1<<i),(i+1)*20,DSf32(-1));
	DSVertex3v16(j,(i+1)*20,DSf32(-1));
}

DSColor3b(0,31,0);

for(int i=0;i<6;i++)
for(int j=0;j<192;j+=2<<i)
{
	DSVertex3v16(i*20,j,DSf32(-1));
	DSVertex3v16(i*20,j+(1<<i),DSf32(-1));
	DSVertex3v16((i+1)*20,j+(1<<i),DSf32(-1));
	DSVertex3v16((i+1)*20,j,DSf32(-1));
}*/
// 	if(fadeout)
// 	{
// 		int col=31-(t-fadestart)/4;
// 		DSColor3b(col,col,col);
// 	}
// 	else DSColor(0x7fff);
	DSColor(0x7fff);
	DSMatrixMode(DS_TEXTURE);
	for(int i=1;i<5;i++)
	{
		s16 rx=rand()%127-63;
		s16 ry=rand()%127-63;
//		int x=(128+10)*16+rx,y=(96+10)*16+ry;
		s16 x=(128+15)*16+rx,y=(96+15)*16+ry;
		DSLoadIdentity();
		DSTranslatef(x,y,0);
		DSScalef((1-(float)i/40),(1-(float)i/40),0);
		DSTranslatef(-x,-y,0);

		DSTranslatef(128*16,96*16,0);
		DSRotateZi(5*i*isin(3*t/2+128)>>12);
		DSTranslatef(-128*16,-96*16,0);

		if(i==1)
//		DSPolyFormat(DS_POLY_CULL_NONE|DS_POLY_ALPHA(16));
		DSPolyFormat(DS_POLY_CULL_NONE|DS_POLY_ALPHA(31/i));
		else
		DSPolyFormat(DS_POLY_MODE_DECAL|DS_POLY_CULL_NONE|DS_POLY_ALPHA(31/i)|DS_POLY_DEPTH_TEST_EQUAL);

		DSBegin(DS_QUADS);
		DSTexCoord2f(0,0); DSVertex3v16(0,0,DSf32(-1));
		DSTexCoord2f(256,0); DSVertex3v16(256,0,DSf32(-1));
		DSTexCoord2f(256,192); DSVertex3v16(256,192,DSf32(-1));
		DSTexCoord2f(0,192); DSVertex3v16(0,192,DSf32(-1));
		DSEnd();
	}

	DSLoadIdentity();

	if(!fadingout)
	{
		DSPolyFormat(DS_POLY_MODE_DECAL|DS_POLY_CULL_NONE|DS_POLY_ALPHA(31));

		DSSetTexture(whitetexture);

		DSBegin(DS_TRIANGLES);
		for(int i=0;i<10;i++)
		{
			switch(rand()%2)
			{
				case 0: DSColor3b(rand()%15+16,rand()%15,rand()%15); break;
//				case 1: DSColor3b(rand()%15,rand()%15+16,rand()%15); break;
				case 1: DSColor3b(rand()%15,rand()%15,rand()%15+16); break;
			}
//			DSColor(rand()&0x7fff);
			DSVertex3v16(rand()%32+112,rand()%32+80,DSf32(-0.5));
			DSVertex3v16(rand()%32+112,rand()%32+80,DSf32(-0.5));
			DSVertex3v16(rand()%32+112,rand()%32+80,DSf32(-0.5));
		}
		DSEnd();
	}

	t++;
}

