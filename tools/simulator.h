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



#ifndef CODER_SIMULATOR_H
# define CODER_SIMULATOR_H


/* Simulator */
/* protected */
/* types */
typedef struct _Simulator Simulator;

typedef struct _SimulatorPrefs
{
	int chooser;
	char const * model;
	char const * title;
	char const * command;
} SimulatorPrefs;


/* public */
/* functions */
/* essential */
Simulator * simulator_new(SimulatorPrefs * prefs);
void simulator_delete(Simulator * simulator);

/* useful */
int simulator_error(Simulator * simulator, char const * message, int ret);

int simulator_run(Simulator * simulator, char const * command);

#endif /* !CODER_SIMULATOR_H */
