/*
 * lattice.c - Create a set of shiny spheres, with each sphere connected to
 *     	its neighbors by matte cylinders, forming a cubic lattice.
 *	Six light sources.
 *
 * Author:  Antonio Costa, INESC-Norte.
 * Version:  3.1 (04/09/93)
 *
 * SizeFactor determines the number of objects output.
 *	Total spheres   = (SF+1)**3.
 *	Total cylinders = 3*(SF**3)+...
 *
 *	SizeFactor	# spheres	# cylinders
 *	     1		    8		     12
 *	     2		    27		     54
 *	     3		    64		    144
 *
 *	     5		   216		    540
 *
 *	    12		   2197		    6084
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"		/* display_close() */
#include "lib.h"

/* These may be read from the command line */
static int size_factor		= 12;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format	= OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

#define RADIUS1 (1.0 / 4.0)
#define RADIUS2 (1.0 / 16.0)

#define inv_factor (1.0 / (double) size_factor)
#define radius1 ((double) RADIUS1 / (double) size_factor)
#define radius2 ((double) RADIUS2 / (double) size_factor)

main(argc, argv)
    int		argc;
    char	*argv[];
{
    COORD4	back_color, obj_color;
    COORD4	light;
    COORD4	from, at, up;
    COORD4	center, center1, center2;
    long	x, y, z;
    double	delta, x0, y0, z0, lscale;
	
    PLATFORM_INIT(SPD_LATTICE);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Lattice" ) ) {
		return EXIT_FAIL;
    }
	
    delta =
		(radius1 * (1.0 - sqrt((double) RADIUS2 / (double) RADIUS1))) * 0.99;
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3(back_color, 0.078, 0.361, 0.753);
    lib_output_background_color(back_color);
	
    /* output viewpoint */
    SET_COORD3(from, ((double) (size_factor >> 1) + 0.5) * inv_factor, 1.0,
		1.0 - 1.0 / (double) (size_factor << 1));
    SET_COORD3(at, ((double) (size_factor >> 1) + 0.5) * inv_factor, -1.0,
		-1.0 - 1.0 / (double) (size_factor << 1));
    SET_COORD3(up, 0.2, 1.0, 0.0);
    lib_output_viewpoint(from, at, up, 60.0, 1.0, 0.0, 512, 512);
	
    /* output light sources */
    lscale = 1.0 / sqrt(6.0);
	
    SET_COORD4(light, 2.0, 0.5, 0.5, lscale);
    lib_output_light(light);
    SET_COORD4(light, -1.0, 0.5, 0.5, lscale);
    lib_output_light(light);
    SET_COORD4(light, 0.5, 2.0, 0.5, lscale);
    lib_output_light(light);
    SET_COORD4(light, 0.5, -1.0, 0.5, lscale);
    lib_output_light(light);
    SET_COORD4(light, 0.5, 0.5, 2.0, lscale);
    lib_output_light(light);
    SET_COORD4(light, 0.5, 0.5, -1.0, lscale);
    lib_output_light(light);
	
    for (x = 0; x <= size_factor; x++) {
		x0 = (double) x / (double) size_factor;
		for (y = 0; y <= size_factor; y++) {
			PLATFORM_PROGRESS(0, x*(size_factor+1)+y, (size_factor+1)*(size_factor+1));
			y0 = (double) y / (double) size_factor;
			for (z = 0; z <= size_factor; z++) {
				
				PLATFORM_MULTITASK();
				
				z0 = (double) z / (double) size_factor;
				
				SET_COORD3(obj_color, 0.9, 0.9, 0.9);
				lib_output_color(NULL, obj_color,
					0.0, 0.5, 0.5, 0.5, 37.0, 0.0, 0.0);
				
				SET_COORD4(center, x0, y0, z0, radius1);
				lib_output_sphere(center, output_format);
				
				if (x != size_factor) {
					SET_COORD3(obj_color, 0.9, 0.1, 0.1);
					lib_output_color(NULL, obj_color,
						0.1, 0.99, 0.0, 0.0, 0.0, 0.0, 0.0);
					
					SET_COORD4(center1, x0 + delta, y0, z0, radius2);
					SET_COORD4(center2, x0 + inv_factor - delta, y0, z0,
						radius2);
					lib_output_cylcone(center1, center2, output_format);
				}
				if (y != size_factor) {
					SET_COORD3(obj_color, 0.1, 0.9, 0.1);
					lib_output_color(NULL, obj_color,
						0.1, 0.99, 0.0, 0.0, 0.0, 0.0, 0.0);
					
					SET_COORD4(center1, x0, y0 + delta, z0, radius2);
					SET_COORD4(center2, x0, y0 + inv_factor - delta, z0,
						radius2);
					lib_output_cylcone(center1, center2, output_format);
				}
				if (z != size_factor)
				{
					SET_COORD3(obj_color, 0.1, 0.1, 0.9);
					lib_output_color(NULL, obj_color,
						0.1, 0.99, 0.0, 0.0, 0.0, 0.0, 0.0);
					
					SET_COORD4(center1, x0, y0, z0 + delta, radius2);
					SET_COORD4(center2, x0, y0, z0 + inv_factor - delta,
						radius2);
					lib_output_cylcone(center1, center2, output_format);
				}
			}
		}
    }
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
