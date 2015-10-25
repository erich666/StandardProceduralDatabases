/*
 * libvec.c - a library of vector operations and a random number generator
 *
 * Author:  Eric Haines
 *
 * Modified: 1 October 1992
 *           Alexander R. Enzmann
 *          Object generation routines split off to keep file size down
 *
 * Modified: 17 Jan 1993
 *         Eduard [esp] Schwan
 *           Removed unused local variables, added <stdlib.h> include for
 *          exit() call
 *
 * Modified: 1 November 1994
 *           Alexander R. Enzmann
 *          Added routines to invert matrices, transform normals, and
 *          determine rotate/scale/translate from a transform matrix.
 *          Modified computation of perspective view matrix to be a
 *          little cleaner.
 *
 * Modified: 4 December 1996
 *           Eric Haines
 *          Lint cleanup.
 */

#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <math.h>
#include "libvec.h"

/*
 * Normalize the vector (X,Y,Z) so that X*X + Y*Y + Z*Z = 1.
 *
 * The normalization divisor is returned.  If the divisor is zero, no
 * normalization occurs.
 *
 */
#ifdef ANSI_FN_DEF
double lib_normalize_vector(COORD3 cvec)
#else
double lib_normalize_vector(cvec)
COORD3 cvec;
#endif
{
    double divisor;
	
    divisor = sqrt( (double)DOT_PRODUCT(cvec, cvec) );
    if (divisor > 0.0) {
		cvec[X] /= divisor;
		cvec[Y] /= divisor;
		cvec[Z] /= divisor;
    }
    return divisor;
}


/* Find two vectors, basis1 and basis2, that form an orthogonal basis with
   the vector axis.  It is assumed that axis is non-zero.  */
#ifdef ANSI_FN_DEF
void lib_create_orthogonal_vectors(COORD3 axis, COORD3 basis1, COORD3 basis2)
#else
void lib_create_orthogonal_vectors(axis, basis1, basis2)
COORD3 axis, basis1, basis2;
#endif
{
    if (fabs(axis[Z]) < EPSILON) {
		SET_COORD3(basis1, 0.0, 0.0, 1.0);
    } else if (fabs(axis[Y]) < EPSILON) {
		SET_COORD3(basis1, 0.0, 1.0, 0.0);
    } else {
		SET_COORD3(basis1, 1.0, 0.0, 0.0);
	}
    CROSS(basis2, axis, basis1);
    (void)lib_normalize_vector(basis2);
    CROSS(basis1, basis2, axis);
    (void)lib_normalize_vector(basis1);
}

/*
 * Set all matrix elements to zero.
 */
#ifdef ANSI_FN_DEF
void lib_zero_matrix(MATRIX mx)
#else
void lib_zero_matrix(mx)
MATRIX mx;
#endif
{
    int i, j;
	
    for (i=0;i<4;++i)
		for (j=0;j<4;++j)
			mx[i][j] = 0.0;
}

/*
 * Create identity matrix.
 */
#ifdef ANSI_FN_DEF
void lib_create_identity_matrix(MATRIX mx)
#else
void lib_create_identity_matrix(mx)
MATRIX mx;
#endif
{
    int i;
	
    lib_zero_matrix(mx);
    for (i=0;i<4;++i)
		mx[i][i] = 1.0;
}

/*
 * Copy one matrix to another
 */
#ifdef ANSI_FN_DEF
void lib_copy_matrix(MATRIX mres, MATRIX mx)
#else
void lib_copy_matrix(mres, mx)
MATRIX mres, mx;
#endif
{
    int i, j;
	
    for (i=0;i<4;i++)
		for (j=0;j<4;j++)
			mres[i][j] = mx[i][j];
}

/*
 * Create a rotation matrix along the given axis by the given angle in radians.
 */
#ifdef ANSI_FN_DEF
void lib_create_rotate_matrix(MATRIX mx, int axis, double angle)
#else
void lib_create_rotate_matrix(mx, axis, angle)
MATRIX mx;
int axis;
double angle;
#endif
{
    double cosine, sine;
	
    lib_zero_matrix(mx);
    cosine = cos((double)angle);
    sine = sin((double)angle);
    switch (axis) {
	case X_AXIS:
		mx[0][0] = 1.0;
		mx[1][1] = cosine;
		mx[2][2] = cosine;
		mx[1][2] = sine;
		mx[2][1] = -sine;
		break ;
	case Y_AXIS:
		mx[1][1] = 1.0;
		mx[0][0] = cosine;
		mx[2][2] = cosine;
		mx[2][0] = sine;
		mx[0][2] = -sine;
		break;
	case Z_AXIS:
		mx[2][2] = 1.0;
		mx[0][0] = cosine;
		mx[1][1] = cosine;
		mx[0][1] = sine;
		mx[1][0] = -sine;
		break;
	default:
		fprintf(stderr, "Internal Error: bad call to lib_create_rotate_matrix\n");
		exit(1);
		break;
    }
    mx[3][3] = 1.0;
}

