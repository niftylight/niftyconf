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
#include <niftyled.h>
#include "ui/ui.h"
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
				gboolean mouse_1_pressed;
				gdouble mouse_hold_x, mouse_hold_y;
		}view;
}_r;


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



/******************************************************************************
 ******************************************************************************/

/** getter for scaling factor */
gdouble renderer_scale_factor()
{
		return _r.view.scale_factor;
}


/** getter for widget */
GtkWidget *renderer_get_widget()
{
		return GTK_WIDGET(UI("drawingarea"));
}


/** getter for cairo surface */
cairo_surface_t *renderer_get_surface(NiftyconfRenderer *r)
{
		if(!r)
				NFT_LOG_NULL(NULL);

		/* if renderer is marked as damaged, re-render it's surface */
		if(r->damaged)
		{
				/* do we have a renderer? */
				if(r->render)
				{
						if(!r->render(r->surface, r->element))
						{
								NFT_LOG(L_ERROR, "%s renderer (%p) failed", 
								        led_prefs_type_to_string(r->type),
								        r->element);
						}
				}
				else
						NFT_LOG(L_DEBUG, "surface marked as damaged but no function to render it.");

				/* repair renderer :-P */
				r->damaged = false;
		}

		return r->surface;
}


/** queue this renderers surface for update */
void renderer_damage(NiftyconfRenderer *r)
{
		if(!r)
				NFT_LOG_NULL();

		r->damaged = true;
}


/** initialize this module */
gboolean renderer_init()
{
		/* build ui */
		_ui = ui_builder("niftyconf-renderer.ui");

		/* initial scale */
		_r.view.scale = 0.5;
		_r.view.scale_delta = 0.1;
		_r.view.scale_factor = 60;

		/* initialize drawingarea */
		gtk_widget_set_app_paintable(GTK_WIDGET(UI("drawingarea")), TRUE);

		return TRUE;
}


/** deinitialize this module */
void renderer_deinit()
{
		g_object_unref(_ui);
}


/** allocate new renderer */
NiftyconfRenderer *renderer_new(NIFTYLED_TYPE type, 
                                gpointer element, 
                                NiftyconfRenderFunc *render, 
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

		if(!(n->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
		{
				g_error("failed to create cairo surface (%dx%d)",
				        width, height);
				free(n);
				return NULL;
		}


		return n;
}


/** destroy renderer */
void renderer_destroy(NiftyconfRenderer *r)
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
gboolean renderer_resize(NiftyconfRenderer *r, int width, int height)
{
		if(!r || !r->surface)
				NFT_LOG_NULL(FALSE);

		/* silently succeed if size is as requested */
		if(width == cairo_image_surface_get_width(r->surface) &&
		   height == cairo_image_surface_get_height(r->surface))
				return TRUE;

		/* destroy old surface */
		cairo_surface_destroy(r->surface);

		if(!(r->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
				return FALSE;

		/* queue renderer for update */
		renderer_damage(r);

		return TRUE;
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
gboolean on_renderer_button_press_event(GtkWidget *w,
                                        GdkEventButton *ev,
                                        gpointer u)
{
		/* save coordinates */
		_r.view.mouse_hold_x = ev->x;
		_r.view.mouse_hold_y = ev->y;

		return FALSE;
}


/**
 * mousebutton released above drawingarea
 */
gboolean on_renderer_button_release_event(GtkWidget *w,
                                          GdkEvent *ev,
                                          gpointer u)
{
		_r.view.pan_x += _r.view.pan_t_x;
		_r.view.pan_y += _r.view.pan_t_y;
		_r.view.pan_t_x = 0;
		_r.view.pan_t_y = 0;
		return FALSE;
}


/**
 * mouse moved above drawingarea
 */
gboolean on_renderer_motion_notify_event(GtkWidget *w,
                                         GdkEventMotion *ev,
                                         gpointer u)
{
		/* mousebutton pressed? */
		if(ev->state & GDK_BUTTON1_MASK)
		{
				_r.view.pan_t_x = -(_r.view.mouse_hold_x - ev->x);
				_r.view.pan_t_y = -(_r.view.mouse_hold_y - ev->y);
		}

		renderer_all_queue_draw();

		return FALSE;
}


/**
 * mousewheel turned
 */
gboolean on_renderer_scroll_event(GtkWidget *w,
                                  GdkEventScroll *ev,
                                  gpointer u)
{
		switch(ev->direction)
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


		return FALSE;
}


/**
 * exposed event (redraw)
 */
gboolean on_renderer_expose_event(GtkWidget *w, GdkEventExpose *e, gpointer d)
{       
		/* create cairo context */
		cairo_t *cr;
		cr = gdk_cairo_create(w->window);

		/* disable antialiasing */
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

		/* get current viewport dimensions */
		GtkAllocation a;
		gtk_widget_get_allocation(GTK_WIDGET(UI("drawingarea")), &a);
		gdouble pan_x, pan_y;
		pan_x = ((double) a.width/2) + _r.view.pan_x+_r.view.pan_t_x;
		pan_y = ((double) a.height/2) + _r.view.pan_y+_r.view.pan_t_y;


		/* current LedSetup */
		LedSetup *s;
		if(!(s = setup_get_current()))
				return FALSE;

		
		/* fill background */
		cairo_set_source_rgb(cr, 0,0,0);
		cairo_rectangle(cr,
		                0, 0,
		                (double) gdk_window_get_width(w->window),
		                (double) gdk_window_get_height(w->window));
		cairo_fill(cr);

		/* pan & scale */
		cairo_translate(cr,
		                pan_x-((double) led_setup_get_width(s)*_r.view.scale*_r.view.scale_factor)/2,
		                pan_y-((double) led_setup_get_height(s)*_r.view.scale*_r.view.scale_factor)/2);
		cairo_scale(cr, _r.view.scale, _r.view.scale);

		/* walk through all hardware LED adapters */
		LedHardware *h;
		for(h = led_setup_get_hardware(s);
		    h;
		    h = led_hardware_list_get_next(h))
		{

				/* Walk all tiles of this hardware & draw their surface */
				LedTile *t;
				for(t = led_hardware_get_tile(h);
				    t;
				    t = led_tile_list_get_next(t))
				{

						/* check visibility */

						/* get surface from tile */
						NiftyconfTile *tile = (NiftyconfTile *) led_tile_get_privdata(t);

						/* draw surface */
						cairo_set_source_surface(cr, tile_get_renderer(tile)->surface,
						                         (double) (led_tile_get_x(t)*_r.view.scale_factor),
						                         (double) (led_tile_get_y(t)*_r.view.scale_factor));
						cairo_paint(cr);
				}

				/* Draw chain of this hardware */
				/*s = chain_get_surface(led_chain_get_privdata(led_hardware_get_chain(h)));
				cairo_set_source_surface(cr, s, 0, 0);
				cairo_paint(cr);*/

		}

		cairo_destroy(cr);
		return FALSE;
}
