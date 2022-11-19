/*
 * ignore.c: handles the ingore command for irc
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
 * $Id: ignore.c,v 1.20 2009-01-08 15:47:06 f Exp $
 */

#include "irc.h"

#include "ignore.h"
#include "ircaux.h"
#include "list.h"
#include "vars.h"
#include "output.h"

/**************************** PATCHED by Flier *****************************/
#include "parse.h"
#include "myvars.h"

extern int matchmcommand _((char *, int));
/***************************************************************************/

/**************************** Patched by Flier ******************************/
/*#define NUMBER_OF_IGNORE_LEVELS 9*/
#define NUMBER_OF_IGNORE_LEVELS 13
/****************************************************************************/

#define IGNORE_REMOVE 1
#define IGNORE_DONT 2
#define IGNORE_HIGH -1

int	ignore_usernames = 0;
char	highlight_char = '\0';
static	int	ignore_usernames_sums[NUMBER_OF_IGNORE_LEVELS] =
/**************************** Patched by Flier ******************************/
	/*{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };*/
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
/*static	int	remove_ignore _((char *));*/
static	void	remove_ignore _((char *));
/****************************************************************************/
static	void	ignore_list _((char *));
static	int	ignore_usernames_mask _((int, int));
/**************************** PATCHED by Flier ******************************/
/*static	void	ignore_nickname _((char *, int, int));*/
static	void	ignore_nickname _((char *, int, int, char *));
/****************************************************************************/

/*
 * Ignore: the ignore list structure,  consists of the nickname, and the type
 * of ignorance which is to take place
 */
typedef struct	IgnoreStru
{
	struct	IgnoreStru *next;
	char	*nick;
	int	type;
	int	dont;
	int	high;
/**************************** Patched by Flier ******************************/
        int     perm;
/****************************************************************************/
}	Ignore;

/* ignored_nicks: pointer to the head of the ignore list */
static	Ignore *ignored_nicks = NULL;

static int 
ignore_usernames_mask (int mask, int thing)
{
	int	i;
	int	p;

	for (i = 0, p = 1; i < NUMBER_OF_IGNORE_LEVELS; i++, p *= 2)
		if (mask & p)
			ignore_usernames_sums[i] += thing;

	mask = 0;
	for (i = 0, p = 1; i < NUMBER_OF_IGNORE_LEVELS; i++, p *= 2)
		if (ignore_usernames_sums[i])
			mask += p;

	return (mask);
}

/*
 * ignore_nickname: adds nick to the ignore list, using type as the type of
 * ignorance to take place.
 */
