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


