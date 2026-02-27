//
//program: background.cpp
//author:  Gordon Griesel
//date:    2017 - 2018
//
//The position of the background QUAD does not change.
//Just the texture coordinates change.
//In this example, only the x coordinates change.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

class Image {
public:
	int width, height;
	unsigned char *data;
	~Image() { delete [] data; }
	Image(const char *fname) {
		if (fname[0] == '\0')
			return;
		char name[40];
		strcpy(name, fname);
		int slen = strlen(name);
		name[slen-4] = '\0';
		char ppmname[80];
		sprintf(ppmname,"%s.ppm", name);
		char ts[100];
		sprintf(ts, "convert %s %s", fname, ppmname);
		system(ts);
		FILE *fpi = fopen(ppmname, "r");
		if (fpi) {
			char line[200];
			fgets(line, 200, fpi);
			fgets(line, 200, fpi);
			//skip comments and blank lines
			while (line[0] == '#' || strlen(line) < 2)
				fgets(line, 200, fpi);
			sscanf(line, "%i %i", &width, &height);
			fgets(line, 200, fpi);
			//get pixel data
			int n = width * height * 3;			
			data = new unsigned char[n];			
			for (int i=0; i<n; i++)
				data[i] = fgetc(fpi);
			fclose(fpi);
		} else {
			printf("ERROR opening image: %s\n", ppmname);
			exit(0);
		}
		unlink(ppmname);
	}
};
Image img[1] = {"fish.jpg"};

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

Box box(640/2, 120);
Box top(640/2, 35);
Box selection1(90, 65);
Box selection2(90, 65);
Box selection3(90, 65);

Box::Box(float w, float h)
{
    width = w;
    height = h;
    pos[0] = 640/2;
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
	Global() {
		xres=640, yres=480;
	}
} g;

static const char* menuSelect[] = {
	"Play"
	"Select Character"
	"Quit"
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
		setup_screen_res(640, 480);
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
		XStoreName(dpy, win, "scrolling background (seamless)");
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
}

void check_mouse(XEvent *e)
{
	//Did the mouse move?
	//Was a mouse button clicked?
	static int savex = 0;
	static int savey = 0;
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
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
			menuSelected = (menuSelected - 1 + menuCount) % menuCount;
		}
		if (key == XK_Return || key == XK_space){
			select_menu_option(menuSelected);
		}
		
	}
	return 0;
}

void physics()
{
	//move the background
	g.tex.xc[0] += 0.001;
	g.tex.xc[1] += 0.001;
}

void render_box()
{
	
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
    
}

void select_menu_option(int i) 
{
	switch (i) {
		case 0:
			printf("Start Game selected\n");
			break;
		case 1:
			printf("Options selected\n");
			break;
		case 2:
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

void render_menu()
{
   
    const int paddingX = 18;
    const int paddingY = 18;
    const int lineStep = 24;

  
    int left = (int)(box.pos[0] - box.width/2 + paddingX);
    int topY = (int)(box.pos[1] + box.height - paddingY);

    
    float selY = (float)(topY - menuSelected * lineStep - 8);
    float barCx = box.pos[0];
    float barCy = selY;
    float barW  = box.width - 20.0f;
    float barH  = 22.0f;

    
    glDisable(GL_TEXTURE_2D);
   	highlight_bar(barCx, barCy, barW, barH);
    glEnable(GL_TEXTURE_2D);


    Rect r;
    r.center = 0;
    r.left = left;
    r.bot = topY;

    for (int i = 0; i < menuCount; i++) {
        unsigned int color = (i == menuSelected) ? 0x000000 : 0x222222;
        const char *prefix = (i == menuSelected) ? "> " : "  ";
        ggprint16(&r, lineStep, color, "%s%s", prefix, menuSelect[i]);
    }
}

void render()
{
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
    Rect r;
    r.center = 1;
    r.left = g.xres/2;
    r.bot = g.yres/2;
    ggprint16(&r, 32, 0x000000, "Pescado Party");
}














