#include "lib/glwin.h"
#include "lib/kvil.h"
#include "mcubes.h"
#include "metaballs.h"
#include <math.h>
#include <stdio.h>
#include <windows.h>

/* pekere til klassene som brukes */
glwin *win;
kvil *kv;
mcubes *mc;
mcubes_metaballs *mb;

/* et array som inneholder texture-idene våre */
unsigned int textures[1];

/* farge som ting skal cleares til */
GLfloat clearcolor[4];
bool just_pressed[256];

void init(w_info &info)
{
	/* generer unike ider til texturen(e) */
	glGenTextures(1, textures);

	/* sett default clearfarge */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	/* tøm bufferet */
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	/* defaultenabling av lyssetting */
	glEnable(GL_LIGHTING);

	/* sett opp lys */
	GLfloat position [] = { 0.0f, 0.0f, -20.0f, 1.0f };
	GLfloat ambient [] = { 0.0f, 0.3f, 0.3f, 1.0f };
	GLfloat diffuse [] = { 1.0f, 1.0f, 1.0f, 1.0f };

	/* gi parametre til opengl */
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

	/* slå på lys0 */
	glEnable(GL_LIGHT0);

	/* enable texturing */
	glEnable(GL_TEXTURE_2D);

	/* bruk første texture */
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	/* hurra! vi vil ha autogenerering av texturekoordinater, kort og godt fordi det blir et helvette å forsøke å regne de ut ellers */
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	/* last inn et bilde til textureid 0 */
	kv->LoadGLSphereMap(textures[0], "env_sphere.bmp");

	/* vi vil ha autonormalisering av vektorer, slik at vi slipper å gjøre dette manuelt */
	glEnable(GL_NORMALIZE);

	/* initialiserer default clearing-farge */
	clearcolor[0] = 0.0f;
	clearcolor[1] = 0.0f;
	clearcolor[2] = 0.0f;
	clearcolor[3] = 1.0f;

	/* setter parametre for tåke, dersom det blir enablet */
	GLfloat fogColor[4]= {1.0f, 0.0f, 0.0f, 1.0f};	
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.1f);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, 10.0f);
	glFogf(GL_FOG_END, 30.0f);


	/* initialiserer statusarrayet for tastetrykk */
	int i;

	for (i = 0; i < 256; i++)
	{
		just_pressed[i] = false;
	}
}

/* statusvariabler for tåke, lys, wireframe og teksturer */
bool fog, light, wireframe, texturing;

/* parametre som justerer rotasjon og bevegelseshastighet */
double movement_factor = 0.01f;
double rotation_x = 0.0f;
double rotation_y = 0.0f;

void checkKeys()
{
	/* sjekk om 'f' har blitt trykt */
	if (GetAsyncKeyState(0x46) && !just_pressed[0x46])
	{
		just_pressed[0x46] = true;

		fog = !fog;

		if (fog)
		{
			/* setter rød bakgrunnsfarge (litt enklere å se den faktiske tåka da */
			glEnable(GL_FOG);
			clearcolor[0] = 1.0f;
			clearcolor[1] = 0.0f;
			clearcolor[2] = 0.0f;
			clearcolor[3] = 1.0f;
		}
		else
		{
			/* setter default svart bakgrunnsfarge og skrur av tåkelegging */
			clearcolor[0] = 0.0f;
			clearcolor[1] = 0.0f;
			clearcolor[2] = 0.0f;
			clearcolor[3] = 1.0f;
			glDisable(GL_FOG);
		}
	}
	else if (!GetAsyncKeyState(0x46))
	{
		just_pressed[0x46] = false;
	}

	/* sjekk om 'l' er blitt trykt */
	if (GetAsyncKeyState(0x4C) && !just_pressed[0x4C])
	{
		just_pressed[0x4C] = true;

		light = !light;

		if (light)
		{
			glEnable(GL_LIGHTING);
		}
		else
		{
			glDisable(GL_LIGHTING);
		}
	}
	else if (!GetAsyncKeyState(0x4C))
	{
		just_pressed[0x4C] = false;
	}

	/* sjekk om 't' er blitt trykt */
	if (GetAsyncKeyState(0x54) && !just_pressed[0x54])
	{
		just_pressed[0x54] = true;

		texturing = !texturing;

		if (texturing)
		{
			glEnable(GL_TEXTURE_2D);
		}
		else
		{
			glDisable(GL_TEXTURE_2D);
		}
	}
	else if (!GetAsyncKeyState(0x54))
	{
		just_pressed[0x54] = false;
	}

	/* sjekk hvorvidt 'w' er blitt trykt */
	if (GetAsyncKeyState(0x57) && !just_pressed[0x57])
	{
		just_pressed[0x57] = true;

		wireframe = !wireframe;

		if (wireframe)
		{
			mc->setWireframe(true);
		}
		else
		{
			mc->setWireframe(false);
		}
	}
	else if (!GetAsyncKeyState(0x57))
	{
		just_pressed[0x57] = false;
	}

	/* rotasjonsparametre for scenen */
	if (GetAsyncKeyState(VK_UP))
	{
		rotation_x += 1;
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		rotation_x -= 1;
	}
	if (GetAsyncKeyState(VK_LEFT))
	{
		rotation_y -= 1;
	}
	if (GetAsyncKeyState(VK_RIGHT))
	{
		rotation_y += 1;
	}

	glClearColor(clearcolor[0], clearcolor[1], clearcolor[2], clearcolor[3]);
}

void render(w_info &info)
{
	/* sjekk om noen taster har blitt trykt */
	checkKeys();

	/* tøm bufferene */
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	/* sett opp fint viewpoint */
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -20.0f);

	/* gjør litt movement av ballene */
	double static movement = 0.0;
	movement += (0.01);

	/* roter til vinklen brukeren ønsker å se ting fra */
	glRotatef((float)rotation_x, 1.0f, 0.0f, 0.0f);
	glRotatef((float)rotation_y, 0.0f, 1.0f, 0.0f);

	/* flytt de faktiske ballene */
	mb->move_ball(0, cos(movement) * movement_factor, sin(movement)*movement_factor, cos(movement*2)*movement_factor);
	mb->move_ball(1, sin(movement) * movement_factor, cos(movement)*movement_factor, sin(movement*2)*movement_factor);

	/* regn ut hva som er innenfor og hva som er utenfor */
	mc->computeMetaBalls();
	
	/* tegn */
	mc->draw();

	/* dersom det dumpes til en fil, så brukes dette - se kvil for nærmere detaljer */
	//kv->DumpSequenceBMP("f:\\dump", 640, 480);
}

int main() {
	int e_code = 0; 

	win = new glwin(640,480,32,16,false,"Marching Cubes","");
	kv = new kvil();

	mb = new mcubes_metaballs();

	mc = new mcubes(-10, 10, 10, 10, -10, -10, 0.7, -0.7, -0.7);
	mc->setMetaBalls(mb, 0.3f);

	try {
		win->SetInitCallback(init);
		win->SetRenderCallback(render);

		e_code = win->go();
	}
	catch(char *s)
	{
		MessageBox(0,s,"ERROR!",MB_ICONEXCLAMATION);
		return 1;	
	}

	return 0;
}