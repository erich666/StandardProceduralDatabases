/*
 * libtx.c - a library of transformation tracking routines.
 *
 * Author:  Alexander Enzmann
 *
 */


/*-----------------------------------------------------------------*/
/* include section */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "lib.h"
#include "drv.h"


/*-----------------------------------------------------------------*/
/* defines/constants section */

typedef struct tx_struct *tx_ptr;
typedef struct tx_struct {
   MATRIX tx;
   tx_ptr next;
   };


/*-----------------------------------------------------------------*/
MATRIX IdentityTx =
   {{1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}};
static MATRIX CurrentTx =
   {{1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}};
static tx_ptr TxStack = NULL;

/* Return 1 if there is an active transformation, 0 if not */
int lib_tx_active()
{
	int i, j;
	for (i=0;i<4;i++)
		for (j=0;j<4;j++)
			if (fabs(CurrentTx[i][j] - (i == j ? 1.0 : 0.0)) > EPSILON)
				return 1;
			return 0;
}

/* Copy the current transform into mat */
#ifdef ANSI_FN_DEF
void lib_get_current_tx(MATRIX mat)
#else
void lib_get_current_tx(mat)
MATRIX mat;
#endif
{
	memcpy(mat, CurrentTx, sizeof(MATRIX));
}

/* Copy matrix mat into the current transform */
#ifdef ANSI_FN_DEF
void lib_set_current_tx(MATRIX mat)
#else
void lib_set_current_tx(mat)
MATRIX mat;
#endif
{
	memcpy(CurrentTx, mat, sizeof(MATRIX));
}

#ifdef _DEBUG
#ifdef ANSI_FN_DEF
static void show_matrix(MATRIX mat)
#else
static void show_matrix(mat)
MATRIX mat;
#endif
{
	int i, j;
	for (i=0;i<4;i++) {
		for (j=0;j<4;j++)
			fprintf(stderr, "%7.4g ", mat[i][j]);
		fprintf(stderr, "\n");
	}
}

