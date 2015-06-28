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
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "debugger.h"
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


/* platform */
#ifdef __NetBSD__
typedef int ptrace_data_t;
#endif


/* Debugger */
/* private */
/* types */
struct _Debugger
{
	/* child */
	char * filename;
	GPid pid;
	guint source;

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
static gboolean _debugger_confirm(Debugger * debugger, char const * message);
static gboolean _debugger_confirm_close(Debugger * debugger);
static gboolean _debugger_confirm_reset(Debugger * debugger);

static int _debugger_error(Debugger * debugger, char const * message, int ret);

/* callbacks */
static void _debugger_on_close(gpointer data);
static gboolean _debugger_on_closex(gpointer data);

static void _debugger_on_continue(gpointer data);
static void _debugger_on_child_watch(GPid pid, gint status, gpointer data);
static void _debugger_on_open(gpointer data);
static void _debugger_on_pause(gpointer data);
static void _debugger_on_run(gpointer data);
static void _debugger_on_stop(gpointer data);


/* variables */
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
	{ NULL, NULL, NULL, 0, 0, NULL }
};


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
	/* child */
	debugger->filename = NULL;
	debugger->pid = -1;
	debugger->source = 0;
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
	/* toolbar */
	widget = desktop_toolbar_create(_debugger_toolbar, debugger, accel);
	g_object_unref(accel);
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
	if(debugger->source != 0)
		g_source_remove(debugger->source);
	if(debugger->pid > 0)
		g_spawn_close_pid(debugger->pid);
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
	return (debugger->pid > 0) ? 1 : 0;
}


/* useful */
/* debugger_close */
int debugger_close(Debugger * debugger)
{
	if(debugger_is_opened(debugger) == FALSE)
		return 0;
	/* FIXME really implement */
	free(debugger->filename);
	debugger->filename = NULL;
	return 0;
}


/* debugger_continue */
int debugger_continue(Debugger * debugger)
{
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	errno = 0;
	if(ptrace(PT_CONTINUE, debugger->pid, (caddr_t)1, (ptrace_data_t)0)
			== -1 && errno != 0)
	{
		error_set_code(-errno, "%s: %s", "ptrace", strerror(errno));
		return -_debugger_error(debugger, error_get(), 1);
	}
	return 0;
}


/* debugger_open */
int debugger_open(Debugger * debugger, char const * arch, char const * format,
		char const * filename)
{
	if(_debugger_confirm_close(debugger) == FALSE)
		return -1;
	if(filename == NULL)
		return debugger_open_dialog(debugger, arch, format);
	if(debugger_close(debugger) != 0)
		return -_debugger_error(debugger, error_get(), 1);
	/* FIXME really implement */
	if((debugger->filename = strdup(filename)) == NULL)
		return -1;
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
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	/* FIXME implement */
	return -1;
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
static int _runv_parent(Debugger * debugger);
/* callbacks */
static void _runv_on_child_setup(gpointer data);

int debugger_runv(Debugger * debugger, va_list ap)
{
	char * argv[3] = { NULL, NULL, NULL };
	const unsigned int flags = G_SPAWN_DO_NOT_REAP_CHILD
		| G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(_debugger_confirm_reset(debugger) == FALSE)
		return -1;
	if(debugger_stop(debugger) != 0)
		return -1;
	argv[0] = debugger->filename;
	argv[1] = debugger->filename;
	if(g_spawn_async(NULL, argv, NULL, flags, _runv_on_child_setup,
				debugger, &debugger->pid, &error) == FALSE)
	{
		error_set_code(-errno, "%s", error->message);
		g_error_free(error);
		return -_debugger_error(debugger, error_get(), 1);
	}
	return _runv_parent(debugger);
}

static int _runv_parent(Debugger * debugger)
{
	debugger->source = g_child_watch_add(debugger->pid,
			_debugger_on_child_watch, debugger);
	return 0;
}

/* callbacks */
static void _runv_on_child_setup(gpointer data)
{
	Debugger * debugger = data;

	errno = 0;
	if(ptrace(PT_TRACE_ME, -1, NULL, (ptrace_data_t)0) == -1
			&& errno != 0)
	{
		_debugger_error(NULL, strerror(errno), 1);
		_exit(125);
	}
}


/* debugger_stop */
int debugger_stop(Debugger * debugger)
{
	if(debugger_is_running(debugger) == FALSE)
		return 0;
	/* FIXME implement */
	return -1;
}


/* private */
/* functions */
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


/* debugger_error */
static int _error_text(char const * message, int ret);

static int _debugger_error(Debugger * debugger, char const * message, int ret)
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


/* callbacks */
/* debugger_on_close */
static void _debugger_on_close(gpointer data)
{
	Debugger * debugger = data;

	/* FIXME really implement */
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


/* debugger_on_child_watch */
static void _debugger_on_child_watch(GPid pid, gint status, gpointer data)
{
	Debugger * debugger = data;
#ifndef G_OS_UNIX
	GError * error = NULL;
#endif

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %d)\n", __func__, pid, status);
#endif
	if(debugger->pid != pid)
		return;
#ifdef G_OS_UNIX
	if(WIFSTOPPED(status))
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() stopped\n", __func__);
#endif
		debugger_continue(debugger);
	}
	else if(WIFSIGNALED(status))
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() signal %d\n", __func__,
				WTERMSIG(status));
#endif
		g_spawn_close_pid(debugger->pid);
		debugger->source = 0;
	}
	else if(WIFEXITED(status))
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() error %d\n", __func__,
				WEXITSTATUS(status));
#endif
		g_spawn_close_pid(debugger->pid);
		debugger->source = 0;
	}
#else
	if(g_spawn_check_exit_status(status, &error) == FALSE)
	{
		error_set_code(WEXITSTATUS(status), "%s", error->message);
		g_error_free(error);
		_debugger_error(debugger, error_get(), WEXITSTATUS(status));
		g_spawn_close_pid(debugger->pid);
		debugger->source = 0;
	}
#endif
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

	debugger_run(debugger);
}


/* debugger_on_stop */
static void _debugger_on_stop(gpointer data)
{
	Debugger * debugger = data;

	debugger_stop(debugger);
}
