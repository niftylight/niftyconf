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


#include <niftyled.h>
#include <gtk/gtk.h>
#include "ui/ui.h"
#include "ui/ui-renderer.h"
#include "ui/ui-setup.h"
#include "ui/ui-setup-props.h"
#include "ui/ui-setup-tree.h"
#include "ui/ui-setup-ledlist.h"
#include "ui/ui-hardware.h"
#include "ui/ui-clipboard.h"
#include "ui/ui-log.h"
#include "live-preview/live-preview.h"
#include "elements/element-setup.h"
#include "renderer/renderer-setup.h"
#include "renderer/renderer-tile.h"
#include "renderer/renderer-chain.h"
#include "prefs/prefs.h"


/** @todo improve design - i hate this */


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
} SETUP_TREEVIEW_COLUMNS;



/** GtkBuilder for this module */
static GtkBuilder *_builder;

/* type of currently selected element */
static NIFTYLED_TYPE _current_type;
/* currently selected element */
static NiftyconfHardware *_current_hw;
static NiftyconfTile *_current_tile;
static NiftyconfChain *_current_chain;
/** narf! */
static bool _clear_in_progress;



/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

static void on_selection_changed(GtkTreeSelection * selection, gpointer u);


/** configure this module from preferences */
static NftResult _this_from_prefs(NftPrefs * prefs,
                                  void **newObj,
                                  NftPrefsNode * node, void *userptr)
{
        /* dummy object to return (we don't actually create an object) */
        *newObj = (void *) 1;


        /* restore paned position from prefs */
        gint pos = 0;
        if(nft_prefs_node_prop_int_get(node, "paned_position", &pos))
        {
                /* set position */
                gtk_paned_set_position(GTK_PANED(ui("paned")), pos);
        }

        return NFT_SUCCESS;
}


/** save current configuration to preferences */
static NftResult _this_to_prefs(NftPrefs * prefs,
                                NftPrefsNode * newNode,
                                void *obj, void *userptr)
{
        /* store paned position in prefs */
        gint pos = gtk_paned_get_position(GTK_PANED(ui("paned")));
        if(!nft_prefs_node_prop_int_set(newNode, "paned_position", pos))
                return NFT_FAILURE;

        return NFT_SUCCESS;
}


/** helper to append element to treeview */
static void _tree_append_chain(GtkTreeStore * s,
                               LedChain * c, GtkTreeIter * parent)
{
        NiftyconfChain *chain = led_chain_get_privdata(c);

        /* don't add an element that's not registered */
        if(!chain)
                return;

        /* create name */
        char title[64];
        snprintf(title, sizeof(title), "%ld LED chain",
                 led_chain_get_ledcount(c));

        GtkTreeIter i;
        gtk_tree_store_append(s, &i, parent);
        gtk_tree_store_set(s, &i,
                           C_SETUP_TYPE, LED_CHAIN_T,
                           C_SETUP_TITLE, title, C_SETUP_ELEMENT,
                           (gpointer) chain, -1);
}


/** helper to append element to treeview */
static void _tree_append_tile(GtkTreeStore * s,
                              LedTile * t, GtkTreeIter * parent)
{
        NiftyconfTile *tile = led_tile_get_privdata(t);

        /* don't add an element that's not registered */
        if(!tile)
                return;

        /* get dimensions */
        LedFrameCord w, h;
        led_tile_get_dim(t, &w, &h);

        /* create name */
        char title[64];
        snprintf(title, sizeof(title), "%dx%d tile", w, h);

        GtkTreeIter i;
        gtk_tree_store_append(s, &i, parent);
        gtk_tree_store_set(s, &i,
                           C_SETUP_TYPE, LED_TILE_T,
                           C_SETUP_TITLE, title, C_SETUP_ELEMENT,
                           (gpointer) tile, -1);

        /* append chain if there is one */
        LedChain *c;
        if((c = led_tile_get_chain(t)))
        {
                _tree_append_chain(s, c, &i);
        }

        /* append children of this tile */
        LedTile *child;
        for(child = led_tile_get_child(t);
            child; child = led_tile_list_get_next(child))
        {
                _tree_append_tile(s, child, &i);
        }

}


