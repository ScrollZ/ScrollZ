/*
 * edit.c: This is really a mishmash of function and such that deal with IRCII
 * commands (both normal and keybinding commands) 
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
 * $Id: edit.c,v 1.72 2001-12-12 21:09:20 f Exp $
 */

#include "irc.h"

#include <sys/stat.h>

#ifdef ESIX
# include <lan/net_types.h>
#endif /* ESIX */

#include "parse.h"
#include "ircterm.h"
#include "server.h"
#include "edit.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "translat.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "if.h"
#include "help.h"
#include "stack.h"
#include "queue.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"
/****************************************************************************/

/*
 * current_exec_timer - used to make sure we don't remove a timer
 * from within itself.
 */
static	int	current_exec_timer = -1;

static	int	save_which;
static	int	save_do_all;

static	void	show_timer _((char *));
static	int	create_timer_ref _((int));
static	void	get_history _((int));
static	void	oper_password_received _((char *, char *));
static	char	*do_channel _((char *, int));
static	void	send_action _((char *, char *));

TimerList *PendingTimers = (TimerList *) 0;

/* used with input_move_cursor */
#define RIGHT 1
#define LEFT 0

/* used with /save */
#define	SFLAG_ALIAS	0x0001
#define	SFLAG_BIND	0x0002
#define	SFLAG_ON	0x0004
#define	SFLAG_SET	0x0008
#define	SFLAG_NOTIFY	0x0010
#define	SFLAG_DIGRAPH	0x0020

/* The maximum number of recursive LOAD levels allowed */
#define MAX_LOAD_DEPTH 10

/* recv_nick: the nickname of the last person to send you a privmsg */
	u_char	*recv_nick = NULL;

/* sent_nick: the nickname of the last person to whom you sent a privmsg */
	u_char	*sent_nick = NULL;
	u_char	*sent_body = NULL;

/* Used to keep down the nesting of /LOADs and to determine if we
 * should activate the warning for /ONs if the NOVICE variable is set.
 */
	int	load_depth = 0;

/* Used to prevent global messaging */
extern	int	in_on_who;

typedef	struct	WaitCmdstru
{
	char	*stuff;
	struct	WaitCmdstru	*next;
}	WaitCmd;

static	WaitCmd	*start_wait_list = NULL,
		*end_wait_list = NULL;

	char	lame_wait_nick[] = "1#LAME";

/* a few advance declarations */
#ifndef LITE
static	void	sendlinecmd _((char *, char *, char *));
#endif
static	void	do_send_text _((char *, char *, char *));
static	void	funny_stuff _((char *, char *, char *));
#ifndef LITE
static	void	catter _((char *, char *, char *));
#endif
static	void	cd _((char *, char *, char *));
#ifndef LITE
static	void	e_wall _((char *, char *, char *));
static	void	send_squery _((char *, char *, char *));
#endif
static	void	send_2comm _((char *, char *, char *));
static	void	send_comm _((char *, char *, char *));
static	void	send_topic _((char *, char *, char *));
static	void	send_channel_nargs _((char *, char *, char *));
static	void	send_channel_2args _((char *, char *, char *));
static	void	send_channel_1arg _((char *, char *, char *));
static	void	my_clear _((char *, char *, char *));
static	void	quote _((char *, char *, char *));
static	void	e_privmsg _((char *, char *, char *));
static	void	flush _((char *, char *, char *));
/**************************** PATCHED by Flier ******************************/
/*static	void	away _((char *, char *, char *));*/
void	away _((char *, char *, char *));
/****************************************************************************/
static	void	oper _((char *, char *, char *));
/**************************** PATCHED by Flier ******************************/
/*static	void	e_channel _((char *, char *, char *));*/
void	e_channel _((char *, char *, char *));
/****************************************************************************/
static	void	who _((char *, char *, char *));
static	void	whois _((char *, char *, char *));
static	void	ison _((char *, char *, char *));
/**************************** PATCHED by Flier ******************************/
/*static	void	userhost _((char *, char *, char *));*/
void	userhost _((char *, char *, char *));
/****************************************************************************/
#ifndef LITE
static	void	info _((char *, char *, char *));
#endif
/**************************** PATCHED by Flier ******************************/
/*static	void	e_nick _((char *, char *, char *));*/
void	e_nick _((char *, char *, char *));
/****************************************************************************/
static	void	commentcmd _((char *, char *, char *));
static	void	sleepcmd _((char *, char *, char *));
static	void	version _((char *, char *, char *));
static	void	ctcp _((char *, char *, char *));
static	void	dcc _((char *, char *, char *));
static	void	deop _((char *, char *, char *));
static	void	my_echo _((char *, char *, char *));
static	void	save_settings _((char *, char *, char *));
static	void	redirect _((char *, char *, char *));
/**************************** PATCHED by Flier ******************************/
/*static	void	waitcmd _((char *, char *, char *));*/
void	waitcmd _((char *, char *, char *));
/*static	void	describe _((char *, char *, char *));*/
void	describe _((char *, char *, char *));
/*static	void	me _((char *, char *, char *));*/
void	me _((char *, char *, char *));
/****************************************************************************/
#ifndef LITE
static	void	mload _((char *, char *, char *));
static	void	mlist _((char *, char *, char *));
#endif
static	void	evalcmd _((char *, char *, char *));
static	void	hook _((char *, char *, char *));
/**************************** PATCHED by Flier ******************************/
/*static	void	timercmd _((char *, char *, char *));*/
void	timercmd _((char *, char *, char *));
/****************************************************************************/
#ifndef LITE
static	void	inputcmd _((char *, char *, char *));
#endif
static	void	pingcmd _((char *, char *, char *));
#ifndef LITE
static	void	xtypecmd _((char *, char *, char *));
static	void	beepcmd _((char *, char *, char *));
static	void	abortcmd _((char *, char *, char *));
#endif
static	void	really_save _((char *, char *));

/**************************** PATCHED by Flier ******************************/
/******************* SHEIK ADDED FUNCTIONS *****************************/
extern  void  LameKick _((char *, char *, char *));
extern  void  ChanWallOp _((char *, char *, char *));
extern  void  NewUser _((char *, char *, char *));
extern  void  ReconnectServer _((char *, char *, char *));
extern  void  MegaDeop _((char *, char *, char *));
/***********************************************************************/
extern  void  MegaDehalfop _((char *, char *, char *));
extern  char  *TimeStamp _((int));

#ifdef OPER
extern  int   StatskNumber;
extern  int   StatsiNumber;
extern  int   StatscNumber;
extern  int   StatslNumber;
#endif
extern  char  *chars;
extern  NickList *tabnickcompl;
/* Patched by Zakath */
extern  char  VersionInfo[];
extern  char  *ScrollZver1;
#ifdef CELECOSM
extern  struct friends *whoisfriend;
#endif
/* End patch */

extern  void  AutoNickComplete _((char *, char *, ChannelList *));
extern  void  NoWindowChannel _((void));
extern  int   CheckServer _((int));
extern  char  *CheckJoinKey _((char *));
extern  int   EncryptMessage _((char *, char *));

extern  void  ListFriends _((char *, char *, char *));
extern  void  ListAutoBanKicks _((char *, char *, char *));
extern  void  Kick _((char *, char *, char *));
extern  void  BanKick _((char *, char *, char *));
extern  void  Op _((char *, char *, char *));
extern  void  Ban _((char *, char *, char *));
extern  void  BanType _((char *, char *, char *));
extern  void  Invite _((char *, char *, char *));
extern  void  Leave _((char *, char *, char *));
extern  void  ModeClear _((char *, char *, char *));
extern  void  ChannelScan _((char *, char *, char *));
extern  void  FServer _((char *, char *, char *));
extern  void  Topic _((char *, char *, char *));
extern  void  Cycle _((char *, char *, char *));
extern  void  ScrollZSave _((char *, char *, char *));
extern  void  DirLM _((char *, char *, char *));
extern  void  AddFriend _((char *, char *, char *));
extern  void  AddAutoBanKick _((char *, char *, char *));
extern  void  ShowBans _((char *, char *, char *));
extern  void  RemoveFriend _((char *, char *, char *));
extern  void  RemoveAutoBanKick _((char *, char *, char *));
extern  void  Version _((char *, char *, char *));
extern  void  Unban _((char *, char *, char *));
extern  void  CdBan _((char *, char *, char *));
extern  void  NoIgnore _((char *, char *, char *));
extern  void  Ignore _((char *, char *, char *));
extern  void  SetAway _((char *, char *, char *));
extern  void  SetBack _((char *, char *, char *));
extern  void  AddWord _((char *, char *, char *));
extern  void  RemoveWord _((char *, char *, char *));
extern  void  ListWords _((char *, char *, char *));
extern  void  WhoLeft _((char *, char *, char *));
extern  void  FilterKick _((char *, char *, char *));
extern  void  ClearKey _((char *, char *, char *));
extern  void  NHProtToggle _((char *, char *, char *));
extern  void  ChanStat _((char *, char *, char *));
extern  void  Cdcc _((char *, char *, char *));
extern  void  Ls _((char *, char *, char *));
extern  void  SZHelp _((char *, char *, char *));
extern  void  Chat _((char *, char *, char *));
extern  void  NoChat _((char *, char *, char *));
extern  void  Finger _((char *, char *, char *));
extern  void  UserMode _((char *, char *, char *));
extern  void  AutoJoinOnInvToggle _((char *, char *, char *));
extern  void  Settings _((char *, char *, char *));
extern  void  FloodProtToggle _((char *, char *, char *));
extern  void  Net _((char *, char *, char *));
extern  void  Reset _((char *, char *, char *));
extern  void  CTCPCloakingToggle _((char *, char *, char *));
extern  void  MassHalfop _((char *, char *, char *));
extern  void  MassOp _((char *, char *, char *));
extern  void  TBan _((char *, char *, char *));
#ifdef SCKICKS
extern  void  ScatterKick _((char *, char *, char *));
extern  void  RandomScatterKick _((char *, char *, char *));
#endif
extern  void  NickStat _((char *, char *, char *));
extern  void  AddNotify _((char *, char *, char *));
extern  void  RemoveNotify _((char *, char *, char *));
extern  void  ListNotify _((char *, char *, char *));
extern  void  MyQuit _((char *));
extern  void  AddNick2List _((char *, int));
extern  void  HandleTabNext _((void));
extern  void  AddServer _((char *, char *, char *));
extern  void  RemoveServer _((char *, char *, char *));
extern  void  ListServers _((char *, char *, char *));
extern  void  ClearTab _((char *, char *, char *));
extern  void  ShowKill _((char *, char *, char *));
#ifdef ACID
extern  void  TagNick _((char *, char *, char *));
extern  void  WhereIs _((char *, char *, char *));
extern  void  WhereList _((char *, char *, char *));
#endif
extern  void  UnFlash _((char *, char *, char *));
extern  void  Password _((char *, char *, char *));
extern  void  PrintPublic _((char *, char *, char *, char *, int, int));
extern  void  PlayBack _((char *, char *, char *));
extern  void  AwaySaveToggle _((char *, char *, char *));
#ifdef OPER
extern  void  MassKill _((char *, char *, char *));
extern  void  FilterTrace _((char *, char *, char *));
extern  void  StatsKFilter _((char *, char *, char *));
extern  void  StatsIFilter _((char *, char *, char *));
extern  void  StatsLFilter _((char *, char *, char *));
extern  void  StatsCFilter _((char *, char *, char *));
extern	void  WhoKill _((char *, char *, char *));
extern	void  TraceKill _((char *, char *, char *));
#endif
extern  void  CurrentChanMode _((char *, char *, char *));
extern  void  Nslookup _((char *, char *, char *));
extern  void  Dump _((char *, char *, char *));
extern  void  RemoveLog _((char *, char *, char *));
extern  void  DirLSM _((char *, char *, char *));
extern  void  ShowUser _((char *, char *, char *));
#ifdef WANTANSI
extern  void  SetColor _((char *, char *, char *));
#endif
extern  void  NotifyModeToggle _((char *, char *, char *));
extern  void  PingMe _((char *, char *, char *));
extern  void  NotePad _((char *, char *, char *));
extern  void  URLCatchToggle _((char *, char *, char *));
extern	void  URLSave _((char *, char *, char *));
extern	void  ReplyWord _((char *, char *, char *));
extern	void  MultiKick _((char *, char *, char *));
extern	void  Map _((char *, char *, char *));
extern  void  AddFriendPrivs _((char *, char *, char *));
extern  void  AddFriendChannel _((char *, char *, char *));
extern  void  MassKick _((char *, char *, char *));
extern  void  OnOffCommand _((char *, char *, char *));
extern  void  NumberCommand _((char *, char *, char *));
extern  void  SetAutoCompletion _((char *, char *, char *));
extern  void  ChannelCommand _((char *, char *, char *));
extern  void  AddChannel _((char *, char *, char *));
extern  void  ScrollZInfo _((char *, char *, char *));
extern  void  switchcmd _((char *, char *, char *));
extern  void  repeatcmd _((char *, char *, char *));
extern  void  Purge _((char *, char *, char *));
extern  void  EncryptMsg _((char *, char *, char *));
extern  void  AwaySave _((char *, int));
extern  void  ChangePassword _((char *, char *, char *));
#ifdef MGS_
extern  void  Terminate _((char *, char *, char *));
#endif
extern  void  ARinWindowToggle _((char *, char *, char *));
extern  void  CJoin _((char *, char *, char *));
/* Coded by Zakath */
extern	void  NewHost _((char *, char *, char *));
extern	void  MegaReop _((char *, char *, char *));
extern	void  ServerPing _((char *, char *, char *));
#ifdef CELE
extern	void  ExecUptime _((char *, char *, char *));
extern	void  CTCPFing _((char *, char *, char *));
extern  void  Cquickstat _((char *, char *, char *));
extern	void  ScrollZTrace _((char *, char *, char *));
extern  void  Cstatusbar _((char *, char *, char *));
#endif
#if defined(OPERVISION) && defined(WANTANSI)
extern  void  OperVision _((char *, char *, char *));
#endif
#ifdef EXTRAS
extern  void  DoBans _((char *, char *, char *));
extern  void  MSay _((char *, char *, char *));
extern  void  ModeLocked _((char *, char *, char *));
extern  void  ModeUnlocked _((char *, char *, char *));
extern  void  RandomLamerKick _((char *, char *, char *));
extern  void  LastNoticeKick _((char *, char *, char *));
extern  void  LastMessageKick _((char *, char *, char *));
extern  void  LLook _((char *, char *, char *));
extern  void  LLookUp _((char *, char *, char *));
extern  void  MegaVoice _((char *, char *, char *));
extern  void  MegaDeVoice _((char *, char *, char *));
extern  void  SetIdleKick _((char *, char *, char *));
extern  void  ShowIdle _((char *, char *, char *));
extern  void  TopicLocked _((char *, char *, char *));
#endif
/****************************************************************************/

/* IrcCommand: structure for each command in the command table */
typedef	struct
{
	char	FAR *name;				/* what the user types */
	char	*server_func;				/* what gets sent to the server
							 * (if anything) */
	void	(*func) _((char *, char *, char *));	/* function that is the command */
	unsigned	flags;
}	IrcCommand;

static	IrcCommand *find_command _((char *, int *));

#define NONOVICEABBREV 0x0001
#define	NOINTERACTIVE  0x0002
#define	NOSIMPLESCRIPT 0x0004
#define	NOCOMPLEXSCRIPT 0x0008
#define SERVERREQ	0x0010

/*
 * irc_command: all the available irc commands:  Note that the first entry has
 * a zero length string name and a null server command... this little trick
 * makes "/ blah blah blah" to always be sent to a channel, bypassing queries,
 * etc.  Neato.  This list MUST be sorted.
 */
