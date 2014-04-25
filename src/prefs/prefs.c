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
#include <niftyprefs.h>
#include "prefs.h"
#include "version.h"


#define PREFS_FILENAME ".niftyconfrc"

static char _filename[256];
static NftPrefs *_prefs;









/** getter */
NftPrefs *prefs()
{
        return _prefs;
}




/** load preferences */
gboolean prefs_load()
{
        NftPrefsNode *setup;
        if(!(setup = nft_prefs_node_from_file(_prefs, _filename)))
                return false;

        NftPrefsNode *n;
        for(n = nft_prefs_node_get_first_child(setup); n;
            n = nft_prefs_node_get_next(n))
        {
                if(!nft_prefs_obj_from_node(_prefs, n, NULL))
                        return false;
        }

        return true;
}


/** save preferences */
gboolean prefs_save()
{
        /* create preferences */
        NftPrefsNode *setup = nft_prefs_node_alloc("niftyconf");

        /* create main prefs */
        const char *nodes[] = { "main", "ui", "renderer" };
        for(unsigned int i = 0; i < sizeof(nodes) / sizeof(char *); i++)
        {
                NftPrefsNode *n =
                        nft_prefs_obj_to_node(_prefs, nodes[i], NULL, NULL);

                nft_prefs_node_add_child(setup, n);
        }


        return nft_prefs_node_to_file(_prefs, setup, _filename, true);
}


/** initialize preferences */
gboolean prefs_init()
{
        if(!(_prefs = nft_prefs_init(NIFTYCONF_PREFS_VERSION)))
                return false;

        if(snprintf
           (_filename, sizeof(_filename), "%s/" PREFS_FILENAME,
            getenv("HOME")) < 0)
                return false;

        return true;
}


/** deinitialize preferences */
void prefs_deinit()
{
        nft_prefs_deinit(_prefs);
}
