/* $Id$ */
static char const _copyright[] =
"Copyright © 2013-2015 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Coder */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";



#include <sys/wait.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#else
# include <gdk/gdkx.h>
#endif
#include <System.h>
#include <Desktop.h>
#include "simulator.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

/* constants */
#ifndef PROGNAME
# define PROGNAME		"simulator"
#endif
#ifndef XEPHYR_PROGNAME
# define XEPHYR_PROGNAME	"Xephyr"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR			PREFIX "/bin"
#endif
#ifndef DATADIR
# define DATADIR		PREFIX "/share"
#endif
#ifndef MODELDIR
# define MODELDIR		DATADIR "/" PACKAGE "/Simulator/models"
#endif

extern char ** environ;


/* Simulator */
/* private */
/* types */
typedef struct _SimulatorChild
{
	unsigned int source;
	GPid pid;
} SimulatorChild;

struct _Simulator
{
	char * model;
	char * title;
	char * command;

	Display * display;
	int dpi;
	int width;
	int height;

	unsigned int source;

	SimulatorChild xephyr;
	SimulatorChild * children;
	size_t children_cnt;

	/* widgets */
	GtkWidget * window;
	GtkWidget * toolbar;
	GtkWidget * socket;
};

typedef struct _SimulatorData
{
	Simulator * simulator;
	Config * config;
	GtkWidget * dpi;
	GtkWidget * width;
	GtkWidget * height;
} SimulatorData;


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
/* callbacks */
static void _simulator_on_button_clicked(GtkToolButton * button, gpointer data);

static void _simulator_on_child_watch(GPid pid, gint status, gpointer data);
static void _simulator_on_children_watch(GPid pid, gint status, gpointer data);
static void _simulator_on_close(gpointer data);
static gboolean _simulator_on_closex(gpointer data);
static void _simulator_on_plug_added(gpointer data);

static void _simulator_on_file_quit(gpointer data);
static void _simulator_on_file_run(gpointer data);
static void _simulator_on_view_toggle_debugging_mode(gpointer data);
static void _simulator_on_help_about(gpointer data);
static void _simulator_on_help_contents(gpointer data);


