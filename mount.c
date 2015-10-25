/*
 * mount.c - creates a fractal mountain, using Carpenter's method with a
 *      different extension to square grids.  A pyramid of 4 glass spheres
 *      is added in front of the mountain.  One light source.
 *
 *      NOTE: the hashing function used to generate the database originally is
 *      faulty.  The function causes repetition to occur within the fractal
 *      mountain (obviously not very fractal behavior!).  A new hashing
 *      function is included immediately after the old one:  merely define
 *      NEW_HASH if you want to use a better hashing function.  To perform ray
 *      tracing comparison tests you should still use the old, faulty database
 *      (it may have repetition, but it's still a good test image).
 *
 * Author:  Eric Haines
 *
 * size_factor determines the number of objects output.
 *      Total triangular polygons = 2 * (4**size_factor)
 *
 *      size_factor     # triangles     # spheres
 *           1               8               4
 *           2              32               4
 *           3             128               4
 *
 *           6            8192               4
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>     /* atoi */
#include "def.h"
#include "drv.h"        /* display_close() */
#include "lib.h"

/* Determine which raytracer we will use */
static int raytracer_format = OUTPUT_RT_DEFAULT;
/* output format determines if polygons or true surfaces are used */
static int output_format = OUTPUT_CURVES;


/* Complexity of the image */
static int size_factor = 6;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

/* to use a much better hashing function, uncomment this next line */
/* #define JENKINS_HASH */

/* fractal dimension - affects variance of z.  Between 2 and 3 */
#define FRACTAL_DIMENSION       2.2
/* change MOUNTAIN_NO to get a different mountain */
#define MOUNTAIN_NO             21

/* lower left corner and width of mountain definitions */
#define X_CORNER        -1.0
#define Y_CORNER        -1.0
#define WIDTH            2.0

#ifndef JENKINS_HASH

/* Hashing function to get a seed for the random number generator. */
/* This is the old, buggy hashing function - use it if you wish to
 * obtain the same image as in the November 1987 IEEE CG&A article. */
#define hash_rand(A,B,C)        ( ( (((unsigned long)(A))<<(23-(C))) +  \
				    (((unsigned long)(B))<<(15-(C)))    \
				  + (((unsigned long)(A))<<(7-(C))) ) & 0xffff)
#else

/*
 * Bob Jenkins invented this great, reasonably fast, very nice
 * has function.  Check out his web page:
 * http://ourworld.compuserve.com/homepages/bob_jenkins/blockcip.htm
 */
#define mix(a,b,c) \
{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8);  \
	c -= a; c -= b; c ^= (b>>13); \
	a -= b; a -= c; a ^= (c>>12); \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5);  \
	a -= b; a -= c; a ^= (c>>3);  \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
}

int
hash_rand(int A, int B, int C)
{
    mix(A,B,C) ;
    return C & 0xffff ;
}

#endif

static  double  Roughness ;

/* create a pyramid of crystal spheres */
static void
create_spheres(center)
COORD4 center;
{
	int i;
	double angle;
	COORD3 axis, pt, new_pt;
	COORD4 sphere;
	MATRIX mx;
	
	SET_COORD3(axis, 1.0, 1.0, 0.0);
	(void)lib_normalize_vector(axis);
	angle = acos((double)(-1.0/3.0));
	
	/* set center of pyramid */
	SET_COORD3(pt, 0.0, 0.0, center[W] * sqrt((double)(3.0/2.0)));
	
	COPY_COORD4(sphere, center);
	ADD2_COORD3(sphere, pt);
	lib_output_sphere(sphere, output_format);
	
	lib_create_axis_rotate_matrix(mx, axis, angle);
	lib_transform_vector(new_pt, pt, mx);
	
	for (i = 0; i < 3; i++) {
		lib_create_rotate_matrix(mx, Z_AXIS, (double)i * 2.0 * PI / 3.0);
		lib_transform_vector(sphere, new_pt, mx);
		ADD2_COORD3(sphere, center);
		lib_output_sphere(sphere, output_format);
	}
}

/*
 * Build mountain section.  If at width > 1, split quadrilateral into four
 * parts.  Else if at width == 1, output quadrilateral as two triangles.
 */
