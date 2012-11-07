/*
 * niftyconf - niftyled GUI
 * Copyright (C) 2011-2012 Daniel Hiepler <daniel@niftylight.de>
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

#include <gtk/gtk.h>
#include <niftyled.h>
#include "elements/element-tile.h"
#include "elements/element-chain.h"
#include "renderer/renderer.h"
#include "renderer/renderer-chain.h"






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** renderer for tiles */
static NftResult _render_tile(cairo_surface_t **s, gpointer element)
{
		if(!s || !element)
				NFT_LOG_NULL(NFT_FAILURE);
			
		/* get this tile */
		NiftyconfTile *tile = (NiftyconfTile *) element;
		LedTile *t = tile_niftyled(tile);
		
		/* if dimensions changed, we need to allocate a new surface */
		int width = led_tile_get_width(t)*renderer_scale_factor();
		int height = led_tile_get_height(t)*renderer_scale_factor();
		//~ int width = led_tile_get_transformed_width(t)*renderer_scale_factor();
		//~ int height = led_tile_get_transformed_height(t)*renderer_scale_factor();

		NiftyconfRenderer *r = tile_get_renderer(tile);
		if(!renderer_resize(r, width, height))
		{
				g_error("Failed to resize renderer to %dx%d", width, height);
				return NFT_FAILURE;
		}


		/* create context for drawing */
		cairo_t *cr = cairo_create(*s);

		/* disable antialiasing */
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

		/* clear surface */
		cairo_set_source_rgba(cr, 0,0,0,1);
		cairo_rectangle(cr,
		                0, 0,
		                (double) cairo_image_surface_get_width(*s),
		                (double) cairo_image_surface_get_height(*s));
		cairo_fill(cr);



		/* redraw children */
		LedTile *ct;
		for(ct = led_tile_get_child(t);
		    ct;
		    ct = led_tile_list_get_next(ct))
		{
				NiftyconfTile *ctt = (NiftyconfTile *) led_tile_get_privdata(ct);

				
				/* move to x/y */
				cairo_translate(cr,
				                (double) (led_tile_get_x(ct))*renderer_scale_factor(),
				                (double) (led_tile_get_y(ct))*renderer_scale_factor());

				/* rotate */
				cairo_translate(cr,
				                (led_tile_get_transformed_pivot_x(ct))*renderer_scale_factor(),
				                (led_tile_get_transformed_pivot_y(ct))*renderer_scale_factor());

				cairo_rotate(cr, led_tile_get_rotation(ct));
				cairo_translate(cr,
				                -(led_tile_get_pivot_x(ct))*renderer_scale_factor(),
				                -(led_tile_get_pivot_y(ct))*renderer_scale_factor());

				/* adapt to new pivot */
				/*cairo_translate(cr,
				                  (led_tile_get_pivot_y(ct)-led_tile_get_transformed_pivot_x(ct))*renderer_scale_factor(),
								  (led_tile_get_pivot_x(ct)-led_tile_get_transformed_pivot_y(ct))*renderer_scale_factor());*/

				/* draw */
				cairo_set_source_surface(cr, renderer_get_surface(tile_get_renderer(ctt)), 0, 0);


				cairo_fill(cr);
				cairo_paint(cr);

				/* reset transformation matrix */
				cairo_identity_matrix(cr);

		}



		/* redraw chain */
		LedChain *chain;
		if((chain = led_tile_get_chain(t)))
		{
				/* redraw chain */
				NiftyconfChain *ch = led_chain_get_privdata(chain);

				/* draw chain's surface to parent tile's surface */
				cairo_set_source_surface(cr, renderer_get_surface(chain_get_renderer(ch)), 0,0);
				cairo_paint(cr);
		}



		if(tile_tree_get_highlighted(tile))
		{
				/* set white */
				cairo_set_source_rgba(cr, 1, 1, 0, 1);
				/* set line-width */
				cairo_set_line_width (cr, 4);
		}
		else
		{
				cairo_set_source_rgba(cr, 1, 1, 1, 1);
				cairo_set_line_width (cr, 2);
		}

		
		/* draw tile outlines */
		cairo_rectangle(cr, 0, 0,
		                (double) led_tile_get_transformed_width(t)*renderer_scale_factor(),
		                (double) led_tile_get_transformed_height(t)*renderer_scale_factor());
		cairo_stroke(cr);



		/* highlight... */
		if(tile_tree_get_highlighted(tile))
		{
				cairo_set_line_width (cr, 1);
				cairo_set_source_rgba(cr, 1,1,1,0.5);

				cairo_rectangle(cr,
				                0, 0,
				                (double) cairo_image_surface_get_width(*s),
				                (double) cairo_image_surface_get_height(*s));
				cairo_fill(cr);
		}

		
		/* draw arrow to mark top */
		cairo_set_source_rgba(cr, 1,1,1,1);

		if(tile_tree_get_highlighted(tile))
				cairo_set_line_width (cr, 8);
		else
				cairo_set_line_width (cr, 2);

		double w = (double) led_tile_get_width(t)*renderer_scale_factor();
		double h = (double) led_tile_get_height(t)*renderer_scale_factor();
		double cx = w/2;
		double cy = h/10;
		double ax = cx - (w/5);
		double ay = cy + (h/5);
		double bx = cx + (w/5);
		double by = ay;
		cairo_move_to(cr, ax, ay);
		cairo_line_to(cr, bx, by);
		cairo_line_to(cr, cx, cy);
		cairo_close_path(cr);
		cairo_stroke(cr);


		
		cairo_destroy(cr);

		
		return NFT_SUCCESS;
}

/******************************************************************************
 ******************************************************************************/

/** allocate new renderer for a Tile */
NiftyconfRenderer *renderer_tile_new(NiftyconfTile *tile)
{
		if(!tile)
				NFT_LOG_NULL(NULL);

		/* dimensions of cairo surface */
		LedTile *t = tile_niftyled(tile);
		gint width = led_tile_get_width(t)*renderer_scale_factor();
		gint height = led_tile_get_height(t)*renderer_scale_factor();

		return renderer_new(LED_TILE_T, tile, &_render_tile, width, height);
}




/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