/* constants */
/* menubar */
static const DesktopMenu _simulator_file_menu[] =
{
	{ N_("_Run..."), G_CALLBACK(_simulator_on_file_run), NULL,
		GDK_CONTROL_MASK, GDK_KEY_R },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Quit"), G_CALLBACK(_simulator_on_file_quit), GTK_STOCK_QUIT,
		GDK_CONTROL_MASK, GDK_KEY_Q },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _simulator_view_menu[] =
{
	{ N_("Toggle _debugging mode"), G_CALLBACK(
			_simulator_on_view_toggle_debugging_mode),
		NULL, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _simulator_help_menu[] =
{
	{ N_("_Contents"), G_CALLBACK(_simulator_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("About"), G_CALLBACK(_simulator_on_help_about), GTK_STOCK_ABOUT, 0,
		0 },
#else
	{ N_("About"), G_CALLBACK(_simulator_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _simulator_menubar[] =
{
	{ N_("_File"), _simulator_file_menu },
	{ N_("_View"), _simulator_view_menu },
	{ N_("_Help"), _simulator_help_menu },
	{ NULL, NULL }
};


/* public */
/* functions */
/* simulator_new */
static int _new_chooser(Simulator * simulator);
static void _new_chooser_list(GtkTreeStore * store);
static void _new_chooser_list_vendor(GtkTreeStore * store, char const * vendor,
		GtkTreeIter * parent);
static void _new_chooser_load(SimulatorData * data, GtkWidget * combobox);
static void _new_chooser_on_changed(GtkWidget * widget, gpointer data);
static void _new_chooser_on_config(String const * section, void * data);
static int _new_load(Simulator * simulator, char const * model);
static Config * _new_load_config(char const * model);
/* callbacks */
static gint _new_chooser_list_sort(GtkTreeModel * model, GtkTreeIter * a,
		GtkTreeIter * b, gpointer data);
static gboolean _new_on_idle(gpointer data);
static gboolean _new_on_quit(gpointer data);
static gboolean _new_on_xephyr(gpointer data);

Simulator * simulator_new(SimulatorPrefs * prefs)
{
	Simulator * simulator;

	if((simulator = object_new(sizeof(*simulator))) == NULL)
		return NULL;
	simulator->model = NULL;
	simulator->title = NULL;
	simulator->command = NULL;
	if(prefs != NULL)
	{
		simulator->model = (prefs->model != NULL)
			? strdup(prefs->model) : NULL;
		simulator->title = (prefs->title != NULL)
			? strdup(prefs->title) : NULL;
		simulator->command = (prefs->command != NULL)
			? strdup(prefs->command) : NULL;
		/* check for errors */
		if((prefs->model != NULL && simulator->model == NULL)
				|| (prefs->title != NULL
					&& simulator->title == NULL)
				|| (prefs->command != NULL
					&& simulator->command == NULL))
		{
			simulator_delete(simulator);
			return NULL;
		}
	}
	simulator->xephyr.source = 0;
	simulator->xephyr.pid = -1;
	simulator->children = NULL;
	simulator->children_cnt = 0;
	simulator->source = 0;
	simulator->window = NULL;
	simulator->toolbar = NULL;
	/* set default values */
	simulator->display = NULL;
	_new_load(simulator, NULL);
	if(prefs != NULL && prefs->chooser != 0)
	{
		if(_new_chooser(simulator) != 0)
		{
			simulator_delete(simulator);
			return NULL;
		}
	}
	else
	{
		/* load the configuration */
		/* XXX no longer ignore errors */
		_new_load(simulator, simulator->model);
		simulator->source = g_idle_add(_new_on_idle, simulator);
	}
	return simulator;
}

static int _new_chooser(Simulator * simulator)
{
	GtkSizeGroup * lgroup;
	GtkSizeGroup * group;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkTreeStore * store;
	GtkTreeModel * model;
	GtkWidget * combobox;
	GtkTreeIter iter;
	GtkTreeIter siter;
	GtkCellRenderer * renderer;
	GtkWidget * widget;
	SimulatorData data;

	data.simulator = simulator;
	data.config = NULL;
	lgroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	dialog = gtk_dialog_new_with_buttons(_("Simulator profiles"),
			NULL, 0, GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	gtk_box_set_spacing(GTK_BOX(vbox), 4);
	/* profile */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Profile: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(lgroup, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	store = gtk_tree_store_new(3,
			G_TYPE_STRING,				/* filename */
			GDK_TYPE_PIXBUF,			/* icon */
			G_TYPE_STRING);				/* name */
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, NULL, 2, _("Custom profile"), -1);
	_new_chooser_list(store);
	model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(model),
			_new_chooser_list_sort, simulator, NULL);
	combobox = gtk_combo_box_new_with_model(model);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox), renderer,
			"pixbuf", 1, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combobox), renderer,
			"text", 2, NULL);
	if(gtk_tree_model_sort_convert_child_iter_to_iter(
				GTK_TREE_MODEL_SORT(model), &siter, &iter))
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combobox), &siter);
	g_signal_connect(combobox, "changed", G_CALLBACK(
				_new_chooser_on_changed), &data);
	gtk_box_pack_end(GTK_BOX(hbox), combobox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* dpi */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Resolution: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(lgroup, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	data.dpi = gtk_spin_button_new_with_range(48.0, 300.0, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data.dpi), simulator->dpi);
	gtk_size_group_add_widget(group, data.dpi);
	gtk_box_pack_end(GTK_BOX(hbox), data.dpi, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* width */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Width: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(lgroup, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	data.width = gtk_spin_button_new_with_range(120, 1600, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data.width),
			simulator->width);
	gtk_size_group_add_widget(group, data.width);
	gtk_box_pack_end(GTK_BOX(hbox), data.width, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* height */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Height: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(lgroup, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	data.height = gtk_spin_button_new_with_range(120, 1600, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data.height),
			simulator->height);
	gtk_size_group_add_widget(group, data.height);
	gtk_box_pack_end(GTK_BOX(hbox), data.height, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy(dialog);
		simulator->source = g_idle_add(_new_on_quit, simulator);
		return 0;
	}
	gtk_widget_hide(dialog);
	/* apply the values */
	simulator->dpi = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
				data.dpi));
	simulator->width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
				data.width));
	simulator->height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
				data.height));
	_new_chooser_load(&data, combobox);
	gtk_widget_destroy(dialog);
	simulator->source = g_idle_add(_new_on_idle, simulator);
	return 0;
}

