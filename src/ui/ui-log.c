/*
 * niftyconf - niftyled GUI
 * Copyright (C) 2011-2013 Daniel Hiepler <daniel@niftylight.de>
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

#include <stdarg.h>
#include <gtk/gtk.h>
#include <niftyled.h>
#include "ui/ui.h"
#include "ui/ui-log.h"
#include "prefs/prefs.h"




/** maximum length of log messages in bytes */
#define MAX_MSG_SIZE	2048

/** GtkBuilder for this module */
static GtkBuilder *_ui;

/* current log window position */
static gint _pos_x, _pos_y;

/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/

/** configure from preferences */
static NftResult _this_from_prefs(NftPrefs * prefs,
                                  void **newObj,
                                  NftPrefsNode * node, void *userptr)
{
        /* dummy object */
        *newObj = (void *) 1;

        /* window geometry */
        gint width = 0, height = 0;
        nft_prefs_node_prop_int_get(node, "x", &_pos_x);
        nft_prefs_node_prop_int_get(node, "y", &_pos_y);
        gtk_window_move(GTK_WINDOW(UI("window")), _pos_x, _pos_y);

        nft_prefs_node_prop_int_get(node, "width", &width);
        nft_prefs_node_prop_int_get(node, "height", &height);
        gtk_window_resize(GTK_WINDOW(UI("window")), width, height);


        /* log visible? */
        bool boolean = false;
        nft_prefs_node_prop_boolean_get(node, "window-visible", &boolean);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM
                                       (ui("item_log_win")), boolean);
        ui_log_show(boolean);

        /* log level */
        char *loglevel;
        if((loglevel = nft_prefs_node_prop_string_get(node, "loglevel")))
        {
                nft_log_level_set(nft_log_level_from_string(loglevel));
                nft_prefs_free(loglevel);

                /* set combobox to current loglevel */
                gtk_combo_box_set_active(GTK_COMBO_BOX(UI("combobox")),
                                         (gint) nft_log_level_get() - 1);
        }

        /* log flags */
        nft_prefs_node_prop_boolean_get(node, "show-file", &boolean);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                     (UI("checkbutton_file")), boolean);
        nft_prefs_node_prop_boolean_get(node, "show-line", &boolean);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                     (UI("checkbutton_line")), boolean);
        nft_prefs_node_prop_boolean_get(node, "show-function", &boolean);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
                                     (UI("checkbutton_function")), boolean);

        return NFT_SUCCESS;
}


/** save configuration to preferences */
static NftResult _this_to_prefs(NftPrefs * prefs,
                                NftPrefsNode * newNode,
                                void *obj, void *userptr)
{
        /* window geometry */
        gint width, height;
        gtk_window_get_size(GTK_WINDOW(UI("window")), &width, &height);
        if(!nft_prefs_node_prop_int_set(newNode, "width", width))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "height", height))
                return NFT_FAILURE;

        gtk_window_get_position(GTK_WINDOW(UI("window")), &_pos_x, &_pos_y);
        if(!nft_prefs_node_prop_int_set(newNode, "x", _pos_x))
                return NFT_FAILURE;
        if(!nft_prefs_node_prop_int_set(newNode, "y", _pos_y))
                return NFT_FAILURE;


        /* visible? */
        if(!nft_prefs_node_prop_boolean_set
           (newNode, "window-visible", ui_log_visible()))
                return NFT_FAILURE;

        /* loglevel */
        if(!nft_prefs_node_prop_string_set
           (newNode, "loglevel",
            (char *) nft_log_level_to_string(nft_log_level_get())))
                return NFT_FAILURE;

        /* logging flags */
        if(!nft_prefs_node_prop_boolean_set
           (newNode, "show-file",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                         (UI("checkbutton_file")))))
                return NFT_FAILURE;

        if(!nft_prefs_node_prop_boolean_set
           (newNode, "show-line",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                         (UI("checkbutton_line")))))
                return NFT_FAILURE;

        if(!nft_prefs_node_prop_boolean_set
           (newNode, "show-function",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                         (UI("checkbutton_function")))))
                return NFT_FAILURE;


        return NFT_SUCCESS;
}


