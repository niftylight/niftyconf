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

#include <gtk/gtk.h>
#include "niftyconf-ui.h"
#include "niftyconf-hardware.h"




/** GtkBuilder for this module */
static GtkBuilder *_ui;






/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/** set info */
void info_hardware_set(NiftyconfHardware *hardware)
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
void info_hardware_set_visible(gboolean visible)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("window")), visible);
        gtk_widget_show(GTK_WIDGET(UI("window")));
}


/** initialize setup tree module */
gboolean  info_hardware_init()
{
        if(!(_ui = ui_builder("niftyconf-info-hardware.ui")))
                return FALSE;
        
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** close main window */
gboolean on_info_hardware_window_delete_event(GtkWidget *w, GdkEvent *e)
{
        gtk_widget_set_visible(w, FALSE);
        return TRUE;
}
