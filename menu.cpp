// edwin aviles, kian hernando, & simon
// pachinko minigame integrated

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

// ============================================================
// GAME STATE
// ============================================================
enum GameState {
	MENU,
	PLAY,
	FISHING,
	CHARACTER,
	SHOPPING,
	PACHINKO
};

GameState gameState = MENU;

// ============================================================
// IMAGES
// ============================================================
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

// ============================================================
// FISHING / BOAT GLOBALS
// ============================================================
float boatBobTime  = 0.5f;
float boatBobAmp   = 3.0f;
float boatBobSpeed = 0.20f;
int   requestedFish = -1;

const int   NUM_FISH = 5;
const float FISH_BOB_AMP[NUM_FISH]   = { 5.0f,  4.0f,  6.0f,  3.5f,  5.5f  };
const float FISH_BOB_SPEED[NUM_FISH] = { 0.50f, 0.35f, 0.45f, 0.28f, 0.40f };
const float FISH_SPEED[NUM_FISH]     = { 2.5f,  3.5f,  4.5f,  3.0f,  5.0f  };
const float FISH_CY[NUM_FISH]        = { 90.0f, 60.0f, 30.0f, 120.0f, 150.0f };
const float FISH_W[NUM_FISH]         = { 150.0f, 120.0f, 130.0f, 140.0f, 125.0f };
const float FISH_H[NUM_FISH]         = { 120.0f, 100.0f, 110.0f, 110.0f, 105.0f };

const char* FISH_NAMES[NUM_FISH] = {
    "Exo Trout", "Grieselly Fish", "Death Snapper", "Milking Fish", "Reynboh Pescado"
};

const int FISH_PRICES[NUM_FISH] = { 5, 20, 35, 40, 10 };

int fishInventory[NUM_FISH] = { 0, 0, 0, 0, 0 };
int playerGold   = 0;
int shopSelected = 0;

int   slotFish[2] = { 0, 1 };
float slotX[2]    = { 0.0f, 0.0f };
float slotBob[2]  = { 0.0f, 0.5f };

enum FishingPhase { PHASE_NONE, PHASE_WAITING, PHASE_MINIGAME, PHASE_HOOKED };
FishingPhase fishingPhase = PHASE_NONE;

float biteTimer   = 0.0f;
float biteDelay   = 0.0f;
bool  bobberActive = false;
int   hookedFishIndex  = -1;
float biteAlertTimer   = 0.0f;

void reset_fishing_state();

bool  skillCheckActive  = false;
float needleAngle       = 0.0f;
float needleSpeed       = 1.8f;
float scZoneStart       = 0.5f;
const float SC_ZONE_SIZE    = 0.60f;
const float SC_PERFECT_SIZE = 0.2f;

enum SkillResult { SC_NONE, SC_MISS, SC_HIT, SC_GREAT };
SkillResult skillResult = SC_NONE;
float       resultTimer = 0.0f;

const int   CHECKS_NEEDED  = 5;
const float REEL_HIT_AMT   = 1.0f;
const float REEL_GREAT_AMT = 2.0f;
const float REEL_MISS_AMT  = 1.5f;

float reelProgress  = 0.0f;
float reelMax       = (float)CHECKS_NEEDED * REEL_HIT_AMT;
int   totalCaught   = 0;
int   streak        = 0;
float catchMsgTimer = 0.0f;
const int MAX_ATTEMPTS = 5;
int attemptCount = 0;

// ============================================================
// PACHINKO GLOBALS
// ============================================================
static const int   PACH_MAX_BALLS = 1000;
static const float PACH_BALL_R    = 8.0f;
static const float PACH_PEG_R     = 10.0f;
static const int   PACH_COST      = 5;
static const float PACH_GRAVITY   = 0.35f;
static const float PACH_DAMPING   = 0.55f;
static const int   PACH_PEG_ROWS  = 6;
static const int   PACH_PEG_COLS  = 7;
static const int   PACH_BUCKETS   = 7;
static const int   PACH_PAYOUT[PACH_BUCKETS]  = { 0, 5, 10, 0, 10, 5, 0 };
static const char* PACH_LABELS[PACH_BUCKETS]  = { "0", "5", "10", "SLOT!", "10", "5", "0" };

static const float PACH_BKT_WEIGHTS[PACH_BUCKETS] = { 2.0f, 1.6f, 1.3f, 0.4f, 1.3f, 1.6f, 2.0f };
static float pach_bkt_x[PACH_BUCKETS + 1];

struct PachBall {
	float pos[2];
	float last_pos[2];
	float radius;
	float col[3];
	bool  active;
	int   landed_bucket;
};

struct PachPeg {
	float x, y;
};

static PachBall pach_balls[PACH_MAX_BALLS];
static int      pach_n = 0;
static PachPeg  pach_pegs[PACH_PEG_ROWS * PACH_PEG_COLS];
static int      pach_npeg = 0;

static float BOARD_L, BOARD_R, BOARD_T, BOARD_B;
static float BOARD_W, BOARD_H, BKT_W;
static float pach_aim_x      = 0.0f;
static int   pach_total_won  = 0;
static int   pach_balls_used = 0;
static bool  pach_show_result   = false;
static float pach_result_timer  = 0.0f;
static int   pach_last_payout   = 0;

// ── Slot machine state ──────────────────────────────────────
static bool  slot_active       = false;
static bool  slot_spinning     = false;
static float slot_spin_timer   = 0.0f;
static float slot_spin_dur     = 2.5f;
static int   slot_reels[3]     = { 0, 0, 0 };
static float slot_reel_offset[3] = { 0.0f, 0.0f, 0.0f };
static int   slot_result       = -1;
static bool  slot_result_shown = false;
static float slot_result_timer = 0.0f;

static const int   SLOT_NUM_SYMS = 5;
static const char* SLOT_SYM_LABELS[SLOT_NUM_SYMS] = { "Exo", "Grie", "DEATH!", "Milk", "Reyn" };
static const int   SLOT_SYM_PAYOUT[SLOT_NUM_SYMS] = { 5, 25, 10000, 100, 500 };
static const float SLOT_SYM_WEIGHT[SLOT_NUM_SYMS] = { 18.0f, 12.0f, 1.0f, 8.0f, 4.0f };
static const int   SLOT_SYM_IMG[SLOT_NUM_SYMS]    = { 8, 9, 7, 5, 6 };

static const int REEL_LEN = 20;
static int slot_reel_strip[3][REEL_LEN];

// ============================================================
// GLOBAL CLASS + TEXTURES
// ============================================================
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
	width  = w;
	height = h;
	pos[0] = 800/2;
	pos[1] = 30;
	color[0] = 0;
	color[1] = 255;
	color[2] = 240;
	boxImage = nullptr;
	texture  = 0;
	xc[0] = 0.0; xc[1] = 1.0;
	yc[0] = 0.0; yc[1] = 1.0;
}

class Global {
public:
	int xres, yres;
	Texture tex;
	GLuint fishingTex;
	GLuint pescadoTex;
	GLuint partyTex;
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

const int   numOptions = 3;
const char* menuSelect[numOptions] = { "Play", "Select Character", "Quit" };
static const int menuCount    = (int)(sizeof(menuSelect) / sizeof(menuSelect[0]));
static int       menuSelected = 0;
void select_menu_option(int i);

// ============================================================
// X11 WRAPPER
// ============================================================
class X11_wrapper {
private:
	Display   *dpy;
	Window     win;
	GLXContext glc;
public:
	X11_wrapper() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		setup_screen_res(800, 640);
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) { printf("\n\tcannot connect to X server\n\n"); exit(EXIT_FAILURE); }
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) { printf("\n\tno appropriate visual found\n\n"); exit(EXIT_FAILURE); }
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		XSetWindowAttributes swa;
		swa.colormap  = cmap;
		swa.event_mask =
			ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
			ButtonPressMask | ButtonReleaseMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
			vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	void cleanupXWindows() { XDestroyWindow(dpy, win); XCloseDisplay(dpy); }
	void setup_screen_res(const int w, const int h) { g.xres = w; g.yres = h; }
	void reshape_window(int width, int height) {
		setup_screen_res(width, height);
		glViewport(0, 0, (GLint)width, (GLint)height);
		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
		glOrtho(0, g.xres, 0, g.yres, -1, 1);
		set_title();
	}
	void set_title() { XMapWindow(dpy, win); XStoreName(dpy, win, "Pescado"); }
	bool   getXPending()    { return XPending(dpy); }
	XEvent getXNextEvent()  { XEvent e; XNextEvent(dpy, &e); return e; }
	void   swapBuffers()    { glXSwapBuffers(dpy, win); }
	void check_resize(XEvent *e) {
		if (e->type != ConfigureNotify) return;
		XConfigureEvent xce = e->xconfigure;
		if (xce.width != g.xres || xce.height != g.yres)
			reshape_window(xce.width, xce.height);
	}
} x11;

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================
void init_opengl(void);
void check_mouse(XEvent *e);
int  check_keys(XEvent *e);
void physics(void);
void render(void);
// fishing
void start_skill_check();
void on_skill_result(SkillResult r);
void open_shop();
// pachinko
void enter_pachinko();
void pachinko_physics();
void render_pachinko();
// slot
void slot_start();
void slot_spin_finish();
void render_slot_overlay();

