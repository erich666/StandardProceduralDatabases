/*
 * ReadNFF.c - Simple NFF file importer.  Uses lib to output to
 * many different raytracer formats.
 *
 * Author:  Eduard [esp] Schwan
 *
 * input file parameter...
 */

#include <stdio.h>
#include <math.h>
#include <string.h>	/* strchr */
#include <stdlib.h>	/* atoi */
#include "def.h"
#include "drv.h"	/* display_close() */
#include "lib.h"

/* These may be read from the command line */
static int raytracer_format = OUTPUT_RT_DEFAULT;
static int output_format    = OUTPUT_CURVES;


/*----------------------------------------------------------------------
Handle an error
----------------------------------------------------------------------*/
static void
show_error(s)
char	* s;
{
    /* SysBeep(1); */
    lib_output_comment("### ERROR! ###\n");
    lib_output_comment(s);
    lib_close();
}

/*----------------------------------------------------------------------
Comment.  Description:
    "#" [ string ]

Format:
    # [ string ]

    As soon as a "#" character is detected, the rest of the line is considered
    a comment.
----------------------------------------------------------------------*/
static void
do_comment(fp)
FILE *fp;
{
    char	*cp;
    char	comment[256];
	
    fgets(comment, 255, fp);
    /* strip out newline */
    cp = (char*)strchr(comment, '\n');
    if (cp != NULL)
		*cp = '\0';
    lib_output_comment(comment);
}


/*----------------------------------------------------------------------
Viewpoint location.  Description:
    "v"
    "from" Fx Fy Fz
    "at" Ax Ay Az
    "up" Ux Uy Uz
    "angle" angle
    "hither" hither
    "resolution" xres yres

Format:

    v
    from %g %g %g
    at %g %g %g
    up %g %g %g
    angle %g
    hither %g
    resolution %d %d

The parameters are:

    From:  the eye location in XYZ.
    At:    a position to be at the center of the image, in XYZ world
	   coordinates.  A.k.a. "lookat".
    Up:    a vector defining which direction is up, as an XYZ vector.
    Angle: in degrees, defined as from the center of top pixel row to
	   bottom pixel row and left column to right column.
    Resolution: in pixels, in x and in y.

  Note that no assumptions are made about normalizing the data (e.g. the
  from-at distance does not have to be 1).  Also, vectors are not
  required to be perpendicular to each other.

  For all databases some viewing parameters are always the same:
    Yon is "at infinity."
    Aspect ratio is 1.0.

  A view entity must be defined before any objects are defined (this
  requirement is so that NFF files can be used by hidden surface machines).
----------------------------------------------------------------------*/
static void
do_view(fp)
FILE *fp;
{
    float    x,y,z;
    COORD3 from;
    COORD3 at;
    COORD3 up;
    float fov_angle;
    float aspect_ratio;
    float hither;
    int resx;
    int resy;
	
    if (fscanf(fp, " from %f %f %f", &x, &y, &z) != 3)
		goto fmterr;
    SET_COORD3(from, x,y,z);
	
    if (fscanf(fp, " at %f %f %f", &x, &y, &z) != 3)
		goto fmterr;
    SET_COORD3(at, x,y,z);
	
    if (fscanf(fp, " up %f %f %f", &x, &y, &z) != 3)
		goto fmterr;
    SET_COORD3(up, x,y,z);
	
    if (fscanf(fp, " angle %f", &fov_angle) != 1)
		goto fmterr;
	
    fscanf(fp, " hither %f", &hither);
	
    aspect_ratio = (float)1.0;
	
    fscanf(fp, " resolution %d %d", &resx, &resy);
	
    lib_output_viewpoint(from, at, up,
		fov_angle, aspect_ratio,
		hither, resx, resy);
    return;
fmterr:
    show_error("NFF view syntax error");
    exit(1);
}


/*----------------------------------------------------------------------
Positional light.  A light is defined by XYZ position.  Description:
    "l" X Y Z

Format:
    l %g %g %g

    All light entities must be defined before any objects are defined (this
    requirement is so that NFF files can be used by hidden surface machines).
    Lights have a non-zero intensity of no particular value [this definition
    may change soon, with the addition of an intensity and/or color].
----------------------------------------------------------------------*/
static void
do_light(fp)
FILE *fp;
{
    float    x, y, z;
    COORD4 acenter;
	
    if (fscanf(fp, "%f %f %f",&x, &y, &z) != 3) {
		show_error("Light source syntax error");
		exit(1);
    }
	
    SET_COORD4(acenter,x,y,z,0.0); /* intensity=0 */
	
    lib_output_light(acenter);
}


