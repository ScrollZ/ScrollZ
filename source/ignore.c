/*
 * ignore.c: handles the ingore command for irc 
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
 * $Id: ignore.c,v 1.10 2002-01-13 16:39:35 f Exp $
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

#define NUMBER_OF_IGNORE_LEVELS 9

#define IGNORE_REMOVE 1
#define IGNORE_DONT 2
#define IGNORE_HIGH -1

int	ignore_usernames = 0;
char	highlight_char = '\0';
static	int	ignore_usernames_sums[NUMBER_OF_IGNORE_LEVELS] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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

static	int
ignore_usernames_mask(mask, thing)
	int	mask;
	int	thing;
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
static	void
/**************************** PATCHED by Flier *****************************/
/*ignore_nickname(nick, type, flag)
	char	*nick;
	int	type;
	int	flag;*/
ignore_nickname(nick, type, flag, timedignore)
	char	*nick;
	int	type;
        int	flag;
        char    *timedignore;
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
				strcpy(buffer, msg);
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
/**************************** PATCHED by Flier ******************************/
				/*say("%s from %s", buffer, new->nick);*/
                                if (timedignore) say("%s from %s for %s seconds", buffer, new->nick, timedignore);
                                else say("%s from %s", buffer, new->nick);
/****************************************************************************/
			}
			if ((new->type == 0) && (new->high == 0))
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
/****************************************************************************/
remove_ignore(nick)
	char	*nick;
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
is_ignored(nick, type)
	char	*nick;
	int	type;
{
	Ignore	*tmp;

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
static	void
ignore_list(nick)
	char	*nick;
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
                len = strlen(nick);
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
				sprintf(s, " %cALL%c", highlight_char, 
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
					sprintf(s, " %cPUBLIC%c",
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
					sprintf(s, " %cMSGS%c",
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
					sprintf(s, " %cWALLS%c",
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
					sprintf(s, " %cWALLOPS%c",
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
					sprintf(s, " %cINVITES%c",
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
					sprintf(s, " %cNOTICES%c",
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
					sprintf(s, " %cNOTES%c",
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
					sprintf(s, " %cCTCPS%c",
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
					sprintf(s, " %cCRAP%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_CRAP)
					strmcat(buffer, " DONT-CRAP",
							BIG_BUFFER_SIZE);
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
ignore(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick,
		*type;
 	size_t	len;
	int	flag,
		no_flags;

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
			if ((len = strlen(type)) == 0)
			{
				say("You must specify one of the following:");
				say("\tALL MSGS PUBLIC WALLS WALLOPS INVITES \
NOTICES NOTES NONE");
				return;
			}
/**************************** PATCHED by Flier ******************************/
			/*if (strncmp(type, "ALL", len) == 0)
				ignore_nickname(nick, IGNORE_ALL, flag);
			else if (strncmp(type, "MSGS", len) == 0)
				ignore_nickname(nick, IGNORE_MSGS, flag);
			else if (strncmp(type, "PUBLIC", len) == 0)
				ignore_nickname(nick, IGNORE_PUBLIC, flag);
			else if (strncmp(type, "WALLS", len) == 0)
				ignore_nickname(nick, IGNORE_WALLS, flag);
			else if (strncmp(type, "WALLOPS", len) == 0)
				ignore_nickname(nick, IGNORE_WALLOPS, flag);
			else if (strncmp(type, "INVITES", len) == 0)
				ignore_nickname(nick, IGNORE_INVITES, flag);
			else if (strncmp(type, "NOTICES", len) == 0)
				ignore_nickname(nick, IGNORE_NOTICES, flag);
			else if (strncmp(type, "NOTES", len) == 0)
				ignore_nickname(nick, IGNORE_NOTES, flag);
			else if (strncmp(type, "CTCPS", len) == 0)
				ignore_nickname(nick, IGNORE_CTCPS, flag);
			else if (strncmp(type, "CRAP", len) == 0)
				ignore_nickname(nick, IGNORE_CRAP, flag);*/
                        if (strncmp(type, "ALL", len) == 0)
				ignore_nickname(nick, IGNORE_ALL, flag, subargs);
			else if (strncmp(type, "MSGS", len) == 0)
				ignore_nickname(nick, IGNORE_MSGS, flag, subargs);
			else if (strncmp(type, "PUBLIC", len) == 0)
				ignore_nickname(nick, IGNORE_PUBLIC, flag, subargs);
			else if (strncmp(type, "WALLS", len) == 0)
				ignore_nickname(nick, IGNORE_WALLS, flag, subargs);
			else if (strncmp(type, "WALLOPS", len) == 0)
				ignore_nickname(nick, IGNORE_WALLOPS, flag, subargs);
			else if (strncmp(type, "INVITES", len) == 0)
				ignore_nickname(nick, IGNORE_INVITES, flag, subargs);
			else if (strncmp(type, "NOTICES", len) == 0)
				ignore_nickname(nick, IGNORE_NOTICES, flag, subargs);
			else if (strncmp(type, "NOTES", len) == 0)
				ignore_nickname(nick, IGNORE_NOTES, flag, subargs);
			else if (strncmp(type, "CTCPS", len) == 0)
				ignore_nickname(nick, IGNORE_CTCPS, flag, subargs);
			else if (strncmp(type, "CRAP", len) == 0)
				ignore_nickname(nick, IGNORE_CRAP, flag, subargs);
/****************************************************************************/
			else if (strncmp(type, "NONE", len) == 0)
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
				say("\tALL MSGS PUBLIC WALLS WALLOPS INVITES \
NOTICES NOTES CTCPS CRAP NONE");
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
set_highlight_char(s)
	char	*s;
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
ignore_combo(flag1, flag2)
	int	flag1;
	int	flag2;
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
double_ignore(nick, userhost, type)
	char	*nick,
		*userhost;
	int	type;
{
/**************************** Patched by Flier ******************************/
        if (userhost && is_channel(userhost)) {
            char tmpbuf[mybufsize / 4 + 1];

            strmcpy(tmpbuf, nick, mybufsize / 4);
            strmcat(tmpbuf, "!", mybufsize / 4);
            strmcat(tmpbuf, userhost, mybufsize / 4);
            return(is_ignored(tmpbuf, type));
        }
/****************************************************************************/
	if (userhost)
		return (ignore_combo(is_ignored(nick, type),
			is_ignored(userhost, type)));
	else
		return (is_ignored(nick, type));
}

/**************************** PATCHED by Flier ******************************/
/* Clean up allocated memory */
void CleanUpIgnore() {
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
        }
        tmpstr = tmpbuf;
        if (*tmpstr == ',') tmpstr++;
        fprintf(fp, "IGN   %s %s\n", tmp->nick, tmpstr);
        count++;
    }
    return(count);
}
/****************************************************************************/
