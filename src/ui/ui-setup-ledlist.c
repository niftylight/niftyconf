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
#include "ui/ui.h"
#include "ui/ui-renderer.h"
#include "ui/ui-setup-props.h"
#include "live-preview/live-preview.h"
#include "elements/element-led.h"
#include "elements/element-chain.h"
#include "renderer/renderer.h"
#include "renderer/renderer-led.h"




/* columns for our listview */
typedef enum
{
        /* (gint) NIFTYLED_TYPE of element */
        C_CHAIN_LED = 0,
        /* (gpointer) to NiftyconfLed descriptor */
        C_CHAIN_ELEMENT,
        NUM_CHAIN_COLS,
} CHAIN_LISTVIEW_COLUMNS;


/** GtkBuilder for this module */
static GtkBuilder *_builder;
/** narf! */
static bool _clear_in_progress;




static void on_selection_changed(GtkTreeSelection * treeselection,
                                 gpointer u);


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


/** build list of Leds */
static void _build(NiftyconfChain * c)
{
        /* rebuild */
        LedCount i;
        for(i = 0; i < led_chain_get_ledcount(chain_niftyled(c)); i++)
        {
                Led *led = led_chain_get_nth(chain_niftyled(c), i);
                GtkTreeIter iter;
                gtk_list_store_append(GTK_LIST_STORE(UI("liststore")), &iter);
                gtk_list_store_set(GTK_LIST_STORE(UI("liststore")), &iter,
                                   C_CHAIN_LED, i,
                                   C_CHAIN_ELEMENT, led_get_privdata(led),
                                   -1);

                led_set_highlighted(led_get_privdata(led), false);
                renderer_led_damage(led_get_privdata(led));
        }

        gtk_widget_show(GTK_WIDGET(UI("treeview")));
}

/******************************************************************************
 ******************************************************************************/

/** getter for list widget */
GtkWidget *ui_setup_ledlist_get_widget()
{
        return GTK_WIDGET(UI("box"));
}


/** clear list */
void ui_setup_ledlist_clear()
{
        _clear_in_progress = true;

        gtk_list_store_clear(GTK_LIST_STORE(UI("liststore")));

        _clear_in_progress = false;
}


/** rebuild list */
void ui_setup_ledlist_refresh(NiftyconfChain * c)
{
        if(!c)
                return;

        /* clear ledlist */
        ui_setup_ledlist_clear();

        _build(c);

        /* redraw */
        ui_renderer_all_queue_draw();
}





/** initialize this module */
gboolean ui_setup_ledlist_init()
{
        _builder = ui_builder("niftyconf-setup-ledlist.ui");


        /* set selection mode for tree */
        GtkTreeSelection *selection = gtk_tree_view_get_selection
                (GTK_TREE_VIEW(UI("treeview")));
        gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

        /* connect signal handler */
        g_signal_connect(selection, "changed",
                         G_CALLBACK(on_selection_changed), NULL);

        /* initialize setup treeview */
        GtkTreeViewColumn *col = GTK_TREE_VIEW_COLUMN(UI("column_led"));
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, renderer, true);
        gtk_tree_view_column_add_attribute(col, renderer, "text",
                                           C_CHAIN_LED);

        return true;
}

/** deinitialize this module */
void ui_setup_ledlist_deinit()
{
        g_object_unref(_builder);
}

/** run function on every selected tree-element (multiple selections) */
void
ui_setup_ledlist_do_foreach_selected_element(void (*func)
                                             (NiftyconfLed * led, void *u),
                                             void *u)
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
                LedCount current_pos;
                NiftyconfLed *current_led;
                gtk_tree_model_get(m, &i, C_CHAIN_LED, &current_pos,
                                   C_CHAIN_ELEMENT, &current_led, -1);

                /* run user function */
                func(current_led, u);
        }

        /* free list */
        g_list_foreach(selected, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(selected);
}




/** run function on every tree-element */
void ui_setup_ledlist_do_foreach_element(void (*func) (NiftyconfLed * led))
{
        /* get model */
        GtkTreeModel *m =
                gtk_tree_view_get_model(GTK_TREE_VIEW(UI("treeview")));
        GtkTreeIter iter;
        if(!gtk_tree_model_iter_nth_child(m, &iter, NULL, 0))
                return;

        do
        {
                /* get this element */
                NiftyconfLed *current_led;
                gtk_tree_model_get(m, &iter, C_CHAIN_ELEMENT, &current_led,
                                   -1);

                /* launch function */
                func(current_led);
        }
        while(gtk_tree_model_iter_next(m, &iter));
}

/******************************************************************************
 ***************************** CALLBACKS ************************************
 ******************************************************************************/

/** function to process an element that is currently selected */
static void _element_selected(GtkTreeModel * m,
                              GtkTreePath * p, GtkTreeIter * i, gpointer data)
{
        /* get element represented by this row */
        gpointer *element;
        LedCount n;
        gtk_tree_model_get(m, i, C_CHAIN_LED, &n, C_CHAIN_ELEMENT, &element,
                           -1);
        NiftyconfLed *l = (NiftyconfLed *) element;

        led_set_highlighted(l, true);
        renderer_led_damage(l);
}


/** deselect all Leds */
static void _foreach_unhighlight(NiftyconfLed * led)
{
        led_set_highlighted(led, false);
        renderer_led_damage(led);
}


/** selection changed */
static void on_selection_changed(GtkTreeSelection * selection, gpointer u)
{
        if(_clear_in_progress)
                return;

        // GtkTreeModel *m = gtk_tree_view_get_model(tv);

        /* unhighlight all elements */
        // GtkTreeIter i;
        // gtk_tree_model_get_iter_root(m, &i);
        // _walk_tree(&i, _unhighlight_element);

        /* clear preview */
        live_preview_clear();
        ui_setup_ledlist_do_foreach_element(_foreach_unhighlight);

        /* process all selected elements */
        gtk_tree_selection_selected_foreach(selection, _element_selected,
                                            NULL);

        /* get last selected element */
        GList *selected;
        GtkTreeModel *m;
        if(!(selected = gtk_tree_selection_get_selected_rows(selection, &m)))
                return;

        GtkTreePath *path = (GtkTreePath *) g_list_last(selected)->data;
        GtkTreeIter iter;
        gtk_tree_model_get_iter(m, &iter, path);

        /* get this element */
        LedCount i;
        gpointer *p;
        gtk_tree_model_get(m, &iter, C_CHAIN_LED, &i, C_CHAIN_ELEMENT, &p,
                           -1);

        /* show property dialog for this led */
        ui_setup_props_hide();
        ui_setup_props_led_show((NiftyconfLed *) p);

        /* refresh live hardware preview */
        live_preview_show();

        /* redraw */
        ui_renderer_all_queue_draw();
}
