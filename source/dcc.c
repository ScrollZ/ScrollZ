/*
 * dcc.c: Things dealing client to client connections. 
 *
 * Written By Troy Rollo <troy@cbme.unsw.oz.au> 
 * mIRC resume code by EPIC
 * Copyright (c) 1995, 2003 EPIC Software Labs
 *
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
 * $Id: dcc.c,v 1.50 2008-05-13 14:58:48 f Exp $
 */

#include "irc.h"

#ifdef ESIX
# include <lan/net_types.h>
#endif /* ESIX */

#if defined(ISC30) && defined(_POSIX_SOURCE)
# undef _POSIX_SOURCE
#include <sys/stat.h>
# define _POSIX_SOURCE
#else
# include <sys/stat.h>
#endif /* ICS30 || _POSIX_SOURCE */

#ifdef HAVE_WRITEV
#include <sys/uio.h>
#endif

/**************************** PATCHED by Flier ******************************/
/*#include "talkd.h"*/
/****************************************************************************/
#include "server.h"
#include "ircaux.h"
#include "whois.h"
#include "lastlog.h"
#include "ctcp.h"
#include "dcc.h"
#include "hook.h"
#include "vars.h"
#include "window.h"
#include "output.h"
#include "newio.h"
#include "crypt.h"

/**************************** PATCHED by Flier ******************************/
/*static	void	dcc_chat _((char *));*/
void	dcc_chat _((char *));
/****************************************************************************/
#ifndef LITE
static	void	dcc_chat_rename _((char *));
#endif
/**************************** PATCHED by Flier ******************************/
/*static	void	dcc_filesend _((char *));
static	void	dcc_getfile _((char *));
static	void	dcc_close _((char *));*/
void	dcc_filesend _((char *));
void	dcc_getfile _((char *));
void	dcc_close _((char *));
/*static	void	dcc_talk _((char *));
static	void	dcc_tmsg _((char *));*/
/****************************************************************************/
#ifndef LITE
static	void	dcc_rename _((char *));
#endif
/**************************** PATCHED by Flier ******************************/
/*static	void	dcc_summon _((char *));*/
/****************************************************************************/
#ifndef LITE
static	void	dcc_send_raw _((char *));
#endif
static	void	process_incoming_chat _((DCC_list *));
/**************************** PATCHED by Flier ******************************/
/*static	void	process_outgoing_file _((DCC_list *));*/
#if defined(NON_BLOCKING_CONNECTS)
static	void	process_outgoing_file _((DCC_list *, int));
#else
static	void	process_outgoing_file _((DCC_list *));
#endif
/****************************************************************************/
static	void	process_incoming_file _((DCC_list *));
/**************************** PATCHED by Flier ******************************/
/*static	void	process_incoming_talk _((DCC_list *));*/
/****************************************************************************/
#ifndef LITE
static	void	process_incoming_raw _((DCC_list *));
static	void	process_incoming_listen _((DCC_list *));
#endif
/**************************** PATCHED by Flier ******************************/
void	dcc_resend _((char *));
void	dcc_regetfile _((char *));

#include "myvars.h"

static int StatusDCC _((DCC_list *));
static void PrintEstablish _((char *, DCC_list *, struct sockaddr_in, unsigned long));
static void PrintComplete _((char *, DCC_list *));
static void PrintError _((char *, char *, char *, char *));

#ifdef EXTRA_STUFF
extern void HandleRename _((char **));
extern void CodeIt _((int, char *, int));
#endif
extern void CheckAutoGet _((char *, char *, char *, char *));
extern void AwaySave _((char *, int));
extern void AddNick2List _((char *, int));
extern struct friends *CheckUsers _((char *, char *));
extern void PrintChatMsg _((DCC_list *, char *, int, int));
extern void PrintMyChatMsg _((char *, char *, int));
extern void CheckDCCSpeed _((DCC_list *, time_t));
extern void RemoveFromQueue _((int));
extern void ColorUserHost _((char *, char *, char *, int));
extern int  CheckServer _((int));
extern int  DecryptMessage _((char *, char *));
extern int  EncryptMessage _((char *, char *));

#ifdef BROKEN_MIRC_RESUME
void dcc_getfile_resume _((char *));
static void dcc_getfile_resume_demanded _((char *, char *, char *, char *));
static void dcc_getfile_resume_start _((char *, char *, char *, char *));
#endif /* BROKEN_MIRC_RESUME */
/****************************************************************************/

#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

struct
{
	char	*name;	/* *MUST* be in ALL CAPITALS */
	int	uniq; /* minimum length to be a unique command */
	void	(*function) _((char *));
}	dcc_commands[] =
{
	{ "CHAT",	2, dcc_chat },
	{ "LIST",	1, dcc_list },
	{ "SEND",	2, dcc_filesend },
	{ "GET",	1, dcc_getfile },
	{ "CLOSE",	2, dcc_close },
/**************************** PATCHED by Flier ******************************/
	/*{ "TALK",	2, dcc_talk },
	{ "TMSG",	2, dcc_tmsg },
	{ "RENAME",	2, dcc_rename },*/
#ifndef LITE
	{ "RENAME",	3, dcc_rename },
#endif
	/*{ "SUMMON",	2, dcc_summon },*/
/****************************************************************************/
#ifndef LITE
	{ "RAW",	2, dcc_send_raw },
#endif
/**************************** PATCHED by Flier ******************************/
	{ "RESEND",	3, dcc_resend },
	{ "REGET",	3, dcc_regetfile },
#ifdef BROKEN_MIRC_RESUME
	{ "RESUME",	4, dcc_getfile_resume },
#endif /* BROKEN_MIRC_RESUME */
/****************************************************************************/
	{ NULL,		0, (void (*) _((char *))) NULL }
};

	char	*dcc_types[] =
{
	"<null>",
	"CHAT",
	"SEND",
	"GET",
/**************************** PATCHED by Flier ******************************/
	/*"TALK",
	"SUMMON",*/
/****************************************************************************/
#ifndef LITE
	"RAW_LISTEN",
	"RAW",
#endif
/**************************** PATCHED by Flier ******************************/
	"RESEND",
	"REGET", 
/****************************************************************************/
	NULL
};

/* this is such a fucking kludge */

struct	deadlist
{
	DCC_list *it;
	struct deadlist *next;
}	*deadlist = NULL;

extern	int	in_ctcp_flag;
extern	char	MyHostName[];
extern	struct	in_addr	MyHostAddr, forced_ip_addr;
extern int dgets_errno;
static	off_t	filesize = 0;

DCC_list	*ClientList = NULL;

static	void	add_to_dcc_buffer _((DCC_list *, char *));
static	void	dcc_really_erase _((void));
static	void	dcc_add_deadclient _((DCC_list *));
static	int	dcc_open _((DCC_list *));
static	char	*dcc_time _((time_t));

/**************************** PATCHED by Flier ******************************/
static int StatusDCC(Client)
DCC_list *Client;
{
    int  size;
    int  flags;
    int  change=0;
    int  percentage;
    long completed;
    char type;
    char tmpbuf[mybufsize/16];

    if (DCCDone || !Client || !(Client->filesize)) new_free(&CurrentDCC);
    else {
        flags=(Client->flags)&DCC_TYPES;
        if (flags==DCC_FILEREAD) {
            completed=Client->bytes_read;
            type='G';
        }
        else if (flags==DCC_FILEREGET) {
            completed=Client->bytes_read+Client->resendoffset;
            type='g';
        }
        else if (flags==DCC_FILEOFFER) {
            completed=Client->bytes_sent;
            type='S';
        }
        else {
            completed=Client->bytes_sent+Client->resendoffset;
            type='s';
        }
        size=Client->filesize;
        if (size>=10000000) percentage=completed/(size/100);
        else percentage=completed*100/size;
        snprintf(tmpbuf,sizeof(tmpbuf),"%s %c%%%d",Client->user,type,percentage);
        if (!CurrentDCC || (CurrentDCC && strcmp(tmpbuf,CurrentDCC))) {
            change=1;
            malloc_strcpy(&CurrentDCC,tmpbuf);
        }
    }
    DCCDone=0;
    return(change);
}

static void PrintEstablish(type,Client,remaddr,byteoffset)
char *type;
DCC_list *Client;
struct sockaddr_in remaddr;
unsigned long byteoffset;
{
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

#ifdef WANTANSI
#ifdef TDF
    malloc_strcpy(&(Client->addr),inet_ntoa(remaddr.sin_addr));
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%d",ntohs(remaddr.sin_port));
    malloc_strcpy(&(Client->port),tmpbuf1);
#endif
    snprintf(tmpbuf2,sizeof(tmpbuf2),"%sDCC%s %s%s%s connection with %s%s%s",
            CmdsColors[COLDCC].color5,Colors[COLOFF],
            CmdsColors[COLDCC].color3,type,Colors[COLOFF],
            CmdsColors[COLDCC].color1,Client->user,Colors[COLOFF]);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s[%s%s,%d%s]",tmpbuf2,
            CmdsColors[COLDCC].color4,inet_ntoa(remaddr.sin_addr),
            ntohs(remaddr.sin_port),Colors[COLOFF]);
    if (byteoffset) {
        snprintf(tmpbuf2,sizeof(tmpbuf2)," at %s%ld%s bytes",
                CmdsColors[COLDCC].color4,byteoffset,Colors[COLOFF]);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
    snprintf(tmpbuf2,sizeof(tmpbuf1),"%s %sestablished%s",
            tmpbuf1,CmdsColors[COLDCC].color4,Colors[COLOFF]);
#else
    snprintf(tmpbuf1,sizeof(tmpbuf1),"DCC %s connection with %s[%s,%d]",
            type,Client->user,inet_ntoa(remaddr.sin_addr),ntohs(remaddr.sin_port));
    if (byteoffset) {
        snprintf(tmpbuf2,sizeof(tmpbuf2)," at %ld bytes",byteoffset);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
    snprintf(tmpbuf2,sizeof(tmpbuf2),"%s established",tmpbuf1);
#endif
    if (CdccVerbose<2) say("%s",tmpbuf2);
    if (away_set || LogOn) AwaySave(tmpbuf2,SAVEDCC);
}

static void PrintComplete(type,Client)
char *type;
DCC_list *Client;
{
    int  flags=(Client->flags)&DCC_TYPES;
    int  premature;
    char *tmpstr="from";
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/4];
    time_t xtime=time((time_t *) 0)-Client->starttime;
    double sent,oldsent;

    if (flags==DCC_FILEREAD || flags==DCC_FILEREGET)
        sent=Client->bytes_read-Client->resendoffset+Client->bytes_read;
    else if (flags==DCC_FILEOFFER || flags==DCC_RESENDOFFER) {
        sent=Client->bytes_sent-Client->resendoffset;
        tmpstr="to";
    }
    else {
        sent=Client->bytes_sent;
        tmpstr="to";
    }
    if (sent<=0) sent=1;
    oldsent=sent;
    premature=!strcmp(tmpstr,"from") && (sent<(double) Client->filesize-Client->resendoffset);
    sent/=(double)1024.0;
    if (xtime<=0) xtime=1;
#ifdef WANTANSI
    snprintf(tmpbuf2,sizeof(tmpbuf2),"%sDCC%s %s%s%s %s %s",
            CmdsColors[COLDCC].color5,Colors[COLOFF],
            CmdsColors[COLDCC].color3,type,Colors[COLOFF],
            Client->description,tmpstr);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s %s%s%s %scompleted%s%s %.2f kb/sec",tmpbuf2,
            CmdsColors[COLDCC].color1,Client->user,Colors[COLOFF],
            CmdsColors[COLDCC].color4,premature?" prematurely":"",Colors[COLOFF],
            (sent/(double) xtime));
#else
    snprintf(tmpbuf1,sizeof(tmpbuf1),"DCC %s %s %s %s completed%s %.2f kb/sec",type,Client->description,
            tmpstr,Client->user,premature?" prematurely":"",(sent/(double) xtime));
#endif
    if (premature) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),", %.0f bytes left",(double) Client->filesize-oldsent);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
    if (!sent) xtime=0;
    snprintf(tmpbuf2,sizeof(tmpbuf2)," [%02ld:%02ld]",xtime/60,xtime%60);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    if (CdccVerbose<2) say("%s",tmpbuf1);
    if (away_set || LogOn) AwaySave(tmpbuf1,SAVEDCC);
}

static void PrintError(type,user,description,error)
char *type;
char *user;
char *description;
char *error;
{
    char *tmpstr="to";
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

#ifdef WANTANSI
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%sDCC%s %s%s%s ",
            CmdsColors[COLDCC].color5,Colors[COLOFF],
            CmdsColors[COLDCC].color3,type,Colors[COLOFF]);
    if (!error) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s ",description);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
    snprintf(tmpbuf2,sizeof(tmpbuf2),"connection %s %s%s%s lost",tmpstr,
            CmdsColors[COLDCC].color1,user,Colors[COLOFF]);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    if (error) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),": %s",error);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
#else
    snprintf(tmpbuf1,sizeof(tmpbuf1),"DCC %s ",type);
    if (!error) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s ",description);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
    snprintf(tmpbuf2,sizeof(tmpbuf2),"connection %s %s lost",tmpstr,user);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    if (error) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),": %s",error);
        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    }
#endif
    if (CdccVerbose<2) say("%s",tmpbuf1);
    if (away_set || LogOn) AwaySave(tmpbuf1,SAVEDCC);
}