static	IrcCommand FAR irc_command[] =
{
	{ "",		empty_string,	do_send_text,		NOSIMPLESCRIPT| NOCOMPLEXSCRIPT },
	/*
	 * I really want to remove #, but it will break a lot of scripts.  - mycroft
	 *
	 * I agree - it should be converted to a special character in parse_line.
	 *                                                            - Troy
	 */
	{ "#",		NULL,		commentcmd, 		0 },
	{ ":",		NULL,		commentcmd, 		0 },
#ifndef LITE
        { "ABORT",      NULL,           abortcmd,               0 },
#endif
  { "ADDBK", 		NULL, 		AddAutoBanKick, 	0 },
  { "ADDCHAN", 		"ADD", 		AddChannel, 		0 },
  { "ADDF", 		NULL, 		AddFriend, 		0 },
  { "ADDFCHAN", 	"ADD", 		AddFriendChannel, 	0 },
  { "ADDFFLAG", 	"ADD", 		AddFriendPrivs, 	0 },
  { "ADDN", 		NULL, 		AddNotify, 		SERVERREQ },
#ifndef LITE
  { "ADDS", 		NULL, 		AddServer, 		0 },
#endif
  { "ADDW", 		NULL, 		AddWord, 		0 },
#ifndef LITE
 	{ "ADMIN",	"ADMIN",	send_comm, 		SERVERREQ },
#endif
  { "AJOIN", 		NULL, 		AutoJoinOnInvToggle, 	0 },
	{ "ALIAS",	"0",		alias,			0 },
#ifdef ALLOC_DEBUG
	{ "ALLOC",	NULL,		alloc_cmd,		0 },
#endif
  { "AREJOIN", 		"AREJOIN", 	ChannelCommand, 	0 },
  { "ARINWIN",	 	NULL, 		ARinWindowToggle, 	0 },
	{ "ASSIGN",	"1",		alias,			0 },
  { "AUTOCOMPL", 	"AUTOCOMPL", 	SetAutoCompletion, 	0 },
#if defined(EXTRAS) || defined(FLIER)
  { "AUTOINV", 		"AUTOINV", 	ChannelCommand,		0 },
#endif
  { "AUTOOPDELAY", 	"AUTOOPDELAY", 	NumberCommand, 		0 },
 	{ "AWAY",	"AWAY",		away,			SERVERREQ },
  { "AWAYSAVE", 	NULL, 		AwaySaveToggle, 	0 },
  { "AWAYT", 		"AWAYT", 	NumberCommand, 		0 },
  { "BAN", 		NULL,  		Ban,      		SERVERREQ },
  { "BANTYPE", 		NULL,  		BanType, 		0 },
#ifndef LITE
	{ "BEEP",	0,		beepcmd,		0 },
#endif
	{ "BIND",	NULL,		bindcmd,		0 },
  { "BITCH",  		"BITCH", 	ChannelCommand, 	0 },
  { "BK",  		"BK", 		BanKick, 		SERVERREQ },
#ifdef EXTRAS
  { "BKI", 		"BKI", 		BanKick, 		SERVERREQ },
#endif
  { "BKLIST", 		"BKLIST", 	ChannelCommand,		SERVERREQ },
#ifdef EXTRAS
  { "BKT", 		"BKT", 		BanKick, 		SERVERREQ },
#endif
 	{ "BYE",	"QUIT",		e_quit,			NONOVICEABBREV },
  { "C", 		NULL, 		CurrentChanMode, 	SERVERREQ },
#ifndef LITE
	{ "CAT",	NULL,		catter,			0 },
#endif
	{ "CD",		NULL,		cd,			0 },
  { "CDBAN", 		NULL, 		CdBan, 			SERVERREQ },
  { "CDCC", 		NULL, 		Cdcc, 			0 },
#ifdef CELE
  { "CH", 		NULL, 		CurrentChanMode, 	SERVERREQ },
#endif
 	{ "CHANNEL",	"JOIN",		e_channel,		SERVERREQ },
  { "CHANST", 		NULL, 		ChanStat, 		SERVERREQ },
  { "CHAT", 		NULL, 		Chat, 			SERVERREQ },
  { "CHPASS", 		NULL, 		ChangePassword,		SERVERREQ },
#ifdef EXTRAS
  { "CHSIGNOFF",	"CHSIGNOFF", 	ChannelCommand, 	0 },
#endif
  { "CJOIN",		"CJOIN", 	CJoin,		 	SERVERREQ },
	{ "CLEAR",	NULL,		my_clear,		0 },
  { "CLEARTAB", 	NULL, 		ClearTab, 		0 },
#ifdef WANTANSI
  { "COLOR", 		NULL, 		SetColor, 		0 },
#endif
#ifndef LITE
	{ "COMMENT",	NULL,		commentcmd,		0 },
#endif
  { "COMPRESS",		"COMPRESS", 	ChannelCommand, 	0 },
 	{ "CONNECT",	"CONNECT",	send_comm,		SERVERREQ },
  { "CSCAN", 		NULL, 		ChannelScan, 		SERVERREQ },
 	{ "CTCC",	NULL,		dcc,			SERVERREQ },
 	{ "CTCP",	NULL,		ctcp,			SERVERREQ },
  { "CTCPCLOAK", 	NULL, 		CTCPCloakingToggle, 	0 },
  { "CYCLE", 		NULL, 		Cycle, 			SERVERREQ },
 	{ "DATE",	"TIME",		send_comm,		SERVERREQ },
 	{ "DCC",	NULL,		dcc,			SERVERREQ },
 	{ "DEOP",	NULL,		deop,			SERVERREQ },
  { "DEOPS", 		"DEOPS", 	NumberCommand, 		0 },
  { "DEOPT", 		"DEOPT", 	NumberCommand, 		0 },
 	{ "DESCRIBE",	NULL,		describe,		SERVERREQ },
  { "DHOP",             "DHOP",         Op,                     SERVERREQ },
#ifndef LITE
 	{ "DIE",	"DIE",		send_comm,		SERVERREQ },
#endif
	{ "DIGRAPH",	NULL,		digraph,		0 },
  { "DIRLM", 		"DIRLM", 	DirLM, 			SERVERREQ },
#ifdef EXTRAS
  { "DIRLMK", 		NULL, 		LastMessageKick, 	SERVERREQ },
#endif
  { "DIRLN", 		"DIRLN", 	DirLM, 			SERVERREQ },
#ifdef EXTRAS
  { "DIRLNK", 		NULL, 		LastNoticeKick, 	SERVERREQ },
#endif
  { "DIRLSM", 		"DIRLSM", 	DirLSM, 		SERVERREQ },
  { "DIRLSN", 		"DIRLSN", 	DirLSM, 		SERVERREQ },
 	{ "DISCONNECT",	NULL,		disconnectcmd,		SERVERREQ },
#ifdef EXTRAS
  { "DOBANS", 		NULL, 		DoBans, 		SERVERREQ },
#endif
  { "DOP", 		"DOP", 		Op, 			SERVERREQ },
  { "DPROT", 		"DPROT", 	ChannelCommand, 	0 },
  { "DUMP", 		NULL, 		Dump, 			0 },
	{ "ECHO",	NULL,		my_echo,		0 },
  { "EGO", 		"EGO", 		OnOffCommand, 		0 },
  { "ENCRMSG",		NULL, 		EncryptMsg, 		0 },
#ifndef LITE
	{ "ENCRYPT",	NULL,		encrypt_cmd,		0 },
#endif
	{ "EVAL",	NULL,		evalcmd,		0 },
	{ "EXEC",	NULL,		execcmd,		0 },
 	{ "EXIT",	"QUIT",		e_quit,			NONOVICEABBREV },
  { "EXTMES", 		"EXTMES", 	OnOffCommand, 		0 },
  { "EXTPUB", 		"EXTPUB", 	OnOffCommand, 		0 },
  { "FAKE", 		"FAKE", 	ChannelCommand, 	0 },
  { "FBK", 		"FBK", 		FilterKick, 		SERVERREQ },
#ifdef OPER
  { "FCLINE", 		NULL, 		StatsCFilter, 		SERVERREQ },
#endif
	{ "FE",		NULL,		foreach_handler,	0 },
	{ "FEC",	NULL,		fec,			0 },
#ifdef OPER
  { "FILINE", 		NULL, 		StatsIFilter, 		SERVERREQ },
#endif
#ifdef CELE
  { "FING", 		NULL, 		CTCPFing, 		SERVERREQ },
#endif
#ifndef LITE
  { "FINGER", 		NULL, 		Finger, 		0 },
#endif
  { "FK", 		"FK", 		FilterKick, 		SERVERREQ },
#ifdef OPER
  { "FKLINE", 		NULL, 		StatsKFilter, 		SERVERREQ },
  { "FLLINE", 		NULL, 		StatsLFilter, 		SERVERREQ },
#endif
  { "FLOODP", 		NULL, 		FloodProtToggle, 	0 },
 	{ "FLUSH",	NULL,		flush,			SERVERREQ },
	{ "FOR",	NULL,		foreach_handler,	0 },
#ifdef ACID
  { "FORCEJOIN",	"FORCEJOIN",	ChannelCommand,		SERVERREQ },
#endif
	{ "FOREACH",	NULL,		foreach_handler,	0 },
  { "FRLIST", 		"FRLIST", 	ChannelCommand, 	0 },
#ifdef OPER
  { "FTRACE", 		NULL,		FilterTrace,		0 },
#endif
#ifndef LITE
 	{ "HASH",	"HASH",		send_comm,		SERVERREQ },
	{ "HELP",	NULL,		help,			0 },
#endif
	{ "HISTORY",	NULL,		history,		0 },
	{ "HOOK",	NULL,		hook,			0 },
  { "HOP",              "HOP",          Op,                     SERVERREQ },
#ifndef LITE
 	{ "HOST",	"USERHOST",	userhost,		SERVERREQ },
#endif
#ifdef EXTRAS
  { "IDLEKICK",		NULL,		SetIdleKick,	 	0 },
  { "IDLETIME",		"IDLETIME", 	NumberCommand, 		0 },
#endif
	{ "IF",		NULL,		ifcmd,			0 },
  { "IG", 		NULL, 		Ignore, 		0 },
/**************************** PATCHED by Flier ******************************/
        /*{ "IGNORE",	NULL,		ignore,			0 },*/
        { "IGNORE",	"IGNORE",	ignore,			0 },
/****************************************************************************/
  { "IGTIME", 		"IGTIME", 	NumberCommand, 		0 },
#ifndef LITE
 	{ "INFO",	"INFO",		info,			SERVERREQ },
	{ "INPUT",	NULL,		inputcmd,		0 },
#endif
  { "INV", 		NULL, 		Invite, 		SERVERREQ },
 	{ "INVITE",	"INVITE",	send_comm,		SERVERREQ },
 	{ "ISON",	"ISON",		ison,			SERVERREQ },
  { "J", 		"JOIN", 	e_channel, 		SERVERREQ },
 	{ "JOIN",	"JOIN",		e_channel,		SERVERREQ },
  { "K", 		NULL, 		Kick, 			SERVERREQ },
 	{ "KICK",	"KICK",		send_channel_2args,	SERVERREQ },
  { "KICKONBAN", 	"KICKONBAN", 	ChannelCommand, 	0 },
  { "KICKONFLOOD", 	"KICKONFLOOD", 	ChannelCommand, 	0 },
  { "KICKOPS", 		"KICKOPS", 	ChannelCommand, 	0 },
  { "KICKS", 		"KICKS", 	NumberCommand, 		0 },
  { "KICKT", 		"KICKT", 	NumberCommand, 		0 },
 	{ "KILL",	"KILL",		send_2comm,		SERVERREQ },
  { "KNOCK",		"KNOCK", 	CJoin,		 	SERVERREQ },
  { "KPROT", 		"KPROT", 	ChannelCommand, 	0 },
  { "L", 		NULL, 		Leave, 			SERVERREQ },
	{ "LASTLOG",	NULL,		lastlog,		0 },
 	{ "LEAVE",	"PART",		send_channel_1arg,	SERVERREQ },
 	{ "LINKS",	"LINKS",	send_comm,		NONOVICEABBREV|SERVERREQ },
 	{ "LIST",	"LIST",		funny_stuff,		SERVERREQ },
  { "LISTBK", 		NULL, 		ListAutoBanKicks, 	0 },
  { "LISTF", 		NULL, 		ListFriends, 		0 },
  { "LISTN", 		NULL, 		ListNotify, 		0 },
#ifndef LITE
  { "LISTS", 		NULL, 		ListServers, 		0 },
#endif
  { "LISTW", 		NULL, 		ListWords, 		0 },
  { "LK", 		NULL, 		LameKick, 		SERVERREQ },
#ifdef EXTRAS
  { "LLOOK", 		NULL, 		LLook, 			SERVERREQ },
  { "LLOOKUP", 		NULL, 		LLookUp, 		SERVERREQ },
#endif
	{ "LOAD",	"LOAD",		load,			0 },
  { "LOGON", 		"LOGON", 	OnOffCommand, 		0 },
#ifndef LITE
  { "LS", 		NULL, 		Ls, 			0 },
#endif
 	{ "LUSERS",	"LUSERS",	send_comm,		SERVERREQ },
  { "M", 		"PRIVMSG", 	e_privmsg, 		0 },
#ifndef LITE
  { "MAP", 		NULL, 		Map, 			SERVERREQ },
#endif
#ifdef EXTRAS
  { "MASSDV", 		"MASSDV",	MegaDeVoice,		SERVERREQ },
  { "MASSV", 		"MASSV",	MegaVoice,		SERVERREQ },
#endif
  { "MC", 		NULL, 		ModeClear, 		SERVERREQ },
  { "MDHOP",            NULL,           MegaDehalfop,           SERVERREQ },
  { "MDOP", 		NULL, 		MegaDeop, 		SERVERREQ },
 	{ "ME",		NULL,		me,			SERVERREQ },
  { "MHOP",		NULL,		MassHalfop,		SERVERREQ },
#ifdef WANTANSI
  { "MIRC", 		"MIRC", 	OnOffCommand, 		0 },
#endif
  { "MK", 		NULL, 		MassKick, 		SERVERREQ },
#ifdef OPER
  { "MKILL",            NULL,           MassKill,               SERVERREQ },
#endif
#ifndef LITE
	{ "MLIST",	NULL,		mlist,			0 },
	{ "MLOAD",	NULL,		mload,			0 },
#endif
 	{ "MODE",	"MODE",		send_channel_nargs,	SERVERREQ },
#ifdef EXTRAS
  { "MODELOCK", 	NULL, 		ModeLocked, 		0 },
  { "MODEUNLOCK", 	NULL, 		ModeUnlocked, 		0 },
#endif
  { "MOP", 		NULL, 		MassOp, 		SERVERREQ },
 	{ "MOTD",	"MOTD",		send_comm,		SERVERREQ },
/* Patched by Zakath */
  { "MREOP", 		NULL, 		MegaReop, 		SERVERREQ },
/* ***************** */
#ifdef EXTRAS
  { "MSAY", 		NULL, 		MSay, 			SERVERREQ },
#endif
 	{ "MSG",	"PRIVMSG",	e_privmsg,		0 },
#ifndef LITE
  { "MULTK", 		NULL, 		MultiKick, 		0 },
#endif
  { "N", 		"NOTICE", 	e_privmsg, 		SERVERREQ },
 	{ "NAMES",	"NAMES",	funny_stuff,		SERVERREQ },
  { "NET", 		NULL, 		Net, 			0 },
  { "NEWHOST", 		NULL, 		NewHost, 		SERVERREQ },
  { "NEWUSER", 		NULL, 		NewUser, 		SERVERREQ },
  { "NHPROT", 		NULL, 		NHProtToggle, 		0 },
 	{ "NICK",	"NICK",		e_nick,			SERVERREQ },
  { "NICKS", 		"NICKS", 	NumberCommand, 		0 },
  { "NICKT", 		"NICKT", 	NumberCommand, 		0 },
  { "NOCHAT", 		NULL, 		NoChat, 		0 },
  { "NOIG", 		NULL, 		NoIgnore, 		0 },
  { "NOKEY", 		NULL, 		ClearKey, 		SERVERREQ },
#ifndef LITE
 	{ "NOTE",	"NOTE",		send_comm,		SERVERREQ },
  { "NOTEPAD", 		NULL, 		NotePad, 		0 },
#endif
 	{ "NOTICE",	"NOTICE",	e_privmsg,		SERVERREQ },
 	{ "NOTIFY",	NULL,		notify,			SERVERREQ },
  { "NPROT", 		"NPROT", 	ChannelCommand, 	0 },
#ifndef LITE
  { "NSLOOKUP", 	NULL, 		Nslookup, 		0 },
#endif
  { "NTFYMODE", 	NULL, 		NotifyModeToggle, 	0 },
  { "NWHOIS", 		NULL, 		NickStat, 		0 },
	{ "ON",		NULL,		on,			0 },
  { "OP", 		"OP", 		Op, 			SERVERREQ },
 	{ "OPER",	"OPER",		oper,			SERVERREQ },
  { "ORIGNICK", 	"ORIGNICK", 	ChannelCommand, 	0 },
  { "ORIGNTIME", 	"ORIGNTIME", 	NumberCommand, 	0 },
#if defined(OPERVISION) && defined(WANTANSI)
  { "OV", 		NULL, 		OperVision, 		0 },
#endif
  { "P", 		NULL, 		pingcmd, 		SERVERREQ },
#ifndef LITE
	{ "PARSEKEY",	NULL,		parsekeycmd,		0 },
#endif
 	{ "PART",	"PART",		send_channel_1arg,	SERVERREQ },
  { "PASSWD", 		NULL, 		Password, 		0 },
 	{ "PING",	NULL, 		pingcmd,		SERVERREQ },
  { "PINGME", 		NULL, 		PingMe, 		SERVERREQ },
  { "PLAYBACK", 	"PLAYBACK", 	PlayBack, 		0 },
#ifndef LITE
  { "PURGE", 		NULL, 		Purge, 			0 },
#endif
	{ "QUERY",	NULL,		query,			0 },
#ifndef LITE
	{ "QUEUE",      NULL,           queuecmd,               0 },
#endif
#ifdef CELE
  { "QUICKSTAT",	NULL,           Cquickstat,		0 },
#endif
 	{ "QUIT",	"QUIT",		e_quit,			NONOVICEABBREV },
 	{ "QUOTE",	NULL,		quote,			SERVERREQ },
#ifdef EXTRAS
  { "RANLK", 		NULL, 		RandomLamerKick, 	SERVERREQ },
#endif
#ifdef SCKICKS
  { "RANSK", 		NULL, 		RandomScatterKick, 	SERVERREQ },
#endif
#ifndef LITE
	{ "RBIND",	0,		rbindcmd,		0 },
#endif
  { "RE", 		NULL, 		redirect, 		0 },
  { "RECONNECT",  	NULL,   	ReconnectServer,  	SERVERREQ },
	{ "REDIRECT",	NULL,		redirect,		0 },
 	{ "REHASH",	"REHASH",	send_comm,		SERVERREQ },
#ifdef CELE
  { "RELM", 		"DIRLM", 	DirLM, 			SERVERREQ },
  { "RELN", 		"DIRLN", 	DirLM, 			SERVERREQ },
#endif
  { "RELOAD", 		NULL, 		Reset, 			0 },
  { "REMBK", 		NULL, 		RemoveAutoBanKick, 	0 },
  { "REMCHAN", 		"REM", 		AddChannel, 		0 },
  { "REMF", 		NULL, 		RemoveFriend, 		0 },
  { "REMFCHAN",		"REM", 		AddFriendChannel,	0 },
  { "REMFFLAG",		"REM", 		AddFriendPrivs,		0 },
  { "REMLOG", 		NULL, 		RemoveLog, 		0 },
  { "REMN", 		NULL, 		RemoveNotify, 		0 },
#ifndef LITE
  { "REMS", 		NULL, 		RemoveServer, 		0 },
#endif
  { "REMW", 		NULL, 		RemoveWord, 		0 },
#ifndef LITE
  { "REPEAT", 		NULL, 		repeatcmd, 		0 },
#endif
  { "REPWORD", 		NULL, 		ReplyWord, 		0 },
#ifndef LITE
 	{ "REQUEST",	NULL,		ctcp,			SERVERREQ },
 	{ "RESTART",	"RESTART",	send_comm,		SERVERREQ },
#endif
  { "S", 		NULL, 		FServer, 		0 },
	{ "SAVE",	NULL,		save_settings,		0 },
 	{ "SAY",	empty_string,	do_send_text,		SERVERREQ },
  { "SB", 		NULL, 		ShowBans, 		SERVERREQ },
#ifdef CELE
  { "SC", 		NULL, 		ChannelScan, 		SERVERREQ },
#endif
 	{ "SEND",	NULL,		do_send_text,		SERVERREQ },
#ifndef LITE
	{ "SENDLINE",	empty_string,	sendlinecmd,		0 },
#endif
	{ "SERVER",	NULL,		servercmd,     		0 },
#ifndef LITE
 	{ "SERVLIST",	"SERVLIST",	send_comm,		SERVERREQ },
#endif
  { "SERVNOTICE", 	"SERVNOTICE", 	OnOffCommand, 		0 },
	{ "SET",	NULL,		set_variable,		0 },
  { "SETAWAY", 		NULL, 		SetAway, 		SERVERREQ },
  { "SETBACK", 		NULL, 		SetBack, 		SERVERREQ },
#ifndef LITE
  { "SETTINGS", 	NULL, 		Settings, 		0 },
#endif
  { "SHELP", 		NULL, 		SZHelp,			0 },
  { "SHOWAWAY", 	"SHOWAWAY", 	ChannelCommand, 	0 },
#ifdef EXTRAS
  { "SHOWIDLE", 	NULL, 		ShowIdle, 		0 },
#endif
  { "SHOWKILL", 	NULL, 		ShowKill, 		0 },
  { "SHOWNICK", 	"SHOWNICK", 	OnOffCommand, 		0 },
#ifdef EXTRAS
  { "SHOWSIGN", 	"SHOWSIGN",	OnOffCommand, 		0 },
#endif
  { "SHOWUSER", 	NULL, 		ShowUser, 		SERVERREQ },
  { "SHOWWALLOP", 	"SHOWWALLOP", 	OnOffCommand,		0 },
 	{ "SIGNOFF",	"QUIT",		e_quit,			NONOVICEABBREV },
/* Patched by Zakath */
  { "SINFO", 		"LUSERS", 	send_comm, 		SERVERREQ },
/* ***************** */
#ifdef SCKICKS
  { "SK", 		NULL, 		ScatterKick, 		SERVERREQ },
#endif
	{ "SLEEP",	NULL,		sleepcmd,		0 },
  { "SPING", 		NULL, 		ServerPing, 		SERVERREQ },
#ifndef LITE
 	{ "SQUERY",	"SQUERY",	send_squery,		SERVERREQ },
#endif
 	{ "SQUIT",	"SQUIT",	send_2comm,		SERVERREQ },
#ifndef LITE
	{ "STACK",	NULL,		stackcmd,		0 },
#endif
  { "STAMP",		"STAMP", 	OnOffCommand, 		0 },
 	{ "STATS",	"STATS",	send_comm,		SERVERREQ },
#ifdef CELE
  { "STATUS", 		NULL, 		Cstatusbar, 		SERVERREQ },
#endif
#ifndef LITE
 	{ "SUMMON",	"SUMMON",	send_comm,		SERVERREQ },
#endif
  { "SVE", 		NULL, 		ScrollZSave, 		0 },
#ifndef LITE
  { "SWITCH", 		NULL, 		switchcmd, 		0 },
#endif
  { "SZINFO", 		NULL, 		ScrollZInfo, 		0 },
  { "T", 		NULL, 		Topic, 			SERVERREQ },
#ifdef ACID
  { "TAG", 		NULL, 		TagNick,		SERVERREQ },
#endif
  { "TBAN", 		NULL, 		TBan, 			SERVERREQ },
#ifdef MGS_
  { "TERMINATE", 	NULL, 		Terminate, 		0 },
#endif
 	{ "TIME",	"TIME",		send_comm,		SERVERREQ },
	{ "TIMER",	"TIMER",	timercmd,		0 },
#ifdef OPER
  { "TKILL", 		NULL, 		TraceKill, 		SERVERREQ },
#endif
 	{ "TOPIC",	"TOPIC",	send_topic,		SERVERREQ },
#ifdef EXTRAS
  { "TOPICLOCK", 	"TOPICLOCK", 	TopicLocked, 		0 },
  { "TOPICUNLOCK", 	"TOPICUNLOCK", 	TopicLocked, 		0 },
#endif
/**************************** PATCHED by Flier ******************************/
	/*{ "TRACE",	"TRACE",	send_comm,		0 },*/
/* Patched by Zakath */
#ifdef CELE
  { "TRACE", 		NULL, 		ScrollZTrace, 		SERVERREQ },
#else
 	{ "TRACE",	"TRACE",	send_comm,		SERVERREQ },
#endif
/****************************************************************************/
#ifndef LITE
	{ "TYPE",	NULL,		type,			0 },
#endif
  { "UMODE", 		NULL, 		UserMode, 		SERVERREQ },
  { "UNBAN", 		NULL, 		Unban, 			SERVERREQ },
  { "UNFLASH", 		NULL, 		UnFlash, 		0 },
#ifdef CELE
  { "UPTIME", 		NULL, 		ExecUptime, 		0 },
#endif
  { "URL", 		NULL, 		URLSave, 		0 },
  { "URLCATCH", 	"URLCATCH", 	URLCatchToggle, 	0 },
 	{ "USERHOST",	NULL,		userhost,		SERVERREQ },
 	{ "USERS",	"USERS",	send_comm,		SERVERREQ },
  { "VER", 		NULL, 		Version, 		SERVERREQ },
	{ "VERSION",	"VERSION",	version,		0 },
/**************************** PATCHED by Flier ******************************/
 	/*{ "VOICE",	"VOICE",	e_privmsg,		SERVERREQ },*/
/****************************************************************************/
  { "W", 		"WHO", 		who, 			SERVERREQ },
 	{ "WAIT",	NULL,		waitcmd,		SERVERREQ },
/**************************** PATCHED by Flier ******************************/
 	/*{ "WALL",	"WALL",		e_wall,			SERVERREQ },*/
  { "WALL",  		NULL, 		ChanWallOp, 		SERVERREQ },
#ifndef LITE
        { "WALLMSG",	"WALL",		e_wall,			SERVERREQ },
/****************************************************************************/
 	{ "WALLOPS",	"WALLOPS",	e_wall,			SERVERREQ },
#endif
#ifdef ACID
  { "WHEREIS", 		NULL, 		WhereIs,		SERVERREQ },
  { "WHERELIST",	NULL, 		WhereList,		0 },
#endif
#ifndef LITE
	{ "WHICH",	"WHICH",	load,			0 },
#endif
	{ "WHILE",	NULL,		whilecmd,		0 },
 	{ "WHO",	"WHO",		who,			SERVERREQ },
 	{ "WHOIS",	"WHOIS",	whois,			SERVERREQ },
  { "WHOLEFT", 		NULL, 		WhoLeft, 		0 },
 	{ "WHOWAS",	"WHOWAS",	whois,			SERVERREQ },
  { "WI", 		"WHOIS", 	whois, 			SERVERREQ },
  { "WII",		"WII",		whois, 			SERVERREQ },
	{ "WINDOW",	NULL,		windowcmd,     		0 },
#ifdef OPER
  { "WKILL", 		NULL, 		WhoKill, 		SERVERREQ },
#endif
  { "WW", 		"WHOWAS", 	whois, 			SERVERREQ },
#ifndef LITE
	{ "XECHO",	"XECHO",	my_echo,		0 },
 	{ "XTRA",	"XTRA",		e_privmsg,		SERVERREQ },
	{ "XTYPE",	NULL,		xtypecmd,		0 },
#endif
	{ NULL,		NULL,		commentcmd,		0 }
};

