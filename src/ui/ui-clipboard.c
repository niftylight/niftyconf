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

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <niftyled.h>
#include <gtk/gtk.h>
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-log.h"
#include "elements/element-setup.h"
#include "elements/element-hardware.h"
#include "elements/element-tile.h"
#include "elements/element-chain.h"
#include "elements/element-led.h"




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


/** cut/copy node */
static LedPrefsNode *_cut_or_copy_node(NIFTYLED_TYPE t, gpointer *e, gboolean cut)
{
		NFT_LOG(L_DEBUG, cut ? "Cutting element (type: %d / ptr: %p)..." : "Copying element (type: %d / ptr: %p)...",
		        t, e);



		LedPrefsNode *n = NULL;
		switch(t)
		{
				case LED_SETUP_T:
				{

						if(!(n = led_prefs_setup_to_node(setup_get_prefs(), setup_get_current())))
						{
								ui_log_alert_show("Failed to create preferences from current setup.");
								return NULL;
						}

						/* also remove element? */
						if(cut)
						{
								led_setup_destroy(setup_get_current());
								setup_register_to_gui(led_setup_new());
								ui_setup_tree_refresh();
								ui_setup_props_hide();
						}
						break;
				}

				case LED_HARDWARE_T:
				{
						LedHardware *h = hardware_niftyled((NiftyconfHardware *) e);
						if(!(n = led_prefs_hardware_to_node(setup_get_prefs(), h)))
						{
								ui_log_alert_show("Failed to create preferences from current setup.");
								return NULL;
						}

						/* also remove element? */
						if(cut)
						{
								hardware_destroy((NiftyconfHardware *) e);
								ui_setup_tree_refresh();

								/* hide properties */
								ui_setup_props_hide();
						}
						break;
				}

				case LED_TILE_T:
				{
						LedTile *t = tile_niftyled((NiftyconfTile *) e);
						if(!(n = led_prefs_tile_to_node(setup_get_prefs(), t)))
								return NULL;

						/* remove element? */
						if(cut)
						{
								tile_destroy((NiftyconfTile *) e);
								ui_setup_tree_refresh();

								/* hide properties */
								ui_setup_props_hide();
						}
						break;
				}

				case LED_CHAIN_T:
				{
						LedChain *c = chain_niftyled((NiftyconfChain *) e);
						if(!(n = led_prefs_chain_to_node(setup_get_prefs(), c)))
						{
								NFT_LOG(L_ERROR, "Failed to dump Chain element");
								return NULL;
						}

						/* don't cut from hardware elements */
						if(led_chain_parent_is_hardware(c))
								break;

						/* remove element? */
						if(cut)
						{
								/* get parent tile of this chain */
								LedTile *t = led_chain_get_parent_tile(c);
								NiftyconfTile *tile = led_tile_get_privdata(t);
								chain_of_tile_destroy(tile);
								ui_setup_tree_refresh();

								/* hide properties */
								ui_setup_props_hide();
						}
						break;
				}

				case LED_T:
				{
						Led *l = led_niftyled((NiftyconfLed *) e);
						if(!(n = led_prefs_led_to_node(setup_get_prefs(), l)))
								return NULL;

						/* remove element? */
						if(cut)
						{
								NFT_TODO();
								//led_unregister((NiftyconfLed *) e);
								//setup_tree_refresh();
						}
						break;
				}

				default:
				{
						NFT_LOG(L_ERROR, "Attempt to cut/copy unknown element. This shouldn't happen?!");
				}
		}


		return n;
}


