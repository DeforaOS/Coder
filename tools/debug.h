/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
/* Redistribution and use in source and binary forms, with or without\n"
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



#ifndef CODER_DEBUGGER_DEBUG_H
# define CODER_DEBUGGER_DEBUG_H

# include <System.h>
# include "common.h"


/* Debug */
/* types */
typedef struct _DebuggerDebug DebuggerDebug;

typedef struct _DebuggerDebugHelper
{
	Debugger * debugger;
	int (*error)(Debugger * debugger, int code, char const * format, ...);
} DebuggerDebugHelper;

typedef const struct _DebuggerDebugDefinition
{
	char const * name;
	char const * description;
	LicenseFlags license;
	DebuggerDebug * (*init)(DebuggerDebugHelper const * helper);
	void (*destroy)(DebuggerDebug * backend);
	int (*start)(DebuggerDebug * backend, va_list argp);
	int (*pause)(DebuggerDebug * backend);
	int (*stop)(DebuggerDebug * backend);
	int (*_continue)(DebuggerDebug * backend);
	int (*next)(DebuggerDebug * backend);
	int (*step)(DebuggerDebug * backend);
} DebuggerDebugDefinition;


#endif /* !CODER_DEBUGGER_DEBUG_H */
