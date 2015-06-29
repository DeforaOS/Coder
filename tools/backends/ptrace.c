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



#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "../debugger.h"


/* ptrace */
/* private */
typedef struct _DebuggerBackend PtraceBackend;

/* platform */
#ifdef __NetBSD__
typedef int ptrace_data_t;
#endif

struct _DebuggerBackend
{
	Debugger * debugger;
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
static PtraceBackend * _ptrace_init(Debugger * debugger);
static void _ptrace_destroy(PtraceBackend * backend);
static int _ptrace_start(PtraceBackend * backend, va_list argp);
static int _ptrace_pause(PtraceBackend * backend);
static int _ptrace_stop(PtraceBackend * backend);
static int _ptrace_continue(PtraceBackend * backend);
static int _ptrace_next(PtraceBackend * backend);
static int _ptrace_step(PtraceBackend * backend);

/* useful */
static void _ptrace_exit(PtraceBackend * backend);
static int _ptrace_request(PtraceBackend * backend, int request, void * addr,
		ptrace_data_t data);
static int _ptrace_schedule(PtraceBackend * backend, int request, void * addr,
		ptrace_data_t data);


/* constants */
static DebuggerBackendDefinition _ptrace_definition =
{
	"ptrace",
	NULL,
	LICENSE_GNU_LGPL3_FLAGS,
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
static PtraceBackend * _ptrace_init(Debugger * debugger)
{
	PtraceBackend * backend;

	if((backend = object_new(sizeof(*backend))) == NULL)
		return NULL;
	backend->debugger = debugger;
	backend->pid = -1;
	backend->source = 0;
	backend->running = FALSE;
	/* events */
	memset(&backend->event, 0, sizeof(backend->event));
#ifdef PTRACE_FORK
	backend->event.pe_set_event = PTRACE_FORK;
#endif
	/* deferred requests */
	backend->request = -1;
	backend->addr = NULL;
	backend->data = 0;
	return backend;
}


/* ptrace_destroy */
static void _ptrace_destroy(PtraceBackend * backend)
{
	if(backend->source != 0)
		g_source_remove(backend->source);
	if(backend->pid > 0)
		g_spawn_close_pid(backend->pid);
	object_delete(backend);
}


/* ptrace_start */
static int _start_parent(PtraceBackend * backend);
/* callbacks */
static void _start_on_child_setup(gpointer data);
static void _start_on_child_watch(GPid pid, gint status, gpointer data);

static int _ptrace_start(PtraceBackend * backend, va_list argp)
{
	char * argv[3] = { NULL, NULL, NULL };
	const unsigned int flags = G_SPAWN_DO_NOT_REAP_CHILD
		| G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	argv[0] = va_arg(argp, char *);
	argv[1] = argv[0];
	if(g_spawn_async(NULL, argv, NULL, flags, _start_on_child_setup,
				backend, &backend->pid, &error) == FALSE)
	{
		error_set_code(-errno, "%s", error->message);
		g_error_free(error);
		return -debugger_error(backend->debugger, error_get(), 1);
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %d\n", __func__, backend->pid);
#endif
	return _start_parent(backend);
}

static int _start_parent(PtraceBackend * backend)
{
	backend->source = g_child_watch_add(backend->pid, _start_on_child_watch,
			backend);
#ifdef PTRACE_FORK
	_ptrace_schedule(backend, PT_SET_EVENT_MASK, &backend->event,
			sizeof(backend->event));
#endif
	return 0;
}

/* callbacks */
static void _start_on_child_setup(gpointer data)
{
	PtraceBackend * backend = data;

	errno = 0;
	if(ptrace(PT_TRACE_ME, 0, (caddr_t)NULL, (ptrace_data_t)0) == -1
			&& errno != 0)
	{
		debugger_error(NULL, strerror(errno), 1);
		_exit(125);
	}
}

static void _start_on_child_watch(GPid pid, gint status, gpointer data)
{
	PtraceBackend * backend = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %d)\n", __func__, pid, status);
#endif
#ifdef G_OS_UNIX
	if(backend->pid != pid)
		return;
	if(WIFSTOPPED(status))
	{
# ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() stopped\n", __func__);
# endif
		backend->running = FALSE;
		if(backend->request >= 0)
		{
			if(_ptrace_request(backend, backend->request,
						backend->addr, backend->data)
					!= 0)
			{
				backend->request = -1;
				return;
			}
			backend->running = TRUE;
			backend->request = -1;
		}
	}
	else if(WIFSIGNALED(status))
	{
# ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() signal %d\n", __func__,
				WTERMSIG(status));
# endif
		_ptrace_exit(backend);
	}
	else if(WIFEXITED(status))
	{
# ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() error %d\n", __func__,
				WEXITSTATUS(status));
# endif
		_ptrace_exit(backend);
	}
#else
	GError * error = NULL;

