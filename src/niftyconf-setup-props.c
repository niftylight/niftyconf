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



/* currently shown elements */
static NiftyconfHardware *current_hw;
static NiftyconfTile *current_tile;
static NiftyconfChain *current_chain;
static NiftyconfLed *current_led;

/* widgets */
static GtkBox   *       box_props;
static GtkFrame *       frame_hardware;
static GtkFrame *       frame_tile;
static GtkFrame *       frame_chain;
static GtkFrame *       frame_led;
static GtkEntry *       entry_hw_name;
static GtkEntry *       entry_hw_plugin;
static GtkEntry *       entry_hw_id;
static GtkSpinButton *  spinbutton_hw_stride;
static GtkSpinButton *  spinbutton_tile_x;
static GtkSpinButton *  spinbutton_tile_y;
static GtkSpinButton *  spinbutton_tile_width;
static GtkSpinButton *  spinbutton_tile_height;
static GtkSpinButton *  spinbutton_tile_rotation;
static GtkSpinButton *  spinbutton_tile_pivot_x;
static GtkSpinButton *  spinbutton_tile_pivot_y;
static GtkSpinButton *  spinbutton_led_x;
static GtkSpinButton *  spinbutton_led_y;
static GtkSpinButton *  spinbutton_led_component;
static GtkSpinButton *  spinbutton_led_gain;
static GtkSpinButton *  spinbutton_chain_ledcount;
static GtkEntry *       entry_chain_format;

/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/**
 * getter for widget
 */
GtkWidget *setup_props_get_widget()
{
        return GTK_WIDGET(box_props);
}


/** show hardware props */
void setup_props_hardware_show(NiftyconfHardware *h)
{
        current_hw = h;
        
        if(h)
        {
                LedHardware *hardware = hardware_niftyled(h);
                gtk_entry_set_text(entry_hw_name, led_hardware_get_name(hardware));
                gtk_entry_set_text(entry_hw_plugin, led_hardware_get_plugin_family(hardware));
                gtk_entry_set_text(entry_hw_id, led_hardware_get_id(hardware));
                gtk_spin_button_set_value(spinbutton_hw_stride, (gdouble) led_hardware_get_stride(hardware));
        }
        
        gtk_widget_show(GTK_WIDGET(frame_hardware));
}


/** show tile props */
void setup_props_tile_show(NiftyconfTile *t)
{
        current_tile = t;
        
        if(t)
        {
                LedTile *tile = tile_niftyled(t);
                gtk_spin_button_set_value(spinbutton_tile_x, (gdouble) led_tile_get_x(tile));
                gtk_spin_button_set_value(spinbutton_tile_y, (gdouble) led_tile_get_y(tile));
                gtk_spin_button_set_value(spinbutton_tile_width, (gdouble) led_tile_get_width(tile));
                gtk_spin_button_set_value(spinbutton_tile_height, (gdouble) led_tile_get_height(tile));
                gtk_spin_button_set_value(spinbutton_tile_rotation, (gdouble) led_tile_get_rotation(tile)*180/M_PI);
                gtk_spin_button_set_value(spinbutton_tile_pivot_x, (gdouble) led_tile_get_pivot_x(tile));
                gtk_spin_button_set_value(spinbutton_tile_pivot_y, (gdouble) led_tile_get_pivot_y(tile));
        }
        
        gtk_widget_show(GTK_WIDGET(frame_tile));
}


/** show chain props */
void setup_props_chain_show(NiftyconfChain *c)
{
        current_chain = c;
        
        if(c)
        {
                LedChain *chain = chain_niftyled(c);
                gtk_spin_button_set_value(spinbutton_chain_ledcount, (gdouble) led_chain_get_ledcount(chain));
                gtk_entry_set_text(entry_chain_format, led_pixel_format_to_string(led_chain_get_format(chain)));
        }
        
        gtk_widget_show(GTK_WIDGET(frame_chain));
}


/** show led props */
void setup_props_led_show(NiftyconfLed *l)
{
        current_led = l;
        
        if(l)
        {
                Led *led = led_niftyled(l);
                gtk_spin_button_set_value(spinbutton_led_x, (gdouble) led_get_x(led));
                gtk_spin_button_set_value(spinbutton_led_y, (gdouble) led_get_y(led));
                gtk_spin_button_set_value(spinbutton_led_component, (gdouble) led_get_component(led));
                gtk_spin_button_set_value(spinbutton_led_gain, (gdouble) led_get_gain(led)); 
        }
        
        gtk_widget_show(GTK_WIDGET(frame_led));
}


/** hide all props */
void setup_props_hide()
{
        gtk_widget_hide(GTK_WIDGET(frame_hardware));
        gtk_widget_hide(GTK_WIDGET(frame_tile));
        gtk_widget_hide(GTK_WIDGET(frame_chain));
        gtk_widget_hide(GTK_WIDGET(frame_led));
}


/**
 * initialize led module
 */