static void 
ignore_nickname (char *nick, int type, int flag, char *timedignore)
/***************************************************************************/
{
	Ignore	*new;
	char	*msg,
		*ptr;
	char	buffer[BIG_BUFFER_SIZE+1];

	while (nick)
	{
		if ((ptr = index(nick, ',')) != NULL)
			*ptr = '\0';
		if (index(nick, '@'))
			ignore_usernames = ignore_usernames_mask(type, 1);
		if (*nick)
		{
			if (!(new = (Ignore *) list_lookup((List **) &ignored_nicks, nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)))
			{
				if (flag == IGNORE_REMOVE)
				{
					say("%s is not on the ignorance list",
							nick);
					if (ptr)
						*(ptr++) = ',';
					nick = ptr;
					continue;
				}
				else
				{
					if ((new = (Ignore *) remove_from_list((List **) &ignored_nicks, nick)) != NULL)
					{
						new_free(&(new->nick));
						new_free(&new);
					}
					new = (Ignore *)
						new_malloc(sizeof(Ignore));
					new->nick = (char *) 0;
					new->type = 0;
					new->dont = 0;
					new->high = 0;
					malloc_strcpy(&(new->nick), nick);
/**************************** PATCHED by Flier ******************************/
                                        /*upper(new->nick);*/
                                        new->perm = timedignore ? 0 : 1;
/****************************************************************************/
					add_to_list((List **) &ignored_nicks, (List *) new);
				}
			}
			switch (flag)
			{
			case IGNORE_REMOVE:
				new->type &= (~type);
				new->high &= (~type);
				new->dont &= (~type);
				msg = "Not ignoring";
				break;
			case IGNORE_DONT:
				new->dont |= type;
				new->type &= (~type);
				new->high &= (~type);
				msg = "Never ignoring";
				break;
			case IGNORE_HIGH:
				new->high |= type;
				new->type &= (~type);
				new->dont &= (~type);
				msg = "Highlighting";
				break;
			default:
				new->type |= type;
				new->high &= (~type);
				new->dont &= (~type);
				msg = "Ignoring";
				break;
			}
			if (type == IGNORE_ALL)
			{
				switch (flag)
				{
				case IGNORE_REMOVE:
					say("%s removed from ignorance list", new->nick);
					remove_ignore(new->nick);
					break;
				case IGNORE_HIGH:
				    say("Highlighting ALL messages from %s",
					new->nick);
					break;
				case IGNORE_DONT:
				    say("Never ignoring messages from %s",
					new->nick);
					break;
				default:
/**************************** PATCHED by Flier *******************************/
				    /*say("Ignoring ALL messages from %s",
					new->nick);*/
                                    if (timedignore)
                                        say("Ignoring ALL messages from %s for %s seconds",
                                            new->nick, timedignore);
                                    else say("Ignoring ALL messages from %s", new->nick);
/*****************************************************************************/
					break;
				}
				return;
			}
			else if (type)
			{
/**************************** PATCHED by Flier ******************************/
				/*strcpy(buffer, msg);
				if (type & IGNORE_MSGS)
					strcat(buffer, " MSGS");
				if (type & IGNORE_PUBLIC)
					strcat(buffer, " PUBLIC");
				if (type & IGNORE_WALLS)
					strcat(buffer, " WALLS");
				if (type & IGNORE_WALLOPS)
					strcat(buffer, " WALLOPS");
				if (type & IGNORE_INVITES)
					strcat(buffer, " INVITES");
				if (type & IGNORE_NOTICES)
					strcat(buffer, " NOTICES");
				if (type & IGNORE_NOTES)
					strcat(buffer, " NOTES");
				if (type & IGNORE_CTCPS)
					strcat(buffer, " CTCPS");
				if (type & IGNORE_CRAP)
					strcat(buffer, " CRAP");
				say("%s from %s", buffer, new->nick);*/
				strmcpy(buffer, msg, sizeof(buffer));
				if (type & IGNORE_MSGS)
					strmcat(buffer, " MSGS", sizeof(buffer));
				if (type & IGNORE_PUBLIC)
					strmcat(buffer, " PUBLIC", sizeof(buffer));
				if (type & IGNORE_WALLS)
					strmcat(buffer, " WALLS", sizeof(buffer));
				if (type & IGNORE_WALLOPS)
					strmcat(buffer, " WALLOPS", sizeof(buffer));
				if (type & IGNORE_INVITES)
					strmcat(buffer, " INVITES", sizeof(buffer));
				if (type & IGNORE_NOTICES)
					strmcat(buffer, " NOTICES", sizeof(buffer));
				if (type & IGNORE_NOTES)
					strmcat(buffer, " NOTES", sizeof(buffer));
				if (type & IGNORE_CTCPS)
					strmcat(buffer, " CTCPS", sizeof(buffer));
				if (type & IGNORE_CRAP)
					strmcat(buffer, " CRAP", sizeof(buffer));
				if (type & IGNORE_PART)
					strmcat(buffer, " PART", sizeof(buffer));
				if (type & IGNORE_JOIN)
					strmcat(buffer, " JOIN", sizeof(buffer));
				if (type & IGNORE_NICK)
					strmcat(buffer, " NICK", sizeof(buffer));
				if (type & IGNORE_QUIT)
					strmcat(buffer, " QUIT", sizeof(buffer));
                                if (timedignore) say("%s from %s for %s seconds", buffer, new->nick, timedignore);
                                else say("%s from %s", buffer, new->nick);
/****************************************************************************/
			}
/**************************** PATCHED by Flier ******************************/
			/*if ((new->type == 0) && (new->high == 0))*/
                        /* fixes /ignore nick ^type */
                        if ((new->type == 0) && (new->high == 0) && (new->dont == 0))
/****************************************************************************/
				remove_ignore(new->nick);
		}
		if (ptr)
			*(ptr++) = ',';
		nick = ptr;
	}
}

/*
 * remove_ignore: removes the given nick from the ignore list and returns 0.
 * If the nick wasn't in the ignore list to begin with, 1 is returned.
 */