/* Turn a triplet of rotations about x, y, and z (in that order) into an
   equivalent rotation around a single axis.

   Rotation of the angle t about the axis (X,Y,Z) is given by:

     | X^2+(1-X^2) Cos(t),    XY(1-Cos(t)) + Z Sin(t), XZ(1-Cos(t))-Y Sin(t) |
 M = | XY(1-Cos(t))-Z Sin(t), Y^2+(1-Y^2) Cos(t),      YZ(1-Cos(t))+X Sin(t) |
     | XZ(1-Cos(t))+Y Sin(t), YZ(1-Cos(t))-X Sin(t),   Z^2+(1-Z^2) Cos(t)    |

   Rotation about the three axes (angles a1, a2, a3) can be represented as
   the product of the individual rotation matrices:

      | 1  0       0       | | Cos(a2) 0 -Sin(a2) | |  Cos(a3) Sin(a3) 0 |
      | 0  Cos(a1) Sin(a1) |*| 0       1  0       |*| -Sin(a3) Cos(a3) 0 |
      | 0 -Sin(a1) Cos(a1) | | Sin(a2) 0  Cos(a2) | |  0       0       1 |
	     Mx                       My                     Mz

   We now want to solve for X, Y, Z, and t given 9 equations in 4 unknowns.
   Using the diagonal elements of the two matrices, we get:

      X^2+(1-X^2) Cos(t) = M[0][0]
      Y^2+(1-Y^2) Cos(t) = M[1][1]
      Z^2+(1-Z^2) Cos(t) = M[2][2]

   Adding the three equations, we get:

      X^2 + Y^2 + Z^2 - (M[0][0] + M[1][1] + M[2][2]) =
	 - (3 - X^2 - Y^2 - Z^2) Cos(t)

   Since (X^2 + Y^2 + Z^2) = 1, we can rewrite as:

      Cos(t) = (1 - (M[0][0] + M[1][1] + M[2][2])) / 2

   Solving for t, we get:

      t = Acos(((M[0][0] + M[1][1] + M[2][2]) - 1) / 2)

    We can substitute t into the equations for X^2, Y^2, and Z^2 above
    to get the values for X, Y, and Z.  To find the proper signs we note
    that:

	2 X Sin(t) = M[1][2] - M[2][1]
	2 Y Sin(t) = M[2][0] - M[0][2]
	2 Z Sin(t) = M[0][1] - M[1][0]

*/
#ifdef ANSI_FN_DEF
void lib_calc_rotation_axis(COORD3 rot_angles, COORD4 axis)
#else
void lib_calc_rotation_axis(rot_angles, axis)
COORD3 rot_angles;
COORD4 axis;
#endif
{
    COORD3 axis1, axis2;
    MATRIX mx, mx1, mx2;
    double cost, cost1, sint;
    double s1, s2, s3;
    int i;
	
    /* See if we are only rotating about a single axis */
    if (fabs(rot_angles[0]) < EPSILON) {
		if (fabs(rot_angles[1]) < EPSILON) {
			SET_COORD4(axis, 0.0, 0.0, 1.0, rot_angles[2]);
			return;
		} else if (fabs(rot_angles[2]) < EPSILON) {
			SET_COORD4(axis, 0.0, 1.0, 0.0, rot_angles[1]);
			return;
		}
    } else if ((fabs(rot_angles[1]) < EPSILON) &&
		(fabs(rot_angles[2]) < EPSILON)) {
		SET_COORD4(axis, 1.0, 0.0, 0.0, rot_angles[0]);
		return;
    }
	
    /* Make the rotation matrix */
    SET_COORD3(axis1, 1.0, 0.0, 0.0);
    lib_create_axis_rotate_matrix(mx, axis1, rot_angles[0]);
    SET_COORD3(axis2, 0.0, 1.0, 0.0);
    lib_create_axis_rotate_matrix(mx2, axis2, rot_angles[1]);
    lib_matrix_multiply(mx1, mx, mx2);
    SET_COORD3(axis2, 0.0, 0.0, 1.0);
    lib_create_axis_rotate_matrix(mx2, axis2, rot_angles[2]);
    lib_matrix_multiply(mx, mx1, mx2);
	
    cost = ((mx[0][0] + mx[1][1] + mx[2][2]) - 1.0) / 2.0;
    if (cost < -1.0)
		cost = -1.0;
    else if (cost > 1.0 - EPSILON) {
		/* Bad angle - this would cause a crash */
		SET_COORD4(axis, 1.0, 0.0, 0.0, 0.0);
		return;
    }
    cost1 = 1.0 - cost;
	
    SET_COORD4(axis,
	       sqrt((mx[0][0] - cost) / cost1),
		   sqrt((mx[1][1] - cost) / cost1),
		   sqrt((mx[2][2] - cost) / cost1),
		   acos(cost));
	
    sint = 2.0 * sqrt(1.0 - cost * cost); /* This is actually 2 Sin(t) */
	
    /* Determine the proper signs */
    for (i=0;i<8;i++) {
		s1 = (i & 1 ? -1.0 : 1.0);
		s2 = (i & 2 ? -1.0 : 1.0);
		s3 = (i & 4 ? -1.0 : 1.0);
		
		if (fabs(s1*axis[0]*sint-mx[1][2]+mx[2][1]) < EPSILON2 &&
			fabs(s2*axis[1]*sint-mx[2][0]+mx[0][2]) < EPSILON2 &&
			fabs(s3*axis[2]*sint-mx[0][1]+mx[1][0]) < EPSILON2) {
			/* We found the right combination of signs */
			axis[0] *= s1;
			axis[1] *= s2;
			axis[2] *= s3;
			return;
		}
    }
}

