/* $Id$ */
/* Copyright (c) 2015-2018 Pierre Pronchery <khorben@defora.org> */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. */



#include <stdarg.h>
#include <string.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#include <Desktop.h>
#include <Devel/Asm.h>
#include "../backend.h"
#include "../debugger.h"
#include "../../config.h"
#define _(string) gettext(string)

#ifndef PREFIX
# define PREFIX	"/usr/local"
#endif
#ifndef LIBDIR
# define LIBDIR	PREFIX "/lib"
#endif


/* asm */
/* private */
typedef struct _DebuggerBackend AsmBackend;

struct _DebuggerBackend
{
	DebuggerBackendHelper const * helper;
	Asm * a;
	AsmCode * code;

	guint source;
};


/* prototypes */
/* plug-in */
static AsmBackend * _asm_init(DebuggerBackendHelper const * helper);
static void _asm_destroy(AsmBackend * backend);
static int _asm_open(AsmBackend * backend, char const * arch,
		char const * format, char const * filename);
static char * _asm_open_dialog(AsmBackend * backend, GtkWidget * window,
		char const * arch, char const * format);
static int _asm_close(AsmBackend * backend);
static char const * _asm_arch_get_name(AsmBackend * backend);
static char const * _asm_format_get_name(AsmBackend * backend);


/* constants */
DebuggerBackendDefinition backend =
{
	"asm",
	NULL,
	LICENSE_BSD3_FLAGS,
	_asm_init,
	_asm_destroy,
	_asm_open,
	_asm_open_dialog,
	_asm_close,
	_asm_arch_get_name,
	_asm_format_get_name
};


/* protected */
/* functions */
/* plug-in */
/* asm_init */
static AsmBackend * _asm_init(DebuggerBackendHelper const * helper)
{
	AsmBackend * backend;

	if((backend = object_new(sizeof(*backend))) == NULL)
		return NULL;
	backend->helper = helper;
	backend->a = NULL;
	backend->code = NULL;
	backend->source = 0;
	return backend;
}


/* asm_destroy */
static void _asm_destroy(AsmBackend * backend)
{
	_asm_close(backend);
	object_delete(backend);
}


/* asm_open */
/* callbacks */
static gboolean _open_on_idle(gpointer data);

static int _asm_open(AsmBackend * backend, char const * arch,
		char const * format, char const * filename)
{
	if(_asm_close(backend) != 0)
		return -1;
	if((backend->a = asm_new(arch, format)) == NULL)
		return -1;
	if((backend->code = asm_open_deassemble(backend->a, filename, TRUE))
			== NULL)
	{
		asm_delete(backend->a);
		backend->a = NULL;
		return -1;
	}
	else
		backend->source = g_idle_add(_open_on_idle, backend);
	return 0;
}

static gboolean _open_on_idle(gpointer data)
{
	AsmBackend * backend = data;
	AsmArchRegister const * registers;
	size_t cnt = 0;

	backend->source = 0;
	if((registers = asmcode_get_arch_registers(backend->code)) != NULL)
		for(cnt = 0; registers[cnt].name != NULL; cnt++);
	backend->helper->set_registers(backend->helper->debugger, registers,
			cnt);
	return FALSE;
}


/* asm_open_dialog */
static void _open_dialog_type(GtkWidget * combobox, char const * type,
		char const * value);

static char * _asm_open_dialog(AsmBackend * backend, GtkWidget * window,
		char const * arch, char const * format)
{
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

	dialog = gtk_file_chooser_dialog_new(_("Open file..."),
			(window != NULL) ? GTK_WINDOW(window) : NULL,
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
	_open_dialog_type(awidget, "arch", arch);
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
	_open_dialog_type(fwidget, "format", format);
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
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(awidget)) == 0)
			a = NULL;
		else
#if GTK_CHECK_VERSION(2, 24, 0)
			a = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(awidget));
#else
			a = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
						awidget));
#endif
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(fwidget)) == 0)
			f = NULL;
		else
#if GTK_CHECK_VERSION(2, 24, 0)
			f = gtk_combo_box_text_get_active_text(
					GTK_COMBO_BOX_TEXT(fwidget));
#else
			f = gtk_combo_box_get_active_text(GTK_COMBO_BOX(
						fwidget));
#endif
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	}
	gtk_widget_destroy(dialog);
	g_free(a);
	g_free(f);
	return filename;
}

static void _open_dialog_type(GtkWidget * combobox, char const * type,
		char const * value)
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
#if GTK_CHECK_VERSION(2, 24, 0)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox),
				de->d_name);
#else
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), de->d_name);
#endif
		if(value != NULL && strcmp(de->d_name, value) == 0)
			active = i;
	}
	closedir(dir);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), active);
}


/* asm_close */
static int _asm_close(AsmBackend * backend)
{
	if(backend->source != 0)
		g_source_remove(backend->source);
	backend->source = 0;
	if(backend->a != NULL)
		asm_delete(backend->a);
	backend->a = NULL;
	return 0;
}


/* asm_arch_get_name */
static char const * _asm_arch_get_name(AsmBackend * backend)
{
	return asmcode_get_arch(backend->code);
}


/* asm_format_get_name */
static char const * _asm_format_get_name(AsmBackend * backend)
{
	return asmcode_get_format(backend->code);
}