static void OverWrite(fullname,Client)
char **fullname;
DCC_list *Client;
{
    struct stat statbuf;

    if (CdccOverWrite || !fullname || !Client) return;
    while ((stat(*fullname,&statbuf))==0) {
        malloc_strcat(fullname,"_");
        malloc_strcat(&(Client->description),"_");
    }
}
/****************************************************************************/

/*
 * dcc_searchlist searches through the dcc_list and finds the client
 * with the the flag described in type set.
 */
DCC_list *
dcc_searchlist(name, user, type, flag, othername)
	char	*name,
		*user;
	int	type,
		flag;
	char	*othername;
{
	DCC_list **Client, *NewClient;

	for (Client = (&ClientList); *Client ; Client = (&(**Client).next))
	{
		if ((((**Client).flags&DCC_TYPES) == type) &&
		    ((!name || (!my_stricmp(name, (**Client).description))) ||
		    (othername && (**Client).othername && (!my_stricmp(othername, (**Client).othername)))) &&
		    (my_stricmp(user, (**Client).user)==0))
			return *Client;
	}
	if (!flag)
		return NULL;
	*Client = NewClient = (DCC_list *) new_malloc(sizeof(DCC_list));
	NewClient->flags = type;
	NewClient->read = NewClient->write = NewClient->file = -1;
	NewClient->filesize = filesize;
	NewClient->next = (DCC_list *) 0;
	NewClient->user = NewClient->description = NewClient->othername = NULL;
	NewClient->bytes_read = NewClient->bytes_sent = 0L;
	NewClient->starttime = 0;
	NewClient->buffer = 0;
	malloc_strcpy(&NewClient->description, name);
	malloc_strcpy(&NewClient->user, user);
	malloc_strcpy(&NewClient->othername, othername);
	time(&NewClient->lasttime);
/**************************** PATCHED by Flier ******************************/
        NewClient->server=from_server;
        NewClient->resendoffset=0;
        NewClient->CdccTime=time((time_t *) 0);
        NewClient->minspeed=0.0;
#ifdef TDF
        NewClient->addr=(char *) 0;
        NewClient->port=(char *) 0;
#endif
/****************************************************************************/
	return NewClient;
}

static	void
dcc_add_deadclient(client)
	DCC_list *client;
{
	struct deadlist *new;

	new = (struct deadlist *) new_malloc(sizeof(struct deadlist));
	new->next = deadlist;
	new->it = client;
	deadlist = new;
}

/*
 * dcc_erase searches for the given entry in the dcc_list and
 * removes it
 */
void
dcc_erase(Element)
	DCC_list	*Element;
{
	DCC_list	**Client;

	for (Client = &ClientList; *Client; Client = &(**Client).next)
		if (*Client == Element)
		{
			*Client = Element->next;

			if ((Element->flags & DCC_TYPES) != DCC_RAW_LISTEN)
				new_close(Element->write);
			new_close(Element->read);
			if (Element->file != -1)
				new_close(Element->file);
			new_free(&Element->description);
/**************************** PATCHED by Flier ******************************/
                        if (Element->user == ComplLast || Element == ComplNext)
                            ComplLast = ComplNext = NULL;
/****************************************************************************/
			new_free(&Element->user);
 			new_free(&Element->othername);
			new_free(&Element->buffer);
/**************************** PATCHED by Flier ******************************/
#ifdef TDF
                        new_free(&(Element->addr));
                        new_free(&(Element->port));
#endif
/****************************************************************************/
			new_free(&Element);
			return;
		}
}

static	void
dcc_really_erase()
{
	struct deadlist *dies;

	while ((dies = deadlist) != NULL)
	{
		deadlist = deadlist->next;
		dcc_erase(dies->it);
 		new_free(&dies);
	}
}

/*
 * Set the descriptor set to show all fds in Client connections to
 * be checked for data.
 */
void
set_dcc_bits(rd, wd)
	fd_set	*rd, *wd;
{
	DCC_list	*Client;

	for (Client = ClientList; Client != NULL; Client = Client->next)
	{
#ifdef DCC_CNCT_PEND
		if (Client->write != -1 && (Client->flags & DCC_CNCT_PEND))
			FD_SET(Client->write, wd);
#endif
#if defined(NON_BLOCKING_CONNECTS)
		if (Client->write != -1 && Client->hyperdcc &&
                    ((Client->flags & DCC_TYPES) == DCC_FILEOFFER) &&
                    (!Client->eof))
                    FD_SET(Client->write, wd);
#endif
		if (Client->read != -1)
			FD_SET(Client->read, rd);
        }
}

/*
 * Check all DCCs for data, and if they have any, perform whatever
 * actions are required.
 */
void
dcc_check(rd, wd)
	fd_set	*rd,
		*wd;
{
	DCC_list	**Client;
	struct	timeval	time_out;
	int	previous_server;
	int	lastlog_level;
/**************************** PATCHED by Flier ******************************/
#ifdef NON_BLOCKING_CONNECTS
        char tmpbuf[mybufsize/32];
#endif
/****************************************************************************/

	previous_server = from_server;
	from_server = (-1);
	time_out.tv_sec = time_out.tv_usec = 0;
	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	for (Client = (&ClientList); *Client != NULL && !break_io_processing;)
	{
#ifdef NON_BLOCKING_CONNECTS
		/*
		 * run all connect-pending sockets.. suggested by deraadt@theos.com
		 */
		if ((*Client)->flags & DCC_CNCT_PEND)
		{
			struct sockaddr_in	remaddr;
			int	rl = sizeof(remaddr);

			if (getpeername((*Client)->read, (struct sockaddr *) &remaddr, &rl) != -1)
			{
				if ((*Client)->flags & DCC_OFFER)
				{
					(*Client)->flags &= ~DCC_OFFER;
 					save_message_from();
 					message_from((*Client)->user, LOG_DCC);
					if (((*Client)->flags & DCC_TYPES) != DCC_RAW)
/**************************** PATCHED by Flier ******************************/
						/*say("DCC %s connection with %s[%s,%d] established",
							dcc_types[(*Client)->flags&DCC_TYPES], (*Client)->user,
							inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port));*/
                                            PrintEstablish(dcc_types[(*Client)->flags&DCC_TYPES],
                                                           *Client,remaddr,(*Client)->bytes_read);
 					restore_message_from();
/****** Coded by Zakath ******/
                                        if (((*Client)->flags&DCC_TYPES)==DCC_FILEREAD ||
                                            ((*Client)->flags&DCC_TYPES)==DCC_FILEREGET) CdccRecvNum++;
                                        if (((*Client)->flags&DCC_TYPES)==DCC_CHAT) {
                                            snprintf(tmpbuf,sizeof(tmpbuf),"=%s",(*Client)->user);
                                            AddNick2List(tmpbuf,(*Client)->server);
                                        }
                                        update_all_status();
/*****************************/
/****************************************************************************/
				}
				(*Client)->starttime = time(NULL);
				(*Client)->flags &= ~DCC_CNCT_PEND;
                                set_blocking((*Client)->read);
				if ((*Client)->read != (*Client)->write)
					set_blocking((*Client)->write);
                                /* Fix by flier: execute hook for DCC RAW connect */
                                if ((((*Client)->flags & DCC_TYPES) == DCC_RAW) &&
                                    do_hook(DCC_RAW_LIST, "%d %s E %d",
                                            (*Client)->write, (*Client)->description,
                                            (*Client)->remport))
                                    put_it("DCC RAW connection to %s on %d via %d established",
                                            (*Client)->description, (*Client)->remport,
                                            (*Client)->write);
			} /* else we're not connected yet */
		}
#endif
/**************************** PATCHED by Flier ******************************/
#if defined(NON_BLOCKING_CONNECTS)
		if ((*Client)->write != -1 && FD_ISSET((*Client)->write, wd) &&
                    (*Client)->hyperdcc)
		{
			switch((*Client)->flags & DCC_TYPES)
			{
			case DCC_FILEOFFER:
				process_outgoing_file(*Client, 0);
				break;
			}
		}
#endif
/****************************************************************************/
		if ((*Client)->read != -1 && FD_ISSET((*Client)->read, rd))
		{
			switch((*Client)->flags & DCC_TYPES)
			{
			case DCC_CHAT:
				process_incoming_chat(*Client);
				break;
#ifndef LITE
			case DCC_RAW_LISTEN:
				process_incoming_listen(*Client);
				break;
			case DCC_RAW:
				process_incoming_raw(*Client);
				break;
#endif
			case DCC_FILEOFFER:
/**************************** PATCHED by Flier ******************************/
			case DCC_RESENDOFFER:
				/*process_outgoing_file(*Client);*/
#if defined(NON_BLOCKING_CONNECTS)
                                if ((*Client)->hyperdcc)
                                    process_outgoing_file(*Client,1);
                                else
                                    process_outgoing_file(*Client,0);
#else
				process_outgoing_file(*Client);
#endif
/****************************************************************************/
				break;
			case DCC_FILEREAD:
/**************************** PATCHED by Flier ******************************/
			case DCC_FILEREGET:
/****************************************************************************/
				process_incoming_file(*Client);
				break;
/**************************** PATCHED by Flier ******************************/
			/*case DCC_TALK:
				process_incoming_talk(*Client);
				break;*/
/****************************************************************************/
			}
		}
		if ((*Client)->flags & DCC_DELETE)
		{
			dcc_add_deadclient(*Client);
			Client = (&(**Client).next);
		}
		else
			Client = (&(**Client).next);
	}
	(void) set_lastlog_msg_level(lastlog_level);
	dcc_really_erase();
	from_server = previous_server;
}

/*
 * Process a DCC command from the user.
 */
void
process_dcc(args)
	char	*args;
{
	char	*command;
	int	i;
 	size_t	len;

	if (!(command = next_arg(args, &args)))
		return;
	len = strlen(command);
	upper(command);
	for (i = 0; dcc_commands[i].name != NULL; i++)
	{
		if (!strncmp(dcc_commands[i].name, command, len))
		{
			if (len < dcc_commands[i].uniq)
			{
				say("DCC command not unique: %s", command );
				return;
			}
 			save_message_from();
			message_from((char *) 0, LOG_DCC);
			dcc_commands[i].function(args);
 			restore_message_from();
			return;
		}
	}
	say("Unknown DCC command: %s", command);
}

static	int
dcc_open(Client)
	DCC_list	*Client;
{
	char    *user,
		*Type;
	struct	sockaddr_in	localaddr;
	struct	in_addr 	myip;
	int	sla;
	int	old_server;
#ifndef NON_BLOCKING_CONNECTS
	struct	sockaddr_in	remaddr;
	int	rl = sizeof(remaddr);
#endif
/**************************** PATCHED by Flier ******************************/
#ifndef NON_BLOCKING_CONNECTS
        char tmpbuf[mybufsize/32];
#endif
	char tmpbuf1[mybufsize/32];
/****************************************************************************/

	user = Client->user;
	old_server = from_server;
	if (-1 == from_server)
		from_server = get_window_server(0);
	myip.s_addr = server_list[from_server].local_addr.s_addr;
	if (myip.s_addr == 0 || myip.s_addr == htonl(0x7f000001))
		myip.s_addr = MyHostAddr.s_addr;
	if (forced_ip_addr.s_addr != 0)
		myip.s_addr = forced_ip_addr.s_addr;
	Type = dcc_types[Client->flags & DCC_TYPES];
	if (Client->flags & DCC_OFFER)
	{
#ifdef DCC_CNCT_PEND
		Client->flags |= DCC_CNCT_PEND;
#endif
		if ((Client->write = connect_by_number(Client->remport,
/**************************** PATCHED by Flier ******************************/
			      /*inet_ntoa(Client->remote), 1)) < 0)*/
			      inet_ntoa(Client->remote),1,1)) < 0)
/****************************************************************************/
		{
 			save_message_from();
			message_from(user, LOG_DCC);
			say("Unable to create connection: %s",
				errno ? strerror(errno) : "Unknown Host");
 			restore_message_from();
			dcc_erase(Client);
			from_server = old_server;
			return 0;
		}
		Client->read = Client->write;
		Client->bytes_read = Client->bytes_sent = 0L;
		Client->flags |= DCC_ACTIVE;
#ifndef NON_BLOCKING_CONNECTS
		Client->flags &= ~DCC_OFFER;
		Client->starttime = time(NULL);
		if (getpeername(Client->read, (struct sockaddr *) &remaddr, &rl) == -1)
		{
 			save_message_from();
			message_from(user, LOG_DCC);
			say("DCC error: getpeername failed: %s", strerror(errno));
 			restore_message_from();
			dcc_erase(Client);
			from_server = old_server;
			return 0;
		}
/**************************** PATCHED by Flier ******************************/
		if ((Client->flags & DCC_TYPES) != DCC_RAW)
		{
 			save_message_from();
			message_from(user, LOG_DCC);
			/*say("DCC %s connection with %s[%s,%d] established",
				Type, user, inet_ntoa(remaddr.sin_addr),
				ntohs(remaddr.sin_port));*/
                        PrintEstablish(Type,Client,remaddr,0L);
 			restore_message_from();
		}
/****** Coded by Zakath ******/
                if ((Client->flags&DCC_TYPES)==DCC_FILEREAD ||
                    (Client->flags&DCC_TYPES)==DCC_FILEREGET) CdccRecvNum++;
                update_all_status();
/*****************************/
                if ((Client->flags&DCC_TYPES)==DCC_CHAT) {
                    snprintf(tmpbuf,sizeof(tmpbuf),"=%s",user);
                    AddNick2List(tmpbuf,Client->server);
                }
/****************************************************************************/
#endif
		from_server = old_server;
		return 1;
	}
	else
	{
#ifdef DCC_CNCT_PEND
		Client->flags |= DCC_WAIT|DCC_CNCT_PEND;
#else
		Client->flags |= DCC_WAIT;
#endif
/**************************** PATCHED by Flier ******************************/
		/*if ((Client->read = connect_by_number(0, empty_string, 1)) < 0)*/
		if ((Client->read=connect_by_number(0,empty_string,1,0))<0)
/****************************************************************************/
		{
 			save_message_from();
 			message_from(user, LOG_DCC);
			say("Unable to create connection: %s",
				errno ? strerror(errno) : "Unknown Host");
 			restore_message_from();
			dcc_erase(Client);
			from_server = old_server;
			return 0;
		}
		sla = sizeof(struct sockaddr_in);
		getsockname(Client->read, (struct sockaddr *) &localaddr, &sla);
		if (Client->flags & DCC_TWOCLIENTS)
		{
			/* patch to NOT send pathname accross */
			char	*nopath;
/**************************** PATCHED by Flier ******************************/
                        /* convert spaces to _ so we can send files with spaces */
                        char    *nospaces=(char *) 0;
/****************************************************************************/

			if ((Client->flags & DCC_FILEOFFER) &&
			    (nopath = rindex(Client->description, '/')))
				nopath++;
			else
				nopath = Client->description;
/**************************** PATCHED by Flier ******************************/
                        malloc_strcpy(&nospaces,nopath);
                        nopath=nospaces;
                        for (;nospaces && *nospaces;nospaces++)
                            if (*nospaces==' ') *nospaces='_';
/****************************************************************************/

			/*
			 * XXX
			 * should make the case below for the filesize into
			 * generic off_t2str() function, or something.  this
			 * cast is merely a STOP-GAP measure.
			 */
			if (Client->filesize)
				send_ctcp(ctcp_type[in_ctcp_flag], user, "DCC",
					 "%s %s %lu %u %ld", Type, nopath,
					 (u_long) ntohl(myip.s_addr),
 					 (unsigned)ntohs(localaddr.sin_port),
					 (long)Client->filesize);
			else
				send_ctcp(ctcp_type[in_ctcp_flag], user, "DCC",
					 "%s %s %lu %u", Type, nopath,
					 (u_long) ntohl(myip.s_addr),
 					 (unsigned) ntohs(localaddr.sin_port));
 			save_message_from();
			message_from(user, LOG_DCC);
			say("Sent DCC %s [%s:%u] request to %s",
			    Type,
			    inet_ntoa(myip),
			    (unsigned) ntohs(localaddr.sin_port),
			    user);
 			restore_message_from();
/**************************** PATCHED by Flier ******************************/
                        new_free(&nopath);
/****************************************************************************/
		}
		snprintf(tmpbuf1, sizeof(tmpbuf1), "%d", ntohs(localaddr.sin_port));
		malloc_strcpy(&Client->othername, tmpbuf1);
		/*
		 * Is this where dcc times are fucked up??  - phone
		 * Yes, it was..  and they are all hunky dory now..
		 */
		Client->starttime = 0;
		from_server = old_server;
		return 2;
	}
}