	if(backend->pid != pid)
		return;
	if(g_spawn_check_exit_status(status, &error) == FALSE)
	{
		error_set_code(WEXITSTATUS(status), "%s", error->message);
		g_error_free(error);
		debugger_error(backend->debugger, error_get(),
				WEXITSTATUS(status));
		_ptrace_exit(backend);
	}
#endif
}


/* ptrace_pause */
static int _ptrace_pause(PtraceBackend * backend)
{
	return _ptrace_schedule(backend, -1, NULL, 0);
}


/* ptrace_stop */
static int _ptrace_stop(PtraceBackend * backend)
{
	return _ptrace_schedule(backend, PT_KILL, NULL, 0);
}


/* ptrace_continue */
static int _ptrace_continue(PtraceBackend * backend)
{
	return _ptrace_schedule(backend, PT_CONTINUE, (caddr_t)1, 0);
}


/* ptrace_next */
static int _ptrace_next(PtraceBackend * backend)
{
	return _ptrace_schedule(backend, PT_SYSCALL, (caddr_t)1, 0);
}


/* ptrace_step */
static int _ptrace_step(PtraceBackend * backend)
{
	return _ptrace_schedule(backend, PT_STEP, (caddr_t)1, 0);
}


/* useful */
/* ptrace_exit */
static void _ptrace_exit(PtraceBackend * backend)
{
	if(backend->source != 0)
		g_source_remove(backend->source);
	backend->source = 0;
	if(backend->pid > 0)
		g_spawn_close_pid(backend->pid);
	backend->pid = -1;
	backend->running = FALSE;
	backend->request = -1;
	backend->addr = NULL;
	backend->data = 0;
}


/* ptrace_request */
static int _ptrace_request(PtraceBackend * backend, int request, void * addr,
		int data)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %p, %d) %d\n", __func__, request, addr,
			data, backend->pid);
#endif
	if(backend->pid <= 0)
		return -1;
	errno = 0;
	if(ptrace(request, backend->pid, addr, data) == -1 && errno != 0)
	{
		error_set_code(-errno, "%s: %s", "ptrace", strerror(errno));
		if(errno == ESRCH)
			_ptrace_exit(backend);
		return -debugger_error(backend->debugger, error_get(), 1);
	}
	backend->running = TRUE;
	return 0;
}


/* ptrace_schedule */
static int _ptrace_schedule(PtraceBackend * backend, int request, void * addr,
		int data)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %p, %d)\n", __func__, request, addr,
			data);
#endif
	if(backend->running)
	{
		/* stop the traced process */
		if(kill(backend->pid, SIGSTOP) != 0)
		{
			error_set_code(-errno, "%s: %s", "kill",
					strerror(errno));
			if(errno == ESRCH)
				_ptrace_exit(backend);
			return -debugger_error(backend->debugger, error_get(),
					1);
		}
		backend->running = FALSE;
		wait(NULL);
		if(request < 0)
			return 0;
		/* schedule the request */
		backend->request = request;
		backend->addr = addr;
		backend->data = data;
		return 0;
	}
	if(request < 0)
		return 0;
	/* we can issue the request directly */
	if(_ptrace_request(backend, request, addr, data) != 0)
		return -1;
	return 0;
}
