/* $Id$ */
static char const _copyright[] =
"Copyright © 2011-2015 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Coder */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program. If not, see <http://www.gnu.org/licenses/>.";
/* TODO:
 * - add a "backend" type of plug-ins (asm, hexedit, make, project, UWff...)
 * - add a "plug-in" type of plug-ins (time tracker, ...) */



#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gdk/gdkkeysyms.h>
#include <Desktop.h>
#include "callbacks.h"
#include "coder.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif


/* Coder */
/* private */
/* types */
struct _Coder
{
	Config * config;
	Project ** projects;
	size_t projects_cnt;
	Project * cur;

	/* widgets */
	/* toolbar */
	GtkWidget * tb_window;

	/* preferences */
	GtkWidget * pr_window;
	GtkWidget * pr_editor_command;
	GtkWidget * pr_editor_terminal;

	/* files */
	GtkWidget * fi_window;
	GtkWidget * fi_combo;
	GtkWidget * fi_view;

	/* about */
	GtkWidget * ab_window;
};


/* constants */
#define ICON_NAME	"applications-development"

static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* menubar */
static const DesktopMenu _coder_menu_file[] =
{
	{ N_("_New file..."), G_CALLBACK(on_file_new), GTK_STOCK_NEW,
		GDK_CONTROL_MASK, GDK_KEY_N },
	{ N_("_Open file..."), G_CALLBACK(on_file_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL , NULL, 0, 0 },
	{ N_("_Preferences..."), G_CALLBACK(on_file_preferences),
		GTK_STOCK_PREFERENCES, GDK_CONTROL_MASK, GDK_KEY_P },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Exit"), G_CALLBACK(on_file_exit), GTK_STOCK_QUIT,
		GDK_CONTROL_MASK, GDK_KEY_Q },
	{ NULL, NULL, NULL, 0, 0 }
};

