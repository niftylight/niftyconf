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
#include "niftyconf-setup-ledlist.h"
#include "niftyconf-hardware.h"
#include "niftyconf-ui.h"
#include "niftyconf-log.h"




/** GtkBuilder for this module */
static GtkBuilder *_ui;
/** current niftyled settings context */
static LedPrefs *_prefs;
/** current niftyled setup */
static LedSetup *_setup;





/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/




/******************************************************************************
 ******************************************************************************/

/** getter for GtkBuilder of this module */
GObject *setup_ui(const char *n)
{
	return UI(n);
}

/** getter for current setup */
LedSetup *setup_get_current()
{
        return _setup;
}


/** getter for current preference context */
LedPrefs *setup_get_prefs()
{
	return _prefs;
}


/** getter for tree widget */
GtkWidget *setup_get_widget()
{
        return GTK_WIDGET(UI("box"));
}


/** load new setup from XML file definition */
gboolean setup_load(gchar *filename)
{
        /* load file */
	LedPrefsNode *n;
	if(!(n = led_prefs_node_from_file(_prefs, filename)))
		return FALSE;
		
        LedSetup *s;
        if(!(s = led_prefs_setup_from_node(_prefs, n)))
                return FALSE;

        
        /* cleanup previously loaded setup */
        if(_setup)
                setup_cleanup();

        /* initialize our element descriptor and set as
           privdata in niftyled model */
        LedHardware *h;
        for(h = led_setup_get_hardware(s); 
            h; 
            h = led_hardware_list_get_next(h))
        {
                /* create new hardware element */
                if(!hardware_register_to_gui(h))
                {
                        g_warning("failed to allocate new hardware element");
                        return FALSE;
                }

                /* create chain of this hardware */
                if(!chain_register_to_gui(led_hardware_get_chain(h)))
                {
                        g_warning("failed to allocate new chain element");
                        return FALSE;
                }

                /* walk all tiles belonging to this hardware & initialize */
                LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_list_get_next(t))
                {
                        if(!tile_register_to_gui(t))
                        {
                                g_warning("failed to allocate new tile element");
                                return FALSE;
                        }
                }
        }
        
        /* save new settings */
        _setup = s;

        /* update ui */
        setup_tree_refresh();
        
        
        /* redraw new setup */
        //setup_redraw();
        
        return TRUE;
}


/** cleanup previously loaded setup */
void setup_cleanup()
{
        /* free all hardware nodes */
        LedHardware *h;
        for(h = led_setup_get_hardware(_setup);
            h;
            h = led_hardware_list_get_next(h))
        {
                LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_list_get_next(t))
                {
                        tile_unregister_from_gui(led_tile_get_privdata(t));
                }
                
                chain_unregister_from_gui(led_chain_get_privdata(led_hardware_get_chain(h)));
                hardware_unregister_from_gui(led_hardware_get_privdata(h));
        }
        
        led_setup_destroy(_setup);
        setup_tree_clear();
	_setup = NULL;
}


/** initialize setup module */
gboolean setup_init()
{
        _ui = ui_builder("niftyconf-setup.ui");

	/* initialize preference context */
	if(!(_prefs = led_prefs_init()))
		return FALSE;
	   
        /* initialize tree module */
        if(!setup_ledlist_init())
                return FALSE;
        if(!setup_tree_init())
                return FALSE;
        if(!setup_props_init())
                return FALSE;
        
        
        /* attach children */
        GtkBox *box_tree = GTK_BOX(gtk_builder_get_object(_ui, "box_tree"));
        gtk_box_pack_start(box_tree, setup_tree_get_widget(), TRUE, TRUE, 0);
        GtkBox *box_props = GTK_BOX(gtk_builder_get_object(_ui, "box_props"));
        gtk_box_pack_start(box_props, setup_props_get_widget(), FALSE, FALSE, 0);
        
        /* initialize file-filter to only show XML files in "open" filechooser dialog */
        GtkFileFilter *filter = GTK_FILE_FILTER(UI("filefilter"));
        gtk_file_filter_add_mime_type(filter, "application/xml");
        gtk_file_filter_add_mime_type(filter, "text/xml");

	/* build "plugin" combobox for "add hardware" dialog */
	unsigned int p;
	for(p = 0; p < led_hardware_plugin_total_count(); p++)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(UI("hardware_add_plugin_combobox")), 
		                               led_hardware_plugin_get_family_by_n(p));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(UI("hardware_add_plugin_combobox")), 0);

	/* initialize pixel-format module */
	led_pixel_format_new();
	
	/* build "pixelformat" combobox for "add hardware" dialog */
	size_t f;
	for(f = led_pixel_format_get_n_formats(); f > 0; f--)
	{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(UI("hardware_add_pixelformat_comboboxtext")),
		                               led_pixel_format_to_string(led_pixel_format_get_nth(f-1)));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(UI("hardware_add_pixelformat_comboboxtext")), 0);
	
        /* start with fresh empty setup */
	_setup = led_setup_new();

        //g_object_unref(ui);
        
        return TRUE;
}



