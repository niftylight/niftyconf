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
#include "niftyconf-ui.h"
#include "niftyconf-setup.h"
#include "niftyconf-setup-props.h"
#include "niftyconf-setup-tree.h"



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


/** main box */
static GtkBox *box;
/** setup tree store */
static GtkTreeStore *tree_store;
/** setup tree-view */
static GtkTreeView *tree_view;

/* type of currently selected element */
NIFTYLED_TYPE current_type;
/* currently selected element */
static NiftyconfHardware *current_hw;
static NiftyconfTile *current_tile;
static NiftyconfChain *current_chain;



/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


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
        for(child = led_tile_get_child(t);
            child;
            child = led_tile_get_next_sibling(child))
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
            t = led_tile_get_next_sibling(t))
        {
                _tree_append_tile(s, t, &i);
        }
}


/** remove chain from current setup */
static void _remove_chain(NiftyconfChain *c)
{
        LedChain *chain = chain_niftyled(c);
        chain_unregister(c);
        led_settings_chain_unlink(setup_get_current(), chain);
        led_chain_destroy(chain);
}

/** remove tile from current setup */
static void _remove_tile(NiftyconfTile *tile)
{
        if(!tile)
                NFT_LOG_NULL();
        
        LedTile *t = tile_niftyled(tile);
        
        /* unregister from gui */
        tile_unregister(tile);
        /* unregister from settings */
        led_settings_tile_unlink(setup_get_current(), t);
        /* destroy with all children */
        led_tile_destroy(t);
}

/** remove hardware from current setup */
static void _remove_hardware(NiftyconfHardware *hw)
{
        LedHardware *h = hardware_niftyled(hw);
        
        /* unregister hardware */
        hardware_unregister(hw);
        led_settings_hardware_unlink(setup_get_current(), h);
        led_hardware_destroy(h);
}


/** wrapper for do_* functions */
static void _foreach_remove_hardware(NIFTYLED_TYPE t, gpointer *e)
{
        if(t != T_LED_HARDWARE)
                return;

        _remove_hardware((NiftyconfHardware *) e);
}


/** wrapper for do_* functions */
static void _foreach_remove_tile(NIFTYLED_TYPE t, gpointer *e)
{
        if(t != T_LED_TILE)
                return;

        _remove_tile((NiftyconfTile *) e);
}


/** wrapper for do_* functions */
static void _foreach_remove_chain(NIFTYLED_TYPE type, gpointer *e)
{
        /* works only if tile-element is selected */
        if(type != T_LED_TILE)
                return;

        /* get niftyled tile */
        NiftyconfTile *tile = (NiftyconfTile *) e;
        LedTile *t = tile_niftyled(tile);
        
        /* if this tile has no chain, silently succeed */
        LedChain *c;
        if(!(c = led_tile_get_chain(t)))
                return;

        /* unregister from tile */
        led_tile_set_chain(t, NULL);
        
        /* unregister from gui */
        NiftyconfChain *chain = led_chain_get_privdata(c);
        _remove_chain(chain);
}


/** either collapse or expand a row of the setup-tree */
gboolean _foreach_element_refresh_collapse(GtkTreeModel *model, GtkTreePath *path,
                               GtkTreeIter *iter, gpointer u)
{  
        /* get niftyled element */
        gpointer *element;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(model, iter, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &element,  -1);

        gboolean collapsed = FALSE;
        
        switch(t)
        {
                case T_LED_HARDWARE:
                {
                        collapsed = hardware_tree_get_collapsed((NiftyconfHardware *) element);
                        break;
                }

                case T_LED_TILE:
                {
                        collapsed = tile_tree_get_collapsed((NiftyconfTile *) element);
                        break;
                }
        }

        if(collapsed)
                gtk_tree_view_collapse_row(tree_view, path);
        else
                gtk_tree_view_expand_row(tree_view, path, FALSE);                

        
        return FALSE;
}


