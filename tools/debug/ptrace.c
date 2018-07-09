/* $Id$ */
/* Copyright (c) 2015-2016 Pierre Pronchery <khorben@defora.org> */
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



#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#ifdef __NetBSD__
# include <machine/reg.h>
#endif
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <glib.h>
#include "../debug.h"
#define _(string) gettext(string)


/* ptrace */
/* private */
typedef struct _DebuggerDebug PtraceDebug;

/* platform */
#ifdef __NetBSD__
typedef int ptrace_data_t;
#endif

struct _DebuggerDebug
{
	DebuggerDebugHelper const * helper;
	GPid pid;
	guint source;
	gboolean running;

	/* events */
	ptrace_event_t event;

	/* deferred requests */
	int request;
	void * addr;
	ptrace_data_t data;
};


/* prototypes */
/* plug-in */
static PtraceDebug * _ptrace_init(DebuggerDebugHelper const * helper);
static void _ptrace_destroy(PtraceDebug * debug);
static int _ptrace_start(PtraceDebug * debug, va_list argp);
static int _ptrace_pause(PtraceDebug * debug);
static int _ptrace_stop(PtraceDebug * debug);
static int _ptrace_continue(PtraceDebug * debug);
static int _ptrace_next(PtraceDebug * debug);
static int _ptrace_step(PtraceDebug * debug);

/* accessors */
static void _ptrace_get_registers(PtraceDebug * debug);

/* useful */
static void _ptrace_exit(PtraceDebug * debug);
static int _ptrace_request(PtraceDebug * debug, int request, void * addr,
		ptrace_data_t data);
static int _ptrace_schedule(PtraceDebug * debug, int request, void * addr,
		ptrace_data_t data);


/* constants */
DebuggerDebugDefinition debug =
{
	"ptrace",
	NULL,
	LICENSE_BSD3_FLAGS,
	_ptrace_init,
	_ptrace_destroy,
	_ptrace_start,
	_ptrace_pause,
	_ptrace_stop,
	_ptrace_continue,
	_ptrace_next,
	_ptrace_step
};


/* protected */
/* functions */
/* plug-in */
/* ptrace_init */
static PtraceDebug * _ptrace_init(DebuggerDebugHelper const * helper)
{
	PtraceDebug * debug;

	if((debug = object_new(sizeof(*debug))) == NULL)
		return NULL;
	debug->helper = helper;
	debug->pid = -1;
	debug->source = 0;
	debug->running = FALSE;
	/* events */
	memset(&debug->event, 0, sizeof(debug->event));
#ifdef PTRACE_FORK
	debug->event.pe_set_event = PTRACE_FORK;
#endif
	/* deferred requests */
	debug->request = -1;
	debug->addr = NULL;
	debug->data = 0;
	return debug;
}


/* ptrace_destroy */
static void _ptrace_destroy(PtraceDebug * debug)
{
	_ptrace_exit(debug);
	object_delete(debug);
}


/* ptrace_start */
static int _start_parent(PtraceDebug * debug);
/* callbacks */
static void _start_on_child_setup(gpointer data);
static void _start_on_child_watch(GPid pid, gint status, gpointer data);

