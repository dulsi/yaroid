#include "asteroid.h"

#define XSCREENSIZE 320
#define YSCREENSIZE 230

#define OT_LENGTH 1
#define MAXOBJ    ((MAX_ASTEROIDS + (MAX_SHOTS * 2) + 1) * 4)

#define IRANDOM(x) (rand()%x)

#define IMAGEDUMP 0x80090000

/* Global Variables */
GsOT WorldOT[2];
GsOT_TAG OTTags[2][1 << OT_LENGTH];
PACKET GpuPacketArea[2][MAXOBJ * (20 + 4)];
int activeBuff;
volatile u_char *control[2];
GsSPRITE Sprite;
int FontStreams[3];

long YHighScore = -1;
unsigned short YMode = 1;

struct ship player;
struct sprite asteroids[MAX_ASTEROIDS];
struct sprite explosion[MAX_SHOTS];
struct blast shots[MAX_SHOTS];
unsigned int shotcount;
unsigned int movecount;
unsigned int level;
unsigned int num_asteroids;
/*u_short other_pics[OTHER_PICS];*/

signed short acceleration[16][2]={{32,0},{29,-12},{22,-22},{12,-29},
  {0,-32},{-12,-29},{-22,-22},{-29,-12},{-32,0},{-29,12},
  {-22,22},{-12,29},{0,32},{12,29},{22,22},{29,12}};
signed short shot_start[16][2]={{12,5},{12,3},{11,0},{9,-1},{5,-1},
  {1,-1},{-1,-1},{-2,1},{-2,5},{-2,9},{-1,10},{1,11},{5,11},{9,11},
  {11,10},{12,9}};
signed short shot_range[16][2]={{160,0},{129,53},{79,79},{37,89},{0,92},
  {37,89},{79,79},{129,53},{160,0},{129,53},{79,79},{37,89},{0,92},
  {37,89},{79,79},{129,53}};
signed short init_asteroids[MAX_LEVEL]={6,7,8};

#ifdef IMAGEGRAB
RECT imageGrab;
#endif

void YInitialize();
int YTitleScreen();
u_long YReadControl(int num);

void startwait();
void init_game();
void create_level();
void play_game();
void move_sprite(struct sprite *cursprite);
void action_player();
void dead_player();
void add_asteroid(unsigned char picnum,signed short x,signed short y,
  signed short vx,signed short vy);
void destroy_asteroid(int number);
int check_center();
int check_collision(struct sprite *object1,struct sprite *object2);
void draw_screen();
void draw_epic(struct sprite *cursprite);
void draw_apic(struct sprite *cursprite);
void end_program();

int main()
{
 int select;

 YInitialize();
 while ((select = YTitleScreen()) != Y_STOP)
 {
  switch (select)
  {
   case Y_START:
    init_game();
    play_game();
    startwait();
    break;
   case Y_OPTIONS:
    if (YMode == Y_MODE_NEW)
     YMode = Y_MODE_CLASSIC;
    else
     YMode = Y_MODE_NEW;
    break;
   default:
    break;
  }
 }
 end_program();
 return 0;
}

