/*
 * teapot.c - Tessellate the famous teapot into triangular patches, and sit the
 *	object on top of a checkerboard which is meshed to the same degree.  In
 *	other words, the number of squares on the checkerboard is the same as
 *	the amount of meshing for each of the 32 patches on the teapot, with
 *	each mesh quadrilateral on the teapot further split into two triangles.
 *	Two light sources.
 *
 *	Note that the teapot should always be rendered as a double sided object
 *	(since for some patches both sides can be seen).  (See IEEE CG&A
 *	January 1987 for a history of the teapot.  Note that their vertex list
 *	has duplicates, e.g. 93 and 271, as well as unused vertices, e.g.
 *	205,206,216,223).  Also, note that the bottom (the last four patches)
 *	is not flat - blame Frank Crow, not me.
 *
 * Author:  Eric Haines
 *
 * size_factor determines the number of objects output.
 *	Total patches = 32*2*n*n - 8*n     [degenerates are deleted]
 *	Total squares = n*n
 *
 *	size_factor	# patches	# squares
 *	     1		    56		     1		[has more degenerates!]
 *	     2		   240		     4
 *	     3		   552		     9
 *
 *	    12		  9120		   144
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"

static int size_factor = 6 ;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

/* comment out the next line to generate all patches except the bottom,
 * i.e. the original Newell teapot
 */
#define BOTTOM

#ifdef	BOTTOM
#define	NUM_PATCHES	32
#else
#define	NUM_PATCHES	28
#endif


static	int	Patches[32][4][4] = {
/* rim */
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
3, 16, 17, 18, 7, 19, 20, 21, 11, 22, 23, 24, 15, 25, 26, 27,
18, 28, 29, 30, 21, 31, 32, 33, 24, 34, 35, 36, 27, 37, 38, 39,
30, 40, 41, 0, 33, 42, 43, 4, 36, 44, 45, 8, 39, 46, 47, 12,
/* body */
12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
15, 25, 26, 27, 51, 60, 61, 62, 55, 63, 64, 65, 59, 66, 67, 68,
27, 37, 38, 39, 62, 69, 70, 71, 65, 72, 73, 74, 68, 75, 76, 77,
39, 46, 47, 12, 71, 78, 79, 48, 74, 80, 81, 52, 77, 82, 83, 56,
56, 57, 58, 59, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
59, 66, 67, 68, 87, 96, 97, 98, 91, 99, 100, 101, 95, 102, 103, 104,
68, 75, 76, 77, 98, 105, 106, 107, 101, 108, 109, 110, 104, 111, 112, 113,
77, 82, 83, 56, 107, 114, 115, 84, 110, 116, 117, 88, 113, 118, 119, 92,
/* handle */
120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
123, 136, 137, 120, 127, 138, 139, 124, 131, 140, 141, 128, 135, 142, 143, 132,
132, 133, 134, 135, 144, 145, 146, 147, 148, 149, 150, 151, 68, 152, 153, 154,
135, 142, 143, 132, 147, 155, 156, 144, 151, 157, 158, 148, 154, 159, 160, 68,
/* spout */
161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
164, 177, 178, 161, 168, 179, 180, 165, 172, 181, 182, 169, 176, 183, 184, 173,
173, 174, 175, 176, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
176, 183, 184, 173, 188, 197, 198, 185, 192, 199, 200, 189, 196, 201, 202, 193,
/* lid */
203, 203, 203, 203, 204, 205, 206, 207, 208, 208, 208, 208, 209, 210, 211, 212,
203, 203, 203, 203, 207, 213, 214, 215, 208, 208, 208, 208, 212, 216, 217, 218,
203, 203, 203, 203, 215, 219, 220, 221, 208, 208, 208, 208, 218, 222, 223, 224,
203, 203, 203, 203, 221, 225, 226, 204, 208, 208, 208, 208, 224, 227, 228, 209,
209, 210, 211, 212, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
212, 216, 217, 218, 232, 241, 242, 243, 236, 244, 245, 246, 240, 247, 248, 249,
218, 222, 223, 224, 243, 250, 251, 252, 246, 253, 254, 255, 249, 256, 257, 258,
224, 227, 228, 209, 252, 259, 260, 229, 255, 261, 262, 233, 258, 263, 264, 237,
/* bottom */
265, 265, 265, 265, 266, 267, 268, 269, 270, 271, 272, 273, 92, 119, 118, 113,
265, 265, 265, 265, 269, 274, 275, 276, 273, 277, 278, 279, 113, 112, 111, 104,
265, 265, 265, 265, 276, 280, 281, 282, 279, 283, 284, 285, 104, 103, 102, 95,
265, 265, 265, 265, 282, 286, 287, 266, 285, 288, 289, 270, 95, 94, 93, 92
} ;

