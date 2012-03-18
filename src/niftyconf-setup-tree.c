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




/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** get last item if one or more elements are highlighted (or NULL) */
static void _tree_get_selection(NIFTYLED_TYPE *t, gpointer **p)
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
                return;
        
        GtkTreePath *path = (GtkTreePath *) g_list_last(selected)->data;
        GtkTreeIter i;
        gtk_tree_model_get_iter(m, &i, path);
        
        /* get this element */
        gtk_tree_model_get(m, &i, C_SETUP_TYPE, t, C_SETUP_ELEMENT, p,  -1);

        
        /* free list */
        g_list_foreach(selected, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selected);
        
}


/** function to process an element that is currently selected */
static void _tree_element_selected(GtkTreeModel *m, GtkTreePath *p, GtkTreeIter *i, gpointer data)
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
                        NiftyconfHardware *h = (NiftyconfHardware *) (element);
                        setup_props_hardware_show(h);

                        /* redraw everything */
                        //setup_redraw();
                        
                        /* clear led-list */
                        chain_ledlist_clear();
                        break;
                }

                /* tile element selected */
                case T_LED_TILE:
                {
                        NiftyconfTile *tile = (NiftyconfTile *) (element);
                        setup_props_tile_show(tile);

                        /* highlight tile */
                        //tile_set_highlight(tile, TRUE);
                        
                        /* redraw everything */
                        //setup_redraw();

                        /* clear led-list */
                        chain_ledlist_clear();
                        break;
                }

                /* chain element selected */
                case T_LED_CHAIN:
                {
                        NiftyconfChain *chain = (NiftyconfChain *) (element);
                        setup_props_chain_show(chain);

                        /* redraw everything */
                        //setup_redraw();
                        
                        /* display led-list */
                        chain_ledlist_rebuild(chain);
                        
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


/** either collapse or expand a row of the setup-tree */
gboolean _tree_element_set_collapse(GtkTreeModel *model, GtkTreePath *path,
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
        for(h = led_settings_hardware_get_first(setup_get_current()); h; h = led_hardware_get_next_sibling(h))
        {         
                _tree_append_hardware(tree_store, h);
        }

        /* show treeview */
        gtk_widget_show(GTK_WIDGET(tree_view));

        /* walk complete tree & collapse or expand element */
        GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
        gtk_tree_model_foreach(model, _tree_element_set_collapse, NULL);
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
/** user collapsed row */
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


/** user expanded row */
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
        gtk_tree_selection_selected_foreach(s, _tree_element_selected, NULL);

        //setup_redraw();
        //scene_redraw();
}




/** popup menu-entry selected */
gboolean on_popup_remove_hardware(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* get currently selected element */
        NIFTYLED_TYPE t;
        gpointer *element;
        _tree_get_selection(&t, &element);

        /* works only if hardware-element is selected */
        if(t != T_LED_HARDWARE)
                return FALSE;

        /* get hardware element */
        NiftyconfHardware *hw = (NiftyconfHardware *) element;
        _remove_hardware(hw);
        
        
        /* refresh tree */
        setup_tree_clear();
        setup_tree_rebuild();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_remove_tile(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* get currently selected element */
        NIFTYLED_TYPE type;
        gpointer *element;
        _tree_get_selection(&type, &element);

        /* works only if tile-element is selected */
        if(type != T_LED_TILE)
                return FALSE;

        /* get element */
        NiftyconfTile *tile = (NiftyconfTile *) element;
        _remove_tile(tile);
                        
        /* refresh tree */
        setup_tree_clear();
        setup_tree_rebuild();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_remove_chain(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* get currently selected element */
        NIFTYLED_TYPE type;
        gpointer *element;
        _tree_get_selection(&type, &element);

        /* works only if tile-element is selected */
        if(type != T_LED_TILE)
                return FALSE;

        /* get element */
        NiftyconfTile *tile = (NiftyconfTile *) element;
        LedTile *t = tile_niftyled(tile);

        /* if this tile has no chain, silently succeed */
        LedChain *c;
        if(!(c = led_tile_get_chain(t)))
                return TRUE;

        /* unregister from tile */
        led_tile_set_chain(t, NULL);
        
        /* unregister from gui */
        NiftyconfChain *chain = led_chain_get_privdata(c);
        _remove_chain(chain);
        
                
        /* refresh tree */
        setup_tree_clear();
        setup_tree_rebuild();

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
        setup_tree_clear();
        setup_tree_rebuild();

        return TRUE;
        
}


/** popup menu-entry selected */
gboolean on_popup_add_tile(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;

        /* get currently selected element */
        NIFTYLED_TYPE t;
        gpointer *element;
        _tree_get_selection(&t, &element);
        
        /* create new tile */
        LedTile *n;
        if(!(n = led_tile_new()))
                return FALSE;

        
        /* different possible element types */
        switch(t)
        {
                /* currently selected element is a hardware-node */
                case T_LED_HARDWARE:
                {
                        /* get last tile of this hardware */
                        LedHardware *h = hardware_niftyled((NiftyconfHardware *) element);
                        
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
                        LedTile *tile = tile_niftyled((NiftyconfTile *) element);
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
        setup_tree_clear();
        setup_tree_rebuild();
        
        return TRUE;
}


/** popup menu-entry selected */
gboolean on_popup_add_chain(GtkWidget *w, GdkEventButton *e, gpointer u)
{
        /* only handle button-press events */
        if((e->type != GDK_BUTTON_PRESS) || (e->button != 1))
                return FALSE;
        
        /* get currently selected element */
        NIFTYLED_TYPE t;
        gpointer *element;
        _tree_get_selection(&t, &element);

        /* can only add chains to tiles */
        if(t != T_LED_TILE)
                return FALSE;
        
        /** create new chain @todo select format */
        LedChain *n;
        if(!(n = led_chain_new(0, "RGB u8")))
                return FALSE;
        
        /* attach chain to tile */
        LedTile *tile = tile_niftyled((NiftyconfTile *) element);
        led_tile_set_chain(tile, n);

        /* create config */
        if(!led_settings_create_from_chain(setup_get_current(), n))
                return FALSE;
        
        /* register chain to gui */
        chain_register(n);
        
        /* refresh tree */
        setup_tree_clear();
        setup_tree_rebuild();
        
        return TRUE;
}


/** create & show setup-tree popup menu */
static void _tree_popup_menu(GtkWidget *w, GdkEventButton *e, gpointer u)
{

        /* get currently selected element from tree */
        NIFTYLED_TYPE t;
        gpointer *element;
        _tree_get_selection(&t, &element);

        
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
        switch(t)
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
                        LedTile *tile = tile_niftyled((NiftyconfTile *) element);
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
                        NFT_LOG(L_ERROR, "Unknown element-type selected (%d)", t);
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