/* number of entries in irc_command array */
# define	NUMBER_OF_COMMANDS (sizeof(irc_command) / sizeof(IrcCommand)) - 2

/*
 * find_command: looks for the given name in the command list, returning a
 * pointer to the first match and the number of matches in cnt.  If no
 * matches are found, null is returned (as well as cnt being 0). The command
 * list is sorted, so we do a binary search.  The returned commands always
 * points to the first match in the list.  If the match is exact, it is
 * returned and cnt is set to the number of matches * -1.  Thus is 4 commands
 * matched, but the first was as exact match, cnt is -4.
 */
static	IrcCommand *
find_command(com, cnt)
	char	*com;
	int	*cnt;
{
 	size_t	len;

	if (com && (len = strlen(com)))
	{
		int	min,
			max,
			pos,
			old_pos = -1,
			c;

		min = 1;
		max = NUMBER_OF_COMMANDS + 1;
		while (1)
		{
			pos = (max + min) / 2;
			if (pos == old_pos)
			{
				*cnt = 0;
				return ((IrcCommand *) 0);
			}
			old_pos = pos;
			c = strncmp(com, irc_command[pos].name, len);
			if (c == 0)
				break;
			else if (c > 0)
				min = pos;
			else
				max = pos;
		}
		*cnt = 0;
		(*cnt)++;
		min = pos - 1;
		while ((min > 0) && (strncmp(com, irc_command[min].name,
				len) == 0))
		{
			(*cnt)++;
			min--;
		}
		min++;
		max = pos + 1;
		while ((max < NUMBER_OF_COMMANDS + 1) && (strncmp(com,
				irc_command[max].name, len) == 0))
		{
			(*cnt)++;
			max++;
		}
		if (*cnt)
		{
			if (strlen(irc_command[min].name) == len)
				*cnt *= -1;
			else if (*cnt == 1 && 
					irc_command[min].flags&NONOVICEABBREV &&
					get_int_var(NOVICE_VAR))
			{
				say("As a novice you may not abbreviate the %s command", irc_command[min].name);
				*cnt=0;
				return ((IrcCommand *) 0);
			}
			return (&(irc_command[min]));
		}
		else
			return ((IrcCommand *) 0);
	}
	else
	{
		*cnt = -1;
		return (irc_command);
	}
}

/*ARGSUSED*/
static	void
ctcp(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*to,
		*tag;
 	int	ctcptype;

	if ((to = next_arg(args, &args)) != NULL)
	{
		if (!strcmp(to, "*"))
			if ((to = get_channel_by_refnum(0)) == NULL)
				to = zero;
		if ((tag = next_arg(args, &args)) != NULL)
			upper(tag);
		else
			tag = "VERSION";
		if ((ctcptype = in_ctcp()) == -1)
			my_echo(NULL, "*** You may not use the CTCP command in an ON CTCP_REPLY!", empty_string);
		else
		{
			if (args && *args)
				send_ctcp(ctcp_type[ctcptype], to, tag, "%s", args);
			else
				send_ctcp(ctcp_type[ctcptype], to, tag, NULL);
		}
	}
	else
		say("Request from whom?");
}

/*ARGSUSED*/
static	void
hook(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (*args)
		do_hook(HOOK_LIST, "%s", args);
	else
		say("Must supply an argument to HOOK");
}

/*ARGSUSED*/
static	void
dcc(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (*args)
		process_dcc(args);
        else
/**************************** PATCHED by Flier ******************************/
		/*dcc_list((char *) NULL);*/
                Cdcc(NULL,NULL,NULL);
/****************************************************************************/
}

/*ARGSUSED*/
static	void
deop(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	send_to_server("MODE %s -o", get_server_nickname(from_server));
}

static	void
funny_stuff(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg,
		*cmd = (char *) 0,
		*stuff,
		*s;
	int	min = 0,
		max = 0,
		flags = 0;
 	size_t	len;

/*************************** PATCHED by Flier ****************************/
        if (!(args && *args)) {
            say("Type %s -YES if you really mean it",command);
            return;
        }
/*************************************************************************/
	stuff = empty_string;
	while ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		malloc_strcpy(&cmd, arg);
		upper(cmd);
		if (strncmp(cmd, "-MAX", len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
				max = atoi(arg);
		}
		else if (strncmp(cmd, "-MIN", len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
				min = atoi(arg);
		}
		else if (strncmp(cmd, "-ALL", len) == 0)
		{
			flags &= ~(FUNNY_PUBLIC | FUNNY_PRIVATE);
		}
		else if (strncmp(cmd, "-PUBLIC", len) == 0)
		{
			flags |= FUNNY_PUBLIC;
			flags &= ~FUNNY_PRIVATE;
		}
		else if (strncmp(cmd, "-PRIVATE", len) == 0)
		{
			flags |= FUNNY_PRIVATE;
			flags &= ~FUNNY_PUBLIC;
		}
		else if (strncmp(cmd, "-TOPIC", len) == 0)
			flags |= FUNNY_TOPIC;
		else if (strncmp(cmd, "-WIDE", len) == 0)
			flags |= FUNNY_WIDE;
		else if (strncmp(cmd, "-USERS", len) == 0)
			flags |= FUNNY_USERS;
		else if (strncmp(cmd, "-NAME", len) == 0)
			flags |= FUNNY_NAME;
/*************************** PATCHED by Flier ****************************/
		/*else*/
		else if (my_strnicmp(arg,"-YES",len))
/*************************************************************************/
			stuff = arg;
		new_free(&cmd);
	}
	set_funny_flags(min, max, flags);
	if (strcmp(stuff, "*") == 0)
		if (!(stuff = get_channel_by_refnum(0)))
			stuff = empty_string;
	if ((s = index(stuff, '*')) && !(s > stuff && s[-1] == ':'))
	{
		funny_match(stuff);
		send_to_server("%s %s", command, empty_string);
	}
	else
	{
		funny_match(NULL);
		send_to_server("%s %s", command, stuff);
	}
}

/*ARGSUSED*/
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
waitcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
#ifdef _Windows
	yell("WAIT is not available under Windows");
#else /* Windows */
	int	wait_index;
	char	*flag;
	char	*procindex;
	int	cmd = 0;
 	size_t	len;
	u_char	buffer[BIG_BUFFER_SIZE+1];

	while (args && *args == '-')
	{
		flag = next_arg(args, &args);
		len = strlen(++flag);
		if (!my_strnicmp("CMD", flag, len))
		{
			cmd = 1;
			break;
		}
		else
			yell("Unknown argument to WAIT: %s", flag);
	}
	if ((procindex = next_arg(args, &args)) && *procindex == '%' &&
			(wait_index = get_process_index(&procindex)) != -1)
	{
		if (is_process_running(wait_index))
		{
			if (cmd)
			{
				add_process_wait(wait_index, args?args:empty_string);
				return;
			}
			else
				set_wait_process(wait_index);
		}
		else
		{
			say("Not a valid process!");
			return;
		}
	}
	else if (cmd)
	{
		WaitCmd	*new;

		sprintf(buffer, "%s %s", procindex, args);
		new = (WaitCmd *) new_malloc(sizeof(WaitCmd));
		new->stuff = NULL;
		malloc_strcpy(&new->stuff, buffer);
		new->next = NULL;
		if (end_wait_list)
			end_wait_list->next = new;
		end_wait_list = new;
		if (!start_wait_list)
			start_wait_list = new;
		send_to_server("%s", wait_nick);
		return;
	}
	else
		send_to_server("%s", lame_wait_nick);
	if (waiting)
		yell("WAIT has been called recursively.");

	waiting++;
	irc_io(NULL, NULL, 0, 1);
	waiting--;
#endif /* _Windows */
}

int
check_wait_command(nick)
	char 	*nick;
{
	if (waiting && !strcmp(nick, lame_wait_nick))
	{
		irc_io_loop = 0;
		return 1;
	}
	if (start_wait_list && !strcmp(nick, wait_nick))
	{
		if (start_wait_list->stuff)
		{
			parse_command(start_wait_list->stuff, 0, empty_string);
			new_free(&start_wait_list->stuff);
		}
		start_wait_list = start_wait_list->next;
		return 1;
	}
	return 0;
}

/*ARGSUSED*/
static	void
redirect(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*to;

	if ((to = next_arg(args, &args)) != NULL)
	{
/**************************** PATCHED by Flier ******************************/
                DCC_list *Client;

                if (*to=='=') {
                    to++;
                    if (!(Client=dcc_searchlist((char *) 0,to,DCC_CHAT,0,(char *) 0)) ||
                        !(Client->flags&DCC_ACTIVE)) {
                        say("No active DCC CHAT connection with %s",to);
                        return;
                    }
                    to--;
                }
/****************************************************************************/
		if (!strcmp(to, "*"))
			if (!(to = get_channel_by_refnum(0)))
			{
				say("Must be on a channel to redirect to '*'");
				return;
			}
		if (!my_stricmp(to, get_server_nickname(from_server)))
		{
			say("You may not redirect output to yourself");
			return;
		}
		window_redirect(to, from_server);
		server_list[from_server].sent = 0;
		parse_line((char *) 0, args, (char *) 0, 0, 0);
		if (server_list[from_server].sent)
			send_to_server("%s", current_screen->redirect_token,
				current_screen->screennum);
		else
			window_redirect(NULL, from_server);
	}
	else
/**************************** PATCHED by Flier ******************************/
		/*say("Usage: REDIRECT <nick|channel|%process> <cmd>");*/
		say("Usage: REDIRECT <nick|channel|%%process> <cmd>");
/****************************************************************************/
}

/*ARGSUSED*/
static	void
sleepcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
#ifndef _Windows
	char	*arg;

	if ((arg = next_arg(args, &args)) != NULL)
 		sleep((unsigned)atoi(arg));
	else
		say("SLEEP: you must specify the amount of time to sleep (in seconds)");
#else
	say("SLEEP: Not available under Windows");
#endif /* _Windows */
}

/*
 * my_echo: simply displays the args to the screen, or, if it's XECHO,
 * processes the flags first, then displays the text on
 * the screen
 */
static void
my_echo(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	unsigned int	display;
	int	lastlog_level = 0;
	int	from_level = 0;
	char	*flag_arg;
	int	temp;
	Window *old_to_window;

 	save_message_from();
	old_to_window = to_window;
	if (command && *command == 'X')
	{
		while (args && *args == '-')
		{
			flag_arg = next_arg(args, &args);
			switch(flag_arg[1])
			{
			case 'L':
			case 'l':
				if (!(flag_arg = next_arg(args, &args)))
					break;
				if ((temp = parse_lastlog_level(flag_arg)) != 0)
				{
					lastlog_level = set_lastlog_msg_level(temp);
					from_level = message_from_level(temp);
				}
				break;
			case 'W':
			case 'w':
				if (!(flag_arg = next_arg(args, &args)))
					break;
				if (isdigit(*flag_arg))
 					to_window = get_window_by_refnum((unsigned)atoi(flag_arg));
				else
					to_window = get_window_by_name(flag_arg);
 				lastlog_level = set_lastlog_msg_level(LOG_CRAP);
				from_level = message_from_level(LOG_CRAP);
				break;
			}
			if (!args)
				args = empty_string;
		}
	}
	display = window_display;
	window_display = 1;
	put_it("%s", args);
	window_display = display;
	if (lastlog_level)
		set_lastlog_msg_level(lastlog_level);
	if (from_level)
		message_from_level(from_level);
 	restore_message_from();
	to_window = old_to_window;
}

/*
 */
static	void
oper_password_received(data, line)
	char	*data;
	char	*line;
{
	send_to_server("OPER %s %s", data, line);
/**************************** PATCHED by Flier ******************************/
        bzero(line,strlen(line));
        say("Password's memory location has been cleared");
/****************************************************************************/
}

/* oper: the OPER command.  */
/*ARGSUSED*/
static	void
oper(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*password;
	char	*nick;

	oper_command = 1;
	if (!(nick = next_arg(args, &args)))
		nick = nickname;
	if (!(password = next_arg(args, &args)))
	{
		add_wait_prompt("Operator Password:",
			oper_password_received, nick, WAIT_PROMPT_LINE);
		return;
	}
	send_to_server("OPER %s %s", nick, password);
/**************************** PATCHED by Flier ******************************/
        bzero(password,strlen(password));
        say("Password's memory location has been cleared");
/****************************************************************************/
}

/* Full scale abort.  Does a "save" into the filename in line, and
        then does a coredump */
#ifndef LITE
static  void   
abortcmd(command, args, subargs)
	char    *command,
		*args,
		*subargs;
{
        char    *filename = next_arg(args, &args);

	if (!filename)
		filename = "irc.aborted";
	save_which = SFLAG_ALIAS | SFLAG_BIND | SFLAG_ON | SFLAG_SET |
			     SFLAG_NOTIFY | SFLAG_DIGRAPH;
        really_save(filename, "y");
#ifdef ALLOC_DEBUG
        alloc_cmd("ALLOC", "d", (char *) 0);
#endif
        abort();
}
#endif /* LITE */
        
