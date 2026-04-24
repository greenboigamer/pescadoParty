// edwin aviles, kian hernando, & simon

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <cstdlib>
#include <GL/gl.h>
#include <GL/glu.h>
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
	CHARACTER,
	SHOPPING
};

GameState gameState = MENU;

Image img[16] = {"./assets/images/fish.jpg", "./assets/images/background_fishing.png",
    "./assets/images/senorpescado.png", "./assets/images/logo.png", "./assets/images/gordoni.png",
    "./assets/images/milking_fish.png", "./assets/images/reynboh_pescado.png",
    "./assets/images/death_snapper.png", "./assets/images/exo_trout.png",
    "./assets/images/grieselly_fish.png", "./assets/images/dip_dip.png",
    "./assets/images/shop.png", "./assets/images/brown_cow_in_a_suit.png",
    "./assets/images/win.png", "./assets/images/kian.png", "./assets/images/simon.png"};

//character selection
const int NUM_CHARACTERS = 4;
const char* CHAR_NAMES[NUM_CHARACTERS] = { "Gordoni", "Win", "Kian", "Simon" };
int selectedCharacter = 0;  // which character is active
int charSelectCursor  = 0;

// for boat machanics
float boatBobTime = 0.5f;
float boatBobAmp = 3.0f;
float boatBobSpeed = 0.20f;
int requestedFish = -1;
// Fish pool - properties per fish type
// Order: 0=milking_fish, 1=reynboh, 2=death_snapper, 3=exo_trout, 4=grieselly
const int NUM_FISH = 5;
const float FISH_BOB_AMP[NUM_FISH]   = { 5.0f,  4.0f,  6.0f,  3.5f,  5.5f  };
const float FISH_BOB_SPEED[NUM_FISH] = { 0.50f, 0.35f, 0.45f, 0.28f, 0.40f };
const float FISH_SPEED[NUM_FISH]     = { 2.5f,  3.5f,  4.5f,  3.0f,  5.0f  };
const float FISH_CY[NUM_FISH]        = { 90.0f, 60.0f, 30.0f, 120.0f, 150.0f };
const float FISH_W[NUM_FISH]         = { 150.0f, 120.0f, 130.0f, 140.0f, 125.0f };
const float FISH_H[NUM_FISH]         = { 120.0f, 100.0f, 110.0f, 110.0f, 105.0f };


// Fish display names matching the pool order
const char* FISH_NAMES[NUM_FISH] = {
    "Exo Trout", "Grieselly Fish", "Death Snapper", "Milking Fish", "Reynboh Pescado"
};

const int FISH_PRICES[NUM_FISH] = { 5, 20, 35, 40, 10 };
// Milking Fish, Reynboh, Death Snapper, Exo Trout, Grieselly

int fishInventory[NUM_FISH] = { 0, 0, 0, 0, 0 };
int playerGold   = 0;
int shopSelected = 0;

// Two active fish slots: slot 0 travels right, slot 1 travels left
// Each slot independently cycles through the fish pool
int   slotFish[2]  = { 0, 1 };   // which fish type each slot is showing
float slotX[2]     = { 0.0f, 0.0f };
float slotBob[2]   = { 0.0f, 0.5f };
// slot 0 always goes right, slot 1 always goes left

// ── Fishing rod / bobber state ───────────────────────────────
// Phase 1: rod is cast, waiting for a fish to bite
// Phase 2: fish bites, minigame becomes active
// Phase 3: minigame meter filled — fish is hooked (not yet caught)
enum FishingPhase { PHASE_NONE, PHASE_WAITING, PHASE_MINIGAME, PHASE_HOOKED };
FishingPhase fishingPhase = PHASE_NONE;

float biteTimer    = 0.0f;  // countdown until a fish bites
float biteDelay    = 0.0f;  // randomised each cast (2–6 seconds)
bool  bobberActive = false;
int   hookedFishIndex = -1; // which fish type bit the bobber (-1 = none)
float biteAlertTimer  = 0.0f; // how long the "fish on the line!" flash shows

void reset_fishing_state(); // defined after all globals

// ── Skill check state ────────────────────────────────────────
bool  skillCheckActive  = false;
float needleAngle       = 0.0f;
float needleSpeed       = 1.8f;
float scZoneStart       = 0.5f;
const float SC_ZONE_SIZE    = 0.60f;
const float SC_PERFECT_SIZE = 0.2f;

// Result display
enum SkillResult { SC_NONE, SC_MISS, SC_HIT, SC_GREAT };
SkillResult skillResult = SC_NONE;
float       resultTimer = 0.0f;

// ── Catch / reeling system ───────────────────────────────────
// A "catch" requires CHECKS_NEEDED successful hits in a row.
// Missing resets progress. Greats count double toward reelProgress.
const int   CHECKS_NEEDED  = 5;     // hits needed to land a fish
const float REEL_HIT_AMT   = 1.0f; // progress per good hit
const float REEL_GREAT_AMT = 2.0f; // progress per great hit
const float REEL_MISS_AMT  = 1.5f; // progress lost on miss

float reelProgress   = 0.0f;  // 0 → CHECKS_NEEDED = fish caught
float reelMax        = (float)CHECKS_NEEDED * REEL_HIT_AMT;
int   totalCaught    = 0;     // fish landed this session
int   streak         = 0;     // consecutive non-miss checks
float catchMsgTimer  = 0.0f;  // how long "FISH CAUGHT!" stays up
const int MAX_ATTEMPTS = 5;
int attemptCount = 0;  

void start_skill_check();
void on_skill_result(SkillResult r);
void open_shop();


void on_skill_result(SkillResult r) {
    
	if (attemptCount < MAX_ATTEMPTS)
    	attemptCount++;

    if (r == SC_GREAT) {
        reelProgress += REEL_GREAT_AMT;
        streak++;
        printf("[SKILL] GREAT HIT  | reel: %.1f / %.1f | streak: %d\n",
               reelProgress, reelMax, streak);
        fflush(stdout);
    } else if (r == SC_HIT) {
        reelProgress += REEL_HIT_AMT;
        streak++;
        printf("[SKILL] GOOD HIT   | reel: %.1f / %.1f | streak: %d\n",
               reelProgress, reelMax, streak);
        fflush(stdout);
    } else {
        reelProgress -= REEL_MISS_AMT;
        if (reelProgress < 0.0f) reelProgress = 0.0f;
        streak = 0;
        printf("[SKILL] MISSED     | reel: %.1f / %.1f | streak reset\n",
               reelProgress, reelMax);
        fflush(stdout);
    }

    if (reelProgress >= reelMax) {
        reelProgress     = 0.0f;
        streak           = 0;
        skillCheckActive = false;
        fishingPhase     = PHASE_HOOKED;
        catchMsgTimer    = 2.0f;
		if (hookedFishIndex >= 0 && hookedFishIndex < NUM_FISH)
        fishInventory[hookedFishIndex]++;
        printf("[REEL] *** FISH HOOKED! ***\n");
        fflush(stdout);
    }
}

