/*
 * vars.h: header for vars.c
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
 * $Id: vars.h,v 1.11 2009-12-21 14:14:17 f Exp $
 */

#ifndef __vars_h_
#define __vars_h_

/**************************** PATCHED by Flier ******************************/
#include "defs.h"
/****************************************************************************/

	int	do_boolean _((char *, int *));
	void	set_variable _((char *, char *, char *));
	int	get_int_var _((int));
	char	*get_string_var _((int));
	void	set_int_var _((int, unsigned int));
	void	set_string_var _((int, char *));
	void	init_variables _((void));
	char	*make_string_var _((char *));
	void	set_highlight_char _((char *));
	int	charset_size _((void));
	void	save_variables _((FILE *, int));
	void	set_var_value _((int, char *));

extern	char	*var_settings[];
extern	int	loading_global;

/* var_settings indexes ... also used in display.c for highlights */
#define OFF 0
#define ON 1
#define TOGGLE 2

#define DEBUG_COMMANDS		0x0001
#define DEBUG_EXPANSIONS	0x0002
#define DEBUG_FUNCTIONS		0x0004

/* IrcVariable: structure for each variable in the variable table */
typedef struct
{
        char    *name;                  /* what the user types */
        int     type;                   /* variable types, see below */
        int     integer;                /* int value of variable */
        char    *string;                /* string value of variable */
        void    (*func) ();             /* function to do every time variable is set */
        char    int_flags;              /* internal flags to the variable */
        unsigned short  flags;          /* flags for this variable */
}       IrcVariable;

