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
 *
 * Rask sak på hvordan bruke kvil-morroa;
 * 
 * #include "kvil.h"
 * 
 * kvil * kv = new kvil();
 * kv->Load("filnavn.jpg/filnavn.tga/filnavn.bmp/filnavn.png");
 * 
 * kv->data inneholder nå RGB-data, evnt. RGBA (se kv->bpp for 24/32 bit)
 * 
 * du må foreløpig også linke mot GL, men det burde ikke være verre enn å kommentere ut LoadGL-funksjonene for å slippe det.
 * 
 * for GL-loading:
 * 
 * kv->LoadGL(texnum, "filnavn");
 * 
 * kv->loadGL(textures, <antallet filer>, "fil1.jpg", "fil2.jpg" ..)
 * der textures er et GLuint-array som det er kjørt glGenTextures() på.
 *
 * kv->setScanlineCallback(funksjonsnavn)
 * der funksjonsnavn er definert som void funksjonsnavn (float prosent, int bildenum)
 *
 * hvis du overhodet ikke skal bruke progressbar-funksjonene og er optimaliseringsgal, kommenter ut
 * #define KVIL_PROGRESS_CALLBACK i kvil.h
 * for debuginformasjon (printf) #define __DEBUG før kvil.h inkluderes (.. tror det er best, iallefall)
 * 
 * link mot lib/libjpeg.lib lib/ligpng.lib lib/zlibstat.lib
 *
 * v0.0.6
 *  * la til DumpSequenceBMP, DumpBMP og WriteBMP:
 *		- DumpSequenceBMP tar et prefix og en størrelse (0,0) - (w,h). Denne rutinen genererer et nytt filnavn for hver gang den kalles;
 *		  prefix_001.bmp, prefix_002.bmp, prefix_003.bmp osv. Kaller deretter DumpBMP.
 *		- DumpBMP dumper OpenGLs videominne til systemminne og kaller deretter WriteBMP
 *		- WriteBMP tar et filnavn, størrelse og RGB-data som input og dumper dette til en BMP-kompatibel fil.
 *	  Til sammen implementerer dette skriving av generelle bmp-bilder, screenshots fra en opengl-applikasjon og dumping av en sekvens til fil (animasjon etc).
 *    For å sette sammen en bildesekvens kan f.eks. bmp2avi, VirtualDub eller Adobe Premiere brukes.
 *  * fikset bug med at jeg prøvde å lukke en fil som ikke var åpnet dersom filen var ikke-eksisterende
 *  * fjernet noe dill som lå igjen fra forrige prosjekt
 *
 * v0.0.5
 *  * la til mulighet for å spesifisere texturefilter-mode på SplitGL, slik at det ikke blir avvik i bildene
 *    når det er snakk om 1:1 mapping.
 *
 * v0.0.4
 *  * LoadMultipleGL(GLuint * indexes, char * filename, unsigned int sizex, unsigned int sizey, int num)
 *    - loader et bilde til tiles (med sizex x sizey-størrelse) inn i texturene i indexes (fra start og 
 *      oppover så langt som nødvendig).
 *    - filename filnavn, num intern identifikator for callback (automagisk 0).
 *
 * v0.0.3
 *  * LoadGL640x480Split (GLuint * indexes, char * filename, int num)
 *     - loader et 640x480-bilde og splitter det opp i 8 textures (for 1:1-visning i fullscreen).
 *     - textures skal inneholde 8 ledige texturedescriptorer (opengl)
 *     - filename er (naturlig nok) filnavnet, num er default 0 (sett til noe annet dersom du
 *       er interessert i en unik id ved callback (num blir sendt tilbake)).
 *
 * v0.0.2
 *  * callback for progressbar-ting (float prosent, int texnum (ved lasting av flere textures - ellers er denne 0))
 *
 * todo:
 *  * autocreation of textures regardless of size (padding, centering image, using transperancy)
 *  * JPEG grayscale .. BMP/TGA other formats?
 *  * texture-descriptor (lagre dill og dall til senere bruk for textures (width, height, bpp etc)
 *  * texture-effekter som kan kjøres inn i minnet. 
 */

#include "kvil.h"
extern "C" {
	#include "include/jpeglib.h"
}
#include <stdarg.h> 
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>



#include "include/png.h"

#define __DEBUG

/**[isExp]***********************************************
 * sjekker om 'answer' kan utredes fra en ren eksponent *
 * av 'fact' (f.eks. om 4 er et svar på en eksponent til*
 * 2 (2^2).												*
 ********************************************************/

bool is_exp(int fact, int answer) 
{
	int ans = fact;

	while (ans < answer) {
		ans *= fact;
	}

	if (ans == answer) return true;
	else return false;
}


/*
	konstruktør for kvil, kan bli brukt i framtiden
*/

kvil::kvil()
{
#ifdef KVIL_PROGRESS_CALLBACK
	this->scanline_callback = NULL;
#endif
}


/*
	dekonstruktør for selve kvil, fjerner bildedata.
*/

kvil::~kvil()
{
	if (this->data != NULL)
		delete this->data;
}


/*
	laster inn et BMP-bilde.
*/

void kvil::LoadBMP(FILE *fp, int num) 
{
	kvil_bmp_fileheader fh;
	kvil_bmp_infoheader ih;

	fread(&fh, 1, 14, fp);
	fread(&ih, 1, 40, fp);

#ifdef __DEBUG
	printf("(bmp) filesize: %d, width: %d, height: %d, bpp: %d\n", fh.file_size, ih.width, ih.height, ih.bpp);
#endif

	if ((ih.bpp == 24) && (ih.compression == 0)) {
		this->data = new unsigned char[ih.width * ih.height * (ih.bpp/8)];
		
		this->width = ih.width;
		this->height = ih.height;
		this->bpp = ih.bpp;

		unsigned int i;
		for (i = 0; i < (ih.width * ih.height); i++) {
			unsigned char buf[3];
			unsigned char tmp;
			fread(buf, 1, (ih.bpp / 8), fp);
			
			tmp = buf[2];
			buf[2] = buf[0];
			buf[0] = tmp;

			memcpy((this->data + i*3),buf, 3);
#ifdef KVIL_PROGRESS_CALLBACK
			if ((i%ih.height == 0) && (this->scanline_callback != NULL)) scanline_callback((float)floor((float)(i/ih.height) * 100 / (float)(ih.height)), num);
#endif
		}
	} else {
#ifdef __DEBUG
		printf("(bmp) format not supported, bpp: %d, compression: %d\n", ih.bpp, ih.compression);
#endif
	}
}

void kvil::DumpBMP(char * filename, int w, int h)
{
	unsigned int  * pixels;

	pixels = new unsigned int[640 * 480];

	memset(pixels, 0, 640*480*sizeof(int));

	glReadBuffer(GL_FRONT);
	glReadPixels(0,0, 640, 480, GL_RGB, GL_BYTE, pixels);

	WriteBMP(filename, w, h, (char *)pixels);

	delete pixels;
}

void kvil::DumpSequenceBMP(char * filename, int w, int h)
{
	char * fname = new char[strlen(filename)+10];
	static int frame = 0;

	sprintf(fname, "%s_%03d.bmp", filename, frame);

	DumpBMP(fname, w, h);

	delete fname;
	frame++;
}

void kvil::WriteBMP(char * filename, int w, int h, char * data)
{
	kvil_bmp_fileheader fh;
	kvil_bmp_infoheader ih;

	FILE * fp = fopen (filename, "wb");

	if (!fp)
	{
		char * message = new char[256];

		if (strlen(filename) > 200)
		{
			filename[200] = '\0';
		}

		sprintf(message, "Could not write file: %s", filename);

		MessageBox(NULL, message, "(kvil) WriteBMP", 0);

		delete message;
	}
	else
	{
		memset(&fh, 0, 14);
		memset(&ih, 0, 40);

		fh.file_size = 640 * 480 * 3;
		/* 0x4D42 = BM */
		fh.signature = 0x4D42;
		/* 0x36 = 54 */
		fh.dataoffset = 0x36;

		ih.bpp = 24;
		ih.compression = 0;
		ih.width = w;
		ih.height = h;
		ih.size_of_header = 40;

		fwrite(&fh, 1, 14, fp);
		fwrite(&ih, 1, 40, fp);

		int tmp;
		int i;

		for (i = 0; i < w*h*3; i+=3)
		{
			tmp = data[i+2];
			data[i+2] = data[i];
			data[i] = tmp;

			fwrite(&data[i], 1, 3, fp);
		}

		fclose(fp);
	}
}

/*
	laster inn et JPEG-bilde (via libjpeg).
*/

void kvil::LoadJPEG(FILE *fp, int num) 
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, fp);

	jpeg_read_header(&cinfo, TRUE);

	this->width = cinfo.image_width;
	this->height = cinfo.image_height;
	this->bpp = 24;

