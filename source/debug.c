/*
 * debug.c - generic debug routines.
 *
 * Copyright (c) 1993 Matthew R. Green.
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
 * $Id: debug.c,v 1.4 2007-03-30 15:27:36 f Exp $
 */

/*
 * void debug(int level,char *format, ...);	* the function to call, at
 *						* most 10 arguments to it
 * int setdlevel(int level); 		* set the debug level to level.
 *					* returns old level
 * int getdlevel();			* returns the debug level..
 * int debuglevel;			* the current level of debugging
 */

#include "irc.h"		/* This is where DEBUG is defined or not */

#ifdef DEBUG
# include <stdio.h>
# include "debug.h"
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# endif

int	debuglevel = 0;

int
setdlevel(level)
	int	level;
{
	int	oldlevel = debuglevel;

	debuglevel = level;
	return oldlevel;
}

int	getdlevel()
{
	return debuglevel;
}

void
#ifdef HAVE_STDARG_H
debug(int level, char *format, ...)
#else
debug(level, format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
	int	level;
	char	*format;
	char	*arg0, *arg1, *arg2, *arg3, *arg4,
		*arg5, *arg6, *arg7, *arg8, *arg9;
#endif
{
#ifdef HAVE_STDARG_H
	va_list vlist;
#endif

	if (!debuglevel || level > debuglevel)
		return;

#ifdef HAVE_STDARG_H
	va_start(vlist, format);
	vfprintf(stderr, format, vlist);
#else
	fprintf(stderr, format, arg0, arg1, arg2, arg3, arg4,
				arg5, arg6, arg7, arg8, arg9);
#endif
	fputc('\n', stderr);
	fflush(stderr);
}
#endif /* DEBUG */