/**************************** PATCHED by Flier ******************************/
/*static void*/
void
/****************************************************************************/
dcc_chat(args)
	char	*args;
{
	char	*user;
	DCC_list	*Client;

	if ((user = next_arg(args, &args)) == NULL)
	{
		say("You must supply a nickname for DCC CHAT");
		return;
	}
	Client = dcc_searchlist("chat", user, DCC_CHAT, 1, (char *) 0);
	if ((Client->flags&DCC_ACTIVE) || (Client->flags&DCC_WAIT))
	{
		say("A previous DCC CHAT to %s exists", user);
		return;
	}
	Client->flags |= DCC_TWOCLIENTS;
	dcc_open(Client);
}

#ifndef LITE
char	*
dcc_raw_listen(iport)
 	u_int	iport;
{
	DCC_list	*Client;
	char	PortName[10];
	struct	sockaddr_in locaddr;
	char	*RetName = NULL;
	int	size;
	int	lastlog_level;
 	u_short	port = (u_short) iport;

	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if (port && port < 1025)
	{
		say("Cannot bind to a privileged port");
		(void) set_lastlog_msg_level(lastlog_level);
		return NULL;
	}
	snprintf(PortName, sizeof PortName, "%d", port);
	Client = dcc_searchlist("raw_listen", PortName, DCC_RAW_LISTEN, 1, (char *) 0);
	if (Client->flags & DCC_ACTIVE)
	{
		say("A previous DCC RAW_LISTEN on %s exists", PortName);
		(void) set_lastlog_msg_level(lastlog_level);
		return RetName;
	}
	bzero((char *) &locaddr, sizeof(locaddr));
	locaddr.sin_family = AF_INET;
	locaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	locaddr.sin_port = htons(port);
	if (0 > (Client->read = socket(AF_INET, SOCK_STREAM, 0)))
	{
		dcc_erase(Client);
		say("socket() failed: %s", strerror(errno));
		(void) set_lastlog_msg_level(lastlog_level);
		return RetName;
	}
	set_socket_options(Client->read);
	if (bind(Client->read, (struct sockaddr *) &locaddr, sizeof(locaddr))
				== -1)
	{
		dcc_erase(Client);
		say("Could not bind port: %s", strerror(errno));
		(void) set_lastlog_msg_level(lastlog_level);
		return RetName;
	}
	listen(Client->read, 4);
	size = sizeof(locaddr);
	Client->starttime = time((time_t *) 0);
	getsockname(Client->read, (struct sockaddr *) &locaddr, &size);
	Client->write = ntohs(locaddr.sin_port);
	Client->flags |= DCC_ACTIVE;
	snprintf(PortName, sizeof PortName, "%d", Client->write);
	malloc_strcpy(&Client->user, PortName);
	malloc_strcpy(&RetName, PortName);
	(void) set_lastlog_msg_level(lastlog_level);
	return RetName;
}

char	*
dcc_raw_connect(host, iport)
	char	*host;
 	u_int	iport;
{
	DCC_list	*Client;
	char	PortName[10];
	struct	in_addr	address;
	struct	hostent	*hp;
	char	*RetName = (char *) 0;
	int	lastlog_level;
 	u_short	port = (u_short)iport;

	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if ((address.s_addr = inet_addr(host)) == -1)
	{
		hp = gethostbyname(host);
		if (!hp)
		{
			say("Unknown host: %s", host);
			(void) set_lastlog_msg_level(lastlog_level);
			return RetName;
		}
		bcopy(hp->h_addr, &address, sizeof(address));
	}
	snprintf(PortName, sizeof PortName, "%d", port);
	Client = dcc_searchlist(host, PortName, DCC_RAW, 1, (char *) 0);
	if (Client->flags & DCC_ACTIVE)
	{
		say("A previous DCC RAW to %s on %s exists", host, PortName);
		(void) set_lastlog_msg_level(lastlog_level);
		return RetName;
	}
	Client->remport = port;
	bcopy((char *) &address, (char *) &Client->remote, sizeof(address));
	Client->flags = DCC_OFFER | DCC_RAW;
	if (!dcc_open(Client))
		return RetName;
	snprintf(PortName, sizeof PortName, "%d", Client->read);
	malloc_strcpy(&Client->user, PortName);
        /* Fix by flier: don't execute hook if connection is still
           pending (e.g. in case of non-blocking connects) */
	/*if (do_hook(DCC_RAW_LIST, "%s %s E %d", PortName, host, port))*/
#ifdef DCC_CNCT_PEND
	if (!(Client->flags & DCC_CNCT_PEND) && do_hook(DCC_RAW_LIST, "%s %s E %d", PortName, host, port))
#else
	if (do_hook(DCC_RAW_LIST, "%s %s E %d", PortName, host, port))
#endif
		put_it("DCC RAW connection to %s on %s via %d established",
				host, PortName, port);
	malloc_strcpy(&RetName, PortName);
	(void) set_lastlog_msg_level(lastlog_level);
	return RetName;
}
#endif /* LITE */

/*char    *talk_errors[] =
{
	"<No Error>",
	"User not logged in",
	"Connection failed",
	"Remote host does not recognise us",
	"Your party is refusing writes",
	"Unknown request",
	"Unknown protocol version",
	"Unable to decipher your address",
	"Unable to decipher return address"*/ /* How the hell does it get
						back then? */
/*};*/

