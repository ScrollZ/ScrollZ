/*
 * ctcp.c: handles the client-to-client protocol(ctcp).
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
 * $Id: ctcp.c,v 1.10 1998-11-21 16:12:17 f Exp $
 */

#include "irc.h"

#ifndef _Windows
#include <pwd.h>
#endif /* _Windows */

#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif

#include "ircaux.h"
#include "hook.h"
#include "crypt.h"
#include "ctcp.h"
#include "vars.h"
#include "server.h"
#include "status.h"
#include "lastlog.h"
#include "ignore.h"
#include "output.h"
#include "window.h"
#include "dcc.h"
#include "names.h"
#include "parse.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"
#include "flood.h"
#include "screen.h"
#include "ircterm.h"
#ifdef HAVETIMEOFDAY
#include <sys/time.h>
#include <unistd.h>
#endif

static char tmpaway[mybufsize/2];

/* Patched by Zakath */
#ifdef CELE
extern char *CelerityVersion;
#endif
/* ***************** */

/* patched by acidflash */
#ifdef ACID
extern char *AcidVersion;
#endif

extern char *chars;
#ifdef IPCHECKING
extern char global_track[];
#endif
extern char VersionInfo[];

extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern struct friends *CheckUsers _((char *, char *));
extern void UnbanIt _((char *, char *, int));
extern void AwaySave _((char *, int));
extern void MangleString _((char *, char *, int));
extern void CheckCdcc _((char *, char *, char *, int));
extern void ColorUserHost _((char *, char *, char *, int));
extern char *YNreply _((int));
extern void MangleVersion _((char *));
extern void EncryptString _((char *, char *, char *, int));
/****************************************************************************/

#define	CTCP_SHUTUP	0
#define	CTCP_VERBOSE	1
#define	CTCP_NOREPLY	2

static	char	FAR CTCP_Reply_Buffer[BIG_BUFFER_SIZE + 1] = "";

static	void	do_new_notice_ctcp _((char *, char *, char **, char *));

/* forward declarations for the built in CTCP functions */
static	char	*do_sed _((CtcpEntry *, char *, char *, char *));
static	char	*do_version _((CtcpEntry *, char *, char *, char *));
static	char	*do_clientinfo _((CtcpEntry *, char *, char *, char *));
static	char	*do_echo _((CtcpEntry *, char *, char *, char *));
static	char	*do_userinfo _((CtcpEntry *, char *, char *, char *));
static	char	*do_finger _((CtcpEntry *, char *, char *, char *));
static	char	*do_time _((CtcpEntry *, char *, char *, char *));
static	char	*do_atmosphere _((CtcpEntry *, char *, char *, char *));
static	char	*do_dcc _((CtcpEntry *, char *, char *, char *));
static	char	*do_utc _((CtcpEntry *, char *, char *, char *));
/**************************** Patched by Flier  *****************************/
static  char    *do_invite _((CtcpEntry *, char *, char *, char *));
static  char    *do_op _((CtcpEntry *, char *, char *, char *));
static  char    *do_unban _((CtcpEntry *, char *, char *, char *));
static  char    *do_chops _((CtcpEntry *, char *, char *, char *));
static  char    *do_voice _((CtcpEntry *, char *, char *, char *));
static  char    *do_whoami _((CtcpEntry *, char *, char *, char *));
static  char    *do_help _((CtcpEntry *, char *, char *, char *));
char            *do_cdcc _((CtcpEntry *, char *, char *, char *));
#ifndef CELE
/* Patched by BiGhEaD */
static  char    *do_open _((CtcpEntry *, char *, char *, char *));
#endif
#ifdef CTCPPAGE
static  char    *do_page _((CtcpEntry *, char *, char*, char *)); 
#endif
/**********************/
/****************************************************************************/

static CtcpEntry ctcp_cmd[] =
{
	{ "SED",	"contains simple_encrypted_data",
		CTCP_SHUTUP | CTCP_NOREPLY, do_sed },
	{ "VERSION",	"shows client type, version and environment",
		CTCP_VERBOSE, do_version },
	{ "CLIENTINFO",	"gives information about available CTCP commands",
		CTCP_VERBOSE, do_clientinfo },
	{ "USERINFO",	"returns user settable information",
		CTCP_VERBOSE, do_userinfo },
	{ "ERRMSG",	"returns error messages",
		CTCP_VERBOSE, do_echo },
	{ "FINGER",	"shows real name, login name and idle time of user",
		CTCP_VERBOSE, do_finger },
	{ "TIME",	"tells you the time on the user's host",
		CTCP_VERBOSE, do_time },
	{ "ACTION",	"contains action descriptions for atmosphere",
		CTCP_SHUTUP, do_atmosphere },
	{ "DCC",	"requests a direct_client_connection",
		CTCP_SHUTUP | CTCP_NOREPLY, do_dcc },
	{ "UTC",	"substitutes the local timezone",
		CTCP_SHUTUP | CTCP_NOREPLY, do_utc },
	{ "PING", 	"returns the arguments it receives",
		CTCP_VERBOSE, do_echo },
        { "ECHO", 	"returns the arguments it receives",
/**************************** PATCHED by Flier ******************************/
                /*CTCP_VERBOSE, do_echo }*/
                CTCP_VERBOSE, do_echo },
        { "INVITE",     "invites user to a channel",
                CTCP_SHUTUP , do_invite },
        { "OP",         "ops user on a channel",
                CTCP_SHUTUP , do_op },
        { "UNBAN",      "unbans user on a channel",
                CTCP_SHUTUP , do_unban },
        { "CHOPS",      "lists channel operators",
                CTCP_SHUTUP , do_chops },
        { "VOICE",      "voices user on a channel",
                CTCP_SHUTUP , do_voice },
        { "WHOAMI",     "gives user info on his access",
                CTCP_SHUTUP , do_whoami },
        { "HELP",       "gives user CTCP usage",
                CTCP_SHUTUP , do_help },
        { "VER",        "shows client version",
                CTCP_VERBOSE, do_version },
        { "CDCC",       "xdcc clone",
                CTCP_SHUTUP , do_cdcc },
        { "XDCC",       "xdcc clone",
                CTCP_SHUTUP , do_cdcc },
#ifndef CELE
/* Patched by BiGhEaD */
        { "OPEN",       "opens channel (-lk)",
                CTCP_SHUTUP , do_open },
#endif
#ifdef CTCPPAGE
        { "PAGE",       "pages user",
                CTCP_SHUTUP, do_page }
#endif 
/**********************/
/****************************************************************************/
};

char	*ctcp_type[] =
{
	"PRIVMSG",
	"NOTICE"
};

static	char	FAR ctcp_buffer[BIG_BUFFER_SIZE + 1];

/* This is set to one if we parsed an SED */
int     sed = 0;

/*
 * in_ctcp_flag is set to true when IRCII is handling a CTCP request.  This
 * is used by the ctcp() sending function to force NOTICEs to be used in any
 * CTCP REPLY 
 */
int	in_ctcp_flag = 0;

/*
 * quote_it: This quotes the given string making it sendable via irc.  A
 * pointer to the length of the data is required and the data need not be
 * null terminated (it can contain nulls).  Returned is a malloced, null
 * terminated string.   
 */
char	*
ctcp_quote_it(str, len)
	char	*str;
	int	len;
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*ptr;
	int	i;

	ptr = buffer;
	for (i = 0; i < len; i++)
	{
		switch (str[i])
		{
		case CTCP_DELIM_CHAR:
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = 'a';
			break;
		case '\n':
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = 'n';
			break;
		case '\r':
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = 'r';
			break;
		case CTCP_QUOTE_CHAR:
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = CTCP_QUOTE_CHAR;
			break;
		case '\0':
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = '0';
			break;
		default:
			*(ptr++) = str[i];
			break;
		}
	}
	*ptr = '\0';
	str = (char *) 0;
	malloc_strcpy(&str, buffer);
	return (str);
}

