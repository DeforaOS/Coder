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



#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include <Database.h>
#include "sequel.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

/* macros */
#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


/* Sequel */
/* private */
/* types */
typedef struct _SequelTab SequelTab;

struct _Sequel
{
	/* internal */
	SequelTab * tabs;
	size_t tabs_cnt;

	Database * database;

	/* widgets */
	PangoFontDescription * font;
	GtkWidget * window;
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * infobar;
	GtkWidget * infobar_label;
#endif
	GtkWidget * notebook;
	GtkWidget * statusbar;
	/* log console */
	GtkWidget * lo_window;
	GtkListStore * lo_store;
	GtkWidget * lo_view;
};

struct _SequelTab
{
	GtkWidget * label;
	GtkWidget * page;
	GtkWidget * text;
	GtkListStore * store;
	GtkWidget * view;
};


/* constants */
#define COLUMN_CNT 10

static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
/* accessors */
static void _sequel_set_status(Sequel * sequel, char const * status);


/* useful */
static int _sequel_open_tab(Sequel * sequel);
static void _sequel_close_all(Sequel * sequel);
static void _sequel_close_tab(Sequel * sequel, unsigned int i);

static int _sequel_connect(Sequel * sequel, char const * engine,
		char const * filename, char const * section);
static int _sequel_connect_dialog(Sequel * sequel);

static void _sequel_clear(Sequel * sequel);
static int _sequel_execute(Sequel * sequel);

static int _sequel_load(Sequel * sequel, char const * filename);
static int _sequel_load_dialog(Sequel * sequel);
static int _sequel_save_as(Sequel * sequel, char const * filename);
static int _sequel_save_as_dialog(Sequel * sequel);

static int _sequel_export(Sequel * sequel, char const * filename);
static int _sequel_export_dialog(Sequel * sequel);

/* callbacks */
static void _sequel_on_close(gpointer data);
static gboolean _sequel_on_closex(gpointer data);
static void _sequel_on_connect(gpointer data);
static void _sequel_on_execute(gpointer data);
static void _sequel_on_export(gpointer data);
static void _sequel_on_load(gpointer data);
static void _sequel_on_save_as(gpointer data);

static void _sequel_on_new_tab(gpointer data);

static void _sequel_on_file_close(gpointer data);
static void _sequel_on_file_close_all(gpointer data);
static void _sequel_on_file_connect(gpointer data);
static void _sequel_on_file_new_tab(gpointer data);
static void _sequel_on_query_clear(gpointer data);
static void _sequel_on_query_execute(gpointer data);
static void _sequel_on_query_export(gpointer data);
static void _sequel_on_query_load(gpointer data);
static void _sequel_on_query_save_as(gpointer data);
static void _sequel_on_view_error_console(gpointer data);
static void _sequel_on_help_about(gpointer data);
static void _sequel_on_help_contents(gpointer data);

static void _sequel_on_tab_close(GtkWidget * widget, gpointer data);
#if GTK_CHECK_VERSION(2, 10, 0)
static void _sequel_on_tab_reordered(GtkWidget * widget, GtkWidget * child,
		guint page, gpointer data);
#endif
static void _sequel_on_tab_switched(GtkWidget * widget, GtkWidget * child,
		guint page, gpointer data);


