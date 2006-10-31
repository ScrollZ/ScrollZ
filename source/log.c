/*
 * log.c: handles the irc session logging functions 
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
 * $Id: log.c,v 1.8 2006-10-31 12:31:27 f Exp $
 */

#include "irc.h"

#include <sys/stat.h>

#include "log.h"
#include "vars.h"
#include "output.h"
#include "ircaux.h"

/**************************** Patched by Flier ******************************/
#include "myvars.h"

extern void StripAnsi _((char *, char *, int));
/****************************************************************************/

#if 0
#if defined(POSIX)
# define fchmod(path, mode) chmod(path, mode)
#endif
#endif

FILE	*irclog_fp = (FILE *) 0;

/**************************** PATCHED by Flier ******************************/
/*FILE	*
do_log(flag, logfile, fp)
	int	flag;
	char	*logfile;
	FILE	*fp;*/
void
do_log(flag, logfile, fp)
	int	flag;
	char	*logfile;
	FILE	**fp;
/****************************************************************************/
{
	time_t	t;

	if (logfile == (char *) 0)
/**************************** PATCHED by Flier ******************************/
		/*return ((FILE *) 0);*/
        {
                *fp = (FILE *) 0;
                return;
        }
/****************************************************************************/
	t = time(0);
	if (flag)
	{
/**************************** PATCHED by Flier ******************************/
		/*if (fp)*/
		if (*fp)
/****************************************************************************/
			say("Logging is already on");
		else
		{
#ifdef DAEMON_UID
			if (getuid() == DAEMON_UID)
			{
				say("You are not permitted to use LOG");
				/* fp = (FILE *) 0;  unused */
			}
			else
			{
#endif /* DAEMON_UID */
				say("Starting logfile %s", logfile);
/**************************** PATCHED by Flier ******************************/
				/*if ((fp = fopen(logfile, "a")) != NULL)*/
				if ((*fp = fopen(logfile, "a")) != NULL)
/****************************************************************************/
				{
#ifdef NEED_FCHMOD
					chmod(logfile, S_IREAD | S_IWRITE);
#else
#ifndef _IBMR2
/**************************** PATCHED by Flier ******************************/
					/*fchmod(fileno(fp),S_IREAD | S_IWRITE);*/
					fchmod(fileno(*fp),S_IREAD | S_IWRITE);
/****************************************************************************/
#else
/**************************** PATCHED by Flier ******************************/
					/*int fd = (int) fileno(fp);*/
					int fd = (int) fileno(*fp);
/****************************************************************************/
					fchmod((char *) &fd, S_IREAD |
							S_IWRITE);
#endif /* !_IBMR2 */
#endif /* M_UNIX */
/**************************** PATCHED by Flier ******************************/
					/*fprintf(fp, "IRC log started %.16s\n",
							ctime(&t));
					fflush(fp);*/
					fprintf(*fp, "IRC log started %.16s\n",
							ctime(&t));
					fflush(*fp);
/****************************************************************************/
				}
				else
				{
					say("Couldn't open logfile %s: %s",
						logfile, strerror(errno));
/**************************** PATCHED by Flier ******************************/
					/*fp = (FILE *) 0;*/
					*fp = (FILE *) 0;
/****************************************************************************/
				}
#ifdef DAEMON_UID
			}
#endif /* DAEMON_UID */
		}
	}
	else
	{
/**************************** PATCHED by Flier ******************************/
		/*if (fp)*/
		if (*fp)
/****************************************************************************/
		{
/**************************** PATCHED by Flier ******************************/
			/*fprintf(fp, "IRC log ended %.16s\n", ctime(&t));
			fflush(fp);
			fclose(fp);
			fp = (FILE *) 0;*/
			fprintf(*fp, "IRC log ended %.16s\n", ctime(&t));
			fflush(*fp);
			fclose(*fp);
			*fp = (FILE *) 0;
/****************************************************************************/
			say("Logfile ended");
		}
	}
/**************************** PATCHED by Flier ******************************/
	/*return (fp);*/
/****************************************************************************/
}

/* logger: if flag is 0, logging is turned off, else it's turned on */
void
logger(flag)
	int	flag;
{
	char	*logfile;

	if ((logfile = get_string_var(LOGFILE_VAR)) == (char *) 0)
	{
		say("You must set the LOGFILE variable first!");
		set_int_var(LOG_VAR, 0);
		return;
	}
/**************************** PATCHED by Flier ******************************/
	/*irclog_fp = do_log(flag, logfile, irclog_fp);*/
	do_log(flag, logfile, &irclog_fp);
/****************************************************************************/
	if ((irclog_fp == (FILE *) 0) && flag)
		set_int_var(LOG_VAR, 0);
}

/*
 * set_log_file: sets the log file name.  If logging is on already, this
 * closes the last log file and reopens it with the new name.  This is called
 * automatically when you SET LOGFILE. 
 */
void
set_log_file(filename)
	char	*filename;
{
	char	*expanded;

	if (filename)
	{
		if (strcmp(filename, get_string_var(LOGFILE_VAR)))
			expanded = expand_twiddle(filename);
		else
			expanded = expand_twiddle(get_string_var(LOGFILE_VAR));
		set_string_var(LOGFILE_VAR, expanded);
		new_free(&expanded);
		if (irclog_fp)
		{
			logger(0);
			logger(1);
		}
	}
}

/*
 * add_to_log: add the given line to the log file.  If no log file is open
 * this function does nothing. 
 */
void
add_to_log(fp, line)
	FILE	*fp;
	char	*line;
{
	if (fp)
	{
/**************************** Patched by Flier ******************************/
		/*fprintf(fp, "%s\n", line);*/
                char *tmpbuf = new_malloc(strlen(line) + 1);

                StripAnsi(line, tmpbuf, 0);
                fprintf(fp,"%s\n", tmpbuf);
                new_free(&tmpbuf);
/****************************************************************************/
		fflush(fp);
	}
}
