/*
 * shells.c - Creates a shell using Pickover's generation method (see IEEE
 *     CG&A November 1989).  One light source.  This thing tends to bring ray
 *     tracers to their knees (lots of overlapping primitives, many quite
 *     tiny) - it's meant as a study of a realistic yet pathological database.
 *
 *     There are various additional characteristics which can be diddled with
 *     for different shell shapes (most pretty unrealistic, but there you go).
 *     See Pickover's article for more information.  Do "shells -?" to see the
 *     additional parameters which can be varied (listed at the end).
 *
 * Author:  Eric Haines
 *
 * Modified:  Antonio Costa, INESC
 *     Changed lib_get_opts to shells_get_opts
 *     Corrected bug with light source definition
 *     Changed several vars from COORD4 to COORD3
 *
 * Size factor determines the number of objects output.
 *      Total objects = 180*(2**SF) spheres
 *
 *      Size factor     # spheres    # squares
 *           1              360           1
 *           2              720           1
 *           3             1440           1
 *           4             2880           1
 *           5             5760           1
 */

#include <stdio.h>
#include <math.h>
#include "def.h"
#include "lib.h"

/* These may be read from the command line */
static int size_factor      = 5;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;


#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */


static  double  fgamma = 1.0 ;  /* 0.01 to 3 */
static  double  alpha = 0.0 ;   /* > 1 - 1.1 is good */
static  double  beta = -2.0 ;   /* ~ -2 */
static  double  a = 0.15 ;      /* exponent constant */
static  double  k = 1.0 ;       /* relative size */

static void
shells_show_usage()
{
    show_gen_usage() ;
    fprintf(stderr, "-a alpha - alpha value (0 to 1.1 is good)\n");
    fprintf(stderr, "-b beta - beta value (-2 is good)\n");
    fprintf(stderr, "-g gamma - gamma value (0.01 to 3 is good)\n");
    fprintf(stderr, "-e exponent - exponent value (0.15 is good)\n");
}

static int
shells_get_opts( argc, argv, p_size, p_rdr, p_curve )
int     argc ;
char    *argv[] ;
int     *p_size, *p_rdr, *p_curve ;
{
	int num_arg ;
	int val ;
	double fval ;
	
    num_arg = 0 ;
	
    while ( ++num_arg < argc ) {
		if ( (*argv[num_arg] == '-') || (*argv[num_arg] == '/') ) {
			switch( argv[num_arg][1] ) {
			case 'g':       /* gamma */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%lf", &fval ) ;
					if ( fval < 0.0 ) {
						fprintf( stderr,
							"bad gamma value %lf given\n",fval);
						shells_show_usage();
						return( TRUE ) ;
					}
					fgamma = fval ;
				} else {
					fprintf( stderr, "not enough args for -g option\n" ) ;
					shells_show_usage();
					return( TRUE ) ;
				}
				break ;
			case 'a':       /* alpha */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%lf", &alpha ) ;
				} else {
					fprintf( stderr, "not enough args for -a option\n" ) ;
					shells_show_usage();
					return( TRUE ) ;
				}
				break ;
			case 'b':       /* beta */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%lf", &beta ) ;
				} else {
					fprintf( stderr, "not enough args for -b option\n" ) ;
					shells_show_usage();
					return( TRUE ) ;
				}
				break ;
			case 'e':       /* exponent selection */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%lf", &a ) ;
				} else {
					fprintf( stderr, "not enough args for -e option\n" ) ;
					shells_show_usage();
					return( TRUE ) ;
				}
				break ;
				
			case 's':       /* size selection */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%d", &val ) ;
					if ( val < 1 ) {
						fprintf( stderr,
							"bad size value %d given\n",val);
						shells_show_usage();
						return( TRUE ) ;
					}
					*p_size = val ;
				} else {
					fprintf( stderr, "not enough args for -s option\n" ) ;
					shells_show_usage();
					return( TRUE ) ;
				}
				break ;
			case 'c':       /* true curve output */
				*p_curve = OUTPUT_CURVES ;
				break ;
			case 't':       /* tessellated curve output */
				*p_curve = OUTPUT_PATCHES ;
				break ;
			case 'r':       /* renderer selection */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%d", &val ) ;
					if ( val < OUTPUT_VIDEO || val >= OUTPUT_DELAYED ) {
						fprintf( stderr,
							"bad renderer value %d given\n",val);
						shells_show_usage();
						return( TRUE ) ;
					}
					*p_rdr = val ;
				} else {
					fprintf( stderr, "not enough args for -r option\n" ) ;
					shells_show_usage();
					return( TRUE ) ;
				}
				break ;
			default:
				fprintf( stderr, "unknown argument -%c\n",
					argv[num_arg][1] ) ;
				shells_show_usage();
				return( TRUE ) ;
			}
		} else {
			fprintf( stderr, "unknown argument %s\n",
				argv[num_arg] ) ;
			shells_show_usage();
			return( TRUE ) ;
		}
    }
    return( FALSE ) ;
}

int
main(argc,argv)
int     argc ;
char    *argv[] ;
{
	double  r,angle ;
	long    i, steps ;
	COORD3  back_color, obj_color ;
	COORD3  from, at, up ;
	COORD4  light ;
	COORD4  sphere;
	
    PLATFORM_INIT(SPD_SHELLS);
	
    /* Start by defining which raytracer we will be using */
    if ( shells_get_opts( argc, argv,
		&size_factor, &raytracer_format, &output_format ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "Shells" ) ) {
		return EXIT_FAIL;
    }
	/*    lib_set_polygonalization(2, 2);*/
	
    /* output background color - UNC sky blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3( back_color, 0.078, 0.361, 0.753 ) ;
    lib_output_background_color( back_color ) ;
	
    /* output viewpoint */
    SET_COORD3( from, -6.0, -60.0, 35.0 ) ;
    SET_COORD3( at, 0.0, 8.0, -15.0 ) ;
    SET_COORD3( up, 0.0, 0.0, 1.0 ) ;
    lib_output_viewpoint( from, at, up, 45.0, 1.0, 0.5, 512, 512 ) ;
	
    /* output light sources */
    SET_COORD4( light, -100.0, -100.0, 100.0, 1.0 ) ;
    lib_output_light( light ) ;
	
    /* set up sphere color */
    SET_COORD3( obj_color, 1.0, 0.8, 0.4 ) ;
    lib_output_color( NULL, obj_color, 0.0, 0.8, 0.2, 0.5, 10.0, 0.0, 1.0 ) ;
	
    steps = (long)(180.0 * pow( 2.0, (double)size_factor )) ;
    for ( i = -steps*2/3; i <= steps/3 ; ++i ) {
		PLATFORM_PROGRESS(-steps*2/3, i, steps/3);
		PLATFORM_MULTITASK();
		
		angle = 3.0 * 6.0 * PI * (double)i / (double)steps ;
		r = k * exp( a * angle ) ;
		sphere[X] = r * sin( angle ) ;
		sphere[Y] = r * cos( angle ) ;
		if ( alpha > 0.0 ) {
			/* alternate formula: z = alpha * angle */
			sphere[Z] = alpha * angle ;
		} else {
			sphere[Z] = beta * r ;
		}
		sphere[W] = r / fgamma ;
		lib_output_sphere( sphere, output_format ) ;
    }
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