void YInitialize()
{
 int i;
 RECT rect;

 /* Initialize Graphics */
 SetVideoMode(MODE_NTSC);
 GsInitGraph(320, 240, 4, 0, 0);
 GsDefDispBuff(0, 0, 0, 240);
 WorldOT[0].length = WorldOT[1].length = OT_LENGTH;
 WorldOT[0].org = OTTags[0];
 WorldOT[1].org = OTTags[1];

 for (i = 0; i < Y_NUMIMAGES; i++)
 {
  GsGetTimInfo(YImagePtrs[i], &YImages[i]);
  rect.x = YImages[i].px;
  rect.y = YImages[i].py;
  rect.w = YImages[i].pw;
  rect.h = YImages[i].ph;
  LoadImage(&rect, YImages[i].pixel);
  rect.x = YImages[i].cx;
  rect.y = YImages[i].cy;
  rect.w = YImages[i].cw;
  rect.h = YImages[i].ch;
  LoadImage(&rect, YImages[i].clut);
  YTPage[i] = GetTPage(YImages[i].pmode & 0x0003, 0, YImages[i].px, YImages[i].py);
 }

 /* Initialize Sound */

 /* Initialize Controller */
 GetPadBuf(&control[0], &control[1]);

 /* Initialize Sprite */
 Sprite.r = Sprite.g = Sprite.b = 0x80;
 Sprite.mx = 0;
 Sprite.my = 0;
 Sprite.scalex = 1;
 Sprite.scaley = 1;
 Sprite.rotate = 0;

 /* Initialize Font Streams */
 FntLoad(512, 0);
 FontStreams[0] = FntOpen(50, (TOPBORDER / 2), 300, TOPBORDER, 0, 40);
 FontStreams[1] = FntOpen(124, 96, 300, 140, 0, 30);
 FontStreams[2] = FntOpen(130, 130, 100, 80, 0, 40);
}

int YTitleScreen()
{
 u_long pad = 0;
 int select = Y_START;
 while (!YStartPress(pad))
 {
  pad = 0;
  activeBuff = GsGetActiveBuff();
  GsSetWorkBase((PACKET *)GpuPacketArea[activeBuff]);
  GsClearOt(0, 0, &WorldOT[activeBuff]);
  /* Draw Title */
  /* Assumes title in 8-bit pixel, larger than 256 width and on tpage
    boundary */
  Sprite.y = 15;
  Sprite.cx = YImages[0].cx;
  Sprite.cy = YImages[0].cy;
  Sprite.u=0;
  Sprite.v=0;
  Sprite.attribute = 0x01000000;
  Sprite.h = YImages[0].ph;
  Sprite.x = 10;
  Sprite.tpage = GetTPage(1, 0, YImages[0].px, YImages[0].py);
  Sprite.w = 256;
  GsSortFastSprite(&Sprite, &WorldOT[activeBuff], 0);
  Sprite.x = 266;
  Sprite.tpage = GetTPage(1, 0, YImages[0].px + 128, YImages[0].py);
  Sprite.w = (2 * YImages[0].pw) - 256;
  GsSortFastSprite(&Sprite, &WorldOT[activeBuff], 0);

  /* Draw 2 Asteroids */
  /* Assumes asteroid in 4-bit pixel */
  Sprite.y = 120;
  Sprite.cx = YImages[YMode + ASTEROIDIMAGE].cx;
  Sprite.cy = YImages[YMode + ASTEROIDIMAGE].cy;
  Sprite.u=0;
  Sprite.v=0;
  Sprite.attribute = 0x00000000;
  Sprite.h = YImages[2].ph;
  Sprite.x = 50;
  Sprite.tpage = YTPage[YMode + ASTEROIDIMAGE];
  Sprite.w = 32;
  GsSortFastSprite(&Sprite, &WorldOT[activeBuff], 0);
  Sprite.u = 32;
  Sprite.x = 260;
  Sprite.y = 160;
  GsSortFastSprite(&Sprite, &WorldOT[activeBuff], 0);

  /* Draw Ship */
  /* Assumes ship in 4-bit pixel */
  Sprite.cx = YImages[YMode + SHIPIMAGE].cx;
  Sprite.cy = YImages[YMode + SHIPIMAGE].cy;
  Sprite.u = 0;
  Sprite.x = 115;
  Sprite.y = 128 + select * 16;
  Sprite.tpage = YTPage[YMode + SHIPIMAGE];
  Sprite.h = YImages[YMode + SHIPIMAGE].ph;
  Sprite.w = 12;
  GsSortFastSprite(&Sprite, &WorldOT[activeBuff], 0);
  GsSortClear(0, 0, 0, &WorldOT[activeBuff]);

  /* Display Text */
  FntPrint(FontStreams[2], "Start Game\n\nOptions\n\nExit Game");
  if (YHighScore != -1)
   FntPrint(FontStreams[0], "Score: %5d     High: %5d", player.score, YHighScore);
  /* Documentation notes that GsDrawOt does not work right if something
   hasn't finished drawing.  So DrawSync called just in case. */
  DrawSync(0);
  GsDrawOt(&WorldOT[activeBuff]);
  FntFlush(FontStreams[2]);
  FntFlush(FontStreams[0]);
  DrawSync(0);
  VSync(0);
  GsSwapDispBuff();
  while ((!YStartPress(pad)) && (!YSelectPress(pad)))
  {
   VSync(0);
   pad = YReadControl(0);
#ifdef IMAGEGRAB
   if (YL1Press(pad))
   {
    imageGrab.x = 0;
    imageGrab.y = activeBuff ? 240 : 0;
    imageGrab.w = 320;
    imageGrab.h = 240;
    StoreImage(&imageGrab, (u_long *)IMAGEDUMP);
    DrawSync(0);
   }
#endif
  }
  if (YSelectPress(pad))
  {
   select++;
   select = select % 3;
  }
 }
 return select;
}

