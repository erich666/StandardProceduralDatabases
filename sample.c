/*
 * sample.c - Simple file to make a scene with a few objects
 *
 * Author:  Alexander Enzmann
 *
 * Modified: 1 September 1993
 *           Eric Haines
 *           changed file name, made sure values were passed as doubles
 *           (the HPUX compiler sees "..., 5, ..." as an integer and merrily
 *           passes the integer representation down to the subroutine which
 *           wants a double).
 *
 * size_factor is ignored.
 *
 *	size_factor	# spheres	# squares
 *	     x		    xx		     x
 */

#include <stdio.h>
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

static COORD3 Red     = { 1.0, 0.0, 0.0 };
static COORD3 Cyan    = { 0.0, 1.0, 1.0 };
static COORD3 Yellow  = { 1.0, 0.0, 1.0 };
static COORD3 Magenta = { 1.0, 1.0, 0.0 };

int
main(argc, argv)
int argc;
char *argv[];
{
    COORD3 back_color;
    COORD4 from, at, up;
    COORD4 center, normal;
    COORD4 base, apex;
	
    PLATFORM_INIT(SPD_GENERIC);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Sample" ) ) {
		return EXIT_FAIL;
    }
	/*    lib_set_polygonalization(8, 8); */
	
    /* output background color - Light Grey */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3( back_color, 0.8, 0.8, 0.8 ) ;
    lib_output_background_color( back_color ) ;
	
    SET_COORD3(from, 0, 5, -5);
    SET_COORD3(at, 0, 0, 0);
    SET_COORD3(up, 0, 1, 0);
    lib_output_viewpoint(from, at, up, 45.0, 1.0, 0.001, 512, 512);
	
    SET_COORD4(center, -10, 20, -20, 1.0);
    lib_output_light(center);
	
    lib_output_color(NULL, Cyan, 0.1, 0.7, 0.0, 0.7, 10.0, 0.0, 1.0);
    SET_COORD3(center,-1, 0, 0);
    SET_COORD3(normal, 1, 1,-0.5);
    lib_output_torus(center, normal, 2.0, 0.5, output_format);
	PLATFORM_PROGRESS(0, 0, 4);
	
    lib_output_color(NULL, Magenta, 0.1, 0.7, 0.0, 0.7, 10.0, 0.0, 1.0);
    SET_COORD3(center, 0, 0, 0);
    SET_COORD3(normal, 0, 1,-0.5);
    lib_output_torus(center, normal, 2.0, 0.5, output_format);
	PLATFORM_PROGRESS(0, 1, 4);
	
    lib_output_color(NULL, Yellow, 0.1, 0.7, 0.0, 0.7, 10.0, 0.0, 1.0);
    SET_COORD3(center, 1, 0, 0);
    SET_COORD3(normal,-1, 1,-0.5);
    lib_output_torus(center, normal, 2.0, 0.5, output_format);
	PLATFORM_PROGRESS(0, 2, 4);
	
    lib_output_color(NULL, Red, 0.1, 0.7, 0.0, 0.7, 10.0, 0.0, 1.0);
    SET_COORD4(base, 0, -2, 0, 2);
    SET_COORD4(apex, 0,  2, 0, 0.1);
    lib_output_cylcone(base, apex, output_format);
	PLATFORM_PROGRESS(0, 3, 4);
	
    lib_set_polygonalization(4, 1);
    lib_output_color(NULL, Red, 0.1, 0.7, 0.0, 0.7, 10.0, 0.0, 1.0);
    SET_COORD3(base, 0, -2, 0);
    SET_COORD3(apex, 0,  1, 0);
    lib_output_disc(base, apex, 0.0, 4.0, output_format);
	PLATFORM_PROGRESS(0, 4, 4);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
