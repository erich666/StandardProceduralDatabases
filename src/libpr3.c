/*
 * libpr3.c - a library of primitive object output routines, part 3 of 3.
 *            Height field & NURBS routines.
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


static unsigned int hfcount = 0;

/*-----------------------------------------------------------------*/\
/* data is between -1.0 and 1.0, for y heightfield */
#ifdef ANSI_FN_DEF
static char * create_height_file(char *filename, int height, int width, float **data, int type)
#else
static char * create_height_file(filename, height, width, data, type)
char *filename;
int height, width;
float **data;
int type;
#endif
{
    FILE *file;
    double v;
    unsigned int i, j;
    unsigned char r, g, b;
    unsigned char tgaheader[18];
	
    if (filename == NULL) {
		/* Need to create a new name for the height file */
		filename = malloc(10 * sizeof(char));
		if (filename == NULL) return NULL;
		sprintf(filename, "hf%03d.tga", hfcount++);
    }
    if ((file = fopen(filename, "wb")) == NULL)
		return NULL;

    if (type == 0) {
		/* Targa style height field for POV-Ray or Polyray */
		memset(tgaheader, 0, 18);
		tgaheader[2] = 2;
		tgaheader[12] = (unsigned char)(width & 0xFF);
		tgaheader[13] = (unsigned char)((width >> 8) & 0xFF);
		tgaheader[14] = (unsigned char)(height & 0xFF);
		tgaheader[15] = (unsigned char)((height >> 8) & 0xFF);
		tgaheader[16] = 24;
		tgaheader[17] = 0x20;
		fwrite(tgaheader, 18, 1, file);
		for (i=0;(int)i<height;i++) {
			PLATFORM_MULTITASK();
			for (j=0;(int)j<width;j++) {
				v = data[i][j]*128.0;
				if (v < -128.0) v = -128.0;
				if (v > 127.0) v = 127.0;
				v += 128.0;
				r = (unsigned char)v;
				v -= (float)r;
				g = (unsigned char)(256.0 * v);
				b = 0;
				fputc(b, file);
				fputc(g, file);
				fputc(r, file);
			}
		}
    } else {
		/* Only square height fields in RayShade */
		if (height < width) width = height;
		else if (width < height) height = width;
		
		/* Start by storing the size as an int */
		fwrite(&height, sizeof(int), 1, file);
		
		/* Now store height values as native floats */
		for (i=0;(int)i<height;i++) {
			PLATFORM_MULTITASK();
			for (j=0;(int)j<width;j++)
				fwrite(&data[i][j], sizeof(float), 1, file);
		}
    }
    fclose(file);
	
    return filename;
}


/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_output_height(char *filename, float **data, int height, int width,
					   double x0, double x1, double y0, double y1, double z0, double z1)
#else
					   void lib_output_height(filename, data, height, width, x0, x1, y0, y1, z0, z1)
					   char *filename;
