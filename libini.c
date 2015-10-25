/*
 * libini.c - initialization and teardown routines.
 *
 * Author:  Eric Haines
 *
 */

/*-----------------------------------------------------------------*/
/* include section */
/*-----------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "lib.h"
#include "drv.h"


/*-----------------------------------------------------------------*/
/* defines/constants section */
/*-----------------------------------------------------------------*/

/* output file name */
char gOutfileName[MAX_OUTFILE_NAME_SIZE];

#ifdef OUTPUT_TO_FILE
/* Global output filename suffix list, for each raytracer type */
static char	*gFnameSuffix[OUTPUT_DELAYED+1] =
{
".XXX", /* OUTPUT_VIDEO      Output direct to the screen (sys dependent) */
".nff", /* OUTPUT_NFF        MTV                                         */
".dat", /* OUTPUT_POVRAY_10  POV-Ray 1.0                                 */
".pov", /* OUTPUT_POVRAY_20  POV-Ray 2.x                                 */
".pov", /* OUTPUT_POVRAY_30  POV-Ray 3.x                                 */
".ply", /* OUTPUT_POLYRAY    Polyray v1.4 -> v1.8                        */
".viv", /* OUTPUT_VIVID      Vivid 2.0                                   */
".qrt", /* OUTPUT_QRT        QRT 1.5                                     */
".ray", /* OUTPUT_RAYSHADE   Rayshade                                    */
".rt",  /* OUTPUT_RTRACE     RTrace 8.0.0                                */
".plg", /* OUTPUT_PLG        PLG format for use with REND386/Avril       */
".raw", /* OUTPUT_RAWTRI     Raw triangle output                         */
".art", /* OUTPUT_ART        Art 2.3                                     */
".rib", /* OUTPUT_RIB        RenderMan RIB format                        */
".dxf", /* OUTPUT_DXF        Autodesk DXF format                         */
".obj", /* OUTPUT_OBJ        Wavefront OBJ format                        */
".rwx", /* OUTPUT_RWX        RenderWare RWX script file                  */
".3dm", /* 3D Metafile (Apple Quickdraw 3D text format)                  */
".wrl", /* Virtual Reality Modeling Language 2.0                         */
".out", /* OUTPUT_DELAYED    Needed for RTRACE/PLG output.               */
};
#endif

