/*
 * parse.c: handles messages from the server.   Believe it or not.  I
 * certainly wouldn't if I were you. 
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
 * $Id: parse.c,v 1.60 2002-01-28 16:23:40 f Exp $
 */

#include "irc.h"

#include "server.h"
#include "names.h"
#include "vars.h"
#include "ctcp.h"
#include "hook.h"
#include "edit.h"
#include "ignore.h"
#include "whois.h"
#include "lastlog.h"
#include "ircaux.h"
#include "funny.h"
#include "crypt.h"
#include "ircterm.h"
#include "flood.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "notify.h"

/************************* PATCHED by Flier *************************/
#include "list.h"
#include "status.h"
#include "myvars.h"

extern void OnWho _((char *, char *, char *, char *, char *));
extern void PrintMessage _((char *, char *, char *, int, int));
extern void HandleSplit _((char *, char *, char *, int *));
extern NickList *ChannelJoin _((char *, char *, ChannelList *));
extern NickList *CheckJoin _((char *, char *, char *, int, ChannelList *));
extern struct friends *CheckUsers _((char *, char *));
extern int  HandleJoin _((NickList *, char *, char *, char *, ChannelList *));
extern void HandleInvite _((char *, char *, char *));
extern void HandleKills _((int, char *, char *, char *));
extern void ReconnectOnKill _((int));
extern void HandleNickChange _((char *, char *, char *, int));
#ifdef EXTRAS
extern void CheckLock _((char *, int, ChannelList *));
extern void CheckTopic _((char *, int, ChannelList *));
#endif
extern void HandleMyKick _((char *, char *, char *, char *, char *));
extern int  HandleKick _((char *, char *, char *, char *, char *, int *));
extern void SplitPrint _((char *, char *, char *, int));
extern void ModePrint _((char *, char *, char *, char *, char *, char *));
extern void KickPrint _((char *, char *, char *, char *, char *, int, int));
extern void PrintWho _((char *, char *, char *, char *, char *, char *, char *,
                        int));
extern void PrintPublic _((char *, char *, char *, char *, int, int));
extern void DoKill _((char *, char *, char *));
extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
#if defined(OPERVISION) && defined(WANTANSI)
extern void OVformat _((char *, char *));
#endif
extern void AwaySave _((char *, int));
extern char *GetNetsplitServer _((char *, char *));
extern int  CheckChannel _((char *, char *));
extern int  DecryptMessage _((char *, char *));
extern int  IsIrcNetOperChannel _((char *));
extern int  CompareAddr _((List *, char *));
extern int  AddLast _((List *, List *));
extern char *TimeStamp _((int));
extern void ChannelLogSave _((char *, ChannelList *));

extern void e_nick _((char *, char *, char *));
extern void e_channel _((char *, char *, char *));

#if defined(CELE)
extern struct timeval PingSent;
#else
extern time_t PingSent;
#endif

static int    NumberMessages=0;
static time_t LastMsgTime=0;
static time_t LastNickFlood=0;
/********************************************************************/

#define STRING_CHANNEL '+'
#define MULTI_CHANNEL '#'
#define LOCAL_CHANNEL '&'
#define SAFE_CHANNEL '!'

#define	MAXPARA	15	/* Taken from the ircd */

static	void	BreakArgs _((char *, char **, char **));
static	void	p_linreply _((char **));
static	void	p_ping _((char **));
static	void	p_topic _((char *, char **));
static	void	p_wall _((char *, char **));
static	void	p_wallops _((char *, char **));
static	void	p_privmsg _((char *, char **));
/*static	void	p_msgcmd _((char *, char **));*/
static	void	p_quit _((char *, char **));
static	void	p_pong _((char *, char **));
static	void	p_error _((char *, char **));
static	void	p_channel _((char *, char **));
static	void	p_invite _((char *, char **));
static	void	p_server_kill _((char *, char **));
static	void	p_nick _((char *, char **));
static	void	p_mode _((char *, char **));
static	void	p_kick _((char *, char **));
static	void	p_part _((char *, char **));

/*
 * joined_nick: the nickname of the last person who joined the current
 * channel 
 */
	u_char	*joined_nick = (u_char *) 0;

/* public_nick: nick of the last person to send a message to your channel */
	u_char	*public_nick = (u_char *) 0;

/* User and host information from server 2.7 */
	char	*FromUserHost = (char *) 0;

/* doing a PRIVMSG */
	int	doing_privmsg = 0;

/*
 * is_channel: determines if the argument is a channel.  If it's a number,
 * begins with MULTI_CHANNEL and has no '*', or STRING_CHANNEL, then its a
 * channel 
 */
int
is_channel(to)
 	char	*to;
{
	int	version;

 	if (to == 0)
 		return (0);
	version = get_server_version(from_server);
       	return ((version < Server2_7 && (isdigit(*to) || (*to == STRING_CHANNEL) || *to == '-'))
		|| (version > Server2_5 && *to == MULTI_CHANNEL)
		|| (version > Server2_7 && *to == LOCAL_CHANNEL)
 		|| (version > Server2_8 && *to == STRING_CHANNEL)
 		|| (version > Server2_9 && *to == SAFE_CHANNEL));
}


char	*
PasteArgs(Args, StartPoint)
	char	**Args;
	int	StartPoint;
{
	int	i;

	for (; StartPoint; Args++, StartPoint--)
		if (!*Args)
			return (char *) 0;
	for (i = 0; Args[i] && Args[i+1]; i++)
		Args[i][strlen(Args[i])] = ' ';
	Args[1] = (char *) 0;
	return Args[0];
}

/*
 * BreakArgs: breaks up the line from the server, in to where its from,
 * setting FromUserHost if it should be, and returns all the arguements
 * that are there.   Re-written by phone, dec 1992.
 */
static	void
BreakArgs(Input, Sender, OutPut)
	char	*Input;
	char	**Sender;
	char	**OutPut;
{
	char	*s = Input,
		*t;
	int	ArgCount = 0;

	/*
	 * Get sender from :sender and user@host if :nick!user@host
	 */
	FromUserHost = (char *) 0;

	if (*Input == ':')
	{
		char	*tmp;
		*Input++ = '\0';
		if ((s = (char *) index(Input, ' ')) != (char *) 0)
			*s++ = '\0';
		*Sender = Input;
		if ((tmp = (char *) index(*Sender, '!')) != (char *) 0)
		{
			*tmp++ = '\0';
			FromUserHost = tmp;
		}
	}
	else
		*Sender = empty_string;

	if (!s)
		return;

	for (;;)
	{
		while (*s == ' ')
			*s++ = '\0';

		if (!*s)
			break;

		if (*s == ':')
		{
			for (t = s; *t; t++)
				*t = *(t + 1);
			OutPut[ArgCount++] = s;
			break;
		}
		OutPut[ArgCount++] = s;
		if (ArgCount >= MAXPARA)
			break;

		for (; *s != ' ' && *s; s++)
			;
	}
	OutPut[ArgCount] = (char *) 0;
}

/* beep_em: Not hard to figure this one out */
void
beep_em(beeps)
	int	beeps;
{
	int	cnt,
		i;

	for (cnt = beeps, i = 0; i < cnt; i++)
		term_beep();
}

