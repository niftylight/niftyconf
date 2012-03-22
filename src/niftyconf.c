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
#include "niftyconf-ui.h"
#include "niftyconf-log.h"
#include "niftyconf-clipboard.h"
#include "niftyconf-setup.h"
#include "niftyconf-setup-tree.h"
#include "niftyconf-setup-ledlist.h"
#include "niftyconf-hardware.h"
#include "niftyconf-tile.h"
#include "niftyconf-chain.h"
#include "niftyconf-led.h"
#include "config.h"



/** GtkBuilder for this module */
static GtkBuilder *_ui;


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


/** build a string with valid loglevels */
static char *_loglevels()
{
        static char s[1024];
        
        NftLoglevel i;        
        for(i = L_MAX+1; i<L_MIN-1; i++)
        {
                strcat(s, nft_log_level_to_name(i));
                if(i<L_MIN-2)
                        strncat(s, ", ", sizeof(s));
        }

        return s;
}

/** parse commandline arguments */
static gboolean _parse_cmdline_args(int argc, char *argv[], gchar **setupfile)
{
        static gchar loglevelmsg[1024];
        g_snprintf(loglevelmsg, sizeof(loglevelmsg), "define loglevel (%s)", _loglevels());
        static gchar *loglevel;
        static gchar *sf;
        static GOptionEntry entries[] =
        {
                { "config", 'c', 0, G_OPTION_ARG_FILENAME, &sf, "Initialize setup from XML config file", NULL },
                { "loglevel", 'l', 0, G_OPTION_ARG_STRING, &loglevel, loglevelmsg, NULL },
                { NULL }
        };


        GError *error = NULL;
        GOptionContext *context;

        context = g_option_context_new ("- niftyled configuration GUI");
        g_option_context_add_main_entries (context, entries, PACKAGE_NAME);
        g_option_context_add_group (context, gtk_get_option_group(TRUE));
        if (!g_option_context_parse (context, &argc, &argv, &error))
        {
                g_print ("option parsing failed: %s\n", error->message);
                return FALSE;
        }

        /* initialize setup if XML filename given */
        if(sf)
        {
                *setupfile = sf;
        }

        /* set loglevel */
        if(loglevel)
        {
                NftLoglevel lev = nft_log_level_from_name(loglevel);
                if(lev == L_INVALID)
                {
                        g_error("Invalid loglevel: %s", loglevel);
                        return FALSE;
                }
                
                if(!nft_log_level_set(lev))
                {
                        g_error("Failed to set loglevel: %s (%d)", 
                                loglevel, lev);
                        return FALSE;
                }
        
                
        }
        
        return TRUE;
}
 
/******************************************************************************
 ******************************************************************************/

/** wrapper to access widget */
void niftyconf_menu_logwindow_set_active(gboolean active)
{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(UI("menuitem_log_win")), active);
}


/** wrapper to access widget */
void niftyconf_menu_hardware_add_set_sensitive(gboolean active)
{
        gtk_widget_set_sensitive(GTK_WIDGET(UI("item_hardware_add")), active);
}


/** wrapper to access widget */
void niftyconf_menu_hardware_remove_set_sensitive(gboolean active)
{
        gtk_widget_set_sensitive(GTK_WIDGET(UI("item_hardware_remove")), active);
}


/** wrapper to access widget */
void niftyconf_menu_tile_add_set_sensitive(gboolean active)
{
        gtk_widget_set_sensitive(GTK_WIDGET(UI("item_tile_add")), active);
}


/** wrapper to access widget */
void niftyconf_menu_tile_remove_set_sensitive(gboolean active)
{
        gtk_widget_set_sensitive(GTK_WIDGET(UI("item_tile_remove")), active);
}


/** wrapper to access widget */
void niftyconf_menu_chain_add_set_sensitive(gboolean active)
{
        gtk_widget_set_sensitive(GTK_WIDGET(UI("item_chain_add")), active);
}


/** wrapper to access widget */
void niftyconf_menu_chain_remove_set_sensitive(gboolean active)
{
        gtk_widget_set_sensitive(GTK_WIDGET(UI("item_chain_remove")), active);
}





