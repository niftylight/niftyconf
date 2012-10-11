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
#include "elements/niftyconf-led.h"
#include "renderer/niftyconf-renderer.h"
#include "renderer/niftyconf-renderer-led.h"





/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/** allocate new renderer for a Chain */
NiftyconfRenderer *renderer_chain_new(NiftyconfChain *chain)
{
	if(!chain)
		NFT_LOG_NULL(NULL);

	/* dimensions of cairo surface */
	LedChain *c = chain_niftyled(chain);
        int width = (led_chain_get_max_x(c)+1)*renderer_scale_factor();
        int height = (led_chain_get_max_y(c)+1)*renderer_scale_factor();

        return renderer_new(LED_CHAIN_T, chain, width, height);
}


/** draw Chain using cairo */
void renderer_chain_redraw(NiftyconfChain *chain)
{
	if(!chain)
		NFT_LOG_NULL();


	/* get this chain */
	LedChain *c = chain_niftyled(chain);

	/* get renderer of this chain */
	NiftyconfRenderer *r = chain_get_renderer(chain);

	/* if dimensions of our chain changed, resize renderer surface */
	int width = (led_chain_get_max_x(c)+1)*renderer_scale_factor();
	int height = (led_chain_get_max_y(c)+1)*renderer_scale_factor();
	if(!renderer_resize(r, width, height))
	{
		g_error("Failed to resize renderer to %dx%d", width, height);
		return;
	}

	/* get cairo surface of this renderer */
       	cairo_surface_t *s = renderer_get_surface(r);

	 /* create context for drawing */
        cairo_t *cr = cairo_create(s);

        /* disable antialiasing */
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

        /* clear surface */
        cairo_set_source_rgba(cr, 0,0,0,1);
        cairo_rectangle(cr,
                        0, 0,
                        (double) cairo_image_surface_get_width(s),
                        (double) cairo_image_surface_get_height(s));
        cairo_fill(cr);


        /* set line-width */
        cairo_set_line_width (cr, 1);



        /* walk all LEDs */
        int i;
        for(i = 0; i < led_chain_get_ledcount(c); i++)
        {
		Led *l = led_chain_get_nth(c, i);
                NiftyconfLed *led = led_get_privdata(l);
		NiftyconfRenderer *lr = led_get_renderer(led);
                renderer_led_redraw(led);
                cairo_set_source_surface(cr, renderer_get_surface(lr),
                                         (double) led_get_x(l)*renderer_scale_factor(),
                                         (double) led_get_y(l)*renderer_scale_factor());
                cairo_paint(cr);
        }

        cairo_destroy(cr);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