/* in response to a TOPIC message from the server */
static	void
p_topic(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag;
/**************************** PATCHED by Flier ******************************/
        time_t  timenow = time(NULL);
        ChannelList *chan;
/****************************************************************************/				

        if (!from)
		return;
/**************************** PATCHED by Flier ******************************/
        if (ArgList[0] && ArgList[1] && (chan = lookup_channel(ArgList[0], parsing_server_index, 0))) {
            char tmpbuf[mybufsize];

            chan->topic++;
            if (*ArgList[1]) malloc_strcpy(&(chan->topicstr), ArgList[1]);
            else new_free(&(chan->topicstr));
            malloc_strcpy(&(chan->topicwho), from);
            chan->topicwhen = timenow;
#ifdef EXTRAS
            CheckTopic(chan->channel, parsing_server_index, chan);
#endif
            if (chan->ChanLog) {
                snprintf(tmpbuf, sizeof(tmpbuf), "%s has changed the topic on channel %s to %s", from, ArgList[0], ArgList[1]);
                ChannelLogSave(tmpbuf, chan);
            }
        }
        if ((double_ignore(ArgList[0], NULL, IGNORE_CRAP)) == IGNORED) return;
/****************************************************************************/				
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	if (flag == IGNORED)
		return;

 	save_message_from();
	if (!ArgList[1])
	{
		message_from((char *) 0, LOG_CRAP);
		if (do_hook(TOPIC_LIST, "%s * %s", from, ArgList[0]))
/**************************** PATCHED by Flier ******************************/
			/*say("%s has changed the topic to %s", from, ArgList[0]);*/
                {
                        if (*ArgList[0])
                            say("%s has set the topic to %s", from, ArgList[0]);
                        else
                            say("%s has unset the topic", from);
                }
/****************************************************************************/				
	}
	else
	{
		message_from(ArgList[0], LOG_CRAP);
		if (do_hook(TOPIC_LIST, "%s %s %s", from, ArgList[0], ArgList[1]))
/**************************** PATCHED by Flier ******************************/
			/*say("%s has changed the topic on channel %s to %s",
				from, ArgList[0], ArgList[1]);*/
                {
                        if (*ArgList[1])
                            say("%s has set the topic on channel %s to %s",
                                    from, ArgList[0], ArgList[1]);
                        else
                            say("%s has unset the topic on channel %s",
                                    from, ArgList[0]);
                }
/****************************************************************************/
	}
/**************************** PATCHED by Flier ******************************/
        update_all_status();
/****************************************************************************/
 	restore_message_from();
}

static	void
p_linreply(ArgList)
	char	**ArgList;
{
	PasteArgs(ArgList, 0);
	say("%s", ArgList[0]);
}

static	void
p_wall(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag,
		level;
	char	*line;
	char	*high;

        if (!from)
		return;
	PasteArgs(ArgList, 0);
	if (!(line = ArgList[0]))
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_WALLS);
 	save_message_from();
	message_from(from, LOG_WALL);
	if (flag != IGNORED)
	{
		if (flag == HIGHLIGHTED)
			high = &highlight_char;
		else
			high = empty_string;
		if ((flag != DONT_IGNORE) && (ignore_usernames & IGNORE_WALLS)
				&& !FromUserHost)
			add_to_whois_queue(from, whois_ignore_walls, "%s",line);
		else
		{
			level = set_lastlog_msg_level(LOG_WALL);
			if (check_flooding(from, WALL_FLOOD, line) &&
					do_hook(WALL_LIST, "%s %s", from, line))
				put_it("%s#%s#%s %s", high, from, high, line);
			if (beep_on_level & LOG_WALL)
				beep_em(1);
			set_lastlog_msg_level(level);
		}
	}
 	restore_message_from();
}

static	void
p_wallops(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag, level;
	char	*line;

	if (!from)
		return;
	if (!(line = PasteArgs(ArgList, 0)))
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_WALLOPS);
	level = set_lastlog_msg_level(LOG_WALLOP);
 	save_message_from();
	message_from(from, LOG_WALLOP);
	if (index(from, '.'))
	{
		char	*high;

		if (flag != IGNORED)
		{
			if (flag == HIGHLIGHTED)
				high = &highlight_char;
			else
				high = empty_string;
			if (do_hook(WALLOP_LIST, "%s S %s", from, line))
/**************************** PATCHED by Flier ******************************/
				/*put_it("%s!%s!%s %s", high, from, high, line);*/
                        {
			    char *stampbuf = TimeStamp(2);
#if defined(OPERVISION) && defined(WANTANSI)
                            if (OperV) OVformat(line, from);
                            else
#endif
                            put_it("%s%s!%s!%s %s", stampbuf, high, from, high, line);
                        }
/****************************************************************************/
			if (beep_on_level & LOG_WALLOP)
				beep_em(1);
		}
	}
	else
	{
		if (get_int_var(USER_WALLOPS_VAR))
		{
			if ((flag != DONT_IGNORE) && (check_flooding(from, WALLOP_FLOOD, line)))
			add_to_whois_queue(from, whois_new_wallops, "%s", line);
		}
/**************************** PATCHED by Flier ******************************/
                /* Why not show user their own wallops ? I will ignore this */
		/*else if (strcmp(from, get_server_nickname(get_window_server(0))) != 0)
			put_it("!%s! %s", from, line);*/
                else {
		    char *stampbuf = TimeStamp(2);
#if defined(OPERVISION) && defined(WANTANSI)
                    if (OperV) OVformat(line,from);
                    else
#endif
                    put_it("%s!%s! %s", stampbuf, from, line);
                }
/****************************************************************************/
	}
	set_lastlog_msg_level(level);
 	restore_message_from();
}

/*ARGSUSED*/
void
whoreply(from, ArgList)
	char	**ArgList,
		*from;
{
	static	char	format[40];
	static	int	last_width = -1;
	int	ok = 1;
	char	*channel,
		*user,
		*host,
		*server,
		*nick,
 		*status,
		*name;
	int	i;
/**************************** PATCHED by Flier ******************************/
        int     show_server = 0;
/****************************************************************************/

	FILE	*fip;
	char	buf_data[BUFSIZ];

	if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR))
	{
		if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
		    snprintf(format, sizeof format, "%%-%u.%us %%-9s %%-3s %%s@%%s (%%s)",
					(unsigned char) last_width,
					(unsigned char) last_width);
		else
		    strcpy(format, "%s\t%-9s %-3s %s@%s (%s)");
	}
	i = 0;
 	channel = user = host = server = nick = status = name = empty_string;
	if (ArgList[i])
		channel = ArgList[i++];
	if (ArgList[i])
		user = ArgList[i++];
	if (ArgList[i])
		host = ArgList[i++];
	if (ArgList[i])
		server = ArgList[i++];
	if (ArgList[i])
		nick = ArgList[i++];
	if (ArgList[i])
 		status = ArgList[i++];
	PasteArgs(ArgList, i);

 	if (*status == 'S')	/* this only true for the header WHOREPLY */
	{
		channel = "Channel";
		if (((who_mask & WHO_FILE) == 0) || (fopen (who_file, "r")))
		{
			if (do_hook(WHO_LIST, "%s %s %s %s %s %s", channel,
 					nick, status, user, host, ArgList[6]))
 				put_it(format, channel, nick, status, user,
					host, ArgList[6]);
			return;
		}
	}

	if (ArgList[i])
		name = ArgList[i];

/**************************** PATCHED by Flier ******************************/
        if (who_mask & WHO_SHOW_SERVER) {
            show_server = 1;
            who_mask &= ~WHO_SHOW_SERVER;
        }