/* indexes for the irc_variable array */
enum {
	ALWAYS_SPLIT_BIGGEST_VAR = 0,
/**************************** PATCHED by Flier ******************************/
	AUTO_RECONNECT_VAR,
/****************************************************************************/
	AUTO_RECONNECT_CHANNELS_VAR,
	AUTO_UNMARK_AWAY_VAR,
	AUTO_WHOWAS_VAR ,
/**************************** PATCHED by Flier ******************************/
	AWAY_FILE_VAR,
/****************************************************************************/
	BEEP_VAR,
	BEEP_MAX_VAR,
/**************************** PATCHED by Flier ******************************/
	BEEP_ON_MAIL_VAR,
/****************************************************************************/
	BEEP_ON_MSG_VAR,
	BEEP_WHEN_AWAY_VAR,
	BOLD_VIDEO_VAR,
	CHANLOG_STRIP_ANSI_VAR,
	CHANNEL_NAME_WIDTH_VAR,
	CLIENTINFO_VAR,
	CLOCK_VAR,
	CLOCK_24HOUR_VAR,
	CLOCK_ALARM_VAR,
	CMDCHARS_VAR,
	COMMAND_MODE_VAR,
	CONTINUED_LINE_VAR,
	CTCP_REPLY_BACKLOG_SECONDS_VAR,
	CTCP_REPLY_FLOOD_SIZE_VAR,
	CTCP_REPLY_IGNORE_SECONDS_VAR,
	DCC_BLOCK_SIZE_VAR,
/**************************** PATCHED by Flier ******************************/
	DCC_HOST_VAR,
	DCC_PORTS_VAR,
/****************************************************************************/
	DEBUG_VAR,
	DECRYPT_PROGRAM_VAR,
/**************************** PATCHED by Flier ******************************/
        DEFAULT_PROTOCOL_VAR,
/****************************************************************************/
	DISPLAY_VAR,
/**************************** PATCHED by Flier ******************************/
	DISPLAY_ANSI_VAR,
/****************************************************************************/
	DISPLAY_ENCODING_VAR,
	EIGHT_BIT_CHARACTERS_VAR,
/**************************** PATCHED by Flier ******************************/
	ENCRYPT_PAD_MSGS_VAR,
	ENCRYPT_PAD_PUBLIC_VAR,
/****************************************************************************/
	ENCRYPT_PROGRAM_VAR,
	EXEC_PROTECTION_VAR,
	FLOOD_AFTER_VAR,
	FLOOD_RATE_VAR,
	FLOOD_USERS_VAR,
	FLOOD_WARNING_VAR,
	FULL_STATUS_LINE_VAR,
	HELP_PAGER_VAR,
	HELP_PATH_VAR,
	HELP_PROMPT_VAR,
	HELP_WINDOW_VAR,
	HIDE_CHANNEL_KEYS_VAR,
	HIDE_PRIVATE_CHANNELS_VAR,
	HIGHLIGHT_CHAR_VAR,
/**************************** PATCHED by Flier ******************************/
	HIGH_ASCII_VAR,
/****************************************************************************/
	HISTORY_VAR,
	HISTORY_FILE_VAR,
	HOLD_MODE_VAR,
	HOLD_MODE_MAX_VAR,
/**************************** Patched by Flier ******************************/
/* Hyper DCC by Annatar */
	HYPER_DCC_VAR,
/****************************************************************************/
	INDENT_VAR,
	INPUT_ALIASES_VAR,
	INPUT_ENCODING_VAR,
	INPUT_PROMPT_VAR,
/**************************** Patched by Flier ******************************/
	INPUT_PROMPT_SHIFT_NEW_VAR,
/****************************************************************************/
	INPUT_PROTECTION_VAR,
	INSERT_MODE_VAR,
	INVERSE_VIDEO_VAR,
	IRCHOST_VAR,
	IRC_ENCODING_VAR,
/**************************** PATCHED by Flier ******************************/
	ISO2022_SUPPORT_VAR,
/****************************************************************************/
	LASTLOG_VAR,
/**************************** PATCHED by Flier ******************************/
	LASTLOG_ANSI_VAR,
/****************************************************************************/
	LASTLOG_LEVEL_VAR,
	LOAD_PATH_VAR,
	LOG_VAR,
	LOGFILE_VAR,
	MAIL_VAR,
	MAKE_NOTICE_MSG_VAR,
/**************************** PATCHED by Flier ******************************/
	MAX_MODES_VAR,
/****************************************************************************/
	MAX_RECURSIONS_VAR,
/**************************** PATCHED by Flier ******************************/
	MAX_WALLOP_NICKS_VAR,
/****************************************************************************/
	MENU_VAR,
	MINIMUM_SERVERS_VAR,
	MINIMUM_USERS_VAR,
	NETSPLIT_TIME_VAR,
	NOTIFY_HANDLER_VAR,
	NOTIFY_LEVEL_VAR,
	NOTIFY_ON_TERMINATION_VAR,
/**************************** PATCHED by Flier ******************************/
	NOTIFY_SHOW_NAME_VAR,
	NOTIFY_STRING_VAR,
/****************************************************************************/
	NOVICE_VAR,
	NO_ASK_NICKNAME_VAR,
	NO_CTCP_FLOOD_VAR,
	OLD_ENCRYPT_PROGRAM_VAR,
	RATE_LIMIT_JOIN_VAR,
	REALNAME_VAR,
	SAME_WINDOW_ONLY_VAR,
/**************************** PATCHED by Flier ******************************/
	SAVE_ENCRYPTION_KEYS_VAR,
/****************************************************************************/
	SCREEN_OPTIONS_VAR,
	SCROLL_VAR,
/**************************** PATCHED by Flier ******************************/
	SCROLLZ_STRING_VAR,
/****************************************************************************/
	SCROLL_LINES_VAR,
	SEND_IGNORE_MSG_VAR,
/**************************** PATCHED by Flier ******************************/
	SEND_USERHOST_ON_NICK_IN_USE_VAR,
/****************************************************************************/
	SHELL_VAR,
	SHELL_FLAGS_VAR,
	SHELL_LIMIT_VAR,
	SHOW_AWAY_ONCE_VAR,
	SHOW_CHANNEL_NAMES_VAR,
	SHOW_END_OF_MSGS_VAR,
	SHOW_NUMERICS_VAR,
	SHOW_STATUS_ALL_VAR,
	SHOW_WHO_HOPCOUNT_VAR,
/**************************** PATCHED by Flier ******************************/
	SSL_CA_FILE_VAR,
	SSL_PRIORITY_STRING_VAR,
	SSL_VERIFY_CERTIFICATE_VAR,
	STAMP_FORMAT,
/****************************************************************************/
	STATUS_AWAY_VAR,
	STATUS_CHANNEL_VAR,
/**************************** PATCHED by Flier ******************************/
	STATUS_CHANNELCOUNT_VAR,
/****************************************************************************/
	STATUS_CHANOP_VAR,
	STATUS_CLOCK_VAR,
	STATUS_FORMAT_VAR,
	STATUS_FORMAT1_VAR,
	STATUS_FORMAT2_VAR,
/**************************** PATCHED by Flier ******************************/
	STATUS_FORMAT3_VAR,
/****************************************************************************/
	STATUS_GROUP_VAR,
	STATUS_HOLD_VAR,
	STATUS_HOLD_LINES_VAR,
	STATUS_INSERT_VAR,
/**************************** PATCHED by Flier ******************************/
	STATUS_LINES_VAR,
	STATUS_LOADAVG_VAR,
/****************************************************************************/
	STATUS_MAIL_VAR,
	STATUS_MODE_VAR,
	STATUS_NOTIFY_VAR,
	STATUS_NOTIFY_REPW_END_VAR,
	STATUS_NOTIFY_REPW_START_VAR,
	STATUS_OPER_VAR,
	STATUS_OVERWRITE_VAR,
	STATUS_QUERY_VAR,
/**************************** PATCHED by Flier ******************************/
	STATUS_REVERSE_VAR,
/****************************************************************************/
	STATUS_SCROLLED_VAR,
	STATUS_SCROLLED_LINES_VAR,
	STATUS_SERVER_VAR,
	STATUS_UMODE_VAR,
/**************************** PATCHED by Flier ******************************/
	STATUS_UPTIME_VAR,
/****************************************************************************/
	STATUS_USER_VAR,
	STATUS_USER1_VAR,
	STATUS_USER2_VAR,
	STATUS_USER3_VAR,
/**************************** PATCHED by Flier ******************************/
	STATUS_USER4_VAR,
	STATUS_USER5_VAR,
	STATUS_USER6_VAR,
	STATUS_USER7_VAR,
	STATUS_USER8_VAR,
	STATUS_USER9_VAR,
/****************************************************************************/
	STATUS_WINDOW_VAR,
	SUPPRESS_SERVER_MOTD_VAR,
	TAB_VAR,
	TAB_MAX_VAR,
/**************************** PATCHED by Flier ******************************/
	TRACE_VAR,
	TRUNCATE_PUBLIC_CHANNEL_VAR,
/****************************************************************************/
	UNDERLINE_VIDEO_VAR,
/**************************** PATCHED by Flier ******************************/
    URL_BUFFER_SIZE_VAR,
	USERNAME_VAR,
/****************************************************************************/
	USER_INFO_VAR,
	USER_WALLOPS_VAR,
	VERBOSE_CTCP_VAR,
	WARN_OF_IGNORES_VAR,
	XTERM_GEOMOPTSTR_VAR,
	XTERM_OPTIONS_VAR,
	XTERM_PATH_VAR,
	NUMBER_OF_VARIABLES
};
#endif /* __vars_h_ */
