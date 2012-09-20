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

/* clipboard_get_func - dummy get_func for gtk_clipboard_set_with_data () */
static void _get_hw_func(GtkClipboard *c,
                        GtkSelectionData *s,
                        guint info, gpointer u)
{
        NiftyconfHardware *h = (NiftyconfHardware *) u;

        GdkAtom target = gtk_selection_data_get_target(s);

        /* request text-form? */
        if(gtk_targets_include_text(&target, 1)) 
        {

        }
        /* deliver hardware pointer */
        else
        {

        }
        
        /*GdkAtom target = gtk_selection_data_get_target(selection_data);
        if(gtk_targets_include_text(&target, 1)) 
        {
                char *str;
                gsize len;

                str = convert_file_list_to_string(clipboard_info, TRUE, &len);
                gtk_selection_data_set_text(selection_data, str, len);
                g_free(str);
        }
        else if(target == copied_files_atom) 
        {
                char *str;
                gsize len;

                str = convert_file_list_to_string(clipboard_info, FALSE, &len);
                gtk_selection_data_set(selection_data, copied_files_atom, 8, str, len);
                g_free(str);
        }*/
}

/* clipboard_clear_func - dummy clear_func for gtk_clipboard_set_with_data () */
static void _clear_hw_func(GtkClipboard *c, gpointer u)
{
        NiftyconfHardware *h = (NiftyconfHardware *) u;
}





/******************************************************************************
 ******************************************************************************/




/** cut/copy to buffer */
void clipboard_cut_or_copy_element(NIFTYLED_TYPE t, gpointer *e, gboolean cut)
{
        //~ const char *xml = NULL;
        
        //~ switch(t)
        //~ {
                //~ case T_LED_HARDWARE:
                //~ {
                        //~ LedHardware *h = hardware_niftyled((NiftyconfHardware *) e);
                        //~ xml = led_settings_hardware_dump_xml(setup_get_current(), h);

                        //~ /* remove element? */
                        //~ if(cut)
                        //~ {
                                //~ hardware_unregister((NiftyconfHardware *) e);
                                //~ led_hardware_destroy(h);
                                //~ setup_tree_refresh();
                        //~ }
                        //~ break;
                //~ }

                //~ case T_LED_TILE:
                //~ {
                        //~ LedTile *t = tile_niftyled((NiftyconfTile *) e);
                        //~ xml = led_settings_tile_dump_xml(setup_get_current(), t);

                        //~ /* remove element? */
                        //~ if(cut)
                        //~ {
                                //~ tile_unregister((NiftyconfTile *) e);
                                //~ led_tile_destroy(t);
                                //~ setup_tree_refresh();
                        //~ }
                        
                        //~ break;
                //~ }

                //~ case T_LED_CHAIN:
                //~ {
                        //~ LedChain *c = chain_niftyled((NiftyconfChain *) e);
                        //~ xml = led_settings_chain_dump_xml(setup_get_current(), c);

                        //~ /* don't cut from hardware elements */
                        //~ if(led_chain_parent_is_hardware(c))
                                //~ break;
                        
                        //~ /* remove element? */
                        //~ if(cut)
                        //~ {
                                //~ chain_unregister((NiftyconfChain *) e);
                                //~ led_chain_destroy(c);
                                //~ setup_tree_refresh();
                        //~ }
                        //~ break;
                //~ }
        //~ }

        //~ if(!xml)
                //~ return;

        //~ /* set the clipboard text */
        //~ gtk_clipboard_set_text(_clipboard, xml, -1);

        //~ /* store the clipboard text */
        //~ gtk_clipboard_store(_clipboard);

        //~ NFT_LOG(L_VERY_NOISY, "%s", xml);
        
        //~ free((void *) xml);
}


/** paste element from clipboard */
void clipboard_paste_element(NIFTYLED_TYPE parent_t, gpointer *parent_element)
{
        //~ gchar *xml;
        //~ if(!(xml = gtk_clipboard_wait_for_text(_clipboard)))
        //~ {
                //~ NFT_LOG(L_ERROR, "recieved NULL from clipboard");
                //~ return;
        //~ }

        //~ switch(led_settings_node_get_type(xml))
        //~ {
                //~ case T_LED_HARDWARE:
                //~ {
                        //~ printf("-> hardware\n");
                        //~ break;
                //~ }

                //~ case T_LED_TILE:
                //~ {
                        //~ switch(parent_t)
                        //~ {
                                //~ /* paste tile to hardware */
                                //~ case T_LED_HARDWARE:
                                //~ {
                                        //~ printf("-> tile -> hw\n");
                                        //~ break;
                                //~ }

                                //~ /* paste tile to tile */
                                //~ case T_LED_TILE:
                                //~ {
                                        //~ printf("-> tile -> tile\n");
                                        //~ break;
                                //~ }

                                //~ default:
                                        //~ break;
                        //~ }
                        
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
