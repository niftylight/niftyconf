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

#include <niftyled.h>
#include <gtk/gtk.h>
#include "niftyconf-setup.h"
#include "niftyconf-setup-tree.h"
#include "niftyconf-hardware.h"
#include "niftyconf-tile.h"
#include "niftyconf-chain.h"
#include "niftyconf-led.h"
#include "niftyconf-log.h"



/**
 * This is simple & generic.
 * For every setup-element we copy the XML representation to
 * the clipboard. If we cut, the element is removed from the setup and destroyed.
 * Upon paste, the XML data is parsed again and, if valid, a new element is created
 * to be inserted as child of the currently selected element (LedHardware
 * elements will always be added to the toplevel of the setup)
 *
 * @todo make this ready for proper multi-select copy of tree elements
 */


/** our clipboard */
static GtkClipboard *_clipboard;






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


/** cut/copy to buffer */
static void _cut_or_copy_element(NIFTYLED_TYPE t, gpointer *e, gboolean cut)
{
	NFT_LOG(L_DEBUG, cut ? "Cutting element..." : "Copying element...");

	const char *xml = NULL;
        
        switch(t)
        {
                case LED_HARDWARE_T:
                {
                        LedHardware *h = hardware_niftyled((NiftyconfHardware *) e);
			LedPrefsNode *n = led_prefs_hardware_to_node(setup_get_prefs(), h);
                        xml = led_prefs_node_to_buffer(setup_get_prefs(), n);
			led_prefs_node_free(n);
			
                        /* remove element? */
                        if(cut)
                        {
				setup_destroy_hardware((NiftyconfHardware *) e);
                                setup_tree_refresh();
                        }
                        break;
                }

                case LED_TILE_T:
                {
                        LedTile *t = tile_niftyled((NiftyconfTile *) e);
			LedPrefsNode *n = led_prefs_tile_to_node(setup_get_prefs(), t);
                        xml = led_prefs_node_to_buffer(setup_get_prefs(), n);
			led_prefs_node_free(n);
			
                        /* remove element? */
                        if(cut)
                        {
				setup_destroy_tile((NiftyconfTile *) e);
                                setup_tree_refresh();
                        }
                        break;
                }

                case LED_CHAIN_T:
                {
                        LedChain *c = chain_niftyled((NiftyconfChain *) e);
			LedPrefsNode *n = led_prefs_chain_to_node(setup_get_prefs(), c);
                        xml = led_prefs_node_to_buffer(setup_get_prefs(), n);
			led_prefs_node_free(n);
			
                        /* don't cut from hardware elements */
                        if(led_chain_parent_is_hardware(c))
                                break;
                        
                        /* remove element? */
                        if(cut)
                        {
				/* get parent tile of this chain */
				LedTile *t = led_chain_get_parent_tile(c);
				NiftyconfTile *tile = led_tile_get_privdata(t);
				setup_destroy_chain_of_tile(tile);
                                setup_tree_refresh();
                        }
                        break;
                }

		case LED_T:
		{
			Led *l = led_niftyled((NiftyconfLed *) e);
			LedPrefsNode *n = led_prefs_led_to_node(setup_get_prefs(), l);
			xml = led_prefs_node_to_buffer(setup_get_prefs(), n);
			led_prefs_node_free(n);

			/* remove element? */
			if(cut)
			{
				NFT_TODO();
				//led_unregister((NiftyconfLed *) e);
				//setup_tree_refresh();
			}
		}
			
		default:
		{
			NFT_LOG(L_ERROR, "Attempt to cut/copy unknown element. This shouldn't happen?!");
		}
        }
	
   
        if(!xml)
                return;

        /* set the clipboard text */
        gtk_clipboard_set_text(_clipboard, xml, -1);

        /* store the clipboard text */
        gtk_clipboard_store(_clipboard);

        NFT_LOG(L_VERY_NOISY, "%s", xml);
        
        free((void *) xml);
}