/** helper to append element to treeview */
static void _tree_append_hardware(GtkTreeStore * s, LedHardware * h)
{
        NiftyconfHardware *hardware = led_hardware_get_privdata(h);

        /* don't add an element that's not registered */
        if(!hardware)
                return;

        GtkTreeIter i;
        gtk_tree_store_append(s, &i, NULL);
        gtk_tree_store_set(s, &i,
                           C_SETUP_TYPE, LED_HARDWARE_T,
                           C_SETUP_TITLE, led_hardware_get_name(h),
                           C_SETUP_ELEMENT, (gpointer) hardware, -1);

                /** append chain */
        _tree_append_chain(s, led_hardware_get_chain(h), &i);

                /** append all tiles */
        LedTile *t;
        for(t = led_hardware_get_tile(h); t; t = led_tile_list_get_next(t))
        {
                _tree_append_tile(s, t, &i);
        }
}


/** either collapse or expand a row of the setup-tree */
static gboolean _foreach_element_refresh_collapse(GtkTreeModel * model,
                                                  GtkTreePath * path,
                                                  GtkTreeIter * iter,
                                                  gpointer u)
{
        /* get niftyled element */
        gpointer *element;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(model, iter, C_SETUP_TYPE, &t, C_SETUP_ELEMENT,
                           &element, -1);

        gboolean collapsed = false;

        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        collapsed =
                                hardware_get_collapsed((NiftyconfHardware *)
                                                       element);
                        break;
                }

                case LED_TILE_T:
                {
                        collapsed =
                                tile_get_collapsed((NiftyconfTile *) element);
                        break;
                }

                default:
                {
                        break;
                }
        }

        if(collapsed)
                gtk_tree_view_collapse_row(GTK_TREE_VIEW(UI("treeview")),
                                           path);
        else
                gtk_tree_view_expand_row(GTK_TREE_VIEW(UI("treeview")), path,
                                         false);


        return false;
}


/** set selection-state from a row of the setup-tree */
static gboolean _foreach_element_refresh_highlight(GtkTreeModel * model,
                                                   GtkTreePath * path,
                                                   GtkTreeIter * iter,
                                                   gpointer u)
{
        /* get niftyled element */
        gpointer *element;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(model, iter, C_SETUP_TYPE, &t, C_SETUP_ELEMENT,
                           &element, -1);

        gboolean highlighted = false;

        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        highlighted =
                                hardware_get_highlighted((NiftyconfHardware *)
                                                         element);
                        break;
                }

                case LED_TILE_T:
                {
                        highlighted = tile_get_highlighted((NiftyconfTile *)
                                                           element);
                        break;
                }

                case LED_CHAIN_T:
                {
                        highlighted = chain_get_highlighted((NiftyconfChain *)
                                                            element);
                        break;
                }

                default:
                {
                        break;
                }
        }

        GtkTreeSelection *s =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(UI("treeview")));

        if(highlighted)
                gtk_tree_selection_select_iter(s, iter);
        else
                gtk_tree_selection_unselect_iter(s, iter);

        return false;
}