// ============================================================
// SLOT HELPERS
// ============================================================
static void slot_build_strips()
{
	for (int r = 0; r < 3; r++) {
		int idx = 0;
		for (int s = 0; s < SLOT_NUM_SYMS && idx < REEL_LEN; s++) {
			int count = (int)SLOT_SYM_WEIGHT[s];
			if (count < 1) count = 1;
			if (s == 2) count = 1;
			for (int k = 0; k < count && idx < REEL_LEN; k++)
				slot_reel_strip[r][idx++] = s;
		}
		while (idx < REEL_LEN) slot_reel_strip[r][idx++] = 0;
		for (int i = REEL_LEN - 1; i > 0; i--) {
			int j = rand() % (i + 1);
			int tmp = slot_reel_strip[r][i];
			slot_reel_strip[r][i] = slot_reel_strip[r][j];
			slot_reel_strip[r][j] = tmp;
		}
	}
}

void slot_start()
{
	slot_active       = true;
	slot_spinning     = true;
	slot_spin_timer   = slot_spin_dur;
	slot_result       = -1;
	slot_result_shown = false;
	slot_result_timer = 0.0f;
	for (int r = 0; r < 3; r++) {
		slot_reels[r]       = rand() % REEL_LEN;
		slot_reel_offset[r] = 0.0f;
	}
	printf("[SLOT] Started spinning!\n"); fflush(stdout);
}

void slot_spin_finish()
{
	slot_spinning = false;
	for (int r = 0; r < 3; r++)
		slot_reels[r] = rand() % REEL_LEN;

	int sym0 = slot_reel_strip[0][slot_reels[0]];
	int sym1 = slot_reel_strip[1][slot_reels[1]];
	int sym2 = slot_reel_strip[2][slot_reels[2]];

	int won = 0;
	if (sym0 == sym1 && sym1 == sym2) {
		won = SLOT_SYM_PAYOUT[sym0];
	} else if (sym0 == sym1 || sym1 == sym2 || sym0 == sym2) {
		int matched = (sym0 == sym1) ? sym0 : ((sym1 == sym2) ? sym1 : sym0);
		won = SLOT_SYM_PAYOUT[matched] / 5;
		if (won < 5) won = 5;
	} else {
		won = 5;
	}

	playerGold += won;
	pach_total_won += won;
	slot_result       = won;
	slot_result_shown = true;
	slot_result_timer = 3.5f;
	printf("[SLOT] Result: %s %s %s → won %d gold\n",
		SLOT_SYM_LABELS[sym0], SLOT_SYM_LABELS[sym1], SLOT_SYM_LABELS[sym2], won);
	fflush(stdout);
}

// ============================================================
// MAIN
// ============================================================
int main()
{
	int nframes   = 0;
	int starttime = time(NULL);
	init_opengl();
	slot_build_strips();
	int done = 0;
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
			nframes   = 0;
		}
		x11.swapBuffers();
	}
	return 0;
}

// ============================================================
// INIT OPENGL
// ============================================================
void init_opengl(void)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glViewport(0, 0, g.xres, g.yres);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_TEXTURE_2D);

	g.tex.backImage = &img[0];
	glGenTextures(1, &g.tex.backTexture);
	int w = g.tex.backImage->width, h = g.tex.backImage->height;
	glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);
	g.tex.xc[0] = 0.0; g.tex.xc[1] = 0.25;
	g.tex.yc[0] = 0.0; g.tex.yc[1] = 1.0;

	glGenTextures(1, &g.fishingTex);
	glBindTexture(GL_TEXTURE_2D, g.fishingTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, img[1].width, img[1].height, 0,
		GL_RGB, GL_UNSIGNED_BYTE, img[1].data);

	unsigned char *alphaDataPescado = buildAlphaData(&img[2]);
	glGenTextures(1, &g.pescadoTex);
	glBindTexture(GL_TEXTURE_2D, g.pescadoTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[2].width, img[2].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataPescado);
	free(alphaDataPescado);

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

	unsigned char *alphaDataDipDip = buildAlphaData(&img[10]);
	glGenTextures(1, &g.dipDipTex);
	glBindTexture(GL_TEXTURE_2D, g.dipDipTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[10].width, img[10].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataDipDip);
	free(alphaDataDipDip);

	unsigned char *alphaDataMilkingFish = buildAlphaData(&img[5]);
	glGenTextures(1, &g.fishOneTex);
	glBindTexture(GL_TEXTURE_2D, g.fishOneTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[5].width, img[5].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataMilkingFish);
	free(alphaDataMilkingFish);

	unsigned char *alphaDataReynboh = buildAlphaData(&img[6]);
	glGenTextures(1, &g.reynbohTex);
	glBindTexture(GL_TEXTURE_2D, g.reynbohTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[6].width, img[6].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataReynboh);
	free(alphaDataReynboh);

	unsigned char *alphaDataDeathSnapper = buildAlphaData(&img[7]);
	glGenTextures(1, &g.deathSnapperTex);
	glBindTexture(GL_TEXTURE_2D, g.deathSnapperTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[7].width, img[7].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataDeathSnapper);
	free(alphaDataDeathSnapper);

	unsigned char *alphaDataExoTrout = buildAlphaData(&img[8]);
	glGenTextures(1, &g.exoTroutTex);
	glBindTexture(GL_TEXTURE_2D, g.exoTroutTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[8].width, img[8].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataExoTrout);
	free(alphaDataExoTrout);

	unsigned char *alphaDataGrieselly = buildAlphaData(&img[9]);
	glGenTextures(1, &g.griesellyTex);
	glBindTexture(GL_TEXTURE_2D, g.griesellyTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[9].width, img[9].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataGrieselly);
	free(alphaDataGrieselly);

	unsigned char *alphaDataShop = buildAlphaData(&img[11]);
	glGenTextures(1, &g.shopTex);
	glBindTexture(GL_TEXTURE_2D, g.shopTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img[11].width, img[11].height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, alphaDataShop);
	free(alphaDataShop);

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

	slotFish[0] = 0; slotFish[1] = 1;
	slotX[0]    = -FISH_W[0] / 2.0f;
	slotX[1]    = g.xres + FISH_W[1] / 2.0f;
	slotBob[0]  = 0.0f; slotBob[1] = 0.5f;
}

