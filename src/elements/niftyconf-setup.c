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
#include "ui/niftyconf-setup-props.h"
#include "ui/niftyconf-setup-tree.h"
#include "ui/niftyconf-setup-ledlist.h"
#include "elements/niftyconf-setup.h"
#include "elements/niftyconf-hardware.h"
#include "ui/niftyconf-ui.h"
#include "ui/niftyconf-log.h"




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

/** save setup to file */
gboolean setup_save(gchar *filename)
{
	if(!_setup)
	{
		log_alert_show("There is no setup to save, yet");
		return FALSE;
	}


	if(!filename)
	{
		/* try to get current filename */
		if(!(filename = (gchar *) led_prefs_current_filename(setup_get_prefs())))
		{
			/* show "save-as" dialog */
			gtk_widget_show(GTK_WIDGET(UI("filechooserdialog_save")));
			return FALSE;
		}
	}

	NFT_LOG(L_INFO, "Saving setup to \"%s\"", filename);

	/* create prefs-node from current setup */
	LedPrefsNode *n;
	if(!(n = led_prefs_setup_to_node(_prefs, _setup)))
	{
		log_alert_show("Failed to create preferences from current setup.");
		return FALSE;
	}

	/* file existing? */
	struct stat sts;
	if(stat(filename, &sts) == -1)
	{
			/* continue if stat error was caused because file doesn't exist */
			if(errno != ENOENT)
			{
					log_alert_show("Failed to access \"%s\" - %s", filename, strerror(errno));
					return false;
			}
	}
	/* stat succeeded, file exists */
	else
	{
			/* remove old file? */
			if(!log_dialog_yesno("Overwrite", "A file named \"%s\" already exists.\nOverwrite?", filename))
					return false;

			if(unlink(filename) == -1)
			{
					log_alert_show("Failed to remove old version of \"%s\" - %s", filename, strerror(errno));
					return false;
			}
	}

	/* save */
	if(!led_prefs_node_to_file(_prefs, n, filename, false))
	{
		log_alert_show("Failed to save preferences to \"%s\"", filename);
		return FALSE;
	}

	/* all went ok */
	return TRUE;
}


/** load new setup from file */
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
        }

        /* save new settings */
        _setup = s;

        /* update ui */
        setup_tree_refresh();

        return TRUE;
}


/** dump element definition to printable string - use free() to deallacote the result */
char *setup_dump(gboolean encapsulation)
{
		LedPrefsNode *n;
		if(!(n = led_prefs_setup_to_node(_prefs, _setup)))
		{
				log_alert_show("Failed to create preferences from current setup.");
				return NULL;
		}

		char *result = NULL;
		if(encapsulation)
			result = led_prefs_node_to_buffer(_prefs, n);
		else
			result = led_prefs_node_to_buffer_light(_prefs, n);

		led_prefs_node_free(n);

		return result;
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
		/* unregister hardware */
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
				const char *format = led_pixel_format_to_string(led_pixel_format_get_nth(f-1));
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(UI("hardware_add_pixelformat_comboboxtext")),
										   format);
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(UI("chain_add_pixelformat_comboboxtext")),
										   format);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(UI("hardware_add_pixelformat_comboboxtext")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(UI("chain_add_pixelformat_comboboxtext")), 0);

		/* start with fresh empty setup */
		_setup = led_setup_new();

		//g_object_unref(ui);

		return TRUE;
}



/** deinitialize setup module */
void setup_deinit()
{
	/* cleanup our current setup */
	setup_cleanup();

	/* deinitialize all modules used by this module */
	setup_props_deinit();
	setup_tree_deinit();
	setup_ledlist_deinit();

	/* denitialize niftyled prefs instance */
	led_prefs_deinit(_prefs);
	_prefs = NULL;
	led_pixel_format_destroy();

	/* gtk stuff */
	g_object_unref(_ui);
}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/** menuitem "new" selected */
G_MODULE_EXPORT void on_setup_menuitem_new_activate(GtkMenuItem *i, gpointer d)
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
G_MODULE_EXPORT void on_setup_menuitem_open_activate(GtkMenuItem *i, gpointer d)
{
        gtk_widget_show(GTK_WIDGET(UI("filechooserdialog_load")));
}


/** menuitem "save" selected */
G_MODULE_EXPORT void on_setup_menuitem_save_activate(GtkMenuItem *i, gpointer d)
{
        if(!setup_save(NULL))
        {
                NFT_LOG(L_ERROR, "Error while saving current setup.");
                return;
        }
}


