/* $Id$ */
static char const _debugger_copyright[] =
"Copyright © 2015 Pierre Pronchery <khorben@defora.org>";
static char const _debugger_license[] =
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions\n"
"are met:\n"
"1. Redistributions of source code must retain the above copyright\n"
"   notice, this list of conditions and the following disclaimer.\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"   notice, this list of conditions and the following disclaimer in the\n"
"   documentation and/or other materials provided with the distribution.\n"
"3. Neither the name of the authors nor the names of the contributors\n"
"   may be used to endorse or promote products derived from this software\n"
"   without specific prior written permission.\n"
"THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS ``AS IS'' AND\n"
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
"ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE\n"
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
"OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
"LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
"OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
"SUCH DAMAGE.";



#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <Desktop.h>
#include "debugger.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

#ifndef PROGNAME
# define PROGNAME	"debugger"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef LIBDIR
# define LIBDIR		PREFIX "/lib"
#endif


/* Debugger */
/* private */
/* types */
struct _Debugger
{
	/* debug */
	DebuggerDebugHelper dhelper;
	Plugin * dplugin;
	DebuggerDebugDefinition * ddefinition;
	DebuggerDebug * debug;

	/* child */
	char * filename;

	/* widgets */
	GtkWidget * window;
	/* toolbar */
	/* disassembly */
	GtkWidget * das_view;
	/* registers */
	GtkWidget * reg_view;
	/* statusbar */
	GtkWidget * statusbar;
};


/* prototypes */
/* accessors */
static void _debugger_set_sensitive_toolbar(Debugger * debugger, gboolean run,
		gboolean debug);

/* useful */
static gboolean _debugger_confirm(Debugger * debugger, char const * message);
static gboolean _debugger_confirm_close(Debugger * debugger);
static gboolean _debugger_confirm_reset(Debugger * debugger);

/* helpers */
static int _debugger_helper_error(Debugger * debugger, int code,
		char const * format, ...);

/* callbacks */
static void _debugger_on_about(gpointer data);
static void _debugger_on_close(gpointer data);
static gboolean _debugger_on_closex(gpointer data);
static void _debugger_on_continue(gpointer data);
static void _debugger_on_next(gpointer data);
static void _debugger_on_open(gpointer data);
static void _debugger_on_pause(gpointer data);
static void _debugger_on_run(gpointer data);
static void _debugger_on_step(gpointer data);
static void _debugger_on_stop(gpointer data);


/* constants */
static char const * _debugger_authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};

