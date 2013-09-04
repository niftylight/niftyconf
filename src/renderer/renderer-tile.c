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

#include <gtk/gtk.h>
#include <niftyled.h>
#include "elements/element-setup.h"
#include "elements/element-tile.h"
#include "elements/element-chain.h"
#include "renderer/renderer.h"
#include "renderer/renderer-chain.h"






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** renderer for tiles */
static NftResult _render_tile(cairo_surface_t ** s, gpointer element)
{
        if(!s || !element)
                NFT_LOG_NULL(NFT_FAILURE);

        /* get this tile */
        NiftyconfTile *tile = (NiftyconfTile *) element;
        LedTile *t = tile_niftyled(tile);

        /* if dimensions changed, we need to allocate a new surface */
        int w, h;
        led_tile_get_dim(t, &w, &h);

        NiftyconfRenderer *r = tile_get_renderer(tile);
        double width = (double) w * renderer_scale_factor();
        double height = (double) h * renderer_scale_factor();
        if(!renderer_resize(r, width, height))
        {
                g_error("Failed to resize renderer to %.0fx%.0f",
                        width, height);
                return NFT_FAILURE;
        }


        /* create context for drawing */
        cairo_t *cr = cairo_create(*s);


        /* clear surface */
        cairo_set_source_rgba(cr, 0, 0, 0, 1);
        cairo_rectangle(cr,
                        0, 0,
                        (double) cairo_image_surface_get_width(*s),
                        (double) cairo_image_surface_get_height(*s));
        cairo_fill(cr);


        /* render children */
        LedTile *ct;
        for(ct = led_tile_get_child(t); ct; ct = led_tile_list_get_next(ct))
        {
                NiftyconfTile *ctt =
                        (NiftyconfTile *) led_tile_get_privdata(ct);


                // ~ /* calculate offscreen compensation */
                // ~ double xOff, yOff;
                // ~ tile_calc_render_offset(tile, 
                // ~ (double) w,
                // ~ (double) h, 
                // ~ &xOff, &yOff);
                // ~ renderer_set_offset(tile_get_renderer(tile),
                // ~ -xOff * renderer_scale_factor(),
                // ~ -yOff * renderer_scale_factor());
                // ~ cairo_translate(cr,
                // ~ xOff * renderer_scale_factor(),
                // ~ yOff * renderer_scale_factor());

                /* move to x/y */
                LedFrameCord x, y;
                led_tile_get_pos(ct, &x, &y);
                cairo_translate(cr,
                                (double) x * renderer_scale_factor(),
                                (double) y * renderer_scale_factor());

                /* rotate around pivot */
                double pX, pY;
                led_tile_get_pivot(ct, &pX, &pY);
                cairo_translate(cr,
                                pX * renderer_scale_factor(),
                                pY * renderer_scale_factor());

                cairo_rotate(cr, led_tile_get_rotation(ct));
                cairo_translate(cr,
                                -pX * renderer_scale_factor(),
                                -pY * renderer_scale_factor());

                /* draw */
                cairo_set_source_surface(cr,
                                         renderer_get_surface
                                         (tile_get_renderer(ctt)), 0, 0);

                /* disable filtering */
                cairo_pattern_set_filter(cairo_get_source(cr),
                                         renderer_filter());
                /* disable antialiasing */
                cairo_set_antialias(cr, renderer_antialias());

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
                cairo_set_source_surface(cr,
                                         renderer_get_surface
                                         (chain_get_renderer(ch)), 0, 0);

                /* disable filtering */
                cairo_pattern_set_filter(cairo_get_source(cr),
                                         renderer_filter());
                /* disable antialiasing */
                cairo_set_antialias(cr, renderer_antialias());

                cairo_paint(cr);
        }



        /* highlighted outlines? */
        if(tile_get_highlighted(tile))
        {
                /* set white */
                cairo_set_source_rgba(cr, 1, 1, 0, 1);
                /* set line-width */
                cairo_set_line_width(cr, renderer_scale_factor() / 5);
        }
        /* normal outlines */
        else
        {
                cairo_set_source_rgba(cr, 1, 1, 1, 1);
                cairo_set_line_width(cr, renderer_scale_factor() / 8);
        }


        /* draw tile pivot */
        double pX, pY;
        led_tile_get_pivot(t, &pX, &pY);
        cairo_translate(cr,
                        pX * renderer_scale_factor(),
                        pY * renderer_scale_factor());

#define PIVOT_FACTOR (renderer_scale_factor()/5)
        cairo_move_to(cr, -PIVOT_FACTOR, -PIVOT_FACTOR);
        cairo_line_to(cr, PIVOT_FACTOR, PIVOT_FACTOR);
        cairo_move_to(cr, PIVOT_FACTOR, -PIVOT_FACTOR);
        cairo_line_to(cr, -PIVOT_FACTOR, PIVOT_FACTOR);
        cairo_stroke(cr);

        cairo_translate(cr,
                        -pX * renderer_scale_factor(),
                        -pY * renderer_scale_factor());


        /* draw tile outlines */
        cairo_rectangle(cr, 0, 0, width, height);
        cairo_stroke(cr);



        /* highlight... */
        if(tile_get_highlighted(tile))
        {
                cairo_set_line_width(cr, 1);
                cairo_set_source_rgba(cr, 1, 1, 1, 0.5);

                cairo_rectangle(cr,
                                0, 0,
                                (double) cairo_image_surface_get_width(*s),
                                (double) cairo_image_surface_get_height(*s));
                cairo_fill(cr);
        }


        /* draw arrow to mark top */
        cairo_set_source_rgba(cr, 1, 1, 1, 1);

        if(tile_get_highlighted(tile))
                cairo_set_line_width(cr, renderer_scale_factor() / 5);
        else
                cairo_set_line_width(cr, renderer_scale_factor() / 10);

        double cx = width / 2;
        double cy = height / 10;
        double ax = cx - (width / 5);
        double ay = cy + (height / 5);
        double bx = cx + (width / 5);
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

/** damage tile renderer to queue re-render */
void renderer_tile_damage(NiftyconfTile * tile)
{
        LedTile *t = tile_niftyled(tile);

        /* damage this tile's renderer */
        renderer_damage(tile_get_renderer(tile));

        /* also damage parent tile... */
        LedTile *pt;
        if((pt = led_tile_get_parent_tile(t)))
        {
                NiftyconfTile *ptile = led_tile_get_privdata(pt);
                renderer_tile_damage(ptile);
        }

        /* ...and the setup that contains us */
        renderer_damage(setup_get_renderer());
}


/** allocate new renderer for a Tile */
NiftyconfRenderer *renderer_tile_new(NiftyconfTile * tile)
{
        if(!tile)
                NFT_LOG_NULL(NULL);

        /* dimensions of cairo surface */
        LedTile *t = tile_niftyled(tile);
        gint width, height;
        if(!led_tile_get_dim(t, &width, &height))
                return NULL;

        width *= renderer_scale_factor();
        height *= renderer_scale_factor();

        return renderer_new(LED_TILE_T, tile, &_render_tile, width, height);
}




/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