float **data;
int height, width;
double x0, x1;
double y0, y1;
double z0, z1;
#endif
{
    MATRIX txmat;
    object_ptr new_object;
	
    if (gRT_out_format == OUTPUT_DELAYED) {
		filename = create_height_file(filename, height, width, data, 0);
		if (filename == NULL) return;
		
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;

		new_object->object_type  = HEIGHT_OBJ;
		new_object->curve_format = OUTPUT_CURVES;
		new_object->surf_index   = gTexture_count;
		if (lib_tx_active()) {
			lib_get_current_tx(txmat);
			new_object->tx = malloc(sizeof(MATRIX));
			if (new_object->tx == NULL)
				return;
			else
				memcpy(new_object->tx, txmat, sizeof(MATRIX));
		}
		else
			new_object->tx = NULL;

		new_object->object_data.height.width = width;
		new_object->object_data.height.height = height;
		new_object->object_data.height.data = data;
		new_object->object_data.height.filename = filename;
		new_object->object_data.height.x0 = (float)x0;
		new_object->object_data.height.x1 = (float)x1;
		new_object->object_data.height.y0 = (float)y0;
		new_object->object_data.height.y1 = (float)y1;
		new_object->object_data.height.z0 = (float)z0;
		new_object->object_data.height.z1 = (float)z1;
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    } else {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_NFF:
		case OUTPUT_PLG:
		case OUTPUT_OBJ:
		case OUTPUT_QRT:
		case OUTPUT_RTRACE:
		case OUTPUT_VIVID:
		case OUTPUT_RAWTRI:
		case OUTPUT_RIB:
		case OUTPUT_DXF:
		case OUTPUT_RWX:
		case OUTPUT_VRML1:
		case OUTPUT_VRML2:
			lib_output_polygon_height(height, width, data,
				x0, x1, y0, y1, z0, z1);
			break;
		case OUTPUT_POVRAY_10:
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
			filename = create_height_file(filename, height, width, data, 0);
			if (filename == NULL) return;
			
			tab_indent();
			fprintf(gOutfile, "object {\n");
			tab_inc();
			
			tab_indent();
			fprintf(gOutfile, "height_field { tga \"%s\" }", filename);
			if (gRT_out_format == OUTPUT_POVRAY_10) {
				tab_indent();
				fprintf(gOutfile, "scale <%g %g %g>\n",
					fabs(x1 - x0), fabs(y1 - y0), fabs(z1 - z0));
				tab_indent();
				fprintf(gOutfile, "translate <%g %g %g>\n", x0, y0, z0);
			} else {
				tab_indent();
				fprintf(gOutfile, "scale <%g, %g, %g>\n",
					fabs(x1 - x0), fabs(y1 - y0), fabs(z1 - z0));
				tab_indent();
				fprintf(gOutfile, "translate <%g, %g, %g>\n", x0, y0, z0);
			}
			
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			if (gTexture_name != NULL) {
				tab_indent();
				fprintf(gOutfile, "texture { %s }", gTexture_name);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "} // object - Height Field\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POLYRAY:
			filename = create_height_file(filename, height, width, data, 0);
			if (filename == NULL) return;
			tab_indent();
			fprintf(gOutfile, "object { height_field \"%s\" ", filename);
			fprintf(gOutfile, "scale <%g, %g, %g> ",
				fabs(x1-x0), fabs(y1-y0), fabs(z1-z0));
			fprintf(gOutfile, "translate <%g, %g, %g> ", x0, y0, z0);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RAYSHADE:
			filename = create_height_file(filename, height, width, data, 1);
			if (filename == NULL) return;
			fprintf(gOutfile, "heightfield ");
			if (gTexture_name != NULL)
				fprintf(gOutfile, "%s ", gTexture_name);
			fprintf(gOutfile, "%s ", filename);	/* some versions may need quotes? */
			fprintf(gOutfile, "rotate 1 0 0 90 ");
			fprintf(gOutfile, "scale  %g %g %g ",
				fabs(x1 - x0), fabs(y1 - y0), fabs(z1 - z0));
			fprintf(gOutfile, "translate  %g %g %g ", x0, y0, z0);
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_ART:
			filename = create_height_file(filename, height, width, data, 1);
			if (filename == NULL) return;
			
			tab_indent();
			fprintf(gOutfile, "geometry {\n");
			tab_inc();
			
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			tab_indent();
			fprintf(gOutfile, "translate(%g, %g, %g)\n", x0, y0, z0);
			tab_indent();
			fprintf(gOutfile, "scale(%g, 1, %g)\n",
				fabs(x1 - x0), fabs(z1 - z0));
			tab_indent();
			fprintf(gOutfile, "rotate(-90, x)\n");
			tab_indent();
			fprintf(gOutfile, "heightfield \"%s\"\n ", filename);
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_3DMF:
			fprintf(gOutfile, "# Heightfield - we should use trigrid\n" ) ;
			lib_output_polygon_height(height, width, data,
				x0, x1, y0, y1, z0, z1);
			break;
			
		default:
			fprintf(stderr, "Internal Error: bad file type in libpr3.c\n");
			exit(1);
			break;
		}
    }
}


/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_output_torus (COORD3 center, COORD3 normal,
					   double iradius, double oradius,
					   int curve_format)
#else
					   void lib_output_torus(center, normal, iradius, oradius, curve_format)
					   COORD3 center, normal;
