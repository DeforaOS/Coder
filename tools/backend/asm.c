/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
/* Redistribution and use in source and binary forms, with or without
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



#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <System.h>
#include <Devel/Asm.h>
#include "../debugger.h"


/* asm */
/* private */
typedef struct _DebuggerBackend AsmBackend;

struct _DebuggerBackend
{
	DebuggerBackendHelper const * helper;
	Asm * a;
	AsmCode * code;

	guint source;
};


/* prototypes */
/* plug-in */
static AsmBackend * _asm_init(DebuggerBackendHelper const * helper);
static void _asm_destroy(AsmBackend * backend);
static int _asm_open(AsmBackend * backend, char const * arch,
		char const * format, char const * filename);
static int _asm_close(AsmBackend * backend);

/* useful */


/* constants */
static DebuggerBackendDefinition _asm_definition =
{
	"asm",
	NULL,
	LICENSE_GNU_LGPL3_FLAGS,
	_asm_init,
	_asm_destroy,
	_asm_open,
	_asm_close
};


/* protected */
/* functions */
/* plug-in */
/* asm_init */
static AsmBackend * _asm_init(DebuggerBackendHelper const * helper)
{
	AsmBackend * backend;

	if((backend = object_new(sizeof(*backend))) == NULL)
		return NULL;
	backend->helper = helper;
	backend->a = NULL;
	backend->code = NULL;
	backend->source = 0;
	return backend;
}


/* asm_destroy */
static void _asm_destroy(AsmBackend * backend)
{
	_asm_close(backend);
	object_delete(backend);
}


/* asm_open */
/* callbacks */
static gboolean _open_on_idle(gpointer data);

static int _asm_open(AsmBackend * backend, char const * arch,
		char const * format, char const * filename)
{
	if(_asm_close(backend) != 0)
		return -1;
	if((backend->a = asm_new(arch, format)) == NULL)
		return -1;
	if((backend->code = asm_open_deassemble(backend->a, filename, TRUE))
			== NULL)
	{
		asm_delete(backend->a);
		backend->a = NULL;
		return -1;
	}
	else
		backend->source = g_idle_add(_open_on_idle, backend);
	return 0;
}

static gboolean _open_on_idle(gpointer data)
{
	AsmBackend * backend = data;
	AsmArchRegister const * registers;
	size_t cnt = 0;

	backend->source = 0;
	if((registers = asmcode_get_arch_registers(backend->code)) != NULL)
		for(cnt = 0; registers[cnt].name != NULL; cnt++);
	backend->helper->set_registers(backend->helper->debugger, registers,
			cnt);
	return FALSE;
}


/* asm_close */
static int _asm_close(AsmBackend * backend)
{
	if(backend->source != 0)
		g_source_remove(backend->source);
	backend->source = 0;
	if(backend->a != NULL)
		asm_delete(backend->a);
	backend->a = NULL;
	return 0;
}