u_long YReadControl(int num)
{
 static u_long oldresult[2] = {0, 0};
 u_long ans;

 ans = (oldresult[num] << 16) +
   ((~(*(control[num] + 3) | *(control[num] + 2) << 8)) & 0x0000FFFF);
 oldresult[num] = ans;
 return ans;
}

void startwait()
{
 u_long padd = 0;

 while (!(YStartPress(padd)))
 {
  VSync(0);
  padd = YReadControl(0);
 }
}

void init_game()
{
 level=0;
 player.score=0;
 player.lives=4;
 player.spinfo.vx=player.spinfo.vy=player.spinfo.rx=player.spinfo.ry=0;
 player.spinfo.picnum=4;
 player.spinfo.sp.x=STARTX;
 player.spinfo.sp.y=STARTY;
 player.spinfo.sp.w=12;
 player.spinfo.sp.h=12;
 player.spinfo.sp.tpage=YTPage[YMode + SHIPIMAGE];
 player.spinfo.sp.u=apics[4].u;
 player.spinfo.sp.v=apics[4].v;
 player.spinfo.sp.cx=YImages[YMode + SHIPIMAGE].cx;
 player.spinfo.sp.cy=YImages[YMode + SHIPIMAGE].cy;
 player.spinfo.sp.r=player.spinfo.sp.g=player.spinfo.sp.b=0x80;
 player.spinfo.sp.mx=0;
 player.spinfo.sp.my=0;
 player.spinfo.sp.scalex=1;
 player.spinfo.sp.scaley=1;
 player.spinfo.sp.rotate=0;
 create_level();
}

void create_level()
{
 int i;

 for (i=0;i<MAX_SHOTS;i++)
 {
  shots[i].spinfo.picnum=NOTHING;
  explosion[i].picnum=NOTHING;
 }
 num_asteroids=0;
 for (i=0;i<init_asteroids[level];i++)
  add_asteroid(LARGE1+IRANDOM(2),(200+IRANDOM(208))%XSCREENSIZE,IRANDOM(185),
    (16+IRANDOM(32))*((IRANDOM(2))?1:-1),128*((IRANDOM(2))?1:-1));
 draw_screen();
 draw_screen();
}