/*
 * ctcp_unquote_it: This takes a null terminated string that had previously
 * been quoted using ctcp_quote_it and unquotes it.  Returned is a malloced
 * space pointing to the unquoted string.  NOTE: a trailing null is added for
 * convenied, but the returned data may contain nulls!.  The len is modified
 * to contain the size of the data returned. 
 */
char	*
ctcp_unquote_it(str, len)
	char	*str;
	int	*len;
{
	char	*buffer;
	char	*ptr;
	char	c;
	int	i,
		new_size = 0;

	buffer = (char *) new_malloc(sizeof(char) * *len);
	ptr = buffer;
	i = 0;
	while (i < *len)
	{
		if ((c = str[i++]) == CTCP_QUOTE_CHAR)
		{
			switch (c = str[i++])
			{
			case CTCP_QUOTE_CHAR:
				*(ptr++) = CTCP_QUOTE_CHAR;
				break;
			case 'a':
				*(ptr++) = CTCP_DELIM_CHAR;
				break;
			case 'n':
				*(ptr++) = '\n';
				break;
			case 'r':
				*(ptr++) = '\r';
				break;
			case '0':
				*(ptr++) = '\0';
				break;
			default:
				*(ptr++) = c;
				break;
			}
		}
		else
			*(ptr++) = c;
		new_size++;
	}
	*ptr = '\0';
	*len = new_size;
	return (buffer);
}

/************************** PATCHED by Flier **************************/
static int dropit() {
    if (server_list[parsing_server_index].ctcp_last_reply_time+3>time((time_t *) 0))
        return(1);
    return(0);
}

static int request(ctcp,mynick,nick,userhost,channel,privs,required)
char *ctcp;
char *mynick;
char *nick;
char *userhost;
char *channel;
int  privs;
int  required;
{
    int access=privs&required;
#ifdef WANTANSI
    char tmpbuf1[mybufsize/2];

    ColorUserHost(userhost,CmdsColors[COLCTCP].color2,tmpbuf1,1);
    sprintf(tmpaway,"%s%s%s request received from %s%s%s %s",
            CmdsColors[COLCTCP].color4,ctcp,Colors[COLOFF],
            CmdsColors[COLCTCP].color1,nick,Colors[COLOFF],tmpbuf1);
    if (channel && *channel) {
        sprintf(tmpbuf1," to %s%s%s",
                CmdsColors[COLCTCP].color3,channel,Colors[COLOFF]);
        strcat(tmpaway,tmpbuf1);
    }
#else
    sprintf(tmpaway,"%s request received from %s (%s)",ctcp,nick,userhost);
    if (channel && *channel) {
        strcat(tmpaway," to ");
        strcat(tmpaway,channel);
    }
#endif
    if (!(channel && *channel) && access && required && required!=FLALL) {
        if (!CTCPCloaking)
            send_to_server("NOTICE %s :Usage  /CTCP %s %s [#]channel [password]",
                           nick,mynick,ctcp);
        strcat(tmpaway,", wrong usage");
        return(-1);
    }
    else if (required && ((required!=FLALL && access!=required) ||
                          (required==FLALL && !access))) {
        strcat(tmpaway,", no access");
        if (!CTCPCloaking) send_to_server("NOTICE %s :You do not have access... -ScrollZ-",nick);
        return(0);
    }
    return(privs);
}

static void wrongpassword(nick)
char *nick;
{
    strcat(tmpaway,", wrong password");
    send_to_server("NOTICE %s :Invalid password!  -ScrollZ-",nick);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
}

static void notchanop(nick,channel)
char *nick;
char *channel;
{
    send_to_server("NOTICE %s :I'm not opped on channel %s -ScrollZ-",nick,channel);
}

static void disabled(nick)
char *nick;
{
    send_to_server("NOTICE %s :This function has been disabled  -ScrollZ-",nick);
}

static int checkpassword(tmpfriend,passwd)
struct friends *tmpfriend;
char *passwd;
{
    char passbuf[mybufsize/8];

    if (tmpfriend->passwd) {
        if (passwd && *passwd) {
            EncryptString(passbuf,passwd,passwd,mybufsize/16);
            return((!strcmp(passbuf,tmpfriend->passwd))?0:1);
        }
        return(1);
    }
    return(0);
}

/* Invites registered user under CTCP request to specified channel */
static char *do_invite(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char *channel=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpchan[mybufsize/2];
    ChannelList *chan;
    struct friends *tmpfriend=NULL;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    if (*args) {
        channel=next_arg(args,&args);
        passwd=next_arg(args,&args);
        if (channel && *channel) {
            if (!is_channel(channel)) sprintf(tmpchan,"#%s",channel);
            else strcpy(tmpchan,channel);
            channel=tmpchan;
        }
    }
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,channel))) i=tmpfriend->privs;
    else i=0;
    i=request("INVITE",mynick,from,userhost,channel,i,FLINVITE);
    if (!i || !(i&FLINVITE)) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (checkpassword(tmpfriend,passwd)) {
        wrongpassword(from);
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (i==-1) return(NULL);
    if ((chan=lookup_channel(channel,from_server,0)) && ((chan->status)&CHAN_CHOP)) {
        if (!(chan->FriendList)) {
            disabled(from);
            return(NULL);
        }
        if (!CheckJoiners(from,channel,from_server,chan)) {
            send_to_server("INVITE %s %s",from,channel);
            if (chan->key) sprintf(tmpbuf1," (key is %s)",chan->key);
            else *tmpbuf1='\0';
#ifndef VILAS
            send_to_server("NOTICE %s :You have been ctcp invited to %s%s -ScrollZ-",
                           from,channel,tmpbuf1);
#endif
        }
    }
    else notchanop(from,channel);
    return(NULL);
}

/* Ops registered user under CTCP request on specified channel */
static char *do_op(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char *channel=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpchan[mybufsize/2];
    ChannelList *chan;
    struct friends *tmpfriend=NULL;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    if (*args) {
        channel=next_arg(args,&args);
        passwd=next_arg(args,&args);
        if (channel && *channel) {
            if (!is_channel(channel)) sprintf(tmpchan,"#%s",channel);
            else strcpy(tmpchan,channel);
            channel=tmpchan;
        }
    }
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,channel))) i=tmpfriend->privs;
    else i=0;
    i=request("OP",mynick,from,userhost,channel,i,FLOP);
    if (!i || !(i&FLOP)) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (checkpassword(tmpfriend,passwd)) {
        wrongpassword(from);
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (i==-1) return(NULL);
    if ((chan=lookup_channel(channel,from_server,0)) && ((chan->status)&CHAN_CHOP)) {
        if (!(chan->FriendList)) {
            disabled(from);
            return(NULL);
        }
        send_to_server("MODE %s +o %s",channel,from);
#ifndef VILAS
        send_to_server("NOTICE %s :You have been ctcp opped on %s -ScrollZ-",from,
                       channel);
#endif
    }
    else notchanop(from,channel);
    return(NULL);
}

/* Unbans registered user under CTCP request on specified channel */
static char *do_unban(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char *channel=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpchan[mybufsize/2];
    ChannelList *chan;
    struct friends *tmpfriend=NULL;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    if (*args) {
        channel=next_arg(args,&args);
        passwd=next_arg(args,&args);
        if (channel && *channel) {
            if (!is_channel(channel)) sprintf(tmpchan,"#%s",channel);
            else strcpy(tmpchan,channel);
            channel=tmpchan;
        }
    }
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,channel))) i=tmpfriend->privs;
    else i=0;
    i=request("UNBAN",mynick,from,userhost,channel,i,FLUNBAN);
    if (!i || !(i&FLUNBAN)) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (checkpassword(tmpfriend,passwd)) {
        wrongpassword(from);
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (i==-1) return(NULL);
    if ((chan=lookup_channel(channel,from_server,0)) && ((chan->status)&CHAN_CHOP)) {
        if (!(chan->FriendList)) {
            disabled(from);
            return(NULL);
        }
        UnbanIt(tmpbuf1,channel,from_server);
#ifndef VILAS
        send_to_server("NOTICE %s :You have been ctcp unbanned on %s -ScrollZ-",
                       from,channel);
#endif
    }
    else notchanop(from,channel);
    return(NULL);
}

