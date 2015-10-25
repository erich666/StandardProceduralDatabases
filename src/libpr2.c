/*
 * libpr2.c - a library of primitive object output routines, part 2 of 3.
 *            Basic shape routines (Cylinder, Box, etc.)
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


/*-----------------------------------------------------------------*/
/*
 * Output cylinder or cone.  A cylinder is defined as having a radius and an
 * axis defined by two points, which also define the top and bottom edge of the
 * cylinder.  A cone is defined similarly, the difference being that the apex
 * and base radii are different.  The apex radius is defined as being smaller
 * than the base radius.  Note that the surface exists without endcaps.
 *
 * If gRT_out_format=OUTPUT_CURVES, output the cylinder/cone in format:
 *     "c"
 *     base.x base.y base.z base_radius
 *     apex.x apex.y apex.z apex_radius
 *
 * If the format=OUTPUT_POLYGONS, the surface is polygonalized and output.
 * (4*OUTPUT_RESOLUTION) polygons are output as rectangles by
 * lib_output_polypatch.
 */
#ifdef ANSI_FN_DEF
void lib_output_cylcone (COORD4 base_pt, COORD4 apex_pt, int curve_format)
#else
void lib_output_cylcone(base_pt, apex_pt, curve_format)
COORD4 base_pt, apex_pt;
int curve_format;
#endif
{
    MATRIX txmat;
    double trans[16];
    object_ptr new_object;
    COORD4  axis, tempv1, tempv2, rotate;
    COORD3  center_pt;
    double  len, cottheta, xang, yang, angle, height;
    int i ;
	
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;
		new_object->object_type  = CONE_OBJ;
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
		COPY_COORD4(new_object->object_data.cone.apex_pt, apex_pt);
		COPY_COORD4(new_object->object_data.cone.base_pt, base_pt);
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
		
    } else if (curve_format == OUTPUT_CURVES) {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_PLG:
		case OUTPUT_OBJ:
		case OUTPUT_RWX:
			lib_output_polygon_cylcone(base_pt, apex_pt);
			break;
			
		case OUTPUT_VRML1:
			if ( base_pt[W] != apex_pt[W] ) {
				/* a cone*/
				
				if (apex_pt[W] == 0) {
					/* a true cone, so can output it */
					tab_indent();
					fprintf(gOutfile, "Separator {\n");
					tab_inc();
					
					if (lib_tx_active()) {
						tab_indent();
						fprintf(gOutfile, "Transform {\n");
						tab_inc();
						lib_output_tx_sequence();
						tab_dec();
						tab_indent();
						fprintf(gOutfile, "}\n");
					}
					
					tab_indent();
					fprintf(gOutfile, "Transform {\n");
					tab_inc();
					
					SUB3_COORD3(axis, apex_pt, base_pt);
					height = lib_normalize_vector(axis);
					ADD3_COORD3(center_pt, base_pt, apex_pt);
					center_pt[X] /= 2.0;
					center_pt[Y] /= 2.0;
					center_pt[Z] /= 2.0;
					tab_indent();
					fprintf(gOutfile, "translation %g %g %g\n",
						center_pt[X], center_pt[Y], center_pt[Z]);
					
					/* find axis and angle for rotation */
					SET_COORD3( tempv1, 0.0, 1.0, 0.0 ) ;
					CROSS( rotate, tempv1, axis ) ;
					lib_normalize_vector( rotate ) ;
					rotate[W] = acos( axis[Y] ) ;
					tab_indent();
					fprintf(gOutfile, "rotation %g %g %g %g\n",
						rotate[X], rotate[Y], rotate[Z], rotate[W]);
					
					tab_dec();
					tab_indent();
					fprintf(gOutfile, "}\n");
					
					tab_indent();
					fprintf(gOutfile, "Cone {\n");
					tab_inc();
					tab_indent();
					fprintf(gOutfile, "bottomRadius %g\n",
						base_pt[W]);
					tab_indent();
					fprintf(gOutfile, "height %g\n",
						height);
					tab_indent();
					fprintf(gOutfile, "parts SIDES\n");
					tab_dec();
					tab_indent();
					fprintf(gOutfile, "}\n");
					
					tab_dec();
					tab_indent();
					fprintf(gOutfile, "}\n");
				} else 
					lib_output_polygon_cylcone(base_pt, apex_pt);
			} else {
				/* a true cylinder, so can output it */
				tab_indent();
				fprintf(gOutfile, "Separator {\n");
				tab_inc();
				
				if (lib_tx_active()) {
					tab_indent();
					fprintf(gOutfile, "Transform {\n");
					tab_inc();
					lib_output_tx_sequence();
					tab_dec();
					tab_indent();
					fprintf(gOutfile, "}\n");
				}
				
				tab_indent();
				fprintf(gOutfile, "Transform {\n");
				tab_inc();
				
				SUB3_COORD3(axis, apex_pt, base_pt);
				height = lib_normalize_vector(axis);
				ADD3_COORD3(center_pt, base_pt, apex_pt);
				center_pt[X] /= 2.0;
				center_pt[Y] /= 2.0;
				center_pt[Z] /= 2.0;
				tab_indent();
				fprintf(gOutfile, "translation %g %g %g\n",
					center_pt[X], center_pt[Y], center_pt[Z]);
				
				/* find axis and angle for rotation */
				SET_COORD3( tempv1, 0.0, 1.0, 0.0 ) ;
				CROSS( rotate, tempv1, axis ) ;
				lib_normalize_vector( rotate ) ;
				rotate[W] = acos( axis[Y] ) ;
				tab_indent();
				fprintf(gOutfile, "rotation %g %g %g %g\n",
					rotate[X], rotate[Y], rotate[Z], rotate[W]);
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n");
				
				tab_indent();
				fprintf(gOutfile, "Cylinder {\n");
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "radius %g\n",
					base_pt[W]);
				tab_indent();
				fprintf(gOutfile, "height %g\n",
					height);
				tab_indent();
				fprintf(gOutfile, "parts SIDES\n");
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n");
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n");
			}
			break;
			
		case OUTPUT_VRML2:
			if ( base_pt[W] != apex_pt[W] ) {
				/* lib_output_polygon_cylcone(base_pt, apex_pt); */
				/* a cone, so output as an Extrusion */
				if (lib_tx_active()) {
					fprintf(gOutfile, "Transform {\n");
					tab_inc();
					lib_output_tx_sequence();
					tab_indent();
					fprintf(gOutfile, "children [\n");
					tab_inc();
				}
				
				tab_indent();
				fprintf(gOutfile, "Transform {\n");
				tab_inc();
				tab_indent();
				
				SUB3_COORD3(axis, apex_pt, base_pt);
				height = lib_normalize_vector(axis);
				ADD3_COORD3(center_pt, base_pt, apex_pt);
				center_pt[X] /= 2.0;
				center_pt[Y] /= 2.0;
				center_pt[Z] /= 2.0;
				fprintf(gOutfile, "translation %g %g %g\n",
					center_pt[X], center_pt[Y], center_pt[Z]);
				
				/* find axis and angle for rotation */
				SET_COORD3( tempv1, 0.0, 1.0, 0.0 ) ;
				CROSS( rotate, tempv1, axis ) ;
				lib_normalize_vector( rotate ) ;
				rotate[W] = acos( axis[Y] ) ;
				tab_indent();
				fprintf(gOutfile, "rotation %g %g %g %g\n",
					rotate[X], rotate[Y], rotate[Z], rotate[W]);
				tab_indent();
				fprintf(gOutfile, "children [\n");
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "Shape {\n");
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "geometry Extrusion { solid FALSE\n" );
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "beginCap FALSE\n" );
				tab_indent();
				fprintf(gOutfile, "endCap FALSE\n" );
				tab_indent();
				fprintf(gOutfile, "creaseAngle 1.58\n" );
				tab_indent();
				fprintf(gOutfile, "spine [ 0 %g 0, 0 %g 0 ]\n",
					(float)(-height/2.0), (float)(height/2.0) );
				tab_indent();
				fprintf(gOutfile, "scale [ %g %g, %g %g ]\n",
					base_pt[W], base_pt[W], apex_pt[W], apex_pt[W] ) ;
				tab_indent();
				fprintf(gOutfile, "crossSection [\n" ) ;
				tab_inc();
				angle = 2.0 * PI / (double)(4*gU_resolution) ;
				for ( i = 0 ; i <= 4*gU_resolution; i++ ) {
					tab_indent();
					if ( i < 4*gU_resolution ) {
						fprintf(gOutfile, "%g %g,\n",
							cos( angle * (double)i ),
							sin( angle * (double)i ) ) ;
					} else {
						fprintf(gOutfile, "%g %g ]\n",
							cos( 0.0 ),
							sin( 0.0 ) ) ;
					}
				}
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n" ) ;
				tab_dec();
				if (gTexture_name != NULL) {
					/* Write out texturing attributes */
					tab_indent();
					fprintf(gOutfile, "appearance Appearance { material %s {} }\n",
						gTexture_name);
				}
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n");
				tab_dec();
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "] }\n");
				
				if (lib_tx_active()) {
					tab_dec();
					tab_indent();
					fprintf(gOutfile, "] }\n");
					tab_dec();
				}
			} else {
				/* a true cylinder, so can output it */
				if (lib_tx_active()) {
					fprintf(gOutfile, "Transform {\n");
					tab_inc();
					lib_output_tx_sequence();
					tab_indent();
					fprintf(gOutfile, "children [\n");
					tab_inc();
				}
				
				tab_indent();
				fprintf(gOutfile, "Transform {\n");
				tab_inc();
				tab_indent();
				
				SUB3_COORD3(axis, apex_pt, base_pt);
				height = lib_normalize_vector(axis);
				ADD3_COORD3(center_pt, base_pt, apex_pt);
				center_pt[X] /= 2.0;
				center_pt[Y] /= 2.0;
				center_pt[Z] /= 2.0;
				fprintf(gOutfile, "translation %g %g %g\n",
					center_pt[X], center_pt[Y], center_pt[Z]);
				
				/* find axis and angle for rotation */
				SET_COORD3( tempv1, 0.0, 1.0, 0.0 ) ;
				CROSS( rotate, tempv1, axis ) ;
				lib_normalize_vector( rotate ) ;
				rotate[W] = acos( axis[Y] ) ;
				tab_indent();
				fprintf(gOutfile, "rotation %g %g %g %g\n",
					rotate[X], rotate[Y], rotate[Z], rotate[W]);
				tab_indent();
				fprintf(gOutfile, "children [\n");
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "Shape {\n");
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "geometry Cylinder { radius %g\n",
					base_pt[W]);
				tab_inc();
				tab_indent();
				fprintf(gOutfile, "height %g\n",
					height);
				tab_indent();
				fprintf(gOutfile, "bottom FALSE\n");
				tab_indent();
				fprintf(gOutfile, "top FALSE }\n");
				tab_dec();
				if (gTexture_name != NULL) {
					/* Write out texturing attributes */
					tab_indent();
					fprintf(gOutfile, "appearance Appearance { material %s {} }\n",
						gTexture_name);
				}
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n");
				tab_dec();
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "] }\n");
				
				if (lib_tx_active()) {
					tab_dec();
					tab_indent();
					fprintf(gOutfile, "] }\n");
					tab_dec();
				}
			} /* we could also check for true cones here, but none in SPD */
			break;
			
		case OUTPUT_NFF:
			if (lib_tx_active()) {
				lib_get_current_tx(txmat);
				lib_tx_unwind(txmat, trans);
				/* Transform the cone by modifying it's endpoints.  This
				   assumes uniform scaling. */
				lib_transform_point(tempv1, base_pt, txmat);
				COPY_COORD3(base_pt, tempv1);
				base_pt[W] *= fabs(trans[U_SCALEX]);
				lib_transform_point(tempv1, apex_pt, txmat);
				COPY_COORD3(apex_pt, tempv1);
				apex_pt[W] *= fabs(trans[U_SCALEX]);
			}
			fprintf(gOutfile, "c " ) ;
			fprintf(gOutfile, "%g %g %g %g ",
				base_pt[X], base_pt[Y], base_pt[Z], base_pt[W]);
			fprintf(gOutfile, "%g %g %g %g\n",
				apex_pt[X], apex_pt[Y], apex_pt[Z], apex_pt[W]);
			break;
			
		case OUTPUT_POVRAY_10:
		/* Since POV-Ray uses infinite primitives, we will start
		   with a cone aligned with the z-axis (QCone_Z) and figure
		   out how to clip and scale it to match what we want
		 */
			if (apex_pt[W] < base_pt[W]) {
				/* Put the bigger end at the top */
				COPY_COORD4(axis, base_pt);
				COPY_COORD4(base_pt, apex_pt);
				COPY_COORD4(apex_pt, axis);
			}
			/* Find the axis and axis length */
			SUB3_COORD3(axis, apex_pt, base_pt);
			len = lib_normalize_vector(axis);
			if (len < EPSILON) {
				/* Degenerate cone/cylinder */
				fprintf(gOutfile, "// degenerate cone/cylinder!  Ignored...\n");
				break;
			}
			if (ABSOLUTE(apex_pt[W] - base_pt[W]) < EPSILON) {
				/* Treat this thing as a cylinder */
				cottheta = len;
				tab_indent();
				fprintf(gOutfile, "object {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "quadric { <1 1 0> <0 0 0> <0 0 0> -1 } // cylinder\n");
				
				tab_indent();
				fprintf(gOutfile, "clipped_by {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "intersection {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "plane { <0 0 -1> 0 }\n");
				tab_indent();
				fprintf(gOutfile, "plane { <0 0  1> 1 }\n");
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "} // intersection\n");
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "} // clip\n");
				
				tab_indent();
				fprintf(gOutfile, "scale <%g %g 1>\n", base_pt[W], base_pt[W]);
			}
			else {
				/* Determine alignment */
				cottheta = len / (apex_pt[W] - base_pt[W]);
				tab_indent();
				fprintf(gOutfile, "object {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "quadric{ <1 1 -1> <0 0 0> <0 0 0> 0 } // cone\n");
				
				tab_indent();
				fprintf(gOutfile, "clipped_by {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "intersection {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "plane { <0 0 -1> %g}\n", -base_pt[W]);
				tab_indent();
				fprintf(gOutfile, "plane { <0 0  1> %g}\n", apex_pt[W]);
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "} // intersection\n");
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "} // clip\n");
				
				tab_indent();
				fprintf(gOutfile, "translate <0 0 %g>\n", -base_pt[W]);
			}
			
			tab_indent();
			fprintf(gOutfile, "scale <1 1 %g>\n", cottheta);
			
			len = sqrt(axis[X] * axis[X] + axis[Z] * axis[Z]);
			xang = -180.0 * asin(axis[Y]) / PI;
			if (len < EPSILON)
				yang = 0.0;
			else
				yang = 180.0 * acos(axis[Z] / len) / PI;
			if (axis[X] < 0)
				yang = -yang;
			tab_indent();
			fprintf(gOutfile, "rotate <%g %g 0>\n", xang, yang);
			tab_indent();
			fprintf(gOutfile, "translate <%g %g %g>\n",
				base_pt[X], base_pt[Y], base_pt[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL) {
				tab_indent();
				fprintf(gOutfile, "texture { %s }\n", gTexture_name);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "} // object\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
			/* of course if apex_pt[W] ~= base_pt[W], could do cylinder */
			tab_indent();
			fprintf(gOutfile, "cone {\n");
			tab_inc();
			
			tab_indent();
			fprintf(gOutfile, "<%g, %g, %g>, %g,\n",
				apex_pt[X], apex_pt[Y], apex_pt[Z], apex_pt[W]);
			tab_indent();
			fprintf(gOutfile, "<%g, %g, %g>, %g open\n",
				base_pt[X], base_pt[Y], base_pt[Z], base_pt[W]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL) {
				tab_indent();
				fprintf(gOutfile, "texture { %s }\n", gTexture_name);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POLYRAY:
			tab_indent();
			fprintf(gOutfile, "object { ");
			if (base_pt[W] == apex_pt[W])
				fprintf(gOutfile, "cylinder <%g, %g, %g>, <%g, %g, %g>, %g ",
				base_pt[X], base_pt[Y], base_pt[Z],
				apex_pt[X], apex_pt[Y], apex_pt[Z], apex_pt[W]);
			else
				fprintf(gOutfile, "cone <%g, %g, %g>, %g, <%g, %g, %g>, %g ",
				base_pt[X], base_pt[Y], base_pt[Z], base_pt[W],
				apex_pt[X], apex_pt[Y], apex_pt[Z], apex_pt[W]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			break;
			
		case OUTPUT_VIVID:
			if (lib_tx_active()) {
				tab_indent();
				fprintf(gOutfile, "transform {\n");
				lib_output_tx_sequence();
				tab_indent();
				fprintf(gOutfile, "}\n");
			}
			tab_indent();
			fprintf(gOutfile, "cone {\n");
			tab_inc();
			
			tab_indent();
			fprintf(gOutfile, " base %g %g %g base_radius %g\n",
				base_pt[X], base_pt[Y], base_pt[Z], base_pt[W]);
			tab_indent();
			fprintf(gOutfile, " apex %g %g %g apex_radius %g\n",
				apex_pt[X], apex_pt[Y], apex_pt[Z], apex_pt[W]);
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			if (lib_tx_active())
				fprintf(gOutfile, "transform_pop\n");
			break;
			
		case OUTPUT_QRT:
			fprintf(gOutfile, "BEGIN_BBOX\n");
			lib_output_polygon_cylcone(base_pt, apex_pt);
			fprintf(gOutfile, "END_BBOX\n");
			break;
			
		case OUTPUT_RAYSHADE:
			fprintf(gOutfile, "cone ");
			if (gTexture_name != NULL)
				fprintf(gOutfile, "%s ", gTexture_name);
			fprintf(gOutfile, " %g %g %g %g %g %g %g %g",
				base_pt[W], base_pt[X], base_pt[Y], base_pt[Z],
				apex_pt[W], apex_pt[X], apex_pt[Y], apex_pt[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RTRACE:
			if (lib_tx_active()) {
				lib_get_current_tx(txmat);
				lib_tx_unwind(txmat, trans);
				/* Transform the cone by modifying it's endpoints.  This
				assumes uniform scaling. */
				lib_transform_point(tempv1, base_pt, txmat);
				COPY_COORD3(base_pt, tempv1);
				base_pt[W] *= fabs(trans[U_SCALEX]);
				lib_transform_point(tempv1, apex_pt, txmat);
				COPY_COORD3(apex_pt, tempv1);
				apex_pt[W] *= fabs(trans[U_SCALEX]);
			}
			fprintf(gOutfile, "4 %d %g %g %g %g %g %g %g %g %g\n",
				gTexture_count, gTexture_ior,
				base_pt[X], base_pt[Y], base_pt[Z], base_pt[W],
				apex_pt[X], apex_pt[Y], apex_pt[Z], apex_pt[W]);
			break;
			
		case OUTPUT_ART:
			if (base_pt[W] != apex_pt[W]) {
				tab_indent();
				fprintf(gOutfile, "cone {\n");
				tab_inc();
				if (lib_tx_active())
					lib_output_tx_sequence();
				tab_indent();
				fprintf(gOutfile, "radius %g  center(%g, %g, %g)\n",
					base_pt[W], base_pt[X], base_pt[Y], base_pt[Z]);
				tab_indent();
				fprintf(gOutfile, "radius %g  center(%g, %g, %g)\n",
					apex_pt[W], apex_pt[X], apex_pt[Y], apex_pt[Z]);
			} else {
				tab_indent();
				fprintf(gOutfile, "cylinder {\n");
				tab_inc();
				if (lib_tx_active())
					lib_output_tx_sequence();
				tab_indent();
				fprintf(gOutfile, "radius %g  center(%g, %g, %g)\n",
					base_pt[W], base_pt[X], base_pt[Y], base_pt[Z]);
				tab_indent();
				fprintf(gOutfile, "center(%g, %g, %g)\n",
					apex_pt[X], apex_pt[Y], apex_pt[Z]);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RAWTRI:
		case OUTPUT_DXF:
			lib_output_polygon_cylcone(base_pt, apex_pt);
			break;
			
		case OUTPUT_RIB:
			/* translate and orient */
			tab_indent();
			fprintf(gOutfile, "TransformBegin\n");
			tab_inc();
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			SUB3_COORD3(axis, apex_pt, base_pt);
			height= len = lib_normalize_vector(axis);
			if (len < EPSILON)
			{
				/* Degenerate cone/cylinder */
				fprintf(gOutfile, "# degenerate cone/cylinder!\nIgnored...\n");
				break;
			}
			
			axis_to_z(axis, &xang, &yang);
			
			/* Calculate transformation from intrisic position */
			tab_indent();
			fprintf(gOutfile, "Translate %#g %#g %#g\n",
				base_pt[X], base_pt[Y], base_pt[Z]);
			tab_indent();
			fprintf(gOutfile, "Rotate %#g 0 1 0\n", yang);  /* was -yang */
			tab_indent();
			fprintf(gOutfile, "Rotate %#g 1 0 0\n", xang);  /* was -xang */
			if (ABSOLUTE(apex_pt[W] - base_pt[W]) < EPSILON) {
				/* Treat this thing as a cylinder */
				tab_indent();
				fprintf(gOutfile, "Cylinder [ %#g %#g %#g %#g ]\n",
					apex_pt[W], 0.0, len, 360.0);
			} else {
				/* We use a hyperboloid, because a cone cannot be cut
				 * at the top */
				tab_indent();
				fprintf(gOutfile, "Hyperboloid %#g 0 0  %#g 0 %#g  360.0\n",
					base_pt[W], apex_pt[W], height);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "TransformEnd\n");
			break;
			
		case OUTPUT_3DMF:
			if (base_pt[W] != apex_pt[W]) {
				if (base_pt[W] == 0.0 || apex_pt[W] == 0.0) {
					/* Can only handle cones with a point */
					if (lib_tx_active()) {
						fprintf(gOutfile, "BeginGroup( OrderedDisplayGroup ( ) )\n");
						tab_inc();
						lib_output_tx_sequence();
					}
					tab_indent();
					fprintf(gOutfile, "Container (\n");
					tab_inc();
					tab_indent();
					fprintf(gOutfile, "Cone (\n");
					if (base_pt[W] == 0.0) {
						SUB3_COORD3(axis, base_pt, apex_pt);
						len = apex_pt[W];
					} else {
						SUB3_COORD3(axis, apex_pt, base_pt);
						len = base_pt[W];
					}
					height = lib_normalize_vector(axis);
					if (height < EPSILON)
					{
						/* Degenerate cone/cylinder */
						fprintf(gOutfile, "# degenerate cone/cylinder!\nIgnored...\n");
						break;
					}
					tab_indent();
					fprintf(gOutfile, "%g %g %g\n",
						height*axis[X], height*axis[Y], height*axis[Z]);
					lib_create_orthogonal_vectors(axis, tempv1, tempv2);
					
					tab_indent();
					fprintf(gOutfile, "%g %g %g\n",
						len*tempv1[X], len*tempv1[Y], len*tempv1[Z]);
					tab_indent();
					fprintf(gOutfile, "%g %g %g\n",
						len*tempv2[X], len*tempv2[Y], len*tempv2[Z]);
					tab_indent();
					if (base_pt[W] == 0.0)
						fprintf(gOutfile, "%g %g %g\n",
						apex_pt[X], apex_pt[Y], apex_pt[Z]);
					else
						fprintf(gOutfile, "%g %g %g\n",
						base_pt[X], base_pt[Y], base_pt[Z]);
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
				} else
					lib_output_polygon_cylcone(base_pt, apex_pt);
			} else {
				if (lib_tx_active()) {
					fprintf(gOutfile, "BeginGroup( OrderedDisplayGroup ( ) )\n");
					tab_inc();
					lib_output_tx_sequence();
				}
				
				tab_indent();
				fprintf(gOutfile, "Container (\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "Cylinder (\n");
				tab_inc();
				
				SUB3_COORD3(axis, apex_pt, base_pt);
				len = base_pt[W];
				height = lib_normalize_vector(axis);
				if (height < EPSILON)
				{
					/* Degenerate cone/cylinder */
					fprintf(gOutfile, "# degenerate cone/cylinder!\nIgnored...\n");
					break;
				}
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
					height*axis[X], height*axis[Y], height*axis[Z]);
				
				
				/* Find major/minor radius axes */
				lib_create_orthogonal_vectors(axis, tempv1, tempv2);
				
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
					len*tempv1[X], len*tempv1[Y], len*tempv1[Z]);
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
					len*tempv2[X], len*tempv2[Y], len*tempv2[Z]);
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
					base_pt[X], base_pt[Y], base_pt[Z]);
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
			}
			break;
			
		default:
			fprintf(stderr, "Internal Error: bad file type in libpr2.c\n");
			exit(1);
			break;
		}
    } else
		lib_output_polygon_cylcone(base_pt, apex_pt);
}


/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_output_disc (COORD3 center, COORD3 normal,
					  double iradius, double oradius,
					  int curve_format)
#else
					  void lib_output_disc(center, normal, iradius, oradius, curve_format)
					  COORD3 center, normal;
double iradius, oradius;
int curve_format;
#endif
{
    MATRIX txmat;
    object_ptr new_object;
    COORD4  axis, base, apex, tempv1, tempv2;
    COORD3  axis_rib;
    double  len, xang, yang;
	
	PLATFORM_MULTITASK();
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;

		new_object->object_type  = DISC_OBJ;
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

		COPY_COORD3(new_object->object_data.disc.center, center);
		COPY_COORD3(new_object->object_data.disc.normal, normal);
		new_object->object_data.disc.iradius = iradius;
		new_object->object_data.disc.iradius = oradius;
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    } else if (curve_format == OUTPUT_CURVES) {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_NFF:
		case OUTPUT_PLG:
		case OUTPUT_OBJ:
		case OUTPUT_RWX:
		case OUTPUT_VIVID:
		case OUTPUT_RAYSHADE:
		case OUTPUT_RAWTRI:
		case OUTPUT_DXF:
		case OUTPUT_VRML1:
		case OUTPUT_VRML2:
			lib_output_polygon_disc(center, normal, iradius, oradius);
			break;
			
		case OUTPUT_POVRAY_10:
		/* A disc is a plane intersected with either one or two
		 * spheres
		 */
			COPY_COORD3(axis, normal);
			len = lib_normalize_vector(axis);
			tab_indent();
			fprintf(gOutfile, "object {\n");
			tab_inc();
			
			tab_indent();
			fprintf(gOutfile, "plane { <0 0 1> 1 }\n");
			
			tab_indent();
			fprintf(gOutfile, "clipped_by {\n");
			tab_inc();
			
			if (iradius > 0.0) {
				tab_indent();
				fprintf(gOutfile, "intersection {\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "sphere { <0 0 0> %g inverse }\n",
					iradius);
				tab_indent();
				fprintf(gOutfile, "sphere { <0 0 1> %g }\n", oradius);
				
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "} // intersection\n");
			}
			else {
				tab_indent();
				fprintf(gOutfile, "object { sphere { <0 0 0> %g } }\n",
					oradius);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "} // clip\n");
			
			len = sqrt(axis[X] * axis[X] + axis[Z] * axis[Z]);
			xang = -180.0 * asin(axis[Y]) / PI;
			yang = 180.0 * acos(axis[Z] / len) / PI;
			if (axis[X] < 0)
				yang = -yang;
			tab_indent();
			fprintf(gOutfile, "rotate <%g %g 0>\n", xang, yang);
			tab_indent();
			fprintf(gOutfile, "translate <%g %g %g>\n",
				center[X], center[Y], center[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			if (gTexture_name != NULL) {
				tab_indent();
				fprintf(gOutfile, "texture { %s }", gTexture_name);
			}
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "} // object - disc\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
			/* disc <center> <normalVector> radius [holeRadius] */
			tab_indent();
			fprintf(gOutfile, "disc { <%g, %g, %g>",
				center[X], center[Y], center[Z]);
			fprintf(gOutfile, " <%g, %g, %g>",
				normal[X], normal[Y], normal[Z]);
			fprintf(gOutfile, " %g", oradius);
			if (iradius > 0.0)
				fprintf(gOutfile, ", %g", iradius);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " texture { %s }", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POLYRAY:
			tab_indent();
			fprintf(gOutfile, "object { disc <%g, %g, %g>,",
				center[X], center[Y], center[Z]);
			fprintf(gOutfile, " <%g, %g, %g>,",
				normal[X], normal[Y], normal[Z]);
			if (iradius > 0.0)
				fprintf(gOutfile, " %g,", iradius);
			fprintf(gOutfile, " %g", oradius);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			break;
			
		case OUTPUT_QRT:
			fprintf(gOutfile, "BEGIN_BBOX\n");
			lib_output_polygon_disc(center, normal, iradius, oradius);
			fprintf(gOutfile, "END_BBOX\n");
			break;
			
		case OUTPUT_RTRACE:
			COPY_COORD3(base, center);
			base[W] = iradius;
			apex[X] = center[X] + normal[X] * EPSILON2;
			apex[Y] = center[Y] + normal[Y] * EPSILON2;
			apex[Z] = center[Z] + normal[Z] * EPSILON2;
			apex[W] = oradius;
			lib_output_cylcone(base, apex, curve_format);
			break;
			
		case OUTPUT_ART:
			tab_indent();
			fprintf(gOutfile, "ring {\n");
			tab_inc();
			
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			tab_indent();
			fprintf(gOutfile, "center(0, 0, 0)  radius %g radius %g\n",
				oradius, iradius);
			
			(void)lib_normalize_vector(normal);
			axis_to_z(normal, &xang, &yang);
			
			if (ABSOLUTE(xang) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "rotate (%g, x)\n", xang);
			}
			if (ABSOLUTE(yang) > EPSILON) {
				tab_indent();
				fprintf(gOutfile, "rotate (%g, y)\n", yang);
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
			
		case OUTPUT_RIB:
			if (iradius > 0)
			{
				/* translate and orient */
				tab_indent();
				fprintf(gOutfile, "TransformBegin\n");
				tab_inc();
				if (lib_tx_active())
					lib_output_tx_sequence();
				
				/* Calculate transformation from intrisic position */
				COPY_COORD3(axis_rib, normal);
				len = lib_normalize_vector(axis_rib);
				axis_to_z(axis_rib, &xang, &yang);
				
				tab_indent();
				fprintf(gOutfile, "translate %#g %#g %#g\n",
					center[X], center[Y], center[Z]);
				tab_indent();
				fprintf(gOutfile, "Rotate %#g 0 1 0\n", yang);  /* was -yang */
				tab_indent();
				fprintf(gOutfile, "Rotate %#g 1 0 0\n", xang);  /* was -xang */
				tab_indent();
				fprintf(gOutfile, "Disk 0 %#g 360\n", oradius);
				tab_dec();
				fprintf(gOutfile, "TransformEnd\n");
			}
			else
				lib_output_polygon_disc(center, normal, iradius, oradius);
			break;
			
		case OUTPUT_3DMF:
			if (iradius == 0.0)
			{
				if (lib_tx_active()) {
					fprintf(gOutfile, "BeginGroup( OrderedDisplayGroup ( ) )\n");
					tab_inc();
					lib_output_tx_sequence();
				}
				
				tab_indent();
				fprintf(gOutfile, "Container (\n");
				tab_inc();
				
				tab_indent();
				fprintf(gOutfile, "Disk (\n");
				
				/* Find major/minor radius axes */
				lib_create_orthogonal_vectors(normal, tempv1, tempv2);
				
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
					oradius*tempv1[X], oradius*tempv1[Y],
					oradius*tempv1[Z]);
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
					oradius*tempv2[X], oradius*tempv2[Y],
					oradius*tempv2[Z]);
				tab_indent();
				fprintf(gOutfile, "%g %g %g\n",
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
			} else
				lib_output_polygon_disc(center, normal, iradius, oradius);
			break;
			
		default:
			fprintf(stderr, "Internal Error: bad file type in libpr2.c\n");
			exit(1);
			break;
	}
	
    } else {
		lib_output_polygon_disc(center, normal, iradius, oradius);
    }
}


