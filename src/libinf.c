/*
 * libinf.c - general info routines.
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

#ifdef OUTPUT_TO_FILE
FILE * gStdout_file = NULL;
#endif /* OUTPUT_TO_FILE */


/*-----------------------------------------------------------------*/
/* Here are some local variables that are used to control things like
   the current output file, current texture, ... */
FILE *gOutfile;
char *gTexture_name = NULL;
int  gTexture_count = 0;
int  gObject_count = 0;
double gTexture_ior = 1.0;
int  gRT_out_format        = OUTPUT_NFF;
int  gRT_orig_format   = OUTPUT_NFF;
int  gU_resolution  = OUTPUT_RESOLUTION;
int  gV_resolution  = OUTPUT_RESOLUTION;
COORD3 gBkgnd_color = {0.0, 0.0, 0.0};
COORD3 gFgnd_color = {0.0, 0.0, 0.0};
double gView_bounds[2][3];
int gView_init_flag = 0;
char *gLib_version_str = LIB_VERSION;

surface_ptr gLib_surfaces = NULL;
object_ptr gLib_objects = NULL;
light_ptr gLib_lights = NULL;
viewpoint gViewpoint = {
	{0, 0, -10},
    {0, 0, 0},
    {0, 1, 0},
     45, 1, 1.0e-3, 10, 128, 128,
    { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1} }
};

/* Globals for tracking indentation level of output file */
int      gTab_width = 4;
int      gTab_level = 0;



/*-----------------------------------------------------------------*/
void tab_indent PARAMS((void))
{
    int      k;
    /* Q&D way to do it... */
    for (k=0; k<gTab_width*gTab_level; k++)
		putc(' ', gOutfile);
} /* tab_printf */


/*-----------------------------------------------------------------*/
void tab_inc PARAMS((void))
{
    gTab_level++;
} /* tab_inc */


/*-----------------------------------------------------------------*/
void tab_dec PARAMS((void))
{
    gTab_level--;
    if (gTab_level < 0)
		gTab_level = 0;
} /* tab_dec */


/* Library info functions */

/*-----------------------------------------------------------------*/
char *
lib_get_version_str PARAMS((void))
{
    return gLib_version_str;
} /* lib_get_version_str */


/*-----------------------------------------------------------------*/
/*
 * Routines to set/reset the various output parameters
 */
/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_set_output_file (FILE *new_outfile)
#else
void lib_set_output_file(new_outfile)
FILE *new_outfile;
#endif
{
    if (new_outfile == NULL)
		gOutfile = stdout;
    else
		gOutfile = new_outfile;
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_set_default_texture (char *default_texture)
#else
void lib_set_default_texture(default_texture)
char *default_texture;
#endif
{
    gTexture_name = default_texture;
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_set_raytracer(int default_tracer)
#else
void lib_set_raytracer(default_tracer)
int default_tracer;
#endif
{
    if (default_tracer < OUTPUT_VIDEO ||
		default_tracer > OUTPUT_DELAYED) {
		fprintf(stderr, "Unknown renderer index: %d\n", default_tracer);
		exit(1);
    }
    gRT_out_format = default_tracer;
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_set_polygonalization(int u_steps, int v_steps)
#else
void lib_set_polygonalization(u_steps, v_steps)
int u_steps, v_steps;
#endif
{
    if ((u_steps > 0) && (v_steps > 0)) {
		gU_resolution = u_steps;
		gV_resolution = v_steps;
    }
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lookup_surface_stats(int index, int *tcount, double *tior)
#else
void lookup_surface_stats(index, tcount, tior)
int index, *tcount;
double *tior;
#endif
{
    surface_ptr temp_ptr = gLib_surfaces;
	
    while (temp_ptr != NULL && (int)temp_ptr->surf_index != index)
		temp_ptr = temp_ptr->next;
    if (temp_ptr != NULL) {
		*tior = temp_ptr->ior;
		if (*tior < 1.0) *tior = 1.0;
		*tcount = temp_ptr->surf_index;
    }
    else {
		*tior = 1.0;
		*tcount = 0;
    }
}