/****************************************************************************/
	if (who_mask)
	{
		if (who_mask & WHO_HERE)
			ok = ok && (*status == 'H');
		if (who_mask & WHO_AWAY)
			ok = ok && (*status == 'G');
		if (who_mask & WHO_OPS)
			ok = ok && (*(status + 1) == '*');
		if (who_mask & WHO_LUSERS)
			ok = ok && (*(status + 1) != '*');
		if (who_mask & WHO_CHOPS)
			ok = ok && ((*(status + 1) == '@') ||
			(*(status + 2) == '@'));
		if (who_mask & WHO_HOPS)
			ok = ok && (*(status + 1) == '%');
		if (who_mask & WHO_NAME)
			ok = ok && wild_match(who_name, user);
		if (who_mask & WHO_NICK)
			ok = ok && wild_match(who_nick, nick);
		if (who_mask & WHO_HOST)
			ok = ok && wild_match(who_host, host);
		if (who_mask & WHO_REAL)
			ok = ok && wild_match(who_real, name);
		if (who_mask & WHO_SERVER)
			ok = ok && wild_match(who_server, server);
		if (who_mask & WHO_FILE)
		{
			ok = 0;
			cannot_open = (char *) 0;
			if ((fip = fopen (who_file, "r")) != (FILE *) 0)
			{
				while (fgets (buf_data, BUFSIZ, fip) !=
								(char *) 0)
				{
					buf_data[strlen(buf_data)-1] = '\0';
					ok = ok || wild_match(buf_data, nick);
				}
				fclose (fip);
			} else
				cannot_open = who_file;
		}
	}
	if (ok)
	{
/**************************** PATCHED by Flier ******************************/
                if (server_list[from_server].SZWho) {
#ifdef OPER
                    if (inSZFKill) DoKill(nick, user, host);
                    else OnWho(nick, user, host, channel, status);
#else
                    OnWho(nick, user, host, channel, status);
#endif
                }
                else
/****************************************************************************/				
		if (do_hook(WHO_LIST, "%s %s %s %s %s %s", channel, nick,
 				status, user, host, name))
		{
			if (get_int_var(SHOW_WHO_HOPCOUNT_VAR))
/**************************** PATCHED by Flier ******************************/
 				/*put_it(format, channel, nick, status, user, host,
					name);*/
                                PrintWho(channel, nick, status, user, host, name,
                                         server, show_server);
/****************************************************************************/				
			else
			{
				char	*tmp;

				if ((tmp = (char *) index(name, ' ')) !=
								(char *) 0)
					tmp++;
				else
					tmp = name;
/**************************** PATCHED by Flier ******************************/
				/*put_it(format, channel, nick, status, user, host,
					tmp);*/
                                PrintWho(channel, nick, status, user, host, tmp,
                                         server, show_server);
/****************************************************************************/				
			}
		}
	}
/**************************** PATCHED by Flier ******************************/
        if (show_server) who_mask |= WHO_SHOW_SERVER;
/****************************************************************************/
}

static	void
p_privmsg(from, Args)
	char	*from,
		**Args;
{
	int	level,
		flag,
		list_type,
		flood_type,
		log_type;
	u_char	ignore_type;
	char	*ptr,
		*to;
	char	*high;
	int	no_flood;
/**************************** PATCHED by Flier ******************************/
        int     iscrypted;
        char    tmpbuf[mybufsize / 8];
        time_t  timenow = time(NULL);
        struct  friends *tmpfriend;
/****************************************************************************/

        if (!from)
		return;
	PasteArgs(Args, 1);
	to = Args[0];
	ptr = Args[1];
	if (!to || !ptr)
		return;
 	save_message_from();
	if (is_channel(to))
	{
		message_from(to, LOG_MSG);
		malloc_strcpy((char **) &public_nick, from);
		if (!is_on_channel(to, parsing_server_index, from))
		{
			log_type = LOG_PUBLIC;
			ignore_type = IGNORE_PUBLIC;
			list_type = PUBLIC_MSG_LIST;
			flood_type = PUBLIC_FLOOD;
		}
		else
		{
			log_type = LOG_PUBLIC;
			ignore_type = IGNORE_PUBLIC;
			if (is_current_channel(to, parsing_server_index, 0))
				list_type = PUBLIC_LIST;
			else
				list_type = PUBLIC_OTHER_LIST;
			flood_type = PUBLIC_FLOOD;
		}
	}
	else
	{
		message_from(from, LOG_MSG);
		flood_type = MSG_FLOOD;
		if (my_stricmp(to, get_server_nickname(parsing_server_index)))
		{
			log_type = LOG_WALL;
			ignore_type = IGNORE_WALLS;
			list_type = MSG_GROUP_LIST;
		}
		else
		{
			log_type = LOG_MSG;
			ignore_type = IGNORE_MSGS;
			list_type = MSG_LIST;
		}
	}
	flag = double_ignore(from, FromUserHost, ignore_type);
	switch (flag)
	{
	case IGNORED:
		if ((list_type == MSG_LIST) && get_int_var(SEND_IGNORE_MSG_VAR))
			send_to_server("NOTICE %s :%s is ignoring you", from,
					get_server_nickname(parsing_server_index));
		goto out;
	case HIGHLIGHTED:
		high = &highlight_char;
		break;
	default:
		high = empty_string;
		break;
	}
/**************************** Patched by Flier ******************************/
        if (ignore_type == IGNORE_PUBLIC && double_ignore(to, NULL, IGNORE_PUBLIC) == IGNORED)
            goto out;
        if (ignore_type == IGNORE_PUBLIC && is_channel(to) && double_ignore(from, to, IGNORE_PUBLIC) == IGNORED)
            goto out;
        if (FloodProt > 1 && flood_type == MSG_FLOOD) {
            snprintf(tmpbuf, sizeof(tmpbuf), "%s!%s", from, FromUserHost);
            tmpfriend = CheckUsers(tmpbuf, NULL);
            if (!tmpfriend || (tmpfriend && !(tmpfriend->privs) & FLNOFLOOD)) {
                if (timenow > LastMsgTime + FloodSeconds) {
                    LastMsgTime = timenow;
                    NumberMessages = 1;
                }
                else NumberMessages++;
                if (NumberMessages >= FloodMessages && timenow >= LastNickFlood + 10) {
                    LastMsgTime = timenow;
                    LastNickFlood = timenow;
                    NumberMessages = 0;
                    snprintf(tmpbuf,sizeof(tmpbuf), "%.7s%c%c", get_server_nickname(from_server),
                           'a' + (rand() % 25) ,'a' + (rand() % 25));
                    e_nick(NULL, tmpbuf, NULL);
#ifdef WANTANSI
                    snprintf(tmpbuf,sizeof(tmpbuf), "%sFlood attack%s detected, changing nick",
                            CmdsColors[COLWARNING].color1, Colors[COLOFF]);
#else
                    snprintf(tmpbuf,sizeof(tmpbuf), "%cFlood attack%c detected, changing nick", bold, bold);
#endif
                    say("%s", tmpbuf);
                    if (away_set || LogOn) AwaySave(tmpbuf, SAVEFLOOD);
                }
            }
        }
/****************************************************************************/
	level = set_lastlog_msg_level(log_type);
	ptr = do_ctcp(from, to, ptr);
	if (!ptr || !*ptr)
		goto out;
	if ((flag != DONT_IGNORE) && (ignore_usernames & ignore_type) && !FromUserHost)
		add_to_whois_queue(from, whois_ignore_msgs, "%s", ptr);
	else
	{
		no_flood = check_flooding(from, flood_type, ptr);
 		if (sed == 0 || do_hook(ENCRYPTED_PRIVMSG_LIST,"%s %s %s",from, to, ptr))
		{
		switch (list_type)
		{
		case PUBLIC_MSG_LIST:
			if (no_flood && do_hook(list_type, "%s %s %s", from, to, ptr))
/**************************** Patched by Flier ******************************/
                        {
			    /*put_it("%s(%s/%s)%s %s", high, from, to, high, ptr);*/
                            char *stampbuf = TimeStamp(2);

			    put_it("%s%s(%s/%s)%s %s", stampbuf, high, from, to, high, ptr);
                        }
/****************************************************************************/
			break;
		case MSG_GROUP_LIST:
/**************************** Patched by Flier ******************************/
                        if (*to == '@' && is_channel(to + 1)) message_from(to+1, LOG_MSG);
/****************************************************************************/
			if (no_flood && do_hook(list_type, "%s %s %s", from, to, ptr))
/**************************** Patched by Flier ******************************/
                        {
			    /*put_it("%s-%s:%s-%s %s", high, from, to, high, ptr);*/
                            char *stampbuf = TimeStamp(2);

			    put_it("%s-%s:%s- %s", stampbuf, from, to, ptr);
                        }
/****************************************************************************/
			break;
		case MSG_LIST:
			if (!no_flood)
				break;
			malloc_strcpy((char **) &recv_nick, from);
			if (away_set)
				beep_em(get_int_var(BEEP_WHEN_AWAY_VAR));
/**************************** PATCHED by Flier ******************************/
                        iscrypted = DecryptMessage(ptr, from);
/****************************************************************************/
			if (do_hook(list_type, "%s %s", from, ptr))
			{
/**************************** PATCHED by Flier ******************************/
			    /*if (away_set)
			    {
				time_t t;
				char *msg = (char *) 0;
				size_t len = my_strlen(ptr) + 20;

				t = time((time_t *) 0);
				msg = (char *) new_malloc(len);
				snprintf(msg, len, "%s <%.16s>", ptr, ctime(&t));
				put_it("%s*%s*%s %s", high, from, high, msg);
				new_free(&msg);
			    }
			    else
				put_it("%s*%s*%s %s", high, from, high, ptr);*/
                            PrintMessage(from, FromUserHost, ptr, 1, iscrypted);
/****************************************************************************/
			}
/**************************** PATCHED by Flier ******************************/
                        else PrintMessage(from, FromUserHost, ptr, 0, iscrypted);
/****************************************************************************/
			break;
		case PUBLIC_LIST:
			if (get_int_var(MAKE_NOTICE_MSG_VAR))
				doing_privmsg = 1;
/**************************** PATCHED by Flier ******************************/
                        iscrypted = DecryptMessage(ptr, to);
/****************************************************************************/
			if (no_flood && do_hook(list_type, "%s %s %s", from, 
			    to, ptr))
/**************************** PATCHED by Flier ******************************/
				/*put_it("%s<%s>%s %s", high, from, high, ptr);*/
                                PrintPublic(from, NULL, to, ptr, 1, iscrypted);
                        else PrintPublic(from, NULL, to, ptr, 0, iscrypted);
/****************************************************************************/
			doing_privmsg = 0;
			break;
		case PUBLIC_OTHER_LIST:
			if (get_int_var(MAKE_NOTICE_MSG_VAR))
				doing_privmsg = 1;
/**************************** PATCHED by Flier ******************************/
                        iscrypted = DecryptMessage(ptr, to);
/****************************************************************************/
			if (no_flood && do_hook(list_type, "%s %s %s", from,
			    to, ptr))
/**************************** PATCHED by Flier ******************************/
				/*put_it("%s<%s:%s>%s %s", high, from, to, high,
					ptr);*/
                                PrintPublic(from, ":", to, ptr, 1, iscrypted);
                        else PrintPublic(from, ":", to, ptr, 0, iscrypted);
/****************************************************************************/
			doing_privmsg = 0;
			break;
		}
		if (beep_on_level & log_type)
			beep_em(1);
		}
	}
	set_lastlog_msg_level(level);
out:
 	restore_message_from();
}