/*
 * Create a rotation matrix along the given axis by the given angle in radians.
 * The axis is a set of direction cosines.
 */
#ifdef ANSI_FN_DEF
void lib_create_axis_rotate_matrix(MATRIX mx, COORD3 axis, double angle)
#else
void lib_create_axis_rotate_matrix(mx, axis, angle)
MATRIX mx;
COORD3 axis;
double angle;
#endif
{
    double  cosine, one_minus_cosine, sine;
	
    cosine = cos((double)angle);
    sine = sin((double)angle);
    one_minus_cosine = 1.0 - cosine;
	
    mx[0][0] = SQR(axis[X]) + (1.0 - SQR(axis[X])) * cosine;
    mx[0][1] = axis[X] * axis[Y] * one_minus_cosine + axis[Z] * sine;
    mx[0][2] = axis[X] * axis[Z] * one_minus_cosine - axis[Y] * sine;
    mx[0][3] = 0.0;
	
    mx[1][0] = axis[X] * axis[Y] * one_minus_cosine - axis[Z] * sine;
    mx[1][1] = SQR(axis[Y]) + (1.0 - SQR(axis[Y])) * cosine;
    mx[1][2] = axis[Y] * axis[Z] * one_minus_cosine + axis[X] * sine;
    mx[1][3] = 0.0;
	
    mx[2][0] = axis[X] * axis[Z] * one_minus_cosine + axis[Y] * sine;
    mx[2][1] = axis[Y] * axis[Z] * one_minus_cosine - axis[X] * sine;
    mx[2][2] = SQR(axis[Z]) + (1.0 - SQR(axis[Z])) * cosine;
    mx[2][3] = 0.0;
	
    mx[3][0] = 0.0;
    mx[3][1] = 0.0;
    mx[3][2] = 0.0;
    mx[3][3] = 1.0;
}


/* Create translation matrix */
#ifdef ANSI_FN_DEF
void lib_create_translate_matrix(MATRIX mx, COORD3 vec)
#else
void lib_create_translate_matrix(mx, vec)
MATRIX mx;
COORD3 vec;
#endif
{
    lib_create_identity_matrix(mx);
    mx[3][0] = vec[X];
    mx[3][1] = vec[Y];
    mx[3][2] = vec[Z];
}


/* Create scaling matrix */
#ifdef ANSI_FN_DEF
void lib_create_scale_matrix(MATRIX mx, COORD3 vec)
#else
void lib_create_scale_matrix(mx, vec)
MATRIX mx;
COORD3 vec;
#endif
{
    lib_zero_matrix(mx);
    mx[0][0] = vec[X];
    mx[1][1] = vec[Y];
    mx[2][2] = vec[Z];
    mx[3][3] = 1.0;
}


/* Given a point and a direction, find the transform that brings a point
   in a canonical coordinate system into a coordinate system defined by
   that position and direction.    Both the forward and inverse transform
   matrices are calculated. */
