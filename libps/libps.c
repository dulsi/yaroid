#include <libps.h>
#include <SDL.h>
#include <SDL_image.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define GPU_FASTSPRITE 1
#define GPU_CLEAR 2

typedef struct {
 int size;
 int x, y;
 int w, h;
 char *content;
} FontStream;

static unsigned char buf[20];
static SDL_Window *mainWindow;
static SDL_Renderer *mainRenderer;
static u_long timerTicksIncrement;
static u_long timerStop;
static PACKET *workBase;
static PACKET *workEnd;
static SDL_Texture *fntTex;
static FontStream fntStream[8];

struct dload_list {
 unsigned long *addr;
 unsigned long size;
 unsigned char *data;
 struct dload_list *next;
};
static struct dload_list *dload_head = 0;

struct image_list {
 int pmode;
 int px;
 int py;
 unsigned int pw;
 unsigned int ph;
 int cx;
 int cy;
 SDL_Texture **tex;
 struct image_list *next;
};
static struct image_list *image_head = 0;

struct tpage_list {
 struct image_list *img;
 int ox;
 int oy;
 int tpage;
 struct tpage_list *next;
};
static struct tpage_list *tpage_head = 0;

void SetVideoMode(int mode)
{
 if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0)
 {
  printf("Failed - SDL_Init\n");
  exit(0);
 }
 timerTicksIncrement = 1000 / 60;
 timerStop = SDL_GetTicks() + timerTicksIncrement;
}

void GsInitGraph(int w, int h, int intl, int dither, int vram)
{
 mainWindow = SDL_CreateWindow("Net Yaroze Simulator",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           w,
                           h,
                           /*(fullScreen ? SDL_WINDOW_FULLSCREEN : 0)*/0);
 if (mainWindow == NULL)
 {
  printf("Failed - SDL_CreateWindow\n");
  exit(0);
 }

 mainRenderer = SDL_CreateRenderer(mainWindow, -1, /*(softRenderer ? SDL_RENDERER_SOFTWARE : 0)*/0);
 if (mainRenderer == NULL)
 {
  printf("Failed - SDL_CreateRenderer\n");
  exit(0);
 }
}

void GsDefDispBuff(int x0, int y0, int x1, int y1)
{
}

