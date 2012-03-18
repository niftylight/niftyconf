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
#include "niftyconf-setup-props.h"
#include "niftyconf-setup-tree.h"
#include "niftyconf-hardware.h"
#include "niftyconf-ui.h"





/** current niftyled settings context */
static LedSettings *current;
/** main container widget of this module */
static GtkBox *box;
/** "open" filechooser dialog */
static GtkFileChooserDialog *open_filechooser;





/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/




/******************************************************************************
 ******************************************************************************/

/** getter for current settings */
LedSettings *setup_get_current()
{
        return current;
}


/**
 * getter for tree widget
 */
GtkWidget *setup_get_widget()
{
        return GTK_WIDGET(box);
}


/**
 * load new setup from XML file definition 
 */
gboolean setup_load(gchar *filename)
{
        /* load new setup */
        LedSettings *s;
        if(!(s = led_settings_load(filename)))
                return FALSE;

        
        /* cleanup previously loaded setup */
        if(current)
                setup_cleanup();

        /* initialize our element descriptor and set as
           privdata in niftyled model */
        LedHardware *h;
        for(h = led_settings_hardware_get_first(s); 
            h; 
            h = led_hardware_get_next_sibling(h))
        {
                /* create new hardware element */
                if(!hardware_register(h))
                {
                        g_warning("failed to allocate new hardware element");
                        return FALSE;
                }

                /* create chain of this hardware */
                if(!chain_register(led_hardware_get_chain(h)))
                {
                        g_warning("failed to allocate new chain element");
                        return FALSE;
                }

                /* walk all tiles belonging to this hardware & initialize */
                LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_get_next_sibling(t))
                {
                        if(!tile_register(t))
                        {
                                g_warning("failed to allocate new tile element");
                                return FALSE;
                        }
                }
        }
        
        /* save new settings */
        current = s;

        /* update ui */
        setup_tree_clear();
        setup_tree_rebuild();
        
        
        /* redraw new setup */
        //setup_redraw();
        
        return TRUE;
}


/** 
 * cleanup previously loaded setup 
 */
void setup_cleanup()
{
        /* free all hardware nodes */
        LedHardware *h;
        for(h = led_settings_hardware_get_first(current);
            h;
            h = led_hardware_get_next_sibling(h))
        {
                LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_get_next_sibling(t))
                {
                        tile_unregister(led_tile_get_privdata(t));
                }
                
                chain_unregister(led_chain_get_privdata(led_hardware_get_chain(h)));
                hardware_unregister(led_hardware_get_privdata(h));
        }
        
        led_settings_destroy(current);
        setup_tree_clear();
}


/**
 * initialize setup module
 */
gboolean setup_init()
{
        GtkBuilder *ui = ui_builder("niftyconf-setup.ui");

        /* initialize tree module */
        if(!setup_tree_init())
                return FALSE;
        if(!setup_props_init())
                return FALSE;
        
        /* get widgets */
        if(!(box = GTK_BOX(gtk_builder_get_object(ui, "box"))))
                return FALSE;
        if(!(open_filechooser = GTK_FILE_CHOOSER_DIALOG(gtk_builder_get_object(ui, "filechooserdialog"))))
                return FALSE;

        /* attach children */
        GtkBox *box_tree = GTK_BOX(gtk_builder_get_object(ui, "box_tree"));
        gtk_box_pack_start(box_tree, setup_tree_get_widget(), TRUE, TRUE, 0);
        GtkBox *box_props = GTK_BOX(gtk_builder_get_object(ui, "box_props"));
        gtk_box_pack_start(box_props, setup_props_get_widget(), FALSE, FALSE, 0);
        
        /* initialize file-filter to only show XML files in "open" filechooser dialog */
        GtkFileFilter *filter = GTK_FILE_FILTER(gtk_builder_get_object(ui, "filefilter"));
        gtk_file_filter_add_mime_type(filter, "application/xml");
        gtk_file_filter_add_mime_type(filter, "text/xml");

        
        //g_object_unref(ui);
        
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/** menuitem "new" selected */
void on_setup_menuitem_new_activate(GtkMenuItem *i, gpointer d)
{
        LedSettings *s;
        if(!(s = led_settings_new()))
        {
                NFT_LOG(L_ERROR, "Failed to create new settings descriptor.");
                return;
        }
        
        setup_cleanup();

        /* save new settings */
        current = s;
}

/** menuitem "open" selected */
void on_setup_menuitem_open_activate(GtkMenuItem *i, gpointer d)
{
        gtk_widget_show(GTK_WIDGET(open_filechooser));
}


/** menuitem "save" selected */
void on_setup_menuitem_save_activate(GtkMenuItem *i, gpointer d)
{
        
}


/** menuitem "save as" selected */
void on_setup_menuitem_save_as_activate(GtkMenuItem *i, gpointer d)
{
        
}

/** "cancel" button in filechooser clicked */
void on_setup_open_cancel_clicked(GtkButton *b, gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(open_filechooser));
}

/** open file */
void on_setup_open_clicked(GtkButton *b, gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_filechooser))))
                return;

        if(!setup_load(filename))
        {
                /* @TODO display error dialog */
                return;
        }
        
        gtk_widget_hide(GTK_WIDGET(open_filechooser));
}