double iradius, oradius;
int curve_format;
#endif
{
    MATRIX txmat;
    object_ptr new_object;
    double len, xang, zang;
    COORD3 basis1, basis2;
	
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;
		new_object->object_type  = TORUS_OBJ;
		new_object->curve_format = curve_format;
		new_object->surf_index   = gTexture_count;
		if (lib_tx_active()) {
			lib_get_current_tx(txmat);
			new_object->tx = malloc(sizeof(MATRIX));
			if (new_object->tx == NULL)
				return;
			else
				memcpy(new_object->tx, txmat, sizeof(MATRIX));
		}
		else
			new_object->tx = NULL;
		COPY_COORD3(new_object->object_data.torus.center, center);
		COPY_COORD3(new_object->object_data.torus.normal, normal);
		new_object->object_data.torus.iradius = iradius;
		new_object->object_data.torus.oradius = oradius;
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    } else if (curve_format == OUTPUT_CURVES) {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_NFF:
		case OUTPUT_VIVID:
		case OUTPUT_QRT:
		case OUTPUT_POVRAY_10:
		case OUTPUT_PLG:
		case OUTPUT_OBJ:
		case OUTPUT_RTRACE:
		case OUTPUT_RAWTRI:
		case OUTPUT_DXF:
		case OUTPUT_RWX:
		case OUTPUT_VRML1:
		case OUTPUT_VRML2:
			lib_output_polygon_torus(center, normal, iradius, oradius);
			break;
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
			/* A torus object lies in the x-z plane.  We need to determine
			   the angles of rotation to get it lined up with "normal".
			 */
			tab_indent();
			fprintf(gOutfile, "torus {\n");
			tab_inc();
			
			tab_indent();
			fprintf(gOutfile, "%g, %g\n", iradius, oradius);
			
			(void)lib_normalize_vector(normal);
			len = sqrt(normal[X] * normal[X] + normal[Y] * normal[Y]);
			xang = 180.0 * asin(normal[Z]) / PI;
			if (len < EPSILON)
				zang = 0.0;
			else
				zang = -180.0 * acos(normal[Y] / len) / PI;
			if (normal[X] < 0)
				zang = -zang;
			
			if (ABSOLUTE(xang) > EPSILON || ABSOLUTE(zang) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "rotate <%g, 0, %g>\n", xang, zang);
			}
			
			if (ABSOLUTE(center[X]) > EPSILON ||
				ABSOLUTE(center[Y]) > EPSILON ||
				ABSOLUTE(center[Z]) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "translate <%g, %g, %g>\n",
					center[X], center[Y], center[Z]);
			}
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			if (gTexture_name != NULL) {
				tab_indent();
				fprintf(gOutfile, "texture { %s }", gTexture_name);
			}
			fprintf(gOutfile, "\n");
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "} // torus\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POLYRAY:
			tab_indent();
			fprintf(gOutfile, "object { torus %g, %g", iradius, oradius);
			fprintf(gOutfile, ", <%g, %g, %g>, <%g, %g, %g>",
				center[X], center[Y], center[Z],
				normal[X], normal[Y], normal[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RAYSHADE:
			fprintf(gOutfile, "torus ");
			if (gTexture_name != NULL)
				fprintf(gOutfile, "%s ", gTexture_name);
			fprintf(gOutfile, " %g %g %g %g %g %g %g %g ",
				iradius, oradius,
				center[X], center[Y], center[Z],
				normal[X], normal[Y], normal[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, "\n");
			
			break;
			
		case OUTPUT_ART:
			tab_indent();
			fprintf(gOutfile, "torus {\n");
			tab_inc();
			
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			tab_indent();
			fprintf(gOutfile, "center(0, 0, 0)  radius %g radius %g\n",
				iradius, oradius);
			
			(void)lib_normalize_vector(normal);
			axis_to_z(normal, &xang, &zang);
			
			if (ABSOLUTE(xang) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "rotate (%g, x)\n", xang);
			}
			if (ABSOLUTE(zang) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "rotate (%g, y)\n", zang);
			}
			
			
			if (ABSOLUTE(center[X]) > EPSILON ||
				ABSOLUTE(center[Y]) > EPSILON ||
				ABSOLUTE(center[Z]) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "translate (%g, %g, %g)\n",
					center[X], center[Y], center[Z]);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_3DMF:
			if (lib_tx_active()) {
				fprintf(gOutfile, "BeginGroup( OrderedDisplayGroup ( ) )\n");
				tab_inc();
				lib_output_tx_sequence();
			}
			
			tab_indent();
			fprintf(gOutfile, "Container (\n");
			tab_inc();
			
			tab_indent();
			fprintf(gOutfile, "Torus (\n");
			
			/* Find major/minor radius axes */
			(void)lib_normalize_vector(normal);
			lib_create_orthogonal_vectors(normal, basis1, basis2);
			
			tab_indent();
			fprintf(gOutfile, "%g %g %g\n",
				iradius*normal[X], iradius*normal[Y],
				iradius*normal[Z]);
			tab_indent();
			fprintf(gOutfile, "%g %g %g\n",
				oradius*basis1[X], oradius*basis1[Y],
				oradius*basis1[Z]);
			tab_indent();
			fprintf(gOutfile, "%g %g %g\n",
				oradius*basis2[X], oradius*basis2[Y],
				oradius*basis2[Z]);
			tab_indent();
			fprintf(gOutfile, "%g %g %g 1.0\n",
				center[X], center[Y], center[Z]);
			tab_dec();
			tab_indent();
			fprintf(gOutfile, ")\n");
			
			if (gTexture_count > 0) {
				/* Write out texturing attributes */
				tab_indent();
				fprintf(gOutfile, "Reference ( %d )\n", gTexture_count);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, ")\n");
			
			if (lib_tx_active()) {
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "EndGroup( )\n");
			}
			break;
			
		default:
			fprintf(stderr, "Internal Error: bad file type in libpr3.c\n");
			exit(1);
			break;
	}
    } else {
		lib_output_polygon_torus(center, normal, iradius, oradius);
    }
}


