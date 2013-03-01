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



#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <System.h>
#include "simulator.h"
#include "../config.h"

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif


/* private */
/* prototypes */
static int _simulator(char const * model, char const * title);
static int _simulator_list(void);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* simulator */
static int _simulator(char const * model, char const * title)
{
	Simulator * simulator;

	if((simulator = simulator_new(model, title)) == NULL)
		return error_print("simulator");
	gtk_main();
	simulator_delete(simulator);
	return 0;
}


/* simulator_list */
static int _simulator_list(void)
{
	int ret = 0;
	char const models[] = DATADIR "/" PACKAGE "/Simulator/models";
	char const ext[] = ".conf";
	DIR * dir;
	struct dirent * de;
	size_t len;
	char const * sep = "";

	if((dir = opendir(models)) == NULL)
		ret = -_error(models, 1);
	else
	{
		puts("Available models:");
		while((de = readdir(dir)) != NULL)
		{
			if(de->d_name[0] == '.')
				continue;
			if((len = strlen(de->d_name)) <= sizeof(ext))
				continue;
			if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
				continue;
			de->d_name[len - sizeof(ext) + 1] = '\0';
			printf("%s%s", sep, de->d_name);
			sep = ", ";
		}
		if(sep[0] != '\0')
			puts("");
		closedir(dir);
	}
	return ret;
}


/* error */
static int _error(char const * message, int ret)
{
	fputs("simulator: ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: simulator [-m model][-t title]\n"
"       simulator -l\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int list = 0;
	char const * model = NULL;
	char const * title = NULL;

	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "lm:t:")) != -1)
		switch(o)
		{
			case 'l':
				list = 1;
				break;
			case 'm':
				model = optarg;
				break;
			case 't':
				title = optarg;
				break;
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	if(list != 0)
		return (_simulator_list() == 0) ? 0 : 2;
	return (_simulator(model, title) == 0) ? 0 : 2;
}
