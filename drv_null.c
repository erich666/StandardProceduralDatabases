/*
 * drv_null.c - generic graphics driver (stubs - outputs no images)
 */

#include "drv.h"

void
display_clear()
{
	/* Insert code to clear the graphics display here */
}

void
display_init(xres, yres, bk_color)
int xres, yres;
COORD3 bk_color;
{
    /* Insert code to open/initialize the graphics display here */
}

void
display_close(wait_flag)
int wait_flag ;
{
    /* Insert code to close the graphics display here */
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
    /* Insert line drawing code here */
}

int
kbhit()
{
    /* Insert keyboard hit (i.e. interrupt operation) test code here */
	
    /* currently always no interrupt */
    return( 0 ) ;
}