static	COORD3	Verts[290] = {
 1.4, 0, 2.4,
 1.4, -0.784, 2.4,
 0.784, -1.4, 2.4,
 0, -1.4, 2.4,
 1.3375, 0, 2.53125,
 1.3375, -0.749, 2.53125,
 0.749, -1.3375, 2.53125,
 0, -1.3375, 2.53125,
 1.4375, 0, 2.53125,
 1.4375, -0.805, 2.53125,
 0.805, -1.4375, 2.53125,
 0, -1.4375, 2.53125,
 1.5, 0, 2.4,
 1.5, -0.84, 2.4,
 0.84, -1.5, 2.4,
 0, -1.5, 2.4,
 -0.784, -1.4, 2.4,
 -1.4, -0.784, 2.4,
 -1.4, 0, 2.4,
 -0.749, -1.3375, 2.53125,
 -1.3375, -0.749, 2.53125,
 -1.3375, 0, 2.53125,
 -0.805, -1.4375, 2.53125,
 -1.4375, -0.805, 2.53125,
 -1.4375, 0, 2.53125,
 -0.84, -1.5, 2.4,
 -1.5, -0.84, 2.4,
 -1.5, 0, 2.4,
 -1.4, 0.784, 2.4,
 -0.784, 1.4, 2.4,
 0, 1.4, 2.4,
 -1.3375, 0.749, 2.53125,
 -0.749, 1.3375, 2.53125,
 0, 1.3375, 2.53125,
 -1.4375, 0.805, 2.53125,
 -0.805, 1.4375, 2.53125,
 0, 1.4375, 2.53125,
 -1.5, 0.84, 2.4,
 -0.84, 1.5, 2.4,
 0, 1.5, 2.4,
 0.784, 1.4, 2.4,
 1.4, 0.784, 2.4,
 0.749, 1.3375, 2.53125,
 1.3375, 0.749, 2.53125,
 0.805, 1.4375, 2.53125,
 1.4375, 0.805, 2.53125,
 0.84, 1.5, 2.4,
 1.5, 0.84, 2.4,
 1.75, 0, 1.875,
 1.75, -0.98, 1.875,
 0.98, -1.75, 1.875,
 0, -1.75, 1.875,
 2, 0, 1.35,
 2, -1.12, 1.35,
 1.12, -2, 1.35,
 0, -2, 1.35,
 2, 0, 0.9,
 2, -1.12, 0.9,
 1.12, -2, 0.9,
 0, -2, 0.9,
 -0.98, -1.75, 1.875,
 -1.75, -0.98, 1.875,
 -1.75, 0, 1.875,
 -1.12, -2, 1.35,
 -2, -1.12, 1.35,
 -2, 0, 1.35,
 -1.12, -2, 0.9,
 -2, -1.12, 0.9,
 -2, 0, 0.9,
 -1.75, 0.98, 1.875,
 -0.98, 1.75, 1.875,
 0, 1.75, 1.875,
 -2, 1.12, 1.35,
 -1.12, 2, 1.35,
 0, 2, 1.35,
 -2, 1.12, 0.9,
 -1.12, 2, 0.9,
 0, 2, 0.9,
 0.98, 1.75, 1.875,
 1.75, 0.98, 1.875,
 1.12, 2, 1.35,
 2, 1.12, 1.35,
 1.12, 2, 0.9,
 2, 1.12, 0.9,
 2, 0, 0.45,
 2, -1.12, 0.45,
 1.12, -2, 0.45,
 0, -2, 0.45,
 1.5, 0, 0.225,
 1.5, -0.84, 0.225,
 0.84, -1.5, 0.225,
 0, -1.5, 0.225,
 1.5, 0, 0.15,
 1.5, -0.84, 0.15,
 0.84, -1.5, 0.15,
 0, -1.5, 0.15,
 -1.12, -2, 0.45,
 -2, -1.12, 0.45,
 -2, 0, 0.45,
 -0.84, -1.5, 0.225,
 -1.5, -0.84, 0.225,
 -1.5, 0, 0.225,
 -0.84, -1.5, 0.15,
 -1.5, -0.84, 0.15,
 -1.5, 0, 0.15,
 -2, 1.12, 0.45,
 -1.12, 2, 0.45,
 0, 2, 0.45,
 -1.5, 0.84, 0.225,
 -0.84, 1.5, 0.225,
 0, 1.5, 0.225,
 -1.5, 0.84, 0.15,
 -0.84, 1.5, 0.15,
 0, 1.5, 0.15,
 1.12, 2, 0.45,
 2, 1.12, 0.45,
 0.84, 1.5, 0.225,
 1.5, 0.84, 0.225,
 0.84, 1.5, 0.15,
 1.5, 0.84, 0.15,
 -1.6, 0, 2.025,
 -1.6, -0.3, 2.025,
 -1.5, -0.3, 2.25,
 -1.5, 0, 2.25,
 -2.3, 0, 2.025,
 -2.3, -0.3, 2.025,
 -2.5, -0.3, 2.25,
 -2.5, 0, 2.25,
 -2.7, 0, 2.025,
 -2.7, -0.3, 2.025,
 -3, -0.3, 2.25,
 -3, 0, 2.25,
 -2.7, 0, 1.8,
 -2.7, -0.3, 1.8,
 -3, -0.3, 1.8,
 -3, 0, 1.8,
 -1.5, 0.3, 2.25,
 -1.6, 0.3, 2.025,
 -2.5, 0.3, 2.25,
 -2.3, 0.3, 2.025,
 -3, 0.3, 2.25,
 -2.7, 0.3, 2.025,
 -3, 0.3, 1.8,
 -2.7, 0.3, 1.8,
 -2.7, 0, 1.575,
 -2.7, -0.3, 1.575,
 -3, -0.3, 1.35,
 -3, 0, 1.35,
 -2.5, 0, 1.125,
 -2.5, -0.3, 1.125,
 -2.65, -0.3, 0.9375,
 -2.65, 0, 0.9375,
 -2, -0.3, 0.9,
 -1.9, -0.3, 0.6,
 -1.9, 0, 0.6,
 -3, 0.3, 1.35,
 -2.7, 0.3, 1.575,
 -2.65, 0.3, 0.9375,
 -2.5, 0.3, 1.125,
 -1.9, 0.3, 0.6,
 -2, 0.3, 0.9,
 1.7, 0, 1.425,
 1.7, -0.66, 1.425,
 1.7, -0.66, 0.6,
 1.7, 0, 0.6,
 2.6, 0, 1.425,
 2.6, -0.66, 1.425,
 3.1, -0.66, 0.825,
 3.1, 0, 0.825,
 2.3, 0, 2.1,
 2.3, -0.25, 2.1,
 2.4, -0.25, 2.025,
 2.4, 0, 2.025,
 2.7, 0, 2.4,
 2.7, -0.25, 2.4,
 3.3, -0.25, 2.4,
 3.3, 0, 2.4,
 1.7, 0.66, 0.6,
 1.7, 0.66, 1.425,
 3.1, 0.66, 0.825,
 2.6, 0.66, 1.425,
 2.4, 0.25, 2.025,
 2.3, 0.25, 2.1,
 3.3, 0.25, 2.4,
 2.7, 0.25, 2.4,
 2.8, 0, 2.475,
 2.8, -0.25, 2.475,
 3.525, -0.25, 2.49375,
 3.525, 0, 2.49375,
 2.9, 0, 2.475,
 2.9, -0.15, 2.475,
 3.45, -0.15, 2.5125,
 3.45, 0, 2.5125,
 2.8, 0, 2.4,
 2.8, -0.15, 2.4,
 3.2, -0.15, 2.4,
 3.2, 0, 2.4,
 3.525, 0.25, 2.49375,
 2.8, 0.25, 2.475,
 3.45, 0.15, 2.5125,
 2.9, 0.15, 2.475,
 3.2, 0.15, 2.4,
 2.8, 0.15, 2.4,
 0, 0, 3.15,
 0.8, 0, 3.15,
 0.8, -0.45, 3.15,
 0.45, -0.8, 3.15,
 0, -0.8, 3.15,
 0, 0, 2.85,
 0.2, 0, 2.7,
 0.2, -0.112, 2.7,
 0.112, -0.2, 2.7,
 0, -0.2, 2.7,
 -0.45, -0.8, 3.15,
 -0.8, -0.45, 3.15,
 -0.8, 0, 3.15,
 -0.112, -0.2, 2.7,
 -0.2, -0.112, 2.7,
 -0.2, 0, 2.7,
 -0.8, 0.45, 3.15,
 -0.45, 0.8, 3.15,
 0, 0.8, 3.15,
 -0.2, 0.112, 2.7,
 -0.112, 0.2, 2.7,
 0, 0.2, 2.7,
 0.45, 0.8, 3.15,
 0.8, 0.45, 3.15,
 0.112, 0.2, 2.7,
 0.2, 0.112, 2.7,
 0.4, 0, 2.55,
 0.4, -0.224, 2.55,
 0.224, -0.4, 2.55,
 0, -0.4, 2.55,
 1.3, 0, 2.55,
 1.3, -0.728, 2.55,
 0.728, -1.3, 2.55,
 0, -1.3, 2.55,
 1.3, 0, 2.4,
 1.3, -0.728, 2.4,
 0.728, -1.3, 2.4,
 0, -1.3, 2.4,
 -0.224, -0.4, 2.55,
 -0.4, -0.224, 2.55,
 -0.4, 0, 2.55,
 -0.728, -1.3, 2.55,
 -1.3, -0.728, 2.55,
 -1.3, 0, 2.55,
 -0.728, -1.3, 2.4,
 -1.3, -0.728, 2.4,
 -1.3, 0, 2.4,
 -0.4, 0.224, 2.55,
 -0.224, 0.4, 2.55,
 0, 0.4, 2.55,
 -1.3, 0.728, 2.55,
 -0.728, 1.3, 2.55,
 0, 1.3, 2.55,
 -1.3, 0.728, 2.4,
 -0.728, 1.3, 2.4,
 0, 1.3, 2.4,
 0.224, 0.4, 2.55,
 0.4, 0.224, 2.55,
 0.728, 1.3, 2.55,
 1.3, 0.728, 2.55,
 0.728, 1.3, 2.4,
 1.3, 0.728, 2.4,
 0, 0, 0,
 1.425, 0, 0,
 1.425, 0.798, 0,
 0.798, 1.425, 0,
 0, 1.425, 0,
 1.5, 0, 0.075,
 1.5, 0.84, 0.075,
 0.84, 1.5, 0.075,
 0, 1.5, 0.075,
 -0.798, 1.425, 0,
 -1.425, 0.798, 0,
 -1.425, 0, 0,
 -0.84, 1.5, 0.075,
 -1.5, 0.84, 0.075,
 -1.5, 0, 0.075,
 -1.425, -0.798, 0,
 -0.798, -1.425, 0,
 0, -1.425, 0,
 -1.5, -0.84, 0.075,
 -0.84, -1.5, 0.075,
 0, -1.5, 0.075,
 0.798, -1.425, 0,
 1.425, -0.798, 0,
 0.84, -1.5, 0.075,
 1.5, -0.84, 0.075
} ;