/**************************** PATCHED by Flier ******************************/
/*static	int*/
static void 
remove_ignore (char *nick)
{
	Ignore	*tmp;
/**************************** PATCHED by Flier ******************************/
        int i;
        int notfound=1;
        Ignore  *last=NULL;
        Ignore  *remove;

	/*if ((tmp = (Ignore *) list_lookup((List **) &ignored_nicks, nick, !USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
	{
		if (index(nick, '@'))
			ignore_usernames = ignore_usernames_mask(tmp->type, -1);
		new_free(&(tmp->nick));
		new_free(&tmp);
		return (0);
	}
	return (1);*/
        for (i = 1, tmp = ignored_nicks; tmp; i++) {
            if ((*nick == '#' && matchmcommand(nick, i)) || wild_match(nick, tmp->nick)) {
                remove = tmp;
                if (last) last->next = tmp->next;
                else ignored_nicks = tmp->next;
                tmp = tmp->next;
                say("%s removed from ignore list", remove->nick);
                new_free(&(remove->nick));
                new_free(&remove);
                notfound = 0;
            }
            else {
                last = tmp;
                tmp = tmp->next;
            }
        }
        if (notfound) say("No match found for %s in ignore list", nick);
/****************************************************************************/
}

/*
 * is_ignored: checks to see if nick is being ignored (poor nick).  Checks
 * against type to see if ignorance is to take place.  If nick is marked as
 * IGNORE_ALL or ignorace types match, 1 is returned, otherwise 0 is
 * returned.
 */
