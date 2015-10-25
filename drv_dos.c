/* drvr_dos.c - DOS driver file (for use with gcc) */

/*
 * Modified: 17 Mar 1993
 *           Eduard [esp] Schwan
 *           Passed bg_color to display_init
*/
#include <stdlib.h> /* Needed for malloc */
#include <stdio.h>  /* Needed for stderr */

/* Video resolutions and mode starting numbers */
#define VIDEO_RESOLUTIONS 5
#define FIRST_8BIT_MODE 1
#define FIRST_HICOLOR_MODE (FIRST_8BIT_MODE + VIDEO_RESOLUTIONS)
#define FIRST_16BIT_MODE (FIRST_HICOLOR_MODE + VIDEO_RESOLUTIONS)
#define FIRST_TRUECOLOR_MODE  (FIRST_16BIT_MODE + VIDEO_RESOLUTIONS)
#define FIRST_4BIT_MODE (FIRST_TRUECOLOR_MODE + VIDEO_RESOLUTIONS)

#include <io.h>
#if !defined( MAC )
#include <dos.h>
#if defined( __GNUC__ )
#include <pc.h>
#define MK_FP(seg, ofs) ((void *)(0xE0000000 + ((seg)<<4) + ofs))
#define FP_OFF(ptr) (unsigned short)(ptr)
#define FP_SEG(ptr) (unsigned short)(((unsigned long)ptr >> 16) & 0x0FFF)
#define segread(x) (void)(x)
#define getch() getkey()
#define __far
#define outp( portid,v )  outportb( portid,v )
#define inp( portid ) inportb( portid )
#elif defined( DOS386 )
#include <conio.h>
#else
#define __far
#endif
#endif
#include "def.h"
#include "drv.h"

#define TEST_ABORT if (kbhit() && getch() == 27) { display_close(0); exit(1); }

typedef struct {
   unsigned char bytes;
   unsigned char byte[3];
   } quantized_color;

static quantized_color black1 = {1, {0, 0, 0}};
static quantized_color black2 = {2, {0, 0, 0}};
static quantized_color black3 = {3, {0, 0, 0}};

int Palette_Start = 0;      /* Palette entries start at 0 */
int Palette_Flag = 1;       /* Use 884 palette */
int Display_Flag = 3;        /* Use 800x600 256 color VESA mode */
int Reset_Display_Flag = 1;  /* Turn on the requested video mode */

/* Define the top right, the width, and the length of usuable display area */
int Display_x0 = -1, Display_y0 = -1;
int Display_xl = -1, Display_yl = -1;

/* The amount of squashing required to get the image resolution to fit into
   the available screen space */
static float X_Display_Scale = 1.0;
static float Y_Display_Scale = 1.0;

/* Format of VESA global information */
struct VgaInfoBlock   {
  unsigned char      VESASignature[4];
  unsigned char      VESAVersion;
  unsigned char      VESARevision;
  unsigned short     OEMStringPtrOff;
  unsigned short     OEMStringPtrSeg;
  unsigned char      Capabilities[4];
  unsigned short     VideoModePtrOff;
  unsigned short     VideoModePtrSeg;
  };

/* Format of the information for individual VESA display modes */
struct ModeInfoBlock   {
  unsigned short ModeAttributes;
  unsigned char  WinAAttributes;
  unsigned char  WinBAttributes;
  unsigned short WinGranularity;
  unsigned short WinSize;
  unsigned short WinASegment;
  unsigned short WinBSegment;
  unsigned short WinFunctPtrOff;
  unsigned short WinFunctPtrSeg;
  unsigned short BytesPerScanLine;

  /* the remainder of this structure is optional */

  unsigned short XResolution;
  unsigned short YResolution;
  unsigned char XCharSize;
  unsigned char YCharSize;
  unsigned char NumberOfPlanes;
  unsigned char BitsPerPixel;
  unsigned char NumberOfBanks;
  unsigned char MemoryModel;
  unsigned char BankSize;
};

