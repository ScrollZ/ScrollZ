/*
 * notice.c: special stuff for parsing NOTICEs
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
 * $Id: notice.c,v 1.35 2006-03-22 17:16:49 f Exp $
 */

#include "irc.h"

#include "whois.h"
#include "ctcp.h"
#include "window.h"
#include "lastlog.h"
#include "flood.h"
#include "vars.h"
#include "ircaux.h"
#include "hook.h"
#include "ignore.h"
#include "server.h"
#include "funny.h"
#include "output.h"
#include "names.h"
#include "parse.h"
#include "notify.h"

/************************* PATCHED by Flier **************************/
#include "status.h"
#include "myvars.h"

extern void HandleFakes _((char *, char *, int));
extern int  HandleNotice _((char *, char *, char *, int, char *, int *));
extern int  IsIrcNetOperChannel _((char *));
#if defined(OPERVISION) && defined(WANTANSI)
extern void OVformat _((char *, char *));
#endif
extern char *TimeStamp _((int));
extern void ChannelLogSave _((char *, ChannelList *));
#ifdef BLAXTHOS
extern int  DecryptString _((char *, char *, char *, int, int));
#endif
/*********************************************************************/

#ifndef LITE
static	void	parse_note _((char *, char *));
#endif
static	void	parse_server_notice _((char *, char *, char *));

/*
 * parse_note: handles the parsing of irc note messages which are sent as
 * NOTICES.  The notice() function determines which notices are note messages
 * and send that info to parse_note() 
 */
#ifndef LITE
static	void
parse_note(server, line)
	char	*server;
	char	*line;
{
	char	*date,
		*nick,
		*flags,
		*high,
		*name,
		*message;
	int	ign1,
		ign2,
		level;
 	time_t	the_time;

	flags = next_arg(line, &date);	/* what to do with these flags */
	nick = next_arg(date, &date);
	name = next_arg(date, &date);
	if ((message = index(date, '*')) != NULL)
		*message = (char) 0;
	if (((ign1 = is_ignored(nick, IGNORE_NOTES)) == IGNORED) ||
			((ign2 = is_ignored(name, IGNORE_NOTES)) == IGNORED))
		return;
	if ((ign1 == HIGHLIGHTED) || (ign2 == HIGHLIGHTED))
		high = &highlight_char;
	else
		high = empty_string;
 	the_time = atol(date);
 	date = ctime(&the_time);
	date[24] = (char) 0;
	level = set_lastlog_msg_level(LOG_NOTES);
	if (do_hook(NOTE_LIST, "%s %s %s %s %s %s", nick, name, flags, date,
		server, message + 2))
	{
		put_it("Note from %s (%s) %s", nick, name, flags);
		put_it("It was queued %s from server %s", date, server);
		put_it("%s[%s]%s %s", high, nick, high, message + 2);
	}
	if (beep_on_level & LOG_NOTES)
		beep_em(1);
	set_lastlog_msg_level(level);
}
#endif

