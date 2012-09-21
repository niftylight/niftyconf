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

/** getter for current settings */
LedSetup *setup_get_current()
{
        return _setup;
}


/** getter for tree widget */
GtkWidget *setup_get_widget()
{
        return GTK_WIDGET(UI("box"));
}

/** show "add hardware" window */
void setup_show_add_hardware_window(bool visible)
{	
	/* show window */
	gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), visible);
}

/** create new hardware element in setup */
NiftyconfHardware *setup_new_hardware(const char *name, const char *family, 
                                      const char *id, LedCount ledcount, 
                                      const char *pixelformat)
{
        /* create new niftyled hardware */
        LedHardware *h;
        if(!(h = led_hardware_new(name, family)))
        {
                log_alert_show("Failed to create new hardware");
                return FALSE;
        }

	/* try to initialize hardware */
	if(!led_hardware_init(h, id, ledcount, pixelformat))
	{
		log_alert_show("Failed to initialize new hardware. Not connected?");
	}
	
        /* register hardware to gui */
        NiftyconfHardware *hardware;
        if(!(hardware = hardware_register(h)))
        {
                log_alert_show("Failed to register hardware to GUI");
                led_hardware_destroy(h);
                return FALSE;
        }

	/* register chain of hardware to gui */
	LedChain *chain;
	if((chain = led_hardware_get_chain(h)))
	{
		chain_register(chain);
	}
	
	/* get last hardware node */
	LedHardware *last = led_setup_get_hardware(setup_get_current());
	if(!last)
	{
		/* first hardware in setup */
		led_setup_set_hardware(setup_get_current(), h);
	}
	else
	{
		/* append to end of setup */
		led_hardware_list_append(last, h);
	}
        
        /* create config */
        /*if(!led_setup_create_from_hardware(setup_get_current(), h))
                return FALSE;*/
        

        return hardware;

}



/** remove hardware from current setup */
void setup_destroy_hardware(NiftyconfHardware *hw)
{
        LedHardware *h = hardware_niftyled(hw);
        
        /* unregister hardware */
        hardware_unregister(hw);

	/* remove hardware from setup? */
	if(led_setup_get_hardware(_setup) == h)
		led_setup_set_hardware(_setup, NULL);
	
        //led_settings_hardware_unlink(setup_get_current(), h);
        led_hardware_destroy(h);
}


/** create new chain for tile */
gboolean setup_new_chain_of_tile(NiftyconfTile *parent, 
                                  LedCount length, 
                                  const char *pixelformat)
{
        /** create new chain @todo select format */
        LedChain *n;
        if(!(n = led_chain_new(length, pixelformat)))
                return FALSE;
        
        /* attach chain to tile */
        LedTile *tile = tile_niftyled(parent);
        led_tile_set_chain(tile, n);

        /* create config */
        //if(!led_settings_create_from_chain(setup_get_current(), n))
        //        return FALSE;
        
        /* register chain to gui */
        chain_register(n);

        return TRUE;
}


/** remove chain from current setup */
void setup_destroy_chain_of_tile(NiftyconfTile *tile)
{
        /* get niftyled tile */
        LedTile *t = tile_niftyled(tile);
        
        /* if this tile has no chain, silently succeed */
        LedChain *c;
        if(!(c = led_tile_get_chain(t)))
                return;

        /* unregister from tile */
        led_tile_set_chain(t, NULL);
        
        /* unregister from gui */
        NiftyconfChain *chain = led_chain_get_privdata(c);
        chain_unregister(chain);
        //led_settings_chain_unlink(setup_get_current(), c);
        led_chain_destroy(c);
}


/** create new tile for hardware parent */
gboolean setup_new_tile_of_hardware(NiftyconfHardware *parent)
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
                led_tile_list_append(tile, n);
        }

        /* register new tile to gui */
        tile_register(n);

        /* create config */
        //if(!led_settings_create_from_tile(setup_get_current(), n))
        //        return FALSE;
        
        return TRUE;
}


/** create new tile for tile parent */
gboolean setup_new_tile_of_tile(NiftyconfTile *parent)
{
        /* create new tile */
        LedTile *n;
        if(!(n = led_tile_new()))
                return FALSE;

        LedTile *tile = tile_niftyled(parent);
        led_tile_append_child(tile, n);

        /* register new tile to gui */
        tile_register(n);

        /* create config */
        //if(!led_settings_create_from_tile(setup_get_current(), n))
        //        return FALSE;
        
        return TRUE;
}


/** remove tile from current setup */
void setup_destroy_tile(NiftyconfTile *tile)
{
        if(!tile)
                NFT_LOG_NULL();
        
        LedTile *t = tile_niftyled(tile);
        
        /* unregister from gui */
        tile_unregister(tile);
        /* unregister from settings */
        //led_settings_tile_unlink(setup_get_current(), t);
        /* destroy with all children */
        led_tile_destroy(t);
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
                    t = led_tile_list_get_next(t))
                {
                        if(!tile_register(t))
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
                        tile_unregister(led_tile_get_privdata(t));
                }
                
                chain_unregister(led_chain_get_privdata(led_hardware_get_chain(h)));
                hardware_unregister(led_hardware_get_privdata(h));
        }
        
        led_setup_destroy(_setup);
        setup_tree_clear();

	led_prefs_deinit(_prefs);
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
        if(!(h = setup_new_hardware(
                  gtk_entry_get_text(GTK_ENTRY(UI("hardware_add_name_entry"))),
                  gtk_combo_box_get_active_text(GTK_COMBO_BOX(UI("hardware_add_plugin_combobox"))),
		  gtk_entry_get_text(GTK_ENTRY(UI("hardware_add_id_entry"))),
                  gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(UI("hardware_add_ledcount_spinbutton"))),
                  gtk_combo_box_get_active_text(GTK_COMBO_BOX(UI("hardware_add_pixelformat_comboboxtext"))))))
		return;
		
	/* hide window */
	setup_show_add_hardware_window(false);
	
        /** @todo refresh our menu */
        
	/* refresh tree */
        setup_tree_refresh();

}

/** add hardware "cancel" clicked */
void on_add_hardware_cancel_clicked(GtkButton *b, gpointer u)
{
	setup_show_add_hardware_window(false);
}