static unsigned offx = 0;
static unsigned offy = 0;
static unsigned maxx = 800;
static unsigned maxy = 600;
static unsigned line_length = 800;
static unsigned screen_maxx = 800;
static unsigned screen_maxy = 600;
static unsigned long granularity = 65536;

typedef unsigned char palette_array[256][3];
static palette_array *palette = NULL;

static int EGA_colors[16][3] =
   {{  0,  0,  0}, {  0,  0,255}, {  0,255,  0}, {  0,127,127},
    {255,  0,  0}, {127,  0,127}, {127,127,  0}, { 85, 85, 85},
    {170,170,170}, {127,127,255}, {127,255,127}, {  0,255,255},
    {255,127,127}, {255,  0,255}, {255,255,  0}, {255,255,255}};
static int EGA_remap[16] =
   {0,  6,  5,  4,  3,  2, 1,  7,
    8, 14, 13, 12, 11, 10, 9, 15};

/* Global variables to keep track of VESA calls */
static struct ModeInfoBlock VESAModeInfo;

#if defined( DOS386 )
static short real_buf[2];
/* Pointer to real memory for looking at VESA information */
static unsigned char __far *real_ptr;

/* Ask the dos extender where to communicate with real mode BIOS calls */
static void
find_real_buf()
{
   struct SREGS sregs;
   union REGS regs;

   regs.x.ax = 0x250d;
   segread(&sregs);
   int86x(0x21, &regs, &regs, &sregs);

   *((unsigned *)&real_buf) = regs.e.ebx;
   real_ptr = MK_FP(sregs.es, regs.e.edx);
}

#elif defined( __GNUC__ )
static int real_buf[2];
static unsigned char __far *real_ptr;

void
find_real_buf()
{
   static unsigned char *memptr = (unsigned char *)0xE0000000;
   static unsigned char buf[0x1000];

   union REGS reg;
   unsigned char *p, *filename;
   int i, occur = 0;

   strcpy(buf, "tmpXXXXX");
   filename = mktemp(buf);

   for (p = &memptr[0xA0000-1-strlen((char *)buf)]; p != memptr; p--)
      if (buf[0] == *p && strcmp((char *)buf, (char *)p) == 0) {
	 real_ptr = p;
	 occur++;
	 }

   if (!occur) {
      fprintf(stderr, "Video init failure");
      exit(1);
      }

   reg.h.ah = 0x1A;
   reg.x.dx = (int)buf;
   int86(0x21, &reg, &reg);

   reg.h.ah = 0x4E;
   reg.x.dx = (int)"*.*";
   int86(0x21, &reg, &reg);
   real_buf[0] = (reg.x.dx & 0xFFFF) - 43;

   getcwd(buf, sizeof(buf));
   if ((i = strlen(buf)) < 2) i = 2;
   real_ptr = real_ptr - 3 - i;
}
#endif

static void
bios_putpixel(int x, int y, int color)
{
   union REGS reg;

   reg.h.ah = 0x0c;
   reg.h.al = color;
   reg.h.bh = 0;
   reg.x.cx = x;
   reg.x.dx = y;
   int86(0x10, &reg, &reg);
}

static void
setvideomode(int mode)
{
   union REGS reg;
   reg.h.ah = 0;
   reg.h.al = mode;
   int86(0x10, &reg, &reg);
}

static int
setvesabank(int bank)
{
   static int current_bank = -1;
   union REGS reg;

   if (bank != current_bank) {
      current_bank = bank;
      reg.x.ax = 0x4F05;
      reg.x.bx = 0;
      reg.x.dx = bank;
      int86(0x10, &reg, &reg);
      return (reg.x.ax == 0x004f ? 1 : 0);
      }
   return 1;
}

