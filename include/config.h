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
 * $Id: config.h,v 1.2 1998-10-06 17:49:19 f Exp $
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

/* Thanks to Allanon a very useful feature, when this is defined, ircII will
 * be able to read help files in compressed format (it will recognize the .Z)
 * If you undefine this you will spare some code, though, so better only
 * set if you are sure you are going to keep your help-files compressed.
 */
#define ZCAT "/bin/zcat"

/* Define ZSUFFIX in case we are using ZCAT */
#ifdef ZCAT
# define ZSUFFIX ".gz"
#endif /* ZCAT */

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
#define DEFAULT_AUTO_UNMARK_AWAY 0
#define DEFAULT_AUTO_WHOWAS 0
#define DEFAULT_BEEP 1
#define DEFAULT_BEEP_MAX 3
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
#define DEFAULT_EXEC_PROTECTION 1
#define DEFAULT_FLOOD_AFTER 6
#define DEFAULT_FLOOD_RATE 6
#define DEFAULT_FLOOD_USERS 6
#define DEFAULT_FLOOD_WARNING 1
#define DEFAULT_FULL_STATUS_LINE 1
#define DEFAULT_HELP_PAGER 1
#define DEFAULT_HELP_PROMPT 1
#define DEFAULT_HELP_WINDOW 0
#define DEFAULT_HIDE_PRIVATE_CHANNELS 0
#define DEFAULT_HIGHLIGHT_CHAR "INVERSE"
#define DEFAULT_HISTORY 30
#define DEFAULT_HISTORY_FILE NULL
#define DEFAULT_HOLD_MODE 0
#define DEFAULT_HOLD_MODE_MAX 0
#define DEFAULT_INDENT 1
#define DEFAULT_INPUT_ALIASES 0
#define DEFAULT_INPUT_PROMPT NULL
#define DEFAULT_INPUT_PROTECTION 1
#define DEFAULT_INSERT_MODE 1
#define DEFAULT_INVERSE_VIDEO 1
#define DEFAULT_LASTLOG 512
#define DEFAULT_LASTLOG_ANSI_VAR 1
#define DEFAULT_LASTLOG_LEVEL "ALL DCC"
#define DEFAULT_LOG 0
#define DEFAULT_LOGFILE "IrcLog"
#define DEFAULT_MAIL 1
#define DEFAULT_MAX_MODES 4
#define DEFAULT_MAX_RECURSIONS 10
#define DEFAULT_MAX_WALLOP_NICKS 10
#define DEFAULT_MINIMUM_SERVERS 0
#define DEFAULT_MINIMUM_USERS 0
#define DEFAULT_NO_CTCP_FLOOD 1
#define DEFAULT_NOTIFY_HANDLER "NOISY"
#define DEFAULT_NOTIFY_LEVEL "ALL DCC"
#define DEFAULT_NOTIFY_ON_TERMINATION 0
#define DEFAULT_NOVICE 0
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
#define DEFAULT_STATUS_AWAY " (away)"
#define DEFAULT_STATUS_CHANNEL " on %C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_CLOCK " %T"
#define DEFAULT_STATUS_FORMAT "[%R]%T %*%@%N%#%S%H%B%Q%A%C%+%I%O%M%F%U %>"
#define DEFAULT_STATUS_FORMAT1 "[%R]%T %*%@%N%#%S%H%B%Q%A%C%+%I%O%M%F%U %>"
#define DEFAULT_STATUS_FORMAT2 "%U %W %X %Y %Z %>"
#define DEFAULT_STATUS_FORMAT3 " %>"
#define	DEFAULT_STATUS_GROUP " [%G]"
#define DEFAULT_STATUS_HOLD " --- more ---"
#define DEFAULT_STATUS_HOLD_LINES " (%B)"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_MODE " (+%+)"
#define DEFAULT_STATUS_MAIL " [Mail: %M]"
#define DEFAULT_STATUS_NOTIFY " [Activity: %F]"
#define DEFAULT_STATUS_OPER "*"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_QUERY " [Query: %Q]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_UMODE " (+%#)"
#define DEFAULT_STATUS_USER " * type /help for help "
#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_WINDOW "^^^^^^^^"
#define DEFAULT_SUPPRESS_SERVER_MOTD 0
#define DEFAULT_TAB 1
#define DEFAULT_TAB_MAX 8
#define DEFAULT_UNDERLINE_VIDEO 1
#define DEFAULT_USE_OLD_MSG 0
#define DEFAULT_USERINFO ""
#define DEFAULT_USER_WALLOPS 0
#define DEFAULT_VERBOSE_CTCP 1
#define DEFAULT_WARN_OF_IGNORES 1
#define DEFAULT_XTERM_OPTIONS NULL