void GsGetTimInfo(unsigned long *addr, GsIMAGE *img)
{
 static int first = 1;
 if (first)
 {
  FILE *f = fopen("auto", "rb");
  if (f)
  {
   char data[1000];
   long sz = fread(data, 1, 1000, f);
   int w = 0;
   while (w < sz)
   {
    if (strncmp(data+w, "local", 5) != 0)
     break;
    w += 5;
    while (isspace(data[w]))
     w++;
    if (strncmp(data+w, "dload", 5) != 0)
     break;
    w += 5;
    while (isspace(data[w]))
     w++;
    int e = w + 1;
    while (!isspace(data[e]))
     e++;
    char filename[100];
    strncpy(filename, data + w, e - w);
    filename[e - w] = 0;
    w = e;
    while (isspace(data[w]))
     w++;
    uintptr_t ptr = 0;
    while (isalnum(data[w]))
    {
     ptr <<= 4;
     if (isalpha(data[w]))
     {
      if (data[w] >= 'a')
       ptr += (data[w] - 'a' + 10);
      else
       ptr += (data[w] - 'A' + 10);
     }
     else
     {
      ptr += (data[w] - '0');
     }
     w++;
    }
    while (isspace(data[w]))
     w++;
    struct dload_list *item = malloc(sizeof(struct dload_list));
    item->next = dload_head;
    item->addr = (unsigned long *)ptr;
    struct stat dstat;
    if (0 == stat(filename, &dstat))
    {
     item->size = dstat.st_size;
     item->data = malloc(dstat.st_size);
     FILE *df = fopen(filename, "rb");
     if (df)
     {
      fread(item->data, 1, dstat.st_size, df);
      dload_head = item;
     }
     else
      free(item);
    }
    else
    {
     free(item);
    }
   }
   fclose(f);
  }
  first = 0;
 }
 struct dload_list *cur = dload_head;
 while (cur)
 {
  if ((cur->addr <= addr) && (cur->addr + cur->size > addr))
  {
   int offset = ((intptr_t)addr) - ((intptr_t)cur->addr);
   img->pmode = cur->data[offset] & 0x07;
   offset += 4;
   if (cur->data[offset] & 0x08)
   {
    u_long clut_sz = cur->data[offset] + (cur->data[offset + 1] << 8) + (cur->data[offset + 2] << 16) + (cur->data[offset + 3] << 24);
    offset += 4;
    img->cx = cur->data[offset] + (cur->data[offset + 1] << 8);
    offset += 2;
    img->cy = cur->data[offset] + (cur->data[offset + 1] << 8);
    offset += 2;
    img->cw = cur->data[offset] + (cur->data[offset + 1] << 8);
    offset += 2;
    img->ch = cur->data[offset] + (cur->data[offset + 1] << 8);
    offset += 2;
    img->clut = (u_long *)(cur->data + offset);
    offset += clut_sz - 12;
   }
   u_long pixel_sz = cur->data[offset] + (cur->data[offset + 1] << 8) + (cur->data[offset + 2] << 16) + (cur->data[offset + 3] << 24);
   offset += 4;
   img->px = cur->data[offset] + (cur->data[offset + 1] << 8);
   offset += 2;
   img->py = cur->data[offset] + (cur->data[offset + 1] << 8);
   offset += 2;
   img->pw = cur->data[offset] + (cur->data[offset + 1] << 8);
   offset += 2;
   img->ph = cur->data[offset] + (cur->data[offset + 1] << 8);
   offset += 2;
   img->pixel = (u_long *)(cur->data + offset);
   struct image_list *image_next = malloc(sizeof(struct image_list));
   image_next->pmode = img->pmode;
   image_next->px = img->px;
   image_next->py = img->py;
   image_next->pw = img->pw * 2;
   image_next->ph = img->ph;
   image_next->cx = img->cx;
   image_next->cy = img->cy;
   image_next->next = image_head;
   image_next->tex = malloc(sizeof(SDL_Texture*) * img->ch);
   for (int cy = 0; cy < img->ch; cy++)
   {
    SDL_Surface *tmpSurface = SDL_CreateRGBSurface(0, img->pw * ((img->pmode == 0) ? 4 : 2), img->ph, 32,
                                         0x00FF0000,
                                         0x0000FF00,
                                         0x000000FF,
                                         0xFF000000);
    SDL_PixelFormat *fmt = tmpSurface->format;
    if (img->pmode == 0)
    {
     for (int y = 0; y < img->ph; y++)
     {
      for (int x = 0; x < img->pw * 2; x++)
      {
       Uint8 index2;
       Uint8 index;
       Uint32 temp;
       Uint8 red, green, blue, alpha;
       index = ((Uint8*)img->pixel)[x + y * (img->pw * 2)] & 0x0F;
       index2 = (((Uint8*)img->pixel)[x + y * (img->pw * 2)] & 0xF0) >> 4;
       temp = ((Uint8*)img->clut)[index * 2 + cy * img->cw * 2] + (((Uint32)((Uint8*)img->clut)[index * 2 + 1 + cy * img->cw * 2]) << 8);
       red = (temp & 0x001F) << 3;
       green = ((temp & 0x03F0) >> 5) << 3;
       blue = ((temp & 0x7C00) >> 10) << 3;
       alpha = 255;
       if (temp & 0x8000)
       {
       }
       else if ((red == 0) && (green == 0) && (blue == 0))
        alpha = 0;
       *((Uint32*)(tmpSurface->pixels + (y * tmpSurface->pitch) + x * 2 * fmt->BytesPerPixel)) = (((Uint32)red) << fmt->Rshift) + (((Uint32)green) << fmt->Gshift) + (((Uint32)blue) << fmt->Bshift) + (((Uint32)alpha) << fmt->Ashift);
       temp = ((Uint8*)img->clut)[index2 * 2 + cy * img->cw * 2] + (((Uint32)((Uint8*)img->clut)[index2 * 2 + 1 + cy * img->cw * 2]) << 8);
       red = (temp & 0x001F) << 3;
       green = ((temp & 0x03F0) >> 5) << 3;
       blue = ((temp & 0x7C00) >> 10) << 3;
       alpha = 255;
       if (temp & 0x8000)
       {
       }
       else if ((red == 0) && (green == 0) && (blue == 0))
        alpha = 0;
       *((Uint32*)(tmpSurface->pixels + (y * tmpSurface->pitch) + (x * 2 + 1) * fmt->BytesPerPixel)) = (((Uint32)red) << fmt->Rshift) + (((Uint32)green) << fmt->Gshift) + (((Uint32)blue) << fmt->Bshift) + (((Uint32)alpha) << fmt->Ashift);
      }
     }
    }
    else if (img->pmode == 1)
    {
     for (int y = 0; y < img->ph; y++)
     {
      for (int x = 0; x < img->pw * 2; x++)
      {
       Uint8 index;
       Uint32 temp;
       Uint8 red, green, blue, alpha;
       index = ((Uint8*)img->pixel)[x + y * img->pw *2];
       temp = ((Uint8*)img->clut)[index * 2 + cy * img->cw * 2] + (((Uint32)((Uint8*)img->clut)[index * 2 + 1 + cy * img->cw * 2]) << 8);
       red = (temp & 0x001F) << 3;
       green = ((temp & 0x03F0) >> 5) << 3;
       blue = ((temp & 0x7C00) >> 10) << 3;
       alpha = 255;
       if (temp & 0x8000)
       {
       }
       else if ((red == 0) && (green == 0) && (blue == 0))
        alpha = 0;
       *((Uint32*)(tmpSurface->pixels + (y * tmpSurface->pitch) + x * fmt->BytesPerPixel)) = (((Uint32)red) << fmt->Rshift) + (((Uint32)green) << fmt->Gshift) + (((Uint32)blue) << fmt->Bshift) + (((Uint32)alpha) << fmt->Ashift);
      }
     }
    }
    else
    {
     // Not implemented
    }
    image_next->tex[cy] = SDL_CreateTextureFromSurface(mainRenderer, tmpSurface);
    SDL_FreeSurface(tmpSurface);
   }
   image_head = image_next;
   return;
  }
  cur = cur->next;
 }
}

