/* $Id$ */
/* Copyright (c) 2011-2013 Pierre Pronchery <khorben@defora.org> */
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



#ifndef CODER_CALLBACKS_H
# define CODER_CALLBACKS_H

# include <gtk/gtk.h>


/* callbacks */
gboolean on_closex(gpointer data);

/* file menu */
void on_file_new(gpointer data);
void on_file_open(gpointer data);
void on_file_preferences(gpointer data);
void on_file_exit(gpointer data);

/* project menu */
void on_project_new(gpointer data);
void on_project_open(gpointer data);
void on_project_properties(gpointer data);
void on_project_save(gpointer data);
void on_project_save_as(gpointer data);

/* tools menu */
void on_tools_simulator(gpointer data);
void on_tools_sql_console(gpointer data);

/* help menu */
void on_help_about(gpointer data);
void on_help_contents(gpointer data);

#endif /* !CODER_CALLBACKS_H */