// ============================================================
// FISHING HELPERS
// ============================================================
void reset_fishing_state()
{
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

float normalizeAngle(float a)
{
	float twoPi = 2.0f * M_PI;
	return fmodf(fmodf(a, twoPi) + twoPi, twoPi);
}

bool isInArc(float needle, float start, float size)
{
	float n = normalizeAngle(needle);
	float s = normalizeAngle(start);
	float e = normalizeAngle(start + size);
	if (s <= e) return n >= s && n <= e;
	return n >= s || n <= e;
}

void on_skill_result(SkillResult r)
{
	if (attemptCount < MAX_ATTEMPTS) attemptCount++;
	if (r == SC_GREAT) {
		reelProgress += REEL_GREAT_AMT; streak++;
		printf("[SKILL] GREAT HIT  | reel: %.1f / %.1f | streak: %d\n", reelProgress, reelMax, streak);
	} else if (r == SC_HIT) {
		reelProgress += REEL_HIT_AMT; streak++;
		printf("[SKILL] GOOD HIT   | reel: %.1f / %.1f | streak: %d\n", reelProgress, reelMax, streak);
	} else {
		reelProgress -= REEL_MISS_AMT;
		if (reelProgress < 0.0f) reelProgress = 0.0f;
		streak = 0;
		printf("[SKILL] MISSED     | reel: %.1f / %.1f | streak reset\n", reelProgress, reelMax);
	}
	fflush(stdout);
	if (reelProgress >= reelMax) {
		reelProgress     = 0.0f;
		streak           = 0;
		skillCheckActive = false;
		fishingPhase     = PHASE_HOOKED;
		catchMsgTimer    = 2.0f;
		if (hookedFishIndex >= 0 && hookedFishIndex < NUM_FISH)
			fishInventory[hookedFishIndex]++;
		printf("[REEL] *** FISH HOOKED! ***\n"); fflush(stdout);
	}
}

void start_skill_check()
{
	uniform_real_distribution<float> zoneDist(0.4f, 2.0f * M_PI - SC_ZONE_SIZE - 0.4f);
	float minSpeed = 1.4f + (reelProgress / reelMax) * 0.8f;
	float maxSpeed = 2.4f + (reelProgress / reelMax) * 1.0f;
	uniform_real_distribution<float> speedDist(minSpeed, maxSpeed);
	scZoneStart      = zoneDist(gen);
	needleSpeed      = speedDist(gen);
	needleAngle      = 0.0f;
	skillCheckActive = true;
	skillResult      = SC_NONE;
	printf("[SKILL] New check | zone: %.2f | speed: %.2f\n", scZoneStart, needleSpeed);
	fflush(stdout);
}

// ============================================================
// PACHINKO HELPERS
// ============================================================
static void pach_build_board()
{
	BOARD_W = g.xres * 0.70f;
	BOARD_H = g.yres * 0.80f;
	BOARD_L = (g.xres - BOARD_W) * 0.5f;
	BOARD_R = BOARD_L + BOARD_W;
	BOARD_B = 30.0f;
	BOARD_T = BOARD_B + BOARD_H;

	float totalWeight = 0.0f;
	for (int k = 0; k < PACH_BUCKETS; k++) totalWeight += PACH_BKT_WEIGHTS[k];
	pach_bkt_x[0] = BOARD_L;
	for (int k = 0; k < PACH_BUCKETS; k++)
		pach_bkt_x[k+1] = pach_bkt_x[k] + (PACH_BKT_WEIGHTS[k] / totalWeight) * BOARD_W;
	BKT_W = BOARD_W / PACH_BUCKETS;

	pach_npeg = 0;
	float rowH = (BOARD_H - 100.0f) / (PACH_PEG_ROWS + 1);
	float colW = BOARD_W / (PACH_PEG_COLS + 1);
	for (int r = 0; r < PACH_PEG_ROWS; r++) {
		float offset = (r % 2 == 0) ? 0.0f : colW * 0.5f;
		int   cols   = (r % 2 == 0) ? PACH_PEG_COLS : PACH_PEG_COLS - 1;
		float y      = BOARD_B + 70.0f + (r + 1) * rowH;
		for (int c = 0; c < cols; c++) {
			if (pach_npeg >= PACH_PEG_ROWS * PACH_PEG_COLS) break;
			pach_pegs[pach_npeg].x = BOARD_L + offset + (c + 1) * colW;
			pach_pegs[pach_npeg].y = y;
			pach_npeg++;
		}
	}
}

void enter_pachinko()
{
	gameState         = PACHINKO;
	pach_n            = 0;
	pach_total_won    = 0;
	pach_balls_used   = 0;
	pach_show_result  = false;
	pach_result_timer = 0.0f;
	pach_last_payout  = 0;
	slot_active       = false;
	slot_spinning     = false;
	memset(pach_balls, 0, sizeof(pach_balls));
	pach_build_board();
	pach_aim_x = g.xres * 0.5f;
	printf("[PACHINKO] Entered. Gold: %d\n", playerGold);
	fflush(stdout);
}

static void pach_launch_ball(float x)
{
	if (slot_active) {
		printf("[PACHINKO] Slot in progress, wait.\n"); fflush(stdout);
		return;
	}
	if (playerGold < PACH_COST) {
		printf("[PACHINKO] Not enough gold!\n"); fflush(stdout);
		return;
	}
	for (int i = 0; i < PACH_MAX_BALLS; i++) {
		if (!pach_balls[i].active) {
			playerGold -= PACH_COST;
			pach_balls_used++;
			PachBall &b   = pach_balls[i];
			b.active      = true;
			b.radius      = PACH_BALL_R;
			b.pos[0]      = x;
			b.pos[1]      = BOARD_T - PACH_BALL_R - 2.0f;
			float nudge   = ((rand() % 201) - 100) * 0.005f;
			b.last_pos[0] = b.pos[0] - nudge;
			b.last_pos[1] = b.pos[1] + 1.0f;
			b.landed_bucket = -1;
			b.col[0] = 0.7f + (rand() % 30) / 100.0f;
			b.col[1] = 0.2f + (rand() % 50) / 100.0f;
			b.col[2] = 0.1f + (rand() % 40) / 100.0f;
			if (pach_n <= i) pach_n = i + 1;
			printf("[PACHINKO] Launched at x=%.0f. Gold: %d\n", x, playerGold);
			fflush(stdout);
			return;
		}
	}
	printf("[PACHINKO] Max balls in play.\n"); fflush(stdout);
}

// ============================================================
// CHECK MOUSE
// ============================================================
void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	if (e->type == ButtonRelease) return;

	if (e->type == ButtonPress) {
		if (e->xbutton.button == 1) {
			printf("click\n"); fflush(stdout);
			if (gameState == PLAY) {
				printf("Rod cast! Waiting for a bite...\n"); fflush(stdout);
				reset_fishing_state();
				gameState    = FISHING;
				fishingPhase = PHASE_WAITING;
				bobberActive = true;
				uniform_real_distribution<float> biteDist(2.0f, 6.0f);
				biteDelay = biteDist(gen);
				biteTimer = biteDelay;
				fflush(stdout);
			}
			if (gameState == PACHINKO) {
				if (slot_active && !slot_spinning && slot_result_shown) {
					slot_active       = false;
					slot_result_shown = false;
				} else if (!slot_active) {
					pach_launch_ball(pach_aim_x);
				}
			}
		}
		if (e->xbutton.button == 3) { }
	}

	if (e->type == MotionNotify) {
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			if (gameState == PACHINKO && !slot_active) {
				pach_aim_x = (float)e->xbutton.x;
				if (pach_aim_x < BOARD_L + PACH_BALL_R) pach_aim_x = BOARD_L + PACH_BALL_R;
				if (pach_aim_x > BOARD_R - PACH_BALL_R) pach_aim_x = BOARD_R - PACH_BALL_R;
			}
		}
	}
}

