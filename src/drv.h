/*
 * Modified: 17 Mar 1993
 *           Eduard [esp] Schwan
 *           Passed bg_color to display_init
 *             (unfortunate side-effect is you should now call
 *             lib_output_background_color BEFORE lib_output_viewpoint.)
 *
 * Modified: 2 August 1993  - More ANSI C & Mac compatibility fixes
 *           Eduard [esp] Schwan
 *
 */

#ifndef DISP_H
#define DISP_H

#include "def.h"


void display_init PARAMS((int xres, int yres, COORD3 bk_color));
void display_close PARAMS((int wait_flag));
void display_clear PARAMS((void));
void display_plot PARAMS((int x, int y, COORD3 color));
void display_line PARAMS((int x0, int y0, int x1, int y1, COORD3 color));
int kbhit PARAMS((void));


#endif /* DISP_H */