/**************************** PATCHED by Flier ******************************/
/*static	void
dcc_talk(args)
	char	*args;
{
	char	*user;
	char	*host;
	struct	hostent	*hp;
	int	status;

	DCC_list	*Client;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	{
	 	say("You are not permitted to use DCC TALK");
		return;
	}
#endif*/ /* DAEMON_UID */
	/*if ((user = next_arg(args, &args)) == NULL)
	{
		say("You must supply a user[@host] for DCC TALK");
		return;
	}
	if ((host = index(user, '@')) != NULL)
		*(host++) = '\0';
	else
		host = MyHostName;
	Client = dcc_searchlist(host, user, DCC_TALK, 1, (char *) 0);
	if (Client->flags & DCC_ACTIVE || Client->flags & DCC_WAIT)
	{
		say("A previous DCC TALK to %s@%s exists", user, host);
		return;
	}
	if (host != MyHostName)
	{
		if ((hp = gethostbyname(host)) == NULL)
		{
			say("Unable to find address for %s", host);
			dcc_erase(Client);
			return;
		}
		bcopy(hp->h_addr, (char *) &(Client->remote),
			sizeof(struct in_addr));
	}
	else
		bcopy((char *) &MyHostAddr, (char *) &(Client->remote),
			sizeof(struct in_addr));
	if ((Client->file = connect_by_number(-1, empty_string, 0)) < 0)
	{
		say("Unable to create DCC TALK connection: %s",
				errno ? strerror(errno) : "Unknown Host");
		dcc_erase(Client);
		return;
	}
	say("Checking for invitation on caller's machine");
	if (!(status = send_talk_control(Client, DCC_TALK_CHECK)))
	{
		new_close(Client->file);
		dcc_erase(Client);
		say("DCC TALK: connection timed out");
		return;
	}
	if (--status || (Client->read = connect_by_number(Client->remport,
				inet_ntoa(Client->remote), 0)) < 0)
	{
		say("Inviting %s@%s", Client->user, Client->description);
		if ((Client->read = connect_by_number(0, empty_string, 0)) == -1 ||
		    !send_talk_control(Client, DCC_TALK_INVITE) ||
		    !(status=send_talk_control(Client, DCC_TALK_ANNOUNCE)))
		{
			send_talk_control(Client, DCC_TALK_DELETE_LOCAL);
			new_close(Client->read);
			new_close(Client->file);
			dcc_erase(Client);
			return;
		}
		if (--status)
		{
			new_close(Client->read);
			new_close(Client->file);
			dcc_erase(Client);
			say("DCC TALK: %s", talk_errors[status]);
			return;
		}
		say("Waiting for your party to respond");
		Client->flags |= DCC_WAIT;
	}
	else
	{
		say("Connected to %s@%s", Client->user, Client->description);
		Client->write = Client->read;
		send(Client->write,  "\008\025\027", 3, 0);
		recv(Client->read, Client->talkchars, 3, 0);
		Client->bytes_read = Client->bytes_sent = 3;
		Client->flags |= DCC_ACTIVE;
	}
}

static	void
dcc_summon(args)
	char	*args;
{
	char	*user;
	char	*host;
	struct	hostent	*hp;
	DCC_list *Client;

	if (0 == (user = next_arg(args, &args)))
	{
		say("You must supply a user[@host] for DCC SUMMON");
		return;
	}
	if (0 != (host = index(user, '@')))
		*host++ = '\0';
	else
		host = MyHostName;

	Client = dcc_searchlist(host, user, DCC_SUMMON, 1, (char *) 0);

	if (host != MyHostName)
	{
		if (0 == (hp = gethostbyname(host)))
		{
			say("Unable to find address for %s", host);
			dcc_erase(Client);
			return;
		}
		bcopy(hp->h_addr, (char *) &(Client->remote),
			sizeof(struct in_addr));
	}
	else
		bcopy((char *) &MyHostAddr, (char *) &(Client->remote),
			sizeof(struct in_addr));
	if ((Client->file = connect_by_number(-1, empty_string, 0)) < 0)
	{
		say("Unable to create DCC SUMMON connection: %s",
				errno ? strerror(errno) : "Unknown Host");
		return;
	}
	if (0 == send_talk_control(Client, DCC_TALK_SUMMON))
		say("DCC SUMMON: connection timed out");
	send_talk_control(Client, DCC_TALK_DELETE_SUMMON);
	new_close(Client->file);
	dcc_erase(Client);
}

int    
send_talk_control(Client, MessageType)
	DCC_list	*Client;
	int     MessageType;
{
	CTL_MSG	Message;
	CTL_RESPONSE	Response;
	static	long	 SeqNum = 0;
	struct	sockaddr_in SockAddr;
	struct	timeval timeout;
	fd_set	selset;
	int	i;
	int	dummy;

	Message.vers = TALK_VERSION;
	Message.id_num = htonl(SeqNum);
	SeqNum++;*/ /* Not in the htonl because on some machines it's a macro */
	/*dummy = sizeof(SockAddr);
	getsockname(Client->file, (struct sockaddr *) &SockAddr, &dummy);
	Message.ctl_addr = (*(struct sockaddr *) &SockAddr);
	if (Client->read > 0)
	{
		getsockname(Client->read, (struct sockaddr *) &SockAddr,
			    &dummy);
		SockAddr.sin_addr=MyHostAddr;
	}
	Message.addr = (*(struct sockaddr *) &SockAddr);
	strncpy(Message.l_name, username, NAME_SIZE);
	Message.l_name[NAME_SIZE - 1] = '\0';
	strncpy(Message.r_name, Client->user, NAME_SIZE);
	Message.r_name[NAME_SIZE - 1] = '\0';
	Message.r_tty[0] = '\0';
	Message.pid = getpid();
	SockAddr.sin_addr = Client->remote;
	SockAddr.sin_port = htons(518);
	switch(MessageType)
	{
	case DCC_TALK_CHECK:
		Message.type = LOOK_UP;
		break;
	case DCC_TALK_INVITE:
		Message.type = LEAVE_INVITE;
		SockAddr.sin_addr = MyHostAddr;
		break;
	case DCC_TALK_ANNOUNCE:
		Message.type = ANNOUNCE;
		break;
	case DCC_TALK_DELETE_LOCAL:
		SockAddr.sin_addr = MyHostAddr;
	case DCC_TALK_DELETE_REMOTE:
		Message.type = DELETE;
		break;
	case DCC_TALK_SUMMON:
		strcpy(Message.l_name, "I:");
		strcat(Message.l_name, get_server_nickname(from_server));
		Message.type = ANNOUNCE;
		break;
	case DCC_TALK_DELETE_SUMMON:
		strcpy(Message.l_name, "I:");
		strcat(Message.l_name, get_server_nickname(from_server));
		Message.type = DELETE;
		break;
	}
	for (i = 0; i < 3; i++)
	{
		if (sendto(Client->file, (char *) &Message, sizeof(Message), 0,
			   (struct sockaddr *) &SockAddr,
			   sizeof(SockAddr))!=sizeof(Message))
		{
			perror("sendto");
			return 0;
		}
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		FD_ZERO(&selset);
		FD_SET(Client->file, &selset);
		switch(select(Client->file+1, &selset, NULL, NULL, &timeout))
		{
		case -1:
			perror("select");
			return 0;
		case 1:
			do
			{
				recv(Client->file, (char *) &Response, sizeof(Response), 0);
				FD_ZERO(&selset);
				FD_SET(Client->file, &selset);
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;
			}
			while (select(Client->file + 1, &selset, NULL, NULL,
					&timeout) > 0);
			if (Response.type != Message.type)
				continue;
			if (LOOK_UP == Response.type &&
			    SUCCESS == Response.answer)
			{
				SockAddr = (*(struct sockaddr_in *)
					&Response.addr);
				Client->remote = SockAddr.sin_addr;
				Client->remport = ntohs(SockAddr.sin_port);
			}
			return Response.answer + 1;
		}
	}
	return 0;
}*/
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
dcc_filesend(args)
	char	*args;
{
	char	*user;
	char	*filename,
		*fullname;
	DCC_list *Client;
	char	FileBuf[BIG_BUFFER_SIZE+1];
	struct	stat	stat_buf;

#ifdef  DAEMON_UID
	if (DAEMON_UID == getuid())
	{
		say("You are not permitted to use DCC to exchange files");
		return;
	}
#endif
	if (0 == (user = next_arg(args, &args)) ||
/**************************** PATCHED by Flier ******************************/
	    /*0 == (filename = next_arg(args, &args)))*/
	    0 == (filename = new_next_arg(args, &args)))
/****************************************************************************/
	{
		say("You must supply a nickname and filename for DCC SEND");
		return;
	}
	if (IS_ABSOLUTE_PATH(filename))
	{
/**************************** Patched by Flier ******************************/
		/*strcpy(FileBuf, filename);*/
		strmcpy(FileBuf, filename, sizeof(FileBuf));
/****************************************************************************/
	}
	else if (*filename == '~')
	{
		if (0 == (fullname = expand_twiddle(filename)))
		{
			yell("Unable to expand %s", filename);
			return;
		}
/**************************** Patched by Flier ******************************/
		/*strcpy(FileBuf, filename);*/
		strmcpy(FileBuf, fullname, sizeof(FileBuf));
/****************************************************************************/
		new_free(&fullname);
	}
	else
	{
/**************************** PATCHED by Flier ******************************/
		/*getcwd(FileBuf, sizeof(FileBuf));
		strcat(FileBuf, "/");
		strcat(FileBuf, filename);*/
                *FileBuf = '\0';
                if (CdccUlDir) {
                    snprintf(FileBuf, sizeof(FileBuf), "%s/", CdccUlDir);
                    if (strncmp(filename, FileBuf, strlen(FileBuf)))
                        strmcpy(FileBuf, CdccUlDir, sizeof(FileBuf));
                    else *FileBuf = '\0';
                }
                if (!(*FileBuf)) getcwd(FileBuf, sizeof(FileBuf));
		strmcat(FileBuf, "/", sizeof(FileBuf));
		strmcat(FileBuf, filename, sizeof(FileBuf));
/****************************************************************************/
	}
	if (0 != access(FileBuf, R_OK))
	{
		yell("Cannot access %s", FileBuf);
		return;
	}
	stat(FileBuf, &stat_buf);
/* some unix didn't have this ???? */
#ifdef S_IFDIR
	if (stat_buf.st_mode & S_IFDIR)
	{
		yell("Cannot send a directory");
		return;
	}
#endif
	filesize = stat_buf.st_size;
	Client = dcc_searchlist(FileBuf, user, DCC_FILEOFFER, 1, filename);
	if ((Client->file = open(Client->description, O_RDONLY | O_BINARY)) == -1)
	{
		say("Unable to open %s: %s", Client->description,
			errno ? strerror(errno) : "Unknown Host");
		new_close(Client->read);
		Client->read = Client->write = (-1);
		Client->flags |= DCC_DELETE;
		return;
	}
	filesize = 0;
	if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT))
	{
		say("A previous DCC SEND:%s to %s exists", FileBuf, user);
		return;
	}
	Client->flags |= DCC_TWOCLIENTS;
	dcc_open(Client);
}

/**************************** PATCHED by Flier ******************************/
void
dcc_resend(args)
	char	*args;
{
	char	*user;
	char	*filename,
		*fullname;
	DCC_list *Client;
	char	FileBuf[BIG_BUFFER_SIZE+1];
	struct	stat	stat_buf;

#ifdef  DAEMON_UID
	if (DAEMON_UID == getuid())
	{
		say("You are not permitted to use DCC to exchange files");
		return;
	}
#endif

	if (0 == (user = next_arg(args, &args)) ||
	    0 == (filename = new_next_arg(args, &args)))
	{
		say("You must supply a nickname and filename for DCC RESEND");
		return;
	}
	if (IS_ABSOLUTE_PATH(filename))
	{
		strmcpy(FileBuf, filename, sizeof(FileBuf));
	}
	else if (*filename == '~')
	{
		if (0 == (fullname = expand_twiddle(filename)))
		{
			yell("Unable to expand %s", filename);
			return;
		}
		strmcpy(FileBuf, fullname, sizeof(FileBuf));
		new_free(&fullname);
	}
	else
	{
                *FileBuf = '\0';
                if (CdccUlDir) {
                    snprintf(FileBuf, sizeof(FileBuf), "%s/", CdccUlDir);
                    if (strncmp(filename, FileBuf, strlen(FileBuf)))
                        strmcpy(FileBuf, CdccUlDir, sizeof(FileBuf));
                    else *FileBuf = '\0';
                }
                if (!(*FileBuf)) getcwd(FileBuf, sizeof(FileBuf));
		strmcat(FileBuf, "/", sizeof(FileBuf));
		strmcat(FileBuf, filename, sizeof(FileBuf));
	}
	if (0 != access(FileBuf, R_OK))
	{
		yell("Cannot access %s", FileBuf);
		return;
	}
	stat(FileBuf, &stat_buf);
	if (stat_buf.st_mode & S_IFDIR)
	{
		yell("Cannot send a directory");
		return;
	}
	filesize = stat_buf.st_size;
	Client = dcc_searchlist(FileBuf, user, DCC_RESENDOFFER, 1, filename);
	if ((Client->file = open(Client->description, O_RDONLY | O_BINARY)) == -1)
	{
		say("Unable to open %s: %s", Client->description,
			errno ? strerror(errno) : "Unknown Host");
		new_close(Client->read);
		Client->read = Client->write = (-1);
		Client->flags |= DCC_DELETE;
		return;
	}
	filesize = 0;
	if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT))
	{
		say("A previous DCC RESEND:%s to %s exists", FileBuf, user);
		return;
	}
	Client->flags |= DCC_TWOCLIENTS;
	dcc_open(Client);
}
/****************************************************************************/


/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
dcc_getfile(args)
	char	*args;
{
	char	*user;
	char	*filename;
	DCC_list	*Client;
	char	*fullname = (char *) 0;
/**************************** PATCHED by Flier ******************************/
        char    *nospaces;
        char    *fullname1=(char *) 0;
/****************************************************************************/

#ifdef  DAEMON_UID
	if (DAEMON_UID == getuid())
	{
		say("You are not permitted to use DCC to exchange files");
		return;
	}
#endif
	if (0 == (user = next_arg(args, &args)))
	{
		say("You must supply a nickname for DCC GET");
		return;
	}
/**************************** PATCHED by Flier ******************************/
	/*filename = next_arg(args, &args);*/
        /* support filenames enclosed in quotes */
        filename = new_next_arg(args, &args);
/****************************************************************************/
	if (0 == (Client = dcc_searchlist(filename, user, DCC_FILEREAD, 0, (char *) 0)))
	{
		if (filename)
			say("No file (%s) offered in SEND mode by %s",
					filename, user);
		else
			say("No file offered in SEND mode by %s", user);
		return;
	}
	if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT))
	{
		if (filename)
			say("A previous DCC GET:%s to %s exists", filename, user);
		else
			say("A previous DCC GET to %s exists", user);
		return;
	}
	if (0 == (Client->flags & DCC_OFFER))
	{
		say("I'm a teapot!");
		dcc_erase(Client);
		return;
	}

/**************************** PATCHED by Flier ******************************/
	/*if (0 == (fullname = expand_twiddle(Client->description)))
		malloc_strcpy(&fullname, Client->description);*/
#ifdef EXTRA_STUFF
        HandleRename(&(Client->description));
#endif
        if (0 == (fullname1 = expand_twiddle(Client->description)))
		malloc_strcpy(&fullname1, Client->description);
        if (CdccDlDir) {
            malloc_strcpy(&fullname,CdccDlDir);
            malloc_strcat(&fullname,"/");
        }
        malloc_strcat(&fullname,fullname1);
        new_free(&fullname1);
        for (nospaces=fullname;nospaces && *nospaces;nospaces++)
            if (*nospaces==' ') *nospaces='_';
        OverWrite(&fullname,Client);
/****************************************************************************/

	Client->file = open(fullname, O_BINARY | O_WRONLY | O_TRUNC | O_CREAT, 0644);
	new_free(&fullname);
	if (-1 == Client->file)
	{
		say("Unable to open %s: %s", Client->description,
				errno ? strerror(errno) : "<No Error>");
                return;
        }
	Client->flags |= DCC_TWOCLIENTS;
	Client->bytes_sent = Client->bytes_read = 0L;
	if (!dcc_open(Client))
		close(Client->file);
}

#ifdef BROKEN_MIRC_RESUME
void
dcc_getfile_resume(args)
	char	*args;
{
	char		*user;
	char		*filename;
	char		*fullname = NULL;
	DCC_list	*Client;
	struct stat	sb;

	if (!(user = next_arg(args, &args))) {
		say("You must supply a nickname for DCC RESUME");
		return;
	}

	filename = new_next_arg(args, &args);

	if (!(Client = dcc_searchlist(filename, user, DCC_FILEREAD, 0, 0))) {
		if (filename)
			say("No file (%s) offered in SEND mode by %s",
				filename, user);
		else
			say("No file offered in SEND mode by %s", user);
		return;
	}

	if (CdccDlDir) {
		malloc_strcpy(&fullname, CdccDlDir);
		malloc_strcat(&fullname, "/");
	}
	malloc_strcat(&fullname, Client->description);

	if (stat(fullname, &sb) == -1) {
		say("DCC RESUME: Can't resume if the file doesn't exist.");
		new_free(&fullname);
		return;
	}

	if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT)) {
		if (filename)
			say("A previous DCC GET:%s to %s exists", filename, user);
		else
			say("A previous DCC GET to %s exists", user);
		return;
	}

	Client->file = open(fullname, O_BINARY | O_WRONLY, 0644);
	new_free(&fullname);
	if (-1 == Client->file) {
		say("Unable to open %s: %s", Client->description,
			errno ? strerror(errno) : "<No Error>");
		return;
	}

	lseek(Client->file, sb.st_size, SEEK_SET);
	Client->bytes_sent = 0;
	Client->bytes_read = Client->resendoffset = sb.st_size;

	send_ctcp(ctcp_type[CTCP_PRIVMSG], user, "DCC", "RESUME %s %d %d",
		Client->description, Client->remport, sb.st_size);
}

