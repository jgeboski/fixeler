/*
 * Copyright 2011-2012 James Geboski <jgeboski@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>

#define FIXELER_UI_TIMEOUT 3000

typedef struct _Color
{
    guint8 r;
    guint8 g;
    guint8 b;
}
Color;

typedef struct _Fixeler
{
    gboolean running;
    GSource * source;
    guint timeout;
    gint mode;

    gboolean ui_hidden;
    GSource * ui_source;

    gint ci;

    GtkWidget * window;
    GtkWidget * toolbar;

    GtkWidget * modew;
    GtkWidget * timeoutw;

    GtkWidget * start;
    GtkWidget * stop;
}
Fixeler;

static GdkColor color_rgb[3] = {
    {0, 0xffff, 0, 0},
    {0, 0, 0xffff, 0},
    {0, 0, 0, 0xffff}
};

static GdkColor color_bw[2] = {
    {0, 0, 0, 0},
    {0xffff, 0xffff, 0xffff, 0xffff}
};

static GdkColor color_rb;
static Color color;

static gboolean fixeler_timeout_rgb(Fixeler * fixeler)
{
    if(fixeler->ci >= 3)
        fixeler->ci = 0;

    gtk_widget_modify_bg(fixeler->window, GTK_STATE_NORMAL,
        &color_rgb[fixeler->ci]);

    fixeler->ci++;
    return fixeler->running;
}

static gboolean fixeler_timeout_bw(Fixeler * fixeler)
{
    if(fixeler->ci >= 2)
        fixeler->ci = 0;

    gtk_widget_modify_bg(fixeler->window, GTK_STATE_NORMAL,
        &color_bw[fixeler->ci]);

    fixeler->ci++;
    return fixeler->running;
}

static gboolean fixeler_timeout_rb(Fixeler * fixeler)
{
    if(!fixeler->ci) {
        color_rb.red   = 255;
        color_rb.green = 0;
        color_rb.blue  = 0;
    }

    switch(fixeler->ci) {
        case 0:
            if(color.b < 255)
                color.b++;
            else
                fixeler->ci++;
            break;

        case 1:
            if(color.r > 0)
                color.r--;
            else
                fixeler->ci++;
            break;

         case 2:
            if(color.g < 255)
                color.g++;
            else
                fixeler->ci++;
            break;

        case 3:
            if(color.b > 0)
                color.b--;
            else
                fixeler->ci++;
            break;

        case 4:
            if(color.r < 255)
                color.r++;
            else
                fixeler->ci++;
            break;

        case 5:
            if(color.g > 0)
                color.g--;
            else
                fixeler->ci = 0;
            break;
    }

    color_rb.red   = color.r << 8;
    color_rb.green = color.g << 8;
    color_rb.blue  = color.b << 8;

    gtk_widget_modify_bg(fixeler->window, GTK_STATE_NORMAL, &color_rb);
    return fixeler->running;
}

static void fixeler_timeout_done(Fixeler * fixeler)
{
    gtk_widget_set_sensitive(fixeler->start,    TRUE);
    gtk_widget_set_sensitive(fixeler->timeoutw, TRUE);
    gtk_widget_set_sensitive(fixeler->modew,    TRUE);
    fixeler->source = NULL;
}

static void fixeler_start(GtkToolButton * widget, Fixeler * fixeler)
{
    GSourceFunc func;

    gtk_widget_set_sensitive(fixeler->start,    FALSE);
    gtk_widget_set_sensitive(fixeler->stop,     TRUE);
    gtk_widget_set_sensitive(fixeler->timeoutw, FALSE);
    gtk_widget_set_sensitive(fixeler->modew,    FALSE);

    fixeler->running = TRUE;

    switch(fixeler->mode) {
        case 0:
            func = (GSourceFunc) fixeler_timeout_rgb;
            break;

        case 1:
            func = (GSourceFunc) fixeler_timeout_bw;
            break;

        case 2:
            func = (GSourceFunc) fixeler_timeout_rb;
            break;

        default:
            g_return_if_reached();
    }

    if(fixeler->source != NULL)
        g_source_destroy(fixeler->source);

    fixeler->source = g_timeout_source_new(fixeler->timeout);
    g_source_set_callback(fixeler->source, func, fixeler,
        (GDestroyNotify) fixeler_timeout_done);
    g_source_attach(fixeler->source, NULL);
}

static void fixeler_stop(GtkToolButton * widget, Fixeler * fixeler)
{
    if(fixeler->source != NULL)
        g_source_destroy(fixeler->source);

    gtk_widget_set_sensitive(fixeler->stop, FALSE);
    fixeler->running = FALSE;
}

static void fixeler_decorate(GtkToggleButton * widget, Fixeler * fixeler)
{
    gtk_window_set_decorated(GTK_WINDOW(fixeler->window),
        !gtk_toggle_button_get_active(widget));
}

static void fixeler_fullscreen(GtkToggleButton * widget, Fixeler * fixeler)
{
    if(gtk_toggle_button_get_active(widget))
        gtk_window_fullscreen(GTK_WINDOW(fixeler->window));
    else
        gtk_window_unfullscreen(GTK_WINDOW(fixeler->window));
}

static void fixeler_mode_changed(GtkComboBox * widget, Fixeler * fixeler)
{
    fixeler->mode = gtk_combo_box_get_active(widget);

    if(fixeler->mode < 0)
        fixeler->mode = 0;
}

static void fixeler_timeout_changed(GtkSpinButton * widget, Fixeler * fixeler)
{
    fixeler->timeout = gtk_spin_button_get_value_as_int(widget);
}

GtkWidget * fixeler_toolbar(Fixeler * fixeler)
{
    GtkWidget * toolbar, * widget, * image;
    GtkComboBoxText * combo;
    GtkToolItem * item;

    toolbar = gtk_toolbar_new();

    item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    gtk_tool_item_set_tooltip_text(item, _("Start Fixeler"));
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);
    gtk_widget_set_sensitive(GTK_WIDGET(item), TRUE);

    g_signal_connect(item, "clicked", G_CALLBACK(fixeler_start), fixeler);
    fixeler->start = GTK_WIDGET(item);

    item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
    gtk_tool_item_set_tooltip_text(item, _("Stop Fixeler"));
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 1);
    gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);

    g_signal_connect(item, "clicked", G_CALLBACK(fixeler_stop), fixeler);
    fixeler->stop  = GTK_WIDGET(item);

    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 2);

    image  = gtk_image_new_from_stock(GTK_STOCK_FULLSCREEN,
                 GTK_ICON_SIZE_LARGE_TOOLBAR);
    widget = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(widget, _("Hide/show window decor"));
    gtk_container_add(GTK_CONTAINER(widget), image);
    g_signal_connect(widget, "toggled",
        G_CALLBACK(fixeler_decorate), fixeler);

    item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(item), widget);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 3);

    image  = gtk_image_new_from_stock(GTK_STOCK_FULLSCREEN,
                 GTK_ICON_SIZE_LARGE_TOOLBAR);
    widget = gtk_toggle_button_new();
    gtk_widget_set_tooltip_text(widget, _("Fullscreen"));
    gtk_container_add(GTK_CONTAINER(widget), image);
    g_signal_connect(widget, "toggled",
        G_CALLBACK(fixeler_fullscreen), fixeler);

    item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(item), widget);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 4);

    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 5);

    item = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_tool_item_set_tooltip_text(item, _("Close"));
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 6);
    g_signal_connect(item, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 7);

    widget = gtk_spin_button_new_with_range(1, 5000, 5);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 100);
    gtk_widget_set_tooltip_text(widget, _("Timeout (ms)"));
    g_signal_connect(widget, "value-changed",
        G_CALLBACK(fixeler_timeout_changed), fixeler);
    fixeler->timeoutw = widget;

    item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(item), widget);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 8);

    item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 9);

    combo  = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    widget = GTK_WIDGET(combo);
    gtk_combo_box_text_append_text(combo, _("Red, Green, Blue"));
    gtk_combo_box_text_append_text(combo, _("Black, White"));
    gtk_combo_box_text_append_text(combo, _("Rainbow"));

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), fixeler->mode);
    gtk_widget_set_tooltip_text(widget, _("Fixeler mode"));
    g_signal_connect(combo, "changed",
        G_CALLBACK(fixeler_mode_changed), fixeler);
    fixeler->modew = widget;

    item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(item), widget);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 10);

    gtk_widget_show_all(toolbar);

    fixeler->toolbar = toolbar;
    return toolbar;
}

static gboolean fixeler_ui_timeout(Fixeler * fixeler)
{
    gtk_widget_hide(fixeler->toolbar);
    fixeler->ui_hidden = TRUE;
    fixeler->ui_source = NULL;

    return FALSE;
}

static gboolean fixeler_window_motion(GtkWidget * widget,
                    GdkEvent * event, Fixeler * fixeler)
{
    if(fixeler->ui_hidden) {
        gtk_widget_show(fixeler->toolbar);
        fixeler->ui_hidden = FALSE;
        return FALSE;
    }

    if(fixeler->ui_source != NULL) {
        g_source_destroy(fixeler->ui_source);
        fixeler->ui_source = NULL;
    }

    if(!fixeler->running)
        return FALSE;

    fixeler->ui_source = g_timeout_source_new(FIXELER_UI_TIMEOUT);
    g_source_set_callback(fixeler->ui_source,
        (GSourceFunc) fixeler_ui_timeout, fixeler, NULL);
    g_source_attach(fixeler->ui_source, NULL);

    return FALSE;
}

void fixeler_window(Fixeler * fixeler)
{
    GtkWidget * window, * vbox, * widget;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    vbox   = gtk_vbox_new(FALSE, 0);

    gtk_window_set_title(GTK_WINDOW(window), "Fixeler");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 200);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    widget = fixeler_toolbar(fixeler);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

    g_signal_connect(window, "motion-notify-event",
        G_CALLBACK(fixeler_window_motion), fixeler);
    g_signal_connect(window, "destroy",
        G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    fixeler->window = window;
}

int main(gint argc, gchar * argv[])
{
    Fixeler * fixeler;
    GTimeVal tv;

    gtk_init(&argc, &argv);
    g_get_current_time(&tv);

    fixeler = g_slice_new0(Fixeler);

    fixeler->running    = FALSE;
    fixeler->source     = NULL;
    fixeler->timeout    = 100;
    fixeler->mode       = 0;
    fixeler->ui_hidden  = FALSE;
    fixeler->ui_source  = NULL;
    fixeler->ci         = 0;

    fixeler_window(fixeler);

    gtk_main();

    g_slice_free(Fixeler, fixeler);

    return 0;
}
