/*
 * lib.c - a library of deferred object output routines.
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
void
dump_plg_file PARAMS((void))
{
    object_ptr temp_obj;
    int i;
    unsigned int fcnt, vcnt;
	
    fcnt = 0;
    vcnt = 0;
    for (temp_obj = gPolygon_stack;
	temp_obj != NULL;
	temp_obj = temp_obj->next_object) {
		fcnt++;
		vcnt += temp_obj->object_data.polygon.tot_vert;
    }
	
    fprintf(gOutfile, "objx %d %d\n", vcnt, fcnt);
	
    /* Dump all vertices */
    for (temp_obj = gPolygon_stack;
	temp_obj != NULL;
	temp_obj = temp_obj->next_object) {
		
		PLATFORM_MULTITASK();
		for (i=0;i<(int)temp_obj->object_data.polygon.tot_vert;i++) {
			fprintf(gOutfile, "%g %g %g\n",
				temp_obj->object_data.polygon.vert[i][X],
				temp_obj->object_data.polygon.vert[i][Y],
				temp_obj->object_data.polygon.vert[i][Z]);
		}
    }
	
    /* Dump all faces */
    vcnt = 0;
    for (temp_obj = gPolygon_stack;
	temp_obj != NULL;
	temp_obj = temp_obj->next_object) {
		
		PLATFORM_MULTITASK();
		fprintf(gOutfile, "0x11ff %d ", temp_obj->object_data.polygon.tot_vert);
		for (i=0;i<(int)temp_obj->object_data.polygon.tot_vert;i++)
			fprintf(gOutfile, "%d ", vcnt + i);
		fprintf(gOutfile, "\n");
		vcnt += i;
    }
}

/*-----------------------------------------------------------------*/
void
dump_obj_file PARAMS((void))
{
    object_ptr temp_obj;
    int i;
    unsigned int vcnt;
	
    /* Dump all vertices */
    for (temp_obj = gPolygon_stack;
	temp_obj != NULL;
	temp_obj = temp_obj->next_object) {
		
		PLATFORM_MULTITASK();
		for (i=0;i<(int)temp_obj->object_data.polygon.tot_vert;i++) {
			fprintf(gOutfile, "v %g %g %g\n",
				temp_obj->object_data.polygon.vert[i][X],
				temp_obj->object_data.polygon.vert[i][Y],
				temp_obj->object_data.polygon.vert[i][Z]);
		}
    }
	
    /* Dump all faces */
    vcnt = 0;
    for (temp_obj = gPolygon_stack;
	temp_obj != NULL;
	temp_obj = temp_obj->next_object) {
		
		PLATFORM_MULTITASK();
		fprintf(gOutfile, "%f ", temp_obj->object_data.polygon.tot_vert);
		for (i=0;i<(int)temp_obj->object_data.polygon.tot_vert;i++) {
			fprintf(gOutfile, "%d", vcnt + i + 1);
			if (i < (int)temp_obj->object_data.polygon.tot_vert - 1)
				fprintf(gOutfile, " ");
		}
		fprintf(gOutfile, "\n");
		vcnt += i;
    }
}