// ============================================================
// CHECK KEYS
// ============================================================
int check_keys(XEvent *e)
{
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);

		// ESC
		if (key == XK_Escape) {
			if (gameState == FISHING) {
				gameState = PLAY; skillCheckActive = false; return 0;
			}
			if (gameState == SHOPPING) { gameState = PLAY; return 0; }
			if (gameState == PACHINKO) {
				if (slot_active) {
					if (!slot_spinning) { slot_active = false; slot_result_shown = false; }
				} else {
					gameState = PLAY;
				}
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
			return 0;
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

		// open pachinko
		if ((key == XK_p || key == XK_P) && gameState == PLAY) {
			enter_pachinko();
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
			printf("[TEST] Entered FISHING, waiting %.1f s\n", biteDelay);
			fflush(stdout);
		}

		// SPACE: skill check input in FISHING
		if (key == XK_space) {
			if (gameState == FISHING && skillCheckActive) {
				skillCheckActive = false;
				if (isInArc(needleAngle, scZoneStart, SC_PERFECT_SIZE)) {
					skillResult = SC_GREAT; resultTimer = 0.8f;
				} else if (isInArc(needleAngle, scZoneStart, SC_ZONE_SIZE)) {
					skillResult = SC_HIT;   resultTimer = 0.8f;
				} else {
					skillResult = SC_MISS;  resultTimer = 0.8f;
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
			return 0;
		}

		// SHOPPING block
		if (gameState == SHOPPING) {
			if (key == XK_b || key == XK_B) {
				gameState = PLAY;
				return 0;
			}
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

// ============================================================
// PHYSICS
// ============================================================
void physics()
{
	if (gameState == MENU) {
		g.tex.xc[0] += 0.001f; g.tex.xc[1] += 0.001f;
		g.logoAngle  += 3.0f;
		if (g.logoAngle >= 360.0f) g.logoAngle -= 360.0f;
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
	
		boatBobTime += boatBobSpeed;
		for (int s = 0; s < 2; s++) {
			if ((fishingPhase == PHASE_MINIGAME || fishingPhase == PHASE_HOOKED)
				&& slotFish[s] == hookedFishIndex) {
				slotBob[s] += FISH_BOB_SPEED[slotFish[s]];
				continue;
			}
			slotBob[s] += FISH_BOB_SPEED[slotFish[s]];
			slotX[s]   += (s == 0) ? FISH_SPEED[slotFish[s]] : -FISH_SPEED[slotFish[s]];
		}
	}


	if (gameState == FISHING) {
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

		if (fishingPhase == PHASE_WAITING) {
			float boatW   = img[10].width * 4.0f;
			float boatCX  = g.xres / 2.0f;
			float rodTipX = boatCX - boatW * 0.25f;
			float hitZoneW = 24.0f;
			for (int s = 0; s < 2; s++) {
				float fishCX = slotX[s];
				float zoneL  = rodTipX - hitZoneW / 2.0f;
				float zoneR  = rodTipX + hitZoneW / 2.0f;
				if (fishCX >= zoneL && fishCX <= zoneR) {
					hookedFishIndex = slotFish[s];
					biteAlertTimer  = 1.5f;
					fishingPhase    = PHASE_MINIGAME;
					printf("[FISHING] Fish hit rod! Type: %d\n", hookedFishIndex); fflush(stdout);
					start_skill_check();
					break;
				}
			}
		}

		if (fishingPhase == PHASE_MINIGAME) {
			if (biteAlertTimer > 0.0f) biteAlertTimer -= dt;
			if (skillCheckActive) {
				needleAngle += needleSpeed * dt;
				if (needleAngle > 2.0f * M_PI) needleAngle -= 2.0f * M_PI;
			}
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

		if (fishingPhase == PHASE_HOOKED) {
			if (catchMsgTimer > 0.0f) {
				catchMsgTimer -= dt;
				if (catchMsgTimer <= 0.0f) {
					catchMsgTimer = 0.0f;
					totalCaught++;
					printf("[FISHING] Fish caught! Total: %d. Back to PLAY.\n", totalCaught);
					fflush(stdout);
					reset_fishing_state();
					gameState = PLAY;
				}
			}
		}

		for (int s = 0; s < 2; s++) {
			if (fishingPhase == PHASE_MINIGAME || fishingPhase == PHASE_HOOKED)
				if (slotFish[s] == hookedFishIndex) continue;
			int   fi    = slotFish[s];
			float speed = FISH_SPEED[fi];
			slotBob[s] += FISH_BOB_SPEED[fi];
			slotX[s]   += (s == 0) ? speed : -speed;
			float halfW = FISH_W[fi] / 2.0f;
			bool exited = (s == 0) ? (slotX[s] - halfW > g.xres) : (slotX[s] + halfW < 0);
			if (exited) {
				int next = (fi + 2) % NUM_FISH;
				slotFish[s] = next; slotBob[s] = 0.0f;
				float nextHalfW = FISH_W[next] / 2.0f;
				slotX[s] = (s == 0) ? -nextHalfW : g.xres + nextHalfW;
			}
		}
	}

	// ── Pachinko + slot physics ──────────────────────────────
	if (gameState == PACHINKO) {
		if (pach_show_result && pach_result_timer > 0.0f) {
			pach_result_timer -= 0.016f;
			if (pach_result_timer <= 0.0f) pach_show_result = false;
		}

		if (slot_spinning) {
			slot_spin_timer -= 0.016f;
			for (int r = 0; r < 3; r++)
				slot_reel_offset[r] += 8.0f + r * 2.0f;
			if (slot_spin_timer <= 0.0f)
				slot_spin_finish();
		}
		if (slot_result_shown && slot_result_timer > 0.0f) {
			slot_result_timer -= 0.016f;
			if (slot_result_timer <= 0.0f) {
				slot_result_shown = false;
				slot_active       = false;
			}
		}

		for (int i = 0; i < pach_n; i++) {
			PachBall &b = pach_balls[i];
			if (!b.active || b.landed_bucket >= 0) continue;

			float vel[2] = {
				b.pos[0] - b.last_pos[0],
				b.pos[1] - b.last_pos[1]
			};
			vel[1] -= PACH_GRAVITY;
			float speed = sqrtf(vel[0]*vel[0] + vel[1]*vel[1]);
			if (speed > 6.0f) { vel[0] = vel[0]/speed*6.0f; vel[1] = vel[1]/speed*6.0f; }

			b.last_pos[0] = b.pos[0];
			b.last_pos[1] = b.pos[1];
			b.pos[0] += vel[0];
			b.pos[1] += vel[1];

			if (b.pos[0] - b.radius < BOARD_L) {
				b.pos[0] = BOARD_L + b.radius;
				b.last_pos[0] = b.pos[0] + fabsf(vel[0]) * PACH_DAMPING;
			}
			if (b.pos[0] + b.radius > BOARD_R) {
				b.pos[0] = BOARD_R - b.radius;
				b.last_pos[0] = b.pos[0] - fabsf(vel[0]) * PACH_DAMPING;
			}

			for (int p = 0; p < pach_npeg; p++) {
				float dx   = b.pos[0] - pach_pegs[p].x;
				float dy   = b.pos[1] - pach_pegs[p].y;
				float dist = sqrtf(dx*dx + dy*dy);
				float minD = b.radius + PACH_PEG_R;
				if (dist < minD && dist > 0.001f) {
					float nx = dx/dist, ny = dy/dist;
					b.pos[0] += nx * (minD - dist);
					b.pos[1] += ny * (minD - dist);
					float dot = vel[0]*nx + vel[1]*ny;
					b.last_pos[0] = b.pos[0] - (vel[0] - 2.0f*dot*nx) * PACH_DAMPING;
					b.last_pos[1] = b.pos[1] - (vel[1] - 2.0f*dot*ny) * PACH_DAMPING;
				}
			}

			for (int j = i+1; j < pach_n; j++) {
				PachBall &c = pach_balls[j];
				if (!c.active || c.landed_bucket >= 0) continue;
				float dx = b.pos[0]-c.pos[0], dy = b.pos[1]-c.pos[1];
				float dist = sqrtf(dx*dx+dy*dy);
				float minD = b.radius + c.radius;
				if (dist < minD && dist > 0.001f) {
					float nx = dx/dist, ny = dy/dist;
					float half = (minD-dist)*0.5f;
					b.pos[0] += nx*half; b.pos[1] += ny*half;
					c.pos[0] -= nx*half; c.pos[1] -= ny*half;
				}
			}

			if (b.pos[1] - b.radius <= BOARD_B + 30.0f) {
				int bkt = PACH_BUCKETS - 1;
				for (int k = 0; k < PACH_BUCKETS; k++) {
					if (b.pos[0] >= pach_bkt_x[k] && b.pos[0] < pach_bkt_x[k+1]) {
						bkt = k; break;
					}
				}
				b.landed_bucket   = bkt;
				b.pos[1]          = BOARD_B + 30.0f + b.radius;
				b.last_pos[0]     = b.pos[0];
				b.last_pos[1]     = b.pos[1];

				if (bkt == 3) {
					pach_last_payout  = 0;
					pach_show_result  = true;
					pach_result_timer = 0.5f;
					slot_start();
					printf("[PACHINKO] Bucket 3 → SLOT MACHINE triggered!\n");
				} else {
					int won           = PACH_PAYOUT[bkt];
					playerGold       += won;
					pach_total_won   += won;
					pach_last_payout  = won;
					pach_show_result  = true;
					pach_result_timer = 1.5f;
					printf("[PACHINKO] Bucket %d → +%d gold. Total gold: %d\n", bkt, won, playerGold);
				}
				fflush(stdout);
			}
		}
	}
}

// ============================================================
// RENDER HELPERS
// ============================================================
void render_box()
{
	glDisable(GL_TEXTURE_2D);
	glColor3ub(box.color[0], box.color[1], box.color[2]);
	glPushMatrix();
	glTranslatef(box.pos[0], box.pos[1], 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-box.width/2, 0); glVertex2f(-box.width/2, box.height);
		glVertex2f( box.width/2, box.height); glVertex2f( box.width/2, 0);
	glEnd();
	glPopMatrix();
	glColor3ub(top.color[0], top.color[1], top.color[2]);
	glPushMatrix();
	glTranslatef(top.pos[0], top.pos[1], 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-top.width/2, 0); glVertex2f(-top.width/2, top.height);
		glVertex2f( top.width/2, top.height); glVertex2f( top.width/2, 0);
	glEnd();
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

void select_menu_option(int i)
{
	switch (i) {
		case 0:
			printf("Start Game selected\n");
			gameState = PLAY;

			break;
		case 1:
			printf("Character selected\n");
			gameState = CHARACTER;
			charSelectCursor = selectedCharacter;
			return;
		case 2:
			printf("Quit selected\n");
			exit(0);
			break;
	
	}
}

void render_menu()
{
	const int paddingX = 25, paddingY = 40, lineStep = 30;
	int left = (int)(box.pos[0] - box.width/2 + paddingX);
	int topY = (int)(box.pos[1] + box.height - paddingY);
	Rect r; r.center = 0; r.left = left; r.bot = topY;
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (int i = 0; i < menuCount; i++) {
		unsigned int color = (menuSelected == i) ? 0x00ff0000 : 0x00000000;
		if (i == menuSelected) ggprint16(&r, lineStep, color, "> %s", menuSelect[i]);
		else                   ggprint16(&r, lineStep, color, "  %s", menuSelect[i]);
	}
	glDisable(GL_BLEND);
}

void render_logo()
{
	float w = 500.0f, h = 240.0f;
	float x = (g.xres - w) / 2, y = g.yres - h - 70;
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glColor4f(1,1,1,1); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, g.partyTex);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1); glVertex2f(x,   y);
		glTexCoord2f(0,0); glVertex2f(x,   y+h);
		glTexCoord2f(1,0); glVertex2f(x+w, y+h);
		glTexCoord2f(1,1); glVertex2f(x+w, y);
	glEnd();
	glDisable(GL_BLEND);
}

void render_senor_pescado()
{
	float boxRight = box.pos[0] + box.width * 0.5f;
	float boxBottom = box.pos[1], boxTop = box.pos[1] + box.height;
	float drawW = 70.0f, drawH = 70.0f;
	float cx = boxRight - 25.0f - drawW * 0.5f;
	float cy = (boxBottom + boxTop) * 0.5f;
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1); glBindTexture(GL_TEXTURE_2D, g.pescadoTex);
	glPushMatrix();
	glTranslatef(cx, cy, 0.0f); glRotatef(g.logoAngle, 0,0,1);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1); glVertex2f(-drawW/2,-drawH/2);
		glTexCoord2f(0,0); glVertex2f(-drawW/2, drawH/2);
		glTexCoord2f(1,0); glVertex2f( drawW/2, drawH/2);
		glTexCoord2f(1,1); glVertex2f( drawW/2,-drawH/2);
	glEnd();
	glPopMatrix();
	glDisable(GL_BLEND);
}


void render_fish_slot(int s)
{
	GLuint textures[NUM_FISH] = {
		g.fishOneTex, g.reynbohTex, g.deathSnapperTex, g.exoTroutTex, g.griesellyTex
	};
	int   fi = slotFish[s];
	float w  = FISH_W[fi], h = FISH_H[fi];
	float cx = slotX[s], cy = FISH_CY[fi];
	float bob = sinf(slotBob[s]) * FISH_BOB_AMP[fi];
	float texLeft  = (s == 0) ? 0.0f : 1.0f;
	float texRight = (s == 0) ? 1.0f : 0.0f;
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glColor4f(1,1,1,1); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, textures[fi]);
	glPushMatrix(); glTranslatef(cx, cy + bob, 0.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(texLeft, 1); glVertex2f(-w/2,-h/2);
		glTexCoord2f(texLeft, 0); glVertex2f(-w/2, h/2);
		glTexCoord2f(texRight,0); glVertex2f( w/2, h/2);
		glTexCoord2f(texRight,1); glVertex2f( w/2,-h/2);
	glEnd();
	glDisable(GL_BLEND); glPopMatrix();
}

void render_bite_alert()
{
	if (biteAlertTimer <= 0.0f) return;
	float alpha = 0.7f + 0.3f * sinf(biteAlertTimer * 20.0f);
	glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float stripH = 40.0f, stripY = g.yres * 0.5f;
	glColor4f(0,0,0, alpha * 0.6f);
	glBegin(GL_QUADS);
		glVertex2f(0,      stripY - stripH*0.5f); glVertex2f(0,      stripY + stripH*0.5f);
		glVertex2f(g.xres, stripY + stripH*0.5f); glVertex2f(g.xres, stripY - stripH*0.5f);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	Rect r; r.center = 1; r.left = g.xres/2; r.bot = (int)(stripY - 10);
	ggprint16(&r, 0, 0x00ffcc00, "!  FISH ON THE LINE  !");
	glDisable(GL_BLEND);
}

void render_catch_screen()
{
	if (fishingPhase != PHASE_HOOKED || catchMsgTimer <= 0.0f) return;
	if (hookedFishIndex < 0 || hookedFishIndex >= NUM_FISH) return;
	GLuint fishTextures[NUM_FISH] = {
		g.fishOneTex, g.reynbohTex, g.deathSnapperTex, g.exoTroutTex, g.griesellyTex
	};
	glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0,0.05f,0.1f,0.72f);
	glBegin(GL_QUADS);
		glVertex2f(0,0); glVertex2f(0,g.yres); glVertex2f(g.xres,g.yres); glVertex2f(g.xres,0);
	glEnd();
	float panW = 320.0f, panH = 280.0f;
	float panX = (g.xres-panW)*0.5f, panY = (g.yres-panH)*0.5f;
	glColor4f(0.05f,0.15f,0.25f,0.92f);
	glBegin(GL_QUADS);
		glVertex2f(panX,panY); glVertex2f(panX,panY+panH);
		glVertex2f(panX+panW,panY+panH); glVertex2f(panX+panW,panY);
	glEnd();
	glLineWidth(2.5f); glColor4f(1.0f,0.85f,0.2f,1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(panX,panY); glVertex2f(panX,panY+panH);
		glVertex2f(panX+panW,panY+panH); glVertex2f(panX+panW,panY);
	glEnd();
	glDisable(GL_BLEND);
	float imgW = FISH_W[hookedFishIndex]*1.3f, imgH = FISH_H[hookedFishIndex]*1.3f;
	float imgCX = g.xres*0.5f, imgCY = panY+panH*0.58f;
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1); glBindTexture(GL_TEXTURE_2D, fishTextures[hookedFishIndex]);
	glBegin(GL_QUADS);
		glTexCoord2f(0,1); glVertex2f(imgCX-imgW*0.5f, imgCY-imgH*0.5f);
		glTexCoord2f(0,0); glVertex2f(imgCX-imgW*0.5f, imgCY+imgH*0.5f);
		glTexCoord2f(1,0); glVertex2f(imgCX+imgW*0.5f, imgCY+imgH*0.5f);
		glTexCoord2f(1,1); glVertex2f(imgCX+imgW*0.5f, imgCY-imgH*0.5f);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect r; r.center = 1; r.left = g.xres/2;
	r.bot = (int)(panY+panH-22); ggprint16(&r, 0, 0x00ffe94f, "FISH CAUGHT!");
	r.bot = (int)(panY+30);      ggprint16(&r, 0, 0x00ffffff, "%s", FISH_NAMES[hookedFishIndex]);
	r.bot = (int)(panY+10);      ggprint16(&r, 0, 0x00aaddff, "Total caught: %d", totalCaught+1);
	glDisable(GL_BLEND);
}

void render_skill_check()
{
	float cx = g.xres * 0.78f, cy = g.yres * 0.55f, r = 80.0f;
	const int SEG = 120;
	glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(14.0f); glColor4f(0,0,0,0.55f);
	glBegin(GL_LINE_LOOP);
	for (int i=0;i<SEG;i++){float a=(float)i/SEG*2*M_PI; glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);}
	glEnd();
	glLineWidth(6.0f); glColor4f(0.1f,0.22f,0.35f,1);
	glBegin(GL_LINE_LOOP);
	for (int i=0;i<SEG;i++){float a=(float)i/SEG*2*M_PI; glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);}
	glEnd();
	glLineWidth(8.0f); glColor4f(0.18f,0.8f,0.44f,1);
	int zSegs = max(2,(int)(SEG*SC_ZONE_SIZE/(2*M_PI)));
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=zSegs;i++){float a=scZoneStart+(float)i/zSegs*SC_ZONE_SIZE; glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);}
	glEnd();
	glLineWidth(8.0f); glColor4f(1.0f,0.85f,0.2f,1);
	int pSegs = max(2,(int)(SEG*SC_PERFECT_SIZE/(2*M_PI)));
	glBegin(GL_LINE_STRIP);
	for (int i=0;i<=pSegs;i++){float a=scZoneStart+(float)i/pSegs*SC_PERFECT_SIZE; glVertex2f(cx+cosf(a)*r,cy+sinf(a)*r);}
	glEnd();
	glLineWidth(2.5f); glColor4f(1,0.26f,0.26f,1);
	glBegin(GL_LINES);
		glVertex2f(cx,cy); glVertex2f(cx+cosf(needleAngle)*r,cy+sinf(needleAngle)*r);
	glEnd();
	glPointSize(9.0f); glColor4f(1,0.45f,0.45f,1);
	glBegin(GL_POINTS); glVertex2f(cx+cosf(needleAngle)*r,cy+sinf(needleAngle)*r); glEnd();
	glPointSize(7.0f); glColor4f(0.9f,0.95f,1,1);
	glBegin(GL_POINTS); glVertex2f(cx,cy); glEnd();
	glDisable(GL_BLEND);

	glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float barW=160,barH=12,barX=cx-barW/2,barY=cy-r-30;
	float filled = (reelProgress/reelMax)*barW;
	if (filled>barW) filled=barW;
	glColor4f(0.1f,0.1f,0.15f,0.8f);
	glBegin(GL_QUADS);
		glVertex2f(barX,barY); glVertex2f(barX,barY+barH);
		glVertex2f(barX+barW,barY+barH); glVertex2f(barX+barW,barY);
	glEnd();
	float fillRatio = reelProgress/reelMax;
	if (fillRatio>0.66f) glColor4f(0.18f,0.8f,0.44f,1);
	else                 glColor4f(0.2f,0.55f,0.9f,1);
	glBegin(GL_QUADS);
		glVertex2f(barX,barY); glVertex2f(barX,barY+barH);
		glVertex2f(barX+filled,barY+barH); glVertex2f(barX+filled,barY);
	glEnd();
	glColor4f(0.7f,0.85f,1,0.6f); glLineWidth(1.5f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(barX,barY); glVertex2f(barX,barY+barH);
		glVertex2f(barX+barW,barY+barH); glVertex2f(barX+barW,barY);
	glEnd();
	glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect rHud; rHud.center=1; rHud.left=(int)cx;
	if (fishingPhase==PHASE_MINIGAME) {
		Rect rHint; rHint.center=1; rHint.left=(int)cx; rHint.bot=(int)(barY-4);
		ggprint16(&rHint,20,0x00aaddff,"SPACE to reel!");
		rHint.bot=(int)(barY-24);
		unsigned int ac = (MAX_ATTEMPTS-attemptCount==1)?0x00ff4f4f:0x00ffffff;
		ggprint16(&rHint,0,ac,"Attempts: %d / %d",attemptCount,MAX_ATTEMPTS);
	}
	if (streak>1) { rHud.bot=(int)(cy+r+30); ggprint16(&rHud,0,0x00ffe94f,"STREAK x%d",streak); }
	Rect rCorner; rCorner.center=0; rCorner.left=10; rCorner.bot=g.yres-20;
	ggprint16(&rCorner,0,0x00c8e6ff,"FISH CAUGHT: %d",totalCaught);
	if (skillResult!=SC_NONE && resultTimer>0) {
		rHud.bot=(int)(cy+r+55);
		unsigned int col; const char *msg;
		if      (skillResult==SC_GREAT){col=0x00ffe94f;msg="GREAT!";}
		else if (skillResult==SC_HIT)  {col=0x004fffb0;msg="NICE!";}
		else                           {col=0x00ff4f4f;msg="MISSED!";}
		ggprint16(&rHud,0,col,msg);
	}
	glDisable(GL_BLEND);
}

