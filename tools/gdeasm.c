/* $Id$ */
static char const _gdeasm_copyright[] =
"Copyright © 2011-2020 Pierre Pronchery <khorben@defora.org>";
static char const _gdeasm_license[] =
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
/* TODO:
 * - implement the help contents (and assembly reference)
 * - (optionally) show a column with the hexadecimal disassembly
 * - allow new files to be created
 * - list registers available (combo box, on the fly)
 * - auto-complete opcodes
 * - display additional information as well (executable type...)
 * - detect mistakes (pre-assemble) the file
 * - assemble the current file again
 * - add a preferences structure
 * - complete the function list
 * - add a window automatically displaying integers in base 2, 8, 10 and 16 */



#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Devel/Asm.h>
#include <Desktop.h>
#include "gdeasm.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)


/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef LIBDIR
# define LIBDIR		PREFIX "/lib"
#endif


/* GDeasm */
/* private */
/* types */
typedef enum _GDeasmAsmColumn
{
	GAC_ADDRESS = 0, GAC_NAME, GAC_OPERAND1, GAC_OPERAND2, GAC_OPERAND3,
	GAC_OPERAND4, GAC_OPERAND5, GAC_COMMENT, GAC_OFFSET, GAC_BASE
} GDeasmAsmColumn;
#define GAC_LAST GAC_BASE
#define GAC_COUNT (GAC_LAST + 1)

typedef enum _GDeasmFuncColumn
{
	GFC_NAME = 0, GFC_OFFSET_DISPLAY, GFC_OFFSET
} GDeasmFuncColumn;
#define GFC_LAST GFC_OFFSET
#define GFC_COUNT (GFC_LAST + 1)

typedef enum _GDeasmStrColumn
{
	GSC_STRING = 0
} GDeasmStrColumn;
#define GSC_LAST GSC_STRING
#define GSC_COUNT (GSC_LAST + 1)

struct _GDeasm
{
	gboolean modified;

	/* widgets */
	GtkWidget * window;
	GtkListStore * func_store;
	GtkListStore * str_store;
	GtkTreeStore * asm_store;
	GtkListStore * ins_store;
	GtkWidget * asm_view;
	GtkWidget * statusbar;
};


/* prototypes */
/* accessors */
static void _gdeasm_set_status(GDeasm * gdeasm, char const * status);

/* useful */
static int _gdeasm_confirm(GDeasm * gdeasm, char const * message, ...);
static int _gdeasm_error(GDeasm * gdeasm, char const * message, int ret);

/* callbacks */
static void _gdeasm_on_about(gpointer data);
static void _gdeasm_on_close(gpointer data);
static gboolean _gdeasm_on_closex(gpointer data);
static void _gdeasm_on_comment_edited(GtkCellRendererText * renderer,
		gchar * arg1, gchar * arg2, gpointer data);
static void _gdeasm_on_function_activated(GtkTreeView * view,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data);
static void _gdeasm_on_load_comments(gpointer data);
static void _gdeasm_on_open(gpointer data);
static void _gdeasm_on_save_comments(gpointer data);


/* constants */
static char const * _gdeasm_authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};

