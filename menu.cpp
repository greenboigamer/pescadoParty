// edwin aviles, kian hernando, & simon

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <iostream>
#include <random>
#include "fonts.h"
#include "image.h"

using namespace std;

random_device rd;
mt19937 gen(rd());


enum GameState {
	MENU,
	PLAY,
	FISHING,
	CHARACTER
};

GameState gameState = MENU;

Image img[10] = {"./assets/images/fish.jpg", "./assets/images/background_fishing.png", 
    "./assets/images/senorpescado.png", "./assets/images/logo.png", "./assets/images/boat.png", 
    "./assets/images/milking_fish.png", "./assets/images/reynboh_pescado.png", 
    "./assets/images/death_snapper.png", "./assets/images/exo_trout.png",
    "./assets/images/grieselly_fish.png"};

// for boat machanics 
float boatBobTime = 0.5f;
float boatBobAmp = 3.0f;
float boatBobSpeed = 0.20f;

// for milk fish machanics 
float milkFishBobTime = 0.2f;
float milkFishBobAmp = 5.0f;
float milkFishBobSpeed = 0.50f;

// for milk fish side-to-side movement
float fishX = 0.0f;        // initialized after g is ready
float fishSpeedX = 2.5f;   // pixels per frame, adjust for faster/slower
bool fishFacingRight = true;

// for reynboh pescado bob and movement
float reynbohBobTime = 0.0f;
float reynbohBobAmp = 4.0f;
float reynbohBobSpeed = 0.35f;
float reynbohX = 0.0f;
float reynbohSpeedX = 3.5f;
bool reynbohFacingRight = false;

// for death snapper bob and movement
float deathSnapperBobTime = 0.1f;
float deathSnapperBobAmp = 6.0f;
float deathSnapperBobSpeed = 0.45f;
float deathSnapperX = 0.0f;
float deathSnapperSpeedX = 4.5f;
bool deathSnapperFacingRight = true;

// for exo trout bob and movement
float exoTroutBobTime = 0.3f;
float exoTroutBobAmp = 3.5f;
float exoTroutBobSpeed = 0.28f;
float exoTroutX = 0.0f;
float exoTroutSpeedX = 3.0f;
bool exoTroutFacingRight = true;

// for grieselly fish bob and movement
float griesellyBobTime = 0.6f;
float griesellyBobAmp = 5.5f;
float griesellyBobSpeed = 0.40f;
float griesellyX = 0.0f;
float griesellySpeedX = 5.0f;
bool griesellyFacingRight = false;

class Texture {
public:
	Image *backImage;
	GLuint backTexture;
	float xc[2];
	float yc[2];
};

class Box {
    public:
        Box(float w, float h);
        float pos[2];
        float width, height;
        unsigned char color[3];
        Image *boxImage;
        GLuint texture;
        float xc[2];
        float yc[2];
};

Box box(800/2, 120);
Box top(800/2, 35);
Box selection1(90, 65);
Box selection2(90, 65);
Box selection3(90, 65);

Box::Box(float w, float h)
{
    width = w;
    height = h;
    pos[0] = 800/2;
    pos[1] = 30;
    color[0] = 0;
    color[1] = 255;
    color[2] = 240;
    boxImage = nullptr;
    texture = 0;
    xc[0] = 0.0;
    xc[1] = 1.0;
    yc[0] = 0.0;
    yc[1] = 1.0;
}

class Global {
public:
	int xres, yres;
	Texture tex;
	GLuint fishingTex; //play background
	GLuint pescadoTex; //spinning senor pescado
	GLuint partyTex; // pescado party logo
	GLuint boatTex;
	GLuint fishOneTex;
	GLuint reynbohTex;
	GLuint deathSnapperTex;
	GLuint exoTroutTex;
	GLuint griesellyTex;
	float logoAngle;
	Global() {
		xres=640, yres=480;
		fishingTex = 0;
		pescadoTex = 0;
		logoAngle = 0.0f;
		partyTex = 0;
		boatTex = 0;
		fishOneTex = 0;
		reynbohTex = 0;
		deathSnapperTex = 0;
		exoTroutTex = 0;
		griesellyTex = 0;
	}
} g;