#if 0
static	void
p_msgcmd(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*high,
		*channel,
		*text;
	int	log_type,
		no_flooding;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_PUBLIC);
	switch (flag)
	{
	case IGNORED:
		return;
	case HIGHLIGHTED:
		high = &highlight_char;
		break;
	default:
		high = empty_string;
		break;
	}
	if ((channel = real_channel()) == (char *) 0)
		return;
	text = do_ctcp(from, channel, ArgList[0]);
	if (!text || !*text)
		return;
	malloc_strcpy((char **) &public_nick, from);
	log_type = set_lastlog_msg_level(LOG_PUBLIC);
	no_flooding = check_flooding(from, PUBLIC_FLOOD, text);
 	save_message_from();
        message_from(channel, LOG_PUBLIC);
	if (is_current_channel(channel, parsing_server_index, 0))
	{
		if (get_int_var(MAKE_NOTICE_MSG_VAR))
				doing_privmsg = 1;
		if (no_flooding && do_hook(PUBLIC_LIST, "%s %s %s", from, channel, text))
			put_it("%s<%s>%s %s", high, from, high, text);
		doing_privmsg = 0;
	}
	else
	{
		if (get_int_var(MAKE_NOTICE_MSG_VAR))
				doing_privmsg = 1;
		if (no_flooding && do_hook(PUBLIC_OTHER_LIST, "%s %s %s", from,
				channel, text))
			put_it("%s<%s:%s>%s %s", high, from, channel, high,
				text);
		doing_privmsg = 0;
	}
 	restore_message_from();
	if (beep_on_level & LOG_PUBLIC)
		beep_em(1);
	set_lastlog_msg_level(log_type);
}
#endif

