/* 'new' config.h:
 *	A configuration file designed to make best use of the abilities
 *	of ircII, and trying to make things more intuitively understandable.
 *
 * Based on the 'classic' config.h by Michael Sandrof.
 *
 * Copyright (c) 1991 Michael Sandrof.
 * Copyright (c) 1991 Carl V. Loesch
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
 * $Id: config.h,v 1.20 2004-08-13 18:38:56 f Exp $
 */

#ifndef __config_h_
#define __config_h_

/*
 * Set your favorite default server list here.  This list should be a
 * whitespace separated hostname:portnum:password list (with portnums and
 * passwords optional).  This IS NOT an optional definition. Please set this
 * to your nearest servers.  However if you use a seperate 'ircII.servers'
 * file and the ircII can find it, this setting is overridden.
 */
#ifndef DEFAULT_SERVER
# define DEFAULT_SERVER	    "change.this.to.a.server"
#endif

/*
 * Uncomment the following if the gecos field of your /etc/passwd has other
 * information in it that you don't want as the user name (such as office
 * locations, phone numbers, etc).  The default delimiter is a comma, change
 * it if you need to. If commented out, the entire gecos field is used. 
 */
#define GECOS_DELIMITER ','

/*
 * Set the following to 1 if you wish for IRCII not to disturb the tty's flow
 * control characters as the default.  Normally, these are ^Q and ^S.  You
 * may have to rebind them in IRCII.  Set it to 0 for IRCII to take over the
 * tty's flow control.
 */
#define USE_FLOW_CONTROL 1

/*
 * MAIL_DELIMITER specifies the unique text that separates one mail message
 * from another in the mail spool file when using UNIX_MAIL.
 */
#define MAIL_DELIMITER "From "

/* Make ^Z stop the irc process by default,
 * if undefined, ^Z will self-insert by default
 */
#define ALLOW_STOP_IRC /**/

/* And here is the port number for default client connections.  */
#define IRC_PORT 6667

/*
 * Uncomment the following to make ircII read a list of irc servers from
 * the ircII.servers file in the ircII library. This file should be
 * whitespace separated hostname:portnum:password (with the portnum and
 * password being optional). This server list will supercede the
 * DEFAULT_SERVER. 
*/

/*#define SERVERS_FILE "ircII.servers"*/

/* Uncomment the following if you want ircII to display the file
 * ircII.motd in the ircII library at startup.
 */
/*#define MOTD_FILE "ircII.motd"*/
/*#define PAUSE_AFTER_MOTD 1*/

/*
 * Below are the IRCII variable defaults.  For boolean variables, use 1 for
 * ON and 0 for OFF.  You may set string variable to NULL if you wish them to
 * have no value.  None of these are optional.  You may *not* comment out or
 * remove them.  They are default values for variables and are required for
 * proper compilation.
 *
 * CTCP reply flood protection sidenote :
 *   1 - Do not put NULL in any of the three DEFAULT_CTCP_REPLY values.
 *   2 - Do not change them if you don't understand them, these ones
 *       do work without being a pain.
 */