#ifdef ANSI_FN_DEF
static void NurbDBasis(int c, float t, int npts, float*x, float*basis, float*dbasis)
#else
static void NurbDBasis(c, t, npts, x, basis, dbasis)
int c, npts;
float t, *x, *basis, *dbasis;
#endif
{
	int i, k, nplusc;
	float b1, b2, f1, f2, f3, f4;
	float numer, denom;
	
	nplusc = npts + c;
	
	for (i=0;i<nplusc;i++) {
		basis[i]  = 0.0;
		dbasis[i] = 0.0;
	}
	
	/* Calculate the first order basis functions */
	for (i=0;i<nplusc-1;i++)
		if (t >= x[i] && t < x[i+1])
			basis[i] = 1.0;
		else
			basis[i] = 0.0;

		if (t >= x[nplusc-1])
			/* The parameter is at the top end of the patch */
			basis[npts-1] = 1.0;
		
		/* Calculate higher order basis functions and their derivatives */
		for (k=2;k<=c;k++) {
			for (i=0;i<nplusc-k;i++) {
				/* Calculate the basis function */
				if (basis[i] != 0.0) {
					numer = (float)((t - x[i]) * basis[i]);
					denom = x[i+k-1] - x[i];
					if (denom == 0.0)
						if (numer == 0.0)
							b1 = 1.0;
						else
							fprintf(stderr, "Bad division: %g / %g\n", numer, denom);
						else
							b1 = numer / denom;
				}
				else
					b1 = 0.0;
				if (basis[i+1] != 0.0) {
					numer = (float)((x[i+k] - t) * basis[i+1]);
					denom = x[i+k] - x[i+1];
					if (denom == 0.0)
						if (numer == 0.0)
							b2 = 1.0;
						else
							fprintf(stderr, "Bad division: %g / %g\n", numer, denom);
						else
							b2 = numer / denom;
				}
				else
					b2 = 0.0;
				
				/* Calculate the first derivative */
				if (basis[i] != 0.0) {
					denom = x[i+k-1] - x[i];
					if (denom == 0.0)
						fprintf(stderr, "Bad division: %g / %g\n", basis[i], denom);
					f1 = basis[i] / denom;
				}
				else
					f1 = 0.0;
				if (basis[i+1] != 0.0) {
					denom = x[i+k] - x[i+1];
					if (denom == 0.0)
						fprintf(stderr, "Bad division: %g / %g\n", basis[i+k], denom);
					f2 = -basis[i+1] / denom;
				}
				else
					f2 = 0.0;
				if (dbasis[i] != 0.0) {
					numer = (float)((t - x[i]) * dbasis[i]);
					denom = x[i+k-1] - x[i];
					if (denom == 0.0)
						if (numer == 0.0)
							f3 = 1.0;
						else
							fprintf(stderr, "Bad division: %g / %g\n", numer, denom);
						else
							f3 = numer / denom;
				}
				else
					f3 = 0.0;
				if (dbasis[i+1] != 0.0) {
					numer = (float)((x[i+k] - t) * dbasis[i+1]);
					denom = x[i+k] - x[i+1];
					if (denom == 0.0)
						if (numer == 0.0)
							f4 = 1.0;
						else
							fprintf(stderr, "Bad division: %g / %g\n", numer, denom);
						else
							f4 = numer / denom;
				}
				else
					f4 = 0.0;
				
				/* Save the results for this level */
				basis[i]  = b1 + b2;
				dbasis[i] = f1 + f2 + f3 + f4;
			}
		}
}