/** menuitem "save as" selected */
G_MODULE_EXPORT void on_setup_menuitem_save_as_activate(GtkMenuItem *i, gpointer d)
{
	gtk_widget_show(GTK_WIDGET(UI("filechooserdialog_save")));
}


/** "save" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_save_save_clicked(GtkButton *b, gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(UI("filechooserdialog_save")))))
	{
		NFT_LOG(L_ERROR, "No filename received from dialog.");
                return;
	}

        if(!setup_save(filename))
        {
                NFT_LOG(L_ERROR, "Error while saving file \"%s\"", filename);
                goto osssc_exit;
        }

        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_save")));

osssc_exit:
	g_free(filename);
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_save_cancel_clicked(GtkButton *b, gpointer u)
{
	gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_save")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_open_cancel_clicked(GtkButton *b, gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_load")));
}

/** "open" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_open_clicked(GtkButton *b, gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(UI("filechooserdialog_load")))))
	{
		log_alert_show("No filename given?");
                return;
	}

        if(!setup_load(filename))
        {
                log_alert_show("Error while loading file \"%s\"", filename);
                goto osoc_exit;
        }

        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_load")));

osoc_exit:
	g_free(filename);
}

/** "export" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_export_clicked(GtkButton *b, gpointer u)
{
		/* filename for export file */
		char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(UI("filechooserdialog_export")))))
		{
				NFT_LOG(L_ERROR, "No filename received from dialog.");
				return;
		}

		/* dumped element */
		char *xml = NULL;

		/* get currently selected element */
		NIFTYLED_TYPE t;
        gpointer e;
        setup_tree_get_first_selected_element(&t, &e);

		switch(t)
		{
				/** nothing selected - whole setup */
				case LED_INVALID_T:
				{
						NFT_LOG(L_INFO, "Exporting LedSetup");

						if(!(xml = setup_dump(false)))
						{
								log_alert_show("Error while saving setup to \"%s\"", filename);
								goto osec_error;
						}

						break;
				}


				/* hardware */
				case LED_HARDWARE_T:
				{
						NFT_LOG(L_INFO, "Exporting LedHardware");
						NiftyconfHardware *h = (NiftyconfHardware *) e;

						/* dump element */
						if(!(xml = hardware_dump(h, false)))
						{
								log_alert_show("Error while dumping Hardware element.");
								goto osec_error;
						}

						break;
				}


				/* tile */
				case LED_TILE_T:
				{
						NFT_LOG(L_INFO, "Exporting LedTile");
						NiftyconfTile *t = (NiftyconfTile *) e;

						/* dump element */
						if(!(xml = tile_dump(t, false)))
						{
								log_alert_show("Error while dumping Tile element.");
								goto osec_error;
						}

						break;
				}

				/* chain */
				case LED_CHAIN_T:
				{
						NFT_LOG(L_INFO, "Exporting LedChain");
						NiftyconfChain *c = (NiftyconfChain *) e;

						/* dump element */
						if(!(xml = chain_dump(c, false)))
						{
								log_alert_show("Error while dumping Chain element.");
								goto osec_error;
						}

						break;
				}

				default:
				{
						log_alert_show("Unhandled NIFTYLED_TYPE %d (currently selected element). This is a bug.", t);
						g_error("Unhandled NIFTYLED_TYPE %d (currently selected element). This is a bug.", t);
						break;
				}
		}

		/* save dump? */
		if(xml)
		{
				int fd;
				if((fd = open(filename, O_EXCL | O_CREAT | O_WRONLY)) == -1 )
				{
						/* file already existing? */
						if(errno == EEXIST)
						{
								/* overwrite file? */
								if(log_dialog_yesno("Overwrite", "A file named \"%s\" already exists.\nOverwrite?", filename))
								{
										fd = open(filename, O_WRONLY | O_TRUNC);
								}
								else
								{
										/* user said "no" */
										goto osec_error;
								}
						}

						/* error occured? */
						if(fd == -1)
						{
								log_alert_show("Failed to save \"%s\": %s", filename, strerror(errno));
								goto osec_error;
						}
				}

				/* write dump into file */
				ssize_t length = strlen(xml);
				ssize_t written;
				if((written = write(fd, xml, length)) != length)
				{
						log_alert_show("Only %d of %d bytes written!", written, length);
				}

				close(fd);
		}

		gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_export")));