/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
static void sq_sphere_val(double a1, double a2, double a3, double n,
						  double e, double u, double v, COORD3 P)
#else
						  static void sq_sphere_val(a1, a2, a3, n, e, u, v, P)
						  double a1, a2, a3, n, e, u, v;
COORD3 P;
#endif
{
    double cu, su, cv, sv;
    double icu, isu, icv, isv;
	
    cu = cos(u); su = sin(u);
    cv = cos(v); sv = sin(v);
    icu = SGN(cu); isu = SGN(su);
    icv = SGN(cv); isv = SGN(sv);
    cu = fabs(cu); cv = fabs(cv);
    su = fabs(su); sv = fabs(sv);
    P[X] = a1 * POW(cv, n) * POW(cu, e) * icv * icu;
    P[Y] = a2 * POW(cv, n) * POW(su, e) * icv * isu;
    P[Z] = a3 * POW(sv, n) * isv;
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
static void sq_sphere_norm(double a1, double a2, double a3, double n,
						   double e, double u, double v, COORD3 N)
#else
						   static void sq_sphere_norm(a1, a2, a3, n, e, u, v, N)
						   double a1, a2, a3, n, e, u, v;
COORD3 N;
#endif
{
    double cu, su, cv, sv;
    double icu, isu, icv, isv;
	
    cu = cos(u); su = sin(u);
    cv = cos(v); sv = sin(v);
    icu = SGN(cu); isu = SGN(su);
    icv = SGN(cv); isv = SGN(sv);
	
    /* May be some singularities in the values, lets catch them & put
	 * a fudged normal into N */
    if (e < 2 || n < 2) {
		if (ABSOLUTE(cu) < 1.0e-3 || ABSOLUTE(su) < 1.0e-3 ||
			ABSOLUTE(cu) < 1.0e-3 || ABSOLUTE(su) < 1.0e-3) {
			SET_COORD3(N, cu*cv, su*cv, sv);
			lib_normalize_vector(N);
			return;
		}
    }
	
    cu = fabs(cu); cv = fabs(cv);
    su = fabs(su); sv = fabs(sv);
	
    N[X] = a1 * POW(cv, 2-n) * POW(cu, 2-e) * icv * icu;
    N[Y] = a2 * POW(cv, 2-n) * POW(su, 2-e) * icv * isu;
    N[Z] = a3 * POW(sv, 2-n) * isv;
    lib_normalize_vector(N);
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
static void lib_output_polygon_sq_sphere(COORD3 center_pt,
										 double a1, double a2, double a3, double n, double e)
#else
										 static void lib_output_polygon_sq_sphere(center_pt, a1, a2, a3, n, e)
										 COORD3 center_pt;
double a1, a2, a3, n, e;
#endif
{
    int i, j, u_res, v_res;
    double u, delta_u, v, delta_v;
    COORD3 verts[4], norms[4];
	
    u_res = 4 * gU_resolution;
    v_res = 4 * gV_resolution;
    delta_u = 2.0 * PI / (double)u_res;
    delta_v = PI / (double)v_res;
	
    for (i=0,u=0.0;i<u_res;i++,u+=delta_u) {
		PLATFORM_MULTITASK();
		for (j=0,v=-PI/2.0;j<v_res;j++,v+=delta_v) {
			if (j == 0) {
				sq_sphere_val(a1, a2, a3, n, e, u, v, verts[0]);
				sq_sphere_norm(a1, a2, a3, n, e, u, v, norms[0]);
				sq_sphere_val(a1, a2, a3, n, e, u, v+delta_v, verts[1]);
				sq_sphere_norm(a1, a2, a3, n, e, u, v+delta_v, norms[1]);
				sq_sphere_val(a1, a2, a3, n, e, u+delta_u, v+delta_v, verts[2]);
				sq_sphere_norm(a1, a2, a3, n, e, u+delta_u, v+delta_v,norms[2]);
				ADD3_COORD3(verts[0], verts[0], center_pt);
				ADD3_COORD3(verts[1], verts[1], center_pt);
				ADD3_COORD3(verts[2], verts[2], center_pt);
				lib_output_polypatch(3, verts, norms);
			} else if (j == v_res-1) {
				sq_sphere_val(a1, a2, a3, n, e, u, v, verts[0]);
				sq_sphere_norm(a1, a2, a3, n, e, u, v, norms[0]);
				sq_sphere_val(a1, a2, a3, n, e, u, v+delta_v, verts[1]);
				sq_sphere_norm(a1, a2, a3, n, e, u, v+delta_v, norms[1]);
				sq_sphere_val(a1, a2, a3, n, e, u+delta_u, v, verts[2]);
				sq_sphere_norm(a1, a2, a3, n, e, u+delta_u, v, norms[2]);
				ADD3_COORD3(verts[0], verts[0], center_pt);
				ADD3_COORD3(verts[1], verts[1], center_pt);
				ADD3_COORD3(verts[2], verts[2], center_pt);
				lib_output_polypatch(3, verts, norms);
			} else {
				sq_sphere_val(a1, a2, a3, n, e, u, v, verts[0]);
				sq_sphere_norm(a1, a2, a3, n, e, u, v, norms[0]);
				sq_sphere_val(a1, a2, a3, n, e, u, v+delta_v, verts[1]);
				sq_sphere_norm(a1, a2, a3, n, e, u, v+delta_v, norms[1]);
				sq_sphere_val(a1, a2, a3, n, e, u+delta_u, v+delta_v, verts[2]);
				sq_sphere_norm(a1, a2, a3, n, e, u+delta_u, v+delta_v,norms[2]);
				ADD3_COORD3(verts[0], verts[0], center_pt);
				ADD3_COORD3(verts[1], verts[1], center_pt);
				ADD3_COORD3(verts[2], verts[2], center_pt);
				lib_output_polypatch(3, verts, norms);
				COPY_COORD3(verts[1], verts[2]);
				COPY_COORD3(norms[1], norms[2]);
				sq_sphere_val(a1, a2, a3, n, e, u+delta_u, v, verts[2]);
				sq_sphere_norm(a1, a2, a3, n, e, u+delta_u, v, norms[2]);
				ADD3_COORD3(verts[2], verts[2], center_pt);
				lib_output_polypatch(3, verts, norms);
			}
		}
    }
}

/*-----------------------------------------------------------------*/
#ifdef ANSI_FN_DEF
void lib_output_sq_sphere(COORD3 center_pt,
						  double a1, double a2, double a3, double n, double e, int curve_format)
#else
						  void lib_output_sq_sphere(center_pt, a1, a2, a3, n, e, curve_format)
						  COORD3 center_pt;
double a1, a2, a3, n, e;
int curve_format;
#endif
{
    MATRIX txmat;
    object_ptr new_object;
	
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;
		new_object->object_type  = SUPERQ_OBJ;
		new_object->curve_format = OUTPUT_PATCHES;
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
		COPY_COORD3(new_object->object_data.superq.center_pt, center_pt);
		new_object->object_data.superq.a1 = a1;
		new_object->object_data.superq.a2 = a2;
		new_object->object_data.superq.a3 = a3;
		new_object->object_data.superq.n  = n;
		new_object->object_data.superq.e  = e;
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    } else if (curve_format == OUTPUT_CURVES) {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_NFF:
		case OUTPUT_POVRAY_10:
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
		case OUTPUT_VIVID:
		case OUTPUT_QRT:
		case OUTPUT_RAYSHADE:
		case OUTPUT_RTRACE:
		case OUTPUT_PLG:
		case OUTPUT_RAWTRI:
		case OUTPUT_ART:
		case OUTPUT_RIB:
		case OUTPUT_DXF:
		case OUTPUT_OBJ:
		case OUTPUT_RWX:
		case OUTPUT_3DMF:
		case OUTPUT_VRML1:
		case OUTPUT_VRML2:
			lib_output_polygon_sq_sphere(center_pt, a1, a2, a3, n, e);
			break;
		case OUTPUT_POLYRAY:
			tab_indent();
			fprintf(gOutfile, "object { superq %g, %g\n", n, e);
			tab_inc();
			tab_indent();
			fprintf(gOutfile, "scale <%g, %g, %g>\n", a1, a2, a3);
			tab_indent();
			fprintf(gOutfile, "translate <%g, %g, %g>\n",
				center_pt[X], center_pt[Y], center_pt[Z]);
			tab_dec();
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			break;
			
		default:
			fprintf(stderr, "Internal Error: bad file type in libpr2.c\n");
			exit(1);
			break;
		}
    } else
		lib_output_polygon_sq_sphere(center_pt, a1, a2, a3, n, e);
}


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
 * (6*2*gU_resolution*gV_resolution) polygons are output as triangles
 * using lib_output_polypatch.
 */