#define DEFAULT_STATUS_USER4 ""
#define DEFAULT_STATUS_USER5 ""
#define DEFAULT_STATUS_USER6 ""
#define DEFAULT_STATUS_USER7 ""
#define DEFAULT_STATUS_USER8 ""
#define DEFAULT_STATUS_USER9 ""
#define DEFAULT_STATUS_CHANNELCOUNT "(O:%o N:%n T:%t)"
#define DEFAULT_STATUS_UPTIME "[up %dd %hh %mm]"

#undef DEFAULT_STATUS_AWAY
#undef DEFAULT_STATUS_CHANNEL
#undef DEFAULT_STATUS_CHANOP
#undef DEFAULT_STATUS_CLOCK
#undef DEFAULT_STATUS_FORMAT
#undef DEFAULT_STATUS_FORMAT1
#undef DEFAULT_STATUS_FORMAT2
#undef DEFAULT_STATUS_FORMAT3
#undef DEFAULT_STATUS_HOLD
#undef DEFAULT_STATUS_MAIL
#undef DEFAULT_STATUS_MODE
#undef DEFAULT_STATUS_OPER
#undef DEFAULT_STATUS_QUERY
#undef DEFAULT_STATUS_UMODE
#undef DEFAULT_STATUS_UPTIME

#if defined(CELE)

#undef DEFAULT_BEEP_ON_MSG
#undef DEFAULT_INPUT_PROMPT
#undef DEFAULT_SHOW_STATUS_ALL
#undef DEFAULT_STATUS_CHANNELCOUNT
#undef DEFAULT_STATUS_USER

#else

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
#define DEFAULT_STATUS_AWAY "[1;36m<[0;37;44mAway:[1;37m%A[1;36m>"
#define DEFAULT_STATUS_CHANNELCOUNT "([0;37;44mO:%o N:%n T:%t[1;36m)"
#define DEFAULT_STATUS_CHANNEL "[0;37;44mChnl:[1;37m%C[1;36m"
#define DEFAULT_STATUS_CHANOP "[1;37m@[1;36m"
#define DEFAULT_STATUS_CLOCK "[0;37;44m%T[1;36m"
#define DEFAULT_STATUS_FORMAT "[44m[1;37m %N[0;37;44m <[1;37m%*[1;36m%@%# %A  %C%+ %W%Q  %M %> Lag[[0;37;44m%2[1;36m] [0m"
#define DEFAULT_STATUS_FORMAT1 "[44m[1;36m [%T][1;37m %N[0;37;44m <[1;37m%*[1;36m%@%# %A %Q %> %M [1;36m([0;37;44mUp:[1;37m%1[1;36m) %W [1;36mLag[[1;37m%2[1;36m] [0m"
#define DEFAULT_STATUS_FORMAT2 "[44m %C%+ %U [1;36m[[0;37;44mlj:[1;37m%3[1;36m] %> %S%H%B%I%O%F [1;36m[[0;37;44mDCC[1;36m:[0;37;44ms[1;37m%4[1;36m:[0;37;44mr[1;37m%5[1;36m:[0;37;44mo[1;37m%6[1;36m] [0m"
#define DEFAULT_STATUS_FORMAT3 "[44m %> [0m"
/*#define DEFAULT_STATUS_FORMAT2 "[46m[0;37;44m [[0;37;41m%4[0;37;41m:[0;37;41m%5[0;37;41m:[0;37;41m%6[0;37;41m:[0;37;41m%7[0;37;41m:[0;37;41m%8[0;37;41m|[0;37;41m%D[0;37;41m:[1;33m%L[0;37;41m:[1;33m%P[0;37;41m:[1;33m%E[0;37;41m] (Up:[1;33m%1[0;37;41m) %S%H%B%I%O%F %> %U [%T] [1;33m"*/
#define DEFAULT_STATUS_HOLD "[Hit ENTER]"
#define DEFAULT_STATUS_MAIL "[1;36m([0;37;44mMail:[1;37m%M[36m)"
#define DEFAULT_STATUS_MODE "[[0;37;44m+%+[1;36m]"
#define DEFAULT_STATUS_OPER "[1;37m*[1;36m"
#define DEFAULT_STATUS_QUERY "[1;36m<[0;37;41mq[1;37m%Q[1;36m>"
#define DEFAULT_STATUS_UMODE "[0;37;44m+[1;37m%#[0;37;44m>"
#define DEFAULT_STATUS_UPTIME "[1;31m([0;37;44mUp:[1;37m%dd %hh %mm[31m)"
#define DEFAULT_STATUS_USER "[*]"
#define DEFAULT_INPUT_PROMPT "[[1;37mcy[0;37m] "