void start_skill_check() {
    uniform_real_distribution<float> zoneDist(
        0.4f, 2.0f * M_PI - SC_ZONE_SIZE - 0.4f);
    // Speed scales with reel progress — gets harder as you reel in
    float minSpeed = 1.4f + (reelProgress / reelMax) * 0.8f;
    float maxSpeed = 2.4f + (reelProgress / reelMax) * 1.0f;
    uniform_real_distribution<float> speedDist(minSpeed, maxSpeed);

    float oldZone  = scZoneStart;
    float oldSpeed = needleSpeed;

    scZoneStart      = zoneDist(gen);
    needleSpeed      = speedDist(gen);
    needleAngle      = 0.0f;   // reset to 12 o'clock
    skillCheckActive = true;
    skillResult      = SC_NONE;

    printf("[SKILL] New check  | zone: %.2f rad (was %.2f) | speed: %.2f (was %.2f) | needle reset to 0\n",
           scZoneStart, oldZone, needleSpeed, oldSpeed);
    fflush(stdout);
}


void reset_fishing_state() {
    fishingPhase     = PHASE_NONE;
    biteTimer        = 0.0f;
    bobberActive     = false;
    hookedFishIndex  = -1;
    biteAlertTimer   = 0.0f;
    skillCheckActive = false;
    reelProgress     = 0.0f;
    streak           = 0;
    catchMsgTimer    = 0.0f;
    resultTimer      = 0.0f;
    skillResult      = SC_NONE;
	attemptCount     = 0; 
}

float normalizeAngle(float a) {
    float twoPi = 2.0f * M_PI;
    return fmodf(fmodf(a, twoPi) + twoPi, twoPi);
}

