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
#include "ui/ui-setup.h"
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-ledlist.h"
#include "ui/ui-clipboard.h"
#include "ui/ui.h"
#include "ui/ui-log.h"
#include "elements/element-setup.h"
#include "elements/element-hardware.h"
#include "live-preview/live-preview.h"



/** GtkBuilder for this module */
static GtkBuilder *_ui;





/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/




/******************************************************************************
 ******************************************************************************/

/** getter for GtkBuilder of this module */
GObject *ui_setup(
        const char *n)
{
        return UI(n);
}


/** getter for tree widget */
GtkWidget *ui_setup_get_widget(
        )
{
        return GTK_WIDGET(UI("box"));
}


/** save setup to file */
gboolean ui_setup_save(
        gchar * filename)
{
        LedSetup *s;
        if(!(s = setup_get_current()))
        {
                ui_log_alert_show("There is no setup to save, yet");
                return FALSE;
        }


        if(!filename)
        {
                /* try to get current filename */
                if(!(filename = (gchar *) setup_get_current_filename()))
                {
                        /* show "save-as" dialog */
                        gtk_widget_show(GTK_WIDGET
                                        (UI("filechooserdialog_save")));
                        return FALSE;
                }
        }

        NFT_LOG(L_INFO, "Saving setup to \"%s\"", filename);

        /* create prefs-node from current setup */
        LedPrefsNode *n;
        if(!(n = led_prefs_setup_to_node(setup_get_prefs(),
                                         setup_get_current())))
        {
                ui_log_alert_show
                        ("Failed to create preferences from current setup.");
                return FALSE;
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
        if(!led_prefs_node_to_file(n, filename, false))
        {
                ui_log_alert_show("Failed to save preferences to \"%s\"",
                                  filename);
                return FALSE;
        }

        /* all went ok */
        return TRUE;
}


/** load new setup from file */
gboolean ui_setup_load(
        gchar * filename)
{
        /* load file */
        LedPrefsNode *n;
        if(!(n = led_prefs_node_from_file(filename)))
                return FALSE;

        /* build setup from prefs node */
        LedSetup *s;
        if(!(s = led_prefs_setup_from_node(setup_get_prefs(), n)))
                return FALSE;

        /* register new setup */
        if(!setup_register_to_gui(s))
                return FALSE;

        /* set current filename */
        setup_set_current_filename(filename);

        /* update ui */
        ui_setup_tree_refresh();

        return TRUE;
}



/** initialize setup module */
gboolean ui_setup_init(
        )
{
        _ui = ui_builder("niftyconf-setup.ui");



        /* initialize tree module */
        if(!ui_setup_ledlist_init())
                return FALSE;
        if(!ui_setup_tree_init())
                return FALSE;
        if(!ui_setup_props_init())
                return FALSE;


        /* attach children */
        GtkBox *box_tree = GTK_BOX(gtk_builder_get_object(_ui, "box_tree"));
        gtk_box_pack_start(box_tree, ui_setup_tree_get_widget(), TRUE, TRUE,
                           0);
        GtkBox *box_props = GTK_BOX(gtk_builder_get_object(_ui, "box_props"));
        gtk_box_pack_start(box_props, ui_setup_props_get_widget(), FALSE,
                           FALSE, 0);

        /* initialize file-filter to only show XML files in "open" filechooser
         * dialog */
        GtkFileFilter *filter = GTK_FILE_FILTER(UI("filefilter"));
        gtk_file_filter_add_mime_type(filter, "application/xml");
        gtk_file_filter_add_mime_type(filter, "text/xml");

        /* build "plugin" combobox for "add hardware" dialog */
        int p;
        for(p = 0; p < led_hardware_plugin_total_count(); p++)
        {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (UI
                                                ("hardware_add_plugin_combobox")),
                                               led_hardware_plugin_get_family_by_n
                                               (p));
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX
                                 (UI("hardware_add_plugin_combobox")), 0);

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
                                               (UI
                                                ("hardware_add_pixelformat_comboboxtext")),
                                               format);
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (UI
                                                ("chain_add_pixelformat_comboboxtext")),
                                               format);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX
                                 (UI
                                  ("hardware_add_pixelformat_comboboxtext")),
                                 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX
                                 (UI("chain_add_pixelformat_comboboxtext")),
                                 0);

        /* start with fresh empty setup */
        LedSetup *setup;
        if(!(setup = led_setup_new()))
                return FALSE;

        setup_register_to_gui(setup);

        // g_object_unref(ui);

        return TRUE;
}



