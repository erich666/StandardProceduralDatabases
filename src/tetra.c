/*
 * tetra.c - Create a tetrahedral pyramid.  This environment is based on the
 *      scene used by Glassner ("Space Subdivision for Fast Ray Tracing," IEEE
 *      CG&A, October 1984) and Kay & Kajiya ("Ray Tracing Complex Scenes,"
 *      SIGGRAPH '86 Proceedings) for testing their ray tracers.
 *      One light source.
 *
 * Author:  Eric Haines
 *
 * Note:  the view and light positions are the same (after transformation to
 *      a different set of world coordinates) as used by Kay & Kajiya,
 *      courtesy of Tim Kay.  For some reason, the number of shadow rays
 *      generated is different (Kay gets 34K, I get 46K).  One light source.
 *
 * size_factor determines the number of polygons output.
 *      Total triangular polygons = 4**SF
 *
 *      size_factor     # triangles
 *           1               4
 *           2              16
 *           3              64
 *
 *           6            4096
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>     /* atoi */
#include "def.h"
#include "drv.h"        /* display_close() */
#include "lib.h"

static  int     size_factor = 6 ;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */


/* Create tetrahedrons recursively */
static void
create_tetra( depth, center )
int depth ;
COORD4 center ;
{
    int num_face, num_vert, swap, vert_ord[3] ;
    int x_dir, y_dir, z_dir ;
    COORD3 face_pt[3], obj_pt[4] ;
    COORD4 sub_center ;
	
    if ( depth <= 1 ) {
		/* Output tetrahedron */
		
		PLATFORM_MULTITASK();
		
		/* find opposite corners of a cube which form a tetrahedron */
		for ( num_vert = 0, x_dir = -1 ; x_dir <= 1 ; x_dir += 2 ) {
			for ( y_dir = -1 ; y_dir <= 1 ; y_dir += 2 ) {
				for ( z_dir = -1 ; z_dir <= 1 ; z_dir += 2 ) {
					if ( x_dir*y_dir*z_dir == 1 ) {
						obj_pt[num_vert][X] =
							center[X] + (double)x_dir * center[W] ;
						obj_pt[num_vert][Y] =
							center[Y] + (double)y_dir * center[W] ;
						obj_pt[num_vert][Z] =
							center[Z] + (double)z_dir * center[W] ;
						++num_vert ;
					}
				}
			}
		}
		
		/* find faces and output */
		for ( num_face = 0 ; num_face < 4 ; ++num_face ) {
			/* output order:
			 *   face 0:  points 0 1 2
			 *   face 1:  points 3 2 1
			 *   face 2:  points 2 3 0
			 *   face 3:  points 1 0 3
			 */
			for ( num_vert = 0 ; num_vert < 3 ; ++num_vert ) {
				vert_ord[num_vert] = (num_face + num_vert) % 4 ;
			}
			if ( num_face%2 == 1 ) {
				swap = vert_ord[0] ;
				vert_ord[0] = vert_ord[2] ;
				vert_ord[2] = swap ;
			}
			
			for ( num_vert = 0 ; num_vert < 3 ; ++num_vert ) {
				COPY_COORD3( face_pt[num_vert], obj_pt[vert_ord[num_vert]] ) ;
			}
			lib_output_polygon( 3, face_pt ) ;
		}
    }
	
    else {
		/* Create sub-tetrahedra */
		
		/* find opposite corners of a cube to form sub-tetrahedra */
		for ( x_dir = -1 ; x_dir <= 1 ; x_dir += 2 ) {
			for ( y_dir = -1 ; y_dir <= 1 ; y_dir += 2 ) {
				if (depth==size_factor)
					PLATFORM_PROGRESS(0, (x_dir+1)*2+(y_dir+1), 7);
				for ( z_dir = -1 ; z_dir <= 1 ; z_dir += 2 ) {
					if ( x_dir*y_dir*z_dir == 1 ) {
						sub_center[X] =
							center[X] + (double)x_dir * center[W] / 2.0 ;
						sub_center[Y] =
							center[Y] + (double)y_dir * center[W] / 2.0 ;
						sub_center[Z] =
							center[Z] + (double)z_dir * center[W] / 2.0 ;
						sub_center[W] = center[W] / 2.0 ;
						
						create_tetra( depth-1, sub_center ) ;
					}
				}
			}
		}
    }
}

int
main(argc,argv)
int argc ;
char *argv[] ;
{
    double  lscale;
    COORD3  back_color, tetra_color ;
    COORD3  from, at, up ;
    COORD4  center_pt, light ;
	
    PLATFORM_INIT(SPD_TETRA);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Tetra" ) ) {
		return EXIT_FAIL;
    }
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3( back_color, 0.078, 0.361, 0.753 ) ;
    lib_output_background_color( back_color ) ;
	
    /* output viewpoint */
    SET_COORD3( from, 1.022846, -3.177154, -2.174512 ) ;
    SET_COORD3( at, -0.004103, -0.004103, 0.216539 ) ;
    SET_COORD3( up, -0.816497, -0.816497, 0.816497 ) ;
    lib_output_viewpoint( from, at, up, 45.0, 1.0, 1.0, 512, 512);

    /*
     * For raytracers that don't scale the light intensity,
     * we will do it for them
     */
    #define NUM_LIGHTS    1
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    /* output light source */
    SET_COORD4( light, 2.0, -18.0, -5.0, lscale) ;
    lib_output_light( light ) ;
	
    /* output tetrahedron color - red */
    SET_COORD3( tetra_color, 1.0, 0.2, 0.2 ) ;
    lib_output_color(NULL, tetra_color, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	
    /* compute and output tetrahedral object */
    SET_COORD4( center_pt, 0.0, 0.0, 0.0, 1.0 ) ;
    create_tetra( size_factor, center_pt ) ;
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