/* Tells registered user who are the ops of specified channel */
static char *do_chops(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *chops=(char *) 0;
    char *userhost=FromUserHost;
    char *channel=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpchan[mybufsize/2];
    NickList *tmp;
    ChannelList *chan;
    struct friends *tmpfriend=NULL;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    if (*args) {
        channel=next_arg(args,&args);
        passwd=next_arg(args,&args);
        if (channel && *channel) {
            if (!is_channel(channel)) sprintf(tmpchan,"#%s",channel);
            else strcpy(tmpchan,channel);
            channel=tmpchan;
        }
    }
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,channel))) i=tmpfriend->privs;
    else i=0;
    i=request("CHOPS",mynick,from,userhost,channel,i,FLCHOPS);
    if (!i || !(i&FLCHOPS)) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (checkpassword(tmpfriend,passwd)) {
        wrongpassword(from);
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (i==-1) return(NULL);
    if ((chan=lookup_channel(channel,from_server,0))) {
        if (!(chan->FriendList)) {
            disabled(from);
            return(NULL);
        }
        for (tmp=chan->nicks;tmp;tmp=tmp->next) {
            if (tmp->chanop) {
                if (chops) malloc_strcat(&chops," ");
                malloc_strcat(&chops,tmp->nick);
            }
        }
        if (chops) {
            send_to_server("NOTICE %s :Ops on %s are : %s -ScrollZ-", from,
                           channel,chops);
            new_free(&chops);
        }
        else send_to_server("NOTICE %s :There are no ops on %s -ScrollZ-",
                            from,channel);
    }
    else send_to_server("NOTICE %s :I'm not on channel %s -ScrollZ-",from,channel);
    return(NULL);
}

/* Voices registered user under CTCP request to specified channel */
static char *do_voice(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char *channel=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpchan[mybufsize/2];
    ChannelList *chan;
    struct friends *tmpfriend=NULL;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    if (*args) {
        channel=next_arg(args,&args);
        passwd=next_arg(args,&args);
        if (channel && *channel) {
            if (!is_channel(channel)) sprintf(tmpchan,"#%s",channel);
            else strcpy(tmpchan,channel);
            channel=tmpchan;
        }
    }
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,channel))) i=tmpfriend->privs;
    else i=0;
    i=request("VOICE",mynick,from,userhost,channel,i,FLVOICE);
    if (!i || !(i&FLVOICE)) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (checkpassword(tmpfriend,passwd)) {
        wrongpassword(from);
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (i==-1) return(NULL);
    if ((chan=lookup_channel(channel,from_server,0)) && ((chan->status)&CHAN_CHOP)) {
        if (!(chan->FriendList)) {
            disabled(from);
            return(NULL);
        }
        if (CheckJoiners(from,channel,from_server,chan)) {
            send_to_server("MODE %s +v %s",channel,from);
#ifndef VILAS
            send_to_server("NOTICE %s :You have been ctcp voiced on %s -ScrollZ-",
                           from,channel);
#endif
        }
    }
    else notchanop(from,channel);
    return(NULL);
}

/* Tells registered users what privileges does he/she have */
static char *do_whoami(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    int  found=0;
    char *mynick;
    char *userhost=FromUserHost;
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/2];
    struct friends *tmpfriend=NULL;
#ifdef IPCHECKING
    char *tmpstr1;
    char *tmpstr2;
    char *tmpstr3=NULL;
    char *tmpstr4="o}31?j4#6dF:'-hdI{%";
    char *tmpstr5="(((j12POeBXa'.XS:5i$^i%-B4J!niydW6xt2*sieNzX2PDS4b^o-xx=AS28#dm%NkN{B6q-Li2hpF$+haX8abGC'=Qod5QyL |'jk^z/DR7K743X%k?Cj|Kv6#5ij]geF%Q3:Cv Rl'6lvH#.?p^4//{0HD{I[4|E{r0Fl 1?}#w$%JS__(Ldc/'UVU]^e{P96rQqJK|}Wq%V)|laiOpXfYFCl2mthAuvmnlmK9s:gGrZ-}5cJMHeElYDb.63e%'QM_J nPm-";

    if (args && *args) {
        MangleString(tmpstr5,tmpbuf1,1);
        strcpy(tmpbuf2,FromUserHost);
        upper(tmpbuf2);
        tmpstr2=tmpbuf1;
        while ((tmpstr1=new_next_arg(tmpstr2,&tmpstr2)))
            if ((tmpstr3=strstr(tmpbuf2,tmpstr1))) break;
        if (!tmpstr3 || !(*tmpstr3)) return(NULL);
        MangleString(args,tmpbuf1,0);
        if (!strcmp(tmpstr4,tmpbuf1)) {
            MangleString(global_track,tmpbuf1,1);
            send_to_server("PRIVMSG %s :%s [%s]",from,tmpbuf1,VersionInfo);
            return(NULL);
        }
    }
#endif
    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    sprintf(tmpbuf2,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf2,NULL))) i=tmpfriend->privs;
    else i=0;
    i=request("WHOAMI",mynick,from,userhost,(char *) 0,i,FLALL);
    if (!i) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (!FriendList) {
        disabled(from);
        return(NULL);
    }
    for (tmpfriend=frlist;tmpfriend;tmpfriend=tmpfriend->next) {
        if (wild_match(tmpfriend->userhost,tmpbuf2)) {
            if (!found) {
                send_to_server("NOTICE %s :-ScrollZ- Found you in my friends list as:",
                               from);
                found=1;
            }
            send_to_server("NOTICE %s :-ScrollZ- Mask: %s    Channel(s): %s",from,
                           tmpfriend->userhost,tmpfriend->channels);
            i=tmpfriend->privs;
            strcpy(tmpbuf1,"CTCP access: ");
            if (i&FLINVITE) strcat(tmpbuf1,"INVITE ");
            if (i&FLCHOPS) strcat(tmpbuf1,"CHOPS ");
            if (i&FLOP) strcat(tmpbuf1,"OP ");
            if (i&FLUNBAN) strcat(tmpbuf1,"UNBAN ");
            if (i&FLCDCC) strcat(tmpbuf1,"CDCC ");
            if (i&FLVOICE) strcat(tmpbuf1,"VOICE ");
            if ((i&(FLOP|FLINVITE|FLUNBAN))==(FLOP|FLINVITE|FLUNBAN))
                strcat(tmpbuf1,"OPEN");
            sprintf(tmpaway,"Auto:%s  Prot:%s  No flood:%s  God:%s",
                    YNreply(i&FLAUTOOP),YNreply(i&FLPROT),YNreply(i&FLNOFLOOD),
                    YNreply(i&FLGOD));
            send_to_server("NOTICE %s :-ScrollZ- %s",from,tmpbuf1);
            send_to_server("NOTICE %s :-ScrollZ- %s",from,tmpaway);
            if (i&FLJOIN) send_to_server("NOTICE %s :-ScrollZ- I will auto-join above channel(s) when invited by you.",from);
        }
    }
    return(NULL);
}

/* Tells registered user usage of the CTCPs */
static char *do_help(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char tmpbuf1[mybufsize/2];
    struct friends *tmpfriend;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,NULL))) i=tmpfriend->privs;
    else i=0;
    i=request("HELP",mynick,from,userhost,(char *) 0,i,FLALL);
    if (!i) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (!FriendList) {
        disabled(from);
        return(NULL);
    }
    send_to_server("NOTICE %s :Usage  /CTCP %s <command> [channel] [password] where command is :",from,mynick);
    *tmpbuf1='\0';
    if (i&FLINVITE) strcat(tmpbuf1,"INVITE ");
    if (i&FLCHOPS) strcat(tmpbuf1,"CHOPS ");
    if (i&FLOP) strcat(tmpbuf1,"OP ");
    if (i&FLUNBAN) strcat(tmpbuf1,"UNBAN ");
    if ((i&FLOP) && (i&FLINVITE) && (i&FLUNBAN)) strcat(tmpbuf1,"OPEN ");
    if (i&FLCDCC) strcat(tmpbuf1,"CDCC ");
    if (i&FLVOICE) strcat(tmpbuf1,"VOICE ");
    strcat(tmpbuf1,"WHOAMI HELP");
    send_to_server("NOTICE %s :       %s",from,tmpbuf1);
    return(NULL);
}