static DesktopMenu const _gdeasm_menu_file[] =
{
	{ N_("_Open..."), G_CALLBACK(_gdeasm_on_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Load comments..."), G_CALLBACK(_gdeasm_on_load_comments), NULL,
		GDK_CONTROL_MASK, GDK_KEY_L },
	{ N_("_Save comments as..."), G_CALLBACK(_gdeasm_on_save_comments),
		NULL, GDK_CONTROL_MASK, GDK_KEY_S },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_gdeasm_on_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenu const _gdeasm_menu_help[] =
{
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(_gdeasm_on_about), GTK_STOCK_ABOUT, 0, 0 },
#else
	{ N_("_About"), G_CALLBACK(_gdeasm_on_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static DesktopMenubar const _gdeasm_menubar[] =
{
	{ N_("_File"), _gdeasm_menu_file },
	{ N_("_Help"), _gdeasm_menu_help },
	{ NULL, NULL },
};


/* variables */
/* toolbar */
static DesktopToolbar _gdeasm_toolbar[] =
{
	{ N_("Open file"), G_CALLBACK(_gdeasm_on_open), GTK_STOCK_OPEN, 0, 0,
		NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Load comments"), G_CALLBACK(_gdeasm_on_load_comments),
		GTK_STOCK_OPEN, 0, 0, NULL },
	{ N_("Save comments"), G_CALLBACK(_gdeasm_on_save_comments),
		GTK_STOCK_SAVE_AS, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* gdeasm_new */
GDeasm * gdeasm_new(void)
{
	GDeasm * gdeasm;
	GtkAccelGroup * accel;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * toolbar;
	GtkWidget * hpaned;
	GtkWidget * vpaned;
	GtkWidget * scrolled;
	GtkWidget * treeview;
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;
	char const * headers1[GFC_COUNT - 1] = { N_("Functions"),
		N_("Offset") };
	char const * headers2[GSC_COUNT] = { N_("Strings") };
	char const * headers3[] = { N_("Address"), N_("Instruction"),
		N_("Operand"), N_("Operand"), N_("Operand"), N_("Operand"),
		N_("Operand"), N_("Comment") };
	size_t i;

	if((gdeasm = malloc(sizeof(*gdeasm))) == NULL)
		return NULL;
	gdeasm->modified = FALSE;
	/* widgets */
	gdeasm->func_store = gtk_list_store_new(GFC_COUNT, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_INT);
	gdeasm->str_store = gtk_list_store_new(GSC_COUNT, G_TYPE_STRING);
	gdeasm->asm_store = gtk_tree_store_new(GAC_COUNT, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
	gdeasm->ins_store = gtk_list_store_new(1, G_TYPE_STRING);
	accel = gtk_accel_group_new();
	gdeasm->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(gdeasm->window), accel);
	g_object_unref(accel);
	gtk_window_set_default_size(GTK_WINDOW(gdeasm->window), 640, 480);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(gdeasm->window),
			"applications-development");
#endif
	gtk_window_set_title(GTK_WINDOW(gdeasm->window), _("GDeasm"));
	g_signal_connect_swapped(gdeasm->window, "delete-event", G_CALLBACK(
				_gdeasm_on_closex), gdeasm);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	/* menubar */
	widget = desktop_menubar_create(_gdeasm_menubar, gdeasm, accel);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	toolbar = desktop_toolbar_create(_gdeasm_toolbar, gdeasm, accel);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	/* view */
	hpaned = gtk_hpaned_new();
	vpaned = gtk_vpaned_new();
	/* functions */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				gdeasm->func_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), FALSE);
	g_signal_connect(treeview, "row-activated", G_CALLBACK(
				_gdeasm_on_function_activated), gdeasm);
	for(i = 0; i < sizeof(headers1) / sizeof(*headers1); i++)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				_(headers1[i]), renderer, "text", i, NULL);
		if(i == 1)
			g_object_set(renderer, "family", "Monospace", NULL);
#if GTK_CHECK_VERSION(2, 4, 0)
		gtk_tree_view_column_set_expand(column, TRUE);
#endif
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, i);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	}
	gtk_container_add(GTK_CONTAINER(scrolled), treeview);
	gtk_paned_pack1(GTK_PANED(vpaned), scrolled, FALSE, TRUE);
	/* strings */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				gdeasm->str_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), FALSE);
	for(i = 0; i < sizeof(headers2) / sizeof(*headers2); i++)
	{
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				_(headers2[i]), renderer, "text", i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	}
	gtk_container_add(GTK_CONTAINER(scrolled), treeview);
	gtk_paned_pack2(GTK_PANED(vpaned), scrolled, FALSE, FALSE);
	gtk_paned_add1(GTK_PANED(hpaned), vpaned);
	/* assembly */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				gdeasm->asm_store));
	gdeasm->asm_view = treeview;
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
	for(i = 0; i < sizeof(headers3) / sizeof(*headers3); i++)
	{
		if(i == 1)
		{
			renderer = gtk_cell_renderer_combo_new();
			g_object_set(renderer, "editable", TRUE,
					"model", gdeasm->ins_store,
					"text-column", 0, NULL);
		}
		else
			renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				_(headers3[i]), renderer, "text", i, NULL);
		if(i == 0)
			g_object_set(renderer, "family", "Monospace", NULL);
		else if(i == 7) /* the last column is editable */
		{
			g_object_set(renderer, "editable", TRUE, "style",
					PANGO_STYLE_ITALIC, NULL);
			g_signal_connect(renderer, "edited", G_CALLBACK(
						_gdeasm_on_comment_edited),
					gdeasm);
		}
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	}
	gtk_container_add(GTK_CONTAINER(scrolled), treeview);
	gtk_paned_add2(GTK_PANED(hpaned), scrolled);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
	/* statusbar */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gdeasm->statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(hbox), gdeasm->statusbar, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(gdeasm->window), vbox);
	gtk_widget_show_all(gdeasm->window);
	return gdeasm;
}


