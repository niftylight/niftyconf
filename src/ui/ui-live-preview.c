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

#include <stdint.h>
#include <gtk/gtk.h>
#include <niftyled.h>
#include "ui-live-preview.h"



static bool _refresh_mapping;






static void _fill_chain(LedChain *c, long long int val)
{
        LedCount i;
		for(i=0; i<led_chain_get_ledcount(c); i++)
		{
				led_chain_set_greyscale(c, i, val);
		}
}


static void _fill_tile(LedTile *t, long long int val)
{
		/* does tile have a chain? */
		LedChain *c;
		if((c = led_tile_get_chain(t)))
				_fill_chain(c, val);

		/* process children */
		LedTile *ct;
		for(ct = led_tile_get_child(t); ct; ct = led_tile_list_get_next(ct))
		{
				_fill_tile(ct, val);
		}
}

/******************************************************************************/

/** clear preview */
void ui_live_preview_clear()
{
		LedHardware *h;
		for(h = led_setup_get_hardware(setup_get_current());
		    h;
		    h = led_hardware_list_get_next(h))
		{
				LedChain *c = led_hardware_get_chain(h);
				_fill_chain(c, 0);

				LedTile *t;
				for(t = led_hardware_get_tile(h); t; t = led_tile_list_get_next(t))
				{
						_fill_tile(t, 0);
				}
		}
}


void ui_live_preview_highlight_chain(NiftyconfChain * chain)
{
		/* no chain? */
        if(!chain)
                NFT_LOG_NULL();

		LedChain *c;
		if(!(c = chain_niftyled(chain)))
				NFT_LOG_NULL();
		
		_fill_chain(c, -1);
}


void ui_live_preview_highlight_hardware(NiftyconfHardware * hardware)
{
        if(!hardware)
                NFT_LOG_NULL();

        LedHardware *h = hardware_niftyled(hardware);
        LedChain *c = led_hardware_get_chain(h);

        ui_live_preview_highlight_chain(led_chain_get_privdata(c));

		_refresh_mapping = false;
}


void ui_live_preview_highlight_tile(NiftyconfTile * tile)
{
        if(!tile)
                NFT_LOG_NULL();

        LedTile *t;
		if(!(t= tile_niftyled(tile)))
				return;

		_fill_tile(t, -1);

		_refresh_mapping = true;
}


void ui_live_preview_highlight_led(NiftyconfLed * led)
{
        if(!led)
                NFT_LOG_NULL();

        // Led *l = led_niftyled(led);
}


void ui_live_preview_show()
{
        LedSetup *s = setup_get_current();
        LedHardware *h = led_setup_get_hardware(s);

		if(_refresh_mapping)
		{
				if(!led_hardware_list_refresh_mapping(h))
						return;
				
				_refresh_mapping = false;
		}
		
		if(!led_hardware_list_send(h))
				return;
		
		if(!led_hardware_list_show(h))
				return;
}
