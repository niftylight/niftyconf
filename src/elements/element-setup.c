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


#include <niftyled.h>
#include <gtk/gtk.h>
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-ledlist.h"
#include "elements/element-setup.h"
#include "elements/element-hardware.h"
#include "ui/ui.h"
#include "ui/ui-log.h"




/** current niftyled settings context */
static LedPrefs *_prefs;
/** current niftyled setup */
static LedSetup *_setup;
/** current filename */
static char *_current_filename;




/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/




/******************************************************************************
 ******************************************************************************/


/** getter for current setup */
LedSetup *setup_get_current()
{
        return _setup;
}

void setup_set_current(LedSetup *s)
{
		_setup = s;
}

/** getter for current filename */
const char *setup_get_current_filename()
{
		return _current_filename;
}

/** setup set current filename */
void setup_set_current_filename(const char *filename)
{
		if(_current_filename)
				free(_current_filename);
		
		_current_filename = strdup(filename);
}

/** getter for current preference context */
LedPrefs *setup_get_prefs()
{
	return _prefs;
}

void setup_set_prefs(LedPrefs *p)
{
	_prefs = p;
}

/** dump element definition to printable string - use free() to deallacote the result */
char *setup_dump(gboolean encapsulation)
{
		LedPrefsNode *n;
		if(!(n = led_prefs_setup_to_node(_prefs, _setup)))
		{
				ui_log_alert_show("Failed to create preferences from current setup.");
				return NULL;
		}

		char *result = NULL;
		if(encapsulation)
			result = led_prefs_node_to_buffer(n);
		else
			result = led_prefs_node_to_buffer_light(n);

		led_prefs_node_free(n);

		return result;
}


/** cleanup previously loaded setup */
void setup_cleanup()
{
        /* free all hardware nodes */
        LedHardware *h;
        for(h = led_setup_get_hardware(_setup);
            h;
            h = led_hardware_list_get_next(h))
        {
				/* unregister all tiles of hardware */
				LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_list_get_next(t))
                {
                        tile_unregister_from_gui(led_tile_get_privdata(t));
                }

				/* unregister chain of hardware */
                chain_unregister_from_gui(led_chain_get_privdata(led_hardware_get_chain(h)));
				
				/* unregister hardware */
                hardware_unregister_from_gui(led_hardware_get_privdata(h));
        }

        led_setup_destroy(_setup);
        ui_setup_tree_clear();
		_setup = NULL;
		free(_current_filename);
		_current_filename = NULL;
}