/** set selection-state from a row of the setup-tree */
gboolean _foreach_element_refresh_highlight(GtkTreeModel *model, GtkTreePath *path,
                               GtkTreeIter *iter, gpointer u)
{
        /* get niftyled element */
        gpointer *element;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(model, iter, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &element,  -1);

        gboolean highlighted = FALSE;

        switch(t)
        {
                case T_LED_HARDWARE:
                {
                        highlighted = hardware_tree_get_highlighted(
                                                (NiftyconfHardware *) element);
                        break;
                }

                case T_LED_TILE:
                {
                        highlighted = tile_tree_get_highlighted(
                                                (NiftyconfTile *) element);
                        break;
                }

                case T_LED_CHAIN:
                {
                        highlighted = chain_tree_get_highlighted(
                                                (NiftyconfChain *) element);
                        break;
                }
        }

        GtkTreeSelection *s = gtk_tree_view_get_selection(tree_view);
        
        if(highlighted)
                gtk_tree_selection_select_iter(s, iter);
        else
                gtk_tree_selection_unselect_iter(s, iter);
                
        return FALSE;
}


/** foreach: function to process an element that is currently selected */
static void _foreach_element_selected(NIFTYLED_TYPE t, gpointer *e)
{
        setup_props_hide();
        
        switch(t)
        {
                /* hardware selected */
                case T_LED_HARDWARE:
                {
                        /* highlight hardware */
                        hardware_tree_set_highlighted((NiftyconfHardware *) e, TRUE);

                        setup_props_hardware_show((NiftyconfHardware *) e);
                        
                        /* redraw everything */
                        //setup_redraw();
                        
                        /* clear led-list */
                        chain_ledlist_clear();
                        break;
                }

                /* tile element selected */
                case T_LED_TILE:
                {
                        /* highlight tile */
                        tile_tree_set_highlighted((NiftyconfTile *) e, TRUE);
                        
                        setup_props_tile_show((NiftyconfTile *) e);

                                                
                        /* redraw everything */
                        //setup_redraw();

                        /* clear led-list */
                        chain_ledlist_clear();
                        break;
                }

                /* chain element selected */
                case T_LED_CHAIN:
                {
                        /* highlight chain */
                        chain_tree_set_highlighted((NiftyconfChain *) e, TRUE);
                        
                        setup_props_chain_show((NiftyconfChain *) e);
                        
                        /* redraw everything */
                        //setup_redraw();
                        
                        /* display led-list */
                        chain_ledlist_rebuild((NiftyconfChain *) e);
                        
                        break;
                }

                default:
                {
                        g_warning("row with unknown tile selected. This is a bug!");
                }
        }
}


/** foreach: unhighlight element */
static void _foreach_unhighlight_element(NIFTYLED_TYPE t, gpointer *e)
{
        switch(t)
        {
                case T_LED_HARDWARE:
                {
                        hardware_tree_set_highlighted((NiftyconfHardware *) e, FALSE);
                        break;
                }

                case T_LED_TILE:
                {
                        tile_tree_set_highlighted((NiftyconfTile *) e, FALSE);
                        break;
                }

                case T_LED_CHAIN:
                {
                        chain_tree_set_highlighted((NiftyconfChain *) e, FALSE);
                        break;
                }
        }

}


/** set currently active element */
static void _foreach_set_current_element(NIFTYLED_TYPE t, gpointer *e)
{
        switch(t)
        {
                case T_LED_HARDWARE:
                {
                        current_hw = (NiftyconfHardware *) e;
                        break;
                }

                case T_LED_TILE:
                {
                        current_tile = (NiftyconfTile *) e;
                        break;
                }

                case T_LED_CHAIN:
                {
                        current_chain = (NiftyconfChain *) e;
                        break;
                }
        }

        current_type = t;
}


/** recursion helper */
static void _do_foreach_iter(GtkTreeModel *m, GtkTreeIter *i,
                          void (*func)(NIFTYLED_TYPE t, gpointer *e))
{
        do
        {
                /* process children */
                GtkTreeIter c;
                if(gtk_tree_model_iter_children(m, &c, i))
                {
                        _do_foreach_iter(m, &c, func);
                }

                /* get this element */
                NIFTYLED_TYPE t;
                gpointer *e;
                gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &e,  -1);

                /* launch function */
                func(t, e);
        }while(gtk_tree_model_iter_next(m, i));
}

/** run function on every tree-element */
static void _do_foreach_element(void (*func)(NIFTYLED_TYPE t, gpointer *e))
{
        /* get model */
        GtkTreeModel *m = gtk_tree_view_get_model(tree_view);
        GtkTreeIter iter;
        gboolean run;
        gtk_tree_model_get_iter_root(m, &iter);
        _do_foreach_iter(m, &iter, func);
}