int 
is_ignored (char *nick, int type)
{
	Ignore	*tmp;

/**************************** PATCHED by Flier ******************************/
        if (nick == NULL) return 0;
/****************************************************************************/

	if (ignored_nicks)
	{
		if ((tmp = (Ignore *) list_lookup((List **) &ignored_nicks, nick, USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		{
			if (tmp->dont & type)
				return (DONT_IGNORE);
			if (tmp->type & type)
				return (IGNORED);
			if (tmp->high & type)
				return (HIGHLIGHTED);
		}
	}
	return (0);
}

/* ignore_list: shows the entired ignorance list */
static void 
ignore_list (char *nick)
{
	Ignore	*tmp;
 	size_t	len = 0;
	char	buffer[BIG_BUFFER_SIZE+1];
/**************************** PATCHED by Flier ******************************/
        int     i;
/****************************************************************************/

	if (ignored_nicks)
	{
		say("Ignorance List:");
/**************************** PATCHED by Flier ******************************/
		/*if (nick)
		{
			len = strlen(nick);
			upper(nick);
		}
		for (tmp = ignored_nicks; tmp; tmp = tmp->next)*/
                if (nick) len = strlen(nick);
		for (i = 1, tmp = ignored_nicks; tmp; i++, tmp = tmp->next)
/****************************************************************************/
		{
			char	s[BIG_BUFFER_SIZE+1];

			if (nick)
			{
/**************************** Patched by Flier ******************************/
				/*if (strncmp(nick, tmp->nick, len))*/
				if (my_strnicmp(nick, tmp->nick, len))
/****************************************************************************/
					continue;
			}
			*buffer = (char) 0;
			if (tmp->type == IGNORE_ALL)
				strmcat(buffer," ALL",BIG_BUFFER_SIZE);
			else if (tmp->high == IGNORE_ALL)
			{
				snprintf(s, sizeof s, " %cALL%c", highlight_char,
					highlight_char);
				strmcat(buffer, s, BIG_BUFFER_SIZE);
			}
			else if (tmp->dont == IGNORE_ALL)
				strmcat(buffer," DONT-ALL", BIG_BUFFER_SIZE);
			else
			{
				if (tmp->type & IGNORE_PUBLIC)
					strmcat(buffer, " PUBLIC",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_PUBLIC)
				{
					snprintf(s, sizeof s, " %cPUBLIC%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_PUBLIC)
					strmcat(buffer, " DONT-PUBLIC",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_MSGS)
					strmcat(buffer, " MSGS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_MSGS)
				{
					snprintf(s, sizeof s, " %cMSGS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_MSGS)
					strmcat(buffer, " DONT-MSGS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_WALLS)
					strmcat(buffer, " WALLS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_WALLS)
				{
					snprintf(s, sizeof s, " %cWALLS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_WALLS)
					strmcat(buffer, " DONT-WALLS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_WALLOPS)
					strmcat(buffer, " WALLOPS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_WALLOPS)
				{
					snprintf(s, sizeof s, " %cWALLOPS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_WALLOPS)
					strmcat(buffer, " DONT-WALLOPS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_INVITES)
					strmcat(buffer, " INVITES",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_INVITES)
				{
					snprintf(s, sizeof s, " %cINVITES%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_INVITES)
					strmcat(buffer, " DONT-INVITES",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_NOTICES)
					strmcat(buffer, " NOTICES",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NOTICES)
				{
					snprintf(s, sizeof s, " %cNOTICES%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_NOTICES)
					strmcat(buffer, " DONT-NOTICES",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_NOTES)
					strmcat(buffer, " NOTES",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NOTES)
				{
					snprintf(s, sizeof s, " %cNOTES%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_NOTES)
					strmcat(buffer, " DONT-NOTES",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_CTCPS)
					strmcat(buffer, " CTCPS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_CTCPS)
				{
					snprintf(s, sizeof s, " %cCTCPS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_CTCPS)
					strmcat(buffer, " DONT-CTCPS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_CRAP)
					strmcat(buffer, " CRAP",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_CRAP)
				{
					snprintf(s, sizeof s, " %cCRAP%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_CRAP)
					strmcat(buffer, " DONT-CRAP",
							BIG_BUFFER_SIZE);
/**************************** Patched by Flier ******************************/
                                if (tmp->type & IGNORE_PART)
                                    strmcat(buffer, " PART", BIG_BUFFER_SIZE);
                                else if (tmp->high & IGNORE_PART)
                                {
                                    snprintf(s, sizeof s, " %cPART%c",
                                            highlight_char, highlight_char);
                                    strmcat(buffer, s, BIG_BUFFER_SIZE);
                                }
                                else if (tmp->dont & IGNORE_PART)
                                    strmcat(buffer, " DONT-PART",
                                            BIG_BUFFER_SIZE);
                                if (tmp->type & IGNORE_JOIN)
                                    strmcat(buffer, " JOIN", BIG_BUFFER_SIZE);
                                else if (tmp->high & IGNORE_JOIN)
                                {
                                    snprintf(s, sizeof s, " %cJOIN%c",
                                            highlight_char, highlight_char);
                                    strmcat(buffer, s, BIG_BUFFER_SIZE);
                                }
                                else if (tmp->dont & IGNORE_JOIN)
                                    strmcat(buffer, " DONT-JOIN",
                                            BIG_BUFFER_SIZE);
                                if (tmp->type & IGNORE_NICK)
                                    strmcat(buffer, " NICK", BIG_BUFFER_SIZE);
                                else if (tmp->high & IGNORE_NICK)
                                {
                                    snprintf(s, sizeof s, " %cNICK%c",
                                            highlight_char, highlight_char);
                                    strmcat(buffer, s, BIG_BUFFER_SIZE);
                                }
                                else if (tmp->dont & IGNORE_NICK)
                                    strmcat(buffer, " DONT-NICK",
                                            BIG_BUFFER_SIZE);
                                if (tmp->type & IGNORE_QUIT)
                                    strmcat(buffer, " QUIT", BIG_BUFFER_SIZE);
                                else if (tmp->high & IGNORE_QUIT)
                                {
                                    snprintf(s, sizeof s, " %cQUIT%c",
                                            highlight_char, highlight_char);
                                    strmcat(buffer, s, BIG_BUFFER_SIZE);
                                }
                                else if (tmp->dont & IGNORE_QUIT)
                                    strmcat(buffer, " DONT-QUIT",
                                            BIG_BUFFER_SIZE);
/****************************************************************************/
			}
/**************************** PATCHED by Flier ******************************/
			/*say("\t%s:\t%s", tmp->nick, buffer);*/
			say("%2d) %-30s  %s", i, tmp->nick, buffer);
/****************************************************************************/
		}
	}
	else
 		say("There is nothing on the ignorance list.");
}

/*
 * ignore: does the /IGNORE command.  Figures out what type of ignoring the
 * user wants to do and calls the proper ignorance command to do it.
 */
/*ARGSUSED*/
void 
ignore (char *command, char *args, char *subargs)
{
	char	*nick,
		*type;
	int	flag, no_flags, ignore_type;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		no_flags = 1;
		while ((type = next_arg(args, &args)) != NULL)
		{
			no_flags = 0;
			upper(type);
			switch (*type)
			{
			case '^':
				flag = IGNORE_DONT;
				type++;
				break;
			case '-':
				flag = IGNORE_REMOVE;
				type++;
				break;
			case '+':
				flag = IGNORE_HIGH;
				type++;
				break;
			default:
				flag = 0;
				break;
			}
			ignore_type = get_ignore_type(type);
			if (ignore_type != -1)
/**************************** PATCHED by Flier ******************************/
				/*ignore_nickname(nick, ignore_type, flag);*/
                                ignore_nickname(nick, ignore_type, flag, subargs);
/****************************************************************************/
			else if (ignore_type == -1)
			{
				char	*ptr;

				while (nick)
				{
					if ((ptr = index(nick, ',')) != NULL)
						*ptr = (char) 0;
					if (*nick)
					{
/**************************** PATCHED by Flier ******************************/
						/*if (remove_ignore(nick))
							say("%s is not in the ignorance list!", nick);
						else
							say("%s removed from ignorance list", nick);*/
                                                remove_ignore(nick);
/****************************************************************************/
					}
					if (ptr)
						*(ptr++) = ',';
					nick = ptr;
				}
			}
			else
			{
				say("You must specify one of the following:");
/**************************** Patched by Flier ******************************/
				/*say("\tALL MSGS PUBLIC WALLS WALLOPS INVITES \
NOTICES NOTES CTCPS CRAP NONE");*/
                                say("\tALL MSGS PUBLIC WALLS WALLOPS INVITES \
NOTICES NOTES CTCPS CRAP PART JOIN NICK QUIT NONE");
/****************************************************************************/
			}
		}
		if (no_flags)
			ignore_list(nick);
	} else
		ignore_list((char *) 0);
}

/*
 * set_highlight_char: what the name says..  the character to use
 * for highlighting..  either BOLD, INVERSE, or UNDERLINE..
 */
void 
set_highlight_char (char *s)
{
 	size_t	len;

	len = strlen(s);
	upper(s);

	if (!strncmp(s, "BOLD", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "BOLD");
		highlight_char = BOLD_TOG;
	}
	else if (!strncmp(s, "INVERSE", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "INVERSE");
		highlight_char = REV_TOG;
	}
	else if (!strncmp(s, "UNDERLINE", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "UNDERLINE");
		highlight_char = UND_TOG;
	}
	else
		say("HIGHLIGHT_CHAR must be one of BOLD, INVERSE, or \
			UNDERLINE");
}

int 
ignore_combo (int flag1, int flag2)
{
        if (flag1 == DONT_IGNORE || flag2 == DONT_IGNORE)
                return DONT_IGNORE;
        if (flag1 == IGNORED || flag2 == IGNORED)
                return IGNORED;
        if (flag1 == HIGHLIGHTED || flag2 == HIGHLIGHTED)
                return HIGHLIGHTED;
        return 0;
}

/*
 * double_ignore - makes live simpiler when using doing ignore code
 * added, april 1993, phone.
 */
int 
double_ignore (char *nick, char *userhost, int type)
{
/**************************** Patched by Flier ******************************/
        int isignored;

        if (nick && userhost && is_channel(userhost)) {
            char tmpbuf[mybufsize / 4 + 1];

            strmcpy(tmpbuf, nick, mybufsize / 4);
            strmcat(tmpbuf, "!", mybufsize / 4);
            strmcat(tmpbuf, userhost, mybufsize / 4);
            isignored = ignore_combo(is_ignored(tmpbuf, type),
                                     is_ignored(userhost, type));
            isignored = ignore_combo(isignored, is_ignored(nick, type));
            return(isignored);
        }
        else if (nick && userhost) {
            char tmpbuf[mybufsize / 4 + 1];

            strmcpy(tmpbuf, nick, mybufsize / 4);
            strmcat(tmpbuf, "!", mybufsize / 4);
            strmcat(tmpbuf, userhost, mybufsize / 4);
            isignored = ignore_combo(is_ignored(tmpbuf, type), is_ignored(nick, type));
            if (isignored == 0) {
                if (userhost) return(ignore_combo(is_ignored(nick, type),
                                                  is_ignored(userhost, type)));
                else return(is_ignored(nick, type));
            }
            return(isignored);
        }
/****************************************************************************/
	if (userhost)
		return (ignore_combo(is_ignored(nick, type),
			is_ignored(userhost, type)));
	else
		return (is_ignored(nick, type));
}

int 
get_ignore_type (char *type)
{
	size_t len = strlen(type);
	int rv;

	if (len == 0)
		rv = 0;
	if (my_strnicmp(type, "ALL", len) == 0)
		rv = IGNORE_ALL;
	else if (my_strnicmp(type, "MSGS", len) == 0)
		rv = IGNORE_MSGS;
	else if (my_strnicmp(type, "PUBLIC", len) == 0)
		rv = IGNORE_PUBLIC;
	else if (my_strnicmp(type, "WALLS", len) == 0)
		rv = IGNORE_WALLS;
	else if (my_strnicmp(type, "WALLOPS", len) == 0)
		rv = IGNORE_WALLOPS;
	else if (my_strnicmp(type, "INVITES", len) == 0)
		rv = IGNORE_INVITES;
	else if (my_strnicmp(type, "NOTICES", len) == 0)
		rv = IGNORE_NOTICES;
	else if (my_strnicmp(type, "NOTES", len) == 0)
		rv = IGNORE_NOTES;
	else if (my_strnicmp(type, "CTCPS", len) == 0)
		rv = IGNORE_CTCPS;
	else if (my_strnicmp(type, "CRAP", len) == 0)
		rv = IGNORE_CRAP;
/**************************** Patched by Flier ******************************/
        else if (my_strnicmp(type, "PART", len) == 0)
                rv = IGNORE_PART;
        else if (my_strnicmp(type, "JOIN", len) == 0)
                rv = IGNORE_JOIN;
        else if (my_strnicmp(type, "NICK", len) == 0)
                rv = IGNORE_NICK;
        else if (my_strnicmp(type, "QUIT", len) == 0)
                rv = IGNORE_QUIT;
/****************************************************************************/
	else if (my_strnicmp(type, "NONE", len) == 0)
		rv = -1;
	else
		rv = 0;

	return (rv);
}

/**************************** PATCHED by Flier ******************************/
/* Clean up allocated memory */
void 
CleanUpIgnore (void) {
    Ignore *tmpignore;

    while (ignored_nicks) {
        tmpignore = ignored_nicks;
        ignored_nicks = ignored_nicks->next;
        new_free(&(tmpignore->nick));
        new_free(&tmpignore);
    }
}

/* Save ignore list */
int IgnoreSave(fp)
FILE *fp;
{
    int count = 0;
    char *tmpstr;
    char tmpbuf[mybufsize / 2 + 1];
    Ignore *tmp;

    for (tmp = ignored_nicks; tmp; tmp = tmp->next) {
        if (!(tmp->perm)) continue;
        *tmpbuf = '\0';
        if (tmp->type == IGNORE_ALL) strmcpy(tmpbuf, "ALL", mybufsize / 2);
        else {
            if (tmp->type & IGNORE_PUBLIC) strmcat(tmpbuf, ",PUBLIC", mybufsize / 2);
            if (tmp->type & IGNORE_MSGS) strmcat(tmpbuf, ",MSGS", mybufsize / 2);
            if (tmp->type & IGNORE_WALLS) strmcat(tmpbuf, ",WALLS", mybufsize / 2);
            if (tmp->type & IGNORE_WALLOPS) strmcat(tmpbuf, ",WALLOPS", mybufsize / 2);
            if (tmp->type & IGNORE_INVITES) strmcat(tmpbuf, ",INVITES", mybufsize / 2);
            if (tmp->type & IGNORE_NOTICES) strmcat(tmpbuf, ",NOTICES", mybufsize / 2);
            if (tmp->type & IGNORE_NOTES) strmcat(tmpbuf, ",NOTES", mybufsize / 2);
            if (tmp->type & IGNORE_CTCPS) strmcat(tmpbuf, ",CTCPS", mybufsize / 2);
            if (tmp->type & IGNORE_CRAP) strmcat(tmpbuf, ",CRAP", mybufsize / 2);
            if (tmp->type & IGNORE_PART) strmcat(tmpbuf, ",PART", mybufsize / 2);
            if (tmp->type & IGNORE_JOIN) strmcat(tmpbuf, ",JOIN", mybufsize / 2);
            if (tmp->type & IGNORE_NICK) strmcat(tmpbuf, ",NICK", mybufsize / 2);
            if (tmp->type & IGNORE_QUIT) strmcat(tmpbuf, ",QUIT", mybufsize / 2);
        }
        tmpstr = tmpbuf;
        if (*tmpstr == ',') tmpstr++;
        fprintf(fp, "IGN   %s %s\n", tmp->nick, tmpstr);
        count++;
    }
    return(count);
}
/****************************************************************************/