/* constants */
/* menubar */
static const DesktopMenu _sequel_file_menu[] =
{
	{ N_("New _tab"), G_CALLBACK(_sequel_on_file_new_tab), "tab-new",
		GDK_CONTROL_MASK, GDK_KEY_T },
	{ "", NULL, NULL, 0, 0 },
	{ N_("C_onnect..."), G_CALLBACK(_sequel_on_file_connect), NULL, 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_sequel_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ N_("Close _all tabs"), G_CALLBACK(_sequel_on_file_close_all), NULL, 0,
		0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _sequel_query_menu[] =
{
	{ N_("Clear"), G_CALLBACK(_sequel_on_query_clear),
		GTK_STOCK_CLEAR, GDK_CONTROL_MASK, GDK_KEY_L },
	{ N_("Execute"), G_CALLBACK(_sequel_on_query_execute),
		GTK_STOCK_EXECUTE, GDK_CONTROL_MASK, GDK_KEY_Return },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Load..."), G_CALLBACK(_sequel_on_query_load),
		GTK_STOCK_OPEN, GDK_CONTROL_MASK, GDK_KEY_O },
	{ N_("Save as..."), G_CALLBACK(_sequel_on_query_save_as),
		GTK_STOCK_SAVE_AS, GDK_CONTROL_MASK, GDK_KEY_S },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Export..."), G_CALLBACK(_sequel_on_query_export),
		"stock_insert-table", GDK_CONTROL_MASK, GDK_KEY_E },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _sequel_view_menu[] =
{
	{ N_("_Error console"), G_CALLBACK(_sequel_on_view_error_console),
		NULL, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _sequel_help_menu[] =
{
	{ N_("_Contents"), G_CALLBACK(_sequel_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(_sequel_on_help_about), GTK_STOCK_ABOUT, 0,
		0 },
#else
	{ N_("About"), G_CALLBACK(_sequel_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _sequel_menubar[] =
{
	{ N_("_File"), _sequel_file_menu },
	{ N_("_Query"), _sequel_query_menu },
	{ N_("_View"), _sequel_view_menu },
	{ N_("_Help"), _sequel_help_menu },
	{ NULL, NULL }
};


/* variables */
/* toolbar */
static DesktopToolbar _sequel_toolbar[] =
{
	{ N_("New tab"), G_CALLBACK(_sequel_on_new_tab), "tab-new", 0, 0,
		NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Connect"), G_CALLBACK(_sequel_on_connect), GTK_STOCK_CONNECT, 0,
		0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Execute"), G_CALLBACK(_sequel_on_execute), GTK_STOCK_EXECUTE, 0,
		0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Load..."), G_CALLBACK(_sequel_on_load), GTK_STOCK_OPEN, 0, 0,
		NULL },
	{ N_("Save as..."), G_CALLBACK(_sequel_on_save_as), GTK_STOCK_SAVE_AS,
		0, 0, NULL },
	{ N_("Export..."), G_CALLBACK(_sequel_on_export), "stock_insert-table",
		0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* sequel_new */
Sequel * sequel_new(void)
{
	Sequel * sequel;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;

	if((sequel = object_new(sizeof(*sequel))) == NULL)
		return NULL;
	sequel->tabs = NULL;
	sequel->tabs_cnt = 0;
	sequel->font = NULL;
	sequel->window = NULL;
	sequel->database = NULL;
	/* widgets */
	group = gtk_accel_group_new();
	sequel->font = pango_font_description_new();
	pango_font_description_set_family(sequel->font, "Monospace");
	sequel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(sequel->window), group);
	gtk_window_set_default_size(GTK_WINDOW(sequel->window), 640, 480);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(sequel->window),
			"stock_insert-table");
#endif
	gtk_window_set_title(GTK_WINDOW(sequel->window), _("Sequel"));
	g_signal_connect_swapped(sequel->window, "delete-event", G_CALLBACK(
				_sequel_on_closex), sequel);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* menubar */
	widget = desktop_menubar_create(_sequel_menubar, sequel, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	widget = desktop_toolbar_create(_sequel_toolbar, sequel, group);
	g_object_unref(group);
	gtk_widget_set_sensitive(GTK_WIDGET(_sequel_toolbar[4].widget), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(_sequel_toolbar[8].widget), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 18, 0)
	/* infobar */
	sequel->infobar = gtk_info_bar_new_with_buttons(GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE, NULL);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(sequel->infobar),
			GTK_MESSAGE_ERROR);
	g_signal_connect(sequel->infobar, "close", G_CALLBACK(gtk_widget_hide),
			NULL);
	g_signal_connect(sequel->infobar, "response", G_CALLBACK(
				gtk_widget_hide), NULL);
	widget = gtk_info_bar_get_content_area(GTK_INFO_BAR(sequel->infobar));
	sequel->infobar_label = gtk_label_new(NULL);
	gtk_widget_show(sequel->infobar_label);
	gtk_box_pack_start(GTK_BOX(widget), sequel->infobar_label, TRUE, TRUE,
			0);
	gtk_widget_set_no_show_all(sequel->infobar, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), sequel->infobar, FALSE, TRUE, 0);
#endif
	/* view */
	sequel->notebook = gtk_notebook_new();
#if GTK_CHECK_VERSION(2, 10, 0)
	g_signal_connect(sequel->notebook, "page-reordered", G_CALLBACK(
				_sequel_on_tab_reordered), sequel);
#endif
	g_signal_connect(sequel->notebook, "switch-page", G_CALLBACK(
				_sequel_on_tab_switched), sequel);
	gtk_box_pack_start(GTK_BOX(vbox), sequel->notebook, TRUE, TRUE, 0);
	/* statusbar */
	sequel->statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), sequel->statusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(sequel->window), vbox);
	_sequel_set_status(sequel, _("Not connected"));
	gtk_widget_show_all(vbox);
	/* error console */
	sequel->lo_window = NULL;
	sequel->lo_store = gtk_list_store_new(3, G_TYPE_UINT, G_TYPE_STRING,
			G_TYPE_STRING);
	if(_sequel_open_tab(sequel) != 0)
	{
		sequel_delete(sequel);
		return NULL;
	}
	gtk_widget_show(sequel->window);
	return sequel;
}


/* sequel_delete */
void sequel_delete(Sequel * sequel)
{
	if(sequel->lo_window != NULL)
		gtk_widget_destroy(sequel->lo_window);
	if(sequel->database != NULL)
		database_delete(sequel->database);
	if(sequel->window != NULL)
		gtk_widget_destroy(sequel->window);
	if(sequel->font != NULL)
		pango_font_description_free(sequel->font);
	object_delete(sequel);
}


/* accessors */
/* sequel_set_status */
static void _sequel_set_status(Sequel * sequel, char const * status)
{
	GtkStatusbar * statusbar = GTK_STATUSBAR(sequel->statusbar);
	guint id;

	id = gtk_statusbar_get_context_id(statusbar, "");
	gtk_statusbar_pop(statusbar, id);
	gtk_statusbar_push(statusbar, id, status);
}


/* useful */
/* sequel_error */
static int _error_text(char const * message, int ret);

int sequel_error(Sequel * sequel, char const * message, int ret)
{
#if !GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * dialog;
#endif
	GtkTreeIter iter;
	time_t date;
	struct tm t;
	char buf[32];

	if(sequel == NULL)
		return _error_text(message, ret);
#if GTK_CHECK_VERSION(2, 18, 0)
	gtk_label_set_text(GTK_LABEL(sequel->infobar_label), message);
	gtk_widget_show(sequel->infobar);
#else
	dialog = gtk_message_dialog_new(GTK_WINDOW(sequel->window),
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
#endif
	gtk_list_store_append(sequel->lo_store, &iter);
	date = time(NULL);
	localtime_r(&date, &t);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &t);
	gtk_list_store_set(sequel->lo_store, &iter, 0, date, 1, buf, 2, message,
			-1);
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", "sequel", message);
	return ret;
}


/* sequel_show_console */
static void _console_window(Sequel * sequel);
/* callbacks */
static gboolean _console_on_closex(gpointer data);

void sequel_show_console(Sequel * sequel, gboolean show)
{
	if(sequel->lo_window == NULL)
		_console_window(sequel);
	if(show)
		gtk_window_present(GTK_WINDOW(sequel->lo_window));
	else
		gtk_widget_hide(sequel->lo_window);
}

static void _console_window(Sequel * sequel)
{
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	sequel->lo_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(sequel->lo_window), 400, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(sequel->lo_window), "logviewer");
#endif
	gtk_window_set_title(GTK_WINDOW(sequel->lo_window), _("Log console"));
	g_signal_connect_swapped(sequel->lo_window, "delete-event", G_CALLBACK(
				_console_on_closex), sequel);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	sequel->lo_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				sequel->lo_store));
	/* columns */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Timestamp"),
			renderer, "text", 1, NULL);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(sequel->lo_view), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Message"),
			renderer, "text", 2, NULL);
	gtk_tree_view_column_set_sort_column_id(column, 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(sequel->lo_view), column);
	gtk_container_add(GTK_CONTAINER(widget), sequel->lo_view);
	gtk_widget_show_all(widget);
	gtk_container_add(GTK_CONTAINER(sequel->lo_window), widget);
}

