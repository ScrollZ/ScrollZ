/*
 * irc.h: header file for all of ircII! 
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
 * $Id: irc.h,v 1.2 1998-10-31 18:26:01 f Exp $
 */

#ifndef __irc_h
#define __irc_h

/**************************** PATCHED by Flier ******************************/
/*#define IRCRC_NAME "/.ircrc"*/
#ifdef __MSDOS__
#define IRCRC_NAME "/scrollz.rc"
#else
#define IRCRC_NAME "/.scrollzrc"
#endif
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
#define IRCII_COMMENT   "Feel the power!"
#endif
/****************************************************************************/

/* these define what characters do, inverse, underline, bold and all off */
#define REV_TOG		'\026'		/* ^V */
#define UND_TOG		'\037'		/* ^_ */
#define BOLD_TOG	'\002'		/* ^B */
#define ALL_OFF		'\017'		/* ^O */

#define IRCD_BUFFER_SIZE	1024
#define BIG_BUFFER_SIZE		(IRCD_BUFFER_SIZE * 4)

#ifndef INPUT_BUFFER_SIZE
/**************************** PATCHED by Flier ******************************/
/*#define INPUT_BUFFER_SIZE	(IRCD_BUFFER_SIZE / 4)*/
#define INPUT_BUFFER_SIZE	IRCD_BUFFER_SIZE
/****************************************************************************/
#endif

#include "struct.h"

#ifdef notdef
# define DAEMON_UID 1
#endif

/**************************** PATCHED by Flier ******************************/
/*#define NICKNAME_LEN 9*/
#define NICKNAME_LEN 15
/****************************************************************************/
#define NAME_LEN 80
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
#if (defined(linux) || defined(__BORLANDC__)) && !defined(X_OK)
# define X_OK  1
# ifdef __BORLANDC__
#  define F_OK 0
#  define W_OK 2
#  define R_OK 4
# endif
#endif /* linux */

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

/*
 (* Some defines in case we're using IRCII setuid->root, so that we can bind
 * to privileged ports. This will allow future IRC nets to verify the true
 * identity of users much more reliably.
 */
#ifdef PRIV_PORT
# define	open		ruid_open
# define	fopen		ruid_fopen
# define	system		ruid_system
# define	unlink		ruid_unlink
# define	stat_file	ruid_stat
#ifdef EOF	/* defined if stdio.h has been included */
FILE *ruid_fopen _(());
# endif /*EOF*/
int	ruid_open _(());
int	ruid_system _(());
int	ruid_unlink _(());
#else
# define stat_file stat
#endif /*PRIV_PORT*/

#ifndef _Windows
# define FAR
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
extern	char	*joined_nick;
extern	char	*public_nick;
extern	char	empty_string[];
extern	char	*zero;
extern	char	*one;

extern	char	irc_version[];
extern	char	internal_version[];
extern	char	FAR buffer[];
extern	char	FAR nickname[];
extern	char	*ircrc_file;
extern	char	FAR hostname[];
extern	char	FAR realname[];
extern	char	FAR username[];
extern	char	*send_umode;
extern	char	*last_notify_nick;
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
extern	char	global_all_off[];
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
extern	struct	in_addr	local_ip_address;

/*
 * XXX some of these should move to a new notice.h
 */
	int	irc_io _((char *, void (*)(unsigned char, char*), int, int));
	void	new_stty _((char *));
	int	wild_match _((char *, char *));
	int	is_channel _((char *));
/**************************** PATCHED by Flier ******************************/
        /*RETSIGTYPE	irc_exit _((void));*/
        RETSIGTYPE	irc_exit _((int));
/****************************************************************************/
	void	beep_em _((int));
	void	got_initial_version _((char *));
	void	load_ircrc _((void));
	void	parse_notice _((char *, char **));
	void	irc_quit _((unsigned char, char *));

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

/**************************** PATCHED by Flier ******************************/
#if defined(_Windows) || defined(SZ32)
/****************************************************************************/
#define IS_FULL_PATH(x) (x[0] == '/' || x[0] == '\\' || (x[0] && x[1] == ':'))
#else
#define IS_FULL_PATH(x) (x[0] == '/')
#endif


#endif /* __irc_h */