#ifdef ANSI_FN_DEF
void lib_create_canonical_matrix(MATRIX trans, MATRIX itrans, COORD3 origin, COORD3 up)
#else
void lib_create_canonical_matrix(trans, itrans, origin, up)
MATRIX trans, itrans;
COORD3 origin;
COORD3 up;
#endif
{
    MATRIX trans1, trans2;
    COORD3 tmpv;
    double ang;
	
    /* Translate "origin" to <0, 0, 0> */
    SET_COORD3(tmpv, -origin[X], -origin[Y], -origin[Z]);
    lib_create_translate_matrix(trans1, tmpv);
	
    /* Determine the axis to rotate about */
    if (fabs(up[Z]) == 1.0) {
		SET_COORD3(tmpv, 1.0, 0.0, 0.0);
    } else {
		SET_COORD3(tmpv, -up[Y], up[X], 0.0);
	}
    lib_normalize_vector(tmpv);
    ang = acos(up[Z]);
	
    /* Calculate the forward matrix */
    lib_create_axis_rotate_matrix(trans2, tmpv, -ang);
    lib_matrix_multiply(trans, trans1, trans2);
	
    /* Calculate the inverse transform */
    lib_create_axis_rotate_matrix(trans1, tmpv, ang);
    lib_create_translate_matrix(trans2, origin);
    lib_matrix_multiply(itrans, trans1, trans2);
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
		for (j=0;j<4;j++) {
			fprintf(stderr, "%7.4g ", mat[i][j]);
		}
		fprintf(stderr, "\n");
    }
}
#endif


/* Find the transformation that takes the current eye position and
   orientation and puts it at {0, 0, 0} with the up vector aligned
   with {0, 1, 0} */
#ifdef ANSI_FN_DEF
void lib_create_view_matrix(MATRIX trans, COORD3 from, COORD3 at, COORD3 up,
							int xres, int yres, double angle, double aspect)
#else
							void lib_create_view_matrix(trans, from, at, up,
							xres, yres, angle, aspect)
							MATRIX trans;
COORD3 from, at, up;
int xres, yres;
double angle, aspect;
#endif
{
    double xscale, yscale, view_dist;
    COORD3 right, up_dir;
    COORD3 Va;
    MATRIX Tv, Tt, Tx;
	
    /* Change the view angle into radians */
    angle = PI * angle / 180.0;
	
    /* Set the hither clipping plane */
    view_dist = 0.001;
	
    /* Translate point of interest to the origin */
    SET_COORD3(Va, -from[X], -from[Y], -from[Z]);
    lib_create_translate_matrix(Tv, Va);
	
    /* Move the eye by the same amount */
    SUB3_COORD3(Va, at, from);
	
    /* Make sure this is a valid sort of setup */
    if (Va[X] == 0.0 && Va[Y] == 0.0 && Va[Z] == 0.0) {
		fprintf(stderr, "Degenerate perspective transformation\n");
		exit(1);
    }
	
    /* Get the up vector to be perpendicular to the view vector */
    lib_normalize_vector(Va);
    COPY_COORD3(up_dir, up);
    CROSS(right, up_dir, Va);
    lib_normalize_vector(right);
    CROSS(up_dir, Va, right);
    lib_normalize_vector(up_dir);
	
    /* Create the orthogonal view transformation */
    Tt[0][0] = right[0];
    Tt[1][0] = right[1];
    Tt[2][0] = right[2];
    Tt[3][0] = 0;
	
    Tt[0][1] = up_dir[0];
    Tt[1][1] = up_dir[1];
    Tt[2][1] = up_dir[2];
    Tt[3][1] = 0;
	
    Tt[0][2] = Va[0];
    Tt[1][2] = Va[1];
    Tt[2][2] = Va[2];
    Tt[3][2] = 0;
	
    Tt[0][3] = 0;
    Tt[1][3] = 0;
    Tt[2][3] = 0;
    Tt[3][3] = 1;
    lib_matrix_multiply(Tx, Tv, Tt);
    lib_copy_matrix(Tv, Tx);
	
    /* Now add in the perspective transformation */
    lib_create_identity_matrix(Tt);
    Tt[2][2] =  1.0 / (1.0 + view_dist);
    Tt[3][2] =  view_dist / (1.0 + view_dist);
    Tt[2][3] =  1.0;
    Tt[3][3] =  0.0;
    lib_matrix_multiply(Tx, Tv, Tt);
    lib_copy_matrix(Tv, Tx);
	
    /* Determine how much to scale things by */
    yscale = (double)yres / (2.0 * tan(angle/2.0));
    xscale = yscale * (double)xres / ((double)yres * aspect);
    SET_COORD3(Va, -xscale, -yscale, 1.0);
    lib_create_scale_matrix(Tt, Va);
    lib_matrix_multiply(Tx, Tv, Tt);
    lib_copy_matrix(Tv, Tx);
	
    /* Translate the points so that <0,0> is at the center of
	the screen */
    SET_COORD3(Va, xres/2, yres/2, 0.0);
    lib_create_translate_matrix(Tt, Va);
	
    /* We now have the final transformation matrix */
    lib_matrix_multiply(trans, Tv, Tt);
}

