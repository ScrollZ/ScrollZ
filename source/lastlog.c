/*
 * lastlog.c: handles the lastlog features of irc. 
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
 * $Id: lastlog.c,v 1.7 2001-06-12 18:18:48 f Exp $
 */

#include "irc.h"

#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "vars.h"
#include "ircaux.h"
#include "output.h"

/**************************** PATCHED by Flier ******************************/
extern void StripAnsi _((char *, char *, int));
/****************************************************************************/

static	void	remove_from_lastlog _((Window *));

/*
 * lastlog_level: current bitmap setting of which things should be stored in
 * the lastlog.  The LOG_MSG, LOG_NOTICE, etc., defines tell more about this 
 */
static	int	lastlog_level;
static	int	notify_level;

/*
 * msg_level: the mask for the current message level.  What?  Did he really
 * say that?  This is set in the set_lastlog_msg_level() routine as it
 * compared to the lastlog_level variable to see if what ever is being added
 * should actually be added 
 */
static	int	msg_level = LOG_CRAP;

static	char	*levels[] =
{
	"CRAP",		"PUBLIC",	"MSGS",		"NOTICES",
	"WALLS",	"WALLOPS",	"NOTES",	"OPNOTES",
	"SNOTES",	"ACTIONS",	"DCC",		"CTCP",
	"USERLOG1",	"USERLOG2",	"USERLOG3",	"USERLOG4",
	"BEEP",		"HELP"
};
#define NUMBER_OF_LEVELS (sizeof(levels) / sizeof(char *))

/*
 * bits_to_lastlog_level: converts the bitmap of lastlog levels into a nice
 * string format.
 */
char	*
bits_to_lastlog_level(level)
	int	level;
{
 	static	char	lbuf[81]; /* this *should* be enough for this */
	int	i,
		p;

	if (level == LOG_ALL)
 		strcpy(lbuf, "ALL");
	else if (level == 0)
 		strcpy(lbuf, "NONE");
	else
	{
 		*lbuf = '\0';
		for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
		{
			if (level & p)
			{
 				strmcat(lbuf, levels[i], 80);
 				strmcat(lbuf, " ", 80);
			}
		}
	}
 	return (lbuf);
}

int
parse_lastlog_level(str)
	char	*str;
{
	char	*ptr,
		*rest,
		*s;
 	int	i,
		p,
		level,
		neg;
 	size_t	len;

	level = 0;
	while ((str = next_arg(str, &rest)) != NULL)
	{
		while (str)
		{
			if ((ptr = index(str, ',')) != NULL)
				*ptr++ = '\0';
			if ((len = strlen(str)) != 0)
			{
				char	*cmd = NULL;

				malloc_strcpy(&cmd, str);
				upper(cmd);
				if (strncmp(cmd, "ALL", len) == 0)
					level = LOG_ALL;
				else if (strncmp(cmd, "NONE", len) == 0)
					level = 0;
				else
				{
					if (*str == '-')
					{
						str++;
						s = cmd + 1;
						neg = 1;
/**************************** PATCHED by Flier ******************************/
                                                len--;
/****************************************************************************/
					}
					else {
						neg = 0;
						s = cmd;
					}
/**************************** PATCHED by Flier ******************************/
					if (*str=='+') {
                                            str++;
                                            s=cmd+1;
                                            len--;
                                        }
/****************************************************************************/
					for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
					{
						if (!strncmp(s, levels[i], len))
						{
							if (neg)
								level &= (LOG_ALL ^ p);
							else
								level |= p;
							break;
						}
					}
					if (i == NUMBER_OF_LEVELS)
						say("Unknown lastlog level: %s",
							str);
				}
				new_free(&cmd);
			}
			str = ptr;
		}
		str = rest;
	}
	return (level);
}

/*
 * set_lastlog_level: called whenever a "SET LASTLOG_LEVEL" is done.  It
 * parses the settings and sets the lastlog_level variable appropriately.  It
 * also rewrites the LASTLOG_LEVEL variable to make it look nice 
 */
void
set_lastlog_level(str)
	char	*str;
{
	lastlog_level = parse_lastlog_level(str);
	set_string_var(LASTLOG_LEVEL_VAR, bits_to_lastlog_level(lastlog_level));
	curr_scr_win->lastlog_level = lastlog_level;
}

