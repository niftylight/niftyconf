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

#ifndef _NIFTYCONF_RENDERER_H
#define _NIFTYCONF_RENDERER_H

#include <cairo.h>
#include <niftyled.h>


typedef struct _NiftyconfRenderer NiftyconfRenderer;
typedef                         NftResult(NiftyconfRenderFunc) (cairo_surface_t ** s, gpointer element);


/* GUI model functions */
NiftyconfRenderer              *renderer_new(NIFTYLED_TYPE type, gpointer element, NiftyconfRenderFunc * render, gint width, gint height);
void                            renderer_destroy(NiftyconfRenderer * r);

/* GUI functions */
gboolean                        renderer_init();
void                            renderer_deinit();
GtkWidget                      *renderer_widget();
cairo_filter_t                  renderer_filter();
cairo_antialias_t               renderer_antialias();
gdouble                         renderer_scale_factor();
void                            renderer_all_queue_draw();
void                            renderer_damage(NiftyconfRenderer * r);
gboolean                        renderer_resize(NiftyconfRenderer * r, gint width, gint height);
cairo_surface_t                *renderer_get_surface(NiftyconfRenderer * r);

/* model functions */
gboolean                        renderer_set_offset(NiftyconfRenderer *r, double xOff, double yOff);


#endif /* _NIFTYCONF_RENDERER_H */