/** run function on every selected tree-element (multiple selections) */
static void _do_foreach_selected_element(void (*func)(NIFTYLED_TYPE t, gpointer *element))
{
        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));

        /* something selected? */
        GList *selected;
        GtkTreeModel *m;
        if(!(selected = gtk_tree_selection_get_selected_rows(selection, &m)))
                return;

        /* walk all selected rows */
        GList *cur;
        for(cur = g_list_last(selected); cur; cur = g_list_previous(cur))
        {
                GtkTreePath *path = (GtkTreePath *) cur->data;
                GtkTreeIter i;
                gtk_tree_model_get_iter(m, &i, path);
                
                /* get this element */
                NIFTYLED_TYPE t;
                gpointer *e;
                gtk_tree_model_get(m, &i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &e,  -1);

                /* run user function */
                func(t, e);
        }
        
        /* free list */
        g_list_foreach(selected, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selected);
}


/** run function on last selected element */
static void _do_for_last_selected_element(void (*func)(NIFTYLED_TYPE t, gpointer *element))
{
        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));

        /* something selected? */
        GList *selected;
        GtkTreeModel *m;
        if(!(selected = gtk_tree_selection_get_selected_rows(selection, &m)))
                return;
        
        GtkTreePath *path = (GtkTreePath *) g_list_last(selected)->data;
        GtkTreeIter i;
        gtk_tree_model_get_iter(m, &i, path);
        
        /* get this element */
        NIFTYLED_TYPE t;
        gpointer *p;
        gtk_tree_model_get(m, &i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &p,  -1);

        func(t, p);
}

/** @todo: eliminate. get last item if one or more elements are highlighted (or NULL) */
static gboolean _tree_gt_selected_element(NIFTYLED_TYPE *t, gpointer **p)
{
        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
        
        *t = 0;
        *p = NULL;
        
        /* something selected? */
        GList *selected;
        GtkTreeModel *m;
        if(!(selected = gtk_tree_selection_get_selected_rows(selection, &m)))
                return FALSE;
        
        GtkTreePath *path = (GtkTreePath *) g_list_last(selected)->data;
        GtkTreeIter i;
        gtk_tree_model_get_iter(m, &i, path);
        
        /* get this element */
        gtk_tree_model_get(m, &i, C_SETUP_TYPE, t, C_SETUP_ELEMENT, p,  -1);

        
        /* free list */
        g_list_foreach(selected, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selected);

        return TRUE;
}


/** build setup-tree according to current setup */
static void _tree_build()
{
        /**
         * add every hardware-node (+ children) 
         * to the setup-treeview 
         */
        LedHardware *h;
        for(h = led_settings_hardware_get_first(setup_get_current()); h; h = led_hardware_get_next_sibling(h))
        {         
                _tree_append_hardware(tree_store, h);
        }

        /* walk complete tree & collapse or expand element */
        GtkTreeModel *m = gtk_tree_view_get_model(tree_view);
        gtk_tree_model_foreach(m, _foreach_element_refresh_collapse, NULL);
        gtk_tree_model_foreach(m, _foreach_element_refresh_highlight, NULL);

        /* show treeview */
        gtk_widget_show(GTK_WIDGET(tree_view));
        
}

/******************************************************************************
 ******************************************************************************/



/** clear setup tree */
void setup_tree_clear()
{
        if(!tree_store)
                return;
        
        gtk_tree_store_clear(tree_store);
}


/** refresh setup-tree to reflect changes to the setup */
void setup_tree_refresh()
{        
        /* clear tree */
        setup_tree_clear();

        /* rebuild tree */
        _tree_build();
}


/** getter for our widget */
GtkWidget *setup_tree_get_widget()
{
        return GTK_WIDGET(box);
}