static	void
remove_from_lastlog(window)
	Window	*window;
{
	Lastlog *tmp;

	if (window->lastlog_tail)
	{
		tmp = window->lastlog_tail->prev;
		new_free(&window->lastlog_tail->msg);
		new_free(&window->lastlog_tail);
		window->lastlog_tail = tmp;
		if (tmp)
			tmp->next = (Lastlog *) 0;
		else
			window->lastlog_head = (Lastlog *) 0;
		window->lastlog_size--;
	}
	else
		window->lastlog_size = 0;
}

/*
 * set_lastlog_size: sets up a lastlog buffer of size given.  If the lastlog
 * has gotten larger than it was before, all previous lastlog entry remain.
 * If it get smaller, some are deleted from the end. 
 */
void
set_lastlog_size(size)
	int	size;
{
	int	i,
		diff;

	if (curr_scr_win->lastlog_size > size)
	{
		diff = curr_scr_win->lastlog_size - size;
		for (i = 0; i < diff; i++)
			remove_from_lastlog(curr_scr_win);
	}
}

/*
 * lastlog: the /LASTLOG command.  Displays the lastlog to the screen. If
 * args contains a valid integer, only that many lastlog entries are shown
 * (if the value is less than lastlog_size), otherwise the entire lastlog is
 * displayed 
 *
 * /lastlog -save filename
 * by StElb <stlb@cs.tu-berlin.de>
 */
