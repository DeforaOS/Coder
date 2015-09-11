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
#include "backend.h"
#include "debug.h"
#include "debugger.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

#ifndef PROGNAME
# define PROGNAME	"debugger"
#endif


/* Debugger */
/* private */
/* types */
enum { NP_DISASSEMBLY = 0, NP_CALL_GRAPH, NP_HEXDUMP };

typedef enum _RegisterValue
{
	RV_NAME = 0, RV_VALUE, RV_VALUE_DISPLAY, RV_SIZE
} RegisterValue;
#define RV_LAST RV_SIZE
#define RV_COUNT (RV_LAST + 1)

struct _Debugger
{
	/* backend */
	DebuggerBackendHelper bhelper;
	Plugin * bplugin;
	DebuggerBackendDefinition * bdefinition;
	DebuggerBackend * backend;

	/* debug */
	DebuggerDebugHelper dhelper;
	Plugin * dplugin;
	DebuggerDebugDefinition * ddefinition;
	DebuggerDebug * debug;

	/* child */
	String * filename;

	/* widgets */
	PangoFontDescription * bold;
	GtkWidget * window;
	GtkWidget * notebook;
	/* call graph */
	GtkWidget * dcg_view;
	/* disassembly */
	GtkWidget * das_view;
	/* hexdump */
	GtkWidget * dhx_view;
	/* registers */
	GtkListStore * reg_store;
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
static void _debugger_helper_set_register(Debugger * debugger,
		char const * name, uint64_t value);
/* backend */
static void _debugger_helper_backend_set_registers(Debugger * debugger,
		AsmArchRegister const * registers, size_t registers_cnt);

/* callbacks */
static void _debugger_on_about(gpointer data);
static void _debugger_on_close(gpointer data);
static gboolean _debugger_on_closex(gpointer data);
static void _debugger_on_continue(gpointer data);
static void _debugger_on_next(gpointer data);
static void _debugger_on_open(gpointer data);
static void _debugger_on_pause(gpointer data);
static void _debugger_on_properties(gpointer data);
static void _debugger_on_run(gpointer data);
static void _debugger_on_step(gpointer data);
static void _debugger_on_stop(gpointer data);
static void _debugger_on_view_call_graph(gpointer data);
static void _debugger_on_view_disassembly(gpointer data);
static void _debugger_on_view_hexdump(gpointer data);


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
	{ N_("_Properties"), G_CALLBACK(_debugger_on_properties),
		GTK_STOCK_PROPERTIES, GDK_MOD1_MASK, GDK_KEY_Return },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_debugger_on_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu const _debugger_menu_debug[] =
{
	{ N_("Run"), G_CALLBACK(_debugger_on_run), "system-run", 0,
		GDK_KEY_F10 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Continue"), G_CALLBACK(_debugger_on_continue),
		"media-playback-start", 0, GDK_KEY_F9 },
	{ N_("Pause"), G_CALLBACK(_debugger_on_pause),
		"media-playback-pause", 0, 0 },
	{ N_("Stop"), G_CALLBACK(_debugger_on_stop),
		"media-playback-stop", 0, GDK_KEY_F11 },
	{ N_("Step"), G_CALLBACK(_debugger_on_step),
		"media-seek-forward", 0, 0 },
	{ N_("Next"), G_CALLBACK(_debugger_on_next),
		"media-skip-forward", 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu const _debugger_menu_view[] =
{
	{ N_("Call graph"), G_CALLBACK(_debugger_on_view_call_graph), NULL, 0,
		0 },
	{ N_("Disassembly"), G_CALLBACK(_debugger_on_view_disassembly), NULL, 0,
		0 },
	{ N_("Hexdump"), G_CALLBACK(_debugger_on_view_hexdump), NULL, 0, 0 },
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
	{ N_("_Debug"), _debugger_menu_debug },
	{ N_("_View"), _debugger_menu_view },
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
#define DEBUGGER_TOOLBAR_PROPERTIES	10
static DesktopToolbar _debugger_toolbar[] =
{
	{ N_("Open"), G_CALLBACK(_debugger_on_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Run"), G_CALLBACK(_debugger_on_run), "system-run", 0, 0,
		NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Continue"), G_CALLBACK(_debugger_on_continue),
		"media-playback-start", 0, 0, NULL },
	{ N_("Pause"), G_CALLBACK(_debugger_on_pause),
		"media-playback-pause", 0, 0, NULL },
	{ N_("Stop"), G_CALLBACK(_debugger_on_stop),
		"media-playback-stop", 0, 0, NULL },
	{ N_("Step"), G_CALLBACK(_debugger_on_step),
		"media-seek-forward", 0, 0, NULL },
	{ N_("Next"), G_CALLBACK(_debugger_on_next),
		"media-skip-forward", 0, 0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Properties"), G_CALLBACK(_debugger_on_properties),
		GTK_STOCK_PROPERTIES, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* debugger_new */
Debugger * debugger_new(char const * backend, char const * debug)
{
	Debugger * debugger;
	GtkAccelGroup * accel;
	GtkWidget * vbox;
	GtkWidget * paned;
	GtkWidget * combo;
	GtkWidget * window;
	GtkWidget * widget;
	GtkTreeViewColumn * column;
	GtkCellRenderer * renderer;

	if((debugger = object_new(sizeof(*debugger))) == NULL)
		return NULL;
	/* backend */
	debugger->bhelper.debugger = debugger;
	debugger->bhelper.error = _debugger_helper_error;
	debugger->bhelper.set_registers
		= _debugger_helper_backend_set_registers;
	debugger->bplugin = plugin_new(LIBDIR, PACKAGE, "backend", backend);
	debugger->bdefinition = (debugger->bplugin != NULL)
		? plugin_lookup(debugger->bplugin, "backend") : NULL;
	debugger->backend = NULL;
	/* debug */
	debugger->dhelper.debugger = debugger;
	debugger->dhelper.error = _debugger_helper_error;
	debugger->dhelper.set_register = _debugger_helper_set_register;
	debugger->dplugin = plugin_new(LIBDIR, PACKAGE, "debug", debug);
	debugger->ddefinition = (debugger->dplugin != NULL)
		? plugin_lookup(debugger->dplugin, "debug") : NULL;
	debugger->debug = NULL;
	/* child */
	debugger->filename = NULL;
	/* widgets */
	debugger->window = NULL;
	/* check for errors */
	if(debugger->bdefinition == NULL
			|| (debugger->backend = debugger->bdefinition->init(
					&debugger->bhelper)) == NULL
			|| debugger->ddefinition == NULL)
	{
		debugger_delete(debugger);
		return NULL;
	}
	/* widgets */
	debugger->bold = pango_font_description_new();
	pango_font_description_set_weight(debugger->bold, PANGO_WEIGHT_BOLD);
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
	/* notebook */
	debugger->notebook = gtk_notebook_new();
	/* disassembly */
	debugger->das_view = gtk_text_view_new();
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(debugger->das_view),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(debugger->das_view), FALSE);
	gtk_notebook_append_page(GTK_NOTEBOOK(debugger->notebook),
			debugger->das_view, gtk_label_new(_("Disassembly")));
	/* call graph */
	debugger->dcg_view = gtk_drawing_area_new();
	gtk_notebook_append_page(GTK_NOTEBOOK(debugger->notebook),
			debugger->dcg_view, gtk_label_new(_("Call graph")));
	/* hexdump */
	debugger->dhx_view = gtk_text_view_new();
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(debugger->dhx_view),
			FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(debugger->dhx_view), FALSE);
	gtk_notebook_append_page(GTK_NOTEBOOK(debugger->notebook),
			debugger->dhx_view, gtk_label_new(_("Hexdump")));
	gtk_paned_add1(GTK_PANED(paned), debugger->notebook);
	/* registers */
#if GTK_CHECK_VERSION(3, 0, 0)
	widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	widget = gtk_vbox_new(FALSE, 0);
#endif
	combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), _("Registers"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	gtk_box_pack_start(GTK_BOX(widget), combo, FALSE, TRUE, 0);
	window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	debugger->reg_store = gtk_list_store_new(RV_COUNT,
			G_TYPE_STRING,	/* name */
			G_TYPE_UINT64,	/* value */
			G_TYPE_STRING,	/* value (string) */
			G_TYPE_UINT);	/* size */
	debugger->reg_view = gtk_tree_view_new_with_model(
			GTK_TREE_MODEL(debugger->reg_store));
	/* registers: name */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "family", "Monospace", NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Name"),
			renderer, "text", RV_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debugger->reg_view), column);
	/* registers: value */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "family", "Monospace", NULL);
	column = gtk_tree_view_column_new_with_attributes(_("Value"), renderer,
			"text", RV_VALUE_DISPLAY, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debugger->reg_view), column);
	gtk_container_add(GTK_CONTAINER(window), debugger->reg_view);
	gtk_box_pack_start(GTK_BOX(widget), window, TRUE, TRUE, 0);
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
	if(debugger->debug != NULL)
		debugger->ddefinition->destroy(debugger->debug);
	if(debugger->dplugin != NULL)
		plugin_delete(debugger->dplugin);
	if(debugger->backend != NULL)
		debugger->bdefinition->destroy(debugger->backend);
	if(debugger->bplugin != NULL)
		plugin_delete(debugger->bplugin);
	string_delete(debugger->filename);
	if(debugger->window != NULL)
		gtk_widget_destroy(debugger->window);
	pango_font_description_free(debugger->bold);
	object_delete(debugger);
}


/* accessors */
/* debugger_is_opened */
int debugger_is_opened(Debugger * debugger)
{
	return (debugger->filename != NULL) ? 1 : 0;
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
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(debugger->dhx_view));
	gtk_text_buffer_set_text(tbuf, "", 0);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(debugger->reg_view));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	/* FIXME really implement */
	string_delete(debugger->filename);
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
	if((debugger->filename = string_new(filename)) == NULL)
		return -1;
	if(debugger->bdefinition->open(debugger->backend, arch, format,
				filename) != 0)
	{
		string_delete(debugger->filename);
		debugger->filename = NULL;
		return -debugger_error(debugger, error_get(), 1);
	}
	if((s = string_new_append(_("Debugger"), " - ", filename, NULL))
			!= NULL)
		gtk_window_set_title(GTK_WINDOW(debugger->window), s);
	string_delete(s);
	_debugger_set_sensitive_toolbar(debugger, TRUE, FALSE);
	return 0;
}