/*-----------------------------------------------------------------*/
/* Library initialization/teardown functions */
/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
int lib_open(int raytracer_format, char *filename)
#else
int lib_open( raytracer_format, filename )
int     raytracer_format ;
char    *filename ;     /* unused except for Mac version */
#endif
{
	gOutfileName[0]=0;
#ifdef OUTPUT_TO_FILE
    /* no stdout, so write to a file! */
    if (raytracer_format == OUTPUT_VIDEO) {
		gStdout_file = stdout;
    } else {
		/* add appropriate suffix to file name */
		strcpy(gOutfileName, filename);
		strcat(gOutfileName, gFnameSuffix[raytracer_format]);
		/* open the file */
		gStdout_file = fopen(gOutfileName, "w");
		if ( gStdout_file == NULL ) return 1 ;
    }
#endif /* OUTPUT_TO_FILE */
	
    lib_set_output_file(gStdout_file);
	
    gRT_orig_format = raytracer_format;
    if ((raytracer_format == OUTPUT_RTRACE) ||
		(raytracer_format == OUTPUT_PLG))
		lib_set_raytracer(OUTPUT_DELAYED);
    else if (raytracer_format == OUTPUT_RWX) {
		fprintf(gOutfile, "ModelBegin\n");
		fprintf(gOutfile, "ClumpBegin\n");
		fprintf(gOutfile, "LightSampling Vertex\n");
		lib_set_raytracer(raytracer_format);
	}
    else if (raytracer_format == OUTPUT_3DMF) {
		fprintf(gOutfile, "3DMetafile ( 1 0 Normal toc> )\n");
		lib_set_raytracer(raytracer_format);
	}
	else if (raytracer_format == OUTPUT_VRML1) {
        fprintf(gOutfile, "#VRML V1.0 ascii\n");
        fprintf(gOutfile, "Separator {\n");
        tab_inc();
		tab_indent();
		fprintf(gOutfile, "ShapeHints {\n");
		tab_inc();
		tab_indent();
		fprintf(gOutfile, "vertexOrdering COUNTERCLOCKWISE \n");
		tab_indent();
		fprintf(gOutfile, "shapeType UNKNOWN_SHAPE_TYPE \n");
		tab_indent();
		fprintf(gOutfile, "faceType UNKNOWN_FACE_TYPE \n");
		tab_indent();
		fprintf(gOutfile, "creaseAngle 0 \n");
		tab_dec();
		tab_indent();
		fprintf(gOutfile, "}\n");
        lib_set_raytracer(raytracer_format);
	}
	else if (raytracer_format == OUTPUT_VRML2) {
		fprintf(gOutfile, "#VRML V2.0 utf8\n");
		lib_set_raytracer(raytracer_format);
	}
    else
		lib_set_raytracer(raytracer_format);
	
    return 0;
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_close(void)
#else
void lib_close PARAMS((void))
#endif
{
    /* Make sure everything is cleaned up */
    if ((gRT_orig_format == OUTPUT_RTRACE) ||
		(gRT_orig_format == OUTPUT_PLG)) {
		lib_set_raytracer(gRT_orig_format);
		lib_flush_definitions();
    }
	
    if (gRT_out_format == OUTPUT_RIB) {
		fprintf(gOutfile, "WorldEnd\n");
		fprintf(gOutfile, "FrameEnd\n");
    }
    else if (gRT_out_format == OUTPUT_DXF) {
		fprintf(gOutfile, "  0\n");
		fprintf(gOutfile, "ENDSEC\n");
		fprintf(gOutfile, "  0\n");
		fprintf(gOutfile, "EOF\n");
    }
    else if (gRT_out_format == OUTPUT_RWX) {
		fprintf(gOutfile, "ClumpEnd\n");
		fprintf(gOutfile, "ModelEnd\n");
    }
    else if (gRT_out_format == OUTPUT_3DMF) {
	/* Build the table of contents based on any texture names
		we printed */
		surface_ptr temp_ptr;
		
		fprintf(gOutfile, "toc: TableOfContents (\n");
		tab_inc();
		tab_indent();
		fprintf(gOutfile, "toc1>\n");
		tab_indent();
		fprintf(gOutfile, "%d -1 0 12 %d\n",
			gTexture_count+2, gTexture_count);
		/* Step through the textures, printing table of contents entries */
		for (temp_ptr=gLib_surfaces;
		temp_ptr!= NULL;
		temp_ptr = temp_ptr->next) {
			tab_indent();
			fprintf(gOutfile, "%d %s>\n",
				temp_ptr->surf_index, temp_ptr->surf_name);
		}
		tab_dec();
		fprintf(gOutfile, ")\n");
	}
	else if (gRT_out_format == OUTPUT_VRML1) {
		tab_dec();
		tab_indent();
		fprintf(gOutfile, "}\n");
	}
	
#ifdef OUTPUT_TO_FILE
    /* no stdout, so close our output! */
    if (gStdout_file)
		fclose(gStdout_file);
#endif /* OUTPUT_TO_FILE */
    if (gRT_out_format == OUTPUT_VIDEO)
		display_close(1);
}


/*-----------------------------------------------------------------*/
void lib_storage_initialize PARAMS((void))
{
    gPoly_vbuffer = (unsigned int*)malloc(VBUFFER_SIZE * sizeof(unsigned int));
    gPoly_end = (int*)malloc(POLYEND_SIZE * sizeof(int));
    if (!gPoly_vbuffer || !gPoly_end) {
		fprintf(stderr,
			"Error(lib_storage_initialize): Can't allocate memory.\n");
		exit(1);
    }
} /* lib_storage_initialize */


/*-----------------------------------------------------------------*/
void
lib_storage_shutdown PARAMS((void))
{
    if (gPoly_vbuffer) {
		free(gPoly_vbuffer);
		gPoly_vbuffer = NULL;
    }
    if (gPoly_end) {
		free(gPoly_end);
		gPoly_end = NULL;
    }
} /* lib_storage_shutdown */


/*-----------------------------------------------------------------*/
void show_gen_usage PARAMS((void))
{
    /* localize the usage strings to be expanded only once for space savings */
#if defined(applec) || defined(THINK_C) || defined(__MWERKS__)
    /* and don't write to stdout on Macs, which don't have console I/O, and  */
    /* won't ever get this error anyway, since parms are auto-generated.     */
#else
    fprintf(stderr, "usage [-s size] [-r format] [-c|t [#]]\n");
    fprintf(stderr, "-s size - input size of database\n");
    fprintf(stderr, "-r format - input database format to output:\n");
    fprintf(stderr, "   0   Output direct to the screen (sys dependent)\n");
    fprintf(stderr, "   1   NFF - MTV\n");
    fprintf(stderr, "   2   POV-Ray 1.0\n");
    fprintf(stderr, "   3   POV-Ray 2.0 to 2.2\n");
    fprintf(stderr, "   4   POV-Ray 3.1\n");
    fprintf(stderr, "   5   Polyray v1.4, v1.5\n");
    fprintf(stderr, "   6   Vivid 2.0\n");
    fprintf(stderr, "   7   QRT 1.5\n");
    fprintf(stderr, "   8   Rayshade\n");
    fprintf(stderr, "   9   RTrace 8.0.0\n");
    fprintf(stderr, "   10  PLG format for use with rend386\n");
    fprintf(stderr, "   11  Raw triangle output\n");
    fprintf(stderr, "   12  art 2.3\n");
    fprintf(stderr, "   13  RenderMan RIB format\n");
    fprintf(stderr, "   14  Autodesk DXF format (polygons only)\n");
    fprintf(stderr, "   15  Wavefront OBJ format (polygons only)\n");
    fprintf(stderr, "   16  RenderWare RWX script file\n");
    fprintf(stderr, "   17  3D Metafile (Apple Quickdraw 3D text format)\n");
    fprintf(stderr, "   18  VRML 1.0 (Virtual Reality Modeling Language)\n");
    fprintf(stderr, "   19  VRML 2.0 (Virtual Reality Modeling Language)\n");
    fprintf(stderr, "-c - output true curved descriptions\n");
    fprintf(stderr, "-t [#] - output tessellated triangle descriptions [and resolution]\n");
	
#endif
} /* show_gen_usage */

void show_read_usage PARAMS((void))
{
    /* localize the usage strings to be expanded only once for space savings */
#if defined(applec) || defined(THINK_C) || defined(__MWERKS__)
    /* and don't write to stdout on Macs, which don't have console I/O, and  */
    /* won't ever get this error anyway, since parms are auto-generated.     */
#else
    fprintf(stderr, "usage [-f filename] [-r format] [-c|t [#]]\n");
    fprintf(stderr, "-f filename - file to import/convert/display\n");
    fprintf(stderr, "-r format - format to output:\n");
    fprintf(stderr, "   0   Output direct to the screen (sys dependent)\n");
    fprintf(stderr, "   1   NFF - MTV\n");
    fprintf(stderr, "   2   POV-Ray 1.0\n");
    fprintf(stderr, "   3   POV-Ray 2.0 to 2.2\n");
    fprintf(stderr, "   4   POV-Ray 3.1\n");
    fprintf(stderr, "   5   Polyray v1.4, v1.5\n");
    fprintf(stderr, "   6   Vivid 2.0\n");
    fprintf(stderr, "   7   QRT 1.5\n");
    fprintf(stderr, "   8   Rayshade\n");
    fprintf(stderr, "   9   RTrace 8.0.0\n");
    fprintf(stderr, "   10  PLG format for use with rend386\n");
    fprintf(stderr, "   11  Raw triangle output\n");
    fprintf(stderr, "   12  art 2.3\n");
    fprintf(stderr, "   13  RenderMan RIB format\n");
    fprintf(stderr, "   14  Autodesk DXF format (polygons only)\n");
    fprintf(stderr, "   15  Wavefront OBJ format (polygons only)\n");
    fprintf(stderr, "   16  RenderWare RWX script file\n");
    fprintf(stderr, "   17  3D Metafile (Apple Quickdraw 3D text format)\n");
    fprintf(stderr, "   18  VRML 1.0 (Virtual Reality Modeling Language)\n");
    fprintf(stderr, "   19  VRML 2.0 (Virtual Reality Modeling Language)\n");
    fprintf(stderr, "-c - output true curved descriptions\n");
    fprintf(stderr, "-t [#] - output tessellated triangle descriptions [and resolution]\n");
	
#endif
} /* show_read_usage */


/*-----------------------------------------------------------------*/
/*
 * Command line option parser for db generator
 *
 * -s size - input size of database (1 to N)
 * -r format - input database format to output (see lib.h for formats)
 * -c - output true curved descriptions
 * -t [#] - output tessellated triangle descriptions [and resolution]
 *
 * TRUE returned if bad command line detected
 * some of these are useless for the various routines - we're being a bit
 * lazy here...
 */
#ifdef ANSI_FN_DEF
int     lib_gen_get_opts (int argc, char *argv[], int *p_size, int *p_rdr, int *p_curve)
#else
int     lib_gen_get_opts( argc, argv, p_size, p_rdr, p_curve )
int     argc ;
char    *argv[] ;
int     *p_size, *p_rdr, *p_curve ;
#endif
{
	int num_arg ;
	int val ;
	
    num_arg = 0 ;
	
    while ( ++num_arg < argc ) {
		if ( (*argv[num_arg] == '-') || (*argv[num_arg] == '/') ) {
			switch( argv[num_arg][1] ) {
			case 'c':       /* true curve output */
				*p_curve = OUTPUT_CURVES ;
				break ;
			case 't':       /* tessellated curve output */
				*p_curve = OUTPUT_PATCHES ;
				if ( num_arg < argc-1 ) {
					if ( argv[num_arg+1][0] != '-' ) {
						num_arg++ ;
						sscanf( argv[num_arg], "%d", &val ) ;
						if ( val < 1 ) {
							fprintf( stderr,
								"bad resolution value %s given\n",
								argv[num_arg]);
							show_gen_usage();
							return( TRUE ) ;
						}
						gU_resolution =
							gV_resolution = val ;
					}
				} /* else no resolution found */
				break ;
			case 'r':       /* renderer selection */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%d", &val ) ;
					if ( val < OUTPUT_VIDEO || val >= OUTPUT_DELAYED ) {
						fprintf( stderr,
							"bad renderer value %d given\n",val);
						show_gen_usage();
						return( TRUE ) ;
					}
					*p_rdr = val ;
				} else {
					fprintf( stderr, "not enough args for -r option\n" ) ;
					show_gen_usage();
					return( TRUE ) ;
				}
				break ;
			case 's':       /* size selection */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%d", &val ) ;
					if ( val < 1 ) {
						fprintf( stderr,
							"bad size value %d given\n",val);
						show_gen_usage();
						return( TRUE ) ;
					}
					*p_size = val ;
				} else {
					fprintf( stderr, "not enough args for -s option\n" ) ;
					show_gen_usage();
					return( TRUE ) ;
				}
				break ;
			default:
				fprintf( stderr, "unknown argument -%c\n",
					argv[num_arg][1] ) ;
				show_gen_usage();
				return( TRUE ) ;
			}
		} else {
			fprintf( stderr, "unknown argument %s\n",
				argv[num_arg] ) ;
			show_gen_usage();
			return( TRUE ) ;
		}
    }
    return( FALSE ) ;
}