/** foreach: function to process an element that is currently selected */
static void _foreach_element_selected(NIFTYLED_TYPE t, gpointer e)
{
        ui_setup_props_hide();

        switch (t)
        {
                        /* hardware selected */
                case LED_HARDWARE_T:
                {
                        /* enable hardware related menus */
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_hardware_remove")),
                                                 true);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui("item_tile_add")), true);

                        /* disable non-hardware related menus */
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_tile_remove")),
                                                 false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_chain_add")), false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_chain_remove")),
                                                 false);

                        /* highlight hardware */
                        hardware_set_highlighted((NiftyconfHardware *) e,
                                                 true);

                        /* show hardware properties */
                        ui_setup_props_hardware_show((NiftyconfHardware *) e);


                        /* clear led-list */
                        ui_setup_ledlist_clear();
                        break;
                }

                        /* tile element selected */
                case LED_TILE_T:
                {
                        /* enable tile related menus */
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_tile_remove")),
                                                 true);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui("item_tile_add")), true);

                        /* disable non-tile related menus */
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_hardware_remove")),
                                                 false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_chain_add")),
                                                 (gboolean) !
                                                 led_tile_get_chain
                                                 (tile_niftyled
                                                  ((NiftyconfTile *) e)));
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_chain_remove")),
                                                 (led_tile_get_chain
                                                  (tile_niftyled
                                                   ((NiftyconfTile *) e)) !=
                                                  NULL));

                        /* highlight tile */
                        tile_set_highlighted((NiftyconfTile *) e, true);

                        ui_setup_props_tile_show((NiftyconfTile *) e);


                        /* redraw everything */
                        renderer_tile_damage((NiftyconfTile *) e);

                        /* clear led-list */
                        ui_setup_ledlist_clear();
                        break;
                }

                        /* chain element selected */
                case LED_CHAIN_T:
                {
                        /* disable non-chain related menus */
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_hardware_remove")),
                                                 false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_tile_add")), false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_tile_remove")),
                                                 false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_chain_add")), false);
                        gtk_widget_set_sensitive(GTK_WIDGET
                                                 (ui
                                                  ("item_chain_remove")),
                                                 false);

                        /* highlight chain */
                        chain_set_highlighted((NiftyconfChain *) e, true);

                        ui_setup_props_chain_show((NiftyconfChain *) e);

                        /* redraw everything */
                        renderer_chain_damage((NiftyconfChain *) e);

                        /* display led-list */
                        ui_setup_ledlist_refresh((NiftyconfChain *) e);

                        break;
                }

                default:
                {
                        g_warning
                                ("row with unknown tile selected. This is a bug!");
                }
        }


}


/** foreach: unhighlight element */
static void _foreach_unhighlight_element(NIFTYLED_TYPE t, gpointer e)
{
        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        hardware_set_highlighted((NiftyconfHardware *) e,
                                                 false);
                        break;
                }

                case LED_TILE_T:
                {
                        if(tile_get_highlighted((NiftyconfTile *) e))
                        {
                                tile_set_highlighted((NiftyconfTile *) e,
                                                     false);
                                renderer_tile_damage((NiftyconfTile *) e);
                        }
                        break;
                }

                case LED_CHAIN_T:
                {
                        if(chain_get_highlighted((NiftyconfChain *) e))
                        {
                                chain_set_highlighted((NiftyconfChain *) e,
                                                      false);
                                renderer_chain_damage((NiftyconfChain *) e);
                        }
                        break;
                }

                default:
                {
                        break;
                }
        }

}


/** foreach: highlight element */
static void _foreach_highlight_element(NIFTYLED_TYPE t, gpointer e)
{
        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        hardware_set_highlighted((NiftyconfHardware *) e,
                                                 true);
                        break;
                }

                case LED_TILE_T:
                {
                        tile_set_highlighted((NiftyconfTile *) e, true);
                        break;
                }

                case LED_CHAIN_T:
                {
                        chain_set_highlighted((NiftyconfChain *) e, true);
                        break;
                }

                default:
                {
                        break;
                }
        }
}


/** set currently active element */
static void _foreach_set_current_element(NIFTYLED_TYPE t, gpointer e)
{
        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        _current_hw = (NiftyconfHardware *) e;

                        /* refresh info view */
                        ui_info_hardware_refresh(_current_hw);
                        break;
                }

                case LED_TILE_T:
                {
                        _current_tile = (NiftyconfTile *) e;
                        break;
                }

                case LED_CHAIN_T:
                {
                        _current_chain = (NiftyconfChain *) e;
                        break;
                }

                default:
                {
                        break;
                }
        }

        _current_type = t;
}


/** recursion helper */
static void _do_foreach_iter(GtkTreeModel * m,
                             GtkTreeIter * i,
                             void (*func) (NIFTYLED_TYPE t, gpointer e))
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
                gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT,
                                   &e, -1);

                /* launch function */
                func(t, e);
        }
        while(gtk_tree_model_iter_next(m, i));
}


