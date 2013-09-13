/*
 * niftyconf - niftyled GUI
 * Copyright (C) 2011-2013 Daniel Hiepler <daniel@niftylight.de>
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
#include <niftyled.h>
#include "ui/ui.h"
#include "ui/ui-renderer.h"
#include "elements/element-setup.h"
#include "elements/element-tile.h"
#include "renderer/renderer.h"
#include "prefs/prefs.h"




/** renderer model */
static struct
{
                /** viewport */
        struct
        {
                gdouble pan_x, pan_y;
                gdouble pan_t_x, pan_t_y;
                gdouble scale;
                gdouble scale_delta;
                gdouble scale_cords;
                gdouble scale_factor;
        } view;

        struct
        {
                gboolean mouse_1_pressed;
                gdouble mouse_hold_x, mouse_hold_y;
        } input;

        struct
        {
                cairo_filter_t filter;
                cairo_antialias_t antialias;
        } rendering;
} _r;


/** GtkBuilder for this module */
static GtkBuilder *_ui;




/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** configure from preferences */
static NftResult _this_from_prefs(NftPrefs * prefs,
                                  void **newObj,
                                  NftPrefsNode * node, void *userptr)
{

        /* dummy object */
        *newObj = (void *) 1;


        /* process child nodes */
        NftPrefsNode *child;
        for(child = nft_prefs_node_get_first_child(node);
            child; child = nft_prefs_node_get_next(child))
        {
                nft_prefs_obj_from_node(prefs, child, NULL);
        }


        /* settings */
        nft_prefs_node_prop_double_get(node, "scale", &_r.view.scale);
        nft_prefs_node_prop_double_get(node, "scale_delta",
                                       &_r.view.scale_delta);
        nft_prefs_node_prop_double_get(node, "pan_x", &_r.view.pan_x);
        nft_prefs_node_prop_double_get(node, "pan_y", &_r.view.pan_y);

        nft_prefs_node_prop_int_get(node, "filter",
                                    (int *) &_r.rendering.filter);
        nft_prefs_node_prop_int_get(node, "antialias",
                                    (int *) &_r.rendering.antialias);

        return NFT_SUCCESS;
}


/** save configuration to preferences */
static NftResult _this_to_prefs(NftPrefs * prefs,
                                NftPrefsNode * newNode,
                                void *obj, void *userptr)
{
        if(!nft_prefs_node_prop_double_set(newNode, "scale", _r.view.scale))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_double_set
           (newNode, "scale_delta", _r.view.scale_delta))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_double_set(newNode, "pan_x", _r.view.pan_x))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_double_set(newNode, "pan_y", _r.view.pan_y))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set
           (newNode, "filter", _r.rendering.filter))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set
           (newNode, "antialias", _r.rendering.antialias))
                return NFT_FAILURE;

        return NFT_SUCCESS;
}



/** increase zoom level of rendered view */
static void _zoom_in()
{
        _r.view.scale += _r.view.scale_delta;
        _r.view.scale_delta *= 1.1;

        ui_renderer_all_queue_draw();
}

/** decrease zoom level of rendered view */
static void _zoom_out()
{
        if((_r.view.scale - _r.view.scale_delta) > 0)
        {
                if(_r.view.scale_delta > 0.1)
                        _r.view.scale_delta /= 1.1;
                _r.view.scale -= _r.view.scale_delta;
        }

        ui_renderer_all_queue_draw();
}


/******************************************************************************
 ******************************************************************************/

/** getter for currently configured antialiasing method */
cairo_antialias_t ui_renderer_antialias()
{
        return _r.rendering.antialias;
}


/** getter for currently configured filter method */
cairo_filter_t ui_renderer_filter()
{
        return _r.rendering.filter;
}


/** getter for scaling factor */
gdouble ui_renderer_scale_factor()
{
        return _r.view.scale_factor;
}


/** getter for widget */
GtkWidget *ui_renderer_widget()
{
        return GTK_WIDGET(UI("drawingarea"));
}


/** queue redraw (manually expose) */
void ui_renderer_all_queue_draw()
{
        gtk_widget_queue_draw(GTK_WIDGET(UI("drawingarea")));
}


/** initialize this module */
gboolean ui_renderer_init()
{
        /* register prefs class for this module */
        if(!nft_prefs_class_register
           (prefs(), "renderer", _this_from_prefs, _this_to_prefs))
                g_error("Failed to register prefs class for \"renderer\"");

        /* build ui */
        _ui = ui_builder("niftyconf-renderer.ui");

        /* initial scale */
        _r.view.scale = 0.5;
        _r.view.scale_delta = 0.1;
        _r.view.scale_factor = 25;
        _r.rendering.filter = CAIRO_FILTER_NEAREST;
        _r.rendering.antialias = CAIRO_ANTIALIAS_DEFAULT;

        /* initialize drawingarea */
        gtk_widget_set_app_paintable(GTK_WIDGET(UI("drawingarea")), true);

        return true;
}