/** initialize setup tree module */
gboolean  setup_tree_init()
{
        GtkBuilder *ui = ui_builder("niftyconf-setup-tree.ui");
        
        /* get widgets */
        if(!(box = GTK_BOX(gtk_builder_get_object(ui, "box"))))
                return FALSE;
        if(!(tree_store = GTK_TREE_STORE(gtk_builder_get_object(ui, "treestore"))))
                return FALSE;
        if(!(tree_view = GTK_TREE_VIEW(gtk_builder_get_object(ui, "treeview"))))
                return FALSE;
        
        /* set selection mode for setup tree */
        gtk_tree_selection_set_mode(
                gtk_tree_view_get_selection(tree_view), 
                GTK_SELECTION_MULTIPLE);

        
        /* initialize setup treeview */
        GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(ui, "column_element"));
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, renderer, TRUE);
        gtk_tree_view_column_add_attribute(col, renderer, "text", C_SETUP_TITLE);

        //g_object_unref(ui);
        
        return TRUE;
}



/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/
/** user collapsed tree row */
void on_setup_treeview_collapsed(GtkTreeView *tv, 
                                 GtkTreeIter *i, GtkTreePath *path, gpointer u)
{
        GtkTreeModel *m = gtk_tree_view_get_model(tv);
        gpointer *p;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &p,  -1);

        switch(t)
        {
                case T_LED_HARDWARE:
                {
                        hardware_tree_set_collapsed((NiftyconfHardware *) p, TRUE);
                        break;
                }

                case T_LED_TILE:
                {
                        tile_tree_set_collapsed((NiftyconfTile *) p, TRUE);
                        break;
                }
        }
}


/** user expanded tree row */
void on_setup_treeview_expanded(GtkTreeView *tv, 
                                 GtkTreeIter *i, GtkTreePath *path, gpointer u)
{
        GtkTreeModel *m;
        if(!(m = gtk_tree_view_get_model(tv)))
                return;
        
        gpointer *p;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &p,  -1);

        switch(t)
        {
                case T_LED_HARDWARE:
                {
                        hardware_tree_set_collapsed((NiftyconfHardware *) p, FALSE);
                        break;
                }

                case T_LED_TILE:
                {
                        tile_tree_set_collapsed((NiftyconfTile *) p, FALSE);
                        break;
                }
        } 
}


/** user selected another row */
void on_setup_treeview_cursor_changed(GtkTreeView *tv, gpointer u)
{
     
        /* nothing selected? */
        GtkTreeSelection *s = gtk_tree_view_get_selection(tv);
        if(gtk_tree_selection_count_selected_rows(s) <= 0)
        {
                setup_props_hide();
                return;
        }

        
        /* unhighlight all rows */
        _do_foreach_element(_foreach_unhighlight_element);
        
        /* set currently active element */
        _do_for_last_selected_element(_foreach_set_current_element);
        
        /* process all selected elements */
        _do_foreach_selected_element(_foreach_element_selected);
        

        //setup_redraw();
        //scene_redraw();
}