void play_game()
{
 int i,k;

 shotcount=0;
 movecount=0;
 while (player.lives > 0)
 {
  if (movecount++==moveturn)
  {
   action_player();
   move_sprite(&player.spinfo);
   for (i=0;i<num_asteroids;i++)
   {
    move_sprite(asteroids+i);
    if ((player.spinfo.picnum != NOTHING) &&
      (check_collision(&player.spinfo,asteroids+i)))
    {
     destroy_asteroid(i);
     dead_player();
     i=num_asteroids;
    }
   }
   movecount=0;
  }
  if (shotcount++==shotturn)
  {
   for (i=0;i<MAX_SHOTS;i++)
    if (shots[i].spinfo.picnum!=NOTHING)
    {
     shots[i].range[0]-=abs((shots[i].spinfo.rx+shots[i].spinfo.vx)>>7);
     shots[i].range[1]-=abs((shots[i].spinfo.ry+shots[i].spinfo.vy)>>7);
     move_sprite(&shots[i].spinfo);
     for (k=0;k<num_asteroids;k++)
      if (check_collision(&shots[i].spinfo,asteroids+k))
      {
       shots[i].spinfo.picnum=NOTHING;
       destroy_asteroid(k);
       k=num_asteroids;
      }
     if (shots[i].range[0]<1 && shots[i].range[1]<1)
      shots[i].spinfo.picnum=NOTHING;
    }
   shotcount=0;
  }
  if (movecount==0 || shotcount==0) draw_screen();
  if (num_asteroids==0)
  {
   if (level<MAX_LEVEL-1) level++;
   create_level();
  }
 }
 if (player.score > YHighScore)
  YHighScore = player.score;
}

void move_sprite(struct sprite *cursprite)
{
 short k,i;

 cursprite->rx+=cursprite->vx;
 cursprite->ry+=cursprite->vy;
 cursprite->sp.x+=(i=cursprite->rx>>7)+XSCREENSIZE;
 cursprite->sp.y+=(k=cursprite->ry>>7)+YSCREENSIZE-TOPBORDER;
 cursprite->sp.x%=XSCREENSIZE;
 cursprite->sp.y%=YSCREENSIZE - TOPBORDER;
 cursprite->rx-=i<<7;
 cursprite->ry-=k<<7;
}

void action_player()
{
 int i;
 signed char r;

 if (player.spinfo.picnum == NOTHING)
 {
  if (!check_center())
  {
   player.spinfo.sp.x=STARTX;
   player.spinfo.sp.y=STARTY;
   player.spinfo.vx=player.spinfo.vy=player.spinfo.rx=player.spinfo.ry=0;
   player.spinfo.picnum=4;
   player.spinfo.sp.u = apics[4].u;
  }
 }
 else
 {
  u_long padd = YReadControl(0);
  r = 0;
  if (YLeftHeld(padd)) r = -1;
  if (YRightHeld(padd)) r = 1;
  if (r)
  {
   player.spinfo.picnum+=16-r;
   player.spinfo.picnum%=16;
   player.spinfo.sp.u=player.spinfo.picnum*12;
  }
  player.spinfo.vx-=(player.spinfo.vx>>4)+((player.spinfo.vx-(player.spinfo.vx>>4<<4))?1:0)*((player.spinfo.vx<0)?-1:1);
  player.spinfo.vy-=(player.spinfo.vy>>4)+((player.spinfo.vy-(player.spinfo.vy>>4<<4))?1:0)*((player.spinfo.vy<0)?-1:1);
  if (YUpHeld(padd))
  {
   player.spinfo.vx+=acceleration[player.spinfo.picnum][0];
   player.spinfo.vy+=acceleration[player.spinfo.picnum][1];
  }
  else if (YDownHeld(padd))
  {
   player.spinfo.vx=player.spinfo.vy=player.spinfo.rx=player.spinfo.ry=0;
   player.spinfo.sp.x=IRANDOM(320);
   player.spinfo.sp.y=IRANDOM(185);
  }
  if (YSquarePress(padd))
  {
   for (i=0;(i<MAX_SHOTS) && (shots[i].spinfo.picnum!=NOTHING);i++) ;
   if (i<MAX_SHOTS)
   {
    shots[i].spinfo.picnum=20;
    shots[i].spinfo.sp.tpage=YTPage[YMode + EXPLOSIONIMAGE];
    shots[i].spinfo.sp.cx=YImages[3].cx;
    shots[i].spinfo.sp.cy=YImages[3].cy;
    shots[i].spinfo.sp.x=player.spinfo.sp.x+shot_start[player.spinfo.picnum][0];
    shots[i].spinfo.sp.y=player.spinfo.sp.y+shot_start[player.spinfo.picnum][1];
    shots[i].spinfo.sp.w=apics[20].x;
    shots[i].spinfo.sp.h=apics[20].y;
    shots[i].spinfo.sp.u=apics[20].u;
    shots[i].spinfo.sp.v=apics[20].v;
    shots[i].spinfo.sp.r=shots[i].spinfo.sp.g=shots[i].spinfo.sp.b=0x80;
    shots[i].spinfo.sp.mx=0;
    shots[i].spinfo.sp.my=0;
    shots[i].spinfo.sp.scalex=1;
    shots[i].spinfo.sp.scaley=1;
    shots[i].spinfo.sp.rotate=0;
    shots[i].spinfo.vx=acceleration[player.spinfo.picnum][0]*16;
    shots[i].spinfo.vy=acceleration[player.spinfo.picnum][1]*16;
    shots[i].spinfo.rx=shots[i].spinfo.ry=0;
    shots[i].range[0]=shot_range[player.spinfo.picnum][0];
    shots[i].range[1]=shot_range[player.spinfo.picnum][1];
   }
  }
#ifdef IMAGEGRAB
  if (YL1Press(padd))
  {
   imageGrab.x = 0;
   imageGrab.y = activeBuff ? 240 : 0;
   imageGrab.w = 320;
   imageGrab.h = 240;
   StoreImage(&imageGrab, (u_long *)IMAGEDUMP);
   DrawSync(0);
  }
#endif
 }
}

