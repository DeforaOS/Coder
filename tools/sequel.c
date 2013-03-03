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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include <Database.h>
#include "sequel.h"
#include "../config.h"


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
	GtkWidget * window;
	GtkWidget * notebook;
};

struct _SequelTab
{
	GtkWidget * label;
	GtkWidget * text;
	GtkListStore * store;
	GtkWidget * view;
};


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
static int _sequel_error(Sequel * sequel, char const * message, int res);

static int _sequel_open_tab(Sequel * sequel);
static void _sequel_close_tab(Sequel * sequel, unsigned int i);

static int _sequel_connect(Sequel * sequel, char const * engine,
		char const * filename, char const * section);
static int _sequel_connect_dialog(Sequel * sequel);

static int _sequel_execute(Sequel * sequel);

/* callbacks */
static void _sequel_on_close(gpointer data);
static gboolean _sequel_on_closex(gpointer data);
static void _sequel_on_connect(gpointer data);
static void _sequel_on_execute(gpointer data);

static void _sequel_on_new_tab(gpointer data);

static void _sequel_on_file_close_all(gpointer data);
static void _sequel_on_file_connect(gpointer data);
static void _sequel_on_file_new_tab(gpointer data);
static void _sequel_on_help_about(gpointer data);

static void _sequel_on_tab_close(GtkWidget * widget, gpointer data);


/* constants */
/* menubar */
static const DesktopMenu _sequel_file_menu[] =
{
	{ "New _tab", G_CALLBACK(_sequel_on_file_new_tab), "tab-new",
		GDK_CONTROL_MASK, GDK_KEY_T },
	{ "", NULL, NULL, 0, 0 },
	{ "Connect...", G_CALLBACK(_sequel_on_file_connect), NULL, 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ "Close all tabs", G_CALLBACK(_sequel_on_file_close_all), NULL, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _sequel_help_menu[] =
{
#if GTK_CHECK_VERSION(2, 6, 0)
	{ "About", G_CALLBACK(_sequel_on_help_about), GTK_STOCK_ABOUT, 0,
		0 },
#else
	{ "About", G_CALLBACK(_sequel_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _sequel_menubar[] =
{
	{ "_File", _sequel_file_menu },
	{ "_Help", _sequel_help_menu },
	{ NULL, NULL }
};


/* variables */
/* toolbar */
static DesktopToolbar _sequel_toolbar[] =
{
	{ "New tab", G_CALLBACK(_sequel_on_new_tab), "tab-new", 0, 0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ "Connect", G_CALLBACK(_sequel_on_connect), GTK_STOCK_CONNECT, 0, 0,
		NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ "Execute", G_CALLBACK(_sequel_on_execute), GTK_STOCK_EXECUTE, 0, 0,
		NULL },
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
	sequel->window = NULL;
	sequel->database = NULL;
	/* widgets */
	group = gtk_accel_group_new();
	sequel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(sequel->window), group);
	gtk_window_set_default_size(GTK_WINDOW(sequel->window), 640, 480);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(sequel->window),
			"stock_insert-table");
#endif
	g_object_unref(group);
	gtk_window_set_title(GTK_WINDOW(sequel->window), "Sequel");
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
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	sequel->notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), sequel->notebook, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(sequel->window), vbox);
	gtk_widget_show_all(vbox);
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
	if(sequel->database != NULL)
		database_delete(sequel->database);
	if(sequel->window != NULL)
		gtk_widget_destroy(sequel->window);
	object_delete(sequel);
}


/* sequel_error */
static int _error_text(char const * message, int ret);

int sequel_error(Sequel * sequel, char const * message, int ret)
{
	GtkWidget * dialog;

	if(sequel == NULL)
		return _error_text(message, ret);
	dialog = gtk_message_dialog_new(GTK_WINDOW(sequel->window),
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
	fprintf(stderr, "%s: %s\n", "sequel", message);
	return ret;
}


/* private */
/* functions */
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
		_sequel_error(sequel, NULL, 1);
	if(config != NULL)
		config_delete(config);
	return (sequel->database != NULL) ? 0 : -1;
}


/* sequel_connect_dialog */
static int _sequel_connect_dialog(Sequel * sequel)
{
	int ret;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * entry1;
	GtkWidget * filesel;
	GtkWidget * entry2;
	gchar const * engine;
	gchar * filename;
	gchar const * section;

	dialog = gtk_dialog_new_with_buttons("Connect...",
			GTK_WINDOW(sequel->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT, NULL);
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
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Engine:"), FALSE, TRUE,
			0);
	entry1 = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* filename */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Connection file:"),
			FALSE, TRUE, 0);
	filesel = gtk_file_chooser_button_new("Open connection file...",
			GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start(GTK_BOX(hbox), filesel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* section */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Section:"), FALSE,
			TRUE, 0);
	entry2 = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_destroy(dialog);
		return 0;
	}
	gtk_widget_hide(dialog);
	engine = gtk_entry_get_text(GTK_ENTRY(entry1));
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
	section = gtk_entry_get_text(GTK_ENTRY(entry2));
	if(filename == NULL)
		/* FIXME report error */
		ret = -1;
	else
		ret = _sequel_connect(sequel, engine, filename, section);
	g_free(filename);
	gtk_widget_destroy(dialog);
	return ret;
}


