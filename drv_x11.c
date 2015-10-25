/*
 * drv_X11.c - A simple X11 graphics driver for SPD.
 *             Bob Kozdemba (koz@sgi.com), 3/8/97
 *
 *             This driver requires an 8-bit PseudoColor Visual and has no
 *             support for redrawing caused by exposure events.
 *
 * drv_x11.c is written using Xlib calls based on the X11R6 distribution. A
 * sample makefile, makefile.x11 is provided. If your X11 distribution is in
 * the standard directories the c compiler looks in, things should build by
 * doing 'make -f makefile.x11'. If not, you will need to tell the c compiler
 * where the X11 distribution is located. 
 * 
 * Here are some examples you might make to the makefile.
 * 
 * CC=cc -O -I/usr/local/include/X11 -L/usr/local/lib/X11
 * 
 * CFLAGS=-O -I/usr/local/include/X11 
 * LDFLAGS=-L/usr/local/lib/X11
 * 
 * To test things out try 'balls -r 0'.
 * 
 * 
 * X11 Server Visual Support
 * -------------------------
 * 
 * The drv_x11 only works with 8-bit PseudoColor visuals which are very 
 * common on unix workstations or PC's running Linux and the XF86 SVGA X server.
 * Adding support for Monochrome or 4-bit Visuals is easy but I wanted to
 * keep the code as simple as possible. If you need one, send me some email.
 * 
 * Enjoy,
 * Koz
 * koz@sgi.com
 */

#include "drv.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* Globals for X11 */
Display *display;
unsigned int screen;
Window window;
GC gc;

/* Window sizes */
unsigned int width = 512;
unsigned int height = 512;
unsigned int depth = 8;

void
display_clear()
{
    /* Insert code to clear the graphics display here */
    XClearArea( display, window, 0, 0, width, height, False );
}

void
display_init(xres, yres, bk_color)
int xres, yres;
COORD3 bk_color;
{
    int i;
    int r, g, b;
    int done = 0;
    XEvent event;
    Colormap colormap;
    XSetWindowAttributes wAttr;
    XVisualInfo *pVisInfo,visInfo;
    int retVal;
    XGCValues gc_vals;
    XColor xcolor;
    unsigned int mask;
    unsigned long bk_pixel;

    /* Insert code to open/initialize the graphics display here */
    display = XOpenDisplay( NULL  );
    if ( display == NULL ) {
	fprintf( stderr, "Error opening %s !\n", getenv("DISPLAY") );
	exit( 1 );
    }

    screen = DefaultScreen( display );

    visInfo.screen = 0;
    visInfo.depth = depth;
    visInfo.class = PseudoColor;
    mask = VisualScreenMask | VisualDepthMask | VisualClassMask;

    pVisInfo = XGetVisualInfo(display, mask, &visInfo, &retVal);

    if (!retVal) {
	    fprintf(stderr,"Could not get an 8-bit PseudoColor visual !\n");
	    exit(1);
	}

    colormap= XCreateColormap(display,
			      RootWindowOfScreen(ScreenOfDisplay(display,0)),
			      pVisInfo->visual, AllocAll);

    if (!colormap){
	fprintf(stderr,"Could not create color map\n");
	exit(1);
    }
    

    /* Store the colors, 16 bits/component */
    i = 0;
    for(r=0; r<8; r+= 1) {
	for(g=0; g<8; g+= 1) {
	    for(b=0; b<4; b+= 1) {
                xcolor.pixel = i;
                xcolor.red = (float) (r/7.0 ) * 65535;
                xcolor.green = (float) (g/7.0 ) * 65535;
                xcolor.blue = (float) (b/3.0 ) * 65535;
                xcolor.flags = DoRed | DoGreen | DoBlue;
		if (XStoreColor(display, colormap, &xcolor) == BadAccess )
		    printf("Pixel %d, Could not alloc color cell !\n", i);
		i += 1;
	    }
	}
	
    }

    /* Calculate the background pixel */
    bk_pixel = ( (unsigned long) (bk_color[0] * 7.0) ) << 5;
    bk_pixel += ( (unsigned long) (bk_color[1] * 7.0) ) << 2;
    bk_pixel += ( (unsigned long) (bk_color[2] * 3.0) );

    wAttr.event_mask = ExposureMask;
    wAttr.border_pixel = BlackPixel( display, screen );
    wAttr.background_pixel = bk_pixel;
    wAttr.colormap = colormap;
    XFlush(display);
    window = XCreateWindow(display,
			   RootWindowOfScreen(ScreenOfDisplay(display,0)),
			   0, 0,
			   width, height,
			   0, depth, CopyFromParent,
			   pVisInfo->visual,
			   CWBackPixel | CWColormap |
			   CWBorderPixel | CWEventMask,
			   &wAttr);
    if (!window) {
	fprintf(stderr,"Could not create a window\n");
	exit(1);
    }

    XStoreName( display, window, "SPD X11 Driver" );
    
    gc_vals.foreground = 0;
    gc_vals.background = bk_pixel;
    gc = XCreateGC( display, window, 
		    (GCForeground| GCBackground), &gc_vals );

    
    XMapWindow( display, window );
    XFlush( display );

    XSelectInput( display, window, ExposureMask | ButtonPressMask 
	          | KeyPressMask );

    /* Wait for the window to be exposed. */
    done = 0;
    do {
	XNextEvent( display, &event);
	switch (event.type)
	    {
		case(Expose):
		    done = 1;
		continue;
		break;
	    }
    } while(!done);

}

void
display_close(wait_flag)
int wait_flag ;
{
    /* Insert code to close the graphics display here */
    XEvent event;
    int done = 0;

    /* Wait for a mouse button or key press event, then exit. */
    do {
	XNextEvent( display, &event);
	switch (event.type)
	    {
		case( ButtonPress ):
		    case( KeyPress ):
		    done = 1;
		continue;
		break;
	    }
    } while(!done);

    XCloseDisplay( display );

}

/* currently not used for anything, so you don't have to implement */
void
display_plot(x, y, color)
int x, y;
COORD3 color;
{
    /* Insert code to plot a single pixel here */
}

/* weirdly enough, x and y are centered around the origin - see hp.c for a
 * way to offset these coordinates to a [0 to resolution-1] type of scale.
 */
void
display_line(x0, y0, x1, y1, color)
int x0, y0, x1, y1;
COORD3 color;
{
    unsigned long c;

    /* Convert (snap) the RGB color to one of 256 colors. */
    /* This probably slows things down a bit !            */
    c = ( (unsigned long) (color[0] * 7.0) ) << 5;
    c += ( (unsigned long) (color[1] * 7.0) ) << 2;
    c += ( (unsigned long) (color[2] * 3.0) );

    XSetForeground( display, gc, c );
    /* Insert line drawing code here */
    XDrawLine( display, window, gc, x0, y0, x1, y1 );

}

int
kbhit()
{
    /* Insert keyboard hit (i.e. interrupt operation) test code here */

    /* currently always no interrupt */
    return( 0 ) ;
}
