/*
 * irc.h: header file for all of ircII! 
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
 * $Id: irc.h,v 1.23 2009-12-21 15:08:26 f Exp $
 */

#ifndef __irc_h
#define __irc_h

/**************************** PATCHED by Flier ******************************/
/*#define IRCRC_NAME "/.ircrc"*/
#define IRCRC_NAME "/.scrollzrc"
/*#define IRCQUICK_NAME "/.ircquick"*/
#define IRCQUICK_NAME "/.scrollzquick"
/****************************************************************************/

/*
 * Here you can set the in-line quote character, normally backslash, to
 * whatever you want.  Note that we use two backslashes since a backslash is
 * also C's quote character.  You do not need two of any other character.
 */
#define QUOTE_CHAR '\\'

#if defined(ISC30)		/* for some reason it doesn't get defined */
# define _POSIX_SOURCE
#endif /* ISC30 */

#include "defs.h"
#include "config.h"
#ifdef NeXT
#   include <libc.h>
#endif
#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#ifdef _Windows
#include <winsock.h>
#include <signal.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#ifndef WINS
#include <netinet/in.h>
#else
#include <sys/twg_config.h>
#include <sys/in.h>
#undef server
#endif /* WINS */
#include <arpa/inet.h>
#include <signal.h>
#include <sys/param.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif /* HAVE_FCNTL_H */

/* machines we don't want to use <unistd.h> on 'cause its broken */
#if defined(pyr) || defined(_SEQUENT_)
# undef HAVE_UNISTD_H
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef HAVE_PROCESS_H
# include <process.h>
#endif

#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif

#ifdef HAVE_STDARG_H
# ifdef __STDC__
#  include <stdarg.h>
# else
#  undef HAVE_STDARG_H
# endif
#endif

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

#include "irc_std.h"
#include "debug.h"

/**************************** PATCHED by Flier ******************************/
#ifdef CELE
#include "celerity.h"
#endif

/*#define IRCII_COMMENT   "this is a bug free client.  honest"*/
#ifdef CELE
#define IRCII_COMMENT   "Need for Speed!"
#else
#define IRCII_COMMENT   "Year of the monkey"
#endif
/****************************************************************************/

/* these define what characters do, inverse, underline, bold and all off */
#define REV_TOG		'\026'		/* ^V */
#define UND_TOG		'\037'		/* ^_ */
#define BOLD_TOG	'\002'		/* ^B */
#define ALL_OFF		'\017'		/* ^O */
#define FULL_OFF	'\004'		/* internal, should be different than all others */

#define IRCD_BUFFER_SIZE	1024
#define BIG_BUFFER_SIZE		(IRCD_BUFFER_SIZE * 4)

#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE	1536
/* INPUT_BUFFER_SIZE:
   irc servers generally accept 512 bytes of input per line.
   Assuming the shortest considerable command begins
   with "PRIVMSG x :" and ends with a linefeed, this
   leaves 500 bytes of space for text.
   In dcc-chats, the line length is unlimited.
   In ircII, the input buffer is utf-8 encoded. This means
   that certain european characters take two bytes of space
   and most asian characters take three bytes of space.
   ( Reference: http://www.utf-8.com/ )
   However, in some encodings that are used in IRC
   (such as ISO-8859-1 and SHIFT-JIS), some to most
   of those characters only take 1 byte of space.
   ( References: http://en.wikipedia.com/wiki/ISO_8859-1
     http://en.wikipeda.com/wiki/SJIS )
   Therefore, to be able to send a full 500-character line
   regardless of what characters it contains, we need 1500
   bytes of free buffer space.
   We also need room for the command used to send the message
   - for example, "/msg bisqwit,#nipponfever,#gurb ".
   For that space, we use an arbitrarily chosen number
   that is bigger than 20.
   As a result, INPUT_BUFFER_SIZE is now set to 1536 (0x600).
                         - Bisqwit
 */
#endif /* INPUT_BUFFER_SIZE */

#include "struct.h"

#ifdef notdef
# define DAEMON_UID 1
#endif

#if 0 /* blundernet */
#define NICKNAME_LEN 9
#endif
#define NAME_LEN 255
#define REALNAME_LEN 50
#define PATH_LEN 1024

#if defined(__hpux) || defined(hpux) || defined(_HPUX_SOURCE)
# undef HPUX
# define HPUX
# ifndef HPUX7
#  define killpg(pgrp,sig) kill(-pgrp,sig)
# endif
#endif

#if defined(__sgi)
# define USE_TERMIO
#endif /* __sgi */

#ifdef DGUX
# define USE_TERMIO
# define inet_addr(x) inet_network(x)	/* dgux lossage */
#endif /* DGUX */

/*
 * Lame Linux doesn't define X_OK in a non-broken header file, so
 * we define it here.. 
 */
#if !defined(X_OK)
# define X_OK  1
#endif

#ifdef __BORLANDC__
# define F_OK 0
# define W_OK 2
# define R_OK 4
#endif

#ifdef __osf__
# if __osf__
#  define _BSD
# endif
#endif

#if defined(UNICOS) && !defined(USE_TERMIO)
# define USE_TERMIO
#endif /* UNICOS */

/* systems without getwd() can lose, if this dies */
#if defined(NEED_GETCWD)
# define getcwd(b, c)	getwd(b);
#endif

