/* $Id$ */
static char const _copyright[] =
"Copyright (c) 2013 Pierre Pronchery <khorben@defora.org>";
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
/* FIXME:
 * - add an interface to run commands within the underlying display */



#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include <System.h>
#include <Desktop.h>
#include "simulator.h"
#include "../config.h"

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif


/* Simulator */
/* private */
/* types */
struct _Simulator
{
	char * model;
	char * title;

	int dpi;
	int width;
	int height;

	unsigned int source;
	GPid pid;

	/* widgets */
	GtkWidget * window;
	GtkWidget * socket;
};


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
/* callbacks */
static void _simulator_on_child_watch(GPid pid, gint status, gpointer data);
static void _simulator_on_close(gpointer data);
static gboolean _simulator_on_closex(gpointer data);

static void _simulator_on_file_close(gpointer data);
static void _simulator_on_file_run(gpointer data);
static void _simulator_on_help_about(gpointer data);


/* constants */
/* menubar */
static const DesktopMenu _simulator_file_menu[] =
{
	{ "_Run...", G_CALLBACK(_simulator_on_file_run), NULL, GDK_CONTROL_MASK,
		GDK_KEY_R },
	{ "", NULL, NULL, 0, 0 },
	{ "_Close", G_CALLBACK(_simulator_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _simulator_help_menu[] =
{
#if GTK_CHECK_VERSION(2, 6, 0)
	{ "About", G_CALLBACK(_simulator_on_help_about), GTK_STOCK_ABOUT, 0,
		0 },
#else
	{ "About", G_CALLBACK(_simulator_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _simulator_menubar[] =
{
	{ "_File", _simulator_file_menu },
	{ "_Help", _simulator_help_menu },
	{ NULL, NULL }
};


/* public */
/* functions */
/* simulator_new */
static int _new_load(Simulator * simulator);
/* callbacks */
static gboolean _new_xephyr(gpointer data);

Simulator * simulator_new(char const * model, char const * title)
{
	Simulator * simulator;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;
	char * p;

	if((simulator = object_new(sizeof(*simulator))) == NULL)
		return NULL;
	simulator->model = (model != NULL) ? strdup(model) : NULL;
	simulator->title = (title != NULL) ? strdup(title) : NULL;
	simulator->pid = -1;
	simulator->source = 0;
	simulator->window = NULL;
	/* check for errors */
	if(model != NULL && simulator->model == NULL)
	{
		simulator_delete(simulator);
		return NULL;
	}
	/* load the configuration */
	/* XXX no longer ignore errors */
	_new_load(simulator);
	/* widgets */
	group = gtk_accel_group_new();
	simulator->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(simulator->window, simulator->width,
			simulator->height);
	gtk_window_add_accel_group(GTK_WINDOW(simulator->window), group);
	if(simulator->title == NULL || (p = string_new_append("Simulator - ",
					simulator->title, NULL)) == NULL)
		gtk_window_set_title(GTK_WINDOW(simulator->window),
				"Simulator");
	else
	{
		gtk_window_set_title(GTK_WINDOW(simulator->window), p);
		free(p);
	}
	g_object_unref(group);
	gtk_window_set_resizable(GTK_WINDOW(simulator->window), FALSE);
	g_signal_connect_swapped(simulator->window, "delete-event", G_CALLBACK(
				_simulator_on_closex), simulator);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* menubar */
	widget = desktop_menubar_create(_simulator_menubar, simulator, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	simulator->socket = gtk_socket_new();
	gtk_box_pack_start(GTK_BOX(vbox), simulator->socket, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(simulator->window), vbox);
	gtk_widget_show_all(vbox);
	simulator->source = g_idle_add(_new_xephyr, simulator);
	gtk_widget_show(simulator->window);
	return simulator;
}

static int _new_load(Simulator * simulator)
{
	int ret = -1;
	Config * config;
	char const * model = (simulator->model != NULL)
		? simulator->model : "default";
	char * p;
	char const * q;
	long l;

	config = config_new();
	p = string_new_append(DATADIR "/" PACKAGE "/Simulator/models/", model,
			".conf", NULL);
	simulator->dpi = 96;
	simulator->width = 640;
	simulator->height = 480;
	if(config != NULL && p != NULL && (ret = config_load(config, p)) == 0)
	{
		if((q = config_get(config, NULL, "dpi")) != NULL
				&& (l = strtol(q, NULL, 10)) > 0)
			simulator->dpi = l;
		if((q = config_get(config, NULL, "width")) != NULL
				&& (l = strtol(q, NULL, 10)) > 0)
			simulator->width = l;
		if((q = config_get(config, NULL, "height")) != NULL
				&& (l = strtol(q, NULL, 10)) > 0)
			simulator->height = l;
		if(simulator->title == NULL
				&& (q = config_get(config, NULL, "title"))
				!= NULL)
			simulator->title = strdup(q);
	}
	free(p);
	if(config != NULL)
		config_delete(config);
	return ret;
}

static gboolean _new_xephyr(gpointer data)
{
	Simulator * simulator = data;
	char * argv[] = { BINDIR "/Xephyr", "Xephyr", "-parent", NULL,
		"-dpi", NULL, ":1", NULL };
	char parent[16];
	char dpi[16];
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO
		| G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;

	/* launch Xephyr */
	snprintf(parent, sizeof(parent), "%u", gtk_socket_get_id(
				GTK_SOCKET(simulator->socket)));
	argv[3] = parent;
	snprintf(dpi, sizeof(dpi), "%u", simulator->dpi);
	argv[5] = dpi;
	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL,
				&simulator->pid, &error) == FALSE)
	{
		simulator_error(simulator, error->message, 1);
		g_error_free(error);
	}
	else
		g_child_watch_add(simulator->pid, _simulator_on_child_watch,
				simulator);
	return FALSE;
}


/* simulator_delete */
void simulator_delete(Simulator * simulator)
{
	if(simulator->source > 0)
		g_source_remove(simulator->source);
	if(simulator->pid > 0)
	{
		kill(simulator->pid, SIGTERM);
		g_spawn_close_pid(simulator->pid);
	}
	if(simulator->window != NULL)
		gtk_widget_destroy(simulator->window);
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
			"%s", "Error");
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", "simulator", message);
	return ret;
}


/* private */
/* functions */
/* callbacks */
/* simulator_on_child_watch */
static void _simulator_on_child_watch(GPid pid, gint status, gpointer data)
{
	Simulator * simulator = data;

	if(simulator->pid != pid)
		return;
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status) != 0)
			fprintf(stderr, "%s: %s%u\n", "Simulator",
					"Xephyr exited with status ",
					WEXITSTATUS(status));
		gtk_main_quit();
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", "Simulator",
				"Xephyr exited with signal ", WTERMSIG(status));
		gtk_main_quit();
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
static void _simulator_on_file_close(gpointer data)
{
	Simulator * simulator = data;

	_simulator_on_close(simulator);
}


/* simulator_on_file_run */
static void _simulator_on_file_run(gpointer data)
{
	Simulator * simulator = data;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	char const * command;
	int res;
	char * argv[] = { "/bin/sh", "run", "-c", NULL, NULL };
	char * envp[] = { "DISPLAY=:1.0", NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO | G_SPAWN_SEARCH_PATH;
	GError * error = NULL;

	dialog = gtk_dialog_new_with_buttons("Run...",
			GTK_WINDOW(simulator->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	widget = gtk_label_new("Command:");
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_hide(dialog);
	if(res == GTK_RESPONSE_ACCEPT)
	{
		command = gtk_entry_get_text(GTK_ENTRY(widget));
		if((argv[3] = strdup(command)) == NULL)
			simulator_error(simulator, strerror(errno), 1);
		else if(g_spawn_async(NULL, argv, envp, flags, NULL, NULL, NULL,
					&error) == FALSE)
		{
			simulator_error(simulator, error->message, 1);
			g_error_free(error);
		}
		free(argv[3]);
	}
	gtk_widget_destroy(dialog);
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
			"Simulator for the DeforaOS desktop");
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "stock_cell-phone");
	desktop_about_dialog_set_name(dialog, "Simulator");
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