#elif defined(WANTANSI)

#define DEFAULT_STATUS_AWAY " ([1;32mzZzZ: %A[37;22m)"
#define DEFAULT_STATUS_CHANNEL " on [1;33m%C[37;22m"
#define DEFAULT_STATUS_CHANOP "[1;35m@[37;22m"
#define DEFAULT_STATUS_CLOCK "time is [1;36m%T[37;22m"

#if defined(OPERVISION)
#define DEFAULT_STATUS_FORMAT "[44;37m[[1;32m%R[37;22m] %* %B%H%A%S %W %F %> [[1;32m%!S[37;22m]"
#else  /* OPERVISION */
#define DEFAULT_STATUS_FORMAT "[44;37m[[1;32m%R[37;22m] %* %B%H%A%S %W %F %>"
#endif /* OPERVISION */

#define DEFAULT_STATUS_FORMAT1 "[44;37m[[1;32m%R[37;22m] %*%@[1;36m%N[37;22m%#%B%H%C%+%A%Q%S%I%O %W %F %> [lj [1;33m%3[37;22m][0m"
#define DEFAULT_STATUS_FORMAT2 "[44;37m%1 [lag [36m%2[37;22m] %M [g [36m%6[37;22m] [s [36m%5[37;22m] [[36m%D[37;22m:[36m%E[37;22m:[36m%P[37;22m:[36m%L[37;22m:[36m%7[37;22m:[36m%8[37;22m] %> %T[0m"
#define DEFAULT_STATUS_FORMAT3 "[44;37m%U [topic [36m%9[37m] %> [0m"
#define DEFAULT_STATUS_HOLD " [..[1;39mmore[37;22m..]"
#define DEFAULT_STATUS_HOLD_LINES " ([1;32m%B[37;22m)"
#define DEFAULT_STATUS_MAIL "[m [36m%M[37;22m]"
#define DEFAULT_STATUS_MODE " [[36m+%+[37;22m]"
#define DEFAULT_STATUS_NOTIFY " [41;1;33m[a:%F][37;22;44m"
#define DEFAULT_STATUS_OPER "[1;32m*[37;22m"
#define DEFAULT_STATUS_OVERWRITE " ([1;32mow[37;22m)"
#define DEFAULT_STATUS_QUERY " [[1;31mQ[37;22m/[1;31m%Q[37;22m]"
#define DEFAULT_STATUS_SERVER " via [1;36m%S[37;22m"
#define DEFAULT_STATUS_UMODE " ([1;31m+%#[37;22m)"
#define DEFAULT_STATUS_UPTIME "[up [36m%dd %hh %mm[37m]"
#define DEFAULT_STATUS_WINDOW "[1;35m^^^^^[37;22m"

#else /* WANTANSI */

#define DEFAULT_STATUS_AWAY " (zZzZ: %A)"
#define DEFAULT_STATUS_CHANNEL " on %C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_CLOCK "time is %T"
#define DEFAULT_STATUS_FORMAT "[%R] %* %B%H%A%S %W %F %>"
#define DEFAULT_STATUS_FORMAT1 "[%R] %*%@%N%#%B%H%C%+%A%Q%S%I%O %W %F %> [lj %3]"
#define DEFAULT_STATUS_FORMAT2 "%1 [lag %2] %M [g %6] [s %5] [%D:%E:%P:%L:%7:%8] %> %T"
#define DEFAULT_STATUS_FORMAT3 "%U [topic %9] %>"
#define DEFAULT_STATUS_HOLD " [..more..]"
#define DEFAULT_STATUS_HOLD_LINES " (%B)"
#define DEFAULT_STATUS_MAIL "[m %M]"
#define DEFAULT_STATUS_MODE " [+%+]"
#define DEFAULT_STATUS_NOTIFY " [a:%F]"
#define DEFAULT_STATUS_OPER "*"
#define DEFAULT_STATUS_OVERWRITE " (ow)"
#define DEFAULT_STATUS_QUERY " [Q/%Q]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_UMODE " (+%#)"
#define DEFAULT_STATUS_UPTIME "[up %dd %hh %mm]"
#define DEFAULT_STATUS_WINDOW "^^^^^"

#endif
/****************************************************************************/

/*
 * define this if you want to have the -l and -L command line
 * options.
 */

#define COMMAND_LINE_L

#endif /* __config_h_ */
