/*
 * numbers.c:handles all those strange numeric response dished out by that
 * wacky, nutty program we call ircd 
 *
 * Written by Michael Sandrof
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
 * $Id: numbers.c,v 1.30 2000-11-01 09:53:11 f Exp $
 */

#include "irc.h"

#include "input.h"
#include "edit.h"
#include "ircaux.h"
#include "vars.h"
#include "lastlog.h"
#include "hook.h"
#include "server.h"
#include "whois.h"
#include "numbers.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "names.h"
#include "whois.h"
#include "funny.h"
#include "parse.h"
/************************* PATCHED by Flier ***************************/
#include "ignore.h"
#include "myvars.h"
#ifdef ACID
#include "whowas.h"
#endif

extern void OnBans _((char **, int));
extern void EndOfBans _((char *, int));
extern void HandleEndOfWho _((char *, int));
extern void HandleNickCollision _((void));
#ifdef EXTRAS
extern void HandleLinks _((char *));
extern void ListSplitedServers _((void));
#endif
extern void TimeReply _((char *, char **));
extern void PrintLinks _((char *, char *, char *));
extern void HandleStatsK _((char *, char *));
extern void HandleStatsI _((char *, char *, char *));
extern void AutoChangeNick _((char *));
extern void ScrollZLoad _((void));
extern void HandleEndOfKill _((void));
extern void RemoveFromDCCList _((char *));
extern void DoTraceKill _((char *));
extern void HandleEndOfTraceKill _((void));
extern void AddToMap _((char *, char *));
extern void PrintMap _((void));
extern void ChannelCreateTime _((char *, char **));
extern void NoSuchServer4SPing _((char *, char **));
extern void PurgeChannel _((char *, int));
extern void CdccTimeWarning _((void));
#ifdef ACID
extern void TryChannelJoin _((void));
#endif
#ifdef OPER
extern void DoFilterTrace _((char *));
extern int  tottcount;
extern int  mattcount;
#endif
/* Patched by Zakath */
#ifdef CELE
extern void HandleTrace _((int, char *, char *, char *, char *, char *, char *));
#endif

extern void userhost _((char *, char *, char *));
/* ***************** */
/**********************************************************************/

/**************************** PATCHED by Flier ******************************/
/*static	void	reset_nickname _((void));*/
void	reset_nickname _((void));
/****************************************************************************/
static	void	nickname_in_use _((char *, char **));
static	void	password_sendline _((char *, char *));
static	void	get_password _((void));
/**************************** PATCHED by Flier ******************************/
/*static	void	nickname_sendline _((char *, char *));*/
void	nickname_sendline _((char *, char *));
/****************************************************************************/
static	void	channel_topic _((char *, char **));
static	void	not_valid_channel _((char *, char **));
static	void	cannot_join_channel _((char *, char **));
static	void	version _((char *, char **));
static	void	invite _((char *, char **));

static	int	already_doing_reset_nickname = 0;

/*
 * numeric_banner: This returns in a static string of either "xxx" where
 * xxx is the current numeric, or "***" if SHOW_NUMBERS is OFF 
 */
char	*
numeric_banner()
{
/**************************** PATCHED by Flier ******************************/
        /*static	char	thing[4];*/
        static	char	thing[64];
/****************************************************************************/

	if (get_int_var(SHOW_NUMERICS_VAR))
		sprintf(thing, "%3.3u", -current_numeric);
	else
/************************* PATCHED by Flier ***************************/
		/*strcpy(thing, "***");*/
		strcpy(thing,ScrollZstr);
/**********************************************************************/
	return (thing);
}


/*
 * display_msg: handles the displaying of messages from the variety of
 * possible formats that the irc server spits out.  you'd think someone would
 * simplify this 
 */
void
display_msg(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*ptr;
	char	*rest;

	rest = PasteArgs(ArgList, 0);
	if (from && (my_strnicmp(get_server_itsname(parsing_server_index), from,
			strlen(get_server_itsname(parsing_server_index))) == 0))
		from = (char *) 0;
	if ((ptr = (char *) index(rest, ':')) != NULL)
	{
		*(ptr++) = (char) 0;
		if (strlen(rest))
		{
			if (from)
				put_it("%s %s: %s (from %s)", numeric_banner(),
					rest, ptr, from);
			else
				put_it("%s %s: %s", numeric_banner(), rest,
					ptr);
		}
		else
		{
			if (from)
				put_it("%s %s (from %s)", numeric_banner(),
					ptr, from);
			else
				put_it("%s %s", numeric_banner(), ptr);
		}
	}
	else
	{
/**************************** PATCHED by Flier ******************************/
                if (!strncmp(rest,"***",3)) rest+=4;
/****************************************************************************/
		if (from)
			put_it("%s %s (from %s)", numeric_banner(), rest, from);
		else
			put_it("%s %s", numeric_banner(), rest);
	}
/**************************** PATCHED by Flier ******************************/
        if (wild_match("*nick*coll*",rest)) HandleNickCollision();
/****************************************************************************/
}