/*-----------------------------------------------------------------*/
/*
 * Command line option parser for db reader (converter/displayer)
 *
 * -f filename - file to import/convert/display
 * -r format - input database format to output (see lib.h for formats)
 * -c - output true curved descriptions
 * -t [#] - output tessellated triangle descriptions [and resolution]
 *
 * TRUE returned if bad command line detected
 * some of these are useless for the various routines - we're being a bit
 * lazy here...
 */
#ifdef ANSI_FN_DEF
int     lib_read_get_opts (int argc, char *argv[], int *p_rdr, int *p_curve, char *p_infname)
#else
int     lib_read_get_opts( argc, argv, p_rdr, p_curve, p_infname )
int     argc ;
char    *argv[] ;
int     *p_rdr, *p_curve ;
char *p_infname;
#endif
{
	int num_arg ;
	int val ;
	
    num_arg = 0 ;
    *p_rdr = OUTPUT_NFF ;	/* default format if none given */

	if ( p_infname != NULL ) {
		*p_infname = (char)0 ;	/* default file name is empty */
	}
	
    while ( ++num_arg < argc ) {
		if ( (*argv[num_arg] == '-') || (*argv[num_arg] == '/') ) {
			switch( argv[num_arg][1] ) {
			case 'c':       /* true curve output */
				*p_curve = OUTPUT_CURVES ;
				break ;
			case 't':       /* tessellated curve output */
				*p_curve = OUTPUT_PATCHES ;
				break ;
			case 'f':       /* input file name */
				if ( p_infname == NULL ) {
					fprintf( stderr, "-f option not allowed\n" ) ;
					show_read_usage();
					return( TRUE ) ;
				} else {
					if ( ++num_arg < argc ) {
						sscanf( argv[num_arg], "%s", p_infname ) ;
					} else {
						fprintf( stderr, "not enough args for -f option\n" ) ;
						show_read_usage();
						return( TRUE ) ;
					}
				}
				break ;
			case 'r':       /* renderer selection */
				if ( ++num_arg < argc ) {
					sscanf( argv[num_arg], "%d", &val ) ;
					if ( val < OUTPUT_VIDEO || val >= OUTPUT_DELAYED ) {
						fprintf( stderr,
							"bad renderer value %d given\n",val);
						show_read_usage();
						return( TRUE ) ;
					}
					*p_rdr = val ;
				} else {
					fprintf( stderr, "not enough args for -r option\n" ) ;
					show_read_usage();
					return( TRUE ) ;
				}
				break ;
			default:
				fprintf( stderr, "unknown argument -%c\n",
					argv[num_arg][1] ) ;
				show_read_usage();
				return( TRUE ) ;
			}
		} else {
			fprintf( stderr, "unknown argument %s\n",
				argv[num_arg] ) ;
			show_read_usage();
			return( TRUE ) ;
		}
    }
	if ( p_infname != NULL && *p_infname == (char)0 ) {
		/* no file input, illegal */
		show_read_usage();
		return( TRUE ) ;
	}
    return( FALSE ) ;
}