static int
setvesamode(int mode, int clear)
{
   union REGS regs;
   struct SREGS sregs;
   struct ModeInfoBlock *VgaPtr = &VESAModeInfo;
   unsigned i;

   /* Call VESA function 1 to get mode info */
   regs.h.ah = 0x4f;
   regs.h.al = 0x01;
   regs.x.cx = mode;
#if defined( DOS386 )
   regs.x.di = real_buf[0];
   segread(&sregs);
   sregs.es  = real_buf[1];
   int86x_real(0x10, &regs, &regs, &sregs);
   if (regs.h.al != 0x4f || regs.h.ah != 0)
      return 0;
   for (i=0; i<sizeof(struct ModeInfoBlock); i++)
      ((unsigned char *)VgaPtr)[i] = real_ptr[i];
#elif defined( __GNUC__ )
   regs.x.di = real_buf[0];
   int86x(0x10, &regs, &regs, &sregs);
   if (regs.h.al != 0x4f || regs.h.ah != 0)
      return 0;
   for (i=0; i<sizeof(struct ModeInfoBlock); i++)
      ((unsigned char *)VgaPtr)[i] = real_ptr[i];
#else
   regs.x.di = FP_OFF(VgaPtr);
   sregs.es  = FP_SEG(VgaPtr);
   int86x(0x10, &regs, &regs, &sregs);
   if (regs.h.al != 0x4f || regs.h.ah != 0)
      return 0;
#endif

   /* The mode is supported - save useful information and initialize
      the graphics display */
   line_length = VgaPtr->BytesPerScanLine;
   granularity = ((unsigned long)VgaPtr->WinGranularity) << 10;
   screen_maxx = VgaPtr->XResolution;
   screen_maxy = VgaPtr->YResolution;

   if (Reset_Display_Flag) {
      /* Now go set the video adapter into the requested mode */
      regs.h.ah = 0x4f;
      regs.h.al = 0x02;
      regs.x.bx = mode;
      int86(0x10, &regs, &regs);
      return (regs.h.al == 0x4f && regs.h.ah == 0x00 ? 1 : 0);
      }
}

/* Set a 256 entry palette with the values given in "palbuf". */
static void
setmany(unsigned char palbuf[256][3], int start, int count)
{
   unsigned i, j;

   for (i=0,j=start;i<count;i++,j++) {
      outp(0x3c8, j);
      outp(0x3c9, palbuf[i][0]);
      outp(0x3c9, palbuf[i][1]);
      outp(0x3c9, palbuf[i][2]);
      }
}

/* Make space for a palette & make a 332 color map */
static void
palette_init()
{
    unsigned i, r, g, b;

    if (palette == NULL) {
	palette = malloc(sizeof(palette_array));
	if (palette == NULL) {
	    fprintf(stderr, "Failed to allocate palette array\n");
	    exit(1);
	}
    }

    i = 0;
    for (r=0;r<8;r++)
	for (g=0;g<8;g++)
	    for (b=0;b<4;b++) {
	       (*palette)[i][0] = r << 3;
	       (*palette)[i][1] = g << 3;
	       (*palette)[i][2] = b << 4;
	       i++;
	    }
    setmany(*palette, 0, 256);
}

static void
quantize_4bit(COORD3 color, quantized_color *qcolor)
{
   unsigned char r, g;
   float d;

   qcolor->bytes = 1;

   /* EGA colors */
   d = 0.5;
   r = 0;
   if (color[0] + color[1] - color[2] > d)
      r |= 0x01;
   if (color[0] - color[1] + color[2] > d)
      r |= 0x02;
   if (-color[0] + color[1] + color[2] > d)
      r |= 0x04;
   d = 1.5;
   g = 0x08;
   if (r == 0) {
      d = 0.5;
      g = 0x07;
      }
   else if (r == 7) {
      d = 2.5;
      r = 0x08;
      g = 0x07;
      }
   if (color[0] + color[1] + color[2] > d)
      r |= g;
   qcolor->byte[0] = EGA_remap[r];
}