/**************************** PATCHED by Flier ******************************/
#define DEFAULT_ALWAYS_SPLIT_BIGGEST 1
#define DEFAULT_AUTO_RECONNECT 1
#define DEFAULT_AUTO_UNMARK_AWAY 0
#define DEFAULT_AUTO_WHOWAS 0
#define DEFAULT_AWAY_FILE "ScrollZ.away"
#define DEFAULT_BEEP 1
#define DEFAULT_BEEP_MAX 3
#define DEFAULT_BEEP_ON_MAIL 0
#define DEFAULT_BEEP_ON_MSG "NONE"
#define DEFAULT_BEEP_WHEN_AWAY 0
#define DEFAULT_BOLD_VIDEO 1
#define DEFAULT_CHANNEL_NAME_WIDTH 10
#define DEFAULT_CLOCK 1
#define DEFAULT_CLOCK_24HOUR 1
#define DEFAULT_CLOCK_ALARM NULL
#define DEFAULT_CMDCHARS "/"
#define DEFAULT_COMMAND_MODE 0
#define DEFAULT_CONTINUED_LINE NULL
#define DEFAULT_CTCP_REPLY_BACKLOG_SECONDS 5
#define DEFAULT_CTCP_REPLY_FLOOD_SIZE_VAR 256
#define DEFAULT_CTCP_REPLY_IGNORE_SECONDS 10
#define DEFAULT_DCC_BLOCK_SIZE 2048
#define DEFAULT_DISPLAY 1
#define DEFAULT_DISPLAY_ANSI 1
#define DEFAULT_EIGHT_BIT_CHARACTERS 1
#define DEFAULT_ENCRYPT_PROGRAM NULL
#define DEFAULT_DECRYPT_PROGRAM NULL
#define DEFAULT_EXEC_PROTECTION 1
#define DEFAULT_FLOOD_AFTER 6
#define DEFAULT_FLOOD_RATE 6
#define DEFAULT_FLOOD_USERS 6
#define DEFAULT_FLOOD_WARNING 1
#define DEFAULT_FULL_STATUS_LINE 1
#define DEFAULT_HELP_PAGER 1
#define DEFAULT_HELP_PROMPT 1
#define DEFAULT_HELP_WINDOW 0
#define DEFAULT_HIDE_CHANNEL_KEYS 0
#define DEFAULT_HIDE_PRIVATE_CHANNELS 0
#define DEFAULT_HIGHLIGHT_CHAR "INVERSE"
#define DEFAULT_HISTORY 30
#define DEFAULT_HISTORY_FILE NULL
#define DEFAULT_HOLD_MODE 0
#define DEFAULT_HOLD_MODE_MAX 0
#define DEFAULT_HYPER_DCC 0
#define DEFAULT_INDENT 1
#define DEFAULT_INPUT_ALIASES 0
#define DEFAULT_INPUT_PROMPT NULL
#define DEFAULT_INPUT_PROTECTION 1
#define DEFAULT_INSERT_MODE 1
#define DEFAULT_INVERSE_VIDEO 1
#define DEFAULT_ISO2022_SUPPORT 0
#define DEFAULT_LASTLOG 512
#define DEFAULT_LASTLOG_ANSI_VAR 1
#define DEFAULT_LASTLOG_LEVEL "ALL DCC USERLOG1 USERLOG2 USERLOG3 USERLOG4"
#define DEFAULT_LOG 0
#define DEFAULT_LOGFILE "IrcLog"
#define DEFAULT_MAIL 1
#define DEFAULT_MAKE_NOTICE_MSG 1
#define DEFAULT_MAX_MODES 4
#define DEFAULT_MAX_RECURSIONS 10
#define DEFAULT_MAX_WALLOP_NICKS 10
#define DEFAULT_MINIMUM_SERVERS 0
#define DEFAULT_MINIMUM_USERS 0
#define DEFAULT_NOTIFY_HANDLER "NOISY"
#define DEFAULT_NOTIFY_LEVEL "ALL DCC USERLOG1 USERLOG2 USERLOG3 USERLOG4"
#define DEFAULT_NOTIFY_ON_TERMINATION 0
#define DEFAULT_NOVICE 0
#define DEFAULT_NO_ASK_NICKNAME 0
#define DEFAULT_NO_CTCP_FLOOD 1
#define DEFAULT_SAME_WINDOW_ONLY 1
#define DEFAULT_SCROLL 1
#define DEFAULT_SCROLL_LINES 1
#define DEFAULT_SEND_IGNORE_MSG 0
#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_SHELL_FLAGS "-c"
#define DEFAULT_SHELL_LIMIT 0
#define DEFAULT_SHOW_AWAY_ONCE 0
#define DEFAULT_SHOW_CHANNEL_NAMES 1
#define DEFAULT_SHOW_END_OF_MSGS 0
#define DEFAULT_SHOW_NUMERICS 0
#define DEFAULT_SHOW_STATUS_ALL 0
#define DEFAULT_SHOW_WHO_HOPCOUNT 0
#ifdef HAVE_STRFTIME
#define DEFAULT_STAMP_FORMAT "%H:%M|"
#else
#define DEFAULT_STAMP_FORMAT "${Z}|"
#endif
#define DEFAULT_STATUS_AWAY " (zZzZ: %A)"
#define DEFAULT_STATUS_CHANNEL " on %C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_CLOCK "time is %T"
#define DEFAULT_STATUS_FORMAT "[%R] %* %B%H%A%S %W %F %>"
#define DEFAULT_STATUS_FORMAT1 "[%R] %*%@%N%#%B%H%C%+%A%Q%S%I%O %W %F %> [lj %3]"
#define DEFAULT_STATUS_FORMAT2 "%1 [lag %2] %M [g %6] [s %5] [%D:%E:%P:%L:%7:%8] %> %T"
#define DEFAULT_STATUS_FORMAT3 "%U [topic %9] %>"
#define	DEFAULT_STATUS_GROUP " [%G]"
#define DEFAULT_STATUS_HOLD " [..more..]"
#define DEFAULT_STATUS_HOLD_LINES " (%B)"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_MAIL "[m %M]"
#define DEFAULT_STATUS_MODE " (+%+)"
#define DEFAULT_STATUS_NOTIFY " [a:%F]"
#define DEFAULT_STATUS_OPER "*"
#define DEFAULT_STATUS_OVERWRITE " (ow)"
#define DEFAULT_STATUS_QUERY " [Q/%Q]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_UMODE " (+%#)"
#define DEFAULT_STATUS_UPTIME "[up %dd %hh %mm]"
#define DEFAULT_STATUS_USER " * type /help for help "
#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_WINDOW "^^^^^"
#define DEFAULT_SUPPRESS_SERVER_MOTD 0
#define DEFAULT_TAB 1
#define DEFAULT_TAB_MAX 8
#define DEFAULT_UNDERLINE_VIDEO 1
#define DEFAULT_USERINFO ""
#define DEFAULT_USER_WALLOPS 0
#define DEFAULT_USE_OLD_MSG 0
#define DEFAULT_VERBOSE_CTCP 1
#define DEFAULT_WARN_OF_IGNORES 1
#define DEFAULT_XTERM_GEOMOPTSTR "-geom"
#define DEFAULT_XTERM_OPTIONS NULL
#define DEFAULT_XTERM_PATH "xterm"