/* This generates a file of your ircII setup */
static	void
really_save(file, line)
	char	*file;
	char	*line;
{
/**************************** PATCHED by Flier ******************************/
        int     oldumask;
/****************************************************************************/
	FILE	*fp;

	if (*line != 'y' && *line != 'Y')
		return;
/**************************** PATCHED by Flier ******************************/
        oldumask=umask(0177);
/****************************************************************************/
	if ((fp = fopen(file, "w")) != NULL)
	{
		if (save_which & SFLAG_BIND)
			save_bindings(fp, save_do_all);
		if (save_which & SFLAG_ON)
			save_hooks(fp, save_do_all);
		if (save_which & SFLAG_NOTIFY)
			save_notify(fp);
		if (save_which & SFLAG_SET)
			save_variables(fp, save_do_all);
		if (save_which & SFLAG_ALIAS)
			save_aliases(fp, save_do_all);
		if (save_which & SFLAG_DIGRAPH)
			save_digraphs(fp);
		fclose(fp);
		say("IRCII settings saved to %s", file);
	}
	else
		say("Error opening %s: %s", file, strerror(errno));
/**************************** PATCHED by Flier ******************************/
        umask(oldumask);
/****************************************************************************/
}

/* save_settings: saves the current state of IRCII to a file */
/*ARGSUSED*/
static	void
save_settings(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	buffer[BIG_BUFFER_SIZE+1];
	char	*arg, *temp;
 	int	all = 1, save_force = 0;

	save_which = save_do_all = 0;
	while ((arg = next_arg(args, &args)) != NULL)
	{
		if ('-' == *arg)
		{
			char	*cmd = NULL;

			all = 0;
			malloc_strcpy(&cmd, arg+1);
			upper(cmd);
			if (0 == strncmp("ALIAS", cmd, 5))
				save_which |= SFLAG_ALIAS;
			else if (0 == strncmp("ASSIGN", cmd, 6))
				save_which |= SFLAG_ALIAS;
			else if (0 == strncmp("BIND", cmd, 4))
				save_which |= SFLAG_BIND;
			else if (0 == strncmp("ON", cmd, 2))
				save_which |= SFLAG_ON;
			else if (0 == strncmp("SET", cmd, 3))
				save_which |= SFLAG_SET;
			else if (0 == strncmp("NOTIFY", cmd, 6))
				save_which |= SFLAG_NOTIFY;
			else if (0 == strncmp("DIGRAPH", cmd, 7))
				save_which |= SFLAG_DIGRAPH;
			else if (0 == strncmp("ALL", cmd, 3))
				save_do_all = 1;
 			else if (0 == strncmp("FORCE", cmd, 3))
 				save_force = 1;
			else
			{
				say("%s: unknown argument", arg);
				new_free(&cmd);
				return;
			}
			new_free(&cmd);
			continue;
		}
#ifdef DAEMON_UID
		if (getuid() == DAEMON_UID)
		{
			say("You may only use the default value");
			return;
		}
#endif /* DAEMON_UID */
		temp = expand_twiddle(arg);
		if (temp)
		{
			if (ircrc_file)
				new_free(&ircrc_file);
			ircrc_file = temp;
		}
		else
		{
			say("Unknown user");
			return;
		}
	}
	if (all)
		save_which = SFLAG_ALIAS | SFLAG_BIND | SFLAG_ON | SFLAG_SET |
/**************************** PATCHED by Flier ******************************/
			     /*SFLAG_NOTIFY | SFLAG_DIGRAPH;*/
			     SFLAG_DIGRAPH;
/****************************************************************************/
 	if (dumb || save_force)
		really_save(ircrc_file, "y"); /* REAL dumb!  -lynx */
	else
	{
		sprintf(buffer, "Really write %s? ", ircrc_file);
		add_wait_prompt(buffer, really_save, ircrc_file,
				WAIT_PROMPT_LINE);
	}
}

/*
 * do_channel : checks whether the channel has already been joined and
 * returns the channel's name if not
 */
static	char *
do_channel(chan, force)
	char	*chan;
	int force;
{
	ChannelList	*channel;
	char		*old;
/**************************** PATCHED by Flier ******************************/
        /* use serv_ind instead of curr_scr_win->server because we might
         * be called from hook or from timer and in that case curr_scr_win
         * might not have proper server context */
        int serv_ind=from_server;
/****************************************************************************/

 	if (serv_ind < 0)
 		return (char *) 0;

        channel = lookup_channel(chan, serv_ind, CHAN_NOUNLINK);
/**************************** PATCHED by Flier ******************************/
        /* if you try to join same channel twice and you reached limit for
           maximum number of channels this fix prevents client from removing
           channel from memory */
        if (channel && channel->connected==CHAN_JOINING) return(NULL);
/****************************************************************************/

	if (is_bound(chan, serv_ind) && channel && channel->window != curr_scr_win)
		say("Channel %s is bound", chan);
	else if (is_on_channel(chan, serv_ind, get_server_nickname(serv_ind)))
	{
		is_current_channel(chan, serv_ind, 1);
		say("You are now talking to channel %s", set_channel_by_refnum(0, chan));
		update_all_windows();
	}
	else
	{
		/* only do this if we're actually joining a new channel */
		if (get_int_var(NOVICE_VAR))
		{
			if ((old = get_channel_by_refnum(0)) && strcmp(old, zero))
				send_to_server("PART %s", old);
		}
/**************************** Patched by Flier ******************************/
                /* we should not add !!channel on ircd 2.10 as they denote
                   channel creation */
                if (*chan=='!' && (get_server_version(serv_ind))==Server2_10)
                    return(chan);
/****************************************************************************/
		add_channel(chan, serv_ind, CHAN_JOINING, (ChannelList *) 0);
		force = 1;
	}
	if (force)
		return chan;
	else
		return (char *) 0;
}

/**************************** PATCHED by Flier ******************************/
/* fix_channel: add # in front of channel if necessary */
static char *fix_channel(channel)
char *channel;
{
    static char chanbuf[mybufsize/4+2];

    if (!is_channel(channel)) {
        strcpy(chanbuf,"#");
        strmcat(chanbuf,channel,mybufsize/4);
    }
    else strmcpy(chanbuf,channel,mybufsize/4);
    return(chanbuf);
}
/****************************************************************************/

/*
 * e_channel: does the channel command.  I just added displaying your current
 * channel if none is given 
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
e_channel(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*chan;
 	size_t	len;
	char	*chanstr = (char *) 0,
		*ptr;
 	int 	force = 0;
/**************************** PATCHED by Flier ******************************/
        char    *chankey;
/****************************************************************************/

	if (get_server_version(from_server) == Server2_5)
		command = "CHANNEL";
 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);		/* XXX should delete this */
/**************************** PATCHED by Flier ******************************/
        /* we are /CYCLEing */
        if (args==subargs) {
            send_to_server("%s %s",command,args);
            restore_message_from();
            return;
        }
/****************************************************************************/
        if ((chan = next_arg(args, &args)) != NULL)
	{
		len = MAX(2, strlen(chan));
		if (my_strnicmp(chan, "-force", len) == 0)
		{
			force = 1;
			if ((chan = next_arg(args, &args)) == NULL)
				goto out;	/* XXX: allow /alias join join -force */
			len = MAX(2, strlen(chan));
		}
		if (my_strnicmp(chan, "-invite", len) == 0)
		{
			if (invite_channel)
			{
				if ((ptr = do_channel(invite_channel, force)))
/**************************** PATCHED by Flier ******************************/
					/*send_to_server("%s %s %s", command, invite_channel, args);*/
                                {
                                    chankey=CheckJoinKey(invite_channel);
                                    send_to_server("%s %s %s %s",command,invite_channel,
                                                   args,chankey);
                                }
/****************************************************************************/
				else
					say("You are already on %s ?", invite_channel);
			}
			else
				say("You have not been invited to a channel!");
		}
		else
		{
			malloc_strcpy(&chanstr, chan);
			chan = chanstr;

			if (get_int_var(NOVICE_VAR))
				chanstr = strtok(chanstr, ",");

			ptr = strtok(chanstr, ",");
/**************************** PATCHED by Flier ******************************/
                        ptr=fix_channel(ptr);
/****************************************************************************/
			if ((ptr = do_channel(ptr, force)) && *ptr)
/**************************** PATCHED by Flier ******************************/
				/*send_to_server("%s %s %s", command, ptr, args);*/
                        {
                            chankey=CheckJoinKey(ptr);
                            send_to_server("%s %s %s %s",command,ptr,args,chankey);
                        }
/****************************************************************************/
			while ((ptr = strtok(NULL, ",")))
/**************************** PATCHED by Flier ******************************/
				/*if ((ptr = do_channel(ptr, force)) && *ptr)
					send_to_server("%s %s %s", command, ptr, args);*/
                        {
                            ptr=fix_channel(ptr);
                            if ((ptr=do_channel(ptr,force)) && *ptr) {
                                chankey=CheckJoinKey(ptr);
                                send_to_server("%s %s %s %s",command,ptr,args,chankey);
                            }
                        }
/****************************************************************************/

			new_free(&chan);
		}
	}
	else
out:
		list_channels();
	restore_message_from();
}

/* comment: does the /COMMENT command, useful in .ircrc */
/*ARGSUSED*/
static	void
commentcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	/* nothing to do... */
}

/*
 * e_nick: does the /NICK command.  Records the users current nickname and
 * sends the command on to the server 
 */
/*ARGSUSED*/
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
e_nick(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick;

	if (!(nick = next_arg(args, &args)))
	{
		say("Your nickname is %s",
			get_server_nickname(get_window_server(0)));
		return;
	}
/*************************** PATCHED by Flier ****************************/
/*#if 0*/ /* blundernet */
/*************************************************************************/
	if ((nick = check_nickname(nick)) != NULL)
	{
/*************************** PATCHED by Flier ****************************/
/*#endif*/
                LastNick=time((time_t *) 0);
/*************************************************************************/
		send_to_server("NICK %s", nick);
/**************************** PATCHED by Flier ******************************/
                /* fix by Flier: this totally confuses client if connection to
                   server fails, so only do this if we are not connected to server
		if (attempting_to_connect)*/
		if (attempting_to_connect && !server_list[from_server].connected)
			/*set_server_nickname(get_window_server(0),nick);
                          fix by Flier: use from_server here otherwise client gets
                          totally confused when using /timer nick and the window where
                          you issued /timer is not connected yet */
			set_server_nickname(from_server,nick);
/****************************************************************************/
		if (get_server_version(from_server) == Server2_5)
			add_to_whois_queue(nick, whois_nickname,
				NULL);
/**************************** PATCHED by Flier ******************************/
/*#if 0*/ /* blundernet */
/****************************************************************************/
	}
	else
		say("Bad nickname");
/**************************** PATCHED by Flier ******************************/
/*#endif*/
/****************************************************************************/
}

/* version: does the /VERSION command with some IRCII version stuff */
static	void
version(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*host;

	if ((host = next_arg(args, &args)) != NULL)
		send_to_server("%s %s", command, host);
	else
	{ 
/**************************** PATCHED by Flier ******************************/
		/*say("Client: ircII %s (internal version %s)", irc_version, internal_version);*/
                say("Client: ircII %s + ScrollZ %s [%s] (int. %s)",irc_version,
                    ScrollZver1,VersionInfo,internal_version);
/****************************************************************************/
		send_to_server("%s", command);
	}
}

/*
 * info: does the /INFO command.  I just added some credits
 * I updated most of the text -phone, feb 1993.
 */
#ifndef LITE
static	void
info(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (!args || !*args)
	{
		say("ircii: originally written by Michael Sandrof");
		say("       versions 2.1 to 2.2pre7 by Troy Rollo");
		say("       development continued by matthew green");
		say("       e-mail: mrg@eterna.com.au  irc: phone");
		say("       copyright (c) 1990-2000");
		say("       do a /help ircii copyright for the full copyright");
		say("       ircii includes software developed by the university");
		say("       of california, berkeley and its contributors");
		say("");
		say("ircii contributors");
		say("");
		say("       \tMichael Sandrof       Mark T. Dameu");
		say("       \tStellan Klebom        Carl v. Loesch");
		say("       \tTroy Rollo            Martin  Friedrich");
		say("       \tMichael Weber         Bill Wisner");
		say("       \tRiccardo Facchetti    Stephen van den Berg");
		say("       \tVolker Paulsen        Kare Pettersson");
		say("       \tIan Frechette         Charles Hannum");
		say("       \tmatthew green         christopher williams");
		say("       \tJonathan Lemon        Brian Koehmstedt");
		say("       \tNicolas Pioch         Brian Fehdrau");
		say("       \tDarren Reed           Jeff Grills");
		say("       \tJeremy Nelson         Philippe Levan");
		say("       \tScott Reynolds        Glen McCready");
		say("       \tChristopher Kalt      Joel Yliluoma");
	}
	send_to_server("%s %s", command, args);
}
#endif

void
/**************************** PATCHED by Flier ******************************/
/*ison_now(notused, nicklist, notused2)*/
ison_now(notused,notused2,nicklist)
/****************************************************************************/
	WhoisStuff	*notused;
	char		*nicklist,
			*notused2;
{
	if (do_hook(current_numeric, "%s", nicklist))
/**************************** Patched by Flier ******************************/
		/*put_it("%s Currently online: %s", numeric_banner(), nicklist);*/
		put_it("%sCurrently online: %s", numeric_banner(), nicklist);
/****************************************************************************/
}

static	void
ison(command, args, subargs)
	char	*command;
	char	*args,
		*subargs;
{
	if (!args[strspn(args, " ")])
		args = get_server_nickname(from_server);
	add_ison_to_whois(args, ison_now);

}

/*
 * userhost: Does the USERHOST command.  Need to split up the queries,
 * since the server will only reply to 5 at a time.
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
userhost(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	n = 0,
		total = 0,
		userhost_cmd = 0;
	char	*nick;
	char	buffer[BIG_BUFFER_SIZE+1];

	while ((nick = next_arg(args, &args)) != NULL)
	{
 		size_t	len;

		++total;
		len = strlen(nick);
		if (!my_strnicmp(nick, "-CMD", len))
		{
			if (total < 2)
			{
				yell("userhost -cmd with no nick!");
				return;
			}
			userhost_cmd = 1;
			break;
		}
		else
		{
			if (n++)
				strmcat(buffer, " ", BIG_BUFFER_SIZE);
			else
				*buffer = '\0';
			strmcat(buffer, nick, BIG_BUFFER_SIZE);
		}
	}
	if (n)
	{
		char	*the_list = (char *) 0;
		char	*s, *t;
		int	i;

		malloc_strcpy(&the_list, buffer);
		s = t = the_list;
		while (n)
		{
			for (i = 5; i && *s; s++)
				if (' ' == *s)
					i--, n--;
			if (' ' == *(s - 1))
				*(s - 1) = '\0';
			else
				n--;
			strcpy(buffer, t);
			t = s;

			if (userhost_cmd)
				add_to_whois_queue(buffer, userhost_cmd_returned, "%s", args);
			else
				add_to_whois_queue(buffer, USERHOST_USERHOST, 0);
		}
		new_free(&the_list);
	}
	else if (!total)
		/* Default to yourself.  */
		add_to_whois_queue(get_server_nickname(from_server), USERHOST_USERHOST, 0);
}

/*
 * whois: the WHOIS and WHOWAS commands.  This translates the 
 * to the whois handlers in whois.c 
 */
static	void
whois(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
/*************************** PATCHED by Flier ****************************/
	/*if (args && *args)
		send_to_server("%s %s", command, args);
	else*/ /* Default to yourself  -lynx */
		/*send_to_server("%s %s", command, get_server_nickname(from_server));*/
#ifdef CELECOSM
    whoisfriend=(struct friends *) 0;
#endif
    if (args && *args) {
        if (strcmp(command,"WII")) send_to_server("%s %s", command, args);
        else send_to_server("WHOIS %s %s",args,args);
    }
    else {
        if (strcmp(command,"WII")) send_to_server("%s %s", command, get_server_nickname(from_server));
        else send_to_server("WHOIS %s %s",get_server_nickname(from_server),get_server_nickname(from_server));
    }
/*************************************************************************/
}

/*
 * who: the /WHO command. Parses the who switches and sets the who_mask and
 * whoo_stuff accordingly.  Who_mask and who_stuff are used in whoreply() in
 * parse.c 
 */
static	void
who(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg,
		*channel = NULL;
 	int	no_args = 1;
 	size_t	len;

	who_mask = 0;
	new_free(&who_name);
	new_free(&who_host);
	new_free(&who_server);
	new_free(&who_file);
	new_free(&who_nick);
	new_free(&who_real);
	while ((arg = next_arg(args, &args)) != NULL)
	{
		no_args = 0;
		if ((*arg == '-') && (!isdigit(*(arg + 1))))
		{
			char	*cmd = NULL;

			arg++;
			if ((len = strlen(arg)) == 0)
			{
				say("Unknown or missing flag");
				return;
			}
			malloc_strcpy(&cmd, arg);
			lower(cmd);
			if (strncmp(cmd, "operators", len) == 0)
				who_mask |= WHO_OPS;
			else if (strncmp(cmd, "lusers", len) == 0)
				who_mask |= WHO_LUSERS;
			else if (strncmp(cmd, "chops", len) == 0)
				who_mask |= WHO_CHOPS;
/**************************** Patched by Flier ******************************/
                        else if (strncmp(cmd, "hops", len) == 0)
                                who_mask |= WHO_HOPS;
/****************************************************************************/
			else if (strncmp(cmd, "hosts", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_HOST;
					malloc_strcpy(&who_host, arg);
					channel = who_host;
				}
				else
				{
					say("WHO -HOSTS: missing arguement");
					new_free(&cmd);
					return;
				}
			}
			else if (strncmp(cmd, "here", len) ==0)
				who_mask |= WHO_HERE;
			else if (strncmp(cmd, "away", len) ==0)
				who_mask |= WHO_AWAY;
			else if (strncmp(cmd, "servers", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_SERVER;
					malloc_strcpy(&who_server, arg);
					channel = who_server;
				}
				else
				{
					say("WHO -SERVERS: missing arguement");
					new_free(&cmd);
					return;
				}
			}
			else if (strncmp(cmd, "name", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_NAME;
					malloc_strcpy(&who_name, arg);
					channel = who_name;
				}
				else
				{
					say("WHO -NAME: missing arguement");
					new_free(&cmd);
					return;
				}
			}
			else if (strncmp(cmd, "realname", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_REAL;
					malloc_strcpy(&who_real, arg);
					channel = who_real;
				}
				else
				{
					say("WHO -REALNAME: missing arguement");
					new_free(&cmd);
					return;
				}
			}
			else if (strncmp(cmd, "nick", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_NICK;
					malloc_strcpy(&who_nick, arg);
					channel = who_nick;
				}
				else
				{
					say("WHO -NICK: missing arguement");
					new_free(&cmd);
					return;
				}
				/* WHO -FILE by Martin 'Efchen' Friedrich */
			}
			else if (strncmp(cmd, "file", len) == 0)
			{
				who_mask |= WHO_FILE;
				if ((arg = next_arg(args, &args)) != NULL)
				{
					malloc_strcpy(&who_file, arg);
				}
				else
				{
					say("WHO -FILE: missing arguement");
					new_free(&cmd);
					return;
				}
			}
/**************************** PATCHED by Flier ******************************/
			else if (strncmp(cmd,"show_server",len)==0) {
                            who_mask|=WHO_SHOW_SERVER;
                            if ((arg=next_arg(args,&args))!=NULL) channel=arg;
                            else channel=get_channel_by_refnum(0);
                        }
/****************************************************************************/
			else
			{
				say("Unknown or missing flag");
				new_free(&cmd);
				return;
			}
			new_free(&cmd);
		}
		else if (strcmp(arg, "*") == 0)
		{
			channel = get_channel_by_refnum(0);
			if (!channel || *channel == '0')

			{
				say("I wouldn't do that if I were you");
				return;
			}
		}
		else
			channel = arg;
	}
/**************************** PATCHED by Flier ******************************/
        if (no_args) {
		/*say("No argument specified");*/
            if (get_channel_by_refnum(0))
                send_to_server("%s %s",command,get_channel_by_refnum(0));
            else NoWindowChannel();
        }
/****************************************************************************/
	else
	{
		if (!channel && who_mask & WHO_OPS)
			channel = "*";
		send_to_server("%s %s %c", command, channel ? channel :
				empty_string, (who_mask & WHO_OPS) ?
					'o' : '\0');
	}
}

