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
#include "prefs/prefs.h"
#include "elements/element-setup.h"
#include "elements/element-tile.h"
#include "renderer/renderer.h"




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


/** one renderer - renders one niftyconf element (setup, hardware, tile, ...) */
struct _NiftyconfRenderer
{
                /** element type */
        NIFTYLED_TYPE type;
                /** element */
        gpointer element;
                /** drawing surface */
        cairo_surface_t *surface;
                /** true if surface needs to be redrawn */
        bool damaged;
                /** render function */
        NiftyconfRenderFunc *render;
};


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


/******************************************************************************
 ******************************************************************************/

/** getter for currently configured antialiasing method */
cairo_antialias_t renderer_antialias()
{
        return _r.rendering.antialias;
}


/** getter for currently configured filter method */
cairo_filter_t renderer_filter()
{
        return _r.rendering.filter;
}


/** getter for scaling factor */
gdouble renderer_scale_factor()
{
        return _r.view.scale_factor;
}


/** getter for widget */
GtkWidget *renderer_widget()
{
        return GTK_WIDGET(UI("drawingarea"));
}


/** getter for cairo surface */
cairo_surface_t *renderer_get_surface(NiftyconfRenderer * r)
{
        if(!r)
                NFT_LOG_NULL(NULL);

        /* if renderer is marked as damaged, re-render it's surface */
        if(r->damaged)
        {
                /* do we have a renderer? */
                if(r->render)
                {
                        if(!r->render(&r->surface, r->element))
                        {
                                NFT_LOG(L_ERROR, "%s renderer (%p) failed",
                                        led_prefs_type_to_string(r->type),
                                        r->element);
                        }
                }
                else
                        NFT_LOG(L_DEBUG,
                                "surface marked as damaged but no function to render it.");

                /* repair renderer :-P */
                r->damaged = false;
        }

        return r->surface;
}


/** queue this renderers surface for update */
void renderer_damage(NiftyconfRenderer * r)
{
        if(!r)
                NFT_LOG_NULL();

        r->damaged = true;
}


/** initialize this module */
gboolean renderer_init()
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
        _r.view.scale_factor = 15;
        _r.rendering.filter = CAIRO_FILTER_NEAREST;
        _r.rendering.antialias = CAIRO_ANTIALIAS_DEFAULT;

        /* initialize drawingarea */
        gtk_widget_set_app_paintable(GTK_WIDGET(UI("drawingarea")), true);

        return true;
}


/** deinitialize this module */
void renderer_deinit()
{
        /* unregister prefs class */
        nft_prefs_class_unregister(prefs(), "renderer");

        g_object_unref(_ui);
}


