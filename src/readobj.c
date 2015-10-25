/*
 * readobj.c - Read polygons from a Wavefront OBJ file and display them.
 * The file "view.dat" is processed for the camera view that will be used.
 *
 * Author:  Alexander Enzmann
 *
 * size_factor is ignored.
 *
 * This code was extracted from the Polyray raytracer.  Currently it will only
 * read polygons, with or without associated vertex normals.  The u/v
 * coordinates are read, but not used in this code.  Groups (including smoothing
 * groups) are ignored.  All of the spline patch types are ignored.
 *
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

#define MAXTRILINE 512

/* Data structure for a stack of vertices */
typedef struct VecVerts_struct VecVerts;
struct VecVerts_struct {
	float V[4];
	VecVerts *next;
};

/* Data structure for a stack of faces */
typedef struct Face_struct Faces;
struct Face_struct {
	int vcount;
	long *verts, *tverts, *nverts;
	/* Texture *texture; */
	Faces *next;
};

typedef struct triverts_struct triverts;
struct triverts_struct {
	COORD3 V[3], N[3], U[3];
	/* Texture *texture; */
	triverts *next;
};

/* Stack of raw triangle collections.  Each collection is associated
   with a single texture name.  (currently unused) */
typedef struct trivertstack_struct trivstack;
struct trivertstack_struct {
	/* Texture *texture; */  /* Texture to apply to all triangles */
	triverts *verts;    /* List of triangles */
	Faces *fstack;      /* Stack of face indices */
	long tcount;        /* Number of triangles in this raw object */
	int nflag;          /* Are there vertex normals? */
	int uvflag;         /* Is there u/v information for the vertices? */
	trivstack *next;    /* Next bag of triangles */
};

/* Structure to hold a smoothing group */
typedef struct SmoothGroup_struct SmoothGroup;
struct SmoothGroup_struct {
	unsigned group_num;
	Faces *faces;
	SmoothGroup *next;
};

#define BETWEEN_OBJECTS  0
#define READING_VERTICES 1
#define READING_FACES    2

#define MAX_VERTICES_PER_FACE 32

static long vertex_count = 0;
static long vertex_texture_count = 0;
static long vertex_normal_count = 0;
static long face_count = 0;
/* static Texture *current_texture; */

static char rbuf[MAXTRILINE];
static int rbuf_offset = 0;
static int rbuf_length = 0;

static int
skip_white_space(filep)
FILE *filep;
{
	for (;
		(rbuf[rbuf_offset] == ' ' ||
			rbuf[rbuf_offset] == '\t' ||
			rbuf[rbuf_offset] == '\\') &&
			rbuf_offset < rbuf_length;
		rbuf_offset++) {

		if (rbuf[rbuf_offset] == '\\') {
			/* Continuation character, get the next line */
			if (fgets(rbuf, MAXTRILINE, filep) != NULL) {
				rbuf_offset = 0;
				rbuf_length = strlen(rbuf);
			}
			else {
				rbuf[0] = '\0';
				rbuf_offset = 0;
				rbuf_length = 0;
				return 0;
			}
		}
		else {
			/* White space, just ignore it */
		}
	}
	return 1;
}

static void
skip_till_white_space(filep)
FILE *filep;
{
	for (;
		rbuf[rbuf_offset] != ' ' &&
			rbuf[rbuf_offset] != '\t' &&
			rbuf_offset < rbuf_length;
		rbuf_offset++)
		;
}

static int
end_of_line()
{
	if (rbuf_offset == rbuf_length ||
		rbuf[rbuf_offset] == '\n' ||
		rbuf[rbuf_offset] == '\0')
		return 1;
	else
		return 0;
}

static int
read_vertex(filep, v, vt, vn)
FILE *filep;
long *v, *vt, *vn;
{
	float v0, vt0, vn0;
	
	skip_white_space(filep);
	if (sscanf(&rbuf[rbuf_offset], "%g/%g/%g", &v0, &vt0, &vn0) == 3) {
		*v  = (long)v0;
		*vt = (long)vt0;
		*vn = (long)vn0;
	}
	else if (sscanf(&rbuf[rbuf_offset], "%g//%g", &v0, &vn0) == 2) {
		*v  = (long)v0;
		*vt = (long)0L;
		*vn = (long)vn0;
	}
	else if (sscanf(&rbuf[rbuf_offset], "%g/%g", &v0, &vt0) == 2) {
		*v  = (long)v0;
		*vt = (long)vt0;
		*vn = (long)0L;
	}
	else if (sscanf(&rbuf[rbuf_offset], "%g", &v0) == 1) {
		*v  = (long)v0;
		*vt = 0L;
		*vn = 0L;
	}
	else {
		fprintf(stderr, "Bad vertex data\n");
		exit(1);
	}
	skip_till_white_space(filep);
	return 1;
}