/* FIXME will certainly be dynamic */
static const DesktopMenu _coder_menu_project[] =
{
	{ N_("_New project..."), G_CALLBACK(on_project_new), GTK_STOCK_NEW, 0,
		0 },
	{ N_("_Open project..."), G_CALLBACK(on_project_open), GTK_STOCK_OPEN,
		0, 0 },
	{ N_("_Save project"), G_CALLBACK(on_project_save), GTK_STOCK_SAVE,
		GDK_CONTROL_MASK, GDK_KEY_S },
	{ N_("Save project _As..."), G_CALLBACK(on_project_save_as),
		GTK_STOCK_SAVE_AS, 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Properties..."), G_CALLBACK(on_project_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _coder_menu_view[] =
{
	{ N_("_Files"), G_CALLBACK(on_view_files), NULL, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _coder_menu_tools[] =
{
	{ N_("_Debugger"), G_CALLBACK(on_tools_debugger), NULL, 0, 0 },
	{ N_("_PHP console"), G_CALLBACK(on_tools_php_console), NULL, 0, 0 },
	{ N_("_Simulator"), G_CALLBACK(on_tools_simulator), "stock_cell-phone",
		0, 0 },
	{ N_("S_QL console"), G_CALLBACK(on_tools_sql_console),
		"stock_insert-table", 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _coder_menu_help[] =
{
	{ N_("_Contents"), G_CALLBACK(on_help_contents), "help-contents", 0,
		0 },
	{ N_("_About"), G_CALLBACK(on_help_about), GTK_STOCK_ABOUT, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _coder_menubar[] =
{
	{ N_("_File"),		_coder_menu_file },
	{ N_("_Project"),	_coder_menu_project },
	{ N_("_View"),		_coder_menu_view },
	{ N_("_Tools"),		_coder_menu_tools },
	{ N_("_Help"),		_coder_menu_help },
	{ NULL,			NULL }
};


/* variables */
/* toolbar */
static DesktopToolbar _coder_toolbar[] =
{
	{ N_("Exit"), G_CALLBACK(on_file_exit), GTK_STOCK_QUIT, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* prototypes */
static Project * _coder_get_current_project(Coder * coder);


/* public */
/* functions */
/* coder_new */
static void _new_config(Coder * g);

Coder * coder_new(void)
{
	Coder * coder;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;

	if((coder = malloc(sizeof(*coder))) == NULL)
		return NULL;
	coder->projects = NULL;
	coder->projects_cnt = 0;
	coder->cur = NULL;
	_new_config(coder);
	/* main window */
	group = gtk_accel_group_new();
	coder->tb_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(coder->tb_window), group);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(coder->tb_window), ICON_NAME);
#endif
	gtk_window_set_title(GTK_WINDOW(coder->tb_window), _("Coder"));
	gtk_window_set_resizable(GTK_WINDOW(coder->tb_window), FALSE);
	g_signal_connect_swapped(coder->tb_window, "delete-event", G_CALLBACK(
				on_closex), coder);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* menubar */
	widget = desktop_menubar_create(_coder_menubar, coder, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	widget = desktop_toolbar_create(_coder_toolbar, coder, group);
	g_object_unref(group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(coder->tb_window), vbox);
	/* files */
	coder->fi_window = NULL;
	coder_show_files(coder, TRUE);
	/* about */
	coder->ab_window = NULL;
	gtk_widget_show_all(coder->tb_window);
	return coder;
}

static char * _config_file(void);
static void _new_config(Coder * coder)
{
	char * filename;

	if((coder->config = config_new()) == NULL)
	{
		coder_error(coder, strerror(errno), 0);
		return;
	}
	config_load(coder->config, PREFIX "/etc/" PACKAGE ".conf");
	if((filename = _config_file()) == NULL)
		return;
	config_load(coder->config, filename);
	free(filename);
}

static char * _config_file(void)
{
	char const conffile[] = ".coder";
	char const * homedir;
	size_t len;
	char * filename;

	if((homedir = getenv("HOME")) == NULL)
		return NULL;
	len = strlen(homedir) + 1 + strlen(conffile) + 1;
	if((filename = malloc(len)) == NULL)
		return NULL;
	snprintf(filename, len, "%s/%s", homedir, conffile);
	return filename;
}


/* coder_delete */
void coder_delete(Coder * coder)
{
	char * filename;
	size_t i;

	if((filename = _config_file()) != NULL)
	{
		config_save(coder->config, filename);
		free(filename);
	}
	config_delete(coder->config);
	for(i = 0; i < coder->projects_cnt; i++)
		project_delete(coder->projects[i]);
	free(coder->projects);
	free(coder);
}


/* useful */
/* coder_about */
static gboolean _about_on_closex(gpointer data);

void coder_about(Coder * coder)
{
	if(coder->ab_window != NULL)
	{
		gtk_window_present(GTK_WINDOW(coder->ab_window));
		return;
	}
	coder->ab_window = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(coder->ab_window), GTK_WINDOW(
				coder->tb_window));
	desktop_about_dialog_set_authors(coder->ab_window, _authors);
	desktop_about_dialog_set_comments(coder->ab_window,
			_("Integrated Development Environment for the DeforaOS"
			" desktop"));
	desktop_about_dialog_set_copyright(coder->ab_window, _copyright);
	desktop_about_dialog_set_logo_icon_name(coder->ab_window, ICON_NAME);
	desktop_about_dialog_set_license(coder->ab_window, _license);
	desktop_about_dialog_set_name(coder->ab_window, PACKAGE);
	desktop_about_dialog_set_version(coder->ab_window, VERSION);
	desktop_about_dialog_set_website(coder->ab_window,
			"http://www.defora.org/");
	g_signal_connect_swapped(coder->ab_window, "delete-event", G_CALLBACK(
				_about_on_closex), coder);
	gtk_widget_show(coder->ab_window);
}

static gboolean _about_on_closex(gpointer data)
{
	Coder * coder = data;

	gtk_widget_hide(coder->ab_window);
	return TRUE;
}


/* coder_error */
int coder_error(Coder * coder, char const * message, int ret)
{
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(coder->tb_window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}


/* coder_file_open */
void coder_file_open(Coder * coder, char const * filename)
{
	/* FIXME really use the MIME sub-system */
	char * argv[] = { NULL, NULL, NULL };
	char const * p;
	GError * error = NULL;

	if((p = config_get(coder->config, "editor", "command")) == NULL)
		p = "editor"; /* XXX gather defaults in a common place */
	if((argv[0] = strdup(p)) == NULL)
		return; /* XXX report error */
	if(filename != NULL)
		argv[1] = strdup(filename); /* XXX check and report error */
	if(g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				NULL, &error) != TRUE)
	{
		coder_error(coder, error->message, 1);
		g_error_free(error);
	}
	free(argv[1]);
	free(argv[0]);
}


/* coder_project_open */
int coder_project_open(Coder * coder, char const * filename)
{
	Project * project;

	if((project = project_new()) == NULL)
		return -coder_error(coder, error_get(NULL), 1);
	if(project_load(project, filename) != 0
			|| coder_project_open_project(coder, project) != 0)
	{
		project_delete(project);
		return -coder_error(coder, error_get(NULL), 1);
	}
	coder->cur = project;
#if GTK_CHECK_VERSION(2, 24, 0)
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(coder->fi_combo),
			project_get_package(project));
#else
	gtk_combo_box_append_text(GTK_COMBO_BOX(coder->fi_combo),
			project_get_package(project));
#endif
	/* FIXME doesn't always select the last project opened */
	gtk_combo_box_set_active(GTK_COMBO_BOX(coder->fi_combo),
			gtk_combo_box_get_active(GTK_COMBO_BOX(coder->fi_combo))
			+ 1);
	return 0;
}


/* coder_project_open_dialog */
void coder_project_open_dialog(Coder * coder)
{
	GtkWidget * dialog;
	GtkFileFilter * filter;
	gchar * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open project..."),
			GTK_WINDOW(coder->tb_window),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Project files"));
	gtk_file_filter_add_pattern(filter, "project.conf");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return;
	coder_project_open(coder, filename);
	g_free(filename);
}


/* coder_project_open_project */
int coder_project_open_project(Coder * coder, Project * project)
{
	Project ** p;

	if(project == NULL)
		return -error_set_code(1, "%s", strerror(EINVAL));;
	if((p = realloc(coder->projects, sizeof(*p) * (coder->projects_cnt + 1)))
			== NULL)
		return -error_set_code(1, "%s", strerror(errno));
	coder->projects = p;
	coder->projects[coder->projects_cnt++] = project;
	return 0;
}


/* coder_project_properties */
void coder_project_properties(Coder * coder)
{
	Project * project;

	if((project = _coder_get_current_project(coder)) == NULL)
		return;
	project_properties(project);
}


/* coder_project_save */
int coder_project_save(Coder * coder)
{
	Project * project;

	if((project = _coder_get_current_project(coder)) == NULL)
		return -1;
	if(project_get_pathname(project) == NULL)
		return coder_project_save_dialog(coder);
	return project_save(project);
}


/* coder_project_save_as */
int coder_project_save_as(Coder * coder, char const * filename)
{
	Project * project;

	if((project = _coder_get_current_project(coder)) == NULL)
		return -1;
	if(project_set_pathname(project, filename) != 0)
		return -1;
	return project_save(project);
}


/* coder_project_save_dialog */
int coder_project_save_dialog(Coder * coder)
{
	int ret = -1;
	Project * project;
	GtkWidget * dialog;
	gchar * filename = NULL;

	if((project = _coder_get_current_project(coder)) == NULL)
		return -1;
	dialog = gtk_file_chooser_dialog_new(_("Save project as..."), NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	/* FIXME add options? (recursive save) */
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename != NULL)
		ret = coder_project_save_as(coder, filename);
	g_free(filename);
	return ret;
}


/* coder_show_files */
static void _show_files_window(Coder * coder);
/* callbacks */
static gboolean _files_on_closex(gpointer data);

void coder_show_files(Coder * coder, gboolean show)
{
	if(coder->fi_window == NULL)
		_show_files_window(coder);
	if(show)
		gtk_window_present(GTK_WINDOW(coder->fi_window));
	else
		gtk_widget_hide(coder->fi_window);
}

static void _show_files_window(Coder * coder)
{
	GtkWidget * vbox;
	GtkWidget * hbox;

	coder->fi_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(coder->fi_window), 150, 200);
	gtk_window_set_title(GTK_WINDOW(coder->fi_window), _("Files"));
	g_signal_connect_swapped(coder->fi_window, "delete-event", G_CALLBACK(
				_files_on_closex), coder);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	hbox = gtk_hbox_new(FALSE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* FIXME use gtk_container_set_border_width() instead */
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 2);
#if GTK_CHECK_VERSION(2, 24, 0)
	coder->fi_combo = gtk_combo_box_text_new();
#else
	coder->fi_combo = gtk_combo_box_new_text();
#endif
	gtk_box_pack_start(GTK_BOX(vbox), coder->fi_combo, FALSE, TRUE, 2);
	coder->fi_view = gtk_tree_view_new();
	gtk_box_pack_start(GTK_BOX(vbox), coder->fi_view, TRUE, TRUE, 2);
	gtk_container_add(GTK_CONTAINER(coder->fi_window), hbox);
	gtk_widget_show_all(hbox);
}

/* callbacks */
static gboolean _files_on_closex(gpointer data)
{
	Coder * coder = data;

	gtk_widget_hide(coder->fi_window);
	return TRUE;
}


/* coder_show_preferences */
static void _show_preferences_window(Coder * coder);
static void _preferences_set(Coder * coder);
/* callbacks */
static gboolean _on_preferences_closex(gpointer data);
static void _on_preferences_apply(gpointer data);
static void _on_preferences_cancel(gpointer data);
static void _on_preferences_ok(gpointer data);

void coder_show_preferences(Coder * coder, gboolean show)
{
	if(coder->pr_window == NULL)
		_show_preferences_window(coder);
	if(show)
		gtk_window_present(GTK_WINDOW(coder->pr_window));
	else
		gtk_widget_hide(coder->pr_window);
}

static void _show_preferences_window(Coder * coder)
{
	GtkWidget * vbox;
	GtkWidget * nb;
	GtkWidget * nb_vbox;
	GtkWidget * hbox;
	GtkWidget * b_ok;
	GtkWidget * b_apply;
	GtkWidget * b_cancel;

	coder->pr_window = gtk_window_new(GTK_WINDOW_TOPLEVEL); /* XXX dialog */
	gtk_container_set_border_width(GTK_CONTAINER(coder->pr_window), 4);
	gtk_window_set_title(GTK_WINDOW(coder->pr_window), _("Preferences"));
	g_signal_connect_swapped(coder->pr_window, "delete-event", G_CALLBACK(
				_on_preferences_closex), coder);
	vbox = gtk_vbox_new(FALSE, 4);
	nb = gtk_notebook_new();
	/* notebook page editor */
	nb_vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(nb_vbox), 4);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Editor:")), FALSE,
			TRUE, 0);
	coder->pr_editor_command = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), coder->pr_editor_command, TRUE, TRUE,
			0);
	gtk_box_pack_start(GTK_BOX(nb_vbox), hbox, FALSE, TRUE, 0);
	coder->pr_editor_terminal = gtk_check_button_new_with_mnemonic(
			_("Run in a _terminal"));
	gtk_box_pack_start(GTK_BOX(nb_vbox), coder->pr_editor_terminal, FALSE,
			TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), nb_vbox, gtk_label_new(
				_("Editor")));
	/* notebook page plug-ins */
	nb_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(nb_vbox), 4);
	gtk_notebook_append_page(GTK_NOTEBOOK(nb), nb_vbox, gtk_label_new(
				_("Plug-ins")));
	gtk_box_pack_start(GTK_BOX(vbox), nb, TRUE, TRUE, 0);
	/* buttons */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(TRUE, 4);
#endif
	b_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(b_ok, "clicked", G_CALLBACK(
				_on_preferences_ok), coder);
	gtk_box_pack_end(GTK_BOX(hbox), b_ok, FALSE, TRUE, 0);
	b_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect_swapped(b_apply, "clicked", G_CALLBACK(
				_on_preferences_apply), coder);
	gtk_box_pack_end(GTK_BOX(hbox), b_apply, FALSE, TRUE, 0);
	b_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(b_cancel, "clicked", G_CALLBACK(
				_on_preferences_cancel), coder);
	gtk_box_pack_end(GTK_BOX(hbox), b_cancel, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(coder->pr_window), vbox);
	_preferences_set(coder);
	gtk_widget_show_all(vbox);
}

