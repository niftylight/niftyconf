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

#include <gtk/gtk.h>
#include "ui/ui.h"
#include "elements/element-hardware.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-props.h"



/** GtkBuilder for this module */
static GtkBuilder *_ui;






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/** refresh info view */
void ui_info_hardware_refresh(NiftyconfHardware * hardware)
{
        LedHardware *h = hardware_niftyled(hardware);

        gtk_link_button_set_uri(GTK_LINK_BUTTON(UI("linkbutton_family")),
                                led_hardware_plugin_get_url(h));
        gtk_button_set_label(GTK_BUTTON(UI("linkbutton_family")),
                             led_hardware_plugin_get_family(h));
        gtk_label_set_text(GTK_LABEL(UI("label_description")),
                           led_hardware_plugin_get_description(h));
        gtk_label_set_text(GTK_LABEL(UI("label_author")),
                           led_hardware_plugin_get_author(h));
        gtk_label_set_text(GTK_LABEL(UI("label_license")),
                           led_hardware_plugin_get_license(h));
        gchar version[64];
        g_snprintf(version, sizeof(version), "v%d.%d.%d",
                   led_hardware_plugin_get_version_major(h),
                   led_hardware_plugin_get_version_minor(h),
                   led_hardware_plugin_get_version_micro(h));
        gtk_label_set_text(GTK_LABEL(UI("label_version")), version);
}


/** show/hide window */
void ui_info_hardware_set_visible(gboolean visible)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("window")), visible);
        gtk_widget_show(GTK_WIDGET(UI("window")));
}


/** initialize setup tree module */
gboolean ui_hardware_init()
{
        if(!(_ui = ui_builder("niftyconf-info-hardware.ui")))
                return false;

        return true;
}


/** deinitialize this module */
void ui_hardware_deinit()
{
        g_object_unref(_ui);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** hardware add */
G_MODULE_EXPORT void on_action_hardware_add_activate(GtkAction * a, gpointer u)
{
        /* show "add hardware" dialog */
        gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")),
                               true);
}


/** hardware info */
G_MODULE_EXPORT void on_action_hardware_info_activate(GtkAction * w, gpointer u)
{
        ui_info_hardware_set_visible(true);
}


/** foreach helper */
static void _foreach_remove_hardware(NIFTYLED_TYPE t, gpointer e)
{
        if(t != LED_HARDWARE_T)
                return;

        hardware_destroy((NiftyconfHardware *) e);
}


/** hardware remove */
G_MODULE_EXPORT void on_action_hardware_remove_activate(GtkAction * a, gpointer u)
{
        /* remove all currently selected elements */
        ui_setup_tree_do_foreach_selected_element(_foreach_remove_hardware);

        /* refresh tree */
        ui_setup_tree_refresh();

        /* hide properties */
        ui_setup_props_hide();
}


/** add hardware "add" clicked */
G_MODULE_EXPORT void on_add_hardware_add_clicked(GtkButton * b, gpointer u)
{
        /* add new hardware */
        if(!hardware_new(gtk_entry_get_text
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
                                                        ("hardware_add_pixelformat_comboboxtext")))))
                return;

        /* hide window */
        gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), false);

                /** @todo refresh our menu */

        /* refresh tree */
        ui_setup_tree_refresh();

}


/** add hardware "cancel" clicked */
G_MODULE_EXPORT void on_add_hardware_cancel_clicked(GtkButton * b, gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("hardware_add_window")), false);
}


/** add hardware "pixelformat" changed */
G_MODULE_EXPORT void
on_hardware_add_pixelformat_comboboxtext_changed(GtkComboBox * w, gpointer u)
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