/* Determine the position and normal at a single coordinate
point (u, v) on a NURB */
#ifdef ANSI_FN_DEF
static void NurbNormal(int norder, int npts,
					   float *nknotvec, float *nbasis, float *ndbasis,
					   int morder, int mpts,
					   float *mknotvec, float *mbasis, float *mdbasis,
					   COORD4 **ctlpts, int rat_flag,
					   float u0, float v0, COORD3 P, COORD3 N)
#else
					   static void NurbNormal(norder, npts,
					   nknotvec, nbasis, ndbasis,
					   morder, mpts,
					   mknotvec, mbasis, mdbasis,
					   ctlpts, rat_flag,
					   u0, v0, P, N)
					   int norder, npts, morder, mpts, rat_flag;
float *nknotvec, *nbasis, *ndbasis;
float *mknotvec, *mbasis, *mdbasis;
float u0, v0;
COORD4 **ctlpts;
COORD3 P, N;
#endif
{
    float t, t1, t2, homog;
    float D, Du, Dv;
    int i, j, k;
    COORD3 U, V, Nu, Nv;
	
    /* Calculate the basis functions */
    NurbDBasis(norder, u0, npts, nknotvec, nbasis, ndbasis);
    NurbDBasis(morder, v0, mpts, mknotvec, mbasis, mdbasis);
	
    /* Now evaluate for this point */
    SET_COORD3(P, 0.0, 0.0, 0.0);
    SET_COORD3(U, 0.0, 0.0, 0.0);
    SET_COORD3(V, 0.0, 0.0, 0.0);
	
    /* Check for a rational component */
    if (rat_flag) {
		SET_COORD3(Nu, 0.0, 0.0, 0.0);
		SET_COORD3(Nv, 0.0, 0.0, 0.0);
		
		D = 0.0; Du = 0.0; Dv = 0.0;
		for (i=0;i<npts;i++) {
			if (nbasis[i] != 0.0 || ndbasis[i] != 0.0) {
				for (j=0;j<mpts;j++) {
					if (mbasis[j] != 0.0 || mdbasis[j] != 0.0) {
						/* Calculate denominator of the rational basis functions */
						homog = (float)ctlpts[i][j][3];
						t = homog * nbasis[i] * mbasis[j];
						t1 = homog * ndbasis[i] * mbasis[j];
						t2 = homog * nbasis[i] * mdbasis[j];
						
						D  += t;
						Du += t1;
						Dv += t2;
						
						/* Calculate the numerators of the rational basis functions */
						for (k=0;k<3;k++) {
							P[k]  += t  * ctlpts[i][j][k];
							Nu[k] += t1 * ctlpts[i][j][k];
							Nv[k] += t2 * ctlpts[i][j][k];
						}
					}
				}
			}
		}
		
		/* Now perform the final scaling and sums */
		D = (float)(1.0 / D);
		for (i=0;i<3;i++) {
			P[i] *= D;
			U[i] = D * (Nu[i] - Du * P[i]);
			V[i] = D * (Nv[i] - Dv * P[i]);
		}
    }
    else {
		for (i=0;i<npts;i++) {
			for (j=0;j<mpts;j++) {
				t  = nbasis[i] * mbasis[j];
				t1 = ndbasis[i] * mbasis[j];
				t2 = nbasis[i] * mdbasis[j];
				for (k=0;k<3;k++) {
					P[k] += t  * ctlpts[i][j][k];
					U[k] += t1 * ctlpts[i][j][k];
					V[k] += t2 * ctlpts[i][j][k];
				}
			}
		}
    }
    CROSS(N, V, U);
    (void)lib_normalize_vector(N);
}