/** build setup-tree according to current setup */
static void _tree_build()
{
        /* get model */
        GtkTreeModel *m =
                gtk_tree_view_get_model(GTK_TREE_VIEW(UI("treeview")));
        g_object_ref(m);

        /* detach model from view */
        gtk_tree_view_set_model(GTK_TREE_VIEW(UI("treeview")), NULL);

                /**
		 * add every hardware-node (+ children)
		 * to the setup-treeview
		 */
        LedHardware *h;
        for(h = led_setup_get_hardware(setup_get_current()); h;
            h = led_hardware_list_get_next(h))
        {
                _tree_append_hardware(GTK_TREE_STORE(UI("treestore")), h);
        }

        /* re attach model to view */
        gtk_tree_view_set_model(GTK_TREE_VIEW(UI("treeview")), m);      /* Re-attach 
                                                                         * model 
                                                                         * to
                                                                         * view 
                                                                         */
        g_object_unref(m);

        /* walk complete tree & collapse or expand element */
        gtk_tree_model_foreach(m, _foreach_element_refresh_collapse, NULL);
        gtk_tree_model_foreach(m, _foreach_element_refresh_highlight, NULL);

}



static void _enable_actions_according_to_selected_element()
{
        /* enable/disable actions - according to new selection */
        GtkAction *a_hardware_remove = GTK_ACTION(ui("action_hardware_remove"));
        gtk_action_set_sensitive(a_hardware_remove, false);

        GtkAction *a_hardware_info = GTK_ACTION(ui("action_hardware_info"));
        gtk_action_set_sensitive(a_hardware_info, false);

        GtkAction *a_tile_add = GTK_ACTION(ui("action_tile_add"));
        gtk_action_set_sensitive(a_tile_add, false);

        GtkAction *a_tile_remove = GTK_ACTION(ui("action_tile_remove"));
        gtk_action_set_sensitive(a_tile_remove, false);

        GtkAction *a_chain_add = GTK_ACTION(ui("action_chain_add"));
        gtk_action_set_sensitive(a_chain_add, false);

        GtkAction *a_chain_remove = GTK_ACTION(ui("action_chain_remove"));
        gtk_action_set_sensitive(a_chain_remove, false);

        GtkAction *a_element_up = GTK_ACTION(ui("action_element_up"));
        gtk_action_set_sensitive(a_element_up, false);

        GtkAction *a_element_down = GTK_ACTION(ui("action_element_down"));
        gtk_action_set_sensitive(a_element_down, false);


        /* decide about type of currently selected element */
        switch (_current_type)
        {
                /* popup menu for hardware element */
                case LED_HARDWARE_T:
                {
                        /* enable "remove hardware" */
                        gtk_action_set_sensitive(a_hardware_remove, true);
                        /* enable "info" */
                        gtk_action_set_sensitive(a_hardware_info, true);
                        /* every hardware can have subtiles - enable "add tile" */
                        gtk_action_set_sensitive(a_tile_add, true);

                        /* get current hardware */
                        //~ LedHardware *hw = hardware_niftyled(_current_hw);
					
                        //~ /* if hardware has a previous sibling, enable "move up" */
                        //~ gtk_action_set_sensitive(a_element_up,
                           //~ (led_hardware_list_get_prev(hw) ? true : false));

                        //~ /* if hardware has a next sibling, enable "move down" */
                        //~ gtk_action_set_sensitive(a_element_down,
                           //~ (led_hardware_list_get_next(hw) ? true : false));
                        
                        break;
                }

                /* popup menu for tile element */
                case LED_TILE_T:
                {
                        /* every tile can have subtiles - enable "add tile" */
                        gtk_action_set_sensitive(a_tile_add, true);
                        /* enable "remove tile" */
                        gtk_action_set_sensitive(a_tile_remove, true);


                        /* get current tile */
                        LedTile *tile = tile_niftyled(_current_tile);

                        /* tile has a chain, enable "remove chain" menu */
                        gtk_action_set_sensitive(a_chain_remove,
                                  (led_tile_get_chain(tile) ? true : false));

                        /* if tile has no chain, enable "add chain" menu */
                        gtk_action_set_sensitive(a_chain_add,
                                  led_tile_get_chain(tile) ? false : true);

                        //~ /* if tile has a previous sibling, enable "move up" */
			//~ gtk_action_set_sensitive(a_element_up, 
                           //~ (led_tile_list_get_prev(tile) ? true : false));

                        //~ /* if tile has a next sibling, enable "move down" */
                        //~ gtk_action_set_sensitive(a_element_down,
                           //~ (led_tile_list_get_next(tile) ? true : false));

                        break;
                }

                /* popup menu for chain element */
                case LED_CHAIN_T:
                {
                        break;
                }

                default:
                {
                        NFT_LOG(L_DEBUG, "Unhandled element selected");
                        break;
                }
        }
}