static gboolean _console_on_closex(gpointer data)
{
	Sequel * sequel = data;

	gtk_widget_hide(sequel->lo_window);
	return TRUE;
}


/* private */
/* functions */
/* sequel_clear */
static void _sequel_clear(Sequel * sequel)
{
	gint i;
	GtkWidget * text;
	GtkTextBuffer * tbuf;

	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	text = sequel->tabs[i].text;
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(tbuf, "", 0);
}


/* sequel_close_all */
static void _sequel_close_all(Sequel * sequel)
{
	gtk_widget_hide(sequel->window);
	gtk_main_quit();
}


/* sequel_close_tab */
static void _sequel_close_tab(Sequel * sequel, unsigned int i)
{
	gtk_notebook_remove_page(GTK_NOTEBOOK(sequel->notebook), i);
	memmove(&sequel->tabs[i], &sequel->tabs[i + 1],
			(sequel->tabs_cnt - (i + 1)) * sizeof(*sequel->tabs));
	if(--sequel->tabs_cnt == 0)
		gtk_main_quit();
}


/* sequel_connect */
static int _sequel_connect(Sequel * sequel, char const * engine,
		char const * filename, char const * section)
{
	Config * config;
	gchar * buf;

	if(engine == NULL || filename == NULL)
		return _sequel_connect_dialog(sequel);
	if(sequel->database != NULL)
	{
		database_delete(sequel->database);
		sequel->database = NULL;
	}
	if((config = config_new()) != NULL
			&& config_load(config, filename) == 0)
		sequel->database = database_new(engine, config, section);
	if(sequel->database == NULL)
	{
		sequel_error(sequel, error_get(), 1);
		gtk_widget_set_sensitive(GTK_WIDGET(_sequel_toolbar[4].widget),
				FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(_sequel_toolbar[4].widget),
				TRUE);
		/* update the title */
		buf = g_strdup_printf("%s - %s%s%s", _("Sequel"), filename,
				(section != NULL && strlen(section)) ? " - "
				: "", (section != NULL && strlen(section))
				? section : "");
		gtk_window_set_title(GTK_WINDOW(sequel->window), buf);
		g_free(buf);
		/* update the status */
		buf = g_strdup_printf(_("Connected to %s"), engine);
		_sequel_set_status(sequel, buf);
		g_free(buf);
	}
	if(config != NULL)
		config_delete(config);
	return (sequel->database != NULL) ? 0 : -1;
}


