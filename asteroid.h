#include <libps.h>

#define Y_START   0
#define Y_OPTIONS 1
#define Y_STOP    2

#define Y_NUMIMAGES 7

#define Y_MODE_NEW     1
#define Y_MODE_CLASSIC 4

extern unsigned long *YImagePtrs[Y_NUMIMAGES];
extern GsIMAGE YImages[Y_NUMIMAGES];
extern u_short YTPage[Y_NUMIMAGES];

#include "pad.h"

#define SHIPIMAGE      0
#define ASTEROIDIMAGE  1
#define EXPLOSIONIMAGE 2

#define TOPBORDER 30
#define MAX_ASTEROIDS 20
#define MAX_SHOTS 2
#define MAX_LEVEL 3
#define NUM_PICS 23
#define SHIP_PICS 16
#define OTHER_PICS (NUM_PICS - SHIP_PICS)
#define NOTHING 255
#define STARTX 154
#define STARTY 86
#define CENTERX1 140
#define CENTERY1 72
#define CENTERX2 180
#define CENTERY2 112

#define moveturn 4
#define shotturn 1

#define LARGE1 16
#define LARGE2 17
#define MEDIUM 18
#define SMALL  19
#define EXPL1  21
#define EXPL2  22

struct sprite {
 GsSPRITE sp;
 signed short vx,vy; /* current (x,y) velocity (in 1/100th of a pixel) */
 signed short rx,ry; /* current (x,y) remainder of movement (in 1/100th of
                          of a pixel) */
 unsigned char picnum; /* picture number */
};

struct ship {
 struct sprite spinfo;
 unsigned short score;
 unsigned char lives;
/* RECT pics[SHIP_PICS];*/
};

struct blast {
 struct sprite spinfo;
 signed short range[2];
};

typedef unsigned char byte;

struct asteroid_pics {
 short x, y;
 unsigned short u, v;

 /* There are (y) pairs of bytes.  The first byte is the number of blank
   pixels.  The second is the number of filled pixels. */
 byte (*image)[2];
};

extern struct ship player;
extern struct sprite asteroids[MAX_ASTEROIDS];
extern struct asteroid_pics apics[NUM_PICS];
extern u_long colors_tbl[];