/******************************************************************************
 ******************************************************************************/

/** run function on every tree-element */
void ui_setup_tree_do_foreach_element(void (*func) (NIFTYLED_TYPE t,
                                                    gpointer e))
{
        /* get model */
        GtkTreeModel *m =
                gtk_tree_view_get_model(GTK_TREE_VIEW(UI("treeview")));
        GtkTreeIter iter;
        gtk_tree_model_iter_nth_child(m, &iter, NULL, 0);
        _do_foreach_iter(m, &iter, func);
}


/** run function on every selected tree-element (multiple selections) */
void ui_setup_tree_do_foreach_selected_element(void (*func) (NIFTYLED_TYPE t,
                                                             gpointer
                                                             element))
{
        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(UI("treeview")));

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
                gtk_tree_model_get(m, &i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT,
                                   &e, -1);

                /* run user function */
                func(t, e);
        }

        /* free list */
        g_list_foreach(selected, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selected);
}


/** run function on last selected element */
void ui_setup_tree_do_for_last_selected_element(void (*func) (NIFTYLED_TYPE t,
                                                              gpointer
                                                              element))
{
        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(UI("treeview")));

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
        gtk_tree_model_get(m, &i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &p, -1);

        func(t, p);
}


/** get last of currently selected elements */
void ui_setup_tree_get_last_selected_element(NIFTYLED_TYPE * t,
                                             gpointer * element)
{
        *t = LED_INVALID_T;
        *element = NULL;

        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(UI("treeview")));

        /* something selected? */
        GList *selected;
        GtkTreeModel *m;
        if(!(selected = gtk_tree_selection_get_selected_rows(selection, &m)))
                return;

        GtkTreePath *path = (GtkTreePath *) g_list_last(selected)->data;
        GtkTreeIter i;
        gtk_tree_model_get_iter(m, &i, path);

        /* get this element */
        NIFTYLED_TYPE type;
        gpointer *pointer;
        gtk_tree_model_get(m, &i, C_SETUP_TYPE, &type, C_SETUP_ELEMENT,
                           &pointer, -1);

        *t = type;
        *element = pointer;
}


/** get first of currently selected elements */
void ui_setup_tree_get_first_selected_element(NIFTYLED_TYPE * t,
                                              gpointer * element)
{
        *t = LED_INVALID_T;
        *element = NULL;

        /* get current treeview selection */
        GtkTreeSelection *selection;
        selection =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(UI("treeview")));

        /* something selected? */
        GList *selected;
        GtkTreeModel *m;
        if(!(selected = gtk_tree_selection_get_selected_rows(selection, &m)))
                return;

        GtkTreePath *path = (GtkTreePath *) g_list_first(selected)->data;
        GtkTreeIter i;
        gtk_tree_model_get_iter(m, &i, path);

        /* get this element */
        NIFTYLED_TYPE type;
        gpointer *pointer;
        gtk_tree_model_get(m, &i, C_SETUP_TYPE, &type, C_SETUP_ELEMENT,
                           &pointer, -1);

        *t = type;
        *element = pointer;
}


/** unselect all but this element */
void ui_setup_tree_highlight_only(NIFTYLED_TYPE t, gpointer element)
{
        /* unselect all elements */
        ui_setup_tree_do_foreach_element(_foreach_unhighlight_element);

        /* highlight this element */
        _foreach_highlight_element(t, element);

        ui_setup_tree_refresh();
}


/** clear setup tree */
void ui_setup_tree_clear()
{
        if(!GTK_TREE_STORE(UI("treestore")))
                return;

        _clear_in_progress = true;
        gtk_tree_store_clear(GTK_TREE_STORE(UI("treestore")));
        _clear_in_progress = false;
}