static void
quantize(color, qcolor)
    COORD3 color;
    quantized_color *qcolor;
{
   int i;
   unsigned r, g, b;

   i = 255.0 * color[Z];
   if (i<0) i=0;
   else if (i>=256) i = 255;
   b = (unsigned char)i;

   i = 255.0 * color[Y];
   if (i<0) i=0;
   else if (i>=256) i = 255;
   g = (unsigned char)i;

   i = 255.0 * color[X];
   if (i<0) i=0;
   else if (i>=256) i = 255;
   r = (unsigned char)i;

   /* Ignore the resolution and make the display 640x480 hicolor colors. */
   if (Display_Flag >= FIRST_4BIT_MODE &&
       Display_Flag <  FIRST_4BIT_MODE + VIDEO_RESOLUTIONS) {
      qcolor->bytes = 1;
      quantize_4bit(color, qcolor);
      qcolor->byte[0] += Palette_Start;
      }
   else if (Display_Flag >= FIRST_8BIT_MODE &&
       Display_Flag <  FIRST_8BIT_MODE + VIDEO_RESOLUTIONS) {
      qcolor->bytes = 1;
      switch (Palette_Flag) {
      case 3:
	 quantize_4bit(color, qcolor);
	 break;
      case 2:
	 /* 666 */
	 qcolor->byte[0] = 6 * (6 * (r / 51) + (g / 51)) + b / 51;
	 break;
      case 1:
	 /* 884 */
	 qcolor->byte[0] = (r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6);
	 break;
      default:
	 /* Greyscale */
	 qcolor->byte[0] = (r>g?(r>b?r>>2:b>>2):(g>b?g>>2:b>>2));
	 }
      qcolor->byte[0] += Palette_Start;
      }
   else if (Display_Flag >= FIRST_HICOLOR_MODE &&
	    Display_Flag <  FIRST_HICOLOR_MODE + VIDEO_RESOLUTIONS) {
      /* Hicolor, add bits together */
      qcolor->bytes = 2;
      qcolor->byte[1] = ((r >> 1) & 0x7c) | (g >> 6);
      qcolor->byte[0] = ((g << 2) & 0xe0) | (b >> 3);
      }
   else if (Display_Flag >= FIRST_16BIT_MODE &&
	    Display_Flag <  FIRST_16BIT_MODE + VIDEO_RESOLUTIONS) {
      /* Hicolor, add bits together */
      qcolor->bytes = 2;
      qcolor->byte[1] = (r & 0xf8) | (g >> 5);
      qcolor->byte[0] = ((g << 2) & 0xe0) | (b >> 3);
      }
   else if (Display_Flag >= FIRST_TRUECOLOR_MODE &&
	    Display_Flag <  FIRST_TRUECOLOR_MODE + VIDEO_RESOLUTIONS) {
      /* Truecolor modes, use quantized pixels */
      qcolor->bytes = 3;
      qcolor->byte[2] = r;
      qcolor->byte[1] = g;
      qcolor->byte[0] = b;
      }
   else {
      display_close(0);
      fprintf(stderr, "Bad display mode in quantize");
      exit(1);
      }
}

static void
plotpoint(int x, int y, quantized_color *color)
{
   unsigned char __far *fp;
   unsigned long fpa, fpb;
   unsigned bank;
   int i;

   /* Don't plot points outside the screen boundaries */
   if (x < 0 || x >= screen_maxx) return;
   if (y < 0 || y >= screen_maxy) return;

#if defined( BIOS_DRAW_ONLY )
   /* Have to do things a little bit differently in 16 color modes */
   bios_putpixel(x, y, color->byte[0]);
#else
   fpa = (unsigned long)line_length * y + (x * color->bytes);
   for (i=0;i<color->bytes;i++,fpa++) {
      if (Display_Flag > 1) {
	 /* Only have more than 64K pixels in VESA modes */
	 bank = fpa / granularity;
	 setvesabank(bank);
	 }
      fpb = fpa % granularity;
#if defined( DOS386 )
      fp = MK_FP(_x386_zero_base_selector, 0xA0000 + fpb);
#elif defined( __GNUC__ )
      fp = (unsigned char *)MK_FP(0xA000, (unsigned int)fpb);
#else
      fp = (void *)MK_FP(0xA000, fpb);
#endif
      *fp = color->byte[i];
      }
#endif
}