const int numOptions = 3;
const char* menuSelect[numOptions] = {
	"Play",
	"Select Character",
	"Quit",
};

static const int menuCount = (int)(sizeof(menuSelect) / sizeof(menuSelect[0]));
static int menuSelected = 0;
void select_menu_option(int i);


class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	X11_wrapper() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
		setup_screen_res(800, 640);
		dpy = XOpenDisplay(NULL);
		if(dpy == NULL) {
			printf("\n\tcannot connect to X server\n\n");
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if(vi == NULL) {
			printf("\n\tno appropriate visual found\n\n");
			exit(EXIT_FAILURE);
		}
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		XSetWindowAttributes swa;
		swa.colormap = cmap;
		swa.event_mask =
			ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
			ButtonPressMask | ButtonReleaseMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
								vi->depth, InputOutput, vi->visual,
								CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	void cleanupXWindows() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	void setup_screen_res(const int w, const int h) {
		g.xres = w;
		g.yres = h;
	}
	void reshape_window(int width, int height) {
		//window has been resized.
		setup_screen_res(width, height);
		glViewport(0, 0, (GLint)width, (GLint)height);
		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW); glLoadIdentity();
		glOrtho(0, g.xres, 0, g.yres, -1, 1);
		set_title();
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "Pescado");
	}
	bool getXPending() {
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
	void check_resize(XEvent *e) {
		//The ConfigureNotify is sent by the
		//server if the window is resized.
		if (e->type != ConfigureNotify)
			return;
		XConfigureEvent xce = e->xconfigure;
		if (xce.width != g.xres || xce.height != g.yres) {
			//Window size did change.
			reshape_window(xce.width, xce.height);
		}
	}
} x11;

void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);


//===========================================================================
//===========================================================================
int main()
{
	init_opengl();
	int done=0;
	while (!done) {
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			check_mouse(&e);
			done = check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
	}
	return 0;
}

void init_opengl(void)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//This sets 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Clear the screen
	glClearColor(1.0, 1.0, 1.0, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//Do this to allow texture maps
	glEnable(GL_TEXTURE_2D);
	//
	//load the images file into a ppm structure.
	//
	g.tex.backImage = &img[0];
	//create opengl texture elements
	// menu background
	glGenTextures(1, &g.tex.backTexture);
	int w = g.tex.backImage->width;
	int h = g.tex.backImage->height;
	glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
							GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);

	g.tex.xc[0] = 0.0;
	g.tex.xc[1] = 0.25;
	g.tex.yc[0] = 0.0;
	g.tex.yc[1] = 1.0;

	//play state background
	glGenTextures(1, &g.fishingTex);
	int pw = img[1].width;
	int ph = img[1].height;
	glBindTexture(GL_TEXTURE_2D, g.fishingTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, pw, ph, 0,
				GL_RGB, GL_UNSIGNED_BYTE, img[1].data);

	//pescado spinning logo
	unsigned char *alphaDataPescado = buildAlphaData(&img[2]);
	glGenTextures(1, &g.pescadoTex);
	glBindTexture(GL_TEXTURE_2D, g.pescadoTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
				img[2].width, img[2].height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, alphaDataPescado);
	free(alphaDataPescado);

	//pescado party logo
	unsigned char *alphaDataParty = buildAlphaData(&img[3]);
	glGenTextures(1, &g.partyTex);
	glBindTexture(GL_TEXTURE_2D, g.partyTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[3].width, img[3].height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataParty);


	free(alphaDataParty);


	unsigned char *alphaDataBoat = buildAlphaData(&img[4]);
	glGenTextures(1, &g.boatTex);
	glBindTexture(GL_TEXTURE_2D, g.boatTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[4].width, img[4].height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataBoat);

	free(alphaDataBoat);

    //milking fish
	unsigned char *alphaDataMilkingFish = buildAlphaData(&img[5]);
	glGenTextures(1, &g.fishOneTex);
	glBindTexture(GL_TEXTURE_2D, g.fishOneTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[5].width, img[5].height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataMilkingFish);

	free(alphaDataMilkingFish);

	//reynboh pescado
	unsigned char *alphaDataReynboh = buildAlphaData(&img[6]);
	glGenTextures(1, &g.reynbohTex);
	glBindTexture(GL_TEXTURE_2D, g.reynbohTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[6].width, img[6].height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataReynboh);
	free(alphaDataReynboh);

	//death snapper
	unsigned char *alphaDataDeathSnapper = buildAlphaData(&img[7]);
	glGenTextures(1, &g.deathSnapperTex);
	glBindTexture(GL_TEXTURE_2D, g.deathSnapperTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[7].width, img[7].height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataDeathSnapper);
	free(alphaDataDeathSnapper);

	//exo trout
	unsigned char *alphaDataExoTrout = buildAlphaData(&img[8]);
	glGenTextures(1, &g.exoTroutTex);
	glBindTexture(GL_TEXTURE_2D, g.exoTroutTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[8].width, img[8].height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataExoTrout);
	free(alphaDataExoTrout);

	//grieselly fish
	unsigned char *alphaDataGrieselly = buildAlphaData(&img[9]);
	glGenTextures(1, &g.griesellyTex);
	glBindTexture(GL_TEXTURE_2D, g.griesellyTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[9].width, img[9].height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataGrieselly);
	free(alphaDataGrieselly);

	initialize_fonts();
	fishX         = g.xres * 0.8f;
	reynbohX      = g.xres * 0.2f;
	deathSnapperX = g.xres * 0.5f;
	exoTroutX     = g.xres * 0.65f;
	griesellyX    = g.xres * 0.35f;
}

