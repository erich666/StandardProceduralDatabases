/* sombrero.c - example of the use of heightfield output
   From: Alexander Enzmann <70323.2461@compuserve.com>
   Updated: 4/22/95 Eduard Schwan - Added code to match other
   example styles, and made width/height variable based on size parameter
 *	size_factor	 width/height  patches
 *	     1		    32x32
 *	     2		    64x64
 *	     3		   128x128
 *	     4		   256x256
   
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"

/* Standard arguments */
static int size_factor = 1;
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;

/* Define constants for the sombrero function */
static double a_const, b_const, c_const, two_pi_a;

static float **
create_sombrero(width, height, x0, x1, y0, y1)
unsigned width, height;
double x0, x1, y0, y1;
{
	float **data;
	double x, deltax, y, deltay;
	unsigned i, j;
	
	a_const = 1.0;
	b_const = 1.0;
	c_const = 3.0;
	two_pi_a = 2.0 * 3.14159265358 * a_const;
	
	deltax = (x1 - x0) / (double)width;
	deltay = (y1 - y0) / (double)height;
	
	if ((data = malloc(height * sizeof(float *))) == NULL) {
		fprintf(stderr, "HF allocation failed\n");
		exit(1);
	}
	for (i=0,y=y0;i<height;i++,y+=deltay) {
		if ((data[i] = malloc(width * sizeof(float ))) == NULL) {
			fprintf(stderr, "HF allocation failed\n");
			exit(1);
		}
		for (j=0,x=x0;j<width;j++,x+=deltax) {
			PLATFORM_PROGRESS(0, i*width+j, height*width-1);
			
			/* your function here...
			 * For the conversion to work for POV-Ray, Polyray, or any other
			 * format which converts fixed range heightfield data to an image,
			 * make sure the function produces values between -1 and 1. */
			data[i][j] = (float)(c_const * cos(two_pi_a * sqrt(x * x + y * y)) *
				exp(-b_const * sqrt(x * x + y * y))/3.0);
			
			/* here's a simple sine wave function:
			data[i][j] = (float)(((cos(two_pi_a * x / 2.0) + sin(two_pi_a * y / 2.0)/2.0))/1.5) ;
			 */
		}
	}
	return data;
}

int
main(argc, argv)
int argc;
char *argv[];
{
	COORD4 back_color, obj_color;
	COORD4 from, at, up, light;
	double lscale;
	unsigned width = 64, height = 64;
	float **data;
	
    PLATFORM_INIT(SPD_SOMBRERO);
	
	/* Start by defining which raytracer we will be using */
	if (lib_gen_get_opts(argc, argv, &size_factor, &raytracer_format,
		&output_format))
		return EXIT_FAIL;
	
	if (lib_open(raytracer_format, "Sombrero"))
		return EXIT_FAIL;
	
	lib_set_polygonalization(3, 3);
	
	/* output background color - UNC sky blue */
	SET_COORD3(back_color, 0.078, 0.361, 0.753);
	lib_output_background_color(back_color);
	
	/* output viewpoint */
	SET_COORD3(from, 0, 5, -8);
	SET_COORD3(at, 0, 0, 0);
	SET_COORD3(up, 0, 0, 1);
	lib_output_viewpoint(from, at, up, 40.0, 1.0, 0.01, 512, 512);
	
    /*
	 * For raytracers that don't scale the light intensity,
	 * we will do it for them
	 */
	#define NUM_LIGHTS    1
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
	/* output light sources */
	SET_COORD4(light, 10, 10, -10, lscale);
	lib_output_light(light);
	
	/* Height field color - shiny_red */
	SET_COORD3(obj_color, 1, 0.1, 0.1);
	lib_output_color(NULL, obj_color, 0.1, 0.8, 0.0, 0.5, 40.0, 0.0, 1.0);
	
	/* Create the height field */
	width = 32*(1 << (size_factor-1) ); /* 32, 64, 128, 256... */
	height = width;
	data = create_sombrero(width, height, -4.0, 4.0, -4.0, 4.0);
	lib_output_height(NULL, data, width, height, -4.0, 4.0, -3.0, 3.0, -4.0, 4.0);
	
	lib_close();
	
    PLATFORM_SHUTDOWN();
	return EXIT_SUCCESS;
}