static void _logger(void *userdata,
                    NftLoglevel level,
                    const char *file,
                    const char *func, int line, const char *msg)
{
        /* output this at current loglevel? */
        NftLoglevel lcur = nft_log_level_get();
        if(lcur > level)
                return;

        /* calc size */
        size_t size = 64;
        if(file)
                size += strlen(file);
        if(func)
                size += strlen(func);
        if(msg)
                size += strlen(msg);

        gchar *s;
        if(!(s = alloca(size)))
                return;

        snprintf(s, size, "(%s) ", nft_log_level_to_string(level));

        /* include filename? */
        if(file &&
           gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                        (UI("checkbutton_file"))))
        {

                strncat(s, file,
                        (size - strlen(s) > 0 ? size - strlen(s) : 0));
        }

        /* include line number? */
        if(gtk_toggle_button_get_active
           (GTK_TOGGLE_BUTTON(UI("checkbutton_line"))))
        {
                gchar *number;
                if(!(number = alloca(64)))
                        return;
                snprintf(number, 64, ":%d ", line);
                strncat(s, number,
                        (size - strlen(s) > 0 ? size - strlen(s) : 0));
        }
        else
        {
                strncat(s, " ",
                        (size - strlen(s) > 0 ? size - strlen(s) : 0));
        }

        /* include function name? */
        if(func &&
           gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                        (UI("checkbutton_function"))))
        {
                strncat(s, func,
                        (size - strlen(s) > 0 ? size - strlen(s) : 0));
                strncat(s, "(): ",
                        (size - strlen(s) > 0 ? size - strlen(s) : 0));
        }

        strncat(s, msg, (size - strlen(s) > 0 ? size - strlen(s) : 0));
        strncat(s, "\n", (size - strlen(s) > 0 ? size - strlen(s) : 0));

        GtkTextView *tv = GTK_TEXT_VIEW(userdata);
        GtkTextBuffer *buf = gtk_text_view_get_buffer(tv);
        GtkTextIter iter;
        gtk_text_buffer_get_end_iter(buf, &iter);
        gtk_text_buffer_insert(buf, &iter, s, -1);
}



/******************************************************************************
 ******************************************************************************/

/**
 *  show yes/no dialog and wat for answer
 */
gboolean ui_log_dialog_yesno(char *title, char *message, ...)
{
        if(!message)
                NFT_LOG_NULL(false);

        /* allocate mem to build message */
        char *tmp;
        if(!(tmp = alloca(MAX_MSG_SIZE)))
        {
                NFT_LOG_PERROR("alloca");
                return false;
        }

        /* build message */
        va_list ap;
        va_start(ap, message);

        /* print log-string */
        if(vsnprintf((char *) tmp, MAX_MSG_SIZE, message, ap) < 0)
        {
                NFT_LOG_PERROR("vsnprintf");
                return false;
        }

        va_end(ap);


        /* Create the widgets */
        GtkWidget *dialog, *label, *content_area;
        dialog = gtk_dialog_new_with_buttons(title,
                                             NULL,
                                             0,
                                             GTK_STOCK_NO,
                                             GTK_RESPONSE_REJECT,
                                             GTK_STOCK_YES,
                                             GTK_RESPONSE_ACCEPT, NULL);
        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        label = gtk_label_new(tmp);

        /* Add the label, and show everything we've added to the dialog. */
        gtk_container_add(GTK_CONTAINER(content_area), label);
        gtk_widget_show_all(dialog);

        /* wait for answer */
        gboolean result = false;
        if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
                result = true;

        gtk_widget_destroy(dialog);

        return result;
}




/**
 * show alert/error message
 *
 * @param message printable text that will be presented to the user
 */
void ui_log_alert_show(char *message, ...)
{
        /* just hide when message is NULL */
        if(!message)
        {
                gtk_widget_set_visible(GTK_WIDGET(UI("alert_dialog")), false);
                return;
        }

        /* allocate mem to build message */
        char *tmp;
        if(!(tmp = alloca(MAX_MSG_SIZE)))
        {
                NFT_LOG_PERROR("alloca");
                return;
        }

        /* build message */
        va_list ap;
        va_start(ap, message);

        /* print log-string */
        if(vsnprintf((char *) tmp, MAX_MSG_SIZE, message, ap) < 0)
        {
                NFT_LOG_PERROR("vsnprintf");
                return;
        }

        va_end(ap);

        /* putout message through niftyled log mechanism also */
        NFT_LOG(L_ERROR, "%s", tmp);

        /* set message */
        gtk_label_set_text(GTK_LABEL(UI("alert_label")), tmp);

        /* show dialog */
        gtk_widget_set_visible(GTK_WIDGET(UI("alert_dialog")), true);
}

