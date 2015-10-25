/*
 * balls.c - Also known as "sphereflake".  Create a set of shiny spheres, with
 *	each sphere blooming sets of 9 more spheres with 1/3rd radius.
 *	A square floor polygon is added.  Three light sources.
 *
 * Author:  Eric Haines
 *
 * size_factor determines the number of objects output.
 *	Total spheres = sum of n=0,SF of (9**SF).
 *
 *	size_factor	# spheres	# squares
 *	     1		    10		     1
 *	     2		    91		     1
 *	     3		   820		     1
 *
 *	     4		  7381		     1
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"

/* These may be read from the command line */
static int size_factor      = 4;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;


#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

static	COORD4	objset[9] ;

/*
 * Output the parent sphere, then output the children of the sphere.
 * Uses global 'objset'.
 */
static void
output_object(depth, center, direction)
int depth;
COORD4 center, direction;
{
    double  angle;
    COORD4  axis, z_axis;
    COORD4  child_pt, child_dir;
    MATRIX  mx;
    long    num_vert;
    double  scale;
	
    PLATFORM_MULTITASK();
	
    /* output sphere at location & radius defined by center */
    lib_output_sphere(center, output_format);
	
    /* check if children should be generated */
    if (depth > 0) {
		--depth ;
		
		/* rotation matrix to new axis from +Z axis */
		if ( direction[Z] >= 1.0 ) {
			/* identity matrix */
			lib_create_identity_matrix(mx);
		}
		else if ( direction[Z] <= -1.0 ) {
			lib_create_rotate_matrix(mx, Y_AXIS, PI);
		}
		else {
			SET_COORD3(z_axis, 0.0, 0.0, 1.0);
			CROSS(axis, z_axis, direction);
			lib_normalize_vector(axis);
			angle = acos((double)DOT_PRODUCT(z_axis, direction));
			lib_create_axis_rotate_matrix(mx, axis, angle);
		}
		
		/* scale down location of new spheres */
		scale = center[W] * (1.0 + direction[W]);
		
		for ( num_vert = 0 ; num_vert < 9 ; ++num_vert ) {
			/* only do progress for top-level objects of recursion */
			if (depth==size_factor-1)
				PLATFORM_PROGRESS(0, num_vert, 8);
			lib_transform_coord(child_pt, objset[num_vert], mx);
			child_pt[X] = child_pt[X] * scale + center[X];
			child_pt[Y] = child_pt[Y] * scale + center[Y];
			child_pt[Z] = child_pt[Z] * scale + center[Z];
			/* scale down radius */
			child_pt[W] = center[W] * direction[W];
			SUB3_COORD3( child_dir, child_pt, center);
			child_dir[X] /= scale;
			child_dir[Y] /= scale;
			child_dir[Z] /= scale;
			child_dir[W] = direction[W];
			output_object(depth, child_pt, child_dir);
		}
    }
}

/* Create the set of 9 vectors needed to generate the sphere set. */
/* Uses global 'objset' */
static void
create_objset()
{
    COORD4  axis, temp_pt, trio_dir[3];
    double  dist;
    MATRIX  mx;
    long    num_set, num_vert;
	
    dist = 1.0 / sqrt((double)2.0);
	
    SET_COORD4(trio_dir[0], dist, dist,   0.0, 0.0);
    SET_COORD4(trio_dir[1], dist,  0.0, -dist, 0.0);
    SET_COORD4(trio_dir[2],  0.0, dist, -dist, 0.0);
	
    SET_COORD3(axis, 1.0, -1.0, 0.0);
    lib_normalize_vector(axis);
    lib_create_axis_rotate_matrix(mx, axis,
		asin((double)(2.0/sqrt((double)6.0))));
	
    for (num_vert=0;num_vert<3;++num_vert) {
		lib_transform_coord(temp_pt, trio_dir[num_vert], mx);
		COPY_COORD4(trio_dir[num_vert], temp_pt);
    }
	
    for (num_set=0;num_set<3;++num_set) {
		lib_create_rotate_matrix(mx, Z_AXIS, num_set*2.0*PI/3.0);
		for (num_vert=0;num_vert<3;++num_vert) {
			lib_transform_coord(objset[num_set*3+num_vert],
				trio_dir[num_vert], mx);
		}
    }
}

int
main(argc,argv)
int argc ;
char *argv[] ;
{
    COORD3 back_color, obj_color;
    COORD3 backg[5], bvec;
    COORD3 from, at, up;
    COORD4 light;
    COORD4 center_pt, direction;
    double radius, lscale;
	
    PLATFORM_INIT(SPD_BALLS);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
	
    if ( lib_open( raytracer_format, "Balls" ) ) {
		return EXIT_FAIL;
    }
	
    /* set radius of sphere which would enclose entire object */
    radius = 1.0 ;
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3(back_color, 0.078, 0.361, 0.753);
    lib_output_background_color(back_color);
	
    /* output viewpoint */
    SET_COORD3(from, 2.1, 1.3, 1.7);
    SET_COORD3(at, 0.0, 0.0, 0.0);
    SET_COORD3(up, 0.0, 0.0, 1.0);
    lib_output_viewpoint(from, at, up, 45.0, 1.0, 0.01, 512, 512);
	
    /*
     * For raytracers that don't scale the light intensity,
     * we will do it for them
     */
	#define NUM_LIGHTS    3
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    /* output light sources */
    SET_COORD4(light, 4.0, 3.0, 2.0, lscale);
    lib_output_light(light);
    SET_COORD4(light, 1.0, -4.0, 4.0, lscale);
    lib_output_light(light);
    SET_COORD4(light, -3.0, 1.0, 5.0, lscale);
    lib_output_light(light);
	
    /* output floor polygon - beige */
    SET_COORD3(back_color, 1.0, 0.75, 0.33);
    lib_output_color(NULL, back_color, 0.2, 0.8, 0.0, 0.0, 0.0, 0.0, 1.0);
    SET_COORD3(bvec, 12.0 * radius, 12.0 * radius, -radius / 2.0);
    SET_COORD3(backg[0],  bvec[X],  bvec[Y], bvec[Z]);
    SET_COORD3(backg[1], -bvec[X],  bvec[Y], bvec[Z]);
    SET_COORD3(backg[2], -bvec[X], -bvec[Y], bvec[Z]);
    SET_COORD3(backg[3],  bvec[X], -bvec[Y], bvec[Z]);
    SET_COORD3(backg[4],  bvec[X],  bvec[Y], bvec[Z]);
    lib_output_polygon(4, backg);
	
    /* set up object color - mirrored */
    SET_COORD3(obj_color, 1.0, 0.9, 0.7);
    lib_output_color(NULL, obj_color, 0.0, 0.5, 0.5, 0.5, 37.0, 0.0, 1.0);
	
    /* create set of spawned points */
    create_objset();
	
    /* compute and output object */
    SET_COORD4(center_pt, 0.0, 0.0, 0.0, radius / 2.0);
    SET_COORD4(direction, 0.0, 0.0, 1.0, 1.0/3.0);
    output_object(size_factor, center_pt, direction);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