/* at the center of the lid's handle and at bottom are cusp points -
 * check if normal is (0 0 0), if so, check that polygon is not degenerate.
 * If degenerate, return FALSE, else set normal.
 */
static int
check_for_cusp( tot_vert, vert, norm )
int	tot_vert ;
COORD3	vert[] ;
COORD3	norm[] ;
{
	int	count, i, nv ;
	
	for ( count = 0, i = tot_vert ; i-- ; ) {
		/* check if vertex is at cusp */
		if ( IS_VAL_ALMOST_ZERO( vert[i][X], 0.0001 ) &&
			IS_VAL_ALMOST_ZERO( vert[i][Y], 0.0001 ) ) {
			count++ ;
			nv = i ;
		}
	}
	
	if ( count > 1 ) {
		/* degenerate */
		return( FALSE ) ;
	}
	if ( count == 1 ) {
		/* check if point is somewhere above the middle of the teapot */
		if ( vert[nv][Z] > 1.5 ) {
			/* cusp at lid */
			SET_COORD3( norm[nv], 0.0, 0.0, 1.0 ) ;
		} else {
			/* cusp at bottom */
			SET_COORD3( norm[nv], 0.0, 0.0, -1.0 ) ;
		}
	}
	return( TRUE ) ;
}

static void
points_from_basis( tot_vert, s, t, mgm, vert, norm )
int	tot_vert ;
double	s[] ;
double	t[] ;
MATRIX	mgm[3] ;
COORD3	vert[] ;
COORD3	norm[] ;
{
	int	i, num_vert, p ;
	double	sval, tval, dsval, dtval, sxyz, txyz ;
	COORD3	sdir, tdir ;
	COORD4	sp, tp, dsp, dtp, tcoord ;
	
	for ( num_vert = 0 ; num_vert < tot_vert ; num_vert++ ) {
		
		sxyz = s[num_vert] ;
		txyz = t[num_vert] ;
		
		/* get power vectors and their derivatives */
		for ( p = 4, sval = tval = 1.0 ; p-- ; ) {
			sp[p] = sval ;
			tp[p] = tval ;
			sval *= sxyz ;
			tval *= txyz ;
			
			if ( p == 3 ) {
				dsp[p] = dtp[p] = 0.0 ;
				dsval = dtval = 1.0 ;
			} else {
				dsp[p] = dsval * (double)(3-p) ;
				dtp[p] = dtval * (double)(3-p) ;
				dsval *= sxyz ;
				dtval *= txyz ;
			}
		}
		
		/* do for x,y,z */
		for ( i = 0 ; i < 3 ; i++ ) {
			/* multiply power vectors times matrix to get value */
			lib_transform_coord( tcoord, sp, mgm[i] ) ;
			vert[num_vert][i] = DOT4( tcoord, tp ) ;
			
			/* get s and t tangent vectors */
			lib_transform_coord( tcoord, dsp, mgm[i] ) ;
			sdir[i] = DOT4( tcoord, tp ) ;
			
			lib_transform_coord( tcoord, sp, mgm[i] ) ;
			tdir[i] = DOT4( tcoord, dtp ) ;
		}
		
		/* find normal */
		CROSS( norm[num_vert], tdir, sdir ) ;
		(void)lib_normalize_vector( norm[num_vert] ) ;
	}
}