/*----------------------------------------------------------------------
Background color.  A color is simply RGB with values between 0 and 1:
    "b" R G B

Format:
    b %g %g %g

    If no background color is set, assume RGB = {0,0,0}.
----------------------------------------------------------------------*/
static void
do_background(fp)
FILE *fp;
{
    float    r, g, b;
    COORD3 acolor;
	
    if (fscanf(fp, "%f %f %f",&r, &g, &b) != 3) {
		show_error("background color syntax error");
		exit(1);
    }
    SET_COORD3(acolor,r,g,b);
	
    lib_output_background_color(acolor);
}


/*----------------------------------------------------------------------
Fill color and shading parameters.  Description:
     "f" red green blue Kd Ks Shine T index_of_refraction

Format:
    f %g %g %g %g %g %g %g %g

    RGB is in terms of 0.0 to 1.0.

    Kd is the diffuse component, Ks the specular, Shine is the Phong cosine
    power for highlights, T is transmittance (fraction of light passed per
    unit).  Usually, 0 <= Kd <= 1 and 0 <= Ks <= 1, though it is not required
    that Kd + Ks == 1.  Note that transmitting objects ( T > 0 ) are considered
    to have two sides for algorithms that need these (normally objects have
    one side).

    The fill color is used to color the objects following it until a new color
    is assigned.
----------------------------------------------------------------------*/
static void
do_fill(fp)
FILE *fp;
{
    float    r, g, b, ka, kd, ks, ks_spec, phong_pow, ang, t, ior;
    COORD3 acolor;
	
    if (fscanf(fp, "%f %f %f",&r, &g, &b) != 3) {
		show_error("fill color syntax error");
		exit(1);
    }
    SET_COORD3(acolor,r,g,b);
	
    if (fscanf(fp, "%f %f %f %f %f", &kd, &ks, &phong_pow, &t, &ior) != 5) {
		show_error("fill material syntax error");
		exit(1);
    }
	
    /* some parms not input in NFF, so hard-coded. */
    ka = (float)0.1;
    ks_spec = ks;
    /* convert phong_pow back into phong hilight angle. */
    /* reciprocal of formula in libpr1.c, lib_output_color() */
	if ( phong_pow < 1.0 )
		phong_pow = 1.0 ;
    ang = (float)((180.0/PI) * acos( exp(log(0.5)/phong_pow) ));
    lib_output_color(NULL, acolor, ka, kd, ks, ks_spec, ang, t, ior);
	
}


/*----------------------------------------------------------------------
Cylinder or cone.  A cylinder is defined as having a radius and an axis
    defined by two points, which also define the top and bottom edge of the
    cylinder.  A cone is defined similarly, the difference being that the apex
    and base radii are different.  The apex radius is defined as being smaller
    than the base radius.  Note that the surface exists without endcaps.  The
    cone or cylinder description:

    "c"
    base.x base.y base.z base_radius
    apex.x apex.y apex.z apex_radius

Format:
    c
    %g %g %g %g
    %g %g %g %g

    A negative value for both radii means that only the inside of the object is
    visible (objects are normally considered one sided, with the outside
    visible).  Note that the base and apex cannot be coincident for a cylinder
    or cone.
----------------------------------------------------------------------*/
static void
do_cone(fp)
FILE *fp;
{
    COORD4    base_pt;
    COORD4    apex_pt;
    float    x0, y0, z0, x1, y1, z1, r0, r1;
	
    if (fscanf(fp, " %f %f %f %f %f %f %f %f", &x0, &y0, &z0, &r0,
		&x1, &y1, &z1, &r1) != 8) {
		show_error("cylinder or cone syntax error");
		exit(1);
    }
    if ( r0 < 0.0) {
		r0 = -r0;
		r1 = -r1;
    }
    SET_COORD4(base_pt,x0,y0,z0,r0);
    SET_COORD4(apex_pt,x1,y1,z1,r1);
	
    lib_output_cylcone (base_pt, apex_pt, output_format);
}


/*----------------------------------------------------------------------
Sphere.  A sphere is defined by a radius and center position:
    "s" center.x center.y center.z radius

Format:
    s %g %g %g %g

    If the radius is negative, then only the sphere's inside is visible
    (objects are normally considered one sided, with the outside visible).
----------------------------------------------------------------------*/
static void
do_sphere(fp)
FILE *fp;
{
    float    x, y, z, r;
    COORD4    center_pt;
	
    if (fscanf(fp, "%f %f %f %f", &x, &y, &z, &r) != 4) {
		show_error("sphere syntax error");
		exit(1);
    }
	
    SET_COORD4(center_pt,x,y,z,r);
	
    lib_output_sphere(center_pt, output_format);
}