bool isInArc(float needle, float start, float size) {
    float n = normalizeAngle(needle);
    float s = normalizeAngle(start);
    float e = normalizeAngle(start + size);
    if (s <= e) return n >= s && n <= e;
    return n >= s || n <= e;
}

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
	GLuint dipDipTex;
	GLuint fishOneTex;
	GLuint reynbohTex;
	GLuint deathSnapperTex;
	GLuint exoTroutTex;
	GLuint griesellyTex;
	GLuint shopTex;
	GLuint brownTex;
	GLuint winTex;
    GLuint kianTex;
    GLuint simonTex;
	float logoAngle;
	//int fps; 
	Global() {
		xres=640, yres=480;
		fishingTex = 0;
		pescadoTex = 0;
		logoAngle = 0.0f;
		partyTex = 0;
		boatTex = 0;
		dipDipTex = 0;
		fishOneTex = 0;
		reynbohTex = 0;
		deathSnapperTex = 0;
		exoTroutTex = 0;
		griesellyTex = 0;
		shopTex = 0;
		brownTex = 0;
		winTex   = 0;
        kianTex  = 0;
        simonTex = 0;
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
	int nframes = 0;
    int starttime = time(NULL);
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
		nframes++;
		int currtime = time(NULL);
        if (currtime > starttime) {
            starttime = currtime;
            //g.fps = nframes;
            nframes = 0;
        }
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

	// dip_dip boat (used when rod is cast)
	unsigned char *alphaDataDipDip = buildAlphaData(&img[10]);
	glGenTextures(1, &g.dipDipTex);
	glBindTexture(GL_TEXTURE_2D, g.dipDipTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[10].width, img[10].height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, alphaDataDipDip);
	free(alphaDataDipDip);

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

	//shop image
	unsigned char *alphaDataShop = buildAlphaData(&img[11]);
	glGenTextures(1, &g.shopTex);
	glBindTexture(GL_TEXTURE_2D, g.shopTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[11].width, img[11].height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataShop);
	free(alphaDataShop);

	//brown cow 
	unsigned char *alphaDataBrown = buildAlphaData(&img[12]);
	glGenTextures(1, &g.brownTex);
	glBindTexture(GL_TEXTURE_2D, g.brownTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[12].width, img[12].height, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, alphaDataBrown);
	free(alphaDataBrown);

	// win
	unsigned char *alphaDataWin = buildAlphaData(&img[13]);
	glGenTextures(1, &g.winTex);
	glBindTexture(GL_TEXTURE_2D, g.winTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[13].width, img[13].height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, alphaDataWin);
	free(alphaDataWin);

	// kian
	unsigned char *alphaDataKian = buildAlphaData(&img[14]);
	glGenTextures(1, &g.kianTex);
	glBindTexture(GL_TEXTURE_2D, g.kianTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[14].width, img[14].height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, alphaDataKian);
	free(alphaDataKian);

	// simon
	unsigned char *alphaDataSimon = buildAlphaData(&img[15]);
	glGenTextures(1, &g.simonTex);
	glBindTexture(GL_TEXTURE_2D, g.simonTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[15].width, img[15].height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, alphaDataSimon);
	free(alphaDataSimon);


	initialize_fonts();
	// slot 0 starts off left edge moving right, slot 1 starts off right edge moving left
	slotFish[0] = 0;
	slotFish[1] = 1;
	slotX[0]    = -FISH_W[0] / 2.0f;
	slotX[1]    = g.xres + FISH_W[1] / 2.0f;
	slotBob[0]  = 0.0f;
	slotBob[1]  = 0.5f;
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
                // Cast the rod: enter FISHING in waiting-for-bite phase
                printf("Rod cast! Waiting for a bite...\n");
                fflush(stdout);
                reset_fishing_state();
                gameState    = FISHING;
                fishingPhase = PHASE_WAITING;
                bobberActive = true;
                // Randomise bite delay: 2–6 seconds
                uniform_real_distribution<float> biteDist(2.0f, 6.0f);
                biteDelay = biteDist(gen);
                biteTimer = biteDelay;
                //printf("[FISHING] Bite expected in %.1f seconds\n", biteDelay);
                fflush(stdout);
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
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);

		// ESC
		if (key == XK_Escape) {
			if (gameState == FISHING) {
				gameState = PLAY;
				skillCheckActive = false;
				return 0;
			}
			if (gameState == SHOPPING) {
				gameState = PLAY;
				return 0;
			}
			if (gameState == CHARACTER) {
				gameState = MENU;
				return 0;
			}

			if (gameState == PLAY) {
				gameState = MENU;
				return 0;
			}
			return 1;
		}

		// CHARACTER block — must be before MENU block
		if (gameState == CHARACTER) {
			if (key == XK_Left || key == XK_a)
				charSelectCursor = (charSelectCursor - 1 + NUM_CHARACTERS) % NUM_CHARACTERS;
			if (key == XK_Right || key == XK_d)
				charSelectCursor = (charSelectCursor + 1) % NUM_CHARACTERS;
			if (key == XK_Return) {
				selectedCharacter = charSelectCursor;
				printf("[CHAR] Selected: %s\n", CHAR_NAMES[selectedCharacter]);
				fflush(stdout);
				gameState = MENU;
			}
			return 0; // nothing else fires in CHARACTER state
		}

		// remove after testing
		if (key == XK_g || key == XK_G) {
			for (int i = 0; i < NUM_FISH; i++)
				fishInventory[i]++;
			printf("[TEST] Added 1 of each fish to inventory\n");
			fflush(stdout);
		}

		// open shop
		if ((key == XK_s || key == XK_S) && gameState == PLAY) {
			gameState = SHOPPING;
			uniform_int_distribution<int> fishDist(0, NUM_FISH - 1);
			requestedFish = fishDist(gen);
		}

		// back to fishing button from shop
		if (gameState == SHOPPING) {
			if (key == XK_b || key == XK_B) {
				gameState = PLAY;
			}
		}

		// TEST KEY: press F anywhere to jump to FISHING
		if (key == XK_f || key == XK_F) {
			reset_fishing_state();
			gameState    = FISHING;
			fishingPhase = PHASE_WAITING;
			bobberActive = true;
			uniform_real_distribution<float> biteDist(2.0f, 6.0f);
			biteDelay = biteDist(gen);
			biteTimer = biteDelay;
			printf("[TEST] Entered FISHING state, waiting %.1f s for bite\n", biteDelay);
			fflush(stdout);
		}


		// SPACE: skill check input in FISHING
		if (key == XK_space) {
			if (gameState == FISHING && skillCheckActive) {
				skillCheckActive = false;
				if (isInArc(needleAngle, scZoneStart, SC_PERFECT_SIZE)) {
					skillResult = SC_GREAT;
					resultTimer = 0.8f;
				} else if (isInArc(needleAngle, scZoneStart, SC_ZONE_SIZE)) {
					skillResult = SC_HIT;
					resultTimer = 0.8f;
				} else {
					skillResult = SC_MISS;
					resultTimer = 0.8f;
				}
				on_skill_result(skillResult);
			} else if (gameState == MENU) {
				select_menu_option(menuSelected);
			}
		}

		// MENU block
		if (gameState == MENU) {
			if (key == XK_Up || key == XK_w)
				menuSelected = (menuSelected - 1 + menuCount) % menuCount;
			if (key == XK_Down || key == XK_s)
				menuSelected = (menuSelected + 1 + menuCount) % menuCount;
			if (key == XK_Return)
				select_menu_option(menuSelected);
			return 0; // nothing else fires in MENU state
		}

		// SHOPPING block
		if (gameState == SHOPPING) {
			if (key == XK_y || key == XK_Y || key == XK_Return) {
				if (requestedFish >= 0 && fishInventory[requestedFish] > 0) {
					fishInventory[requestedFish]--;
					playerGold += FISH_PRICES[requestedFish];
					uniform_int_distribution<int> fishDist(0, NUM_FISH - 1);
					requestedFish = fishDist(gen);
					printf("[SHOP] Sold! Gold: %d. Next customer wants: %s\n",
						playerGold, FISH_NAMES[requestedFish]);
					fflush(stdout);
				} else {
					printf("[SHOP] Can't sell — don't have that fish.\n");
					fflush(stdout);
				}
			}
			if (key == XK_n || key == XK_N) {
				uniform_int_distribution<int> fishDist(0, NUM_FISH - 1);
				requestedFish = fishDist(gen);
				printf("[SHOP] Declined. Next customer wants: %s\n",
					FISH_NAMES[requestedFish]);
				fflush(stdout);
			}
		}
	}
	return 0;
}


