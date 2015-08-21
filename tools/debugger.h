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



#ifndef CODER_DEBUGGER_H
# define CODER_DEBUGGER_H

# include <stdarg.h>
# include <System.h>
# include <Devel/Asm.h>


/* Debugger */
/* protected */
/* types */
typedef struct _Debugger Debugger;

/* backend */
typedef struct _DebuggerBackend DebuggerBackend;

typedef struct _DebuggerBackendHelper
{
	Debugger * debugger;
	int (*error)(Debugger * debugger, int code, char const * format, ...);
	void (*set_registers)(Debugger * debugger,
			AsmArchRegister const * registers,
			size_t registers_cnt);
} DebuggerBackendHelper;

typedef const struct _DebuggerBackendDefinition
{
	char const * name;
	char const * description;
	LicenseFlags license;
	DebuggerBackend * (*init)(DebuggerBackendHelper const * helper);
	void (*destroy)(DebuggerBackend * backend);
	int (*open)(DebuggerBackend * backend, char const * arch,
			char const * format, char const * filename);
	int (*close)(DebuggerBackend * backend);
	char const * (*arch_get_name)(DebuggerBackend * backend);
	char const * (*format_get_name)(DebuggerBackend * backend);
} DebuggerBackendDefinition;

/* debug */
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


/* public */
/* functions */
Debugger * debugger_new(void);
void debugger_delete(Debugger * debugger);

/* accessors */
int debugger_is_opened(Debugger * debugger);
int debugger_is_running(Debugger * debugger);

/* useful */
int debugger_open(Debugger * debugger, char const * arch, char const * format,
		char const * filename);
int debugger_open_dialog(Debugger * debugger, char const * arch,
		char const * format);
int debugger_close(Debugger * debugger);

int debugger_error(Debugger * debugger, char const * message, int ret);

int debugger_continue(Debugger * debugger);
int debugger_next(Debugger * debugger);
int debugger_pause(Debugger * debugger);
int debugger_run(Debugger * debugger, ...);
int debugger_runv(Debugger * debugger, va_list ap);
int debugger_step(Debugger * debugger);
int debugger_stop(Debugger * debugger);

#endif /* !CODER_DEBUGGER_H */
