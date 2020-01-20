#if !defined(KVIL_H)
#define KVIL_H

/**
 * KVIL - KVasigen Image Library
 *
 * Copyright (c) 2002 - 2003 Mats Lindh
 * PNG and JPG loading based on example code from libpng and libjpg.
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the 
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject to 
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define KVIL_PROGRESS_CALLBACK
#define KVIL_VERSION 006

#pragma pack(1)

typedef struct {
	unsigned int size_of_header;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bpp;
	unsigned int compression;
	unsigned int imagesize;
	unsigned int xpixelsprm;
	unsigned int ypixelsprm;
	unsigned int colorsused;
	unsigned int colorsimportant;
} kvil_bmp_infoheader;

typedef struct {
	short signature;
	// filesize er skrot.
	unsigned int file_size;
	unsigned int reserved;
	unsigned int dataoffset;
} kvil_bmp_fileheader;

typedef struct {
	unsigned short first_idx;
	unsigned short num_color_maps;
	char bit_pr_map;
} kvil_tga_colormap;

typedef struct {
	unsigned short x_origin;
	unsigned short y_origin;
	unsigned short width;
	unsigned short height;
	char bpp;
	char image_descriptor;
} kvil_tga_imagedesc;

typedef struct {
	char idlength;
	char colormap_type;
	char image_type;
	kvil_tga_colormap color_map;
	kvil_tga_imagedesc image_desc;
} kvil_tga_fileheader;

#pragma pack()

class kvil  
{
public:
	kvil();
	virtual ~kvil();

	void LoadGL(unsigned int texidx, char * filename, int num = 0);
	void LoadGLSphereMap(unsigned int texidx, char * filename, int num = 0);
	void LoadGL(GLuint * indexes, unsigned int files, ...);

	void LoadGL640x480Split(GLuint * indexes, char * filename, int num = 0);
	void LoadMultipleGL(GLuint * indexes, char * filename, unsigned int sizex, unsigned int sizey, int num = 0);

	void LoadBMP(FILE *fp, int num);
	void DumpSequenceBMP(char * filename, int w, int h);
	void DumpBMP(char * filename, int w, int h);
	void WriteBMP(char * filename, int w, int h, char * data);

	void LoadJPEG(FILE *fp, int num);
	void LoadTGA(FILE *fp, int num);
	void LoadPNG(FILE *fp, int num);
	
	void LoadRAWGL(unsigned int texidx, char * filename, unsigned int x, unsigned int y, int num = 0);

	bool Load(char * filename, int num = 0);

#ifdef KVIL_PROGRESS_CALLBACK
	void setScanlineCallback(void (*func)(float percent, int idx)) {
		scanline_callback = func;
	}
#endif
private:
	unsigned char *data;
	unsigned int height;
	unsigned int width;
	unsigned short bpp;
	splitToGL(int idx, int width, int height, int startx, int starty, GLuint filtermode = GL_NEAREST);
#ifdef KVIL_PROGRESS_CALLBACK
	void (*scanline_callback)(float percent, int idx);
#endif
};

#endif