static DesktopMenu const _debugger_menu_file[] =
{
	{ N_("_Open..."), G_CALLBACK(_debugger_on_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_debugger_on_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu const _debugger_menu_help[] =
{
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(_debugger_on_about), GTK_STOCK_ABOUT, 0, 0 },
#else
	{ N_("_About"), G_CALLBACK(_debugger_on_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenubar const _debugger_menubar[] =
{
	{ N_("_File"), _debugger_menu_file },
	{ N_("_Help"), _debugger_menu_help },
	{ NULL, NULL },
};


/* variables */
#define DEBUGGER_TOOLBAR_RUN		2
#define DEBUGGER_TOOLBAR_CONTINUE	4
#define DEBUGGER_TOOLBAR_PAUSE		5
#define DEBUGGER_TOOLBAR_STOP		6
#define DEBUGGER_TOOLBAR_STEP		7
#define DEBUGGER_TOOLBAR_NEXT		8
static DesktopToolbar _debugger_toolbar[] =
{
	{ N_("Open"), G_CALLBACK(_debugger_on_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Run"), G_CALLBACK(_debugger_on_run), GTK_STOCK_EXECUTE, 0,
		GDK_KEY_F10, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Continue"), G_CALLBACK(_debugger_on_continue),
		GTK_STOCK_MEDIA_PLAY, 0, GDK_KEY_F9, NULL },
	{ N_("Pause"), G_CALLBACK(_debugger_on_pause),
		GTK_STOCK_MEDIA_PAUSE, 0, 0, NULL },
	{ N_("Stop"), G_CALLBACK(_debugger_on_stop),
		GTK_STOCK_MEDIA_STOP, 0, GDK_KEY_F11, NULL },
	{ N_("Step"), G_CALLBACK(_debugger_on_step),
		GTK_STOCK_MEDIA_FORWARD, 0, 0, NULL },
	{ N_("Next"), G_CALLBACK(_debugger_on_next),
		GTK_STOCK_MEDIA_NEXT, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* XXX load at run-time */
#include "debug/ptrace.c"


/* public */
/* functions */
/* debugger_new */
Debugger * debugger_new(void)
{
	Debugger * debugger;
	GtkAccelGroup * accel;
	GtkWidget * vbox;
	GtkWidget * paned;
	GtkWidget * widget;
	GtkListStore * store;
	GtkTreeViewColumn * column;

	if((debugger = object_new(sizeof(*debugger))) == NULL)
		return NULL;
	/* debug */
	debugger->dhelper.debugger = debugger;
	debugger->dhelper.error = _debugger_helper_error;
	debugger->dplugin = NULL;
	debugger->ddefinition = NULL;
	debugger->debug = NULL;
	/* child */
	debugger->filename = NULL;
	/* widgets */
	accel = gtk_accel_group_new();
	debugger->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(debugger->window), accel);
	gtk_window_set_default_size(GTK_WINDOW(debugger->window), 640, 480);
	gtk_window_set_title(GTK_WINDOW(debugger->window), _("Debugger"));
	g_signal_connect_swapped(debugger->window, "delete-event", G_CALLBACK(
				_debugger_on_closex), debugger);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* menubar */
	widget = desktop_menubar_create(_debugger_menubar, debugger, accel);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	widget = desktop_toolbar_create(_debugger_toolbar, debugger, accel);
	g_object_unref(accel);
	_debugger_set_sensitive_toolbar(debugger, FALSE, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	paned = gtk_hpaned_new();
	/* assembly */
	debugger->das_view = gtk_text_view_new();
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(debugger->das_view),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(debugger->das_view), FALSE);
	gtk_paned_add1(GTK_PANED(paned), debugger->das_view);
	/* registers */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	store = gtk_list_store_new(2,
			G_TYPE_STRING,	/* name */
			G_TYPE_STRING);	/* value */
	debugger->reg_view = gtk_tree_view_new_with_model(
			GTK_TREE_MODEL(store));
	column = gtk_tree_view_column_new_with_attributes(_("Register"),
			gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debugger->reg_view), column);
	column = gtk_tree_view_column_new_with_attributes(_("Value"),
			gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debugger->reg_view), column);
	gtk_container_add(GTK_CONTAINER(widget), debugger->reg_view);
	gtk_paned_add2(GTK_PANED(paned), widget);
	gtk_paned_set_position(GTK_PANED(paned), 380);
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	/* statusbar */
	debugger->statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), debugger->statusbar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(debugger->window), vbox);
	gtk_widget_show_all(debugger->window);
	return debugger;
}


/* debugger_delete */
void debugger_delete(Debugger * debugger)
{
	if(debugger_is_running(debugger))
		debugger_stop(debugger);
	free(debugger->filename);
	gtk_widget_destroy(debugger->window);
	object_delete(debugger);
}


/* accessors */
/* debugger_is_opened */
int debugger_is_opened(Debugger * debugger)
{
	if(debugger_is_running(debugger))
		return TRUE;
	/* FIXME really implement */
	return FALSE;
}


/* debugger_is_running */
int debugger_is_running(Debugger * debugger)
{
	return (debugger->debug != NULL) ? 1 : 0;
}


/* useful */
/* debugger_close */
int debugger_close(Debugger * debugger)
{
	GtkTextBuffer * tbuf;
	GtkTreeModel * model;

	if(debugger_is_opened(debugger) == FALSE)
		return 0;
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(debugger->das_view));
	gtk_text_buffer_set_text(tbuf, "", 0);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(debugger->reg_view));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	/* FIXME really implement */
	free(debugger->filename);
	debugger->filename = NULL;
	gtk_window_set_title(GTK_WINDOW(debugger->window), _("Debugger"));
	return 0;
}


/* debugger_continue */
int debugger_continue(Debugger * debugger)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	return debugger->ddefinition->_continue(debugger->debug);
}


/* debugger_error */
static int _error_text(char const * message, int ret);

int debugger_error(Debugger * debugger, char const * message, int ret)
{
	const unsigned int flags = GTK_DIALOG_MODAL
		| GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget * widget;

	if(debugger == NULL)
		return _error_text(message, ret);
	widget = gtk_message_dialog_new(GTK_WINDOW(debugger->window), flags,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(widget),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(widget), _("Error"));
	gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
	return ret;
}

static int _error_text(char const * message, int ret)
{
	fprintf(stderr, "%s: %s\n", PROGNAME, message);
	return ret;
}


/* debugger_next */
int debugger_next(Debugger * debugger)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	return debugger->ddefinition->next(debugger->debug);
}