/* Compute points on each spline surface of teapot by brute force.
 * Forward differencing would be faster, but this is compact & simple.
 */
static void
output_teapot()
{
/* bezier form */
static MATRIX ms = { -1.0,  3.0, -3.0,  1.0,
		      3.0, -6.0,  3.0,  0.0,
		     -3.0,  3.0,  0.0,  0.0,
		      1.0,  0.0,  0.0,  0.0 } ;
int	surf, i, r, c, sstep, tstep, num_tri, num_vert, num_tri_vert ;
double	s[3], t[3] ;
COORD3	vert[4], norm[4] ;
COORD3	obj_color ;
MATRIX	mst, g, mgm[3], tmtx ;

SET_COORD3( obj_color, 1.0, 0.5, 0.1 ) ;
lib_output_color(NULL, obj_color, 0.0, 0.75, 0.25, 0.25, 37.0, 0.0, 0.0 ) ;

lib_transpose_matrix( mst, ms ) ;

for ( surf = 0 ; surf < NUM_PATCHES ; surf++ ) {
	
	/* get M * G * M matrix for x,y,z */
	for ( i = 0 ; i < 3 ; i++ ) {
		/* get control patches */
		for ( r = 0 ; r < 4 ; r++ ) {
			for ( c = 0 ; c < 4 ; c++ ) {
				g[r][c] = Verts[Patches[surf][r][c]][i] ;
			}
		}
		
		lib_matrix_multiply( tmtx, ms, g ) ;
		lib_matrix_multiply( mgm[i], tmtx, mst ) ;
	}
	
	/* step along, get points, and output */
	for ( sstep = 0 ; sstep < size_factor ; sstep++ ) {
		PLATFORM_PROGRESS(0, surf*size_factor+sstep, NUM_PATCHES*size_factor-1);
		for ( tstep = 0 ; tstep < size_factor ; tstep++ ) {
			for ( num_tri = 0 ; num_tri < 2 ; num_tri++ ) {
				for ( num_vert = 0 ; num_vert < 3 ; num_vert++ ) {
					num_tri_vert = ( num_vert + num_tri * 2 ) % 4 ;
					/* trickiness: add 1 to sstep if 1 or 2 */
					s[num_vert] = (double)(sstep + (num_tri_vert/2 ? 1:0) )
						/ (double)size_factor ;
					/* trickiness: add 1 to tstep if 2 or 3 */
					t[num_vert] = (double)(tstep + (num_tri_vert%3 ? 1:0) )
						/ (double)size_factor ;
				}
				points_from_basis( 3, s, t, mgm, vert, norm ) ;
				/* don't output degenerate polygons */
				if ( check_for_cusp( 3, vert, norm ) ) {
					lib_output_polypatch( 3, vert, norm ) ;
					PLATFORM_MULTITASK();
				}
			}
		}
	}
}
}