#define DEFAULT_STATUS_USER4 ""
#define DEFAULT_STATUS_USER5 ""
#define DEFAULT_STATUS_USER6 ""
#define DEFAULT_STATUS_USER7 ""
#define DEFAULT_STATUS_USER8 ""
#define DEFAULT_STATUS_USER9 ""
#define DEFAULT_STATUS_CHANNELCOUNT "[O:%o N:%n T:%t]"
#define DEFAULT_STATUS_UPTIME "[up %dd %hh %mm]"

#define DEFAULT_TRUNCATE_PUBLIC_CHANNEL 1

#if defined(CELE) || defined(WANTANSI)

#undef DEFAULT_STAMP_FORMAT
#undef DEFAULT_STATUS_AWAY
#undef DEFAULT_STATUS_CHANNEL
#undef DEFAULT_STATUS_CHANNELCOUNT
#undef DEFAULT_STATUS_CLOCK
#undef DEFAULT_STATUS_FORMAT
#undef DEFAULT_STATUS_FORMAT1
#undef DEFAULT_STATUS_FORMAT2
#undef DEFAULT_STATUS_FORMAT3
#undef DEFAULT_STATUS_HOLD
#undef DEFAULT_STATUS_MAIL
#undef DEFAULT_STATUS_MODE
#undef DEFAULT_STATUS_QUERY
#undef DEFAULT_STATUS_UMODE
#undef DEFAULT_STATUS_UPTIME
#undef DEFAULT_STATUS_WINDOW