/*ARGSUSED*/
void
lastlog(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	cnt,
		from = 0,
		p,
		i,
		level = 0,
 		m_level,
		mask = 0,
		header = 1;
	Lastlog *start_pos;
	char	*match = NULL,
		*save = NULL,
		*expanded = NULL,
		*arg;
 	FILE	*fp = NULL;
 	char	*cmd = (char *) 0;
 	size_t	len;
/**************************** PATCHED by Flier ******************************/
        int     lines=0;
        int     count=0;
        int     timeend=-1;
        int     timestart=-1;
        char    *blah=(char *) 0;
        char    *tmpbuf=(char *) 0;
	Lastlog *tmp;
	Lastlog *next;
/****************************************************************************/

 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);
	cnt = curr_scr_win->lastlog_size;

	while ((arg = next_arg(args, &args)) != NULL)
	{
		if (*arg == '-')
		{
			arg++;
			if (!(len = strlen(arg)))
			{
				header = 0;
				continue;
			}
			malloc_strcpy(&cmd, arg);
			upper(cmd);
/**************************** PATCHED by Flier ******************************/
                        if (!strncmp(cmd,"MAX",len)) {
                            char *ptr=(char *) 0;

                            if ((ptr=next_arg(args,&args))) lines=atoi(ptr);
                            else {
                                say("Need number for MAX");
                                goto out;
                            }
                            if (lines<0) lines=0;
                        }
                        else if (!strncmp(cmd,"CLEAR",len)) {
                            for (tmp=curr_scr_win->lastlog_head;tmp;tmp=next) {
                                next=tmp->next;
                                new_free(&(tmp->msg));
                                new_free(&tmp);
                                count++;
                            }
                            curr_scr_win->lastlog_head=(Lastlog *) 0;
                            curr_scr_win->lastlog_tail=(Lastlog *) 0;
                            curr_scr_win->lastlog_size=0;
                            say("Lastlog cleared, %d entries removed",count);
                            goto out;
                        }
                        else if (!strncmp(cmd,"TIME",len)) {
                            int hours;
                            int minutes=0;
                            char *minstr;
                            char *tmpstr;
                            char *timestr=(char *) 0;

                            if ((timestr=next_arg(args,&args))) {
                                /* time format: 12:30-14:30 or 3:30PM-4:15PM */
                                hours=atoi(timestr);
                                minstr=timestr;
                                while (*minstr && *minstr!='-') {
                                    if (*minstr==':') {
                                        *minstr++='\0';
                                        minutes=atoi(minstr);
                                        timestr=minstr;
                                        break;
                                    }
                                    minstr++;
                                }
                                tmpstr=timestr;
                                while (*tmpstr && *tmpstr!='-') {
                                    if (*tmpstr=='P' || *tmpstr=='p') {
                                        hours+=12;
                                        break;
                                    }
                                    tmpstr++;
                                }
                                if (hours<0 || hours>24) {
                                    say("Illegal start time specified");
                                    continue;
                                }
                                timestart=hours*60+minutes;
                                timestr=index(timestr,'-');
                                if (timestr) {
                                    timestr++;
                                    hours=atoi(timestr);
                                    minstr=timestr;
                                    while (*minstr) {
                                        if (*minstr==':') {
                                            *minstr++='\0';
                                            minutes=atoi(minstr);
                                            timestr=minstr;
                                            break;
                                        }
                                        minstr++;
                                    }
                                    tmpstr=timestr;
                                    while (*tmpstr) {
                                        if (*tmpstr=='P' || *tmpstr=='p') {
                                            hours+=12;
                                            break;
                                        }
                                        tmpstr++;
                                    }
                                    if (hours<0 || hours>24) {
                                        say("Illegal end time specified");
                                        continue;
                                    }
                                    timeend=hours*60+minutes;
                                }
                            }
                            else {
                                say("Need time string for TIME");
                                goto out;
                            }
                        }
			/*if (!strncmp(cmd, "LITERAL", len))*/
			else if (!strncmp(cmd, "LITERAL", len))
/****************************************************************************/
			{
				if (match)
				{
					say("Second -LITERAL argument ignored");
					(void) next_arg(args, &args);
					continue;
				}
				if ((match = next_arg(args, &args)) != NULL)
					continue;
				say("Need pattern for -LITERAL");
 				goto out;
			}
			else if (!strncmp(cmd, "BEEP", len))
			{
				if (match)
				{
					say("-BEEP is exclusive; ignored");
					continue;
				}
				else
					match = "\007";
			}
			else if (!strncmp(cmd, "SAVE", len))
			{
#ifdef DAEMON_UID
				if (getuid() == DAEMON_UID)
				{
					say("You are not permitted to use -SAVE flag");
 					goto out;
				}
#endif /* DAEMON_UID */
				if (save)
				{
					say("Second -SAVE argument ignored");
					(void) next_arg(args, &args);
					continue;
				}
				if ((save = next_arg(args, &args)) != NULL)
				{
					if (!(expanded = expand_twiddle(save)))
					{
						say("Unknown user");
 						goto out;
					}
					if ((fp = fopen(expanded, "w")) != NULL)
 						continue;
					say("Error opening %s: %s", save, strerror(errno));
 					goto out;
				}
				say("Need filename for -SAVE");
 				goto out;
			}
			else
			{
				for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
				{
					if (strncmp(cmd, levels[i], len) == 0)
					{
						mask |= p;
						break;
					}
				}
				if (i == NUMBER_OF_LEVELS)
				{
					say("Unknown flag: %s", arg);
					message_from((char *) 0, LOG_CRAP);
 					goto out;
				}
			}
 			continue;
out:
 			restore_message_from();
 			new_free(&cmd);
 			return;
		}
		else
		{
			if (level == 0)
			{
				if (match || isdigit(*arg))
				{
					cnt = atoi(arg);
					level++;
				}
				else
					match = arg;
			}
			else if (level == 1)
			{
				from = atoi(arg);
				level++;
			}
		}
	}
 	if (cmd)
 		new_free(&cmd);

	start_pos = curr_scr_win->lastlog_head;
	for (i = 0; (i < from) && start_pos; start_pos = start_pos->next)
		if (!mask || (mask & start_pos->level))
			i++;

	for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->next)
		if (!mask || (mask & start_pos->level))
			i++;

	level = curr_scr_win->lastlog_level;
 	m_level = set_lastlog_msg_level(0);
	if (start_pos == (Lastlog *) 0)
		start_pos = curr_scr_win->lastlog_tail;
	else
		start_pos = start_pos->prev;

	/* Let's not get confused here, display a seperator.. -lynx */
	if (header && !save)
		say("Lastlog:");
/**************************** PATCHED by Flier ******************************/
        if (match) {
            blah=(char *) new_malloc(strlen(match)+3);
            sprintf(blah,"*%s*",match);
        }