/* sequel_connect_dialog */
static void _connect_dialog_engines(GtkWidget * combobox);
static void _connect_dialog_on_file_set(GtkWidget * widget, gpointer data);
static void _connect_dialog_config_foreach(char const * section, void * data);

static int _sequel_connect_dialog(Sequel * sequel)
{
	int ret;
	GtkSizeGroup * group;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * label;
	GtkWidget * entry1;
	GtkWidget * filesel;
	GtkFileFilter * filter;
	GtkWidget * entry2;
	gchar const * engine;
	gchar * filename;
	gchar const * section;

	dialog = gtk_dialog_new_with_buttons(_("Connect..."),
			GTK_WINDOW(sequel->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT, NULL);
	group = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	/* engine */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	label = gtk_label_new(_("Engine:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_size_group_add_widget(group, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	entry1 = gtk_combo_box_new_text();
	_connect_dialog_engines(entry1);
	gtk_box_pack_start(GTK_BOX(hbox), entry1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* filename */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	label = gtk_label_new(_("Connection file:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_size_group_add_widget(group, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	filesel = gtk_file_chooser_button_new(_("Open connection file..."),
			GTK_FILE_CHOOSER_ACTION_OPEN);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Connection files"));
	gtk_file_filter_add_pattern(filter, "*.conf");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel), filter);
	gtk_box_pack_start(GTK_BOX(hbox), filesel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* section */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	label = gtk_label_new(_("Section:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_size_group_add_widget(group, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	entry2 = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox), entry2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	g_signal_connect(filesel, "file-set", G_CALLBACK(
				_connect_dialog_on_file_set), entry2);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy(dialog);
		return 0;
	}
	gtk_widget_hide(dialog);
	engine = gtk_combo_box_get_active_text(GTK_COMBO_BOX(entry1));
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
	section = gtk_combo_box_get_active_text(GTK_COMBO_BOX(entry2));
	if(filename == NULL)
		/* FIXME report error */
		ret = -1;
	else
		ret = _sequel_connect(sequel, engine, filename, section);
	g_free(filename);
	gtk_widget_destroy(dialog);
	return ret;
}

static void _connect_dialog_engines(GtkWidget * combobox)
{
	char const path[] = LIBDIR "/Database/engine";
	DIR * dir;
	struct dirent * de;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	size_t len;

	if((dir = opendir(path)) == NULL)
		return;
	while((de = readdir(dir)) != NULL)
	{
		if(strcmp(de->d_name, ".") == 0
				|| strcmp(de->d_name, "..") == 0)
			continue;
		if((len = strlen(de->d_name)) < sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), de->d_name);
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	}
	closedir(dir);
}

static void _connect_dialog_on_file_set(GtkWidget * widget, gpointer data)
{
	GtkWidget * entry2 = data;
	GtkTreeModel * model;
	GtkListStore * store;
	gchar * filename;
	Config * config;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(entry2));
	store = GTK_LIST_STORE(model);
	gtk_list_store_clear(store);
	if((filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)))
			== NULL)
		return;
	if((config = config_new()) != NULL)
		config_load(config, filename);
	g_free(filename);
	if(config == NULL)
		return;
	config_foreach(config, _connect_dialog_config_foreach, entry2);
	config_delete(config);
}