static void _new_chooser_list(GtkTreeStore * store)
{
	GtkIconTheme * icontheme;
	GtkTreeIter iter;
	GtkTreeIter parent;
	int size = 16;
	char const models[] = MODELDIR;
	char const ext[] = ".conf";
	DIR * dir;
	struct dirent * de;
	size_t len;
	Config * config;
	char const * p;
	char const * q;
	String * title;
	GdkPixbuf * pixbuf = NULL;

	if((dir = opendir(models)) == NULL)
		return;
	icontheme = gtk_icon_theme_get_default();
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &size, &size);
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_name[0] == '.')
			continue;
		if((len = strlen(de->d_name)) <= sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
		if((config = _new_load_config(de->d_name)) == NULL)
			continue;
		if((p = config_get(config, NULL, "icon")) != NULL)
			pixbuf = gtk_icon_theme_load_icon(icontheme, p, size, 0,
					NULL);
		q = config_get(config, NULL, "model");
		if((p = config_get(config, NULL, "vendor")) != NULL)
		{
			_new_chooser_list_vendor(store, p, &parent);
			gtk_tree_store_append(store, &iter, &parent);
			title = string_new_append((q != NULL)
					? " " : de->d_name, q, NULL);
		}
		else
		{
			gtk_tree_store_append(store, &iter, NULL);
			title = string_new_append((p != NULL) ? p : "",
					(q != NULL) ? " " : de->d_name, q, NULL);
		}
		gtk_tree_store_set(store, &iter, 0, de->d_name, 1, pixbuf,
				2, title, -1);
		string_delete(title);
		if(pixbuf != NULL)
		{
			g_object_unref(pixbuf);
			pixbuf = NULL;
		}
		config_delete(config);
	}
	closedir(dir);
}

static void _new_chooser_list_vendor(GtkTreeStore * store, char const * vendor,
		GtkTreeIter * parent)
{
	GtkTreeModel * model = GTK_TREE_MODEL(store);
	GtkTreeIter iter;
	gboolean valid;
	gchar * v;
	int res;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, 2, &v, -1);
		res = strcmp(v, vendor);
		g_free(v);
		if(res == 0)
			break;
	}
	if(valid == TRUE && res == 0)
		*parent = iter;
	else
	{
		gtk_tree_store_append(store, parent, NULL);
		gtk_tree_store_set(store, parent, 2, vendor, -1);
	}
}

static void _new_chooser_load(SimulatorData * data, GtkWidget * combobox)
{
	GtkTreeModel * smodel;
	GtkTreeIter siter;
	GtkTreeModel * model;
	GtkTreeIter iter;
	gchar * profile;

	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox), &siter)
			== FALSE)
		return;
	smodel = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(
				smodel), &iter, &siter);
	model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(smodel));
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &profile, -1);
	if(profile != NULL
			&& (data->config = _new_load_config(profile)) != NULL)
	{
		config_foreach(data->config, _new_chooser_on_config, data);
		config_delete(data->config);
		data->config = NULL;
	}
	g_free(profile);
}

static void _new_chooser_on_changed(GtkWidget * widget, gpointer data)
{
	SimulatorData * d = data;
	GtkTreeIter siter;
	GtkTreeModel * smodel;
	GtkTreeIter iter;
	GtkTreeModel * model;
	gchar * name;

	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &siter) == FALSE)
		return;
	smodel = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	gtk_tree_model_sort_convert_iter_to_child_iter(
			GTK_TREE_MODEL_SORT(smodel), &iter, &siter);
	model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(smodel));
	gtk_tree_model_get(model, &iter, 0, &name, -1);
	if(_new_load(d->simulator, name) == 0)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi),
				d->simulator->dpi);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->width),
				d->simulator->width);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->height),
				d->simulator->height);
	}
	g_free(name);
}

