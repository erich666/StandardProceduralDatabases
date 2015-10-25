/*
 * drv_hp.c - a budget displayer for HP workstations, by Eric Haines
 *
 * A real kludge, I admit - do want you want in gopen().
 *
 * I assume you're in X windows and start with a graphics window:
	xwcreate -depth 24 -g 480x512+0+0 GraphWin
 */

#include <stdio.h>
#include <starbase.c.h>
#include "drv.h"

#define	ClrList(c) (float)((c)[0]), (float)((c)[1]), (float)((c)[2])
static int FD = -1 ;
static double X_res, Y_res ;

void
display_clear()
{
    /* Insert code to clear the graphics display here */
    clear_control( FD, CLEAR_VDC_EXTENT ) ;
    clear_view_surface( FD ) ;
}

void
display_init(xres, yres, bk_color)
    int xres, yres;
    COORD3 bk_color;
{
    /* Insert code to open/initialize the graphics display here */
    if ( -1 == ( FD =
	gopen( "/dev/screen/GraphWin", OUTINDEV, NULL, INIT|ACCELERATED) ) ) {

	fprintf( stderr, "could not open device for output!\n" ) ;
	return ;
    }
    shade_mode( FD, CMAP_FULL|INIT, FALSE ) ;
    mapping_mode( FD, TRUE ) ;
    background_color( FD, ClrList( bk_color ) ) ;
    display_clear() ;
    X_res = xres ;
    Y_res = yres ;
}

void
display_close(wait_flag)
    int wait_flag ;
{
    /* Insert code to close the graphics display here */
    gclose( FD ) ;
}

/* currently not used for anything, so you don't have to implement */
void
display_plot(x, y, color)
    int x, y;
    COORD3 color;
{
    float clist[2] ;

    /* Insert code to plot a single pixel here */
    marker_color( FD, ClrList( color ) ) ;
    clist[0] = (float)x/X_res ;
    clist[1] = (float)y/Y_res ;
    polymarker2d( FD, clist, 1, FALSE ) ;
}

/* weirdly enough, x and y are centered around the origin */
void
display_line(x0, y0, x1, y1, color)
    int x0, y0, x1, y1;
    COORD3 color;
{
    /* Insert line drawing code here */
    line_color( FD, ClrList( color ) ) ;
    move2d( FD, (float)(((double)x0)/X_res),
	    (float)(((double)-y0+Y_res)/Y_res) ) ;
    draw2d( FD, (float)(((double)x1)/X_res),
	    (float)(((double)-y1+Y_res)/Y_res) ) ;
}

int
kbhit()
{
    /* Insert keyboard hit (i.e. interrupt operation) test code here */

    /* currently always no interrupt */
    return( 0 ) ;
}