static void _connect_dialog_config_foreach(char const * section, void * data)
{
	GtkWidget * entry2 = data;

	gtk_combo_box_append_text(GTK_COMBO_BOX(entry2), section);
	gtk_combo_box_set_active(GTK_COMBO_BOX(entry2), 0);
}


/* sequel_execute */
static int _execute_on_callback(void * data, int argc, char ** argv,
		char ** columns);

static int _sequel_execute(Sequel * sequel)
{
	gint i;
	GtkTextBuffer * buffer;
	GtkTextIter start;
	GtkTextIter end;
	gchar * query;
	int res;

	if(sequel->database == NULL)
		return _sequel_connect_dialog(sequel);
	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sequel->tabs[i].text));
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	query = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	if(sequel->tabs[i].store != NULL)
	{
		gtk_list_store_clear(sequel->tabs[i].store);
		gtk_tree_view_set_model(GTK_TREE_VIEW(sequel->tabs[i].view),
				NULL);
		g_object_unref(sequel->tabs[i].store);
		sequel->tabs[i].store = NULL;
	}
	res = database_query(sequel->database, query, _execute_on_callback,
			sequel);
	g_free(query);
	gtk_widget_set_sensitive(GTK_WIDGET(_sequel_toolbar[8].widget),
			(sequel->tabs[i].store != NULL) ? TRUE : FALSE);
	if(res != 0)
		return -sequel_error(sequel, error_get(), 1);
	return 0;
}

static int _execute_on_callback(void * data, int argc, char ** argv,
		char ** columns)
{
	Sequel * sequel = data;
	gint i;
	GtkTreeView * view;
	GtkListStore * store;
	GList * l;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * c;
	GtkWidget * widget;
	GList * p;
	GtkTreeIter iter;

	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	view = GTK_TREE_VIEW(sequel->tabs[i].view);
	if((store = sequel->tabs[i].store) == NULL)
	{
		/* create the current store */
		/* XXX no longer hard-code the number of columns */
		sequel->tabs[i].store = gtk_list_store_new(COLUMN_CNT,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				G_TYPE_STRING);
		store = sequel->tabs[i].store;
		gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
		if((l = gtk_tree_view_get_columns(view)) == NULL)
		{
			/* create the rendering columns */
			for(i = 0; i < COLUMN_CNT; i++)
			{
				renderer = gtk_cell_renderer_text_new();
				c = gtk_tree_view_column_new_with_attributes("",
						renderer, "text", i, NULL);
				/* use our own label as the column title */
				widget = gtk_label_new(NULL);
				gtk_widget_show(widget);
				gtk_tree_view_column_set_widget(c, widget);
				gtk_tree_view_column_set_resizable(c, TRUE);
				gtk_tree_view_column_set_sort_column_id(c, i);
				gtk_tree_view_append_column(view, c);
			}
			l = gtk_tree_view_get_columns(view);
		}
		/* set the visible columns (and their respective title) */
		for(p = l, i = 0; p != NULL && i < MIN(argc, COLUMN_CNT);
				p = p->next, i++)
		{
			gtk_tree_view_column_set_title(p->data, columns[i]);
			widget = gtk_tree_view_column_get_widget(p->data);
			gtk_label_set_text(GTK_LABEL(widget), columns[i]);
			gtk_tree_view_column_set_visible(p->data, TRUE);
		}
		/* hide the remaining columns */
		for(; p != NULL && i < COLUMN_CNT; p = p->next, i++)
			gtk_tree_view_column_set_visible(p->data, FALSE);
		gtk_tree_view_columns_autosize(view);
		g_list_free(l);
	}
	gtk_list_store_append(store, &iter);
	for(i = 0; i < MIN(argc, COLUMN_CNT); i++)
		/* XXX the data may not be valid UTF-8 */
		gtk_list_store_set(store, &iter, i, argv[i], -1);
	return 0;
}


