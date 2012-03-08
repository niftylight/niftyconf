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
#include "niftyconf-ui.h"
#include "niftyconf-setup-props.h"
#include "niftyconf-led.h"
#include "niftyconf-chain.h"



/** one element */
struct _NiftyconfChain
{
        /** niftyled descriptor */
        LedChain *c;
};


/* columns for our listview */
typedef enum
{
        /* (gint) NIFTYLED_TYPE of element */
        C_CHAIN_LED = 0,
        /* (gpointer) to NiftyconfLed descriptor */
        C_CHAIN_ELEMENT,
        NUM_CHAIN_COLS,
}CHAIN_LISTVIEW_COLUMNS;



static GtkBox *         box_chain;
static GtkListStore *   liststore;


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


/** function to process an element that is currently selected */
static void _element_selected(GtkTreeModel *m, GtkTreePath *p, GtkTreeIter *i, gpointer data)
{
        /* get element represented by this row */
        gpointer *element;
        LedCount n;
        gtk_tree_model_get(m, i, C_CHAIN_LED, &n, C_CHAIN_ELEMENT, &element,  -1);
        NiftyconfLed *l = (NiftyconfLed *) element;

        setup_props_hide();
        setup_props_led_show(l);
}


/******************************************************************************
 ******************************************************************************/
/**
 * rebuild list
 */
void chain_list_rebuild(NiftyconfChain *c)
{
        if(!c)
                return;
        LedCount i;
        for(i = 0;
            i < led_chain_get_ledcount(c->c);
            i++)
        {         
                Led *led = led_chain_get_nth(c->c, i);
                GtkTreeIter iter;
                gtk_list_store_append(liststore, &iter);
                gtk_list_store_set(liststore, &iter,
                                  C_CHAIN_LED, i,
                                  C_CHAIN_ELEMENT, led_get_privdata(led),
                                  -1);
        }

}


/**
 * clear list
 */
void chain_list_clear()
{
        if(!liststore)
                return;
        
        gtk_list_store_clear(liststore);
}


/**
 * getter for list widget
 */
GtkWidget *chain_list_widget()
{
        return GTK_WIDGET(box_chain);
}


/**
 * getter for libniftyled object
 */
LedChain *chain_niftyled(NiftyconfChain *c)
{
        if(!c)
                return NULL;
        
        return c->c;
}


/**
 * allocate new element
 */
NiftyconfChain *chain_new(LedChain *c)
{
        NiftyconfChain *n;
        if(!(n = calloc(1, sizeof(NiftyconfChain))))
        {
                g_error("calloc: %s", strerror(errno));
                return NULL;
        }

        /* save descriptor */
        n->c = c;

        /* register descriptor as niftyled privdata */
        led_chain_set_privdata(c, n);

        /* allocate all LEDs of chain */
        LedCount i;
        for(i = 0; i < led_chain_get_ledcount(c); i++)
        {
                led_new(led_chain_get_nth(c, i));
        }
}


/**
 * free element
 */
void chain_free(NiftyconfChain *c)
{
        if(!c)
                return;

        /* free all LEDs of chain */
        LedCount i;
        for(i = 0; i < led_chain_get_ledcount(c->c); i++)
        {
                led_free(led_get_privdata(led_chain_get_nth(c->c, i)));
        }
        
        led_chain_set_privdata(c->c, NULL);
        free(c);
}


/**
 * initialize chain module
 */
gboolean chain_init()
{
        GtkBuilder *ui = ui_builder("niftyconf-chain.ui");

        /* get widgets */
        if(!(box_chain = GTK_BOX(gtk_builder_get_object(ui, "box_chain"))))
                return FALSE;
        if(!(liststore = GTK_LIST_STORE(gtk_builder_get_object(ui, "liststore"))))
                return FALSE;

        /* set selection mode for tree */
        GtkTreeView *tree = GTK_TREE_VIEW(gtk_builder_get_object(ui, "treeview"));       
        gtk_tree_selection_set_mode(
                gtk_tree_view_get_selection(tree), 
                GTK_SELECTION_MULTIPLE);
        
        /* initialize setup treeview */
        GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(ui, "column_led"));
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, renderer, TRUE);
        gtk_tree_view_column_add_attribute(col, renderer, "text", C_CHAIN_LED);
        
        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/


/**
 * user selected another row
 */
void on_chain_treeview_cursor_changed(GtkTreeView *tv, gpointer u)
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
                return;
        }

        /* process all selected elements */
        gtk_tree_selection_selected_foreach(s, _element_selected, NULL);

        //setup_redraw();
        //scene_redraw();
}
