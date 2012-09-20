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
#include "niftyconf-ui.h"
#include "niftyconf-setup-props.h"
#include "niftyconf-led.h"
#include "niftyconf-chain.h"



/** one element */
struct _NiftyconfChain
{
        /** niftyled descriptor */
        LedChain *c;
        /** true if element is currently highlighted */
        gboolean highlight;
};






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/





/******************************************************************************
 ******************************************************************************/






/** getter for boolean value whether element is currently highlighted */
gboolean chain_tree_get_highlighted(NiftyconfChain *c)
{
        if(!c)
                NFT_LOG_NULL(FALSE);
        
        return c->highlight;
}


/* setter for boolean value whether element is currently highlighted */
void chain_tree_set_highlighted(NiftyconfChain *c, gboolean is_highlighted)
{
        if(!c)
                NFT_LOG_NULL();

        c->highlight = is_highlighted;
}


/** getter for libniftyled object */
LedChain *chain_niftyled(NiftyconfChain *c)
{
        if(!c)
                return NULL;
        
        return c->c;
}


/** unregister all LEDs of a chain */
void chain_unregister_leds(NiftyconfChain *c)
{
        if(!c)
                NFT_LOG_NULL();
        
        /* free all LEDs of chain */
        if(c->c)
        {
                LedCount i;
                for(i = 0; i < led_chain_get_ledcount(c->c); i++)
                {
                        led_unregister(led_get_privdata(led_chain_get_nth(c->c, i)));
                }
        }
}

/** register all LEDs of a chain */
void chain_register_leds(NiftyconfChain *c)
{
        /* allocate all LEDs of chain */
        LedCount i;
        for(i = 0; i < led_chain_get_ledcount(c->c); i++)
        {
                led_register(led_chain_get_nth(c->c, i));
        }
}


/** allocate new element */
NiftyconfChain *chain_register(LedChain *c)
{
        NiftyconfChain *n;
        if(!(n = calloc(1, sizeof(NiftyconfChain))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        /* save descriptor */
        n->c = c;

        /* register descriptor as niftyled privdata */
        led_chain_set_privdata(c, n);

        chain_register_leds(n);

	return n;
}


/** free element */
void chain_unregister(NiftyconfChain *c)
{
        if(!c)
                return;

        chain_unregister_leds(c);
        led_chain_set_privdata(c->c, NULL);

        free(c);
}


/** initialize chain module */
gboolean chain_init()
{
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
