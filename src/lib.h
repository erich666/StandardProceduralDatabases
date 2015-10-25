/*
 * lib.h - library definitions
 *
 * Author:  Eric Haines
 *
 * Modified: 1 October 1992
 *           Alexander R. Enzmann
 *           I made quite a few changes in order to support multiple raytracers,
 *           with the hopes that this library would become even more useful than
 *           it already is.
 *
 * Modified: 17 Jan 1993
 *           Eduard [esp] Schwan
 *           Removed unused local variables & returned value in lib_output_color
 *
 * Modified: 17 Mar 1993
 *           Eduard [esp] Schwan
 *           Changed POV-Ray refs to OUTPUT_POVRAY_10 & OUTPUT_POVRAY_20
 *           Changed POV-Ray 1.5 refs to 2.0
 *           Passed bg_color to display routines
 *             (unfortunate side-effect is you should now call
 *             lib_output_background_color BEFORE lib_output_viewpoint.
 *             This may not be the best approach - please review!)
 *           Added OUTPUT_RAWTRI output, for creating Raw output for Raw2POV.
 *           Added separator comment lines between fns for readability
 *
 * Modified: 30 Mar 1993
 *           Eduard [esp] Schwan
 *           Made poly static arrays dynamic
 *           Created lib_init/lib_shutdown routines
 *           Removed unused local variables
 *           Added creation of enclosing sphere of bg color for POV-Ray
 *
 * Modified: 12 April 1993
 *           Alexander R. Enzmann
 *           Updated to include Eric Haines' version 3.1 SPD data types
 *
 * Modified: 16 April 1993
 *           Eduard [esp] Schwan
 *           Sprinkled MULTITASK macros in loops to let cooperative
 *           multitasking OS systems (Macs) breathe, and added a
 *           string version # & accessor routine for reporting.
 *
 * Modified: 27 July 1993
 *           David Hook
 *           Added converter for "art" ray tracer.
 *
 * Modified: 5 August 1993
 *           Eduard [esp] Schwan
 *           Fixed filename bug in lib_get_opts,
 *           cleaned up Mac #pragma statements, made USAGE a fn,
 *           added '/' as command line switch prefix for MSDOS dudes.
 *           Added POV 2.0 cone and disc primitive syntax, removed
 *           redundant "object { }" wrapper around POV 2.0 objects,
 *           added POV 2.0 "background {}" statement.  Also cleaned
 *           up indenting by adding tab_() routines.  Added A Enzmann's
 *           fixes to the POV 1.0 cylinder/cone routines.
 *
 * Modified: 31 August 1993
 *           Eric Haines
 *           Minor syntax things (hey, I like 4 space indents).
 *
 * Modified: 5 September 1993
 *           Antonio Costa
 *           Changed EPSILON to EPSILON2 in several places
 *           Removed incorrect statement in lib_output_polygon_cylcone()
 *           Added missing break statement to RTrace related code (in
 *           old line 757)
 *
 * Modified: 6 September 1993
 *           Eric Haines
 *           Created standardized lib_open/lib_close functions
 *           Renamed lib_initialize/lib_shutdown to lib_storage_*
 *
 * Modified: 8 September 1993
 *           Antonio Costa
 *           Added global gRT_orig_format to make lib_open/lib_close work!
 *
 * Modified: 16 October 1993
 *           Eduard [esp] Schwan
 *           Added  show_usage prototype so shells.c can call it, broke lib.c
 *           into several smaller sub-libraries for small-model compiles.
 *
 * Modified  19 January 1994
 *           Philipp Slusallek
 *           Added RIB file output
 *
 * Modified: 3 May 1994
 *           Eric Haines
 *           Split lib_get_opts to lib_gen_get_opts and lib_read_get_opts,
 *           since the read*.c programs are functionally different than the
 *           database generators.  Also split show_usage.
 *
 * Modified: 6 May 1994
 *           Eduard [esp] Schwan
 *           Broke libprm.c into even smaller sub-libraries for small-model
 *           Mac compiles, since the new RIB code pushed us over again.
 *           So libprm.c is now libpr1.c and libpr2.c.  Also took a guess and
 *           added OUTPUT_RIB to the list in lib_output_comment() in libpr1.c.
 *
 * Modified: 13 May 1994
 *           Eric Haines
 *           Various changes from esp added, and put in primitive DXF output.
 *           Named version 3.3 now to differentiate from DXF-less output
 *           version Eduard is distributing.
 *
 * Modified: 15 November 1994
 *           Alexander R. Enzmann
 *           Added code for Wavefront OBJ files and RenderWare RWX files.
 *           Adding code for transformation stacks, file jacks.c demonstrates
 *           how they work.  Fixed PLG output.  Fixed ART output of tori
 *           and added ART output of discs.  Modified Polyray and POV-Ray
 *           viewpoints to give right handed coordinates.  Added the
 *           file readobj.c to read WaveFront polygon files.  Added routines
 *           to invert matrices, transform normals, and determine
 *           rotate/scale/translate from a transform matrix. Modified
 *           computation of perspective view matrix to be a little cleaner.
 *
 * Modified: 20 June 1995
 *           Alexander R. Enzmann
 *           Added code for 3D Metafile output (Apple Quickdraw 3D).  Added
 *           output of NURBS (as triangular patches for most renderers).
 *
 * Modified: 6 January 1998  - Added PLATFORM_PROGRESS to allow UI platforms
 *           to display a progress indicator if they want during generation,
 *           Added ANSI_FN_DEFS flag to turn on or off K&R versus ANSI function
 *           declarations.
 *           Eduard [esp] Schwan
 */


