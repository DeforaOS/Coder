/* $Id$ */
static char const _copyright[] =
"Copyright © 2013 Pierre Pronchery <khorben@defora.org>";
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include <System.h>
#include <Desktop.h>
#include "simulator.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

/* constants */
#ifndef PROGNAME
# define PROGNAME	"simulator"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif

extern char ** environ;


/* Simulator */
/* private */
/* types */
struct _Simulator
{
	char * model;
	char * title;
	char * command;

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
static void _simulator_on_plug_added(gpointer data);

static void _simulator_on_file_quit(gpointer data);
static void _simulator_on_file_run(gpointer data);
static void _simulator_on_help_about(gpointer data);
static void _simulator_on_help_contents(gpointer data);


/* constants */
/* menubar */
static const DesktopMenu _simulator_file_menu[] =
{
	{ N_("_Run..."), G_CALLBACK(_simulator_on_file_run), NULL,
		GDK_CONTROL_MASK, GDK_KEY_R },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_simulator_on_file_quit), GTK_STOCK_QUIT,
		GDK_CONTROL_MASK, GDK_KEY_Q },
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
	{ N_("_Help"), _simulator_help_menu },
	{ NULL, NULL }
};


/* public */
/* functions */
/* simulator_new */
static int _new_chooser(Simulator * simulator);
static int _new_load(Simulator * simulator);
/* callbacks */
static gboolean _new_xephyr(gpointer data);

Simulator * simulator_new(SimulatorPrefs * prefs)
{
	Simulator * simulator;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;
	char * p;

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
	simulator->pid = -1;
	simulator->source = 0;
	simulator->window = NULL;
	/* set default values */
	simulator->dpi = 96;
	simulator->width = 640;
	simulator->height = 480;
	if(prefs != NULL && prefs->chooser != 0)
	{
		if(_new_chooser(simulator) != 0)
		{
			simulator_delete(simulator);
			return NULL;
		}
	}
	else
		/* load the configuration */
		/* XXX no longer ignore errors */
		_new_load(simulator);
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
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* menubar */
	widget = desktop_menubar_create(_simulator_menubar, simulator, group);
	g_object_unref(group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	simulator->socket = gtk_socket_new();
	g_signal_connect_swapped(simulator->socket, "plug-added", G_CALLBACK(
				_simulator_on_plug_added), simulator);
	gtk_box_pack_start(GTK_BOX(vbox), simulator->socket, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(simulator->window), vbox);
	gtk_widget_show_all(simulator->window);
	simulator->source = g_idle_add(_new_xephyr, simulator);
	return simulator;
}

static int _new_chooser(Simulator * simulator)
{
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * combobox;
	GtkWidget * dpi;
	GtkWidget * width;
	GtkWidget * height;
	GtkWidget * widget;

	dialog = gtk_dialog_new_with_buttons(_("Simulator properties"),
			NULL, 0, GTK_STOCK_QUIT, GTK_RESPONSE_CLOSE,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	gtk_box_set_spacing(GTK_BOX(vbox), 4);
	/* profile */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Profile: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 24, 0)
	combobox = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),
			_("Custom profile"));
#else
	combobox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), _("Custom profile"));
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	gtk_box_pack_end(GTK_BOX(hbox), combobox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* dpi */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Resolution: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	dpi = gtk_spin_button_new_with_range(48.0, 300.0, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dpi), simulator->dpi);
	gtk_box_pack_end(GTK_BOX(hbox), dpi, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* width */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Width: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	width = gtk_spin_button_new_with_range(120, 1600, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(width), simulator->width);
	gtk_box_pack_end(GTK_BOX(hbox), width, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* height */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Height: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	height = gtk_spin_button_new_with_range(120, 1600, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(height), simulator->height);
	gtk_box_pack_end(GTK_BOX(hbox), height, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
	{
		gtk_widget_destroy(dialog);
		return -1;
	}
	gtk_widget_hide(dialog);
	/* apply the values */
	simulator->width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
				width));
	simulator->height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
				height));
	simulator->dpi = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dpi));
	gtk_widget_destroy(dialog);
	return 0;
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

	/* load the selected model */
	config = config_new();
	p = string_new_append(DATADIR "/" PACKAGE "/Simulator/models/", model,
			".conf", NULL);
	if(config != NULL && p != NULL)
		ret = config_load(config, p);
	free(p);
	if(ret == 0)
	{
		if((q = config_get(config, NULL, "dpi")) != NULL
				&& (l = strtol(q, &p, 10)) > 0
				&& q[0] != '\0' && *p == '\0')
			simulator->dpi = l;
		if((q = config_get(config, NULL, "width")) != NULL
				&& (l = strtol(q, &p, 10)) > 0
				&& q[0] != '\0' && *p == '\0')
			simulator->width = l;
		if((q = config_get(config, NULL, "height")) != NULL
				&& (l = strtol(q, &p, 10)) > 0
				&& q[0] != '\0' && *p == '\0')
			simulator->height = l;
		if(simulator->title == NULL
				&& (q = config_get(config, NULL, "title"))
				!= NULL)
			simulator->title = strdup(q);
	}
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
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO | G_SPAWN_SEARCH_PATH;
	GError * error = NULL;

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
	if(g_spawn_async(NULL, argv, envp, flags, NULL, NULL, NULL,
				&error) == FALSE)
	{
		simulator_error(simulator, error->message, 1);
		g_error_free(error);
	}
	for(i = 0; envp[i] != NULL; i++)
		free(envp[i]);
	free(envp);
	free(argv[3]);
	return 0;
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
					_("Xephyr exited with status "),
					WEXITSTATUS(status));
		gtk_main_quit();
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", "Simulator",
				_("Xephyr exited with signal "),
				WTERMSIG(status));
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

	dialog = gtk_dialog_new_with_buttons("Run...",
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
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	/* label */
	widget = gtk_label_new("Command:");
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* entry */
	entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	/* file chooser */
	widget = gtk_file_chooser_dialog_new("Run program...",
			GTK_WINDOW(dialog),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	/* file chooser: file filters */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Executable files");
	gtk_file_filter_add_mime_type(filter, "application/x-executable");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Perl scripts");
	gtk_file_filter_add_mime_type(filter, "application/x-perl");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Python scripts");
	gtk_file_filter_add_mime_type(filter, "text/x-python");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Shell scripts");
	gtk_file_filter_add_mime_type(filter, "application/x-shellscript");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All files");
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