void
display_clear(void)
{
   unsigned i, j;

   if (Display_Flag >= 1) {
      /* clear the display the hard way, a pixel at a time. */
      setvesabank(0);
      for (i=offy;i<maxy;i++)
	 for (j=offx;j<maxx;j++)
	    if (Display_Flag < FIRST_HICOLOR_MODE)
	       plotpoint(j, i, &black1);
	    else if (Display_Flag < FIRST_TRUECOLOR_MODE)
	       plotpoint(j, i, &black2);
	    else
	       plotpoint(j, i, &black3);
      }
}

void
display_init(xres, yres, bk_color)
    int xres, yres;
    COORD3 bk_color;
{
   static int init_flag = 0;
   int sflag, x1, y1;
   COORD3 white;

   sflag = 0; /* No subscreen window */
   if (Display_x0 < 0) Display_x0 = 0; else sflag = 1;
   if (Display_y0 < 0) Display_y0 = 0; else sflag = 1;
   if (Display_xl < 0) Display_xl = xres; else sflag = 1;
   if (Display_yl < 0) Display_yl = yres; else sflag = 1;

   /* Now go set up the display */
   if (init_flag)
      display_clear();
   else {
#if defined( DOS386 ) || defined( __GNUC__ )
      if (Display_Flag != FIRST_8BIT_MODE)
	 find_real_buf();
#endif
      init_flag = 1;
      switch (Display_Flag) {

	 /* Truecolor video modes */
	 case FIRST_TRUECOLOR_MODE+4:
	    if (setvesamode(0x11b, 0)) {
	       Display_Flag = FIRST_TRUECOLOR_MODE+4;
	       break;
	       }
	 case FIRST_TRUECOLOR_MODE+3:
	    if (setvesamode(0x118, 0)) {
	       Display_Flag = FIRST_TRUECOLOR_MODE+3;
	       break;
	       }
	 case FIRST_TRUECOLOR_MODE+2:
	    if (setvesamode(0x115, 0)) {
	       Display_Flag = FIRST_TRUECOLOR_MODE+2;
	       break;
	       }
	 case FIRST_TRUECOLOR_MODE+1:
	    if (setvesamode(0x112, 0)) {
	       Display_Flag = FIRST_TRUECOLOR_MODE+1;
	       break;
	       }
	 case FIRST_TRUECOLOR_MODE:
	    if (setvesamode(0x10f, 0)) {
	       Display_Flag = FIRST_TRUECOLOR_MODE;
	       break;
	       }

	 /* Hicolor video modes */
	 case FIRST_16BIT_MODE+4:
	    if (setvesamode(0x11a, 0)) {
	       Display_Flag = FIRST_16BIT_MODE+4;
	       break;
	       }
	 case FIRST_HICOLOR_MODE+4:
	    if (setvesamode(0x119, 0)) {
	       Display_Flag = FIRST_HICOLOR_MODE+4;
	       break;
	       }
	 case FIRST_16BIT_MODE+3:
	    if (setvesamode(0x117, 0)) {
	       Display_Flag = FIRST_16BIT_MODE+3;
	       break;
	       }
	 case FIRST_HICOLOR_MODE+3:
	    if (setvesamode(0x116, 0)) {
	       Display_Flag = FIRST_HICOLOR_MODE+3;
	       break;
	       }
	 case FIRST_16BIT_MODE+2:
	    if (setvesamode(0x114, 0)) {
	       Display_Flag = FIRST_16BIT_MODE+2;
	       break;
	       }
	 case FIRST_HICOLOR_MODE+2:
	    if (setvesamode(0x113, 0)) {
	       Display_Flag = FIRST_HICOLOR_MODE+2;
	       break;
	       }
	 case FIRST_16BIT_MODE+1:
	    if (setvesamode(0x111, 0)) {
	       Display_Flag = FIRST_16BIT_MODE+1;
	       break;
	       }
	 case FIRST_HICOLOR_MODE+1:
	    if (setvesamode(0x110, 0)) {
	       Display_Flag = FIRST_HICOLOR_MODE+1;
	       break;
	       }
	 case FIRST_16BIT_MODE:
	    if (setvesamode(0x10e, 0)) {
	       Display_Flag = FIRST_16BIT_MODE;
	       break;
	       }
	 case FIRST_HICOLOR_MODE:
	    if (setvesamode(0x10d, 0)) {
	       Display_Flag = FIRST_HICOLOR_MODE;
	       break;
	       }

	 /* Now try the 8 bit modes, starting at the most extreme */
	 case FIRST_8BIT_MODE+4:
	    if (setvesamode(0x107, 0)) {
	       Display_Flag = FIRST_8BIT_MODE+4;
	       palette_init();
	       break;
	       }
	 case FIRST_8BIT_MODE+3:
	    if (setvesamode(0x105, 0)) {
	       Display_Flag = FIRST_8BIT_MODE+3;
	       palette_init();
	       break;
	       }
	 case FIRST_8BIT_MODE+2:
	    if (setvesamode(0x103, 0)) {
	       Display_Flag = FIRST_8BIT_MODE+2;
	       palette_init();
	       break;
	       }
	 case FIRST_8BIT_MODE+1:
	    if (setvesamode(0x101, 0)) {
	       Display_Flag = FIRST_8BIT_MODE+1;
	       palette_init();
	       break;
	       }
	 case FIRST_8BIT_MODE:
	    setvideomode(19);
	    screen_maxx = 320;
	    screen_maxy = 200;
	    Display_Flag = FIRST_8BIT_MODE;
	    palette_init();
	    break;

	 case FIRST_4BIT_MODE+1:
	    setvideomode(18);
	    screen_maxx = 640;
	    screen_maxy = 480;
	    Palette_Flag = 3; /* Must be 4 bit palette */
	    Display_Flag = FIRST_4BIT_MODE+1;
	    break;

	 case FIRST_4BIT_MODE:
	    setvideomode(13);
	    screen_maxx = 320;
	    screen_maxy = 200;
	    Palette_Flag = 3; /* Must be 4 bit palette */
	    Display_Flag = FIRST_4BIT_MODE;
	    break;

	 default:
	    fprintf(stderr, "Failed to set video mode: %d\n", Display_Flag);
	    exit(1);
	    }
	 }

   /* Do some conditioning on the screen window to ensure it doesn't cause
      pixels to be generated off screen */
   x1 = Display_x0 + Display_xl;
   y1 = Display_y0 + Display_yl;
   if (x1 >= screen_maxx) { Display_xl = screen_maxx - Display_x0; }
   if (y1 >= screen_maxy) { Display_yl = screen_maxy - Display_y0; }

   /* If there is a rendering subwindow of the screen that we should
      be working in.  If so, then set the scaling values appropriately. */
   if (sflag) {
      offx = Display_x0;
      offy = Display_y0;
      maxx = Display_x0 + Display_xl;
      maxy = Display_y0 + Display_yl;
      }
   else {
      offx = 0;
      offy = 0;
      maxx = screen_maxx;
      maxy = screen_maxy;
      }

   if (xres > Display_xl)
      X_Display_Scale = (float)Display_xl / (float)xres;
   else
      X_Display_Scale = 1.0;
   if (yres > Display_yl)
      Y_Display_Scale = (float)Display_yl / (float)yres;
   else
      Y_Display_Scale = 1.0;
   if (X_Display_Scale < Y_Display_Scale)
      Y_Display_Scale = X_Display_Scale;
   else if (Y_Display_Scale < X_Display_Scale)
      X_Display_Scale = Y_Display_Scale;

    /* Outline the actual "visible" display area in the window */
    SET_COORD3(white, 1.0, 1.0, 1.0);
    display_line(0, 0, 0, yres, white);
    display_line(0, yres,  xres,  yres, white);
    display_line(xres, yres, xres,  0, white);
    display_line(xres, 0,  0, 0, white);
}