/* Uniform subdivision of a NURB into triangular patches */
#ifdef ANSI_FN_DEF
static void lib_output_polygon_nurb(int norder, int npts, int nknots, float *nknotvec,
									int morder, int mpts, int mknots, float *mknotvec,
									COORD4 **ctlpts, int rat_flag)
#else
									static void lib_output_polygon_nurb(norder, npts, nknots, nknotvec,
									morder, mpts, mknots, mknotvec,
									ctlpts, rat_flag)
									int norder, npts, nknots, morder, mpts, mknots, rat_flag;
float *nknotvec, *mknotvec;
COORD4 **ctlpts;
#endif
{
    float *nbasis, *ndbasis, *mbasis, *mdbasis;
    float ubnd0, ubnd1, vbnd0, vbnd1;
    float u, v, udelta, vdelta;
    int i, j, usteps, vsteps;
    COORD3 *Prow0, *Prow1, *trow;
    COORD3 *Nrow0, *Nrow1;
    COORD3 verts[3], norms[3];
	
    ubnd0 = 0.0;
    vbnd0 = 0.0;
    ubnd1 = (float)(npts - norder + 1);
    vbnd1 = (float)(mpts - morder + 1);
    usteps = npts * gU_resolution;
    vsteps = mpts * gV_resolution;
	
    nbasis  = (float *)malloc(nknots * sizeof(float));
    ndbasis = (float *)malloc(nknots * sizeof(float));
    mbasis  = (float *)malloc(mknots * sizeof(float));
    mdbasis = (float *)malloc(mknots * sizeof(float));
	
    Prow0 = (COORD3 *)malloc((vsteps + 1) * sizeof(COORD3));
    Prow1 = (COORD3 *)malloc((vsteps + 1) * sizeof(COORD3));
    Nrow0 = (COORD3 *)malloc((vsteps + 1) * sizeof(COORD3));
    Nrow1 = (COORD3 *)malloc((vsteps + 1) * sizeof(COORD3));
	
    udelta = (ubnd1 - ubnd0) / (float)(usteps);
    vdelta = (vbnd1 - vbnd0) / (float)(vsteps);
    rat_flag = 0 ;
    for (i=0,u=ubnd0;i<=usteps;i++,u+=udelta) {
		/* Generate a row of positions/normals */
		for (j=0,v=vbnd0;j<=vsteps;j++,v+=vdelta)
			NurbNormal(norder, npts, nknotvec, nbasis, ndbasis,
			morder, mpts, mknotvec, mbasis, mdbasis,
			ctlpts, rat_flag, u, v, Prow1[j], Nrow1[j]);
		
		PLATFORM_MULTITASK();
		
		if ( i > 0 ) {
			/* Output a row of triangles */
			for (j=0;i>0&&j<vsteps;j++) {
				PLATFORM_MULTITASK();
				COPY_COORD3(verts[0], Prow0[j]);
				COPY_COORD3(verts[1], Prow1[j]);
				COPY_COORD3(verts[2], Prow1[j+1]);
				COPY_COORD3(norms[0], Nrow0[j]);
				COPY_COORD3(norms[1], Nrow1[j]);
				COPY_COORD3(norms[2], Nrow1[j+1]);
				lib_output_polypatch(3, verts, norms);
				COPY_COORD3(verts[1], verts[2]);
				COPY_COORD3(verts[2], Prow0[j+1]);
				COPY_COORD3(norms[1], norms[2]);
				COPY_COORD3(norms[2], Nrow0[j+1]);
				lib_output_polypatch(3, verts, norms);
			}
		}
		/* Roll the points/normals */
		trow = Prow0; Prow0 = Prow1; Prow1 = trow;
		trow = Nrow0; Nrow0 = Nrow1; Nrow1 = trow;
    }
	
    free(Nrow1);
    free(Nrow0);
    free(Prow1);
    free(Prow0);
    free(mdbasis);
    free(mbasis);
    free(ndbasis);
    free(nbasis);
}


