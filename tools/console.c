/* $Id$ */
static char const _copyright[] =
"Copyright © 2015-2017 Pierre Pronchery <khorben@defora.org>";
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



#include <stdio.h>
#include <Desktop.h>
#include "../config.h"

#ifndef PROGNAME_CONSOLE
# define PROGNAME_CONSOLE	"console"
#endif
#ifndef PROGNAME_PHP
# define PROGNAME_PHP	"php"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif


/* private */
/* types */
typedef struct _Console
{
	GtkWidget * window;
	GtkWidget * code;
	GtkWidget * console;
} Console;


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
static int _console(void);


/* functions */
/* console */
/* callbacks */
static void _console_on_about(gpointer data);
static void _console_on_clear(gpointer data);
static void _console_on_close(gpointer data);
static gboolean _console_on_closex(gpointer data);
static void _console_on_execute(gpointer data);
static void _execute_php(Console * console, char * text);

static int _console(void)
{
	/* FIXME allocate dynamically */
	static Console console;
	PangoFontDescription * desc;
	GtkWidget * vbox;
	GtkWidget * menuitem;
	GtkWidget * menu;
	GtkToolItem * toolitem;
	GtkWidget * paned;
	GtkWidget * widget;

	desc = pango_font_description_new();
	pango_font_description_set_family(desc, "Monospace");
	console.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(console.window), 400, 300);
	gtk_window_set_title(GTK_WINDOW(console.window), "PHP Console");
	g_signal_connect_swapped(console.window, "delete-event",
			G_CALLBACK(_console_on_closex), NULL);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	/* menu bar */
	widget = gtk_menu_bar_new();
	/* menu bar: file */
	menuitem = gtk_menu_item_new_with_mnemonic("_File");
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), menuitem);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	menuitem = gtk_menu_item_new_with_mnemonic("_Run");
	g_signal_connect_swapped(menuitem, "activate",
			G_CALLBACK(_console_on_execute), &console);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_menu_item_new_with_mnemonic("C_lear");
	g_signal_connect_swapped(menuitem, "activate",
			G_CALLBACK(_console_on_clear), &console);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_menu_item_new_with_mnemonic("_Close");
	g_signal_connect_swapped(menuitem, "activate",
			G_CALLBACK(_console_on_close), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* menu bar: help */
	menuitem = gtk_menu_item_new_with_mnemonic("_Help");
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), menuitem);
	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), menu);
	menuitem = gtk_menu_item_new_with_mnemonic("_About");
	g_signal_connect_swapped(menuitem, "activate",
			G_CALLBACK(_console_on_about), &console);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	widget = gtk_toolbar_new();
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_EXECUTE);
	g_signal_connect_swapped(toolitem, "clicked",
			G_CALLBACK(_console_on_execute), &console);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR);
	g_signal_connect_swapped(toolitem, "clicked",
			G_CALLBACK(_console_on_clear), &console);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* paned */
	paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	console.code = gtk_text_view_new();
	gtk_widget_override_font(console.code, desc);
	gtk_container_add(GTK_CONTAINER(widget), console.code);
	gtk_paned_add1(GTK_PANED(paned), widget);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	console.console = gtk_text_view_new();
	gtk_widget_override_font(console.console, desc);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(console.console), FALSE);
	gtk_container_add(GTK_CONTAINER(widget), console.console);
	gtk_paned_add2(GTK_PANED(paned), widget);
	gtk_paned_set_position(GTK_PANED(paned), 150);
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	/* statusbar */
	widget = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(console.window), vbox);
	gtk_widget_show_all(console.window);
	return 0;
}

/* callbacks */
static void _console_on_about(gpointer data)
{
	Console * console = data;
	GtkWidget * dialog;

	dialog = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
			GTK_WINDOW(console->window));
	desktop_about_dialog_set_authors(dialog, _authors);
	desktop_about_dialog_set_comments(dialog,
			"PHP console for the DeforaOS desktop");
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_name(dialog, "Console");
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "https://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void _console_on_clear(gpointer data)
{
	Console * console = data;
	GtkTextBuffer * tbuf;

	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console->console));
	gtk_text_buffer_set_text(tbuf, "", 0);
}

static void _console_on_close(gpointer data)
{
	_console_on_closex(data);
}

static gboolean _console_on_closex(gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

static void _console_on_execute(gpointer data)
{
	Console * console = data;
	GtkTextBuffer * tbuf;
	GtkTextIter start;
	GtkTextIter end;
	gchar * text;

	/* obtain the code */
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console->code));
	gtk_text_buffer_get_start_iter(tbuf, &start);
	gtk_text_buffer_get_end_iter(tbuf, &end);
	text = gtk_text_buffer_get_text(tbuf, &start, &end, FALSE);
	_execute_php(console, text);
	g_free(text);
}

static void _execute_php(Console * console, char * text)
{
	const unsigned int flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	char * argv[] = { BINDIR "/" PROGNAME_PHP, PROGNAME_PHP, "-r", NULL,
		NULL };
	gchar * output = NULL;
	int status;
	GError * error = NULL;
	GtkTextBuffer * tbuf;
	GtkTextIter end;

	argv[3] = text;
	/* FIXME implement asynchronously */
	if(g_spawn_sync(NULL, argv, NULL, flags, NULL, NULL, &output, NULL,
				&status, &error) == FALSE)
	{
		fprintf(stderr, "%s: %s (status: %d)\n", PROGNAME_CONSOLE,
				error->message, status);
		g_error_free(error);
		return;
	}
	/* append the output */
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console->console));
	gtk_text_buffer_get_end_iter(tbuf, &end);
	gtk_text_buffer_insert(tbuf, &end, output, -1);
	g_free(output);
}


/* public */
/* main */
int main(int argc, char * argv[])
{
	int ret;

	gtk_init(&argc, &argv);
	ret = _console();
	gtk_main();
	return (ret == 0) ? 0 : 2;
}
