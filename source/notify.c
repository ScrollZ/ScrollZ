/*
 * notify.c: a few handy routines to notify you when people enter and leave irc 
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
 * $Id: notify.c,v 1.14 2003-01-08 20:00:54 f Exp $
 */

/*
 *
 * Revamped by lynX - Dec '91
 */

#include "irc.h"

#include "list.h"
#include "notify.h"
#include "ircaux.h"
#include "whois.h"
#include "hook.h"
#include "server.h"
#include "output.h"
#include "vars.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

extern void HandleNotifyOn _((char *, int));
extern void HandleNotifyOff _((char *, time_t));
extern void HandleNotifyOffUh _((char *, char *, char *, time_t, int));
extern void AddDelayNotify _((char *));
extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));

extern void e_nick _((char *, char *, char *));

/* NotifyList has been moved to notify.h - Flier */
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
/*static	NotifyList	*notify_list = (NotifyList *) 0;*/
NotifyList	*notify_list = (NotifyList *) 0;
/****************************************************************************/

/* Rewritten, -lynx */
char *
get_notify_list (int which)
{
	char	*list = (char *) 0;
	NotifyList	*tmp;
	int first = 0;

	malloc_strcpy(&list, empty_string);
	for (tmp = notify_list; tmp; tmp = tmp->next)
	{
		if ((which & NOTIFY_LIST_ALL) == NOTIFY_LIST_ALL ||
		   ((which & NOTIFY_LIST_HERE) && tmp->flag) ||
		   ((which & NOTIFY_LIST_GONE) && !tmp->flag))
		{
			if (first++)
				malloc_strcat(&list, " ");
			malloc_strcat(&list, tmp->nick);
		}
	}
	return list;
}

/* Rewritten, -lynx */
void 
show_notify_list (int all)
{
/**************************** Patched by Flier ******************************/
	/*char	*list;

	list = get_notify_list(NOTIFY_LIST_HERE);
	if (*list)
		say("Currently present: %s", list);
	if (all)
	{
		new_free(&list);
		list = get_notify_list(NOTIFY_LIST_GONE);
		if (*list)
			say("Currently absent: %s", list);
	}
	new_free(&list);*/
        int i;
        char *list = (char *) 0;
        NotifyList *tmp, *tmp2;

        /* mark all as not printed */
        for (tmp = notify_list; tmp; tmp = tmp->next)
            tmp->printed = 0;
        for (i = 0; i <= all; i++) {
            /* first print users by groups */
            for (tmp = notify_list; tmp; tmp = tmp->next) {
                /* skip users not online */
                if ((i == 0) && (tmp->flag != 1)) continue;
                /* skip online users */
                if ((i == 1) && (tmp->flag == 1)) continue;
                /* skip already printed entries */
                if (tmp->printed) continue;
                /* if a group is found print all entries in that group */
                if (tmp->group) {
                    for (tmp2 = tmp; tmp2; tmp2 = tmp2->next) {
                        /* skip entry if group doesn't match */
                        if (!tmp2->group || strcmp(tmp->group, tmp2->group)) continue;
                        if (strcmp(tmp->group, tmp2->group)) continue;
                        /* skip users not online */
                        if ((i == 0) && (tmp2->flag != 1)) continue;
                        /* skip online users */
                        if ((i == 1) && (tmp2->flag == 1)) continue;
                        tmp2->printed = 1;
                        if (list) malloc_strcat(&list, " ");
                        malloc_strcat(&list, tmp2->nick);
                    }
                    say("Currently %s [%s]: %s", i ? "absent" : "present", tmp->group, list);
                    new_free(&list);
                }
            }
            /* print the rest of list */
            for (tmp = notify_list; tmp; tmp = tmp->next) {
                /* skip users not online */
                if ((i == 0) && (tmp->flag != 1)) continue;
                /* skip online users */
                if ((i == 1) && (tmp->flag == 1)) continue;
                /* skip already printed entries */
                if (tmp->printed) continue;
                tmp->printed = 1;
                if (list) malloc_strcat(&list, " ");
                malloc_strcat(&list, tmp->nick);
            }
            if (list) {
                say("Currently %s: %s", i ? "absent" : "present", list);
                new_free(&list);
            }
        }
/****************************************************************************/
}