/*ARGSUSED*/
static	void
p_quit(from, ArgList)
	char	*from,
		**ArgList;
{
	int	one_prints = 0;
	char	*chan;
	char	*Reason;
	int	flag;
/**************************** PATCHED by Flier ******************************/
        int     netsplit;
#ifdef WANTANSI
        char    *colnick;
#endif
        NickList *joiner;
#ifdef EXTRAS
        typedef struct winlist_str Winlist;
        struct winlist_str {
            Winlist *next;
            Window *window;
        };
        Winlist *winlist=NULL,*winelem;
#endif
/****************************************************************************/

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
 	save_message_from();
	if (flag != IGNORED)
	{
		PasteArgs(ArgList, 0);
		Reason = ArgList[0] ? ArgList[0] : "?";
		for (chan = walk_channels(from, 1, parsing_server_index); chan; chan = walk_channels(from, 0, -1))
		{
			message_from(chan, LOG_CRAP);
/**************************** PATCHED by Flier ******************************/
			/*if (do_hook(CHANNEL_SIGNOFF_LIST, "%s %s %s", chan, from, Reason))
				one_prints = 1;*/
                        HandleSplit(Reason,from,chan,&netsplit);
                        if (do_hook(CHANNEL_SIGNOFF_LIST, "%s %s %s", chan, from,Reason)) {
                            SplitPrint(Reason,from,chan,netsplit);
                            one_prints = 1;
                        }
/****************************************************************************/
		}
		if (one_prints)
		{
			message_from(what_channel(from, parsing_server_index), LOG_CRAP);
			if (do_hook(SIGNOFF_LIST, "%s %s", from, Reason))
/**************************** PATCHED by Flier *****************************/
				/*say("Signoff: %s (%s)", from, Reason);*/
                            if (!netsplit || !NHDisp) {
#ifdef EXTRAS
                                int showed=0;
                                char chanbuf[mybufsize/4+1];
                                Window *oldto_window;
                                ChannelList *chan;

                                *chanbuf='\0';
                                oldto_window=to_window;
                                chan=server_list[parsing_server_index].chan_list;
                                for (;chan;chan=chan->next) {
                                    joiner=CheckJoiners(from,chan->channel,parsing_server_index,chan);
                                    /* only if user was on a channel */
                                    if (ShowSignAllChan && joiner) {
                                        /* major pain in the ass: if we have several channels in
                                         * one window we have to display signoff only once in
                                         * that window so we have to know which windows have
                                         * already displayed signoff - we use window pointer */
                                        if (!list_lookup_ext((List **) &winlist,(char *) chan->window,
                                                    !USE_WILDCARDS,!REMOVE_FROM_LIST,CompareAddr)) {
                                            winelem=(Winlist *) malloc(sizeof(Winlist));
                                            winelem->window=chan->window;
                                            winelem->next=NULL;
                                            add_to_list_ext((List **) &winlist,(List *) winelem,
                                                    (int (*) _((List *, List *))) AddLast);
                                        }
                                    }
                                    if (ShowSignoffChan && SignoffChannels && *SignoffChannels) {
                                        if (!CheckChannel(SignoffChannels,chan->channel))
                                            continue;
                                        if (strlen(chan->channel)+strlen(chanbuf)>=sizeof(chanbuf)-1)
                                            continue;
                                        if (joiner) {
                                            if (*chanbuf) strmcat(chanbuf,",",sizeof(chanbuf));
                                            else strcpy(chanbuf,"(");
                                            strmcat(chanbuf,chan->channel,sizeof(chanbuf));
                                            chanbuf[sizeof(chanbuf)-1]='\0';
                                        }
                                    }
                                }
                                if (ShowSignoffChan && SignoffChannels && *SignoffChannels) {
                                    if (strlen(chanbuf)>=sizeof(chanbuf)-3) {
                                        chanbuf[sizeof(chanbuf)-3]=')';
                                        chanbuf[sizeof(chanbuf)-2]=' ';
                                        chanbuf[sizeof(chanbuf)-1]='\0';
                                    }
                                    else strmcat(chanbuf,") ",sizeof(chanbuf));
                                }
                                for (winelem=winlist;;) {
                                    if (winelem) to_window=winelem->window;
                                    else if (showed) break;
                                    showed=1;
#endif /* EXTRAS */
#ifdef WANTANSI
                                    joiner=CheckJoiners(from,NULL,parsing_server_index,NULL);
                                    if (joiner && joiner->shitlist && joiner->shitlist->shit)
                                        colnick=CmdsColors[COLLEAVE].color5;
                                    else if (joiner && joiner->frlist && joiner->frlist->privs)
                                        colnick=CmdsColors[COLLEAVE].color4;
                                    else colnick=CmdsColors[COLLEAVE].color1;
#ifdef EXTRAS
                                    say("Signoff: %s%s%s (%s%s%s%s)",
#else  /* EXTRAS */
                                    say("Signoff: %s%s%s (%s%s%s)",
#endif /* EXTRAS */
                                        colnick,from,Colors[COLOFF],
#ifdef EXTRAS
                                        chanbuf,
#endif /* EXTRAS */
                                        CmdsColors[COLLEAVE].color3,Reason,Colors[COLOFF]);
#else  /* WANTANSI */
#ifdef EXTRAS
                                    say("Signoff: %s (%s%s)",from,chanbuf,Reason);
#else  /* EXTRAS */
                                    say("Signoff: %s (%s)",from,Reason);
#endif /* EXTRAS */
#endif /* WANTANSI */
#ifdef EXTRAS
                                    if (!winelem) break;
                                    else winelem=winelem->next;
                                }
                                to_window=oldto_window;
                                if (ShowSignAllChan && winlist) {
                                    /* free stored info about windows */
                                    while (winlist) {
                                        winelem=winlist;
                                        winlist=winlist->next;
                                        new_free(&winelem);
                                    }
                                }
#endif /* EXTRAS */
                            }
/***************************************************************************/
		}
	}
	message_from((char *) 0, LOG_CURRENT);
	remove_from_channel((char *) 0, from, parsing_server_index);
	notify_mark(from, 0, 0);
 	restore_message_from();
}

/*ARGSUSED*/
static	void
p_pong(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag;
/**************************** PATCHED by Flier ******************************/
        char tmpbuf[mybufsize/32];
        struct timeval timenow;
        struct spingstr *spingtmp;
/****************************************************************************/

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	if (flag == IGNORED)
		return;

/**************************** PATCHED by Flier ******************************/
	/*if (ArgList[0])
		say("%s: PONG received from %s", ArgList[0], from);*/
        if (ArgList[0]) {
            if (ArgList[1] &&
                (index(ArgList[0],'.') || !strcmp(ArgList[0],get_server_itsname(parsing_server_index)))) {
                if ((spingtmp=(struct spingstr *) list_lookup((List **) &spinglist,
                                                              ArgList[0],!USE_WILDCARDS,
                                                              REMOVE_FROM_LIST))) {
                    gettimeofday(&timenow,NULL);
                    timenow.tv_sec-=spingtmp->sec;
                    if (timenow.tv_usec>=spingtmp->usec)
                        timenow.tv_usec-=spingtmp->usec;
                    else {
                        timenow.tv_usec=timenow.tv_usec-spingtmp->usec+1000000;
                        timenow.tv_sec--;
                    }
#ifdef WANTANSI
                    snprintf(tmpbuf,sizeof(tmpbuf),"%06ld",timenow.tv_usec);
                    tmpbuf[3]='\0';
                    say("Server pong from %s%s%s received in %s%ld.%s%s seconds",
                        CmdsColors[COLCSCAN].color1,ArgList[0],Colors[COLOFF],
                        CmdsColors[COLCSCAN].color2,timenow.tv_sec,tmpbuf,Colors[COLOFF]);
#else  /* WANTANSI */
                    say("Server pong from %s received in %ld.%s seconds",ArgList[0],
                        timenow.tv_sec,tmpbuf);
#endif /* WANTANSI */
                    new_free(&(spingtmp->servername));
                    new_free(&spingtmp);
                    if (!my_stricmp(server_list[parsing_server_index].itsname,ArgList[0])) {
#if defined(CELE)
                        LagTimer=timenow;
#else
                        LagTimer=timenow.tv_sec;
#endif
                        update_all_status();
                    }
                }
                else if (!strcmp(ArgList[1],"szlagmeter") ||
                         !strcmp(ArgList[0],get_server_itsname(parsing_server_index))) {
#if defined(CELE)
                    gettimeofday(&LagTimer,NULL);
                    LagTimer.tv_sec-=PingSent.tv_sec;
                    if (LagTimer.tv_usec>=PingSent.tv_usec)
                        LagTimer.tv_usec-=PingSent.tv_usec;
                    else {
                        LagTimer.tv_usec=LagTimer.tv_usec-PingSent.tv_usec+1000000;
                        LagTimer.tv_sec--;
                    }
                    update_all_status();
#else
                    LagTimer=time((time_t *) 0)-PingSent;
#endif
                    update_all_status();
                }
                else say("%s: PONG received from %s", ArgList[0], from);
            }
            else say("%s: PONG received from %s", ArgList[0], from);
        }
/****************************************************************************/
}

/*ARGSUSED*/
static	void
p_error(from, ArgList)
	char	*from,
		**ArgList;
{
	PasteArgs(ArgList, 0);
	if (!ArgList[0])
		return;
	say("%s", ArgList[0]);
}

static	void
p_channel(from, ArgList)
	char	*from;
	char	**ArgList;
{
	int	join;
	char	*channel;
	int	flag;
 	char	*s, *ov = NULL;
	int	chan_oper = 0, chan_voice = 0, chan_halfop = 0;
/**************************** PATCHED by Flier ******************************/
        int     donelj = 0;
        char    tmpbuf[mybufsize + 1];
        NickList *joiner = NULL;
        ChannelList *chan = NULL;
/****************************************************************************/

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
        if (strcmp(ArgList[0], zero))
	{
		join = 1;
		channel = ArgList[0];
		/*
		 * this \007 should be \a but a lot of compilers are
		 * broken.  *sigh*  -mrg
		 */
		if ((ov = s = index(channel, '\007')))
		{
			*s = '\0';
			ov++;
			while (*++s)
			{
				if (*s == 'o')
					chan_oper = 1;
				if (*s == 'h')
					chan_halfop = 1;
				if (*s == 'v')
					chan_voice = 1;

			}
		}
		malloc_strcpy((char **) &joined_nick, from);
	}
	else
	{
		channel = zero;
		join = 0;
        }
	if (!my_stricmp(from, get_server_nickname(parsing_server_index)))
	{
		if (join)
		{
			add_channel(channel, parsing_server_index, CHAN_JOINED, (ChannelList *) 0);
/***************************** PATCHED by Flier **************************/	
			/*send_to_server("MODE %s", channel);*/
                        if ((get_server_version(from_server) == Server2_9 || 
                             get_server_version(from_server) == Server2_10) &&
                            IsIrcNetOperChannel(channel)) {
                            snprintf(tmpbuf,sizeof(tmpbuf), "MODE %s", channel);
                        }
                        else {
                            snprintf(tmpbuf,sizeof(tmpbuf), "WHO %s", channel);
                            if (*channel != '+') {
                                snprintf(&tmpbuf[strlen(tmpbuf)], sizeof(tmpbuf),
                                        "\r\nMODE %s\r\nMODE %s e\r\nMODE %s b",
                                        channel, channel, channel);
                            }
                        }
                        send_to_server("%s", tmpbuf);
/*************************************************************************/
			if (get_server_version(parsing_server_index) == Server2_5)
				send_to_server("NAMES %s", channel);
/***************************** PATCHED by Flier **************************/	
                        chan = add_to_channel(channel, from, parsing_server_index, chan_oper,
                                              chan_halfop, chan_voice, FromUserHost, NULL);
                        if (*channel == '+') chan->gotbans = 1;
                        joiner = ChannelJoin(from, channel, chan);
                        donelj = 1;
                        gettimeofday(&(chan->time), NULL);
/*************************************************************************/	
                }
                else
			remove_channel(channel, parsing_server_index);
	}
	else
	{
/************************* PATCHED by Flier ***************************/
		/*if (join)
			add_to_channel(channel, from, parsing_server_index, chan_oper, chan_voice);*/
#ifdef WANTANSI
                char tmpbuf1[mybufsize];
#endif
                char tmpbuf2[mybufsize];

                if (join) {
                        chan = add_to_channel(channel, from, parsing_server_index, chan_oper,
                                              chan_halfop, chan_voice, FromUserHost, NULL);
                        joiner = CheckJoin(from, FromUserHost, channel, parsing_server_index, chan);
                        if (chan && (chan->status & CHAN_CHOP) && chan_oper && chan->NHProt) {
                            if (!(joiner && joiner->frlist && joiner->frlist->privs))
                                send_to_server("MODE %s -o %s", channel, from);
                        }
                        if (chan_oper && NHDisp == 2) {
                            save_message_from();
                            message_from(channel, LOG_CRAP);
#ifdef WANTANSI
                            snprintf(tmpbuf1, sizeof(tmpbuf1), "[%s%s%s] on %s%s%s",
                                     CmdsColors[COLNETSPLIT].color3, GetNetsplitServer(channel, from),
                                     Colors[COLOFF], CmdsColors[COLNETSPLIT].color4, channel, Colors[COLOFF]);
                            snprintf(tmpbuf2, sizeof(tmpbuf2), "%sNetsplit hack%s %s by : %s%s%s",
                                     CmdsColors[COLNETSPLIT].color1, Colors[COLOFF], tmpbuf1,
                                     CmdsColors[COLNETSPLIT].color5, from, Colors[COLOFF]);
#else
                            snprintf(tmpbuf2, sizeof(tmpbuf2), "Netsplit hack [%s] on %s by : %s",
                                     GetNetsplitServer(channel, from), channel, from);
#endif
                            say("%s",tmpbuf2);
                            if (away_set || LogOn) AwaySave(tmpbuf2, SAVEHACK);
                            if (chan && chan->ChanLog) ChannelLogSave(tmpbuf2, chan);
                            restore_message_from();
                        }
                }
/**********************************************************************/
		else
			remove_from_channel(channel, from, parsing_server_index);
	}
	if (join)
	{
/**************************** PATCHED by Flier ******************************/
                if ((double_ignore(channel, NULL, IGNORE_CRAP)) == IGNORED) return;
/****************************************************************************/
		if (!get_channel_oper(channel, parsing_server_index))
			in_on_who = 1;
 		save_message_from();
		message_from(channel, LOG_CRAP);
		if (flag != IGNORED && do_hook(JOIN_LIST, "%s %s %s", from,
 						channel, ov ? ov : ""))
		{
/**************************** PATCHED by Flier ******************************/
			/*if (FromUserHost)
				if (ov && *ov)
					say("%s (%s) has joined channel %s +%s", from,
				    FromUserHost, channel, ov);
				else
					say("%s (%s) has joined channel %s", from,
				    FromUserHost, channel);
			else
				if (ov && *ov)
					say("%s has joined channel %s +%s", from, 
					channel, ov);
				else
					say("%s has joined channel %s", from, channel);*/
                        if (FromUserHost) {
                            if (HandleJoin(joiner, from, FromUserHost, channel, chan) && !donelj) {
                                strmcpy(tmpbuf, from, sizeof(tmpbuf));
                                strmcat(tmpbuf, "/", sizeof(tmpbuf));
                                if (ov && *ov) strmcat(tmpbuf, "+", sizeof(tmpbuf));
                                strmcat(tmpbuf, channel, sizeof(tmpbuf));
                                malloc_strcpy(&(server_list[parsing_server_index].LastJoin), tmpbuf);
                                update_all_status();
                            }
                        }
                        else
                            if (ov && *ov)
                                say("%s has joined channel %s +%s", from, channel, ov);
                            else
                                say("%s has joined channel %s", from, channel);
/****************************************************************************/				
		}
/**************************** PATCHED by Flier ******************************/
                else if (flag != IGNORED) {
                    strmcpy(tmpbuf, from, sizeof(tmpbuf));
                    strmcat(tmpbuf, "/", sizeof(tmpbuf));
                    if (ov && *ov) strmcat(tmpbuf, "+", sizeof(tmpbuf));
                    strmcat(tmpbuf, channel, sizeof(tmpbuf));
                    malloc_strcpy(&(server_list[parsing_server_index].LastJoin), tmpbuf);
                    update_all_status();
                }
/****************************************************************************/
 		restore_message_from();
                in_on_who = 0;
        }
}

static	void
p_invite(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*high;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_INVITES);
	switch (flag)
	{
	case IGNORED:
		if (get_int_var(SEND_IGNORE_MSG_VAR))
			send_to_server("NOTICE %s :%s is ignoring you",
				from, get_server_nickname(parsing_server_index));
		return;
	case HIGHLIGHTED:
		high = &highlight_char;
		break;
	default:
		high = empty_string;
		break;
	}
	if (ArgList[0] && ArgList[1])
	{
		if ((flag != DONT_IGNORE) && (ignore_usernames & IGNORE_INVITES)
		    && !FromUserHost)
			add_to_whois_queue(from, whois_ignore_invites,
					"%s", ArgList[1]);
		else
		{
/**************************** PATCHED by Flier ******************************/
                        if ((double_ignore(ArgList[1],NULL,IGNORE_INVITES))==IGNORED) return;
/****************************************************************************/
 			save_message_from();
			message_from(from, LOG_CRAP);
			if (do_hook(INVITE_LIST, "%s %s", from, ArgList[1]))
/************************** PATCHED by Flier **************************/
				/*say("%s%s%s invites you to channel %s", high,
						from, high, ArgList[1]);*/
                           HandleInvite(from,FromUserHost,ArgList[1]);
/**********************************************************************/
 			restore_message_from();
			malloc_strcpy(&invite_channel, ArgList[1]);
			malloc_strcpy((char **) &recv_nick, from);
		}
	}
}