/* debugger_open */
int debugger_open(Debugger * debugger, char const * arch, char const * format,
		char const * filename)
{
	String * s;

	if(_debugger_confirm_close(debugger) == FALSE)
		return -1;
	if(filename == NULL)
		return debugger_open_dialog(debugger, arch, format);
	if(debugger_close(debugger) != 0)
		return -debugger_error(debugger, error_get(), 1);
	/* FIXME really implement */
	if((debugger->filename = strdup(filename)) == NULL)
		return -1;
	if((s = string_new_append(_("Debugger"), " - ", filename, NULL))
			!= NULL)
		gtk_window_set_title(GTK_WINDOW(debugger->window), s);
	string_delete(s);
	_debugger_set_sensitive_toolbar(debugger, TRUE, FALSE);
	return 0;
}


/* debugger_open_dialog */
static void _open_dialog_type(GtkWidget * combobox, char const * type,
		char const * value);

int debugger_open_dialog(Debugger * debugger, char const * arch,
		char const * format)
{
	int ret = 0;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * awidget;
	GtkWidget * fwidget;
	GtkWidget * widget;
	GtkFileFilter * filter;
	char * a = NULL;
	char * f = NULL;
	char * filename = NULL;

	if(_debugger_confirm_close(debugger) == FALSE)
		return -1;
	dialog = gtk_file_chooser_dialog_new(_("Open file..."),
			GTK_WINDOW(debugger->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	/* arch */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	awidget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(awidget), _("Auto-detect"));
	_open_dialog_type(awidget, "arch", arch);
	gtk_box_pack_end(GTK_BOX(hbox), awidget, FALSE, TRUE, 0);
	widget = gtk_label_new(_("Architecture:"));
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* format */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	fwidget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(fwidget), _("Auto-detect"));
	_open_dialog_type(fwidget, "format", format);
	gtk_box_pack_end(GTK_BOX(hbox), fwidget, FALSE, TRUE, 0);
	widget = gtk_label_new(_("File format:"));
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	/* executable files */
	filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("Executable files"));
        gtk_file_filter_add_mime_type(filter, "application/x-executable");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	/* all files */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(awidget)) == 0)
			a = NULL;
		else
			a = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
						awidget));
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(fwidget)) == 0)
			f = NULL;
		else
			f = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
						fwidget));
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	}
	gtk_widget_destroy(dialog);
	if(filename != NULL)
		ret = debugger_open(debugger, a, f, filename);
	g_free(a);
	g_free(f);
	g_free(filename);
	return ret;
}

static void _open_dialog_type(GtkWidget * combobox, char const * type,
		char const * value)
{
	char * path;
	DIR * dir;
	struct dirent * de;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	size_t len;
	int i;
	int active = 0;

	if((path = g_build_filename(LIBDIR, "Asm", type, NULL)) == NULL)
		return;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%s) \"%s\"\n", __func__, type, path);
#endif
	dir = opendir(path);
	g_free(path);
	if(dir == NULL)
		return;
	for(i = 0; (de = readdir(dir)) != NULL; i++)
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
		if(value != NULL && strcmp(de->d_name, value) == 0)
			active = i;
	}
	closedir(dir);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), active);
}


/* debugger_pause */
int debugger_pause(Debugger * debugger)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	return debugger->ddefinition->pause(debugger->debug);
}


/* debugger_run */
int debugger_run(Debugger * debugger, ...)
{
	int ret;
	va_list ap;

	va_start(ap, debugger);
	ret = debugger_runv(debugger, ap);
	va_end(ap);
	return ret;
}


/* debugger_runv */
int debugger_runv(Debugger * debugger, va_list ap)
{
	int ret;

	if(_debugger_confirm_reset(debugger) == FALSE)
		return -1;
	if(debugger_stop(debugger) != 0)
		return -1;
	debugger->ddefinition = &_ptrace_definition; /* XXX */
	if((debugger->debug = debugger->ddefinition->init(&debugger->dhelper))
			== NULL)
	{
		debugger_stop(debugger);
		return -1;
	}
	if((ret = debugger->ddefinition->start(debugger->debug, ap)) == 0)
		_debugger_set_sensitive_toolbar(debugger, TRUE, TRUE);
	return ret;
}


/* debugger_step */
int debugger_step(Debugger * debugger)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	return debugger->ddefinition->step(debugger->debug);
}


/* debugger_stop */
int debugger_stop(Debugger * debugger)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	debugger->ddefinition->stop(debugger->debug);
	debugger->ddefinition->destroy(debugger->debug);
	debugger->debug = NULL;
	debugger->ddefinition = NULL;
	if(debugger->dplugin != NULL)
		plugin_delete(debugger->dplugin);
	debugger->dplugin = NULL;
	_debugger_set_sensitive_toolbar(debugger, TRUE, FALSE);
	return 0;
}