/* notify: the NOTIFY command.  Does the whole ball-o-wax */
/*ARGSUSED*/
void 
notify (char *command, char *args, char *subargs)
{
	char	*nick,
		*list = (char *) 0,
		*ptr;
	int	no_nicks = 1;
	int	do_ison = 0;
 	int	old_server = from_server;
	NotifyList	*new;
/**************************** PATCHED by Flier ******************************/
        char tmpbuf[mybufsize/2];

        *tmpbuf='\0';
/****************************************************************************/
	malloc_strcpy(&list, empty_string);
	while ((nick = next_arg(args, &args)) != NULL)
	{
		no_nicks = 0;
		while (nick)
		{
			if ((ptr = index(nick, ',')) != NULL)
				*ptr++ = '\0';
			if (*nick == '-')
			{
				nick++;
				if (*nick)
				{
					if ((new = (NotifyList *) remove_from_list((List **) &notify_list, nick)) != NULL)
					{
						new_free(&(new->nick));
/**************************** PATCHED by Flier ******************************/
						/*say("%s removed from notification list", nick);*/
                                                new_free(&(new->userhost));
                                                new_free(&(new->mask));
                                                new_free(&(new->group));
                                                if (inSZNotify!=1)
                                                    say("%s removed from notification list",nick);
/****************************************************************************/
						new_free(&new);
					}
					else
						say("%s is not on the notification list", nick);
				}
				else
				{
					while ((new = notify_list))
					{
						notify_list = new->next;
						new_free(&new->nick);
/**************************** PATCHED by Flier ******************************/
                                                new_free(&(new->userhost));
                                                new_free(&(new->mask));
                                                new_free(&(new->group));
/****************************************************************************/
						new_free(&new);
					}
/**************************** PATCHED by Flier ******************************/
					/*say("Notify list cleared");*/
					if (inSZNotify!=1)
                                            say("Notify list cleared");
/****************************************************************************/
				}
			}
			else
			{
				/* compatibility */
				if (*nick == '+')
					nick++;
				if (*nick)
				{
/**************************** PATCHED by Flier ******************************/
                                        char *mask;
                                        char *group;

                                        if ((mask=index(nick,'!'))) *mask++='\0';
                                        if (mask && (group=index(mask,':'))) *group++='\0';
                                        else if ((group=index(nick,':'))) *group++='\0';
/****************************************************************************/
					do_ison = 1;
					if (index(nick, '*'))
						say("Wildcards not allowed in NOTIFY nicknames!");
					else
					{
						if ((new = (NotifyList *) remove_from_list((List **) &notify_list, nick)) != NULL)
						{
							new_free(&(new->nick));
/**************************** PATCHED by Flier ******************************/
                                                        new_free(&(new->userhost));
                                                        new_free(&(new->mask));
                                                        new_free(&(new->group));
/****************************************************************************/
							new_free(&new);
						}
						new = (NotifyList *) new_malloc(sizeof(NotifyList));
						new->nick = (char *) 0;
/**************************** PATCHED by Flier ******************************/
                                                new->isfriend=0;
                                                new->userhost=(char *) 0;
                                                new->mask=(char *) 0;
                                                new->group=(char *) 0;
                                                if (mask) malloc_strcpy(&(new->mask),mask);
                                                if (group) malloc_strcpy(&(new->group),group);
/****************************************************************************/
						malloc_strcpy(&(new->nick), nick);
						new->flag = 0;
						add_to_list((List **) &notify_list, (List *) new);
						from_server = primary_server;
						if (get_server_2_6_2(from_server))
						{
							malloc_strcat(&list, new->nick);
							malloc_strcat(&list, " ");
						}
						else
 							add_to_whois_queue(new->nick, whois_notify, (char *) 0);
/**************************** PATCHED by Flier ******************************/
						/*say("%s added to the notification list", nick);*/
                                                if (inSZNotify!=1)
                                                    say("%s added to the notification list",nick);
                                                else new->flag=2;
                                                if (*tmpbuf) strmcat(tmpbuf," ",sizeof(tmpbuf));
                                                strmcat(tmpbuf,new->nick,sizeof(tmpbuf));
                                                new->server=from_server;
/****************************************************************************/
 						from_server = old_server;
					}
				} else
					show_notify_list(0);
			}
			nick = ptr;
		}
	}
/**************************** PATCHED by Flier ******************************/
 	/*if (do_ison) {
 		from_server = primary_server;
  		add_ison_to_whois(list, ison_notify);
 		from_server = old_server;
 	}*/
        if (do_ison && inSZNotify==1 && *tmpbuf)
            AddDelayNotify(tmpbuf);
        else if (do_ison) {
 		from_server = primary_server;
  		add_ison_to_whois(list, ison_notify);
 		from_server = old_server;
        }
/****************************************************************************/
	new_free(&list);
	if (no_nicks)
		show_notify_list(1);
}

/*
 * do_notify: This simply goes through the notify list, sending out a WHOIS
 * for each person on it.  This uses the fancy whois stuff in whois.c to
 * figure things out.  Look there for more details, if you can figure it out.
 * I wrote it and I can't figure it out.
 *
 * Thank you Michael... leaving me bugs to fix :) Well I fixed them!
 */
void 
do_notify (void)
{
	static	int	location = 0;
	int	count,
 		c2,
 		old_server = from_server;
	char	buf[BIG_BUFFER_SIZE+1];
	NotifyList	*tmp;

	*buf = '\0';
	from_server = primary_server;
	for (tmp = notify_list, c2 = count = 0; tmp; tmp = tmp->next, count++)
	{
/**************************** PATCHED by Flier ******************************/
		/*if (count >= location && count < location + 40)*/
		if (tmp->flag!=2 && count >= location && count < location + 40)
/****************************************************************************/
		{
			c2++;
/**************************** Patched by Flier ******************************/
			/*strcat(buf, " ");
			strcat(buf, tmp->nick);*/
			strmcat(buf, " ", sizeof(buf));
			strmcat(buf, tmp->nick, sizeof(buf));
/****************************************************************************/
		}
	}
	if (c2)
		add_ison_to_whois(buf, ison_notify);
	if ((location += 40) > count)
		location = 0;
 	from_server = old_server;
}

