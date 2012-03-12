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
#include "niftyconf-setup.h"
#include "niftyconf-setup-props.h"
#include "niftyconf-hardware.h"
#include "niftyconf-ui.h"


/* columns for our setup-treeview */
typedef enum
{
        /* (gint) NIFTYLED_TYPE of element */
        C_SETUP_TYPE = 0,
        /* (gchararray) title of element */
        C_SETUP_TITLE,
        /* (gpointer) niftyled element (LedHardware, LedChain, LedTile, Led) */
        C_SETUP_ELEMENT,
        NUM_SETUP_COLS,
}SETUP_TREEVIEW_COLUMNS;


/** current niftyled settings context */
static LedSettings *current;
/** main container widget of this module */
static GtkBox *box_tree;
/** setup tree store */
static GtkTreeStore *tree_store;





/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** function to process an element that is currently selected */
static void _element_selected(GtkTreeModel *m, GtkTreePath *p, GtkTreeIter *i, gpointer data)
{
        /* get element represented by this row */
        gpointer *element;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &element,  -1);

        setup_props_hide();
        
        switch(t)
        {
                /* hardware selected */
                case T_LED_HARDWARE:
                {
                        NiftyconfHardware *h = (NiftyconfHardware *) element;
                        setup_props_hardware_show(h);

                        /* redraw everything */
                        //setup_redraw();
                        
                        /* clear led-list */
                        chain_list_clear();
                        break;
                }

                /* tile element selected */
                case T_LED_TILE:
                {
                        NiftyconfTile *tile = (NiftyconfTile *) element;
                        setup_props_tile_show(tile);

                        /* highlight tile */
                        //tile_set_highlight(tile, TRUE);
                        
                        /* redraw everything */
                        //setup_redraw();

                        /* clear led-list */
                        chain_list_clear();
                        break;
                }

                /* chain element selected */
                case T_LED_CHAIN:
                {
                        NiftyconfChain *chain = (NiftyconfChain *) element;
                        setup_props_chain_show(chain);

                        /* redraw everything */
                        //setup_redraw();
                        
                        /* display led-list */
                        chain_list_rebuild(chain);
                        
                        break;
                }

                default:
                {
                        g_warning("row with unknown tile selected. This is a bug!");
                }
        }
}


/** append chain element to setup-tree */
static void _tree_append_chain(GtkTreeStore *s, LedChain *c, GtkTreeIter *parent)
{
        NiftyconfChain *chain = led_chain_get_privdata(c);
        GtkTreeIter i;
        gtk_tree_store_append(s, &i, parent);
        gtk_tree_store_set(s, &i,
                           C_SETUP_TYPE, T_LED_CHAIN,
                           C_SETUP_TITLE, "chain", 
                           C_SETUP_ELEMENT, (gpointer) chain,
                           -1);
}


/** append tile element to setup-tree */
static void _tree_append_tile(GtkTreeStore *s, LedTile *t, GtkTreeIter *parent)
{
        NiftyconfTile *tile = led_tile_get_privdata(t);
        GtkTreeIter i;
        gtk_tree_store_append(s, &i, parent);
        gtk_tree_store_set(s, &i,
                           C_SETUP_TYPE, T_LED_TILE,
                           C_SETUP_TITLE, "tile", 
                           C_SETUP_ELEMENT, (gpointer) tile,
                           -1);

        /* append chain if there is one */
        LedChain *c;
        if((c = led_tile_get_chain(t)))
        {
                _tree_append_chain(s, c, &i);
        }
        
        /* append children of this tile */
        LedTile *child;
        for(child = led_tile_child_get(t);
            child;
            child = led_tile_sibling_get_next(child))
        {
                _tree_append_tile(s, child, &i);
        }
        
}


/** append hardware element to setup-tree */
static void _tree_append_hardware(GtkTreeStore *s, LedHardware *h)
{
        NiftyconfHardware *hardware = led_hardware_get_privdata(h);
        GtkTreeIter i;
        gtk_tree_store_append(s, &i, NULL);
        gtk_tree_store_set(s, &i,
                           C_SETUP_TYPE, T_LED_HARDWARE,
                           C_SETUP_TITLE, led_hardware_get_name(h), 
                           C_SETUP_ELEMENT, (gpointer) hardware,
                           -1);

        /** append chain */
        _tree_append_chain(s, led_hardware_get_chain(h), &i);

        /** append all tiles */
        LedTile *t;
        for(t = led_hardware_get_tile(h);
            t;
            t = led_tile_sibling_get_next(t))
        {
                _tree_append_tile(s, t, &i);
        }
}



