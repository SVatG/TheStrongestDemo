/**[FILE]*********************************************************
	author		: Morten Hansen (morten@devnull.no)
	started		: 16:48:40 18/03/02

	filename	: glwin.cpp
	description	: glwin implementation
 *****************************************************************/
#include "glwin.h"

LRESULT CALLBACK WndProc(HWND hWnd,UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam,LPARAM lParam);

BOOL glwin::g_running = TRUE;
w_info glwin::g_info;

glwin::glwin(VOID)
{
	// set default values
	g_info.width = 640;
	g_info.height = 480;
	g_info.bpp = 32;
	g_info.zbuffer = 16;
	g_info.fullscreen = TRUE;
	g_info.sound = TRUE;
	
	g_info.view.fov = 50.0f;
	g_info.view.nearPlane = 0.0f;
	g_info.view.farPlane = 1000.0f;

	m_buffer = new char[1200];
	m_setup = FALSE;

	sprintf(g_info.name,"default");
}

glwin::glwin(DWORD width, DWORD height, DWORD bpp, DWORD zbuffer, BOOL fullscreen,CHAR *title,CHAR *cmd)
{
	g_info.width = width;
	g_info.height = height;
	g_info.bpp = bpp;
	g_info.zbuffer = zbuffer;
	g_info.fullscreen = fullscreen;

	g_info.view.fov = 50.0f;
	g_info.view.nearPlane = 0.5f;
	g_info.view.farPlane = 1000.0f;

	m_buffer = new char[1200];
	m_setup = FALSE;

	sprintf(g_info.cmd,"%s",cmd);
	sprintf(g_info.name,"%s",title);
}

glwin::~glwin(VOID)
{
	delete [] m_buffer;
}

BOOL glwin::SendCommandLine(LPSTR strCmd)
{
	BOOL help = false;
			
	if(strlen(strCmd)>0) 
	{
		help = true;
	}

	if(help) {
		sprintf(m_buffer,
		    "Demo parameters (*NOT* IMPLEMENTED)\n\n"
		    "[-windowed] run windowed                 \n"
		    "[-fullscreen] run fullscreen             \n"
		    "[-z16] 16bits zbuffer                    \n"
		    "[-z32] 32bits zbuffer                    \n"
		    "[-16] 16bits graphics mode               \n"
		    "[-32] 32bits graphics mode               \n"
		    "[-640x480] graphics mode 640x480         \n"
		    "[-800x600] graphics mode 800x600         \n"
		    "[-1024x768] graphics mode 1024x768       \n"
		    "[-1280x1024] graphics mode 1280x1024     \n"
		);

		MessageBox(HWND_DESKTOP,m_buffer,"commandline help",MB_OK|MB_ICONINFORMATION);
	}
	
	return help;
}

BOOL glwin::SetPixelFormat(VOID)
{
	int index;

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		g_info.bpp,
		0,0,0,0,0,
		1,
		0,
		0,
		0,0,0,0,
		g_info.zbuffer,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0,0,0	
	};
	
	index = ::ChoosePixelFormat(m_hdc,&pfd);
	if(!::SetPixelFormat(m_hdc,index,&pfd))
	{
		return FALSE;
	}
	
	return TRUE;	
}

