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
#include "elements/element-led.h"
#include "renderer/renderer.h"
#include "renderer/renderer-led.h"
#include "renderer/renderer-tile.h"




/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** renderer for chains */
static NftResult _render_chain(cairo_surface_t ** s, gpointer element)
{
        if(!s || !*s || !element)
                NFT_LOG_NULL(NFT_FAILURE);

        /* get this chain */
        NiftyconfChain *chain = (NiftyconfChain *) element;
        LedChain *c = chain_niftyled(chain);

        /* if dimensions changed, we need to allocate a new surface */
        int width = (led_chain_get_max_x(c) + 1) * renderer_scale_factor();
        int height = (led_chain_get_max_y(c) + 1) * renderer_scale_factor();

        NiftyconfRenderer *r = chain_get_renderer(chain);
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
        cairo_set_source_rgba(cr, 0, 0, 0, 1);
        cairo_rectangle(cr,
                        0, 0,
                        (double) cairo_image_surface_get_width(*s),
                        (double) cairo_image_surface_get_height(*s));
        cairo_fill(cr);


        /* set line-width */
        cairo_set_line_width(cr, 1);



        /* walk all LEDs */
        LedCount i;
        for(i = 0; i < led_chain_get_ledcount(c); i++)
        {
                Led *l = led_chain_get_nth(c, i);
                NiftyconfLed *led = led_get_privdata(l);
                NiftyconfRenderer *lr = led_get_renderer(led);
                cairo_set_source_surface(cr, renderer_get_surface(lr),
                                         (double) led_get_x(l) *
                                         renderer_scale_factor(),
                                         (double) led_get_y(l) *
                                         renderer_scale_factor());
                cairo_paint(cr);
        }

        cairo_destroy(cr);


        return NFT_SUCCESS;
}

/******************************************************************************
 ******************************************************************************/

/** damage chain renderer to queue re-render */
void renderer_chain_damage(NiftyconfChain * chain)
{
        LedChain *c = chain_niftyled(chain);

        /* damage this chain's renderer */
        renderer_damage(chain_get_renderer(chain));

        /* also damage parent tile */
        LedTile *t;
        if((t = led_chain_get_parent_tile(c)))
        {
                NiftyconfTile *tile = led_tile_get_privdata(t);
                renderer_tile_damage(tile);
        }
}


/** allocate new renderer for a Chain */
NiftyconfRenderer *renderer_chain_new(NiftyconfChain * chain)
{
        if(!chain)
                NFT_LOG_NULL(NULL);

        /* dimensions of cairo surface */
        LedChain *c = chain_niftyled(chain);
        int width = (led_chain_get_max_x(c) + 1) * renderer_scale_factor();
        int height = (led_chain_get_max_y(c) + 1) * renderer_scale_factor();

        return renderer_new(LED_CHAIN_T, chain, &_render_chain, width,
                            height);
}





/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