/* sequel_error */
static int _sequel_error(Sequel * sequel, char const * message, int res)
{
	/* FIXME really implement */
	error_print("sequel");
	return res;
}


/* sequel_execute */
static int _execute_on_callback(void * data, int argc, char ** argv,
		char ** columns);

static int _sequel_execute(Sequel * sequel)
{
	int i;
	GtkTextBuffer * buffer;
	GtkTextIter start;
	GtkTextIter end;
	gchar * query;

	if(sequel->database == NULL)
		return _sequel_connect_dialog(sequel);
	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(sequel->notebook));
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(sequel->tabs[i].text));
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	query = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	database_query(sequel->database, query, _execute_on_callback, sequel);
	g_free(query);
	/* FIXME really detect errors */
	return 0;
}

static int _execute_on_callback(void * data, int argc, char ** argv,
		char ** columns)
{
	/* FIXME really implement */
	char const * sep = "";
	int i;

	for(i = 0; i < argc; i++)
	{
		fprintf(stderr, "%s%s", sep, columns[i]);
		sep = ",";
	}
	sep = "\n";
	for(i = 0; i < argc; i++)
	{
		fprintf(stderr, "%s%s", sep, argv[i]);
		sep = ",";
	}
	fputs("\n", stderr);
	return 0;
}


/* sequel_open_tab */
static int _sequel_open_tab(Sequel * sequel)
{
	SequelTab * p;
	GtkWidget * vbox;
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
	snprintf(buf, sizeof(buf), "Tab %lu", sequel->tabs_cnt);
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
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* text area */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	p->text = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(widget), p->text);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	/* results area */
	p->store = gtk_list_store_new(0);
	p->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(p->store));
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(widget), p->view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(sequel->notebook), vbox,
			p->label);
	return 0;
}


/* callbacks */
/* sequel_on_close_all */
static void _sequel_on_close(gpointer data)
{
	Sequel * sequel = data;

	gtk_widget_hide(sequel->window);
	gtk_main_quit();
}


/* sequel_on_closex */
static gboolean _sequel_on_closex(gpointer data)
{
	Sequel * sequel = data;

	_sequel_on_close(sequel);
	return TRUE;
}


/* sequel_on_connect */
static void _sequel_on_connect(gpointer data)
{
	Sequel * sequel = data;

	_sequel_connect_dialog(sequel);
}


/* sequel_on_file_close */
static void _sequel_on_file_close_all(gpointer data)
{
	Sequel * sequel = data;

	_sequel_on_close(sequel);
}


/* sequel_on_file_connect */
static void _sequel_on_file_connect(gpointer data)
{
	Sequel * sequel = data;

	_sequel_on_connect(sequel);
}


/* sequel_on_execute */
static void _sequel_on_execute(gpointer data)
{
	Sequel * sequel = data;

	_sequel_execute(sequel);
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
			"SQL console for the DeforaOS desktop");
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "stock_insert-table");
	desktop_about_dialog_set_name(dialog, "Sequel");
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* sequel_on_new_tab */
static void _sequel_on_new_tab(gpointer data)
{
	Sequel * sequel = data;

	_sequel_open_tab(sequel);
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
	if(i == sequel->tabs_cnt)
		/* should not happen */
		return;
	_sequel_close_tab(sequel, i);
}