/*-----------------------------------------------------------------*/
void
dump_all_objects PARAMS((void))
{
    object_ptr temp_obj;
	
    if (gRT_out_format == OUTPUT_RTRACE)
		fprintf(gOutfile, "Objects\n");
	
    /* Step through all objects dumping them as we go. */
    for (temp_obj = gLib_objects, gObject_count = 0;
	temp_obj != NULL;
	temp_obj = temp_obj->next_object, gObject_count++) {
		
		PLATFORM_MULTITASK();
		lookup_surface_stats(temp_obj->surf_index, &gTexture_count,
			&gTexture_ior);
		if (temp_obj->tx != NULL) {
		    /* Set the active transform to what it was at the time
			 * the object was created
			 */
			lib_tx_push();
			lib_set_current_tx(*temp_obj->tx);
		}
		switch (temp_obj->object_type) {
		case BOX_OBJ:
			lib_output_box(temp_obj->object_data.box.point1,
				temp_obj->object_data.box.point2);
			break;
		case CONE_OBJ:
			lib_output_cylcone(temp_obj->object_data.cone.base_pt,
				temp_obj->object_data.cone.apex_pt,
				temp_obj->curve_format);
			break;
		case DISC_OBJ:
			lib_output_disc(temp_obj->object_data.disc.center,
				temp_obj->object_data.disc.normal,
				temp_obj->object_data.disc.iradius,
				temp_obj->object_data.disc.oradius,
				temp_obj->curve_format);
			break;
		case HEIGHT_OBJ:
			lib_output_height(temp_obj->object_data.height.filename,
				temp_obj->object_data.height.data,
				temp_obj->object_data.height.height,
				temp_obj->object_data.height.width,
				temp_obj->object_data.height.x0,
				temp_obj->object_data.height.x1,
				temp_obj->object_data.height.y0,
				temp_obj->object_data.height.y1,
				temp_obj->object_data.height.z0,
				temp_obj->object_data.height.z1);
			break;
		case POLYGON_OBJ:
			lib_output_polygon(temp_obj->object_data.polygon.tot_vert,
				temp_obj->object_data.polygon.vert);
			break;
		case POLYPATCH_OBJ:
			lib_output_polypatch(temp_obj->object_data.polypatch.tot_vert,
				temp_obj->object_data.polypatch.vert,
				temp_obj->object_data.polypatch.norm);
			break;
		case SPHERE_OBJ:
			lib_output_sphere(temp_obj->object_data.sphere.center_pt,
				temp_obj->curve_format);
			break;
		case SUPERQ_OBJ:
			lib_output_sq_sphere(temp_obj->object_data.superq.center_pt,
				temp_obj->object_data.superq.a1,
				temp_obj->object_data.superq.a2,
				temp_obj->object_data.superq.a3,
				temp_obj->object_data.superq.n,
				temp_obj->object_data.superq.e,
				temp_obj->curve_format);
			break;
		case TORUS_OBJ:
			lib_output_torus(temp_obj->object_data.torus.center,
				temp_obj->object_data.torus.normal,
				temp_obj->object_data.torus.iradius,
				temp_obj->object_data.torus.oradius,
				temp_obj->curve_format);
			break;
		default:
			fprintf(gOutfile, "Bad object type: %d\n",
				temp_obj->object_type);
			exit(1);
		}
		if (temp_obj->tx != NULL) {
			/* Reset the active transform */
			lib_tx_pop();
		}
    }
	
    if (gRT_out_format == OUTPUT_RTRACE)
		fprintf(gOutfile, "\n");
}

/*-----------------------------------------------------------------*/
void
dump_reorder_surfaces PARAMS((void))
{
    surface_ptr temp_ptr, head_ptr = NULL;
	
    while (gLib_surfaces != NULL) {
		temp_ptr = gLib_surfaces;
		gLib_surfaces = gLib_surfaces->next;
		temp_ptr->next = head_ptr;
		head_ptr = temp_ptr;
    }
	
    gLib_surfaces = head_ptr;
}

/*-----------------------------------------------------------------*/
void
dump_all_lights PARAMS((void))
{
    light_ptr temp_ptr = gLib_lights;
	
    if (gRT_out_format == OUTPUT_RTRACE)
		fprintf(gOutfile, "Lights\n");
	
    while (temp_ptr != NULL) {
		lib_output_light(temp_ptr->center_pt);
		temp_ptr = temp_ptr->next;
    }
	
    if (gRT_out_format == OUTPUT_RTRACE)
		fprintf(gOutfile, "\n");
}

/*-----------------------------------------------------------------*/
void
dump_all_surfaces PARAMS((void))
{
    surface_ptr temp_ptr = gLib_surfaces;
	
    if (gRT_out_format == OUTPUT_RTRACE)
		fprintf(gOutfile, "Surfaces\n");
	
    while (temp_ptr != NULL) {
		lib_output_color(temp_ptr->surf_name, temp_ptr->color, temp_ptr->ka,
			temp_ptr->kd, temp_ptr->ks, temp_ptr->ks_spec,
			temp_ptr->ang, temp_ptr->kt, temp_ptr->ior);
		temp_ptr = temp_ptr->next;
    }
	
    if (gRT_out_format == OUTPUT_RTRACE)
		fprintf(gOutfile, "\n");
}