/** deinitialize setup module */
void ui_setup_deinit(
        )
{

        /* deinitialize all modules used by this module */
        ui_setup_props_deinit();
        ui_setup_tree_deinit();
        ui_setup_ledlist_deinit();

        /* denitialize niftyled prefs instance */
        led_pixel_format_destroy();

        /* gtk stuff */
        g_object_unref(_ui);
}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/** menuitem "new" selected */
G_MODULE_EXPORT void on_setup_menuitem_new_activate(
        GtkMenuItem * i,
        gpointer d)
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
}

/** menuitem "open" selected */
G_MODULE_EXPORT void on_setup_menuitem_open_activate(
        GtkMenuItem * i,
        gpointer d)
{
        gtk_widget_show(GTK_WIDGET(UI("filechooserdialog_load")));
}


/** menuitem "save" selected */
G_MODULE_EXPORT void on_setup_menuitem_save_activate(
        GtkMenuItem * i,
        gpointer d)
{
        if(!ui_setup_save(NULL))
        {
                NFT_LOG(L_ERROR, "Error while saving current setup.");
                return;
        }
}


/** menuitem "save as" selected */
G_MODULE_EXPORT void on_setup_menuitem_save_as_activate(
        GtkMenuItem * i,
        gpointer d)
{
        gtk_widget_show(GTK_WIDGET(UI("filechooserdialog_save")));
}


/** "save" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_save_save_clicked(
        GtkButton * b,
        gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (UI
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

        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_save")));

osssc_exit:
        g_free(filename);
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_save_cancel_clicked(
        GtkButton * b,
        gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_save")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_open_cancel_clicked(
        GtkButton * b,
        gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_load")));
}

/** "open" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_open_clicked(
        GtkButton * b,
        gpointer u)
{
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (UI
                                                       ("filechooserdialog_load")))))
        {
                ui_log_alert_show("No filename given?");
                return;
        }

        if(!ui_setup_load(filename))
        {
                ui_log_alert_show("Error while loading file \"%s\"",
                                  filename);
                goto osoc_exit;
        }

        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_load")));

osoc_exit:
        g_free(filename);
}

/** add hardware "add" clicked */
G_MODULE_EXPORT void on_add_hardware_add_clicked(
        GtkButton * b,
        gpointer u)
{
        /* add new hardware */
        NiftyconfHardware *h;
        if(!(h = hardware_new(gtk_entry_get_text
                              (GTK_ENTRY(UI("hardware_add_name_entry"))),
                              gtk_combo_box_get_active_text(GTK_COMBO_BOX
                                                            (UI
                                                             ("hardware_add_plugin_combobox"))),
                              gtk_entry_get_text(GTK_ENTRY
                                                 (UI
                                                  ("hardware_add_id_entry"))),
                              gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                               (UI
                                                                ("hardware_add_ledcount_spinbutton"))),
                              gtk_combo_box_get_active_text(GTK_COMBO_BOX
                                                            (UI
                                                             ("hardware_add_pixelformat_comboboxtext"))))))
                return;

        /* hide window */
        gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), FALSE);

                /** @todo refresh our menu */

        /* refresh tree */
        ui_setup_tree_refresh();

}

/** add hardware "cancel" clicked */
G_MODULE_EXPORT void on_add_hardware_cancel_clicked(
        GtkButton * b,
        gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), FALSE);
}


/** add hardware window close */
G_MODULE_EXPORT gboolean on_add_hardware_window_delete_event(
        GtkWidget * w,
        GdkEvent * e)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), FALSE);
        return TRUE;
}


