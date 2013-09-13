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
#include "ui/ui-about.h"
#include "ui/ui.h"
#include "ui/ui-log.h"
#include "ui/ui-clipboard.h"
#include "ui/ui-renderer.h"
#include "ui/ui-setup.h"
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-ledlist.h"
#include "ui/ui-info-hardware.h"
#include "prefs/prefs.h"
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

        /* dummy object */
        *newObj = (void *) 1;


        /* process child nodes */
        NftPrefsNode *child;
        for(child = nft_prefs_node_get_first_child(node);
            child; child = nft_prefs_node_get_next(child))
        {
                nft_prefs_obj_from_node(prefs, child, NULL);
        }

        /* UI dimensions */
        gint x = 0, y = 0;
        nft_prefs_node_prop_int_get(node, "x", &x);
        nft_prefs_node_prop_int_get(node, "y", &y);
        gtk_window_move(GTK_WINDOW(UI("main_window")), x, y);

        gint width = 0, height = 0;
        nft_prefs_node_prop_int_get(node, "width", &width);
        nft_prefs_node_prop_int_get(node, "height", &height);
        gtk_window_resize(GTK_WINDOW(UI("main_window")), width, height);
        gtk_widget_show(GTK_WIDGET(UI("main_window")));

        return NFT_SUCCESS;
}


/** save configuration to preferences */
static NftResult _this_to_prefs(NftPrefs * prefs,
                                NftPrefsNode * newNode,
                                void *obj, void *userptr)
{

        /* submodules */
        nft_prefs_node_add_child(newNode,
                                 nft_prefs_obj_to_node(prefs, "live-preview",
                                                       NULL, NULL));
        nft_prefs_node_add_child(newNode,
                                 nft_prefs_obj_to_node(prefs, "ui-log", NULL,
                                                       NULL));

        /* main window geometry */
        gint x, y, width, height;
        gtk_window_get_size(GTK_WINDOW(UI("main_window")), &width, &height);
        gtk_window_get_position(GTK_WINDOW(UI("main_window")), &x, &y);


        if(!nft_prefs_node_prop_int_set(newNode, "x", x))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "y", y))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "width", width))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "height", height))
                return NFT_FAILURE;

        return NFT_SUCCESS;
}




/******************************************************************************
 ******************************************************************************/

/** initialize module */
gboolean ui_init()
{
        /* register prefs class for this module */
        if(!nft_prefs_class_register
           (prefs(), "ui", _this_from_prefs, _this_to_prefs))
                g_error("Failed to register prefs class for \"ui\"");

        /* build our ui */
        _ui = ui_builder("niftyconf.ui");


        if(!ui_log_init())
                g_error("Failed to initialize \"log\" module");
        if(!ui_info_hardware_init())
                g_error("Failed to initialize \"info-hardware\" module");
        if(!ui_setup_init())
                g_error("Failed to initialize \"setup\" module");
        if(!ui_clipboard_init())
                g_error("Failed to initialize \"clipboard\" module");
        if(!ui_about_init())
                g_error("Failed to initialize \"about\" module");
        if(!ui_renderer_init())
                g_error("Failed to initialize \"renderer\" module");

        GtkBox *box_setup = GTK_BOX(UI("box_setup"));
        gtk_box_pack_start(box_setup, ui_setup_get_widget(), true, true, 0);
        GtkBox *box_chain = GTK_BOX(UI("box_chain"));
        gtk_box_pack_start(box_chain, ui_setup_ledlist_get_widget(), true,
                           true, 0);
        GtkBox *box_canvas = GTK_BOX(UI("box_canvas"));
        gtk_box_pack_start(box_canvas, ui_renderer_widget(), true, true, 0);

        /* set window title */
        gtk_window_set_title(GTK_WINDOW(UI("main_window")),
                             PACKAGE " v" PACKAGE_VERSION);

        return true;
}


/** deinitialize module */
void ui_deinit()
{
        ui_renderer_deinit();
        ui_about_deinit();
        ui_clipboard_deinit();
        ui_setup_deinit();
        ui_info_hardware_deinit();
        ui_log_deinit();

        /* unregister prefs class */
        nft_prefs_class_unregister(prefs(), "ui");

        g_object_unref(_ui);
}


/**
 * build UI
 */
GtkBuilder *ui_builder(gchar * file)
{
        /* create ui */
        GtkBuilder *ui;
        if(!(ui = gtk_builder_new()))
        {
                g_error("Failed to create GtkBuilder");
        }

        /* try to load from datadir */
        gchar *s = g_build_filename("./data", file, NULL);
        if(gtk_builder_add_from_file(ui, s, NULL) == 0)
        {
                /* try to load from source dir */
                gchar *s2 =
                        g_build_filename(DATADIR, PACKAGE_NAME, file, NULL);

                GError *error = NULL;
                if(gtk_builder_add_from_file(ui, s2, &error) == 0)
                {
                        g_error("Failed to add UI from \"%s\" or \"%s\": %s",
                                s, s2, error->message);
                        g_error_free(error);
                        g_free(s2);
                        g_free(s);
                        return NULL;
                }
                g_free(s2);
        }
        g_free(s);

        /* connect signals defined in XML file */
        gtk_builder_connect_signals(ui, NULL);

        return ui;
}


/** getter for GtkBuilder of this module */
GObject *ui(const char *n)
{
        return UI(n);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** close main window */
G_MODULE_EXPORT gboolean on_niftyconf_window_delete_event(GtkWidget * w,
                                                          GdkEvent * e)
{
        /* store preferences */
        prefs_save();

        /* bye bye */
        gtk_main_quit();
        return true;
}


/** menuitem "quit" selected */
G_MODULE_EXPORT void on_action_quit_activate(GtkAction * a, gpointer u)
{
        /* store preferences */
        prefs_save();

        /* bye bye */
        gtk_main_quit();
}




/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_help_about_activate(GtkWidget * i,
                                                           gpointer u)
{
        ui_about_set_visible(true);
}