/*-----------------------------------------------------------------*/
void
lib_clear_database PARAMS((void))
{
    surface_ptr ts1, ts2;
    object_ptr to1, to2;
    light_ptr tl1, tl2;
	
    gOutfile = stdout;
    gTexture_name = NULL;
    gTexture_count = 0;
    gObject_count = 0;
    gTexture_ior = 1.0;
    gRT_out_format = OUTPUT_RT_DEFAULT;
    gU_resolution = OUTPUT_RESOLUTION;
    gV_resolution = OUTPUT_RESOLUTION;
    SET_COORD3(gBkgnd_color, 0.0, 0.0, 0.0);
    SET_COORD3(gFgnd_color, 0.0, 0.0, 0.0);
	
    /* Remove all surfaces */
    ts1 = gLib_surfaces;
    while (ts1 != NULL) {
		ts2 = ts1;
		ts1 = ts1->next;
		free(ts2->surf_name);
		free(ts2);
    }
    gLib_surfaces = NULL;
	
    /* Remove all objects */
    to1 = gLib_objects;
    while (to1 != NULL) {
		to2 = to1;
		to1 = to1->next_object;
		free(to2);
    }
    gLib_objects = NULL;
	
    /* Remove all lights */
    tl1 = gLib_lights;
    while (tl1 != NULL) {
		tl2 = tl1;
		tl1 = tl1->next;
		free(tl2);
    }
    gLib_lights = NULL;
	
    /* Reset the view */
	
    /* Deallocate polygon buffer */
    if (gPoly_vbuffer != NULL)
		lib_storage_shutdown();
	
    /* Clear vertex counters for polygons */
    gVertex_count = 0; /* Vertex coordinates */
    gNormal_count = 0; /* Vertex normals */
	
    /* Clear out the polygon stack */
    to1 = gPolygon_stack;
    while (to1 != NULL) {
		to2 = to1;
		to1 = to1->next_object;
		free(to2);
    }
    gPolygon_stack = NULL;
}