static void
loc_to_square( sstep, tstep, vert )
int	sstep ;
int	tstep ;
COORD3	vert[4] ;
{
	int	i ;
	
    for ( i = 0 ; i < 4 ; i++ ) {
		/* vertex 0 & 3 are x.low, 1 & 2 are x.high */
		vert[i][X] = 4.0 * ( 2.0 * (double)(sstep + (i%3 ? 1 : 0) ) /
			(double)size_factor - 1.0 ) ;
		
		/* vertex 0 & 1 are y.low, 2 & 3 are y.high */
		vert[i][Y] = 4.0 * ( 2.0 * (double)(tstep + (i/2 ? 1 : 0) ) /
			(double)size_factor - 1.0 ) ;
		
		vert[i][Z] = 0.0 ;
    }
}

static void
output_checkerboard()
{
	int	sstep, tstep ;
	COORD3	vert[4] ;
	COORD3	obj_color ;
	
    SET_COORD3( obj_color, 1.0, 1.0, 1.0 ) ;
    lib_output_color(NULL, obj_color, 0.0, 0.5, 0.5, 0.5, 30.0, 0.0, 0.0 ) ;
    for ( sstep = 0 ; sstep < size_factor ; sstep++ ) {
		for ( tstep = 0 ; tstep < size_factor ; tstep++ ) {
			if ( ( sstep + tstep ) % 2 ) {
				loc_to_square( sstep, tstep, vert ) ;
				lib_output_polygon( 4, vert ) ;
			}
		}
    }
	
    SET_COORD3( obj_color, 0.5, 0.5, 0.5 ) ;
    lib_output_color(NULL, obj_color, 0.0, 0.5, 0.5, 0.5, 30.0, 0.0, 0.0 ) ;
    for ( sstep = 0 ; sstep < size_factor ; sstep++ ) {
		for ( tstep = 0 ; tstep < size_factor ; tstep++ ) {
			if ( !(( sstep + tstep ) % 2) ) {
				loc_to_square( sstep, tstep, vert ) ;
				lib_output_polygon( 4, vert ) ;
			}
		}
    }
}


