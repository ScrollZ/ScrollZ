/*
 * help.c: handles the help stuff for irc 
 *
 * Written by Michael Sandrof, Troy Rollo and Matthew R. Green.
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
 * $Id: help.c,v 1.15 2006-10-31 12:31:27 f Exp $
 */

/*
 * This has been replaced almost entirely from the original by Michael
 * Sandrof in order to fit in with the multiple screen code.
 *
 * ugh, this wasn't easy to do, but I got there, after working out what
 * had been changed and why, by myself - phone, October 1992.
 *
 * And when I started getting /window create working, I discovered new
 * bugs, and there has been a few more major changes in here again.
 * It is illegal to call help from more than one screen, at the moment,
 * because there is to much to keep track of - phone, jan 1993.
 */

#include "irc.h"

#ifndef LITE

#include "ircterm.h"
#include "server.h"
#include "vars.h"
#include "ircaux.h"
#include "input.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "help.h"
#include "scandir.h"

#if defined(ISC22)
extern char *strrchr();
# define rindex strrchr
# include <sys/dirent.h>
# define direct dirent
#endif /* ISC22 */

/* Forward declarations */

static	void	help_me _((char *, char *));
static	void	help_show_paused_topic _((char *, char *));
static	void	create_help_window _((void));
static	void	set_help_screen _((Screen *));
static	int	compar _((const struct dirent **, const struct dirent **));
static	int	selectent _((const struct dirent *));
static	int	show_help _((Window *, char *));
static	void	help_prompt _((char *, char *));
static	void	help_topic _((char *, char *));

/*
 * A few variables here - A lot added to get help working with
 * non - recursive calls to irc_io, and also have it still 
 * reading things from the server(s), so not to ping timeout.
 */
static	Window	*help_window = (Window *) 0;
static	FILE	*help_fp;
static	char	no_help[] = "NOHELP";
static	int	entry_size;
static	char	*this_arg;
static	int	finished_help_paging = 0;
static	int	help_show_directory = 0;
static	int	help_paused_lines;
static	int	dont_pause_topic = 0;
static	Screen  *help_screen = (Screen *) 0;
static	char	*help_paused_topic[100];	/* 100 should be enough */
static	char	paused_topic[128];
static	char	help_topic_list[BIG_BUFFER_SIZE + 1] = "";
static	int	use_help_window = 0;

/* we are piglet */

/* compar: used by scandir to alphabetize the help entries */
static int 
compar (const struct dirent **e1, const struct dirent **e2)
{
	return (my_stricmp((char *) (*e1)->d_name, (char *) (*e2)->d_name));
}

/*
 * selectent: used by scandir to decide which entries to include in the help
 * listing.  
 */
static int 
selectent (const struct dirent *entry)
{
	if (*(entry->d_name) == '.')
		return (0);
	if (my_strnicmp((char *) entry->d_name, this_arg, strlen(this_arg)))
		return (0);
	else
	{
		int len = strlen(entry->d_name);
#ifdef ZCAT
		const char *temp;
	/*
	 * Handle major length of filename is case of suffix .Z:
	 * stripping suffix length
	 */
		temp = &(entry->d_name[len - strlen(ZSUFFIX)]);
		if (!strcmp(temp, ZSUFFIX))
			len -= strlen(ZSUFFIX);
#endif /*ZCAT*/
		entry_size = (len > entry_size) ? len : entry_size;
		return (1);
	}
}

/*
 * show_help:  show's either a page of text from a help_fp, or the whole
 * thing, depending on the value of HELP_PAGER_VAR.  If it gets to the end,
 * (in either case it will eventally), it closes the file, and returns 0
 * to indicate this.
 */ 
