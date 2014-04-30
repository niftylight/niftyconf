/*
 * niftyconf - niftyled GUI
 * Copyright (C) 2011-2014 Daniel Hiepler <daniel@niftylight.de>
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
#include "ui/ui.h"
#include "ui/ui-log.h"
#include "ui/ui-renderer.h"
#include "ui/ui-setup.h"
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-ledlist.h"
#include "ui/ui-clipboard.h"
#include "ui/ui-hardware.h"
#include "elements/element-setup.h"
#include "elements/element-hardware.h"
#include "live-preview/live-preview.h"






/** GtkBuilder for this module */
static GtkBuilder *_builder;



/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/
static GtkBuilder * _get_builder()
{
        return _builder;
}


static GObject *_ui(const char *name)
{
        return gtk_builder_get_object(_get_builder(), name);
}


/******************************************************************************
 ****************************** PUBLIC FUNCTIONS ****************************** 
 ******************************************************************************/

/** getter for GtkBuilder of this module */
GObject *ui_setup(const char *n)
{
        return _ui(n);
}


/** getter for tree widget */
GtkWidget *ui_setup_get_widget()
{
        return GTK_WIDGET(_ui("box"));
}


/** save setup to file */
gboolean ui_setup_save(gchar * filename)
{
        LedSetup *s;
        if(!(s = setup_get_current()))
        {
                ui_log_alert_show("There is no setup to save, yet");
                return false;
        }


        if(!filename)
        {
                /* try to get current filename */
                if(!(filename = (gchar *) setup_get_current_filename()))
                {
                        /* show "save-as" dialog */
                        gtk_widget_show(GTK_WIDGET
                                        (_ui("filechooserdialog_save")));
                        return false;
                }
        }

        NFT_LOG(L_INFO, "Saving setup to \"%s\"", filename);

        /* create prefs-node from current setup */
        LedPrefsNode *n;
        if(!(n = led_prefs_setup_to_node(setup_get_prefs(), s)))
        {
                ui_log_alert_show
                        ("Failed to create preferences from current setup.");
                return false;
        }

        /* file existing? */
        struct stat sts;
        if(stat(filename, &sts) == -1)
        {
                /* continue if stat error was caused because file doesn't exist 
                 */
                if(errno != ENOENT)
                {
                        ui_log_alert_show("Failed to access \"%s\" - %s",
                                          filename, strerror(errno));
                        return false;
                }
        }
        /* stat succeeded, file exists */
        else
        {
                /* remove old file? */
                if(!ui_log_dialog_yesno
                   ("Overwrite",
                    "A file named \"%s\" already exists.\nOverwrite?",
                    filename))
                        return false;

                if(unlink(filename) == -1)
                {
                        ui_log_alert_show
                                ("Failed to remove old version of \"%s\" - %s",
                                 filename, strerror(errno));
                        return false;
                }
        }

        /* save */
        if(!led_prefs_node_to_file(setup_get_prefs(), n, filename, false))
        {
                ui_log_alert_show("Failed to save preferences to \"%s\"",
                                  filename);
                return false;
        }

        /* all went ok */
        return true;
}


/** load new setup from file */
gboolean ui_setup_load(gchar * filename)
{
        /* load file */
        LedPrefsNode *n;
        if(!(n = led_prefs_node_from_file(setup_get_prefs(), filename)))
                return false;

        /* build setup from prefs node */
        LedSetup *s;
        if(!(s = led_prefs_setup_from_node(setup_get_prefs(), n)))
                return false;

        /* register new setup */
        if(!setup_register_to_gui(s))
                return false;

        /* set current filename */
        setup_set_current_filename(filename);

        /* update ui */
        ui_setup_tree_refresh();

        return true;
}



/** initialize setup module */
gboolean ui_setup_init()
{
        _builder = ui_builder("niftyconf-setup.ui");



        /* initialize tree module */
        if(!ui_setup_ledlist_init())
                return false;
        if(!ui_setup_tree_init())
                return false;
        if(!ui_setup_props_init())
                return false;


        /* attach children */
        GtkBox *box_tree = GTK_BOX(_ui("box_tree"));
        gtk_box_pack_start(box_tree, ui_setup_tree_get_widget(), true, true,
                           0);
        GtkBox *box_props = GTK_BOX(_ui("box_props"));
        gtk_box_pack_start(box_props, ui_setup_props_get_widget(), false,
                           false, 0);

        /* initialize file-filter to only show XML files in "open" filechooser
         * dialog */
        GtkFileFilter *filter = GTK_FILE_FILTER(_ui("filefilter"));
        gtk_file_filter_add_mime_type(filter, "application/xml");
        gtk_file_filter_add_mime_type(filter, "text/xml");

        /* clear "plugin family" comobobox */
        gtk_list_store_clear(GTK_LIST_STORE(
            gtk_combo_box_get_model(
                GTK_COMBO_BOX(_ui("hardware_add_plugin_combobox")))));
    
        /* build "plugin family" combobox for "add hardware" dialog */        
        for(int p = 0; p < led_hardware_plugin_total_count(); p++)
        {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (_ui
                                                ("hardware_add_plugin_combobox")),
                                               led_hardware_plugin_get_family_by_n
                                               (p));
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX
                                 (_ui("hardware_add_plugin_combobox")), 0);

        /* initialize pixel-format module */
        led_pixel_format_new();

        /* build "pixelformat" combobox for "add hardware" dialog */
        size_t f;
        for(f = led_pixel_format_get_n_formats(); f > 0; f--)
        {
                const char *format =
                        led_pixel_format_to_string(led_pixel_format_get_nth
                                                   (f - 1));
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (_ui
                                                ("hardware_add_pixelformat_comboboxtext")),
                                               format);
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (_ui
                                                ("chain_add_pixelformat_comboboxtext")),
                                               format);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX
                                 (_ui
                                  ("hardware_add_pixelformat_comboboxtext")),
                                 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX
                                 (_ui("chain_add_pixelformat_comboboxtext")),
                                 0);

        /* start with fresh empty setup */
        LedSetup *setup;
        if(!(setup = led_setup_new()))
                return false;

        setup_register_to_gui(setup);

        // g_object_unref(ui);

        return true;
}