/*
 * query: the /QUERY command.  Works much like the /MSG, I'll let you figure
 * it out.
 */
/*ARGSUSED*/
void
query(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick,
		*rest;

 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);
	if ((nick = next_arg(args, &rest)) != NULL)
	{
		if (strcmp(nick, ".") == 0)
		{
			if (!(nick = (char *) sent_nick))
			{
				say("You have not messaged anyone yet");
				goto out;
			}
		}
		else if (strcmp(nick, ",") == 0)
		{
			if (!(nick = (char *) recv_nick))
			{
				say("You have not recieved a message from \
						anyone yet");
				goto out;
			}
		}
		else if (strcmp(nick, "*") == 0)
			if (!(nick = get_channel_by_refnum(0)))
			{
				say("You are not on a channel");
				goto out;
			}

#ifndef _Windows
		if (*nick == '%')
		{
			if (is_process(nick) == 0)
			{
				say("Invalid processes specification");
				goto out;
			}
		}
#endif /* _Windows */
		say("Starting conversation with %s", nick);
		set_query_nick(nick);
	}
	else
	{
		if (query_nick())
		{
			say("Ending conversation with %s", query_nick());
			set_query_nick(NULL);
		}
		else
			say("You aren't querying anyone!");
	}
	update_input(UPDATE_ALL);
out:
 	restore_message_from();
}

/*
 * away: the /AWAY command.  Keeps track of the away message locally, and
 * sends the command on to the server.
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
away(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
 	size_t	len;
	char	*arg = NULL;
	int	flag = AWAY_ONE;
	int	i;

	if (*args)
	{
		if (*args == '-')
		{
                        char	*cmd = (char *) 0;

			args = next_arg(args, &arg);
			len = strlen(args);
			if (len == 0)
			{
				say("%s: No argument given with -", command);
				return;
			}
			malloc_strcpy(&cmd, args);
 			upper(cmd);
			if (0 == strncmp(cmd, "-ALL", len))
			{
				flag = AWAY_ALL;
				args = arg;
			}
			else if (0 == strncmp(cmd, "-ONE", len))
			{
				flag = AWAY_ONE;
				args = arg;
			}
			else
			{
				say("%s: %s unknown flag", command, args);
 				new_free(&cmd);
				return;
			}
 			new_free(&cmd);
		}
	}
	if (flag == AWAY_ALL)
		if (*args)
		{
			away_set = 1;
			MarkAllAway(command, args);
		}
		else
		{
			away_set = 0;
/**************************** PATCHED by Flier ******************************/
                        MarkAllAway(command, NULL);
/****************************************************************************/
			for(i = 0; (i < number_of_servers); i++)
/**************************** PATCHED by Flier ******************************/
				/*if (server_list[i].whois_stuff.away)*/
/****************************************************************************/
					new_free(&(server_list[i].away));
		}
	else
	{
		send_to_server("%s :%s", command, args);
		if (*args)
		{
			away_set = 1;
			malloc_strcpy(&(server_list[
				curr_scr_win->server].away), args);
		}
		else
		{
			new_free(&(server_list[
				curr_scr_win->server].away));
			away_set = 0;
			for(i = 0; (i < number_of_servers) && !away_set ; i++)
				if (server_list[i].read != -1 &&
						server_list[i].away)
					away_set = 1;
		}
	}
	update_all_status();
}

/* e_quit: The /QUIT, /EXIT, etc command */
/*ARGSUSED*/
void
e_quit(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
/**************************** PATCHED by Flier ******************************/
	/*int	max;
	int	i;*/
/****************************************************************************/
	char	*Reason;

/**************************** PATCHED by Flier ******************************/
	/*Reason = ((args && *args) ? args : "Leaving");
        max = number_of_servers;
	for (i = 0; i < max; i++)
		if (is_server_connected(i))
		{
			from_server = i;
			send_to_server("QUIT :%s", Reason);
		}
        irc_exit();*/
        Reason = ((args && *args) ? args : NULL);
        IRCQuit=0;
        MyQuit(Reason);
/****************************************************************************/
}

/* flush: flushes all pending stuff coming from the server */
/*ARGSUSED*/
static	void
flush(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (get_int_var(HOLD_MODE_VAR))
	{
		while (curr_scr_win->held_lines)
			remove_from_hold_list(curr_scr_win);
		hold_mode((Window *) 0, OFF, 1);
	}
	say("Standby, Flushing server output...");
	flush_server();
	say("Done");
}

/* e_wall: used for WALL and WALLOPS */
#ifndef LITE
static	void
e_wall(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
 	save_message_from();
	if (strcmp(command, "WALL") == 0)
	{	/* I hate this */
		message_from(NULL, LOG_WALL);
		if (!get_server_operator(from_server))
			put_it("## %s", args);
	}
	else
	{
		message_from(NULL, LOG_WALLOP);
/**************************** PATCHED by Flier ******************************/
		/*if (!get_server_flag(from_server, USER_MODE_W))*/
		if (!get_server_umode_flag(from_server,'w'))
			put_it("!! %s", args);
/****************************************************************************/
	}
	if (!in_on_who)
		send_to_server("%s :%s", command, args);
 	restore_message_from();
}
#endif

/*
 * e_privmsg: The MSG command, displaying a message on the screen indicating
 * the message was sent.  Also, this works for the NOTICE command. 
 */
static	void
e_privmsg(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		if (strcmp(nick, ".") == 0)
		{
			if (!(nick = (char *) sent_nick))
			{
				say("You have not sent a message to anyone yet");
				return;
			}
		}

		else if (strcmp(nick, ",") == 0)
		{
			if (!(nick = (char *) recv_nick))
			{
				say("You have not received a message from anyone yet");
				return;
			}
		}
		else if (!strcmp(nick, "*"))
			if (!(nick = get_channel_by_refnum(0)))
				nick = zero;
/**************************** PATCHED by Flier ******************************/
                if (my_stricmp(nick,"0")) AddNick2List(nick,from_server);
/****************************************************************************/
		send_text(nick, args, command);
	}
	else
		say("You must specify a nickname or channel!");
}

/*
 * quote: handles the QUOTE command.  args are a direct server command which
 * is simply send directly to the server 
 */
/*ARGSUSED*/
static	void
quote(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (!in_on_who)
		send_to_server("%s", args);
}

/* clear: the CLEAR command.  Figure it out */
/*ARGSUSED*/
static	void
my_clear(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	int	all = 0,
		unhold = 0;

	while ((arg = next_arg(args, &args)) != NULL)
	{
		upper(arg);
		/* -ALL and ALL here becuase the help files used to be wrong */
		if (!strncmp(arg, "ALL", strlen(arg))
				|| !strncmp(arg, "-ALL", strlen(arg)))
			all = 1;
		else if (!strncmp(arg, "-UNHOLD", strlen(arg)))
			unhold = 1;
		else
			say("Unknown flag: %s", arg);
	}
	if (all)
		clear_all_windows(unhold);
	else
	{
		if (unhold)
			hold_mode((Window *) 0, OFF, 1);
		clear_window_by_refnum(0);
	}
	update_input(UPDATE_JUST_CURSOR);
}

/*
 * send_comm: the generic command function.  Uses the full command name found
 * in 'command', combines it with the 'args', and sends it to the server 
 */
static	void
send_comm(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
/**************************** PATCHED by Flier ******************************/
        if (command && *command) {
            if (!my_stricmp(command,"LINKS")) {
                time_t timenow=time((time_t *) 0);

                if (timenow-LastLinks<120 && inSZLinks) {
                    say("Wait till previous LINKS, LLOOK, LLOOKUP or MAP completes");
                    return;
                }
                LastLinks=timenow;
                LinksNumber=0;
                inSZLinks=3;
            }
            else if (!my_stricmp(command,"STATS")) {
#ifdef OPER
                StatskNumber=0;
                StatsiNumber=0;
                StatscNumber=0;
                StatslNumber=0;
                new_free(&StatskFilter);
                new_free(&StatsiFilter);
                new_free(&StatscFilter);
                new_free(&StatslFilter);
#endif
            }
        }
/****************************************************************************/
	if (args && *args)
		send_to_server("%s %s", command, args);
	else
		send_to_server("%s", command);
}


static	void
send_topic(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	u_char	*arg;
	u_char	*arg2;

	if (!(arg = next_arg(args, &args)) || (strcmp(arg, "*") == 0))
		arg = get_channel_by_refnum(0);

	if (!arg)
	{
		say("You aren't on a channel in this window");
		return;
	}
	if (is_channel(arg))
	{
		if ((arg2 = next_arg(args, &args)) != NULL)
			send_to_server("%s %s :%s %s", command, arg,
					arg2, args);
		else
			send_to_server("%s %s", command, arg);
	}
	else
	if (get_channel_by_refnum(0))
/**************************** Patched by Flier ******************************/
		/*send_to_server("%s %s :%s", command, get_channel_by_refnum(0), subargs);*/
		send_to_server("%s %s :%s %s",command,get_channel_by_refnum(0),arg,args);
/****************************************************************************/
	else
		say("You aren't on a channel in this window");
}

#ifndef LITE
static void
send_squery(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	put_it("*** Sent to service %s: %s", command, args);
	send_2comm(command, args, subargs);
}
#endif

/*
 * send_2comm: Sends a command to the server with one arg and
 * one comment. Used for KILL and SQUIT.
 */

static	void
send_2comm(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
 	char	*comment;

 	args = next_arg(args, &comment);

 	send_to_server("%s %s %c%s",
 		       command,
 		       args && *args ? args : "",
 		       comment && *comment ? ':' : ' ',
 		       comment && *comment ? comment : "");
}

/*
 * send_channel_nargs: Sends a command to the server with one channel,
 * and 0-n args. Used for MODE.
 */
  
static	void
send_channel_nargs(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg1 = 0,
	        *s = get_channel_by_refnum(0);

 	args = next_arg(args, &arg1);
 	if (!args || !strcmp(args, "*"))
 	{
 		if(s)
 			args = s;
  		else
 		{
 			say("You aren't on a channel in this window");
 			return;
 		}
 	}

 	send_to_server("%s %s %s",
 		       command,
 		       args,
 		       arg1 && *arg1 ? arg1 : "");
}

/*
 * send_channel_2args: Sends a command to the server with one channel,
 * one arg and one comment. Used for KICK
 */

static	void
send_channel_2args(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg1 = 0,
		*comment = 0,
	        *s = get_channel_by_refnum(0);

	args = next_arg(args, &arg1);
	if (!args || !strcmp(args, "*"))
	{
		if(s)
			args = s;
		else
		{
			say("You aren't on a channel in this window");
			return;
		}
	}

	if (arg1 && *arg1)
		arg1 = next_arg(arg1, &comment);
	
	send_to_server("%s %s %s %c%s",
		       command,
		       args,
		       arg1 && *arg1 ? arg1 : "",
		       comment && *comment ? ':' : ' ',
		       comment && *comment ? comment : "");
}

/*
 * send_channel_1arg: Sends a command to the server with one channel
 * and one comment. Used for PART (LEAVE)
 */
static	void
send_channel_1arg(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*comment,
		*s = get_channel_by_refnum(0);

	args = next_arg(args, &comment);

	if (!args || !strcmp(args, "*"))
	{
		if (s)
			args = s;
		else
		{
			say("You aren't on a channel in this window");
			return;
		}
	}

	send_to_server("%s %s %c%s",
		       command,
		       args,
		       comment && *comment ? ':' : ' ',
		       comment && *comment ? comment : "");
}

/*
 * send_text: Sends the line of text to whomever the user is currently
 * talking.  If they are quering someone, it goes to that user, otherwise
 * it's the current channel.  Some tricky parameter business going on. If
 * nick is null (as if you had typed "/ testing" on the command line) the
 * line is sent to your channel, and the command parameter is never used. If
 * nick is not null, and command is null, the line is sent as a PRIVMSG.  If
 * nick is not null and command is not null, the message is sent using
 * command.  Currently, only NOTICEs and PRIVMSGS work. 
 * fixed to not be anal about "/msg foo,bar foobar" -phone
 */
