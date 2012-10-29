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
#include "ui/ui.h"
#include "ui/ui-log.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-ledlist.h"
#include "elements/element-setup.h"
#include "renderer/renderer-setup.h"



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
GtkWidget *ui_setup_props_get_widget()
{
        return GTK_WIDGET(UI("box_props"));
}


/** show hardware props */
void ui_setup_props_hardware_show(NiftyconfHardware *h)
{
        current_hw = h;

        if(h)
        {
                LedHardware *hardware = hardware_niftyled(h);
                gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_name")), led_hardware_get_name(hardware));
                gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_plugin")), led_hardware_plugin_get_family(hardware));
                gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_id")), led_hardware_get_id(hardware));
                gtk_widget_set_tooltip_text(GTK_WIDGET(UI("entry_hw_id")), led_hardware_plugin_get_id_example(hardware));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_hw_stride")), (gdouble) led_hardware_get_stride(hardware));
		/* set button to right toggled state */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(UI("togglebutton_initialized")), led_hardware_is_initialized(hardware));
        }

        gtk_widget_show(GTK_WIDGET(UI("frame_hardware")));
}


/** show tile props */
void ui_setup_props_tile_show(NiftyconfTile *t)
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
void ui_setup_props_chain_show(NiftyconfChain *c)
{
        current_chain = c;

        if(c)
        {
                LedChain *chain = chain_niftyled(c);
                gtk_entry_set_text(GTK_ENTRY(UI("entry_chain_format")), led_pixel_format_to_string(led_chain_get_format(chain)));
		/* set minimum for "ledcount" spinbutton according to format */
		size_t minimum = led_pixel_format_get_n_components(led_chain_get_format(chain));
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(UI("adjustment_chain_ledcount")), (gdouble) minimum);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(UI("spinbutton_chain_ledcount")), (gdouble) led_chain_get_ledcount(chain));
        }

        gtk_widget_show(GTK_WIDGET(UI("frame_chain")));
}


/** show led props */
void ui_setup_props_led_show(NiftyconfLed *l)
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
void ui_setup_props_hide()
{
        gtk_widget_hide(GTK_WIDGET(UI("frame_hardware")));
        gtk_widget_hide(GTK_WIDGET(UI("frame_tile")));
        gtk_widget_hide(GTK_WIDGET(UI("frame_chain")));
        gtk_widget_hide(GTK_WIDGET(UI("frame_led")));
}


/** initialize led module */
gboolean ui_setup_props_init()
{
        _ui = ui_builder("niftyconf-setup-props.ui");

        return TRUE;
}


/** deinitialize this module */
void ui_setup_props_deinit()
{
	g_object_unref(_ui);
}