//Random Gen for mouseClicks
int randGen()
{
    std::uniform_int_distribution<> dis(10, 20);
    int randNum = dis(gen);

    return randNum;
}

int threshold()
{
    int limit = randGen();
    return limit;
}

void check_mouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
    static int mouseClicks = 0;
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
            printf("click \n");
            fflush(stdout);
            if (gameState == PLAY) {
                mouseClicks++;
                if (mouseClicks == 1) {
			        printf("Fishing Game Started\n");
                    gameState = FISHING;
                }
                if (mouseClicks == 10) {
                    printf("threshold met! \n");
                    fflush(stdout);
                    mouseClicks = 0;
                    int amount = threshold();
                    printf("%i\n", amount);
                    fflush(stdout);
                    //amount = 0;
                }
            }
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	if (savex != e->xbutton.x || savey != e->xbutton.y) {
		//Mouse moved
		savex = e->xbutton.x;
		savey = e->xbutton.y;
	}
}

int check_keys(XEvent *e)
{
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		if (key == XK_Escape) {
			return 1;
		}
		if (key == XK_Up || key == XK_w){
			menuSelected = (menuSelected - 1 + menuCount) % menuCount;
		}
		if (key == XK_Down || key == XK_s){
			menuSelected = (menuSelected + 1 + menuCount) % menuCount;
		}
		if (key == XK_Return || key == XK_space){
			select_menu_option(menuSelected);
		}

	}
	return 0;
}

