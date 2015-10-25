/*
 * libvec.h - vector library definitions
 *
 * Version:  2.2 (11/17/87)
 * Author:  Eric Haines
 *
 * Modified: 1 October 1992
 *           Alexander R. Enzmann
 *
 * Modified: 2 August 1993  - More ANSI C compatibility fixes (LIBVEC_H)
 *           Eduard [esp] Schwan
 *
 */
#ifndef LIBVEC_H
#define LIBVEC_H

#include "def.h"

#if __cplusplus
extern "C" {
#endif

#define X_AXIS  0
#define Y_AXIS  1
#define Z_AXIS  2

/* Basic math functions */
void lib_zero_matrix PARAMS((MATRIX mx));
void lib_create_identity_matrix PARAMS((MATRIX mx));
void lib_copy_matrix PARAMS((MATRIX mres, MATRIX mx));
void lib_create_translate_matrix PARAMS((MATRIX mx, COORD3 vec));
void lib_create_scale_matrix PARAMS((MATRIX mx, COORD3 vec));
void lib_create_rotate_matrix PARAMS((MATRIX mx, int axis, double angle));
void lib_create_axis_rotate_matrix PARAMS((MATRIX mx, COORD3 rvec,
	double angle));
void lib_create_canonical_matrix PARAMS((MATRIX trans, MATRIX itrans,
	COORD3 origin, COORD3 up));
void lib_create_view_matrix PARAMS((MATRIX T, COORD3 from, COORD3 at,
	COORD3 up, int xres, int yres,
	double angle, double aspect));
void lib_calc_view_vector PARAMS((COORD3 from, COORD3 at, COORD3 up, COORD4 axis));
void lib_transform_coord PARAMS((COORD4 vres, COORD4 vec, MATRIX mx));
void lib_transform_point PARAMS((COORD3 vres, COORD3 vec, MATRIX mx));
void lib_transform_vector PARAMS((COORD3 vres, COORD3 vec, MATRIX mx));
void lib_transform_normal PARAMS((COORD3 vres, COORD3 vec, MATRIX mx));
void lib_transpose_matrix PARAMS((MATRIX mxres, MATRIX mx));
void lib_matrix_multiply PARAMS((MATRIX mxres, MATRIX mx1, MATRIX mx2));
double lib_matrix_det4x4 PARAMS((MATRIX));
void lib_invert_matrix PARAMS((MATRIX, MATRIX));
void lib_rotate_cube_face PARAMS((COORD3 vec, int major_axis, int mod_face));
int lib_clip_to_box PARAMS((COORD3 start, COORD3 end,  double bounds[2][3]));
double lib_normalize_vector PARAMS((COORD3 cvec));
double lib_gauss_rand PARAMS((long iseed));
void lib_create_orthogonal_vectors PARAMS((COORD3 axis, COORD3 basis1,
	COORD3 basis2));
void lib_calc_rotation_axis PARAMS((COORD3 rot_angles, COORD4 axis));

#if __cplusplus
}
#endif

#endif /* LIBVEC_H */