/* sequel_export */
static int _sequel_export(Sequel * sequel, char const * filename)
{
	struct stat st;
	GtkWidget * dialog;
	gboolean res;
	FILE * fp;
	gint i;
	GtkWidget * view;
	GtkTreeModel * model;
	GList * columns;
	gboolean valid;
	GtkTreeIter iter;
	gchar * buf;
	char const * sep = "";
	size_t len;
	GList * p;

	if(filename == NULL)
		return _sequel_export_dialog(sequel);
	if(stat(filename, &st) == 0)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(sequel->window),
				GTK_DIALOG_MODAL
				| GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
				"%s", _("Question"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(
					dialog),
#endif
				"%s", _("This file already exists."
					" Overwrite?"));
		gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(res == GTK_RESPONSE_NO)
			return -1;
	}
	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	if(i < 0)
		return -1;
	view = sequel->tabs[i].view;
	model = GTK_TREE_MODEL(sequel->tabs[i].store);
	if((fp = fopen(filename, "w")) == NULL)
		return -sequel_error(sequel, strerror(errno), 1);
	columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(view));
	for(valid = gtk_tree_model_get_iter_first(model, &iter);
			valid; valid = gtk_tree_model_iter_next(model, &iter))
	{
		for(p = columns, i = 0; p != NULL; p = p->next, i++)
		{
			if(gtk_tree_view_column_get_visible(p->data) != TRUE)
				break;
			gtk_tree_model_get(model, &iter, i, &buf, -1);
			/* XXX buf may contain commas */
			len = (buf != NULL) ? strlen(buf) : 0;
			if(fwrite(sep, sizeof(*sep), strlen(sep), fp)
					!= strlen(sep)
					|| fwrite(buf, sizeof(*buf), len, fp)
					!= len)
			{
				g_free(buf);
				fclose(fp);
				unlink(filename);
				return -sequel_error(sequel, strerror(errno),
						1);
			}
			g_free(buf);
			sep = ",";
		}
		if(fwrite("\n", sizeof(char), 1, fp) != sizeof(char))
		{
			fclose(fp);
			unlink(filename);
			return -sequel_error(sequel, strerror(errno), 1);
		}
		sep = "";
	}
	if(fclose(fp) != 0)
	{
		unlink(filename);
		return -sequel_error(sequel, strerror(errno), 1);
	}
	return 0;
}


/* sequel_export_dialog */
static int _sequel_export_dialog(Sequel * sequel)
{
	int ret;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	gchar * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Export..."),
			GTK_WINDOW(sequel->window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("CSV files"));
	gtk_file_filter_add_mime_type(filter, "text/csv");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return FALSE;
	ret = _sequel_export(sequel, filename);
	g_free(filename);
	return ret;
}


/* sequel_load */
static int _sequel_load(Sequel * sequel, char const * filename)
{
	int ret = 0;
	gint i;
	GtkWidget * text;
	GtkTextBuffer * tbuf;
	FILE * fp;
	char buf[BUFSIZ];
	size_t size;
	GtkTextIter iter;

	if(filename == NULL)
		return _sequel_load_dialog(sequel);
	if((fp = fopen(filename, "r")) == NULL)
		return -sequel_error(sequel, strerror(errno), 1);
	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	text = sequel->tabs[i].text;
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(tbuf, "", 0);
	while((size = fread(buf, sizeof(*buf), sizeof(buf), fp)) > 0)
	{
		gtk_text_buffer_get_end_iter(tbuf, &iter);
		gtk_text_buffer_insert(tbuf, &iter, buf, size);
	}
	if(ferror(fp))
		ret = -sequel_error(sequel, strerror(errno), 1);
	fclose(fp);
	return ret;
}


/* sequel_load_dialog */
static int _sequel_load_dialog(Sequel * sequel)
{
	int ret;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	gchar * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Load query..."),
			GTK_WINDOW(sequel->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("SQL files"));
	gtk_file_filter_add_mime_type(filter, "text/x-sql");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return FALSE;
	ret = _sequel_load(sequel, filename);
	g_free(filename);
	return ret;
}