/* private */
/* functions */
/* accessors */
/* debugger_set_sensitive_toolbar */
static void _debugger_set_sensitive_toolbar(Debugger * debugger, gboolean run,
		gboolean debug)
{
	const unsigned int widgets[] =
	{
		DEBUGGER_TOOLBAR_CONTINUE,
		DEBUGGER_TOOLBAR_PAUSE,
		DEBUGGER_TOOLBAR_STOP,
		DEBUGGER_TOOLBAR_STEP,
		DEBUGGER_TOOLBAR_NEXT
	};
	size_t i;

	gtk_widget_set_sensitive(GTK_WIDGET(
				_debugger_toolbar[DEBUGGER_TOOLBAR_RUN].widget),
			run);
	for(i = 0; i < sizeof(widgets) / sizeof(*widgets); i++)
		gtk_widget_set_sensitive(GTK_WIDGET(
					_debugger_toolbar[widgets[i]].widget),
				debug);
}


/* useful */
/* debugger_confirm */
static gboolean _debugger_confirm(Debugger * debugger, char const * message)
{
	const unsigned int flags = GTK_DIALOG_MODAL
		| GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget * widget;
	int res;

	widget = gtk_message_dialog_new(GTK_WINDOW(debugger->window), flags,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(widget),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(widget), _("Question"));
	res = gtk_dialog_run(GTK_DIALOG(widget));
	gtk_widget_destroy(widget);
	return (res == GTK_RESPONSE_YES) ? TRUE : FALSE;
}


/* debugger_confirm_close */
static gboolean _debugger_confirm_close(Debugger * debugger)
{
	if(debugger_is_opened(debugger)
			&& _debugger_confirm(debugger, _(
					"A file is already opened.\n"
					"Would you like to close it?\n"
					"Any progress will be lost.")) == FALSE)
		return FALSE;
	return TRUE;
}


/* debugger_confirm_reset */
static gboolean _debugger_confirm_reset(Debugger * debugger)
{
	if(debugger_is_running(debugger)
			&& _debugger_confirm(debugger, _(
					"A process is being debugged already\n"
					"Would you like to restart it?\n"
					"Any progress will be lost.")) == FALSE)
		return FALSE;
	return TRUE;
}


/* helpers */
/* debugger_helper_error */
static int _debugger_helper_error(Debugger * debugger, int code,
		char const * format, ...)
{
	va_list ap;
	int size;
	char * message;

	va_start(ap, format);
	size = vsnprintf(NULL, 0, format, ap);
	message = (size >= 0) ? malloc(size + 1) : NULL;
	if(message != NULL)
		vsnprintf(message, size + 1, format, ap);
	va_end(ap);
	if(message == NULL)
		return code;
	debugger_error(debugger, message, code);
	free(message);
	return code;
}


/* callbacks */
/* debugger_on_about */
static void _debugger_on_about(gpointer data)
{
	Debugger * debugger = data;
	GtkWidget * dialog;

	dialog = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(
				debugger->window));
	desktop_about_dialog_set_authors(dialog, _debugger_authors);
	desktop_about_dialog_set_comments(dialog,
			_("Debugger for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(dialog, _debugger_copyright);
	desktop_about_dialog_set_logo_icon_name(dialog,
			"applications-development");
	desktop_about_dialog_set_license(dialog, _debugger_license);
	desktop_about_dialog_set_name(dialog, _("Debugger"));
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog,
			"http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* debugger_on_close */
static void _debugger_on_close(gpointer data)
{
	Debugger * debugger = data;

	if(debugger_is_running(debugger))
		debugger_stop(debugger);
	gtk_main_quit();
}


/* debugger_on_closex */
static gboolean _debugger_on_closex(gpointer data)
{
	Debugger * debugger = data;

	_debugger_on_close(debugger);
	return TRUE;
}


/* debugger_on_continue */
static void _debugger_on_continue(gpointer data)
{
	Debugger * debugger = data;

	debugger_continue(debugger);
}


/* debugger_on_next */
static void _debugger_on_next(gpointer data)
{
	Debugger * debugger = data;

	debugger_next(debugger);
}


/* debugger_on_open */
static void _debugger_on_open(gpointer data)
{
	Debugger * debugger = data;

	debugger_open_dialog(debugger, NULL, NULL);
}


/* debugger_on_pause */
static void _debugger_on_pause(gpointer data)
{
	Debugger * debugger = data;

	debugger_pause(debugger);
}


/* debugger_on_run */
static void _debugger_on_run(gpointer data)
{
	Debugger * debugger = data;

	debugger_run(debugger, debugger->filename, NULL);
}


/* debugger_on_step */
static void _debugger_on_step(gpointer data)
{
	Debugger * debugger = data;

	debugger_step(debugger);
}


/* debugger_on_stop */
static void _debugger_on_stop(gpointer data)
{
	Debugger * debugger = data;

	debugger_stop(debugger);
}