/* gdeasm_delete */
void gdeasm_delete(GDeasm * gdeasm)
{
	g_object_unref(gdeasm->ins_store);
	g_object_unref(gdeasm->asm_store);
	g_object_unref(gdeasm->str_store);
	g_object_unref(gdeasm->func_store);
	free(gdeasm);
}


/* useful */
/* gdeasm_load_comments */
int gdeasm_load_comments(GDeasm * gdeasm, char const * filename)
{
	Config * config;
	GtkTreeModel * model = GTK_TREE_MODEL(gdeasm->asm_store);
	GtkTreeIter parent;
	GtkTreeIter iter;
	gboolean valid;
	gint offset;
	char const * p;
	char buf[12];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, filename);
#endif
	if((config = config_new()) == NULL)
		return -_gdeasm_error(gdeasm, error_get(NULL), 1);
	if(config_load(config, filename) != 0)
	{
		config_delete(config);
		return -_gdeasm_error(gdeasm, error_get(NULL), 1);
	}
	for(valid = gtk_tree_model_get_iter_first(model, &parent); valid;
			valid = gtk_tree_model_iter_next(model, &parent))
		for(valid = gtk_tree_model_iter_children(model, &iter, &parent);
				valid;
				valid = gtk_tree_model_iter_next(model, &iter))
		{
			gtk_tree_model_get(model, &iter, GAC_OFFSET, &offset,
					-1);
			if(offset < 0)
				continue;
			snprintf(buf, sizeof(buf), "0x%x", offset);
			if((p = config_get(config, "comments", buf)) == NULL)
				continue;
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %d \"%s\"\n", __func__,
					offset, p);
#endif
			gtk_tree_store_set(gdeasm->asm_store, &iter,
					GAC_COMMENT, p, -1);
		}
	config_delete(config);
	return -1;
}


/* gdeasm_load_comments_dialog */
int gdeasm_load_comments_dialog(GDeasm * gdeasm)
{
	int ret = -1;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	char * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Load comments..."),
			GTK_WINDOW(gdeasm->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("GDeasm files"));
        gtk_file_filter_add_pattern(filter, "*.gdeasm");
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
	if(filename != NULL)
		ret = gdeasm_load_comments(gdeasm, filename);
	g_free(filename);
	return ret;
}


/* gdeasm_open */
static int _open_code(GDeasm * gdeasm, AsmCode * af);
static int _open_code_section(GDeasm * gdeasm, AsmCode * code,
		AsmSection * section);
static void _open_functions(GDeasm * gdeasm, AsmFunction * af, size_t af_cnt);
static void _open_instruction(GDeasm * gdeasm, GtkTreeIter * parent,
		AsmArchInstructionCall * call);
static void _open_parse_dregister(char * buf, size_t size, AsmArchOperand * ao);
static void _open_parse_dregister2(char * buf, size_t size,
		AsmArchOperand * ao);