/** deinitialize setup module */
void setup_deinit()
{
	setup_cleanup();
	led_prefs_deinit(_prefs);
	_prefs = NULL;
}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/** menuitem "new" selected */
void on_setup_menuitem_new_activate(GtkMenuItem *i, gpointer d)
{
        LedSetup *s;
        if(!(s = led_setup_new()))
        {
                NFT_LOG(L_ERROR, "Failed to create new settings descriptor.");
                return;
        }
        
        setup_cleanup();

        /* save new settings */
        _setup = s;
}

/** menuitem "open" selected */
void on_setup_menuitem_open_activate(GtkMenuItem *i, gpointer d)
{
        gtk_widget_show(GTK_WIDGET(UI("filechooserdialog")));
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
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog")));
}

/** open file */
void on_setup_open_clicked(GtkButton *b, gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(UI("filechooserdialog")))))
	{
		log_alert_show("No filename?");
                return;
	}
	
        if(!setup_load(filename))
        {
                log_alert_show("Error while loading file");
                return;
        }
        
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog")));
}

/** add hardware "add" clicked */
void on_add_hardware_add_clicked(GtkButton *b, gpointer u)
{
	/* add new hardware */
	NiftyconfHardware *h;
        if(!(h = hardware_new(
                  gtk_entry_get_text(GTK_ENTRY(UI("hardware_add_name_entry"))),
                  gtk_combo_box_get_active_text(GTK_COMBO_BOX(UI("hardware_add_plugin_combobox"))),
		  gtk_entry_get_text(GTK_ENTRY(UI("hardware_add_id_entry"))),
                  gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(UI("hardware_add_ledcount_spinbutton"))),
                  gtk_combo_box_get_active_text(GTK_COMBO_BOX(UI("hardware_add_pixelformat_comboboxtext"))))))
		return;
		
	/* hide window */
	gtk_widget_set_visible(GTK_WIDGET(setup_ui("hardware_add_window")), FALSE);
	
        /** @todo refresh our menu */
        
	/* refresh tree */
        setup_tree_refresh();

}

/** add hardware "cancel" clicked */
void on_add_hardware_cancel_clicked(GtkButton *b, gpointer u)
{
	gtk_widget_set_visible(GTK_WIDGET(setup_ui("hardware_add_window")), FALSE);
}

/** add hardware "pixelformat" changed */
void on_hardware_add_pixelformat_comboboxtext_changed(GtkComboBox *w, gpointer u)
{
	LedPixelFormat *f;
	if(!(f = led_pixel_format_from_string(gtk_combo_box_get_active_text(w))))
	{
		/* invalid pixel format? */
		gtk_widget_set_sensitive(GTK_WIDGET(UI("hardware_add_ledcount_spinbutton")), false);
		return;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(UI("hardware_add_ledcount_spinbutton")), true);

	/* set minimum for "ledcount" spinbutton according to format */
	size_t minimum = led_pixel_format_get_n_components(f);
	gtk_adjustment_set_lower(GTK_ADJUSTMENT(UI("hardware_add_ledcount_adjustment")), (gdouble) minimum);
	if(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(UI("hardware_add_ledcount_spinbutton"))) < minimum)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("hardware_add_ledcount_spinbutton")), (gdouble) minimum);
	}
}