void
display_close(int wait_flag)
{
    union REGS regs;

   if (wait_flag) {
#if !defined( _WINDOWS )
      while (!kbhit()) ;
#endif
      if (!getch()) getch();
      }

   if (palette != NULL) {
      free(palette);
      palette = NULL;
      }

   /* Go back to standard text mode */
   regs.x.ax = 0x0003;
   int86(0x10, &regs, &regs);

   return;
}

void
display_plot(x, y, color)
    int x, y;
    COORD3 color;
{
   quantized_color qcolor;

   x = offx + X_Display_Scale * x;
   y = offy + Y_Display_Scale * y;

   quantize(color, &qcolor);
   plotpoint(x, y, &qcolor);
}

/* Draw a line between two points */
static void
line2d(int x1, int y1, int x2, int y2, quantized_color *qcolor)
{
   int d1, x, y;
   int ax, ay;
   int sx, sy;
   int dx, dy;

   dx = x2 - x1;
   ax = ABSOLUTE(dx) << 1;
   sx = SGN(dx);
   dy = y2 - y1;
   ay = ABSOLUTE(dy) << 1;
   sy = SGN(dy);

   x = x1;
   y = y1;

   plotpoint(x, y, qcolor);
   if (ax > ay) {
      /* x dominant */
      d1 = ay - (ax >> 1);
      for (;;) {
	 if (x==x2) return;
	 if (d1>=0) {
	    y += sy;
	    d1 -= ax;
	    }
	 x += sx;
	 d1 += ay;
	 plotpoint(x, y, qcolor);
	 }
      }
   else {
      /* y dominant */
      d1 = ax - (ay >> 1);
      for (;;) {
	 if (y == y2) return;
	 if (d1 >= 0) {
	    x += sx;
	    d1 -= ay;
	    }
	 y += sy;
	 d1 += ax;
	 plotpoint(x, y, qcolor);
	 }
      }
}

void
display_line(x0, y0, x1, y1, color)
    int x0, y0, x1, y1;
    COORD3 color;
{
   quantized_color qcolor;

TEST_ABORT

   /* Scale from image size to actual screen pixel size */
   x0 = offx + X_Display_Scale * x0;
   y0 = offy + Y_Display_Scale * y0;
   x1 = offx + X_Display_Scale * x1;
   y1 = offy + Y_Display_Scale * y1;

   if (x0 < Display_x0)
      x0 = Display_x0;
   else if (x0 >= Display_x0 + Display_xl)
      x0 = Display_x0 + Display_xl - 1;
   if (y0 < Display_y0)
      y0 = Display_y0;
   else if (y0 >= Display_y0 + Display_yl)
      y0 = Display_y0 + Display_yl - 1;
   if (x1 < Display_x0)
      x1 = Display_x0;
   else if (x1 >= Display_x0 + Display_xl)
      x1 = Display_x0 + Display_xl - 1;
   if (y1 < Display_y0)
      y1 = Display_y0;
   else if (y0 >= Display_y0 + Display_yl)
      y1 = Display_y0 + Display_yl - 1;

   quantize(color, &qcolor);
   line2d(x0, y0, x1, y1, &qcolor);
}