/*
 * notify_mark: This marks a given person on the notify list as either on irc
 * (if flag is 1), or not on irc (if flag is 0).  If the person's status has
 * changed since the last check, a message is displayed to that effect.  If
 * the person is not on the notify list, this call is ignored 
 * doit if passed as 0 means it comes from a join, or a msg, etc, not from
 * an ison reply.  1 is the other..
 * we also bail out if the primary_server != from_server and the from-server
 * is valid.
 *
 * NOTE:  this function should be called with no particular to_window or other
 * variables set that would affect what window it would be displayed in.
 * ideally, a message_from((char *) 0, LOG_CURRENT) should be the what is the
 * current window level.
 */
void 
notify_mark (char *nick, int flag, int doit)
{
	NotifyList	*tmp;
	char	*s = get_string_var(NOTIFY_HANDLER_VAR);
/**************************** PATCHED by Flier ******************************/
        char    tmpbuf[mybufsize/4];
        time_t  timenow=time((time_t *) 0);
/****************************************************************************/

	if (from_server != primary_server && from_server != -1)
		return;
	/* if (!s || (!doit && 'O' == *s)) - this broke notify -gkm *//* old notify */
	if (!doit && 'O' == *s)		/* old notify */
		return;	
	if ('N' == *s)			/* noisy notify */
		doit = 1;
	if ((tmp = (NotifyList *) list_lookup((List **) &notify_list, nick,
			!USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
	{
/**************************** PATCHED by Flier ******************************/
                if (flag==3) {
                    tmp->flag=0;
                    return;
                }
                if (flag!=2 && tmp->flag==2) return;
/****************************************************************************/
		if (flag)
		{
/**************************** PATCHED by Flier ******************************/
                        if (tmp->server!=parsing_server_index) {
                            new_free(&(tmp->userhost));
                            tmp->flag=0;
                            tmp->server=parsing_server_index;
                        }
/****************************************************************************/
			if (tmp->flag != 1)
			{
				if (tmp->flag != -1 && doit
#ifndef LITE
                                    && do_hook(NOTIFY_SIGNON_LIST, "%s", nick)
#endif
                                   )
/**************************** PATCHED by Flier ******************************/
					/*say("Signon by %s detected", nick);*/
					if (flag!=2) HandleNotifyOn(nick,tmp->server);
/****************************************************************************/
				/*
				 * copy the correct case of the nick
				 * into our array  ;)
				 */
				malloc_strcpy(&(tmp->nick), nick);
				malloc_strcpy((char **) &last_notify_nick, nick);
				tmp->flag = 1;
			}
		}
		else
		{
/**************************** PATCHED by Flier ******************************/
			/*if (tmp->flag == 1 && do_hook(NOTIFY_SIGNOFF_LIST, "%s", nick) && doit)
				say("Signoff by %s detected", nick);*/
                        char *host=NULL;

                        if (tmp->userhost) {
                            strmcpy(tmpbuf,tmp->userhost,sizeof(tmpbuf));
                            host=index(tmpbuf,'@');
                            if (host) host++;
                            if (tmp->flag == 1 && host && tmpbuf[0] && doit 
#ifndef LITE
                                && do_hook(NOTIFY_SIGNOFF_UH_LIST, "%s %s %s", nick,tmpbuf,host)
#endif
                               )
                                HandleNotifyOffUh(tmp->nick,tmp->userhost,tmp->mask,
                                                  timenow,tmp->isfriend);
                        }
                        else if (tmp->flag == 1 && doit
#ifndef LITE
                                 && do_hook(NOTIFY_SIGNOFF_LIST, "%s", nick)
#endif
                                )
                            HandleNotifyOff(tmp->nick,timenow);
                        new_free(&tmp->userhost);
/****************************************************************************/
			tmp->flag = 0;
		}
	}
}

void
save_notify(fp)
	FILE	*fp;
{
	NotifyList	*tmp;

	if (notify_list)
	{
		fprintf(fp, "NOTIFY");
		for (tmp = notify_list; tmp; tmp = tmp->next)
			fprintf(fp, " %s", tmp->nick);
		fprintf(fp, "\n");
	}
}

/* I hate broken compilers -mrg */
static	char	*vals[] = { "NOISY", "QUIET", "OLD", (char *) 0 };

void 
set_notify_handler (char *value)
{
 	size_t	len;
	int	i;
	char	*s;

	if (!value)
		value = empty_string;
	for (i = 0, len = strlen(value); (s = vals[i]); i++)
		if (0 == my_strnicmp(value, s, len))
			break;
	set_string_var(NOTIFY_HANDLER_VAR, s);
	return;
}
