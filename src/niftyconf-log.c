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
#include <niftyled.h>
#include "niftyconf.h"
#include "niftyconf-ui.h"
#include "niftyconf-log.h"





/** GtkBuilder for this module */
static GtkBuilder *_ui;


/******************************************************************************
 ****************************** STATIC FUNCTIONS ******************************
 ******************************************************************************/


static void _logger(void *userdata, 
                    NftLoglevel level, 
                    const char * file, 
                    const char * func, 
                    int line, const char * msg)
{
        /* output this at current loglevel? */
	NftLoglevel lcur = nft_log_level_get();
        if (lcur > level)
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

	snprintf(s, size, "(%s) ", 
	         nft_log_level_to_string(level));

	/* include filename? */
	if(file && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(UI("checkbutton_file"))))
	{
                
                strncat(s, file, (size-strlen(s) > 0 ? size-strlen(s) : 0));
	}

	/* include line number? */
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(UI("checkbutton_line"))))
	{
		gchar *number;
		if(!(number = alloca(64)))
			return;
		snprintf(number, 64, ":%d ", line);
		strncat(s, number, (size-strlen(s) > 0 ? size-strlen(s) : 0));
	}
	else
	{
		strncat(s, " ", (size-strlen(s) > 0 ? size-strlen(s) : 0));
	}

	/* include function name? */
	if(func && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(UI("checkbutton_function"))))
	{
		strncat(s, func, (size-strlen(s) > 0 ? size-strlen(s) : 0));
		strncat(s, "(): ", (size-strlen(s) > 0 ? size-strlen(s) : 0));
	}

	strncat(s, msg, (size-strlen(s) > 0 ? size-strlen(s) : 0));
	strncat(s, "\n", (size-strlen(s) > 0 ? size-strlen(s) : 0));
	
	GtkTextView *tv = GTK_TEXT_VIEW(userdata);
	GtkTextBuffer *buf = gtk_text_view_get_buffer(tv);
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buf, &iter);
	gtk_text_buffer_insert(buf, &iter, s, -1);
        fprintf(stderr, "%s", s);
}



/******************************************************************************
 ******************************************************************************/

/**
 * show alert/error message
 *
 * @param message printable text that will be presented to the user
 */
void log_alert_show(char *message)
{
	if(!message)
	{
		gtk_widget_set_visible(GTK_WIDGET(UI("alert_dialog")), FALSE);
		return;
	}
	
	/* set message */
	gtk_label_set_text(GTK_LABEL(UI("alert_label")), message);
	
	/* show dialog */
	gtk_widget_set_visible(GTK_WIDGET(UI("alert_dialog")), TRUE);
}

/**
 * show/hide log window
 */
void log_show(gboolean visible)
{
        gtk_widget_set_visible(GTK_WIDGET(UI("window")), visible);        
}

/** 
 * build a string with valid loglevels 
 */
const char *log_loglevels()
{
        static char s[1024];

        NftLoglevel i;
        for(i = L_MAX+1; i<L_MIN-1; i++)
        {
                strcat(s, nft_log_level_to_string(i));
                if(i<L_MIN-2)
                        strncat(s, ", ", sizeof(s));
        }

        return (const char *) s;
}


/**
 * initialize module
 */
gboolean log_init()
{
        _ui = ui_builder("niftyconf-log.ui");

        /* initialize loglevel combobox */
        NftLoglevel i;        
        for(i = L_MAX+1; i<L_MIN-1; i++)
        {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(UI("combobox")), nft_log_level_to_string(i));                
        }

        /* set combobox to current loglevel */
        gtk_combo_box_set_active(GTK_COMBO_BOX(UI("combobox")), (gint) nft_log_level_get()-1);

        /* register our custom logger function */
        nft_log_func_register(_logger, UI("textview"));

        return TRUE;
}


/******************************************************************************
 ***************************** CALLBACKS **************************************
 ******************************************************************************/

/** close main window */
gboolean on_log_window_delete_event(GtkWidget *w, GdkEvent *e)
{
        log_show(FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(niftyconf_ui("item_log_win")), FALSE);
        return TRUE;
}

/** close alert dialog */
gboolean on_alert_dialog_delete_event(GtkWidget *w, GdkEvent *e)
{
	log_alert_show(NULL);
	return TRUE;
}

/** loglevel changed */
void on_log_combobox_changed(GtkComboBox *w, gpointer u)
{
	NftLoglevel l = (NftLoglevel) gtk_combo_box_get_active(w)+1;
	
	if(!nft_log_level_set(l))
		g_warning("Failed to set loglevel: \"%s\" (%d)",
		          nft_log_level_to_string(l), (gint) l);
}

/** "clear" button pressed */
void on_log_button_clicked(GtkButton *b, gpointer u)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(UI("textview")));
	gtk_text_buffer_set_text(buf, "", -1);
}

/** "dismiss" button in alert-dialog clicked */
void on_alert_dismiss_button_clicked(GtkButton *b, gpointer u)
{
	gtk_widget_set_visible(GTK_WIDGET(UI("alert_dialog")), FALSE);
}
