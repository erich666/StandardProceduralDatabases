/*
 * gears.c - Create a set of gears.  Each gear face has 144 vertices, and
 *   contains concavities.  Note that the first 3 vertices of all polygons
 *   define the two edges of a convex section of the polygon.  Background
 *   square ground is reflective.  Some gears are clipped.
 *   Five light sources.
 *
 * Version:  2.2 (11/17/87)
 * Author:  Eric Haines
 *
 * size_factor determines the number of polygons output.
 *     Total gears = SF**3:  concave polygons = 2 * SF**3
 *     rectangles = 4*TEETH * SF**3
 *
 * size_factor   # gears # gear faces    # rectangles
 *       1          1          2           144
 *       2          8         16          1152
 *       3         27         54          3888
 *       4         64        128          9216
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"

/* These may be read from the command line */
static int size_factor = 4;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

/* define number of teeth on a gear - must be a multiple of 4 */
#define	TEETH			36
/* define ratio of radius taken up by teeth and the gear thickness */
/* the outer radius is made slightly smaller that the full radius to create
 * a finite separation between intermeshing gears.  This gets rid of the bug
 * of having two surfaces occupy exactly the same space.  Note that if these
 * are changed, the gears may interpenetrate.
 */
#define	OUTER_EDGE_RATIO	0.995
#define	INNER_EDGE_RATIO	0.9
#define	EDGE_DIFF		( 1.0 - INNER_EDGE_RATIO )

/* ratio of width of gear to thickness */
#define	DEPTH_RATIO		0.1

/* Create gear tooth */
static void
create_tooth(gear_angle, tooth_angle, center, outer_pt, inner_pt, edge_pts)
double gear_angle, tooth_angle;
COORD3 center, outer_pt, inner_pt;
COORD3 edge_pts[];
{
    MATRIX mx;
	
    lib_create_rotate_matrix(mx, Z_AXIS, gear_angle - 0.19 * tooth_angle);
    lib_transform_vector(edge_pts[0], outer_pt, mx);
    ADD2_COORD3( edge_pts[0], center);
    lib_create_rotate_matrix(mx, Z_AXIS, gear_angle + 0.19 * tooth_angle);
    lib_transform_vector(edge_pts[1], outer_pt, mx);
    ADD2_COORD3(edge_pts[1], center);
    lib_create_rotate_matrix(mx, Z_AXIS, gear_angle + 0.3 * tooth_angle);
    lib_transform_vector(edge_pts[2], inner_pt, mx);
    ADD2_COORD3(edge_pts[2], center);
    lib_create_rotate_matrix(mx, Z_AXIS, gear_angle + 0.7 * tooth_angle);
    lib_transform_vector(edge_pts[3], inner_pt, mx);
    ADD2_COORD3(edge_pts[3], center);
}

/* Create gear */
static void
create_gear(center, offset_angle, outer_radius, inner_radius, thickness)
COORD3 center;
double offset_angle, outer_radius, inner_radius, thickness;
{
    int next_side, num_side, num_teeth ;
    double gear_angle, tooth_angle ;
    COORD3 side_pts[4], gear_pts[4*TEETH], outer_pt, inner_pt ;
	
    PLATFORM_MULTITASK();
	
    SET_COORD3( outer_pt, outer_radius, 0.0, 0.0 ) ;
    SET_COORD3( inner_pt, inner_radius, 0.0, 0.0 ) ;
	
    tooth_angle = 2.0 * PI / (double)TEETH ;
	
    /* output gear top */
    for (num_teeth=0;num_teeth<TEETH;num_teeth++) {
		gear_angle = offset_angle +
			2.0 * PI * (double)num_teeth / (double)TEETH;
		create_tooth(gear_angle, tooth_angle, center, outer_pt, inner_pt,
			(COORD3 *)&gear_pts[num_teeth*4]);
    }
    lib_output_polygon(4*TEETH, gear_pts);
	
    /* output teeth */
    for (num_side=0;num_side<4*TEETH;num_side++) {
		next_side = (num_side + 1) % ( 4 * TEETH);
		COPY_COORD3(side_pts[0], gear_pts[num_side]);
		COPY_COORD3(side_pts[1], gear_pts[num_side]);
		side_pts[1][Z] -= thickness;
		COPY_COORD3(side_pts[2], gear_pts[next_side]);
		side_pts[2][Z] -= thickness;
		COPY_COORD3(side_pts[3], gear_pts[next_side]);
		lib_output_polygon(4, side_pts);
    }
	
    /* output gear bottom */
    outer_pt[Z] = inner_pt[Z] = -thickness;
    for (num_teeth=0;num_teeth<TEETH;num_teeth++) {
		gear_angle = offset_angle -
			2.0 * PI * (double)num_teeth / (double)TEETH;
		create_tooth(gear_angle, -tooth_angle, center, outer_pt, inner_pt,
			(COORD3 *)&gear_pts[num_teeth*4]);
    }
	
    lib_output_polygon(4*TEETH, gear_pts);
}