static void _open_parse_immediate(char * buf, size_t size, AsmArchOperand * ao);
static void _open_strings(GDeasm * gdeasm, AsmString * as, size_t as_cnt);

int gdeasm_open(GDeasm * gdeasm, char const * arch, char const * format,
		char const * filename)
{
	int ret = -1;
	int res;
	Asm * a;
	AsmCode * code;
	AsmFunction * af;
	size_t af_cnt;
	AsmString * as;
	size_t as_cnt;

	if(filename == NULL)
		return gdeasm_open_dialog(gdeasm);
	if(gdeasm->modified != FALSE)
	{
		res = _gdeasm_confirm(gdeasm, _("There are unsaved comments.\n"
					"Discard or save them?"),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
#if GTK_CHECK_VERSION(2, 12, 0)
				GTK_STOCK_DISCARD, GTK_RESPONSE_REJECT,
#else
				_("Discard"), GTK_RESPONSE_REJECT,
#endif
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
		if(res == GTK_RESPONSE_ACCEPT)
		{
			if(gdeasm_save_comments_dialog(gdeasm) != 0)
				return 0;
		}
		else if(res != GTK_RESPONSE_REJECT)
			return 0;
	}
	if((a = asm_new(arch, format)) == NULL)
		return -_gdeasm_error(gdeasm, error_get(NULL), 1);
	_gdeasm_set_status(gdeasm, "");
	if((code = asm_open_deassemble(a, filename, TRUE)) != NULL)
	{
		gtk_list_store_clear(gdeasm->func_store);
		gtk_list_store_clear(gdeasm->str_store);
		gtk_tree_store_clear(gdeasm->asm_store);
		gdeasm->modified = FALSE;
		ret = _open_code(gdeasm, code);
		asmcode_get_functions(code, &af, &af_cnt);
		_open_functions(gdeasm, af, af_cnt);
		asmcode_get_strings(code, &as, &as_cnt);
		_open_strings(gdeasm, as, as_cnt);
		asm_close(a);
	}
	asm_delete(a);
	if(ret != 0)
		_gdeasm_error(gdeasm, error_get(NULL), 1);
	return ret;
}

static int _open_code(GDeasm * gdeasm, AsmCode * code)
{
	int ret = 0;
	AsmSection * sections;
	size_t sections_cnt;
	size_t i;
	char const * arch;
	char const * format;
	gchar * buf;
	AsmArchInstruction const * ai;
	GtkTreeIter iter;
	char const * p;

	asmcode_get_sections(code, &sections, &sections_cnt);
	for(i = 0; i < sections_cnt; i++)
		if((ret = _open_code_section(gdeasm, code, &sections[i])) != 0)
			break;
	gtk_list_store_clear(gdeasm->ins_store);
	if(ret == 0)
	{
		/* update the status */
		if((arch = asmcode_get_arch_description(code)) == NULL)
			arch = asmcode_get_arch(code);
		if((format = asmcode_get_format_description(code)) == NULL)
			format = asmcode_get_format(code);
		buf = g_strdup_printf("%s%s | %s%s", _("Architecture: "),
				arch, _("Format: "), format);
		_gdeasm_set_status(gdeasm, buf);
		g_free(buf);
		p = NULL;
		/* update the instructions list */
		for(ai = asmcode_get_arch_instructions(code);
				ai != NULL && ai->name != NULL; ai++)
		{
			if(p != NULL && strcmp(p, ai->name) == 0)
				continue;
#if GTK_CHECK_VERSION(2, 6, 0)
			gtk_list_store_insert_with_values(gdeasm->ins_store,
					&iter, -1,
#else
			gtk_list_store_append(gdeasm->ins_store, &iter);
			gtk_list_store_set(gdeasm->ins_store, &iter,
#endif
					0, ai->name, -1);
			p = ai->name;
		}
	}
	return ret;
}

static int _open_code_section(GDeasm * gdeasm, AsmCode * code,
		AsmSection * section)
{
	GtkTreeIter iter;
	AsmArchInstructionCall * calls = NULL;
	size_t calls_cnt = 0;
	size_t i;

#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_tree_store_insert_with_values(gdeasm->asm_store, &iter, NULL, -1,
#else
	gtk_tree_store_append(gdeasm->asm_store, &iter, NULL);
	gtk_tree_store_set(gdeasm->asm_store, &iter,
#endif
			1, section->name, -1);
	if(asmcode_decode_section(code, section, &calls, &calls_cnt) != 0)
		return -1;
	for(i = 0; i < calls_cnt; i++)
		_open_instruction(gdeasm, &iter, &calls[i]);
	free(calls);
	return 0;
}

static void _open_functions(GDeasm * gdeasm, AsmFunction * af, size_t af_cnt)
{
	size_t i;
	GtkTreeIter iter;
	char buf[16];

	for(i = 0; i < af_cnt; i++)
	{
		if(af[i].offset >= 0)
			snprintf(buf, sizeof(buf), "%08lx", af[i].offset);
		else
			buf[0] = '\0';
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_list_store_insert_with_values(gdeasm->func_store, &iter, -1,
#else
		gtk_list_store_append(gdeasm->func_store, &iter);
		gtk_list_store_set(gdeasm->func_store, &iter,
#endif
				GFC_NAME, af[i].name, GFC_OFFSET_DISPLAY, buf,
				GFC_OFFSET, af[i].offset, -1);
	}
}

static void _open_instruction(GDeasm * gdeasm, GtkTreeIter * parent,
		AsmArchInstructionCall * call)
{
	GtkTreeIter iter;
	char buf[32];
	size_t i;
	AsmArchOperand * ao;
	char const * name;

	snprintf(buf, sizeof(buf), "%08lx", call->base);
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_tree_store_insert_with_values(gdeasm->asm_store, &iter, parent, -1,
#else
	gtk_tree_store_append(gdeasm->asm_store, &iter, parent);
	gtk_tree_store_set(gdeasm->asm_store, &iter,
#endif
			GAC_ADDRESS, buf, GAC_NAME, call->name,
			GAC_OFFSET, call->offset, GAC_BASE, call->base, -1);
	for(i = 0; i < call->operands_cnt; i++)
	{
		ao = &call->operands[i];
		switch(AO_GET_TYPE(ao->definition))
		{
			case AOT_DREGISTER:
				_open_parse_dregister(buf, sizeof(buf), ao);
				break;
			case AOT_DREGISTER2:
				_open_parse_dregister2(buf, sizeof(buf), ao);
				break;
			case AOT_IMMEDIATE:
				_open_parse_immediate(buf, sizeof(buf), ao);
				if(AO_GET_VALUE(ao->definition)
						== AOI_REFERS_STRING
						|| AO_GET_VALUE(ao->definition)
						== AOI_REFERS_FUNCTION)
					gtk_tree_store_set(gdeasm->asm_store,
							&iter, GAC_COMMENT,
							ao->value.immediate.name,
							-1);
				break;
			case AOT_REGISTER:
				name = call->operands[i].value._register.name;
				snprintf(buf, sizeof(buf), "%%%s", name);
				break;
			default:
				buf[0] = '\0';
				break;
		}
		gtk_tree_store_set(gdeasm->asm_store, &iter, GAC_OPERAND1 + i,
				buf, -1);
	}
}

static void _open_parse_dregister(char * buf, size_t size, AsmArchOperand * ao)
{
	char const * name;

	name = ao->value.dregister.name;
	if(ao->value.dregister.offset == 0)
		snprintf(buf, size, "[%%%s]", name);
	else
		snprintf(buf, size, "[%%%s + $0x%lx]", name,
				(unsigned long)ao->value.dregister.offset);
}

static void _open_parse_dregister2(char * buf, size_t size, AsmArchOperand * ao)
{
	snprintf(buf, size, "[%%%s + %%%s]", ao->value.dregister2.name,
			ao->value.dregister2.name2);
}

static void _open_parse_immediate(char * buf, size_t size, AsmArchOperand * ao)
{
	snprintf(buf, size, "%s$0x%lx", ao->value.immediate.negative
			? "-" : "", (unsigned long)ao->value.immediate.value);
}

static void _open_strings(GDeasm * gdeasm, AsmString * as, size_t as_cnt)
{
	size_t i;
	GtkTreeIter iter;

	for(i = 0; i < as_cnt; i++)
	{
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_list_store_insert_with_values(gdeasm->str_store, &iter, -1,
#else
		gtk_list_store_append(gdeasm->str_store, &iter);
		gtk_list_store_set(gdeasm->str_store, &iter,
#endif
				GSC_STRING, as[i].name, -1);
	}
}


/* gdeasm_open_dialog */
static void _open_dialog_type(GtkWidget * combobox, char const * type);

int gdeasm_open_dialog(GDeasm * gdeasm)
{
	int ret = 0;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * awidget;
	GtkWidget * fwidget;
	GtkWidget * widget;
	GtkFileFilter * filter;
	char * arch = NULL;
	char * format = NULL;
	char * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open file..."),
			GTK_WINDOW(gdeasm->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	/* arch */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#if GTK_CHECK_VERSION(2, 24, 0)
	awidget = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(awidget),
			_("Auto-detect"));
#else
	awidget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(awidget), _("Auto-detect"));
#endif
	_open_dialog_type(awidget, "arch");
	gtk_combo_box_set_active(GTK_COMBO_BOX(awidget), 0);
	gtk_box_pack_end(GTK_BOX(hbox), awidget, FALSE, TRUE, 0);
	widget = gtk_label_new(_("Architecture:"));
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* format */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#if GTK_CHECK_VERSION(2, 24, 0)
	fwidget = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fwidget),
			_("Auto-detect"));
#else
	fwidget = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(fwidget), _("Auto-detect"));
#endif
	_open_dialog_type(fwidget, "format");
	gtk_combo_box_set_active(GTK_COMBO_BOX(fwidget), 0);
	gtk_box_pack_end(GTK_BOX(hbox), fwidget, FALSE, TRUE, 0);
	widget = gtk_label_new(_("File format:"));
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	/* core files */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Core files"));
	gtk_file_filter_add_mime_type(filter, "application/x-core");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	/* executable files */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Executable files"));
	gtk_file_filter_add_mime_type(filter, "application/x-executable");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	/* java classes */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Java classes"));
	gtk_file_filter_add_mime_type(filter, "application/x-java");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	/* objects */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Objects"));
	gtk_file_filter_add_mime_type(filter, "application/x-object");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	/* shared objects */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Shared objects"));
	gtk_file_filter_add_mime_type(filter, "application/x-sharedlib");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	/* all files */
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
#if GTK_CHECK_VERSION(2, 24, 0)
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(awidget)) == 0)
			arch = NULL;
		else
			arch = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(awidget));
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(fwidget)) == 0)
			format = NULL;
		else
			format = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(fwidget));