void physics()
{
	if (gameState == MENU) {
        g.tex.xc[0] += 0.001;
        g.tex.xc[1] += 0.001;

		g.logoAngle += 3.0f;          // pescado spin speed
    	if (g.logoAngle >= 360.0f)
        	g.logoAngle -= 360.0f;
	}

	if (gameState == PLAY || gameState == FISHING) {
		boatBobTime         += boatBobSpeed;
		milkFishBobTime     += milkFishBobSpeed;
		reynbohBobTime      += reynbohBobSpeed;
		deathSnapperBobTime += deathSnapperBobSpeed;
		exoTroutBobTime     += exoTroutBobSpeed;
		griesellyBobTime    += griesellyBobSpeed;
	}	
    if (gameState == FISHING) {
		float halfW = 75.0f;
		fishX += fishFacingRight ? fishSpeedX : -fishSpeedX;
		if (fishX + halfW >= g.xres) { fishX = g.xres - halfW; fishFacingRight = false; }
		if (fishX - halfW <= 0)      { fishX = halfW;           fishFacingRight = true;  }

		float reynbohHalfW = 60.0f;
		reynbohX += reynbohFacingRight ? reynbohSpeedX : -reynbohSpeedX;
		if (reynbohX + reynbohHalfW >= g.xres) { reynbohX = g.xres - reynbohHalfW; reynbohFacingRight = false; }
		if (reynbohX - reynbohHalfW <= 0)      { reynbohX = reynbohHalfW;           reynbohFacingRight = true;  }

		float snapperHalfW = 65.0f;
		deathSnapperX += deathSnapperFacingRight ? deathSnapperSpeedX : -deathSnapperSpeedX;
		if (deathSnapperX + snapperHalfW >= g.xres) { deathSnapperX = g.xres - snapperHalfW; deathSnapperFacingRight = false; }
		if (deathSnapperX - snapperHalfW <= 0)      { deathSnapperX = snapperHalfW;           deathSnapperFacingRight = true;  }

		float exoTroutHalfW = 60.0f;
		exoTroutX += exoTroutFacingRight ? exoTroutSpeedX : -exoTroutSpeedX;
		if (exoTroutX + exoTroutHalfW >= g.xres) { exoTroutX = g.xres - exoTroutHalfW; exoTroutFacingRight = false; }
		if (exoTroutX - exoTroutHalfW <= 0)      { exoTroutX = exoTroutHalfW;           exoTroutFacingRight = true;  }

		float griesellyHalfW = 65.0f;
		griesellyX += griesellyFacingRight ? griesellySpeedX : -griesellySpeedX;
		if (griesellyX + griesellyHalfW >= g.xres) { griesellyX = g.xres - griesellyHalfW; griesellyFacingRight = false; }
		if (griesellyX - griesellyHalfW <= 0)      { griesellyX = griesellyHalfW;           griesellyFacingRight = true;  }
	}
}

void render_box()
{
	glDisable(GL_TEXTURE_2D);
    glColor3ub(box.color[0], box.color[1], box.color[2]);
    glPushMatrix();
    glTranslatef(box.pos[0], box.pos[1], 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(-box.width/2, 0);
        glVertex2f(-box.width/2, box.height);
        glVertex2f(box.width/2, box.height);
        glVertex2f(box.width/2, 0);
    glEnd();
    glPopMatrix();

    glColor3ub(top.color[0], top.color[1], top.color[2]);
    glPushMatrix();
    glTranslatef(top.pos[0], top.pos[1], 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(-top.width/2, 0);
        glVertex2f(-top.width/2, top.height);
        glVertex2f(top.width/2, top.height);
        glVertex2f(top.width/2, 0);
    glEnd();
    glPopMatrix();
    glEnable(GL_TEXTURE_2D);
}

void select_menu_option(int i)
{
	switch (i) {
		case 0:
			glColor3ub(0, 0, 0);
			printf("Start Game selected\n");
			gameState = PLAY;

			break;
		case 1:
			glColor3ub(0, 0, 0);
			printf("Character selected\n");
			gameState = CHARACTER;
			break;
		case 2:
			glColor3ub(0, 0, 0);
			printf("Quit selected\n");
			exit(0);
			break;
	}

}

void highlight_bar(float cx, float cy, float w, float h)
{
	 glColor3ub(255, 255, 255);
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(-w/2, -h/2);
        glVertex2f(-w/2,  h/2);
        glVertex2f( w/2,  h/2);
        glVertex2f( w/2, -h/2);
    glEnd();
    glPopMatrix();

}


void render_senor_pescado() {

    float boxRight  = box.pos[0] + box.width * 0.5f;
    float boxBottom = box.pos[1];
    float boxTop    = box.pos[1] + box.height;
    
    float drawW = 70.0f;
    float drawH = 70.0f;

   
    float padRight = 25.0f;
    float cx = boxRight - padRight - drawW * 0.5f;
    float cy = (boxBottom + boxTop) * 0.5f;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1,1,1,1);
    glBindTexture(GL_TEXTURE_2D, g.pescadoTex);

	glPushMatrix();
	glTranslatef(cx, cy, 0.0f); 
	glRotatef(g.logoAngle, 0.0f, 0.0f, 1.0f);

   	glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-drawW/2, -drawH/2);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-drawW/2,  drawH/2);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( drawW/2,  drawH/2);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( drawW/2, -drawH/2);
    glEnd();

	glPopMatrix();

    glDisable(GL_BLEND);

}

