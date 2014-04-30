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
//#include "ui/ui.h"
#include "ui/ui-setup.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-props.h"


#define UI(name) (gtk_builder_get_object(_get_builder(), name))


/** GtkBuilder for this module */
static GtkBuilder *_builder;


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/
static GtkBuilder *_get_builder()
{
        return GTK_BUILDER(_builder);
}



/******************************************************************************
 ****************************** PUBLIC FUNCTIONS ****************************** 
 ******************************************************************************/




/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** chain add */
G_MODULE_EXPORT void on_action_chain_add_activate(GtkAction * a, gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(ui_setup("chain_add_window")),
                               true);
}


/** foreach helper */
static void _foreach_remove_chain(NIFTYLED_TYPE type, gpointer e)
{
        /* works only if tile-element is selected */
        if(type != LED_TILE_T)
                return;

        chain_of_tile_destroy((NiftyconfTile *) e);
}


/** chain remove */
G_MODULE_EXPORT void on_action_chain_remove_activate(GtkWidget * a, gpointer u)
{
        /* remove all currently selected elements */
        ui_setup_tree_do_foreach_selected_element(_foreach_remove_chain);

        /* refresh tree */
        ui_setup_tree_refresh();

        /* hide properties */
        ui_setup_props_hide();
}


/** add chain "add" clicked */
G_MODULE_EXPORT void on_add_chain_add_clicked(GtkButton * b, gpointer u)
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
        gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), false);

        /* refresh tree */
        ui_setup_tree_refresh();

}


/** add chain "cancel" clicked */
G_MODULE_EXPORT void on_add_chain_cancel_clicked(GtkButton * b, gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("chain_add_window")), false);
}


/** add chain "pixelformat" changed */
G_MODULE_EXPORT void on_chain_add_pixelformat_comboboxtext_changed(GtkComboBox
                                                                   * w,
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