void
dcc_getfile_resume_demanded(user, filename, port, offset)
	char	*user,
		*filename,
		*port,
		*offset;
{
	DCC_list	*Client;

	if (!(Client = dcc_searchlist(filename, user, DCC_FILEOFFER, 0, port)))
		return;	/* fake */

	Client->bytes_sent = Client->resendoffset = atoi(offset);
	Client->bytes_read = 0;

	lseek(Client->file, Client->bytes_sent, SEEK_SET);

	send_ctcp(ctcp_type[CTCP_PRIVMSG], user, "DCC", "ACCEPT %s %s %s",
		filename, port, offset);
}

void
dcc_getfile_resume_start(user, filename, port, offset)
	char	*user,
		*filename,
		*port,
		*offset;
{
	DCC_list	*Client;

	if (!strcmp(filename, "file.ext"))
		filename = NULL;

	if (!(Client = dcc_searchlist(filename, user, DCC_FILEREAD, 0, port)))
		return;	/* fake */

	Client->flags |= DCC_TWOCLIENTS;
	if (!dcc_open(Client))
		close(Client->file);

	Client->bytes_read = atoi(offset);
}
#endif /* BROKEN_MIRC_RESUME */

/**************************** PATCHED by Flier ******************************/
void
dcc_regetfile(args)
	char	*args;
{
	char	*user;
	char	*filename;
	DCC_list	*Client;
	char	*fullname = (char *) 0;
	struct stat buf;
	struct transfer_struct transfer_orders;
/**************************** PATCHED by Flier ******************************/
        char *fullname1 = NULL;
/****************************************************************************/

#ifdef  DAEMON_UID
	if (DAEMON_UID == getuid())
	{
		say("You are not permitted to use DCC to exchange files");
		return;
	}
#endif
	if (0 == (user = next_arg(args, &args)))
	{
		say("You must supply a nickname for DCC REGET");
		return;
	}
	filename = next_arg(args, &args);
	if (((Client = dcc_searchlist(filename, user, DCC_FILEREGET, 0, (char *) 0)) == 0))
	{
		if (filename)
			say("No file (%s) offered in RESEND mode by %s",
					filename, user);
		else
			say("No file offered in RESEND mode by %s", user);
		return;
	}
	if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT))
	{
		if (filename)
			say("A previous DCC REGET:%s to %s exists", filename, user);
		else
			say("A previous DCC REGET to %s exists", user);
		return;
	}
	if (0 == (Client->flags & DCC_OFFER))
	{
		say("I'm a teapot!");
		dcc_erase(Client);
		return;
	}

/**************************** PATCHED by Flier ******************************/
#ifdef EXTRA_STUFF
        HandleRename(&Client->description);
#endif
	if (0 == (fullname1 = expand_twiddle(Client->description)))
		malloc_strcpy(&fullname1, Client->description);
        if (CdccDlDir) {
            malloc_strcpy(&fullname, CdccDlDir);
            malloc_strcat(&fullname, "/");
        }
        malloc_strcat(&fullname, fullname1);
        new_free(&fullname1);
/****************************************************************************/

        Client->file = open(fullname, O_BINARY | O_WRONLY | O_CREAT, 0644);
	new_free(&fullname);
	if (-1 == Client->file)
	{
		say("Unable to open %s: %s", Client->description,
				errno ? strerror(errno) : "<No Error>");
                return;
	}
	Client->flags |= DCC_TWOCLIENTS;
	Client->bytes_sent = Client->bytes_read = 0L;

        if (dcc_open(Client)) {
            /* seek to the end of the file about to be resumed */
            lseek(Client->file, 0, SEEK_END);

            /* get the size of our file to be resumed */
            fstat(Client->file, &buf);
            Client->resendoffset = buf.st_size;
            say("Telling remote we want to start at %ld bytes",Client->resendoffset);
/**************************** PATCHED by Flier ******************************/
            /*Client->bytes_sent=buf.st_size;*/
/****************************************************************************/

            transfer_orders.packet_id = DCC_PACKETID;
            transfer_orders.byteoffset = buf.st_size;
            transfer_orders.byteorder = byteordertest();

            /* send a packet to the sender with transfer resume instructions */
            send(Client->read, &transfer_orders, sizeof(transfer_orders), 0);
        }
        else
            close(Client->file);
}
/****************************************************************************/

void
register_dcc_offer(user, type, description, address, port, size)
	char	*user;
	char	*type;
	char	*description;
	char	*address;
	char	*port;
	char	*size;
{
	DCC_list	*Client;
	int	CType;
	char	*c;
	u_long	TempLong;
	unsigned	TempInt;
	int	do_auto = 0;	/* used in dcc chat collisions */
	char	*cmd = (char *) 0;
 	int	lastlog_level;
/**************************** PATCHED by Flier ******************************/
        int  warning=0;
        char *fromhost;
        char tmpbuf1[mybufsize];
        char tmpbuf2[mybufsize];
        char tmpbuf3[mybufsize];
        struct in_addr addr1;
        struct in_addr addr2;
        struct hostent *hostent_fromhost;
        struct friends *tmpfriend;
/****************************************************************************/

 	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if (0 != (c = rindex(description, '/')))
		description = c + 1;
	if ('.' == *description)
		*description = '_';
	if (size && *size)
		filesize = atoi(size);
	else
		filesize = 0;
	malloc_strcpy(&cmd, type);
	upper(cmd);
	if (!strcmp(cmd, "CHAT"))
		CType = DCC_CHAT;
#ifndef  DAEMON_UID
	else if (!strcmp(cmd, "SEND"))
#else
	else if (!strcmp(cmd, "SEND") && DAEMON_UID != getuid())
#endif /*DAEMON_UID*/
		CType = DCC_FILEREAD;
/**************************** PATCHED by Flier ******************************/
#ifndef  DAEMON_UID
	else if (!strcmp(type, "RESEND"))
#else
	else if(!strcmp(type, "RESEND") && DAEMON_UID != getuid())
#endif /* DAEMON_UID */
		CType = DCC_FILEREGET;
#ifdef BROKEN_MIRC_RESUME
	else if (!strcmp(type, "RESUME")) {
		dcc_getfile_resume_demanded(user, description, address, port);
		goto out;
	}
	else if (!strcmp(type, "ACCEPT")) {
		dcc_getfile_resume_start(user, description, address, port);
		goto out;
	}
#endif /* BROKEN_MIRC_RESUME */
/****************************************************************************/
	else
	{
/**************************** PATCHED by Flier ******************************/
		/*say("Unknown DCC %s (%s) received from %s", type, description, user);*/
#ifdef WANTANSI
                snprintf(tmpbuf1,sizeof(tmpbuf1),"Unknown %sDCC%s %s%s%s (%s%s%s) received from",
                        CmdsColors[COLDCC].color5,Colors[COLOFF],
                        CmdsColors[COLDCC].color3,type,Colors[COLOFF],
                        CmdsColors[COLDCC].color4,description,Colors[COLOFF]);
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%s %s%s%s",tmpbuf1,
                        CmdsColors[COLDCC].color1,user,Colors[COLOFF]);
                ColorUserHost(FromUserHost,CmdsColors[COLDCC].color2,tmpbuf3,1);
                strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
#else
                snprintf(tmpbuf2,sizeof(tmpbuf2),"Unknown DCC %s (%s) received from %s (%s)",type,
                        description,user,FromUserHost);
#endif
                say("%s",tmpbuf2);
                if (away_set || LogOn) AwaySave(tmpbuf1,SAVEDCC);
/****************************************************************************/
                goto out;
        }
	Client = dcc_searchlist(description, user, CType, 1, (char *) 0);
	filesize = 0;
	if (Client->flags & DCC_WAIT)
	{
		new_close(Client->read);
		dcc_erase(Client);
		if (DCC_CHAT == CType)
		{
			Client = dcc_searchlist(description, user, CType, 1, (char *) 0);
			do_auto = 1;
		}
		else
		{
			say("DCC %s collision for %s:%s", type, user,
				description);
			send_ctcp_reply(user, "DCC", "DCC %s collision occured while connecting to %s (%s)", type, nickname, description);
                        goto out;
		}
	}
	if (Client->flags & DCC_ACTIVE)
	{
		say("Received DCC %s request from %s while previous session still active", type, user);
                goto out;
	}
	Client->flags |= DCC_OFFER;
	sscanf(address, "%lu", &TempLong);
	Client->remote.s_addr = htonl(TempLong);
	sscanf(port, "%u", &TempInt);
	Client->remport = TempInt;
/**************************** PATCHED by Flier ******************************/
        Client->resendoffset = 0;
	/*if (TempInt < 1024)
	{
		say("DCC %s (%s) request from %s rejected [port = %d]", type, description, user, TempInt);*/
	if (TempInt<1024 || TempInt>65535)
	{
#ifdef WANTANSI
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%sDCC%s %s%s%s (%s%s%s) request from",
                        CmdsColors[COLDCC].color5,Colors[COLOFF],
                        CmdsColors[COLDCC].color3,type,Colors[COLOFF],
                        CmdsColors[COLDCC].color4,description,Colors[COLOFF]);
                ColorUserHost(FromUserHost,CmdsColors[COLDCC].color2,tmpbuf3,1);
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%s %s%s%s %s %srejected%s [Port=%u]",tmpbuf1,
                        CmdsColors[COLDCC].color1,user,Colors[COLOFF],
                        tmpbuf3,CmdsColors[COLDCC].color6,Colors[COLOFF],TempInt);
#else
                snprintf(tmpbuf2,sizeof(tmpbuf2),"DCC %s (%s) request from %s (%s) rejected [Port=%u]",
                        type,description,user,FromUserHost,TempInt);
#endif
                say("%s",tmpbuf2);
                if (away_set || LogOn) AwaySave(tmpbuf3,SAVEDCC);
/****************************************************************************/
		dcc_erase(Client);
                goto out;
	}
	if ((u_long) 0 == TempLong || 0 == Client->remport)
	{
		dcc_erase(Client);
                goto out;
	}
	if (do_auto)
	{
		say("DCC CHAT already requested by %s, connecting...", user);
		dcc_chat(user);
	}
	/*
	 * XXX
	 * should make the case below for the filesize into
	 * generic off_t2str() function, or something.  this
	 * cast is merely a STOP-GAP measure.
	 */
/**************************** PATCHED by Flier ******************************/
	/*else if (Client->filesize)
 		say("DCC %s (%s %ld) request received from %s [%s:%s]", type, description, (long)Client->filesize, user,inet_ntoa(Client->remote), port);
	else
 		say("DCC %s (%s) request received from %s [%s:%s]", type, description, user, inet_ntoa(Client->remote), port);*/
/****************************************************************************/
	if (beep_on_level & LOG_CTCP)
		beep_em(1);
/**************************** PATCHED by Flier ******************************/
	else if (Client->filesize) {
#ifdef WANTANSI
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%sDCC%s %s%s%s (%s%s %ld%s) request ",
                        CmdsColors[COLDCC].color5,Colors[COLOFF],
                        CmdsColors[COLDCC].color3,type,Colors[COLOFF],
                        CmdsColors[COLDCC].color4,description,
                        (long) Client->filesize,Colors[COLOFF]);
                ColorUserHost(FromUserHost,CmdsColors[COLDCC].color2,tmpbuf3,1);
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%sreceived%s from %s%s%s %s [%s:%s]",tmpbuf1,
                        CmdsColors[COLDCC].color4,Colors[COLOFF],
                        CmdsColors[COLDCC].color1,user,Colors[COLOFF],tmpbuf3,
                        inet_ntoa(Client->remote), port);
#else
                snprintf(tmpbuf2,sizeof(tmpbuf2),"DCC %s (%s %ld) request received from %s (%s) [%s:%s]",
                        type,description,(long) Client->filesize,user,
                        FromUserHost,inet_ntoa(Client->remote),port);
#endif
                if (DCCWarning) {
                    strmcpy(tmpbuf1,FromUserHost,sizeof(tmpbuf1));
                    fromhost=index(tmpbuf1,'@');
                    fromhost++;
                    hostent_fromhost=gethostbyname(fromhost);
                    if (!hostent_fromhost) {
#ifdef WANTANSI
                        say("%sWarning%s, incoming DCC has an address that couldn't be figured out",
                            CmdsColors[COLDCC].color6,Colors[COLOFF]);
#else
                        say("%cWarning%c, incoming DCC has an address that couldn't be figured out",
                            bold,bold);
#endif
                        say("It is recommended that you ignore this DCC request");
                        warning=1;
                    }
                    else {
                        addr1.s_addr=*((unsigned long *)hostent_fromhost->h_addr_list[0]);
                        addr2.s_addr=inet_addr(fromhost);
                        if ((addr1.s_addr!=Client->remote.s_addr) &&
                            (addr2.s_addr!=Client->remote.s_addr)) {
                            strmcpy(tmpbuf3,inet_ntoa(addr1),sizeof(tmpbuf3));
#ifdef WANTANSI
                            say("%sWarning%s, fake DCC handshake detected [%s!=%s]",
                                CmdsColors[COLDCC].color6,Colors[COLOFF],
                                tmpbuf3,inet_ntoa(Client->remote));
#else
                            say("%cWarning%c, fake DCC handshake detected [%s!=%s]",bold,bold,
                                tmpbuf3,inet_ntoa(Client->remote));
#endif
                            say("It is recommended that you ignore this DCC request");
                            warning=2;
                        }
                    }
                }
                say("%s",tmpbuf2);
                if (away_set || LogOn) {
                    snprintf(tmpbuf1,sizeof(tmpbuf1),"DCC %s (%s %ld) request received from %s (%s) [%s:%s]",
                            type,description,(long) Client->filesize,user,
                            FromUserHost,inet_ntoa(Client->remote), port);
                    if (warning==1) strmcat(tmpbuf1,", address couldn't be figured out",sizeof(tmpbuf1));
                    else if (warning==2) strmcat(tmpbuf1,", fake DCC handshake detected",sizeof(tmpbuf1));
                    AwaySave(tmpbuf1,SAVEDCC);
                }
                if ((AutoGet>1) || (!warning && AutoGet)) {
#ifdef BROKEN_MIRC_RESUME
			char *fullpath = NULL;
			struct stat sb;

			malloc_strcpy(&fullpath, CdccDlDir);
			malloc_strcat(&fullpath, "/");
			malloc_strcat(&fullpath, description);
			if (!stat(fullpath, &sb) && sb.st_size)
				CheckAutoGet(user,FromUserHost,description,"RESUME");
			else
#endif /* BROKEN_MIRC_RESUME */
				CheckAutoGet(user,FromUserHost,description,type);
#ifdef BROKEN_MIRC_RESUME
			new_free(&fullpath);
#endif /* BROKEN_MIRC_RESUME */
		}
	}
        else {
#ifdef WANTANSI
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%sDCC%s %s%s%s (%s%s%s) request %sreceived",
                        CmdsColors[COLDCC].color5,Colors[COLOFF],
                        CmdsColors[COLDCC].color3,type,Colors[COLOFF],
                        CmdsColors[COLDCC].color4,description,Colors[COLOFF],
                        CmdsColors[COLDCC].color4);
                ColorUserHost(FromUserHost,CmdsColors[COLDCC].color2,tmpbuf3,1);
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%s from %s%s%s %s [%s:%s]",tmpbuf1,Colors[COLOFF],
                        CmdsColors[COLDCC].color1,user,Colors[COLOFF],tmpbuf3,
                        inet_ntoa(Client->remote),port);