static void _preferences_set(Coder * coder)
{
	char const * p;

	if((p = config_get(coder->config, "editor", "command")) == NULL)
		p = "editor";
	gtk_entry_set_text(GTK_ENTRY(coder->pr_editor_command), p);
	/* FIXME implement the rest */
}

static void _on_preferences_apply(gpointer data)
{
	Coder * coder = data;

	config_set(coder->config, "editor", "command", gtk_entry_get_text(
				GTK_ENTRY(coder->pr_editor_command)));
	/* FIXME implement the rest */
}


static void _on_preferences_cancel(gpointer data)
{
	Coder * coder = data;

	_preferences_set(coder);
	gtk_widget_hide(coder->pr_window);
}

static void _on_preferences_ok(gpointer data)
{
	Coder * coder = data;

	_on_preferences_apply(coder);
	gtk_widget_hide(coder->pr_window);
	/* FIXME actually save preferences */
}

/* callbacks */
static gboolean _on_preferences_closex(gpointer data)
{
	Coder * coder = data;

	_on_preferences_cancel(coder);
	return TRUE;
}


/* private */
/* coder_get_current_project */
static Project * _coder_get_current_project(Coder * coder)
{
	if(coder->cur == NULL)
	{
		/* FIXME should not happen (disable callback action) */
		coder_error(coder, _("No project opened"), 1);
		return NULL;
	}
	return coder->cur;
}