static Faces *
read_face(filep)
FILE *filep;
{
	Faces *face;
	int i, vcount, vtp_flag, vnp_flag;
	long v[MAX_VERTICES_PER_FACE], *vp;
	long vt[MAX_VERTICES_PER_FACE], *vtp;
	long vn[MAX_VERTICES_PER_FACE], *vnp;
	
	vp = &v[0];
	vtp = &vt[0];
	vnp = &vn[0];
	vtp_flag = 0;
	vnp_flag = 0;
	for (vcount=0;
	vcount<MAX_VERTICES_PER_FACE && !end_of_line();
	vcount++,vp++,vtp++,vnp++) {
		read_vertex(filep, vp, vtp, vnp);
		if (*vtp != 0)
			vtp_flag = 1;
		if (*vnp != 0)
			vnp_flag = 1;
		skip_white_space(filep);
	}
	if (vcount >= MAX_VERTICES_PER_FACE)
		fprintf(stderr, "Too many vertices in a face");
	
	face = malloc(sizeof(Faces));
	face->verts = malloc(vcount * sizeof(long));
	if (vtp_flag)
		face->tverts = malloc(vcount * sizeof(long));
	else
		face->tverts = NULL;
	if (vnp_flag)
		face->nverts = malloc(vcount * sizeof(long));
	else
		face->nverts = NULL;
	face->vcount = vcount;
	for (i=0;i<vcount;i++) {
		if (v[i] > 0)
			face->verts[i] = v[i] - 1;
		else
			face->verts[i] = vertex_count - vt[i];
		if (vtp_flag) {
			if (vt[i] > 0)
				face->tverts[i] = vt[i] - 1;
			else
				face->tverts[i] = vertex_texture_count - vt[i];
		}
		if (vnp_flag) {
			if (vn[i] > 0)
				face->nverts[i] = vn[i] - 1;
			else
				face->nverts[i] = vertex_normal_count - vn[i];
		}
	}
	face->next = NULL;
	return face;
}

static void
make_triangles(vcount, normal_count, fstack,
			   vstack, nstack)
			   long vcount, normal_count;
Faces *fstack;
VecVerts *vstack, *nstack;
{
	COORD3 *V, *N;
	long tcnt;
	VecVerts *vtemp;
	Faces *ftemp1, *ftemp2;
	
	/* Now we need to allocate space for the vertices and process
	the face stacks into a set of triangles */
	V = (COORD3 *)malloc(vcount * sizeof(COORD3));
	if (normal_count > 0)
		N = (COORD3 *)malloc(normal_count * sizeof(COORD3));
	else
		N = NULL;
	
	/* Copy the vertices into the V array */
	for (tcnt=vcount-1;vstack!=NULL&&tcnt>=0;tcnt--) {
		/* Copy this vertex into the array */
		COPY_COORD3(V[tcnt], vstack->V);
		/* Free up the space used for this vertex */
		vtemp = vstack;
		vstack = vstack->next;
		free(vtemp);
	}
	if (tcnt != -1 || vstack != NULL) {
		fprintf(stderr, "Didn't properly process .obj vertices");
		exit(1);
	}
	
	/* Copy the normals into the N array */
	for (tcnt=normal_count-1;nstack!=NULL&&tcnt>=0;tcnt--) {
		/* Copy this vertex into the array */
		COPY_COORD3(N[tcnt], nstack->V);
		/* Free up the space used for this vertex */
		vtemp = nstack;
		nstack = nstack->next;
		free(vtemp);
	}
	if (tcnt != -1 || nstack != NULL) {
		fprintf(stderr, "Didn't properly process .obj normals");
		exit(1);
	}
	
	/* Create triangles in the form we want them */
	for (ftemp1=fstack,tcnt=0;ftemp1!=NULL;tcnt++) {
	    /* We need to turn the face into a set of triangles and
		   stuff each one onto the stack */
		COORD3 *verts, *norms;
		int j, npoints;
		
		/* Allocate temporary space to hold the polygon (yes,
		   I know this loop thrashes malloc(), but that's
		   just tough. */
		npoints = ftemp1->vcount;
		verts = (COORD3 *)malloc(npoints * sizeof(COORD3));
		if (ftemp1->nverts != NULL)
			norms = (COORD3 *)malloc(npoints * sizeof(COORD3));
		else
			norms = NULL;
		
		/* Stuff the vertices of the polygon into the array
		   verts for subsequent chopping. */
		for (j=0;j<npoints;j++) {
			COPY_COORD3(verts[j], V[ftemp1->verts[j]]);
			if (norms != NULL)
				COPY_COORD3(norms[j], N[ftemp1->nverts[j]]);
		}
		if (norms != NULL)
			lib_output_polypatch(npoints, verts, norms);
		else
			lib_output_polygon(npoints, verts);
		
		/* Free the temporary polygon storage */
		free(verts);
		if (norms != NULL) free(norms);
		
		/* Dispose of the ones we just looked at */
		ftemp2 = ftemp1;
		ftemp1 = ftemp1->next;
		free(ftemp2->verts);
		if (ftemp2->tverts) free(ftemp2->tverts);
		if (ftemp2->nverts) free(ftemp2->nverts);
		free(ftemp2);
	}
}