static	int
show_help(window, name)
	Window	*window;
	char	*name;
{
	Window	*old_window;
	int	rows = 0;
	char	line[81];

	if (!help_fp)
		return (0);
	if (window)
	{
		old_window = curr_scr_win;
		curr_scr_win = window;
	}
	else
	{
		old_window = (Window *) 0;
		window = curr_scr_win;
	}
	if (get_int_var(HELP_PAGER_VAR))
		rows = window->display_size;
	while (--rows)
	{
 		if (fgets(line, 80, help_fp))
		{
			if (*(line + strlen(line) - 1) == '\n')
			*(line + strlen(line) - 1) = (char) 0;

	/*
	 * I want to remove the else portion of this code, as I
	 * find it offsensive, but too many help files rely on
	 * it.. sigh.. -phone
	 */
#if defined(NON_FASCIST_HELP)
			if (*line != '!' && *line != '#')
				help_put_it(name, "%s", line);
			else
				rows++;
#else

			switch (*line)
			{
			case '!':
			case '#':
				rows++;
				break;
			case '*':
				if (get_server_operator(from_server))
					help_put_it(name, "%s", line + 1);
				else
					rows++;
				break;
			case '-':
				if (!get_server_operator(from_server))
					help_put_it(name, "%s", line + 1);
				else
					rows++;
				break;
			default:
				help_put_it(name, "%s", line);
				break;
			}
#endif
		}
		else
			return (0);
	}
	return (1);
}

/*
 * help_prompt: The main procedure called to display the help file
 * currently being accessed.  Using add_wait_prompt(), it sets it
 * self up to be recalled when the next page is asked for.   If
 * called when we have finished paging the help file, we exit, as
 * there is nothing left to show.  If line is 'q' or 'Q', exit the
 * help pager, clean up, etc..  If all is cool for now, we call
 * show_help, and either if its finished, exit, or prompt for the
 * next page.   From here, if we've finished the help page, and
 * doing help prompts, prompt for the help..
 */

static void 
help_prompt (char *name, char *line)
{
	if (finished_help_paging)
	{
		if (*paused_topic)
			help_show_paused_topic(paused_topic, (char *) 0);
		return;
	}

	if (line && ((*line == 'q') || (*line == 'Q')))
	{
		finished_help_paging = 1;
		if (help_fp)
		{
			fclose(help_fp);
			help_fp = NULL;
		}
		set_help_screen((Screen *) 0);
		return;
	}

	if (show_help(help_window, name))
		if (dumb)
			help_prompt(name,(char *) 0);
		else
			add_wait_prompt("*** Hit any key for more, 'q' to quit ***",
				help_prompt, name, WAIT_PROMPT_KEY);
	else
	{
		finished_help_paging = 1;
		if (help_fp)
		{
			fclose(help_fp);
			help_fp = NULL;
		}
		if (help_show_directory)
		{
			if (get_int_var(HELP_PAGER_VAR))
				if (dumb)
					help_show_paused_topic(paused_topic, (char *) 0);
				else
					add_wait_prompt("*** Hit any key to end ***", 
						help_show_paused_topic, paused_topic,
						WAIT_PROMPT_KEY);
			else
			{
				help_show_paused_topic(paused_topic, (char *) 0);
				set_help_screen((Screen *) 0);
			}
			help_show_directory = 0;
			return;
		}
	}

	if (finished_help_paging)
	{
		if (get_int_var(HELP_PROMPT_VAR))
		{
			char	tmp[BIG_BUFFER_SIZE + 1];

			snprintf(tmp, sizeof tmp, "%s%sHelp? ", help_topic_list,
				*help_topic_list ? " " : "");
			if (!dumb)
				add_wait_prompt(tmp, help_me, help_topic_list,
					WAIT_PROMPT_LINE);
		}
		else
		{
			if (*paused_topic)
				help_show_paused_topic(paused_topic, (char *) 0);
			set_help_screen((Screen *) 0);
		}
	}
}

/*
 * help_topic:  Given a topic, we search the help directory, and try to
 * find the right file, if all is cool, and we can open it, or zcat it,
 * then we call help_prompt to get the actually displaying of the file
 * on the road.
 */