static	void
p_server_kill(from, ArgList)
	char	*from,
		**ArgList;
{
	/*
	 * this is so bogus checking for a server name having a '.'
	 * in it - phone, april 1993.
	 */
	if (index(from, '.'))
		say("You have been rejected by server %s", from);
	else
	{
		say("You have been killed by operator %s %s", from,
			ArgList[1] ? ArgList[1] : "(No Reason Given)");
/*************************** PATCHED by SHEIK ****************************/
                HandleKills(from_server, from, FromUserHost, ArgList[1]);
/****************************************************************************/
	}
	close_server(parsing_server_index, empty_string);
/*************************** PATCHED by SHEIK ****************************/
        ReconnectOnKill(from_server);
/*************************************************************************/
	window_check_servers();
	if (!connected_to_server)
		say("Use /SERVER to reconnect to a server");
}

static	void
p_ping(ArgList)
	char	**ArgList;
{
	PasteArgs(ArgList, 0);
	send_to_server("PONG :%s", ArgList[0]);
}

static	void
p_nick(from, ArgList)
	char	*from,
		**ArgList;
{
	int	one_prints = 0,
		its_me = 0;
	char	*chan;
	char	*line;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	line = ArgList[0];
	if (my_stricmp(from, get_server_nickname(parsing_server_index)) == 0){
		if (parsing_server_index == primary_server)
 			malloc_strcpy(&nickname, line);
		set_server_nickname(parsing_server_index, line);
		its_me = 1;
/**************************** Patched by Flier ******************************/
                OrigNickSent=0;
/****************************************************************************/
	}
 	save_message_from();
	if (flag != IGNORED)
	{
		for (chan = walk_channels(from, 1, parsing_server_index); chan;
				chan = walk_channels(from, 0, -1))
		{
			message_from(chan, LOG_CRAP);
			if (do_hook(CHANNEL_NICK_LIST, "%s %s %s", chan, from, line))
				one_prints = 1;
		}
		if (one_prints)
		{
			if (its_me)
				message_from((char *) 0, LOG_CRAP);
			else
				message_from(what_channel(from, parsing_server_index), LOG_CRAP);
/**************************** PATCHED by Flier *****************************/
                        HandleNickChange(from,line,FromUserHost,from_server);
/***************************************************************************/
			if (do_hook(NICKNAME_LIST, "%s %s", from, line))
/**************************** PATCHED by Flier ******************************/
				/*say("%s is now known as %s", from, line);*/
#ifdef WANTANSI
                            say("%s%s%s is now %sknown%s as %s%s%s",
                                CmdsColors[COLNICK].color1,from,Colors[COLOFF],
                                CmdsColors[COLNICK].color2,Colors[COLOFF],
                                CmdsColors[COLNICK].color3,line,Colors[COLOFF]);
#else
                            say("%s is now known as %s", from, line);
#endif
/****************************************************************************/
		}
	}
        rename_nick(from, line, parsing_server_index);
	if (my_stricmp(from, line))
	{
 		message_from((char *) 0, LOG_CURRENT);
		notify_mark(from, 0, 0);
		notify_mark(line, 1, 0);
	}
 	restore_message_from();
}