/* sequel_open_tab */
static int _sequel_open_tab(Sequel * sequel)
{
	SequelTab * p;
	GtkWidget * paned;
	GtkWidget * widget;
	char buf[24];

	if((p = realloc(sequel->tabs, sizeof(*p) * (sequel->tabs_cnt + 1)))
			== NULL)
		return -1;
	sequel->tabs = p;
	p = &sequel->tabs[sequel->tabs_cnt++];
	/* create the tab */
#if GTK_CHECK_VERSION(3, 0, 0)
	p->label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	p->label = gtk_hbox_new(FALSE, 4);
#endif
	snprintf(buf, sizeof(buf), _("Tab %lu"), sequel->tabs_cnt);
	gtk_box_pack_start(GTK_BOX(p->label), gtk_label_new(buf), TRUE, TRUE,
			0);
	widget = gtk_button_new();
	g_signal_connect(widget, "clicked", G_CALLBACK(_sequel_on_tab_close),
			sequel);
	gtk_container_add(GTK_CONTAINER(widget), gtk_image_new_from_stock(
				GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(p->label), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(p->label);
	paned = gtk_vpaned_new();
	p->page = paned;
	/* text area */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	p->text = gtk_text_view_new();
	gtk_widget_modify_font(p->text, sequel->font);
	gtk_container_add(GTK_CONTAINER(widget), p->text);
	gtk_paned_add1(GTK_PANED(paned), widget);
	/* results area */
	p->store = NULL;
	p->view = gtk_tree_view_new();
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(p->view), TRUE);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(widget), p->view);
	gtk_paned_add2(GTK_PANED(paned), widget);
	gtk_widget_show_all(paned);
	gtk_notebook_append_page(GTK_NOTEBOOK(sequel->notebook), paned,
			p->label);
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(sequel->notebook), paned,
			TRUE);
#endif
	return 0;
}


/* sequel_save_as */
static int _sequel_save_as(Sequel * sequel, char const * filename)
{
	struct stat st;
	GtkWidget * dialog;
	gboolean res;
	FILE * fp;
	gint i;
	GtkWidget * text;
	GtkTextBuffer * tbuf;
	GtkTextIter start;
	GtkTextIter end;
	gchar * buf;
	size_t len;

	if(filename == NULL)
		return _sequel_save_as_dialog(sequel);
	if(stat(filename, &st) == 0)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(sequel->window),
				GTK_DIALOG_MODAL
				| GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
				"%s", _("Question"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(
					dialog),
#endif
				"%s", _("This file already exists."
					" Overwrite?"));
		gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(res == GTK_RESPONSE_NO)
			return -1;
	}
	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	if(i < 0)
		return -1;
	text = sequel->tabs[i].text;
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	if((fp = fopen(filename, "w")) == NULL)
		return -sequel_error(sequel, strerror(errno), 1);
	/* XXX allocating the complete file is not optimal */
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(tbuf), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tbuf), &end);
	buf = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(tbuf), &start, &end,
			FALSE);
	len = strlen(buf);
	if(fwrite(buf, sizeof(*buf), len, fp) != len)
	{
		g_free(buf);
		fclose(fp);
		unlink(filename);
		return -sequel_error(sequel, strerror(errno), 1);
	}
	g_free(buf);
	if(fclose(fp) != 0)
	{
		unlink(filename);
		return -sequel_error(sequel, strerror(errno), 1);
	}
	return 0;
}


/* sequel_save_as_dialog */
static int _sequel_save_as_dialog(Sequel * sequel)
{
	int ret;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	gchar * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Save as..."),
			GTK_WINDOW(sequel->window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("SQL files"));
	gtk_file_filter_add_mime_type(filter, "text/x-sql");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return FALSE;
	ret = _sequel_save_as(sequel, filename);
	g_free(filename);
	return ret;
}


/* callbacks */
/* sequel_on_close */
static void _sequel_on_close(gpointer data)
{
	Sequel * sequel = data;
	gint i;

	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	if(i < 0)
		return;
	_sequel_close_tab(sequel, i);
}


/* sequel_on_closex */
static gboolean _on_closex_confirm(Sequel * sequel);

static gboolean _sequel_on_closex(gpointer data)
{
	Sequel * sequel = data;

	if(sequel->tabs_cnt > 1 && _on_closex_confirm(sequel) != TRUE)
		return TRUE;
	_sequel_close_all(sequel);
	return TRUE;
}

static gboolean _on_closex_confirm(Sequel * sequel)
{
	GtkWidget * dialog;
	gboolean res;

	dialog = gtk_message_dialog_new(GTK_WINDOW(sequel->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", _("There are multiple tabs opened.\n"
				"Do you really want to close every tab"
				" opened in this window?"));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return (res == GTK_RESPONSE_CLOSE) ? TRUE : FALSE;
}


/* sequel_on_connect */
static void _sequel_on_connect(gpointer data)
{
	Sequel * sequel = data;

	_sequel_connect_dialog(sequel);
}


/* sequel_on_execute */
static void _sequel_on_execute(gpointer data)
{
	Sequel * sequel = data;

	_sequel_execute(sequel);
}


/* sequel_on_export */
static void _sequel_on_export(gpointer data)
{
	Sequel * sequel = data;

	_sequel_export_dialog(sequel);
}