#ifdef __DEBUG
	printf ("(jpeg) width: %d, height: %d\n", this->width, this->height);
#endif

	jpeg_start_decompress(&cinfo);

	this->data = new unsigned char[this->width * this->height * (this->bpp / 8)];

	unsigned char * buf = new unsigned char[this->width * (this->bpp / 8)];

	int i = 0;

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &buf, 1);
		// leser nedenfra og oppover, så vi setter øverst i minnet først..
		memcpy(data + ((cinfo.output_height - cinfo.output_scanline) * (this->width*3)), buf, this->width*3);
		i++;
#ifdef KVIL_PROGRESS_CALLBACK
		if (this->scanline_callback != NULL) scanline_callback((float)floor((float)(i) * 100 / (float)(cinfo.output_height)), num);
#endif
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

/*
	laster inn et TGA-bilde.
*/

void kvil::LoadTGA(FILE *fp, int num) 
{
	kvil_tga_fileheader fh;

	fread(&fh, 1, 18, fp);

	this->width = fh.image_desc.width;
	this->height = fh.image_desc.height;
	this->bpp = fh.image_desc.bpp;

#ifdef __DEBUG
	printf("(tga) width: %d, height: %d, bpp: %d\n", this->width, this->height, this->bpp);
#endif

	if (this->bpp < 24) printf("(tga) WARNING: tga-loading for BPP < 24 not implemented!\n");

	if (this->scanline_callback != NULL) scanline_callback(0, num);

	fseek(fp, fh.idlength, SEEK_CUR);
	fseek(fp, fh.color_map.num_color_maps * (fh.color_map.bit_pr_map/8), SEEK_CUR);

	this->data = new unsigned char[this->width * this->height * (this->bpp / 8)];

	fread(this->data, 1, this->width * this->height * (this->bpp / 8), fp);

	unsigned int i;
	unsigned char tmp;

	for (i = 0; i < (this->width * this->height * (this->bpp / 8)); i+=(this->bpp / 8)) {
		tmp = this->data[i+2];
		this->data[i+2] = this->data[i];
		this->data[i] = tmp;
	}
	if (this->scanline_callback != NULL) scanline_callback(100, num);
}