/*-----------------------------------------------------------------*/
void
lib_flush_definitions PARAMS((void))
{
    switch (gRT_out_format) {
	case OUTPUT_RTRACE:
	case OUTPUT_VIDEO:
	case OUTPUT_NFF:
	case OUTPUT_POVRAY_10:
	case OUTPUT_POVRAY_20:
	case OUTPUT_POVRAY_30:
	case OUTPUT_POLYRAY:
	case OUTPUT_VIVID:
	case OUTPUT_QRT:
	case OUTPUT_RAYSHADE:
	case OUTPUT_PLG:
	case OUTPUT_OBJ:
	case OUTPUT_RWX:
	case OUTPUT_RAWTRI:
	case OUTPUT_3DMF:
	case OUTPUT_VRML1:
	case OUTPUT_VRML2:
		lib_output_viewpoint(gViewpoint.from, gViewpoint.at, gViewpoint.up, gViewpoint.angle,
			gViewpoint.aspect, gViewpoint.hither, gViewpoint.resx, gViewpoint.resy);
		
		lib_output_background_color(gBkgnd_color);
		
		dump_all_lights();
		
		dump_reorder_surfaces();
		
		dump_all_surfaces();
		
		dump_all_objects();
		
		if (gRT_out_format == OUTPUT_RTRACE)
			fprintf(gOutfile, "Textures\n\n");
		
		break;
	case OUTPUT_DELAYED:
		fprintf(stderr, "Error: Renderer not selected before flushing\n");
		exit(1);
	default:
		fprintf(stderr, "Internal Error: bad file type in libini.c\n");
		exit(1);
    }
	
    if (gRT_out_format == OUTPUT_PLG)
		/* An extra step is needed to build the polygon file. */
		dump_plg_file();
}