static void _new_chooser_on_config(String const * section, void * data)
{
	SimulatorData * d = data;
	const String button[] = "button::";
	GtkWidget * image;
	GtkToolItem * toolitem;
	char const * p;
	char const * q;

	if(strncmp(section, button, sizeof(button) - 1) != 0)
		return;
	if(d->simulator->toolbar == NULL)
		d->simulator->toolbar = gtk_toolbar_new();
	if((p = config_get(d->config, section, "icon")) != NULL)
		image = gtk_image_new_from_icon_name(p,
				GTK_ICON_SIZE_LARGE_TOOLBAR);
	else
		image = NULL;
	p = config_get(d->config, section, "name");
	toolitem = gtk_tool_button_new(image, p);
	/* XXX memory leaks */
	if((p = config_get(d->config, section, "command")) != NULL)
			g_object_set_data(G_OBJECT(toolitem), "command",
					g_strdup(p));
	if((q = config_get(d->config, section, "keysym")) != NULL)
			g_object_set_data(G_OBJECT(toolitem), "keysym",
					g_strdup(q));
	if(p == NULL && q == NULL)
		gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);
	else
		g_signal_connect(toolitem, "clicked",
				G_CALLBACK(_simulator_on_button_clicked),
				d->simulator);
	gtk_toolbar_insert(GTK_TOOLBAR(d->simulator->toolbar), toolitem, -1);
}

static int _new_load(Simulator * simulator, char const * model)
{
	Config * config;
	char const * p;
	char * q;
	long l;
	char const * v;

	simulator->dpi = 96;
	simulator->width = 640;
	simulator->height = 480;
	if((config = _new_load_config(model)) == NULL)
		return -1;
	if((p = config_get(config, NULL, "dpi")) != NULL
			&& (l = strtol(p, &q, 10)) > 0
			&& p[0] != '\0' && *q == '\0')
		simulator->dpi = l;
	if((p = config_get(config, NULL, "width")) != NULL
			&& (l = strtol(p, &q, 10)) > 0
			&& p[0] != '\0' && *q == '\0')
		simulator->width = l;
	if((p = config_get(config, NULL, "height")) != NULL
			&& (l = strtol(p, &q, 10)) > 0
			&& p[0] != '\0' && *q == '\0')
		simulator->height = l;
	free(simulator->title);
	v = config_get(config, NULL, "vendor");
	if((p = config_get(config, NULL, "model")) != NULL)
		simulator->title = string_new_append((v != NULL) ? v : "",
				(v != NULL) ? " " : "", p, NULL);
	else
		simulator->title = NULL;
	config_delete(config);
	return 0;
}

static Config * _new_load_config(char const * model)
{
	Config * config;
	char * p;
	int res = -1;

	if(model == NULL)
		model = "default";
	/* load the selected model */
	if((config = config_new()) == NULL)
		return NULL;
	p = string_new_append(MODELDIR "/", model, ".conf", NULL);
	if(p != NULL)
		res = config_load(config, p);
	free(p);
	if(res != 0)
	{
		config_delete(config);
		return NULL;
	}
	return config;
}

static gint _new_chooser_list_sort(GtkTreeModel * model, GtkTreeIter * a,
		GtkTreeIter * b, gpointer data)
{
	gint ret;
	gchar * afilename;
	gchar * aname;
	gchar * bfilename;
	gchar * bname;

	gtk_tree_model_get(model, a, 0, &afilename, 2, &aname, -1);
	gtk_tree_model_get(model, b, 0, &bfilename, 2, &bname, -1);
	if(afilename == NULL && bfilename != NULL)
		ret = -1;
	else if(afilename != NULL && bfilename == NULL)
		ret = 1;
	else
		ret = strcmp(aname, bname);
	g_free(afilename);
	g_free(aname);
	g_free(bfilename);
	g_free(bname);
	return ret;
}

static gboolean _new_on_idle(gpointer data)
{
	Simulator * simulator = data;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;
	char * p;

	/* widgets */
	group = gtk_accel_group_new();
	simulator->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(simulator->window),
			"stock_cell-phone");