/**
 * show/hide log window
 */
void ui_log_show(gboolean visible)
{
        /* save/restore window position */
        if(visible)
        {
                /* set position */
                gtk_window_move(GTK_WINDOW(UI("window")), _pos_x, _pos_y);
        }
        else
        {
                /* store position */
                gtk_window_get_position(GTK_WINDOW(UI("window")), &_pos_x,
                                        &_pos_y);
        }

        /* show/hide */
        gtk_widget_set_visible(GTK_WIDGET(UI("window")), visible);
}


/**
 * return whether log window is visible or not
 */
gboolean ui_log_visible()
{
        return gtk_widget_get_visible(GTK_WIDGET(UI("window")));
}


/**
 * build a string with valid loglevels
 */
const char *ui_log_loglevels()
{
        static char s[1024];

        NftLoglevel i;
        for(i = L_MAX + 1; i < L_MIN - 1; i++)
        {
                strcat(s, nft_log_level_to_string(i));
                if(i < L_MIN - 2)
                        strncat(s, ", ", sizeof(s));
        }

        return (const char *) s;
}


/**
 * initialize module
 */
gboolean ui_log_init()
{
        _ui = ui_builder("niftyconf-log.ui");

        /* initialize loglevel combobox */
        NftLoglevel i;
        for(i = L_MAX + 1; i < L_MIN - 1; i++)
        {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT
                                               (UI("combobox")),
                                               nft_log_level_to_string(i));
        }

        /* set combobox to current loglevel */
        gtk_combo_box_set_active(GTK_COMBO_BOX(UI("combobox")),
                                 (gint) nft_log_level_get() - 1);

        /* register our custom logger function */
        nft_log_func_register(_logger, UI("textview"));

        /* register prefs class for this module */
        if(!nft_prefs_class_register
           (prefs(), "ui-log", _this_from_prefs, _this_to_prefs))
                g_error("Failed to register prefs class for \"ui-log\"");

        return true;
}


/** deinitialize this module */
void ui_log_deinit()
{
        /* unregister prefs class */
        nft_prefs_class_unregister(prefs(), "ui-log");

        g_object_unref(_ui);
}

/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** menuitem "show log-window" toggled */
G_MODULE_EXPORT void on_toggleaction_log_show_toggled(GtkToggleAction * i,
                                                      gpointer u)
{
        ui_log_show(gtk_toggle_action_get_active(i));
}


/** close main window */
G_MODULE_EXPORT gboolean on_log_window_delete_event(GtkWidget * w,
                                                    GdkEvent * e)
{
        ui_log_show(false);
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION
                                     (ui("toggleaction_log_show")), false);

        return true;
}

/** close alert dialog */
G_MODULE_EXPORT gboolean on_alert_dialog_delete_event(GtkWidget * w,
                                                      GdkEvent * e)
{
        ui_log_alert_show(NULL);
        return true;
}

/** loglevel changed */
G_MODULE_EXPORT void on_log_combobox_changed(GtkComboBox * w, gpointer u)
{
        NftLoglevel l = (NftLoglevel) gtk_combo_box_get_active(w) + 1;

        if(!nft_log_level_set(l))
                g_warning("Failed to set loglevel: \"%s\" (%d)",
                          nft_log_level_to_string(l), (gint) l);
}

/** "clear" button pressed */
G_MODULE_EXPORT void on_log_button_clicked(GtkButton * b, gpointer u)
{
        GtkTextBuffer *buf =
                gtk_text_view_get_buffer(GTK_TEXT_VIEW(UI("textview")));
        gtk_text_buffer_set_text(buf, "", -1);
}

/** "dismiss" button in alert-dialog clicked */
G_MODULE_EXPORT void on_alert_dismiss_button_clicked(GtkButton * b,
                                                     gpointer u)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("alert_dialog")), false);
}