static int _ptrace_start(PtraceDebug * debug, va_list argp)
{
	char * argv[3] = { NULL, NULL, NULL };
	const unsigned int flags = G_SPAWN_DO_NOT_REAP_CHILD
		| G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if((argv[0] = va_arg(argp, char *)) == NULL)
		return -debug->helper->error(debug->helper->debugger, 1,
				"%s", strerror(EINVAL));
	argv[1] = argv[0];
	if(g_spawn_async(NULL, argv, NULL, flags, _start_on_child_setup,
				debug, &debug->pid, &error) == FALSE)
	{
		error_set_code(-errno, "%s", error->message);
		g_error_free(error);
		return -debug->helper->error(debug->helper->debugger, 1,
				"%s", _("Could not start execution"));
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %d\n", __func__, debug->pid);
#endif
	return _start_parent(debug);
}

static int _start_parent(PtraceDebug * debug)
{
	debug->source = g_child_watch_add(debug->pid, _start_on_child_watch,
			debug);
#ifdef PTRACE_FORK
	_ptrace_schedule(debug, PT_SET_EVENT_MASK, &debug->event,
			sizeof(debug->event));
#endif
	return 0;
}

/* callbacks */
static void _start_on_child_setup(gpointer data)
{
	PtraceDebug * debug = data;
	DebuggerDebugHelper const * helper = debug->helper;

	errno = 0;
	if(ptrace(PT_TRACE_ME, 0, (caddr_t)NULL, (ptrace_data_t)0) == -1
			&& errno != 0)
	{
		helper->error(NULL, 1, "%s", strerror(errno));
		_exit(125);
	}
}

static void _start_on_child_watch(GPid pid, gint status, gpointer data)
{
	PtraceDebug * debug = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %d)\n", __func__, pid, status);
#endif
#ifdef G_OS_UNIX
	if(debug->pid != pid)
		return;
	if(WIFSTOPPED(status))
	{
# ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() stopped\n", __func__);
# endif
		debug->running = FALSE;
		if(debug->request >= 0)
		{
			if(_ptrace_request(debug, debug->request,
						debug->addr, debug->data) != 0)
			{
				debug->request = -1;
				return;
			}
			debug->running = TRUE;
			debug->request = -1;
		}
	}
	else if(WIFSIGNALED(status))
	{
# ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() signal %d\n", __func__,
				WTERMSIG(status));
# endif
		debug->source = 0;
		_ptrace_exit(debug);
	}
	else if(WIFEXITED(status))
	{
# ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() error %d\n", __func__,
				WEXITSTATUS(status));
# endif
		debug->source = 0;
		_ptrace_exit(debug);
	}
#else
	GError * error = NULL;

	if(debug->pid != pid)
		return;
	if(g_spawn_check_exit_status(status, &error) == FALSE)
	{
		error_set_code(WEXITSTATUS(status), "%s", error->message);
		g_error_free(error);
		debug->helper->error(debug->helper->debugger,
				WEXITSTATUS(status), "%s", error_get(NULL),
	}
	debug->source = 0;
	_ptrace_exit(debug);
#endif
}


/* ptrace_pause */
static int _ptrace_pause(PtraceDebug * debug)
{
	return _ptrace_schedule(debug, -1, NULL, 0);
}


/* ptrace_stop */
static int _ptrace_stop(PtraceDebug * debug)
{
	return _ptrace_schedule(debug, PT_KILL, NULL, 0);
}


/* ptrace_continue */
static int _ptrace_continue(PtraceDebug * debug)
{
	return _ptrace_schedule(debug, PT_CONTINUE, (caddr_t)1, 0);
}


/* ptrace_next */
static int _ptrace_next(PtraceDebug * debug)
{
	return _ptrace_schedule(debug, PT_SYSCALL, (caddr_t)1, 0);
}


/* ptrace_step */
static int _ptrace_step(PtraceDebug * debug)
{
	return _ptrace_schedule(debug, PT_STEP, (caddr_t)1, 0);
}