/*
	laster inn et PNG-bilde (via libpng).
*/

void kvil::LoadPNG(FILE *fp, int num)
{
	png_uint_32 width, height, scanline;
	int bit_depth, color_type, interlace_type, row;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_infop end_info = png_create_info_struct(png_ptr);

	png_set_sig_bytes(png_ptr, 0);

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr,
			&end_info);
		
		fclose(fp);
		return;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	
	this->height = height;
	this->width = width;
	this->bpp = info_ptr->pixel_depth;

#ifdef __DEBUG
	printf("(png) width: %d, height: %d, bpp: %d\n", this->width, this->height, this->bpp);
#endif

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	scanline = png_get_rowbytes(png_ptr,info_ptr);

	this->data = new unsigned char[(this->height * this->width * (this->bpp / 8))];
	
	for(row = (this->height - 1);row >= 0; row--) {
		png_read_row(png_ptr,(this->data+(row*scanline)),NULL);
#ifdef KVIL_PROGRESS_CALLBACK
		if (this->scanline_callback != NULL) scanline_callback((float)floor((float)(height-row) * 100 / (float)(height)), num);
#endif
	}
	
	png_read_end(png_ptr,info_ptr);
		
	png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)NULL);
}

/*
	sørger for å laste inn et bilde til minnet før det
	sendes videre inn til opengl og frigjøres fra vanlig
	systemminne.
*/


