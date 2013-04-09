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



#ifndef CODER_SEQUEL_H
# define CODER_SEQUEL_H


/* Sequel */
/* protected */
/* types */
typedef struct _Sequel Sequel;


/* public */
/* functions */
/* essential */
Sequel * sequel_new(void);
void sequel_delete(Sequel * sequel);

/* useful */
int sequel_error(Sequel * sequel, char const * message, int ret);

#endif /* !CODER_SEQUEL_H */