#endif
	gtk_widget_set_size_request(simulator->window, simulator->width,
			simulator->height);
	gtk_window_add_accel_group(GTK_WINDOW(simulator->window), group);
	if(simulator->title == NULL || (p = string_new_append(_("Simulator - "),
					simulator->title, NULL)) == NULL)
		gtk_window_set_title(GTK_WINDOW(simulator->window),
				_("Simulator"));
	else
	{
		gtk_window_set_title(GTK_WINDOW(simulator->window), p);
		free(p);
	}
	gtk_window_set_resizable(GTK_WINDOW(simulator->window), FALSE);
	g_signal_connect_swapped(simulator->window, "delete-event", G_CALLBACK(
				_simulator_on_closex), simulator);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	/* menubar */
	widget = desktop_menubar_create(_simulator_menubar, simulator, group);
	g_object_unref(group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	if(simulator->toolbar != NULL)
		gtk_box_pack_start(GTK_BOX(vbox), simulator->toolbar, FALSE,
				TRUE, 0);
	/* view */
	simulator->socket = gtk_socket_new();
	g_signal_connect_swapped(simulator->socket, "plug-added", G_CALLBACK(
				_simulator_on_plug_added), simulator);
	gtk_box_pack_start(GTK_BOX(vbox), simulator->socket, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(simulator->window), vbox);
	gtk_widget_show_all(simulator->window);
	simulator->source = g_idle_add(_new_on_xephyr, simulator);
	return FALSE;
}

static gboolean _new_on_quit(gpointer data)
{
	Simulator * simulator = data;

	simulator->source = 0;
	gtk_main_quit();
	return FALSE;
}

static gboolean _new_on_xephyr(gpointer data)
{
	Simulator * simulator = data;
	char * argv[] = { BINDIR "/" XEPHYR_PROGNAME, XEPHYR_PROGNAME,
		"-parent", NULL, "-dpi", NULL, ":1", NULL };
	char parent[16];
	char dpi[16];
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO
		| G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;

	simulator->source = 0;
	/* launch Xephyr */
	snprintf(parent, sizeof(parent), "%u", gtk_socket_get_id(
				GTK_SOCKET(simulator->socket)));
	argv[3] = parent;
	snprintf(dpi, sizeof(dpi), "%u", simulator->dpi);
	argv[5] = dpi;
	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL,
				&simulator->xephyr.pid, &error) == FALSE)
	{
		simulator_error(simulator, error->message, 1);
		g_error_free(error);
	}
	else
		simulator->xephyr.source = g_child_watch_add(
				simulator->xephyr.pid,
				_simulator_on_child_watch, simulator);
	return FALSE;
}


/* simulator_delete */
void simulator_delete(Simulator * simulator)
{
	size_t i;

	for(i = 0; i < simulator->children_cnt; i++)
	{
		g_source_remove(simulator->children[i].source);
		g_spawn_close_pid(simulator->children[i].pid);
	}
	free(simulator->children);
	if(simulator->source > 0)
		g_source_remove(simulator->source);
	if(simulator->display != NULL)
		XCloseDisplay(simulator->display);
	if(simulator->xephyr.pid > 0)
	{
		kill(simulator->xephyr.pid, SIGTERM);
		g_source_remove(simulator->xephyr.source);
		g_spawn_close_pid(simulator->xephyr.pid);
	}
	if(simulator->window != NULL)
		gtk_widget_destroy(simulator->window);
	free(simulator->command);
	free(simulator->title);
	free(simulator->model);
	object_delete(simulator);
}


/* simulator_error */
static int _error_text(char const * message, int ret);

