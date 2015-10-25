/*
 * nurbtst.c - Simple file to create a single NURB patch
 *
 * Author:  Alexander Enzmann
 *
 * size_factor is ignored.
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"


/* These may be read from the command line */
static int size_factor      = 1;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

/* Some standard colors */
static COORD3 White   = { 1.0, 1.0, 1.0 };
static COORD3 Red     = { 1.0, 0.0, 0.0 };
static COORD3 Green   = { 0.0, 1.0, 0.0 };
static COORD3 Blue    = { 0.0, 0.0, 1.0 };
static COORD3 Cyan    = { 0.0, 1.0, 1.0 };
static COORD3 Yellow  = { 1.0, 0.0, 1.0 };
static COORD3 Magenta = { 1.0, 1.0, 0.0 };
static COORD3 Black   = { 0.0, 0.0, 0.0 };

float nknots[] = {0, 0, 0, 0, 1.5, 1.5, 3, 3, 3, 3}; /* Non-uniform knot vector */
float mknots[] = {0, 0, 0, 0, 1, 2, 2, 2, 2};        /* Uniform open knot vector */
COORD4 ctlpts0[] = {{ 0, 0, 0, 1}, { 1, 0, 3, 1}, { 2, 0,-3, 1},
{ 3, 0, 3, 1}, { 4, 0, 0, 1}};
COORD4 ctlpts1[] = {{ 0, 1, 0, 1}, { 1, 1, 0, 1}, { 2, 1, 0, 1},
{ 3, 1, 0, 1}, { 4, 1, 0, 1}};
COORD4 ctlpts2[] = {{ 0, 2, 0, 1}, { 1, 2, 0, 2}, { 2, 2, 5, 0.5},
{ 3, 2, 0, 1}, { 4, 2, 0, 1}};
COORD4 ctlpts3[] = {{ 0, 3, 0, 1}, { 1, 3, 0, 2}, { 2, 3, 5, 0.5},
{ 3, 3, 0, 1}, { 4, 3, 0, 1}};
COORD4 ctlpts4[] = {{ 0, 4, 0, 1}, { 1, 4, 0, 1}, { 2, 4, 0, 20},
{ 3, 4, 0, 1}, { 4, 4, 0, 1}};
COORD4 ctlpts5[] = {{ 0, 5, 0, 1}, { 1, 5,-3, 1}, { 2, 5, 3, 1},
{ 3, 5,-3, 1}, { 4, 5, 0, 1}};

/* Build a single NURB patch and dump it */
static void
create_a_nurb(curve_format)
int curve_format ;
{
    COORD4 *ctlpts[6];
    COORD3 vec;
	
    lib_output_color(NULL, Red, 0.1, 0.7, 0.0, 0.7, 10.0, 0.0, 1.0);
	
    ctlpts[0] = ctlpts0;
    ctlpts[1] = ctlpts1;
    ctlpts[2] = ctlpts2;
    ctlpts[3] = ctlpts3;
    ctlpts[4] = ctlpts4;
    ctlpts[5] = ctlpts5;
	
    lib_tx_push();
	
	lib_tx_rotate(Y, DEG2RAD(30.0));
	lib_tx_rotate(X, DEG2RAD(-90.0));
	SET_COORD3(vec, -2.0, -2.5, 0.0);
	lib_tx_translate(vec);
	
	PLATFORM_PROGRESS(0, 1, 2);
	/* lib_output_nurb(4, 6, 4, 5, nknots, mknots, ctlpts, curve_format); */
	lib_output_nurb(4, 6, 4, 5, NULL, NULL, ctlpts, curve_format);
	PLATFORM_PROGRESS(0, 2, 2);
	
    lib_tx_pop();
}

/* Main driver - looks the same as every other SPD file... */
int
main(argc, argv)
int argc;
char *argv[];
{
    COORD4 from, at, up;
    COORD4 center;
	
    PLATFORM_INIT(SPD_NURBTST);
	
    /* Start by defining which raytracer we will be using */
	if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "NurbTst" ) ) {
		return EXIT_FAIL;
    }
	
    lib_set_polygonalization(5, 5);
	
    /* output background color */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    lib_output_background_color( Black ) ;
	
    SET_COORD3(from, 0, 5, -10);
    SET_COORD3(at, 0, 0, 0);
    SET_COORD3(up, 0, 1, 0);
    lib_output_viewpoint(from, at, up, 30.0, 1.0, 0.001, 256, 256);
	
    SET_COORD4(center, 0, 50, -10, 1.0);
    lib_output_light(center);
	
    create_a_nurb(output_format);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
