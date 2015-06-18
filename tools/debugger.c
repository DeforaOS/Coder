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



#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#include "debugger.h"
#define _(string) gettext(string)


/* Debugger */
/* private */
/* types */
struct _Debugger
{
	/* widgets */
	GtkWidget * window;
	/* toolbar */
	/* disassembly */
	GtkWidget * das_view;
	/* registers */
	GtkWidget * reg_view;
};


/* prototypes */
/* callbacks */
static void _debugger_on_close(gpointer data);
static gboolean _debugger_on_closex(gpointer data);


/* public */
/* functions */
/* debugger_new */
Debugger * debugger_new(void)
{
	Debugger * debugger;
	GtkWidget * vbox;
	GtkWidget * paned;
	GtkWidget * widget;
	GtkListStore * store;

	if((debugger = object_new(sizeof(*debugger))) == NULL)
		return NULL;
	debugger->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(debugger->window), _("Debugger"));
	g_signal_connect_swapped(debugger->window, "delete-event", G_CALLBACK(
				_debugger_on_closex), debugger);
	vbox = gtk_vbox_new(FALSE, 0);
	/* menubar */
	/* toolbar */
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
	gtk_container_add(GTK_CONTAINER(widget), debugger->reg_view);
	gtk_paned_add2(GTK_PANED(paned), widget);
	gtk_paned_set_position(GTK_PANED(paned), 380);
	gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(debugger->window), vbox);
	gtk_widget_show_all(debugger->window);
	return debugger;
}


/* debugger_delete */
void debugger_delete(Debugger * debugger)
{
	gtk_widget_destroy(debugger->window);
	object_delete(debugger);
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