#else
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(awidget)) == 0)
			arch = NULL;
		else
			arch = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
						awidget));
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(fwidget)) == 0)
			format = NULL;
		else
			format = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
						fwidget));
#endif
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	}
	gtk_widget_destroy(dialog);
	if(filename != NULL)
		ret = gdeasm_open(gdeasm, arch, format, filename);
	g_free(arch);
	g_free(format);
	g_free(filename);
	return ret;
}

static void _open_dialog_type(GtkWidget * combobox, char const * type)
{
	char * path;
	DIR * dir;
	struct dirent * de;
#if defined(__APPLE__)
	char const ext[] = ".dylib";
#elif defined(__WIN32__)
	char const ext[] = ".dll";
#else
	char const ext[] = ".so";
#endif
	size_t len;

	if((path = g_build_filename(LIBDIR, "Asm", type, NULL)) == NULL)
		return;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%s) \"%s\"\n", __func__, type, path);
#endif
	dir = opendir(path);
	g_free(path);
	if(dir == NULL)
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
#if GTK_CHECK_VERSION(2, 24, 0)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),
				de->d_name);
#else
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), de->d_name);
#endif
	}
	closedir(dir);
}


/* gdeasm_save_comments */
struct _save_comments_foreach_args
{
	int ret;
	GDeasm * gdeasm;
	Config * config;
};
static gboolean _save_comments_foreach(GtkTreeModel * model, GtkTreePath * path,
		GtkTreeIter * iter, gpointer data);