void render_logo() 
{

    float w = 500.0f;
    float h = 240.0f;
    float x = (g.xres - w) / 2;
    float y = g.yres - h - 70;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, g.partyTex);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + h);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y + h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y);
    glEnd();
	glDisable(GL_BLEND);
}


void render_boat() {
   
    float w = 300.0f;
    float h = 400.0f;
    float cx = g.xres / 2.0f;
    float cy = (g.yres / 2.0f) - 80.0f;

	float bob = sinf(boatBobTime) * boatBobAmp;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, g.boatTex);
	glPushMatrix();
	glTranslatef(cx, cy + bob, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-w/2, -h/2);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-w/2, h/2);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(w/2, h/2);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(w/2, -h/2);
    glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();
    
}

void render_fish() {
   
    float w = 150.0f;
    float h = 120.0f;
    float cx = fishX;
    float cy = 90.0f; // well below the boat

	float bob = sinf(milkFishBobTime) * milkFishBobAmp;

    // flip texture coords horizontally based on direction
	float texLeft  = fishFacingRight ? 0.0f : 1.0f;
	float texRight = fishFacingRight ? 1.0f : 0.0f;

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, g.fishOneTex);
	glPushMatrix();
	glTranslatef(cx, cy + bob, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(texLeft, 1.0f); glVertex2f(-w/2, -h/2);
        glTexCoord2f(texLeft, 0.0f); glVertex2f(-w/2, h/2);
        glTexCoord2f(texRight, 0.0f); glVertex2f(w/2, h/2);
        glTexCoord2f(texRight, 1.0f); glVertex2f(w/2, -h/2);
    glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();
    
}

void render_reynboh_fish() {

    float w = 120.0f;
    float h = 100.0f;
    float cx = reynbohX;
    float cy = 60.0f; // different depth than milking fish

	float bob = sinf(reynbohBobTime) * reynbohBobAmp;

	float texLeft  = reynbohFacingRight ? 0.0f : 1.0f;
	float texRight = reynbohFacingRight ? 1.0f : 0.0f;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, g.reynbohTex);
	glPushMatrix();
	glTranslatef(cx, cy + bob, 0.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(texLeft,  1.0f); glVertex2f(-w/2, -h/2);
		glTexCoord2f(texLeft,  0.0f); glVertex2f(-w/2,  h/2);
		glTexCoord2f(texRight, 0.0f); glVertex2f( w/2,  h/2);
		glTexCoord2f(texRight, 1.0f); glVertex2f( w/2, -h/2);
	glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();

}

void render_death_snapper() {

	float w = 130.0f;
	float h = 110.0f;
	float cx = deathSnapperX;
	float cy = 30.0f; // deepest of the three fish

	float bob = sinf(deathSnapperBobTime) * deathSnapperBobAmp;

	float texLeft  = deathSnapperFacingRight ? 0.0f : 1.0f;
	float texRight = deathSnapperFacingRight ? 1.0f : 0.0f;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, g.deathSnapperTex);
	glPushMatrix();
	glTranslatef(cx, cy + bob, 0.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(texLeft,  1.0f); glVertex2f(-w/2, -h/2);
		glTexCoord2f(texLeft,  0.0f); glVertex2f(-w/2,  h/2);
		glTexCoord2f(texRight, 0.0f); glVertex2f( w/2,  h/2);
		glTexCoord2f(texRight, 1.0f); glVertex2f( w/2, -h/2);
	glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();

}