static	void
p_mode(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel;
	char	*line;
	int	flag;
/**************************** PATCHED by Flier ******************************/
        char    tmpbuf1[mybufsize/2];
        char    tmpbuf2[mybufsize/2];
        char    tmpbuf3[mybufsize/2];
/****************************************************************************/

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	PasteArgs(ArgList, 1);
	channel = ArgList[0];
	line = ArgList[1];
 	save_message_from();
	message_from(channel, LOG_CRAP);
	if (channel && line)
	{
		if (is_channel(channel))
		{
/************************ PATCHED by Flier *************************/
			/*if (flag != IGNORED && do_hook(MODE_LIST, "%s %s %s",
					from, channel, line))
				say("Mode change \"%s\" on channel %s by %s",
						line, channel, from);
			update_channel_mode(channel, parsing_server_index, line);*/
                        strmcpy(tmpbuf3,line,sizeof(tmpbuf3));
                        update_channel_mode(channel,parsing_server_index,tmpbuf3,
                                            from,FromUserHost,tmpbuf1,tmpbuf2,NULL);
                        if (flag!=IGNORED) flag=double_ignore(channel,NULL,IGNORE_CRAP);
                        if (*tmpbuf3 && flag!=IGNORED && do_hook(MODE_LIST, "%s %s %s",
                                                                 from,channel,tmpbuf3))
                            ModePrint(tmpbuf3,channel,from,FromUserHost,tmpbuf1,tmpbuf2);
#ifdef EXTRAS
                        if (my_stricmp(from,get_server_nickname(from_server)))
                            CheckLock(channel,from_server,NULL);
#endif
/*******************************************************************/
		}
		else
		{
			if (flag != IGNORED && do_hook(MODE_LIST, "%s %s %s",
					from, channel, line))
				say("Mode change \"%s\" for user %s by %s",
						line, channel, from);
			update_user_mode(line);
		}
		update_all_status();
	}
 	restore_message_from();
}

static	void
p_kick(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel,
		*who,
		*comment;
/**************************** PATCHED by Flier *******************************/
        int     frkick;
/****************************************************************************/

	if (!from)
		return;
	channel = ArgList[0];
	who = ArgList[1];
	comment = ArgList[2];

	if (channel && who)
	{
 		save_message_from();
 		message_from(channel, LOG_CRAP);
		if (my_stricmp(who, get_server_nickname(parsing_server_index)) == 0)
		{
/**************************** PATCHED by Flier *******************************/
			/*if (comment && *comment)
			{
				if (do_hook(KICK_LIST, "%s %s %s %s", who,
						from, channel, comment))
					say("You have been kicked off channel %s by %s (%s)",
						channel, from, comment);
			}
			else
			{
				if (do_hook(KICK_LIST, "%s %s %s", who, from,
						channel))
					say("You have been kicked off channel %s by %s",
						channel, from);
			}*/
                        int rejoin=0;
                        char tmpbuf[mybufsize/2];
                        NickList *joiner;
                        ChannelList *chan;

                        chan=lookup_channel(channel,parsing_server_index,0);
                        if (chan) {
                            if (chan->AutoRejoin) {
                                snprintf(tmpbuf,sizeof(tmpbuf),"%s ",chan->channel);
                                if (chan->key) strmcat(tmpbuf,chan->key, sizeof(tmpbuf));
                                rejoin=1;
                            }
                            chan->kick++;
                        }
                        joiner=CheckJoiners(who,channel,from_server,chan);
                        frkick=(joiner && joiner->frlist)?joiner->frlist->privs:0;
                        HandleMyKick(who,from,FromUserHost,channel,comment);
                        if ((double_ignore(channel,NULL,IGNORE_CRAP))!=IGNORED) {
                            if (comment && *comment)
                            {
                                if (do_hook(KICK_LIST, "%s %s %s %s", who,
                                            from, channel, comment))
#ifdef CELECOSM
                                    KickPrint("You","were",from,channel,comment,rejoin,frkick);
#else
                                    KickPrint("You","have",from,channel,comment,rejoin,frkick);
#endif /* CELECOSM */
                            }
                            else
                            {
                                if (do_hook(KICK_LIST, "%s %s %s", who, from,
                                            channel))
#ifdef CELECOSM
                                    KickPrint("You","were",from,channel,NULL,rejoin,frkick);
#else
                                    KickPrint("You","have",from,channel,NULL,rejoin,frkick);
#endif /*CELECOSM*/
                            }
                        }
/****************************************************************************/
			remove_channel(channel, parsing_server_index);
/**************************** PATCHED by Flier ******************************/
                        if (rejoin) e_channel("JOIN",tmpbuf,NULL);
/****************************************************************************/
			update_all_status();
		}
		else
		{
/**************************** PATCHED by Flier ******************************/
                        HandleKick(from,who,FromUserHost,channel,comment,&frkick);
			/*if (comment && *comment)
			{
				if (do_hook(KICK_LIST, "%s %s %s %s", who,
						from, channel, comment))
					say("%s has been kicked off channel %s by %s (%s)",
						who, channel, from, comment);
			}
			else
			{
				if (do_hook(KICK_LIST, "%s %s %s", who, from,
						channel))
					say("%s has been kicked off channel %s by %s",
						who, channel, from);
			}*/
                        if ((double_ignore(channel,NULL,IGNORE_CRAP))!=IGNORED) {
                            if (comment && *comment)
                            {
                                if (do_hook(KICK_LIST, "%s %s %s %s", who,
                                            from, channel, comment))
#ifdef CELECOSM
                                    KickPrint(who,"was",from,channel,comment,0,frkick);
#else
                                    KickPrint(who,"has",from,channel,comment,0,frkick);
#endif /* CELECOSM */
                            }
                            else
                            {
                                if (do_hook(KICK_LIST, "%s %s %s", who, from,
                                            channel))
#ifdef CELECOSM
                                    KickPrint(who,"was",from,channel,NULL,0,frkick);
#else
                                    KickPrint(who,"has",from,channel,NULL,0,frkick);
#endif /*CELECOSM*/
                            }
                        }
/****************************************************************************/
			remove_from_channel(channel, who, parsing_server_index);
		}
 		restore_message_from();
	}
}