/******************************************************************************
 ******************************************************************************/

/**
 * rebuild setup-tree according to current setup
 */
void setup_tree_rebuild()
{
        /**
         * add every hardware-node (+ children) 
         * to the setup-treeview 
         */
        LedHardware *h;
        for(h = led_settings_hardware_get_first(current); h; h = led_hardware_sibling_get_next(h))
        {         
                _tree_append_hardware(tree_store, h);
        }

}


/**
 * clear setup tree
 */
void setup_tree_clear()
{
        if(!tree_store)
                return;
        
        gtk_tree_store_clear(tree_store);
}


/**
 * getter for tree widget
 */
GtkWidget *setup_tree_widget()
{
        return GTK_WIDGET(box_tree);
}


/**
 * load new setup from XML file definition 
 */
gboolean setup_load(gchar *filename)
{
        /* load new setup */
        LedSettings *s;
        if(!(s = led_settings_load(filename)))
                return FALSE;

        
        /* cleanup previously loaded setup */
        if(current)
                setup_cleanup();

        /* initialize our element descriptor and set as
           privdata in niftyled model */
        LedHardware *h;
        for(h = led_settings_hardware_get_first(s); 
            h; 
            h = led_hardware_sibling_get_next(h))
        {
                /* create new hardware element */
                if(!hardware_new(h))
                {
                        g_warning("failed to allocate new hardware element");
                        return FALSE;
                }

                /* create chain of this hardware */
                if(!chain_new(led_hardware_get_chain(h)))
                {
                        g_warning("failed to allocate new chain element");
                        return FALSE;
                }

                /* walk all tiles belonging to this hardware & initialize */
                LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_sibling_get_next(t))
                {
                        if(!tile_new(t))
                        {
                                g_warning("failed to allocate new tile element");
                                return FALSE;
                        }
                }
        }
        
        /* save new settings */
        current = s;

        /* update ui */
        setup_tree_clear();
        setup_tree_rebuild();
        
        
        /* redraw new setup */
        //setup_redraw();
        
        return TRUE;
}


/** 
 * cleanup previously loaded setup 
 */
void setup_cleanup()
{
        /* free all hardware nodes */
        LedHardware *h;
        for(h = led_settings_hardware_get_first(current);
            h;
            h = led_hardware_sibling_get_next(h))
        {
                LedTile *t;
                for(t = led_hardware_get_tile(h);
                    t;
                    t = led_tile_sibling_get_next(t))
                {
                        tile_free(led_tile_get_privdata(t));
                }
                
                chain_free(led_chain_get_privdata(led_hardware_get_chain(h)));
                hardware_free(led_hardware_get_privdata(h));
        }
        
        led_settings_destroy(current);
        setup_tree_clear();
}


/**
 * initialize setup module
 */
gboolean setup_init()
{
        GtkBuilder *ui = ui_builder("niftyconf-setup.ui");

        /* get widgets */
        if(!(box_tree = GTK_BOX(gtk_builder_get_object(ui, "box_tree"))))
                return FALSE;
        if(!(tree_store = GTK_TREE_STORE(gtk_builder_get_object(ui, "treestore"))))
                return FALSE;
        

        /* set selection mode for setup tree */
        GtkTreeView *tree = GTK_TREE_VIEW(gtk_builder_get_object(ui, "treeview"));       
        gtk_tree_selection_set_mode(
                gtk_tree_view_get_selection(tree), 
                GTK_SELECTION_MULTIPLE);
        
        /* initialize setup treeview */
        GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(ui, "column_element"));
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, renderer, TRUE);
        gtk_tree_view_column_add_attribute(col, renderer, "text", C_SETUP_TITLE);

        
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/**
 * user selected another row
 */
void on_setup_treeview_cursor_changed(GtkTreeView *tv, gpointer u)
{
        GtkTreeModel *m = gtk_tree_view_get_model(tv);
        
        /* unhighlight all elements */
        //GtkTreeIter i;
        //gtk_tree_model_get_iter_root(m, &i);
        //_walk_tree(&i, _unhighlight_element);

        
        /* get current selection */
        GtkTreeSelection *s;
        if(!(s = gtk_tree_view_get_selection(tv)))
        {
                setup_props_hide();
                return;
        }

        /* process all selected elements */
        gtk_tree_selection_selected_foreach(s, _element_selected, NULL);

        //setup_redraw();
        //scene_redraw();
}