/** refresh setup-tree to reflect changes to the setup */
void ui_setup_tree_refresh()
{
        /* clear tree */
        ui_setup_tree_clear();

        _clear_in_progress = true;

        /* rebuild tree */
        _tree_build();

        _clear_in_progress = false;

        /* redraw */
        renderer_setup_damage();
        ui_renderer_all_queue_draw();
}


/** getter for our widget */
GtkWidget *ui_setup_tree_get_widget()
{
        return GTK_WIDGET(UI("box"));
}


/** getter for treeview */
GtkTreeView *ui_setup_tree_view()
{
        return GTK_TREE_VIEW(UI("treeview"));
}


/** get type of currently selected element */
NIFTYLED_TYPE ui_setup_tree_current_element_type()
{
        return _current_type;
}


/** initialize setup tree module */
gboolean ui_setup_tree_init()
{
        if(!(_builder = ui_builder("niftyconf-setup-tree.ui")))
                return false;

        /* set selection mode for setup tree */
        GtkTreeSelection *selection =
                gtk_tree_view_get_selection(GTK_TREE_VIEW(UI("treeview")));
        gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

        /* connect signal handler */
        g_signal_connect(selection, "changed",
                         G_CALLBACK(on_selection_changed), NULL);

        /* initialize setup treeview */
        GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(UI("column_element"));
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, renderer, true);
        gtk_tree_view_column_add_attribute(col, renderer, "text",
                                           C_SETUP_TITLE);

        /* register prefs class for this module */
        if(!nft_prefs_class_register
           (prefs(), "ui-setup-tree", _this_from_prefs, _this_to_prefs))
                g_error("Failed to register prefs class for \"ui-setup-tree\"");

        return true;
}


/** deinitialize this module */
void ui_setup_tree_deinit()
{
        /* unregister prefs class */
        nft_prefs_class_unregister(prefs(), "ui-setup-tree");

        g_object_unref(_builder);
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** menuitem "collapse all" activated */
G_MODULE_EXPORT void on_action_tree_collapse_activate(GtkAction * i,
                                                      gpointer u)
{
        gtk_tree_view_collapse_all(ui_setup_tree_view());
}


/** menuitem "expand all" activated */
G_MODULE_EXPORT void on_action_tree_expand_activate(GtkAction * i, gpointer u)
{
        gtk_tree_view_expand_all(ui_setup_tree_view());
}


/** user collapsed tree row */
G_MODULE_EXPORT void on_setup_treeview_collapsed(GtkTreeView * tv,
                                                 GtkTreeIter * i,
                                                 GtkTreePath * path,
                                                 gpointer u)
{
        GtkTreeModel *m = gtk_tree_view_get_model(tv);
        gpointer *p;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &p, -1);

        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        hardware_set_collapsed((NiftyconfHardware *) p, true);
                        break;
                }

                case LED_TILE_T:
                {
                        tile_set_collapsed((NiftyconfTile *) p, true);
                        break;
                }

                default:
                {
                        break;
                }
        }
}


/** user expanded tree row */
G_MODULE_EXPORT void on_setup_treeview_expanded(GtkTreeView * tv,
                                                GtkTreeIter * i,
                                                GtkTreePath * path,
                                                gpointer u)
{
        GtkTreeModel *m;
        if(!(m = gtk_tree_view_get_model(tv)))
                return;

        gpointer *p;
        NIFTYLED_TYPE t;
        gtk_tree_model_get(m, i, C_SETUP_TYPE, &t, C_SETUP_ELEMENT, &p, -1);

        switch (t)
        {
                case LED_HARDWARE_T:
                {
                        hardware_set_collapsed((NiftyconfHardware *) p,
                                               false);
                        break;
                }

                case LED_TILE_T:
                {
                        tile_set_collapsed((NiftyconfTile *) p, false);
                        break;
                }

                default:
                {
                        break;
                }
        }
}