void
send_text(org_nick, line, command)
	char	*org_nick;
	char	*line;
	char	*command;
{
#ifndef LITE
 	crypt_key	*key;
#endif
 	char 	*ptr,
		*free_nick,
		*nick = NULL;
	int	lastlog_level,
		list_type,
		old_server;
	int	check_away = 0;
	char	the_thing;
	char	*query_command = NULL;
	char	nick_list[IRCD_BUFFER_SIZE+1];
	int	do_final_send = 0;
/**************************** PATCHED by Flier ******************************/
        int     iscrypted;
        char    thing;
        char    *mynick=get_server_nickname(from_server);
        char    tmpbuf[BIG_BUFFER_SIZE+1];
        char    *stampbuf=TimeStamp(2);

        if (get_int_var(HIGH_ASCII_VAR)) thing='ù';
        else thing='-';
/****************************************************************************/
	if (dumb && translation)
	{
		ptr = line;
		while (*ptr)
		{
			*ptr = transFromClient[*(u_char*) ptr];
			ptr++;
		}
	}

	*nick_list = '\0';
	malloc_strcpy(&nick, org_nick);
	free_nick = ptr = nick;
 	save_message_from();
	while ((nick = ptr) != NULL)
	{
		if ((ptr = index(nick, ',')) != NULL)
			*(ptr++) = (char) 0;
		if (!*nick)
			continue;
#ifndef _Windows
		if (is_process(nick))
		{
			int	i;

			if ((i = get_process_index(&nick)) == -1)
				say("Invalid process specification");
			else
				text_to_process(i, line, 1);
			continue;
		}
#endif /* _Windows */
		if (!*line)
			continue; /* lynx */
		if (in_on_who && *nick != '=') /* allow dcc messages anyway */
		{
			say("You may not send messages from ON WHO, ON WHOIS, or ON JOIN");
			continue;
		}
		if (doing_privmsg)
			command	= "NOTICE";
		/* Query quote -lynx */
		if (strcmp(nick, "\"") == 0) /* quote */
		{
			send_to_server("%s", line);
			continue;
		}
		if (*nick == '=') /* DCC chat */
		{
			old_server = from_server;
			from_server = -1;
			dcc_chat_transmit(nick + 1, line);
			from_server = old_server;
			continue;
		}
/**************************** PATCHED by Flier ******************************/
                /*if (*nick == '@')*/ /* DCC talk */
		/*{
			old_server = from_server;
			from_server = -1;
			dcc_message_transmit(nick + 1, line, DCC_TALK, 1);
			from_server = old_server;
			continue;
		}*/
/****************************************************************************/
		if (*nick == '/') /* Command */
		{
			malloc_strcpy(&query_command, nick);
			malloc_strcat(&query_command, " ");
			malloc_strcat(&query_command, line);
			parse_command(query_command, 0, empty_string);
			new_free(&query_command);
			continue;
		}
		switch (send_text_flag)
		{
		case MSG_LIST:
			command = "NOTICE";
			break;
		case NOTICE_LIST:
			say("You cannot send a message from a NOTICE");
			new_free(&free_nick);
			goto out;
		}
/**************************** PATCHED by Flier ******************************/
                *tmpbuf='\0';
                iscrypted=EncryptMessage(tmpbuf,nick);
/****************************************************************************/
                if (is_channel(nick))
		{
			int	current;

			current = is_current_channel(nick, curr_scr_win->server, 0);
			if (!command || strcmp(command, "NOTICE"))
			{
				check_away = 1;
				command = "PRIVMSG";
				lastlog_level = set_lastlog_msg_level(LOG_PUBLIC);
				message_from(nick, LOG_PUBLIC);
				list_type = SEND_PUBLIC_LIST;
				the_thing = '>';
			}
			else
			{
				lastlog_level = set_lastlog_msg_level(LOG_NOTICE);
				message_from(nick, LOG_NOTICE);
				list_type = SEND_NOTICE_LIST;
				the_thing = '-';
			}
			if (do_hook(list_type, "%s %s", nick, line))
			{
/**************************** PATCHED by Flier ******************************/
				/*if (current)
					put_it("%c %s", the_thing, line);
				else
					put_it("%c%s> %s", the_thing, nick,
						line);*/
                                if (current) {
                                    if (!my_stricmp(command,"NOTICE")) put_it("%s-%s- %s",stampbuf,nick,line);
                                    else PrintPublic(mynick,NULL,nick,line,1,iscrypted);
                                }
                                else {
                                    if (!my_stricmp(command,"NOTICE")) put_it("%s-%s- %s",stampbuf,nick,line);
                                    else PrintPublic(mynick,":",nick,line,1,iscrypted);
                                }
                                tabnickcompl=NULL;
/****************************************************************************/
			}
/**************************** PATCHED by Flier ******************************/
                        else if (my_stricmp(command,"NOTICE")) 
                            PrintPublic(mynick,NULL,nick,line,0,iscrypted);
                        if ((away_set || LogOn) && my_stricmp(command,"NOTICE")) {
                            sprintf(tmpbuf,"<%s:%s> %s",mynick,nick,line);
                            AwaySave(tmpbuf,SAVESENTMSG);
                        }
/****************************************************************************/
			set_lastlog_msg_level(lastlog_level);
#ifndef LITE
			if ((key = is_crypted(nick)) != 0)
			{
				char	*crypt_line;

				if ((crypt_line = crypt_msg(line, key, 1)))
					send_to_server("%s %s :%s", command, nick, crypt_line);
				continue;
			}
#endif
/**************************** PATCHED by Flier ******************************/
                        strmcpy(tmpbuf,line,mybufsize);
                        if (EncryptMessage(tmpbuf,nick)) {
                            send_to_server("%s %s :%s",command,nick,tmpbuf);
                            continue;
                        }
/****************************************************************************/
			if (!in_on_who)
			{
				if (*nick_list)
				{
					strcat(nick_list, ",");
					strcat(nick_list, nick);
				}
				else
					strcpy(nick_list, nick);
			}
			do_final_send = 1;
		}
		else
		{
			if (!command || strcmp(command, "NOTICE"))
			{
 				check_away = 1;
				lastlog_level = set_lastlog_msg_level(LOG_MSG);
				command = "PRIVMSG";
				message_from(nick, LOG_MSG);
				list_type = SEND_MSG_LIST;
				the_thing = '*';
			}
			else
			{
				lastlog_level = set_lastlog_msg_level(LOG_NOTICE);
				message_from(nick, LOG_NOTICE);
				list_type = SEND_NOTICE_LIST;
				the_thing = '-';
			}
/**************************** PATCHED by Flier ******************************/ 
                        /*if (window_display && do_hook(list_type, "%s %s", nick, line))
				put_it("-> %c%s%c %s", the_thing, nick, the_thing, line);*/
                        if (window_display && do_hook(list_type, "%s %s", nick, line)) {
                            if (!my_stricmp(command,"NOTICE")) {
#ifdef WANTANSI
#ifdef CELECOSM
                                sprintf(tmpbuf,"%s[%s%s(notice)%s%s%s%s%s]%s",
                                          CmdsColors[COLNOTICE].color4,Colors[COLOFF],
                                          CmdsColors[COLCELE].color2,Colors[COLOFF],
                                          CmdsColors[COLNOTICE].color2,nick,Colors[COLOFF],
                                          CmdsColors[COLNOTICE].color4,Colors[COLOFF]);
#else  /* CELECOSM */
                                sprintf(tmpbuf,"%s<%s-%s%s%s-%s>%s",
                                        CmdsColors[COLNOTICE].color4,Colors[COLOFF],
                                        CmdsColors[COLNOTICE].color2,nick,Colors[COLOFF],
                                        CmdsColors[COLNOTICE].color4,Colors[COLOFF]);
#endif /* CELECOSM */
                                put_it("%s%s%s %s%s%s",iscrypted?"[!]":"",stampbuf,tmpbuf,
                                       CmdsColors[COLNOTICE].color3,line,Colors[COLOFF]);
#else  /* WANTANSI */
                                put_it("%s%s<-%s-> %s",iscrypted?"[!]":"",stampbuf,nick,line);
#endif /* WANTANSI */
                            }
                            else {
#ifdef WANTANSI
#ifdef CELECOSM
                                sprintf(tmpbuf,"%s[%s%s(msg)%s%s%s%s%s]%s",
                                        CmdsColors[COLMSG].color5,Colors[COLOFF],
                                        CmdsColors[COLCELE].color1,Colors[COLOFF],
                                        CmdsColors[COLMSG].color6,nick,Colors[COLOFF],
                                        CmdsColors[COLMSG].color5,Colors[COLOFF]);
#else  /* CELECOSM */
                                sprintf(tmpbuf,"%s[%s%c%s%s%s%c%s]%s",
					CmdsColors[COLMSG].color5,Colors[COLOFF],thing,
					CmdsColors[COLMSG].color6,nick,Colors[COLOFF],
                                        thing,CmdsColors[COLMSG].color5,Colors[COLOFF]);
#endif /* CELECOSM */
                                put_it("%s%s%s %s%s%s",stampbuf,iscrypted?"[!]":"",tmpbuf,
					CmdsColors[COLMSG].color3,line,Colors[COLOFF]);
#else  /* WANTANSI */
                                put_it("%s%s[%c%s%c] %s",stampbuf,iscrypted?"[!]":"",thing,nick,thing,line);
#endif /* WANTANSI */
                            }
                        }
                        if (line!=server_list[from_server].LastMessageSent &&
                            line!=server_list[from_server].LastNoticeSent) {
                            if (!my_stricmp(command,"NOTICE")) {
                                sprintf(tmpbuf,"-> -%s- %s",nick,line);
                                if (CheckServer(from_server))
                                    malloc_strcpy(&(server_list[from_server].LastNoticeSent),
                                            tmpbuf);
                            }
                            else {
                                sprintf(tmpbuf,"-> [-%s-] %s",nick,line);
                                if (CheckServer(from_server))
                                    malloc_strcpy(&(server_list[from_server].LastMessageSent),
                                            tmpbuf);
                                if (away_set || LogOn) AwaySave(tmpbuf,SAVESENTMSG);
                            }
                        }
/****************************************************************************/
#ifndef LITE
			if ((key = is_crypted(nick)) != NULL)
			{
				char	*crypt_line;

				if ((crypt_line = crypt_msg(line, key, 1)))
					send_to_server("%s %s :%s", command ? command : "PRIVMSG", nick, crypt_line);
				continue;
			}
#endif
			set_lastlog_msg_level(lastlog_level);
/**************************** PATCHED by Flier ******************************/
                        strmcpy(tmpbuf,line,mybufsize);
                        if (EncryptMessage(tmpbuf,nick)) {
                            send_to_server("%s %s :%s",command?command:"PRIVMSG",nick,
                                           tmpbuf);
                            continue;
                        }
/****************************************************************************/

			if (!in_on_who)
			{
				if (*nick_list)
				{
					strcat(nick_list, ",");
					strcat(nick_list, nick);
				}
				else
					strcpy(nick_list, nick);
			}

			if (get_int_var(WARN_OF_IGNORES_VAR) && (is_ignored(nick, IGNORE_MSGS) == IGNORED))
				say("Warning: You are ignoring private messages from %s", nick);

			malloc_strcpy((char **) &sent_nick, nick);
			do_final_send = 1;
		}
	}
	if (check_away && server_list[curr_scr_win->server].away && get_int_var(AUTO_UNMARK_AWAY_VAR))
		away("AWAY", empty_string, empty_string);

	malloc_strcpy((char **) &sent_body, line);
	if (do_final_send)
		send_to_server("%s %s :%s", command ? command : "PRIVMSG", nick_list, line);
	new_free(&free_nick);
out:
 	restore_message_from();
}

static void
do_send_text(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*tmp;
/**************************** PATCHED by Flier ******************************/ 
        char    tmpbuf[mybufsize];
        ChannelList *tmpchan;
/****************************************************************************/

	if (command)
		tmp = get_channel_by_refnum(0);
	else
		tmp = get_target_by_refnum(0);
/**************************** PATCHED by Flier ******************************/ 
        if (AutoNickCompl && tmp && args && *args &&
            (tmpchan=lookup_channel(tmp,curr_scr_win->server,0))) {
            AutoNickComplete(args,tmpbuf,tmpchan);
            args=tmpbuf;
        }
/****************************************************************************/
	send_text(tmp, args, NULL);
}

/*
 * command_completion: builds lists of commands and aliases that match the
 * given command and displays them for the user's lookseeing 
 */
void
command_completion(key, ptr)
 	u_int	key;
	char *	ptr;
{
	int	do_aliases;
	int	cmd_cnt,
		alias_cnt,
		i,
		c,
		len;
	char	**aliases = NULL;
	char	*line = NULL,
		*com,
		*cmdchars,
		*rest,
		firstcmdchar = '/';
	char	buffer[BIG_BUFFER_SIZE+1];
	IrcCommand	*command;

	malloc_strcpy(&line, get_input());
	if ((com = next_arg(line, &rest)) != NULL)
	{
		if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
			cmdchars = DEFAULT_CMDCHARS;
		if (index(cmdchars, *com))
		{
			firstcmdchar = *cmdchars;
			com++;
			if (*com && index(cmdchars, *com))
			{
				do_aliases = 0;
				alias_cnt = 0;
				com++;
			}
			else
				do_aliases = 1;
			upper(com);
			if (do_aliases)
				aliases = match_alias(com, &alias_cnt,
					COMMAND_ALIAS);
			if ((command = find_command(com, &cmd_cnt)) != NULL)
			{
				if (cmd_cnt < 0)
					cmd_cnt *= -1;
				/* special case for the empty string */

				if (*(command[0].name) == (char) 0)
				{
					command++;
					cmd_cnt = NUMBER_OF_COMMANDS;
				}
			}
			if ((alias_cnt == 1) && (cmd_cnt == 0))
			{
				sprintf(buffer, "%c%s %s", firstcmdchar,
					aliases[0], rest);
				set_input(buffer);
				new_free(&(aliases[0]));
				new_free(&aliases);
				update_input(UPDATE_ALL);
			}
			else if (((cmd_cnt == 1) && (alias_cnt == 0)) ||
			    ((cmd_cnt == 1) && (alias_cnt == 1) &&
			    (strcmp(aliases[0], command[0].name) == 0)))
			{
				sprintf(buffer, "%c%s%s %s", firstcmdchar,
					do_aliases ? "" : &firstcmdchar,
					command[0].name, rest);
				set_input(buffer);
				update_input(UPDATE_ALL);
			}
			else
			{
				*buffer = (char) 0;
				if (command)
				{
					say("Commands:");
					strmcpy(buffer, "\t", BIG_BUFFER_SIZE);
					c = 0;
					for (i = 0; i < cmd_cnt; i++)
					{
						strmcat(buffer, command[i].name,
							BIG_BUFFER_SIZE);
						for (len =
						    strlen(command[i].name);
						    len < 15; len++)
							strmcat(buffer, " ",
							    BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							say("%s", buffer);
							strmcpy(buffer, "\t",
							    BIG_BUFFER_SIZE);
							c = 0;
						}
					}
					if (c)
						say("%s", buffer);
				}
				if (aliases)
				{
					say("Aliases:");
					strmcpy(buffer, "\t", BIG_BUFFER_SIZE);
					c = 0;
					for (i = 0; i < alias_cnt; i++)
					{
						strmcat(buffer, aliases[i],
							BIG_BUFFER_SIZE);
						for (len = strlen(aliases[i]);
								len < 15; len++)
							strmcat(buffer, " ",
							    BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							say("%s", buffer);
							strmcpy(buffer, "\t",
							    BIG_BUFFER_SIZE);
							c = 0;
						}
						new_free(&(aliases[i]));
					}
					if ((int) strlen(buffer) > 1)
						say("%s", buffer);
					new_free(&aliases);
				}
				if (!*buffer)
					term_beep();
			}
		}
		else
			term_beep();
	}
	else
		term_beep();
	new_free(&line);
}

/*
 * parse_line: This is the main parsing routine.  It should be called in
 * almost all circumstances over parse_command().
 *
 * parse_line breaks up the line into commands separated by unescaped
 * semicolons if we are in non interactive mode. Otherwise it tries to leave
 * the line untouched.
 *
 * Currently, a carriage return or newline breaks the line into multiple
 * commands too. This is expected to stop at some point when parse_command
 * will check for such things and escape them using the ^P convention.
 * We'll also try to check before we get to this stage and escape them before
 * they become a problem.
 *
 * Other than these two conventions the line is left basically untouched.
 */
void
parse_line(name, org_line, args, hist_flag, append_flag)
	char	*name,
		*org_line,
		*args;
	int	hist_flag,
		append_flag;
{
	char	*line = NULL,
		*free_line,
		*stuff,
		*lbuf,
		*s,
		*t;
	int	args_flag;

	malloc_strcpy(&line, org_line);
	free_line = line;
	args_flag = 0;
	if (!*line)
		do_send_text(NULL, empty_string, empty_string);
	else if (args)
		do
		{
			stuff = expand_alias(name, line, args, &args_flag,
					&line);
			if (!line && append_flag && !args_flag && args && *args)
			{
				lbuf = (char *) new_malloc(strlen(stuff) + 1 + strlen(args) + 1);
				strcpy(lbuf, stuff);
				strcat(lbuf, " ");
				strcat(lbuf, args);
  				new_free(&stuff);
 				stuff = lbuf;
			}
			parse_command(stuff, hist_flag, args);
			new_free(&stuff);
		}
		while(line);
	else
	{
		if (load_depth)
			parse_command(line, hist_flag, args);
		else
			while ((s = line))
			{
				if ((t = sindex(line, "\r\n")) != NULL)
				{
					*t++ = '\0';
					line = t;
				}
				else
					line = NULL;
				parse_command(s, hist_flag, args);
			}
	}
	new_free(&free_line);
	return;
}

/*
 * parse_command: parses a line of input from the user.  If the first
 * character of the line is equal to irc_variable[CMDCHAR_VAR].value, the
 * line is used as an irc command and parsed appropriately.  If the first
 * character is anything else, the line is sent to the current channel or to
 * the current query user.  If hist_flag is true, commands will be added to
 * the command history as appropriate.  Otherwise, parsed commands will not
 * be added. 
 *
 * Parse_command() only parses a single command.  In general, you will want
 * to use parse_line() to execute things.Parse command recognized no quoted
 * characters or anything (beyond those specific for a given command being
 * executed). 
 */
void
parse_command(line, hist_flag, sub_args)
	char	*line;
	int	hist_flag;
	char	*sub_args;
{
	static	unsigned int	 level = 0;
	unsigned int	display,
			old_display_var;
	char	*cmdchars,
		*com,
		*this_cmd = NULL;
	int	args_flag,
		add_to_hist,
		cmdchar_used;

	if (!line || !*line)
		return;
	if (get_int_var(DEBUG_VAR) & DEBUG_COMMANDS)
		yell("Executing [%d] %s", level, line);
	level++;
	if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
		cmdchars = DEFAULT_CMDCHARS;
	malloc_strcpy(&this_cmd, line);
	add_to_hist = 1;
	if (index(cmdchars, *line))
	{
		cmdchar_used = 1;
		com = line + 1;
	}
	else
	{
		cmdchar_used = 0;
		com = line;
	}
	/*
	 * always consider input a command unless we are in interactive mode
	 * and command_mode is off.   -lynx
	 */
	if (hist_flag && !cmdchar_used && !get_int_var(COMMAND_MODE_VAR))
	{
		do_send_text(NULL, line, empty_string);
		if (hist_flag && add_to_hist)
		{
			add_to_history(this_cmd);
			set_input(empty_string);
		}
		/* Special handling for ' and : */
	}
	else if (*com == '\'' && get_int_var(COMMAND_MODE_VAR))
	{
		do_send_text(NULL, line+1, empty_string);
		if (hist_flag && add_to_hist)
		{
			add_to_history(this_cmd);
			set_input(empty_string);
		}
	}
	else if (*com == '@')
	{
		/* This kludge fixes a memory leak */
		char	*tmp;

		tmp = parse_inline(line + 1, sub_args, &args_flag);
		if (tmp)
			new_free(&tmp);
		if (hist_flag && add_to_hist)
		{
			add_to_history(this_cmd);
			set_input(empty_string);
		}
	}
	else
	{
		char	*rest,
			*nalias = NULL,
			*alias_name;
		int	cmd_cnt,
			alias_cnt;
		IrcCommand	*command; /* = (IrcCommand *) 0 */

		display = window_display;
		old_display_var = (unsigned) get_int_var(DISPLAY_VAR);
		if ((rest = (char *) index(com, ' ')) != NULL)
			*(rest++) = (char) 0;
		else
			rest = empty_string;
		upper(com);

		/* first, check aliases */
		if (*com && index(cmdchars, *com))
		{
			alias_cnt = 0;
			com++;
			if (*com == '^')
			{
				com++;
				window_display = 0;
			}
		}
		else
		{
			if (*com == '^')
			{
				com++;
				window_display = 0;
			}
			nalias = get_alias(COMMAND_ALIAS, com, &alias_cnt,
				&alias_name);
		}
		if (nalias && (alias_cnt == 0))
		{
			if (hist_flag && add_to_hist)
			{
				add_to_history(this_cmd);
				set_input(empty_string);
			}
			execute_alias(alias_name, nalias, rest);
			new_free(&alias_name);
		}
		else
		{
			/* History */
			if (*com == '!')
			{
				if ((com = do_history(com + 1, rest)) != NULL)
				{
					if (level == 1)
					{
						set_input(com);
						update_input(UPDATE_ALL);
					}
					else
						parse_command(com, 0, sub_args);
					new_free(&com);
				}
				else
					set_input(empty_string);
			}
			else
			{
				if (hist_flag && add_to_hist)
				{
					add_to_history(this_cmd);
					set_input(empty_string);
				}
				command = find_command(com, &cmd_cnt);
				if ((command && cmd_cnt < 0) || (0 == alias_cnt && 1 == cmd_cnt))
				{
 					if ((command->flags & SERVERREQ) && connected_to_server == 0)
 						say("%s: You are not connected to a server. Use /SERVER to connect.", com);
 					else if (command->func)
						command->func(command->server_func, rest, sub_args);
					else
						say("%s: command disabled", command->name);
				}
				else if (nalias && 1 == alias_cnt && cmd_cnt == 1 && !strcmp(alias_name, command[0].name))
					execute_alias(alias_name, nalias, rest);
				else if ((alias_cnt + cmd_cnt) > 1)
					say("Ambiguous command: %s", com);
				else if (nalias && 1 == alias_cnt)
					execute_alias(alias_name, nalias, rest);
				else if (!my_stricmp(com, nickname))
						/* nick = /me  -lynx */
					me(NULL, rest, empty_string);
				else
					say("Unknown command: %s", com);
			}
			if (nalias)
				new_free(&alias_name);
		}
		if (old_display_var != get_int_var(DISPLAY_VAR))
			window_display = get_int_var(DISPLAY_VAR);
		else
			window_display = display;
	}
	new_free(&this_cmd);
	level--;
}

/*
 * load: the /LOAD command.  Reads the named file, parsing each line as
 * though it were typed in (passes each line to parse_command). 
 */
/*ARGSUSED*/
void
load(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	FILE	*fp;
	char	*filename,
		*expanded = NULL;
	int	flag = 0;
	struct	stat	stat_buf;
	int	paste_level = 0;
	char	*start,
		*current_row = NULL,
 		lbuf[BIG_BUFFER_SIZE + 1];
	int	no_semicolon = 1;
	char	*ircpath;
	int	display;
#ifdef ZCAT
	char	*expand_z = NULL;
	int	exists;
	int	pos;
#endif
/**************************** PATCHED by Flier ******************************/
        int     linenumber=0;
        int     pasteline=-1;
/****************************************************************************/

 	ircpath = get_string_var(LOAD_PATH_VAR);
 	if (!ircpath)
	{
		say("LOAD_PATH has not been set");
		return;
	}

	if (load_depth == MAX_LOAD_DEPTH)
	{
		say("No more than %d levels of LOADs allowed", MAX_LOAD_DEPTH);
		return;
	}
	load_depth++;
	status_update(0);
#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	{
		say("You may only load your SAVED file");
		filename = ircrc_file;
	}
	else