/* Figure out the vector and angle that takes a viewpoint given in
   from/at/up to the orthogonal basis x/y/-z.  This code assumes that
   the combination of from/at/up is not degenerate - that from is not
   colocated with at and that the up direction is not the same as the
   direction between from and at. */
/* This is how we get from a from/at/up description to an axis/angle
   description:

   Given a view description as two points (eye location, point of interest) and
   a vector (nominal up direction), we can create an orthogonal basis with the
   following form (it takes a couple of cross products + vector normalization):

     |R0 R1 R2|
     |U0 U1 U2|
     |V0 V1 V2|

   Where R is the "right" vector, U is the "up" vector, and "V" is the direction
   of view.

   The default view basis in VRML 2.0 is then given by the following matrix:

     | 1  0  0|
     | 0  1  0|
     | 0  0 -1|

   So what we want to do is create a transformation that takes the first basis
   into the second basis, i.e.

     |R0 R1 R2|     | x0 x1 x2 |     | 1  0  0|
     |U0 U1 U2|  *  | y0 y1 y2 |  =  | 0  1  0|
     |V0 V1 V2|     | z0 z1 z2 |     | 0  0 -1|

	 M1      *       M2       =      M3

   What is now necessary is to determine the values in M2.  Since the form of M2
   is a rotation by an angle t about the unit vector (X,Y,Z), we know it has the
   form:


      | X^2+(1-X^2) Cos(t),    XY(1-Cos(t)) + Z Sin(t), XZ(1-Cos(t))-Y Sin(t) |
      | XY(1-Cos(t))-Z Sin(t), Y^2+(1-Y^2) Cos(t),      YZ(1-Cos(t))+X Sin(t) |
      | XZ(1-Cos(t))+Y Sin(t), YZ(1-Cos(t))-X Sin(t),   Z^2+(1-Z^2) Cos(t)    |

   Now solve for t, X, Y, and Z.  By looking at the diagonal elements of M2 and
   the diagonal elements of transpose(M1) * M3, we get the following three
   equations:

       R0 = X^2 + (1 - X^2) Cos(t)
       U1 = Y^2 + (1 - Y^2) Cos(t)
       V2 = -(Z^2 + (1 - Z^2) Cos(t))

   Adding them together we get:

       R0 + U1 - V2 = X^2 + Y^2 + Z^2 + (3 - X^2 - Y^2 - Z^2) Cos(t)

   Since X^2 + Y^2 + Z^2 = 1, we can rewrite as:

       R0 + U1 - V2 = 1 + 2 Cos(t)

   So we can now solve for t:

       Cos(t) = (R0 + U1 - V2 - 1) / 2, or
       t = Arccos((R0 + U1 - V2 - 1) / 2)

   Substituting back into the three equations above, we can solve for X, Y,
   and Z:

       X^2 = (R0 - Cos(t)) / (1 - Cos(t))
       Y^2 = (U1 - Cos(t)) / (1 - Cos(t))
       Z^2 = (V2 - Cos(t)) / (1 - Cos(t))

   Note that this equation is degenerate when Cos(t) = 1.  Since this implies
   that the angle of rotation is zero (e.g., the original view basis is already
   the one we want), we can choose any axis we want for the rotation.  Note
   also that if the original view basis is left handed then there is no pure
   rotation matrix that takes it to the VRML one, and the value of Cos(t) can
   be out of the range -1 to 1.

   All that's left is to find the proper signs for X, Y, Z, and t.  Again
   turning to the elements of the rotation matrix, we can deduce that:

      V1 + U2 = -2 X Sin(t)
      R2 + V0 =  2 Y Sin(t)
      U0 - R1 =  2 Z Sin(t)

   By checking these values we can make sure that we have the proper sign for
   the angle and each component of the axis of rotation.
*/
void
lib_calc_view_vector(from, at, up, axis)
COORD3 from, at, up;
COORD4 axis;
{
    COORD3 U, R, V;
    double cost, sint;
    int i;
    double s1, s2, s3;
	
    /* Create an orthogonal view basis from the input from/at/up values */
    SUB3_COORD3(V, at, from);
    lib_normalize_vector(V);
    CROSS(R, V, up);
    lib_normalize_vector(R);
    CROSS(U, R, V);
    lib_normalize_vector(U);
	
    /* Let's solve for the axis and angle */
    cost = 0.5 * (R[0] + U[1] - V[2] - 1.0);
    if (fabs(cost - 1.0) < EPSILON2) {
		/* Everything is already aligned */
		axis[0] = 1.0;
		axis[1] = 0.0;
		axis[2] = 0.0;
		axis[3] = 0.0;
		return;
    } else {
		axis[0] = sqrt((R[0] - cost) / (1.0 - cost));
		axis[1] = sqrt((U[1] - cost) / (1.0 - cost));
		axis[2] = sqrt((-V[2] - cost) / (1.0 - cost));
		axis[3] = acos(cost);
    }
    sint = 2.0 * sqrt(1.0 - cost * cost);
	
    /* Determine the proper signs the hard way - try all combinations.  This
	   could be rewritten as one complex, nested, if statement but I didn't
	   feel like writing it that way. */
    for (i=0;i<8;i++) {
		s1 = (i & 1 ? -1.0 : 1.0);
		s2 = (i & 2 ? -1.0 : 1.0);
		s3 = (i & 4 ? -1.0 : 1.0);
		
		if (fabs(V[1]+U[2]-s1*axis[0]*sint) < EPSILON2 &&
			fabs(R[2]+V[0]+s2*axis[1]*sint) < EPSILON2 &&
			fabs(U[0]-R[1]+s3*axis[2]*sint) < EPSILON2) {
			/* We found the right combination of signs */
			axis[0] *= s1;
			axis[1] *= s2;
			axis[2] *= s3;
			return;
		}
    }
	
    /* This is bad, shouldn't ever get here */
}