/** paste element from clipboard */
static void _paste_node(LedPrefsNode *n, NIFTYLED_TYPE parent_t, gpointer parent_element)
{

		/* handle different element types */
		switch(led_prefs_node_get_type(n))
		{
				/* LedHardware */
				case LED_HARDWARE_T:
				{

						/* create hardware from prefs node */
						LedHardware *h;
						if(!(h = led_prefs_hardware_from_node(setup_get_prefs(), n)))
						{
								ui_log_alert_show("Failed to parse Hardware node from clipboard buffer");
								goto cpe_exit;
						}

						/* hardware nodes will always be pasted top-level, no matter
						 what is currently selected */
						if(!hardware_register_to_gui_and_niftyled(h))
						{
								ui_log_alert_show("Failed to paste Hardware node");
								goto cpe_exit;
						}
						break;
				}


						/* LedTile */
				case LED_TILE_T:
				{
						/* create tile from prefs node */
						LedTile *t;
						if(!(t = led_prefs_tile_from_node(setup_get_prefs(), n)))
						{
								ui_log_alert_show("Failed to parse Tile node from clipboard buffer");
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
												ui_log_alert_show("Failed to get Hardware node to paste to");
												led_tile_destroy(t);
												goto cpe_exit;
										}

										/* does parent hardware already have a tile? */
										LedTile *pt;
										if((pt = led_hardware_get_tile(h)))
										{
												if(!led_tile_list_append_head(pt, t))
												{
														ui_log_alert_show("Failed to append Tile to parent Hardware list of tiles");
														led_tile_destroy(t);
														goto cpe_exit;
												}
										}
										/* parent hardware doesn't have a tile, yet */
										else
										{
												if(!led_hardware_set_tile(h, t))
												{
														ui_log_alert_show("Failed to set Tile to parent Hardware");
														led_tile_destroy(t);
														goto cpe_exit;
												}
										}

										/* register tile to GUI */
										tile_register_to_gui(t);
										break;
								}

										/* paste tile to tile */
								case LED_TILE_T:
								{
										/* parent tile element */
										LedTile *pt;
										if(!(pt = tile_niftyled((NiftyconfTile *) parent_element)))
										{
												ui_log_alert_show("Failed to get Tile node to paste to");
												led_tile_destroy(t);
												goto cpe_exit;
										}

										/* append new tile to parent */
										if(!led_tile_list_append_child(pt, t))
										{
												ui_log_alert_show("Failed to append Tile to parent Tile");
												led_tile_destroy(t);
												goto cpe_exit;
										}

										/* register tile to GUI */
										tile_register_to_gui(t);
										break;
								}

								default:
								{
										ui_log_alert_show("Tile nodes can only be pasted to Hardware or other Tiles");

										/* destroy created tile */
										led_tile_destroy(t);
										break;
								}
						}

						break;
				}


						/* LedChain */
				case LED_CHAIN_T:
				{
						/* chains only go in tiles, pasting into hardware is not supported */
						switch(parent_t)
						{

								/* paste chain into tile */
								case LED_TILE_T:
								{
										/* parent tile element */
										LedTile *pt;
										if(!(pt = tile_niftyled((NiftyconfTile *) parent_element)))
										{
												ui_log_alert_show("Failed to get Tile node to paste to");
												goto cpe_exit;
										}

										/* does tile already have a chain? */
										if(led_tile_get_chain(pt))
										{
												ui_log_alert_show("Selected Ttile already has a Chain. Please remove that Chain first.");
												goto cpe_exit;
										}

										/* create chain from prefs node */
										LedChain *c;
										if(!(c = led_prefs_chain_from_node(setup_get_prefs(), n)))
										{
												ui_log_alert_show("Failed to parse Chain node from clipboard buffer");
												goto cpe_exit;
										}

										/* set chain to parent tile */
										if(!(led_tile_set_chain(pt, c)))
										{
												ui_log_alert_show("Failed to attach Chain to selected Tile");
												led_chain_destroy(c);
												goto cpe_exit;
										}

										/* register new chain to GUI */
										if(!chain_register_to_gui(c))
										{
												ui_log_alert_show("Failed to register new Chain to GUI model");
												led_chain_destroy(c);
												led_tile_set_chain(pt, NULL);
												goto cpe_exit;
										}

										break;
								}

								default:
								{
										ui_log_alert_show("Chains can only be pasted into Tiles");
										break;
								}
						}

						break;
				}

						/* LedSetup */
				case LED_SETUP_T:
				{
						/* create setup from prefs node */
						LedSetup *s;
						if(!(s = led_prefs_setup_from_node(setup_get_prefs(), n)))
						{
								ui_log_alert_show("Failed to parse Niftyled node from clipboard buffer");
								goto cpe_exit;
						}

						/* no hardware node? */
						LedHardware *h;
						if(!(h = led_setup_get_hardware(s)))
								goto cpe_exit;

						/* append hardware to current setup */
						if(!(led_hardware_list_append_head(led_setup_get_hardware(setup_get_current()), h)))
						{
								ui_log_alert_show("Failed to append hardware from new Setup to this Setup.");
								led_setup_destroy(s);
								goto cpe_exit;
						}

						/* register all hardware nodes to GUI */
						for(h = led_setup_get_hardware(s); h; h = led_hardware_list_get_next(h))
						{
								if(!hardware_register_to_gui(h))
								{
										ui_log_alert_show("Failed to register new Hardware to GUI model");
										goto cpe_exit;
								}
						}

						led_setup_set_hardware(s, NULL);
						led_setup_destroy(s);
						break;
				}

						/* huh? */
				case LED_INVALID_T:
				default:
				{
						ui_log_alert_show("Unhandled element type \"%s\"",
						                  nft_prefs_node_get_name(n));
						return;
				}
		}


		cpe_exit:
				/* refresh whole tree view */
				ui_setup_tree_refresh();
}


/******************************************************************************
 ******************************************************************************/

/** cut currently selected element to clipboard */
NftResult ui_clipboard_cut_current_selection()
{
		/* get currently selected element */
		NIFTYLED_TYPE t;
		gpointer e;
		ui_setup_tree_get_first_selected_element(&t, &e);

		if(t == LED_INVALID_T)
		{
				NFT_LOG(L_DEBUG, "could not get first selected element from tree (nothing selected?)");
				return NFT_FAILURE;
		}

		/* highlight only this element */
		ui_setup_tree_highlight_only(t, e);

		LedPrefsNode *n;
		if(!(n = _cut_or_copy_node(t, e, TRUE)))
				return NFT_FAILURE;

		char *xml;
		if(!(xml = led_prefs_node_to_buffer(n)))
				return NFT_FAILURE;

		/* set the clipboard text */
		gtk_clipboard_set_text(_clipboard, xml, -1);

		/* store the clipboard text */
		gtk_clipboard_store(_clipboard);

		NFT_LOG(L_VERY_NOISY, "%s", xml);

		free((void *) xml);

		return NFT_SUCCESS;
}


