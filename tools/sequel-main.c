/* $Id$ */
/* Copyright (c) 2013-2017 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#include "sequel.h"
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME_SEQUEL
# define PROGNAME_SEQUEL	"sequel"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif


/* private */
/* prototypes */
static int _sequel(void);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* sequel */
static int _sequel(void)
{
	Sequel * sequel;

	if((sequel = sequel_new()) == NULL)
		return error_print(PROGNAME_SEQUEL);
	gtk_main();
	sequel_delete(sequel);
	return 0;
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME_SEQUEL ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s\n"), PROGNAME_SEQUEL);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if(optind != argc)
		return _usage();
	return (_sequel() == 0) ? 0 : 2;
}