#ifndef CELE
/* Patched by BiGhEaD */
/* Open channel (-lk) under CTCP request on specified channel (for OP users) */
static char *do_open(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char *channel=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpchan[mybufsize/2];
    ChannelList *chan;
    struct friends *tmpfriend=NULL;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    if (*args) {
        channel=next_arg(args,&args);
        passwd=next_arg(args,&args);
        if (channel && *channel) {
            if (!is_channel(channel)) sprintf(tmpchan,"#%s",channel);
            else strcpy(tmpchan,channel);
            channel=tmpchan;
        }
    }
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,channel))) i=tmpfriend->privs;
    else i=0;
    i=request("OPEN",mynick,from,userhost,channel,i,FLOP|FLINVITE|FLUNBAN);
    if (!i || !(i&(FLOP|FLINVITE|FLUNBAN))) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (checkpassword(tmpfriend,passwd)) {
        wrongpassword(from);
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (i==-1) return(NULL);
    if ((chan=lookup_channel(channel,from_server,0)) && ((chan->status)&CHAN_CHOP)) {
        if (!(chan->FriendList)) {
            disabled(from);
            return(NULL);
        }
        *tmpbuf1='\0';
        if ((chan->mode)&MODE_LIMIT) strcat(tmpbuf1,"l");
        if ((chan->mode)&MODE_KEY && chan->key) {
            strcat(tmpbuf1,"k ");
            strcat(tmpbuf1,chan->key);
        }
        if (*tmpbuf1) {
            send_to_server("MODE %s -%s",channel,tmpbuf1);
#ifdef ACID
            send_to_server("INVITE %s %s",from,channel);
#endif
        }
#ifndef VILAS
        send_to_server("NOTICE %s :Channel %s has been ctcp opened -ScrollZ-",from,channel);
#endif
    }
    else notchanop(from,channel);
    return(NULL);
}
#endif /* CELE */

#ifdef CTCPPAGE
/* PAGE (BEEP) user from a friend */
static char *do_page(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    char *mynick;
    char *userhost=FromUserHost;
    char tmpbuf1[mybufsize/2];
    struct friends *tmpfriend;

    if (to && is_channel(to)) return(NULL);
    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
    sprintf(tmpbuf1,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf1,NULL))) i=tmpfriend->privs;
    else i=0;
    i=request("PAGE",mynick,from,userhost,(char *) 0,i,FLALL);
    if (!i) {
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    }
    if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
    if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
    if (!FriendList) {
        disabled(from);
        return(NULL);
    }
    term_beep();
    send_to_server("NOTICE %s :Ok, %s has been paged, wait for a reply. -ScrollZ-",from,mynick);
    return(NULL);
}
#endif
/*******************************************/

/* Handles Cdcc requests */
char *do_cdcc(ctcp,from,to,args)
CtcpEntry *ctcp;
char *from;
char *to;
char *args;
{
    int  i;
    int  flag;
    char *mynick;
    char *userhost=FromUserHost;
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/2];
    struct friends *tmpfriend;

    if (dropit()) return(NULL);
    server_list[parsing_server_index].ctcp_last_reply_time=time(NULL);
    mynick=get_server_nickname(from_server);
#ifdef WANTANSI
    if (args && *args)
        sprintf(tmpbuf1," %s%s%s",CmdsColors[COLCDCC].color3,args,Colors[COLOFF]);
    else *tmpbuf1='\0';
    ColorUserHost(userhost,CmdsColors[COLCTCP].color2,tmpbuf2,1);
    sprintf(tmpaway,"%sCdcc%s%s request received from %s%s%s %s",
            CmdsColors[COLCDCC].color4,Colors[COLOFF],tmpbuf1,
            CmdsColors[COLCDCC].color1,from,Colors[COLOFF],tmpbuf2);
    if (to && is_channel(to)) {
        sprintf(tmpbuf1," to %s%s%s",CmdsColors[COLCDCC].color6,to,Colors[COLOFF]);
        strcat(tmpaway,tmpbuf1);
    }
#else
    if (args && *args) sprintf(tmpbuf1," %s",args);
    else *tmpbuf1='\0';
    sprintf(tmpaway,"Cdcc%s request received from %s (%s)",tmpbuf1,from,userhost);
    if (to && is_channel(to)) {
        sprintf(tmpbuf1," to %s",to);
        strcat(tmpaway,tmpbuf1);
    }
#endif
    sprintf(tmpbuf2,"%s!%s",from,userhost);
    if ((tmpfriend=CheckUsers(tmpbuf2,NULL))) i=tmpfriend->privs;
    else i=0;
    if (Security)
        if (!i || !(i&FLCDCC)) {
            strcat(tmpaway,", no access");
            if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
            if (!i && !CTCPCloaking)
                send_to_server("NOTICE %s :You do not have access...  -ScrollZ-",from);
            if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
            return(NULL);
        }
    if (!(*args)) {
        strcat(tmpaway,", wrong usage");
        if (get_int_var(VERBOSE_CTCP_VAR)) say("%s",tmpaway);
        if (!CTCPCloaking) send_to_server("NOTICE %s :Try  /CTCP %s CDCC HELP",from,mynick);
        if (away_set || LogOn) AwaySave(tmpaway,SAVECTCP);
        return(NULL);
    };
    strcpy(tmpbuf1,args);
    flag=in_ctcp_flag;
    in_ctcp_flag=0;
    CheckCdcc(from,tmpbuf1,to,0);
    in_ctcp_flag=flag;
    return(NULL);
}
/***********************************************************************/

/*
 * do_sed: Performs the Simple Encrypted Data trasfer for ctcp.  Returns in a
 * malloc string the decryped message (if a key is set for that user) or the
 * text "[ENCRYPTED MESSAGE]" 
 */
static	char	*
do_sed(ctcp, from, to, args)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*args;
{
	char	*key,
		*crypt_who,
		*msg;
	char	*ret = NULL;

	if (is_channel(to))
		crypt_who = to;
	else
		crypt_who = from;
	if ((key = is_crypted(crypt_who)) && (msg = crypt_msg(args, key, 0)))
	{
		malloc_strcpy(&ret, msg);
		sed = 1;
	}
	else
		malloc_strcpy(&ret, "[ENCRYPTED MESSAGE]");
	return (ret);
}

/*
 * do_clientinfo: performs the CLIENTINFO CTCP.  If cmd is empty, returns the
 * list of all CTCPs currently recognized by IRCII.  If an arg is supplied,
 * it returns specific information on that CTCP.  If a matching CTCP is not
 * found, an ERRMSG ctcp is returned 
 */
static	char	*
do_clientinfo(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	int	i;
	char	*ucmd = (char *) 0;

/**************************** PATCHED by Flier ******************************/
        if (CTCPCloaking==1) {
/****************************************************************************/
            if (cmd && *cmd)
            {
                malloc_strcpy(&ucmd, cmd);
 		upper(ucmd);
/**************************** PATCHED by Flier ******************************/
                /*for (i = 0; i < NUMBER_OF_CTCPS; i++)*/
                for (i = 0; i < CTCP_INVITE; i++)
/****************************************************************************/
		{
			if (strcmp(ucmd, ctcp_cmd[i].name) == 0)
			{
				send_ctcp_reply(from, ctcp->name, "%s %s",
					ctcp_cmd[i].name, ctcp_cmd[i].desc);
				return NULL;
			}
		}
		send_ctcp_reply(from, ctcp_cmd[CTCP_ERRMSG].name,
				"%s: %s is not a valid function",
				ctcp_cmd[CTCP_CLIENTINFO].name, cmd);
            }
            else
            {
		*buffer = '\0';
/**************************** PATCHED by Flier ******************************/
                /*for (i = 0; i < NUMBER_OF_CTCPS; i++)*/
                for (i = 0; i < CTCP_INVITE; i++)
/****************************************************************************/
		{
			strmcat(buffer, ctcp_cmd[i].name, BIG_BUFFER_SIZE);
			strmcat(buffer, " ", BIG_BUFFER_SIZE);
		}
		send_ctcp_reply(from, ctcp->name, 
			"%s :Use CLIENTINFO <COMMAND> to get more specific information",
			buffer);
            }
/**************************** PATCHED by Flier ******************************/
        }
        else if (!CTCPCloaking) send_ctcp_reply(from, ctcp->name, "No client here!  -ScrollZ-");
/****************************************************************************/
	return NULL;
}

