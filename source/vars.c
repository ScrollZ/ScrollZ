/*
 * vars.c: All the dealing of the irc variables are handled here. 
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
 * $Id: vars.c,v 1.24 2003-04-29 15:43:36 f Exp $
 */

#include "irc.h"

#include "status.h"
#include "window.h"
#include "lastlog.h"
#include "log.h"
#include "crypt.h"
#include "history.h"
#include "notify.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "whois.h"
#include "translat.h"
#include "ircterm.h"
#include "output.h"
#include "server.h"

/**************************** PATCHED by Flier ******************************/
#include "alias.h"
#include "myvars.h"

extern char *HelpPathVar;
extern char *TimeStampString;
extern time_t LastTS;
/****************************************************************************/

/* IrcVariable: structure for each variable in the variable table */
typedef struct
{
	char	*name;			/* what the user types */
	int	type;			/* variable types, see below */
	int	integer;		/* int value of variable */
	char	*string;		/* string value of variable */
 	void	(*func) ();		/* function to do every time variable is set */
	char	int_flags;		/* internal flags to the variable */
	unsigned short	flags;		/* flags for this variable */
}	IrcVariable;

#define	VF_NODAEMON	0x0001
#define VF_EXPAND_PATH	0x0002

#define VIF_CHANGED	0x01
#define VIF_GLOBAL	0x02

/* the types of IrcVariables */
#define BOOL_TYPE_VAR 0
#define CHAR_TYPE_VAR 1
#define INT_TYPE_VAR 2
#define STR_TYPE_VAR 3

char	*var_settings[] =
{
	"OFF", "ON", "TOGGLE"
};

/* For the NOVICE variable. Complain loudlly if turned off manually.  */
extern	int	load_depth;

extern  Screen	*screen_list, *current_screen;

	int	loading_global = 0;

static	int	find_variable _((char *, int *));
static	void	exec_warning _((int));
static	void	input_warning _((int));
static	void	eight_bit_characters _((int));
static	void	set_realname _((char *));

/**************************** PATCHED by Flier ******************************/
static  void    SetScrollZstr _((char *));
static  void    SetMaxModes _((int));
static  void    SetMaxWallopNicks _((int));
static  void    SetDCCBlockSize _((int));
static  void    SetDCCPorts _((char *));
static	void	Cnotifystring _((char *));
static  void    SetAwayFile _((char *));
void    SetStampFormat _((char *));

extern  void    RedrawAll _((void));

extern  int     DCCLowPort;
extern  int     DCCHighPort;
extern	char	*CelerityNtfy;
/****************************************************************************/

#ifdef _Windows
extern	char *get_ini_value(char *pchEntry);
#endif /* _Windows */

/*
 * irc_variable: all the irc variables used.  Note that the integer and
 * boolean defaults are set here, which the string default value are set in
 * the init_variables() procedure 
 */