/**************************** PATCHED by Flier ******************************/
/* display_luser: handles the /Luser formatting. By Zakath */
void display_luser(comm, from, ArgList)
int  comm;
char *from;
char **ArgList;
{
    int  invnum;
    int  opernum;
    int	 usernum;
    int  clientnum;
    int  servernum;
    int  connectnum;
    int  channelnum;
    int  servicenum;
    char *rest;
    char *sname;
    char tmpbuf[mybufsize/2];
    char tmpbuf2[mybufsize/2];
    float totalper;
    static int totalnum;

    PasteArgs(ArgList,0);
    rest=ArgList[0];
    if (!do_hook(current_numeric,"%s %s",from,*ArgList)) return;
    if (from && (my_strnicmp(get_server_itsname(from_server),from,
	strlen(get_server_itsname(from_server)))==0)) from=(char *) 0;
/* Too lazy to put ifdef's for every put_it(), so it's just in two chunks */
    if (from) {
        sname=from;
        sprintf(tmpbuf2," (from %s)",from);
    }
    else {
        sname=server_list[from_server].itsname;
        if (!sname) sname=server_list[from_server].name;
        *tmpbuf2='\0';
    }
#ifdef WANTANSI
    if (comm==251) {
        say("Server information for: %s%s%s",
            CmdsColors[COLCSCAN].color1,sname,Colors[COLOFF]);
	if (sscanf(rest,"There are %d users and %d invisible on %d servers",
                   &usernum,&invnum,&servernum)==3) {
            totalnum=usernum+invnum;
	    sprintf(tmpbuf,"%s There are %s%d%s users (%d + %d invisible) on %s%d%s servers",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,totalnum,Colors[COLOFF],usernum,invnum,
                    CmdsColors[COLCSCAN].color2,servernum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
        }
/* for hybrid ircd */
        else if (sscanf(rest,"There are %d users plus %d invisible on %d servers",
                        &usernum,&invnum,&servernum)==3) {
            totalnum=usernum+invnum;
            sprintf(tmpbuf,"%s There are %s%d%s users (%d + %d invisible) on %s%d%s servers",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,totalnum,Colors[COLOFF],usernum,invnum,
                    CmdsColors[COLCSCAN].color2,servernum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
        }
/* for lame 2.9.2 servers */
	else if (sscanf(rest,"There are %d users and %d services on %d servers",
                        &usernum,&servicenum,&servernum)==3) {
            totalnum=usernum;
	    sprintf(tmpbuf,"%s There are %s%d%s users and %s%d%s services on %s%d%s servers",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,usernum,Colors[COLOFF],
                    CmdsColors[COLCSCAN].color2,servicenum,Colors[COLOFF],
                    CmdsColors[COLCSCAN].color2,servernum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
        }
        else if (sscanf(rest,"There are %d users on %d servers",&totalnum,&servernum)==2) {
	    usernum=totalnum;
	    invnum=0;
            sprintf(tmpbuf,"%s There are %s%d%s users on %s%d%s servers",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,totalnum,Colors[COLOFF],
                    CmdsColors[COLCSCAN].color2,servernum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
        }
    }
    else if (comm==252) {
        if (sscanf(rest,"%d IRC Operators online",&opernum)==1) {
            sprintf(tmpbuf,"%s There are %s%d%s IRC Operator(s) online",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,opernum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
        else if (sscanf(rest,"%d operators(s) online",&opernum)==1) {
            sprintf(tmpbuf,"%s There are %s%d%s Operator(s) online",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,opernum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
    else if (comm==254) {
	if (sscanf(rest,"%d channels formed",&channelnum)==1)
	{
            sprintf(tmpbuf,"%s Currently, %s%d%s channels have been formed",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,channelnum,Colors[COLOFF]);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
    else if (comm==255) {
        if (sscanf(rest,"I have %d clients and %d servers",&clientnum,&connectnum)==2) {
            if (totalnum) totalper=(float) (clientnum*100)/totalnum;
            else totalper=0.0;
            sprintf(tmpbuf,"%s Connected are: %s%d%s server(s) and %s%d%s users (apx. %c%.1f%%%c of total users)",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,connectnum,Colors[COLOFF],
                    CmdsColors[COLCSCAN].color2,clientnum,Colors[COLOFF],
                    bold,totalper,bold);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
/* for lame 2.9.2 servers */
        else if (sscanf(rest,"I have %d clients, %d services and %d servers",&clientnum,
                        &servicenum,&connectnum)==3) {
            if (totalnum) totalper=(float) (clientnum*100)/totalnum;
            else totalper=0.0;
            sprintf(tmpbuf,"%s Connected are: %s%d%s server(s) with %s%d%s users and %s%d%s services (apx. %c%.1f%%%c of total users)",
                    numeric_banner(),
                    CmdsColors[COLCSCAN].color2,connectnum,Colors[COLOFF],
                    CmdsColors[COLCSCAN].color2,clientnum,Colors[COLOFF],
                    CmdsColors[COLCSCAN].color2,servicenum,Colors[COLOFF],
                    bold,totalper,bold);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
#else
    if (comm==251) {
	say("Server information for: %s",sname);
	if (sscanf(rest,"There are %d users and %d invisible on %d servers",
                   &usernum,&invnum,&servernum)==3) {
	    totalnum=usernum+invnum;
            sprintf(tmpbuf,"%s There are %d users (%d + %d invisible) on %d servers",
                    numeric_banner(),totalnum,usernum,invnum,servernum);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
/* for lame 2.9.2 servers */
	else if (sscanf(rest,"There are %d users and %d services on %d servers",
                        &usernum,&servicenum,&servernum)==3) {
            totalnum=usernum;
	    sprintf(tmpbuf,"%s There are %d users and %d services on %d servers",
                    numeric_banner(),usernum,servicenum,servernum);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
        else if (sscanf(rest,"There are %d users on %d servers",&totalnum,&servernum)==2) {
	    usernum=totalnum;
	    invnum=0;
            sprintf(tmpbuf,"%s There are %c%d%c users on %c%d%c servers",
                   numeric_banner(),bold,totalnum,bold,bold,servernum,bold);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
    else if (comm==252) {
        if (sscanf(rest,"%d IRC Operators online",&opernum)==1) {
            sprintf(tmpbuf,"%s There are %d IRC Operator(s) online",numeric_banner(),
                    opernum);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
        else if (sscanf(rest,"%d operators(s) online",&opernum)==1) {
            sprintf(tmpbuf,"%s There are %d Operator(s) online",numeric_banner(),opernum);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
    else if (comm==254) {
        if (sscanf(rest,"%d channels formed",&channelnum)==1) {
            sprintf(tmpbuf,"%s Currently, %d channels have been formed",
                   numeric_banner(),channelnum);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
    else if (comm==255) {
        if (sscanf(rest,"I have %d clients and %d servers",&clientnum,&connectnum)==2) {
            if (totalnum) totalper=(float) (clientnum*100)/totalnum;
            else totalper=0.0;
            sprintf(tmpbuf,"%s Connected are: %d server(s) and %d users (apx. %.1f%% of total users)",
                   numeric_banner(),connectnum,clientnum,totalper);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
/* for lame 2.9.2 servers */
        else if (sscanf(rest,"I have %d clients, %d services and %d servers",&clientnum,
                        &servicenum,&connectnum)==3) {
            if (totalnum) totalper=(float) (clientnum*100)/totalnum;
            else totalper=0.0;
            sprintf(tmpbuf,"%s Connected are: %d server(s) with %d users and %d services (apx. %.1f%% of total users)",
                    numeric_banner(),connectnum,clientnum,servicenum,totalper);
            if (from) strcat(tmpbuf,tmpbuf2);
            put_it("%s",tmpbuf);
	}
    }
#endif
/* 
*    else if (comm=265) {
*	if (!sscanf(rest,"Current local  users:  %d  Max: %d",&clientnum,&maxlnum)==2) 
*		put_it("%s %s",numeric_banner(),rest);
*	else put_it("%s %s",numeric_banner(),rest);
*    }
*    else if (comm=266) {
*	if (sscanf(rest,"Current global users:  %d  Max: %d",&totalnum,&maxgnum)==2) 
*        put_it("%s Maximum users:  %d local and %d global",numeric_banner(),
*		maxlnum,maxgnum);
*	else
*		put_it("%s %s",numeric_banner(),rest);
*    }
*/
}
/****************************************************************************/

/*
 * password_sendline: called by send_line() in get_password() to handle
 * hitting of the return key, etc 
 */
static	void
password_sendline(data, line)
	char	*data;
	char	*line;
{
	int	new_server;

	new_server = atoi(line);
	set_server_password(new_server, line);
	connect_to_server(get_server_name(new_server),
		get_server_port(new_server), get_server_nickname(new_server), -1);
}

/*
 * get_password: when a host responds that the user needs to supply a
 * password, it gets handled here!  the user is prompted for a password and
 * then reconnection is attempted with that password.  but, the reality of
 * the situation is that no one really uses user passwords.  ah well 
 */
static	void
get_password()
{
	char	server_num[8];

	say("password required for connection to server %s",
		get_server_name(parsing_server_index));
	close_server(parsing_server_index, empty_string);
        if (!dumb)
	{
		sprintf(server_num, "%d", parsing_server_index);
		add_wait_prompt("Server Password:", password_sendline,
			server_num, WAIT_PROMPT_LINE);
	}
}

/*ARGSUSED*/
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
nickname_sendline(data, nick)
	char	*data;
	char	*nick;
{
	int	new_server, server;

	new_server = atoi(data);
/**************************** PATCHED by Flier ******************************/
/*#if 0*/ /* blundernet */
/****************************************************************************/
	if ((nick = check_nickname(nick)) != NULL)
	{
/**************************** PATCHED by Flier ******************************/
/*#endif*/
/****************************************************************************/
		server = parsing_server_index;
		from_server = new_server;
		send_to_server("NICK %s", nick);
		if (new_server == primary_server)
 			malloc_strcpy(&nickname, nick);
		set_server_nickname(new_server, nick);
		from_server = server;
		already_doing_reset_nickname = 0;
		update_all_status();
/**************************** PATCHED by Flier ******************************/
/*#if 0*/ /* blundernet */
/****************************************************************************/
	}
	else
	{
		say("illegal nickname, try again");
		if (!dumb)
			add_wait_prompt("Nickname: ", nickname_sendline, data,
					WAIT_PROMPT_LINE);
	}
/**************************** PATCHED by Flier ******************************/
/*#endif*/
/****************************************************************************/
}

/*
 * reset_nickname: when the server reports that the selected nickname is not
 * a good one, it gets reset here. 
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
reset_nickname()
{
	char	server_num[10];

	if (already_doing_reset_nickname)
		return;
	say("You have specified an illegal nickname");
	if (!dumb && !get_int_var(NO_ASK_NICKNAME_VAR))
	{
		already_doing_reset_nickname = 1;
		say("Please enter your nickname");
		sprintf(server_num, "%d", parsing_server_index);
		add_wait_prompt("Nickname: ", nickname_sendline, server_num,
			WAIT_PROMPT_LINE);
	}
	update_all_status();
}

/*ARGSUSED*/
static	void
channel_topic(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*topic, *channel;

 	save_message_from();
	if (ArgList[1] && is_channel(ArgList[0]))
	{
		topic = ArgList[1];
		channel = ArgList[0];
		message_from(channel, LOG_CRAP);
		put_it("%s Topic for %s: %s", numeric_banner(), channel,
			topic);
	}
	else
	{
		message_from((char *) 0, LOG_CURRENT);	/* XXX should remove this */
		PasteArgs(ArgList, 0);
		put_it("%s Topic: %s", numeric_banner(), ArgList[0]);
	}
 	restore_message_from();
}

static	void
nickname_in_use(from, ArgList)
	char	*from,
		**ArgList;
{
	PasteArgs(ArgList, 0);
 	if (is_server_connected(parsing_server_index)) {
		if (do_hook(current_numeric, "%s", *ArgList))
			display_msg(from, ArgList);
 	} else if (never_connected || parsing_server_index != primary_server ||
	    !attempting_to_connect)
	{
		if (do_hook(current_numeric, "%s", *ArgList))
			display_msg(from, ArgList);
/**************************** PATCHED by Flier ******************************/
                /*reset_nickname();*/
/****************************************************************************/
	}
	else
	{
		send_to_server("USER %s %s . :%s", username,
			(send_umode && *send_umode) ? send_umode : ".",
			realname);
		send_to_server("NICK %s", get_server_nickname(parsing_server_index));
	}
}

static	void
not_valid_channel(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel;
	char	*s;

	if (!(channel = ArgList[0]) || !ArgList[1])
		return;
	PasteArgs(ArgList, 1);
/**************************** PATCHED by Flier ******************************/
	/*s = get_server_name(parsing_server_index);*/
        s=get_server_itsname(parsing_server_index);
/****************************************************************************/
	if (0 == my_strnicmp(s, from, strlen(s)))
	{
		remove_channel(channel, parsing_server_index);
		put_it("%s %s %s", numeric_banner(), channel, ArgList[1]);
	}
}

/* from ircd .../include/numeric.h */
/*
#define ERR_CHANNELISFULL    471
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define ERR_BADCHANNELKEY    475
#define ERR_BADCHANMASK      476
*/
static	void
cannot_join_channel(from, ArgList)
	char	*from,	
		**ArgList;
{
	char	*chan;
	char	buffer[BIG_BUFFER_SIZE+1];
/**************************** PATCHED by Flier ******************************/
#ifdef ACID
        int     tryjoin=0;
        char    *tmpchan;
        char    tmpbuf[mybufsize/4+1];
#endif
/****************************************************************************/

	if (ArgList[0])
		chan = ArgList[0];
	else
		return;

	if (!is_on_channel(chan, parsing_server_index,
			get_server_nickname(parsing_server_index)))
		remove_channel(chan, parsing_server_index);
	else
		return;

/**************************** PATCHED by Flier ******************************/
        /*PasteArgs(ArgList, 0);
	strcpy(buffer, ArgList[0]);*/
        if (-current_numeric==437) {
            /* special case for ircd 2.9 */
            strcpy(buffer,ArgList[0]);
            strcat(buffer," Sorry, cannot join channel.");
        }
        else {
            /* valid for the rest of numerics covered here */
            PasteArgs(ArgList,0);
            strcpy(buffer,ArgList[0]);
        }
#ifdef ACID
        strmcpy(tmpbuf,buffer,mybufsize/4);
        if ((tmpchan=index(tmpbuf,' '))) *tmpchan='\0';
#endif
/****************************************************************************/
 	if (do_hook(current_numeric, "%s %s", from, *ArgList)) {
		switch(-current_numeric)
		{
/**************************** PATCHED by Flier ******************************/
        	case 437:
			strcat(buffer, " (Channel is temporarily unavailable)");
			break;
/****************************************************************************/
		case 471:
			strcat(buffer, " (Channel is full)");
/**************************** PATCHED by Flier ******************************/
#ifdef ACID
                        tryjoin=1;
#endif
/****************************************************************************/
			break;
		case 473:
			strcat(buffer, " (Invite only channel)");
/**************************** PATCHED by Flier ******************************/
#ifdef ACID
                        tryjoin=1;
#endif
/****************************************************************************/
			break;
		case 474:
			strcat(buffer, " (Banned from channel)");
/**************************** PATCHED by Flier ******************************/
#ifdef ACID
                        tryjoin=1;
#endif
/****************************************************************************/
			break;
		case 475:
			strcat(buffer, " (Bad channel key)");
/**************************** PATCHED by Flier ******************************/
#ifdef ACID
                        tryjoin=1;
#endif
/****************************************************************************/
			break;
		case 476:
			strcat(buffer, " (Bad channel mask)");
			break;
/**************************** PATCHED by Flier ******************************/
		case 477:
			strcat(buffer, " (You must first identify yourself with NickServ to join that channel)");
                        break;
/****************************************************************************/
		}
        	put_it("%s %s", numeric_banner(), buffer);
	}
/**************************** PATCHED by Flier ******************************/
#ifdef ACID
        if (tryjoin) {
            WhowasChanList *whowaschan=check_whowas_chan_buffer(tmpbuf,0);

            if (whowaschan && whowaschan->channellist &&
                whowaschan->channellist->TryRejoin==0) {
                whowaschan->channellist->TryRejoin=1;
                TryChannelJoin();
            }
        }
#endif
/****************************************************************************/
}


/*ARGSUSED*/
static	void
version(from, ArgList)
	char	*from,
		**ArgList;
{
	if (ArgList[2])
	{
		PasteArgs(ArgList, 2);
		put_it("%s Server %s: %s %s", numeric_banner(), ArgList[1],
			ArgList[0], ArgList[2]);
	}
	else
	{
		PasteArgs(ArgList, 1);
		put_it("%s Server %s: %s", numeric_banner(), ArgList[1],
			ArgList[0]);
	}
}

/**************************** PATCHED by Flier ******************************/
/* 
 * CheckSPing() - Checks to see if 351/402 are involved in server ping.
 * If they are, deal with it. If not, pass them on. By Zakath
 *
void CheckSPing(comm,from,ArgList)
int comm;
char *from;
char **ArgList;
{
    char tmpbuf[mybufsize/32];
    char tmpbuf2[mybufsize/32];
#ifndef HAVETIMEOFDAY
    time_t timediff=time((time_t *) 0)-SPingTime;

    if ((comm==351) && SPingTime) {
	sprintf(tmpbuf,"%d second%s",timediff,(timediff==1)?"":"s");

#else
    struct timeval timeofday;

    if ((comm==351) && SPingTime.tv_sec) {
	gettimeofday(&timeofday,NULL);
        timeofday.tv_sec-=SPingTime.tv_sec;
        if (timeofday.tv_usec>=SPingTime.tv_usec) timeofday.tv_usec-=SPingTime.tv_usec;
        else {
            timeofday.tv_usec=timeofday.tv_usec-SPingTime.tv_usec+1000000;
            timeofday.tv_sec--;
        }
        sprintf(tmpbuf2,"%06d",timeofday.tv_usec);
        tmpbuf2[3]='\0';
#ifdef WANTANSI
        sprintf(tmpbuf,"%s%d.%s%s",
                CmdsColors[COLCSCAN].color2,timeofday.tv_sec,tmpbuf2,Colors[COLOFF]);
#else
        sprintf(tmpbuf,"%c%d.%s%c",bold,timeofday.tv_sec,tmpbuf2,bold);
#endif
        strcat(tmpbuf," seconds");
#endif
        
#ifdef WANTANSI
        say("Received server pong from %s%s%s: %s",
            CmdsColors[COLCSCAN].color1,from,Colors[COLOFF],tmpbuf);
#else
        say("Received server pong from %s: %s",from,tmpbuf);
#endif

#ifdef HAVETIMEOFDAY
        SPingTime.tv_sec=0;
        SPingTime.tv_usec=0;
#else
        SPingTime=0;
#endif
    }
    else if (comm==351) version(from,ArgList);
    else if (comm==402) {
#ifdef HAVETIMEOFDAY
	if (SPingTime.tv_sec) {
#else
	if (SPingTime) {
#endif
	    say("Error, no such server to ping");
#ifdef HAVETIMEOFDAY
            SPingTime.tv_sec=0;
            SPingTime.tv_usec=0;
#else
            SPingTime=0;
#endif
	}
        else if (inSZLinks==3) inSZLinks=0;
        else if (do_hook(current_numeric,"%s %s",from,ArgList[0]))
            display_msg(from,ArgList);
    }
}*/
/****************************************************************************/

/*ARGSUSED*/
static	void
invite(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*who,
		*channel;

	if ((who = ArgList[0]) && (channel = ArgList[1]))
	{
 		save_message_from();
		message_from(channel, LOG_CRAP);
		if (do_hook(current_numeric, "%s %s %s", from, who, channel))
			put_it("%s Inviting %s to channel %s",
					numeric_banner(), who, channel);
 		restore_message_from();
	}
}


/*
 * numbered_command: does (hopefully) the right thing with the numbered
 * responses from the server.  I wasn't real careful to be sure I got them
 * all, but the default case should handle any I missed (sorry) 
 */
void
numbered_command(from, comm, ArgList)
	char	*from;
	int	comm;
	char	**ArgList;
{
	char	*user;
	char	none_of_these = 0;
	char	blah[BIG_BUFFER_SIZE+1];
	int	flag,
		lastlog_level;
#if 0
	int	user_cnt,
		inv_cnt,
		server_cnt;
#endif
/**************************** Patched by Flier ******************************/
        int     skipit=0;
        char    *tmpnick; /* userhost() on 433 by Zakath */
        char    tmpbuf[(mybufsize/4)+1];
        ChannelList *chan;
/****************************************************************************/

	if (!from || !*from)
		return;
	if (!*ArgList[0])
		user = (char *) 0;
	else
		user = ArgList[0];
	if (!ArgList[1])
		return;
	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
 	save_message_from();
	message_from((char *) 0, LOG_CRAP);
	ArgList++;
	current_numeric = -comm;	/* must be negative of numeric! */
	switch (comm)
	{
	case 001:	/* #define RPL_WELCOME          001 */
		PasteArgs(ArgList, 0);
                if (do_hook(current_numeric, "%s %s", from, *ArgList)) 
			display_msg(from, ArgList);
		clean_whois_queue();
		break;
	case 002:	/* #define RPL_YOURHOST         002 */
		PasteArgs(ArgList, 0);
		sprintf(blah, "*** %s", ArgList[0]);
		got_initial_version(blah);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;

/* should do something with this some day, 2.8 had channel/user mode switches */
	case 004:	/* #define RPL_MYINFO           004 */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;

/*
 * this part of ircii has been broken for most of ircd 2.7, so someday I'll
 * make it work for ircd 2.8 ...  phone..
 */
#if 0
	case 251:		/* #define RPL_LUSERCLIENT      251 */
		display_msg(from, ArgList);
		if (server_list[from_server].connected)
			break;
		if ((from_server == primary_server) && ((sscanf(ArgList[1],
		    "There are %d users and %d invisible on %d servers",
		    &user_cnt, &inv_cnt, &server_cnt) == 3)||(sscanf(ArgList[1],
		    "There are %d users and %d invisible on %d servers",
		    &user_cnt, &inv_cnt, &server_cnt) == 3)))
		{
			user_cnt =+ inv_cnt;
			if ((server_cnt < get_int_var(MINIMUM_SERVERS_VAR)) ||
			    (user_cnt < get_int_var(MINIMUM_USERS_VAR)))
			{
				say("Trying better populated server...");
				get_connected(from_server + 1, 0);
			}
		}
 		break;
#endif
/**************************** PATCHED by Flier ******************************/
/* Patched by Zakath */
/*	case 250:
*		display_luser(comm, from, ArgList);
*		break;
*/
        case 251:
		display_luser(comm,from,ArgList);
		break;
	case 252:
		display_luser(comm,from,ArgList);
		break;
	case 254:
		display_luser(comm,from,ArgList);
		break;
	case 255:
		display_luser(comm,from,ArgList);
		break;
/*
*	case 265:
*		display_luser(comm,from,ArgList);
*		break;
*	case 266:
*		display_luser(comm,from,ArgList);
*		break;
*/
/* ***************** */
/****************************************************************************/
	case 301:		/* #define RPL_AWAY             301 */
		user_is_away(from, ArgList);
		break;

	case 302:		/* #define RPL_USERHOST         302 */
		userhost_returned(from, ArgList);
		break;

	case 303:		/* #define RPL_ISON             303 */
		ison_returned(from, ArgList);
		break;

/**************************** PATCHED by Flier ******************************/
/* Patched by Zakath */
#ifdef CELE
        case 306:		/* #define RPL_SETAWAY */
                if (!SentAway) say("%s",ArgList[0]);
                else SentAway--;
                break;
#endif
/* ******* ** ****** */
/****************************************************************************/
	case 311:		/* #define RPL_WHOISUSER        311 */
		whois_name(from, ArgList);
		break;

	case 312:		/* #define RPL_WHOISSERVER      312 */
		whois_server(from, ArgList);
		break;

	case 313:		/* #define RPL_WHOISOPERATOR    313 */
		whois_oper(from, ArgList);
		break;

	case 314:		/* #define RPL_WHOWASUSER       314 */
		whowas_name(from, ArgList);
		break;

	case 316:		/* #define RPL_WHOISCHANOP      316 */
		whois_chop(from, ArgList);
		break;

	case 317:		/* #define RPL_WHOISIDLE        317 */
		whois_lastcom(from, ArgList);
		break;

	case 318:		/* #define RPL_ENDOFWHOIS       318 */
		end_of_whois(from, ArgList);
		break;

	case 319:		/* #define RPL_WHOISCHANNELS    319 */
		whois_channels(from, ArgList);
		break;

	case 321:		/* #define RPL_LISTSTART        321 */
		ArgList[0] = "Channel\0Users\0Topic";
		ArgList[1] = ArgList[0] + 8;
		ArgList[2] = ArgList[1] + 6;
		ArgList[3] = (char *) 0;
		funny_list(from, ArgList);
		break;

	case 322:		/* #define RPL_LIST             322 */
		funny_list(from, ArgList);
		break;

	case 324:		/* #define RPL_CHANNELMODEIS    324 */
		funny_mode(from, ArgList);
		break;

	case 341:		/* #define RPL_INVITING         341 */
		invite(from, ArgList);
		break;

	case 352:		/* #define RPL_WHOREPLY         352 */
		whoreply((char *) 0, ArgList);
		break;

	case 353:		/* #define RPL_NAMREPLY         353 */
		funny_namreply(from, ArgList);
		break;

	case 366:		/* #define RPL_ENDOFNAMES       366 */
		{
			char	*tmp = (char *) 0,
				*chan;
			ChannelList	*ptr = (ChannelList *) 0;

			PasteArgs(ArgList, 0);
			malloc_strcpy(&tmp, ArgList[0]);
			chan = strtok(tmp," ");
			flag = do_hook(current_numeric, "%s %s", from, ArgList[0]);

			if ((ptr = lookup_channel(chan, from_server, CHAN_NOUNLINK)) && !((ptr->status & CHAN_NAMES) && (ptr->status & CHAN_MODE)))
				if (get_int_var(SHOW_END_OF_MSGS_VAR) && flag)
					display_msg(from, ArgList);

			new_free(&tmp);
		}
		break;

	case 381: 		/* #define RPL_YOUREOPER        381 */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		set_server_operator(parsing_server_index, 1);
		update_all_status();	/* fix the status line */
		break;

	case 401:		/* #define ERR_NOSUCHNICK       401 */
/************************** PATCHED by Flier *****************************/
		/*no_such_nickname(from, ArgList);*/
                RemoveFromDCCList(ArgList[0]);
                if (!server_list[parsing_server_index].SZWI)
                    no_such_nickname(from, ArgList);
                else server_list[parsing_server_index].SZWI--;
                break;
/*************************************************************************/		

 	case 405:		/* #define ERR_TOOMANYCHANNELS  405 */
/************************** PATCHED by Flier *****************************/
 		/*remove_channel(ArgList[0], parsing_server_index);*/
                PasteArgs(ArgList,0);
                strmcpy(tmpbuf,ArgList[0],mybufsize/4);
                tmpnick=tmpbuf;
                next_arg(tmpnick,&tmpnick);
                PurgeChannel(tmpbuf,parsing_server_index);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
                    display_msg(from, ArgList);
/*************************************************************************/		
		break;

	case 421:		/* #define ERR_UNKNOWNCOMMAND   421 */
		if (check_screen_redirect(ArgList[0]))
			break;
		if (check_wait_command(ArgList[0]))
			break;
		PasteArgs(ArgList, 0);
		flag = do_hook(current_numeric, "%s %s", from, *ArgList);
		if (!strncmp("ISON", *ArgList, 4) || !strncmp("USERHOST",
		    *ArgList, 8))
		{
			set_server_2_6_2(parsing_server_index, 0);
			convert_to_whois();
		}
		else if (flag)
			display_msg(from, ArgList);
		break;

	case 432:		/* #define ERR_ERRONEUSNICKNAME 432 */
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
/**************************** Patched by Flier ******************************/
		/*reset_nickname();*/
                AutoChangeNick(ArgList[0]);
/****************************************************************************/
		break;

	case 433:		/* #define ERR_NICKNAMEINUSE    433 */ 
                nickname_in_use(from, ArgList);
/**************************** Patched by Flier ******************************/
		/*reset_nickname();*/
                strmcpy(tmpbuf,ArgList[0],mybufsize/4);
                if (server_list[parsing_server_index].connected) {
                    tmpnick=tmpbuf;
                    userhost(NULL,next_arg(tmpnick,&tmpnick),NULL); /* uhost - Zakath */
                }
                else AutoChangeNick(tmpbuf);
/****************************************************************************/
		break;
        case 437:		/* Nickname/channel temp. unavailable */
		if (!is_channel(*ArgList))
		{
			nickname_in_use(from, ArgList);
/**************************** PATCHED by Flier ******************************/
                        /*reset_nickname();*/
                        if (!(server_list[parsing_server_index].connected))
                            AutoChangeNick(ArgList[0]);
/****************************************************************************/
                }
/**************************** PATCHED by Flier ******************************/
                else cannot_join_channel(from,ArgList);
/****************************************************************************/
		break;

	case 463:		/* #define ERR_NOPERMFORHOST    463 */
		display_msg(from, ArgList);
		close_server(parsing_server_index, empty_string);
		window_check_servers();
		if (!connected_to_server)
			get_connected(parsing_server_index + 1, 0);
		break;

	case 464:		/* #define ERR_PASSWDMISMATCH   464 */
		PasteArgs(ArgList, 0);
		flag = do_hook(current_numeric, "%s %s", from, ArgList[0]);
		if (oper_command)
		{
			if (flag)
				display_msg(from, ArgList);
		}
		else
			get_password();
		break;

	case 465:		/* #define ERR_YOUREBANNEDCREEP 465 */
		{
			int	klined_server = parsing_server_index;

			PasteArgs(ArgList, 0);
			if (do_hook(current_numeric, "%s %s", from, ArgList[0]))
				display_msg(from, ArgList);
			close_server(parsing_server_index, empty_string);
                        window_check_servers();
                        if (number_of_servers > 1)
			{
                                remove_from_server_list(klined_server);
			}
			if (!connected_to_server)
				say("You are not connected to a server. Use /SERVER to connect.");
			break;
		}

 	case 471:		/* #define ERR_CHANNELISFULL    471 */
 	case 473:		/* #define ERR_INVITEONLYCHAN   473 */
 	case 474:		/* #define ERR_BANNEDFROMCHAN   474 */
 	case 475: 		/* #define ERR_BADCHANNELKEY    475 */
 	case 476:		/* #define ERR_BADCHANMASK      476 */
/**************************** PATCHED by Flier ******************************/
 	case 477:
/****************************************************************************/
 		cannot_join_channel(from, ArgList);
 		break;

/**************************** PATCHED by Flier ******************************/
	case 472:
	case 482:
                /* 
                 * Don't display if we're joining a channel since it
                 * most likely means it came from MODE #channel e
                 */
                for (chan=server_list[from_server].chan_list;chan;chan=chan->next)
                    if (!(chan->gotbans) || !(chan->gotwho)) break;
		if (!chan && do_hook(current_numeric, "%s %s", from, *ArgList))
                    display_msg(from, ArgList);
                break;
/****************************************************************************/

	case 484:		/* #define ERR_RESTRICTED       484 */
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
/**************************** PATCHED by Flier ******************************/
                set_server_umode_flag(parsing_server_index,'r',1);
/****************************************************************************/
		break;

		/*
		 * The following accumulates the remaining arguments
		 * in ArgSpace for hook detection.  We can't use
		 * PasteArgs here because we still need the arguments
		 * separated for use elsewhere.
		 */
	default:
		{
			char	*ArgSpace = (char *) 0;
			int	i,
				do_message_from = 0;
 			size_t	len;

			for (i = len = 0; ArgList[i]; len += strlen(ArgList[i++]))
				;
			len += (i - 1);
			ArgSpace = new_malloc(len + 1);
			ArgSpace[0] = '\0';
			/* this is cheating */
			if (ArgList[0] && is_channel(ArgList[0]))
				do_message_from = 1;
			for (i = 0; ArgList[i]; i++)
			{
				if (i)
					strcat(ArgSpace, " ");
				strcat(ArgSpace, ArgList[i]);
			}
			if (do_message_from)
				message_from(ArgList[0], LOG_CRAP);
/**************************** PATCHED by Flier ******************************/
                        if (comm==332 && ArgList[0] && ArgList[1] &&
                            (chan=lookup_channel(ArgList[0],from_server,0)))
                            malloc_strcpy(&(chan->topicstr),ArgList[1]);
                        if (comm==315 && server_list[from_server].SZWho) {
#ifdef OPER
                            if (inSZFKill) HandleEndOfKill();
                            else HandleEndOfWho(ArgList[0],from_server);
#else
                            HandleEndOfWho(ArgList[0],from_server);
#endif
                            server_list[from_server].SZWho--;
                            skipit=1;
                        }
                        else if (comm==368 && server_list[from_server].SZUnban) {
                            EndOfBans(ArgList[0],from_server);
                            skipit=1;
                        }
                        else if ((comm==367 || comm==348) && server_list[from_server].SZUnban) {
                            OnBans(ArgList,comm==348?1:0);
                            skipit=1;
                        }
                        else
/****************************************************************************/
 			i = do_hook(current_numeric, "%s %s", from, ArgSpace);
			new_free(&ArgSpace);
 			if (do_message_from)
 				restore_message_from();
/**************************** PATCHED by Flier ******************************/
                        /*
                         * This is fix for lost messages! We can't call
                         * restore_message_from without first calling
                         * save_message_from
                         */
 			if (do_message_from)
                            save_message_from();
/****************************************************************************/
 			if (i == 0)
 				goto done;
/**************************** PATCHED by Flier ******************************/
                        /*none_of_these = 1;*/
                        if (!skipit) none_of_these=1;
/****************************************************************************/
		}
	}
	/* the following do not hurt the ircII if intercepted by a hook */
	if (none_of_these)
	{
		switch (comm)
		{
		case 221: 		/* #define RPL_UMODEIS          221 */
			put_it("%s Your user mode is \"%s\"", numeric_banner(),
				ArgList[0]);
			break;

		case 242:		/* #define RPL_STATSUPTIME      242 */
			PasteArgs(ArgList, 0);
			if (from && !my_strnicmp(get_server_itsname(parsing_server_index),
			    from, strlen(get_server_itsname(parsing_server_index))))
				from = NULL;
			if (from)
				put_it("%s %s from (%s)", numeric_banner(),
					*ArgList, from);
			else
				put_it("%s %s", numeric_banner(), *ArgList);
			break;

		case 332:		/* #define RPL_TOPIC            332 */
			channel_topic(from, ArgList);
			break;

/**************************** PATCHED by Flier ******************************/
#ifdef OPER
                case 215:
                        PasteArgs(ArgList,2);
                        HandleStatsI(ArgList[0],ArgList[1],ArgList[2]);
                        break;
                case 216:
                        PasteArgs(ArgList,2);
                        HandleStatsK(ArgList[1],ArgList[2]);
                        break;
#endif
                case 329:
                        ChannelCreateTime(from,ArgList);
                        break;
                case 333:
                        TimeReply(from,ArgList);
                        break;
                case 422:
                        if (!usersloaded) {
                            ScrollZLoad();
                            if (PlistTime<7200 || NlistTime<7200)
                                CdccTimeWarning();
                        }
                        break;
/****************************************************************************/

		case 351:		/* #define RPL_VERSION          351 */
			version(from, ArgList);
			break;

		case 364:		/* #define RPL_LINKS            364 */
			if (ArgList[2])
			{
/**************************** PATCHED by Flier ******************************/
				/*PasteArgs(ArgList, 2);
				put_it("%s %-20s %-20s %s", numeric_banner(),
					ArgList[0], ArgList[1], ArgList[2]);*/
                                if (inSZLinks==3) PrintLinks(ArgList[0],ArgList[1],ArgList[2]);
#ifndef LITE
                                else if (inSZLinks==4) AddToMap(ArgList[0],ArgList[2]);
#endif
#ifdef EXTRAS
                                else if (inSZLinks) {
                                    sprintf(tmpbuf,"%-20s %-20s",ArgList[0],ArgList[1]);
                                    HandleLinks(tmpbuf);
                                }
#endif
                                else put_it("%s %-20s %-20s %s", numeric_banner(),
                                            ArgList[0], ArgList[1], ArgList[2]);
/****************************************************************************/
			}
			else
			{
/**************************** PATCHED by Flier ******************************/
				/*PasteArgs(ArgList, 1);
				put_it("%s %-20s %s", numeric_banner(),
					ArgList[0], ArgList[1]);*/
                                if (inSZLinks==3) PrintLinks(ArgList[0],ArgList[1],"");
#ifndef LITE
                                else if (inSZLinks==4) AddToMap(ArgList[0],"");
#endif
#ifdef EXTRAS
                                else if (inSZLinks) {
                                    sprintf(tmpbuf,"%-20s",ArgList[0]);
                                    HandleLinks(tmpbuf);
                                }
#endif
                                else put_it("%s %-20s %s", numeric_banner(),
                                            ArgList[0], ArgList[1]);
/****************************************************************************/
			}
			break;
		case 372:		/* #define RPL_MOTD             372 */
/**************************** PATCHED by Flier ******************************/
		case 377:		/* #define RPL_FORCE_MOTD       377 */
/****************************************************************************/
			if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(parsing_server_index))
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s", numeric_banner(), ArgList[0]);
			}
			break;

		case 375:		/* #define RPL_MOTDSTART        375 */
			if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(parsing_server_index))
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s", numeric_banner(), ArgList[0]);
			}
			break;

		case 376:		/* #define RPL_ENDOFMOTD        376 */
 			if (attempting_to_connect)
 				got_initial_version("*** Your host is broken and not running any version");
			if (get_int_var(SHOW_END_OF_MSGS_VAR) &&
			    (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(parsing_server_index)))
			{
				PasteArgs(ArgList, 0);
				put_it("%s %s", numeric_banner(), ArgList[0]);
			}
			set_server_motd(parsing_server_index, 0);
/**************************** PATCHED by Flier ******************************/
                        if (!usersloaded) {
                            ScrollZLoad();
                            if (PlistTime<7200 || NlistTime<7200)
                                CdccTimeWarning();
                        }
/****************************************************************************/
			break;

		case 384:		/* #define RPL_MYPORTIS         384 */
			PasteArgs(ArgList, 0);
			put_it("%s %s %s", numeric_banner(), ArgList[0], user);
			break;

		case 385:		/* #define RPL_NOTOPERANYMORE   385 */
			set_server_operator(parsing_server_index, 0);
			display_msg(from, ArgList);
			update_all_status();
			break;
                        
/**************************** PATCHED by Flier ******************************/
		case 402:		/* #define ERR_NOSUCHSERVER     402 */
			NoSuchServer4SPing(from,ArgList);
			break;
/****************************************************************************/

		case 403:		/* #define ERR_NOSUCHCHANNEL    403 */
			not_valid_channel(from, ArgList);
			break;

/**************************** PATCHED by Flier ******************************/
		case 441:
		case 443:
                        if (ArgList[1]) {
                            strmcpy(tmpbuf,ArgList[1],mybufsize/4);
                            PasteArgs(ArgList,0);
                            if (do_hook(current_numeric, "%s %s", from,*ArgList)) {
                                if ((double_ignore(tmpbuf,NULL,IGNORE_CRAP))==IGNORED)
                                    break;
                                display_msg(from,ArgList);
                            }
                        }
			break;
/****************************************************************************/

		case 451:		/* #define ERR_NOTREGISTERED    451 */
	/*
	 * Sometimes the server doesn't catch the USER line, so
	 * here we send a simplified version again  -lynx 
	 */
			send_to_server("USER %s %s . :%s", username,
				(send_umode && *send_umode) ? send_umode : ".",
				realname);
			send_to_server("NICK %s",
				get_server_nickname(parsing_server_index));
			break;

		case 462:		/* #define ERR_ALREADYREGISTRED 462 */
			display_msg(from, ArgList);
			break;

#define RPL_CLOSEEND         363
#define RPL_SERVLISTEND      235
		case 315:		/* #define RPL_ENDOFWHO         315 */
			if (cannot_open != (char *) 0)
				yell("Cannot open: %s", cannot_open);

		case 323:               /* #define RPL_LISTEND          323 */
			funny_print_widelist();

		case 219:		/* #define RPL_ENDOFSTATS       219 */
		case 232:		/* #define RPL_ENDOFSERVICES    232 */
		case 365:		/* #define RPL_ENDOFLINKS       365 */
/**************************** PATCHED by Flier ******************************/
                        if (comm==365 && inSZLinks) {
#ifdef EXTRAS
                            if (inSZLinks==1) say("LLook is now complete");
                            else if (inSZLinks==2) ListSplitedServers();
                            else
#endif
#ifndef LITE
                            if (inSZLinks==4) PrintMap();
                            else
#endif
                            if (LinksNumber) {

/****** Coded by Zakath ******/
#ifdef WANTANSI
                                if (get_int_var(HIGH_ASCII_VAR))
                                    say("%sÀÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÙ   ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ%s",
                                        CmdsColors[COLLINKS].color5,Colors[COLOFF]);
                                else
                                    say("%s`----' `--------------------------' `----'   `-------------------------'%s",
                                        CmdsColors[COLLINKS].color5,Colors[COLOFF]);
#endif
/*****************************/
                            }
                            inSZLinks=0;
                            LinksNumber=0;
                            break;
                        }
                case 262:		/* #define RPL_ENDOFTRACE       262 */
                        if (comm==262 && inSZTrace) {
#ifdef OPER
                            if (inSZTrace==2)
                                say("Matched %d out of %d entries",mattcount,tottcount);
#endif
                            inSZTrace=0;
                            break;
                        }
/****************************************************************************/
		case 368:		/* #define RPL_ENDOFBANLIST     368 */
		case 369:		/* #define RPL_ENDOFWHOWAS      369 */
		case 374:		/* #define RPL_ENDOFINFO        374 */
#if 0	/* this case needs special handing - see above */
		case 376:		/* #define RPL_ENDOFMOTD        376 */
#endif
		case 394:		/* #define RPL_ENDOFUSERS       394 */
/**************************** PATCHED by Flier ******************************/
		case 349:               /* end of exception list */
/****************************************************************************/
			if (!get_int_var(SHOW_END_OF_MSGS_VAR))
				break;
/**************************** PATCHED by Flier ******************************/
                case 203:/* moved here because we need to show trace stuff
                            when we're not doing trace kill */
                case 204:
                case 206:
                        if (comm==203 || comm==204 || comm==206) {
                            if (inSZTrace!=2 && !inSZFKill) inSZTrace=0;
                            if (inSZTrace) break;
                        }
#ifdef CELE
                        HandleTrace(comm,ArgList[0],ArgList[1],ArgList[2],ArgList[3],ArgList[4],ArgList[5]);
                        break;
#endif
                case 205:
#ifdef OPER
                        if (comm==205 && inSZTrace) {
                            if (inSZTrace==2) DoFilterTrace(ArgList[2]);
                            else
                                DoTraceKill(ArgList[2]);
                            break;
                        }
#endif
#ifdef CELE
                        HandleTrace(comm,ArgList[0],ArgList[1],ArgList[2],ArgList[3],ArgList[4],ArgList[5]);
                        break;
#endif
                case 209:
#ifdef OPER
                        if (comm==209 && inSZTrace) {
                            if (inSZTrace==1) HandleEndOfTraceKill();
                            break;
                        }
#endif
#ifdef CELE
                        HandleTrace(comm,ArgList[0],ArgList[1],ArgList[2],ArgList[3],ArgList[4],ArgList[5]);
                        break;
#endif
/****************************************************************************/
		default:
			display_msg(from, ArgList);
		}
	}
	set_lastlog_msg_level(lastlog_level);
done:
 	restore_message_from();
}