#ifdef ANSI_FN_DEF
void lib_output_sphere (COORD4 center_pt, int curve_format)
#else
void lib_output_sphere(center_pt, curve_format)
COORD4 center_pt;
int curve_format;
#endif
{
    MATRIX txmat;
    double trans[16];
    COORD3 tempv;
    object_ptr new_object;
	
	PLATFORM_MULTITASK();
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;
		new_object->object_type  = SPHERE_OBJ;
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
		COPY_COORD4(new_object->object_data.sphere.center_pt, center_pt);
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    }
    else if (curve_format == OUTPUT_CURVES) {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_PLG:
		case OUTPUT_OBJ:
			lib_output_polygon_sphere(center_pt);
			break;
			
		case OUTPUT_RWX:
			fprintf(gOutfile, "TransformBegin\n");
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, "Translate %g %g %g\n",
				center_pt[X], center_pt[Y], center_pt[Z]);
			fprintf(gOutfile, "Sphere %g 3\n", center_pt[W]);
			fprintf(gOutfile, "TransformEnd\n");
			break;
			
		case OUTPUT_NFF:
			if (lib_tx_active()) {
				lib_get_current_tx(txmat);
				lib_tx_unwind(txmat, trans);
				/* Transform the sphere by modifying it's center.  This
				assumes uniform scaling. */
				lib_transform_point(tempv, center_pt, txmat);
				COPY_COORD3(center_pt, tempv);
				center_pt[W] *= fabs(trans[U_SCALEX]);
			}
			fprintf(gOutfile, "s %g %g %g %g\n",
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			break;
			
		case OUTPUT_POVRAY_10:
			tab_indent();
			fprintf(gOutfile, "object { sphere { <%g %g %g> %g } ",
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " texture { %s }", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
			tab_indent();
			fprintf(gOutfile, "sphere { <%g, %g, %g>, %g ",
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " texture { %s }", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POLYRAY:
			tab_indent();
			fprintf(gOutfile, "object { sphere <%g, %g, %g>, %g ",
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			break;
			
		case OUTPUT_VIVID:
			if (lib_tx_active()) {
				tab_indent();
				fprintf(gOutfile, "transform {\n");
				lib_output_tx_sequence();
				tab_indent();
				fprintf(gOutfile, "}\n");
			}
			tab_indent();
			fprintf(gOutfile, "sphere { center %g %g %g radius %g }\n",
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			fprintf(gOutfile, "\n");
			if (lib_tx_active())
				fprintf(gOutfile, "transform_pop\n");
			break;
			
		case OUTPUT_QRT:
			if (lib_tx_active()) {
				lib_get_current_tx(txmat);
				lib_tx_unwind(txmat, trans);
				/* Transform the cone by modifying it's endpoints.  This
				assumes uniform scaling. */
				lib_transform_point(tempv, center_pt, txmat);
				COPY_COORD3(center_pt, tempv);
				center_pt[W] *= fabs(trans[U_SCALEX]);
			}
			tab_indent();
			fprintf(gOutfile, "sphere ( loc = (%g, %g, %g), radius = %g )\n",
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			break;
			
		case OUTPUT_RAYSHADE:
			fprintf(gOutfile, "sphere ");
			if (gTexture_name != NULL)
				fprintf(gOutfile, "%s ", gTexture_name);
			fprintf(gOutfile, " %g %g %g %g ",
				center_pt[W], center_pt[X], center_pt[Y], center_pt[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RTRACE:
			if (lib_tx_active()) {
				lib_get_current_tx(txmat);
				lib_tx_unwind(txmat, trans);
				/* Transform the cone by modifying it's endpoints.  This
				assumes uniform scaling. */
				lib_transform_point(tempv, center_pt, txmat);
				COPY_COORD3(center_pt, tempv);
				center_pt[W] *= fabs(trans[U_SCALEX]);
			}
			fprintf(gOutfile, "1 %d %g %g %g %g %g\n",
				gTexture_count, gTexture_ior,
				center_pt[X], center_pt[Y], center_pt[Z], center_pt[W]);
			break;
			
		case OUTPUT_ART:
			tab_indent();
			fprintf(gOutfile, "sphere {\n");
			tab_inc();
			if (lib_tx_active())
				lib_output_tx_sequence();
			
			tab_indent();
			fprintf(gOutfile, "radius %g\n", center_pt[W]);
			tab_indent();
			fprintf(gOutfile, "center(%g, %g, %g)\n",
				center_pt[X], center_pt[Y], center_pt[Z]);
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RAWTRI:
		case OUTPUT_DXF:
			lib_output_polygon_sphere(center_pt);
			break;
		case OUTPUT_RIB:
			tab_indent();
			fprintf(gOutfile, "TransformBegin\n");
			tab_inc();
			if (lib_tx_active())
				lib_output_tx_sequence();
			tab_indent();
			fprintf(gOutfile, "Translate %#g %#g %#g\n",
				center_pt[X], center_pt[Y], center_pt[Z]);
			tab_indent();
			fprintf(gOutfile, "Sphere %#g %#g %#g 360\n",
				center_pt[W], -center_pt[W], center_pt[W]);
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "TransformEnd\n");
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
			fprintf(gOutfile, "Ellipsoid ( %g 0 0 0 %g 0 0 0 %g %g %g %g )\n",
				center_pt[W], center_pt[W], center_pt[W],
				center_pt[X], center_pt[Y], center_pt[Z]);
			
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
			
		case OUTPUT_VRML1:
			tab_indent();
			fprintf(gOutfile, "Separator {\n");
			tab_inc();
			
			if (lib_tx_active()) {
				tab_indent();
				fprintf(gOutfile, "Transform {\n");
				tab_inc();
				lib_output_tx_sequence();
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "}\n");
			}
			
			tab_indent();
			fprintf(gOutfile, "Transform {\n");
			tab_inc();
			tab_indent();
			fprintf(gOutfile, "translation %g %g %g\n",
				center_pt[X], center_pt[Y], center_pt[Z]);
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			
			tab_indent();
			fprintf(gOutfile, "Sphere {\n");
			tab_inc();
			tab_indent();
			fprintf(gOutfile, "radius %g\n",
				center_pt[W]);
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			break;
			
		case OUTPUT_VRML2:
			if (lib_tx_active()) {
				fprintf(gOutfile, "Transform {\n");
				tab_inc();
				lib_output_tx_sequence();
				tab_indent();
				fprintf(gOutfile, "children [\n");
				tab_inc();
			}
			
			tab_indent();
			fprintf(gOutfile, "Transform {\n");
			tab_inc();
			tab_indent();
			fprintf(gOutfile, "translation %g %g %g\n",
				center_pt[X], center_pt[Y], center_pt[Z]);
			tab_indent();
			fprintf(gOutfile, "children [\n");
			tab_inc();
			tab_indent();
			fprintf(gOutfile, "Shape {\n");
			tab_inc();
			tab_indent();
			fprintf(gOutfile, "geometry Sphere { radius %g }\n",
				center_pt[W]);
			if (gTexture_name != NULL) {
				/* Write out texturing attributes */
				tab_indent();
				fprintf(gOutfile, "appearance Appearance { material %s {} }\n",
					gTexture_name);
			}
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "}\n");
			tab_dec();
			tab_dec();
			tab_indent();
			fprintf(gOutfile, "] }\n");
			
			if (lib_tx_active()) {
				tab_dec();
				tab_indent();
				fprintf(gOutfile, "] }\n");
				tab_dec();
			}
			break;
			
		default:
			fprintf(stderr, "Internal Error: bad file type in libpr2.c\n");
			exit(1);
			break;
			
		}
    } else {
		lib_output_polygon_sphere(center_pt);
    }
}



/*-----------------------------------------------------------------*/
/* Output box.  A box is defined by a diagonally opposite corners. */
#ifdef ANSI_FN_DEF
void lib_output_box (COORD3 p1, COORD3 p2)
#else
void lib_output_box(p1, p2)
COORD3 p1, p2;
#endif
{
    MATRIX txmat;
    object_ptr new_object;
	
    if (gRT_out_format == OUTPUT_DELAYED) {
		/* Save all the pertinent information */
		new_object = (object_ptr)malloc(sizeof(struct object_struct));
		if (new_object == NULL)
			/* Quietly fail */
			return;
		new_object->object_type  = BOX_OBJ;
		new_object->curve_format = OUTPUT_PATCHES;
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
		COPY_COORD3(new_object->object_data.box.point1, p1);
		COPY_COORD3(new_object->object_data.box.point2, p2);
		new_object->next_object = gLib_objects;
		gLib_objects = new_object;
    } else {
		switch (gRT_out_format) {
		case OUTPUT_VIDEO:
		case OUTPUT_NFF:
		case OUTPUT_VIVID:
		case OUTPUT_PLG:
		case OUTPUT_OBJ:
		case OUTPUT_RAWTRI:
		case OUTPUT_RIB:
		case OUTPUT_DXF:
		case OUTPUT_RWX:
		case OUTPUT_VRML1:
		case OUTPUT_VRML2:
			lib_output_polygon_box(p1, p2);
			break;
			
		case OUTPUT_POVRAY_10:
			tab_indent();
			fprintf(gOutfile, "object { box { <%g %g %g> <%g %g %g> }",
				p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " texture { %s }", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POVRAY_20:
		case OUTPUT_POVRAY_30:
			tab_indent();
			fprintf(gOutfile, "box { <%g, %g, %g>, <%g, %g, %g>  ",
				p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " texture { %s }", gTexture_name);
			fprintf(gOutfile, " }\n");
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_POLYRAY:
			fprintf(gOutfile, "object { box <%g, %g, %g>, <%g, %g, %g>",
				p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			if (gTexture_name != NULL)
				fprintf(gOutfile, " %s", gTexture_name);
			fprintf(gOutfile, " }\n");
			break;
			
		case OUTPUT_QRT:
			fprintf(gOutfile, "BEGIN_BBOX\n");
			lib_output_polygon_box(p1, p2);
			fprintf(gOutfile, "END_BBOX\n");
			break;
			
		case OUTPUT_RAYSHADE:
			fprintf(gOutfile, "box ");
			if (gTexture_name != NULL)
				fprintf(gOutfile, "%s ", gTexture_name);
			fprintf(gOutfile, " %g %g %g %g %g %g",
				p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_ART:
			tab_indent();
			fprintf(gOutfile, "box {\n");
			if (lib_tx_active())
				lib_output_tx_sequence();
			fprintf(gOutfile, " vertex(%g, %g, %g)\n",
				p1[X], p1[Y], p1[Z]);
			fprintf(gOutfile, " vertex(%g, %g, %g) }\n",
				p2[X], p2[Y], p2[Z]);
			fprintf(gOutfile, "\n");
			break;
			
		case OUTPUT_RTRACE:
			if (lib_tx_active())
				lib_output_polygon_box(p1, p2);
			else
				fprintf(gOutfile, "2 %d %g %g %g %g %g %g %g\n",
				gTexture_count, gTexture_ior,
				(p1[X] + p2[X]) / 2.0,
				(p1[Y] + p2[Y]) / 2.0,
				(p1[Z] + p2[Z]) / 2.0,
				p2[X] - p1[X], p2[Y] - p1[Y], p2[Z] - p1[Z]);
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
			fprintf(gOutfile, "Box ( %g 0 0 0 %g 0 0 0 %g %g %g %g )\n",
				p2[X] - p1[X], p2[Y] - p1[Y], p2[Z] - p1[Z],
				p1[X], p1[Y], p1[Z]);
			
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
			fprintf(stderr, "Internal Error: bad file type in libpr2.c\n");
			exit(1);
			break;
		}
    }
}