/** deinitialize this module */
void ui_renderer_deinit()
{
        /* unregister prefs class */
        nft_prefs_class_unregister(prefs(), "renderer");

        g_object_unref(_ui);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** zoom in */
G_MODULE_EXPORT void on_action_zoom_in_activate(GtkAction * a, gpointer u)
{
        _zoom_in();
}

/** zoom out */
G_MODULE_EXPORT void on_action_zoom_out_activate(GtkAction * a, gpointer u)
{
        _zoom_out();
}

/** mousebutton pressed above drawingarea */
G_MODULE_EXPORT gboolean on_renderer_button_press_event(GtkWidget * w,
                                                        GdkEventButton * ev,
                                                        gpointer u)
{
        /* save coordinates */
        _r.input.mouse_hold_x = ev->x;
        _r.input.mouse_hold_y = ev->y;

        return false;
}


/** mousebutton released above drawingarea */
G_MODULE_EXPORT gboolean on_renderer_button_release_event(GtkWidget * w,
                                                          GdkEvent * ev,
                                                          gpointer u)
{
        _r.view.pan_x += _r.view.pan_t_x;
        _r.view.pan_y += _r.view.pan_t_y;
        _r.view.pan_t_x = 0;
        _r.view.pan_t_y = 0;
        return false;
}


/** mouse moved above drawingarea */
G_MODULE_EXPORT gboolean on_renderer_motion_notify_event(GtkWidget * w,
                                                         GdkEventMotion * ev,
                                                         gpointer u)
{
        /* mousebutton pressed? */
        if(ev->state & GDK_BUTTON1_MASK)
        {
                _r.view.pan_t_x = -(_r.input.mouse_hold_x - ev->x);
                _r.view.pan_t_y = -(_r.input.mouse_hold_y - ev->y);
        }

        ui_renderer_all_queue_draw();

        return false;
}


/** mousewheel turned */
G_MODULE_EXPORT gboolean on_renderer_scroll_event(GtkWidget * w,
                                                  GdkEventScroll * ev,
                                                  gpointer u)
{
        switch (ev->direction)
        {
                case GDK_SCROLL_UP:
                {
                        _zoom_in();
                        break;
                }

                case GDK_SCROLL_DOWN:
                {
                        _zoom_out();
                        break;
                }

                default:
                        break;
        }


        return false;
}


/** exposed event (redraw) */
G_MODULE_EXPORT gboolean on_renderer_expose_event(GtkWidget * w,
                                                  GdkEventExpose * e,
                                                  gpointer d)
{
        /* create cairo context */
        cairo_t *cr;
        cr = gdk_cairo_create(w->window);

        /* get current viewport dimensions */
        GtkAllocation a;
        gtk_widget_get_allocation(GTK_WIDGET(UI("drawingarea")), &a);
        gdouble pan_x, pan_y;
        pan_x = _r.view.pan_x + _r.view.pan_t_x;
        pan_y = _r.view.pan_y + _r.view.pan_t_y;

        /* fill background */
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr,
                        0, 0,
                        (double) gdk_window_get_width(w->window),
                        (double) gdk_window_get_height(w->window));
        cairo_fill(cr);

        /* pan & scale */
        cairo_translate(cr, pan_x, pan_y);
        cairo_scale(cr, _r.view.scale, _r.view.scale);


        /* draw origin */
        cairo_set_source_rgba(cr, 1, 1, 1, 1);
        cairo_set_line_width(cr, _r.view.scale_factor / 25);
#define ORIGIN_FACTOR (_r.view.scale_factor /2.5)
        cairo_move_to(cr, 0, -ORIGIN_FACTOR);
        cairo_line_to(cr, 0, ORIGIN_FACTOR);
        cairo_move_to(cr, -ORIGIN_FACTOR, 0);
        cairo_line_to(cr, ORIGIN_FACTOR, 0);
        cairo_stroke(cr);

        /* renderer of current setup */
        NiftyconfRenderer *r = setup_get_renderer();

        /* get surface */
        cairo_surface_t *surface = renderer_get_surface(r);

        /* compensate offset */
        gdouble xOff, yOff;
        renderer_get_offset(r, &xOff, &yOff);
        cairo_translate(cr, xOff, yOff);

        /* draw surface */
        cairo_set_source_surface(cr, surface, 0, 0);

        /* disable filtering */
        cairo_pattern_set_filter(cairo_get_source(cr), _r.rendering.filter);
        /* disable antialiasing */
        cairo_set_antialias(cr, _r.rendering.antialias);

        /* render surface */
        cairo_paint(cr);

        /* free context */
        cairo_destroy(cr);


        return false;
}