/** add hardware "pixelformat" changed */
G_MODULE_EXPORT void on_hardware_add_pixelformat_comboboxtext_changed(
        GtkComboBox * w,
        gpointer u)
{
        LedPixelFormat *f;
        if(!(f = led_pixel_format_from_string(gtk_combo_box_get_active_text
                                              (w))))
        {
                /* invalid pixel format? */
                gtk_widget_set_sensitive(GTK_WIDGET
                                         (UI
                                          ("hardware_add_ledcount_spinbutton")),
                                         false);
                return;
        }

        gtk_widget_set_sensitive(GTK_WIDGET
                                 (UI("hardware_add_ledcount_spinbutton")),
                                 true);

        /* set minimum for "ledcount" spinbutton according to format */
        LedCount minimum = led_pixel_format_get_n_components(f);
        gtk_adjustment_set_lower(GTK_ADJUSTMENT(UI("ledcount_adjustment")),
                                 minimum);
        if((LedCount)
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                            (UI
                                             ("hardware_add_ledcount_spinbutton")))
           < minimum)
        {
                gtk_spin_button_set_value(GTK_SPIN_BUTTON
                                          (UI
                                           ("hardware_add_ledcount_spinbutton")),
                                          (gdouble) minimum);
        }
}


/** add chain "add" clicked */
G_MODULE_EXPORT void on_add_chain_add_clicked(
        GtkButton * b,
        gpointer u)
{

        /* get parent element */
        NIFTYLED_TYPE t;
        gpointer element;
        ui_setup_tree_get_first_selected_element(&t, &element);

        /* add new chain */
        if(!chain_of_tile_new(t, element,
                              gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                               (UI
                                                                ("chain_add_ledcount_spinbutton"))),
                              gtk_combo_box_get_active_text(GTK_COMBO_BOX
                                                            (UI
                                                             ("chain_add_pixelformat_comboboxtext")))))
                return;

        /* hide dialog */
        gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), FALSE);

        /* refresh tree */
        ui_setup_tree_refresh();

}


/** add chain "cancel" clicked */
G_MODULE_EXPORT void on_add_chain_cancel_clicked(
        GtkButton * b,
        gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), FALSE);
}


/** add chain window close */
G_MODULE_EXPORT gboolean on_add_chain_window_delete_event(
        GtkWidget * w,
        GdkEvent * e)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), FALSE);
        return TRUE;
}


/** add chain "pixelformat" changed */
G_MODULE_EXPORT void on_chain_add_pixelformat_comboboxtext_changed(
        GtkComboBox * w,
        gpointer u)
{
        LedPixelFormat *f;
        if(!(f = led_pixel_format_from_string(gtk_combo_box_get_active_text
                                              (w))))
        {
                /* invalid pixel format? */
                gtk_widget_set_sensitive(GTK_WIDGET
                                         (UI
                                          ("chain_add_ledcount_spinbutton")),
                                         false);
                return;
        }

        gtk_widget_set_sensitive(GTK_WIDGET
                                 (UI("chain_add_ledcount_spinbutton")), true);

        /* set minimum for "ledcount" spinbutton according to format */
        size_t minimum = led_pixel_format_get_n_components(f);
        gtk_adjustment_set_lower(GTK_ADJUSTMENT(UI("ledcount_adjustment")),
                                 (gdouble) minimum);
        if((size_t)
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                            (UI
                                             ("chain_add_ledcount_spinbutton")))
           < minimum)
        {
                gtk_spin_button_set_value(GTK_SPIN_BUTTON
                                          (UI
                                           ("chain_add_ledcount_spinbutton")),
                                          (gdouble) minimum);
        }
}


/** "export" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_export_clicked(
        GtkButton * b,
        gpointer u)
{
        /* filename for export file */
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (UI
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

        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_export")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_export_cancel_clicked(
        GtkButton * b,
        gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_export")));
}