void physics()
{
	if (gameState == MENU) {
        g.tex.xc[0] += 0.001;
        g.tex.xc[1] += 0.001;

		g.logoAngle += 3.0f;
    	if (g.logoAngle >= 360.0f)
        	g.logoAngle -= 360.0f;
	}

	if (gameState == PLAY || gameState == FISHING) {
    boatBobTime += boatBobSpeed;
}

	if (gameState == PLAY) {
		for (int s = 0; s < 2; s++) {
			int fi = slotFish[s];
			slotBob[s] += FISH_BOB_SPEED[fi];
			slotX[s]   += (s == 0) ? FISH_SPEED[fi] : -FISH_SPEED[fi];

			float halfW = FISH_W[fi] / 2.0f;
			bool exited = (s == 0) ? (slotX[s] - halfW > g.xres)
								: (slotX[s] + halfW < 0);
			if (exited) {
				int next = (fi + 2) % NUM_FISH;
				slotFish[s] = next;
				slotBob[s]  = 0.0f;
				float nextHalfW = FISH_W[next] / 2.0f;
				slotX[s] = (s == 0) ? -nextHalfW : g.xres + nextHalfW;
			}
		}
	}

	if (gameState == FISHING) {
		// Real delta time for smooth needle movement
		static struct timeval lastTime = {0, 0};
		struct timeval now;
		gettimeofday(&now, NULL);
		float dt = 0.0f;
		if (lastTime.tv_sec != 0) {
			dt = (now.tv_sec  - lastTime.tv_sec) +
			     (now.tv_usec - lastTime.tv_usec) / 1000000.0f;
			if (dt > 0.1f) dt = 0.1f;
		}
		lastTime = now;

		// ── Phase 1: waiting for a bite ──────────────────────────
		if (fishingPhase == PHASE_WAITING) {
			// The rod tip sits at the LEFT quarter of the boat.
			// Boat center is at xres/2; boat width scales with img[10].
			float boatW      = img[10].width * 4.0f;   // same scale as render_boat()
			float boatCX     = g.xres / 2.0f;
			float rodTipX    = boatCX - boatW * 0.25f; // 1/4 from left edge = left quarter
			float hitZoneW   = 24.0f;                  // width of the collision zone around the rod

			for (int s = 0; s < 2; s++) {
				float fishCX = slotX[s]; // center of the fish sprite

				float zoneL = rodTipX - hitZoneW / 2.0f;
				float zoneR = rodTipX + hitZoneW / 2.0f;

				bool overlaps = (fishCX >= zoneL && fishCX <= zoneR);
				if (overlaps) {
					hookedFishIndex = slotFish[s];
					biteAlertTimer  = 1.5f;
					fishingPhase    = PHASE_MINIGAME;
					printf("[FISHING] Fish collided with rod! Fish type: %d. Minigame starting!\n", hookedFishIndex);
					fflush(stdout);
					start_skill_check();
					break;
				}
			}
		}

		// ── Phase 2: minigame active ─────────────────────────────
		if (fishingPhase == PHASE_MINIGAME) {
			// Tick down the bite alert flash
			if (biteAlertTimer > 0.0f) biteAlertTimer -= dt;

			// Advance needle
			if (skillCheckActive) {
				needleAngle += needleSpeed * dt;
				if (needleAngle > 2.0f * M_PI) needleAngle -= 2.0f * M_PI;
			}
			// Tick down result flash, then fire next skill check
			if (!skillCheckActive && resultTimer > 0.0f) {
				resultTimer -= dt;
				if (resultTimer <= 0.0f) {
					resultTimer = 0.0f;
					skillResult = SC_NONE;

					if (fishingPhase == PHASE_MINIGAME) {
						if (attemptCount < MAX_ATTEMPTS) {
							start_skill_check();
						} else {
							printf("[FISHING] Fish got away after %d attempts.\n", attemptCount);
							fflush(stdout);
							reset_fishing_state();
							gameState = PLAY;
						}
					}
				}
			}
		}

		// ── Phase 3: fish hooked — banner countdown ──────────────
		if (fishingPhase == PHASE_HOOKED) {
			if (catchMsgTimer > 0.0f) {
				catchMsgTimer -= dt;
				if (catchMsgTimer <= 0.0f) {
					catchMsgTimer = 0.0f;
					totalCaught++;
					printf("[FISHING] Fish caught! Total: %d. Returning to PLAY.\n", totalCaught);
					fflush(stdout);
					reset_fishing_state();
					gameState = PLAY;
				}
			}
		}

		// ── Fish swimming animation (always runs in FISHING) ─────
		for (int s = 0; s < 2; s++) {
			if (fishingPhase == PHASE_MINIGAME || fishingPhase == PHASE_HOOKED) {
        	if (slotFish[s] == hookedFishIndex) continue;
    	}

			int   fi    = slotFish[s];
			float speed = FISH_SPEED[fi];
			slotBob[s] += FISH_BOB_SPEED[fi];
			slotX[s]   += (s == 0) ? speed : -speed;

			float halfW = FISH_W[fi] / 2.0f;
			bool exited = (s == 0) ? (slotX[s] - halfW > g.xres)
			                       : (slotX[s] + halfW < 0);
			if (exited) {
				int next = (fi + 2) % NUM_FISH;
				slotFish[s] = next;
				slotBob[s]  = 0.0f;
				float nextHalfW = FISH_W[next] / 2.0f;
				slotX[s] = (s == 0) ? -nextHalfW : g.xres + nextHalfW;
			}
		}
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
			charSelectCursor = selectedCharacter;
			return;
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

void render_shop_back_button()
{
    float boxW = 180.0f;
    float boxH = 32.0f;
    float pad  = 12.0f;

    float x = g.xres - boxW - pad;
    float y = g.yres - boxH - pad;

    glDisable(GL_TEXTURE_2D);
    glColor3ub(255, 255, 255);
    glBegin(GL_QUADS);
        glVertex2f(x,        y);
        glVertex2f(x,        y + boxH);
        glVertex2f(x + boxW, y + boxH);
        glVertex2f(x + boxW, y);
    glEnd();

    glColor3ub(0, 0, 0);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,        y);
        glVertex2f(x,        y + boxH);
        glVertex2f(x + boxW, y + boxH);
        glVertex2f(x + boxW, y);
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Rect r;
    r.center = 1;
    r.left = (int)(x + boxW / 2.0f);
    r.bot  = (int)(y + 10.0f);

    ggprint16(&r, 0, 0x00000000, "[B] Back to Fishing");

    glDisable(GL_BLEND);
}

void open_shop(){

	GLuint fishTextures[NUM_FISH] = {
	g.fishOneTex, g.reynbohTex, g.deathSnapperTex,
	g.exoTroutTex, g.griesellyTex
	};

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	float fishScale = 0.700f;

	// Top shelf — first 2 fish
	float topShelfY  = 95.0f;
	float topStartX  = 390.0f;
	float topSpacing = 90.0f;

	for (int i = 0; i < 2; i++) {
		if (fishInventory[i] <= 0) continue; 
		float fw = FISH_W[i] * fishScale;
		float fh = FISH_H[i] * fishScale;
		float fx = topStartX + i * topSpacing;

		glBindTexture(GL_TEXTURE_2D, fishTextures[i]);
		glPushMatrix();
		glTranslatef(fx, topShelfY, 0.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-fw/2, -fh/2);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-fw/2,  fh/2);
			glTexCoord2f(1.0f, 0.0f); glVertex2f( fw/2,  fh/2);
			glTexCoord2f(1.0f, 1.0f); glVertex2f( fw/2, -fh/2);
		glEnd();
		glPopMatrix();
	}

	// Bottom shelf — last 3 fish
	float botShelfY  = 50.0f;
	float botStartX  = 330.0f;
	float botSpacing = 90.0f;

	for (int i = 2; i < NUM_FISH; i++) {
		if (fishInventory[i] <= 0) continue; 
		float fw = FISH_W[i] * fishScale;
		float fh = FISH_H[i] * fishScale;
		float fx = botStartX + (i - 2) * botSpacing;

		glBindTexture(GL_TEXTURE_2D, fishTextures[i]);
		glPushMatrix();
		glTranslatef(fx, botShelfY, 0.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(-fw/2, -fh/2);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-fw/2,  fh/2);
			glTexCoord2f(1.0f, 0.0f); glVertex2f( fw/2,  fh/2);
			glTexCoord2f(1.0f, 1.0f); glVertex2f( fw/2, -fh/2);
		glEnd();
		glPopMatrix();
	}

	glDisable(GL_BLEND);

	// ── Gold display ─────────────────────────────────────────────
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect rGold;
	rGold.center = 0;
	rGold.left   = 10;
	rGold.bot    = g.yres - 20;
	ggprint16(&rGold, 0, 0x00000000, "Gold: %d", playerGold);
	glDisable(GL_BLEND);

	// ── Brown cow NPC ─────────────────────────────────────────────
	float scale = 4.0f;
    float w = img[12].width  * scale;
    float h = img[12].height * scale;
    float cx = g.xres / 2.0f;
    float cy = g.yres - 265.0f;
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, g.brownTex);
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-w/4, -h/4);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-w/4, h/4);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(w/4, h/4);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(w/4, -h/4);
    glEnd();
    glDisable(GL_BLEND);
    glPopMatrix();

	// ── Customer speech bubble ────────────────────────────────────
    glDisable(GL_TEXTURE_2D);
    glColor3ub(255, 127, 80);

    float boxW = 350.0f;
    float boxH = 40.0f;
    float boxX = cx - boxW / 2.0f;
    float boxY = cy - h / 4.0f - 20.0f;

    glBegin(GL_QUADS);
        glVertex2f(boxX,         boxY);
        glVertex2f(boxX,         boxY + boxH);
        glVertex2f(boxX + boxW,  boxY + boxH);
        glVertex2f(boxX + boxW,  boxY);
    glEnd();

    glColor3ub(0, 0, 0);
    glBegin(GL_LINE_LOOP);
        glVertex2f(boxX,         boxY);
        glVertex2f(boxX,         boxY + boxH);
        glVertex2f(boxX + boxW,  boxY + boxH);
        glVertex2f(boxX + boxW,  boxY);
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Rect r;
    r.center = 1;
    r.left = (int)cx;
    r.bot = (int)(boxY + 12);

    if (requestedFish >= 0 && requestedFish < NUM_FISH) {
        ggprint16(&r, 0, 0x00000000,
                  "Howdy! Can I get a %s?",
                  FISH_NAMES[requestedFish]);
    }
    glDisable(GL_BLEND);

	// ── Inventory panel ───────────────────────────────────────────
	// Shows each fish type, your stock count, and price
	float panW  = 300.0f;
	float panH  = 180.0f;
	float panX  = 490.0f;
	float panY  = g.yres - 270.0f;
	float rowH  = panH / NUM_FISH;

	glDisable(GL_TEXTURE_2D);
	// Panel background
	glColor4f(0.05f, 0.1f, 0.15f, 0.85f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f(panX,        panY);
		glVertex2f(panX,        panY + panH);
		glVertex2f(panX + panW, panY + panH);
		glVertex2f(panX + panW, panY);
	glEnd();
	// Panel border
	glLineWidth(1.5f);
	glColor4f(0.7f, 0.85f, 1.0f, 0.7f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(panX,        panY);
		glVertex2f(panX,        panY + panH);
		glVertex2f(panX + panW, panY + panH);
		glVertex2f(panX + panW, panY);
	glEnd();
	glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Header
	Rect rHeader;
	rHeader.center = 0;
	rHeader.left   = (int)(panX + 8);
	rHeader.bot    = (int)(panY + panH - 4);
	ggprint16(&rHeader, 0, 0x00000000, "Your Inventory");

	// One row per fish
	for (int i = 0; i < NUM_FISH; i++) {
		float rowY = panY + panH - 26 - (i + 1) * rowH * 0.78f;

		// Highlight the row if this is what the customer wants
		bool wanted = (i == requestedFish);
		if (wanted) {
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f, 0.55f, 0.2f, 0.4f);
			glBegin(GL_QUADS);
				glVertex2f(panX + 2,        rowY - 2);
				glVertex2f(panX + 2,        rowY + rowH * 0.7f);
				glVertex2f(panX + panW - 2, rowY + rowH * 0.7f);
				glVertex2f(panX + panW - 2, rowY - 2);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}

		Rect rRow;
		rRow.center = 0;
		rRow.left   = (int)(panX + 8);
		rRow.bot    = (int)rowY;

		// Grey out if out of stock, white if in stock, gold if wanted
		unsigned int col;
		if (wanted)                    col = 0x0088ff44; // green — customer wants this
		else if (fishInventory[i] > 0) col = 0x00ffffff;
		else                           col = 0x00666666; // grey — out of stock

		ggprint16(&rRow, 0, col, "%-16s x%d  ($%d)",
			FISH_NAMES[i], fishInventory[i], FISH_PRICES[i]);
	}
	glDisable(GL_BLEND);

	// ── Sell / Decline buttons ────────────────────────────────────
	bool canSell = (requestedFish >= 0 && fishInventory[requestedFish] > 0);

	float btnY  = panY - 55.0f;
	float btnH  = 36.0f;
	float btnW  = 140.0f;
	float sellX = panX;
	float decX  = panX + btnW + 10.0f;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Sell button — green if possible, grey if not
	if (canSell) glColor4f(0.15f, 0.65f, 0.25f, 1.0f);
	else         glColor4f(0.25f, 0.25f, 0.25f, 0.7f);
	glBegin(GL_QUADS);
		glVertex2f(sellX,         btnY);
		glVertex2f(sellX,         btnY + btnH);
		glVertex2f(sellX + btnW,  btnY + btnH);
		glVertex2f(sellX + btnW,  btnY);
	glEnd();

	// Decline button — always red
	glColor4f(0.65f, 0.15f, 0.15f, 1.0f);
	glBegin(GL_QUADS);
		glVertex2f(decX,         btnY);
		glVertex2f(decX,         btnY + btnH);
		glVertex2f(decX + btnW,  btnY + btnH);
		glVertex2f(decX + btnW,  btnY);
	glEnd();

	// Button borders
	glLineWidth(1.5f);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(sellX,        btnY);        glVertex2f(sellX,        btnY + btnH);
		glVertex2f(sellX + btnW, btnY + btnH); glVertex2f(sellX + btnW, btnY);
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex2f(decX,        btnY);        glVertex2f(decX,        btnY + btnH);
		glVertex2f(decX + btnW, btnY + btnH); glVertex2f(decX + btnW, btnY);
	glEnd();
	glDisable(GL_BLEND);

	// Button labels
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Rect rSell;
	rSell.center = 1;
	rSell.left   = (int)(sellX + btnW / 2.0f);
	rSell.bot    = (int)(btnY + 10);
	ggprint16(&rSell, 0, 0x00ffffff, "[Y] Sell");

	Rect rDec;
	rDec.center = 1;
	rDec.left   = (int)(decX + btnW / 2.0f);
	rDec.bot    = (int)(btnY + 10);
	ggprint16(&rDec, 0, 0x00ffffff, "[N] Decline");

	// "Out of stock" warning under the sell button if player can't sell
	if (!canSell && requestedFish >= 0) {
		Rect rWarn;
		rWarn.center = 0;
		rWarn.left   = (int)sellX + 30;
		rWarn.bot    = (int)(btnY - 18);
		ggprint16(&rWarn, 0, 0x00ff5555, "You don't have that fish!");
	}

	glDisable(GL_BLEND);
}