void render_shop_back_button()
{
	float boxW=180,boxH=32,pad=12;
	float x=g.xres-boxW-pad, y=g.yres-boxH-pad;
	glDisable(GL_TEXTURE_2D);
	glColor3ub(255,255,255);
	glBegin(GL_QUADS);
		glVertex2f(x,y); glVertex2f(x,y+boxH); glVertex2f(x+boxW,y+boxH); glVertex2f(x+boxW,y);
	glEnd();
	glColor3ub(0,0,0);
	glBegin(GL_LINE_LOOP);
		glVertex2f(x,y); glVertex2f(x,y+boxH); glVertex2f(x+boxW,y+boxH); glVertex2f(x+boxW,y);
	glEnd();
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect r; r.center=1; r.left=(int)(x+boxW/2); r.bot=(int)(y+10);
	ggprint16(&r, 0, 0x00000000, "[B] Back to Fishing");
	glDisable(GL_BLEND);
}

// ============================================================
// OPEN SHOP
// ============================================================
void open_shop()
{
	GLuint fishTextures[NUM_FISH] = {
		g.fishOneTex, g.reynbohTex, g.deathSnapperTex, g.exoTroutTex, g.griesellyTex
	};

	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);

	// Top shelf — first 2 fish
	float fishScale  = 0.700f;
	float topShelfY  = 95.0f;
	float topStartX  = 390.0f;
	float topSpacing = 90.0f;

	for (int i = 0; i < 2; i++) {
		if (fishInventory[i] <= 0) continue;
		float fw = FISH_W[i] * fishScale;
		float fh = FISH_H[i] * fishScale;
		float fx = topStartX + i * topSpacing;
		glBindTexture(GL_TEXTURE_2D, fishTextures[i]);
		glPushMatrix(); glTranslatef(fx, topShelfY, 0.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1); glVertex2f(-fw/2,-fh/2);
			glTexCoord2f(0,0); glVertex2f(-fw/2, fh/2);
			glTexCoord2f(1,0); glVertex2f( fw/2, fh/2);
			glTexCoord2f(1,1); glVertex2f( fw/2,-fh/2);
		glEnd(); glPopMatrix();
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
		glPushMatrix(); glTranslatef(fx, botShelfY, 0.0f);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1); glVertex2f(-fw/2,-fh/2);
			glTexCoord2f(0,0); glVertex2f(-fw/2, fh/2);
			glTexCoord2f(1,0); glVertex2f( fw/2, fh/2);
			glTexCoord2f(1,1); glVertex2f( fw/2,-fh/2);
		glEnd(); glPopMatrix();
	}
	glDisable(GL_BLEND);

	// Gold display
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect rGold; rGold.center=0; rGold.left=10; rGold.bot=g.yres-20;
	ggprint16(&rGold,0,0x00000000,"Gold: %d",playerGold);
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
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-w/4,  h/4);
		glTexCoord2f(1.0f, 0.0f); glVertex2f( w/4,  h/4);
		glTexCoord2f(1.0f, 1.0f); glVertex2f( w/4, -h/4);
	glEnd();
	glDisable(GL_BLEND);
	glPopMatrix();

	// ── Customer speech bubble ────────────────────────────────────
	float boxW = 350.0f;
	float boxH = 40.0f;
	float boxX = cx - boxW / 2.0f;
	float boxY = cy - h / 4.0f - 20.0f;

	glDisable(GL_TEXTURE_2D);
	glColor3ub(255, 127, 80);
	glBegin(GL_QUADS);
		glVertex2f(boxX,        boxY);
		glVertex2f(boxX,        boxY + boxH);
		glVertex2f(boxX + boxW, boxY + boxH);
		glVertex2f(boxX + boxW, boxY);
	glEnd();
	glColor3ub(0, 0, 0);
	glBegin(GL_LINE_LOOP);
		glVertex2f(boxX,        boxY);
		glVertex2f(boxX,        boxY + boxH);
		glVertex2f(boxX + boxW, boxY + boxH);
		glVertex2f(boxX + boxW, boxY);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect r;
	r.center = 1;
	r.left = (int)cx;
	r.bot = (int)(boxY + 12);
	if (requestedFish >= 0 && requestedFish < NUM_FISH)
		ggprint16(&r, 0, 0x00000000, "Howdy! Can I get a %s?", FISH_NAMES[requestedFish]);
	glDisable(GL_BLEND);

	// ── Inventory panel ───────────────────────────────────────────
	float panW  = 300.0f;
	float panH  = 180.0f;
	float panX  = 10.0f;
	float panY  = (g.yres - panH) / 2.0f;
	float rowH  = panH / NUM_FISH;

	glDisable(GL_TEXTURE_2D);
	glColor4f(0.05f,0.1f,0.15f,0.85f); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f(panX,panY); glVertex2f(panX,panY+panH);
		glVertex2f(panX+panW,panY+panH); glVertex2f(panX+panW,panY);
	glEnd();
	glLineWidth(1.5f); glColor4f(0.7f,0.85f,1,0.7f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(panX,panY); glVertex2f(panX,panY+panH);
		glVertex2f(panX+panW,panY+panH); glVertex2f(panX+panW,panY);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Rect rHeader;
	rHeader.center = 0;
	rHeader.left   = (int)(panX + 8);
	rHeader.bot    = (int)(panY + panH - 4);
	ggprint16(&rHeader, 0, 0x00000000, "Your Inventory");

	for (int i = 0; i < NUM_FISH; i++) {
		float rowY = panY + panH - 26 - (i + 1) * rowH * 0.78f;
		bool wanted = (i == requestedFish);
		if (wanted) {
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.2f,0.55f,0.2f,0.4f);
			glBegin(GL_QUADS);
				glVertex2f(panX+2,rowY-2); glVertex2f(panX+2,rowY+rowH*0.7f);
				glVertex2f(panX+panW-2,rowY+rowH*0.7f); glVertex2f(panX+panW-2,rowY-2);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}
		Rect rRow; rRow.center=0; rRow.left=(int)(panX+8); rRow.bot=(int)rowY;
		unsigned int col = wanted ? 0x0088ff44 : (fishInventory[i]>0 ? 0x00ffffff : 0x00666666);
		ggprint16(&rRow,0,col,"%-16s x%d  ($%d)",FISH_NAMES[i],fishInventory[i],FISH_PRICES[i]);
	}
	glDisable(GL_BLEND);

	// Sell / Decline buttons
	bool canSell = (requestedFish >= 0 && fishInventory[requestedFish] > 0);
	float btnY=panY-55, btnH=36, btnW=140, sellX=panX, decX=panX+btnW+10;
	glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (canSell) glColor4f(0.15f,0.65f,0.25f,1);
	else         glColor4f(0.25f,0.25f,0.25f,0.7f);
	glBegin(GL_QUADS);
		glVertex2f(sellX,btnY); glVertex2f(sellX,btnY+btnH);
		glVertex2f(sellX+btnW,btnY+btnH); glVertex2f(sellX+btnW,btnY);
	glEnd();
	glColor4f(0.65f,0.15f,0.15f,1);
	glBegin(GL_QUADS);
		glVertex2f(decX,btnY); glVertex2f(decX,btnY+btnH);
		glVertex2f(decX+btnW,btnY+btnH); glVertex2f(decX+btnW,btnY);
	glEnd();
	glLineWidth(1.5f); glColor4f(1,1,1,0.5f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(sellX,btnY); glVertex2f(sellX,btnY+btnH);
		glVertex2f(sellX+btnW,btnY+btnH); glVertex2f(sellX+btnW,btnY);
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex2f(decX,btnY); glVertex2f(decX,btnY+btnH);
		glVertex2f(decX+btnW,btnY+btnH); glVertex2f(decX+btnW,btnY);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
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
// ============================================================
// SLOT MACHINE OVERLAY RENDER
// ============================================================
void render_slot_overlay()
{
	if (!slot_active) return;

	auto get_slot_tex = [](int symIdx) -> GLuint {
		switch (symIdx) {
			case 0: return g.exoTroutTex;
			case 1: return g.griesellyTex;
			case 2: return g.deathSnapperTex;
			case 3: return g.fishOneTex;
			case 4: return g.reynbohTex;
			default: return g.exoTroutTex;
		}
	};

	glDisable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0f, 0.0f, 0.0f, 0.78f);
	glBegin(GL_QUADS);
		glVertex2f(0,0); glVertex2f(0,g.yres);
		glVertex2f(g.xres,g.yres); glVertex2f(g.xres,0);
	glEnd();

	float panW=380, panH=260;
	float panX=(g.xres-panW)*0.5f, panY=(g.yres-panH)*0.5f;

	glColor4f(0,0,0,0.5f);
	glBegin(GL_QUADS);
		glVertex2f(panX+6,panY-6); glVertex2f(panX+6,panY+panH-6);
		glVertex2f(panX+panW+6,panY+panH-6); glVertex2f(panX+panW+6,panY-6);
	glEnd();

	glColor4f(0.12f, 0.04f, 0.04f, 1.0f);
	glBegin(GL_QUADS);
		glVertex2f(panX,panY); glVertex2f(panX,panY+panH);
		glVertex2f(panX+panW,panY+panH); glVertex2f(panX+panW,panY);
	glEnd();

	glLineWidth(3.0f); glColor4f(1.0f,0.85f,0.2f,1.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(panX,panY); glVertex2f(panX,panY+panH);
		glVertex2f(panX+panW,panY+panH); glVertex2f(panX+panW,panY);
	glEnd();

	glLineWidth(1.0f); glColor4f(0.7f,0.5f,0.1f,0.5f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(panX+4,panY+4); glVertex2f(panX+4,panY+panH-4);
		glVertex2f(panX+panW-4,panY+panH-4); glVertex2f(panX+panW-4,panY+4);
	glEnd();

	float reelW=82, reelH=82;
	float reelCY = panY + panH*0.50f;
	float reelXs[3] = {
		panX + panW*0.20f,
		panX + panW*0.50f,
		panX + panW*0.80f
	};

	for (int r=0; r<3; r++){
		float rx = reelXs[r]-reelW*0.5f, ry = reelCY-reelH*0.5f;
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.95f,0.92f,0.82f,1);
		glBegin(GL_QUADS);
			glVertex2f(rx,ry); glVertex2f(rx,ry+reelH);
			glVertex2f(rx+reelW,ry+reelH); glVertex2f(rx+reelW,ry);
		glEnd();

		if (slot_spinning) {
			float stripe = fmodf(slot_reel_offset[r], reelH);
			glColor4f(0.0f,0.0f,0.0f,0.06f);
			for (float sy = -reelH; sy < reelH*2; sy += 12.0f) {
				float s0 = ry + sy + stripe;
				float s1 = s0 + 6.0f;
				if (s1 > ry && s0 < ry+reelH) {
					if (s0 < ry) s0 = ry;
					if (s1 > ry+reelH) s1 = ry+reelH;
					glBegin(GL_QUADS);
						glVertex2f(rx,s0); glVertex2f(rx,s1);
						glVertex2f(rx+reelW,s1); glVertex2f(rx+reelW,s0);
					glEnd();
				}
			}
		}

		glColor4f(0.25f,0.08f,0.08f,1); glLineWidth(2.5f);
		glBegin(GL_LINE_LOOP);
			glVertex2f(rx,ry); glVertex2f(rx,ry+reelH);
			glVertex2f(rx+reelW,ry+reelH); glVertex2f(rx+reelW,ry);
		glEnd();

		int symIdx;
		if (slot_spinning) {
			int offset = (int)(slot_reel_offset[r] / 15.0f) % REEL_LEN;
			symIdx = slot_reel_strip[r][offset];
		} else {
			symIdx = slot_reel_strip[r][slot_reels[r]];
		}

		GLuint fishTex = get_slot_tex(symIdx);
		float imgPad = 6.0f;
		float imgW = reelW - imgPad*2, imgH = reelH - imgPad*2;
		float imgX = reelXs[r] - imgW*0.5f, imgY = reelCY - imgH*0.5f;

		glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1,1,1, slot_spinning ? 0.55f : 1.0f);
		glBindTexture(GL_TEXTURE_2D, fishTex);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1); glVertex2f(imgX,       imgY);
			glTexCoord2f(0,0); glVertex2f(imgX,       imgY+imgH);
			glTexCoord2f(1,0); glVertex2f(imgX+imgW,  imgY+imgH);
			glTexCoord2f(1,1); glVertex2f(imgX+imgW,  imgY);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		if (!slot_spinning && slot_result_shown) {
			int sym0 = slot_reel_strip[0][slot_reels[0]];
			int sym1 = slot_reel_strip[1][slot_reels[1]];
			int sym2 = slot_reel_strip[2][slot_reels[2]];
			bool isMatch = false;
			if (sym0 == sym1 && sym1 == sym2) {
				isMatch = true;
			} else {
				if (r == 0 && sym0 == sym1) isMatch = true;
				if (r == 1 && (sym0 == sym1 || sym1 == sym2)) isMatch = true;
				if (r == 2 && sym1 == sym2) isMatch = true;
			}
			if (isMatch) {
				float pulse = 0.4f + 0.6f * fabsf(sinf((float)clock() / 120.0f));
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(1.0f, 0.85f, 0.0f, pulse * 0.35f);
				glBegin(GL_QUADS);
					glVertex2f(rx,ry); glVertex2f(rx,ry+reelH);
					glVertex2f(rx+reelW,ry+reelH); glVertex2f(rx+reelW,ry);
				glEnd();
				glLineWidth(3.0f); glColor4f(1.0f, 0.9f, 0.2f, pulse);
				glBegin(GL_LINE_LOOP);
					glVertex2f(rx,ry); glVertex2f(rx,ry+reelH);
					glVertex2f(rx+reelW,ry+reelH); glVertex2f(rx+reelW,ry);
				glEnd();
				glDisable(GL_BLEND);
			}
		}
	}

	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	Rect rTitle; rTitle.center=1; rTitle.left=(int)(panX+panW*0.5f); rTitle.bot=(int)(panY+panH-22);
	ggprint16(&rTitle,0,0x00ffe94f,"~ FISH SLOT MACHINE ~");

	for (int r=0; r<3; r++){
		int symIdx;
		if (slot_spinning) {
			int offset = (int)(slot_reel_offset[r] / 15.0f) % REEL_LEN;
			symIdx = slot_reel_strip[r][offset];
		} else {
			symIdx = slot_reel_strip[r][slot_reels[r]];
		}
		unsigned int col;
		switch(symIdx){
			case 2: col=0x00ff4444; break;
			case 4: col=0x00ff88ff; break;
			case 3: col=0x0044ffaa; break;
			case 1: col=0x00ffcc44; break;
			default: col=0x00aaddff; break;
		}
		Rect rs; rs.center=1; rs.left=(int)reelXs[r]; rs.bot=(int)(reelCY - reelH*0.5f - 16);
		ggprint16(&rs,0,col,"%s",SLOT_SYM_LABELS[symIdx]);
	}

	if (slot_spinning){
		float prog = 1.0f - (slot_spin_timer / slot_spin_dur);
		glDisable(GL_TEXTURE_2D);
		float bx=panX+20, by=panY+14, bw=panW-40, bh=10;
		glColor4f(0.15f,0.05f,0.05f,1);
		glBegin(GL_QUADS);
			glVertex2f(bx,by); glVertex2f(bx,by+bh);
			glVertex2f(bx+bw,by+bh); glVertex2f(bx+bw,by);
		glEnd();
		glColor4f(1.0f,0.7f,0.1f,1);
		glBegin(GL_QUADS);
			glVertex2f(bx,by); glVertex2f(bx,by+bh);
			glVertex2f(bx+bw*prog,by+bh); glVertex2f(bx+bw*prog,by);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		Rect rs; rs.center=1; rs.left=(int)(panX+panW*0.5f); rs.bot=(int)(panY+28);
		ggprint16(&rs,0,0x00ffaa44,"Spinning...");
	} else if (slot_result_shown){
		Rect rr; rr.center=1; rr.left=(int)(panX+panW*0.5f);
		rr.bot=(int)(panY+28);

		int sym0 = slot_reel_strip[0][slot_reels[0]];
		int sym1 = slot_reel_strip[1][slot_reels[1]];
		int sym2 = slot_reel_strip[2][slot_reels[2]];
		bool threeMatch = (sym0 == sym1 && sym1 == sym2);

		if (threeMatch && slot_result >= 10000){
			float pulse = fabsf(sinf((float)clock()/80.0f));
			unsigned int jcol = (pulse > 0.5f) ? 0x00ffee00 : 0x00ff8800;
			ggprint16(&rr,0,jcol,"*** JACKPOT! +%d gold! ***", slot_result);
		} else if (slot_result > 5) {
			unsigned int wcol = (slot_result >= 500) ? 0x00ffdd44 : 0x0088ff88;
			ggprint16(&rr,0,wcol,"+%d gold!", slot_result);
		} else {
			ggprint16(&rr,0,0x00aaddff,"Min payout: +%d gold", slot_result);
		}
		rr.bot=(int)(panY+10);
		ggprint16(&rr,0,0x00888888,"[Click or ESC] to continue");
	}

	Rect rLeg; rLeg.center=1; rLeg.left=(int)(panX+panW*0.5f); rLeg.bot=(int)(panY+panH-40);
	ggprint16(&rLeg,0,0x00cccccc,"ExoTrout=5  Grie=25  Milk=100");
	rLeg.bot=(int)(panY+panH-56);
	ggprint16(&rLeg,0,0x00ff8888,"Reyn=500    DEATH=10000!");

	glDisable(GL_BLEND);
}

