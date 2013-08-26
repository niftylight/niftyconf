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
#include "elements/element-hardware.h"
#include "elements/element-tile.h"
#include "elements/element-chain.h"
#include "elements/element-led.h"
#include "elements/element-setup.h"
#include "ui/ui-about.h"
#include "ui/ui.h"
#include "ui/ui-log.h"
#include "ui/ui-clipboard.h"
#include "ui/ui-setup.h"
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-ledlist.h"
#include "ui/ui-info-hardware.h"
#include "prefs/prefs.h"
#include "renderer/renderer.h"
#include "live-preview/live-preview.h"
#include "config.h"



/** GtkBuilder for this module (check data/ directory) */
static GtkBuilder *_ui;



/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


/** configure from preferences */
static NftResult _this_from_prefs(NftPrefs * prefs,
                                  void **newObj,
                                  NftPrefsNode * node, void *userptr)
{

        *newObj = NULL;


        /* process child nodes */
        NftPrefsNode *child;
        for(child = nft_prefs_node_get_first_child(node);
            child; child = nft_prefs_node_get_next(child))
        {
                nft_prefs_obj_from_node(prefs, child, NULL);
        }

        /* first launch ? */
        bool launched_before = false;
        nft_prefs_node_prop_boolean_get(node, "launched_before",
                                        &launched_before);

        /* show about dialog if we never launched before */
        if(!launched_before)
        {
                ui_about_set_visible(true);
        }

        /* UI dimensions */
        gint x = 0, y = 0, width = 0, height = 0;

        nft_prefs_node_prop_int_get(node, "x", &x);
        nft_prefs_node_prop_int_get(node, "y", &y);
        nft_prefs_node_prop_int_get(node, "width", &width);
        nft_prefs_node_prop_int_get(node, "height", &height);

        if(width > 0 && height > 0)
                gtk_window_resize(GTK_WINDOW(UI("window")), width, height);
        if(x > 0 && y > 0)
                gtk_window_move(GTK_WINDOW(UI("window")), x, y);


        /* restore last project? */
        char *filename;
        if((filename = nft_prefs_node_prop_string_get(node, "last-project")))
        {
                ui_setup_load(filename);
                nft_prefs_free(filename);
        }

        return NFT_SUCCESS;
}


/** save configuration to preferences */
static NftResult _this_to_prefs(NftPrefs * prefs,
                                NftPrefsNode * newNode,
                                void *obj, void *userptr)
{

        /* submodules */
        nft_prefs_node_add_child(newNode,
                                 nft_prefs_obj_to_node(prefs, "ui-log", NULL,
                                                       NULL));
        nft_prefs_node_add_child(newNode,
                                 nft_prefs_obj_to_node(prefs, "live-preview",
                                                       NULL, NULL));

        /* mark that we were launched before */
        nft_prefs_node_prop_boolean_set(newNode, "launched_before", true);

        /* main window geometry */
        gint x, y, width, height;
        gtk_window_get_size(GTK_WINDOW(UI("window")), &width, &height);
        gtk_window_get_position(GTK_WINDOW(UI("window")), &x, &y);


        if(!nft_prefs_node_prop_int_set(newNode, "x", x))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "y", y))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "width", width))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "height", height))
                return NFT_FAILURE;

        /* save current project */
        if(!nft_prefs_node_prop_string_set(newNode,
                                           "last-project",
                                           setup_get_current_filename()?
                                           (char *)
                                           setup_get_current_filename() : ""))
                return NFT_FAILURE;


        return NFT_SUCCESS;
}


/** parse commandline arguments */
static gboolean _parse_cmdline_args(int argc,
                                    char *argv[], gchar ** setupfile)
{
        static gchar loglevelmsg[1024];
        g_snprintf(loglevelmsg, sizeof(loglevelmsg), "define loglevel (%s)",
                   ui_log_loglevels());
        static gchar *loglevel;
        static gchar *sf;
        static GOptionEntry entries[] = {
                {"config", 'c', 0, G_OPTION_ARG_FILENAME, &sf,
                 "Initialize setup from XML config file", NULL},
                {"loglevel", 'l', 0, G_OPTION_ARG_STRING, &loglevel,
                 loglevelmsg, NULL},
                {NULL, 0, 0, 0, NULL, NULL, NULL}
        };


        GError *error = NULL;
        GOptionContext *context;

        context = g_option_context_new("- niftyled configuration GUI");
        g_option_context_add_main_entries(context, entries, PACKAGE_NAME);
        g_option_context_add_group(context, gtk_get_option_group(true));
        if(!g_option_context_parse(context, &argc, &argv, &error))
        {
                g_print("option parsing failed: %s\n", error->message);
                g_error_free(error);
                return false;
        }

        /* initialize setup if XML filename given */
        if(sf)
        {
                *setupfile = sf;
        }

        /* set loglevel */
        if(loglevel)
        {
                NftLoglevel lev = nft_log_level_from_string(loglevel);
                if(lev == L_INVALID)
                {
                        g_error("Invalid loglevel: %s", loglevel);
                        return false;
                }

                if(!nft_log_level_set(lev))
                {
                        g_error("Failed to set loglevel: %s (%d)",
                                loglevel, lev);
                        return false;
                }


        }

        return true;
}