void kvil::LoadGL(unsigned int texidx, char * filename, int num) 
{
#ifdef __DEBUG
	printf ("(kvil) loading file: %s into texture %d\n", filename, texidx);
#endif
	if (this->Load(filename, num)) {

		glBindTexture(GL_TEXTURE_2D, texidx);

		if (this->bpp == 24) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->width, this->height, 0, GL_RGB, GL_UNSIGNED_BYTE, this->data);
		} else if(this->bpp == 32) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->data);
		}

		delete this->data;
		this->data = NULL;

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}
}

void kvil::LoadGLSphereMap(unsigned int texidx, char * filename, int num) 
{
#ifdef __DEBUG
	printf ("(kvil) loading file: %s into texture %d as spheremap\n", filename, texidx);
#endif
	if (this->Load(filename, num)) {
		glBindTexture(GL_TEXTURE_2D, texidx);

		if (this->bpp == 24) {
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, this->width, this->height, GL_RGB, GL_UNSIGNED_BYTE, this->data);
		} else if(this->bpp == 32) {
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, this->data);
		}

		delete this->data;
		this->data = NULL;


		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	
	}
}

/*
	proxyfunksjon for lasting av flere bilder i samme
	Load-statement.
*/

void kvil::LoadGL(GLuint * indexes, unsigned int files, ...)
{
	unsigned int i;

	va_list argp;
	va_start(argp, files);
	
	for (i = 0; i < files; i++)
		this->LoadGL(indexes[i], va_arg(argp, char *), i);

	va_end(argp);
}

/*
	laster inn og splitter opp et bilde i ulike tiles
	som dyttes opp til opengl som egne texture-objekter.
*/

kvil::splitToGL(int idx, int width, int height, int startx, int starty, GLuint filtermode) {
	unsigned char * buffer = new unsigned char[width*height*(this->bpp/8)];

	int x, y, i;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			for (i = 0; i < (this->bpp/8); i++) {
				buffer[i +  x *(this->bpp/8) + y * width * (this->bpp/8)] = this->data[i + (startx + x)*(this->bpp/8) + (starty + y) * (this->bpp/8) * this->width];
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, idx);

	if (this->bpp == 24) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	} else if(this->bpp == 32) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtermode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtermode);

	delete buffer;
}

/*
	laster inn og splitter opp et bilde i passende
	størrelser slik at et bilde på 640x480 kan dekke
	hele skjermen. OpenGL støtter bare bilder i 2^m x 2^n
	i utgangspunktet, så bildet deles opp i åtte deler
	som alle er 2^m * 2^n. Dermed har man 640x480 i 1:1.
*/

void kvil::LoadGL640x480Split(GLuint * indexes, char * filename, int num)
{
#ifdef __DEBUG
	printf ("(kvil) loading file: %s into 8 textures, beginning at %d\n", filename, indexes[0]);
#endif

	if (this->Load(filename, num)) {
		if ((this->width == 640) && (this->height == 480)) {
			splitToGL(indexes[0], 512, 256, 0, 0, GL_NEAREST );
			splitToGL(indexes[1], 128, 256, 512, 0, GL_NEAREST );

			splitToGL(indexes[2], 512, 128, 0, 256, GL_NEAREST );
			splitToGL(indexes[3], 128, 128, 512, 256, GL_NEAREST );

			splitToGL(indexes[4], 512, 64, 0, 384, GL_NEAREST );
			splitToGL(indexes[5], 128, 64, 512, 384, GL_NEAREST );

			splitToGL(indexes[6], 512, 32, 0, 448, GL_NEAREST );
			splitToGL(indexes[7], 128, 32, 512, 448, GL_NEAREST );

#ifdef __DEBUG
			printf("(split640x480) did NOT free memory, free it yourself.\n");
#endif
			//delete this->data;
		} else {
#ifdef __DEBUG
			printf("(split640x480) texture is not the right dimension.\n");
#endif
		}
	} else {
		printf("(split640x480) couldnt load file.\n");
	}
}

