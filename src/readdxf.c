/*
 * ReadDXF.c - Read polygons from a DXF file and display them.
 * The file "view.dat" is processed for the camera view that will be used.
 *
 * Author:  Alexander Enzmann
 *
 * size_factor is ignored.
 *
 *    size_factor       # spheres        # squares
 *	     x               xx                 x
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>     /* atoi */
#include <string.h>     /* strcmp() */
#include "def.h"
#include "drv.h"       /* display_close() */
#include "lib.h"


/* These may be read from the command line */
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;


#ifdef OUTPUT_TO_FILE
static FILE * stdout_file = NULL;
#else
#define stdout_file stdout
#endif /* OUTPUT_TO_FILE */


/* This is an outrageous hack to read the polygons from a DXF file.
   No attempt is made to be smart, just to graph 3DFACEs.  If you have
   something better, go for it. There are plenty of DXF files that this
   routine won't work for... */
static void
read_dxf_faces( file )
FILE *file;
{
    char buffer[128];
    int i, j, ind, vcnt, fcnt, lineno, maxvert;
    float x;
    COORD3 face[16];
	
    fcnt = lineno = 0;
    while (!feof(file)) {
		
		/* Skip over uninteresting stuff */
		while (!feof(file) &&
			fgets(buffer, 127, file) != NULL &&
			strcmp(buffer, "3DFACE\n"))
			/* Void - we are reading lines we can't deal with */
			lineno++;
		
		if (!feof(file)) {
			
#if !defined(applec) && !defined(THINK_C) && !defined(__MWERKS__)
			/* Hmm, Xander, isn't kbhit() only in MSDOS, not in Unix libraries..? */
			
			/* Test to see if we should stop */
			if (kbhit()) {
				display_close(0);
				fprintf(stderr, "Draw aborted\n");
				exit(EXIT_FAIL);
			}
#endif
			
			/* As long as there are more faces, read them in */
			fgets(buffer, 127, file); /* Skip the "8" */
			lineno++;
			
			fgets(buffer, 127, file); /* Skip the "0main" */
			lineno++;
			
			if (buffer[0] == '0')
				;
			else if (!strcmp(buffer, "3DFURN\n")) {
				fgets(buffer, 127, file); /* Skip the thing after the 3DFURN */
				fgets(buffer, 127, file); /* Skip over the CONTINUOUS */
				lineno += 2;
				if (!strcmp(buffer, "CONTINUOUS\n") &&
					fgets(buffer, 127, file) != NULL &&
					sscanf(buffer, "%d", &ind) != 0) {
					lineno++;
					if (ind == 62) {
						fgets(buffer, 127, file); /* Max # of vertices? */
						sscanf(buffer, "%d", &maxvert);
						fgets(buffer, 127, file);
						sscanf(buffer, "%d", &ind);
						lineno++;
					} else {
						maxvert = -1;
					}
					fgets(buffer, 127, file);
					sscanf(buffer, "%f", &x);
					lineno++;
					goto inside_vertex_loop;
				}
				else
					break;
			} else if (!strcmp(buffer, "2\n")) {
				fgets(buffer, 127, file);
				sscanf(buffer, "%d", &ind);
				lineno++;
				if (ind == 62) {
					fgets(buffer, 127, file); /* Max # of vertices */
					sscanf(buffer, "%d", &maxvert);
					fgets(buffer, 127, file);
					sscanf(buffer, "%d", &ind);
					lineno++;
				} else {
					maxvert = -1;
				}
				fgets(buffer, 127, file);
				sscanf(buffer, "%f", &x);
				lineno++;
				goto inside_vertex_loop;
			} else
				break;
			
			lineno += 3;
			vcnt = 0;
			maxvert = -1;
			
			/* This is a face, read the vertices */
			while (fgets(buffer, 127, file) != NULL &&
				(sscanf(buffer, "%d", &ind) != 0) &&
				ind != 0 &&
				fgets(buffer, 127, file) != NULL &&
				sscanf(buffer, "%f", &x)) {
				
				PLATFORM_MULTITASK();
				
				lineno += 2;
inside_vertex_loop:
				/* Got a vertex value */
				j = ind / 10;
				i = ind % 10;
				
				if (maxvert > 0 && i > maxvert)
					break;
				else if (i > vcnt)
					vcnt = i;
				
				/* Place the value into the appropriate face */
				if (j == 1)
					face[i][X] = x;
				else if (j == 2)
					face[i][Y] = x;
				else if (j == 3)
					face[i][Z] = x;
				else if (maxvert > 0)
					break;
				else {
					display_close(1);
					fprintf(stderr, "Bad vertex component: %d/%d at line: %d\n",
						i, j, lineno);
					exit(EXIT_FAIL);
				}
			}
			/* Display the polygon */
			if (vcnt > 0)
				lib_output_polygon(vcnt+1, face);
				/*
				printf("Vert[%d]: ", fcnt);
				for (i=0;i<vcnt;i++)
				printf("<%g, %g, %g> ", face[i][X], face[i][Y], face[i][Z]);
				printf("\n");
			*/
			fcnt++;
	}
    }
}