/** popup menu-entry selected */
gboolean on_popup_remove_hardware(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* remove all currently selected elements */
        _do_foreach_selected_element(_foreach_remove_hardware);
        
        /* refresh tree */
        setup_tree_refresh();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_remove_tile(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* remove all currently selected elements */
        _do_foreach_selected_element(_foreach_remove_tile);
                               
        /* refresh tree */
        setup_tree_refresh();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_remove_chain(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* remove all currently selected elements */
        _do_foreach_selected_element(_foreach_remove_chain);
                        
        /* refresh tree */
        setup_tree_refresh();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_add_hardware(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

                                
        /* create new niftyled hardware */
        LedHardware *h;
        if(!(h = led_hardware_new("Unnamed", "dummy")))
        {
                NFT_LOG(L_ERROR, "Failed to add new dummy-hardware");
                return FALSE;
        }
        
        /* register hardware to gui */
        NiftyconfHardware *hardware;
        if(!(hardware = hardware_register(h)))
        {
                NFT_LOG(L_ERROR, "Failed to register hardware to GUI");
                led_hardware_destroy(h);
                return FALSE;
        }

        /* append to end of setup */
        LedHardware *last;
        for(last = led_settings_hardware_get_first(setup_get_current());
            led_hardware_get_next_sibling(last);
            last = led_hardware_get_next_sibling(last));

        led_hardware_set_sibling(last, h);

        /* create config */
        if(!led_settings_create_from_hardware(setup_get_current(), h))
                return FALSE;
        
        /* refresh tree */
        setup_tree_refresh();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_add_tile(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* set currently active element */
        _do_for_last_selected_element(_foreach_set_current_element);
        
        /* create new tile */
        LedTile *n;
        if(!(n = led_tile_new()))
                return FALSE;

        
        /* different possible element types */
        switch(current_type)
        {
                /* currently selected element is a hardware-node */
                case T_LED_HARDWARE:
                {
                        /* get last tile of this hardware */
                        LedHardware *h = hardware_niftyled((NiftyconfHardware *) current_hw);
                        
                        /* does hw already have a tile? */
                        LedTile *tile;
                        if(!(tile = led_hardware_get_tile(h)))
                        {
                                led_hardware_set_tile(h, n);
                        }
                        else
                        {
                                led_tile_append_sibling(tile, n);
                        }
                        break;
                }

                /* currently selected element is a tile-node */
                case T_LED_TILE:
                {
                        LedTile *tile = tile_niftyled(current_tile);
                        led_tile_append_child(tile, n);
                        break;
                }
        }

        /* register new tile to gui */
        tile_register(n);

        /* create config */
        if(!led_settings_create_from_tile(setup_get_current(), n))
                return FALSE;
        
        /* refresh tree */
        setup_tree_refresh();
        
        return TRUE;
}


/** popup menu-entry selected */
gboolean on_popup_add_chain(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;
        
        /* set currently active element */
        _do_for_last_selected_element(_foreach_set_current_element);

        /* can only add chains to tiles */
        if(current_type != T_LED_TILE)
                return FALSE;
        
        /** create new chain @todo select format */
        LedChain *n;
        if(!(n = led_chain_new(0, "RGB u8")))
                return FALSE;
        
        /* attach chain to tile */
        LedTile *tile = tile_niftyled(current_tile);
        led_tile_set_chain(tile, n);

        /* create config */
        if(!led_settings_create_from_chain(setup_get_current(), n))
                return FALSE;
        
        /* register chain to gui */
        chain_register(n);
        
        /* refresh tree */
        setup_tree_refresh();
        
        return TRUE;
}


/** create & show setup-tree popup menu */
static void _tree_popup_menu(GtkWidget *w, GdkEventButton *e, gpointer u)
{

        /* set currently active element */
        _do_for_last_selected_element(_foreach_set_current_element);

        
        /* create new popup menu */
        GtkWidget *menu = gtk_menu_new ();
        g_signal_connect(menu, "deactivate", 
                         G_CALLBACK (gtk_widget_destroy), NULL);

        
        /* always generate "add hardware" menuitem (will be added toplevel only) */
        GtkWidget *menu_hw = gtk_image_menu_item_new_with_label("Add hardware");        
        gtk_image_menu_item_set_image(
                        GTK_IMAGE_MENU_ITEM(menu_hw), 
                        gtk_image_new_from_stock(
                                        "gtk-add", 
                                        GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_hw);
        g_signal_connect(menu_hw, "button-press-event",
                                        (GCallback) (on_popup_add_hardware), NULL);


        
        /* decide about type of currently selected element */
        switch(current_type)
        {
                /* nothing selected */
                case 0:
                {
                        break;
                }
                        
                case T_LED_HARDWARE:
                {
                        /* generate "add tile" menuitem */
                        GtkWidget *add_tile = gtk_image_menu_item_new_with_label("Add tile");
                        gtk_image_menu_item_set_image(
                                        GTK_IMAGE_MENU_ITEM(add_tile), 
                                        gtk_image_new_from_stock(
                                                        "gtk-add", 
                                                        GTK_ICON_SIZE_MENU));
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), add_tile);
                        g_signal_connect(add_tile, "button-press-event",
                                                (GCallback) on_popup_add_tile, NULL);

                        /* generate "remove hardware" menuitem */
                        GtkWidget *remove_hw = gtk_image_menu_item_new_with_label("Remove hardware");
                        gtk_image_menu_item_set_image(
                                        GTK_IMAGE_MENU_ITEM(remove_hw), 
                                        gtk_image_new_from_stock(
                                                        "gtk-remove", 
                                                        GTK_ICON_SIZE_MENU));
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove_hw);
                        g_signal_connect(remove_hw, "button-press-event",
                                                (GCallback) on_popup_remove_hardware, NULL);

                        /* generate "initialize/deinitialize hw" menuitem */

                        /* generate "move up" menuitem */

                        /* generate "move down" menuitem */

                        /* generate "cut" menuitem */
                        
                        /* generate "copy" menuitem */
                        
                        /* generate "paste" menuitem */
                        
                        break;
                }

                case T_LED_TILE:
                {
                        /* generate "add chain" menuitem */
                        GtkWidget *menu_chain = gtk_image_menu_item_new_with_label("Add chain");                
                        gtk_image_menu_item_set_image(
                                        GTK_IMAGE_MENU_ITEM(menu_chain), 
                                        gtk_image_new_from_stock(
                                                        "gtk-add", 
                                                        GTK_ICON_SIZE_MENU));
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_chain);
                        g_signal_connect(menu_chain, "button-press-event",
                                                (GCallback) on_popup_add_chain, NULL);
                        /* if tile already has chain, set widget unsensitive */
                        gtk_widget_set_sensitive(menu_chain, (gboolean) !led_tile_get_chain(tile_niftyled(current_tile)));
                        
                        /* generate "add tile" menuitem */
                        GtkWidget *menu_tile = gtk_image_menu_item_new_with_label("Add tile");
                        gtk_image_menu_item_set_image(
                                        GTK_IMAGE_MENU_ITEM(menu_tile), 
                                        gtk_image_new_from_stock(
                                                        "gtk-add", 
                                                        GTK_ICON_SIZE_MENU));
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_tile);
                        g_signal_connect(menu_tile, "button-press-event",
                                                (GCallback) on_popup_add_tile, NULL);

                        /* generate "remove tile" menuitem */
                        GtkWidget *remove_tile = gtk_image_menu_item_new_with_label("Remove tile");
                        gtk_image_menu_item_set_image(
                                        GTK_IMAGE_MENU_ITEM(remove_tile), 
                                        gtk_image_new_from_stock(
                                                        "gtk-remove", 
                                                        GTK_ICON_SIZE_MENU));
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove_tile);
                        g_signal_connect(remove_tile, "button-press-event",
                                                (GCallback) on_popup_remove_tile, NULL);

                        /* generate "remove chain" menuitem */
                        GtkWidget *remove_chain = gtk_image_menu_item_new_with_label("Remove chain");
                        gtk_image_menu_item_set_image(
                                        GTK_IMAGE_MENU_ITEM(remove_chain), 
                                        gtk_image_new_from_stock(
                                                        "gtk-remove", 
                                                        GTK_ICON_SIZE_MENU));
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove_chain);
                        g_signal_connect(remove_chain, "button-press-event",
                                                (GCallback) on_popup_remove_chain, NULL);
                        LedTile *tile = tile_niftyled(current_tile);
                        gtk_widget_set_sensitive(remove_chain, 
                                        (gboolean) led_tile_get_chain(tile));
                        
                        /* generate "move up" menuitem */

                        /* generate "move down" menuitem */

                        /* generate "cut" menuitem */
                        
                        /* generate "copy" menuitem */
                        
                        /* generate "paste" menuitem */
                        
                        break;
                }

                case T_LED_CHAIN:
                {
                        /* generate "remove chain" menuitem */

                        /* generate "cut" menuitem */
                        
                        /* generate "copy" menuitem */
                        
                        /* generate "paste" menuitem */
                        break;
                }

                default:
                {
                        NFT_LOG(L_ERROR, "Unknown element-type selected (%d)", current_type);
                }
        }

        
        /* set event-time */
        int button, event_time;
        if(e)
        {
                button = e->button;
                event_time = e->time;
        }
        else
        {
                button = 0;
                event_time = gtk_get_current_event_time();
        }

        
        /* attach menu to treeview */
        gtk_menu_attach_to_widget(GTK_MENU(menu), w, NULL);
        /* draw... */
        gtk_widget_show_all(menu);
        /* popup... */
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 
                       button, event_time);
}


/** mouseclick over element-tree */
gboolean on_setup_treeview_button_pressed(GtkTreeView *t, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if(e->type != GDK_BUTTON_PRESS)
                return FALSE;

        
        /* what kind of button pressed? */
        switch(e->button)
        {
                case 3:
                {
                        _tree_popup_menu(GTK_WIDGET(t), e, u);
                        return TRUE;
                }
        }
    
        return FALSE;
}


/** request to generate popup-menu */
gboolean on_setup_treeview_popup(GtkWidget *t, gpointer u)
{
        _tree_popup_menu(GTK_WIDGET(t), NULL, u);
        return TRUE;
}


