/*
 * server.h: header for server.c 
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
 * $Id: server.h,v 1.18 2007-03-30 15:27:36 f Exp $
 */

#ifndef __server_h_
#define __server_h_
  
/* for ChannelList */
#include "names.h"
/**************************** Patched by Flier ******************************/
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
#include "myssl.h"
#endif
/****************************************************************************/

/*
 * type definition to distinguish different
 * server versions
 */
#define Server2_5	0
#define Server2_6	1
#define Server2_7	2
#define Server2_8	3
#define Server2_9	4
#define Server2_10	5
#define Server2_11	6
#define Server2_12	7
#define Server2_90	90

/* Server: a structure for the server_list */
typedef	struct
{
	char	*name;			/* the name of the server */
	char	*itsname;		/* the server's idea of its name */
	char	*password;		/* password for that server */
	int	port;			/* port number on that server */
	char	*nickname;		/* nickname for this server */
	char	*away;			/* away message for this server */
	int	operator;		/* true if operator */
	int	version;		/* the version of the server -
					 * defined above */
	char	*version_string;	/* what is says */
	int	whois;			/* true if server sends numeric 318 */
        int	flags;			/* Various flags */
/**************************** PATCHED by Flier ******************************/
        int     umodeflags;             /* holds usermode for lowercase modes */
        int     umodeflags2;            /* holds usermode for uppercase modes */
/****************************************************************************/
	int	connected;		/* true if connection is assured */
	int	write;			/* write descriptor */
	int	read;			/* read descriptior */
	pid_t	pid;			/* process id of server */
	int	eof;			/* eof flag for server */
	int	motd;			/* motd flag (used in notice.c) */
	int	sent;			/* set if something has been sent,
					 * used for redirect */
	char	*buffer;		/* buffer of what dgets() doesn't get */
	WhoisQueue	*WQ_head;	/* WHOIS Queue head */
	WhoisQueue	*WQ_tail;	/* WHOIS Queue tail */
	WhoisStuff	whois_stuff;	/* Whois Queue current collection buffer */
	int	close_serv;		/* Server to close when we're LOGGED_IN */
	time_t	ctcp_last_reply_time;	/* used to limit flooding */
	time_t  ctcp_flood_time;
	int	ctcp_backlog_size;
        int     *ctcp_send_size;
	struct in_addr local_addr;	/* ip address of this connection */
	ChannelList	*chan_list;	/* list of channels for this server */
	void	(*parse_server) _((char *));	/* pointer to parser for this server */
/**************************** PATCHED by Flier ******************************/
        int     SZWI;                   /* when doing whois */
        int     SZWho;                  /* when doing who */
        int     SZUnban;                /* when doing unban */
        char    *LastMessage;           /* last received message */
        char    *LastNotice;            /* last received notice */
        char    *LastMessageSent;       /* last sent message */
        char    *LastNoticeSent;        /* last sent notice */
	char    *LastJoin;              /* last person to join */
        time_t  ConnectTime;            /* when the server was connected */
        struct  nicks *arlist,*arcur;   /* auto-reply list */
        struct  nicks *nicklist,        /* tabkey list */
                      *nickcur;
	ChannelList *ChanPendingList;   /* list of channels pending for join */
        void    *compl_last;            /* pointer to last completion string */
        void    *compl_next;            /* where to start next completion */
        ChannelList *compl_channel;     /* last channel completion */

#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
        int        enable_ssl;
#if defined(HAVE_SSL)
        gnutls_session session;
        gnutls_certificate_credentials xcred;
#elif defined(HAVE_OPENSSL)
        SSL        *ssl_fd;
        SSL_CTX    *ctx;
        SSL_METHOD *meth;
#endif
#endif
/****************************************************************************/
}	Server;

typedef struct ser_group_list
{
	struct ser_group_list	*next;
	char	*name;
	int	number;
}	SGroup;

typedef	unsigned	short	ServerType;

	int	find_server_group _((char *, int));
	char *	find_server_group_name _((int));
	void	add_to_server_list _((char *, int, char *, char *, int));
	void	build_server_list _((char *));
	int	connect_to_server _((char *, int, char *, int));
/**************************** PATCHED by Flier ******************************/
	/*void	get_connected _((int));*/
	void	get_connected _((int, int));
/****************************************************************************/
	int	read_server_file _((void));
	void	display_server_list _((void));
	void	do_server _((fd_set *, fd_set *));
#ifdef HAVE_STDARG_H
	void	send_to_server _((char *, ...));
#else
	void	send_to_server _(());
#endif /* HAVE_STDARG_H */
	int	get_server_whois _((int));

	WhoisStuff	*get_server_whois_stuff _((int));
	WhoisQueue	*get_server_qhead _((int));
	WhoisQueue	*get_server_qtail _((int));

extern	int	save_chan_from;	/* to keep the channel list if all servers
				 * are lost */

extern	int	attempting_to_connect;
extern	int	number_of_servers;
extern	int	connected_to_server;
extern	int	never_connected;
extern	int	using_server_process;
extern	int	primary_server;
extern	int	from_server;
extern	char	*connect_next_nick;
extern	char	*connect_next_password;
extern	int	parsing_server_index;
extern	SGroup	*server_group_list;

	void	servercmd _((char *, char *, char *));
	char	*get_server_nickname _((int));
	char	*get_server_name _((int));
	char	*get_server_itsname _((int));
	void	set_server_flag _((int, int, int));
	int	find_in_server_list _((char *, int, char *));
	char	*create_server_list _((void));
	void	remove_from_server_list _((int));
	void	set_server_motd _((int, int));
	int	get_server_motd _((int));
	int	get_server_operator _((int));
	int	get_server_2_6_2 _((int));
	int	get_server_version _((int));
	char	*get_server_password _((int));
	void	close_server _((int, char *));
	void	MarkAllAway _((char *, char *));
	int	is_server_connected _((int));
	void	flush_server _((void));
        int	get_server_flag _((int, int));
/**************************** PATCHED by Flier ******************************/
	int	get_server_umode_flag _((int, char));
	void    set_server_umode_flag _((int, char, int));
/****************************************************************************/
	void	set_server_operator _((int, int));
	void	server_is_connected _((int, int));
	int	parse_server_index _((char *));
	void	parse_server_info _((char **, char **, char **, char **));
	void	set_server_bits _((fd_set *, fd_set *));
	void	set_server_itsname _((int, char *));
	void	set_server_version _((int, int));
	int	is_server_open _((int));
/**************************** PATCHED by Flier ******************************/
        int     is_server_valid _((int));
/****************************************************************************/
	int	get_server_port _((int));
	char	*set_server_password _((int, char *));
	void	set_server_nickname _((int, char *));
	void	set_server_2_6_2 _((int, int));
	void	set_server_qhead _((int, WhoisQueue *));
	void	set_server_qtail _((int, WhoisQueue *));
	void	set_server_whois _((int, int));
	void	close_all_server _((void));
	void	disconnectcmd _((char *, char *, char *));
	void	ctcp_reply_backlog_change _((int));

	/* server_list: the list of servers that the user can connect to,etc */
	extern	Server	*server_list;

#define	SERVER_2_6_2	0x0100
#define CLOSE_PENDING	0x0200	/* set for servers who are being switched away from, but have not yet connected. */
#define LOGGED_IN	0x0400
#define	CLEAR_PENDING	0x0800	/* set for servers whose channels are to be removed when a connect has been established. */
/**************************** Patched by Flier ******************************/
#define SSL_CONNECT     0x100000
/****************************************************************************/

#endif /* __server_h_ */