/* sequel_on_file_close */
static void _sequel_on_file_close(gpointer data)
{
	Sequel * sequel = data;

	_sequel_on_close(sequel);
}


/* sequel_on_file_close */
static void _sequel_on_file_close_all(gpointer data)
{
	Sequel * sequel = data;

	_sequel_close_all(sequel);
}


/* sequel_on_file_connect */
static void _sequel_on_file_connect(gpointer data)
{
	Sequel * sequel = data;

	_sequel_on_connect(sequel);
}


/* sequel_on_file_new_tab */
static void _sequel_on_file_new_tab(gpointer data)
{
	Sequel * sequel = data;

	_sequel_on_new_tab(sequel);
}


/* sequel_on_help_about */
static void _sequel_on_help_about(gpointer data)
{
	Sequel * sequel = data;
	GtkWidget * dialog;

	dialog = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
			GTK_WINDOW(sequel->window));
	desktop_about_dialog_set_authors(dialog, _authors);
	desktop_about_dialog_set_comments(dialog,
			_("SQL console for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "stock_insert-table");
	desktop_about_dialog_set_name(dialog, "Sequel");
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* sequel_on_help_contents */
static void _sequel_on_help_contents(gpointer data)
{
	desktop_help_contents(PACKAGE, "sequel");
}


/* sequel_on_load */
static void _sequel_on_load(gpointer data)
{
	Sequel * sequel = data;

	_sequel_load_dialog(sequel);
}


/* sequel_on_new_tab */
static void _sequel_on_new_tab(gpointer data)
{
	Sequel * sequel = data;

	_sequel_open_tab(sequel);
}


/* sequel_on_query_clear */
static void _sequel_on_query_clear(gpointer data)
{
	Sequel * sequel = data;

	_sequel_clear(sequel);
}


/* sequel_on_query_execute */
static void _sequel_on_query_execute(gpointer data)
{
	Sequel * sequel = data;

	_sequel_execute(sequel);
}


/* sequel_on_query_export */
static void _sequel_on_query_export(gpointer data)
{
	Sequel * sequel = data;

	_sequel_export_dialog(sequel);
}


/* sequel_on_query_load */
static void _sequel_on_query_load(gpointer data)
{
	Sequel * sequel = data;

	_sequel_load_dialog(sequel);
}


/* sequel_on_query_save_as */
static void _sequel_on_query_save_as(gpointer data)
{
	Sequel * sequel = data;

	_sequel_save_as_dialog(sequel);
}


/* sequel_on_save_as */
static void _sequel_on_save_as(gpointer data)
{
	Sequel * sequel = data;

	_sequel_save_as_dialog(sequel);
}


/* sequel_on_tab_close */
static void _sequel_on_tab_close(GtkWidget * widget, gpointer data)
{
	Sequel * sequel = data;
	size_t i;

	widget = gtk_widget_get_parent(widget);
	for(i = 0; i < sequel->tabs_cnt; i++)
		if(sequel->tabs[i].label == widget)
			break;
	if(i >= sequel->tabs_cnt)
		/* should not happen */
		return;
	_sequel_close_tab(sequel, i);
}


#if GTK_CHECK_VERSION(2, 10, 0)
/* sequel_on_tab_reordered */
static void _sequel_on_tab_reordered(GtkWidget * widget, GtkWidget * child,
		guint page, gpointer data)
{
	Sequel * sequel = data;
	SequelTab tab;
	guint old = 0;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, page);
#endif
	for(i = 0; i < sequel->tabs_cnt; i++)
		if(sequel->tabs[i].page == child)
		{
			old = i;
			break;
		}
	if(i == sequel->tabs_cnt)
		return;
	memcpy(&tab, &sequel->tabs[old], sizeof(tab));
	memcpy(&sequel->tabs[old], &sequel->tabs[page], sizeof(tab));
	memcpy(&sequel->tabs[page], &tab, sizeof(tab));
}
#endif


/* sequel_on_tab_switched */
static void _sequel_on_tab_switched(GtkWidget * widget, GtkWidget * child,
		guint page, gpointer data)
{
	Sequel * sequel = data;
	GtkListStore * store;
	gboolean active;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, page);
#endif
	active = ((store = sequel->tabs[page].store) != NULL) ? TRUE : FALSE;
	gtk_widget_set_sensitive(GTK_WIDGET(_sequel_toolbar[8].widget), active);
}


/* sequel_on_view_error_console */
static void _sequel_on_view_error_console(gpointer data)
{
	Sequel * sequel = data;

	sequel_show_console(sequel, TRUE);
}
