#ifndef LIBPS_H
#define LIBPS_H

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u_char;
typedef uint16_t u_short;
//typedef uint32_t u_long;

#define MODE_NTSC	0

typedef unsigned char PACKET;

typedef struct {
 short x, y;
 short w, h;
} RECT;

typedef struct {
 unsigned p : 24;
 unsigned char num : 8;
} GsOT_TAG;

typedef struct {
 unsigned short length;
 GsOT_TAG *org;
 unsigned short offset;
 unsigned short point;
 GsOT_TAG *tag;
} GsOT;

typedef struct {
 int pmode;
 int px;
 int py;
 unsigned int pw;
 unsigned int ph;
 u_long *pixel;
 int cx;
 int cy;
 unsigned int cw;
 unsigned int ch;
 u_long *clut;
} GsIMAGE;

typedef struct {
 unsigned long attribute;
 short x, y;
 unsigned short w, h;
 unsigned short tpage;
 unsigned char u, v;
 short cx, cy;
 unsigned char r, g, b;
 short mx, my;
 short scalex, scaley;
 long rotate;
} GsSPRITE;

extern void SetVideoMode(int mode);
extern void GsInitGraph(int w, int h, int intl, int dither, int vram);
extern void GsDefDispBuff(int x0, int y0, int x1, int y1);
extern void GsGetTimInfo(unsigned long *addr, GsIMAGE *img);
extern int LoadImage(RECT *recp, u_long *p);
extern u_short GetTPage(int tp, int abr, int x, int y);
extern void GetPadBuf(volatile unsigned char **buf1, volatile unsigned char **buf2);
extern void FntLoad(int tx, int ty);
extern int FntOpen(int x, int y, int w, int h, int isbg, int n);
extern int GsGetActiveBuff();
extern void GsSetWorkBase(PACKET *base_addr);
extern void GsClearOt(unsigned short offset, unsigned short point, GsOT *otp);
extern void GsSortFastSprite(GsSPRITE *sp, GsOT *otp, unsigned short pri);
extern void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT *otp);
extern int FntPrint(int num, ...);
extern int DrawSync(int mode);
extern void GsDrawOt(GsOT *otp);
extern u_long *FntFlush(int id);
extern int VSync(int mode);
extern void GsSwapDispBuff();

#endif