int
main(argc,argv)
int argc;
char *argv[];
{
    double lscale;
    COORD3 back_color;
    COORD3 from, at, up;
    COORD4 light;
	
    PLATFORM_INIT(SPD_TEAPOT);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Teapot" ) ) {
		return EXIT_FAIL;
    }
	
	/*    lib_set_polygonalization(3, 3); */
	
    if ( size_factor == 1 ) {
		fprintf(stderr,
			"warning: a size of 1 is not supported - use at your own risk\n" ) ;
    }
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3( back_color, 0.078, 0.361, 0.753 ) ;
    lib_output_background_color( back_color ) ;
	
    /* output viewpoint */
    SET_COORD3( from, 4.86, 7.2, 5.4 ) ;
    SET_COORD3( at, 0.0, 0.0, 0.0 ) ;
    SET_COORD3( up, 0.0, 0.0, 1.0 ) ;
    lib_output_viewpoint( from, at, up, 45.0, 1.0, 1.0, 512, 512 ) ;
	
    /*
	 * For raytracers that don't scale the light intensity,
	 * we will do it for them
	 */
	#define NUM_LIGHTS    2
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    /* output light sources */
    SET_COORD4( light, -3.1, 9.8, 12.1, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, 11.3, 5.1, 8.8, lscale ) ;
    lib_output_light( light ) ;
	
    output_checkerboard() ;
    output_teapot() ;
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