BOOL glwin::SetFullscreen(VOID)
{
	DEVMODE dm;
	memset(&dm,0,sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);
	dm.dmPelsWidth = g_info.width;
	dm.dmPelsHeight = g_info.height;
	dm.dmBitsPerPel = g_info.bpp;
	dm.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;	

	if(ChangeDisplaySettings(&dm,CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		return FALSE;	
	}
	
	return TRUE;	
}

VOID glwin::SetView(VOID)
{
	::glViewport(0,0,g_info.width,g_info.height);
	::glMatrixMode(GL_PROJECTION);
	::glLoadIdentity();
	::gluPerspective(g_info.view.fov,(GLfloat)g_info.width/(GLfloat)g_info.height,g_info.view.nearPlane,g_info.view.farPlane);
	::glMatrixMode(GL_MODELVIEW);
	::glLoadIdentity();
}

double glwin::SetTimer(double &timeMultiplier)
{
	double framecount=0.0f;

	m_timePrevFrame.QuadPart = m_timeThisFrame.QuadPart;
	QueryPerformanceCounter(&m_timeThisFrame);

	if(m_timeFrequency != 0 && m_timeThisFrame.QuadPart-m_timePrevFrame.QuadPart != 0) {
		timeMultiplier = (m_timeThisFrame.QuadPart-m_timePrevFrame.QuadPart)/m_timeFrequency;

		if(timeMultiplier>1) {
			timeMultiplier = 0.0000001;
		}

		double tempDiv = (m_timeThisFrame.QuadPart-m_timePrevFrame.QuadPart)/m_timeFrequency;
		if(tempDiv != 0) framecount = 1/tempDiv;
	}

	return framecount;
}

void glwin::InitTimer(VOID) {
	QueryPerformanceFrequency(&m_countsPerSec);
	m_timeFrequency = (double)m_countsPerSec.QuadPart;
}

DWORD glwin::go(VOID)
{
	DWORD style = WS_BORDER|WS_SYSMENU;
	DWORD exStyle = WS_EX_APPWINDOW;
	
	if(SendCommandLine(g_info.cmd))
	{
		return 1;	
	}

	if(m_setup)
	{
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETUP),0,DLGPROC(DlgProc));
		PeekMessage(&m_msg,NULL,0,0,PM_REMOVE);
	}
	
	if(!glwin::g_running)
	{
		return 1;	
	}

	RECT wndRegion = { 0,0,g_info.width,g_info.height };
	WNDCLASSEX wc;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	sprintf(m_buffer,"(%s) -- [%ux%u^%u^%u]",g_info.name,g_info.width,g_info.height,g_info.bpp,g_info.zbuffer);
	strcpy(g_info.name,m_buffer);

	memset(&wc,0,sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL,IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hInstance = hInstance;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "glwin";

	if(g_info.fullscreen)
	{
		style = WS_POPUP;
		exStyle |= WS_EX_TOPMOST;
		ShowCursor(FALSE);
		
		SetFullscreen();
	} else {
		exStyle |= WS_EX_WINDOWEDGE;
	}

	if(!RegisterClassEx(&wc))
	{
		MessageBox(HWND_DESKTOP,"Could not register class","ERROR!",MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	AdjustWindowRectEx(&wndRegion,style,0,exStyle);

	g_info.hwnd = CreateWindowEx(exStyle,
				"glwin",
				g_info.name,
				style|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
				0,0,
				wndRegion.right-wndRegion.left,
				wndRegion.bottom-wndRegion.top,
				HWND_DESKTOP,
				0,
				hInstance,
				0);

	if(g_info.hwnd==NULL) return 1;

	m_hdc = GetDC(g_info.hwnd);
	if(!m_hdc)
	{
		MessageBox(HWND_DESKTOP,"Could not get devicecontext","ERROR!",MB_OK | MB_ICONEXCLAMATION);
		return 1;	
	}
	
	SetPixelFormat();
	
	m_hrc = wglCreateContext(m_hdc);
	if(!m_hrc)
	{
		MessageBox(HWND_DESKTOP,"Creation of OpenGL context failed!","ERROR!",MB_OK | MB_ICONEXCLAMATION);
		return 1;	
	}
	
	if(!wglMakeCurrent(m_hdc,m_hrc))
	{
		MessageBox(HWND_DESKTOP,"Could not make OpenGL context active!","ERROR!",MB_OK | MB_ICONEXCLAMATION);
		return 1;	
	}

	ShowWindow(g_info.hwnd,SW_NORMAL);
	SetForegroundWindow(g_info.hwnd);
	SetFocus(g_info.hwnd);

	SetView();

	glClearColor(0.0f,0.0f,0.0f,1.0f);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

	InitTimer();
	m_init(g_info);

	while(glwin::g_running)
	{
		if(PeekMessage(&m_msg,NULL,0,0,PM_REMOVE) != 0) 
		{
			if(m_msg.message != WM_QUIT)
			{
				DispatchMessage(&m_msg);
			} else 	{
				glwin::g_running = false;
			}
		} else {
			m_fps = SetTimer(m_time);
			m_render(g_info);

			if(!g_info.fullscreen)
			{
				sprintf(m_buffer,"%s [fps: %5.2f]",g_info.name,m_fps);
				SetWindowText(g_info.hwnd,m_buffer);
			}

			SwapBuffers(m_hdc);
		}
	}

	if(g_info.fullscreen)
	{
		ChangeDisplaySettings(NULL,0);
		ShowCursor(TRUE);
	}

	wglMakeCurrent(m_hdc,NULL);
	wglDeleteContext(m_hrc);

	ReleaseDC(g_info.hwnd,m_hdc);
	DestroyWindow(g_info.hwnd);

	UnregisterClass("glwin",hInstance);

	return m_msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_KEYUP:
		case WM_KEYDOWN:
		{
			if(wParam == VK_ESCAPE) PostQuitMessage(0);	
			return 0;
		}
	}
	
	return DefWindowProc(hWnd,message,wParam,lParam);
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam,LPARAM lParam)
{
	HWND hFullscreen = GetDlgItem(hWnd,IDC_FULLSCREEN);
	HWND hSound = GetDlgItem(hWnd,IDC_SOUND);

	HWND hZbuffer = GetDlgItem(hWnd,IDC_ZBUFFER);
	HWND hBpp = GetDlgItem(hWnd,IDC_COLMODE);
	
	HWND hRes1 = GetDlgItem(hWnd,IDC_RESOLUTION1);
	HWND hRes2 = GetDlgItem(hWnd,IDC_RESOLUTION2);
	HWND hRes3 = GetDlgItem(hWnd,IDC_RESOLUTION3);
	HWND hRes4 = GetDlgItem(hWnd,IDC_RESOLUTION4);
	
	switch(message)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hRes1,BM_SETCHECK,wParam,0);
			SendMessage(hSound,BM_SETCHECK,wParam,0);
			
			if(glwin::g_info.fullscreen)
			{
				SendMessage(hFullscreen,BM_SETCHECK,wParam,0);
			}
			
			if(glwin::g_info.sound)
			{
				SendMessage(hSound,BM_SETCHECK,wParam,0);
			}

			if(glwin::g_info.bpp==32)
			{
				SendMessage(hBpp,BM_SETCHECK,wParam,0);	
			}

			if(glwin::g_info.zbuffer==32)
			{
				SendMessage(hZbuffer,BM_SETCHECK,wParam,0);	
			}

			break;	
		}
		case WM_CLOSE:
		{
			glwin::g_running = FALSE;
			PostQuitMessage(0);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_START:
				{
					if(SendMessage(hFullscreen,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.fullscreen = TRUE;
					} else {
						glwin::g_info.fullscreen = FALSE;
					}
					
					if(SendMessage(hSound,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.sound = TRUE;
					} else {
						glwin::g_info.sound = FALSE;
					}
					
					if(SendMessage(hBpp,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.bpp = 32;	
					} else {
						glwin::g_info.bpp = 16;
					}

					if(SendMessage(hZbuffer,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.zbuffer = 32;
					} else {
						glwin::g_info.zbuffer = 16;
					}
					
					if(SendMessage(hRes1,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.width = 640;
						glwin::g_info.height = 480;
					}

					if(SendMessage(hRes2,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.width = 800;
						glwin::g_info.height = 600;						
					}

					if(SendMessage(hRes3,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.width = 1024;
						glwin::g_info.height = 768;
					}

					if(SendMessage(hRes4,BM_GETCHECK,wParam,0)==BST_CHECKED)
					{
						glwin::g_info.width = 1280;
						glwin::g_info.height = 1024;
					}
					
					PostQuitMessage(0);
					break;
				}
				case IDC_ABORT:
				{
					glwin::g_running = FALSE;
					PostQuitMessage(0);
					break;
				}
				case IDC_ABOUT:
				{
					char *about = new char[400];
					
					MessageBox(hWnd,
						"Kvasigen demosystem\n\nv0.1.5\n\n(c) 2002\n\n[logo by flipside^quazim]",
						"About",MB_OK|MB_ICONINFORMATION);
					
					delete [] about;
										
					break;	
				}
			}
	}

	return FALSE;
}