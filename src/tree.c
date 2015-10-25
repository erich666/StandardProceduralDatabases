/*
 * tree.c - Creates a tree using Aono & Kunii's generation method.
 *      (See IEEE CG&A May 1984).  A square polygon is placed beneath the
 *      tree to act as a field.  Seven light sources.
 *
 * Author:  Eric Haines
 *
 * size_factor determines the number of objects output.
 *      Total objects = 2**(SF+1)-1 cones and spheres + 1 square polygon.
 *
 *      size_factor     # spheres          # cones      # squares
 *           1               3                 3             1
 *           2               7                 7             1
 *           3              15                15             1
 *
 *          11            4095              4095             1
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>     /* atoi */
#include "def.h"
#include "drv.h"        /* display_close() */
#include "lib.h"

/* These may be read from the command line */
static int size_factor       = 11;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */

/* the following affect the shape of the tree */
#define BR_ANGLE_0              40.0
#define BR_ANGLE_1              25.0
#define BR_CONTR_0              0.65
#define BR_CONTR_1              0.70
#define BR_DIAMETER             0.67
#define DIV_ANGLE               140.0
#define WIDTH_HEIGHTH_RATIO     0.15

static  MATRIX  Rst_mx[2] ;

/* grow tree branches recursively */
static void
grow_tree(cur_mx, scale, depth)
MATRIX cur_mx;
double scale;
int depth;
{
    int i;
    COORD3 vec;
    COORD4 apex, base;
    MATRIX new_mx;
	
    PLATFORM_MULTITASK();
	
    /* output branch */
    SET_COORD3( vec, 0.0, 0.0, 0.0 ) ;
    lib_transform_point( base, vec, cur_mx ) ;
    base[W] = scale * WIDTH_HEIGHTH_RATIO ;
	
    SET_COORD3( vec, 0.0, 0.0, 1.0 ) ;
    lib_transform_point( apex, vec, cur_mx ) ;
    apex[W] = base[W] * BR_DIAMETER ;
	
    lib_output_cylcone( base, apex, output_format ) ;
    lib_output_sphere( apex, output_format ) ;
	
    if ( depth > 0 ) {
		--depth ;
		
		for ( i = 0 ; i < 2 ; ++i ) {
			if (depth==size_factor-1)
				PLATFORM_PROGRESS(0, i, 1);
			lib_matrix_multiply( new_mx, Rst_mx[i], cur_mx ) ;
			grow_tree( new_mx, scale * BR_DIAMETER, depth ) ;
		}
    }
}

/*
 * Set up matrices for growth of each branch with respect to the
 * parent branch, then grow each branch.
 */
static void
create_tree()
{
    int i;
    double branch_angle, branch_contraction, divergence;
    MATRIX ident_mx, temp1_mx, temp2_mx, tempr_mx, tempst_mx;
	
    for ( i = 0 ; i < 2 ; ++i ) {
		if ( i == 0 ) {
			branch_angle = BR_ANGLE_0 ;
			divergence = 90.0 ;
			branch_contraction = BR_CONTR_0 ;
		} else {
			branch_angle = BR_ANGLE_1 ;
			divergence = DIV_ANGLE + 90.0 ;
			branch_contraction = BR_CONTR_1 ;
		}
		
		/* rotate along X axis by branching angle */
		lib_create_rotate_matrix( temp1_mx, X_AXIS, branch_angle*PI/180.0 ) ;
		
		/* rotate along Z axis by divergence angle */
		lib_create_rotate_matrix( temp2_mx, Z_AXIS, divergence*PI/180.0 ) ;
		
		lib_matrix_multiply( tempr_mx, temp1_mx, temp2_mx ) ;
		
		/* include translation of branch, scaled */
		lib_create_identity_matrix( tempst_mx ) ;
		tempst_mx[0][0] = branch_contraction;
		tempst_mx[1][1] = branch_contraction;
		tempst_mx[2][2] = branch_contraction;
		tempst_mx[3][2] = 1.0;
		
		/* concatenate */
		lib_matrix_multiply( Rst_mx[i], tempr_mx, tempst_mx ) ;
    }
	
    /* set up initial matrix */
    lib_create_identity_matrix( ident_mx ) ;
    grow_tree( ident_mx, 1.0, size_factor ) ;
}

int
main(argc,argv)
int argc;
char *argv[];
{
    COORD3 field[4];
    COORD3 from, at, up;
    COORD3 back_color, tree_color;
    COORD4 light;
    double lscale;
	
    PLATFORM_INIT(SPD_TREE);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_gen_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Tree" ) ) {
		return EXIT_FAIL;
    }
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3( back_color, 0.078, 0.361, 0.753 ) ;
    lib_output_background_color( back_color ) ;
	
    /* output viewpoint */
    SET_COORD3( from, 4.5, 0.4, 2.0 ) ;
    SET_COORD3( at, 0.0, 0.0, 1.5 ) ;
    SET_COORD3( up, 0.0, 0.0, 1.0 ) ;
    lib_output_viewpoint(from, at, up, 45.0, 1.0, 1.0, 512, 512);

    /*
     * For raytracers that don't scale the light intensity,
     * we will do it for them
     */
    #define NUM_LIGHTS    7
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    /* output light source */
    SET_COORD4( light, -5.0, 5.0, 50.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, 30.0, -30.0, 30.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, -40.0, -30.0, 20.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, 10.0, 30.0, 40.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, -30.0, 40.0, 10.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, 50.0, 25.0, 20.0, lscale ) ;
    lib_output_light( light ) ;
    SET_COORD4( light, -10.0, -60.0, 30.0, lscale ) ;
    lib_output_light( light ) ;
	
    /* output field polygon - green */
    SET_COORD3( back_color, 0.2, 0.7, 0.2 ) ;
    lib_output_color(NULL, back_color, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    SET_COORD3( field[0],  50.0,  50.0, 0.0 ) ;
    SET_COORD3( field[1], -50.0,  50.0, 0.0 ) ;
    SET_COORD3( field[2], -50.0, -50.0, 0.0 ) ;
    SET_COORD3( field[3],  50.0, -50.0, 0.0 ) ;
    lib_output_polygon( 4, field ) ;
	
    /* set up tree color - brown */
    SET_COORD3( tree_color, 0.55, 0.4, 0.2 ) ;
    lib_output_color(NULL, tree_color, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	
    /* create tree */
    create_tree();
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