/** "cancel" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_import_cancel_clicked(
        GtkButton * b,
        gpointer u)
{
        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_import")));
}


/** "import" button in filechooser clicked */
G_MODULE_EXPORT void on_setup_import_clicked(
        GtkButton * b,
        gpointer u)
{
        /* filename for export file */
        char *filename;
        if(!(filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER
                                                      (UI
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

        gtk_widget_hide(GTK_WIDGET(UI("filechooserdialog_import")));
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_add_hardware_activate(
        GtkWidget * i,
        gpointer u)
{
        /* rebuild plugin combobox */
        // gtk_combo_box_
        /* show "add hardware" dialog */
        gtk_widget_set_visible(GTK_WIDGET(ui_setup("hardware_add_window")),
                               TRUE);
}

/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_add_tile_activate(
        GtkWidget * i,
        gpointer u)
{
        NIFTYLED_TYPE t;
        gpointer e;
        ui_setup_tree_get_last_selected_element(&t, &e);


        /* different possible element types */
        switch (t)
        {
                        /* currently selected element is a hardware-node */
                case LED_HARDWARE_T:
                {
                        tile_of_hardware_new((NiftyconfHardware *) e);
                        break;
                }

                        /* currently selected element is a tile-node */
                case LED_TILE_T:
                {
                        tile_of_tile_new((NiftyconfTile *) e);
                        break;
                }

                default:
                {
                        break;
                }
        }

        /** @todo refresh our menu */

        /* refresh tree */
        ui_setup_tree_refresh();
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_add_chain_activate(
        GtkWidget * i,
        gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(ui_setup("chain_add_window")),
                               TRUE);
}


/** wrapper for do_* functions */
static void _foreach_remove_hardware(
        NIFTYLED_TYPE t,
        gpointer e)
{
        if(t != LED_HARDWARE_T)
                return;

        hardware_destroy((NiftyconfHardware *) e);
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_remove_hardware_activate(
        GtkWidget * i,
        gpointer u)
{
        /* remove all currently selected elements */
        ui_setup_tree_do_foreach_selected_element(_foreach_remove_hardware);

        /* refresh tree */
        ui_setup_tree_refresh();

        /* hide properties */
        ui_setup_props_hide();
}


/** wrapper for do_* functions */
static void _foreach_remove_tile(
        NIFTYLED_TYPE t,
        gpointer e)
{
        if(t != LED_TILE_T)
                return;

        tile_destroy((NiftyconfTile *) e);
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_remove_tile_activate(
        GtkWidget * i,
        gpointer u)
{
        /* remove all currently selected elements */
        ui_setup_tree_do_foreach_selected_element(_foreach_remove_tile);

        /* refresh tree */
        ui_setup_tree_refresh();

        /* hide properties */
        ui_setup_props_hide();
}


/** wrapper for do_* functions */
static void _foreach_remove_chain(
        NIFTYLED_TYPE type,
        gpointer e)
{
        /* works only if tile-element is selected */
        if(type != LED_TILE_T)
                return;

        chain_of_tile_destroy((NiftyconfTile *) e);
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_remove_chain_activate(
        GtkWidget * i,
        gpointer u)
{
        /* remove all currently selected elements */
        ui_setup_tree_do_foreach_selected_element(_foreach_remove_chain);

        /* refresh tree */
        ui_setup_tree_refresh();

        /* hide properties */
        ui_setup_props_hide();
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_import_activate(
        GtkWidget * i,
        gpointer u)
{
        gtk_widget_show(GTK_WIDGET(ui_setup("filechooserdialog_import")));
}


/** menu-entry selected */
G_MODULE_EXPORT void on_niftyconf_menu_export_activate(
        GtkWidget * i,
        gpointer u)
{
        gtk_widget_show(GTK_WIDGET(ui_setup("filechooserdialog_export")));
}


/** live preview toggled */
void on_live_preview_toggled(
        GtkCheckMenuItem * item,
        gpointer user_data)
{
        live_preview_set_enabled(gtk_check_menu_item_get_active(item));
}