/** deinitialize setup module */
void ui_setup_deinit()
{

        /* deinitialize all modules used by this module */
        ui_setup_props_deinit();
        ui_setup_tree_deinit();
        ui_setup_ledlist_deinit();

        /* denitialize niftyled prefs instance */
        led_pixel_format_destroy();

        /* gtk stuff */
        g_object_unref(_builder);
}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/** New setup */
G_MODULE_EXPORT void on_action_setup_new_activate(GtkAction * a, gpointer u)
{
        LedSetup *s;
        if(!(s = led_setup_new()))
        {
                NFT_LOG(L_ERROR, "Failed to create new settings descriptor.");
                return;
        }

        /* save new settings */
        setup_register_to_gui(s);
        setup_set_current_filename("Unnamed.xml");
        ui_setup_tree_clear();
        ui_renderer_all_queue_draw();
}


/** menuitem "open" selected */
G_MODULE_EXPORT void on_action_setup_open_activate(GtkAction * a, gpointer u)
{
        gtk_widget_show(GTK_WIDGET(_ui("filechooserdialog_load")));
}


/** menuitem "save" selected */
G_MODULE_EXPORT void on_action_setup_save_activate(GtkAction * a, gpointer u)
{
        if(!ui_setup_save(NULL))
        {
                NFT_LOG(L_ERROR, "Error while saving current setup.");
                return;
        }
}


/** menuitem "save as" selected */
G_MODULE_EXPORT void on_action_setup_saveas_activate(GtkAction * a, gpointer u)
{
        gtk_widget_show(GTK_WIDGET(_ui("filechooserdialog_save")));
}







/** import */
G_MODULE_EXPORT void on_action_import_activate(GtkAction * a, gpointer u)
{
        gtk_widget_show(GTK_WIDGET(_ui("filechooserdialog_import")));
}


/** export */
G_MODULE_EXPORT void on_action_export_activate(GtkAction * a, gpointer u)
{
        gtk_widget_show(GTK_WIDGET(_ui("filechooserdialog_export")));
}


/** live preview toggled */
G_MODULE_EXPORT void on_toggleaction_live_preview_show_toggled(GtkToggleAction
                                                               * a,
                                                               gpointer user_data)
{
        live_preview_set_enabled(gtk_toggle_action_get_active(a));
}


/** "save" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_save_save_clicked(GtkButton * b, gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (_ui
                                                       ("filechooserdialog_save")))))
        {
                NFT_LOG(L_ERROR, "No filename received from dialog.");
                return;
        }

        if(!ui_setup_save(filename))
        {
                NFT_LOG(L_ERROR, "Error while saving file \"%s\"", filename);
                goto osssc_exit;
        }

        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_save")));

osssc_exit:
        g_free(filename);
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_save_cancel_clicked(GtkButton * b, gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_save")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_open_cancel_clicked(GtkButton * b, gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_load")));
}

/** "open" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_open_clicked(GtkButton * b, gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (_ui
                                                       ("filechooserdialog_load")))))
        {
                ui_log_alert_show("No filename given?");
                return;
        }

        if(!ui_setup_load(filename))
        {
                ui_log_alert_show("Error while loading file\n \"%s\"\n"
                                  "(s. log for further info)",
                                  filename);
        }

        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_load")));
        g_free(filename);
}

/** "export" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_export_clicked(GtkButton * b, gpointer u)
{
        /* filename for export file */
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (_ui
                                                       ("filechooserdialog_export")))))
        {
                NFT_LOG(L_ERROR, "No filename received from dialog.");
                return;
        }

        if(!ui_clipboard_copy_to_file(filename))
        {
                ui_log_alert_show("Failed to copy element to file \"%s\"",
                                  filename);
                return;
        }

        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_export")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_export_cancel_clicked(GtkButton * b, gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_export")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_import_cancel_clicked(GtkButton * b, gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_import")));
}


/** "import" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_import_clicked(GtkButton * b, gpointer u)
{
        /* filename for export file */
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (_ui
                                                       ("filechooserdialog_import")))))
        {
                ui_log_alert_show("No filename received from dialog.");
                return;
        }

        if(!ui_clipboard_paste_from_file(filename))
        {
                ui_log_alert_show("Failed to paste content of \"%s\"",
                                  filename);
                return;
        }

        gtk_widget_hide(GTK_WIDGET(_ui("filechooserdialog_import")));
}
