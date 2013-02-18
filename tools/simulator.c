/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Coder */
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
/* FIXME:
 * - terminate the underlying server on quit */



#include <sys/wait.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <System.h>
#include "simulator.h"
#include "../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif


/* Simulator */
/* private */
/* types */
struct _Simulator
{
	GPid pid;

	/* widgets */
	GtkWidget * window;
	GtkWidget * socket;
};


/* prototypes */
/* callbacks */
static void _simulator_on_child_watch(GPid pid, gint status, gpointer data);
static gboolean _simulator_on_closex(gpointer data);


/* public */
/* functions */
/* simulator_new */
Simulator * simulator_new(void)
{
	Simulator * simulator;
	GtkWidget * vbox;
	GtkWidget * widget;
	char * argv[] = { BINDIR "/Xephyr", "Xephyr", "-parent", NULL, ":1",
		NULL };
	char buf[16];
	int flags = G_SPAWN_FILE_AND_ARGV_ZERO | G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;

	if((simulator = object_new(sizeof(*simulator))) == NULL)
		return NULL;
	simulator->window = NULL;
	/* widgets */
	simulator->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(simulator->window, 800, 600);
	gtk_window_set_resizable(GTK_WINDOW(simulator->window), FALSE);
	gtk_window_set_title(GTK_WINDOW(simulator->window), "Simulator");
	g_signal_connect_swapped(simulator->window, "delete-event", G_CALLBACK(
				_simulator_on_closex), simulator);
	vbox = gtk_vbox_new(FALSE, 0);
	/* menu bar */
	widget = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	simulator->socket = gtk_socket_new();
	gtk_box_pack_start(GTK_BOX(vbox), simulator->socket, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(simulator->window), vbox);
	gtk_widget_show_all(vbox);
	/* launch Xephyr */
	snprintf(buf, sizeof(buf), "%u", gtk_socket_get_id(
				GTK_SOCKET(simulator->socket)));
	argv[3] = buf;
	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL,
				&simulator->pid, &error) == FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", "Simulator", argv[1],
				error->message);
		g_error_free(error);
		simulator_delete(simulator);
		return NULL;
	}
	g_child_watch_add(simulator->pid, _simulator_on_child_watch,
			simulator);
	gtk_widget_show(simulator->window);
	return simulator;
}


/* simulator_delete */
void simulator_delete(Simulator * simulator)
{
	if(simulator->pid > 0)
		g_spawn_close_pid(simulator->pid);
	/* FIXME also take care of the sub-processes */
	if(simulator->window != NULL)
		gtk_widget_destroy(simulator->window);
	object_delete(simulator);
}


/* private */
/* functions */
/* callbacks */
/* simulator_on_child_watch */
static void _simulator_on_child_watch(GPid pid, gint status, gpointer data)
{
	Simulator * simulator = data;

	if(simulator->pid != pid)
		return;
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status) != 0)
			fprintf(stderr, "%s: %s%u\n", "Simulator",
					"Xephyr exited with status ",
					WEXITSTATUS(status));
		gtk_main_quit();
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", "Simulator",
				"Xephyr exited with signal ", WTERMSIG(status));
		gtk_main_quit();
	}
}


/* simulator_on_closex */
static gboolean _simulator_on_closex(gpointer data)
{
	Simulator * simulator = data;

	gtk_widget_hide(simulator->window);
	gtk_main_quit();
	return TRUE;
}