GLuint get_character_tex(int charIndex) {
    switch (charIndex) {
        case 0: return g.boatTex;   // Gordoni
        case 1: return g.winTex;    // Win
        case 2: return g.kianTex;   // Kian
        case 3: return g.simonTex;  // Simon
        default: return g.boatTex;
    }
}


void render_boat() {
    float scale = 4.0f;

    // Character img indices: 0=gordoni(4), 1=win(13), 2=kian(14), 3=simon(15)
    int charImgIndex[NUM_CHARACTERS] = { 4, 13, 14, 15 };

    // Pick the right image based on fishing state and selected character
    int activeIndex = (gameState == FISHING) ? 10 : charImgIndex[selectedCharacter];

    float w = img[activeIndex].width  * scale;
    float h = img[activeIndex].height * scale;
    float cx = g.xres / 2.0f;
    float cy = (g.yres / 2.0f) - 80.0f;
    float bob = sinf(boatBobTime) * boatBobAmp;

    GLuint activeTex = (gameState == FISHING) ? g.dipDipTex : get_character_tex(selectedCharacter);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, activeTex);
    glPushMatrix();
    glTranslatef(cx, cy + bob, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-w/2, -h/2);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-w/2,  h/2);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( w/2,  h/2);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( w/2, -h/2);
    glEnd();
    glDisable(GL_BLEND);
    glPopMatrix();
}


