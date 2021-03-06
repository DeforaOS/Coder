/* $Id$ */
/* Copyright (c) 2011-2020 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Devel Coder */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <Desktop.h>
#include "coder.h"
#include "callbacks.h"
#include "../config.h"

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif


/* functions */
/* callbacks */
/* _on_closex */
gboolean on_closex(gpointer data)
{
	on_file_exit(data);
	return FALSE;
}


/* on_file_new */
void on_file_new(gpointer data)
{
	Coder * coder = data;

	coder_file_open(coder, NULL); /* XXX hack */
}


/* on_file_open */
void on_file_open(gpointer data)
{
	Coder * coder = data;
	GtkWidget * dialog;
	char * file;

	dialog = gtk_file_chooser_dialog_new ("Open file...", NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		coder_file_open(coder, file);
		g_free(file);
	}
	gtk_widget_destroy(dialog);
}


/* on_file_preferences */
void on_file_preferences(gpointer data)
{
	Coder * coder = data;

	coder_show_preferences(coder, TRUE);
}


/* on_file_exit */
void on_file_exit(gpointer data)
{
	(void) data;

	/* FIXME check that everything is properly saved */
	gtk_main_quit();
}


/* on_help_about */
void on_help_about(gpointer data)
{
	Coder * coder = data;

	coder_about(coder);
}


/* on_help_api_reference */
void on_help_api_reference(gpointer data)
{
	Coder * coder = data;

	coder_api_reference(coder);
}


/* on_help_contents */
void on_help_contents(gpointer data)
{
	(void) data;

	desktop_help_contents(PACKAGE, "coder");
}


/* on_project_new */
void on_project_new(gpointer data)
{
	Coder * coder = data;
	Project * project;

	if((project = project_new()) == NULL)
		return;
	coder_project_open_project(coder, project);
	project_properties(project);
}


/* on_project_open */
void on_project_open(gpointer data)
{
	Coder * coder = data;

	coder_project_open_dialog(coder);
}


/* on_project_properties */
void on_project_properties(gpointer data)
{
	Coder * coder = data;

	coder_project_properties(coder);
}


/* on_project_save */
void on_project_save(gpointer data)
{
	Coder * coder = data;

	coder_project_save(coder);
}


/* on_project_save_as */
void on_project_save_as(gpointer data)
{
	Coder * coder = data;

	coder_project_save_dialog(coder);
}


/* on_tools_debugger */
void on_tools_debugger(gpointer data)
{
	Coder * coder = data;
	char * argv[] = { BINDIR "/debugger", "debugger", NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			!= TRUE)
	{
		coder_error(coder, error->message, 1);
		g_error_free(error);
	}
}


/* on_tools_php_console */
void on_tools_php_console(gpointer data)
{
	Coder * coder = data;
	char * argv[] = { BINDIR "/console", "console", NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			!= TRUE)
	{
		coder_error(coder, error->message, 1);
		g_error_free(error);
	}
}


/* on_tools_simulator */
void on_tools_simulator(gpointer data)
{
	Coder * coder = data;
	char * argv[] = { BINDIR "/simulator", "simulator", "-n", NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			!= TRUE)
	{
		coder_error(coder, error->message, 1);
		g_error_free(error);
	}
}


/* on_tools_sql_console */
void on_tools_sql_console(gpointer data)
{
	Coder * coder = data;
	char * argv[] = { BINDIR "/sequel", "sequel", NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			!= TRUE)
	{
		coder_error(coder, error->message, 1);
		g_error_free(error);
	}
}


/* on_view_files */
void on_view_files(gpointer data)
{
	Coder * coder = data;

	coder_show_files(coder, TRUE);
}