/** copy currently selected element to clipboard */
NftResult ui_clipboard_copy_current_selection()
{
		/* get currently selected element */
		NIFTYLED_TYPE t;
		gpointer e;
		ui_setup_tree_get_first_selected_element(&t, &e);

		/* if nothing is selected, copy complete setup */
		if(t == LED_INVALID_T)
		{
				e = setup_get_current();
				t = LED_SETUP_T;
		}

		/* highlight only this element */
		ui_setup_tree_highlight_only(t, e);

		LedPrefsNode *n;
		if(!(n = _cut_or_copy_node(t, e, FALSE)))
				return NFT_FAILURE;

		char *xml;
		if(!(xml = led_prefs_node_to_buffer(n)))
				return NFT_FAILURE;

		/* set the clipboard text */
		gtk_clipboard_set_text(_clipboard, xml, -1);

		/* store the clipboard text */
		gtk_clipboard_store(_clipboard);

		NFT_LOG(L_VERY_NOISY, "%s", xml);

		free((void *) xml);

		return NFT_SUCCESS;
}


/** paste element in clipboard after currently (or end of rootlist) */
NftResult ui_clipboard_paste_current_selection()
{
		/* get currently selected element */
		NIFTYLED_TYPE t;
		gpointer e;
		ui_setup_tree_get_first_selected_element(&t, &e);

		/* paste to setup if no element selected */
		if(t == LED_INVALID_T)
		{
				e = setup_get_current();
				t = LED_SETUP_T;
		}

		/* get XML data from clipboard */
		gchar *xml;
		if(!(xml = gtk_clipboard_wait_for_text(_clipboard)))
		{
				NFT_LOG(L_ERROR, "received NULL from clipboard?");
				return NFT_FAILURE;
		}

		/* parse buffer to LedPrefsNode */
		LedPrefsNode *n;
		if(!(n = led_prefs_node_from_buffer(xml, strlen(xml))))
		{
				NFT_LOG(L_ERROR, "failed to parse XML from clipboard");
				g_free(xml);
				return NFT_FAILURE;
		}

		_paste_node(n, t, e);

		led_prefs_node_free(n);
		g_free(xml);

		return NFT_SUCCESS;
}


/** copy element to file */
NftResult ui_clipboard_copy_to_file(const char *filename)
{
		/* get currently selected element */
		NIFTYLED_TYPE t;
		gpointer e;
		ui_setup_tree_get_first_selected_element(&t, &e);

		/* if nothing is selected, copy complete setup */
		if(t == LED_INVALID_T)
		{
				e = setup_get_current();
				t = LED_SETUP_T;
		}

		/* highlight only this element */
		ui_setup_tree_highlight_only(t, e);

		LedPrefsNode *n;
		if(!(n = _cut_or_copy_node(t, e, FALSE)))
				return NFT_FAILURE;

		/* file existing? */
		struct stat sts;
		if(stat(filename, &sts) == -1)
		{
				/* continue if stat error was caused because file doesn't exist */
				if(errno != ENOENT)
				{
						ui_log_alert_show("Failed to access \"%s\" - %s", filename, strerror(errno));
						return NFT_FAILURE;
				}
		}
		/* stat succeeded, file exists */
		else
		{
				/* remove old file? */
				if(!ui_log_dialog_yesno("Overwrite", "A file named \"%s\" already exists.\nOverwrite?", filename))
						return NFT_FAILURE;

				if(unlink(filename) == -1)
				{
						ui_log_alert_show("Failed to remove old version of \"%s\" - %s", filename, strerror(errno));
						return NFT_FAILURE;
				}
		}

		return led_prefs_node_to_file_light(n, filename, true);
}


/** paste element from a file */
NftResult ui_clipboard_paste_from_file(const char *filename)
{
		/* get currently selected element */
		NIFTYLED_TYPE t;
		gpointer e;
		ui_setup_tree_get_first_selected_element(&t, &e);

		LedPrefsNode *n;
		if(!(n = led_prefs_node_from_file(filename)))
		{
				NFT_LOG(L_ERROR, "failed to parse XML from \"%s\"", filename);
				return NFT_FAILURE;
		}

		_paste_node(n, t, e);

		led_prefs_node_free(n);

		return NFT_SUCCESS;
}


/** initialize this module */
gboolean ui_clipboard_init()
{
		/* get clipboard */
		if(!(_clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)))
				return FALSE;

		/* initialize clipboard */
		gtk_clipboard_set_can_store(_clipboard, NULL, 0);

		return TRUE;
}


/** deinitialize this module */
void ui_clipboard_deinit()
{

}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
