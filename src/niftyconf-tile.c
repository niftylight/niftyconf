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
#include "niftyconf-chain.h"
#include "niftyconf-tile.h"



/** one element */
struct _NiftyconfTile
{
        /** niftyled descriptor */
        LedTile *t;
        /** true if element is currently highlighted */
        gboolean highlight;
        /** true if element tree is currently collapsed */
        gboolean collapsed;
};


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/** getter for boolean value whether element is currently highlighted */
gboolean tile_tree_get_highlighted(NiftyconfTile *t)
{
        if(!t)
                NFT_LOG_NULL(FALSE);
        
        return t->highlight;
}


/* setter for boolean value whether element is currently highlighted */
void tile_tree_set_highlighted(NiftyconfTile *t, gboolean is_highlighted)
{
        if(!t)
                NFT_LOG_NULL();

        t->highlight = is_highlighted;
}


/**
 * getter for boolean value whether element 
 * tree is currently collapsed
 */
gboolean tile_tree_get_collapsed(NiftyconfTile *t)
{
        if(!t)
                NFT_LOG_NULL(FALSE);

        return t->collapsed;
}


/**
 * setter for boolean value whether element row in
 * tree is currently collapsed
 */
void tile_tree_set_collapsed(NiftyconfTile *t, gboolean is_collapsed)
{
        if(!t)
                NFT_LOG_NULL();

        t->collapsed = is_collapsed;
}


/**
 * getter for libniftyled object
 */
LedTile *tile_niftyled(NiftyconfTile *t)
{
        if(!t)
                return NULL;
        
        return t->t;
}


/**
 * allocate new element
 */
NiftyconfTile *tile_register_to_gui(LedTile *t)
{
        /* also allocate all children */
        LedTile *tile;
        for(tile = led_tile_get_child(t);
            tile;
            tile = led_tile_list_get_next(tile))
        {
                tile_register_to_gui(tile);
        }

        /* allocate chain if this tile has one */
        LedChain *c;
        if((c = led_tile_get_chain(t)))
        {
                chain_register_to_gui(c);
        }
        
        NiftyconfTile *n;
        if(!(n = calloc(1, sizeof(NiftyconfTile))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        /* save descriptor */
        n->t = t;

        /* default hardware is collapsed */
        n->collapsed = TRUE;
        
        /* register descriptor as niftyled privdata */
        led_tile_set_privdata(t, n);

	return n;
}


/**
 * free element
 */
void tile_unregister_from_gui(NiftyconfTile *t)
{
        if(!t)
                return;

        if(t->t)
        {
                /* free all children */
                LedTile *tile;
                for(tile = led_tile_get_child(t->t);
                    tile;
                    tile = led_tile_list_get_next(tile))
                {
                        tile_unregister_from_gui(led_tile_get_privdata(tile));
                }
                
                led_tile_set_privdata(t->t, NULL);
        }
        
        free(t);
}


/** create new tile for hardware parent */
gboolean tile_of_hardware_new(NiftyconfHardware *parent)
{
        /* create new tile */
        LedTile *n;
        if(!(n = led_tile_new()))
                return FALSE;

        /* get last tile of this hardware */
        LedHardware *h = hardware_niftyled(parent);
        
        /* does hw already have a tile? */
        LedTile *tile;
        if(!(tile = led_hardware_get_tile(h)))
        {
                led_hardware_set_tile(h, n);
        }
        else
        {
                led_tile_list_append_head(tile, n);
        }

        /* register new tile to gui */
        tile_register_to_gui(n);

        /* create config */
        //if(!led_settings_create_from_tile(setup_get_current(), n))
        //        return FALSE;
        
        return TRUE;
}


/** create new tile for tile parent */
gboolean tile_of_tile_new(NiftyconfTile *parent)
{
        /* create new tile */
        LedTile *n;
        if(!(n = led_tile_new()))
                return FALSE;

        LedTile *tile = tile_niftyled(parent);
        led_tile_append_child(tile, n);

        /* register new tile to gui */
        tile_register_to_gui(n);

        /* create config */
        //if(!led_settings_create_from_tile(setup_get_current(), n))
        //        return FALSE;
        
        return TRUE;
}


/** remove tile from current setup */
void tile_destroy(NiftyconfTile *tile)
{
        if(!tile)
                NFT_LOG_NULL();
        
        LedTile *t = tile_niftyled(tile);
        
        /* unregister from gui */
        tile_unregister_from_gui(tile);
        /* unregister from settings */
        //led_settings_tile_unlink(setup_get_current(), t);
        /* destroy with all children */
        led_tile_destroy(t);
}


/**
 * initialize tile module
 */
gboolean tile_init()
{
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