/* accessors */
/* ptrace_get_registers */
static void _ptrace_get_registers(PtraceDebug * debug)
{
#ifdef PT_GETREGS
	DebuggerDebugHelper const * helper = debug->helper;
	struct reg regs;

	if(_ptrace_request(debug, PT_GETREGS, &regs, 0) != 0)
		return;
# if defined(__amd64__)
	/* XXX also support 32-bits on 64-bits */
	helper->set_register(helper->debugger, "rax", regs.regs[_REG_RAX]);
	helper->set_register(helper->debugger, "rcx", regs.regs[_REG_RCX]);
	helper->set_register(helper->debugger, "rdx", regs.regs[_REG_RDX]);
	helper->set_register(helper->debugger, "rbx", regs.regs[_REG_RBX]);
	helper->set_register(helper->debugger, "r8", regs.regs[_REG_R8]);
	helper->set_register(helper->debugger, "r9", regs.regs[_REG_R9]);
	helper->set_register(helper->debugger, "r10", regs.regs[_REG_R10]);
	helper->set_register(helper->debugger, "r11", regs.regs[_REG_R11]);
	helper->set_register(helper->debugger, "r12", regs.regs[_REG_R12]);
	helper->set_register(helper->debugger, "r13", regs.regs[_REG_R13]);
	helper->set_register(helper->debugger, "r14", regs.regs[_REG_R14]);
	helper->set_register(helper->debugger, "r15", regs.regs[_REG_R15]);
	helper->set_register(helper->debugger, "rsi", regs.regs[_REG_RSI]);
	helper->set_register(helper->debugger, "rdi", regs.regs[_REG_RDI]);
	helper->set_register(helper->debugger, "rsp", regs.regs[_REG_RSP]);
	helper->set_register(helper->debugger, "rbp", regs.regs[_REG_RBP]);
	helper->set_register(helper->debugger, "rip", regs.regs[_REG_RIP]);
# elif defined(__i386__)
	helper->set_register(helper->debugger, "eax", regs.r_eax);
	helper->set_register(helper->debugger, "ecx", regs.r_ecx);
	helper->set_register(helper->debugger, "edx", regs.r_edx);
	helper->set_register(helper->debugger, "ebx", regs.r_ebx);
	helper->set_register(helper->debugger, "esi", regs.r_esi);
	helper->set_register(helper->debugger, "edi", regs.r_edi);
	helper->set_register(helper->debugger, "esp", regs.r_esp);
	helper->set_register(helper->debugger, "ebp", regs.r_ebp);
	helper->set_register(helper->debugger, "eip", regs.r_eip);
# endif
#endif
}


/* useful */
/* ptrace_exit */
static void _ptrace_exit(PtraceDebug * debug)
{
	if(debug->source != 0)
		g_source_remove(debug->source);
	debug->source = 0;
	if(debug->pid > 0)
		g_spawn_close_pid(debug->pid);
	debug->pid = -1;
	debug->running = FALSE;
	debug->request = -1;
	debug->addr = NULL;
	debug->data = 0;
}


/* ptrace_request */
static int _ptrace_request(PtraceDebug * debug, int request, void * addr,
		int data)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %p, %d) %d\n", __func__, request, addr,
			data, debug->pid);
#endif
	if(debug->pid <= 0)
		return -1;
	errno = 0;
	if(ptrace(request, debug->pid, addr, data) == -1 && errno != 0)
	{
		error_set_code(-errno, "%s: %s", "ptrace", strerror(errno));
		if(errno == ESRCH)
			_ptrace_exit(debug);
		return -debug->helper->error(debug->helper->debugger, 1,
				"%s", error_get(NULL));
	}
	debug->running = TRUE;
	return 0;
}


/* ptrace_schedule */
static int _ptrace_schedule(PtraceDebug * debug, int request, void * addr,
		int data)
{
	DebuggerDebugHelper const * helper = debug->helper;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %p, %d)\n", __func__, request, addr,
			data);
#endif
	if(debug->running)
	{
		/* stop the traced process */
		if(kill(debug->pid, SIGSTOP) != 0)
		{
			error_set_code(-errno, "%s: %s", "kill",
					strerror(errno));
			if(errno == ESRCH)
				_ptrace_exit(debug);
			return -helper->error(helper->debugger, 1,
					"%s", "Could not schedule command"
					" (could not stop the traced process)");
		}
		debug->running = FALSE;
		wait(NULL);
		if(request < 0)
			return 0;
		/* schedule the request */
		debug->request = request;
		debug->addr = addr;
		debug->data = data;
		return 0;
	}
	if(request < 0)
		return 0;
	/* we can issue the request directly */
	return (_ptrace_request(debug, request, addr, data) == 0) ? 0 : -1;
}