void render_character_select()
{
    // Draw the fishing background behind the UI
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);
    glBindTexture(GL_TEXTURE_2D, g.fishingTex);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, g.yres);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(g.xres, g.yres);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(g.xres, 0);
    glEnd();

    GLuint charTextures[NUM_CHARACTERS] = {
        g.boatTex, g.winTex, g.kianTex, g.simonTex
    };

    // Title
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Rect rTitle;
    rTitle.center = 1;
    rTitle.left   = g.xres / 2;
    rTitle.bot    = g.yres - 50;
    ggprint16(&rTitle, 0, 0x00ffffff, "-- SELECT YOUR CHARACTER --");
    glDisable(GL_BLEND);

    // Layout: space 4 characters evenly across the screen
    float cardW   = 120.0f;
    float cardH   = 140.0f;
    float spacing = (float)g.xres / (NUM_CHARACTERS + 1);
    float cardY   = (g.yres / 2.0f) - 20.0f;

    for (int i = 0; i < NUM_CHARACTERS; i++) {
        float cx = spacing * (i + 1);

        // Highlight box around currently cursored character
        glDisable(GL_TEXTURE_2D);
        if (i == charSelectCursor) {
            // Bright gold selection box
            glLineWidth(3.0f);
            glColor3ub(255, 220, 50);
            glBegin(GL_LINE_LOOP);
                glVertex2f(cx - cardW/2 - 6, cardY - cardH/2 - 6);
                glVertex2f(cx - cardW/2 - 6, cardY + cardH/2 + 6);
                glVertex2f(cx + cardW/2 + 6, cardY + cardH/2 + 6);
                glVertex2f(cx + cardW/2 + 6, cardY - cardH/2 - 6);
            glEnd();
            // Subtle gold fill behind selected card
            glColor4f(1.0f, 0.85f, 0.1f, 0.15f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_QUADS);
                glVertex2f(cx - cardW/2 - 6, cardY - cardH/2 - 6);
                glVertex2f(cx - cardW/2 - 6, cardY + cardH/2 + 6);
                glVertex2f(cx + cardW/2 + 6, cardY + cardH/2 + 6);
                glVertex2f(cx + cardW/2 + 6, cardY - cardH/2 - 6);
            glEnd();
            glDisable(GL_BLEND);
        }

        // Green checkmark box around the actively selected character
        if (i == selectedCharacter) {
            glLineWidth(2.0f);
            glColor3ub(50, 220, 100);
            glBegin(GL_LINE_LOOP);
                glVertex2f(cx - cardW/2 - 3, cardY - cardH/2 - 3);
                glVertex2f(cx - cardW/2 - 3, cardY + cardH/2 + 3);
                glVertex2f(cx + cardW/2 + 3, cardY + cardH/2 + 3);
                glVertex2f(cx + cardW/2 + 3, cardY - cardH/2 - 3);
            glEnd();
        }

        // Character sprite
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, charTextures[i]);
        glPushMatrix();
        glTranslatef(cx, cardY, 0.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(-cardW/2, -cardH/2);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(-cardW/2,  cardH/2);
            glTexCoord2f(1.0f, 0.0f); glVertex2f( cardW/2,  cardH/2);
            glTexCoord2f(1.0f, 1.0f); glVertex2f( cardW/2, -cardH/2);
        glEnd();
        glPopMatrix();
        glDisable(GL_BLEND);

        // Character name below sprite
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Rect rName;
        rName.center = 1;
        rName.left   = (int)cx;
        rName.bot    = (int)(cardY - cardH/2 - 40);
        unsigned int nameCol = (i == charSelectCursor) ? 0x00ffe94f : 0x00ffffff;
        ggprint16(&rName, 0, nameCol, CHAR_NAMES[i]);

        // "ACTIVE" tag under the selected character's name
        if (i == selectedCharacter) {
            Rect rActive;
            rActive.center = 1;
            rActive.left   = (int)cx;
            rActive.bot    = (int)(cardY - cardH/2 - 60);
            ggprint16(&rActive, 0, 0x0044ff88, "[ ACTIVE ]");
        }
        glDisable(GL_BLEND);
    }

    // Instructions at the bottom
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Rect rHint;
    rHint.center = 1;
    rHint.left   = g.xres / 2;
    rHint.bot    = 20;
    ggprint16(&rHint, 0, 0x00aaddff, "LEFT / RIGHT to browse   ENTER to select   ESC to go back");
    glDisable(GL_BLEND);
}