/*
	laster inn og splitter opp tiles ved å bruke den
	tidligere nevnte funksjonen for å splitte opp.
	denne baserer seg på at alle tiles skal være like
	store (selve splitToGL kan ta vilkårlige størrelser)
*/

void kvil::LoadMultipleGL(GLuint * indexes, char * filename, unsigned int sizex, unsigned int sizey, int num) {
#ifdef __DEBUG
	printf ("(kvil) loading file: %s for splitting into textures.\n", filename, indexes[0]);
#endif

	if (this->Load(filename, num)) {
		if ((this->width%sizex == 0) && (this->height%sizey == 0) && (is_exp(2, sizex)) && (is_exp(2, sizey))) {
			unsigned int x,y;
			for (y = 0; y < (this->height / sizey); y++) {
				for (x = 0; x < (this->width / sizex);x++) {
					splitToGL(indexes[x + y * (this->width / sizex)], sizex, sizey, x * sizex, (this->height - sizey) - y * sizey );
				}
			}
		} else {
#ifdef __DEBUG
			printf ("(kvil_multiple_gl) wrong dimensions (every tile must be 2^x*2^y sized)\n");
#endif
		}
	} else {
#ifdef __DEBUG
		printf ("(kvil_multiple_gl) could not load file %d\n", num);
#endif

	}
}

/*
	returnerer størrelsen på en fil. brukes ikke.
	(skal brukes til gjetting av dimensjoner på raw)
*/

unsigned int filesize(char * filename) {
	struct _stat buf;
	_stat( filename, &buf );
	return buf.st_size;
}

void kvil::LoadRAWGL(unsigned int texidx, char * filename, unsigned int x, unsigned int y, int num) {
	FILE *fp = fopen (filename, "rb");

	if ((x * y * 4) == filesize(filename)) {
#ifdef __DEBUG
		printf("(raw) loading %d * %d * 32bpp\n", x, y);
#endif
		this->data = new unsigned char [x*y*4];
	}
}

/*
	laster inn et enkelt bilde til objektets data-område.
	brukes av andre funksjoner etter hvor ting skal
	lastes. Brukes direkte om dataene skal brukes et
	annet sted i applikasjonen (ikke inn i OpenGL)
*/

bool kvil::Load(char * filename, int num)
{
	FILE *fp = fopen(filename, "rb");

	bool loaded = false;

	if (fp) {
		char buf[20];
		fread(buf, 1, 10, fp);
		rewind(fp);

		if ((buf[0] == 'B') && (buf[1] == 'M')) {
			this->LoadBMP(fp, num);
			loaded = true;
		} else if ((buf[6] == 'J') && (buf[7] == 'F') && (buf[8] == 'I') && (buf[9] == 'F')) {
			this->LoadJPEG(fp, num);
			loaded = true;
		} else if (png_sig_cmp((unsigned char *)&buf[0], 0, 8) == 0) {
			this->LoadPNG(fp, num);
			loaded = true;
		} else {
			fseek(fp, -18, SEEK_END);
			fread(buf, 1, 18, fp);
			if (strcmp(buf, "TRUEVISION-XFILE.") == 0) {
				rewind(fp);
				this->LoadTGA(fp, num);
				loaded = true;
			}
		}
		fclose(fp);
	} else {
#ifdef __DEBUG
		printf ("(kvil) sorry, couldnt open %s!\n", filename);
#endif
		char * message = new char[256];

		if (strlen(filename) > 200)
		{
			filename[200] = '\0';
		}

		sprintf(message, "Could not open file: %s", filename);

		MessageBox(NULL, message, "(kvil) Load", 0);

		delete message;
	}
	return loaded;
}