#else
                snprintf(tmpbuf2,sizeof(tmpbuf2),"DCC %s (%s) request received from %s (%s) [%s:%s]",
                        type,description,user,FromUserHost,inet_ntoa(Client->remote),port);
#endif
                if (((Client->flags)&DCC_TYPES)==DCC_FILEREAD ||
                    ((Client->flags)&DCC_TYPES)==DCC_FILEREGET) {
#ifdef WANTANSI
                    say("%sWarning%s, incoming DCC has zero file size",
                        CmdsColors[COLDCC].color6,Colors[COLOFF]);
#else
                    say("%cWarning%c, incoming DCC has zero file size",
                        bold,bold);
#endif
                    say("It is recommended that you ignore this DCC request");
                    warning=1;
                }
                say("%s",tmpbuf2);
                if (away_set || LogOn) {
                    snprintf(tmpbuf1,sizeof(tmpbuf1),"DCC %s (%s) request received from %s (%s)",
                            type,description,user,FromUserHost);
                    if (warning) strmcat(tmpbuf1,", file size was zero",sizeof(tmpbuf1));
                    AwaySave(tmpbuf1,SAVEDCC);
                }
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%s!%s",user,FromUserHost);
                if (!my_stricmp("CHAT",type)) {
                    if ((tmpfriend=CheckUsers(tmpbuf1,NULL)) && (tmpfriend->privs)&FLCDCC)
                        dcc_chat(user);
                    malloc_strcpy(&LastChat,user);
                }
                if (((Client->flags)&DCC_TYPES)==DCC_FILEREAD ||
                    ((Client->flags)&DCC_TYPES)==DCC_FILEREGET) {
                    if (AutoGet>1) {
#ifdef BROKEN_MIRC_RESUME
			char *fullpath = NULL;
			struct stat sb;

			malloc_strcpy(&fullpath, CdccDlDir);
			malloc_strcat(&fullpath, "/");
			malloc_strcat(&fullpath, description);
			if (!stat(fullpath, &sb) && sb.st_size)
				CheckAutoGet(user,FromUserHost,description,"RESUME");
			else
#endif /* BROKEN_MIRC_RESUME */
				CheckAutoGet(user,FromUserHost,description,type);
#ifdef BROKEN_MIRC_RESUME
			new_free(&fullpath);
#endif /* BROKEN_MIRC_RESUME */
		    }
                }
        }
/****************************************************************************/
out:
 	set_lastlog_msg_level(lastlog_level);
	new_free(&cmd);
}

static	void
process_incoming_chat(Client)
	DCC_list	*Client;
{
	struct	sockaddr_in	remaddr;
	int	sra;
        char	tmp[BIG_BUFFER_SIZE+1];
 	char	tmpuser[IRCD_BUFFER_SIZE+1];
	char	*s, *bufptr;
	long	bytesread;
	int	old_timeout;
 	size_t	len;
/**************************** PATCHED by Flier ******************************/
        int     iscrypted;
        char    tmpbuf[mybufsize*4];
/****************************************************************************/

 	save_message_from();
 	message_from(Client->user, LOG_DCC);
	if (Client->flags & DCC_WAIT)
	{
		sra = sizeof(struct sockaddr_in);
		Client->write = accept(Client->read, (struct sockaddr *)
			&remaddr, &sra);
#if defined(ESIX)
		mark_socket(Client->write);
#endif
		new_close(Client->read);
		Client->read = Client->write;
		Client->flags &= ~DCC_WAIT;
		Client->flags |= DCC_ACTIVE;
/**************************** PATCHED by Flier ******************************/
		/*say("DCC chat connection to %s[%s,%d] established", Client->user,
			inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port));*/
                PrintEstablish("chat",Client,remaddr,0L);
                snprintf(tmpbuf,sizeof(tmpbuf),"=%s",Client->user);
                AddNick2List(tmpbuf,Client->server);
/****************************************************************************/
		Client->starttime = time(NULL);
		goto out;
	}
	s = Client->buffer;
	bufptr = tmp;
	if (s && *s)
	{
		len = strlen(s);
		strncpy(tmp, s, len);
		bufptr += len;
	}
	else
		len = 0;
	old_timeout = dgets_timeout(1);
 	bytesread = dgets(bufptr, (int)((BIG_BUFFER_SIZE/2) - len), Client->read, (char *)0);
	(void) dgets_timeout(old_timeout);
	switch (bytesread)
	{
	case -1:
		add_to_dcc_buffer(Client, bufptr);
		if (Client->buffer && strlen(Client->buffer) > BIG_BUFFER_SIZE/2)
		{
			new_free(&Client->buffer);
			say("*** dropped long DCC CHAT message from %s", Client->user);
		}
		break;
	case 0:
/**************************** PATCHED by Flier ******************************/
		/*say("DCC CHAT connection to %s lost: %s", Client->user, dgets_errno == -1 ? "Remote end closed connection" : strerror(dgets_errno));*/
                PrintError("CHAT",Client->user,NULL,
                           dgets_errno==-1?"Remote end closed connection":strerror(dgets_errno));
/****************************************************************************/
		new_close(Client->read);
		Client->read = Client->write = -1;
		Client->flags |= DCC_DELETE;
		break;
	default:
		new_free(&Client->buffer);
		len = strlen(tmp);
		if (len > BIG_BUFFER_SIZE/2)
			len = BIG_BUFFER_SIZE/2;
		Client->bytes_read += len;
		*tmpuser = '=';
 		strmcpy(tmpuser+1, Client->user, IRCD_BUFFER_SIZE-2);
		s = do_ctcp(tmpuser, nickname, tmp);
 		s[strlen(s) - 1] = '\0';	/* remove newline */
		if (s && *s)
		{
 			s[BIG_BUFFER_SIZE/2-1] = '\0';	/* XXX XXX: stop dcc long messages, stupid but "safe"? */
/**************************** PATCHED by Flier ******************************/
                        snprintf(tmpbuf,sizeof(tmpbuf),"=%s",Client->user);
                        iscrypted=DecryptMessage(s,tmpbuf);
/****************************************************************************/
#ifndef LITE
			if (do_hook(DCC_CHAT_LIST, "%s %s", Client->user, s))
#endif
                        {
/**************************** PATCHED by Flier ******************************/
	                        /*if (away_set)
				{
					time_t	t;

					t = time(0);
					snprintf(tmp, sizeof tmp, "%s <%.16s>", s, ctime(&t));
					s = tmp;
				}
        	                put_it("=%s= %s", Client->user, s);*/
/****************************************************************************/
				if (beep_on_level & LOG_CTCP)
					beep_em(1);
/**************************** PATCHED by Flier ******************************/
                	        PrintChatMsg(Client,s,bytesread,iscrypted);
/****************************************************************************/
                        }
		}
/**************************** PATCHED by Flier ******************************/
                if (away_set) {
                    snprintf(tmpbuf,sizeof(tmpbuf),"=%s= %s",Client->user,s);
                    AwaySave(tmpbuf,SAVECHAT);
                }
                snprintf(tmpbuf,sizeof(tmpbuf),"=%s",Client->user);
                AddNick2List(tmpbuf,Client->server);
/****************************************************************************/
	}
out:
	restore_message_from();
}

#ifndef LITE
static	void
process_incoming_listen(Client)
	DCC_list	*Client;
{
	struct	sockaddr_in	remaddr;
	int	sra;
	char	FdName[10];
	DCC_list	*NewClient;
	int	new_socket;
	struct	hostent	*hp;
	char	*Name;

	sra = sizeof(struct sockaddr_in);
	new_socket = accept(Client->read, (struct sockaddr *) &remaddr,
			     &sra);
	if (0 != (hp = gethostbyaddr((char *)&remaddr.sin_addr,
	    sizeof(remaddr.sin_addr), remaddr.sin_family)))
		Name = hp->h_name;
	else
		Name = inet_ntoa(remaddr.sin_addr);
#if defined(ESIX)
	mark_socket(new_socket);
#endif
	snprintf(FdName, sizeof FdName, "%d", new_socket);
        NewClient = dcc_searchlist(Name, FdName, DCC_RAW, 1, (char *) 0);
	NewClient->starttime = time((time_t *) 0);
	NewClient->read = NewClient->write = new_socket;
	NewClient->remote = remaddr.sin_addr;
	NewClient->remport = remaddr.sin_port;
	NewClient->flags |= DCC_ACTIVE;
	NewClient->bytes_read = NewClient->bytes_sent = 0L;
 	save_message_from();
 	message_from(NewClient->user, LOG_DCC);
	if (do_hook(DCC_RAW_LIST, "%s %s N %d", NewClient->user,
						NewClient->description,
						Client->write))
		say("DCC RAW connection to %s on %s via %d established",
					NewClient->description,
					NewClient->user,
					Client->write);
 	restore_message_from();
}

static	void
process_incoming_raw(Client)
	DCC_list	*Client;
{
	char	tmp[BIG_BUFFER_SIZE+1];
	char	*s, *bufptr;
	long	bytesread;
	int     old_timeout;
 	size_t	len;

 	save_message_from();
 	message_from(Client->user, LOG_DCC);

        s = Client->buffer;
	bufptr = tmp;
	if (s && *s)
	{
		len = strlen(s);
		strncpy(tmp, s, len);
		bufptr += len;
	}
	else
		len = 0;
	old_timeout = dgets_timeout(1);
 	switch(bytesread = dgets(bufptr, (int)((BIG_BUFFER_SIZE/2) - len), Client->read, (char *)0))
	{
	case -1:
		add_to_dcc_buffer(Client, bufptr);
		if (Client->buffer && strlen(Client->buffer) > BIG_BUFFER_SIZE/2)
		{
			new_free(&Client->buffer);
			say("*** dropping long DCC raw message from %s", Client->user);
		}
		break;
	case 0:
		if (do_hook(DCC_RAW_LIST, "%s %s C",
				Client->user, Client->description))
			say("DCC RAW connection to %s on %s lost",
				Client->user, Client->description);
		new_close(Client->read);
		Client->read = Client->write = -1;
		Client->flags |= DCC_DELETE;
		(void) dgets_timeout(old_timeout);
		break;
	default:
		new_free(&Client->buffer);
		len = strlen(tmp);
		if (len > BIG_BUFFER_SIZE / 2)
			len = BIG_BUFFER_SIZE / 2;
		tmp[len - 1] = '\0';
		Client->bytes_read += len;
		if (do_hook(DCC_RAW_LIST, "%s %s D %s",
				Client->user, Client->description, tmp))
			say("Raw data on %s from %s: %s",
				Client->user, Client->description, tmp);
		(void) dgets_timeout(old_timeout);
	}
 	restore_message_from();
}
#endif /* LITE */