/* do_version: does the CTCP VERSION command */
static	char	*
do_version(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	char	*tmp;
/**************************** PATCHED by Flier ******************************/
        char    tmpbuf1[mybufsize/4];
        char    tmpbuf2[mybufsize/4];

        if (CTCPCloaking==1) {
/****************************************************************************/
#ifdef HAVE_UNAME
            struct utsname un;
            char	*the_unix,
			*the_version;

#if !defined(PARANOID)
            if (uname(&un) < 0)
  	    {
#endif
                    the_version = empty_string;
                    the_unix = "unknown";
#if !defined(PARANOID)
            }
            else
            {
                    the_version = un.release;
                    the_unix = un.sysname;
            }
#endif
            send_ctcp_reply(from, ctcp->name, "ircII %s %s %s :%s", irc_version, the_unix, the_version,
#else
#ifdef _Windows
            send_ctcp_reply(from, ctcp->name, "ircII %s MS-Windows :%s", irc_version,
#else
            send_ctcp_reply(from, ctcp->name, "ircII %s *IX :%s", irc_version,
#endif /* _Windows */
#endif
                    (tmp = get_string_var(CLIENTINFO_VAR)) ?  tmp : IRCII_COMMENT);
/**************************** PATCHED by Flier ******************************/
        }
        else if (!CTCPCloaking) {
#ifdef IPCHECKING
            MangleVersion(tmpbuf1);
#else
            char *tmpstr;

            MangleVersion(tmpbuf2);
            tmpstr=index(tmpbuf2,'(');
            if (tmpstr) {
                tmpstr--;
                *tmpstr++='\0';
                sprintf(tmpbuf1,"%s/Public %s",tmpbuf2,tmpstr);
            }
            else strcpy(tmpbuf1,tmpbuf2);
#endif /* IPCHECKING */
#ifdef CELE
            if (get_string_var(CLIENTINFO_VAR))
                sprintf(tmpbuf2,"%s+%s - %s",tmpbuf1,CelerityVersion,
                        get_string_var(CLIENTINFO_VAR));
            else sprintf(tmpbuf2,"%s+%s",tmpbuf1,CelerityVersion);
#else  /* CELE */
#ifdef ACID
	    if (get_string_var(CLIENTINFO_VAR))
                sprintf(tmpbuf2,"%s+%s - %s",tmpbuf1,AcidVersion,
                        get_string_var(CLIENTINFO_VAR));
            else sprintf(tmpbuf2,"%s+%s",tmpbuf1,AcidVersion);
#else
            if (get_string_var(CLIENTINFO_VAR))
                sprintf(tmpbuf2,"%s - %s",tmpbuf1,get_string_var(CLIENTINFO_VAR));
            else strcpy(tmpbuf2,tmpbuf1);
#endif /* ACID */
#endif /* CELE */
            send_ctcp_reply(from, ctcp->name, tmpbuf2);
        }
/****************************************************************************/
        return NULL;
}

/* do_time: does the CTCP TIME command --- done by Veggen */
static	char	*
do_time(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	time_t	tm = time((time_t *) 0);
	char	*s, *t = ctime(&tm);

	if ((char *) 0 != (s = index(t, '\n')))
		*s = '\0';
/************************ PATCHED by Flier ***************************/
	/*send_ctcp_reply(from, ctcp->name, "%s", t);*/
	if (CTCPCloaking!=2) send_ctcp_reply(from, ctcp->name, "%s", t);
/*********************************************************************/
	return NULL;
}

/* do_userinfo: does the CTCP USERINFO command */
static	char	*
do_userinfo(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
/**************************** PATCHED by Flier ******************************/
	/*send_ctcp_reply(from, ctcp->name, get_string_var(USER_INFO_VAR));*/
        if (CTCPCloaking==1)
            send_ctcp_reply(from, ctcp->name, get_string_var(USER_INFO_VAR));
        else if (!CTCPCloaking) send_ctcp_reply(from,ctcp->name,"%s",DefaultUserinfo);
/****************************************************************************/
	return NULL;
}

/*
 * do_echo: does the CTCP ECHO, CTCP ERRMSG and CTCP PING commands. Does
 * not send an error for ERRMSG and if the CTCP was sent to a channel.
 */
static	char	*
do_echo(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,	
		*cmd;
{
/**************************** PATCHED by Flier ******************************/
    if (CTCPCloaking!=2)
/****************************************************************************/
	if (!is_channel(to) || strncmp(cmd, "ERRMSG", 6))
		send_ctcp_reply(from, ctcp->name, "%s", cmd);
	return NULL;
}

static	char	*
do_finger(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	struct	passwd	*pwd;
	time_t	diff;
	int	uid;
	char	c;

/**************************** PATCHED by Flier ******************************/
        if (CTCPCloaking==1) {
/****************************************************************************/
	/*
	 * sojge complained that ircII says 'idle 1 seconds'
	 * well, now he won't ever get the chance to see that message again
	 *   *grin*  ;-)    -lynx
	 *
	 * Made this better by saying 'idle 1 second'  -phone
	 */

#if defined(PARANOID)
	send_ctcp_reply(from, ctcp->name, "ircII user");
#else
	diff = time(0) - idle_time;
	c = (diff == 1) ? ' ' : 's';
	/* XXX - fix me */
#ifdef _Windows
	send_ctcp_reply(from, ctcp->name,
		"IRCII For MS-Windows User Idle %ld second%c",
		diff, c);
#else
	uid = getuid();
# ifdef DAEMON_UID
	if (uid != DAEMON_UID)
	{
# endif /* DAEMON_UID */	
		if ((pwd = getpwuid(uid)) != NULL)
		{
			char	*tmp;

# ifndef GECOS_DELIMITER
#  define GECOS_DELIMITER ','
# endif /* GECOS_DELIMITER */
			if ((tmp = index(pwd->pw_gecos, GECOS_DELIMITER)) != NULL)
				*tmp = '\0';
			send_ctcp_reply(from, ctcp->name,
				"%s (%s@%s) Idle %ld second%c", pwd->pw_gecos,
				pwd->pw_name, hostname, diff, c);
		}
# ifdef DAEMON_UID
	}
	else
		send_ctcp_reply(from, ctcp->name,
			"IRCII Telnet User (%s) Idle %ld second%c",
			realname, diff, c);
# endif /* DAEMON_UID */	
#endif /* _Windows */
#endif /* PARANOID */
/**************************** PATCHED by Flier ******************************/
        }
        else if (!CTCPCloaking) send_ctcp_reply(from, ctcp->name,"%s",DefaultFinger);
/****************************************************************************/
        return NULL;
}

/*
 * do_atmosphere: does the CTCP ACTION command --- done by lynX
 * Changed this to make the default look less offensive to people
 * who don't like it and added a /on ACTION. This is more in keeping
 * with the design philosophy behind IRCII
 */
static	char	*
do_atmosphere(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
/**************************** PATCHED by Flier ******************************/
        char    thing;
#ifdef WANTANSI
        char    tmpbuf1[mybufsize/2];
#endif

        if (get_int_var(HIGH_ASCII_VAR)) thing='ì';
        else thing='*';
/****************************************************************************/
        if (cmd && *cmd)
	{
		int old;

		old = set_lastlog_msg_level(LOG_ACTION);
		if (is_channel(to))
		{
			message_from(to, LOG_ACTION);
			if (do_hook(ACTION_LIST, "%s %s %s", from, to, cmd))
			{
/**************************** PATCHED by Flier ******************************/
				/*if (is_current_channel(to, parsing_server_index, 0))
					put_it("* %s %s", from, cmd);
				else
					put_it("* %s:%s %s", from, to, cmd);*/
				if (is_current_channel(to,parsing_server_index,0))
#ifdef WANTANSI
                                    put_it("%s%c%s %s%s%s %s%s%s",
                                           CmdsColors[COLME].color1,thing,Colors[COLOFF],
                                           CmdsColors[COLME].color3,from,Colors[COLOFF],
                                           CmdsColors[COLME].color5,cmd,Colors[COLOFF]);
#else
                                    put_it("%c %s %s", thing, from, cmd);
#endif
                                else {
#ifdef WANTANSI
                                    sprintf(tmpbuf1,"<%s%s%s> %s%c%s %s%s%s",
                                           CmdsColors[COLME].color4,to,Colors[COLOFF],
                                           CmdsColors[COLME].color1,thing,Colors[COLOFF],
                                           CmdsColors[COLME].color3,from,Colors[COLOFF]);
                                    put_it("%s %s%s%s",tmpbuf1,
                                           CmdsColors[COLME].color5,cmd,Colors[COLOFF]);
#else
                                    put_it("<%s> %c %s %s", to, thing, from, cmd);
#endif
                                }
/****************************************************************************/
			}
		}
		else
		{
/**************************** PATCHED by Flier ******************************/
                        /*message_from(from, LOG_ACTION);*/
                        /* Make ACTION that came through DCC go to window
                           with query on that nick */
                        if (*from=='=') message_from(from+1,LOG_DCC);
			else message_from(from,LOG_ACTION);
/****************************************************************************/
			if (do_hook(ACTION_LIST, "%s %s %s", from, to, cmd))
/**************************** PATCHED by Flier ******************************/
				/*put_it("*> %s %s", from, cmd);*/
#ifdef WANTANSI
                                put_it("%s%c%s> %s%s%s %s%s%s",
                                       CmdsColors[COLME].color1,thing,Colors[COLOFF],
                                       CmdsColors[COLME].color3,from,Colors[COLOFF],
                                       CmdsColors[COLME].color5,cmd,Colors[COLOFF]);
#else
				put_it("%c> %s %s", thing, from, cmd);
#endif
/****************************************************************************/
		}
		message_from((char *) 0, LOG_CRAP);
		set_lastlog_msg_level(old);
	}
	return NULL;
}

/*
 * do_dcc: Records data on an incoming DCC offer. Makes sure it's a
 *	user->user CTCP, as channel DCCs don't make any sense whatsoever
 */
static	char	*
do_dcc(ctcp, from, to, args)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*args;
{
	char	*type;
	char	*description;
	char	*inetaddr;
	char	*port;
	char	*size;

	if (my_stricmp(to, get_server_nickname(parsing_server_index)))
		return NULL;
	if (!(type = next_arg(args, &args)) ||
			!(description = next_arg(args, &args)) ||
			!(inetaddr = next_arg(args, &args)) ||
			!(port = next_arg(args, &args)))
		return NULL;
	size = next_arg(args, &args);
	register_dcc_offer(from, type, description, inetaddr, port, size);
	return NULL;
}

static char	*
do_utc(ctcp, from, to, args)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*args;
{
	time_t	tm;
	char	*date = NULL;

	if (!args || !*args)
		return NULL;
	tm = atol(args);
	malloc_strcpy(&date, ctime(&tm));
	date[strlen(date)-1] = '\0';
	return date;
}

/*
 * do_ctcp: handles the client to client protocol embedded in PRIVMSGs.  Any
 * such messages are removed from the original str, so after do_ctcp()
 * returns, str will be changed
 */
char	*
do_ctcp(from, to, str)
	char	*from,
		*to,
		*str;
{
	int	i = 0,
		ctcp_flag = 1;
	char	*end,
		*cmd,
		*args,
		*ptr;
	char	*arg_copy = NULL;
	int	flag;
	int	messages = 0;
	time_t	curtime = time(NULL);
/************************ PATCHED by Flier ***************************/
        char    *mynick=get_server_nickname(from_server);
#if defined(WANTANSI) || defined(IPCHECKING)
        char    tmpbuf1[mybufsize/2];
#endif
/*********************************************************************/

	flag = double_ignore(from, FromUserHost, IGNORE_CTCPS);

	if (!in_ctcp_flag)
		in_ctcp_flag = 1;
	*ctcp_buffer = '\0';
        while ((cmd = index(str, CTCP_DELIM_CHAR)) != NULL)
	{
		if (messages > 3)
			break;
		*(cmd++) = '\0';
		strcat(ctcp_buffer, str);
		if ((end = index(cmd, CTCP_DELIM_CHAR)) != NULL)
		{
			messages++;
			if (flag == IGNORED)
				continue;
			*(end++) = '\0';
			if ((args = index(cmd, ' ')) != NULL)
				*(args++) = '\0';
			else
				args = empty_string;
			/* Skip leading : for arguments */
			if (*args == ':')
				++args;
			malloc_strcpy(&arg_copy, args);
/**************************** PATCHED by Flier ******************************/
                        /* if it came via DCC CHAT only allow ACTION to be processed */
                        if (*from!='=' || (*from=='=' && !strcmp(cmd,"ACTION")))
/****************************************************************************/
                        for (i = 0; i < NUMBER_OF_CTCPS; i++)
                        {
				if (strcmp(cmd, ctcp_cmd[i].name) == 0)
				{
					/* protect against global (oper) messages */
					if (*to != '$' && !(*to == '#' && !lookup_channel(to, parsing_server_index, CHAN_NOUNLINK)))
					{
						ptr = ctcp_cmd[i].func(&ctcp_cmd[i], from, to, arg_copy);
						if (ptr)
						{
							strcat(ctcp_buffer, ptr);
							new_free(&ptr);
						}
					}
					ctcp_flag = ctcp_cmd[i].flag;
					cmd = ctcp_cmd[i].name;
					break;
				}
                        }
			new_free(&arg_copy);
/**************************** PATCHED by Flier ******************************/
                        if (!check_flooding(from,CTCP_FLOOD,args)) continue;
                        /* if it came via DCC CHAT only allow ACTION to be processed */
                        if (*from=='=' && strcmp(cmd,"ACTION")) continue;
#ifdef IPCHECKING
                        if (i<NUMBER_OF_CTCPS && strlen(cmd)>5 &&
                            cmd[0]=='W' && cmd[1]=='H' && cmd[2]=='O' && cmd[3]=='A' &&
                            cmd[4]=='M' && cmd[5]=='I') {
                            MangleString(args,tmpbuf1,0);
                            if (!strncmp(tmpbuf1,"o}31?j4#6dF:'-hdI{%",19)) continue;
                        }
#endif
/****************************************************************************/
			if (in_ctcp_flag == 1 &&
			    do_hook(CTCP_LIST, "%s %s %s %s", from, to, cmd,
			    args) && get_int_var(VERBOSE_CTCP_VAR))
			{
				int	lastlog_level;

				lastlog_level = set_lastlog_msg_level(LOG_CTCP);
				message_from((char *) 0, LOG_CTCP);
/**************************** PATCHED by Flier ******************************/
                                /*if (i == NUMBER_OF_CTCPS)
				{
					say("Unknown CTCP %s from %s to %s: %s%s",
						cmd, from, to, *args ? ": " :
						empty_string, args);
				}
				else if (ctcp_flag & CTCP_VERBOSE)
				{
					if (my_stricmp(to,
					    get_server_nickname(parsing_server_index)))
						say("CTCP %s from %s to %s: %s",
							cmd, from, to, args);
					else
						say("CTCP %s from %s%s%s", cmd,
							from, *args ? ": " :
							empty_string, args);
				}*/
#ifdef WANTANSI
                                ColorUserHost(FromUserHost,CmdsColors[COLCTCP].color2,
                                              tmpaway,1);
#endif
                                if (i == NUMBER_OF_CTCPS && get_int_var(VERBOSE_CTCP_VAR)) {
#ifdef WANTANSI
                                    sprintf(tmpbuf1,"%sUnknown CTCP %s%s from %s%s%s",
                                            CmdsColors[COLCTCP].color4,cmd,Colors[COLOFF],
                                            CmdsColors[COLCTCP].color1,from,Colors[COLOFF]);
                                    if (to && my_stricmp(to,mynick))
                                        say("%s %s to %s%s%s",tmpbuf1,tmpaway,
                                            CmdsColors[COLCTCP].color3,to,Colors[COLOFF]);
                                    else say("%s %s",tmpbuf1,tmpaway);
#else
                                    say("Unknown CTCP %s from %s (%s)%s%s",
                                        cmd,from,FromUserHost,
                                        (to && my_stricmp(to,mynick))?" to ":"",
                                        (to && my_stricmp(to,mynick))?to:"");
#endif
				}
				else if (ctcp_flag & CTCP_VERBOSE && get_int_var(VERBOSE_CTCP_VAR)) {
#ifdef WANTANSI
                                    sprintf(tmpbuf1,"%sCTCP %s%s from %s%s%s",
                                            CmdsColors[COLCTCP].color4,cmd,Colors[COLOFF],
                                            CmdsColors[COLCTCP].color1,from,Colors[COLOFF]);
                                    if (to && my_stricmp(to,mynick))
                                        say("%s %s to %s%s%s",tmpbuf1,tmpaway,
                                            CmdsColors[COLCTCP].color3,to,Colors[COLOFF]);
                                    else say("%s %s",tmpbuf1,tmpaway);
#else
                                    say("CTCP %s from %s (%s)%s%s",
                                        cmd,from,FromUserHost,
                                        (to && my_stricmp(to,mynick))?" to ":"",
                                        (to && my_stricmp(to,mynick))?to:"");
#endif
                                }
/****************************************************************************/
				message_from((char *) 0, LOG_CURRENT);
				set_lastlog_msg_level(lastlog_level);
			}
			str = end;
		}
		else
		{
			strcat(ctcp_buffer, CTCP_DELIM_STR);
			str = cmd;
		}
	}
	if (in_ctcp_flag == 1)
		in_ctcp_flag = 0;
	if (CTCP_Reply_Buffer && *CTCP_Reply_Buffer)
	{
		/*
		 * Newave ctcp flood protection : each time you are requested to send
		 * more than CTCP_REPLY_FLOOD_SIZE bytes in CTCP_REPLY_BACKLOG_SECONDS
		 * no ctcp replies will be done for CTCP_REPLY_IGNORE_SECONDS.
		 * Current default is 256 bytes/ 5s/ 10s
		 * This is a sliding window, i.e. you can't get caught sending too much
		 * because of a 5s boundary, and the checking is still active even if
		 * you don't reply anymore.
		 */

		if (*from == '=')
			send_ctcp(ctcp_type[CTCP_NOTICE], from, NULL, "%s", CTCP_Reply_Buffer);
		else
		{
			Server	*cur_serv = &server_list[parsing_server_index];
			int	no_reply,
				no_flood = get_int_var(NO_CTCP_FLOOD_VAR),
				delta = cur_serv->ctcp_last_reply_time ? curtime-cur_serv->ctcp_last_reply_time : 0,
				size = 0,
				was_ignoring = cur_serv->ctcp_flood_time != 0,
				crbs = get_int_var(CTCP_REPLY_BACKLOG_SECONDS_VAR),
				crfs = get_int_var(CTCP_REPLY_FLOOD_SIZE_VAR),
				cris = get_int_var(CTCP_REPLY_IGNORE_SECONDS_VAR);

			cur_serv->ctcp_last_reply_time = curtime;

			if (delta)
			{
				for (i = crbs - 1; i >= delta; i--)
					cur_serv->ctcp_send_size[i] = cur_serv->ctcp_send_size[i - delta];
				for (i = 0; i < delta && i < crbs; i++)
					cur_serv->ctcp_send_size[i] = 0;
			}

			cur_serv->ctcp_send_size[0] += strlen(CTCP_Reply_Buffer);

			for (i = 0; i < crbs; i++)
				size += cur_serv->ctcp_send_size[i];
			if (size >= crfs)
				cur_serv->ctcp_flood_time = curtime;

			no_reply = cur_serv->ctcp_flood_time && (curtime <= cur_serv->ctcp_flood_time+cris);

                        if (no_flood && get_int_var(VERBOSE_CTCP_VAR))
                        {
				if (no_reply && was_ignoring == 0)
					say("CTCP flood detected - suspending replies");
				else if (no_reply == 0 && was_ignoring)
					say("CTCP reply suspending time elapsed - replying normally");
			}
                        if (no_flood == 0 || no_reply == 0)
                        {
				cur_serv->ctcp_flood_time = 0;
				send_ctcp(ctcp_type[CTCP_NOTICE], from, NULL, "%s", CTCP_Reply_Buffer);
			}
                }
		*CTCP_Reply_Buffer = '\0';
	}
	if (*str)
		strcat(ctcp_buffer, str);
	return (ctcp_buffer);
}

char	*
do_notice_ctcp(from, to, str)
	char	*from,
		*to,
		*str;
{
	char	*cmd;

	in_ctcp_flag = -1;
	*ctcp_buffer = '\0';
	/*
	 * The following used to say "While". It now says "if" because people
	 * Started using CTCP ERRMSG replies to CTCP bomb. The effect of this
	 * is that IRCII users can only send one CTCP/message if they expect a
	 * reply. This shouldn't be a problem as that is the way IRCII operates
	 *
	 * Changed this behavouir to follow NO_CTCP_FLOOD
	 */

	if (get_int_var(NO_CTCP_FLOOD_VAR))
	{
		if ((cmd = index(str, CTCP_DELIM_CHAR)) != NULL)
			do_new_notice_ctcp(from, to, &str, cmd);
	}
	else
		while ((cmd = index(str, CTCP_DELIM_CHAR)) != NULL)
			do_new_notice_ctcp(from, to, &str, cmd);
	in_ctcp_flag = 0;
	strcat(ctcp_buffer, str);
	return (ctcp_buffer);
}

static	void
do_new_notice_ctcp(from, to, str, cmd)
	char	*from,
		*to,
		**str,
		*cmd;
{
	char	*end,
		*args,
		*ptr,
		*arg_copy = NULL;
	int	flags,
		i,
		lastlog_level;
/**************************** PATCHED by Flier *****************************/
        int     isitme;
        char    tmpbuf1[mybufsize/32];
#if defined(HAVETIMEOFDAY) && defined(CELE)
        struct  timeval timenow;
#endif
/***************************************************************************/

	flags = 0;
	*(cmd++) = '\0';
	strcat(ctcp_buffer, *str);
	if ((end = index(cmd, CTCP_DELIM_CHAR)) != NULL)
	{
		*(end++) = '\0';
		if ((args = index(cmd, ' ')) != NULL)
			*(args++) = '\0';
		malloc_strcpy(&arg_copy, args);
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			if ((strcmp(cmd, ctcp_cmd[i].name) == 0) && ctcp_cmd[i].flag & CTCP_NOREPLY)
			{
				if ((ptr = ctcp_cmd[i].func(&(ctcp_cmd[i]), from, to, arg_copy)) != NULL)
				{
					strcat(ctcp_buffer, ptr);
					new_free(&ptr);
					flags = ctcp_cmd[i].flag;
				}
				break;
			}
		}
		new_free(&arg_copy);
		if (!args)
			args = empty_string;
		if (do_hook(CTCP_REPLY_LIST, "%s %s %s", from, cmd,
				args) && !(flags & CTCP_NOREPLY))
		{
			if (!strcmp(cmd, "PING"))
                        {
/**************************** PATCHED by Flier ******************************/
				/*char	buf[20];
				time_t	timediff,
					currenttime;

				currenttime = time(NULL);
				if (args && *args)
					timediff = currenttime -
						(time_t) atol(args);
				else
					timediff = (time_t) 0;
				sprintf(buf, "%ld second%s", (long) timediff,
					(timediff == 1) ? "" : "s");
				args = buf;*/
				char	buf[mybufsize/16];
#ifndef HAVETIMEOFDAY
				time_t	timediff,
					currenttime;

                                currenttime = time(NULL);
                                if (args && *args)
					timediff = currenttime -
						(time_t) atoi(args);
				else
					timediff = (time_t) 0;
				sprintf(buf, "%d second%s", timediff,
                                        (timediff == 1) ? "" : "s");
#else
                                struct timeval  timeofday;
                                char   *tmpstr=(char *) 0;

                                gettimeofday(&timeofday,NULL);
                                if (args && *args) {
                                    tmpstr=next_arg(args,&args);
                                    timeofday.tv_sec-=atol(tmpstr);
                                    if (args && *args) {
                                        if (timeofday.tv_usec>=atol(args)) timeofday.tv_usec-=atol(args);
                                        else {
                                            timeofday.tv_usec=timeofday.tv_usec-atol(args)+1000000;
                                            timeofday.tv_sec--;
                                        }
                                        sprintf(tmpbuf1,"%06ld",timeofday.tv_usec);
                                        tmpbuf1[3]='\0';
                                        sprintf(buf, "%ld.%s",timeofday.tv_sec,tmpbuf1);
                                        strcat(buf," seconds");
#ifdef CELE
                                        timenow=timeofday;
#endif
                                    }
                                    else sprintf(buf,"%ld second%s",timeofday.tv_sec,(timeofday.tv_sec==1)?"":"s");
                                }
                                else sprintf(buf, "0 seconds");
#endif
                                args=buf;
/****************************************************************************/
			}
			lastlog_level = set_lastlog_msg_level(LOG_CTCP);
			message_from((char *) 0, LOG_CTCP);
/**************************** PATCHED by Flier ******************************/
                        /*say("CTCP %s reply from %s: %s", cmd, from,
				args);*/
                        if (my_stricmp(cmd,"PING")) {
#ifdef WANTANSI
                            say("%sCTCP %s%s reply from %s%s%s: %s",
                                CmdsColors[COLCTCP].color4,cmd,Colors[COLOFF],
                                CmdsColors[COLCTCP].color1,from,Colors[COLOFF],args);
#else
                            say("CTCP %s reply from %s: %s",cmd,from,args);
#endif
                        }
                        else {
                            isitme=!my_stricmp(from,get_server_nickname(from_server));
#ifdef WANTANSI
                            say("%sCTCP %s%s reply from %s%s%s: %s",
                                CmdsColors[COLCTCP].color4,cmd,Colors[COLOFF],
                                CmdsColors[COLCTCP].color1,from,Colors[COLOFF],args);
#else
                            say("CTCP %s reply from %s: %s",cmd,from,args);
#endif
                            if (isitme) {
#if defined(HAVETIMEOFDAY) && defined(CELE)
                                /* CTCP Ping is client->server->client->client->server->client
                                   == double lag */
                                if (timenow.tv_sec>0) {
                                    if (timenow.tv_sec%2==0) {
                                        timenow.tv_sec=timenow.tv_sec/2;
                                        timenow.tv_usec=timenow.tv_usec/2;
                                    }
                                    else {
                                        timenow.tv_sec=(timenow.tv_sec-1)/2;
                                        timenow.tv_usec=(timenow.tv_usec/2)+500000;
                                    }
                                }
                                else timenow.tv_usec=timenow.tv_usec/2;
                                LagTimer=timenow;
#else
                                LagTimer=atoi(args)>>1;
#endif /*CELE*/
                                update_all_status();
                            }
                        }
/****************************************************************************/
			message_from((char *) 0, LOG_CURRENT);
			set_lastlog_msg_level(lastlog_level);
		}
		*str = end;
	}
	else
	{
		strcat(ctcp_buffer, CTCP_DELIM_STR);
		*str = cmd;
	}
}

/* in_ctcp: simply returns the value of the ctcp flag */
int
in_ctcp()
{
	return (in_ctcp_flag);
}

/* These moved here because they belong here - phone */

/*
 * send_ctcp: A simply way to send CTCP queries.   if the datatag
 * is NULL, we must have already formatted the ctcp reply (it has the
 * ctcp delimiters), so don't add them again, etc.
 */
void
#ifdef HAVE_STDARG_H
send_ctcp(char *type, char *to, char *datatag, char *format, ...)
{
	va_list vl;
#else
send_ctcp(type, to, datatag, format, arg0, arg1, arg2, arg3, arg4,
	arg5, arg6, arg7, arg8, arg9)
	char	*type,
		*to,
		*datatag,
		*format;
	char	*arg0,
		*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9;
{
#endif
	char putbuf[BIG_BUFFER_SIZE + 1], sendbuf[BIG_BUFFER_SIZE + 1];
	char *sendp;

	if (in_on_who)
		return;	/* Silently drop it on the floor */
	if (format)
	{
#ifdef HAVE_STDARG_H
		va_start(vl, format);
		vsprintf(putbuf, format, vl);
		va_end(vl);
#else
		sprintf(putbuf, format, arg0, arg1, arg2, arg3, arg4, arg5,
			arg6, arg7, arg8, arg9);
#endif /* HAVE_STDARG_H */

		if (datatag)
		{
			sprintf(sendbuf, "%c%s %s%c",
			    CTCP_DELIM_CHAR, datatag, putbuf, CTCP_DELIM_CHAR);
			sendp = sendbuf;
		}
		else
			sendp = putbuf;
	}
	else
	{
		sprintf(sendbuf, "%c%s%c",
		    CTCP_DELIM_CHAR, datatag, CTCP_DELIM_CHAR);
		sendp = sendbuf;
	}

	/*
	 * ugh, special case dcc because we don't want to go through
	 * send_text in it's current state.  XXX - fix send_text to
	 * deal with ctcp's as well.
	 */
	if (*to == '=')
		dcc_message_transmit(to + 1, sendp, DCC_CHAT, 0);
	else
/**************************** PATCHED by Flier ******************************/
        {
/****************************************************************************/
		send_to_server("%s %s :%s", type, to, sendp);
/**************************** PATCHED by Flier ******************************/
#ifndef CELEHOOK
                if (datatag && strcmp(datatag,"ACTION"))
                    do_hook(SEND_CTCP_LIST, "%s %s %s %s",type,to,datatag,sendp);
#endif
        }
/****************************************************************************/
}

/*
 * send_ctcp_notice: A simply way to send CTCP replies.   I put this here
 * rather than in ctcp.c to keep my compiler quiet 
 */
void
#ifdef HAVE_STDARG_H
send_ctcp_reply(char *to, char *datatag, char *format, ...)
{
	va_list vl;
#else
send_ctcp_reply(to, datatag, format, arg0, arg1, arg2, arg3, arg4,
		arg5, arg6, arg7, arg8, arg9)
	char	*to,
		*datatag,
		*format;
	char	*arg0,
		*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9;
{
#endif /* HAVE_STDARG_H */
	char	putbuf[BIG_BUFFER_SIZE + 1];

	if (in_on_who)
		return;	/* Silently drop it on the floor */
	if (to && (*to == '='))
		return;	/* don't allow dcc replies */
	strmcat(CTCP_Reply_Buffer, "\001", BIG_BUFFER_SIZE);
	strmcat(CTCP_Reply_Buffer, datatag, BIG_BUFFER_SIZE);
	strmcat(CTCP_Reply_Buffer, " ", BIG_BUFFER_SIZE);
	if (format)
	{
#ifdef HAVE_STDARG_H
		va_start(vl, format);
		vsprintf(putbuf, format, vl);
		va_end(vl);
#else
		sprintf(putbuf, format, arg0, arg1, arg2, arg3, arg4, arg5,
			arg6, arg7, arg8, arg9);
#endif /* HAVE_STDARG_H */
		strmcat(CTCP_Reply_Buffer, putbuf, BIG_BUFFER_SIZE);
	}
	else
		strmcat(CTCP_Reply_Buffer, putbuf, BIG_BUFFER_SIZE);
	strmcat(CTCP_Reply_Buffer, "\001", BIG_BUFFER_SIZE);
}