static	void
parse_server_notice(from, to, line)
	char	*from,
 		*to,
		*line;
{
	char	server[81],
		version[21];
	int	user_cnt,
		server_cnt,
		lastlog_level;
	int	flag;

	if (!from || !*from)
		from = server_list[parsing_server_index].itsname ?
			server_list[parsing_server_index].itsname :
			server_list[parsing_server_index].name;
	if (get_int_var(SUPPRESS_SERVER_MOTD_VAR) &&
		get_server_motd(parsing_server_index))
	{
		if (strncmp("*** Message-of-today", line, 20) == 0)
		{
			set_server_motd(parsing_server_index, 0);
			return;
		}
		if (strncmp("MOTD ", line, 5) == 0)
		{
			set_server_motd(parsing_server_index, 1);
			return;
		}
		if (strcmp("* End of /MOTD command.", line) == 0)
		{
			set_server_motd(parsing_server_index, 0);
			return;
		}
	}
 	save_message_from();
	if (!strncmp(line, "*** Notice --", 13))
	{
		message_from((char *) 0, LOG_OPNOTE);
		lastlog_level = set_lastlog_msg_level(LOG_OPNOTE);
	}
	else
	{
		message_from((char *) 0, LOG_SNOTE);
		lastlog_level = set_lastlog_msg_level(LOG_SNOTE);
	}
	if (to)
	{
		if (do_hook(SERVER_NOTICE_LIST, "%s %s %s", from, to, line))
/*************************** PATCHED by Flier ******************************/
                {
#if defined(OPERVISION) && defined(WANTANSI)
                        if (OperV &&
                           (get_server_version(from_server)==Server2_9 || 
                            get_server_version(from_server)==Server2_10 ||
                            get_server_version(from_server)==Server2_11) &&
                           IsIrcNetOperChannel(to))
                            OVformat(line,NULL);
                        else
#endif
/***************************************************************************/      
			put_it("%s %s", to, line);
/**************************** PATCHED by Flier ******************************/
                }
/****************************************************************************/
	}
	else
	{
		if (get_server_version(parsing_server_index) >= Server2_7 && 
		    *line != '*'  && *line != '#' && strncmp(line, "MOTD ", 4))
			flag = 1;
		else
			flag = 0;
		if (do_hook(SERVER_NOTICE_LIST, flag ? "%s *** %s"
						     : "%s %s", from, line))
/*************************** PATCHED by Flier ******************************/      
			/*put_it(flag ? "*** %s" : "%s", line);*/
                        if (strncmp(line,"*** Your host is ",17)) {
#if defined(OPERVISION) && defined(WANTANSI)
                            if (OperV) OVformat(line,NULL);
                            else
#endif
                            if (!ServerNotice && strncmp(line, "*** Notice --", 13)) say("%s",line);
                            else if (ServerNotice) say("%s",line);
                            else if (ShowFakes && wild_match("*notice*fake*",line))
                                HandleFakes(line,from,from_server);
                        }
/***************************************************************************/      
	}
	if ((parsing_server_index == primary_server) &&
			((sscanf(line, "*** There are %d users on %d servers",
			&user_cnt, &server_cnt) == 2) ||
			(sscanf(line, "There are %d users on %d servers",
			&user_cnt, &server_cnt) == 2)))
	{
		if ((server_cnt < get_int_var(MINIMUM_SERVERS_VAR)) ||
				(user_cnt < get_int_var(MINIMUM_USERS_VAR)))
		{
			say("Trying better populated server...");
			get_connected(parsing_server_index + 1, 0);
		}
        }
#ifdef BROKEN_SCANF
	else if (!strncmp(line, "*** Your host is ", 17))
#else
	else if ((sscanf(line, "*** Your host is %80s running version %20s",
			server, version) == 2))
#endif /* BROKEN_SCANF */
        {
		if (get_server_version(parsing_server_index) < Server2_8)
			got_initial_version(line);
        }
	if (lastlog_level)
	{
		set_lastlog_msg_level(lastlog_level);
 		restore_message_from();
	}
}