/****************************************************************************/
	for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->prev)
	{
		if (!mask || (mask & start_pos->level))
		{
			i++;
/**************************** PATCHED by Flier ******************************/
			/*if (match)
			{
 				if (scanstr(start_pos->msg, match)) {
					if (save)
						fprintf(fp, "%s\n", start_pos->msg);
					else
						put_it("%s", start_pos->msg);
 				}
			}
			else
				if (save)
					fprintf(fp, "%s\n", start_pos->msg);
				else
					put_it("%s", start_pos->msg);*/
                        if (timestart>=0 || timeend>=0) {
                            int timeline=-1;
                            char *tmpstr;

                            tmpbuf=(char *) new_malloc(strlen(start_pos->msg)+1);
                            StripAnsi(start_pos->msg,tmpbuf,1);
                            tmpstr=tmpbuf;
                            if (*tmpstr=='(' && isdigit(*(tmpstr+1))) {
                                int hours;
                                int minutes=0;
                                char *minstr;

                                tmpstr++;
                                hours=atoi(tmpstr);
                                minstr=tmpstr;
                                while (*minstr && !isspace(*minstr)) {
                                    if (*minstr==':') {
                                        *minstr++='\0';
                                        minutes=atoi(minstr);
                                        tmpstr=minstr;
                                        break;
                                    }
                                    minstr++;
                                }
                                while (*tmpstr && !isspace(*tmpstr)) {
                                    if (*tmpstr=='P' || *tmpstr=='p') {
                                        hours+=12;
                                        break;
                                    }
                                    tmpstr++;
                                }
                                timeline=hours*60+minutes;
                            }
                            new_free(&tmpbuf);
                            if (timeline>=0) {
                                if (timestart>=0 && timeend>=0) {
                                    if (timeline<timestart || timeline>timeend) continue;
                                }
                                else if (timestart>=0) {
                                    if (timeline<timestart) continue;
                                }
                                else if (timeend>=0) {
                                    if (timeline>timeend) continue;
                                }
                            }
                        }
                        if (!match || wild_match(blah, start_pos->msg)) {
                            if (get_int_var(LASTLOG_ANSI_VAR)) {
                                tmpbuf=(char *) new_malloc(strlen(start_pos->msg)+1);
                                StripAnsi(start_pos->msg,tmpbuf,0);
                            }
                            else malloc_strcpy(&tmpbuf,start_pos->msg);
                            if (save) fprintf(fp, "%s\n", tmpbuf);
                            else put_it("%s", tmpbuf);
                            new_free(&tmpbuf);
                            if (!lines) continue;
                            if (lines==1) break;
                            lines--;
			}
/****************************************************************************/
		}
	}
/**************************** PATCHED by Flier ******************************/
        new_free(&blah);
/****************************************************************************/
	if (save)
	{
		say("Saved Lastlog to %s", expanded);
		fclose(fp);
	}
	if (header)
		say("End of Lastlog");
 	set_lastlog_msg_level(m_level);
 	restore_message_from();
}

/* set_lastlog_msg_level: sets the message level for recording in the lastlog */
int
set_lastlog_msg_level(level)
	int	level;
{
	int	old;

	old = msg_level;
	msg_level = level;
	return (old);
}

/*
 * add_to_lastlog: adds the line to the lastlog.  If the LASTLOG_CONVERSATION
 * variable is on, then only those lines that are user messages (private
 * messages, channel messages, wall's, and any outgoing messages) are
 * recorded, otherwise, everything is recorded 
 */
void
add_to_lastlog(window, line)
	Window	*window;
	char	*line;
{
	Lastlog *new;

	if (window == (Window *) 0)
		window = curr_scr_win;
	if (window->lastlog_level & msg_level)
	{
		/* no nulls or empty lines (they contain "> ") */
		if (line && ((int) strlen(line) > 2))
		{
			new = (Lastlog *) new_malloc(sizeof(Lastlog));
			new->next = window->lastlog_head;
			new->prev = (Lastlog *) 0;
			new->level = msg_level;
			new->msg = (char *) 0;
			malloc_strcpy(&(new->msg), line);

			if (window->lastlog_head)
				window->lastlog_head->prev = new;
			window->lastlog_head = new;

			if (window->lastlog_tail == (Lastlog *) 0)
				window->lastlog_tail = window->lastlog_head;

			if (window->lastlog_size++ == get_int_var(LASTLOG_VAR))
				remove_from_lastlog(window);
		}
	}
}

int
islogged(window)
	Window	*window;
{
	return (window->lastlog_level & msg_level) ? 1 : 0;
}

int
real_notify_level()
{
	return (notify_level);
}

int
real_lastlog_level()
{
	return (lastlog_level);
}

void
set_notify_level(str)
	char	*str;
{
	notify_level = parse_lastlog_level(str);
	set_string_var(NOTIFY_LEVEL_VAR, bits_to_lastlog_level(notify_level));
	curr_scr_win->notify_level = notify_level;
}