void dead_player()
{
 player.spinfo.picnum = NOTHING;
 --player.lives;
}

void add_asteroid(unsigned char picnum,signed short x,signed short y,
  signed short vx,signed short vy)
{
 if (num_asteroids<MAX_ASTEROIDS)
 {
  asteroids[num_asteroids].picnum=picnum;
  asteroids[num_asteroids].sp.x=x;
  asteroids[num_asteroids].sp.y=y;
  asteroids[num_asteroids].vx=vx;
  asteroids[num_asteroids].vy=vy;
  asteroids[num_asteroids].rx=asteroids[num_asteroids].ry=0;
  asteroids[num_asteroids].sp.w=apics[picnum].x;
  asteroids[num_asteroids].sp.h=apics[picnum].y;
  asteroids[num_asteroids].sp.tpage=YTPage[YMode + ASTEROIDIMAGE];
  asteroids[num_asteroids].sp.u=apics[picnum].u;
  asteroids[num_asteroids].sp.v=apics[picnum].v;
  asteroids[num_asteroids].sp.cx=YImages[YMode + ASTEROIDIMAGE].cx;
  asteroids[num_asteroids].sp.cy=YImages[YMode + ASTEROIDIMAGE].cy + IRANDOM(YImages[YMode + ASTEROIDIMAGE].ch);
  asteroids[num_asteroids].sp.r=asteroids[num_asteroids].sp.g=asteroids[num_asteroids].sp.b=0x80;
  asteroids[num_asteroids].sp.mx=0;
  asteroids[num_asteroids].sp.my=0;
  asteroids[num_asteroids].sp.scalex=1;
  asteroids[num_asteroids].sp.scaley=1;
  asteroids[num_asteroids].sp.rotate=0;
  num_asteroids++;
 }
}