static	void
p_part(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel;
	char	*comment;
	int	flag;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
        char    *colnick;
        NickList *joiner;
#endif
        ChannelList *chan;
/****************************************************************************/

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
/**************************** PATCHED by Flier ******************************/
        if (flag!=IGNORED) flag=double_ignore(ArgList[0],NULL,IGNORE_CRAP);
/****************************************************************************/
	channel = ArgList[0];
	if (!is_on_channel(channel, parsing_server_index, from))
		return;
	comment = ArgList[1];
	if (!comment)
		comment = empty_string;
	in_on_who = 1;
        if (flag != IGNORED)
        {
 		save_message_from();
		message_from(channel, LOG_CRAP);
            	if (do_hook(LEAVE_LIST, "%s %s %s", from, channel, comment)) {
/**************************** PATCHED by Flier ******************************/
			/*if (comment && *comment != '\0')
				say("%s has left channel %s (%s)", from, channel, comment);
			else
				say("%s has left channel %s", from, channel);*/
#ifdef WANTANSI
                    joiner=CheckJoiners(from,channel,parsing_server_index,NULL);
                    if (joiner && joiner->shitlist && joiner->shitlist->shit)
                        colnick=CmdsColors[COLLEAVE].color5;
                    else if (joiner && joiner->frlist && joiner->frlist->privs)
                        colnick=CmdsColors[COLLEAVE].color4;
                    else colnick=CmdsColors[COLLEAVE].color1;
                    if (comment && *comment!='\0' && strcmp(from,comment))
                        say("%s%s%s has left channel %s%s%s (%s%s%s)",
                            colnick,from,Colors[COLOFF],
                            CmdsColors[COLLEAVE].color2,channel,Colors[COLOFF],
                            CmdsColors[COLLEAVE].color6,comment,Colors[COLOFF]);
                    else say("%s%s%s has left channel %s%s%s",colnick,from,Colors[COLOFF],
                             CmdsColors[COLLEAVE].color2,channel,Colors[COLOFF]);
#else
                    if (comment && *comment!='\0' && strcmp(from,comment))
                        say("%s has left channel %s (%s)",from,channel,comment);
                    else say("%s has left channel %s",from,channel);
#endif
                    if (ChanLog) {
                        chan = lookup_channel(channel, parsing_server_index, 0);
                        if (chan->ChanLog) {
                            char tmpbuf2[mybufsize];

                            snprintf(tmpbuf2, sizeof(tmpbuf2), "%s has left channel %s", from, channel);
                            if (comment && *comment)
                                snprintf(&tmpbuf2[strlen(tmpbuf2)], sizeof(tmpbuf2), "(%s)", comment);
                            ChannelLogSave(tmpbuf2, chan);
                        }
                    }
/****************************************************************************/
                }
 		restore_message_from();
        }
	if (my_stricmp(from, get_server_nickname(parsing_server_index)) == 0)
		remove_channel(channel, parsing_server_index);
	else
		remove_from_channel(channel, from, parsing_server_index);
	in_on_who = 0;
}


/*
 * parse_server: parses messages from the server, doing what should be done
 * with them 
 */
void
parse_server(line)
	char	*line;
{
	server_list[parsing_server_index].parse_server(line);
}

void
irc2_parse_server(line)
	char	*line;
{
	char	*from,
		*comm,
		*end,
		*copy = (char *) 0;
	int	numeric;
	char	**ArgList;
	char	*TrueArgs[MAXPARA + 1];

	if ((char *) 0 == line)
		return;

        end = strlen(line) + line;
	if (*--end == '\n')
		*end-- = '\0';
	if (*end == '\r')
		*end-- = '\0';

	if (*line == ':')
	{
		if (!do_hook(RAW_IRC_LIST, "%s", line + 1))
			return;
	}
	else if (!do_hook(RAW_IRC_LIST, "%s %s", "*", line))
		return;

	malloc_strcpy(&copy, line);
	ArgList = TrueArgs;
	BreakArgs(copy, &from, ArgList);

	if (!(comm = (*ArgList++)))
		return;		/* Empty line from server - ByeBye */

	/*
	 * XXX!!!
	 * this should fail on '1xxx'!!!
	 */
	if (0 != (numeric = atoi(comm)))
/**************************** PATCHED by Flier ******************************/
        {
                if (numeric == 1) {
                    char *nickstart, *nickend;
                    char tmpbuf[mybufsize / 8];

                    if ((nickstart = index(line,' ')) && (nickstart = index(nickstart + 1, ' ')) &&
                        (nickend = index(nickstart + 1, ' '))) {
                        strmcpy(tmpbuf, nickstart + 1, nickend - nickstart);
                        set_server_nickname(parsing_server_index, tmpbuf);
                    }
                }
/****************************************************************************/
		numbered_command(from, numeric, ArgList);
/**************************** PATCHED by Flier ******************************/
        }
/****************************************************************************/
	else if (strcmp(comm, "NAMREPLY") == 0)
		funny_namreply(from, ArgList);
	else if (strcmp(comm, "WHOREPLY") == 0)
		whoreply(from, ArgList);
	else if (strcmp(comm, "NOTICE") == 0)
		parse_notice(from, ArgList);
	/* everything else is handled locally */
	else if (strcmp(comm, "PRIVMSG") == 0)
		p_privmsg(from, ArgList);
	else if (strcmp(comm, "JOIN") == 0)
		p_channel(from, ArgList);
	else if (strcmp(comm, "PART") == 0)
		p_part(from, ArgList);
	else if (strcmp(comm, "CHANNEL") == 0)
		p_channel(from, ArgList);
	else if (strcmp(comm, "MSG") == 0)
		/*p_msgcmd(from, ArgList)*/;
	else if (strcmp(comm, "QUIT") == 0)
		p_quit(from, ArgList);
	else if (strcmp(comm, "WALL") == 0)
		p_wall(from, ArgList);
	else if (strcmp(comm, "WALLOPS") == 0)
		p_wallops(from, ArgList);
	else if (strcmp(comm, "LINREPLY") == 0)
		p_linreply(ArgList);
	else if (strcmp(comm, "PING") == 0)
		p_ping(ArgList);
	else if (strcmp(comm, "TOPIC") == 0)
		p_topic(from, ArgList);
	else if (strcmp(comm, "PONG") == 0)
		p_pong(from, ArgList);
	else if (strcmp(comm, "INVITE") == 0)
		p_invite(from, ArgList);
	else if (strcmp(comm, "NICK") == 0)
		p_nick(from, ArgList);
	else if (strcmp(comm, "KILL") == 0)
		p_server_kill(from, ArgList);
	else if (strcmp(comm, "MODE") == 0)
		p_mode(from, ArgList);
	else if (strcmp(comm, "KICK") == 0)
		p_kick(from, ArgList);
	else if (strcmp(comm, "ERROR") == 0)
		p_error(from, ArgList);
	else if (strcmp(comm, "ERROR:") == 0) /* Server bug makes this a must */
		p_error(from, ArgList);
	else
	{
		PasteArgs(ArgList, 0);
		if (from)
			say("Odd server stuff: \"%s %s\" (%s)", comm,
				ArgList[0], from);
		else
			say("Odd server stuff: \"%s %s\"", comm, ArgList[0]);
	}
	new_free(&copy);
}