/*
 * Multiply a vector by a matrix.
 */
#ifdef ANSI_FN_DEF
void lib_transform_vector(COORD3 vres, COORD3 vec, MATRIX mx)
#else
void lib_transform_vector(vres, vec, mx)
COORD3 vres, vec;
MATRIX mx;
#endif
{
    COORD3 vtemp;
    vtemp[X] = vec[X]*mx[0][0] + vec[Y]*mx[1][0] + vec[Z]*mx[2][0];
    vtemp[Y] = vec[X]*mx[0][1] + vec[Y]*mx[1][1] + vec[Z]*mx[2][1];
    vtemp[Z] = vec[X]*mx[0][2] + vec[Y]*mx[1][2] + vec[Z]*mx[2][2];
    COPY_COORD3(vres, vtemp);
}

/*
 * Multiply a normal by a matrix (i.e. multiply by transpose).
 */
#ifdef ANSI_FN_DEF
void lib_transform_normal(COORD3 vres, COORD3 vec, MATRIX mx)
#else
void lib_transform_normal(vres, vec, mx)
COORD3 vres, vec;
MATRIX mx;
#endif
{
    COORD3 vtemp;
    vtemp[X] = vec[X]*mx[0][0] + vec[Y]*mx[0][1] + vec[Z]*mx[0][2];
    vtemp[Y] = vec[X]*mx[1][0] + vec[Y]*mx[1][1] + vec[Z]*mx[1][2];
    vtemp[Z] = vec[X]*mx[2][0] + vec[Y]*mx[2][1] + vec[Z]*mx[2][2];
    COPY_COORD3(vres, vtemp);
}

/*
 * Multiply a point by a matrix.
 */
#ifdef ANSI_FN_DEF
void lib_transform_point(COORD3 vres, COORD3 vec, MATRIX mx)
#else
void lib_transform_point(vres, vec, mx)
COORD3 vres, vec;
MATRIX mx;
#endif
{
    COORD3 vtemp;
    vtemp[X] = vec[X]*mx[0][0] + vec[Y]*mx[1][0] + vec[Z]*mx[2][0] + mx[3][0];
    vtemp[Y] = vec[X]*mx[0][1] + vec[Y]*mx[1][1] + vec[Z]*mx[2][1] + mx[3][1];
    vtemp[Z] = vec[X]*mx[0][2] + vec[Y]*mx[1][2] + vec[Z]*mx[2][2] + mx[3][2];
    COPY_COORD3(vres, vtemp);
}

/*
 * Multiply a 4 element vector by a matrix.  Typically used for
 * homogenous transformation from world space to screen space.
 */
#ifdef ANSI_FN_DEF
void lib_transform_coord(COORD4 vres, COORD4 vec, MATRIX mx)
#else
void lib_transform_coord(vres, vec, mx)
COORD4 vres, vec;
MATRIX mx;
#endif
{
    COORD4 vtemp;
    vtemp[X] = vec[X]*mx[0][0]+vec[Y]*mx[1][0]+vec[Z]*mx[2][0]+vec[W]*mx[3][0];
    vtemp[Y] = vec[X]*mx[0][1]+vec[Y]*mx[1][1]+vec[Z]*mx[2][1]+vec[W]*mx[3][1];
    vtemp[Z] = vec[X]*mx[0][2]+vec[Y]*mx[1][2]+vec[Z]*mx[2][2]+vec[W]*mx[3][2];
    vtemp[W] = vec[X]*mx[0][3]+vec[Y]*mx[1][3]+vec[Z]*mx[2][3]+vec[W]*mx[3][3];
    COPY_COORD4(vres, vtemp);
}

