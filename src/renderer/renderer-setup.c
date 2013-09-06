/*
 * niftyconf - niftyled GUI
 * Copyright (C) 2011-2013 Daniel Hiepler <daniel@niftylight.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <math.h>
#include <gtk/gtk.h>
#include <niftyled.h>
#include "elements/element-setup.h"
#include "renderer/renderer.h"
#include "renderer/renderer-tile.h"






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** renderer for setups */
static NftResult _render_setup(cairo_surface_t ** surface, gpointer element)
{
        /* setup to render */
        LedSetup *s = (LedSetup *) element;

        /* if dimensions changed, we need to allocate a new surface */
        LedFrameCord w, h;
        led_setup_get_dim(s, &w, &h);

		double width = (double) w * renderer_scale_factor();
        double height = (double) h * renderer_scale_factor();

        if(!renderer_resize(setup_get_renderer(), width, height))
        {
                g_error("Failed to resize renderer to %dx%d", w, h);
                return NFT_FAILURE;
        }

        /* create context for drawing */
        cairo_t *cr = cairo_create(*surface);


        /* clear surface */
#ifdef DEBUG
        cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 1);
#else
        cairo_set_source_rgba(cr, 0, 0, 0, 1);
#endif
        cairo_rectangle(cr,
                        0, 0,
                        (double) cairo_image_surface_get_width(*surface),
                        (double) cairo_image_surface_get_height(*surface));
        cairo_fill(cr);

        /* walk through all hardware LED adapters */
        LedHardware *hw;
        for(hw = led_setup_get_hardware(s);
            hw; hw = led_hardware_list_get_next(hw))
        {

				//~ /* calculate offset of complete setup */
				LedTile *t;
				double xOff = 0, yOff = 0;
                for(t = led_hardware_get_tile(hw);
                    t; t = led_tile_list_get_next(t))
				{
						double xOffT, yOffT;
						tile_calc_render_offset(led_tile_get_privdata(t),
						                        (double) w, (double) h, 
												&xOffT, &yOffT);
						xOff = MIN(xOff, xOffT);
						yOff = MIN(yOff, yOffT);                
				}

				renderer_set_offset(setup_get_renderer(),
										xOff * renderer_scale_factor(),
										yOff * renderer_scale_factor());
						
                /* Walk all tiles of this hardware & draw their surface */                
                for(t = led_hardware_get_tile(hw);
                    t; t = led_tile_list_get_next(t))
                {
                        /** @todo check for visibility? */

                        /* get surface from tile */
                        NiftyconfTile *tile =
                                (NiftyconfTile *) led_tile_get_privdata(t);

						/* get surface */
						cairo_surface_t *tsurface = renderer_get_surface
                                         (tile_get_renderer(tile));
						
						/* compensate child tiles' offset */
						double xOffT, yOffT;
						renderer_get_offset(tile_get_renderer(tile), &xOffT, &yOffT);
                		cairo_translate(cr, xOffT, yOffT);

						/* compensate setup offset */
						cairo_translate(cr, 
						                -xOff*renderer_scale_factor(), 
						                -yOff*renderer_scale_factor());
						
                        /* move to x/y */
                        LedFrameCord x, y;
                        led_tile_get_pos(t, &x, &y);
                        cairo_translate(cr,
                                        ((double) x) *
                                        renderer_scale_factor(),
                                        ((double) y) *
                                        renderer_scale_factor());

                        /* rotate around pivot */
                        double pX, pY;
                        led_tile_get_pivot(t, &pX, &pY);
                        cairo_translate(cr,
                                        pX * renderer_scale_factor(),
                                        pY * renderer_scale_factor());
                        cairo_rotate(cr, led_tile_get_rotation(t));
                        cairo_translate(cr,
                                        -pX * renderer_scale_factor(),
                                        -pY * renderer_scale_factor());

                        /* draw surface */
                        cairo_set_source_surface(cr, tsurface, 0, 0);

                        /* disable filtering */
                        cairo_pattern_set_filter(cairo_get_source(cr),
                                                 renderer_filter());
                        /* disable antialiasing */
                        cairo_set_antialias(cr, renderer_antialias());

                        /* render surface */
                        cairo_paint(cr);

                        /* reset transformation matrix */
                        cairo_identity_matrix(cr);
                }

                /* Draw chain of this hardware */
                /* s =
                 * chain_get_surface(led_chain_get_privdata(led_hardware_get_chain(h)));
                 * cairo_set_source_surface(cr, s, 0, 0); cairo_paint(cr); */

        }

        cairo_destroy(cr);

        return NFT_SUCCESS;
}


/******************************************************************************
 ******************************************************************************/

/** allocate new renderer for a Setup */
NiftyconfRenderer *renderer_setup_new()
{
        LedSetup *s = setup_get_current();

        if(!s)
                NFT_LOG_NULL(NULL);

        /* dimensions of cairo surface */
        gint width, height;
        led_setup_get_dim(s, &width, &height);

        width *= renderer_scale_factor();
        height *= renderer_scale_factor();

        return renderer_new(LED_SETUP_T, s, &_render_setup, width, height);
}


/** damage setup renderer to queue re-render */
void renderer_setup_damage()
{
        renderer_damage(setup_get_renderer());
}





/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