int gdeasm_save_comments(GDeasm * gdeasm, char const * filename)
{
	struct _save_comments_foreach_args args;

	args.ret = 0;
	args.gdeasm = gdeasm;
	if((args.config = config_new()) == NULL)
		return -_gdeasm_error(gdeasm, error_get(NULL), 1);
	gtk_tree_model_foreach(GTK_TREE_MODEL(gdeasm->asm_store),
			_save_comments_foreach, &args);
	if(args.ret == 0)
	{
		if(config_save(args.config, filename) == 0)
			gdeasm->modified = FALSE;
		else
			args.ret = -_gdeasm_error(gdeasm, error_get(NULL), 1);
	}
	config_delete(args.config);
	return args.ret;
}

static gboolean _save_comments_foreach(GtkTreeModel * model, GtkTreePath * path,
		GtkTreeIter * iter, gpointer data)
{
	struct _save_comments_foreach_args * args = data;
	int offset;
	gchar * p;
	char buf[16];
	(void) path;

	gtk_tree_model_get(model, iter, GAC_OFFSET, &offset, GAC_COMMENT, &p,
			-1);
	if(p != NULL && strlen(p) > 0)
	{
		snprintf(buf, sizeof(buf), "0x%x", offset);
		if(config_set(args->config, "comments", buf, p) != 0)
			args->ret = -_gdeasm_error(args->gdeasm,
					error_get(NULL), 1);
	}
	g_free(p);
	return (args->ret == 0) ? FALSE : TRUE;
}