#ifndef LIB_H
#define LIB_H

#include "def.h"
#include "libvec.h"

#if __cplusplus
extern "C" {
#endif

/* The version of this Library, see lib_get_version_str() */
#define LIB_VERSION     "3.13"

/* Raytracers supported by this package (default OUTPUT_POVRAY): */

/* Note: any new renderers should be added between OUTPUT_VIDEO and
   OUTPUT_DELAYED.  These two values are used as a range check that a known
   renderer has been selected in "lib_set_raytracer" */
#define OUTPUT_VIDEO      0 /* Output direct to the screen (sys dependent)  */
#define OUTPUT_NFF        1 /* MTV                                          */
#define OUTPUT_POVRAY_10  2 /* POV-Ray 1.0                                  */
#define OUTPUT_POVRAY_20  3 /* POV-Ray 2.0                                  */
#define OUTPUT_POVRAY_30  4 /* POV-Ray 3.1                                  */
#define OUTPUT_POLYRAY    5 /* Polyray v1.4 -> v1.8                         */
#define OUTPUT_VIVID      6 /* Vivid 2.0                                    */
#define OUTPUT_QRT        7 /* QRT 1.5                                      */
#define OUTPUT_RAYSHADE   8 /* Rayshade                                     */
#define OUTPUT_RTRACE     9 /* RTrace 8.0.0                                 */
#define OUTPUT_PLG       10 /* PLG format for use with REND386/Avril        */
#define OUTPUT_RAWTRI    11 /* Raw triangle output                          */
#define OUTPUT_ART       12 /* Art 2.3                                      */
#define OUTPUT_RIB       13 /* RenderMan RIB format                         */
#define OUTPUT_DXF       14 /* Autodesk DXF format                          */
#define OUTPUT_OBJ       15 /* Wavefront OBJ format                         */
#define OUTPUT_RWX       16 /* RenderWare RWX script file                   */
#define OUTPUT_3DMF      17 /* 3D Metafile (Apple Quickdraw 3D text format) */
#define OUTPUT_VRML1     18 /* Virtual Reality Modeling Language 1.0        */
#define OUTPUT_VRML2     19 /* Virtual Reality Modeling Language 2.0        */
#define OUTPUT_DELAYED   20 /* Needed for RTRACE/PLG output.
			       When this is used, all definitions will be
			       stored rather than immediately dumped.  When
			       all definitions are complete, use the call
			       "lib_flush_definitions" to spit them all out. */

/* Default setting of output RT type - change to your favorite & recompile */
#define OUTPUT_RT_DEFAULT   OUTPUT_NFF

/* Sets raw triangle output format to include texture name */
#define RAWTRI_WITH_TEXTURES    0       /* set to 1 to include texture in RAW files */

#define OUTPUT_RESOLUTION       3       /* default amount of polygonalization */


/* ========== don't mess from here on down ============================= */

/* Output library definitions */
#define OUTPUT_CURVES           0       /* true curve output */
#define OUTPUT_PATCHES          1       /* polygonal patches output */

/* polygon stuff for libply.c and lib.c */
#define VBUFFER_SIZE    1024
#define POLYEND_SIZE    512

/*-----------------------------------------------------------------*/
/* The following type definitions are used to build & store the database
   internally.  For some renderers, you need to build the data file according
   to a particular scheme (notably RTrace).  These data types are used to
   hold the objects, lights, etc. until it is time to build the file.  */

/* Forward declaration of a pointer to a generic object */
typedef struct object_struct *object_ptr;

/* Type definition for holding a light */
typedef struct light_struct *light_ptr;
struct light_struct {
   COORD4 center_pt;
   light_ptr next;
   };

/* Type definition for holding a surface declaration */
typedef struct surface_struct *surface_ptr;
struct surface_struct {
   char *surf_name;
   unsigned int surf_index;
   COORD3 color;
   double ka, kd, ks, ks_spec, ang, kt, ior;
   surface_ptr next;
   };

/* Diagonally opposite corners of a box */
struct box_struct {
   COORD3 point1, point2;
   };

/* Base point/radius and Apex point/radius */
struct cone_struct {
   COORD4 apex_pt, base_pt;
   };

/* Center, normal, inner, and outer radii of a annulus */
struct disc_struct {
   COORD3 center, normal;
   double iradius, oradius;
   };

/* 2D gridded data for a height field.  Limited support for this thing... */
struct height_struct {
   char *filename;
   float **data;
   unsigned int width, height;
   float x0, x1, y0, y1, z0, z1;
   };

/* Polygon - # of vertices and the 3D coordinates of the vertices themselves */
struct polygon_struct {
   unsigned int tot_vert;
   COORD3 *vert;
   };

/* Smooth patch.  Vertices and normals associated with them. */
struct polypatch_struct {
   unsigned int tot_vert;
   COORD3 *vert, *norm;
   };

/* Center/radius of a sphere */
struct sphere_struct {
   COORD4 center_pt;
   };

/* Center, axis lengths, and exponents of a superquadric */
struct superq_struct {
   COORD3 center_pt;
   double a1, a2, a3, n, e;
   };

/* Center, direction of axis of symmetry, inner, and outer radii of a torus */
struct torus_struct {
   COORD3 center, normal;
   double iradius, oradius;
   };

/* Define a NURB patch */
struct nurb_struct {
   int rat_flag;              /* Does this patch have any rational vertices */
   int norder, npts, nknots;
   int morder, mpts, mknots;
   float *nknotvec, *mknotvec;
   COORD4 **ctlpts;
   };

#define CSG_UNION        1
#define CSG_INTERSECTION 2
#define CSG_DIFFERENCE   3

/* Operator, list operand objects */
struct csg_struct {
   int csg_operation, object_count;
   object_ptr objects;
   };

/* Standard viewpoint stuff */
typedef struct {
   COORD3 from, at, up;
   double angle, aspect, hither, dist;
   int resx, resy;
   MATRIX tx;
   } viewpoint;

/*-----------------------------------------------------------------*/
#define BOX_OBJ        1
#define CONE_OBJ       2
#define DISC_OBJ       3
#define HEIGHT_OBJ     4
#define POLYGON_OBJ    5
#define POLYPATCH_OBJ  6
#define SPHERE_OBJ     7
#define SUPERQ_OBJ     8
#define TORUS_OBJ      9
#define NURB_OBJ      10
#define CSG_OBJ       11

/* Union of all the object types */
struct object_struct {
   unsigned int object_type;  /* Identify what kind of object */
   unsigned int curve_format; /* Output as surface or as polygons? */
   unsigned int surf_index;   /* Which surface was associated with this object? */
   MATRIX *tx;                /* Was there a matrix associated with this object? */
   union {
      struct box_struct       box;
      struct cone_struct      cone;
      struct disc_struct      disc;
      struct height_struct    height;
      struct polygon_struct   polygon;
      struct polypatch_struct polypatch;
      struct sphere_struct    sphere;
      struct superq_struct    superq;
      struct torus_struct     torus;
      struct nurb_struct      nurb;
      struct csg_struct       csg;
      } object_data;
   object_ptr next_object;
   };

/*-----------------------------------------------------------------*/
/* Global variables - lib.h */
/*-----------------------------------------------------------------*/
#ifdef OUTPUT_TO_FILE
extern FILE * gStdout_file;
#else
#define gStdout_file stdout
#endif /* OUTPUT_TO_FILE */

#define MAX_OUTFILE_NAME_SIZE  80
/*
Here are some local variables that are used to control things like
the current output file, current texture, ...
*/
extern FILE *gOutfile;
extern char gOutfileName[MAX_OUTFILE_NAME_SIZE];
extern char *gTexture_name;
extern int  gTexture_count;
extern double gTexture_ior;
extern int  gObject_count;
extern int  gRT_out_format;
extern int  gRT_orig_format;
extern int  gU_resolution;
extern int  gV_resolution;
extern COORD3 gBkgnd_color;
extern COORD3 gFgnd_color;
extern double gView_bounds[2][3];
extern int gView_init_flag;
extern char *gLib_version_str;

extern surface_ptr gLib_surfaces;
extern object_ptr gLib_objects;
extern light_ptr gLib_lights;
extern viewpoint gViewpoint;

/* Globals for tracking indentation level of output file */
extern int      gTab_width;
extern int      gTab_level;

/*-----------------------------------------------------------------*/
/* Global variables - libply.h */
/*-----------------------------------------------------------------*/
/* Polygon stack for making PLG files */
extern object_ptr gPolygon_stack;
extern unsigned long gVertex_count; /* Vertex coordinates */
extern unsigned long gNormal_count; /* Vertex normals */

/* Storage for polygon indices */
extern unsigned int *gPoly_vbuffer;
extern int *gPoly_end;

/* Globals to determine which axes can be used to split the polygon */
extern int gPoly_Axis1;
extern int gPoly_Axis2;


/*-----------------------------------------------------------------*/
/* Function Prototypes */
/*-----------------------------------------------------------------*/

/*==== Prototypes from libinf.c ====*/

void    tab_indent PARAMS((void));
void    tab_inc PARAMS((void));
void    tab_dec PARAMS((void));
char *  lib_get_version_str PARAMS((void));
void    lib_set_output_file PARAMS((FILE *new_outfile));
void    lib_set_default_texture PARAMS((char *default_texture));
void    lib_set_raytracer PARAMS((int default_tracer));
void    lib_set_polygonalization PARAMS((int u_steps, int v_steps));
void    lookup_surface_stats PARAMS((int index, int *tcount, double *tior));


/*==== Prototypes from libpr1.c ====*/

void lib_output_comment PARAMS((char *comment));

void lib_output_vector PARAMS((double x, double y, double z));

void axis_to_z PARAMS((COORD3 axis, double *xang, double *yang));

/*
 * Output viewpoint location.  The parameters are:
 *   From:   The eye location.
 *   At:     A position to be at the center of the image.  A.k.a. "lookat"
 *   Up:     A vector defining which direction is up.
 *   Fov:    Vertical field of view of the camera
 *   Aspect: Aspect ratio of horizontal fov to vertical fov
 *   Hither: Minimum distance to any ray-surface intersection
 *   Resx:   X resolution of resulting image
 *   Resy:   Y resolution of resulting image
 *
 * For all databases some viewing parameters are always the same:
 *
 *   Viewing angle is defined as from the center of top pixel row to bottom
 *     pixel row and left column to right column.
 *   Yon is "at infinity."
 */
void lib_output_viewpoint PARAMS((COORD3 from, COORD3 at, COORD3 up,
								 double fov_angle, double aspect_ratio,
								 double hither, int resx, int resy));


/*
 * Output light.  A light is defined by position.  All lights have the same
 * intensity.
 *
 */
void lib_output_light PARAMS((COORD4 center_pt));


/*-----------------------------------------------------------------*/
/*
 * Output background color.  A color is simply RGB (monitor dependent, but
 * that's life).
 * NOTE: Do this BEFORE lib_output_viewpoint(), for display_init()
 */
void lib_output_background_color PARAMS((COORD3 color));


/*-----------------------------------------------------------------*/
/*
 * Output color and shading parameters for all following objects
 *
 * For POV-Ray and Polyray, a character string will be returned that
 * identified this texture.  The default texture will be updated with
 * the name generated by this function.
 *
 * Meaning of the color and shading parameters:
 *    name      = name that this surface can be referenced by...
 *    color     = surface color
 *    ka        = ambient component
 *    kd        = diffuse component
 *    ks        = amount contributed from the reflected direction
 *    ks_spec   = contribution from specular highlights (vs. reflection)
 *    ang       = half angle, in degrees, at which specular highlight
 *                drops to 50% strength
 *    t         = amount contributed from the refracted direction
 *    i_of_r    = index of refraction of the surface
 *
 */
char *lib_output_color PARAMS((char *name, COORD3 color, double ka,
							  double kd, double ks, double ks_spec,
							  double ang, double kt, double i_of_r));



/*==== Prototypes from libpr2.c ====*/

/*-----------------------------------------------------------------*/
/*
 * Output cylinder or cone.  A cylinder is defined as having a radius and an
 * axis defined by two points, which also define the top and bottom edge of the
 * cylinder.  A cone is defined similarly, the difference being that the apex
 * and base radii are different.  The apex radius is defined as being smaller
 * than the base radius.  Note that the surface exists without endcaps.
 *
 * If format=OUTPUT_CURVES, output the cylinder/cone in format:
 *     "c"
 *     base.x base.y base.z base_radius
 *     apex.x apex.y apex.z apex_radius
 *
 * If the format=OUTPUT_POLYGONS, the surface is polygonalized and output.
 * (4*OUTPUT_RESOLUTION) polygons are output as rectangles by
 * lib_output_polypatch.
 */
void lib_output_cylcone PARAMS((COORD4 base_pt, COORD4 apex_pt,
							   int curve_format));


/*-----------------------------------------------------------------*/
void lib_output_disc PARAMS((COORD3 center, COORD3 normal,
							double iradius, double oradius,
							int curve_format));


/*-----------------------------------------------------------------*/
/*
 * Output sphere.  A sphere is defined by a radius and center position.
 *
 * If format=OUTPUT_CURVES, output the sphere in format:
 *     "s" center.x center.y center.z radius
 *
 * If the format=OUTPUT_POLYGONS, the sphere is polygonalized and output.
 * The sphere is polygonalized by splitting it into 6 faces (of a cube
 * projected onto the sphere) and dividing these faces by equally spaced
 * great circles.  OUTPUT_RESOLUTION affects the number of great circles.
 * (6*2*u_resolution*v_resolution) polygons are output as triangles
 * using lib_output_polypatch.
 */
void lib_output_sphere PARAMS((COORD4 center_pt, int curve_format));



/*-----------------------------------------------------------------*/
/* Output box.  A box is defined by a diagonally opposite corners. */
void lib_output_box PARAMS((COORD3 point1, COORD3 point2));


/*-----------------------------------------------------------------*/
void lib_output_sq_sphere PARAMS((COORD4 center_pt, double a1, double a2,
								 double a3, double n, double e,
								 int curve_format));


/*==== Prototypes from libpr3.c ====*/

/*-----------------------------------------------------------------*/
void lib_output_height PARAMS((char *, float **, int, int,
							  double, double, double, double, double, double));


/*-----------------------------------------------------------------*/
void lib_output_torus PARAMS((COORD3 center, COORD3 normal,
							 double iradius, double oradius,
							 int curve_format));
/*-----------------------------------------------------------------*/
void
lib_output_nurb PARAMS((int norder, int npts, int morder, int mpts,
					   float *nknots, float *mknots, COORD4 **ctlpts,
					   int curve_format));


/*==== Prototypes from libini.c ====*/

int     lib_open PARAMS((int raytracer_format, char *filename));

void    lib_close PARAMS((void));

void    lib_storage_initialize PARAMS((void));

void    lib_storage_shutdown PARAMS((void));

void    show_gen_usage PARAMS((void));
void    show_read_usage PARAMS((void));

int     lib_gen_get_opts PARAMS((int argc, char *argv[],
				 int *p_size, int *p_rdr, int *p_curve));
int     lib_read_get_opts PARAMS((int argc, char *argv[],
				  int *p_rdr, int *p_curve, char *p_infname));

void    lib_clear_database PARAMS((void));
void    lib_flush_definitions PARAMS((void));


/*==== Prototypes from libply.c ====*/

void    lib_output_polygon_cylcone PARAMS((COORD4 base_pt, COORD4 apex_pt));
void    lib_output_polygon_disc PARAMS((COORD3 center, COORD3 normal,
									   double iradius, double oradius));
void    lib_output_polygon_sphere PARAMS((COORD4 center_pt));
void    lib_output_polygon_height PARAMS((int height, int width, float **data,
										 double x0, double x1,
										 double y0, double y1,
										 double z0, double z1));
void    lib_output_polygon_torus PARAMS((COORD3 center, COORD3 normal,
										double iradius, double oradius));
void    lib_output_polygon_box PARAMS((COORD3 p1, COORD3 p2));
void    lib_output_polygon PARAMS((int tot_vert, COORD3 vert[]));
void    lib_output_polypatch PARAMS((int tot_vert, COORD3 vert[], COORD3 norm[]));


/*==== Prototypes from libdmp.c ====*/

void    dump_plg_file PARAMS((void));
void    dump_obj_file PARAMS((void));
void    dump_all_objects PARAMS((void));
void    dump_reorder_surfaces PARAMS((void));
void    dump_all_lights PARAMS((void));
void    dump_all_surfaces PARAMS((void));

/*==== Prototypes from libtx.c ====*/
#define U_SCALEX   0
#define U_SCALEY   1
#define U_SCALEZ   2
#define U_SHEARXY  3
#define U_SHEARXZ  4
#define U_SHEARYZ  5
#define U_ROTATEX  6
#define U_ROTATEY  7
#define U_ROTATEZ  8
#define U_TRANSX   9
#define U_TRANSY  10
#define U_TRANSZ  11
#define U_PERSPX  12
#define U_PERSPY  13
#define U_PERSPZ  14
#define U_PERSPW  15


int lib_tx_active PARAMS((void));         /* Is a transform active? */
void lib_get_current_tx PARAMS((MATRIX)); /* Get the current transform */
void lib_set_current_tx PARAMS((MATRIX)); /* Replace the current transform */
void lib_output_tx_sequence PARAMS((void)); /* Write transform */
void lib_tx_pop PARAMS((void));           /* Pop off the top transform */
void lib_tx_push PARAMS((void));          /* Push the current transform */
void lib_tx_rotate PARAMS((int, double)); /* Rotate about a single axis (radians) */
void lib_tx_scale PARAMS((COORD3));       /* Scale on each axis */
void lib_tx_translate PARAMS((COORD3));   /* Add a translation */
int lib_tx_unwind PARAMS((MATRIX, double *)); /* Turn tx into rotate/scale/translate */
extern MATRIX IdentityTx; /* Identity matrix.  Don't write into this! */

#if __cplusplus
}
#endif

#endif /* LIB_H */

