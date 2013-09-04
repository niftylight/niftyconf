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
#include "elements/element-led.h"
#include "elements/element-setup.h"
#include "renderer/renderer.h"
#include "renderer/renderer-led.h"
#include "live-preview/live-preview.h"


/** one element */
struct _NiftyconfLed
{
                /** niftyled descriptor */
        Led *l;
                /** renderer */
        NiftyconfRenderer *renderer;
                /** true if element is currently highlighted */
        gboolean highlight;
                /** chain to which this LED belongs */
        NiftyconfChain *chain;
                /** position of this Led inside its chain */
        LedCount pos;
};





/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/** getter for position inside chain */
LedCount led_get_chainpos(NiftyconfLed * l)
{
        if(!l)
                NFT_LOG_NULL(0);

        return l->pos;
}


/** getter for parent chain */
NiftyconfChain *led_get_chain(NiftyconfLed * l)
{
        if(!l)
                NFT_LOG_NULL(NULL);

        return l->chain;
}


/** getter for renderer */
NiftyconfRenderer *led_get_renderer(NiftyconfLed * l)
{
        if(!l)
                NFT_LOG_NULL(NULL);

        return l->renderer;
}


/** getter for boolean value whether element is currently highlighted */
gboolean led_get_highlighted(NiftyconfLed * l)
{
        if(!l)
                NFT_LOG_NULL(false);

        return l->highlight;
}


/* setter for boolean value whether element is currently highlighted */
void led_set_highlighted(NiftyconfLed * l, gboolean is_highlighted)
{
        if(!l)
                NFT_LOG_NULL();

        l->highlight = is_highlighted;

        /* highlight real hardware */
        if(is_highlighted)
        {
                live_preview_highlight_led(l);
        }
}


/**
 * getter for libniftyled object
 */
Led *led_niftyled(NiftyconfLed * l)
{
        if(!l)
                return NULL;

        return l->l;
}


/** dump element definition to printable string - use free() to deallacote the result */
char *led_dump(NiftyconfLed * led, gboolean encapsulation)
{
        Led *l = led_niftyled(led);
        LedPrefsNode *n;
        if(!(n = led_prefs_led_to_node(setup_get_prefs(), l)))
                return NULL;

        char *result = NULL;
        if(encapsulation)
                result = led_prefs_node_to_buffer(n);
        else
                result = led_prefs_node_to_buffer_light(n);

        led_prefs_node_free(n);

        return result;
}


/**
 * allocate new element
 */
NiftyconfLed *led_register_to_gui(Led * l,
                                  NiftyconfChain * chain, LedCount pos)
{
        NiftyconfLed *n;
        if(!(n = calloc(1, sizeof(NiftyconfLed))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        /* save descriptor */
        n->l = l;
        n->chain = chain;
        n->pos = pos;

        /* allocate renderer */
        if(!(n->renderer = renderer_led_new(n)))
        {
                g_error("Failed to allocate renderer for Led");
                led_unregister_from_gui(n);
                return NULL;
        }

        /* initially draw led */
        renderer_led_damage(n);

        /* register descriptor as niftyled privdata */
        led_set_privdata(l, n);

        return n;
}


/**
 * free element
 */
void led_unregister_from_gui(NiftyconfLed * l)
{
        if(!l)
                return;

        /* destroy renderer of this tile */
        renderer_destroy(l->renderer);

        led_set_privdata(l->l, NULL);

        free(l);
}


/**
 * initialize led module
 */
gboolean led_init()
{
        return true;
}


/** deinitialize this module */
void led_deinit()
{

}
