/* $Id$ */
/* Copyright (c) 2011 Pierre Pronchery <khorben@defora.org> */
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



#ifndef CODER_CODER_H
# define CODER_CODER_H

# include <System.h>
# include <gtk/gtk.h>
# include "project.h"


/* Coder */
/* types */
typedef struct _Coder Coder;


/* functions */
Coder * gedi_new(void);
void gedi_delete(Coder * gedi);

/* useful */
void gedi_about(Coder * gedi);

int gedi_error(Coder * gedi, char const * message, int ret);

void gedi_file_open(Coder * gedi, char const * filename);

/* project */
int gedi_project_open(Coder * gedi, char const * filename);
void gedi_project_open_dialog(Coder * gedi);
int gedi_project_open_project(Coder * gedi, Project * project);
void gedi_project_properties(Coder * gedi);
int gedi_project_save(Coder * gedi);
int gedi_project_save_as(Coder * gedi, char const * filename);
int gedi_project_save_dialog(Coder * gedi);

/* interface */
void gedi_show_preferences(Coder * gedi, gboolean show);

#endif /* !CODER_CODER_H */
