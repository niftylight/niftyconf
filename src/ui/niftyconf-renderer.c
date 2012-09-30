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
#include "niftyconf-renderer.h"



/** descriptor for renderer - renders one niftyconf element (setup, hardware, tile, ...) */
struct _NiftyconfRenderer
{
        /** element type */
	NIFTYLED_TYPE type;
	/** element */
	gpointer element;
};



/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/


/** initialize this module */
gboolean renderer_init()
{

        return TRUE;
}


/** deinitialize this module */
void renderer_deinit()
{

}


/** allocate new renderer */
NiftyconfRenderer *renderer_new(NIFTYLED_TYPE type, gpointer element)
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

	return n;
}


/** destroy renderer */
void renderer_destroy(NiftyconfRenderer *r)
{
	if(!r)
		NFT_LOG_NULL();


	free(r);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
