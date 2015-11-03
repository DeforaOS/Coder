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



#ifndef CODER_CODER_H
# define CODER_CODER_H

# include <System.h>
# include <gtk/gtk.h>
# include "project.h"


/* Coder */
/* types */
typedef struct _Coder Coder;


/* functions */
Coder * coder_new(void);
void coder_delete(Coder * coder);

/* useful */
void coder_about(Coder * coder);

int coder_error(Coder * coder, char const * message, int ret);

void coder_file_open(Coder * coder, char const * filename);

/* project */
int coder_project_open(Coder * coder, char const * filename);
void coder_project_open_dialog(Coder * coder);
int coder_project_open_project(Coder * coder, Project * project);
void coder_project_properties(Coder * coder);
int coder_project_save(Coder * coder);
int coder_project_save_as(Coder * coder, char const * filename);
int coder_project_save_dialog(Coder * coder);

/* interface */
void coder_show_files(Coder * coder, gboolean show);
void coder_show_preferences(Coder * coder, gboolean show);

#endif /* !CODER_CODER_H */