// ============================================================
// PACHINKO RENDER HELPERS
// ============================================================
static void pach_draw_circle_filled(float cx, float cy, float r, int segs)
{
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(cx, cy);
	for (int i=0;i<=segs;i++){
		float a=(float)i/segs*2*3.14159265f;
		glVertex2f(cx+cosf(a)*r, cy+sinf(a)*r);
	}
	glEnd();
}

static void pach_draw_circle_outline(float cx, float cy, float r, int segs)
{
	glBegin(GL_LINE_LOOP);
	for (int i=0;i<segs;i++){
		float a=(float)i/segs*2*3.14159265f;
		glVertex2f(cx+cosf(a)*r, cy+sinf(a)*r);
	}
	glEnd();
}

void render_pachinko()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_TEXTURE_2D);

	glColor3f(0.05f,0.08f,0.12f);
	glBegin(GL_QUADS);
		glVertex2f(BOARD_L,BOARD_B); glVertex2f(BOARD_L,BOARD_T);
		glVertex2f(BOARD_R,BOARD_T); glVertex2f(BOARD_R,BOARD_B);
	glEnd();
	glColor3f(0.25f,0.45f,0.70f); glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(BOARD_L,BOARD_B); glVertex2f(BOARD_L,BOARD_T);
		glVertex2f(BOARD_R,BOARD_T); glVertex2f(BOARD_R,BOARD_B);
	glEnd();

	if (!slot_active) {
		glColor3f(0.4f,0.6f,1.0f); glLineWidth(1.0f);
		glBegin(GL_LINES);
			glVertex2f(pach_aim_x, BOARD_T-2); glVertex2f(pach_aim_x, BOARD_T-30);
		glEnd();
		glColor3f(0.3f,0.6f,1.0f);
		pach_draw_circle_outline(pach_aim_x, BOARD_T-PACH_BALL_R-4, PACH_BALL_R, 16);
	}

	static const float BKT_COLORS[PACH_BUCKETS][3] = {
		{0.5f,0.08f,0.08f}, {0.55f,0.38f,0.06f}, {0.15f,0.55f,0.25f},
		{0.12f,0.35f,0.75f},
		{0.15f,0.55f,0.25f}, {0.55f,0.38f,0.06f}, {0.5f,0.08f,0.08f}
	};
	float bktTop = BOARD_B + 30.0f;
	for (int k=0;k<PACH_BUCKETS;k++){
		float x0=pach_bkt_x[k], x1=pach_bkt_x[k+1];
		if (k==3){
			float pulse = 0.5f + 0.5f*sinf((float)(clock())/50.0f);
			glColor3f(0.05f + pulse*0.2f, 0.15f + pulse*0.3f, 0.55f + pulse*0.25f);
		} else {
			glColor3fv(BKT_COLORS[k]);
		}
		glBegin(GL_QUADS);
			glVertex2f(x0,BOARD_B); glVertex2f(x0,bktTop);
			glVertex2f(x1,bktTop);  glVertex2f(x1,BOARD_B);
		glEnd();
		glColor3f(0.25f,0.45f,0.70f);
		glBegin(GL_LINES);
			glVertex2f(x0,BOARD_B); glVertex2f(x0,bktTop);
		glEnd();
	}

	glColor3f(0.75f,0.80f,0.90f);
	for (int p=0;p<pach_npeg;p++)
		pach_draw_circle_filled(pach_pegs[p].x, pach_pegs[p].y, PACH_PEG_R, 14);
	glColor3f(0.55f,0.60f,0.72f);
	for (int p=0;p<pach_npeg;p++)
		pach_draw_circle_outline(pach_pegs[p].x, pach_pegs[p].y, PACH_PEG_R, 14);

	for (int i=0;i<pach_n;i++){
		PachBall &b = pach_balls[i];
		if (!b.active) continue;
		glColor3fv(b.col);
		pach_draw_circle_filled(b.pos[0], b.pos[1], b.radius, 14);
		glColor3f(1.0f,0.9f,0.7f);
		pach_draw_circle_outline(b.pos[0], b.pos[1], b.radius, 14);
	}

	glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Rect r; r.center=0; r.left=10; r.bot=g.yres-20;
	ggprint16(&r,22,0x00ffe94f,"PACHINKO");
	ggprint16(&r,22,0x00aaddff,"Won this session: %d", pach_total_won);
	ggprint16(&r,22,0x00888888,"Balls launched: %d",   pach_balls_used);
	r.bot=90;
	ggprint16(&r,22,0x00cccccc,"[Click] Drop ball  (-%d gold)", PACH_COST);
	ggprint16(&r,22,0x00cccccc,"[ESC]   Back to fishing");

	{
		float gx = g.xres - 150.0f, gy = g.yres - 38.0f;
		float gw = 140.0f, gh = 28.0f;
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.0f,0.0f,0.0f,0.55f);
		glBegin(GL_QUADS);
			glVertex2f(gx-4,gy-4); glVertex2f(gx-4,gy+gh+4);
			glVertex2f(gx+gw+4,gy+gh+4); glVertex2f(gx+gw+4,gy-4);
		glEnd();
		glColor4f(0.85f,0.65f,0.05f,1.0f); glLineWidth(2.0f);
		glBegin(GL_LINE_LOOP);
			glVertex2f(gx-4,gy-4); glVertex2f(gx-4,gy+gh+4);
			glVertex2f(gx+gw+4,gy+gh+4); glVertex2f(gx+gw+4,gy-4);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		Rect rg; rg.center=1; rg.left=(int)(gx+gw*0.5f); rg.bot=(int)(gy+6);
		ggprint16(&rg,0,0x00ffe94f,"Gold: %d",playerGold);
	}

	for (int k=0;k<PACH_BUCKETS;k++){
		float lx = (pach_bkt_x[k] + pach_bkt_x[k+1]) * 0.5f;
		Rect rb; rb.center=1; rb.left=(int)lx; rb.bot=(int)(BOARD_B+8);
		unsigned int col = (k==3) ? 0x00aaddff : 0x00ffffff;
		ggprint16(&rb,0,col,"%s",PACH_LABELS[k]);
	}

	if (playerGold < PACH_COST && !slot_active){
		Rect rw; rw.center=1; rw.left=g.xres/2; rw.bot=(int)(BOARD_T+10);
		ggprint16(&rw,0,0x00ff4444,"Not enough gold! (need %d)",PACH_COST);
	}

	if (pach_show_result && pach_result_timer > 0.0f && !slot_active){
		Rect rf; rf.center=1; rf.left=g.xres/2; rf.bot=(int)(BOARD_T+30);
		unsigned int col = (pach_last_payout > 0) ? 0x0088ff44 : 0x00ff4f4f;
		if (pach_last_payout > 0) ggprint16(&rf,0,col,"+%d gold!",pach_last_payout);
		else                      ggprint16(&rf,0,col,"Miss!");
	}

	glDisable(GL_BLEND);

	render_slot_overlay();
}