/* Determinant of a 3x3 matrix */
#ifdef ANSI_FN_DEF
static double det3x3(double a1, double a2, double a3,
    double b1, double b2, double b3,
    double c1, double c2, double c3)
#else
static double det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3)
    double a1, a2, a3;
    double b1, b2, b3;
    double c1, c2, c3;
#endif
{
    double ans;

    ans = a1 * (b2 * c3 - b3 * c2) -
	  b1 * (a2 * c3 - a3 * c2) +
	  c1 * (a2 * b3 - a3 * b2);
    return ans;
}

/* Adjoint of a 4x4 matrix - used in the computation of the inverse
   of a 4x4 matrix */
#ifdef ANSI_FN_DEF
static void adjoint(MATRIX out_mat, MATRIX in_mat)
#else
static void adjoint(out_mat, in_mat)
MATRIX out_mat, in_mat;
#endif
{
    double a1, a2, a3, a4, b1, b2, b3, b4;
    double c1, c2, c3, c4, d1, d2, d3, d4;
	
    a1 = in_mat[0][0]; b1 = in_mat[0][1];
    c1 = in_mat[0][2]; d1 = in_mat[0][3];
    a2 = in_mat[1][0]; b2 = in_mat[1][1];
    c2 = in_mat[1][2]; d2 = in_mat[1][3];
    a3 = in_mat[2][0]; b3 = in_mat[2][1];
    c3 = in_mat[2][2]; d3 = in_mat[2][3];
    a4 = in_mat[3][0]; b4 = in_mat[3][1];
    c4 = in_mat[3][2]; d4 = in_mat[3][3];
	
	
    /* row column labeling reversed since we transpose rows & columns */
    out_mat[0][0]  =   det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
    out_mat[1][0]  = - det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
    out_mat[2][0]  =   det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
    out_mat[3][0]  = - det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
	
    out_mat[0][1]  = - det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
    out_mat[1][1]  =   det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
    out_mat[2][1]  = - det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
    out_mat[3][1]  =   det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
	
    out_mat[0][2]  =   det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
    out_mat[1][2]  = - det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
    out_mat[2][2]  =   det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
    out_mat[3][2]  = - det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
	
    out_mat[0][3]  = - det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
    out_mat[1][3]  =   det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
    out_mat[2][3]  = - det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
    out_mat[3][3]  =   det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

/* Determinant of a 4x4 matrix */
#ifdef ANSI_FN_DEF
double lib_matrix_det4x4(MATRIX mat)
#else
double lib_matrix_det4x4(mat)
MATRIX mat;
#endif
{
    double ans;
    double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;
	
    a1 = mat[0][0]; b1 = mat[0][1]; c1 = mat[0][2]; d1 = mat[0][3];
    a2 = mat[1][0]; b2 = mat[1][1]; c2 = mat[1][2]; d2 = mat[1][3];
    a3 = mat[2][0]; b3 = mat[2][1]; c3 = mat[2][2]; d3 = mat[2][3];
    a4 = mat[3][0]; b4 = mat[3][1]; c4 = mat[3][2]; d4 = mat[3][3];
	
    ans = a1 * det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4) -
		b1 * det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4) +
		c1 * det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4) -
		d1 * det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);
    return ans;
}

/* Find the inverse of a 4x4 matrix */
#ifdef ANSI_FN_DEF
void lib_invert_matrix(MATRIX out_mat, MATRIX in_mat)
#else
void lib_invert_matrix(out_mat, in_mat)
MATRIX out_mat, in_mat;
#endif
{
    int i, j;
    double det;
	
    adjoint(out_mat, in_mat);
    det = lib_matrix_det4x4(in_mat);
    if (fabs(det) < EPSILON) {
		lib_create_identity_matrix(out_mat);
		return;
    }
    for (i=0;i<4;i++)
		for (j=0;j<4;j++)
			out_mat[i][j] /= det;
}

/*
 * Compute transpose of matrix.
 */
#ifdef ANSI_FN_DEF
void lib_transpose_matrix( MATRIX mxres, MATRIX mx )
#else
void lib_transpose_matrix( mxres, mx )
MATRIX mxres, mx;
#endif
{
    int i, j;
	
    for (i=0;i<4;i++)
		for (j=0;j<4;j++)
			mxres[j][i] = mx[i][j];
}

/*
 * Multiply two 4x4 matrices.  Note that mxres had better not be
 * the same as either mx1 or mx2 or bad results will be returned.
 */
