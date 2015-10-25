/*
 * rings.c - Create objects with 6 pentagonal rings which connect the midpoints
 *	of the edges of a dodecahedron.  A pyramid of these objects is formed,
 *	which the viewer looks upon from the point.  A plane is placed behind
 *	the pyramid for shadows.  Three light sources.
 *
 * Author:  Eric Haines
 *
 * size_factor determines the number of objects output.
 *	Each object has 30 cylinders and 30 spheres.
 *	Total objects = SF*SF + (SF-1)*(SF-1) + ... + 1 plus 1 backdrop square.
 *	formula for # of spheres or cylinders = 5*SF*(SF+1)*(2*SF+1)
 *
 *	size_factor	# spheres	# cylinders	# squares
 *	     1		    30		      30	     1
 *	     2		   150		     150	     1
 *	     3		   420		     420	     1
 *
 *	     7		  4200		    4200	     1
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"

static int size_factor = 7 ;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

/* if spread out is > 1, succeeding layers spread out more */
#define	SPREAD_OUT		1

/* Create the set of 30 points needed to generate the rings */
static void
create_dodec( minor_radius, vertex )
double minor_radius ;
COORD3 vertex[30] ;
{
    int num_vertex, num_pentagon ;
    double scale, x_rotation, z_rotation ;
    COORD3 temp_vertex ;
    MATRIX x_matrix, z_matrix ;
	
    /* scale object to fit in a sphere of radius 1 */
    scale = 1.0 / ( 1.0 + minor_radius ) ;
	
    /*
	 * define one pentagon as on the XY plane, with points starting along +X
	 * and N fifths of the way around the Z axis.
	 */
    for ( num_vertex = 0 ; num_vertex < 5 ; ++num_vertex ) {
		vertex[num_vertex][X] = scale * cos((double)num_vertex * 2.0*PI/5.0 ) ;
		vertex[num_vertex][Y] = scale * sin((double)num_vertex * 2.0*PI/5.0 ) ;
		vertex[num_vertex][Z] = 0.0 ;
		vertex[num_vertex][W] = 1.0 ;
    }
	
    /*
	 * find the rotation angle (in radians) along the X axis:
	 * angle between two adjacent dodecahedron faces.
	 */
    x_rotation = 2.0 *
		acos( cos( (double)(PI/3.0) ) / sin( (double)(PI/5.0) ) ) ;
    lib_create_rotate_matrix( x_matrix, X_AXIS, x_rotation ) ;
	
    /*
	 * Find each of the other 5 pentagons:  rotate along the X axis,
	 * then rotate on the Z axis.
	 */
    for ( num_pentagon = 1 ; num_pentagon < 6 ; ++num_pentagon ) {
		/*
		 * find the rotation angle (in radians) along the Z axis:
		 * 1/10th plus N fifths of the way around * 2 * PI.
		 */
		z_rotation = PI*( 2.0*(double)(num_pentagon-1)+1.0 ) / 5.0 ;
		lib_create_rotate_matrix( z_matrix, Z_AXIS, z_rotation ) ;
		
		for ( num_vertex = 0 ; num_vertex < 5 ; ++num_vertex ) {
			
			lib_transform_point( temp_vertex
				, vertex[num_vertex]
				, x_matrix
				) ;
			
			lib_transform_point( vertex[5*num_pentagon+num_vertex]
				, temp_vertex
				, z_matrix
				) ;
		}
    }
}

int
main(argc,argv)
int argc;
char *argv[];
{
    int	prev_elem, num_elem, num_depth, num_objx, num_objz ;
    double radius, spread, y_diff, xz_diff ;
    COORD4 base_pt, apex_pt, light ;
    COORD3 from, at, up ;
    COORD3 wvec ;
    COORD3 back_color, ring_color[6] ;
    COORD3 wall[4], offset, dodec[30] ;
    double lscale;
	
    PLATFORM_INIT(SPD_RINGS);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Rings" ) ) {
		return EXIT_FAIL;
    }
	
    radius = 0.07412 ;	/* cone and sphere radius */
	
    /* calculate spread of objects */
    spread = 1 / sin( (double)( PI/8.0 ) ) ;
    if ( SPREAD_OUT <= spread ) {
		y_diff = spread / SPREAD_OUT ;
		xz_diff = 1.0 ;
    }
    else {
		y_diff = 1.0 ;
		xz_diff = SPREAD_OUT / spread ;
    }
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    /* note that the background color should never be seen */
    SET_COORD3( back_color, 0.078, 0.361, 0.753 ) ;
    lib_output_background_color( back_color ) ;
	
    /* output viewpoint */
    SET_COORD3( from, -1.0, -spread, 0.5 ) ;
    SET_COORD3( at, from[X], from[Y] + 1.0, from[Z] ) ;
    SET_COORD3( up, 0.0, 0.0, 1.0 ) ;
    lib_output_viewpoint( from, at, up, 45.0, 1.0, 1.0, 512, 512);
	
    /*
	 * For raytracers that don't scale the light intensity,
	 * we will do it for them
	 */
