/*
 * output.c: handles a variety of tasks dealing with the output from the irc
 * program 
 *
 * Written By Michael Sandrof
 *
 * Copyright (c) 1990 Michael Sandrof.
 * Copyright (c) 1991, 1992 Troy Rollo.
 * Copyright (c) 1992-1998 Matthew R. Green.
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
 * $Id: output.c,v 1.2 1998-09-10 17:46:00 f Exp $
 */

#include "irc.h"

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#include "output.h"
#include "vars.h"
#include "input.h"
#include "ircterm.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "hook.h"
#include "ctcp.h"
#include "log.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"
/****************************************************************************/

	int	in_help = 0;

/* make this buffer *much* bigger than needed */
/**************************** PATCHED by Flier ******************************/
/*static	char	FAR putbuf[BIG_BUFFER_SIZE + 1] = "";*/
static	char	FAR putbuf[2*BIG_BUFFER_SIZE + 1] = "";
/****************************************************************************/

/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
/*ARGSUSED*/
RETSIGTYPE
#ifdef __STDC__
refresh_screen(unsigned char key, char *ptr)
#else
refresh_screen(key, ptr)
	unsigned char	key;
	char *	ptr;
#endif
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
init_screen()
{
	term_init();
	term_clear_screen();
	term_resize();
	new_window();
	recalculate_windows();
	update_all_windows();
	init_input();
	term_move_cursor(0, 0);
}

/* put_file: uses put_it() to display the contents of a file to the display */
void
put_file(filename)
	char	*filename;
{
	FILE	*fp;
	char	line[256];		/* too big?  too small?  who cares? */
	int	len;

	if ((fp = fopen(filename, "r")) != (FILE *) 0)
	{
		while (fgets(line, 256, fp))
		{
			if (line)
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
void
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
	if (window_display)
	{
#ifdef HAVE_STDARG_H
		va_start(vl, format);
		vsprintf(putbuf, format, vl);
		va_end(vl);
#else
		sprintf(putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
		add_to_log(irclog_fp, putbuf);
		add_to_screen(putbuf);
	}
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
void
#ifdef HAVE_STDARG_H
say(char *format, ...)
{				/* } */
	va_list vl;
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
{
#endif
	if (window_display)
	{
/**************************** PATCHED by Flier ******************************/
		/*putbuf[0] = putbuf[1] = putbuf[2] = '*';
		putbuf[3] = ' ';*/
/****************************************************************************/
#ifdef HAVE_STDARG_H
		va_start(vl, format);
/**************************** PATCHED by Flier ******************************/
		/* vsprintf(&putbuf[4], format, vl); */
                if (ScrollZstr && *ScrollZstr) {
                    sprintf(putbuf,"%s ",ScrollZstr);
                    vsprintf(&putbuf[strlen(putbuf)],format,vl);
                }
                else vsprintf(putbuf,format,vl);
/****************************************************************************/
		va_end(vl);
#else
/**************************** PATCHED by Flier ******************************/
		/* sprintf(&putbuf[4], format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10); */
                if (ScrollZstr && *ScrollZstr) {
                    sprintf(putbuf,"%s ",ScrollZstr);
                    sprintf(&putbuf[strlen(putbuf)],format,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);
                }
                else sprintf(putbuf,format,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10);
/****************************************************************************/
#endif
		add_to_log(irclog_fp, putbuf);
		add_to_screen(putbuf);
	}
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
#ifdef HAVE_STDARG_H
	va_start(vl, format);
	vsprintf(putbuf, format, vl);
	va_end(vl);
#else
	sprintf(putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
	add_to_log(irclog_fp, putbuf);
	add_to_screen(putbuf);
}


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
#ifdef HAVE_STDARG_H
	va_start(vl, format);
	vsprintf(putbuf, format, vl);
	va_end(vl);
#else
	sprintf(putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
	in_help = 1;
	if (do_hook(HELP_LIST, "%s %s", topic, putbuf))
	{
		if (window_display)
		{
			add_to_log(irclog_fp, putbuf);
			add_to_screen(putbuf);
		}
	}
	in_help = 0;
}