/* debugger_open_dialog */
int debugger_open_dialog(Debugger * debugger, char const * arch,
		char const * format)
{
	int ret;
	char * filename;

	if(_debugger_confirm_close(debugger) == FALSE)
		return -1;
	if((filename = debugger->bdefinition->open_dialog(debugger->backend,
					debugger->window, arch, format))
			== NULL)
		return -1;
	if(debugger_close(debugger) != 0)
	{
		free(filename);
		return -debugger_error(debugger, error_get(), 1);
	}
	ret = debugger_open(debugger, arch, format, filename);
	free(filename);
	return ret;
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


/* debugger_properties */
static GtkWidget * _properties_label(Debugger * debugger, GtkSizeGroup * group,
		char const * label, char const * value);

void debugger_properties(Debugger * debugger)
{
	const unsigned int flags = GTK_DIALOG_MODAL
		| GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkSizeGroup * group;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * widget;
	String * s;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(debugger_is_opened(debugger) == FALSE)
		return;
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	dialog = gtk_message_dialog_new(GTK_WINDOW(debugger->window), flags,
			GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Properties"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"");
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog),
			gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,
				GTK_ICON_SIZE_DIALOG));
#endif
	if((s = string_new_format(_("Properties of %s"), debugger->filename))
			!= NULL)
		gtk_window_set_title(GTK_WINDOW(dialog), s);
	string_delete(s);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	/* FIXME really implement */
	/* architecture */
	widget = _properties_label(debugger, group, _("Architecture: "),
			debugger->bdefinition->arch_get_name(debugger->backend));
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* format */
	widget = _properties_label(debugger, group, _("Format: "),
			debugger->bdefinition->format_get_name(debugger->backend));
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static GtkWidget * _properties_label(Debugger * debugger, GtkSizeGroup * group,
		char const * label, char const * value)
{
	GtkWidget * hbox;
	GtkWidget * widget;

#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_modify_font(widget, debugger->bold);
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_label_new((value != NULL) ? value : "");
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
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
	if(_debugger_confirm_reset(debugger) == FALSE)
		return -1;
	if(debugger_stop(debugger) != 0)
		return -1;
	if((debugger->debug = debugger->ddefinition->init(&debugger->dhelper))
			== NULL
			|| debugger->ddefinition->start(debugger->debug, ap)
			!= 0)
	{
		debugger_stop(debugger);
		return -1;
	}
	_debugger_set_sensitive_toolbar(debugger, TRUE, TRUE);
	return 0;
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
	GtkWidget * widget;
	size_t i;

	widget = GTK_WIDGET(_debugger_toolbar[DEBUGGER_TOOLBAR_RUN].widget);
	gtk_widget_set_sensitive(widget, run);
	for(i = 0; i < sizeof(widgets) / sizeof(*widgets); i++)
		gtk_widget_set_sensitive(GTK_WIDGET(
					_debugger_toolbar[widgets[i]].widget),
				debug);
	widget = GTK_WIDGET(_debugger_toolbar[DEBUGGER_TOOLBAR_PROPERTIES]
			.widget);
	gtk_widget_set_sensitive(widget, debugger_is_opened(debugger));
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


/* debugger_helper_set_register */
static void _debugger_helper_set_register(Debugger * debugger,
		char const * name, uint64_t value)
{
	GtkTreeModel * model = GTK_TREE_MODEL(debugger->reg_store);
	GtkTreeIter iter;
	gboolean valid;
	gchar * p;
	unsigned int size;
	int res;
	char buf[33];

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, RV_NAME, &p, RV_SIZE, &size,
				-1);
		res = strcasecmp(p, name);
		g_free(p);
		if(res == 0)
			break;
	}
	if(valid != TRUE)
		return;
	if(size <= 16)
		snprintf(buf, sizeof(buf), "%04lx", value);
	else if(size <= 20)
		snprintf(buf, sizeof(buf), "%05lx", value);
	else if(size <= 32)
		snprintf(buf, sizeof(buf), "%08lx", value);
	else if(size <= 64)
		snprintf(buf, sizeof(buf), "%016llx", value);
	else
		snprintf(buf, sizeof(buf), "%032llx", value);
	gtk_list_store_set(debugger->reg_store, &iter, RV_VALUE, value,
			RV_VALUE_DISPLAY, buf, -1);
}


/* helpers: backend */
/* debugger_helper_backend_set_registers */
static void _debugger_helper_backend_set_registers(Debugger * debugger,
		AsmArchRegister const * registers, size_t registers_cnt)
{
	GtkTreeModel * model;
	size_t i;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(debugger->reg_view));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	for(i = 0; i < registers_cnt; i++)
	{
		if(registers[i].flags & ARF_ALIAS)
			continue;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				RV_NAME, registers[i].name,
				RV_SIZE, registers[i].size, -1);
	}
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


/* debugger_on_properties */
static void _debugger_on_properties(gpointer data)
{
	Debugger * debugger = data;

	debugger_properties(debugger);
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


/* debugger_on_view_call_graph */
static void _debugger_on_view_call_graph(gpointer data)
{
	Debugger * debugger = data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(debugger->notebook),
			NP_CALL_GRAPH);
}


/* debugger_on_view_disassembly */
static void _debugger_on_view_disassembly(gpointer data)
{
	Debugger * debugger = data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(debugger->notebook),
			NP_DISASSEMBLY);
}


/* debugger_on_view_hexdump */
static void _debugger_on_view_hexdump(gpointer data)
{
	Debugger * debugger = data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(debugger->notebook),
			NP_HEXDUMP);
}