/******************************************************************************
 ******************************************************************************/

/** getter for GtkBuilder of this module */
GObject *niftyconf_ui(const char *n)
{
        return UI(n);
}






/******************************************************************************/
int main(int argc, char *argv[])
{
        /* initialize GTK stuff */
        // gtk_set_locale();
        gtk_init(&argc, &argv);

        /* check version */
        if(!LED_CHECK_VERSION)
                return EXIT_FAILURE;

        /* set default loglevel */
        nft_log_level_set(L_NOTICE);


        /* parse commandline arguments */
        static gchar *setupfile;
        if(!_parse_cmdline_args(argc, argv, &setupfile))
                return EXIT_FAILURE;


        /* initialize modules */
        if(!prefs_init())
                g_error("Failed to initialize \"prefs\" module");
        if(!ui_log_init())
                g_error("Failed to initialize \"log\" module");
        if(!renderer_init())
                g_error("Failed to initialize \"renderer\" module");
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
        if(!live_preview_init())
                g_error("Failed to initialize \"live-preview\" module");
        if(!ui_info_hardware_init())
                g_error("Failed to initialize \"info-hardware\" module");
        if(!ui_setup_init())
                g_error("Failed to initialize \"setup\" module");
        if(!ui_clipboard_init())
                g_error("Failed to initialize \"clipboard\" module");
        if(!ui_about_init())
                g_error("Failed to initialize \"about\" module");

        /* register prefs class for this module */
        if(!nft_prefs_class_register
           (prefs(), "niftyconf", _this_from_prefs, _this_to_prefs))
                g_error("Failed to register prefs class for \"niftyconf\"");

        /* build our ui */
        _ui = ui_builder("niftyconf.ui");
        GtkBox *box_setup = GTK_BOX(UI("box_setup"));
        gtk_box_pack_start(box_setup, ui_setup_get_widget(), true, true, 0);
        GtkBox *box_chain = GTK_BOX(UI("box_chain"));
        gtk_box_pack_start(box_chain, ui_setup_ledlist_get_widget(), true,
                           true, 0);
        GtkBox *box_canvas = GTK_BOX(UI("box_canvas"));
        gtk_box_pack_start(box_canvas, renderer_get_widget(), true, true, 0);


        /* restore window size & position */
        prefs_load();

        /* load setup file if any given from commandline */
        if(setupfile)
        {
                if(!ui_setup_load(setupfile))
                {
                        g_warning("Failed to initialize setup from \"%s\"",
                                  setupfile);
                        return EXIT_FAILURE;
                }
                g_free(setupfile);
        }

        /* main loop... */
        gtk_main();

        g_object_unref(_ui);

        /* unregister prefs class */
        nft_prefs_class_unregister(prefs(), "niftyconf");

        ui_about_deinit();
        ui_clipboard_deinit();
        ui_setup_deinit();
        ui_info_hardware_deinit();
        live_preview_deinit();
        setup_deinit();
        hardware_deinit();
        tile_deinit();
        led_deinit();
        renderer_deinit();
        ui_log_deinit();
        prefs_deinit();

        return EXIT_SUCCESS;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** close main window */
G_MODULE_EXPORT gboolean on_niftyconf_window_delete_event(GtkWidget * w,
                                                          GdkEvent * e)
{
        /* store window size & position */
        prefs_save();

        /* bye bye */
        gtk_main_quit();
        return false;
}


/** menuitem "quit" selected */
G_MODULE_EXPORT void on_niftyconf_menu_quit_activate(GtkMenuItem * i,
                                                     gpointer d)
{
        gtk_main_quit();
}




/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_help_about_activate(GtkWidget * i,
                                                           gpointer u)
{
        ui_about_set_visible(true);
}