#ifdef ANSI_FN_DEF
void lib_matrix_multiply(MATRIX mxres, MATRIX mx1, MATRIX mx2)
#else
void lib_matrix_multiply(mxres, mx1, mx2)
MATRIX mxres, mx1, mx2;
#endif
{
    int i, j;
	
    for (i=0;i<4;i++)
		for (j=0;j<4;j++)
			mxres[i][j] = mx1[i][0]*mx2[0][j] + mx1[i][1]*mx2[1][j] +
			mx1[i][2]*mx2[2][j] + mx1[i][3]*mx2[3][j];
}

/* Performs a 3D clip of a line segment from start to end against the
    box defined by bounds.  The actual values of start and end are modified. */
#ifdef ANSI_FN_DEF
int lib_clip_to_box(COORD3 start, COORD3 end,  double bounds[2][3])
#else
int lib_clip_to_box(start, end,  bounds)
COORD3 start, end;
double bounds[2][3];
#endif
{
    int i;
    double d, t, dir, pos;
    double tmin, tmax;
    COORD3 D;
	
    SUB3_COORD3(D, end, start);
    d = lib_normalize_vector(D);
    tmin = 0.0;
    tmax = d;
    for (i=0;i<3;i++) {
		pos = start[i];
		dir = D[i];
		if (dir < -EPSILON) {
			t = (bounds[0][i] - pos) / dir;
			if (t < tmin)
				return 0;
			if (t <= tmax)
				tmax = t;
			t = (bounds[1][i] - pos) / dir;
			if (t >= tmin) {
				if (t > tmax)
					return 0;
				tmin = t;
			}
		} else if (dir > EPSILON) {
			t = (bounds[1][i] - pos) / dir;
			if (t < tmin)
				return 0;
			if (t <= tmax)
				tmax = t;
			t = (bounds[0][i] - pos) / dir;
			if (t >= tmin) {
				if (t > tmax)
					return 0;
				tmin = t;
			}
		} else if (pos < bounds[0][i] || pos > bounds[1][i])
			return 0;
    }
    if (tmax < d) {
		end[X] = start[X] + tmax * D[X];
		end[Y] = start[Y] + tmax * D[Y];
		end[Z] = start[Z] + tmax * D[Z];
    }
    if (tmin > 0.0) {
		start[X] = start[X] + tmin * D[X];
		start[Y] = start[Y] + tmin * D[Y];
		start[Z] = start[Z] + tmin * D[Z];
    }
    return 1;
}

/*
 * Rotate a vector pointing towards the major-axis faces (i.e. the major-axis
 * component of the vector is defined as the largest value) 90 degrees to
 * another cube face.  Mod_face is a face number.
 *
 * If the routine is called six times, with mod_face=0..5, the vector will be
 * rotated to each face of a cube.  Rotations are:
 *   mod_face = 0 mod 3, +Z axis rotate
 *   mod_face = 1 mod 3, +X axis rotate
 *   mod_face = 2 mod 3, -Y axis rotate
 */
#ifdef ANSI_FN_DEF
void lib_rotate_cube_face(COORD3 vec, int major_axis, int mod_face)
#else
void lib_rotate_cube_face(vec, major_axis, mod_face)
COORD3 vec;
int major_axis, mod_face;
#endif
{
    double swap;
	
    mod_face = (mod_face+major_axis) % 3;
    if (mod_face == 0) {
		swap   = vec[X];
		vec[X] = -vec[Y];
		vec[Y] = swap;
    } else if (mod_face == 1) {
		swap   = vec[Y];
		vec[Y] = -vec[Z];
		vec[Z] = swap;
    } else {
		swap   = vec[X];
		vec[X] = -vec[Z];
		vec[Z] = swap;
    }
}

/*
 * Portable gaussian random number generator (from "Numerical Recipes", GASDEV)
 * Returns a uniform random deviate between 0.0 and 1.0.  'iseed' must be
 * less than M1 to avoid repetition, and less than (2**31-C1)/A1 [= 300718]
 * to avoid overflow.
 */
#define M1  134456
#define IA1   8121
#define IC1  28411
#define RM1 1.0/M1

#ifdef ANSI_FN_DEF
double lib_gauss_rand(long iseed)
#else
double lib_gauss_rand(iseed)
long iseed;
#endif
{
    double fac, v1, v2, r;
    long     ix1, ix2;
	
    ix2 = iseed;
    do {
		ix1 = (IC1+ix2*IA1) % M1;
		ix2 = (IC1+ix1*IA1) % M1;
		v1 = ix1 * 2.0 * RM1 - 1.0;
		v2 = ix2 * 2.0 * RM1 - 1.0;
		r = v1*v1 + v2*v2;
    } while ( r >= 1.0 );
	
    fac = sqrt((double)(-2.0 * log((double)r) / r));
    return v1 * fac;
}