#endif /* DAEMON_UID */
		while ((filename = next_arg(args, &args)) != NULL)
		{
			if (my_strnicmp(filename, "-args", strlen(filename)) == 0)
				flag = 1;
			else
				break;
		}
	if (filename)
	{
		if ((expanded = expand_twiddle(filename)) != NULL)
		{
#ifdef ZCAT
			/* Handle both <expanded> and <expanded>.Z */
			pos = strlen(expanded) - strlen(ZSUFFIX);
			if (pos < 0 || strcmp(expanded + pos, ZSUFFIX))
			{
				malloc_strcpy(&expand_z, expanded);
				malloc_strcat(&expand_z, ZSUFFIX);
			}
#endif /*ZCAT*/
			if (*expanded != '/')
			{
				filename = path_search(expanded, ircpath);
#ifdef ZCAT
				if (!filename && expand_z)
					filename = path_search(expand_z, ircpath);
#endif /*ZCAT*/
				if (!filename)
				{
					say("%s: File not found", expanded);
					status_update(1);
					load_depth--;
#ifdef ZCAT
					new_free(&expand_z);
#endif /* ZCAT */
					new_free(&expanded);
					return;
				}
				else
					malloc_strcpy(&expanded, filename);
			}
#ifdef ZCAT
			if ((exists = stat_file(expanded, &stat_buf)) == -1)
 			{
				if (!(exists = stat_file(expand_z, &stat_buf)))
				{
					if (expanded)
						new_free(&expanded);
					expanded = expand_z;
				}
				else
					new_free(&expand_z);
 			}
			if (exists == 0)
#else
				if (!stat_file(expanded, &stat_buf))
#endif /*ZCAT*/
				{
					if (stat_buf.st_mode & S_IFDIR)
					{
						say("%s is a directory", expanded);
						status_update(1);
						load_depth--;
#ifdef ZCAT
						new_free(&expand_z);
#endif /* ZCAT */
						new_free(&expanded);
						return;
					}
					/* sigh.  this is lame */
#if defined(S_IXUSR) && defined(S_IXGRP) && defined(S_IXOTH)
# define IS_EXECUTABLE (S_IXUSR|S_IXGRP|S_IXOTH)
#else
# define IS_EXECUTABLE 0111
#endif
					if (stat_buf.st_mode & IS_EXECUTABLE)
					{
						yell("*** %s is executable and may not be loaded", expanded);
						status_update(1);
						load_depth--;
#ifdef ZCAT
						new_free(&expand_z);
#endif /* ZCAT */
						new_free(&expanded);
						return;
					}
				}
			if (command && *command == 'W')
			{
				say("%s", expanded);
				status_update(1);
				load_depth--;
				new_free(&expanded);
#ifdef ZCAT
				new_free(&expand_z);
#endif /* ZCAT */
				return;
			}
#ifdef ZCAT
			/* Open if uncompressed, zcat if compressed */
			pos = strlen(expanded) - strlen(ZSUFFIX);
			if (pos >= 0 && !strcmp(expanded + pos, ZSUFFIX))
				fp = zcat(expanded);
			else
				fp = fopen(expanded, "r");
			if (fp != NULL)
#else
				if (fp = fopen(expanded, "r"))
#endif /*ZCAT*/
				{
					display = window_display;
					window_display = 0;
					current_row = NULL;
					if (!flag)
						args = NULL;
					for (;;)
					{
						if (fgets(lbuf, BIG_BUFFER_SIZE / 2, fp))	/* XXX need better /load policy, but this will do for now */
	{
 		size_t	len;
		char	*ptr;

/**************************** PATCHED by Flier ******************************/
                linenumber++;
/****************************************************************************/
 		for (start = lbuf; isspace(*start); start++)
			;
		if (!*start || *start == '#')
			continue;

		len = strlen(start);
	/*
	 * this line from stargazer to allow \'s in scripts for continued
	 * lines <spz@specklec.mpifr-bonn.mpg.de>
	 */
		while (start[len-1] == '\n' && start[len-2] == '\\' &&
		    len < BIG_BUFFER_SIZE / 2 && fgets(&(start[len-2]),
 		    (int)(BIG_BUFFER_SIZE / 2 - len), fp))
			len = strlen(start);

		if (start[len - 1] == '\n')
		    start[--len] = '\0';

		while (start && *start)
		{
			char	*optr = start;
			while ((ptr = sindex(optr, "{};")) &&
					ptr != optr &&
					ptr[-1] == '\\')
				optr = ptr+1;

			if (no_semicolon)
				no_semicolon = 0;
			else if ((!ptr || ptr != start) && current_row)
			{
				if (!paste_level)
				{
					parse_line(NULL, current_row,
						args, 0, 0);
					new_free(&current_row);
				}
				else
					malloc_strcat(&current_row, ";");
			}

			if (ptr)
			{
				char	c = *ptr;

				*ptr = '\0';
				malloc_strcat(&current_row, start);
				*ptr = c;

				switch (c)
				{
				case '{' :
/**************************** PATCHED by Flier ******************************/
                                        if (!paste_level) pasteline=linenumber;
/****************************************************************************/
					paste_level++;
					if (ptr == start)
						malloc_strcat(&current_row, " {");
					else
						malloc_strcat(&current_row, "{");
					no_semicolon = 1;
					break;

				case '}' :
					if (!paste_level)
/**************************** PATCHED by Flier ******************************/
						/*yell("Unexpected }");*/
                                                yell("Unexpected } in %s, line %d",
                                                     expanded,linenumber);
/****************************************************************************/
					else
					{
						--paste_level;
						malloc_strcat(&current_row, "}");
						no_semicolon = ptr[1] ? 1 : 0;
					}
/**************************** PATCHED by Flier ******************************/
                                        if (!paste_level) pasteline=-1;
/****************************************************************************/
					break;

				case ';' :
					malloc_strcat(&current_row, ";");
					no_semicolon = 1;
					break;
				}

				start = ptr+1;
			}
			else
			{
				malloc_strcat(&current_row, start);
				start = NULL;
			}
		}
	}
						else
							break;
					}
					if (current_row)
					{
						if (paste_level)
/**************************** PATCHED by Flier ******************************/
							/*yell("Unexpected EOF");*/
                                                        yell("Unexpected EOF in %s trying to match '{' at line %d",
                                                             expanded,pasteline);
/****************************************************************************/
						else
							parse_line(NULL,
								current_row, 
								args, 0, 0);
						new_free(&current_row);
					}
					window_display = display;
					fclose(fp);
				}
				else
					say("Couldn't open %s: %s", expanded,
						strerror(errno));
			new_free(&expanded);
#ifdef ZCAT
			new_free(&expand_z);
#endif /* ZCAT */
		}
		else
			say("Unknown user");
	}
	else
		say("No filename specified");
	status_update(1);
	load_depth--;
}

/*
 * get_history: gets the next history entry, either the PREV entry or the
 * NEXT entry, and sets it to the current input string 
 */
static void	
get_history(which)
	int	which;
{
	char	*ptr;

	if ((ptr = get_from_history(which)) != NULL)
	{
		set_input(ptr);
		update_input(UPDATE_ALL);
	}
}

/* BIND function: */
void
forward_character(key, ptr)
 	u_int	key;
	char *	ptr;
{
	input_move_cursor(RIGHT);
}

void
backward_character(key, ptr)
 	u_int	key;
	char *	ptr;
{
	input_move_cursor(LEFT);
}

void
backward_history(key, ptr)
 	u_int	key;
	char *	ptr;
{
	get_history(PREV);
}

void
forward_history(key, ptr)
 	u_int	key;
	char *	ptr;
{
	get_history(NEXT);
}

void
toggle_insert_mode(key, ptr)
 	u_int	key;
	char *	ptr;
{
/**************************** PATCHED by Flier ******************************/
	/*set_var_value(INSERT_MODE_VAR, "TOGGLE");*/
    HandleTabNext();
/****************************************************************************/
}

/*ARGSUSED*/
void
send_line(key, ptr)
 	u_int	key;
	char *	ptr;
{
	int	server;
	WaitPrompt	*OldPrompt;

	server = from_server;
	from_server = get_window_server(0);
	reset_hold((Window *) 0);
	hold_mode((Window *) 0, OFF, 1);
	if (current_screen->promptlist && current_screen->promptlist->type == WAIT_PROMPT_LINE)
	{
		OldPrompt = current_screen->promptlist;
		(*OldPrompt->func)(OldPrompt->data, get_input());
		set_input(empty_string);
		current_screen->promptlist = OldPrompt->next;
		new_free(&OldPrompt->data);
		new_free(&OldPrompt->prompt);
		new_free(&OldPrompt);
		change_input_prompt(-1);
	}
	else
	{
		char	*line,
			*tmp = NULL;

		line = get_input();
		malloc_strcpy(&tmp, line);

		if (do_hook(INPUT_LIST, "%s", tmp))
		{
			if (get_int_var(INPUT_ALIASES_VAR))
				parse_line(NULL, tmp, empty_string,
					1, 0);
			else
				parse_line(NULL, tmp, NULL,
					1, 0);
		}
		update_input(UPDATE_ALL);
		new_free(&tmp);
	}
	from_server = server;
/**************************** PATCHED by Flier ******************************/
        tabnickcompl=NULL;
/****************************************************************************/
}

/* The SENDLINE command.. */
#ifndef LITE
static	void
sendlinecmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	server;
	int	display;

	server = from_server;
	display = window_display;
	window_display = 1;
	if (get_int_var(INPUT_ALIASES_VAR))
		parse_line(NULL, args, empty_string, 1, 0);
	else
		parse_line(NULL, args, NULL, 1, 0);
	update_input(UPDATE_ALL);
	window_display = display;
	from_server = server;
}
#endif

/*ARGSUSED*/
#ifndef LITE
void
meta8_char(key, ptr)
	u_int	key;
	char *	ptr;
{
	current_screen->meta8_hit = 1;
}

/*ARGSUSED*/
void
meta7_char(key, ptr)
	u_int	key;
	char *	ptr;
{
	current_screen->meta7_hit = 1;
}

/*ARGSUSED*/
void
meta6_char(key, ptr)
	u_int	key;
	char *	ptr;
{
	current_screen->meta6_hit = 1;
}
#endif /* LITE */

/*ARGSUSED*/
void
meta5_char(key, ptr)
	u_int	key;
	char *	ptr;
{
	current_screen->meta5_hit = 1;
}

/*ARGSUSED*/
void
meta4_char(key, ptr)
 	u_int	key;
	char *	ptr;
{
	current_screen->meta4_hit = 1 - current_screen->meta4_hit;
}

/*ARGSUSED*/
void
meta3_char(key, ptr)
 	u_int	key;
	char *	ptr;
{
	current_screen->meta3_hit = 1;
}

/*ARGSUSED*/
void
meta2_char(key, ptr)
 	u_int	key;
	char *	ptr;
{
	current_screen->meta2_hit = 1;
}

/*ARGSUSED*/
void
meta1_char(key, ptr)
 	u_int	key;
	char *	ptr;
{
	current_screen->meta1_hit = 1;
}

void
quote_char(key, ptr)
 	u_int	key;
	char *	ptr;
{
	current_screen->quote_hit = 1;
}

/* type_text: the BIND function TYPE_TEXT */
/*ARGSUSED*/
void
type_text(key, ptr)
 	u_int	key;
	char	*ptr;
{
	for (; *ptr; ptr++)
 		input_add_character((u_int)*ptr, (char *) 0);
}

/*
 * irc_clear_screen: the CLEAR_SCREEN function for BIND.  Clears the screen and
 * starts it if it is held 
 */
/*ARGSUSED*/
void
irc_clear_screen(key, ptr)
 	u_int	key;
	char	*ptr;
{
	hold_mode((Window *) 0, OFF, 1);
	my_clear(NULL, empty_string, empty_string);
}

/* parse_text: the bindable function that executes its string */
void
parse_text(key, ptr)
 	u_int	key;
	char	*ptr;
{
	parse_line(NULL, ptr, empty_string, 0, 0);
}

/*
 * edit_char: handles each character for an input stream.  Not too difficult
 * to work out.
 */
void
edit_char(ikey)
 	u_int ikey;
{
 	void	(*func) _((u_int, char *));
	char	*str;
 	u_char	extended_key, key = (u_char)ikey;
	WaitPrompt *oldprompt;
/**************************** PATCHED by Flier ******************************/
        static int cloaktilde=0;
/****************************************************************************/

        if (current_screen->promptlist &&
			current_screen->promptlist->type == WAIT_PROMPT_KEY)
	{
		oldprompt = current_screen->promptlist;
		(*oldprompt->func)(oldprompt->data, (char *)&key);
		set_input(empty_string);
		current_screen->promptlist = oldprompt->next;
		new_free(&oldprompt->data);
		new_free(&oldprompt->prompt);
		new_free(&oldprompt);
		change_input_prompt(-1);
		return;
	}
	if (!get_int_var(EIGHT_BIT_CHARACTERS_VAR))
		key &= 0x7f;			/* mask out non-ascii crap */

	if (translation)
		extended_key = transFromClient[key];
	else
		extended_key = key;

	if (current_screen->meta1_hit)
	{
		func = key_names[meta1_keys[key].index].func;
		str = meta1_keys[key].stuff;
		current_screen->meta1_hit = 0;
	}
	else if (current_screen->meta2_hit)
	{
		func = key_names[meta2_keys[key].index].func;
		str = meta2_keys[key].stuff;
		current_screen->meta2_hit = 0;
	}
	else if (current_screen->meta3_hit)
	{
		func = key_names[meta3_keys[key].index].func;
		str = meta3_keys[key].stuff;
		current_screen->meta3_hit = 0;
	}
	else if (current_screen->meta4_hit)
	{
		func = key_names[meta4_keys[key].index].func;
		str = meta4_keys[key].stuff;
	}
	else if (current_screen->meta5_hit)
	{
		func = key_names[meta5_keys[key].index].func;
		str = meta5_keys[key].stuff;
/**************************** PATCHED by Flier ******************************/
		current_screen->meta5_hit=0;
/****************************************************************************/
	}
#ifndef LITE
	else if (current_screen->meta6_hit)
	{
		func = key_names[meta6_keys[key].index].func;
		str = meta6_keys[key].stuff;
	}
	else if (current_screen->meta7_hit)
	{
		func = key_names[meta7_keys[key].index].func;
		str = meta7_keys[key].stuff;
	}
	else if (current_screen->meta8_hit)
	{
		func = key_names[meta8_keys[key].index].func;
		str = meta8_keys[key].stuff;
	}
#endif
	else
	{
		func = key_names[keys[key].index].func;
		str = keys[key].stuff;
        }
/**************************** PATCHED by Flier ******************************/
	/*if (!current_screen->meta1_hit && !current_screen->meta2_hit &&
			!current_screen->meta3_hit)*/
	if (!current_screen->meta1_hit && !current_screen->meta2_hit &&
			!current_screen->meta3_hit && !current_screen->meta5_hit)
/****************************************************************************/
	{
#ifndef LITE
		if (current_screen->inside_menu == 1)
 			menu_key((u_int)key);
                else
#endif
		if (current_screen->digraph_hit)
		{
			if (extended_key == 0x08 || extended_key == 0x7f)
				current_screen->digraph_hit = 0;
			else if (current_screen->digraph_hit == 1)
			{
				current_screen->digraph_first = extended_key;
				current_screen->digraph_hit = 2;
			}
			else if (current_screen->digraph_hit == 2)
			{
				if ((extended_key =
 				    get_digraph((u_int)extended_key)) != '\0')
 					input_add_character((u_int)extended_key, (char *) 0);
				else
					term_beep();
			}
		}
		else if (current_screen->quote_hit)
		{
			current_screen->quote_hit = 0;
 			input_add_character((u_int)extended_key, (char *) 0);
		}
/**************************** PATCHED by Flier ******************************/
		/*else if (func)
			func(extended_key, str ? str : empty_string);*/
                else if (func) {
                    if (key==27) cloaktilde=1;
                    else if (cloaktilde==1 && key==91) cloaktilde++;
                    else if (cloaktilde==2 && key==91) cloaktilde=0;
                    else if (cloaktilde==2 && key>=65 && key<=68) cloaktilde=0;
                    else if (cloaktilde==2) cloaktilde++;
                    else if (cloaktilde>=3 && key==126) {
                        cloaktilde=0;
                        return;
                    }
                    else if (cloaktilde==3) cloaktilde++;
                    else cloaktilde=0;
                    func(extended_key, str ? str : empty_string);
                }
/****************************************************************************/
	}
	else
		term_beep();
}

/*ARGSUSED*/
#ifndef LITE
static	void
catter(command, args, subargs)
	char *command;
	char *args;
	char *subargs;
{
	char *target = next_arg(args, &args);

	if (target && args && *args)
	{
		char *text = args;
		FILE *fp = fopen(target, "r+");

		if (!fp)
		{
			fp = fopen(target, "w");
			if (!fp)
			{
				say("CAT: error: '%s': %s", target, strerror(errno));
				return;
		}	}
		
		fseek(fp, 0, SEEK_END);
		fprintf(fp, "%s\n", text),
		fclose(fp);
	}
	else
		say("Usage: /CAT <destfile> <line>");
}
#endif

/*ARGSUSED*/
static	void
cd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	lbuf[BIG_BUFFER_SIZE+1];
	char	*arg,
		*expand;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	{
		say("You are not permitted to use this command");
		return;
	}
#endif /* DAEMON_UID */
	if ((arg = next_arg(args, &args)) != NULL)
	{
		if ((expand = expand_twiddle(arg)) != NULL)
		{
			if (chdir(expand))
				say("CD: %s", strerror(errno));
			new_free(&expand);
		}
		else
			say("CD: No such user");
	}
	getcwd(lbuf, BIG_BUFFER_SIZE);
	say("Current directory: %s", lbuf);
}

static	void
send_action(target, text)
	char	*target, *text;
{
	send_ctcp(ctcp_type[CTCP_PRIVMSG], target, "ACTION", "%s", text);
}

#ifdef LYNX_STUFF
static	char	*
prepare_action(string)
	char	*string;
{
	short	last;
	char	*message;

	last = strlen(string) - 1;
	while(string[last] == ' ')
		if (--last < 0) return NULL;

	if ((string[last] > 'a' && string[last] < 'z') ||
			(string[last] > 'A' && string[last] < 'Z'))
	{
		message = new_malloc(last + 2);
		strmcpy (message, string, last+1);
		message[last + 1] = '.';
		message[last + 2] = '\0';
		return message;
	}
	else
		return NULL;
}
#endif

