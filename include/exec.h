/*
 * exec.h: header for exec.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright (c) 1990 Michael Sandrof.
 * Copyright (c) 1991, 1992 Troy Rollo.
 * Copyright (c) 1992-2003 Matthew R. Green.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: exec.h,v 1.2 2003-01-08 20:00:54 f Exp $
 */

#ifndef __exec_h_
#define __exec_h_

#include <sys/types.h>

#if defined(NeXT)		/* lameness for configure/NeXT -phone */
# if !defined(_POSIX_SOURCE) && !defined(BSDWAIT)
#  define BSDWAIT
# endif /* !_POSIX_SOURCE && !BSDWAIT */
#else /* !NeXT */
# ifndef WAITSTUFF_DECLARED
#  ifdef BSDWAIT
#   ifndef WAIT3_DECLARED
struct rusage;
union wait;
int   wait3 _((union wait *, int, struct rusage *));
#   endif /* WAIT3_DECLARED */
#  else /* BSDWAIT */
#   ifndef WAITPID_DECLARED
short waitpid _((int, int *, int));
#   endif /* WAITPID_DECLARED */
#  endif /* BSDWAIT */
# endif /* WAITSTUFF_DECLARED */
#endif /* NeXT */

#ifndef WTERMSIG
# ifndef BSDWAIT /* if wait is NOT a union */
#  define WTERMSIG(status) ((status) & 0177)
# else
#  define WTERMSIG(status) status.w_T.w_Termsig
# endif
#endif

#ifndef WSTOPSIG
# ifndef BSDWAIT
#  define WSTOPSIG(status) ((status) >> 8)
# else
#  define WSTOPSIG(status) status.w_S.w_Stopsig
# endif
#endif

#ifndef WEXITSTATUS
# ifndef BSDWAIT
#  define WEXITSTATUS(status) ((status) & 0xff00) >> 8		/* dgux 5.4.1 */
# else
#  define WEXITSTATUS(status) status.w_T.w_Retcode
# endif
#endif

	int	get_child_exit _((int));
	int	check_wait_status _((int));
	void	check_process_limits _((void));
	void	do_processes _((fd_set *));
	void	set_process_bits _((fd_set *));
	int	text_to_process _((int, char *, int));
	void	clean_up_processes _((void));
	int	is_process _((char *));
	int	get_process_index _((char **));
	void	exec_server_delete _((int));
	int	is_process_running _((int));
	void	add_process_wait _((int, char *));
	void	set_wait_process _((int));
	void	close_all_exec _((void));
	int	logical_to_index _((char *));
	void	execcmd _((char *, char *, char *));

extern	char	*signals[];

#endif /* __exec_h_ */