/**************************** PATCHED by Flier ******************************/
/*static	void
process_incoming_talk(Client)
	DCC_list	*Client;
{
	struct	sockaddr_in	remaddr;
	int	sra;
	char	tmp[BIG_BUFFER_SIZE+1];
	char	*s, *bufptr;
	long	bytesread;
	int     old_timeout;

	if (Client->flags & DCC_WAIT)
	{
		sra = sizeof(struct sockaddr_in);
		Client->write = accept(Client->read, (struct sockaddr *)
			&remaddr, &sra);
#ifdef ESIX
		mark_socket(Client->write);
#endif
		new_close(Client->read);
		Client->read = Client->write;
		Client->flags &= ~DCC_WAIT;
		Client->flags |= DCC_ACTIVE;
		send_talk_control(Client, DCC_TALK_DELETE_LOCAL);
		new_close(Client->file);
		send(Client->write, "\010\025\027", 3, 0);
		recv(Client->read, Client->talkchars, 3, 0);
		Client->bytes_read = Client->bytes_sent = 3;
		say("TALK connection to %s[%s,%d] established", Client->user,
			inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port));
		return;
	}
	s = Client->buffer;
	bufptr = tmp;
	if (s && *s)
	{
		int	len = strlen(s);

		strncpy(tmp, s, len);
		bufptr += len;
	}
	old_timeout = dgets_timeout(1);
	switch(bytesread = dgets(bufptr, -BIG_BUFFER_SIZE, Client->read,
			Client->talkchars))
	{
	case -2: return;
	case -1:
		add_to_dcc_buffer(Client, tmp);
		return;
	case 0:
		say("TALK connection to %s lost", Client->user);
		new_close(Client->read);
		Client->read=Client->write = -1;
		Client->flags |= DCC_DELETE;
		(void) dgets_timeout(old_timeout);
		return;
	default:
		new_free(&Client->buffer);
		Client->bytes_read += bytesread;
		message_from(Client->user, LOG_DCC);
		if (do_hook(TALK_LIST, "%s %s", Client->user, tmp))
			put_it("+%s+ %s", Client->user, tmp);
		message_from((char *) 0, LOG_CURRENT);
		(void) dgets_timeout(old_timeout);
		return;
	}
}*/
/****************************************************************************/

static	void
/**************************** PATCHED by Flier ******************************/
/*process_outgoing_file(Client)
	DCC_list	*Client;*/
#if defined(NON_BLOCKING_CONNECTS)
process_outgoing_file(Client,readwaiting)
DCC_list *Client;
int readwaiting;
#else
process_outgoing_file(Client)
DCC_list *Client;
#endif
/****************************************************************************/
{
	struct	sockaddr_in	remaddr;
	int	sra;
	char	tmp[BIG_BUFFER_SIZE+1];
	u_32int	bytesrecvd;
	int	bytesread;
	int	BlockSize;
/**************************** Patched by Flier ******************************/
        time_t timenow=time((time_t *) 0);
        unsigned char packet[50];
        struct transfer_struct *received, orders;
/****************************************************************************/

 	save_message_from();
 	message_from(Client->user, LOG_DCC);
	if (Client->flags & DCC_WAIT)
	{
		sra = sizeof(struct sockaddr_in);
		Client->write = accept(Client->read,
				(struct sockaddr *) &remaddr, &sra);
#if defined(ESIX)
		mark_socket(Client->write);
#endif
/**************************** Patched by Flier ******************************/
                if ((Client->flags&DCC_TYPES)==DCC_RESENDOFFER) {
                    recv(Client->write, packet, sizeof(struct transfer_struct), 0);
                    received=(struct transfer_struct *) packet;
                    if (byteordertest()!=received->byteorder) {
                        /* the packet sender orders bytes differently than us,
                         reverse what they sent to get the right value */
                        orders.packet_id=
                            ((received->packet_id&0x00ff)<<8) |
                            ((received->packet_id&0xff00)>>8);
                        orders.byteoffset=
                            ((received->byteoffset&0xff000000)>>24) |
                            ((received->byteoffset&0x00ff0000)>>8)  |
                            ((received->byteoffset&0x0000ff00)<<8)  |
                            ((received->byteoffset&0x000000ff)<<24);
                    }
                    else memcpy(&orders,packet,sizeof(struct transfer_struct));
                    if (orders.packet_id != DCC_PACKETID)
                        say("DCC REGET packet is invalid!");
                }
/****************************************************************************/
		new_close(Client->read);
		Client->read = Client->write;
#if defined(NON_BLOCKING_CONNECTS)
                if (get_int_var(HYPER_DCC_VAR))
                    set_non_blocking(Client->write);
#endif
		Client->flags &= ~DCC_WAIT;
		Client->flags |= DCC_ACTIVE;
#if defined(NON_BLOCKING_CONNECTS)
                Client->eof = 0;
                Client->hyperdcc = get_int_var(HYPER_DCC_VAR);
#endif
		Client->starttime = time(NULL);
/**************************** Patched by Flier ******************************/
		/*say("DCC SEND connection to %s[%s,%d] established", Client->user,
			inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port));*/
                if ((Client->flags&DCC_TYPES)==DCC_RESENDOFFER)
                    PrintEstablish("RESEND",Client,remaddr,orders.byteoffset);
                else PrintEstablish("SEND",Client,remaddr,Client->bytes_sent);
                if ((Client->flags&DCC_TYPES)==DCC_RESENDOFFER) {
			lseek(Client->file,orders.byteoffset,SEEK_SET);
			Client->resendoffset=orders.byteoffset;
		}
/****** Coded by Zakath ******/
                CdccSendNum++;
                update_all_status();
/*****************************/
/****************************************************************************/
	}
/**************************** PATCHED by Flier ******************************/
        /*else*/
#if defined(NON_BLOCKING_CONNECTS)
        else if ((readwaiting && Client->hyperdcc) || !(Client->hyperdcc))
#else
        else
#endif
/****************************************************************************/
	{
 		if ((bytesread = recv(Client->read, (char *) &bytesrecvd, sizeof(u_32int), 0)) < sizeof(u_32int))
		{
/**************************** Patched by Flier ******************************/
	       		/*say("DCC SEND:%s connection to %s lost: %s", Client->description, Client->user, strerror(errno));*/
                        if ((Client->flags&DCC_TYPES)==DCC_RESENDOFFER)
                            PrintError("RESEND",Client->user,Client->description,
                                       strerror(errno));
                        else  PrintError("SEND",Client->user,Client->description,
                                         strerror(errno));
                        CdccSendNum--;
                        if (CdccSendNum<0) CdccSendNum=0;
                        DCCDone=1;
                        if (ShowDCCStatus) StatusDCC(NULL);
                        RemoveFromQueue(1);
                        update_all_status();
/****************************************************************************/
			new_close(Client->read);
			Client->read = Client->write = (-1);
			Client->flags |= DCC_DELETE;
			new_close(Client->file);
			goto out;
		}
		else
			if (ntohl(bytesrecvd) != Client->bytes_sent)
				goto out;
        }
	BlockSize = get_int_var(DCC_BLOCK_SIZE_VAR);
	if (BlockSize > BIG_BUFFER_SIZE)
		BlockSize = BIG_BUFFER_SIZE;
	else if (BlockSize < 16)
		BlockSize = 16;
/**************************** PATCHED by Flier ******************************/
        /*if ((bytesread = read(Client->file, tmp, BIG_BUFFER_SIZE)) != 0)*/
        if ((bytesread = read(Client->file, tmp, BlockSize)) != 0)
/****************************************************************************/
	{
 		send(Client->write, tmp, (size_t)bytesread, 0);
/**************************** Patched by Flier ******************************/
                if (ShowDCCStatus && StatusDCC(Client)) update_all_status();
/****************************************************************************/
		Client->bytes_sent += bytesread;
	}
/**************************** PATCHED by Flier ******************************/
#if defined(NON_BLOCKING_CONNECTS)
	else if (!readwaiting && Client->hyperdcc) {
            Client->eof=1;
            goto out;
        }
#endif
/****************************************************************************/
	else
	{
		/*
		 * We do this here because lame Ultrix doesn't let us
		 * call put_it() with a float.  Perhaps put_it() should
		 * be fixed properly, and this kludge removed ..
		 * sometime....  -phone jan, 1993.
		 */

/**************************** PATCHED by Flier ******************************/
		/*char	lame_ultrix[10];*/	/* should be plenty */
		/*time_t	xtime = time(NULL) - Client->starttime;
		double	sent = (double)Client->bytes_sent;

		if (sent <= 0)
			sent = 1;
		sent /= (double)1024.0;
		if (xtime <= 0)
			xtime = 1;
		snprintf(lame_ultrix, sizeof lame_ultrix, "%2.4g", (sent / (double)xtime));
		say("DCC SEND:%s to %s completed %s kb/sec",
			Client->description, Client->user, lame_ultrix);*/
                if ((Client->flags&DCC_TYPES)==DCC_RESENDOFFER)
                    PrintComplete("RESEND",Client);
                else PrintComplete("SEND",Client);
                BytesSent+=(double) Client->bytes_sent;
                CdccSendNum--;
                if (CdccSendNum<0) CdccSendNum=0;
                DCCDone=1;
                if (ShowDCCStatus) StatusDCC(NULL);
                RemoveFromQueue(1);
                update_all_status();
/****************************************************************************/
		new_close(Client->read);
		Client->read = Client->write = -1;
		Client->flags |= DCC_DELETE;
		new_close(Client->file);
	}
/**************************** Patched by Flier ******************************/
        if (Client->minspeed>=0.001 && timenow>=Client->CdccTime)
            CheckDCCSpeed(Client,timenow);
/****************************************************************************/
out:
 	restore_message_from();
}

static	void
process_incoming_file(Client)
	DCC_list	*Client;
{
	char	tmp[BIG_BUFFER_SIZE+1];
	u_32int	bytestemp;
	int	bytesread;

        if ((bytesread = recv(Client->read, tmp, BIG_BUFFER_SIZE, 0)) <= 0)
	{
		/*
		 * We do this here because lame Ultrix doesn't let us
		 * call put_it() with a float.  Perhaps put_it() should
		 * be fixed properly, and this kludge removed ..
		 * sometime....  -phone jan, 1993.
		 */

/**************************** PATCHED by Flier ******************************/
		/*char    lame_ultrix[10];*/        /* should be plenty */
		/*time_t	xtime = time(NULL) - Client->starttime;
		double	sent = (double)Client->bytes_read;

		if (sent <= 0)
			sent = 1;
		sent /= (double)1024.0;
		if (xtime <= 0)
			xtime = 1;
		snprintf(lame_ultrix, sizeof lame_ultrix, "%2.4g", (sent / (double)xtime));
		say("DCC GET:%s from %s completed %s kb/sec",
			Client->description, Client->user, lame_ultrix);*/
 		save_message_from();
 		message_from(Client->user, LOG_DCC);
                if ((Client->flags&DCC_TYPES)==DCC_FILEREGET)
                    PrintComplete("REGET",Client);
                else PrintComplete("GET",Client);
 		restore_message_from();
                BytesReceived+=(double) Client->bytes_read;
                CdccRecvNum--;
                if (CdccRecvNum<0) CdccRecvNum=0;
                DCCDone=1;
                if (ShowDCCStatus) StatusDCC(NULL);
                update_all_status();
/****************************************************************************/
		new_close(Client->read);
		new_close(Client->file);
		Client->read = Client->write = (-1);
		Client->flags |= DCC_DELETE;
		return;
	}
/**************************** PATCHED by Flier *****************************/
#ifdef EXTRA_STUFF
        CodeIt(Client->file,tmp,bytesread);
#endif
/***************************************************************************/
 	write(Client->file, tmp, (size_t)bytesread);
	Client->bytes_read += bytesread;
	bytestemp = htonl(Client->bytes_read);
	send(Client->write, (char *) &bytestemp, sizeof(u_32int), 0);
/**************************** PATCHED by Flier ******************************/
        if (ShowDCCStatus && StatusDCC(Client)) update_all_status();
/****************************************************************************/
}

/* flag == 1 means show it.  flag == 0 used by redirect */

void
dcc_message_transmit(user, text, type, flag)
	char	*user;
	char	*text;
	int	type,
		flag;
{
	DCC_list	*Client;
	char	tmp[BIG_BUFFER_SIZE+1];
#ifndef LITE
 	char	nickbuf[128];
#endif
	char	thing = '\0';
	char	*host = (char *) 0;
#ifndef LITE
 	crypt_key	*key;
#endif
 	char	*line;
	int	lastlog_level;
	int	list = 0;
 	size_t	len;
/**************************** PATCHED by Flier ******************************/
        int     iscrypted=0;
        char    tmpbuf[mybufsize/8];
/****************************************************************************/

 	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	switch(type)
	{
/**************************** PATCHED by Flier ******************************/
	/*case DCC_TALK:
		if ((host = index(user, '@')) != NULL)
			*(host++) = '\0';
		thing = '+';
		list = SEND_TALK_LIST;
		break;*/
/****************************************************************************/
	case DCC_CHAT:
		host = "chat";
		thing = '=';
		list = SEND_DCC_CHAT_LIST;
		break;
#ifndef LITE
	case DCC_RAW:
		host = next_arg(text, &text);
		if (!host)
		{
			say("No host specified for DCC RAW");
			goto out1;
		}
		break;
#endif
	}
 	save_message_from();
 	message_from(user, LOG_DCC);
	if (!(Client = dcc_searchlist(host, user, type, 0, (char *) 0)) || !(Client->flags&DCC_ACTIVE))
	{
		say("No active DCC %s:%s connection for %s", dcc_types[type], host ? host : "<any>", user);
		goto out;
	}
#ifdef DCC_CNCT_PEND
	/*
	 * XXX - should make this buffer
	 * XXX - just for dcc chat ?  maybe raw dcc too.  hmm.
	 */
	if (Client->flags & DCC_CNCT_PEND)
	{
		say("DCC %s:%s connection to %s is still connecting...", dcc_types[type], host ? host : "<any>", user);
		goto out;
	}
#endif /* DCC_DCNT_PEND */
	strmcpy(tmp, text, BIG_BUFFER_SIZE);
/**************************** PATCHED by Flier ******************************/
	if (type==DCC_CHAT) {
            snprintf(tmpbuf,sizeof(tmpbuf),"=%s",Client->user);
            iscrypted=EncryptMessage(tmp,tmpbuf);
        }
/****************************************************************************/
	if (type == DCC_CHAT) {
#ifndef LITE
		nickbuf[0] = '=';
		strmcpy(nickbuf+1, user, sizeof(nickbuf) - 2);

		if ((key = is_crypted(nickbuf)) == 0 || (line = crypt_msg(tmp, key, 1)) == 0)
			line = tmp;
#else
                line=tmp;
#endif
	}
	else
		line = tmp;
#ifdef HAVE_WRITEV
	{
		struct iovec iov[2];
/**************************** PATCHED by Flier *****************************/
                int cnt = 2;

                if (type == DCC_RAW) cnt = 1;
/***************************************************************************/

		iov[0].iov_base = line;
 		iov[0].iov_len = len = strlen(line);
 		iov[1].iov_base = "\n";
		iov[1].iov_len = 1;
		len++;
/**************************** PATCHED by Flier *****************************/
 		/*(void)writev(Client->write, iov, 2);*/
 		(void)writev(Client->write, iov, cnt);
/***************************************************************************/
	}
#else
 	/* XXX XXX XXX THIS IS TERRIBLE! XXX XXX XXX */
#define CRYPT_BUFFER_SIZE (IRCD_BUFFER_SIZE - 50)    /* XXX XXX FROM: crypt.c XXX XXX */
/**************************** PATCHED by Flier *****************************/
	/*strmcat(line, "\n", (size_t)((line == tmp) ? BIG_BUFFER_SIZE : CRYPT_BUFFER_SIZE));*/
	if (type != DCC_RAW) strmcat(line, "\n", (size_t)((line == tmp) ? BIG_BUFFER_SIZE : CRYPT_BUFFER_SIZE));
/***************************************************************************/
	len = strlen(line);
 	(void)send(Client->write, line, len, 0);
#endif
	Client->bytes_sent += len;
	if (flag && type != DCC_RAW) {
#ifndef LITE
                if (do_hook(list, "%s %s", Client->user, text))
#endif
/**************************** PATCHED by Flier *****************************/
                        /*put_it("=> %c%s%c %s", thing, Client->user, thing, text);*/
                    PrintMyChatMsg(Client->user,text,iscrypted);
/***************************************************************************/
	}
out:
 	restore_message_from();
#ifndef LITE
out1:
#endif
	set_lastlog_msg_level(lastlog_level);
	return;
}