/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
describe(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*target;
/**************************** PATCHED by Flier ******************************/
        char    thing;
        char    *curchan;
#ifdef WANTANSI
        char    tmpbuf[mybufsize/2];
#endif

        if (get_int_var(HIGH_ASCII_VAR)) thing='ì';
        else thing='*';
/****************************************************************************/
	target = next_arg(args, &args);
	if (target && args && *args)
	{
 		int	old, from_level;
#ifdef LYNX_STUFF
		char	*result;
#endif /* LYNX_STUFF */
		char	*message;

#ifdef LYNX_STUFF
		if (result = prepare_action(args))
			message = result;
		else
#endif /* LYNX_STUFF */
			message = args;

		old = set_lastlog_msg_level(LOG_ACTION);
 		save_message_from();
		from_level = message_from_level(LOG_ACTION);
		send_action(target, message);
		if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
/**************************** PATCHED by Flier ******************************/
			/*put_it("* -> %s: %s %s", target,
				get_server_nickname(from_server), message);*/
                {
                    char *stampbuf=TimeStamp(2);

                    if ((curchan=get_channel_by_refnum(0)) && !my_stricmp(curchan,target)) {
#ifdef WANTANSI
                        char *tmpstr=(char *) 0;
                    
                        malloc_strcpy(&tmpstr,stampbuf);
                        malloc_strcat(&tmpstr,CmdsColors[COLME].color1);
                        put_it("%s%c%s %s%s%s %s%s%s",
                               tmpstr,thing,Colors[COLOFF],
                               CmdsColors[COLME].color2,get_server_nickname(from_server),Colors[COLOFF],
                               CmdsColors[COLME].color5,message,Colors[COLOFF]);
                        new_free(&tmpstr);
#else
                        put_it("%s%c%c%c %s %s",stampbuf,bold,thing,bold,
                               get_server_nickname(from_server),message);
#endif
                    }
                    else {
#ifdef WANTANSI
                        sprintf(tmpbuf,"<%s%s%s> %s%c%s %s%s%s",
                               CmdsColors[COLME].color4,target,Colors[COLOFF],
                               CmdsColors[COLME].color1,thing,Colors[COLOFF],
                               CmdsColors[COLME].color2,get_server_nickname(from_server),Colors[COLOFF]);
                        put_it("%s%s %s%s%s",stampbuf,tmpbuf,
                              CmdsColors[COLME].color5,message,Colors[COLOFF]);
#else
                        put_it("%s<%s> %c%c%c %s %s",stampbuf,target,bold,thing,bold,
                               get_server_nickname(from_server),message);
#endif
                    }
                }
/****************************************************************************/
		set_lastlog_msg_level(old);
 		restore_message_from();

#ifdef LYNX_STUFF
		if (result)
			new_free(&result);
#endif
	}
	else
		say("Usage: /DESCRIBE <target> <action description>");
}

/*
 * New 'me' command - now automatically appends period.
 * Necessary for new 'action' script.   -lynx'92
 * Hardly, removed this as it was generally considered offensive
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
me(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
/**************************** PATCHED by Flier ******************************/
        char thing;

        if (get_int_var(HIGH_ASCII_VAR)) thing='ì';
        else thing='*';
/****************************************************************************/
        if (args && *args)
	{
		char	*target;

		if ((target = get_target_by_refnum(0)) != NULL)
		{
			int	old;
#ifdef LYNX_STUFF
			char	*result;
#endif /* LYNX_STUFF */
			char	*message;

			/* handle "/ foo" */
			if (!strncmp(target, get_string_var(CMDCHARS_VAR), 1) &&
			    (!(target = get_channel_by_refnum(0))))
			{
				say("No target, neither channel nor query");
				return;
			}
#ifdef LYNX_STUFF
			if (result = prepare_action(args))
				message = result;
			else
#endif /* LYNX_STUFF */
				message = args;

			old = set_lastlog_msg_level(LOG_ACTION);
 			save_message_from();
			message_from(target, LOG_ACTION);
			send_action(target, message);
			if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
/**************************** PATCHED by Flier ******************************/
				/*put_it("* %s %s",
				    get_server_nickname(from_server), message);*/
                        {
#ifdef WANTANSI
                            char *tmpstr=(char *) 0;
#endif
                            char *stampbuf=TimeStamp(2);

#ifdef WANTANSI
                            malloc_strcpy(&tmpstr,stampbuf);
                            malloc_strcat(&tmpstr,CmdsColors[COLME].color1);
                            put_it("%s%c%s %s%s%s %s%s%s",
                                    tmpstr,thing,Colors[COLOFF],
                                    CmdsColors[COLME].color2,get_server_nickname(from_server),Colors[COLOFF],
                                    CmdsColors[COLME].color5,message,Colors[COLOFF]);
                            new_free(&tmpstr);
#else
                            put_it("%s%c%c%c %s %s",stampbuf,bold,thing,bold,
                                    get_server_nickname(from_server), message);
#endif
                        }
/****************************************************************************/
			set_lastlog_msg_level(old);
 			restore_message_from();

#ifdef LYNX_STUFF
			if (result)
				new_free(&result);
#endif
		}
		else
			say("No target, neither channel nor query");
	}
	else
		say("Usage: /ME <action description>");
}

#ifndef LITE
static	void
mload(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*file;

	while ((file = next_arg(args, &args)) != NULL)
		load_menu(file);
}

static	void
mlist(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*menu;

	while ((menu = new_next_arg(args, &args)) != NULL)
		(void) ShowMenu(menu);
}
#endif /* LITE */

static	void
evalcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	parse_line(NULL, args, subargs ? subargs : empty_string, 0, 0);
}

/*
 * execute_timer:  checks to see if any currently pending timers have
 * gone off, and if so, execute them, delete them, etc, setting the
 * current_exec_timer, so that we can't remove the timer while its
 * still executing.
 */
extern	void
execute_timer()
{
/**************************** PATCHED by Flier ******************************/
	/*time_t	current;*/
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        struct  timeval current;
#else
        time_t	current;
#endif
/****************************************************************************/
	TimerList *next;
	int	old_in_on_who,
 		old_server = from_server;

/**************************** PATCHED by Flier ******************************/
	/*time(&current);
	while (PendingTimers && PendingTimers->time <= current)*/
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        gettimeofday(&current,NULL);
        while (PendingTimers && (PendingTimers->time.tv_sec<current.tv_sec ||
                                (PendingTimers->time.tv_sec==current.tv_sec &&
                                 PendingTimers->time.tv_usec<=current.tv_usec)))
#else
        time(&current);
	while (PendingTimers && PendingTimers->time <= current)
#endif
/****************************************************************************/
	{
		old_in_on_who = in_on_who;
		in_on_who = PendingTimers->in_on_who;
		current_exec_timer = PendingTimers->ref;
 		save_message_from();
 		(void)message_from_level(LOG_CURRENT);
		if (PendingTimers->server >= 0)
			from_server = PendingTimers->server;
/**************************** PATCHED by Flier ******************************/
                /*parse_command(PendingTimers->command, 0, empty_string);*/
                if (PendingTimers->func) PendingTimers->func(PendingTimers->command);
                else parse_command(PendingTimers->command,0,empty_string);
/****************************************************************************/
                from_server = old_server;
		restore_message_from();
		current_exec_timer = -1;
		new_free(&PendingTimers->command);
		next = PendingTimers->next;
		new_free(&PendingTimers);
		PendingTimers = next;
		in_on_who = old_in_on_who;
	}
}

/*
 * timercmd: the bit that handles the TIMER command.  If there are no
 * arguements, then just list the currently pending timers, if we are
 * give a -DELETE flag, attempt to delete the timer from the list.  Else
 * consider it to be a timer to add, and add it.
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
timercmd(command, args, subargs)
	char	*command;
	char	*args,
	*subargs;
{
	char	*waittime,
		*flag;
/**************************** PATCHED by Flier ******************************/
	/*time_t	current;*/
        int  count=0;
        int  delaytime=0;
        char *dot=(char *) 0;
        char *tmpstr;
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        char tmpbuf[mybufsize/32];
        struct  timeval current;
#else
        time_t	current;
#endif
        int  visible=1;
/****************************************************************************/
	TimerList	**slot,
			*ntimer;
	int	want = -1,
		refnum;

/**************************** PATCHED by Flier ******************************/
        if (!strcmp(command,"FTIMER")) command="TIMER";
        else subargs=(char *) NULL;
/****************************************************************************/
	if (*args == '-')
	{
 		size_t	len;

		flag = next_arg(args, &args);
		len = strlen(flag);
		upper(flag);

		/* first check for the -DELETE flag */

		if (!strncmp(flag, "-DELETE", len))
		{
			char	*ptr;
			int	ref;
			TimerList	*tmp,
					*prev;

			if (current_exec_timer != -1)
			{
				say("You may not remove a TIMER from itself");
				return;
			}
			if (!(ptr = next_arg(args, &args)))
			{
				say("%s: Need a timer reference number for -DELETE", command);
				return;
			}
			ref = atoi(ptr);
			for (prev = tmp = PendingTimers; tmp; prev = tmp,
					tmp = tmp->next)
			{
				if (tmp->ref == ref)
				{
					if (tmp == prev)
						PendingTimers =
							PendingTimers->next;
					else
						prev->next = tmp->next;
					new_free(&tmp->command);
					new_free(&tmp);
					return;
				}
			}
			say("%s: Can't delete %d, no such refnum",
				command, ref);
			return;
		}
		else if (!strncmp(flag, "-REFNUM", len))
		{
			char	*ptr;

			ptr = next_arg(args, &args);
			want = atoi(ptr);
			if (want < 0)
			{
				say("%s: Illegal refnum %d", command, want);
				return;
			}
		}
/**************************** PATCHED by Flier ******************************/
                else if (!strncmp(flag,"-INVISIBLE",len)) visible=0;
/****************************************************************************/
		else
		{
			say("%s: %s no such flag", command, flag);
			return;
		}
	}

	/* else check to see if we have no args -> list */

	if (!(waittime = next_arg(args, &args)))
	{
		show_timer(command);
		return;
	}

	/* must be something to add */

	if ((refnum = create_timer_ref(want)) == -1)
	{
		say("%s: Refnum %d already exists", command, want);
		return;
	}
/**************************** PATCHED by Flier ******************************/
	/*time(&current);*/
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        gettimeofday(&current,NULL);
#else
        time(&current);
#endif
/****************************************************************************/
	ntimer = (TimerList *) new_malloc(sizeof(TimerList));
	ntimer->in_on_who = in_on_who;
/**************************** PATCHED by Flier ******************************/
	/*ntimer->time = current + atol(waittime);*/
        for (tmpstr=waittime;*tmpstr && !dot;tmpstr++) {
            if (isdigit(*tmpstr))
                count=10*count+(*tmpstr)-'0';
            else
                switch (*tmpstr) {
                    case '.':
                    case 's':
                        delaytime+=count;
                        count=0;
                        dot=tmpstr;
                        break;
                    case 'm':
                        count*=60;
                        delaytime+=count;
                        count=0;
                        break;
                    case 'h':
                        count*=3600;
                        delaytime+=count;
                        count=0;
                        break;
                    case 'd':
                        count*=86400;
                        delaytime+=count;
                        count=0;
                        break;
                }
        }
        if (count>0) delaytime+=count;
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        ntimer->time.tv_sec=current.tv_sec+delaytime;
        ntimer->time.tv_usec=current.tv_usec;
        if ((dot=index(waittime,'.'))) {
            dot++;
            strcpy(tmpbuf,dot);
            dot=tmpbuf;
            tmpstr=dot;
            count=0;
            while (*tmpstr && count<6) {
                if (isdigit(*tmpstr)) {
                    tmpstr++;
                    count++;
                }
            }
            while (count<6) {
                *tmpstr='0';
                tmpstr++;
                count++;
            }
            *tmpstr='\0';
            ntimer->time.tv_usec+=atol(dot);
        }
        if (ntimer->time.tv_usec>1000000) {
            ntimer->time.tv_sec++;
            ntimer->time.tv_usec-=1000000;
        }
#else
	ntimer->time=current+delaytime;
#endif
        ntimer->visible=visible;
        if (subargs) ntimer->func=(void *) subargs;
        else ntimer->func=NULL;
/****************************************************************************/
 	ntimer->server = from_server;
	ntimer->ref = refnum;
	ntimer->command = NULL;
	malloc_strcpy(&ntimer->command, args);

	/* we've created it, now put it in order */

	for (slot = &PendingTimers; *slot; slot = &(*slot)->next)
	{
/**************************** PATCHED by Flier ******************************/
		/*if ((*slot)->time > ntimer->time)*/
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
                if ((float) ((*slot)->time.tv_sec)+(*slot)->time.tv_usec/1000000.0 >
                    (float) (ntimer->time.tv_sec)+ntimer->time.tv_usec/1000000.0)
#else
		if ((*slot)->time > ntimer->time)
#endif
/****************************************************************************/
			break;
	}
	ntimer->next = *slot;
	*slot = ntimer;
}

/*
 * show_timer:  Display a list of all the TIMER commands that are
 * pending to be executed.
 */
static	void
show_timer(command)
	char	*command;
{
	TimerList	*tmp;
/**************************** PATCHED by Flier ******************************/
	/*time_t	current,
		time_left;*/
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        struct  timeval current;
        float   time_left;
#else
        time_t	current,
        time_left;
#endif
/****************************************************************************/

	if (!PendingTimers)
	{
		say("%s: No commands pending to be executed", command);
		return;
	}

/**************************** PATCHED by Flier ******************************/
	/*time(&current);*/
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
        gettimeofday(&current,NULL);
#else
        time(&current);
#endif
        /*say("Timer Seconds   Command");*/
	say("Timer Seconds    Command");
/****************************************************************************/
	for (tmp = PendingTimers; tmp; tmp = tmp->next)
	{
/**************************** PATCHED by Flier ******************************/
		/*time_left = tmp->time - current;
		if (time_left < 0)
			time_left = 0;
		say("%-5d %-10d %s", tmp->ref, time_left, tmp->command);*/
                if (!(tmp->visible)) continue;
#if defined(HAVETIMEOFDAY) && defined(BETTERTIMER)
                time_left=(float) (tmp->time.tv_sec)+tmp->time.tv_usec/1000000.0 -
                          ((float) (current.tv_sec)+current.tv_usec/1000000.0);
                if (time_left < 0.0)
			time_left = 0.0;
		say("%-5d %-10.3f %s", tmp->ref, time_left, tmp->command);
#else
                time_left = tmp->time - current;
		if (time_left < 0)
			time_left = 0;
		say("%-5d %-10d %s", tmp->ref, time_left, tmp->command);
#endif
/****************************************************************************/
	}
}

/*
 * create_timer_ref:  returns the lowest unused reference number for
 * a timer
 */
static	int
create_timer_ref(want)
	int	want;
{
	TimerList	*tmp;
	int	ref = 0;
	int	done = 0;

	if (want == -1)
		while (!done)
		{
			done = 1;
			for (tmp = PendingTimers; tmp; tmp = tmp->next)
				if (ref == tmp->ref)
				{
					ref++;
					done = 0;
					break;
				}
		}
	else
	{
		ref = want;
		for (tmp = PendingTimers; tmp; tmp = tmp->next)
			if (ref == tmp->ref)
			{
				ref = -1;
				break;
			}
	}

	return (ref);
}

/**************************** PATCHED by Flier ******************************/
/* Clean up all memory used by timers */
void CleanUpTimer() {
    TimerList *tmptimer;

    while (PendingTimers) {
        tmptimer=PendingTimers;
        PendingTimers=PendingTimers->next;
        new_free(&(tmptimer->command));
        new_free(&tmptimer);
    }
}
/****************************************************************************/

/*
 * inputcmd:  the INPUT command.   Takes a couple of arguements...
 * the first surrounded in double quotes, and the rest makes up
 * a normal ircII command.  The command is evalutated, with $*
 * being the line that you input.  Used add_wait_prompt() to prompt
 * the user...  -phone, jan 1993.
 */

#ifndef LITE
static	void
inputcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*prompt;

	if (!args || !*args)
		return;
	
	if (*args++ != '"')
	{
		say("Need \" to begin prompt for INPUT");
		return;
	}

	prompt = args;
	if ((args = index(prompt, '"')) != NULL)
		*args++ = '\0';
	else
	{
		say("Missing \" in INPUT");
		return;
	}

	for (; *args == ' '; args++)
		;

	add_wait_prompt(prompt, eval_inputlist, args, WAIT_PROMPT_LINE);
}
#endif

/*
 * eval_inputlist:  Cute little wrapper that calls parse_line() when we
 * get an input prompt ..
 */

void
eval_inputlist(args, line)
	char	*args,
		*line;
{
	parse_line(NULL, args, line ? line : empty_string, 0, 0);
}

/* pingcmd: ctcp ping, duh - phone, jan 1993. */
static	void
pingcmd(command, args, subargs)
	char    *command,
		*args,
		*subargs;
{
	char	buffer[BIG_BUFFER_SIZE+1];

/**************************** PATCHED by Flier ******************************/
	/*sprintf(buffer, "%s PING %ld", args, (long)time(NULL));*/
        char *target;
#ifdef  HAVETIMEOFDAY
        struct timeval  timeofday;
#endif
    
        if (args && *args) target=new_next_arg(args,&args);
        else target=get_channel_by_refnum(0);
        if (!target) {
            NoWindowChannel();
            return;
        }
#ifdef  HAVETIMEOFDAY
        gettimeofday(&timeofday,NULL);
        sprintf(buffer, "%s PING %ld %ld", target,
                (long) timeofday.tv_sec,(long) timeofday.tv_usec);
#else
        sprintf(buffer, "%s PING %ld",target, (long) time(NULL));
#endif
/****************************************************************************/
	ctcp(command, buffer, empty_string);
}

#ifndef LITE
static	void
xtypecmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
 	size_t	len;

	if (args && *args == '-')
	{
		args++;
		if ((arg = next_arg(args, &args)) != NULL)
		{
			len = strlen(arg);
			if (!my_strnicmp(arg, "LITERAL", len))
			{
				for (; *args; args++)
 					input_add_character((u_int)*args, (char *) 0);
			}
#ifdef _Windows
			else if (!my_strnicmp(arg, "REPLACE", len))
			{
				set_input(args);
				term_resetall();
			}
#endif /* _Windows */
			else
				say ("Unknown flag -%s to XTYPE", arg);
			return;
		}
		input_add_character('-', (char *) 0);
	}
	else
		type(command, args, empty_string);
	return;
}

static	void
beepcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	term_beep();
}
#endif /* LITE */