/*----------------------------------------------------------------------
Polygon.  A polygon is defined by a set of vertices.  With these databases,
    a polygon is defined to have all points coplanar.  A polygon has only
    one side, with the order of the vertices being counterclockwise as you
    face the polygon (right-handed coordinate system).  The first two edges
    must form a non-zero convex angle, so that the normal and side visibility
    can be determined.  Description:

    "p" total_vertices
    vert1.x vert1.y vert1.z
    [etc. for total_vertices vertices]

Format:
    p %d
    [ %g %g %g ] <-- for total_vertices vertices
----------------------------------------------------------------------
Polygonal patch.  A patch is defined by a set of vertices and their normals.
    With these databases, a patch is defined to have all points coplanar.
    A patch has only one side, with the order of the vertices being
    counterclockwise as you face the patch (right-handed coordinate system).
    The first two edges must form a non-zero convex angle, so that the normal
    and side visibility can be determined.  Description:

    "pp" total_vertices
    vert1.x vert1.y vert1.z norm1.x norm1.y norm1.z
    [etc. for total_vertices vertices]

Format:
    pp %d
    [ %g %g %g %g %g %g ] <-- for total_vertices vertices
----------------------------------------------------------------------*/
static void
do_poly(fp)
FILE *fp;
{
    int    ispatch;
    int    nverts;
    int    vertcount;
    COORD3 *verts;
    COORD3 *norms;
    float    x, y, z;
	
    ispatch = getc(fp);
    if (ispatch != 'p') {
		ungetc(ispatch, fp);
		ispatch = 0;
    }
	
    if (fscanf(fp, "%d", &nverts) != 1)
		goto fmterr;
	
    verts = (COORD3*)malloc(nverts*sizeof(COORD3));
    if (verts == NULL)
		goto memerr;
	
    if (ispatch) {
		norms = (COORD3*)malloc(nverts*sizeof(COORD3));
		if (norms == NULL)
			goto memerr;
    }
	
    /* read all the vertices into temp array */
    for (vertcount = 0; vertcount < nverts; vertcount++) {
		if (fscanf(fp, " %f %f %f", &x, &y, &z) != 3)
			goto fmterr;
		SET_COORD3(verts[vertcount],x,y,z);
		
		if (ispatch) {
			if (fscanf(fp, " %f %f %f", &x, &y, &z) != 3)
				goto fmterr;
			SET_COORD3(norms[vertcount],x,y,z);
		}
    }
	
    /* write output */
    if (ispatch)
		lib_output_polypatch(nverts, verts, norms);
    else
		lib_output_polygon(nverts, verts);
	
    free(verts);
    if (ispatch)
		free(norms);
	
    return;
fmterr:
    show_error("polygon or patch syntax error");
    exit(1);
memerr:
    show_error("can't allocate memory for polygon or patch");
    exit(1);
}


/*----------------------------------------------------------------------
----------------------------------------------------------------------*/
static void
parse_nff(fp)
FILE *fp;
{
    int        c;
	
    while ( (c = getc(fp)) != EOF )
		switch (c) {
	case ' ':            /* white space */
	case '\t':
	case '\n':
	case '\f':
	case '\r':
		continue;
	case '#':            /* comment */
		do_comment(fp);
		break;
	case 'v':            /* view point */
		do_view(fp);
		break;
	case 'l':            /* light source */
		do_light(fp);
		break;
	case 'b':            /* background color */
		do_background(fp);
		break;
	case 'f':            /* fill material */
		do_fill(fp);
		break;
	case 'c':            /* cylinder or cone */
		do_cone(fp);
		break;
	case 's':            /* sphere */
		do_sphere(fp);
		break;
	case 'p':            /* polygon or patch */
		do_poly(fp);
		break;
	default:            /* unknown */
		show_error("unknown NFF primitive code");
		exit(1);
	}
} /* parse_nff */


/*----------------------------------------------------------------------
----------------------------------------------------------------------*/
int
main(argc,argv)
int argc ;
char *argv[] ;
{
    char file_name[256];
    FILE *fp;
	
    PLATFORM_INIT(SPD_READNFF);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_read_get_opts( argc, argv,
		&raytracer_format, &output_format, file_name ) ) {
		return EXIT_FAIL;
    }
	
    if ( lib_open( raytracer_format, "ReadNFF" ) ) {
		return EXIT_FAIL;
    }
	
    fp = fopen(file_name, "r");
    if (fp == NULL) {
		fprintf(stderr, "Cannot open nff file: '%s'\n", file_name);
		return EXIT_FAIL;
    }
	
    /*lib_set_polygonalization(3, 3);*/
	
    parse_nff(fp);
	
    fclose(fp);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
