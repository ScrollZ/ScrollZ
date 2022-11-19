/*
 * output.c: handles a variety of tasks dealing with the output from the irc
 * program 
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
 * $Id: output.c,v 1.18 2008-12-01 15:41:36 f Exp $
 */

#include "irc.h"

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include "output.h"
#include "vars.h"
#include "input.h"
#include "ircterm.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "hook.h"
#include "ctcp.h"
#include "log.h"
#include "alias.h"

#include "buffer.h"

/**************************** PATCHED by Flier ******************************/
#include "status.h"
#include "myvars.h"
/****************************************************************************/

#ifdef NEED_PUTBUF_DECLARED
/*
 * put_it has to be reentrant - this works because the text is used before
 * it's overwritten by the reentering, but it's not The Right Thing ...
 */
static	u_char	putbuf[4*BIG_BUFFER_SIZE + 1] = "";
#endif

	int	in_help = 0;
	int	do_refresh_screen;

/**************************** Patched by Flier ******************************/
extern char *TimeStamp _((int));
/****************************************************************************/

/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
/*ARGSUSED*/
void 
refresh_screen (u_int key, char *ptr)
{
	term_clear_screen();
	if (term_resize())
		recalculate_windows();
	else
		redraw_all_windows();
	update_all_windows();
	update_input(UPDATE_ALL);
}

/* init_windows:  */
void 
init_screen (void)
{
	new_window();
	term_init();
	term_clear_screen();
	term_resize();
	recalculate_windows();
	update_all_windows();
	init_input();
	term_move_cursor(0, 0);
}

/* put_file: uses put_it() to display the contents of a file to the display */
void 
put_file (char *filename)
{
	FILE	*fp;
	char	line[1024];		/* too big?  too small?  who cares? */
 	size_t	len;

	if ((fp = fopen(filename, "r")) != (FILE *) 0)
	{
		while (fgets(line, 1024, fp))
		{
			if (line && *line)
			{
 				if ((len = strlen(line)))
 				{
 					if (*(line + len - 1) == '\n')
 						*(line + len - 1) = (char) 0;
 				}
  				put_it("%s", line);
			}
			else
				put_it(" ");
		}
		fclose(fp);
	}
}

/*
 * put_it: the irc display routine.  Use this routine to display anything to
 * the main irc window.  It handles sending text to the display or stdout as
 * needed, add stuff to the lastlog and log file, etc.  Things NOT to do:
 * Dont send any text that contains \n, very unpredictable.  Tabs will also
 * screw things up.  The calling routing is responsible for not overwriting
 * the 1K buffer allocated.  
 *
 * For Ultrix machines, you can't call put_it() with floating point arguements.
 * It just doesn't work.  - phone, jan 1993.
 */
/*VARARGS*/
Window *
#ifdef HAVE_STDARG_H
put_it(char *format, ...)
{				/* } */
	va_list vl;
#else
put_it(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
        Window *w = NULL;

	if (window_display)
	{
#ifdef HAVE_STDARG_H
		PUTBUF_INIT
		va_start(vl, format);
		PUTBUF_SPRINTF(format, vl)
		va_end(vl);
#else
		snprintf(putbuf, sizeof putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
		add_to_log(irclog_fp, putbuf);
		w = add_to_screen(putbuf);
		PUTBUF_END
	}
        return w;
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
Window *
#ifdef HAVE_STDARG_H
say(char *format, ...)
#else
say(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
#endif
{
#ifdef HAVE_STDARG_H
	va_list vl;
#endif /* HAVE_STDARG_H */
/**************************** PATCHED by Flier ******************************/
        char *stampbuf=TimeStamp(2);
        Window *w = NULL;
/****************************************************************************/

	if (window_display)
	{
		char *fmt = (char *) 0;
		PUTBUF_INIT

/**************************** PATCHED by Flier ******************************/
                if (Stamp==2) malloc_strcpy(&fmt,stampbuf);
                else if (ScrollZstr && *ScrollZstr) {
                    malloc_strcpy(&fmt,ScrollZstr);
                    malloc_strcat(&fmt," ");
                }
                else malloc_strcpy(&fmt,empty_string);
/****************************************************************************/
                malloc_strcat(&fmt, format);
                format = fmt;
#ifdef HAVE_STDARG_H
		va_start(vl, format);
 		PUTBUF_SPRINTF(format, vl)
		va_end(vl);
#else
		snprintf(putbuf, sizeof putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
		add_to_log(irclog_fp, putbuf);
		w = add_to_screen(putbuf);
		PUTBUF_END
		if (fmt)
			new_free(&fmt);
	}
        return w;
}

void
#ifdef HAVE_STDARG_H
yell(char *format, ...)
{				/* } */
	va_list vl;
#else
yell(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
	PUTBUF_INIT

#ifdef HAVE_STDARG_H
	va_start(vl, format);
	PUTBUF_SPRINTF(format, vl)
	va_end(vl);
#else
	snprintf(putbuf, sizeof putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
	add_to_log(irclog_fp, putbuf);
	add_to_screen(putbuf);
	PUTBUF_END
}


#ifndef LITE
/* help_put_it: works just like put_it, but is specially used by help */
void
#ifdef HAVE_STDARG_H
help_put_it(char *topic, char *format, ...)
{						/* } */
	va_list vl;
#else
help_put_it(topic, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format,
		*topic;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
	PUTBUF_INIT
	int	lastlog_level;
#ifdef HAVE_STDARG_H
	va_start(vl, format);
	PUTBUF_SPRINTF(format, vl)
	va_end(vl);
#else
	snprintf(putbuf, sizeof putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif

        in_help = 1;
	lastlog_level = set_lastlog_msg_level(LOG_HELP);
	if (do_hook(HELP_LIST, "%s %s", topic, putbuf))
	{
		if (window_display)
		{
			add_to_log(irclog_fp, putbuf);
			add_to_screen(putbuf);
		}
	}
	(void) set_lastlog_msg_level(lastlog_level);
	in_help = 0;
	PUTBUF_END
}
#endif /* LITE */