int main (int argc, char *argv[])
{
        /* initialize GTK stuff */
        gtk_set_locale();
        gtk_init (&argc, &argv);
        

        /* set default loglevel */
        nft_log_level_set(L_INFO);

        
        /* parse commandline arguments */
        static gchar *setupfile;
        if(!_parse_cmdline_args(argc, argv, &setupfile))
                return -1;

        /* initialize modules */
        if(!log_init())
                g_error("Failed to initialize \"log\" module");
        if(!led_init())
                g_error("Failed to initialize \"led\" module");
        if(!chain_init())
                g_error("Failed to initialize \"chain\" module");
        if(!tile_init())
                g_error("Failed to initialize \"tile\" module");
        if(!hardware_init())
                g_error("Failed to initialize \"hardware\" module");
        if(!setup_init())
                g_error("Failed to initialize \"setup\" module");
        if(!clipboard_init())
                g_error("Failed to initialize \"clipboard\" module");
        
        
        /* build our ui */
        _ui = ui_builder("niftyconf.ui");
        GtkBox *box_setup = GTK_BOX(gtk_builder_get_object(_ui, "box_setup"));
        gtk_box_pack_start(box_setup, setup_get_widget(), TRUE, TRUE, 0);
        GtkBox *box_chain = GTK_BOX(gtk_builder_get_object(_ui, "box_chain"));
        gtk_box_pack_start(box_chain, setup_ledlist_get_widget(), TRUE, TRUE, 0);
      

        
        /* load setup file if any given from commandline */
        if(setupfile)
        {
                if(!setup_load(setupfile))
                {
                        g_warning("Failed to initialize setup from \"%s\"", 
                                  setupfile);
                        return -1;
                }
                g_free(setupfile);
        }


        
        /* main loop... */
        gtk_main();

        
        return 0;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** close main window */
gboolean on_niftyconf_window_delete_event(GtkWidget *w, GdkEvent *e)
{
        setup_cleanup();
        gtk_main_quit();
        return FALSE;
}

/** menuitem "quit" selected */
void on_niftyconf_menu_quit_activate(GtkMenuItem *i, gpointer d)
{
        setup_cleanup();
        gtk_main_quit();
}

/** menuitem "show log-window" toggled */
void on_niftyconf_menu_log_window_activate(GtkWidget *i, gpointer u)
{       
        log_show(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(i)));
}

/** menu-entry selected */
void on_niftyconf_menu_add_hardware_activate(GtkWidget *i, gpointer u)
{
        setup_new_hardware("Unnamed", "dummy");

        /** @todo refresh our menu */
        
        /* refresh tree */
        setup_tree_refresh();
}


/** menu-entry selected */
void on_niftyconf_menu_add_tile_activate(GtkWidget *i, gpointer u)
{
        NIFTYLED_TYPE t;
        gpointer *e;
        setup_tree_get_last_selected_element(&t, &e);

        
        /* different possible element types */
        switch(t)
        {
                /* currently selected element is a hardware-node */
                case T_LED_HARDWARE:
                {
                        setup_new_tile_of_hardware((NiftyconfHardware *) e);
                        break;
                }

                /* currently selected element is a tile-node */
                case T_LED_TILE:
                {
                        setup_new_tile_of_tile((NiftyconfTile *) e);
                        break;
                }
        }

        /** @todo refresh our menu */
        
        /* refresh tree */
        setup_tree_refresh();
}


/** menu-entry selected */
void on_niftyconf_menu_add_chain_activate(GtkWidget *i, gpointer u)
{
        NIFTYLED_TYPE t;
        gpointer *e;
        setup_tree_get_last_selected_element(&t, &e);
        
        /* can only add chains to tiles */
        if(t != T_LED_TILE)
                return;

        /* add new chain */
        setup_new_chain_of_tile((NiftyconfTile *) e, 0, "RGB u8");

        /** @todo refresh our menu */
        
        /* refresh tree */
        setup_tree_refresh();
}


/** wrapper for do_* functions */
static void _foreach_remove_hardware(NIFTYLED_TYPE t, gpointer *e)
{
        if(t != T_LED_HARDWARE)
                return;

        setup_destroy_hardware((NiftyconfHardware *) e);
}


/** menu-entry selected */
void on_niftyconf_menu_remove_hardware_activate(GtkWidget *i, gpointer u)
{
        /* remove all currently selected elements */
        setup_tree_do_foreach_selected_element(_foreach_remove_hardware);

        /** @todo refresh our menu */
        
        /* refresh tree */
        setup_tree_refresh();
}


/** wrapper for do_* functions */
static void _foreach_remove_tile(NIFTYLED_TYPE t, gpointer *e)
{
        if(t != T_LED_TILE)
                return;

        setup_destroy_tile((NiftyconfTile *) e);
}


/** menu-entry selected */
void on_niftyconf_menu_remove_tile_activate(GtkWidget *i, gpointer u)
{
        /* remove all currently selected elements */
        setup_tree_do_foreach_selected_element(_foreach_remove_tile);

        /** @todo refresh our menu */
        /* refresh tree */
        setup_tree_refresh();
}


/** wrapper for do_* functions */
static void _foreach_remove_chain(NIFTYLED_TYPE type, gpointer *e)
{
        /* works only if tile-element is selected */
        if(type != T_LED_TILE)
                return;

        setup_destroy_chain_of_tile((NiftyconfTile *) e);
}


/** menu-entry selected */
void on_niftyconf_menu_remove_chain_activate(GtkWidget *i, gpointer u)
{
        /* remove all currently selected elements */
        setup_tree_do_foreach_selected_element(_foreach_remove_chain);

        /** @todo refresh our menu */
        
        /* refresh tree */
        setup_tree_refresh();
}
