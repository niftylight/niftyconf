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
#include "niftyconf-hardware.h"
#include "niftyconf-chain.h"
#include "niftyconf-setup.h"
#include "niftyconf-log.h"



/** one Hardware element */
struct _NiftyconfHardware
{
        /** niftyled hardware descriptor */
        LedHardware *h;
        /** true if element is currently highlighted */
        gboolean highlight;
        /** true if element tree is currently collapsed */
        gboolean collapsed;
};


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/



/******************************************************************************
 ******************************************************************************/

/** getter for boolean value whether element is currently highlighted */
gboolean hardware_tree_get_highlighted(NiftyconfHardware *h)
{
        if(!h)
                NFT_LOG_NULL(FALSE);
        
        return h->highlight;
}


/* setter for boolean value whether element is currently highlighted */
void hardware_tree_set_highlighted(NiftyconfHardware *h, gboolean is_highlighted)
{
        if(!h)
                NFT_LOG_NULL();

        h->highlight = is_highlighted;
}


/**
 * getter for boolean value whether element row in
 * tree is currently collapsed
 */
gboolean hardware_tree_get_collapsed(NiftyconfHardware *h)
{
        if(!h)
                NFT_LOG_NULL(FALSE);

        return h->collapsed;
}


/**
 * setter for boolean value whether element row in
 * tree is currently collapsed
 */
void hardware_tree_set_collapsed(NiftyconfHardware *h, gboolean is_collapsed)
{
        if(!h)
                NFT_LOG_NULL();

        h->collapsed = is_collapsed;
}

 
/**
 * getter for libniftyled object
 */
LedHardware *hardware_niftyled(NiftyconfHardware *h)
{
        if(!h)
                return NULL;
        
        return h->h;
}





/**
 * allocate new hardware element for GUI
 */
NiftyconfHardware *hardware_register_to_gui(LedHardware *h)
{
        NiftyconfHardware *n;
        if(!(n = calloc(1, sizeof(NiftyconfHardware))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        /* refresh tile->chain mapping */
        led_hardware_refresh_mapping(h);
        
        /* save LedHardware descriptor */
        n->h = h;

        /* default hardware is collapsed */
        n->collapsed = TRUE;
	/* not highlighted */
        n->highlight = FALSE;
	
        /* register Hardware descriptor as LedHardware privdata */
        led_hardware_set_privdata(h, n);

	return n;
}


/**
 * free hardware element
 */
void hardware_unregister_from_gui(NiftyconfHardware *h)
{
        if(!h)
                return;

        if(h->h)
                led_hardware_set_privdata(h->h, NULL);
        
        free(h);
}


/**
 * add hardware to model
 */
NiftyconfHardware *hardware_register_to_gui_and_niftyled(LedHardware *h)
{	
        /* register hardware to gui */
        NiftyconfHardware *hardware;
        if(!(hardware = hardware_register_to_gui(h)))
        {
                log_alert_show("Failed to register hardware to GUI");
                led_hardware_destroy(h);
                return NULL;
        }

	/* register chain of hardware to gui */
	LedChain *chain;
	if((chain = led_hardware_get_chain(h)))
	{
		chain_register_to_gui(chain);
	}
	
	/* get last hardware node */
	LedHardware *last = led_setup_get_hardware(setup_get_current());
	if(!last)
	{
		/* first hardware in setup */
		led_setup_set_hardware(setup_get_current(), h);
	}
	else
	{
		/* append to end of setup */
		led_hardware_list_append(last, h);
	}

	return hardware;
}


/** 
 * create new hardware element in setup 
 */
NiftyconfHardware *hardware_new(const char *name, const char *family, 
                                      const char *id, LedCount ledcount, 
                                      const char *pixelformat)
{
        /* create new niftyled hardware */
        LedHardware *h;
        if(!(h = led_hardware_new(name, family)))
        {
                log_alert_show("Failed to create new hardware");
                return FALSE;
        }

	/* try to initialize hardware */
	if(!led_hardware_init(h, id, ledcount, pixelformat))
	{
		log_alert_show("Failed to initialize new hardware. Not connected?");
	}
	
	/* add hardware to model */
	return hardware_register_to_gui_and_niftyled(h);
        
}


/** 
 * remove hardware from current setup 
 */
void hardware_destroy(NiftyconfHardware *hw)
{
        LedHardware *h = hardware_niftyled(hw);
        
        /* unregister hardware */
        hardware_unregister_from_gui(hw);

	/* remove hardware from setup? */
	if(led_setup_get_hardware(setup_get_current()) == h)
		led_setup_set_hardware(setup_get_current(), NULL);
	
        //led_settings_hardware_unlink(setup_get_current(), h);
        led_hardware_destroy(h);
}


/**
 * initialize hardware module
 */
gboolean hardware_init()
{
        return TRUE;
}

/** deinitialize this module */
void hardware_deinit()
{

}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