/** show "hardware initialized" image */
void ui_setup_props_hardware_initialized_image(gboolean is_initialized)
{
	/* show correct image */
	gtk_widget_set_visible(GTK_WIDGET(UI("image_initialized")), is_initialized);
	gtk_widget_set_visible(GTK_WIDGET(UI("image_deinitialized")), !is_initialized);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_led_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

	/* value really changed? */
	int new_val = gtk_spin_button_get_value_as_int(s);
	if(led_get_x(l) == new_val)
		return;

	/* set new value */
        if(!led_set_x(l, new_val))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_led_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

	/* value really changed? */
	int new_val = gtk_spin_button_get_value_as_int(s);
	if(led_get_y(l) == new_val)
		return;

        if(!led_set_y(l, new_val))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_led_component_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

	/* value really changed? */
	int new_val = gtk_spin_button_get_value_as_int(s);
	if(led_get_component(l) == new_val)
		return;

        if(!led_set_component(l, new_val))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_led_gain_changed(GtkSpinButton *s, gpointer u)
{
        /* get currently selected LED */
        Led *l = led_niftyled(current_led);

	/* value really changed? */
	int new_val = gtk_spin_button_get_value_as_int(s);
	if(led_get_gain(l) == new_val)
		return;

        if(!led_set_gain(l, new_val))
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        else
                _widget_set_error_background(GTK_WIDGET(s), FALSE);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_chain_ledcount_changed(GtkSpinButton *s, gpointer u)
{
        /* get current chain */
        LedChain *chain = chain_niftyled(current_chain);

        /* get new ledcount */
        int ledcount = gtk_spin_button_get_value_as_int(s);

        /* ledcount really changed? */
        if(led_chain_get_ledcount(chain) == ledcount)
                return;

        /* remove all current LEDs */
        ui_setup_ledlist_clear();

        /* unregister all LEDs from chain */
        chain_unregister_leds_from_gui(current_chain);

	/* is this the chain of a hardware element? */
	LedHardware *h;
	if((h = led_chain_get_parent_hardware(chain)))
	{
		/* set new ledcount */
		if(!led_hardware_set_ledcount(h, ledcount))
		/* error background color */
		{
			_widget_set_error_background(GTK_WIDGET(s), TRUE);
			return;
		}
		/* normal background color */
		else
		{
			_widget_set_error_background(GTK_WIDGET(s), FALSE);
		}
	}
	/* this is an ordinary chain of a tile element */
	{
		/* set new ledcount */
		if(!led_chain_set_ledcount(chain, ledcount))
		/* error background color */
		{
			_widget_set_error_background(GTK_WIDGET(s), TRUE);
			return;
		}
		/* normal background color */
		else
		{
			_widget_set_error_background(GTK_WIDGET(s), FALSE);
		}
	}

        /* re-register (less or more) LEDs in chain */
        chain_register_leds_to_gui(current_chain);

        /* refresh view */
        ui_setup_ledlist_refresh(current_chain);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_tile_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);

	/* value really changed? */
	int new_val = gtk_spin_button_get_value_as_int(s);
	if(led_tile_get_x(tile) == new_val)
		return;

        /* set new value */
        if(!led_tile_set_x(tile, new_val))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }

        /* redraw */
	renderer_setup_redraw();

}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_tile_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);

	/* value really changed? */
	int new_val = gtk_spin_button_get_value_as_int(s);
	if(led_tile_get_y(tile) == new_val)
		return;

        /* set new value */
        if(!led_tile_set_y(tile, new_val))
        /* error background color */
        {
                _widget_set_error_background(GTK_WIDGET(s), TRUE);
        }
        /* normal background color */
        else
        {
                _widget_set_error_background(GTK_WIDGET(s), FALSE);
        }

	/* redraw */
	renderer_setup_redraw();

}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_tile_rotation_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);

	/* value really changed? */
	double new_val = (gtk_spin_button_get_value(s)*M_PI)/180;
	if(led_tile_get_rotation(tile) == new_val)
		return;

        /* set new value */
        if(!led_tile_set_rotation(tile, new_val))
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
        ui_setup_props_tile_show(current_tile);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_tile_pivot_x_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);

	/* value really changed? */
	double new_val = gtk_spin_button_get_value_as_int(s);
	if(led_tile_get_pivot_x(tile) == new_val)
		return;

        /* set new value */
        if(!led_tile_set_pivot_x(tile, new_val))
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
        ui_setup_props_tile_show(current_tile);

	/* redraw */
	renderer_setup_redraw();
}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_tile_pivot_y_changed(GtkSpinButton *s, gpointer u)
{
        /* get current tile */
        LedTile *tile = tile_niftyled(current_tile);

	/* value really changed? */
	double new_val = gtk_spin_button_get_value_as_int(s);
	if(led_tile_get_pivot_y(tile) == new_val)
		return;

        /* set new value */
        if(!led_tile_set_pivot_y(tile, new_val))
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
        ui_setup_props_tile_show(current_tile);

	/* redraw */
	renderer_setup_redraw();
}


/** entry text changed */
G_MODULE_EXPORT void on_entry_hardware_name_changed(GtkEditable *e, gpointer u)
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
        ui_setup_tree_refresh();

}


/** entry text changed */
G_MODULE_EXPORT void on_entry_hardware_id_changed(GtkEditable *e, gpointer u)
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

}


/** spinbutton value changed */
G_MODULE_EXPORT void on_spinbutton_hardware_stride_changed(GtkSpinButton *s, gpointer u)
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

}


/** togglebutton toggled */
G_MODULE_EXPORT void on_togglebutton_hardware_init_toggled(GtkToggleButton *b, gpointer u)
{
	LedHardware *h = hardware_niftyled(current_hw);

	/* initialize */
	if(gtk_toggle_button_get_active(b))
	{
		/* initialize hardware */
		LedChain *c = led_hardware_get_chain(h);
		const gchar *id = gtk_entry_get_text(GTK_ENTRY(UI("entry_hw_id")));
		LedCount ledcount = led_chain_get_ledcount(c);
		const char *pixelformat = led_pixel_format_to_string(led_chain_get_format(c));

		if(!(led_hardware_init(h, id, ledcount, pixelformat)))
		{
			ui_log_alert_show("Failed to initialize hardware.");
			return;
		}

		/* show correct image */
		ui_setup_props_hardware_initialized_image(TRUE);

		/* update ID (it might have changed) */
		gtk_entry_set_text(GTK_ENTRY(UI("entry_hw_id")), led_hardware_get_id(h));
	}
	/* deinitialize */
	else
	{
		/* deinitialize hardware */
		led_hardware_deinit(h);

		/* show correct image */
		ui_setup_props_hardware_initialized_image(FALSE);
	}
}