static	IrcVariable irc_variable[] =
{
	{ "ALWAYS_SPLIT_BIGGEST",	BOOL_TYPE_VAR,	DEFAULT_ALWAYS_SPLIT_BIGGEST, NULL, NULL, 0, 0 },
/**************************** PATCHED by Flier ******************************/
	{ "AUTO_RECONNECT",		BOOL_TYPE_VAR,	DEFAULT_AUTO_RECONNECT, NULL, NULL, 0, 0 },
/****************************************************************************/
	{ "AUTO_UNMARK_AWAY",		BOOL_TYPE_VAR,	DEFAULT_AUTO_UNMARK_AWAY, NULL, NULL, 0, 0 },
	{ "AUTO_WHOWAS",		BOOL_TYPE_VAR,	DEFAULT_AUTO_WHOWAS, NULL, NULL, 0, 0 },
/**************************** PATCHED by Flier ******************************/
	{ "AWAY_FILE",			STR_TYPE_VAR,	0, NULL, SetAwayFile, 0, 0 },
/****************************************************************************/
	{ "BEEP",			BOOL_TYPE_VAR,	DEFAULT_BEEP, NULL, NULL, 0, 0 },
	{ "BEEP_MAX",			INT_TYPE_VAR,	DEFAULT_BEEP_MAX, NULL, NULL, 0, 0 },
/**************************** PATCHED by Flier ******************************/
	{ "BEEP_ON_MAIL",		BOOL_TYPE_VAR,	DEFAULT_BEEP_ON_MAIL, NULL, NULL, 0, 0 },
/****************************************************************************/
	{ "BEEP_ON_MSG",		STR_TYPE_VAR,	0, NULL, set_beep_on_msg, 0, 0 },
	{ "BEEP_WHEN_AWAY",		INT_TYPE_VAR,	DEFAULT_BEEP_WHEN_AWAY, NULL, NULL, 0, 0 },
	{ "BOLD_VIDEO",			BOOL_TYPE_VAR,	DEFAULT_BOLD_VIDEO, NULL, NULL, 0, 0 },
	{ "CHANNEL_NAME_WIDTH",		INT_TYPE_VAR,	DEFAULT_CHANNEL_NAME_WIDTH, NULL, update_all_status, 0, 0 },
	{ "CLIENT_INFORMATION",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "CLOCK",			BOOL_TYPE_VAR,	DEFAULT_CLOCK, NULL, update_all_status, 0, 0 },
	{ "CLOCK_24HOUR",		BOOL_TYPE_VAR,	DEFAULT_CLOCK_24HOUR, NULL, reset_clock, 0, 0 },
	{ "CLOCK_ALARM",		STR_TYPE_VAR,	0, NULL, set_alarm, 0, 0 },
	{ "CMDCHARS",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "COMMAND_MODE",		BOOL_TYPE_VAR,	DEFAULT_COMMAND_MODE, NULL, NULL, 0, 0 },
	{ "CONTINUED_LINE",		STR_TYPE_VAR,	0, NULL, set_continued_line, 0, 0 },
	{ "CTCP_REPLY_BACKLOG_SECONDS",	INT_TYPE_VAR,	DEFAULT_CTCP_REPLY_BACKLOG_SECONDS, NULL, ctcp_reply_backlog_change, 0, 0 },
	{ "CTCP_REPLY_FLOOD_SIZE",	INT_TYPE_VAR,	DEFAULT_CTCP_REPLY_FLOOD_SIZE_VAR, NULL, NULL, 0, 0 },
	{ "CTCP_REPLY_IGNORE_SECONDS",	INT_TYPE_VAR,	DEFAULT_CTCP_REPLY_IGNORE_SECONDS, NULL, NULL, 0, 0 },
/**************************** PATCHED by Flier ******************************/
/*        { "DCC_BLOCK_SIZE",		INT_TYPE_VAR,	DEFAULT_DCC_BLOCK_SIZE, NULL, NULL, 0, 0 },*/
	{ "DCC_BLOCK_SIZE",		INT_TYPE_VAR,	DEFAULT_DCC_BLOCK_SIZE, NULL, SetDCCBlockSize, 0, 0 },
	{ "DCC_PORTS",		        STR_TYPE_VAR,	0, NULL, SetDCCPorts, 0, 0 },
/****************************************************************************/
	{ "DEBUG",			INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "DECRYPT_PROGRAM",		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "DISPLAY",			BOOL_TYPE_VAR,	DEFAULT_DISPLAY, NULL, NULL, 0, 0 },
/**************************** PATCHED by Flier ******************************/
	{ "DISPLAY_ANSI",		BOOL_TYPE_VAR,	DEFAULT_DISPLAY_ANSI, NULL, RedrawAll, 0, 0 },
/****************************************************************************/
	{ "EIGHT_BIT_CHARACTERS",	BOOL_TYPE_VAR,	DEFAULT_EIGHT_BIT_CHARACTERS, NULL, eight_bit_characters, 0, 0 },
	{ "ENCRYPT_PROGRAM",		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "EXEC_PROTECTION",		BOOL_TYPE_VAR,	DEFAULT_EXEC_PROTECTION, NULL, exec_warning, 0, VF_NODAEMON },
	{ "FLOOD_AFTER",		INT_TYPE_VAR,	DEFAULT_FLOOD_AFTER, NULL, NULL, 0, 0 },
	{ "FLOOD_RATE",			INT_TYPE_VAR,	DEFAULT_FLOOD_RATE, NULL, NULL, 0, 0 },
	{ "FLOOD_USERS",		INT_TYPE_VAR,	DEFAULT_FLOOD_USERS, NULL, NULL, 0, 0 },
	{ "FLOOD_WARNING",		BOOL_TYPE_VAR,	DEFAULT_FLOOD_WARNING, NULL, NULL, 0, 0 },
	{ "FULL_STATUS_LINE",		BOOL_TYPE_VAR,	DEFAULT_FULL_STATUS_LINE, NULL, update_all_status, 0, 0 },
	{ "HELP_PAGER",			BOOL_TYPE_VAR,	DEFAULT_HELP_PAGER, NULL, NULL, 0, 0 },
	{ "HELP_PATH",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_EXPAND_PATH|VF_NODAEMON },
	{ "HELP_PROMPT",		BOOL_TYPE_VAR,	DEFAULT_HELP_PROMPT, NULL, NULL, 0, 0 },
	{ "HELP_WINDOW",		BOOL_TYPE_VAR,	DEFAULT_HELP_WINDOW, NULL, NULL, 0, 0 },
	{ "HIDE_CHANNEL_KEYS",		BOOL_TYPE_VAR,	DEFAULT_HIDE_CHANNEL_KEYS, NULL, update_all_status, 0, 0 },
	{ "HIDE_PRIVATE_CHANNELS",	BOOL_TYPE_VAR,	DEFAULT_HIDE_PRIVATE_CHANNELS, NULL, update_all_status, 0, 0 },
	{ "HIGHLIGHT_CHAR",		STR_TYPE_VAR,	0, NULL, set_highlight_char, 0, 0 },
/**************************** PATCHED by Flier ******************************/
#if defined(__linux__) || defined(FORCE_HASCII)
        { "HIGH_ASCII",		        BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
#else
        { "HIGH_ASCII",		        BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
#endif
/****************************************************************************/
	{ "HISTORY",			INT_TYPE_VAR,	DEFAULT_HISTORY, NULL, set_history_size, 0, VF_NODAEMON },
	{ "HISTORY_FILE",		STR_TYPE_VAR,	0, NULL, set_history_file, 0, 0 },
	{ "HOLD_MODE",			BOOL_TYPE_VAR,	DEFAULT_HOLD_MODE, NULL, reset_line_cnt, 0, 0 },
	{ "HOLD_MODE_MAX",		INT_TYPE_VAR,	DEFAULT_HOLD_MODE_MAX, NULL, NULL, 0, 0 },
	{ "INDENT",			BOOL_TYPE_VAR,	DEFAULT_INDENT, NULL, NULL, 0, 0 },
	{ "INPUT_ALIASES",		BOOL_TYPE_VAR,	DEFAULT_INPUT_ALIASES, NULL, NULL, 0, 0 },
	{ "INPUT_PROMPT",		STR_TYPE_VAR,	0, NULL, set_input_prompt, 0, 0 },
	{ "INPUT_PROTECTION",		BOOL_TYPE_VAR,	DEFAULT_INPUT_PROTECTION, NULL, input_warning, 0, 0 },
	{ "INSERT_MODE",		BOOL_TYPE_VAR,	DEFAULT_INSERT_MODE, NULL, update_all_status, 0, 0 },
	{ "INVERSE_VIDEO",		BOOL_TYPE_VAR,	DEFAULT_INVERSE_VIDEO, NULL, NULL, 0, 0 },
	{ "IRCHOST",			STR_TYPE_VAR,	0, NULL, set_irchost, 0, 0 },
	{ "LASTLOG",			INT_TYPE_VAR,	DEFAULT_LASTLOG, NULL, set_lastlog_size, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "LASTLOG_ANSI",		BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
/****************************************************************************/
	{ "LASTLOG_LEVEL",		STR_TYPE_VAR,	0, NULL, set_lastlog_level, 0, 0 },
	{ "LOAD_PATH",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "LOG",			BOOL_TYPE_VAR,	DEFAULT_LOG, NULL, logger, 0, 0 },
 	{ "LOGFILE",			STR_TYPE_VAR,	0, NULL, set_log_file, 0, VF_NODAEMON },
	{ "MAIL",			INT_TYPE_VAR,	DEFAULT_MAIL, NULL, update_all_status, 0, VF_NODAEMON },
	{ "MAKE_NOTICE_MSG",		BOOL_TYPE_VAR,	DEFAULT_MAKE_NOTICE_MSG, NULL, NULL, 0, 0},
/**************************** PATCHED by Flier ******************************/
        { "MAX_MODES",                  INT_TYPE_VAR,   DEFAULT_MAX_MODES, NULL, SetMaxModes, 0, 0 },
/****************************************************************************/
	{ "MAX_RECURSIONS",		INT_TYPE_VAR,	DEFAULT_MAX_RECURSIONS, NULL, NULL, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "MAX_WALLOP_NICKS",           INT_TYPE_VAR,   DEFAULT_MAX_WALLOP_NICKS, NULL, SetMaxWallopNicks, 0, 0 },
/****************************************************************************/
#ifdef LITE
	{ "MENU",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
#else
	{ "MENU",			STR_TYPE_VAR,	0, NULL, set_menu, 0, 0 },
#endif
	{ "MINIMUM_SERVERS",		INT_TYPE_VAR,	DEFAULT_MINIMUM_SERVERS, NULL, NULL, 0, VF_NODAEMON },
	{ "MINIMUM_USERS",		INT_TYPE_VAR,	DEFAULT_MINIMUM_USERS, NULL, NULL, 0, VF_NODAEMON },
	{ "NO_ASK_NICKNAME",		BOOL_TYPE_VAR,	DEFAULT_NO_ASK_NICKNAME, NULL, NULL, 0, 0 },
	{ "NO_CTCP_FLOOD",		BOOL_TYPE_VAR,	DEFAULT_NO_CTCP_FLOOD, NULL, NULL, 0, 0 },
	{ "NOTIFY_HANDLER",		STR_TYPE_VAR, 	0, 0, set_notify_handler, 0, 0 },
	{ "NOTIFY_LEVEL",		STR_TYPE_VAR,	0, NULL, set_notify_level, 0, 0 },
	{ "NOTIFY_ON_TERMINATION",	BOOL_TYPE_VAR,	DEFAULT_NOTIFY_ON_TERMINATION, NULL, NULL, 0, VF_NODAEMON },
/**************************** PATCHED by Flier ******************************/
	{ "NOTIFY_STRING",		STR_TYPE_VAR,	0, NULL, Cnotifystring, 0, 0 },
/****************************************************************************/
	{ "NOVICE",			BOOL_TYPE_VAR,	DEFAULT_NOVICE, NULL, NULL, 0, 0 },
	{ "OLD_ENCRYPT_PROGRAM",	BOOL_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "REALNAME",			STR_TYPE_VAR,	0, 0, set_realname, 0, VF_NODAEMON },
	{ "SAME_WINDOW_ONLY",		BOOL_TYPE_VAR,	DEFAULT_SAME_WINDOW_ONLY, NULL, NULL, 0, 0 },
	{ "SCREEN_OPTIONS", 		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SCROLL",			BOOL_TYPE_VAR,	DEFAULT_SCROLL, NULL, set_scroll, 0, 0 },
	{ "SCROLL_LINES",		INT_TYPE_VAR,	DEFAULT_SCROLL_LINES, NULL, set_scroll_lines, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "SCROLLZ_STRING",             STR_TYPE_VAR,   0, NULL, SetScrollZstr, 0, 0 },
/****************************************************************************/
	{ "SEND_IGNORE_MSG",		BOOL_TYPE_VAR,	DEFAULT_SEND_IGNORE_MSG, NULL, NULL, 0, 0 },
	{ "SHELL",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_FLAGS",		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_LIMIT",		INT_TYPE_VAR,	DEFAULT_SHELL_LIMIT, NULL, NULL, 0, VF_NODAEMON },
	{ "SHOW_AWAY_ONCE",		BOOL_TYPE_VAR,	DEFAULT_SHOW_AWAY_ONCE, NULL, NULL, 0, 0 },
	{ "SHOW_CHANNEL_NAMES",		BOOL_TYPE_VAR,	DEFAULT_SHOW_CHANNEL_NAMES, NULL, NULL, 0, 0 },
	{ "SHOW_END_OF_MSGS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_END_OF_MSGS, NULL, NULL, 0, 0 },
	{ "SHOW_NUMERICS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_NUMERICS, NULL, NULL, 0, 0 },
	{ "SHOW_STATUS_ALL",		BOOL_TYPE_VAR,	DEFAULT_SHOW_STATUS_ALL, NULL, update_all_status, 0, 0 },
	{ "SHOW_WHO_HOPCOUNT", 		BOOL_TYPE_VAR,	DEFAULT_SHOW_WHO_HOPCOUNT, NULL, NULL, 0, 0 },
/**************************** Patched by Flier ******************************/
	{ "STAMP_FORMAT",		STR_TYPE_VAR,	0, NULL, SetStampFormat, 0, 0 },
/****************************************************************************/
	{ "STATUS_AWAY",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CHANNEL",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "STATUS_CHANNELCOUNT",       	STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/****************************************************************************/
	{ "STATUS_CHANOP",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CLOCK",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_FORMAT",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_FORMAT1",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_FORMAT2",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/**************************** PATCHED by Flier ******************************/
	{ "STATUS_FORMAT3",             STR_TYPE_VAR,   0, NULL, build_status, 0, 0 },
/****************************************************************************/
	{ "STATUS_GROUP",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_HOLD",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_HOLD_LINES",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_INSERT",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "STATUS_LOADAVG",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/****************************************************************************/
	{ "STATUS_MAIL",		STR_TYPE_VAR,	0, NULL, build_status, 0, VF_NODAEMON },
	{ "STATUS_MODE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_NOTIFY",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_OPER",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_OVERWRITE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_QUERY",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/**************************** PATCHED by Flier ******************************/
#if defined(CELE) || defined(WANTANSI)
        { "STATUS_REVERSE",		BOOL_TYPE_VAR,	0, NULL, build_status, 0, 0 },
#else
        { "STATUS_REVERSE",		BOOL_TYPE_VAR,	1, NULL, build_status, 0, 0 },
#endif
/****************************************************************************/
	{ "STATUS_SERVER",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_UMODE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "STATUS_UPTIME",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/****************************************************************************/
	{ "STATUS_USER",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER1",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER2",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER3",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "STATUS_USER4",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
        { "STATUS_USER5",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
        { "STATUS_USER6",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
        { "STATUS_USER7",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
        { "STATUS_USER8",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
        { "STATUS_USER9",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
/****************************************************************************/
	{ "STATUS_WINDOW",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "SUPPRESS_SERVER_MOTD",	BOOL_TYPE_VAR,	DEFAULT_SUPPRESS_SERVER_MOTD, NULL, NULL, 0, VF_NODAEMON },
	{ "TAB",			BOOL_TYPE_VAR,	DEFAULT_TAB, NULL, NULL, 0, 0 },
	{ "TAB_MAX",			INT_TYPE_VAR,	DEFAULT_TAB_MAX, NULL, NULL, 0, 0 },
	{ "TRANSLATION",		STR_TYPE_VAR,	0, NULL, set_translation, 0, 0 },
/**************************** PATCHED by Flier ******************************/
        { "TRUNCATE_PUBLIC_CHANNEL",	BOOL_TYPE_VAR, 	DEFAULT_TRUNCATE_PUBLIC_CHANNEL, NULL, NULL, 0, 0 },
/****************************************************************************/
	{ "UNDERLINE_VIDEO",		BOOL_TYPE_VAR,	DEFAULT_UNDERLINE_VIDEO, NULL, NULL, 0, 0 },
	{ "USE_OLD_MSG",		BOOL_TYPE_VAR,	DEFAULT_USE_OLD_MSG, NULL, NULL, 0, 0 },
	{ "USER_INFORMATION", 		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "USER_WALLOPS",		BOOL_TYPE_VAR,	DEFAULT_USER_WALLOPS, NULL, NULL, 0, 0 },
	{ "VERBOSE_CTCP",		BOOL_TYPE_VAR,	DEFAULT_VERBOSE_CTCP, NULL, NULL, 0, 0 },
	{ "WARN_OF_IGNORES",		BOOL_TYPE_VAR,	DEFAULT_WARN_OF_IGNORES, NULL, NULL, 0, 0 },
	{ "XTERM_GEOMOPTSTR", 		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "XTERM_OPTIONS", 		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "XTERM_PATH", 		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ (char *) 0, 0, 0, 0, 0, 0, 0 }
};

/*
 * init_variables: initializes the string variables that can't really be
 * initialized properly above 
 */
void
init_variables()
{
/**************************** PATCHED by Flier ******************************/
        int old_disp;
/****************************************************************************/

	set_string_var(CMDCHARS_VAR, DEFAULT_CMDCHARS);
	set_string_var(LOGFILE_VAR, DEFAULT_LOGFILE);
	set_string_var(SHELL_VAR, DEFAULT_SHELL);
	set_string_var(SHELL_FLAGS_VAR, DEFAULT_SHELL_FLAGS);
	set_string_var(DECRYPT_PROGRAM_VAR, UP(DEFAULT_DECRYPT_PROGRAM));
	set_string_var(ENCRYPT_PROGRAM_VAR, DEFAULT_ENCRYPT_PROGRAM);
	set_string_var(CONTINUED_LINE_VAR, DEFAULT_CONTINUED_LINE);
	set_string_var(INPUT_PROMPT_VAR, DEFAULT_INPUT_PROMPT);
	set_string_var(HIGHLIGHT_CHAR_VAR, DEFAULT_HIGHLIGHT_CHAR);
	set_string_var(HISTORY_FILE_VAR, DEFAULT_HISTORY_FILE);
	set_string_var(IRCHOST_VAR, empty_string);
	set_string_var(LASTLOG_LEVEL_VAR, DEFAULT_LASTLOG_LEVEL);
	set_string_var(NOTIFY_HANDLER_VAR, DEFAULT_NOTIFY_HANDLER);
	set_string_var(NOTIFY_LEVEL_VAR, DEFAULT_NOTIFY_LEVEL);
	set_string_var(REALNAME_VAR, realname);
	set_string_var(STAMP_FORMAT, DEFAULT_STAMP_FORMAT);
	set_string_var(STATUS_FORMAT_VAR, DEFAULT_STATUS_FORMAT);
	set_string_var(STATUS_FORMAT1_VAR, DEFAULT_STATUS_FORMAT1);
	set_string_var(STATUS_FORMAT2_VAR, DEFAULT_STATUS_FORMAT2);
/**************************** PATCHED by Flier ******************************/
        set_string_var(STATUS_FORMAT3_VAR, DEFAULT_STATUS_FORMAT3);
/****************************************************************************/
	set_string_var(STATUS_AWAY_VAR, DEFAULT_STATUS_AWAY);
	set_string_var(STATUS_CHANNEL_VAR, DEFAULT_STATUS_CHANNEL);
	set_string_var(STATUS_CHANOP_VAR, DEFAULT_STATUS_CHANOP);
	set_string_var(STATUS_CLOCK_VAR, DEFAULT_STATUS_CLOCK);
	set_string_var(STATUS_GROUP_VAR, DEFAULT_STATUS_GROUP);
	set_string_var(STATUS_HOLD_VAR, DEFAULT_STATUS_HOLD);
	set_string_var(STATUS_HOLD_LINES_VAR, DEFAULT_STATUS_HOLD_LINES);
	set_string_var(STATUS_INSERT_VAR, DEFAULT_STATUS_INSERT);
/**************************** PATCHED by Flier ******************************/
#ifdef CELE
        set_string_var(STATUS_LOADAVG_VAR, DEFAULT_STATUS_LOADAVG);
#endif
/****************************************************************************/
	set_string_var(STATUS_MAIL_VAR, DEFAULT_STATUS_MAIL);
	set_string_var(STATUS_MODE_VAR, DEFAULT_STATUS_MODE);
	set_string_var(STATUS_OPER_VAR, DEFAULT_STATUS_OPER);
	set_string_var(STATUS_OVERWRITE_VAR, DEFAULT_STATUS_OVERWRITE);
	set_string_var(STATUS_QUERY_VAR, DEFAULT_STATUS_QUERY);
	set_string_var(STATUS_SERVER_VAR, DEFAULT_STATUS_SERVER);
	set_string_var(STATUS_UMODE_VAR, DEFAULT_STATUS_UMODE);
	set_string_var(STATUS_USER_VAR, DEFAULT_STATUS_USER);
	set_string_var(STATUS_USER1_VAR, DEFAULT_STATUS_USER1);
	set_string_var(STATUS_USER2_VAR, DEFAULT_STATUS_USER2);
	set_string_var(STATUS_USER3_VAR, DEFAULT_STATUS_USER3);
	set_string_var(STATUS_WINDOW_VAR, DEFAULT_STATUS_WINDOW);
	set_string_var(USER_INFO_VAR, DEFAULT_USERINFO);
	set_string_var(XTERM_GEOMOPTSTR_VAR, DEFAULT_XTERM_GEOMOPTSTR);
	set_string_var(XTERM_OPTIONS_VAR, DEFAULT_XTERM_OPTIONS);
	set_string_var(XTERM_PATH_VAR, DEFAULT_XTERM_PATH);
	set_alarm(DEFAULT_CLOCK_ALARM);
	set_beep_on_msg(DEFAULT_BEEP_ON_MSG);
	set_string_var(STATUS_NOTIFY_VAR, DEFAULT_STATUS_NOTIFY);
	set_string_var(CLIENTINFO_VAR, IRCII_COMMENT);
/**************************** PATCHED by Flier ******************************/
        /* set below */
/*#ifdef _Windows
	set_string_var(TRANSLATION_VAR, "LATIN_1");
	set_translation("LATIN_1");
#else
	set_string_var(TRANSLATION_VAR, "ASCII");
	set_translation("ASCII");
#endif*/ /* _Windows */
	/*set_string_var(HELP_PATH_VAR, DEFAULT_HELP_PATH);*/
/****************************************************************************/
	set_lastlog_size(irc_variable[LASTLOG_VAR].integer);
	set_history_size(irc_variable[HISTORY_VAR].integer);
	set_history_file(irc_variable[HISTORY_FILE_VAR].string);
	set_highlight_char(irc_variable[HIGHLIGHT_CHAR_VAR].string);
	set_lastlog_level(irc_variable[LASTLOG_LEVEL_VAR].string);
	set_notify_level(irc_variable[NOTIFY_LEVEL_VAR].string);
	if (get_int_var(LOG_VAR))
		set_int_var(LOG_VAR, 1);
/**************************** PATCHED by Flier ******************************/
        set_string_var(SCROLLZ_STRING_VAR, ScrollZstr);
 	set_string_var(STATUS_CHANNELCOUNT_VAR, DEFAULT_STATUS_CHANNELCOUNT);
 	set_string_var(STATUS_UPTIME_VAR, DEFAULT_STATUS_UPTIME);
        set_string_var(STATUS_USER4_VAR, DEFAULT_STATUS_USER4);
        set_string_var(STATUS_USER5_VAR, DEFAULT_STATUS_USER5);
        set_string_var(STATUS_USER6_VAR, DEFAULT_STATUS_USER6);
        set_string_var(STATUS_USER7_VAR, DEFAULT_STATUS_USER7);
        set_string_var(STATUS_USER8_VAR, DEFAULT_STATUS_USER8);
        set_string_var(STATUS_USER9_VAR, DEFAULT_STATUS_USER9);
        set_string_var(NOTIFY_STRING_VAR,CelerityNtfy);
        old_disp=window_display;
        window_display=0;
#ifdef _Windows
	set_string_var(TRANSLATION_VAR, "LATIN_1");
	set_translation("LATIN_1");
#else
#if defined(WANTANSI)
        if (get_int_var(HIGH_ASCII_VAR)) {
            set_string_var(TRANSLATION_VAR,"LATIN_1");
            set_translation("LATIN_1");
        }
#else
	set_string_var(TRANSLATION_VAR, "ASCII");
        set_translation("ASCII");
#endif /* WANTANSI && HIGHASCII */
#endif /* _Windows */
        window_display=old_disp;
        if (HelpPathVar) {
            set_string_var(HELP_PATH_VAR,HelpPathVar);
            new_free(&HelpPathVar);
        }
	else set_string_var(HELP_PATH_VAR, DEFAULT_HELP_PATH);
        set_string_var(DCC_PORTS_VAR,"0");
/****************************************************************************/
}

/*
 * find_variable: looks up variable name in the variable table and returns
 * the index into the variable array of the match.  If there is no match, cnt
 * is set to 0 and -1 is returned.  If more than one match the string, cnt is
 * set to that number, and it returns the first match.  Index will contain
 * the index into the array of the first found entry 
 */
static	int
find_variable(org_name, cnt)
	char	*org_name;
	int	*cnt;
{
	IrcVariable *v,
		    *first;
 	size_t	len;
 	int	var_index;
	char	*name = (char *) 0;

 	malloc_strcpy(&name, org_name);
	upper(name);
	len = strlen(name);
	var_index = 0;
	for (first = irc_variable; first->name; first++, var_index++)

	{
		if (strncmp(name, first->name, len) == 0)
		{
			*cnt = 1;
			break;
		}
	}
	if (first->name)

	{
		if (strlen(first->name) != len)
		{
			v = first;
			for (v++; v->name; v++, (*cnt)++)

			{
				if (strncmp(name, v->name, len) != 0)
					break;
			}
		}
		new_free(&name);
		return (var_index);
	}
	else

	{
		*cnt = 0;
		new_free(&name);
		return (-1);
	}
}

/*
 * do_boolean: just a handy thing.  Returns 1 if the str is not ON, OFF, or
 * TOGGLE 
 */
int
do_boolean(str, value)
	char	*str;
	int	*value;
{
	upper(str);
	if (strcmp(str, var_settings[ON]) == 0)
		*value = 1;
	else if (strcmp(str, var_settings[OFF]) == 0)
		*value = 0;
	else if (strcmp(str, "TOGGLE") == 0)
	{
		if (*value)
			*value = 0;
		else
			*value = 1;
	}
	else
		return (1);
	return (0);
}

/*
 * set_var_value: Given the variable structure and the string representation
 * of the value, this sets the value in the most verbose and error checking
 * of manors.  It displays the results of the set and executes the function
 * defined in the var structure 
 */
void
set_var_value(var_index, value)
	int	var_index;
	char	*value;
{
	char	*rest;
	IrcVariable *var;
	int	old;


	var = &(irc_variable[var_index]);
#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID && var->flags&VF_NODAEMON && value && *value)
	{
		say("You are not permitted to set that variable");
		return;
	}
#endif
	switch (var->type)
	{
	case BOOL_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			old = var->integer;
			if (do_boolean(value, &(var->integer)))

			{
				say("Value must be either ON, OFF, or TOGGLE");
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (old != var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			if (var->func)
				(var->func) (var->integer);
			say("Value of %s set to %s", var->name,
				var->integer ? var_settings[ON]
					     : var_settings[OFF]);
		}
		else
			say("Current value of %s is %s", var->name,
				(var->integer) ?
				var_settings[ON] : var_settings[OFF]);
		break;
	case CHAR_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			if ((int) strlen(value) > 1)
				say("Value of %s must be a single character",
					var->name);
			else
			{
				if (!(var->int_flags & VIF_CHANGED))
				{
					if (var->integer != *value)
						var->int_flags |= VIF_CHANGED;
				}
				if (loading_global)
					var->int_flags |= VIF_GLOBAL;
				var->integer = *value;
				if (var->func)
					(var->func) (var->integer);
				say("Value of %s set to '%c'", var->name,
					var->integer);
			}
		}
		else
			say("Current value of %s is '%c'", var->name,
				var->integer);
		break;
	case INT_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			int	val;

			if (!is_number(value))
			{
				say("Value of %s must be numeric!", var->name);
				break;
			}
			if ((val = atoi(value)) < 0)
			{
				say("Value of %s must be greater than 0",
					var->name);
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer != val)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			var->integer = val;
			if (var->func)
				(var->func) (var->integer);
			say("Value of %s set to %d", var->name, var->integer);
		}
		else
			say("Current value of %s is %d", var->name,
				var->integer);
		break;
	case STR_TYPE_VAR:
		if (value)
		{
			if (*value)
			{
				char	*temp = NULL;

				if (var->flags & VF_EXPAND_PATH)
				{
					temp = expand_twiddle(value);
					if (temp)
						value = temp;
					else
						say("SET: no such user");
				}
				if ((!var->int_flags & VIF_CHANGED))
				{
					if ((var->string && ! value) ||
					    (! var->string && value) ||
					    my_stricmp(var->string, value))
						var->int_flags |= VIF_CHANGED;
				}
				if (loading_global)
					var->int_flags |= VIF_GLOBAL;
				malloc_strcpy(&(var->string), value);
				if (temp)
					new_free(&temp);
			}
			else
			{
				if (var->string)
					say("Current value of %s is %s",
						var->name, var->string);
				else
					say("No value for %s has been set",
						var->name);
				return;
			}
		}
		else
			new_free(&(var->string));
		if (var->func)
			(var->func) (var->string);
		say("Value of %s set to %s", var->name, var->string ?
			var->string : "<EMPTY>");
		break;
	}
}

/*
 * set_variable: The SET command sets one of the irc variables.  The args
 * should consist of "variable-name setting", where variable name can be
 * partial, but non-ambbiguous, and setting depends on the variable being set 
 */
/*ARGSUSED*/
void
set_variable(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*var;
	int     cnt,
 		var_index,
 		lastlog_level;

	if ((var = next_arg(args, &args)) != NULL)
	{
		if (*var == '-')
		{
			var++;
			args = (char *) 0;
		}
		var_index = find_variable(var, &cnt);
		switch (cnt)
		{
		case 0:
			say("No such variable \"%s\"", var);
			return;
		case 1:
			set_var_value(var_index, args);
			return;
		default:
			say("%s is ambiguous", var);
			for (cnt += var_index; var_index < cnt; var_index++)
				set_var_value(var_index, empty_string);
			return;
		}
	}
 	lastlog_level = message_from_level(LOG_CRAP);
	for (var_index = 0; var_index < NUMBER_OF_VARIABLES; var_index++)
		set_var_value(var_index, empty_string);
 	(void) message_from_level(lastlog_level);
}

/*
 * get_string_var: returns the value of the string variable given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
char	*
get_string_var(var)
	int	var;
{
	return (irc_variable[var].string);
}

/*
 * get_int_var: returns the value of the integer string given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
int
get_int_var(var)
	int	var;
{
	return (irc_variable[var].integer);
}

/*
 * set_string_var: sets the string variable given as an index into the
 * variable table to the given string.  If string is null, the current value
 * of the string variable is freed and set to null 
 */
void
set_string_var(var, string)
	int	var;
	char	*string;
{
	if (string)
		malloc_strcpy(&(irc_variable[var].string), string);
	else
		new_free(&(irc_variable[var].string));
}

/*
 * set_int_var: sets the integer value of the variable given as an index into
 * the variable table to the given value 
 */
void
set_int_var(var, value)
	int	var;
	unsigned int	value;
{
#ifndef LITE
	if (var == NOVICE_VAR && !load_depth && !value)
	{
say("WARNING: Setting NOVICE to OFF enables commands in your client which");
say("         could be used by others on IRC to control your IRC session");
say("         or compromise security on your machine. If somebody has");
say("         asked you to do this, and you do not know EXACTLY why, or if");
say("         you are not ABSOLUTELY sure what you are doing, you should");
say("         immediately /SET NOVICE ON and find out more information.");
	}
#endif
	irc_variable[var].integer = value;
}

/*
 * save_variables: this writes all of the IRCII variables to the given FILE
 * pointer in such a way that they can be loaded in using LOAD or the -l switch 
 */
void
save_variables(fp, do_all)
	FILE	*fp;
	int	do_all;
{
	IrcVariable *var;

	for (var = irc_variable; var->name; var++)
	{
		if (!(var->int_flags & VIF_CHANGED))
			continue;
		if (do_all || !(var->int_flags & VIF_GLOBAL))
		{
			if (strcmp(var->name, "DISPLAY") == 0 || strcmp(var->name, "CLIENT_INFORMATION") == 0)
				continue;
			fprintf(fp, "SET ");
			switch (var->type)
			{
			case BOOL_TYPE_VAR:
				fprintf(fp, "%s %s\n", var->name, var->integer ?
					var_settings[ON] : var_settings[OFF]);
				break;
			case CHAR_TYPE_VAR:
				fprintf(fp, "%s %c\n", var->name, var->integer);
				break;
			case INT_TYPE_VAR:
				fprintf(fp, "%s %u\n", var->name, var->integer);
				break;
			case STR_TYPE_VAR:
				if (var->string)
					fprintf(fp, "%s %s\n", var->name,
						var->string);
				else
					fprintf(fp, "-%s\n", var->name);
				break;
			}
		}
	}
}

char	*
make_string_var(var_name)
	char	*var_name;
{
	int	cnt,
		var_index;
 	char	lbuf[BIG_BUFFER_SIZE + 1],
		*ret = (char *) 0,
		*cmd = (char *) 0;

	malloc_strcpy(&cmd, var_name);
	upper(cmd);
	if (((var_index = find_variable(cmd, &cnt)) == -1) ||
	    (cnt > 1) ||
	    strcmp(cmd, irc_variable[var_index].name))
 		goto out;
	switch (irc_variable[var_index].type)
	{
	case STR_TYPE_VAR:
		malloc_strcpy(&ret, irc_variable[var_index].string);
		break;
	case INT_TYPE_VAR:
		snprintf(lbuf, sizeof lbuf, "%u", irc_variable[var_index].integer);
 		malloc_strcpy(&ret, lbuf);
		break;
	case BOOL_TYPE_VAR:
		malloc_strcpy(&ret, var_settings[irc_variable[var_index].integer]);
		break;
	case CHAR_TYPE_VAR:
		snprintf(lbuf, sizeof lbuf, "%c", irc_variable[var_index].integer);
 		malloc_strcpy(&ret, lbuf);
		break;
	}
out:
	new_free(&cmd);
	return (ret);

}

/* exec_warning: a warning message displayed whenever EXEC_PROTECTION is turned off.  */
static	void
exec_warning(value)
	int	value;
{
#ifndef LITE
	if (value == OFF)
	{
		say("Warning!  You have turned EXEC_PROTECTION off");
		say("Please read the /HELP SET EXEC_PROTECTION documentation");
	}
#endif
}

static	void
input_warning(value)
	int	value;
{
#ifndef LITE
	if (value == OFF)
	{
		say("Warning!  You have turned INPUT_PROTECTION off");
		say("Please read the /HELP ON INPUT, and /HELP SET INPUT_PROTECTION documentation");
	}
#endif
}

/* returns the size of the character set */
int
charset_size()
{
	return get_int_var(EIGHT_BIT_CHARACTERS_VAR) ? 256 : 128;
}

static	void
eight_bit_characters(value)
	int	value;
{
	if (value == ON && !term_eight_bit())
		say("Warning!  Your terminal says it does not support eight bit characters");
	set_term_eight_bit(value);
}

static	void
set_realname(value)
	char	*value;
{

        if (value)
		strmcpy(realname, value, REALNAME_LEN);
	else
		*realname = '\0';
}

/**************************** PATCHED by Flier ******************************/
static void SetScrollZstr(value)
char *value;
{
    if (value && *value) malloc_strcpy(&ScrollZstr,value);
    else malloc_strcpy(&ScrollZstr,empty_string);
}

static void SetMaxModes(value)
int value;
{
    if (value>6) value=6;
    if (value<1) value=1;
    set_int_var(MAX_MODES_VAR,value);
}

static void SetMaxWallopNicks(value)
int value;
{
    if (value>70) value=70;
    if (value<1) value=1;
    set_int_var(MAX_WALLOP_NICKS_VAR,value);
}

static void SetDCCBlockSize(value)
int value;
{
    if (value>BIG_BUFFER_SIZE || value<16 || (value%2)) value=1024;
    set_int_var(DCC_BLOCK_SIZE_VAR,value);
}

static void SetDCCPorts(value)
char *value;
{
    char *tmpstr;
    char tmpbuf[mybufsize/4+1];

    if (value && *value) {
        strmcpy(tmpbuf,value,mybufsize/4);
        tmpstr=index(tmpbuf,'-');
        if (!tmpstr) tmpstr=index(tmpbuf,':');
        if (tmpstr) {
            *tmpstr++='\0';
            DCCLowPort=atoi(tmpbuf);
            DCCHighPort=atoi(tmpstr);
        }
        else {
            DCCLowPort=atoi(tmpbuf);
            DCCHighPort=65500;
        }
        if (DCCLowPort<1024) DCCLowPort=1024;
        if (DCCHighPort>65500) DCCLowPort=65500;
    }
}

void CleanUpVars() {
    int i;

    for (i=0;irc_variable[i].name;i++)
        if (irc_variable[i].type==STR_TYPE_VAR) new_free(&(irc_variable[i].string));
}

static void Cnotifystring(value)
char *value;
{
    if (value && *value) malloc_strcpy(&CelerityNtfy,value);
    else malloc_strcpy(&CelerityNtfy,empty_string);
}

static void SetAwayFile(file)
char *file;
{
    char *ptr;

    if (file) {
        if ((ptr=expand_twiddle(file))==NULL) {
            say("Bad filename: %s",file);
            set_string_var(AWAY_FILE_VAR,DEFAULT_AWAY_FILE);
            return;
        }
        set_string_var(AWAY_FILE_VAR,ptr);
        new_free(&ptr);
    }
    else set_string_var(AWAY_FILE_VAR,DEFAULT_AWAY_FILE);
}

void SetStampFormat(tsformat)
char *tsformat;
{
    int flag = 0;
    char *format = get_string_var(STAMP_FORMAT);
    time_t timenow;
#ifdef HAVE_STRFTIME
    char tmpstr[mybufsize / 2 + 1];
    struct tm *tm;
#endif /* HAVE_STRFTIME */

    timenow = time(NULL);
    if (timenow <= LastTS)
        return;
    LastTS = timenow;
    new_free(&TimeStampString);
#ifdef HAVE_STRFTIME
    tm = localtime(&timenow);
    strftime(tmpstr, sizeof(tmpstr) - 1, format ? format : empty_string, tm);
    TimeStampString = expand_alias(NULL, tmpstr, empty_string, &flag, NULL);
#else  /* HAVE_STRFTIME */
    TimeStampString = expand_alias(NULL,
                                   format ? format : empty_string,
                                   empty_string, &flag, NULL);
#endif /* HAVE_STRFTIME */
}
/****************************************************************************/