static void 
help_topic (char *path, char *name)
{
	struct	stat	stat_buf;
	char	filename[BIG_BUFFER_SIZE+1];

#ifdef ZCAT
	char	*name_z = (char *) 0;
	char	*temp;
#endif /* ZCAT */

	if (name == (char *) 0)
		return;

	/*
	 * Check the existence of <name> or <name>.Z .. Handle suffix
	 * .Z if present.  Open the file if it isn't present, zcat the
	 * file if it is present, and ends with .Z ..
	 */

	snprintf(filename, sizeof filename, "%s/%s", path, name);

#ifdef ZCAT

	if (strcmp(name + (strlen(name) - strlen(ZSUFFIX)), ZSUFFIX))
	{
		malloc_strcpy(&name_z, name);
		malloc_strcat(&name_z, ZSUFFIX);
	}
	if (stat(filename, &stat_buf) == -1)
	{
		snprintf(filename, sizeof filename, "%s/%s", path, name_z);
		if (stat(filename, &stat_buf) == -1)
		{
			help_put_it(name, "*** No help available on %s: Use \
? for list of topics", name);
			return;
		}
		else
			name = name_z;
	}
	else
		new_free(&name_z);
#else
	if (stat(filename, &stat_buf) == -1)
	{
		help_put_it(name, "*** No help available on %s: Use \
? for list of topics", name);
		return;

	}

#endif /* ZCAT */

	if (stat_buf.st_mode & S_IFDIR)
		return;

	if (help_fp)
		fclose(help_fp);
#ifdef ZCAT

	if (strcmp(filename + (strlen(filename) - strlen(ZSUFFIX)), ZSUFFIX))
	{

#endif /* ZCAT */

		if ((help_fp = fopen(filename, "r")) == (FILE *) 0)
		{
			help_put_it(name, "*** No help available on %s: Use \
? for list of topics", name);
			return;
		}
#ifdef ZCAT

	}
	else
	{
		if ((help_fp = zcat(filename)) == (FILE *) 0)
		{
			help_put_it(name, "*** No help available on %s: Use \
? for list of topics", name);
			return;
		}
	}

	/*
	 * If the name ended in a .Z, truncate it, so we display the name
	 * with out the .Z
	 */

	temp = &(name[strlen(name) - strlen(ZSUFFIX)]);
	if (!strcmp(temp, ZSUFFIX))
		temp[0] = '\0';
	
#endif /* ZCAT */

	/*
	 * Hopefully now we have got a file descriptor <help_fp>, a name
	 * so we start displaying the help file, calling help_prompt for
	 * the first time.
	 */

	help_put_it(name, "*** Help on %s", name);
	help_prompt(name, (char *) 0);
}

/*
 * help_pause_add_line: this procedure does a help_put_it() call, but
 * puts off the calling, until help_show_paused_topic() is called.
 * I do this because I need to create the list of help topics, but
 * not show them, until we've seen the whole file, so we called
 * help_show_paused_topic() when we've seen the file, if it is needed.
 */