void 
parse_notice(from, Args)
	char 	*from;
	char 	**Args;
{
	int	level,
		type;
	char	*to;
	int	no_flooding;
	int	flag;
	char	*high,
		not_from_server = 1;
	char	*line;
/**************************** Patched by Flier ******************************/
        char    *stampbuf = TimeStamp(2);
/****************************************************************************/

	PasteArgs(Args, 1);
	to = Args[0];
	line = Args[1];
	if (!to || !line)
		return;
	if (*to)
	{
 		save_message_from();
		if (is_channel(to))
		{
			message_from(to, LOG_NOTICE);
			type = PUBLIC_NOTICE_LIST;
		}
		else
		{
			message_from(from, LOG_NOTICE);
			type = NOTICE_LIST;
		}
		if (from && *from && strcmp(get_server_itsname(parsing_server_index), from))
		{
			flag = double_ignore(from, FromUserHost,IGNORE_NOTICES);
			if (flag != IGNORED)
			{
				if (flag == HIGHLIGHTED)
					high = &highlight_char;
				else
					high = empty_string;
				if (index(from, '.'))
				{
		/*
		 * only dots in servernames, right ?
		 *
		 * But a server name doesn't nessicarily have to have
		 * a 'dot' in it..  - phone, jan 1993.
		 */
					not_from_server = 0;
#ifndef LITE
					if (strncmp(line, "*/", 2) == 0)
					{
						parse_note(from, line + 1);
 						goto out;
					}
#endif
				}
				if (not_from_server && (flag != DONT_IGNORE) && !FromUserHost &&
							(ignore_usernames & IGNORE_NOTICES))
					add_to_whois_queue(from, whois_ignore_notices, "%s", line);
				else
				{
					line = do_notice_ctcp(from, to, line);
					if (*line == '\0')
					{
 						goto out;
					}
					level = set_lastlog_msg_level(LOG_NOTICE);
					no_flooding = check_flooding(from, NOTICE_FLOOD, line);

 					if (sed == 0 || do_hook(ENCRYPTED_NOTICE_LIST, "%s %s %s", from, to, line))
					{
/**************************** Patched by Flier ******************************/
                                                int iscrypted = 0;
/****************************************************************************/

						if (type == NOTICE_LIST)
						{
/**************************** PATCHED by Flier ******************************/
                                                        if (HandleNotice(from, line, FromUserHost, 0, to,
                                                                         &iscrypted))
/****************************************************************************/
							if (no_flooding &&
							    do_hook(type, "%s %s", from, line))
/**************************** PATCHED by Flier ******************************/
								/*put_it("%s-%s-%s %s", high, from, high, line);*/
                                                                HandleNotice(from, line, FromUserHost, 1, to,
                                                                             &iscrypted);
/****************************************************************************/
						}
						else
						{
/**************************** PATCHED by Flier ******************************/
                                                        if (HandleNotice(from, line, FromUserHost, 0, to,
                                                                         &iscrypted))
/****************************************************************************/
							if (no_flooding &&
							    do_hook(type, "%s %s %s", from, to, line))
/**************************** Patched by Flier ******************************/
								/*put_it("%s-%s:%s-%s %s", high, from, to, high, line);*/
                                                            put_it("%s%s%s-%s:%s-%s %s",
                                                                   iscrypted ? "[!]" : "",
                                                                   stampbuf, high, from, to, high, line);
                                                        if (ChanLog) {
                                                            ChannelList *chan;

                                                            chan = lookup_channel(to, parsing_server_index, 0);
                                                            if (chan && chan->ChanLog) {
                                                                char tmpbuf3[mybufsize];

                                                                snprintf(tmpbuf3, sizeof(tmpbuf3), "-%s- %s", from, line);
                                                                ChannelLogSave(tmpbuf3, chan);
                                                            }
    }
/****************************************************************************/
						}
						if (beep_on_level & LOG_NOTICE)
							beep_em(1);
						set_lastlog_msg_level(level);
						if (not_from_server)
							notify_mark(from, 1, 0);
					}
				}
			}
		}
		else 
 			parse_server_notice(from, type == PUBLIC_NOTICE_LIST ? to : 0, line);
	}
	else
		put_it("%s", line + 1);
out:
 	restore_message_from();
}

/*
 * load the initial .ircrc
 */
void
load_ircrc()
{
	static	int done = 0;

	if (done++)
		return;
	if (access(ircrc_file, R_OK) == 0)
	{
 		char lbuf[BIG_BUFFER_SIZE+1];

 		strmcpy(lbuf,ircrc_file,BIG_BUFFER_SIZE);
 		strmcat(lbuf," ",BIG_BUFFER_SIZE);
 		strmcat(lbuf,args_str,BIG_BUFFER_SIZE);
 		load(empty_string, lbuf, empty_string);
	}
	else if (get_int_var(NOVICE_VAR))
		say("If you have not already done so, please read the new user information with /HELP NEWUSER");
}

/*
 * load the initial .ircquick
 */
void
load_ircquick()
{
	static	int done = 0;

	if (done++)
		return;
	if (access(ircquick_file, R_OK) == 0)
	{
		char lbuf[BIG_BUFFER_SIZE+1];

		strmcpy(lbuf,ircquick_file,BIG_BUFFER_SIZE);
		strmcat(lbuf," ",BIG_BUFFER_SIZE);
		strmcat(lbuf,args_str,BIG_BUFFER_SIZE);
		load(empty_string, lbuf, empty_string);
	}
}

/*
 * got_initial_version: this is called when ircii get the NOTICE in
 * all version of ircd before 2.8, or the 002 numeric for 2.8 and
 * beyond.  I guess its handled rather badly at the moment....
 * added by phone, late 1992.
 */