static VecVerts *
new_vecvert(x, y, z, w)
float x, y, z, w;
{
	VecVerts *vert;
	vert = malloc(sizeof(VecVerts));
	vert->V[0] = (float)x;
	vert->V[1] = (float)y;
	vert->V[2] = (float)z;
	vert->V[3] = (float)w;
	vert->next = NULL;
	return vert;
}

static int
read_obj_faces(filep)
FILE *filep;
{
	char ctype[MAXTRILINE], tbuf1[MAXTRILINE], tbuf2[MAXTRILINE];
	float v0, v1, v2, v3;
	int icnt;
	VecVerts *vstack, *nstack, *vtemp;
	Faces *fstack, *ftemp1;
	
	fseek(filep, 0, SEEK_SET);
	
	vstack = NULL;
	nstack = NULL;
	fstack = NULL;
	vertex_count = 0;
	vertex_texture_count = 0;
	vertex_normal_count = 0;
	face_count = 0;
	
	/* Read the entire file, processing triangles as we go. */
	while(TRUE){
		if (fgets(rbuf, MAXTRILINE, filep) == NULL)
			break;
		/* First read in the command for this line */
		icnt = sscanf(rbuf, "%s", ctype);
		rbuf_offset = strlen(ctype);
		rbuf_length = strlen(rbuf);
		
		/* Looking for a statement like: "v x y z w" */
		if (!strcmp(ctype, "v")) {
			/* Read a vertex */
			icnt = sscanf(rbuf, "%s %g %g %g %g", tbuf1, &v0, &v1, &v2, &v3);
			if (icnt == 4 || icnt == 5) {
				/* Valid vertex */
				vtemp = new_vecvert(v0, v1, v2, (icnt == 4 ? 0.0 : v3));
				vtemp->next = vstack;
				vstack = vtemp;
				vertex_count++;
			}
			else
				fprintf(stderr, "Bad vertex\n");
			continue;
		}
		
		/* Looking for a statement like: "vn x y z" */
		if (!strcmp(ctype, "vn")) {
			/* Read a vertex */
			icnt = sscanf(rbuf, "%s %g %g %g", tbuf1, &v0, &v1, &v2);
			if (icnt == 4) {
				/* Valid vertex */
				vtemp = new_vecvert(v0, v1, v2, 0.0 );
				vtemp->next = nstack;
				nstack = vtemp;
				vertex_normal_count++;
			}
			else
				fprintf(stderr, "Bad normal\n");
			continue;
		}
		
		/* Looking for a statement like: "vt u v w" */
		if (!strcmp(ctype, "vt")) {
			/* Read a vertex */
			icnt = sscanf(rbuf, "%s %g %g %g", tbuf1, &v0, &v1, &v2);
			/* For now we are ignoring texture coordinates */
			continue;
		}
		
		/* Look for: "usemtl texture_name" */
		if (!strcmp(ctype, "usemtl")) {
			icnt = sscanf(rbuf, "%s %s", tbuf1, tbuf2);
			if (icnt == 2) {
				/* Got a texture name, do nothing for now */
			}
			else
				fprintf(stderr, "Bad texture (usemtl) name\n");
			continue;
		}
		
		if (!strcmp(ctype, "f")) {
			/* Read a face */
			ftemp1 = read_face(filep);
			if (ftemp1 != NULL) {
				/* ftemp1->texture = current_texture; */
				ftemp1->next = fstack;
				fstack = ftemp1;
				face_count++;
			}
			else
				fprintf(stderr, "Bad face\n");
			continue;
		}
	}

	/* Turn the contents of the face stack into triangle objects.  This
       routine removes the memory associated with tristack. */
	make_triangles(vertex_count, vertex_normal_count,
		fstack, vstack, nstack);
	
	return face_count;
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

/* Read in the camera view, then read in OBJ polygons, then display them. */
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
	
    PLATFORM_INIT(SPD_READOBJ);
	
    /* Start by defining which raytracer we will be using */
    if ( lib_read_get_opts( argc, argv,
		&raytracer_format, &output_format, file_name ) ) {
		return EXIT_FAIL;
    }
    if ( lib_open( raytracer_format, "ReadOBJ" ) ) {
		return EXIT_FAIL;
    }
	
    file = fopen(file_name, "r");
    if (file == NULL) {
		fprintf(stderr, "Cannot open obj file: '%s'\n", file_name);
		return EXIT_FAIL;
    }
	
    /*lib_set_polygonalization(3, 3);*/
	
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
	
    read_obj_faces(file);
	
    fclose(file);
	
    lib_close();
	
    PLATFORM_SHUTDOWN();
    return EXIT_SUCCESS;
}