osec_error:
		return;
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_export_cancel_clicked(GtkButton *b, gpointer u)
{
		gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_export")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_import_cancel_clicked(GtkButton *b, gpointer u)
{
		gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_import")));
}


/** "import" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_import_clicked(GtkButton *b, gpointer u)
{
		/* get currently selected element */
		NIFTYLED_TYPE t;
        gpointer e;
        setup_tree_get_first_selected_element(&t, &e);

		/* currently selected element to import into */
		switch(t)
		{
				case LED_INVALID_T:
				{
						NFT_LOG(L_INFO, "Importing Setup");
						break;
				}

				case LED_HARDWARE_T:
				{
						NFT_LOG(L_INFO, "Importing Hardware element");
						break;
				}

				case LED_TILE_T:
				{
						NFT_LOG(L_INFO, "Importing Tile element");
						break;
				}

				case LED_CHAIN_T:
				{
						NFT_LOG(L_INFO, "Importing Chain element");
						break;
				}

				default:
				{
						NFT_LOG(L_ERROR, "Unhandled element type %d. This is a bug.", t);
						break;
				}
		}
}


/** add hardware "add" clicked */
G_MODULE_EXPORT void on_add_hardware_add_clicked(GtkButton *b, gpointer u)
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
	gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), FALSE);

        /** @todo refresh our menu */

	/* refresh tree */
        setup_tree_refresh();

}

/** add hardware "cancel" clicked */
G_MODULE_EXPORT void on_add_hardware_cancel_clicked(GtkButton *b, gpointer u)
{
	gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), FALSE);
}


/** add hardware window close */
G_MODULE_EXPORT gboolean on_add_hardware_window_delete_event(GtkWidget *w, GdkEvent *e)
{
	gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), FALSE);
        return TRUE;
}


/** add hardware "pixelformat" changed */
G_MODULE_EXPORT void on_hardware_add_pixelformat_comboboxtext_changed(GtkComboBox *w, gpointer u)
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
	gtk_adjustment_set_lower(GTK_ADJUSTMENT(UI("ledcount_adjustment")), (gdouble) minimum);
	if(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(UI("hardware_add_ledcount_spinbutton"))) < minimum)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("hardware_add_ledcount_spinbutton")), (gdouble) minimum);
	}
}


/** add chain "add" clicked */
G_MODULE_EXPORT void on_add_chain_add_clicked(GtkButton *b, gpointer u)
{

	/* get parent element */
	NIFTYLED_TYPE t;
	gpointer element;
	setup_tree_get_first_selected_element (&t, &element);

        /* add new chain */
       if(!chain_of_tile_new(t, element,
                         gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(UI("chain_add_ledcount_spinbutton"))),
                         gtk_combo_box_get_active_text (GTK_COMBO_BOX(UI("chain_add_pixelformat_comboboxtext")))))
		return;

	/* hide dialog */
	gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), FALSE);

        /* refresh tree */
        setup_tree_refresh();

}


/** add chain "cancel" clicked */
G_MODULE_EXPORT void on_add_chain_cancel_clicked(GtkButton *b, gpointer u)
{
	gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), FALSE);
}


/** add chain window close */
G_MODULE_EXPORT gboolean on_add_chain_window_delete_event(GtkWidget *w, GdkEvent *e)
{
	gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), FALSE);
        return TRUE;
}


/** add chain "pixelformat" changed */
G_MODULE_EXPORT void on_chain_add_pixelformat_comboboxtext_changed(GtkComboBox *w, gpointer u)
{
	LedPixelFormat *f;
	if(!(f = led_pixel_format_from_string(gtk_combo_box_get_active_text(w))))
	{
		/* invalid pixel format? */
		gtk_widget_set_sensitive(GTK_WIDGET(UI("chain_add_ledcount_spinbutton")), false);
		return;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(UI("chain_add_ledcount_spinbutton")), true);

	/* set minimum for "ledcount" spinbutton according to format */
	size_t minimum = led_pixel_format_get_n_components(f);
	gtk_adjustment_set_lower(GTK_ADJUSTMENT(UI("ledcount_adjustment")), (gdouble) minimum);
	if(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(UI("chain_add_ledcount_spinbutton"))) < minimum)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("chain_add_ledcount_spinbutton")), (gdouble) minimum);
	}
}