void
got_initial_version(line)
	char	*line;
{
 	char	server[256],
		version[256];
	char	*s, c;
/**************************** Patched by Flier ******************************/
        int     was_connected;
/****************************************************************************/

	/*
	 * BROKEN_SCANF crap here provided by Aiken <adrum@u.washington.edu>
	 * sometime 1993...
	 */

#ifdef BROKEN_SCANF
	if (strncmp(line, "*** Your host is ", 17))
		return;
 	strncpy(server, &line[17], 256);

	server[79] = 0;
	if (s = index(server, ','))
		*s = 0;
	if (s = index(server, ' '))
		*s = 0;
	version[0] = 0;

	if (s = my_index(&line[17], ' '))
	{
		if (!strncmp(c, " running version ", 17))
		{
			strncpy(version, &s[17], 255);
			version[255] = 0;
		}
                else return;
	}
	else return;
#else
 	if ((sscanf(line, "*** Your host is %256s running version %255s",
 			server, version)) != 2) {
 		yell("This server has a non-standard connection message!");
 		strcpy(version, "2.9");
 		strcpy(server, server_list[parsing_server_index].name);
 	}
	else if ((c = server[strlen(server) - 1]) == ',' || c == '.')
		server[strlen(server) - 1] = '\0';
#endif /* BROKEN_SCANF */
	attempting_to_connect--;
/**************************** PATCHED by Flier ******************************/
        /* attempting_to_connect is broken if we are connecting to multiple
           servers simoultaneously -> correct the value apropriately */
        if (attempting_to_connect < 0) attempting_to_connect = 0;
        was_connected = server_list[parsing_server_index].connected;
/****************************************************************************/
	set_server_motd(parsing_server_index, 1);
	server_is_connected(parsing_server_index, 1);
	if ((s = (char *) index(server, '[')) != NULL)
		*s = '\0';	/*
				 * Handles the case where the server name is
				 * different to the host name.
				 */
	if (!strncmp(version, "2.5", 3))
		set_server_version(parsing_server_index, Server2_5);
	else if (!strncmp(version, "2.6", 3))
		set_server_version(parsing_server_index, Server2_6);
	else if (!strncmp(version, "2.7", 3))
		set_server_version(parsing_server_index, Server2_7);
	else if (!strncmp(version, "2.8", 3))
		set_server_version(parsing_server_index, Server2_8);
	else if (!strncmp(version, "2.9", 3))
		set_server_version(parsing_server_index, Server2_9);
	else if (!strncmp(version, "2.10", 4))
		set_server_version(parsing_server_index, Server2_10);
/**************************** Patched by Flier ******************************/
        /* this is here so we can identify Freenode servers */
	else if (strstr(version, "hyperion"))
		set_server_version(parsing_server_index, Server2_90);
	else
		set_server_version(parsing_server_index, Server2_11);
/****************************************************************************/
	malloc_strcpy(&server_list[parsing_server_index].version_string, version);
	set_server_itsname(parsing_server_index, server);
/**************************** Patched by Flier ******************************/
        /* reinstate oper mode if password is stored */
#ifdef BLAXTHOS
        if (EncryptPassword && OperNick && OperPassword) {
            char nickbuf[mybufsize];
            char passbuf[mybufsize];

            say("Reinstating OPER mode with stored password");
            DecryptString(nickbuf, OperNick, EncryptPassword, sizeof(nickbuf) - 1, 1);
            DecryptString(passbuf, OperPassword, EncryptPassword, sizeof(passbuf) - 1, 1);
            send_to_server("OPER %s %s", nickbuf, passbuf);
            bzero(nickbuf, strlen(nickbuf));
            bzero(passbuf, strlen(passbuf));
        }
#endif
        /* read above, we already parsed this so skip it this time since
           it will corrupt all server information we stored in previous
           pass */
        if (was_connected) {
            do_hook(CONNECT_LIST, "%s %d", get_server_name(parsing_server_index),
                    get_server_port(parsing_server_index));
            return;
        }
/****************************************************************************/
	reconnect_all_channels(parsing_server_index);
	reinstate_user_modes(/* parsing_server_index */); /* XXX */
	maybe_load_ircrc();
	if (server_list[parsing_server_index].away)
		send_to_server("AWAY :%s",
			server_list[parsing_server_index].away);
	update_all_status();
	do_hook(CONNECT_LIST, "%s %d", get_server_name(parsing_server_index),
		get_server_port(parsing_server_index));
}

void
maybe_load_ircrc()
{
	if (never_connected)
	{
		never_connected = 0;
		if (!bflag)
		{
			loading_global = 1;
/**************************** PATCHED by Flier ******************************/
			/*load(empty_string, "global", empty_string);*/
			load(empty_string, "szglobal", empty_string);
/****************************************************************************/
			loading_global = 0;
		}
		/* read the .ircrc file */
		if (!qflag)
			load_ircrc();
	}
}