#endif /* CELE || WANTANSI */

#ifdef CELE

#undef DEFAULT_BEEP_ON_MSG
#undef DEFAULT_BEEP_ON_MAIL
#undef DEFAULT_INPUT_PROMPT
#undef DEFAULT_SHOW_STATUS_ALL
#undef DEFAULT_STATUS_USER
#undef DEFAULT_STATUS_USER1

#elif defined(WANTANSI)

#undef DEFAULT_STATUS_FORMAT3
#undef DEFAULT_STATUS_HOLD_LINES
#undef DEFAULT_STATUS_NOTIFY
#undef DEFAULT_STATUS_OVERWRITE
#undef DEFAULT_STATUS_SERVER
#undef DEFAULT_STATUS_WINDOW

#endif /* CELE */

#if defined(CELE)

#define DEFAULT_SHOW_STATUS_ALL 1
#define DEFAULT_BEEP_ON_MSG "MSG"
#define DEFAULT_BEEP_ON_MAIL 1
#ifdef HAVE_STRFTIME
#define DEFAULT_STAMP_FORMAT "%H:%M$color(cyan)|$color(off)"
#else
#define DEFAULT_STAMP_FORMAT "$Z$color(cyan)|$color(off)"
#endif
#define DEFAULT_STATUS_AWAY "%y6(%y7zZz:%y4%A%y6)"
#define DEFAULT_STATUS_CHANNELCOUNT "%y6|%y7O:%y5%o %y7N:%y5%n %y7T:%y5%t%y6| "
#define DEFAULT_STATUS_CHANNEL "%y7Ch:%y3%C "
#define DEFAULT_STATUS_CLOCK "%y6[%y5%T%y6]"
#define DEFAULT_STATUS_HOLD "%y6.o %y7please hit enter %y6o."
#define DEFAULT_STATUS_LOADAVG "%y7load/%y4%Z"
#define DEFAULT_STATUS_WINDOW "~~~"
#define DEFAULT_STATUS_MAIL "%y6(%y7M:%y5%M%y6)"
#define DEFAULT_STATUS_MODE "%y6(%y5+%y4%+%y6)"
#define DEFAULT_STATUS_QUERY "%y6(%y7Q:%y5%Q%y6)"
#define DEFAULT_STATUS_UMODE "%y6(%y5+%y4%#%y6)"
#define DEFAULT_STATUS_UPTIME "%y6|%y7Up:%y5 %dd %hh %mm%y6|"
#define DEFAULT_STATUS_USER "?.??"
#define DEFAULT_STATUS_USER1 "no cele.sz!"
#define DEFAULT_INPUT_PROMPT "%y3${N}%y6:%y3${C}%y6> "
#define DEFAULT_STATUS_FORMAT "%y1 %y6[%y7OperVision%y6] %> %y5!S "
#define DEFAULT_STATUS_FORMAT1 "%y1 %T %y4%*%@%y3%N %# %A%Q %>%M %1 "
#define DEFAULT_STATUS_FORMAT2 "%y1 %C%+%y6|%y7lj:%y5%3%y6| %U %> %S%H%B%I%O%F %y5%W %y6|%y7load:%y5%!0%y6| %y6|%y3%!1%y6| "
#define DEFAULT_STATUS_FORMAT3 "%y1 %y5QuickStat %> %y5lag/%y4%2 %y6[%y5DCC:%y4s%6%y5:%y4r%5%y5:%y4o%4%y6] "

#elif defined(WANTANSI)

