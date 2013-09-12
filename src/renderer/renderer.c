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
#include "ui/ui.h"
#include "renderer/renderer.h"





/** one renderer - renders one niftyconf element (setup, hardware, tile, ...) */
struct _NiftyconfRenderer
{
                /** element type */
        NIFTYLED_TYPE type;
                /** element */
        gpointer element;
                /** drawing surface */
        cairo_surface_t *surface;
                /** true if surface needs to be redrawn */
        bool damaged;
                /** render function */
        NiftyconfRenderFunc *render;
                /** rendering offset */
        gdouble xOffset, yOffset;
};




/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


/******************************************************************************
 ******************************************************************************/



/** getter for cairo surface */
cairo_surface_t *renderer_get_surface(NiftyconfRenderer * r)
{
        if(!r)
                NFT_LOG_NULL(NULL);

        /* if renderer is marked as damaged, re-render it's surface */
        if(r->damaged)
        {
                /* do we have a renderer? */
                if(r->render)
                {
                        if(!r->render(&r->surface, r->element))
                        {
                                NFT_LOG(L_ERROR, "%s renderer (%p) failed",
                                        led_prefs_type_to_string(r->type),
                                        r->element);
                        }
                }
                else
                        NFT_LOG(L_DEBUG,
                                "surface marked as damaged but no function to render it.");

                /* repair renderer :-P */
                r->damaged = false;
        }

        return r->surface;
}


/** queue this renderers surface for update */
void renderer_damage(NiftyconfRenderer * r)
{
        if(!r)
                NFT_LOG_NULL();

        r->damaged = true;
}


/** allocate new renderer */
NiftyconfRenderer *renderer_new(NIFTYLED_TYPE type,
                                gpointer element,
                                NiftyconfRenderFunc * render,
                                gint width, gint height)
{
        if(!element)
                NFT_LOG_NULL(NULL);

        NiftyconfRenderer *n;
        if(!(n = calloc(1, sizeof(NiftyconfRenderer))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        n->type = type;
        n->element = element;
        n->render = render;

        if(!(n->surface =
             cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
        {
                g_error("failed to create cairo surface (%dx%d)",
                        width, height);
                free(n);
                return NULL;
        }


        return n;
}


/** destroy renderer */
void renderer_destroy(NiftyconfRenderer * r)
{
        if(!r)
                NFT_LOG_NULL();

        if(r->surface)
        {
                cairo_surface_destroy(r->surface);
                r->surface = NULL;
        }

        free(r);
}


/** resize surface of renderer */
gboolean renderer_resize(NiftyconfRenderer * r, int width, int height)
{
        if(!r || !r->surface)
                NFT_LOG_NULL(false);

        /* silently succeed if size is as requested */
        if(width == cairo_image_surface_get_width(r->surface) &&
           height == cairo_image_surface_get_height(r->surface))
                return true;

        /* destroy old surface */
        cairo_surface_destroy(r->surface);

        if(!(r->surface =
             cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
                return false;

        /* queue renderer for update */
        renderer_damage(r);

        return true;
}


/** set drawing offset for this renderer */
gboolean renderer_set_offset(NiftyconfRenderer * r, double xOff, double yOff)
{
        if(!r)
                NFT_LOG_NULL(false);

        r->xOffset = xOff;
        r->yOffset = yOff;

        return true;
}

/** get drawing offset for this renderer */
gboolean renderer_get_offset(NiftyconfRenderer * r, double *xOff,
                             double *yOff)
{
        if(!r)
                NFT_LOG_NULL(false);

        if(xOff)
                *xOff = r->xOffset;
        if(yOff)
                *yOff = r->yOffset;

        return true;
}