int LoadImage(RECT *recp, u_long *p)
{
}

u_short GetTPage(int tp, int abr, int x, int y)
{
 struct tpage_list *cur = tpage_head;
 while (cur)
 {
  if ((cur->img->px + cur->ox == x) && (cur->img->py + cur->oy == y))
   return cur->tpage;
  cur = cur->next;
 }
 struct image_list *imgcur = image_head;
 while (imgcur)
 {
  if ((imgcur->px <= x) && (imgcur->py <= y) && (imgcur->px + imgcur->pw > x) && (imgcur->py + imgcur->ph > y))
  {
   cur = malloc(sizeof(struct tpage_list));
   cur->img = imgcur;
   cur->ox = (x - imgcur->px) * 2;
   cur->oy = y - imgcur->py;
   if (tpage_head)
    cur->tpage = tpage_head->tpage + 1;
   else
    cur->tpage = 1;
   cur->next = tpage_head;
   tpage_head = cur;
   return cur->tpage;
  }
  imgcur = imgcur->next;
 }
 return 0;
}

void GetPadBuf(volatile unsigned char **buf1, volatile unsigned char **buf2)
{
 *buf1 = buf + 0;
 *buf2 = buf + 10;
 buf[0] = 0;
 buf[10] = 0;
 buf[1] = 0x55;
 buf[11] = 0x55;
 for (int i = 2; i < 10; i++)
 {
  buf[i] = 0xFF;
  buf[10 + i] = 0xFF;
 }
}

void FntLoad(int tx, int ty)
{
 SDL_Surface *fnt = IMG_Load("libps/font.png");
 fntTex = SDL_CreateTextureFromSurface(mainRenderer, fnt);
 SDL_FreeSurface(fnt);
 for (int i = 0; i < 8; i++)
 {
  fntStream[0].size = 0;
  fntStream[0].content = 0;
 }
}

int FntOpen(int x, int y, int w, int h, int isbg, int n)
{
 int i;
 for (i = 0; i < 7; i++)
 {
  if (fntStream[i].content == NULL)
   break;
 }
 fntStream[i].x = x;
 fntStream[i].y = y;
 fntStream[i].w = w;
 fntStream[i].h = h;
 fntStream[i].size = n;
 fntStream[i].content = malloc((n + 1) * sizeof(char));
 fntStream[i].content[0] = 0;
 return i;
}

int GsGetActiveBuff()
{
}

void GsSetWorkBase(PACKET *base_addr)
{
 workBase = base_addr;
 workEnd = base_addr;
}

void GsClearOt(unsigned short offset, unsigned short point, GsOT *otp)
{
}