// ============================================================
// RENDER (top-level dispatcher)
// ============================================================
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
    //ggprint16(&rFPS, 12, 0x00ffffff, "fps: %i", g.fps);
	}
	else if (gameState == PLAY) {
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1,1,1);
		glBindTexture(GL_TEXTURE_2D, g.fishingTex);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1); glVertex2i(0,      0);
			glTexCoord2f(0,0); glVertex2i(0,      g.yres);
			glTexCoord2f(1,0); glVertex2i(g.xres, g.yres);
			glTexCoord2f(1,1); glVertex2i(g.xres, 0);
		glEnd();
		render_boat();
		render_fish_slot(0);
		render_fish_slot(1);
		glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		Rect rCast; rCast.center=1; rCast.left=g.xres/2; rCast.bot=60;
		ggprint16(&rCast,0,0x00ffffff,"[ Click to cast your rod ]");
		Rect rPach; rPach.center=1; rPach.left=g.xres/2; rPach.bot=35;
		ggprint16(&rPach,0,0x00ffe94f,"[ P ] Pachinko   [ S ] Shop");
		glDisable(GL_BLEND);
	}
	else if (gameState == FISHING) {
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1,1,1);
		glBindTexture(GL_TEXTURE_2D, g.fishingTex);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1); glVertex2i(0,      0);
			glTexCoord2f(0,0); glVertex2i(0,      g.yres);
			glTexCoord2f(1,0); glVertex2i(g.xres, g.yres);
			glTexCoord2f(1,1); glVertex2i(g.xres, 0);
		glEnd();
		render_boat();
		render_fish_slot(0);
		render_fish_slot(1);
		if (fishingPhase==PHASE_MINIGAME || fishingPhase==PHASE_HOOKED)
			render_skill_check();
		render_bite_alert();
		render_catch_screen();
	}
	else if (gameState == SHOPPING) {
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1,1,1);
		glBindTexture(GL_TEXTURE_2D, g.shopTex);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1); glVertex2i(0,      0);
			glTexCoord2f(0,0); glVertex2i(0,      g.yres);
			glTexCoord2f(1,0); glVertex2i(g.xres, g.yres);
			glTexCoord2f(1,1); glVertex2i(g.xres, 0);
		glEnd();
		open_shop();
		render_shop_back_button();
	}
	else if (gameState == CHARACTER) {
    	render_character_select();
	}

	else if (gameState == PACHINKO) {
		render_pachinko();
	}
}