/* Read in the camera specifics: from, at, up, fov.  Aspect is hard coded
to 1.  */
static void
setup_view()
{
    char buffer[128];
    COORD3 from, at, up;
    double angle;
    FILE *setup;
	
    /* output viewpoint */
    if ((setup = fopen("view.dat", "r")) != NULL) {
		if (fgets(buffer, 127, setup) &&
			sscanf(buffer, "%lf %lf %lf",
			&from[X], &from[Y], &from[Z]) != 0 &&
			fgets(buffer, 127, setup) &&
			sscanf(buffer, "%lf %lf %lf",
			&at[X], &at[Y], &at[Z]) != 0 &&
			fgets(buffer, 127, setup) &&
			sscanf(buffer, "%lf %lf %lf",
			&up[X], &up[Y], &up[Z]) != 0 &&
			fgets(buffer, 127, setup) &&
			sscanf(buffer, "%lf", &angle)) {
			lib_output_viewpoint(from, at, up, 45.0, 1.0, 1.0, 512, 512);
		} else {
#if defined(applec) || defined(THINK_C)
#else
			fprintf(stderr, "Invalid 'view.dat' file\n");
#endif
			exit(EXIT_FAIL);
		}
		fclose( setup );
    } else {
		SET_COORD3(from, 0, 10, -10);
		SET_COORD3(at, 0, 0, 0);
		SET_COORD3(up, 0, 0, 1);
		lib_output_viewpoint(from, at, up, 45.0, 1.0, 1.0, 512, 512);
    }
}

/* Read in the camera view, then read in DXF polygons, then display them. */
int
main(argc, argv)
int argc;
char *argv[];
{
    COORD3 back_color, dxf_color;
    COORD4 light;
    double lscale;
    char file_name[64] ;
    FILE *file;
	
    PLATFORM_INIT(SPD_READDXF);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_read_get_opts( argc, argv,
		&raytracer_format, &output_format, file_name ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "ReadDXF" ) ) {
		return EXIT_FAIL;
    }
	
    file = fopen(file_name, "r");
    if (file == NULL) {
		fprintf(stderr, "Cannot open dxf file: '%s'\n", file_name);
		return EXIT_FAIL;
    }
	
	
    /* output background color - dark blue */
    /* NOTE: Do this BEFORE lib_output_viewpoint(), for display_init() */
    SET_COORD3(back_color, 0.1, 0.0, 0.5);
    lib_output_background_color(back_color);
	
    setup_view();
	
    /* output object color - light gray */
    SET_COORD3(dxf_color, 0.8, 0.8, 0.8);
    lib_output_color(NULL, dxf_color, 0.1, 0.8, 0.0, 0.2, 10.0, 0.0, 1.0);

    /*
     * For raytracers that don't scale the light intensity,
     * we will do it for them
     */
	#define NUM_LIGHTS    2
    lscale = ( ((raytracer_format==OUTPUT_NFF) || (raytracer_format==OUTPUT_RTRACE))
	       ? 1.0 : 1.0 / sqrt(NUM_LIGHTS));
	
    SET_COORD4(light, 40.0, 30.0, 20.0, lscale);
    lib_output_light(light);
    SET_COORD4(light, -40, -20, 10, lscale);
    lib_output_light(light);
	
    read_dxf_faces(file);
	
    fclose(file);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