void GsSortFastSprite(GsSPRITE *sp, GsOT *otp, unsigned short pri)
{
 workEnd[0] = GPU_FASTSPRITE;
 workEnd[1] = pri;
 workEnd[2] = sp->x & 0x00FF;
 workEnd[3] = sp->x >> 8;
 workEnd[4] = sp->y & 0x00FF;
 workEnd[5] = sp->y >> 8;
 workEnd[6] = sp->w & 0x00FF;
 workEnd[7] = sp->w >> 8;
 workEnd[8] = sp->h & 0x00FF;
 workEnd[9] = sp->h >> 8;
 workEnd[10] = sp->tpage & 0x00FF;
 workEnd[11] = sp->tpage >> 8;
 workEnd[12] = sp->u & 0x00FF;
 workEnd[13] = sp->u >> 8;
 workEnd[14] = sp->v & 0x00FF;
 workEnd[15] = sp->v >> 8;
 workEnd[16] = sp->cx & 0x00FF;
 workEnd[17] = sp->cx >> 8;
 workEnd[18] = sp->cy & 0x00FF;
 workEnd[19] = sp->cy >> 8;
 workEnd += 20;
}

void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT *otp)
{
 workEnd[0] = GPU_CLEAR;
 workEnd[1] = r;
 workEnd[2] = g;
 workEnd[3] = b;
 workEnd += 4;
}

int FntPrint(int num, ...)
{
 va_list valist;
 va_start(valist, num);
 char *s = va_arg(valist, char *);
 vsprintf(fntStream[num].content, s, valist);
 va_end(valist);
}

int DrawSync(int mode)
{
}

void GsDrawOt(GsOT *otp)
{
 int where = 0;
 int lastPri = 0;
 while (workBase + where  < workEnd)
 {
  if (workBase[where] == GPU_CLEAR)
  {
   SDL_SetRenderDrawColor(mainRenderer, workBase[where + 1], workBase[where + 2], workBase[where + 3], 255);
   SDL_RenderClear(mainRenderer);
   where +=4;
  }
  else if (workBase[where] == GPU_FASTSPRITE)
  {
   if (workBase[where + 1] > lastPri)
    lastPri = workBase[where + 1];
   where += 20;
  }
 }
 for (int pri = 0; pri <= lastPri; pri++)
 {
  where = 0;
  while (workBase + where  < workEnd)
  {
   if (workBase[where] == GPU_CLEAR)
   {
    where +=4;
   }
   else if (workBase[where] == GPU_FASTSPRITE)
   {
    if (workBase[where + 1] == pri)
    {
     int x = workBase[where + 2] + (((int)workBase[where + 3]) << 8);
     int y = workBase[where + 4] + (((int)workBase[where + 5]) << 8);
     int w = workBase[where + 6] + (((int)workBase[where + 7]) << 8);
     int h = workBase[where + 8] + (((int)workBase[where + 9]) << 8);
     int tpage = workBase[where + 10] + (((int)workBase[where + 11]) << 8);
     int u = workBase[where + 12] + (((int)workBase[where + 13]) << 8);
     int v = workBase[where + 14] + (((int)workBase[where + 15]) << 8);
     int cx = workBase[where + 16] + (((int)workBase[where + 17]) << 8);
     int cy = workBase[where + 18] + (((int)workBase[where + 19]) << 8);
     struct tpage_list *cur = tpage_head;
     while (cur)
     {
      if (cur->tpage == tpage)
      {
  					SDL_Rect destrect;
					  SDL_Rect srcrect;
       destrect.x = x;
       destrect.y = y;
       destrect.w = w;
       destrect.h = h;
       srcrect.x = cur->ox;
       srcrect.y = cur->oy;
       srcrect.w = w;
       srcrect.h = h;
       if (cur->img->pmode == 0)
       {
        srcrect.x *= 2;
        srcrect.y *= 2;
       }
       srcrect.x += u;
       srcrect.y += v;
  					SDL_RenderCopy(mainRenderer, cur->img->tex[cy - cur->img->cy], &srcrect, &destrect);
       cur = 0;
      }
      else
       cur = cur->next;
     }
    }
    where += 20;
   }
  }
 }
}