int simulator_error(Simulator * simulator, char const * message, int ret)
{
	GtkWidget * dialog;

	if(simulator == NULL)
		return _error_text(message, ret);
	dialog = gtk_message_dialog_new(GTK_WINDOW(simulator->window),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
# if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, PROGNAME ": %s\n", message);
	return ret;
}


/* simulator_run */
int simulator_run(Simulator * simulator, char const * command)
{
	char const display[] = "DISPLAY=";
	char const display1[] = "DISPLAY=:1.0"; /* XXX may be wrong */
	char * argv[] = { "/bin/sh", "run", "-c", NULL, NULL };
	char ** envp = NULL;
	size_t i;
	char ** p;
	GSpawnFlags flags = G_SPAWN_DO_NOT_REAP_CHILD
		| G_SPAWN_SEARCH_PATH | G_SPAWN_FILE_AND_ARGV_ZERO;
	GPid pid;
	GError * error = NULL;
	SimulatorChild * sc;

	/* prepare the arguments */
	if((argv[3] = strdup(command)) == NULL)
		return -simulator_error(simulator, strerror(errno), 1);
	/* prepare the environment */
	for(i = 0; environ[i] != NULL; i++)
	{
		if((p = realloc(envp, sizeof(*p) * (i + 2))) == NULL)
			break;
		envp = p;
		envp[i + 1] = NULL;
		if(strncmp(environ[i], display, sizeof(display) - 1) == 0)
			envp[i] = strdup(display1);
		else
			envp[i] = strdup(environ[i]);
		if(envp[i] == NULL)
			break;
	}
	if(environ[i] != NULL)
	{
		for(i = 0; envp[i] != NULL; i++)
			free(envp[i]);
		free(envp);
		free(argv[3]);
		return -simulator_error(simulator, strerror(errno), 1);
	}
	if(g_spawn_async(NULL, argv, envp, flags, NULL, NULL, &pid,
				&error) == FALSE)
	{
		simulator_error(simulator, error->message, 1);
		g_error_free(error);
	}
	else if((sc = realloc(simulator->children, sizeof(*sc)
					* (simulator->children_cnt + 1)))
			!= NULL)
	{
		simulator->children = sc;
		sc = &simulator->children[simulator->children_cnt++];
		sc->source = g_child_watch_add(pid,
				_simulator_on_children_watch, simulator);
		sc->pid = pid;
	}
	else
		g_spawn_close_pid(pid);
	for(i = 0; envp[i] != NULL; i++)
		free(envp[i]);
	free(envp);
	free(argv[3]);
	return 0;
}


/* private */
/* functions */
/* callbacks */
/* simulator_on_button_clicked */
static void _simulator_on_button_clicked(GtkToolButton * button, gpointer data)
{
	Simulator * simulator = data;
	/* FIXME may be wrong */
	char const display[] = ":1.0";
	char const * p;
	KeySym keysym;
	KeyCode keycode;

	/* run commands */
	if((p = g_object_get_data(G_OBJECT(button), "command")) != NULL)
		simulator_run(simulator, p);
	/* simulate keys */
	if(simulator->display == NULL)
		simulator->display = XOpenDisplay(display);
	if(simulator->display != NULL
			&& (p = g_object_get_data(G_OBJECT(button), "keysym"))
			&& (keysym = XStringToKeysym(p)) != NoSymbol
			&& (keycode = XKeysymToKeycode(simulator->display,
					keysym)) != NoSymbol)
	{
		XTestGrabControl(simulator->display, True);
		XTestFakeKeyEvent(simulator->display, keycode, True, 0);
		XTestFakeKeyEvent(simulator->display, keycode, False, 0);
		XTestGrabControl(simulator->display, False);
	}
}


/* simulator_on_child_watch */
static void _simulator_on_child_watch(GPid pid, gint status, gpointer data)
{
	Simulator * simulator = data;
	GError * error = NULL;

	if(simulator->xephyr.pid != pid)
		return;
	if(g_spawn_check_exit_status(status, &error) == FALSE)
	{
		simulator_error(simulator, error->message, 1);
		g_error_free(error);
	}
	g_spawn_close_pid(pid);
	simulator->xephyr.pid = -1;
	simulator->xephyr.source = 0;
}


/* simulator_on_children_watch */
static void _simulator_on_children_watch(GPid pid, gint status, gpointer data)
{
	Simulator * simulator = data;
	size_t i;
	size_t s = sizeof(*simulator->children);
#if GLIB_CHECK_VERSION(2, 34, 0)
	GError * error = NULL;

	if(g_spawn_check_exit_status(status, &error) == FALSE)
	{
		simulator_error(simulator, error->message, 1);
		g_error_free(error);
	}
#else
	char buf[64];

	if(!(WIFEXITED(status) && WEXITSTATUS(status) == 0))
	{
		if(WIFEXITED(status))
			snprintf(buf, sizeof(buf), "%s%d",
					_("Child exited with error code "),
					WEXITSTATUS(status));
		else if(WIFSIGNALED(status))
			snprintf(buf, sizeof(buf), "%s%d",
					_("Child killed with signal "),
					WTERMSIG(status));
		else
			snprintf(buf, sizeof(buf), "%s",
					_("Child exited with an error"));
		simulator_error(simulator, buf, 1);
	}
#endif
	g_spawn_close_pid(pid);
	for(i = 0; i < simulator->children_cnt; i++)
	{
		if(simulator->children[i].pid != pid)
			continue;
		memmove(&simulator->children[i], &simulator->children[i + 1],
				s * (--simulator->children_cnt - i));
		break;
	}
}


/* simulator_on_close */
static void _simulator_on_close(gpointer data)
{
	Simulator * simulator = data;

	gtk_widget_hide(simulator->window);
	gtk_main_quit();
}


/* simulator_on_closex */
static gboolean _simulator_on_closex(gpointer data)
{
	Simulator * simulator = data;

	_simulator_on_close(simulator);
	return TRUE;
}


/* simulator_on_file_close */
static void _simulator_on_file_quit(gpointer data)
{
	Simulator * simulator = data;

	_simulator_on_close(simulator);
}


/* simulator_on_file_run */
static void _run_on_choose_response(GtkWidget * widget, gint arg1,
		gpointer data);

static void _simulator_on_file_run(gpointer data)
{
	Simulator * simulator = data;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * entry;
	GtkWidget * widget;
	GtkFileFilter * filter;
	int res;
	char const * command;

	dialog = gtk_dialog_new_with_buttons(_("Run..."),
			GTK_WINDOW(simulator->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog),
			GTK_RESPONSE_ACCEPT);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	/* label */
	widget = gtk_label_new(_("Command:"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* entry */
	entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	/* file chooser */
	widget = gtk_file_chooser_dialog_new(_("Run program..."),
			GTK_WINDOW(dialog),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	/* file chooser: file filters */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Executable files"));
	gtk_file_filter_add_mime_type(filter, "application/x-executable");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Perl scripts"));
	gtk_file_filter_add_mime_type(filter, "application/x-perl");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Python scripts"));
	gtk_file_filter_add_mime_type(filter, "text/x-python");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Shell scripts"));
	gtk_file_filter_add_mime_type(filter, "application/x-shellscript");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	g_signal_connect(widget, "response", G_CALLBACK(
				_run_on_choose_response), entry);
	widget = gtk_file_chooser_button_new_with_dialog(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	/* run the dialog */
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if(res == GTK_RESPONSE_ACCEPT)
	{
		command = gtk_entry_get_text(GTK_ENTRY(entry));
		simulator_run(simulator, command);
	}
	gtk_widget_destroy(dialog);
}

static void _run_on_choose_response(GtkWidget * widget, gint arg1,
		gpointer data)
{
	GtkWidget * entry = data;

	if(arg1 != GTK_RESPONSE_ACCEPT)
		return;
	gtk_entry_set_text(GTK_ENTRY(entry), gtk_file_chooser_get_filename(
				GTK_FILE_CHOOSER(widget)));
}


/* simulator_on_view_toggle_debugging_mode */
static void _simulator_on_view_toggle_debugging_mode(gpointer data)
{
	Simulator * simulator = data;

	kill(simulator->xephyr.pid, SIGUSR1);
}


/* simulator_on_help_about */
static void _simulator_on_help_about(gpointer data)
{
	Simulator * simulator = data;
	GtkWidget * dialog;

	dialog = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
			GTK_WINDOW(simulator->window));
	desktop_about_dialog_set_authors(dialog, _authors);
	desktop_about_dialog_set_comments(dialog,
			_("Simulator for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "stock_cell-phone");
	desktop_about_dialog_set_name(dialog, "Simulator");
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* simulator_on_help_contents */
static void _simulator_on_help_contents(gpointer data)
{
	desktop_help_contents(PACKAGE, PROGNAME);
}


/* simulator_on_plug_added */
static void _simulator_on_plug_added(gpointer data)
{
	Simulator * simulator = data;

	if(simulator->command != NULL)
		simulator_run(simulator, simulator->command);
}