void destroy_asteroid(int number)
{
 int i;

 switch (asteroids[number].picnum)
 {
  case LARGE1:
  case LARGE2:
   add_asteroid(MEDIUM,asteroids[number].sp.x+16,asteroids[number].sp.y+14,
     (10+IRANDOM(30))*((IRANDOM(2))?1:-1),100*((IRANDOM(2))?1:-1));
   player.score-=30;
  case MEDIUM:
   for (i=0;(i<MAX_SHOTS) && (explosion[i].picnum!=NOTHING);i++) ;
   if (i<MAX_SHOTS)
   {
    explosion[i].picnum=((asteroids[number].picnum==MEDIUM)?EXPL2:EXPL1);
    explosion[i].sp.tpage=YTPage[YMode + EXPLOSIONIMAGE];
    explosion[i].sp.x=asteroids[number].sp.x;
    explosion[i].sp.y=asteroids[number].sp.y;
    explosion[i].sp.cx=YImages[3].cx;
    explosion[i].sp.cy=YImages[3].cy;
    explosion[i].sp.h=apics[explosion[i].picnum].y;
    explosion[i].sp.w=apics[explosion[i].picnum].x;
    explosion[i].sp.u=apics[explosion[i].picnum].u;
    explosion[i].sp.v=apics[explosion[i].picnum].v;
    explosion[i].sp.r=explosion[i].sp.g=explosion[i].sp.b=0x80;
    explosion[i].sp.mx=0;
    explosion[i].sp.my=0;
    explosion[i].sp.scalex=1;
    explosion[i].sp.scaley=1;
    explosion[i].sp.rotate=0;
   }
   player.score+=50;
   asteroids[number].picnum=((asteroids[number].picnum==MEDIUM)?SMALL:MEDIUM);
   asteroids[number].sp.tpage=YTPage[YMode + ASTEROIDIMAGE];
   asteroids[number].sp.v=apics[asteroids[number].picnum].v;
   asteroids[number].sp.u=apics[asteroids[number].picnum].u;
   asteroids[number].sp.w=apics[asteroids[number].picnum].x;
   asteroids[number].sp.h=apics[asteroids[number].picnum].y;
   asteroids[number].sp.cx=YImages[YMode + ASTEROIDIMAGE].cx;
   asteroids[number].sp.cy=YImages[YMode + ASTEROIDIMAGE].cy+IRANDOM(YImages[YMode + ASTEROIDIMAGE].ch);
/*   asteroids[number].sp.cy=481+IRANDOM(8);*/
   asteroids[number].vx=(16+IRANDOM(32))*((IRANDOM(2))?1:-1);
   asteroids[number].vy=128*((IRANDOM(2))?1:-1);
   asteroids[number].rx=asteroids[number].ry=0;
   break;
  case SMALL:
   num_asteroids--;
   if (number<num_asteroids)
   {
    memcpy(asteroids+number,asteroids+num_asteroids,sizeof(struct sprite));
   }
   player.score+=100;
   break;
  default:
   break;
 }
}

int check_collision(struct sprite *object1,struct sprite *object2)
{
 int x1,y1,x2,y2;

 for (y1=0,y2=object1->sp.y-object2->sp.y;y1<apics[object1->picnum].y;
   y1++,y2++)
 {
  if (y2<-160) y2+=(YSCREENSIZE-TOPBORDER);
  else if (y2>160) y2-=(YSCREENSIZE-TOPBORDER);
  if ((y2>=0) && (y2<apics[object2->picnum].y) &&
    (apics[object2->picnum].image[y2][1]!=0) &&
    (apics[object1->picnum].image[y1][1]!=0))
  {
   x1 = object1->sp.x + apics[object1->picnum].image[y1][0];
   x2 = object2->sp.x + apics[object2->picnum].image[y2][0];
   if (x1 - x2 > 290) x1 -= 290;
   else if (x2 - x1 > 290) x2 -= 290;
   if (x1 > x2)
   {
    if (x2 + apics[object2->picnum].image[y2][1] >= x1) return 1;
   }
   else
   {
    if (x1 + apics[object1->picnum].image[y1][1] >= x2) return 1;
   }
  }
 }
 return 0;
}

int check_center()
{
 int x[2],y[2],i;

 for (i=0;i<num_asteroids;i++)
 {
  x[0]=asteroids[i].sp.x;
  x[1]=x[0]+apics[asteroids[i].picnum].x;
  y[0]=asteroids[i].sp.y;
  y[1]=y[0]+apics[asteroids[i].picnum].y;
  if ((((x[0]>CENTERX1) && (x[0]<CENTERX2)) ||
    ((x[1]>CENTERX1) && (x[1]<CENTERX2))) &&
    (((y[0]>CENTERY1) && (y[0]<CENTERY2)) ||
    ((y[1]>CENTERY1) && (y[1]<CENTERY2)))) return 1;
 }
 return 0;
}