/** allocate new renderer */
NiftyconfRenderer *renderer_new(NIFTYLED_TYPE type,
                                gpointer element,
                                NiftyconfRenderFunc * render,
                                gint width, gint height)
{
        if(!element)
                NFT_LOG_NULL(NULL);

        NiftyconfRenderer *n;
        if(!(n = calloc(1, sizeof(NiftyconfRenderer))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        n->type = type;
        n->element = element;
        n->render = render;

        if(!(n->surface =
             cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
        {
                g_error("failed to create cairo surface (%dx%d)",
                        width, height);
                free(n);
                return NULL;
        }


        return n;
}


/** destroy renderer */
void renderer_destroy(NiftyconfRenderer * r)
{
        if(!r)
                NFT_LOG_NULL();

        if(r->surface)
        {
                cairo_surface_destroy(r->surface);
                r->surface = NULL;
        }

        free(r);
}


/** resize surface of renderer */
gboolean renderer_resize(NiftyconfRenderer * r, int width, int height)
{
        if(!r || !r->surface)
                NFT_LOG_NULL(false);

        /* silently succeed if size is as requested */
        if(width == cairo_image_surface_get_width(r->surface) &&
           height == cairo_image_surface_get_height(r->surface))
                return true;

        /* destroy old surface */
        cairo_surface_destroy(r->surface);

        if(!(r->surface =
             cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
                return false;

        /* queue renderer for update */
        renderer_damage(r);

        return true;
}


/** queue redraw (manually expose) */
void renderer_all_queue_draw()
{
        gtk_widget_queue_draw(GTK_WIDGET(UI("drawingarea")));
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/**
 * mousebutton pressed above drawingarea
 */
gboolean on_renderer_button_press_event(GtkWidget * w,
                                        GdkEventButton * ev, gpointer u)
{
        /* save coordinates */
        _r.input.mouse_hold_x = ev->x;
        _r.input.mouse_hold_y = ev->y;

        return false;
}


/**
 * mousebutton released above drawingarea
 */
gboolean on_renderer_button_release_event(GtkWidget * w,
                                          GdkEvent * ev, gpointer u)
{
        _r.view.pan_x += _r.view.pan_t_x;
        _r.view.pan_y += _r.view.pan_t_y;
        _r.view.pan_t_x = 0;
        _r.view.pan_t_y = 0;
        return false;
}


/**
 * mouse moved above drawingarea
 */
gboolean on_renderer_motion_notify_event(GtkWidget * w,
                                         GdkEventMotion * ev, gpointer u)
{
        /* mousebutton pressed? */
        if(ev->state & GDK_BUTTON1_MASK)
        {
                _r.view.pan_t_x = -(_r.input.mouse_hold_x - ev->x);
                _r.view.pan_t_y = -(_r.input.mouse_hold_y - ev->y);
        }

        renderer_all_queue_draw();

        return false;
}


/**
 * mousewheel turned
 */
gboolean on_renderer_scroll_event(GtkWidget * w,
                                  GdkEventScroll * ev, gpointer u)
{
        switch (ev->direction)
        {
                case GDK_SCROLL_UP:
                {
                        _r.view.scale += _r.view.scale_delta;
                        _r.view.scale_delta *= 1.1;

                        renderer_all_queue_draw();
                        break;
                }

                case GDK_SCROLL_DOWN:
                {
                        if((_r.view.scale - _r.view.scale_delta) > 0)
                        {
                                if(_r.view.scale_delta > 0.1)
                                        _r.view.scale_delta /= 1.1;
                                _r.view.scale -= _r.view.scale_delta;
                        }

                        renderer_all_queue_draw();
                        break;
                }

                default:
                        break;
        }


        return false;
}


/**
 * exposed event (redraw)
 */
gboolean on_renderer_expose_event(GtkWidget * w,
                                  GdkEventExpose * e, gpointer d)
{
        /* create cairo context */
        cairo_t *cr;
        cr = gdk_cairo_create(w->window);

        /* get current viewport dimensions */
        GtkAllocation a;
        gtk_widget_get_allocation(GTK_WIDGET(UI("drawingarea")), &a);
        gdouble pan_x, pan_y;
        pan_x = ((double) a.width / 2) + _r.view.pan_x + _r.view.pan_t_x;
        pan_y = ((double) a.height / 2) + _r.view.pan_y + _r.view.pan_t_y;

        /* fill background */
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr,
                        0, 0,
                        (double) gdk_window_get_width(w->window),
                        (double) gdk_window_get_height(w->window));
        cairo_fill(cr);

        /* pan & scale */
        cairo_translate(cr,
                        pan_x -
                        ((double) led_setup_get_width(setup_get_current()) *
                         _r.view.scale * _r.view.scale_factor) / 2,
                        pan_y -
                        ((double) led_setup_get_height(setup_get_current()) *
                         _r.view.scale * _r.view.scale_factor) / 2);
        cairo_scale(cr, _r.view.scale, _r.view.scale);


        /* renderer of current setup */
        NiftyconfRenderer *r = setup_get_renderer();

        /* draw surface */
        cairo_set_source_surface(cr, renderer_get_surface(r), 0, 0);

        /* disable filtering */
        cairo_pattern_set_filter(cairo_get_source(cr), _r.rendering.filter);
        /* disable antialiasing */
        cairo_set_antialias(cr, _r.rendering.antialias);

        /* render surface */
        cairo_paint(cr);

        /* draw origin */
        cairo_set_source_rgba(cr, 1, 1, 1, 1);
        cairo_set_line_width(cr, 0.5);
        cairo_move_to(cr, 0, -5);
        cairo_line_to(cr, 0, 5);
        cairo_move_to(cr, -5, 0);
        cairo_line_to(cr, 5, 0);
        cairo_stroke(cr);

        /* free context */
        cairo_destroy(cr);


        return false;
}