/** selection changed - new element(s) highlighted */
static void on_selection_changed(GtkTreeSelection * selection, gpointer u)
{
        if(_clear_in_progress)
                return;

        /* clear live preview */
        live_preview_clear();

        if(gtk_tree_selection_count_selected_rows(selection) <= 0)
        {
                ui_setup_props_hide();
                live_preview_show();
                return;
        }

        /* unhighlight all rows */
        ui_setup_tree_do_foreach_element(_foreach_unhighlight_element);

        /* set currently active element */
        ui_setup_tree_do_for_last_selected_element
                (_foreach_set_current_element);

        /* process all selected elements */
        ui_setup_tree_do_foreach_selected_element(_foreach_element_selected);


        /* enable/disable actions, according to currently selected element */
        _enable_actions_according_to_selected_element();

        /* update live preview */
        live_preview_show();

        /* redraw */
        ui_renderer_all_queue_draw();
}


/** create & show setup-tree popup menu */
static void _tree_popup_menu(GtkWidget * w, GdkEventButton * e, gpointer u)
{

        /* set currently active element */
        ui_setup_tree_do_for_last_selected_element
                (_foreach_set_current_element);

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

        /* get popup menu */
        GtkWidget *menu = GTK_WIDGET(ui("popup_menu"));

        /* draw... */
        gtk_widget_show_all(menu);
        /* popup... */
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                       button, event_time);
}


/** mouseclick over element-tree */
G_MODULE_EXPORT gboolean on_setup_treeview_button_pressed(GtkTreeView * t,
                                                          GdkEventButton * e,
                                                          gpointer u)
{
        /* only handle button-press events */
        if(e->type != GDK_BUTTON_PRESS)
                return false;


        /* what kind of button pressed? */
        switch (e->button)
        {
                /* RMB */
                case 3:
                {
                        _tree_popup_menu(GTK_WIDGET(t), e, u);
                        return true;
                }
        }

        return false;
}


/** request to generate popup-menu */
G_MODULE_EXPORT gboolean on_setup_treeview_popup(GtkWidget * t, gpointer u)
{
        _tree_popup_menu(GTK_WIDGET(t), NULL, u);
        return true;
}
/** menuitem "move up" selected */
G_MODULE_EXPORT void on_action_element_up_activate(GtkAction * a, gpointer u)
{
        switch(_current_type)
        {
                case LED_HARDWARE_T:
                {
                        //~ LedHardware *h = hardware_niftyled(_current_hw);
                        //~ LedHardware *prev = led_hardware_list_get_prev(h);

                        
                        //~ if(!led_hardware_list_swap(h, ))
                        //~ {
                                //~ NFT_LOG(L_ERROR, "Failed to swap hardware elements.");
                        //~ }
                        break;
                }

                case LED_TILE_T:
                {
                        //~ LedTile *t = tile_niftyled(_current_tile);
                        //~ if(!led_tile_list_swap(t, led_tile_list_get_prev(t)))
                        //~ {
                                //~ NFT_LOG(L_ERROR, "Failed to swap tile elements");
                        //~ }
                        break;
                }

                default:
                {
                        NFT_LOG(L_WARNING, "attempt to move up "
                                         "unknown type \"%d\"", _current_type);
                        break;
                }
        }

        /* refresh tree */
        ui_setup_tree_refresh();
        _enable_actions_according_to_selected_element();
}


/** menuitem "move down" selected */
G_MODULE_EXPORT void on_action_element_down_activate(GtkAction * a, gpointer u)
{
        switch(_current_type)
        {
                case LED_HARDWARE_T:
                {
                        //~ LedHardware *h = hardware_niftyled(_current_hw);
                        //~ if(!led_hardware_list_swap(h, led_hardware_list_get_next(h)))
                        //~ {
                                //~ NFT_LOG(L_ERROR, "Failed to swap hardware elements.");
                        //~ }
                        break;
                }

                case LED_TILE_T:
                {
                        //~ LedTile *t = tile_niftyled(_current_tile);
                        //~ if(!led_tile_list_swap(t, led_tile_list_get_next(t)))
                        //~ {
                                //~ NFT_LOG(L_ERROR, "Failed to swap tile elements");
                        //~ }
                        break;
                }

                default:
                {
                        NFT_LOG(L_WARNING, "attempt to move up "
                                         "unknown type \"%d\"", _current_type);
                        break;
                }
        }

        /* refresh tree */
        ui_setup_tree_refresh();
        _enable_actions_according_to_selected_element();
}