#define NUM_LIGHTS    3
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    /* output light source */
    SET_COORD4( light, 3.0, -spread, 3.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, -4.0, -spread, 1.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, 2.0, -spread, -4.0, lscale ) ;
    lib_output_light( light ) ;
	
    /* output wall polygon - white */
    SET_COORD3( back_color, 1.0, 1.0, 1.0 ) ;
    lib_output_color(NULL, back_color, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	
    /* just spans 45 degree view + 1% */
    wvec[Y] = y_diff * ( size_factor + 1 ) ;
    wvec[X] = wvec[Z] = 1.01 * ( wvec[Y] - from[Y] ) * tan( PI / 8.0 ) ;
    SET_COORD3( wall[0],  wvec[X]+from[X], wvec[Y],  wvec[Z]+from[Z] ) ;
    SET_COORD3( wall[1], -wvec[X]+from[X], wvec[Y],  wvec[Z]+from[Z] ) ;
    SET_COORD3( wall[2], -wvec[X]+from[X], wvec[Y], -wvec[Z]+from[Z] ) ;
    SET_COORD3( wall[3],  wvec[X]+from[X], wvec[Y], -wvec[Z]+from[Z] ) ;
    lib_output_polygon( 4, wall ) ;
	
    /* set up ring colors - RGB and complements */
    SET_COORD3( ring_color[0], 1.0, 0.0, 0.0 ) ;
    SET_COORD3( ring_color[1], 0.0, 1.0, 0.0 ) ;
    SET_COORD3( ring_color[2], 0.0, 0.0, 1.0 ) ;
    SET_COORD3( ring_color[3], 0.0, 1.0, 1.0 ) ;
    SET_COORD3( ring_color[4], 1.0, 0.0, 1.0 ) ;
    SET_COORD3( ring_color[5], 1.0, 1.0, 0.0 ) ;
	
    create_dodec( radius, dodec ) ;
    /* radius of osculating cylinders and spheres (no derivation given) */
    base_pt[W] = apex_pt[W] = radius ;
	
    for ( num_depth = 0 ; num_depth < size_factor ; ++num_depth ) {
		PLATFORM_PROGRESS(0, num_depth, size_factor-1);
		offset[Y] = y_diff * (double)(num_depth+1) ;
		for ( num_objz = 0 ; num_objz <= num_depth ; ++num_objz ) {
			offset[Z] = xz_diff * (double)(2*num_objz - num_depth) ;
			for ( num_objx = 0 ; num_objx <= num_depth ; ++num_objx ) {
				offset[X] = xz_diff * (double)(2*num_objx - num_depth) ;
				for ( num_elem = 0 ; num_elem < 30 ; ++num_elem ) {
					PLATFORM_MULTITASK();
					COPY_COORD3( base_pt, dodec[num_elem] ) ;
					ADD2_COORD3( base_pt, offset ) ;
					if ( num_elem%5 == 0 ) {
						prev_elem = num_elem + 4 ;
						/* new ring beginning - output color */
						lib_output_color(NULL, ring_color[num_elem/5],
							0.0, 0.5, 0.2, 0.3, 37.0, 0.0, 0.0);
					}
					else {
						prev_elem = num_elem - 1 ;
					}
					COPY_COORD3( apex_pt, dodec[prev_elem] ) ;
					ADD2_COORD3( apex_pt, offset ) ;
					
					lib_output_cylcone( base_pt, apex_pt, output_format ) ;
					lib_output_sphere( base_pt, output_format ) ;
				}
			}
		}
    }
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