#ifndef LITE
#ifdef HAVE_STRFTIME
#define DEFAULT_STAMP_FORMAT "%H:%M$color(cyan)|$color(off)"
#else
#define DEFAULT_STAMP_FORMAT "$Z$color(cyan)|$color(off)"
#endif
#else  /* LITE */
#ifdef HAVE_STRFTIME
#define DEFAULT_STAMP_FORMAT "%H:%M|"
#else
#define DEFAULT_STAMP_FORMAT "$Z|"
#endif
#endif /* LITE */
#define DEFAULT_STATUS_AWAY " (%y2zZzZ: %A%y6)"
#define DEFAULT_STATUS_CHANNEL " on %y5%C%y6"
#define DEFAULT_STATUS_CHANNELCOUNT "[O:%y8%o%y6 N:%y8%n%y6 T:%y8%t%y6]"
#define DEFAULT_STATUS_CLOCK "[time %y7%T%y6]"

#if defined(OPERVISION)
#define DEFAULT_STATUS_FORMAT "%y1[%y2%R%y6] %* %B%H%A%S %W %F %> [%y2%!S%y6]"
#else  /* OPERVISION */
#define DEFAULT_STATUS_FORMAT "%y1[%y2%R%y6] %* %B%H%A%S %W %F %>"
#endif /* OPERVISION */

#define DEFAULT_STATUS_FORMAT1 "%y1[%y2%R%y6] %y2%*%y6%y3%@%y6%y7%N%y6%#%C%+%A%Q%S%I%O %y3%W%y6 %F%y9%H%y6 %> [lj %y5%3%y6]%y0"
#define DEFAULT_STATUS_FORMAT2 "%y1%1 [lag %y8%2%y6] %M [g %y8%6%y6] [s %y8%5%y6] [%y8%D%y6:%y8%E%y6:%y8%P%y6:%y8%L%y6:%y8%7%y6:%y8%8%y6]%B %> %T%y0"
#define DEFAULT_STATUS_FORMAT3 "%y1%U [topic %y8%9%y6] %> %y0"
#define DEFAULT_STATUS_HOLD " [..more..]"
#define DEFAULT_STATUS_HOLD_LINES " (%y2%B%y6)"
#define DEFAULT_STATUS_MAIL "[m %y8%M%y6]"
#define DEFAULT_STATUS_MODE " [%y8+%+%y6]"
#define DEFAULT_STATUS_NOTIFY " %ya[a:%F]%y1%y6"
#define DEFAULT_STATUS_OVERWRITE " (%y2ow%y6)"
#define DEFAULT_STATUS_QUERY " [%y4Q%y6/%y4%Q%y6]"
#define DEFAULT_STATUS_SERVER " via %y7%S%y6"
#define DEFAULT_STATUS_UMODE " (%y4+%#%y6)"
#define DEFAULT_STATUS_UPTIME "[up %y8%dd %hh %mm%y6]"
#define DEFAULT_STATUS_WINDOW "^^^^^"

#endif /* CELE */

#ifdef ACID

#undef DEFAULT_AUTO_WHOWAS
#undef DEFAULT_CLOCK_24HOUR
#undef DEFAULT_SEND_IGNORE_MSG
#undef DEFAULT_SHOW_STATUS_ALL
#undef DEFAULT_SUPPRESS_SERVER_MOTD
#undef DEFAULT_STATUS_FORMAT1
#undef DEFAULT_BEEP_ON_MSG

#define DEFAULT_BEEP_ON_MSG "MSG"
#define DEFAULT_AUTO_WHOWAS 1
#define DEFAULT_CLOCK_24HOUR 0
#define DEFAULT_SEND_IGNORE_MSG 1
#define DEFAULT_SHOW_STATUS_ALL 1
#define DEFAULT_SUPPRESS_SERVER_MOTD 1
#define DEFAULT_STATUS_FORMAT1 "%y1[%y2%R%y6] %y2%*%y6%y3%@%y6%y7%N%y6%#%C%+%A%Q%S%I%O %y3%W%y6 %F%y9%H%y6 %> [%y4AcidMods v2.5%y6]%y0"

#endif /* ACID */
/****************************************************************************/

/*
 * define this if you want to have the -l and -L command line
 * options.
 */

#define COMMAND_LINE_L

#endif /* __config_h_ */