/* gdeasm_save_comments_dialog */
int gdeasm_save_comments_dialog(GDeasm * gdeasm)
{
	int ret = -1;
	GtkWidget * dialog;
	GtkFileFilter * filter;
	char * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Save comments as..."),
			GTK_WINDOW(gdeasm->window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("GDeasm files"));
        gtk_file_filter_add_pattern(filter, "*.gdeasm");
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
	if(filename != NULL)
		ret = gdeasm_save_comments(gdeasm, filename);
	g_free(filename);
	return ret;
}


/* private */
/* accessors */
static void _gdeasm_set_status(GDeasm * gdeasm, char const * status)
{
	GtkStatusbar * statusbar = GTK_STATUSBAR(gdeasm->statusbar);
	guint id;

	id = gtk_statusbar_get_context_id(statusbar, "");
	gtk_statusbar_pop(statusbar, id);
	gtk_statusbar_push(statusbar, id, status);
}


/* useful */
/* gdeasm_confirm */
static int _gdeasm_confirm(GDeasm * gdeasm, char const * message, ...)
{
	GtkWidget * dialog;
	va_list ap;
	char const * action;
	int res;

	dialog = gtk_message_dialog_new(GTK_WINDOW(gdeasm->window),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_NONE,
# if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	va_start(ap, message);
	while((action = va_arg(ap, char const *)) != NULL)
		gtk_dialog_add_button(GTK_DIALOG(dialog),
				action, va_arg(ap, int));
	va_end(ap);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return res;
}


/* gdeasm_error */
static int _gdeasm_error(GDeasm * gdeasm, char const * message, int ret)
{
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(gdeasm->window),
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


/* callbacks */
/* gdeasm_on_about */
static void _gdeasm_on_about(gpointer data)
{
	GDeasm * gdeasm = data;
	GtkWidget * dialog;

	dialog = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(
				gdeasm->window));
	desktop_about_dialog_set_authors(dialog, _gdeasm_authors);
	desktop_about_dialog_set_comments(dialog,
			_("Disassembler for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(dialog, _gdeasm_copyright);
	desktop_about_dialog_set_logo_icon_name(dialog,
			"applications-development");
	desktop_about_dialog_set_license(dialog, _gdeasm_license);
	desktop_about_dialog_set_name(dialog, _("GDeasm"));
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog,
			"https://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* gdeasm_on_close */
static void _gdeasm_on_close(gpointer data)
{
	GDeasm * gdeasm = data;
	int res;

	if(gdeasm->modified != FALSE)
	{
		res = _gdeasm_confirm(gdeasm, _("There are unsaved comments.\n"
					"Discard or save them?"),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				NULL);
		if(res == GTK_RESPONSE_ACCEPT)
		{
			if(gdeasm_save_comments_dialog(gdeasm) != 0)
				return;
		}
		else if(res != GTK_RESPONSE_CLOSE)
			return;
	}
	gtk_widget_hide(gdeasm->window);
	gtk_main_quit();
}


/* gdeasm_on_closex */
static gboolean _gdeasm_on_closex(gpointer data)
{
	GDeasm * gdeasm = data;

	_gdeasm_on_close(gdeasm);
	return TRUE;
}


/* gdeasm_on_comment_edited */
static void _gdeasm_on_comment_edited(GtkCellRendererText * renderer,
		gchar * arg1, gchar * arg2, gpointer data)
{
	GDeasm * gdeasm = data;
	GtkTreeModel * model = GTK_TREE_MODEL(gdeasm->asm_store);
	GtkTreeIter iter;
	(void) renderer;

	if(gtk_tree_model_get_iter_from_string(model, &iter, arg1) == TRUE)
	{
		gtk_tree_store_set(gdeasm->asm_store, &iter, 7, arg2, -1);
		gdeasm->modified = TRUE;
	}
}


/* gdeasm_on_function_activated */
static void _gdeasm_on_function_activated(GtkTreeView * view,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data)
{
	GDeasm * gdeasm = data;
	GtkTreeModel * model = GTK_TREE_MODEL(gdeasm->func_store);
	GtkTreeIter iter;
	GtkTreeIter parent;
	gint offset;
	gboolean valid;
	gint u;
	GtkTreeSelection * treesel;
	(void) column;

	if(gtk_tree_model_get_iter(model, &iter, path) != TRUE)
		return;
	gtk_tree_model_get(model, &iter, GFC_OFFSET, &offset, -1);
	if(offset < 0)
		return;
	model = GTK_TREE_MODEL(gdeasm->asm_store);
	for(valid = gtk_tree_model_get_iter_first(model, &parent); valid;
			valid = gtk_tree_model_iter_next(model, &parent))
		for(valid = gtk_tree_model_iter_children(model, &iter, &parent);
				valid;
				valid = gtk_tree_model_iter_next(model, &iter))
		{
			gtk_tree_model_get(model, &iter, GAC_BASE, &u, -1);
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %x, %x\n", __func__,
					offset, u);
#endif
			if(offset != u)
				continue;
			view = GTK_TREE_VIEW(gdeasm->asm_view);
			treesel = gtk_tree_view_get_selection(view);
			gtk_tree_selection_select_iter(treesel, &iter);
			path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_to_path(view, path);
			gtk_tree_view_scroll_to_cell(view, path, NULL, FALSE,
					0.0, 0.0);
			gtk_tree_path_free(path);
			return;
		}
}


/* gdeasm_on_load_comments */
static void _gdeasm_on_load_comments(gpointer data)
{
	GDeasm * gdeasm = data;

	gdeasm_load_comments_dialog(gdeasm);
}


/* gdeasm_on_open */
static void _gdeasm_on_open(gpointer data)
{
	GDeasm * gdeasm = data;

	gdeasm_open_dialog(gdeasm);
}


/* gdeasm_on_save_comments */
static void _gdeasm_on_save_comments(gpointer data)
{
	GDeasm * gdeasm = data;

	gdeasm_save_comments_dialog(gdeasm);
}