#if defined(ISC22) || defined(ISC30)
# define USE_TERMIO
# define ISC
#endif /* ISC22 || ISC30 */

#if defined(_AUX_SOURCE) && !defined(USE_TERMIO)
# define USE_TERMIO
#endif

#ifdef MAIL_DIR
# undef UNIX_MAIL
# define UNIX_MAIL MAIL_DIR
#endif

#ifndef MIN
# define MIN(a,b) ((a < b) ? (a) : (b))
#endif

#ifndef MAX
# define MAX(a,b) ((a < b) ? (b) : (a))
#endif /* MAX */

/* flags used by who() and whoreply() for who_mask */
#define WHO_OPS		0x0001
#define WHO_NAME	0x0002
#define WHO_ZERO	0x0004
#define WHO_CHOPS	0x0008
#define WHO_FILE	0x0010
#define WHO_HOST	0x0020
#define WHO_SERVER	0x0040
#define	WHO_HERE	0x0080
#define	WHO_AWAY	0x0100
#define	WHO_NICK	0x0200
#define	WHO_LUSERS	0x0400
#define	WHO_REAL	0x0800
/**************************** PATCHED by Flier ******************************/
#define WHO_HOPS        0x1000
#define WHO_SHOW_SERVER 0x8000
/****************************************************************************/
#define WHO_NONCHOPS	0x10000

#ifdef ICONV_CONST_ARG2
#define iconv_const const
#else
#define iconv_const
#endif

/*
 * declared in irc.c 
 */
extern	char	*cut_buffer;
extern	char	oper_command;
extern	int	irc_port;
extern	int	send_text_flag;
extern	int	irc_io_loop;
extern	int	break_io_processing;
extern	int	use_flow_control;
extern	u_char	*joined_nick;
extern	u_char	*public_nick;
extern	char	empty_string[];
extern	char	*irczero;
extern	char	*one;

extern	char	irc_version[];
extern	char	*nickname;
extern	char	*ircrc_file;
extern	char	*ircquick_file;
extern	char	hostname[];
extern	char	realname[];
extern	char	username[];
extern	char	*send_umode;
extern	u_char	*last_notify_nick;
extern	int	away_set;
extern	int	background;
extern	char	*my_path;
extern	char	*irc_path;
extern	char	*irc_lib;
extern	char	*args_str;
extern	char	*invite_channel;
extern	int	who_mask;
extern	char	*who_name;
extern	char	*who_host;
extern	char	*who_server;
extern	char	*who_file;
extern	char	*who_nick;
extern	char	*who_real;
extern	char	*cannot_open;
extern	int	dumb;
extern	int	use_input;
extern	time_t	idle_time;
extern	int	waiting;
extern	char	wait_nick[];
extern	char	whois_nick[];
extern	char	lame_wait_nick[];
extern	char	**environ;
extern	int	current_numeric;
extern	int	qflag;
extern	int	bflag;
extern	int	tflag;
extern	struct	in_addr	local_ip_address;

/*
 * XXX some of these should move to a new notice.h
 */
 	int	irc_io _((char *, void (*)(u_int, char*), int, int));
 	void	set_irchost _((void));
	int	wild_match _((char *, char *));
/**************************** PATCHED by Flier ******************************/
        /*RETSIGTYPE	irc_exit _((void));*/
        RETSIGTYPE	irc_exit _((int));
/****************************************************************************/
	void	beep_em _((int));
	void	got_initial_version _((char *));
	void	maybe_load_ircrc _((void));
	void	load_ircrc _((void));
 	void	load_ircquick _((void));
	void	parse_notice _((char *, char **));
 	void	irc_quit _((u_int, char *));

typedef	struct	WhoisStuffStru
{
	char	*nick;
	char	*user;
	char	*host;
	char	*channel;
	char	*channels;
	char	*name;
	char	*server;
	char	*server_stuff;
	char	*away;
	int	oper;
	int	chop;
	int	not_on;
}	WhoisStuff;

/* Moved into here, because some weird CC's can't do (void *) */
typedef	struct	WhoisQueueStru
{
	char	*nick;			/* nickname of whois'ed person(s) */
	char	*text;			/* additional text */
	int	type;			/* Type of WHOIS queue entry */
	/*
	 * called with func((WhoisStuff *)stuff,(char *) nick, (char *) text) 
	 */
	void	 (*func) _((WhoisStuff *, char *, char *));
	struct	WhoisQueueStru	*next;/* next element in queue */
}	WhoisQueue;

char	*getenv _((const char *));
#ifdef _Windows
typedef long off_t;
extern char *get_path(int iVal);
#endif

#ifdef _Windows
#define	define_big_buffer(x)	char *x = (char *) new_malloc(BIG_BUFFER_SIZE + 1)
#define	free_big_buffer(x)	new_free(&x)
#else
#define	define_big_buffer(x)	char x[BIG_BUFFER_SIZE + 1]
#define	free_big_buffer(x)	(0)
#endif

#if defined(_Windows)
#define IS_FULL_PATH(x) (x[0] == '/' || x[0] == '\\' || (x[0] && x[1] == ':'))
#else
#define IS_FULL_PATH(x) (x[0] == '/')
#endif


#endif /* __irc_h */