/* Show what the transform looks like */
static void lib_tx_status()
{
	double trans[16];
	
#if 0
	fprintf(stderr, "Current tx:\n");
	show_matrix(CurrentTx);
#endif
	
	if (lib_tx_active()) {
		if (lib_tx_unwind(CurrentTx, trans)) {
			fprintf(stderr, "Tx: scale     <%g,%g,%g>\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
			fprintf(stderr, "    rotate    <%g,%g,%g>\n",
				trans[U_ROTATEX], trans[U_ROTATEY], trans[U_ROTATEZ]);
			fprintf(stderr, "    translate <%g,%g,%g>\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		else
			fprintf(stderr, "Tx: failed to unwind\n");
	}
	else
		fprintf(stderr, "Tx: Identity\n");
}
#endif

/* Turn a transformation matrix into the appropriate sequence of
translate/rotate/scale instructions */
void
lib_output_tx_sequence()
{
	MATRIX txmat;
	double trans[16];
	int i, j, tflag, rflag, sflag;
	COORD3 rotang;
	COORD4 axis;
	
    lib_get_current_tx(txmat);
	
    if (!lib_tx_unwind(txmat, trans)) {
		fprintf(stderr, "Tx: failed to unwind\n");
		return;
	}
    if (fabs(trans[U_SCALEX] - 1.0) > EPSILON ||
		fabs(trans[U_SCALEY] - 1.0) > EPSILON ||
		fabs(trans[U_SCALEZ] - 1.0) > EPSILON)
		sflag = 1;
    else
		sflag = 0;
    if (fabs(trans[U_ROTATEX]) > EPSILON ||
		fabs(trans[U_ROTATEY]) > EPSILON ||
		fabs(trans[U_ROTATEZ]) > EPSILON) {
		if ((gRT_out_format != OUTPUT_3DMF) &&
			(gRT_out_format != OUTPUT_VRML1) &&
			(gRT_out_format != OUTPUT_VRML2)) {
			/* 3DMF & VRML2 seem to be the only ones that don't use degrees */
			trans[U_ROTATEX] = RAD2DEG(trans[U_ROTATEX]);
			trans[U_ROTATEY] = RAD2DEG(trans[U_ROTATEY]);
			trans[U_ROTATEZ] = RAD2DEG(trans[U_ROTATEZ]);
		}
		rflag = 1;
	}
    else
		rflag = 0;
    if (fabs(trans[U_TRANSX]) > EPSILON ||
		fabs(trans[U_TRANSY]) > EPSILON ||
		fabs(trans[U_TRANSZ]) > EPSILON)
		tflag = 1;
    else
		tflag = 0;
    if (!sflag && !rflag && !tflag)
		return;
    switch (gRT_out_format) {
	case OUTPUT_VIDEO:
	case OUTPUT_DELAYED:
	case OUTPUT_DXF:
	case OUTPUT_PLG:
	case OUTPUT_NFF:
	case OUTPUT_OBJ:
	case OUTPUT_QRT:
	case OUTPUT_RAWTRI:
	/* Can't do inline transforms in these renderers, the
	code does the transformations on the shapes
		themselves. */
		break;
		
	case OUTPUT_RTRACE:
		fprintf(gOutfile, "65 %ld ", gObject_count+1);
		for (i=0;i<4;i++)
			for (j=0;j<4;j++)
				fprintf(gOutfile, "%g ", txmat[j][i]);
			fprintf(gOutfile, "\n");
			break;
			
	case OUTPUT_RWX:
	/* Transforms are possible in RWX,
	       this code needs to be finished... */
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "Translate %g %g %g\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		if (rflag) {
			tab_indent();
			fprintf(gOutfile, "Rotate 0 0 1 %g\n",
				trans[U_ROTATEZ]);
			tab_indent();
			fprintf(gOutfile, "Rotate 0 1 0 %g\n",
				trans[U_ROTATEY]);
			tab_indent();
			fprintf(gOutfile, "Rotate 1 0 0 %g\n",
				trans[U_ROTATEX]);
		}
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "Scale %g %g %g\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		break;
		
	case OUTPUT_RIB:
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "Scale %#g %#g %#g\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			tab_indent();
			fprintf(gOutfile, "Rotate %#g 1 0 0\n",
				trans[U_ROTATEX]);
			tab_indent();
			fprintf(gOutfile, "Rotate %#g 0 1 0\n",
				trans[U_ROTATEY]);
			tab_indent();
			fprintf(gOutfile, "Rotate %#g 0 0 1\n",
				trans[U_ROTATEZ]);
		}
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "Translate %#g %#g %#g\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_VIVID:
		tab_inc();
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "scale %g\n", trans[U_SCALEX]);
		}
		tab_dec();
		if (rflag) {
			tab_indent();
			fprintf(gOutfile, "rotate %g %g %g\n",
				trans[U_ROTATEX], trans[U_ROTATEY], trans[U_ROTATEZ]);
		}
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "translate %g %g %g\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_RAYSHADE:
		if (sflag) {
			fprintf(gOutfile, " scale %g %g %g",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			fprintf(gOutfile, " rotate 1 0 0 %g",
				trans[U_ROTATEX]);
			tab_indent();
			fprintf(gOutfile, " rotate 0 1 0 %g",
				trans[U_ROTATEY]);
			tab_indent();
			fprintf(gOutfile, " rotate 0 0 1 %g",
				trans[U_ROTATEZ]);
		}
		if (tflag) {
			fprintf(gOutfile, " translate %g %g %g",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_POVRAY_10:
		if (sflag) {
			fprintf(gOutfile, " scale %g %g %g",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			fprintf(gOutfile, " rotate %g %g %g",
				trans[U_ROTATEX], trans[U_ROTATEY], trans[U_ROTATEZ]);
		}
		if (tflag) {
			fprintf(gOutfile, " translate %g %g %g",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_POVRAY_20:
	case OUTPUT_POVRAY_30:
	case OUTPUT_POLYRAY:
		if (sflag) {
			fprintf(gOutfile, " scale <%g, %g, %g>",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			tab_indent();
			fprintf(gOutfile, " rotate <%g, %g, %g>",
				trans[U_ROTATEX], trans[U_ROTATEY], trans[U_ROTATEZ]);
		}
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, " translate <%g, %g, %g>",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_ART:
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "scale(%g, %g, %g)\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			tab_indent();
			fprintf(gOutfile, "rotate(%g, x)\n",
				trans[U_ROTATEX]);
			tab_indent();
			fprintf(gOutfile, "rotate(%g, y)\n",
				trans[U_ROTATEY]);
			tab_indent();
			fprintf(gOutfile, "rotate(%g, z)\n",
				trans[U_ROTATEZ]);
		}
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "translate(%g, %g, %g)\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_3DMF:
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "Translate ( %g %g %g )\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		if (rflag) {
			tab_indent();
			fprintf(gOutfile, "Rotate ( Z %g )\n",
				trans[U_ROTATEZ]);
			tab_indent();
			fprintf(gOutfile, "Rotate ( Y %g )\n",
				trans[U_ROTATEY]);
			tab_indent();
			fprintf(gOutfile, "Rotate ( X %g )\n",
				trans[U_ROTATEX]);
		}
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "Scale ( %g %g %g )\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		break;
		
	case OUTPUT_VRML1:
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "scaleFactor %g %g %g\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			SET_COORD3(rotang, trans[U_ROTATEX], trans[U_ROTATEY],
				trans[U_ROTATEZ]);
			lib_calc_rotation_axis(rotang, axis);
			tab_indent();
			fprintf(gOutfile, "rotation %g %g %g %g\n",
				axis[0], axis[1], axis[2], axis[3]);
		}
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "translation %g %g %g\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	case OUTPUT_VRML2:
		if (sflag) {
			tab_indent();
			fprintf(gOutfile, "scale %g %g %g\n",
				trans[U_SCALEX], trans[U_SCALEY], trans[U_SCALEZ]);
		}
		if (rflag) {
			SET_COORD3(rotang, trans[U_ROTATEX], trans[U_ROTATEY],
				trans[U_ROTATEZ]);
			lib_calc_rotation_axis(rotang, axis);
			tab_indent();
			fprintf(gOutfile, "rotation %g %g %g %g\n",
				axis[0], axis[1], axis[2], axis[3]);
		}
		if (tflag) {
			tab_indent();
			fprintf(gOutfile, "translation %g %g %g\n",
				trans[U_TRANSX], trans[U_TRANSY], trans[U_TRANSZ]);
		}
		break;
		
	default:
		fprintf(stderr, "Internal Error: bad file type in libtx.c\n");
		exit(1);
		break;
    }
} /* lib_output_tx_sequence */

void
lib_tx_pop()
{
	tx_ptr last_tx;
	
	if (TxStack == NULL) {
		fprintf(stderr, "Attempt to pop beyond bottom of transform stack\n");
	}
	else {
		last_tx = TxStack;
		lib_copy_matrix(CurrentTx, last_tx->tx);
		TxStack = TxStack->next;
		free(last_tx);
	}
}

void
lib_tx_push()
{
	tx_ptr new_tx;
	
    if ((new_tx = malloc(sizeof(struct tx_struct))) == NULL) {
		fprintf(stderr, "Failed to allocate polygon data\n");
		exit(EXIT_FAIL);
	}
    lib_copy_matrix(new_tx->tx, CurrentTx);
    new_tx->next = TxStack;
    TxStack = new_tx;
}

#ifdef ANSI_FN_DEF
void lib_tx_rotate(int axis, double angle)
#else
void lib_tx_rotate(axis, angle)
int axis;
double angle;
#endif
{
    MATRIX mx1, mx2;
	
    lib_create_rotate_matrix(mx1, axis, angle);
    lib_copy_matrix(mx2, CurrentTx);
    lib_matrix_multiply(CurrentTx, mx1, mx2);
}

#ifdef ANSI_FN_DEF
void lib_tx_scale(COORD3 vec)
#else
void lib_tx_scale(vec)
COORD3 vec;
#endif
{
    MATRIX mx1, mx2;
	
    lib_create_scale_matrix(mx1, vec);
    lib_copy_matrix(mx2, CurrentTx);
    lib_matrix_multiply(CurrentTx, mx1, mx2);
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_tx_translate(COORD3 vec)
#else
void lib_tx_translate(vec)
COORD3 vec;
#endif
{
    MATRIX mx1, mx2;
	
    lib_create_translate_matrix(mx1, vec);
    lib_copy_matrix(mx2, CurrentTx);
    lib_matrix_multiply(CurrentTx, mx1, mx2);
}

/*-----------------------------------------------------------------*/
/* From Graphics Gems II, "unmatrix" written by Spencer W. Thomas.
Note that tran has to have at least 16 entries which will be set
to:
Sx, Sy, Sz, Shearxy, Shearxz, Shearyz, Rx, Ry, Rz, Tx, Ty, Tz,
P(x, y, z, w)
*/
#ifdef ANSI_FN_DEF
int lib_tx_unwind(MATRIX tx_mat, double *tran)
#else
int lib_tx_unwind(tx_mat, tran)
MATRIX tx_mat;
double *tran;
#endif
{
	int i, j;
	MATRIX locmat, pmat, invpmat;
	COORD4 prhs, psol;
	COORD3 row[3];
	
	lib_copy_matrix(locmat, tx_mat);
	
	/* Divide through by the homogenous value (normalize) */
	if (locmat[3][3] != 0.0)
		for (i=0;i<4;i++)
			for (j=0;j<4;j++)
				locmat[i][j] /= locmat[3][3];
			
	/* pmat is used to solve for perspective, but it also provides
	   an easy way to test for singularity of the upper 3x3 component */
	lib_copy_matrix(pmat, locmat);
	for (i=0;i<3;i++)
		pmat[i][3] = 0.0;
	pmat[3][3] = 1.0;
	
	if (lib_matrix_det4x4(pmat) == 0.0)
		return 0;
	
	/* First, isolate perspective */
	if (locmat[0][3] != 0.0 ||
		locmat[1][3] != 0.0 ||
		locmat[2][3] != 0.0) {
		/* prhs is the right hand side of the equation */
		prhs[X] = locmat[0][3];
		prhs[Y] = locmat[1][3];
		prhs[Z] = locmat[2][3];
		prhs[W] = locmat[3][3];
		
		/* Solve the equation by inverting pmat and multiplying
		prhs by the inverse.  */
		lib_invert_matrix(invpmat, pmat);
		lib_transform_coord(psol, prhs, invpmat);
		
		/* Save the perspective information */
		tran[U_PERSPX] = psol[X];
		tran[U_PERSPY] = psol[Y];
		tran[U_PERSPZ] = psol[Z];
		tran[U_PERSPW] = psol[W];
		
		/* Clear the perspective partition */
		locmat[0][3] = 0.0;
		locmat[1][3] = 0.0;
		locmat[2][3] = 0.0;
		locmat[3][3] = 1.0;
	}
	else {
		/* No perspective */
		tran[U_PERSPX] = 0.0;
		tran[U_PERSPY] = 0.0;
		tran[U_PERSPZ] = 0.0;
		tran[U_PERSPW] = 0.0;
	}
	
	/* Pull out the translation */
	for (i=0;i<3;i++) {
		tran[U_TRANSX+i] = locmat[3][i];
		locmat[3][i] = 0.0;
	}
	
	/* Figure out scale and shear */
	for (i=0;i<3;i++) {
		row[i][X] = locmat[i][0];
		row[i][Y] = locmat[i][1];
		row[i][Z] = locmat[i][2];
	}
	/* Compute X scale factor and normalize the first row */
	tran[U_SCALEX] = lib_normalize_vector(row[0]);
	
	/* Compute XY shear factor and make 2nd row orthogonal to 1st */
	tran[U_SHEARXY] = DOT_PRODUCT(row[0], row[1]);
	COMB_COORD(row[1], row[1], row[0], 1.0, -tran[U_SHEARXY]);
	
	/* Compute Y scale and normalize 2nd row */
	tran[U_SCALEY] = lib_normalize_vector(row[1]);
	tran[U_SHEARXY] /= tran[U_SCALEY];
	
	/* Compute XZ and YZ shears, orthogonalize 3rd row */
	tran[U_SHEARXZ] = DOT_PRODUCT(row[0], row[2]);
	COMB_COORD(row[2], row[2], row[0], 1.0, -tran[U_SHEARXZ]);
	tran[U_SHEARYZ] = DOT_PRODUCT(row[1], row[2]);
	COMB_COORD(row[2], row[2], row[1], 1.0, -tran[U_SHEARYZ]);
	
	/* Get Z scale and normalize 3rd row */
	tran[U_SCALEZ] = lib_normalize_vector(row[2]);
	tran[U_SHEARXZ] /= tran[U_SCALEZ];
	tran[U_SHEARYZ] /= tran[U_SCALEZ];
	
	/* At this point, the matrix (in rows[]) is orthonormal.
	   Check for a coordinate system flip.  If the determinant
	   is -1, then negate thematrix and the scaling factors */
	CROSS(prhs, row[1], row[2]);
	if (DOT_PRODUCT(row[0], prhs) < 0.0) {
		for (i=0;i<3;i++) {
			tran[U_SCALEX+i] *= -1.0;
			row[i][X] *= -1.0;
			row[i][Y] *= -1.0;
			row[i][Z] *= -1.0;
		}
	}
		
	/* Get the rotations out */
	tran[U_ROTATEY] = asin(-row[0][Z]);
	if (cos(tran[U_ROTATEY]) != 0) {
		if (fabs(row[1][Z]) < EPSILON && fabs(row[2][Z]) < EPSILON)
			tran[U_ROTATEX] = 0.0;
		else
			tran[U_ROTATEX] = atan2(row[1][Z], row[2][Z]);
		if (fabs(row[0][Y]) < EPSILON && fabs(row[0][X]) < EPSILON)
			tran[U_ROTATEZ] = 0.0;
		else
			tran[U_ROTATEZ] = atan2(row[0][Y], row[0][X]);
	}
	else {
		tran[U_ROTATEX] = atan2(row[1][X], row[1][Y]);
		tran[U_ROTATEZ] = 0.0;
	}
	
	return 1;
}