u_long *FntFlush(int id)
{
 if (fntStream[id].content[0] == 0)
  return 0;
 int h = 8;
 int w = 8;
 SDL_Rect destrect;
 SDL_Rect srcrect;
 destrect.w = w;
 destrect.h = h;
 destrect.y = fntStream[id].y;
 srcrect.w = 8;
 srcrect.h = 8;
 int x = fntStream[id].x;
 for (int i = 0; fntStream[id].content[i]; i++)
 {
  if ('\n' == fntStream[id].content[i])
  {
   x = fntStream[id].x;
   destrect.y += h;
  }
  else
  {
   destrect.x = x;
   x += w;
   if (('a' <= fntStream[id].content[i]) && (fntStream[id].content[i] <= 'z'))
   {
    srcrect.x = 8 * ((fntStream[id].content[i] - 'a' + 'A' - 1) % 16);
    srcrect.y = 8 * ((fntStream[id].content[i] - 'a' + 'A' - 1) / 16);
   }
   else
   {
    srcrect.x = 8 * ((fntStream[id].content[i] - 1) % 16);
    srcrect.y = 8 * ((fntStream[id].content[i] - 1) / 16);
   }
   SDL_RenderCopy(mainRenderer, fntTex, &srcrect, &destrect);
  }
 }
}

int VSync(int mode)
{
 u_long millisec;
 SDL_Event sdlevent;
 int avail;
 unsigned char mask;
 int index;

 millisec = SDL_GetTicks();
 if (millisec < timerStop)
 {
  SDL_Delay(timerStop - millisec);
 }
 timerStop = SDL_GetTicks() + timerTicksIncrement;
 for (avail = SDL_PollEvent(&sdlevent); avail;
   avail = SDL_PollEvent(&sdlevent))
 {
  switch (sdlevent.type)
  {
   case SDL_QUIT:
   {
    SDL_Quit();
    exit(0);
   }
   case SDL_KEYDOWN:
   case SDL_KEYUP:
    index = 0;
    switch (sdlevent.key.keysym.sym)
    {
     case SDLK_KP_8:
     case SDLK_UP:
      index = 2;
      mask=0x10;
      break;
     case SDLK_KP_6:
     case SDLK_RIGHT:
      index = 2;
      mask=0x20;
      break;
     case SDLK_KP_4:
     case SDLK_LEFT:
      index = 2;
      mask=0x80;
      break;
     case SDLK_KP_2:
     case SDLK_DOWN:
      index = 2;
      mask=0x40;
      break;
     case SDLK_m:
      index = 2;
      mask=0x08;
      break;
     case SDLK_n:
      index = 2;
      mask=0x01;
      break;
     case SDLK_s:
      index = 3;
      mask=0x80;
      break;
/*     case SDLK_RETURN:
      mask=0x0008;
      break;
     case SDLK_LSHIFT:
     case SDLK_RSHIFT:
      mask=0x0004;
      break;
     case SDLK_RCTRL:
     case SDLK_LCTRL:
      mask=0x0001;
      break;
     case SDLK_RALT:
     case SDLK_LALT:
      mask=0x0002;
      break;*/
     default:
      index=0;
      mask=0;
      break;
    }
    if (sdlevent.key.state == SDL_PRESSED) buf[index] &=(~mask);
    else buf[index] |= mask;
    break;
/*   case SDL_JOYAXISMOTION:
    if (0 == sdlevent.jaxis.which)
    {
      if (0 == sdlevent.jaxis.axis)
      {
       jsstatus_bits &= ~0x0C00;
       if (sdlevent.jaxis.value > 1000)
        jsstatus_bits |= 0x0400;
       else if (sdlevent.jaxis.value < -1000)
        jsstatus_bits |= 0x0800;
      }
      else if (1 == sdlevent.jaxis.axis)
      {
       jsstatus_bits &= ~0x0300;
       if (sdlevent.jaxis.value > 1000)
        jsstatus_bits |= 0x0100;
       else if (sdlevent.jaxis.value < -1000)
        jsstatus_bits |= 0x0200;
      }
    }
    break;
   case SDL_JOYBUTTONDOWN:
   case SDL_JOYBUTTONUP:
    if (0 == sdlevent.jbutton.which)
    {
     switch (sdlevent.jbutton.button)
     {
      case 0:
       mask = 0x0001;
       break;
      case 1:
       mask = 0x0002;
       break;
      case 2:
      case 8:
       mask = 0x0004;
       break;
      case 3:
      case 9:
       mask = 0x0008;
       break;
      default:
       mask = 0;
       break;
     }
     if (sdlevent.jbutton.state == SDL_PRESSED)
     {
      jsstatus_bits |= mask;
     }
     else
     {
      jsstatus_bits &= ~mask;
     }
    }
    break;*/
   default:
    break;
  }
 }
}

void GsSwapDispBuff()
{
 SDL_RenderPresent(mainRenderer);
}
