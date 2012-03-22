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

#include <math.h>
#include <gtk/gtk.h>
#include "niftyconf-ui.h"
#include "niftyconf-setup.h"
#include "niftyconf-setup-tree.h"
#include "niftyconf-setup-props.h"




/** GtkBuilder for this module */
static GtkBuilder *_ui;


/* currently shown elements */
static NiftyconfHardware *current_hw;
static NiftyconfTile *current_tile;
static NiftyconfChain *current_chain;
static NiftyconfLed *current_led;


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


static void _widget_set_error_background(GtkWidget *w, gboolean error)
{
        if(error)
        {
                GdkColor color;
                gdk_color_parse("#f96b5f", &color);
                gtk_widget_modify_base(w, GTK_STATE_NORMAL, &color);
        }
        else
        {
                gtk_widget_modify_base(w, GTK_STATE_NORMAL, NULL);
        }
}

/******************************************************************************
 ******************************************************************************/

/**
 * getter for widget
 */
GtkWidget *setup_props_get_widget()
{
        return GTK_WIDGET(UI("box_props"));
}


/** show hardware props */
void setup_props_hardware_show(NiftyconfHardware *h)
{
        current_hw = h;
        
        if(h)
        {
                LedHardware *hardware = hardware_niftyled(h);
                gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_name")), led_hardware_get_name(hardware));
                gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_plugin")), led_hardware_get_plugin_family(hardware));
                gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_id")), led_hardware_get_id(hardware));
                gtk_widget_set_tooltip_text(GTK_WIDGET(UI("entry_hw_id")), led_hardware_get_plugin_id_example(hardware));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_hw_stride")), (gdouble) led_hardware_get_stride(hardware));
        }
        
        gtk_widget_show(GTK_WIDGET(UI("frame_hardware")));
}


/** show tile props */
void setup_props_tile_show(NiftyconfTile *t)
{
        current_tile = t;
        
        if(t)
        {
                LedTile *tile = tile_niftyled(t);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_x")), (gdouble) led_tile_get_x(tile));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_y")), (gdouble) led_tile_get_y(tile));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_width")), (gdouble) led_tile_get_transformed_width(tile));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_height")), (gdouble) led_tile_get_transformed_height(tile));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_rotation")), (gdouble) led_tile_get_rotation(tile)*180/M_PI);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_pivot_x")), (gdouble) led_tile_get_pivot_x(tile));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_tile_pivot_y")), (gdouble) led_tile_get_pivot_y(tile));
        }
        
        gtk_widget_show(GTK_WIDGET(UI("frame_tile")));
}


/** show chain props */
void setup_props_chain_show(NiftyconfChain *c)
{
        current_chain = c;
        
        if(c)
        {
                LedChain *chain = chain_niftyled(c);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_chain_ledcount")), (gdouble) led_chain_get_ledcount(chain));
                gtk_entry_set_text(GTK_ENTRY(UI("entry_chain_format")), led_pixel_format_to_string(led_chain_get_format(chain)));
        }
        
        gtk_widget_show(GTK_WIDGET(UI("frame_chain")));
}


/** show led props */
void setup_props_led_show(NiftyconfLed *l)
{
        current_led = l;
        
        if(l)
        {
                Led *led = led_niftyled(l);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_led_x")), (gdouble) led_get_x(led));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_led_y")), (gdouble) led_get_y(led));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_led_component")), (gdouble) led_get_component(led));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_led_gain")), (gdouble) led_get_gain(led)); 
        }
        
        gtk_widget_show(GTK_WIDGET(UI("frame_led")));
}


/** hide all props */
void setup_props_hide()
{
        gtk_widget_hide(GTK_WIDGET(UI("frame_hardware")));
        gtk_widget_hide(GTK_WIDGET(UI("frame_tile")));
        gtk_widget_hide(GTK_WIDGET(UI("frame_chain")));
        gtk_widget_hide(GTK_WIDGET(UI("frame_led")));
}


/** initialize led module */
gboolean setup_props_init()
{
        _ui = ui_builder("niftyconf-setup-props.ui");
                
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** spinbutton value changed */
void on_spinbutton_led_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

        if(!led_set_x(l, (LedFrameCord) gtk_spin_button_get_value_as_int(s)))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
}


/** spinbutton value changed */
void on_spinbutton_led_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

        if(!led_set_y(l, (LedFrameCord) gtk_spin_button_get_value_as_int(s)))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
}


/** spinbutton value changed */
void on_spinbutton_led_component_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);
        
        if(!led_set_component(l, (LedFrameComponent) gtk_spin_button_get_value_as_int(s)))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
}


/** spinbutton value changed */
void on_spinbutton_led_gain_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

        if(!led_set_gain(l, (LedGain) gtk_spin_button_get_value_as_int(s)))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
}


/** spinbutton value changed */
void on_spinbutton_chain_ledcount_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedChain *chain = chain_niftyled(current_chain);

        if(!led_chain_set_ledcount(chain,
                                (LedCount) gtk_spin_button_get_value_as_int(s)))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }
}


/** spinbutton value changed */
void on_spinbutton_tile_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        if(!led_tile_set_x(tile, (LedFrameCord) gtk_spin_button_get_value_as_int(s)))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_tile_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        if(!led_tile_set_y(tile, (LedFrameCord) gtk_spin_button_get_value_as_int(s)))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }
        

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_tile_rotation_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        if(!led_tile_set_rotation(tile, (double) gtk_spin_button_get_value(s)*M_PI/180))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }

        /* refresh view */
        setup_props_tile_show(current_tile);
}


/** spinbutton value changed */
void on_spinbutton_tile_pivot_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        if(!led_tile_set_pivot_x(tile, (double) gtk_spin_button_get_value(s)))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }
        

        /* refresh view */
        setup_props_tile_show(current_tile);
}


/** spinbutton value changed */
void on_spinbutton_tile_pivot_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        if(!led_tile_set_pivot_y(tile, (double) gtk_spin_button_get_value(s)))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }

        /* refresh view */
        setup_props_tile_show(current_tile);
}


/** entry text changed */
void on_entry_hardware_name_changed(GtkEditable *e, gpointer u)
{
        /* get currently selected hardware */
        LedHardware *h = hardware_niftyled(current_hw);

        /* set value */
        if(!led_hardware_set_name(h, gtk_entry_get_text(GTK_ENTRY(e))))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(e), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(e), FALSE);
        }

        /* refresh view */
        setup_tree_refresh();
        
}


/** entry text changed */
void on_entry_hardware_id_changed(GtkEditable *e, gpointer u)
{
        /* get currently selected hardware */
        LedHardware *h = hardware_niftyled(current_hw);

        /* set value */
        if(!led_hardware_set_id(h, gtk_entry_get_text(GTK_ENTRY(e))))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(e), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(e), FALSE);
        }

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_hardware_stride_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected hardware */
        LedHardware *h = hardware_niftyled(current_hw);

        /* set value */
        if(!led_hardware_set_stride(h, (LedCount) gtk_spin_button_get_value_as_int(s)))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }
        
        
        /* refresh view */

        
}