/** paste element from clipboard */
static void _paste_element(NIFTYLED_TYPE parent_t, gpointer *parent_element)
{
	/* get XML data from clipboard */
        gchar *xml;
        if(!(xml = gtk_clipboard_wait_for_text(_clipboard)))
        {
                NFT_LOG(L_ERROR, "received NULL from clipboard?");
                return;
        }

	/* parse buffer to LedPrefsNode */
	LedPrefsNode *n;
	if(!(n = led_prefs_node_from_buffer(setup_get_prefs(), xml, strlen(xml))))
	{
		NFT_LOG(L_ERROR, "failed to parse XML from clipboard");
		return;
	}

	/* hardware node? */
	if(led_prefs_is_hardware_node(n))
	{
		/* create hardware from prefs node */
		LedHardware *h;
		if(!(h = led_prefs_hardware_from_node(setup_get_prefs(), n)))
		{
			log_alert_show("Failed to parse Hardware node from clipboard buffer");
			goto cpe_exit;
		}
		
		/* hardware nodes will always be pasted top-level, no matter
		   what is currently selected */
		if(!hardware_new(h))
		{
			log_alert_show("Failed to paste Hardware node");
			goto cpe_exit;
		}
	}

	/* tile node? */
	else if(led_prefs_is_tile_node(n))
	{
		/* create tile from prefs node */
		LedTile *t;
		if(!(t = led_prefs_tile_from_node(setup_get_prefs(), n)))
		{
			log_alert_show("Failed to parse Tile node from clipboard buffer");
			goto cpe_exit;
		}
		
 		switch(parent_t)
		{		
			/* paste tile to hardware */
			case LED_HARDWARE_T:
			{				
				/* parent hardware element */
				LedHardware *h;
				if(!(h = hardware_niftyled((NiftyconfHardware *) parent_element)))
				{
					log_alert_show("Failed to get Hardware node to paste to");
					led_tile_destroy(t);
					goto cpe_exit;
				}

				/* does parent hardware already have a tile? */
				LedTile *pt;
				if((pt = led_hardware_get_tile(h)))
				{
					if(!led_tile_list_append_head(pt, t))
					{
						log_alert_show("Failed to append Tile to parent Hardware list of tiles");
						led_tile_destroy(t);
						goto cpe_exit;
					}
				}
				/* parent hardware doesn't have a tile, yet */
				else
				{
					if(!led_hardware_set_tile(h, t))
					{
						log_alert_show("Failed to set Tile to parent Hardware");
						led_tile_destroy(t);
						goto cpe_exit;
					}
				}

				break;
			}

			/* paste tile to tile */
			case LED_TILE_T:
			{
				/* parent tile element */
				LedTile *pt;
				if(!(pt = tile_niftyled((NiftyconfTile *) parent_element)))
				{
					log_alert_show("Failed to get Tile node to paste to");
					led_tile_destroy(t);
					goto cpe_exit;
				}

				/* append new tile to parent */
				if(!led_tile_append_child(pt, t))
				{
					log_alert_show("Failed to append Tile to parent Tile");
					led_tile_destroy(t);
					goto cpe_exit;
				}
				break;
			}

			default:
			{
				log_alert_show("Tile nodes can only be pasted to Hardware or other Tiles");

				/* destroy created tile */
				led_tile_destroy(t);
				break;
			}
		}
	}

	/* chain node? */
	else if(led_prefs_is_chain_node(n))
	{
		switch(parent_t)
		{
			case LED_TILE_T:
			{
				/* parent tile element */
				LedTile *pt;
				if(!(pt = tile_niftyled((NiftyconfTile *) parent_element)))
				{
					log_alert_show("Failed to get Tile node to paste to");
					goto cpe_exit;
				}

				/* does tile already have a chain? */
				if(led_tile_get_chain(pt))
				{
					log_alert_show("Selected Ttile already has a Chain. Please remove that Chain first.");
					goto cpe_exit;
				}
				
				/* create chain from prefs node */
				LedChain *c;
				if(!(c = led_prefs_chain_from_node(setup_get_prefs(), n)))
				{
					log_alert_show("Failed to parse Chain node from clipboard buffer");
					goto cpe_exit;
				}

				/* register new chain to GUI */
				NiftyconfChain *chain;
				if(!(chain = chain_register_to_gui(c)))
				{
					log_alert_show("Failed to register new Chain to GUI model");
					led_chain_destroy(c);
					goto cpe_exit;
				}
				
				/* set chain to parent tile */
				if(!(led_tile_set_chain(pt, c)))
				{
					log_alert_show("Failed to attach Chain to selected Tile");
					chain_unregister_from_gui(chain);
					led_chain_destroy(c);
					goto cpe_exit;
				}
				
				break;
			}
				
			default:
			{
				log_alert_show("Chains can only be pasted into Tiles");
				break;
			}
		}
	}

	/* unknown node? */
	else
		goto cpe_exit;

	/* refresh whole tree view */
	setup_tree_refresh();
	
        //~ switch(led_settings_node_get_type(xml))
        //~ {
               
                //~ case T_LED_TILE:
                //~ {
                       

                        //~ break;
                //~ }

                //~ case T_LED_CHAIN:
                //~ {
                        //~ switch(parent_t)
                        //~ {
                                //~ /** paste chain to tile */
                                //~ case T_LED_TILE:
                                //~ {
                                        //~ printf("-> chain -> tile\n");
                                        //~ break;
                                //~ }

                                //~ default:
                                        //~ break;
                        //~ }

                        //~ break;
                //~ }
        //~ }

cpe_exit:
	led_prefs_node_free(n);
}


/******************************************************************************
 ******************************************************************************/

/** cut currently selected element to clipboard */
NftResult clipboard_cut_current_selection()
{
	/* get currently selected element */
	NIFTYLED_TYPE t;
        gpointer *e;
        setup_tree_get_first_selected_element(&t, &e);

	if(t == LED_INVALID_T)
	{
		NFT_LOG(L_DEBUG, "could not get first selected element from tree (nothing selected?)");
		return NFT_FAILURE;
	}

	_cut_or_copy_element(t, e, TRUE);

	return NFT_SUCCESS;
}


/** copy currently selected element to clipboard */
NftResult clipboard_copy_current_selection()
{
	/* get currently selected element */
	NIFTYLED_TYPE t;
        gpointer *e;
        setup_tree_get_first_selected_element(&t, &e);

	if(t == LED_INVALID_T)
	{
		NFT_LOG(L_DEBUG, "could not get first selected element from tree (nothing selected?)");
		return NFT_FAILURE;
	}

	_cut_or_copy_element(t, e, FALSE);

	return NFT_SUCCESS;
}


/** paste element in clipboard after currently (or end of rootlist) */
NftResult clipboard_paste_current_selection()
{
	/* get currently selected element */
	NIFTYLED_TYPE t;
        gpointer *e;
        setup_tree_get_first_selected_element(&t, &e);

	if(t == LED_INVALID_T)
	{
		NFT_LOG(L_DEBUG, "could not get first selected element from tree (nothing selected?)");
		return NFT_FAILURE;
	}

        _paste_element(t, e);
	return NFT_SUCCESS;
}


/** initialize this module */
gboolean clipboard_init()
{
        /* get clipboard */
        if(!(_clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)))
                return FALSE;

        /* initialize clipboard */
        gtk_clipboard_set_can_store(_clipboard, NULL, 0);

        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