void render_exo_trout() {

	float w = 140.0f;
	float h = 110.0f;
	float cx = exoTroutX;
	float cy = 120.0f; // higher than the other fish

	float bob = sinf(exoTroutBobTime) * exoTroutBobAmp;

	float texLeft  = exoTroutFacingRight ? 0.0f : 1.0f;
	float texRight = exoTroutFacingRight ? 1.0f : 0.0f;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, g.exoTroutTex);
	glPushMatrix();
	glTranslatef(cx, cy + bob, 0.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(texLeft,  1.0f); glVertex2f(-w/2, -h/2);
		glTexCoord2f(texLeft,  0.0f); glVertex2f(-w/2,  h/2);
		glTexCoord2f(texRight, 0.0f); glVertex2f( w/2,  h/2);
		glTexCoord2f(texRight, 1.0f); glVertex2f( w/2, -h/2);
	glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();

}

void render_grieselly_fish() {

	float w = 125.0f;
	float h = 105.0f;
	float cx = griesellyX;
	float cy = 150.0f; // highest of all fish, still well below boat

	float bob = sinf(griesellyBobTime) * griesellyBobAmp;

	float texLeft  = griesellyFacingRight ? 0.0f : 1.0f;
	float texRight = griesellyFacingRight ? 1.0f : 0.0f;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, g.griesellyTex);
	glPushMatrix();
	glTranslatef(cx, cy + bob, 0.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(texLeft,  1.0f); glVertex2f(-w/2, -h/2);
		glTexCoord2f(texLeft,  0.0f); glVertex2f(-w/2,  h/2);
		glTexCoord2f(texRight, 0.0f); glVertex2f( w/2,  h/2);
		glTexCoord2f(texRight, 1.0f); glVertex2f( w/2, -h/2);
	glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();

}

void render_menu()
{
    const int paddingX = 25;
    const int paddingY = 40;
    const int lineStep = 30;
    int left = (int)(box.pos[0] - box.width/2 + paddingX);
    int topY = (int)(box.pos[1] + box.height - paddingY);

    Rect r;
    r.center = 0;
    r.left = left;
    r.bot = topY;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < menuCount; i++) {
        unsigned int color = (menuSelected == i) ? 0x00ff0000 : 0x00000000;
        if (i == menuSelected)
            ggprint16(&r, lineStep, color, "> %s", menuSelect[i]);
        else
            ggprint16(&r, lineStep, color, "  %s", menuSelect[i]);
    }
	glDisable(GL_BLEND);

}

void render()
{
	if (gameState == MENU) {
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
	glBegin(GL_QUADS);
		glTexCoord2f(g.tex.xc[0], g.tex.yc[1]);
        glVertex2i(0,      0);
		glTexCoord2f(g.tex.xc[0], g.tex.yc[0]);
        glVertex2i(0,      g.yres);
		glTexCoord2f(g.tex.xc[1], g.tex.yc[0]);
        glVertex2i(g.xres, g.yres);
		glTexCoord2f(g.tex.xc[1], g.tex.yc[1]);
        glVertex2i(g.xres, 0);
	glEnd();

    render_box();
	render_menu();
    render_logo();
	render_senor_pescado();

	}
	else if (gameState == PLAY) {
        glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1.0, 1.0, 1.0);
    	glBindTexture(GL_TEXTURE_2D, g.fishingTex);
    	glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, g.yres);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(g.xres, g.yres);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(g.xres, 0);
    	glEnd();
        render_boat();
	}
	else if (gameState == FISHING) {
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, g.fishingTex);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, g.yres);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(g.xres, g.yres);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(g.xres, 0);
        glEnd();
        render_boat();
        render_fish();
        render_reynboh_fish();
        render_death_snapper();
        render_exo_trout();
        render_grieselly_fish();
	}

}