int
main(argc,argv)
int argc ;
char *argv[] ;
{
    COORD4 light;
    COORD3 back_color, gear_color;
    COORD3 center, ground[4], offset, zero_pt;
    COORD3 from, at, up;
    double angle, color_scale, outer_radius, thickness, lscale;
    int     ix, iy, iz ;
	
    PLATFORM_INIT(SPD_GEARS);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Gears" ) ) {
		return EXIT_FAIL;
    }
	/*      lib_set_polygonalization(3, 3); */
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3(back_color, 0.078, 0.361, 0.753);
    lib_output_background_color(back_color);
	
    /* output viewpoint */
    SET_COORD3(from, -1.1, -2.1, 2.6);
    SET_COORD3(at, 0.0, 0.0, 0.0);
    SET_COORD3(up, 0.0, 0.0, 1.0);
    lib_output_viewpoint(from, at, up, 45.0, 1.0, 1.0, 512, 512);
	
    /* output light sources */
    /*
	 * For raytracers that don't scale the light intensity,
	 * we will do it for them
	 */
#define NUM_LIGHTS    5
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    SET_COORD4(light, 2.0, 4.0, 4.0, lscale);
    lib_output_light(light);
    SET_COORD4(light, -2.0, 4.0, 3.0, lscale);
    lib_output_light(light);
    SET_COORD4(light, 2.0, -2.5, 2.5, lscale);
    lib_output_light(light);
    SET_COORD4(light, -1.0, -4.0, 2.0, lscale);
    lib_output_light(light);
	
    /* just behind the eye */
    SET_COORD4(light, -1.111, -2.121, 2.626, lscale);
    lib_output_light(light);
	
    /* output ground polygon - off white */
    SET_COORD3(back_color, 1.0, 0.85, 0.7);
    lib_output_color(NULL, back_color, 0.0, 0.3, 0.6, 0.3, 37.0, 0.0, 0.0);
    SET_COORD3(ground[0],  2.0,  2.0, 0.0);
    SET_COORD3(ground[1], -2.0,  2.0, 0.0);
    SET_COORD3(ground[2], -2.0, -2.0, 0.0);
    SET_COORD3(ground[3],  2.0, -2.0, 0.0);
    lib_output_polygon(4, ground);
	
    outer_radius = 1.0/
		((double)size_factor-(double)(size_factor-1)*EDGE_DIFF/2.0);
	
    /* calculate first gear center */
    zero_pt[X] = zero_pt[Y] = -1.0 + outer_radius;
    zero_pt[Z] = 1.0;
	
    /* calculate offset */
    offset[X] = offset[Y] = outer_radius * (2.0 - EDGE_DIFF);
    offset[Z] = -1.0 / (double)size_factor;
	
    /* create gears */
    for (iz=0;iz<size_factor;iz++) {
		center[Z] = zero_pt[Z] + (double)iz * offset[Z] ;
		for (iy=0;iy<size_factor;iy++) {
			PLATFORM_PROGRESS(0, iz*size_factor+iy, size_factor*size_factor-1);
			center[Y] = zero_pt[Y] + (double)iy * offset[Y] ;
			for (ix=0;ix<size_factor;ix++) {
				center[X] = zero_pt[X] + (double)ix * offset[X] ;
				/* output pseudo-random gear color */
				SET_COORD3(gear_color,
					0.01 + FRACTION((double)(ix*3+iy*2+iz+1)*5.0/7.0),
					0.01 + FRACTION((double)(iy*3+iz*2+ix+1)*3.0/7.0),
					0.01 + FRACTION((double)(iz*3+ix*2+iy+1)*2.0/7.0));
				color_scale = MAX(gear_color[0],gear_color[1]);
				color_scale = MAX(color_scale,gear_color[2]);
				gear_color[X] /= color_scale;
				gear_color[Y] /= color_scale;
				gear_color[Z] /= color_scale;
				if ((ix*4 + iy*2 + iz ) % 5 == 0)
					lib_output_color(NULL, gear_color, 0.0, 0.2, 0.0, 0.0, 0.0,
					0.8, 1.1);
				else
					lib_output_color(NULL, gear_color, 0.0, 1.0, 0.0, 0.0, 0.0,
					0.0, 0.0);
				
				/* output gear */
				angle = PI * (double)((ix+iy+iz) % 2) / (double)(TEETH);
				thickness = MIN(DEPTH_RATIO, 1.0 / (2.0 * (double)size_factor));
				create_gear(center, angle, OUTER_EDGE_RATIO * outer_radius,
					(1.0 - EDGE_DIFF) * outer_radius, thickness);
			}
		}
    }
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