static void
grow_mountain(fnum_pts, width, ll_x, ll_y, ll_fz, lr_fz, ur_fz, ul_fz)
double fnum_pts;
int width;
int ll_x;
int ll_y ;
double ll_fz;
double lr_fz;
double ur_fz;
double ul_fz;
{
    long iz;
    int half_width, num_tri, num_tri_vert, num_vert;
    double l_fx, r_fx, l_fy, u_fy;
    double lower_fz, right_fz, upper_fz, left_fz, middle_fz;
    double rise_height, hside_length;
    COORD3 tri_vert[3];
	
    if ( width == 1 ) {
		/* calculate x and y coordinates of corners */
		l_fx = X_CORNER + (double)ll_x * WIDTH / fnum_pts;
		r_fx = X_CORNER + (double)(ll_x+1) * WIDTH / fnum_pts;
		l_fy = Y_CORNER + (double)ll_y * WIDTH / fnum_pts;
		u_fy = Y_CORNER + (double)(ll_y+1) * WIDTH / fnum_pts;
		
		/* output two triangles for section */
		for (num_tri = 0; num_tri < 2; num_tri++) {
			for (num_vert = 0; num_vert < 3; num_vert++) {
				num_tri_vert = (num_vert + num_tri * 2) % 4;
				switch (num_tri_vert) {
				case 0:
					SET_COORD3(tri_vert[num_vert], l_fx, l_fy, ll_fz);
					break;
				case 1:
					SET_COORD3(tri_vert[num_vert], r_fx, l_fy, lr_fz);
					break;
				case 2:
					SET_COORD3(tri_vert[num_vert], r_fx, u_fy, ur_fz);
					break;
				case 3:
					SET_COORD3(tri_vert[num_vert], l_fx, u_fy, ul_fz);
					break;
				}
			}
			lib_output_polygon(3, tri_vert);
		}
    } else {
		/* subdivide edges and move in z direction */
		half_width = width>>1;
		hside_length = (double)half_width * WIDTH / fnum_pts;
		rise_height = hside_length * Roughness;
		
		/* for each midpoint, find z */
		iz = MOUNTAIN_NO + hash_rand(ll_x + half_width, ll_y, size_factor);
		lower_fz = (ll_fz + lr_fz) / 2.0 + rise_height * lib_gauss_rand(iz);
		iz = MOUNTAIN_NO + hash_rand(ll_x + width, ll_y + half_width,
			size_factor);
		right_fz = ( lr_fz + ur_fz ) / 2.0 + rise_height * lib_gauss_rand(iz);
		iz = MOUNTAIN_NO + hash_rand(ll_x + half_width, ll_y + width,
			size_factor);
		upper_fz = ( ur_fz + ul_fz ) / 2.0 + rise_height * lib_gauss_rand(iz);
		iz = MOUNTAIN_NO + hash_rand( ll_x, ll_y + half_width, size_factor);
		left_fz = ( ul_fz + ll_fz ) / 2.0 + rise_height * lib_gauss_rand(iz);
		iz = MOUNTAIN_NO + hash_rand(ll_x + half_width, ll_y + half_width,
			size_factor);
		middle_fz = ( ll_fz + lr_fz + ur_fz + ul_fz ) / 4.0 +
			1.4142136 * rise_height * lib_gauss_rand(iz);
		
		/* check subsections for subdivision or output */
		PLATFORM_MULTITASK();
		if (width == 1<<size_factor)
			PLATFORM_PROGRESS(0, 0, 3);
		grow_mountain(fnum_pts, half_width, ll_x, ll_y,
			ll_fz, lower_fz, middle_fz, left_fz);
		if (width == 1<<size_factor)
			PLATFORM_PROGRESS(0, 1, 3);
		grow_mountain(fnum_pts, half_width, ll_x+half_width, ll_y,
			lower_fz, lr_fz, right_fz, middle_fz);
		if (width == 1<<size_factor)
			PLATFORM_PROGRESS(0, 2, 3);
		grow_mountain(fnum_pts, half_width, ll_x+half_width, ll_y+half_width,
			middle_fz, right_fz, ur_fz, upper_fz);
		if (width == 1<<size_factor)
			PLATFORM_PROGRESS(0, 3, 3);
		grow_mountain(fnum_pts, half_width, ll_x, ll_y+half_width,
			left_fz, middle_fz, upper_fz, ul_fz);
    }
}

int
main(argc,argv)
int argc;
char *argv[];
{
    int num_pts;
    double ratio;
    double lscale;
    COORD3 back_color, obj_color;
    COORD3 from, at, up;
    COORD4 light, center;
	
    PLATFORM_INIT(SPD_MOUNT);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format )) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Mount" ) ) {
		return EXIT_FAIL;
    }
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3(back_color, 0.078, 0.361, 0.753);
    lib_output_background_color(back_color);
	
    /* output viewpoint */
    SET_COORD3(from, -1.6, 1.6, 1.7);
    SET_COORD3(at, 0.0, 0.0, 0.0);
    SET_COORD3(up, 0.0, 0.0, 1.0);
    lib_output_viewpoint(from, at, up, 45.0, 1.0, 0.01, 512, 512);
	
    /* output light sources */
    /*
     * For raytracers that don't scale the light intensity,
     * we will do it for them
     */
	#define NUM_LIGHTS    1
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    SET_COORD4(light, -100.0, -100.0, 100.0, lscale);
    lib_output_light(light);
	
    /* set up crystal sphere color - clear white */
    SET_COORD3(obj_color, 1.0, 1.0, 1.0);
    lib_output_color(NULL, obj_color, 0.0, 0.1, 0.1, 0.4, 6.7, 0.9, 1.5);
	
    /* output crystal spheres */
    SET_COORD4(center, -0.8, 0.8, 1.00, 0.17);
    create_spheres(center);
	
    /* set up mountain color - grey */
    SET_COORD3(obj_color, 0.5, 0.45, 0.35);
    lib_output_color(NULL, obj_color, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	
    /* grow mountain */
    num_pts = 1<<size_factor;
    ratio = 2.0 / exp((double)(log((double)2.0) / (FRACTAL_DIMENSION-1.0)));
    Roughness = sqrt((double)(SQR(ratio) - 1.0));
    grow_mountain((double)num_pts, num_pts, 0, 0, 0.0, 0.0, 0.0, 0.0);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