void render_fish_slot(int s) {

	GLuint textures[NUM_FISH] = {
		g.fishOneTex, g.reynbohTex, g.deathSnapperTex, g.exoTroutTex, g.griesellyTex
	};

	int   fi       = slotFish[s];
	float w        = FISH_W[fi];
	float h        = FISH_H[fi];
	float cx       = slotX[s];
	float cy       = FISH_CY[fi];
	float bob      = sinf(slotBob[s]) * FISH_BOB_AMP[fi];
	bool  facingRight = (s == 0); // slot 0 goes right, slot 1 goes left

	float texLeft  = facingRight ? 0.0f : 1.0f;
	float texRight = facingRight ? 1.0f : 0.0f;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, textures[fi]);
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
        unsigned int color = (menuSelected == i) ? 0x00d0ea0d : 0x00000000;
        if (i == menuSelected)
            ggprint16(&r, lineStep, color, "> %s <", menuSelect[i]);
        else
            ggprint16(&r, lineStep, color, "  %s", menuSelect[i]);
    }
	glDisable(GL_BLEND);

}


// ── "! FISH ON THE LINE !" alert flash ───────────────────────
void render_bite_alert()
{
    if (biteAlertTimer <= 0.0f) return;

    // Pulse the alpha so it blinks urgently
    float alpha = 0.7f + 0.3f * sinf(biteAlertTimer * 20.0f);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dark backing strip across the screen center
    float stripH = 40.0f;
    float stripY = g.yres * 0.5f;
    glColor4f(0.0f, 0.0f, 0.0f, alpha * 0.6f);
    glBegin(GL_QUADS);
        glVertex2f(0,        stripY - stripH * 0.5f);
        glVertex2f(0,        stripY + stripH * 0.5f);
        glVertex2f(g.xres,   stripY + stripH * 0.5f);
        glVertex2f(g.xres,   stripY - stripH * 0.5f);
    glEnd();

    glEnable(GL_TEXTURE_2D);

    Rect r;
    r.center = 1;
    r.left   = g.xres / 2;
    r.bot    = (int)(stripY - 10);
    // bright yellow-orange, alpha blinks
    unsigned int col = 0x00ffcc00;
    ggprint16(&r, 0, col, "!  FISH ON THE LINE  !");

    glDisable(GL_BLEND);
}

