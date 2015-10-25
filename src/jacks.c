/*
 * jacks.c - recursive jack shapes, sort of like a Menger sponge
 *
 * Author:  Alexander Enzmann
 *
 * Modified:
 *
 *      size_factor     # spheres       # squares
 *           x              xx               x
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>     /* atoi */
#include "def.h"
#include "drv.h"        /* display_close() */
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

/* Some basic colors */
static COORD3 Pink    = { 0.737, 0.561, 0.561};
static COORD3 DarkPurple = {0.2, 0.05, 0.2};

/* Create a single copy of our recursive object.  The general
   sizing and placement of the object are maintained by the
   recursive routine make_rec_jack.  This routine does the
   actual geometry building */
static void
make_jack_obj()
{
	COORD3 scale;
	COORD4 center, base, apex;
	
	/* Save the current transformation */
	lib_tx_push();
	
	/* Scale the object down to prevent overlaps */
	SET_COORD3(scale, 0.75, 0.75, 0.75);
	lib_tx_scale(scale);
	
	/* Now create the object - three intersecting cylinders
	 * with spheres at both ends
	 */
	
	/* Cylinders on each axis */
	SET_COORD4(base, -1, 0, 0, 0.1);
	SET_COORD4(apex,  1, 0, 0, 0.1);
	lib_output_cylcone(base, apex, output_format);
	SET_COORD4(base, 0, -1, 0, 0.1);
	SET_COORD4(apex, 0,  1, 0, 0.1);
	lib_output_cylcone(base, apex, output_format);
	SET_COORD4(base, 0, 0, -1, 0.1);
	SET_COORD4(apex, 0, 0,  1, 0.1);
	lib_output_cylcone(base, apex, output_format);
	
	/* Spheres at the ends of the cylinders */
	SET_COORD4(center, 1, 0, 0, 0.2);
	lib_output_sphere(center, output_format);
	SET_COORD4(center, 0, 1, 0, 0.2);
	lib_output_sphere(center, output_format);
	SET_COORD4(center, 0, 0, 1, 0.2);
	lib_output_sphere(center, output_format);
	SET_COORD4(center,-1, 0, 0, 0.2);
	lib_output_sphere(center, output_format);
	SET_COORD4(center, 0,-1, 0, 0.2);
	lib_output_sphere(center, output_format);
	SET_COORD4(center, 0, 0,-1, 0.2);
	lib_output_sphere(center, output_format);
	
	/* Restore the transform to what it was prior to
	 * entering this routine.
	 */
	lib_tx_pop();
}

/* Create a jack shaped object, then put a smaller copy in
   each of the octants defined by the arms of the jack.
   This process is repeated until depth reaches max_depth. */
static void
make_rec_jack(depth, max_depth)
{
    double i, j, k;
    COORD3 scale, trans;
	
    make_jack_obj();
	
    if (depth < max_depth) {
		SET_COORD3(scale, 0.5, 0.5, 0.5);
		for (i=-0.5;i<=0.5;i+=1)
			for (j=-0.5;j<=0.5;j+=1)
				for (k=-0.5;k<=0.5;k+=1) {
					if (depth==1)
						PLATFORM_PROGRESS(0, 4+i*4+j*2+k, 7);
					lib_tx_push();
					SET_COORD3(trans, i, j, k);
					lib_tx_translate(trans);
					lib_tx_scale(scale);
					make_rec_jack(depth+1, max_depth);
					lib_tx_pop();
				}
	}
}

int
main(argc, argv)
int argc;
char *argv[];
{
    COORD4 from, at, up;
    COORD4 center;
    double lscale;
	
    PLATFORM_INIT(SPD_JACKS);
	
    /* Start by defining which raytracer we will be using */
	if (lib_gen_get_opts(argc, argv, &size_factor,
		&raytracer_format, &output_format))
		return EXIT_FAIL;
	
    /* Set the output file */
    if (lib_open(raytracer_format, "Jacks"))
		return EXIT_FAIL;
	
    lib_set_polygonalization(2, 2);
	
    /* output background color - Light Grey */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    lib_output_background_color(DarkPurple);
	
    /* Viewpoint */
    SET_COORD3(from, 0, 0, -8);
    SET_COORD3(at, 0, 0, 0);
    SET_COORD3(up, 0, 1, 0);
    lib_output_viewpoint(from, at, up, 25.0, 1.0, 0.001, 256, 256);
	
    /*
	 * For raytracers that don't scale the light intensity,
	 * we will do it for them
	 */
#define NUM_LIGHTS    1
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    SET_COORD4(center, -10, 3, -20, lscale);
    lib_output_light(center);
	
#if 0
    /* Declare all the textures we will be using - this way we only
	 * need to have one set of declarations, rather than creating
	 * a new texture for every single object
	 */
    allocate_textures();
#endif
	
    /* Save the root transformation */
    lib_tx_push();
	
	/* Rotate everything about the x and y axes */
	lib_tx_rotate(X_AXIS,-30 * PI / 180.0);
	lib_tx_rotate(Y_AXIS,-20 * PI / 180.0);
	
	lib_output_color(NULL, Pink, 0.1, 0.7, 0.7, 0.4, 20.0, 0.0, 1.0);
	make_rec_jack(1, size_factor);
	
    /* Back to where we started */
    lib_tx_pop();
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