gboolean setup_props_init()
{
        GtkBuilder *ui = ui_builder("niftyconf-setup-props.ui");

        /* get widgets */
        if(!(box_props = GTK_BOX(gtk_builder_get_object(ui, "box_props"))))
                return FALSE;
        if(!(frame_hardware = GTK_FRAME(gtk_builder_get_object(ui, "frame_hardware"))))
                return FALSE;
        if(!(frame_tile = GTK_FRAME(gtk_builder_get_object(ui, "frame_tile"))))
                return FALSE;
        if(!(frame_chain = GTK_FRAME(gtk_builder_get_object(ui, "frame_chain"))))
                return FALSE;
        if(!(frame_led = GTK_FRAME(gtk_builder_get_object(ui, "frame_led"))))
                return FALSE;
        if(!(entry_hw_name = GTK_ENTRY(gtk_builder_get_object(ui, "entry_hw_name"))))
                return FALSE;
        if(!(entry_hw_plugin = GTK_ENTRY(gtk_builder_get_object(ui, "entry_hw_plugin"))))
                return FALSE;
        if(!(entry_hw_id = GTK_ENTRY(gtk_builder_get_object(ui, "entry_hw_id"))))
                return FALSE;
        if(!(spinbutton_hw_stride = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_hw_stride"))))
                return FALSE;
        
        if(!(spinbutton_tile_x = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_x"))))
                return FALSE;
        if(!(spinbutton_tile_y = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_y"))))
                return FALSE;
        if(!(spinbutton_tile_width = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_width"))))
                return FALSE;
        if(!(spinbutton_tile_height = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_height"))))
                return FALSE;
        if(!(spinbutton_tile_rotation = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_rotation"))))
                return FALSE;
        if(!(spinbutton_tile_pivot_x = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_pivot_x"))))
                return FALSE;
        if(!(spinbutton_tile_pivot_y = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_tile_pivot_y"))))
                return FALSE;

        if(!(spinbutton_led_x = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_led_x"))))
                return FALSE;
        if(!(spinbutton_led_y = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_led_y"))))
                return FALSE;
        if(!(spinbutton_led_component = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_led_component"))))
                return FALSE;
        if(!(spinbutton_led_gain = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_led_gain"))))
                return FALSE;
        
        if(!(spinbutton_chain_ledcount = GTK_SPIN_BUTTON(gtk_builder_get_object(ui, "spinbutton_chain_ledcount"))))
                return FALSE;
        if(!(entry_chain_format = GTK_ENTRY(gtk_builder_get_object(ui, "entry_chain_format"))))
                return FALSE;
                
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** spinbutton value changed */
void on_spinbutton_led_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */

        /* set new value */
        //led_set_x(led, (LedFrameCord) gtk_spin_button_get_value_as_int(s));
        
        /* refresh */
}


/** spinbutton value changed */
void on_spinbutton_led_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */

        /* set new value */
        //led_set_y(led, (LedFrameCord) gtk_spin_button_get_value_as_int(s));
        
        /* refresh */
}


/** spinbutton value changed */
void on_spinbutton_led_component_changed(GtkSpinButton *s, gpointer u)
{

}


/** spinbutton value changed */
void on_spinbutton_led_gain_changed(GtkSpinButton *s, gpointer u)
{

}


/** spinbutton value changed */
void on_spinbutton_chain_ledcount_changed(GtkSpinButton *s, gpointer u)
{

}


/** entry text changed */
void on_entry_chain_format_changed(GtkEditable *e, gpointer u)
{

}


/** spinbutton value changed */
void on_spinbutton_tile_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        led_tile_set_x(tile, (LedFrameCord) gtk_spin_button_get_value_as_int(s));

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_tile_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        led_tile_set_y(tile, (LedFrameCord) gtk_spin_button_get_value_as_int(s));

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_tile_rotation_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        led_tile_set_rotation(tile, (double) gtk_spin_button_get_value(s)*M_PI/180);

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_tile_pivot_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        led_tile_set_pivot_x(tile, (double) gtk_spin_button_get_value(s));

        /* refresh view */

}


/** spinbutton value changed */
void on_spinbutton_tile_pivot_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);
        
        /* set new value */
        led_tile_set_pivot_y(tile, (double) gtk_spin_button_get_value(s));

        /* refresh view */

}


/** entry text changed */
void on_entry_hardware_name_changed(GtkEditable *e, gpointer u)
{
        /* get currently selected hardware */
        LedHardware *h = hardware_niftyled(current_hw);

        /* set value */
        led_hardware_set_name(h, gtk_entry_get_text(GTK_ENTRY(e)));

        /* refresh view */
        setup_tree_refresh();
        
}


/** entry text changed */
void on_entry_hardware_plugin_changed(GtkEditable *e, gpointer u)
{

}


/** entry text changed */
void on_entry_hardware_id_changed(GtkEditable *e, gpointer u)
{

}


/** spinbutton value changed */
void on_spinbutton_hardware_stride_changed(GtkSpinButton *s, gpointer u)
{

}