void
dcc_chat_transmit(user,	text)
	char	*user;
	char	*text;
{
	dcc_message_transmit(user, text, DCC_CHAT, 1);
}

/**************************** PATCHED by Flier ******************************/
/*static	void
dcc_tmsg(args)
	char	*args;
{
	char	*user;

	if (!(user = next_arg(args, &args)))
	{
		say("You must specify a connection for a DCC TMSG");
		return;
	}
	dcc_message_transmit(user, args, DCC_TALK, 1);
}*/
/****************************************************************************/

#ifndef LITE
static	void
dcc_send_raw(args)
	char	*args;
{
	char	*name;

	if (!(name = next_arg(args, &args)))
	{
 		int	lastlog_level;

 		lastlog_level = set_lastlog_msg_level(LOG_DCC);
		say("No name specified for DCC RAW");
 		(void) set_lastlog_msg_level(lastlog_level);
		return;
	}
	dcc_message_transmit(name, args, DCC_RAW, 1);
}
#endif

/*
 * dcc_time: Given a time value, it returns a string that is in the
 * format of "hours:minutes:seconds month day year" .  Used by 
 * dcc_list() to show the start time.
 */
static	char	*
dcc_time(the_time)
	time_t	the_time;
{
	struct	tm	*btime;
	char	*buf;
	static	char	*months[] = 
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	btime = localtime(&the_time);
	buf = (char *) malloc(22);
	if (snprintf(buf, sizeof buf, "%-2.2d:%-2.2d:%-2.2d %s %-2.2d %d",
			btime->tm_hour, btime->tm_min, btime->tm_sec,
			months[btime->tm_mon], btime->tm_mday,
			btime->tm_year + 1900))
		return buf;
	else
		return empty_string;
}

void
dcc_list(args)
	char	*args;
{
	DCC_list	*Client;
	static	char	*format =
			"%-7.7s %-9.9s %-10.10s %-20.20s %-8.8s %-8.8s %s";
	unsigned	flags;
 	int	lastlog_level;

 	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	put_it(format, "Type", "Nick", "Status", "Start time", "Sent", "Read",
			"Arguments");
	for (Client = ClientList ; Client != NULL ; Client = Client->next)
	{
		char	sent[9],
			rd[9];
		char	*timestr;

		snprintf(sent, sizeof sent, "%ld", (long)Client->bytes_sent);
		snprintf(rd, sizeof rd, "%ld", (long)Client->bytes_read);
 		timestr = (Client->starttime) ? dcc_time(Client->starttime) : "";
		flags = Client->flags;
		put_it(format, dcc_types[flags & DCC_TYPES],
				Client->user,
				flags & DCC_OFFER ? "Offered" :
				flags & DCC_DELETE ? "Closed" :
				flags & DCC_ACTIVE ? "Active" :
				flags & DCC_WAIT ? "Waiting" :
/**************************** PATCHED by Flier ******************************/
/*#ifdef DCC_DCNT_PEND*/
#ifdef DCC_CNCT_PEND
/****************************************************************************/
				flags & DCC_CNCT_PEND ?	"Connecting" :
#endif
				"Unknown",
				timestr, sent, rd, Client->description);
		if (*timestr)
			new_free(&timestr);
	}
 	(void) set_lastlog_msg_level(lastlog_level);
}

/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
dcc_close(args)
	char	*args;
{
	DCC_list	*Client;
	unsigned	flags;
	char	*Type;
	char	*user;
	char	*description;
	int	CType;
	char	*cmd = NULL;
 	int	lastlog_level;

 	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if (!(Type = next_arg(args, &args)) || !(user=next_arg(args, &args)))
	{
		say("You must specify a type and nick for DCC CLOSE");
		goto out;
	}
/**************************** PATCHED by Flier ******************************/
	/*description = next_arg(args, &args);*/
	description=new_next_arg(args,&args);
/****************************************************************************/
	malloc_strcpy(&cmd, Type);
	upper(cmd);
	for (CType = 0; dcc_types[CType] != NULL; CType++)
		if (!strcmp(cmd, dcc_types[CType]))
			break;
  	if (!dcc_types[CType])
  		say("Unknown DCC type: %s", Type);
 	else if ((Client = dcc_searchlist(description, user, CType, 0, description)))
	{
		flags = Client->flags;
		if (flags & DCC_DELETE)
			goto out;
		if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE))
		{
			new_close(Client->read);
			if (Client->file)
				new_close(Client->file);
		}
		say("DCC %s:%s to %s closed", Type,
			description ? description : "<any>", user);
/**************************** PATCHED by Flier ******************************/
/****** Coded by Zakath ******/
                if (flags&DCC_ACTIVE) {
                    if ((flags&DCC_TYPES)==DCC_FILEOFFER ||
                        (flags&DCC_TYPES)==DCC_RESENDOFFER) CdccSendNum--;
                    if (CdccSendNum<0) CdccSendNum=0;
                    if ((flags&DCC_TYPES)==DCC_FILEREAD ||
                        (flags&DCC_TYPES)==DCC_FILEREGET) CdccRecvNum--;
                    if (CdccRecvNum<0) CdccRecvNum=0;
                    DCCDone=1;
                    if (ShowDCCStatus) StatusDCC(NULL);
                    update_all_status();
                }
/*****************************/
/****************************************************************************/
		dcc_erase(Client);
	}
	else
		say("No DCC %s:%s to %s found", Type,
			description ? description : "<any>", user);
 	new_free(&cmd);
out:
 	(void) set_lastlog_msg_level(lastlog_level);
}

/* this depends on dcc_rename() setting loglevel */
#ifndef LITE
static void
dcc_chat_rename(args)
	char	*args;
{
	DCC_list	*Client;
	char	*user;
	char	*temp;
/**************************** PATCHED by Flier ******************************/
        char    oldnick[mybufsize/16+3];
        char    newnick[mybufsize/16+3];
        struct  nicks *nickstr;
/****************************************************************************/

	if (!(user = next_arg(args, &args)) || !(temp = next_arg(args, &args)))
	{
		say("You must specify a current DCC CHAT connection, and a new name for it");
		return;
	}
	if (dcc_searchlist("chat", temp, DCC_CHAT, 0, (char *) 0))
	{
		say("You already have a DCC CHAT connection with %s, unable to rename.", temp);
		return;
	}
	if ((Client = dcc_searchlist("chat", user, DCC_CHAT, 0, (char *) 0)))
	{
		new_free(&(Client->user));
		malloc_strcpy(&(Client->user), temp);
		say("DCC CHAT connection with %s renamed to %s", user, temp);
/**************************** PATCHED by Flier ******************************/
                if (strlen(user)>mybufsize/16) user[mybufsize/16]='\0';
                if (strlen(temp)>mybufsize/16) temp[mybufsize/16]='\0';
                snprintf(oldnick,sizeof(oldnick),"=%s",user);
                snprintf(newnick,sizeof(newnick),"=%s",temp);
                if (CheckServer(Client->server)) {
                    for (nickstr=server_list[Client->server].nicklist;nickstr;
                         nickstr=nickstr->next)
                        if (!my_stricmp(nickstr->nick,oldnick))
                            malloc_strcpy(&(nickstr->nick),newnick);
                }
/****************************************************************************/
	}
	else
		say("No DCC CHAT connection with %s", user);
}


static	void
dcc_rename(args)
	char	*args;
{
	DCC_list	*Client;
	char	*user;
	char	*description;
	char	*newdesc;
	char	*temp;
 	int	lastlog_level;

 	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if ((user = next_arg(args, &args)) && my_strnicmp(user, "-chat", strlen(user)) == 0)
	{
		dcc_chat_rename(args);
		return;
	}
	if (!user || !(temp = next_arg(args, &args)))
	{
		say("You must specify a nick and new filename for DCC RENAME");
		goto out;
	}
	if ((newdesc = next_arg(args, &args)) != NULL)
		description = temp;
	else
	{
		newdesc = temp;
		description = NULL;
	}
	if ((Client = dcc_searchlist(description, user, DCC_FILEREAD, 0, (char *) 0)))
	{
		if (!(Client->flags & DCC_OFFER))
		{
			say("Too late to rename that file");
			goto out;
		}
		new_free(&(Client->description));
		malloc_strcpy(&(Client->description), newdesc);
		say("File %s from %s renamed to %s",
			 description ? description : "<any>", user, newdesc);
	}
	else
		say("No file %s from %s found",
			description ? description : "<any>", user);
out:
 	(void) set_lastlog_msg_level(lastlog_level);
}
#endif /* LITE */

/*
 * close_all_dcc:  We call this when we create a new process so that
 * we don't leave any fd's lying around, that won't close when we
 * want them to..
 */

void
close_all_dcc()
{
	DCC_list *Client;

	while ((Client = ClientList))
		dcc_erase(Client);
}

/**************************** Patched by Flier ******************************/
/* 
   returns 1 if system stores numbers in little indian/big indian order,
   returns 0 if system stores numbers in big indian/little indian order.
 */
unsigned char byteordertest()
{
    unsigned short test=DCC_PACKETID;

    if (*((unsigned char *)&test)==((DCC_PACKETID&0xff00)>>8)) return(0);
    if (*((unsigned char *)&test)==(DCC_PACKETID&0x00ff)) return(1);
    return(0);
}
/****************************************************************************/

static	void
add_to_dcc_buffer(Client, buf)
	DCC_list	*Client;
	char	*buf;
{
	if (buf && *buf)
	{
		if (Client->buffer)
			malloc_strcat(&Client->buffer, buf);
		else
			malloc_strcpy(&Client->buffer, buf);
	}
}

void dcc_reject(user, type, filename)
char *user;
char *type;
char *filename;
{
    int i;
    DCC_list *Client;
#ifdef WANTANSI
    char tmpbuf[mybufsize / 2];
    char tmpbuf2[mybufsize / 4 + 1];
#endif
    char tmpbuf3[mybufsize];

    if (!user || !type || !filename) return;
    for (i = 0; dcc_types[i] != NULL; i++) {
        if (!strcmp(dcc_types[i], type))
            break;
    }
    if (i == 0) return;
    Client = dcc_searchlist(filename, user, i, 0, (char *) 0);
    if (!Client) return;
#ifdef WANTANSI
    snprintf(tmpbuf, sizeof(tmpbuf), "%sDCC%s %sREJECT%s (%s%s%s) request ",
             CmdsColors[COLDCC].color5, Colors[COLOFF],
             CmdsColors[COLDCC].color3, Colors[COLOFF],
             CmdsColors[COLDCC].color4, filename, Colors[COLOFF]);
    ColorUserHost(FromUserHost, CmdsColors[COLDCC].color2, tmpbuf2, 1);
    snprintf(tmpbuf3, sizeof(tmpbuf3), "%s%sreceived%s from %s%s%s %s", tmpbuf,
             CmdsColors[COLDCC].color4, Colors[COLOFF],
             CmdsColors[COLDCC].color1, user, Colors[COLOFF], tmpbuf2);
#else
    snprintf(tmpbuf3, sizeof(tmpbuf3), "DCC REJECT (%s) request received from %s (%s)",
             type, description, (long) Client->filesize, user, FromUserHost);
#endif
    say("%s", tmpbuf3);
    dcc_erase(Client);
}
/****************************************************************************/