/*VARARGS*/
static	void
#ifdef HAVE_STDARG_H
help_pause_add_line(char *format, ...)
{
	va_list vl;
#else
help_pause_add_line(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
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

	char	buf[BIG_BUFFER_SIZE+1];

#ifdef HAVE_STDARG_H
	va_start(vl, format);
	vsnprintf(buf, sizeof buf, format, vl);
	va_end(vl);
#else
	snprintf(buf, sizeof buf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
			arg8, arg9, arg10);
#endif
	malloc_strcpy(&help_paused_topic[help_paused_lines], buf);
	help_paused_lines++;
}

/*
 * help_show_paused_topic:  see above.  Called when we've seen the
 * whole help file, and we have a list of topics to display.
 */
static	void
help_show_paused_topic(name, unused)
	char	*name;
	char	*unused;
{
	int	i = 0;

	if (!help_paused_lines)
		return;
	for (i = 0; i < help_paused_lines; i++)
	{
		help_put_it(name, "%s", help_paused_topic[i]);
		new_free(&help_paused_topic[i]);
	}
	if (get_int_var(HELP_PROMPT_VAR))
	{
		char	buf[BIG_BUFFER_SIZE+1];

		snprintf(buf, sizeof buf, "%s%sHelp? ", name, (name && *name) ? " " : "");
		if (!dumb)
			add_wait_prompt(buf, help_me, name, WAIT_PROMPT_LINE);
	}
	else
		set_help_screen((Screen *) 0);

	dont_pause_topic = 0;
	help_paused_lines = 0;
}

/*
 * help_me:  The big one.  The help procedure that handles working out
 * what was actually requested, sets up the paused topic list if it is
 * needed, does pretty much all the hard work.
 */
static	void
help_me(topics, args)
	char	*topics;
	char	*args;
{
	char	*ptr;
	struct	dirent	**namelist = NULL;
	int	entries,
		free_cnt = 0,
		cnt,
		i,
		cols;
	struct	stat	stat_buf;
	char	path[BIG_BUFFER_SIZE+1];
	int	help_paused_first_call = 0;
	char	*help_paused_path = (char *) 0;
	char	*help_paused_name = (char *) 0;
	char	*temp;
	char	tmp[BIG_BUFFER_SIZE+1];
	char	buffer[BIG_BUFFER_SIZE+1];

#ifdef ZCAT
	char	*arg_z = (char *) 0;
#endif /*ZCAT*/ 

/**************************** Patched by Flier ******************************/
	/*strcpy(help_topic_list, topics);*/
	strmcpy(help_topic_list, topics, sizeof(help_topic_list));
/****************************************************************************/

#ifdef DAEMON_UID
	if (DAEMON_UID == getuid())
		ptr = DEFAULT_HELP_PATH;
	else
#endif
		ptr = get_string_var(HELP_PATH_VAR);

	snprintf(path, sizeof path, "%s/%s", ptr, topics);
	for (ptr = path; (ptr = index(ptr, ' '));)
		*ptr = '/';

	/*
	 * first we check access to the help dir, whinge if we can't, then
	 * work out we need to ask them for more help, else we check the
	 * args list, and do the stuff 
	 */

	if (help_show_directory)
	{
		help_show_paused_topic(paused_topic, (char *) 0);
		help_show_directory = 0;
	}
		
	finished_help_paging = 0;
	if (access(path, R_OK|X_OK))
	{
		help_put_it(no_help, "*** Cannot access help directory!");
		set_help_screen((Screen *) 0);
		return;
	}

	this_arg = next_arg(args, &args);
	if (!this_arg && *help_topic_list && get_int_var(HELP_PROMPT_VAR))
	{
		if ((temp = rindex(help_topic_list, ' ')) != NULL)
			*temp = '\0';
		else
			*help_topic_list = '\0';
		snprintf(tmp, sizeof tmp, "%s%sHelp? ", help_topic_list,
			*help_topic_list ? " " : "");
		if (!dumb)
			add_wait_prompt(tmp, help_me, help_topic_list,
				WAIT_PROMPT_LINE);
		return;
	}

	if (!this_arg)		/*  && *help_topic_list) */
	{
		set_help_screen((Screen *) 0);
		return;
	}

	create_help_window();
	while (this_arg)
	{
 		save_message_from();
		message_from((char *) 0, LOG_CURRENT);
		if (*this_arg == (char) 0)
			help_topic(path, NULL);
		if (strcmp(this_arg, "?") == 0)
		{
			this_arg = empty_string;
			if (!dont_pause_topic)
				dont_pause_topic = 1;
		}
		entry_size = 0;

		/*
		 * here we clean the namelist if it exists, and then go to
		 * work on the directory.. working out if is dead, or if we
		 * can show some help, or create the paused topic list.
		 */

		if (namelist)
		{
			for (i = 0; i < free_cnt; i++)
				new_free(&(namelist[i]));
			new_free(&namelist);
		}
/**************************** PATCHED by Flier ******************************/
                /*free_cnt = entries = scandir(path, &namelist, selectent, (int (*) _((const void *, const void *))) compar);*/
                free_cnt = entries = scandir(path, &namelist,
                                         (const void *)(int (*) _((const struct dirent *))) selectent,
                                         (const void *)(int (*) _((const struct dirent *, const struct dirent *))) compar);
/****************************************************************************/
		/* special case to handle stuff like LOG and LOGFILE */
		if (entries > 1)
		{
#ifdef ZCAT
		/* Check if exist compressed or uncompressed entries */
			malloc_strcpy(&arg_z, this_arg);
			malloc_strcat(&arg_z, ZSUFFIX);
			if (my_stricmp(namelist[0]->d_name, arg_z) == 0 ||
				my_stricmp(namelist[0]->d_name, this_arg) == 0)
#else
			if (my_stricmp(namelist[0]->d_name, this_arg) == 0)
#endif /*ZCAT*/
				entries = 1;
#ifdef ZCAT
			new_free(&arg_z);
#endif /*ZCAT*/
		}

		/*
		 * entries: -1 means something really died, 0 means there
		 * was no help, 1, means it wasn't a directory, and so to
		 * show the help file, and the default means to add the
		 * stuff to the paused topic list..
		 */

		if (!*help_topic_list)
			dont_pause_topic = 1;
		switch (entries)
		{
		case -1:
			help_put_it(no_help, "*** Error during help function: %s", strerror(errno));
			set_help_screen((Screen *) 0);
			if (help_paused_first_call)
			{
				help_topic(help_paused_path, help_paused_name);
				help_paused_first_call = 0;
				new_free(&help_paused_path);
				new_free(&help_paused_name);
			}
			return;
		case 0:
			help_put_it(this_arg, "*** No help available on %s: Use ? for list of topics", this_arg);
			if (!get_int_var(HELP_PROMPT_VAR))
			{
				set_help_screen((Screen *) 0);
				break;
			}
			snprintf(tmp, sizeof tmp, "%s%sHelp? ", help_topic_list,
				*help_topic_list ? " " : "");
			if (!dumb)
				add_wait_prompt(tmp, help_me, help_topic_list,
						WAIT_PROMPT_LINE);
			if (help_paused_first_call)
			{
				help_topic(help_paused_path, help_paused_name);
				help_paused_first_call = 0;
				new_free(&help_paused_path);
				new_free(&help_paused_name);
			}
			for (i = 0; i < free_cnt; i++)
			{
				new_free(&namelist[i]);
			}
			break;
		case 1:
			snprintf(tmp, sizeof tmp, "%s/%s", path, namelist[0]->d_name);
			if (stat(tmp, &stat_buf) == -1)
			{
				for (i = 0; i < free_cnt; i++)
				{
					new_free(&namelist[i]);
				}
				continue;
			}
			if (stat_buf.st_mode & S_IFDIR)
			{
/**************************** Patched by Flier ******************************/
				/*strcpy(path, tmp);
				if (*help_topic_list)
					strcat(help_topic_list, " ");
				strcat(help_topic_list, namelist[0]->d_name);*/
				strmcpy(path, tmp, sizeof(path));
				if (*help_topic_list)
					strmcat(help_topic_list, " ", sizeof(help_topic_list));
				strmcat(help_topic_list, namelist[0]->d_name, sizeof(help_topic_list));
/****************************************************************************/
				if ((this_arg = next_arg(args, &args)) ==
						(char *) 0)
				{
					help_paused_first_call = 1;
					malloc_strcpy(&help_paused_path, path);
					malloc_strcpy(&help_paused_name,
						namelist[0]->d_name);
					dont_pause_topic = -1;
					this_arg = "?";
				}
				for (i = 0; i < free_cnt; i++)
				{
					new_free(&namelist[i]);
				}
				continue;
			}
			else
			{
				help_topic(path, namelist[0]->d_name);
				finished_help_paging = 0;	/* this is a big kludge */
				for (i = 0; i < free_cnt; i++)
				{
					new_free(&namelist[i]);
				}
				break;
			}
		default:
			help_show_directory = 1;
/**************************** Patched by Flier ******************************/
			/*strcpy(paused_topic, help_topic_list);*/
			strmcpy(paused_topic, help_topic_list, sizeof(paused_topic));
/****************************************************************************/
			help_pause_add_line("*** %s choices:", help_topic_list);
			*buffer = (char) 0;
			cnt = 0;
			entry_size += 2;
			cols = (current_screen->co - 10) / entry_size;
			for (i = 0; i < entries; i++)
			{
#ifdef ZCAT
		/*
		 * In tmp store the actual help choice and strip .Z
		 * suffix in compressed files: put filename (without
		 * .Z) on the help screen.  If it is the first choice
		 * cat it to the buffer and save the last choice
		 */
				strmcpy(tmp, namelist[i]->d_name, BIG_BUFFER_SIZE);
				temp = &(tmp[strlen(tmp) - strlen(ZSUFFIX)]);
				if (!strcmp(temp, ZSUFFIX))
					temp[0] = '\0';
				strmcat(buffer, tmp, BIG_BUFFER_SIZE);
#else
				strmcat(buffer, namelist[i]->d_name, BIG_BUFFER_SIZE);
#endif /*ZCAT*/
				if (++cnt == cols)
				{
					help_pause_add_line("%s", buffer);
					*buffer = (char) 0;
					cnt = 0;
				}
				else
				{
					int	x,
						l;

					l = strlen(namelist[i]->d_name);
#ifdef ZCAT
					/* XXX - this needs to be fixed properly */
					if ((temp = rindex(namelist[i]->d_name, '.')) != NULL &&
						index(namelist[i]->d_name, *ZSUFFIX))
					l -= strlen(ZSUFFIX);
#endif /*ZCAT*/
					for (x = l; x < entry_size; x++)
						strmcat(buffer, " ", BIG_BUFFER_SIZE);
				}
			}
			help_pause_add_line("%s", buffer);
			if (help_paused_first_call)
			{
				help_topic(help_paused_path, help_paused_name);
				help_paused_first_call = 0;
				new_free(&help_paused_path);
				new_free(&help_paused_name);
			}
			if (dont_pause_topic == 1)
			{
				help_show_paused_topic(paused_topic, (char *) 0);
				help_show_directory = 0;
			}
			break;
		}
		for (i = 0; i < free_cnt; i++)
		{
			new_free(&namelist[i]);
		}
		new_free(&namelist);
 		restore_message_from();
		break;
	}
	/*
	 * This one is for when there was never a topic and the prompt
	 * never got a topic..  and help_screen was never reset..
	 * phone, jan 1993.
	 */
	if (!*help_topic_list && finished_help_paging)
		set_help_screen((Screen *) 0);
}

/*
 * help: the HELP command, gives help listings for any and all topics out
 * there 
 */
/*ARGSUSED*/
void
help(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*help_path;

	finished_help_paging = 0;
	help_show_directory = 0;
	dont_pause_topic = 0;
	use_help_window = 0;

#ifdef DAEMON_UID
	if (DAEMON_UID == getuid())
		help_path = DEFAULT_HELP_PATH;
	else
#endif
		help_path = get_string_var(HELP_PATH_VAR);
	if (!(help_path && *help_path && !access(help_path, R_OK | X_OK)))
	{
		if (*help_path)
			help_put_it(no_help, "*** HELP_PATH error: %s", strerror(errno));
		else
			help_put_it(no_help, "*** No HELP_PATH variable set");
		return;
        }

	/*
	 * Check that we aren't doing HELP in a more than one screen, as this
	 * really leads to seriously difficult to fix problems!.  This is all
	 * due to the wildly popular /window create.  blah.
	 */
	if (help_path && help_screen && help_screen != current_screen)
	{
		say("You may not run help in two screens");
		return;
	}
	help_screen = current_screen;
	help_window = (Window *) 0;
        help_me(empty_string, (args && *args) ? args : "?");

}

static	void
create_help_window()
{
	if (help_window)
		return;

	if (!dumb && get_int_var(HELP_WINDOW_VAR))
	{
		use_help_window = 1;
		help_window = new_window();
		help_window->hold_mode = OFF;
		update_all_windows();
	}
	else
		help_window = curr_scr_win;
}

static	void
set_help_screen(screen)
	Screen	*screen;
{
	help_screen = screen;
	if (!help_screen && help_window)
	{
		if (use_help_window)
		{
			int display = window_display;

			window_display = 0;
			delete_window(help_window);
			window_display = display;
		}
		help_window = (Window *) 0;
		update_all_windows();
	}
}

#endif /* LITE */
