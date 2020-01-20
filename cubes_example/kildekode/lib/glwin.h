/**[FILE]*********************************************************
	author		: Morten Hansen (morten@devnull.no)
	started		: 16:48:25 18/03/02

	filename	: glwin.h
	description	: glwin declaration
 *****************************************************************/
#ifndef __GLWIN_H
#define __GLWIN_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>

#include <gl/gl.h>
#include <gl/glu.h>

#include "resource.h"

typedef struct w_view_t {
	float fov;
	float nearPlane;
	float farPlane;
} w_view;

typedef struct w_info_t {
	CHAR name[64];
	CHAR cmd[256];
	DWORD width;
	DWORD height;
	DWORD bpp;
	DWORD zbuffer;
	BOOL fullscreen;
	BOOL sound;
	w_view view;

	HWND hwnd;
} w_info;

class glwin
{
	public:
		glwin(VOID);
		glwin(DWORD width, DWORD height, DWORD bpp, DWORD zbuffer, BOOL fullscreen, CHAR *title,CHAR *cmd=NULL);
		~glwin(VOID);

		VOID SetFullscreen(BOOL value)
		{
			g_info.fullscreen = value; 
		}

		VOID SetResolution(DWORD width, DWORD height)
		{ 
			g_info.width = width; 
			g_info.height = height; 
		}

		VOID SetBPP(DWORD bpp)
		{
			g_info.bpp = bpp; 
		}

		VOID SetZBuffer(DWORD zbuffer)
		{ 
			g_info.zbuffer = zbuffer; 
		};

		BOOL SendCommandLine(LPSTR strCmd);

		VOID SetInitCallback(VOID (*callback)(w_info&))
		{
			m_init = callback;	
		}

		VOID SetRenderCallback(VOID (*callback)(w_info&))
		{
			m_render = callback;
		}

		double SetTimer(double &timeMultiplier);
		VOID SetupDialog(VOID)
		{
			m_setup = TRUE;	
		}
		
		DWORD go(VOID);
	public:
		static BOOL g_running;
		static w_info g_info;
	private:
		BOOL SetPixelFormat(VOID);
		BOOL SetFullscreen(VOID);
		VOID SetView(VOID);
		VOID InitTimer(VOID);
	private:
		CHAR *m_buffer;
		BOOL m_setup;
		
		VOID (*m_init)(w_info&);
		VOID (*m_render)(w_info&);

		MSG m_msg;
		HDC m_hdc;
		HGLRC m_hrc;

		double m_timeFrequency;
		double m_fps;
		double m_time;

		LARGE_INTEGER m_timeThisFrame;
		LARGE_INTEGER m_timePrevFrame;
		LARGE_INTEGER m_countsPerSec;
};

#endif // __GLWIN_H