// ── Fish caught portrait screen ──────────────────────────────
void render_catch_screen()
{
    if (fishingPhase != PHASE_HOOKED || catchMsgTimer <= 0.0f) return;
    if (hookedFishIndex < 0 || hookedFishIndex >= NUM_FISH) return;

    GLuint fishTextures[NUM_FISH] = {
        g.fishOneTex, g.reynbohTex, g.deathSnapperTex, g.exoTroutTex, g.griesellyTex
    };

    // Semi-transparent dark overlay over the whole screen
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.05f, 0.1f, 0.72f);
    glBegin(GL_QUADS);
        glVertex2f(0,       0);
        glVertex2f(0,       g.yres);
        glVertex2f(g.xres,  g.yres);
        glVertex2f(g.xres,  0);
    glEnd();

    // Panel background
    float panW = 320.0f, panH = 280.0f;
    float panX = (g.xres - panW) * 0.5f;
    float panY = (g.yres - panH) * 0.5f;
    glColor4f(0.05f, 0.15f, 0.25f, 0.92f);
    glBegin(GL_QUADS);
        glVertex2f(panX,        panY);
        glVertex2f(panX,        panY + panH);
        glVertex2f(panX + panW, panY + panH);
        glVertex2f(panX + panW, panY);
    glEnd();

    // Panel border (gold)
    glLineWidth(2.5f);
    glColor4f(1.0f, 0.85f, 0.2f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(panX,        panY);
        glVertex2f(panX,        panY + panH);
        glVertex2f(panX + panW, panY + panH);
        glVertex2f(panX + panW, panY);
    glEnd();
    glDisable(GL_BLEND);

    // Fish portrait — centred in top portion of the panel
    float imgW = FISH_W[hookedFishIndex] * 1.3f;
    float imgH = FISH_H[hookedFishIndex] * 1.3f;
    float imgCX = g.xres * 0.5f;
    float imgCY = panY + panH * 0.58f;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, fishTextures[hookedFishIndex]);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(imgCX - imgW * 0.5f, imgCY - imgH * 0.5f);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(imgCX - imgW * 0.5f, imgCY + imgH * 0.5f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(imgCX + imgW * 0.5f, imgCY + imgH * 0.5f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(imgCX + imgW * 0.5f, imgCY - imgH * 0.5f);
    glEnd();
    glDisable(GL_BLEND);

    // Text
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Rect r;
    r.center = 1;
    r.left   = g.xres / 2;

    // "FISH CAUGHT!" header
    r.bot = (int)(panY + panH - 22);
    ggprint16(&r, 0, 0x00ffe94f, "FISH CAUGHT!");

    // Fish name
    r.bot = (int)(panY + 30);
    ggprint16(&r, 0, 0x00ffffff, "%s", FISH_NAMES[hookedFishIndex]);

    // Running total
    r.bot = (int)(panY + 10);
    ggprint16(&r, 0, 0x00aaddff, "Total caught: %d", totalCaught + 1);

    glDisable(GL_BLEND);
}

void render_skill_check()
{
	// Offset to the right of the boat (boat is centered at xres/2)
	float cx = g.xres * 0.78f;
	float cy = g.yres * 0.55f;
	float r  = 80.0f;
	const int SEG = 120;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Dark backing ring
	glLineWidth(14.0f);
	glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < SEG; i++) {
		float a = (float)i / SEG * 2.0f * M_PI;
		glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
	}
	glEnd();

	// Base ring (dark teal)
	glLineWidth(6.0f);
	glColor4f(0.1f, 0.22f, 0.35f, 1.0f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < SEG; i++) {
		float a = (float)i / SEG * 2.0f * M_PI;
		glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
	}
	glEnd();

	// Green success zone
	glLineWidth(8.0f);
	glColor4f(0.18f, 0.8f, 0.44f, 1.0f);
	int zSegs = max(2, (int)(SEG * SC_ZONE_SIZE / (2.0f * M_PI)));
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i <= zSegs; i++) {
		float a = scZoneStart + (float)i / zSegs * SC_ZONE_SIZE;
		glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
	}
	glEnd();

	// Yellow perfect zone
	glLineWidth(8.0f);
	glColor4f(1.0f, 0.85f, 0.2f, 1.0f);
	int pSegs = max(2, (int)(SEG * SC_PERFECT_SIZE / (2.0f * M_PI)));
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i <= pSegs; i++) {
		float a = scZoneStart + (float)i / pSegs * SC_PERFECT_SIZE;
		glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
	}
	glEnd();

	// Needle
	glLineWidth(2.5f);
	glColor4f(1.0f, 0.26f, 0.26f, 1.0f);
	glBegin(GL_LINES);
		glVertex2f(cx, cy);
		glVertex2f(cx + cosf(needleAngle) * r, cy + sinf(needleAngle) * r);
	glEnd();

	// Needle tip
	glPointSize(9.0f);
	glColor4f(1.0f, 0.45f, 0.45f, 1.0f);
	glBegin(GL_POINTS);
		glVertex2f(cx + cosf(needleAngle) * r, cy + sinf(needleAngle) * r);
	glEnd();

	// Center pivot
	glPointSize(7.0f);
	glColor4f(0.9f, 0.95f, 1.0f, 1.0f);
	glBegin(GL_POINTS);
		glVertex2f(cx, cy);
	glEnd();

	glDisable(GL_BLEND);

	// ── Reel progress bar ────────────────────────────────────
	// Drawn just below the circle
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float barW   = 160.0f;
	float barH   = 12.0f;
	float barX   = cx - barW / 2.0f;
	float barY   = cy - r - 30.0f;
	float filled = (reelProgress / reelMax) * barW;
	if (filled > barW) filled = barW;

	// Background
	glColor4f(0.1f, 0.1f, 0.15f, 0.8f);
	glBegin(GL_QUADS);
		glVertex2f(barX,        barY);
		glVertex2f(barX,        barY + barH);
		glVertex2f(barX + barW, barY + barH);
		glVertex2f(barX + barW, barY);
	glEnd();

	// Fill — green normally, yellow when almost full
	float fillRatio = reelProgress / reelMax;
	if (fillRatio > 0.66f)
		glColor4f(0.18f, 0.8f, 0.44f, 1.0f);
	else
		glColor4f(0.2f, 0.55f, 0.9f, 1.0f);
	glBegin(GL_QUADS);
		glVertex2f(barX,          barY);
		glVertex2f(barX,          barY + barH);
		glVertex2f(barX + filled, barY + barH);
		glVertex2f(barX + filled, barY);
	glEnd();

	// Border
	glColor4f(0.7f, 0.85f, 1.0f, 0.6f);
	glLineWidth(1.5f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(barX,        barY);
		glVertex2f(barX,        barY + barH);
		glVertex2f(barX + barW, barY + barH);
		glVertex2f(barX + barW, barY);
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	// ── HUD text ─────────────────────────────────────────────
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Rect rHud;
	rHud.center = 1;
	rHud.left   = (int)cx;

	// Space bar hint (shown while minigame is active)
	if (fishingPhase == PHASE_MINIGAME) {
		Rect rHint;
		rHint.center = 1;
		rHint.left   = (int)cx;
		rHint.bot    = (int)(barY - 4);
		ggprint16(&rHint, 20, 0x00aaddff, "SPACE to reel!");
		rHint.bot = (int)(barY - 24);
    	unsigned int attemptCol = (MAX_ATTEMPTS - attemptCount == 1) ? 0x00ff4f4f : 0x00ffffff;
    	ggprint16(&rHint, 0, attemptCol, "Attempts: %d / %d", attemptCount, MAX_ATTEMPTS);
	}

	// Streak counter (above circle)
	if (streak > 1) {
		rHud.bot = (int)(cy + r + 30);
		ggprint16(&rHud, 0, 0x00ffe94f, "STREAK x%d", streak);
	}

	// Fish caught counter (top-left corner)
	Rect rCorner;
	rCorner.center = 0;
	rCorner.left   = 10;
	rCorner.bot    = g.yres - 20;
	ggprint16(&rCorner, 0, 0x00c8e6ff, "FISH CAUGHT: %d", totalCaught);

	// Skill check result flash
	if (skillResult != SC_NONE && resultTimer > 0.0f) {
		rHud.bot = (int)(cy + r + 55);
		unsigned int col;
		const char *msg;
		if      (skillResult == SC_GREAT) { col = 0x00ffe94f; msg = "GREAT!"; }
		else if (skillResult == SC_HIT)   { col = 0x004fffb0; msg = "NICE!"; }
		else                              { col = 0x00ff4f4f; msg = "MISSED!"; }
		ggprint16(&rHud, 0, col, msg);
	}

	glDisable(GL_BLEND);
}


void render()
{
	// Rect rFPS;
    // rFPS.bot = g.yres - 24;
    // rFPS.left = 10;
    // rFPS.center = 0;
    // ggprint16(&rFPS, 12, 0x00ffffff, "fps: %i", g.fps);

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

    //render_box();
	render_menu();
    render_logo();
	render_senor_pescado();
    //ggprint16(&rFPS, 12, 0x00ffffff, "fps: %i", g.fps);
	

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
		render_fish_slot(0);  // add this
    	render_fish_slot(1);

        // Prompt the player to cast
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Rect rCast;
        rCast.center = 1;
        rCast.left   = g.xres / 2;
        rCast.bot    = 60;
        ggprint16(&rCast, 0, 0x00ffffff, "[ Click to cast your rod ]");
        glDisable(GL_BLEND);

    	//ggprint16(&rFPS, 12, 0x00ffffff, "fps: %i", g.fps);

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
        render_fish_slot(0);
        render_fish_slot(1);
        // Only show the skill check UI once a fish has bitten
        if (fishingPhase == PHASE_MINIGAME || fishingPhase == PHASE_HOOKED)
            render_skill_check();
        // Bite alert flashes over the minigame briefly
        render_bite_alert();
        // Caught portrait overlays everything when the fish is landed
        render_catch_screen();
   // ggprint16(&rFPS, 12, 0x00ffffff, "fps: %i", g.fps);

	}
	
	else if (gameState == SHOPPING) {

		glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, g.shopTex);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, g.yres);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(g.xres, g.yres);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(g.xres, 0);
        glEnd();
		open_shop();
		render_shop_back_button();
    //ggprint16(&rFPS, 12, 0x00ffffff, "fps: %i", g.fps);


	}
	else if (gameState == CHARACTER) {
    	render_character_select();
	}

}