void draw_screen()
{
 char s[6];
 int i;

 activeBuff = GsGetActiveBuff();
 GsSetWorkBase((PACKET *)GpuPacketArea[activeBuff]);
 GsClearOt(0, 0, &WorldOT[activeBuff]);
 for (i=0;i<num_asteroids;i++)
  draw_apic(asteroids+i);
 for (i=0;i<MAX_SHOTS;i++)
 {
  if (shots[i].spinfo.picnum!=NOTHING) draw_apic(&shots[i].spinfo);
  if (explosion[i].picnum!=NOTHING) draw_epic(explosion+i);
  explosion[i].picnum=NOTHING;
 }
 if (player.spinfo.picnum!=NOTHING) draw_apic(&player.spinfo);
 FntPrint(FontStreams[0], "Score: %5d    Lives: %5d", player.score, player.lives);
 FntFlush(FontStreams[0]);
 if (player.lives == 0)
 {
  FntPrint(FontStreams[1], "GAME OVER");
  FntFlush(FontStreams[1]);
 }
 DrawSync(0);
 VSync(0);
 GsSwapDispBuff();
 GsSortClear(0, 0, 0, &WorldOT[activeBuff]);
 GsDrawOt(&WorldOT[activeBuff]);
}

void draw_epic(struct sprite *cursprite)
{
 cursprite->sp.y+=TOPBORDER;
 GsSortFastSprite(&cursprite->sp, &WorldOT[activeBuff], 0);
 cursprite->sp.y-=TOPBORDER;
}

void draw_apic(struct sprite *cursprite)
{
 short x,y;
 unsigned short w,h;
 unsigned char u,v;
 
 y = cursprite->sp.y;
 x = cursprite->sp.x;
 w = cursprite->sp.w;
 h = cursprite->sp.h;
 u = cursprite->sp.u;
 v = cursprite->sp.v;
 cursprite->sp.y+=TOPBORDER;
 GsSortFastSprite(&cursprite->sp, &WorldOT[activeBuff], 0);
 if (cursprite->sp.x + cursprite->sp.w > XSCREENSIZE)
 {
  cursprite->sp.x = 0;
  cursprite->sp.w = x + w - XSCREENSIZE;
  cursprite->sp.u = u + w - cursprite->sp.w;
  GsSortFastSprite(&cursprite->sp, &WorldOT[activeBuff], 0);
  if (cursprite->sp.y + cursprite->sp.h > YSCREENSIZE)
  {
   cursprite->sp.y = TOPBORDER;
   cursprite->sp.h = y + h + TOPBORDER - YSCREENSIZE;
   cursprite->sp.v = v + h - cursprite->sp.h;
   GsSortFastSprite(&cursprite->sp, &WorldOT[activeBuff], 0);
  }
  cursprite->sp.y=y+TOPBORDER;
  cursprite->sp.x=x;
  cursprite->sp.w=w;
  cursprite->sp.h=h;
  cursprite->sp.u=u;
  cursprite->sp.v=v;
 }
 if (cursprite->sp.y + cursprite->sp.h > YSCREENSIZE)
 {
  cursprite->sp.y = TOPBORDER;
  cursprite->sp.h = y + h + TOPBORDER - YSCREENSIZE;
  cursprite->sp.v = v + h - cursprite->sp.h;
  GsSortFastSprite(&cursprite->sp, &WorldOT[activeBuff], 0);
 }
 cursprite->sp.y=y;
 cursprite->sp.x=x;
 cursprite->sp.w=w;
 cursprite->sp.h=h;
 cursprite->sp.u=u;
 cursprite->sp.v=v;
}

void end_program()
{
 VSync(0);
}