#ifdef ANSI_FN_DEF
void lib_output_nurb(int norder, int npts, int morder, int mpts,
					 float *in_nknotvec, float *in_mknotvec, COORD4 **ctlpts,
					 int curve_format)
#else
					 void lib_output_nurb(norder, npts, morder, mpts, in_nknotvec, in_mknotvec, ctlpts,
					 curve_format)
					 int norder, npts, morder, mpts;
float *in_nknotvec, *in_mknotvec;
COORD4 **ctlpts;
int curve_format;
#endif
{
    MATRIX txmat;
    object_ptr new_object;
    float *nknotvec, *mknotvec;
    COORD4 **points;
    int rat_flag, nknots, mknots, i, j;
	
    /* Copy the data into local structures. Build the knot vectors if
	   they weren't passed in. */
    nknots = norder + npts;
    mknots = morder + mpts;
    nknotvec = (float *)malloc(nknots * sizeof(float));
    if (in_nknotvec == NULL) {
		/* Create an open uniform knot vector in the n direction */
		nknotvec[0] = 0.0;
		for (i=1;i<nknots;i++)
			if (i >= norder && i < npts + 1)
				nknotvec[i] = nknotvec[i-1] + 1;
			else
				nknotvec[i] = nknotvec[i-1];
    } else
		memcpy(nknotvec, in_nknotvec, nknots * sizeof(float));
    mknotvec = (float *)malloc(mknots * sizeof(float));
    if (in_mknotvec == NULL) {
		/* Create an open uniform knot vector in the m direction */
		mknotvec[0] = 0.0;
		for (i=1;i<mknots;i++)
			if (i >= morder && i < mpts + 1)
				mknotvec[i] = mknotvec[i-1] + 1;
			else
				mknotvec[i] = mknotvec[i-1];
    } else
		memcpy(mknotvec, in_mknotvec, mknots * sizeof(float));
    points = (COORD4 **)malloc(npts * sizeof(COORD4 *));
    for (i=0;i<npts;i++) {
		points[i] = (COORD4 *)malloc(mpts * sizeof(COORD4));
		memcpy(points[i], ctlpts[i], mpts * sizeof(COORD4));
		for (j=0;j<mpts;j++)
			if (!rat_flag && points[i][j][3] != 1.0)
				rat_flag = 1;
			
    }
	
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;
		new_object->object_type  = NURB_OBJ;
		new_object->curve_format = curve_format;
		new_object->surf_index   = gTexture_count;
		if (lib_tx_active()) {
			lib_get_current_tx(txmat);
			new_object->tx = malloc(sizeof(MATRIX));
			if (new_object->tx == NULL)
				return;
			else
				memcpy(new_object->tx, txmat, sizeof(MATRIX));
		}
		else
			new_object->tx = NULL;
		new_object->object_data.nurb.rat_flag = rat_flag;
		new_object->object_data.nurb.npts = npts;
		new_object->object_data.nurb.norder = norder;
		new_object->object_data.nurb.nknots = nknots;
		new_object->object_data.nurb.mpts = mpts;
		new_object->object_data.nurb.morder = morder;
		new_object->object_data.nurb.mknots = mknots;
		new_object->object_data.nurb.nknotvec = nknotvec;
		new_object->object_data.nurb.mknotvec = mknotvec;
		new_object->object_data.nurb.ctlpts = points;
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    } else if (curve_format == OUTPUT_CURVES) {
		switch (gRT_out_format) {
		default:
			lib_output_polygon_nurb(norder, npts, nknots, nknotvec,
				morder, mpts, mknots, mknotvec,
				points, rat_flag);
		}
    } else {
		lib_output_polygon_nurb(norder, npts, nknots, nknotvec,
			morder, mpts, mknots, mknotvec,
			points, rat_flag);
    }
	
    for (i=npts-1;i>=0;i--)
		free(points[i]);
    free(points);
    free(mknotvec);
    free(nknotvec);
}

