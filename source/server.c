/*
 * server.c: Things dealing with server connections, etc. 
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
 * $Id: server.c,v 1.25 2000-08-27 18:01:56 f Exp $
 */

#include "irc.h"

#ifdef ESIX
# include <lan/net_types.h>
#endif /* ESIX */

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>

int	connect_to_unix _((int, char *));
#endif /* HAVE_SYS_UN_H */

#include "server.h"
#include "screen.h"
#include "ircaux.h"
#include "whois.h"
#include "lastlog.h"
#include "exec.h"
#include "window.h"
#include "output.h"
#include "names.h"
#include "parse.h"
#include "list.h"
#include "newio.h"
#include "vars.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

extern void HandleClosedConn _((int, char *));
extern int  CheckServer _((int));

/*#define BNCCRYPT 1*/

#ifdef BNCCRYPT
static char enctablebnc[256];
static char dectablebnc[256];
#endif
/****************************************************************************/

static	void	add_to_server_buffer _((int, char *));
static	void	login_to_server _((int));
static	int	connect_to_server_direct _((char *, int, char *));
static	int	connect_to_server_process _((char *, int, char *));
static	void	irc2_login_to_server _((int));

/*
 * Don't want to start ircio by default...
 */
	int	using_server_process = 0;

/* server_list: the list of servers that the user can connect to,etc */
	Server	*server_list = (Server *) 0;

/* number_of_servers: in the server list */
	int	number_of_servers = 0;

/* server_group_list:  list of server groups */
	SGroup	*server_group_list = (SGroup *) 0;

extern	WhoisQueue	*WQ_head;
extern	WhoisQueue	*WQ_tail;

	int	primary_server = -1;
	int	from_server = -1;
	int	attempting_to_connect = 0;
	int	never_connected = 1;		/* true until first connection
						 * is made */
	int	connected_to_server = 0;	/* true when connection is
						 * confirmed */
	char	*connect_next_nick;
	char	*connect_next_password;
	int	parsing_server_index = -1;

extern	int	dgets_errno;

#define DEFAULT_SERVER_VER Server2_8

/*
 * close_server: Given an index into the server list, this closes the
 * connection to the corresponding server.  It does no checking on the
 * validity of the index.  It also first sends a "QUIT" to the server being
 * closed 
 */
void
close_server(server_index, message)
	int	server_index;
	char	*message;
{
	char	buffer[BIG_BUFFER_SIZE+1];
	int	i,
		min,
		max;

	if (server_index == -1)
	{
		min = 0;
		max = number_of_servers;
	}
	else
	{
		min = server_index;
		max = server_index + 1;
	}
	for (i = min; i < max; i++)
	{
		int	old_server = from_server;

		if (server_list[i].flags & CLOSE_PENDING)
			continue;
		if (waiting)
			irc_io_loop = 0;
		if (i == primary_server)
			clean_whois_queue();

		from_server = -1;
		mark_not_connected(i);
		from_server = old_server;

		server_list[i].operator = 0;
		server_list[i].connected = 0;
		server_list[i].buffer = (char *) 0;
 		server_list[i].flags = SERVER_2_6_2;
/**************************** PATCHED by Flier ******************************/
 		server_list[i].umodeflags=0;
/****************************************************************************/
		if (-1 != server_list[i].write)
		{
			if (message && *message)
			{
				sprintf(buffer, "QUIT :%s\n", message);
				send(server_list[i].write, buffer, strlen(buffer), 0);
			}
			new_close(server_list[i].write);
			if (server_list[i].write == server_list[i].read)
				server_list[i].read = -1;
			server_list[i].write = -1;
		}
		if (-1 != server_list[i].read)
		{
			new_close(server_list[i].read);
			server_list[i].read = -1;
		}
#ifndef _Windows
		if (-1 != server_list[i].pid)
		{
			kill(server_list[i].pid, SIGKILL);
			server_list[i].pid = (pid_t) -1;
		}
#endif /* _Windows */
	}
}

/*
 * set_server_bits: Sets the proper bits in the fd_set structure according to
 * which servers in the server list have currently active read descriptors.  
 */

void
set_server_bits(rd, wd)
	fd_set *rd, *wd;
{
	int	i;

	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].read != -1)
			FD_SET(server_list[i].read, rd);
#ifdef NON_BLOCKING_CONNECTS
		if (!(server_list[i].flags & (LOGGED_IN|CLOSE_PENDING)) &&
		    server_list[i].write != -1)
			FD_SET(server_list[i].write, wd);
#endif
	}
}

/**************************** PATCHED by Flier ******************************/
#ifdef BNCCRYPT
void DecryptBNC(buffer,buflen)
char *buffer;
int  buflen;
{
    int i;

    for (i=0;i<buflen;i++) buffer[i]=dectablebnc[(unsigned char) buffer[i]];
}

void EncryptBNC(buffer,buflen)
char *buffer;
int  buflen;
{
    int i;

    for (i=0;i<buflen;i++) buffer[i]=enctablebnc[(unsigned char) buffer[i]];
}
#endif /* BNCCRYPT */

/****************************************************************************/
/*
 * do_server: check the given fd_set against the currently open servers in
 * the server list.  If one have information available to be read, it is read
 * and and parsed appropriately.  If an EOF is detected from an open server,
 * one of two things occurs. 1) If the server was the primary server,
 * get_connected() is called to maintain the connection status of the user.
 * 2) If the server wasn't a primary server, connect_to_server() is called to
 * try to keep that connection alive. 
 */
void
do_server(rd, wd)
	fd_set	*rd, *wd;
{
	char	lbuf[BIG_BUFFER_SIZE + 1];
	int	des,
		j;
	static	int	times = 0;
	int	old_timeout;
/**************************** PATCHED by Flier ******************************/
        char tmpbuf[mybufsize/4];
/****************************************************************************/

	for (j = 0; j < number_of_servers && !break_io_processing; j++)
	{
#ifdef NON_BLOCKING_CONNECTS
		/*
		 *	deraadt@theos.com suggests that every fd awaiting connection
		 *	should be run at this point.
		 */
		if ((des = server_list[j].write) != -1 && /*FD_ISSET(des, wd) && */
		    !(server_list[j].flags & LOGGED_IN)) {
			struct sockaddr_in sa;
			int salen = sizeof(struct sockaddr_in);

			if (getpeername(server_list[j].write, (struct sockaddr *) &sa, &salen) != -1)
				login_to_server((from_server = j));
		}
#endif
		if ((des = server_list[j].read) != -1 && FD_ISSET(des, rd))
		{
			int	junk;
			char 	*bufptr;
			char	*s;
			int	i = j;
			size_t	len;

			from_server = i;
			old_timeout = dgets_timeout(1);
			s = server_list[from_server].buffer;
 			bufptr = lbuf;
			if (s && *s)
			{
				len = strlen(s);
 				strncpy(lbuf, s, len);
				bufptr += len;
			}
			else
				len = 0;
			if (len >= BIG_BUFFER_SIZE)
				goto buffer_is_full_hack;	/* XXX? */
/**************************** PATCHED by Flier ******************************/
			/*junk = dgets(bufptr, BIG_BUFFER_SIZE, des, (char *) 0);*/
#ifdef BNCCRYPT
			junk = dgets(bufptr, BIG_BUFFER_SIZE, des, (char *) 0, 1);
#else
			junk = dgets(bufptr, BIG_BUFFER_SIZE, des, (char *) 0, 0);
#endif
/****************************************************************************/
			(void) dgets_timeout(old_timeout);
			switch (junk)
			{
			case -1:
 				add_to_server_buffer(from_server, lbuf);
				goto real_continue;
			case 0:
			{
#ifdef NON_BLOCKING_CONNECTS
				int	old_serv = server_list[i].close_serv;
			/* Get this here before close_server() clears it -Sol */
				int	logged_in = server_list[i].flags & LOGGED_IN;
#endif /* NON_BLOCKING_CONNECTS */

				close_server(i, empty_string);
/**************************** Patched by Flier ******************************/
                                /* to force window activity */
                                save_message_from();
                                message_from((char *) 0,LOG_CRAP);
                                /* bug when server closed connection before
                                   we connected - in that case client reported
                                   non standard version which is wrong */
                                if (attempting_to_connect>0)
                                    attempting_to_connect--;
/****************************************************************************/
				say("Connection closed from %s: %s", server_list[i].name,
					dgets_errno == -1 ? "Remote end closed connection" : strerror(dgets_errno));
/**************************** Patched by Flier ******************************/
                                restore_message_from();
				sprintf(tmpbuf,"Connection closed from %s: %s",server_list[i].name,
					dgets_errno == -1?"Remote end closed connection":strerror(dgets_errno));
                                HandleClosedConn(i,tmpbuf);
/****************************************************************************/
				server_list[i].connected = 0;
				server_list[i].read = server_list[i].write = -1;
#ifdef NON_BLOCKING_CONNECTS
				if (!logged_in)
				{
					if (old_serv == i)	/* a hack?  you bet */
						goto a_hack;
					if (old_serv != -1 && (server_list[old_serv].flags & CLOSE_PENDING))
					{
						say("Connection to server %s resumed...", server_list[old_serv].name);
/**************************** PATCHED by Flier ******************************/
                                                /* moved below because we need
                                                 * it in get_connected() to
                                                 * transfer channels when
                                                 * connection fails */
						/*server_list[i].close_serv = -1;*/
/****************************************************************************/
						server_list[old_serv].flags &= ~(CLOSE_PENDING|CLEAR_PENDING);
						server_list[old_serv].flags |= LOGGED_IN;
						server_list[old_serv].connected = 1;
#if 1
						get_connected(old_serv);
#else
						/* get_connected(old_serv);
						   should be used only for
						   primary_server -Sol */
						/* connect_to_server(server_list[old_serv].name, server_list[old_serv].port, server_list[server].nickname, -1);
						   actually, do we have to
						   reconnect at all ? -Sol */
						{ /* Reassign windows ? -Sol */
						Window	*tmp;
						int	flag = 1;

						while ((tmp = traverse_all_windows(&flag)) != (Window *) 0)
							if (tmp->server == i)
							{
								window_set_server(tmp->refnum, old_serv, WIN_ALL);
								break;
							}
						}
#endif /* 1 */
					}
/**************************** PATCHED by Flier ******************************/
                                        /* see comment above! */
                                        server_list[i].close_serv = -1;
                                        if (curr_scr_win->server==old_serv) {
                                            is_current_channel(NULL,old_serv,
                                                               curr_scr_win->refnum);
                                        }
/****************************************************************************/
					window_check_servers();
					break;
				}
a_hack:
#endif /* NON_BLOCKING_CONNECTS */
				if (i == primary_server)
				{
					if (server_list[i].eof)
					{
						say("Unable to connect to server %s",
							server_list[i].name);
						if (++i == number_of_servers)
  						{
							clean_whois_queue();
							window_check_servers();
							if (!connected_to_server)
								say("Use /SERVER to connect to a server");
							times = 0;
						}
						else
							get_connected(i);
					}
					else
					{
						if (times++ > 1)
						{
							clean_whois_queue();
							window_check_servers();
							if (!connected_to_server);
								say("Use /SERVER to connect to a server");
							times = 0;
  						}
						else
							get_connected(i);
					}
				}
				else if (server_list[i].eof)
				{
					say("Connection to server %s lost.", server_list[i].name);
					clean_whois_queue();
					window_check_servers();
				}
				else if (connect_to_server(server_list[i].name, server_list[i].port, server_list[i].nickname, -1))
				{
					say("Connection to server %s lost.", server_list[i].name);
					clean_whois_queue();
					window_check_servers();
				}
				server_list[i].eof = 1;
				break;
			}
			default:
buffer_is_full_hack:
 				{
 					int	old_psi = parsing_server_index;

  					parsing_server_index = i;
 					server_list[parsing_server_index].parse_server(lbuf);
  					new_free(&server_list[i].buffer);
 					parsing_server_index = old_psi;
  					break;
  				}
                        }
real_continue:
			from_server = primary_server;
		}
	}
}

/*
 * find_in_server_list: given a server name, this tries to match it against
 * names in the server list, returning the index into the list if found, or
 * -1 if not found 
 */
extern	int
find_in_server_list(server, port, nick)
	char	*server;
	int	port;
	char	*nick;
{
	int	i, maybe = -1;
	size_t	len;

	len = strlen(server);
	for (i = 0; i < number_of_servers; i++)
	{
		if (port && server_list[i].port &&
		    port != server_list[i].port)
			continue;

		if (my_strnicmp(server, server_list[i].name, len) != 0)
			continue;

		if (nick)
		{
			if (server_list[i].nickname == NULL)
			{
				maybe = i;
				continue;
			}
			if (my_stricmp(server_list[i].nickname, nick))
				continue;
		}
		maybe = i;
		break;
	}
	return (maybe);
}

/*
 * parse_server_index:  given a string, this checks if it's a number, and if
 * so checks it validity as a server index.  Otherwise -1 is returned 
 */
int
parse_server_index(str)
	char	*str;
{
	int	i;

	if (is_number(str))
	{
		i = atoi(str);
		if ((i >= 0) && (i < number_of_servers))
			return (i);
	}
	return (-1);
}

/*
 * add_to_server_list: adds the given server to the server_list.  If the
 * server is already in the server list it is not re-added... however, if the
 * overwrite flag is true, the port and passwords are updated to the values
 * passes.  If the server is not on the list, it is added to the end. In
 * either case, the server is made the current server. 
 */
void
add_to_server_list(server, port, password, nick, overwrite)
	char	*server;
	int	port;
	char	*password;
	char	*nick;
	int	overwrite;
{
	int	i;

/**************************** PATCHED by Flier ******************************/
#ifdef BNCCRYPT
        /* Decryption table */
        for (i=0;i<32;i++) dectablebnc[i]=i;
        for (i=123;i<156;i++) dectablebnc[i]=i-91;
        for (i=198;i<256;i++) dectablebnc[i]=320-i;
        for (i=32;i<65;i++) dectablebnc[i]=i+91;
        for (i=157;i<198;i++) dectablebnc[i]=i;
        for (i=65;i<123;i++) dectablebnc[i]=i+133;
        /* Encryption table */
        for (i=0;i<32;i++) enctablebnc[i]=i;
        for (i=123;i<156;i++) enctablebnc[i-91]=i;
        for (i=198;i<256;i++) enctablebnc[320-i]=i;
        for (i=32;i<65;i++) enctablebnc[i+91]=i;
        for (i=157;i<198;i++) enctablebnc[i]=i;
        for (i=65;i<123;i++) enctablebnc[i+133]=i;
#endif
/****************************************************************************/
	if ((from_server = find_in_server_list(server, port, nick)) == -1)
	{
		from_server = number_of_servers++;
		if (server_list)
			server_list = (Server *) new_realloc((char *) server_list, number_of_servers * sizeof(Server));
		else
			server_list = (Server *) new_malloc(number_of_servers * sizeof(Server));
		server_list[from_server].name = (char *) 0;
		server_list[from_server].itsname = (char *) 0;
		server_list[from_server].password = (char *) 0;
		server_list[from_server].away = (char *) 0;
		server_list[from_server].version_string = (char *) 0;
		server_list[from_server].operator = 0;
		server_list[from_server].read = -1;
		server_list[from_server].write = -1;
		server_list[from_server].pid = -1;
		server_list[from_server].version = DEFAULT_SERVER_VER;	/* default */
		server_list[from_server].whois = 0;
		server_list[from_server].flags = SERVER_2_6_2;
/**************************** PATCHED by Flier ******************************/
                server_list[from_server].umodeflags=0;
                server_list[from_server].LastMessage=(char *) 0;
                server_list[from_server].LastNotice=(char *) 0;
                server_list[from_server].LastMessageSent=(char *) 0;
                server_list[from_server].LastNoticeSent=(char *) 0;
                server_list[from_server].LastJoin=(char *) 0;
                malloc_strcpy(&(server_list[from_server].LastJoin),"none yet");
                server_list[from_server].arcur=(struct nicks *) 0;
                server_list[from_server].arlist=(struct nicks *) 0;
                server_list[from_server].nickcur=(struct nicks *) 0;
                server_list[from_server].nicklist=(struct nicks *) 0;
/****************************************************************************/
		server_list[from_server].nickname = (char *) 0;
		server_list[from_server].connected = 0;
		server_list[from_server].eof = 0;
		server_list[from_server].motd = 1;
		server_list[from_server].chan_list = (ChannelList *) 0;
		malloc_strcpy(&(server_list[from_server].name), server);
		if (password && *password)
			malloc_strcpy(&(server_list[from_server].password),
				password);
		if (nick && *nick)
			malloc_strcpy(&(server_list[from_server].nickname),
				nick);
		server_list[from_server].port = port;
		server_list[from_server].WQ_head = (WhoisQueue *) 0;
		server_list[from_server].WQ_tail = (WhoisQueue *) 0;
		server_list[from_server].whois_stuff.nick = (char *) 0;
		server_list[from_server].whois_stuff.user = (char *) 0;
		server_list[from_server].whois_stuff.host = (char *) 0;
		server_list[from_server].whois_stuff.channel = (char *) 0;
		server_list[from_server].whois_stuff.channels = (char *) 0;
		server_list[from_server].whois_stuff.name = (char *) 0;
		server_list[from_server].whois_stuff.server = (char *) 0;
		server_list[from_server].whois_stuff.server_stuff = (char *) 0;
		server_list[from_server].whois_stuff.away = (char *) 0;
		server_list[from_server].whois_stuff.oper = 0;
		server_list[from_server].whois_stuff.chop = 0;
		server_list[from_server].whois_stuff.not_on = 0;
		server_list[from_server].buffer = (char *) 0;
		server_list[from_server].close_serv = -1;
		server_list[from_server].local_addr.s_addr = 0;
		server_list[from_server].parse_server = irc2_parse_server;
		server_list[from_server].ctcp_last_reply_time = 0;
		server_list[from_server].ctcp_flood_time = 0;
		server_list[from_server].ctcp_backlog_size = get_int_var(CTCP_REPLY_BACKLOG_SECONDS_VAR);
		server_list[from_server].ctcp_send_size =
			(int *)new_malloc(server_list[from_server].ctcp_backlog_size*sizeof(int));

		for(i = 0; i<server_list[from_server].ctcp_backlog_size; i++)
			server_list[from_server].ctcp_send_size[i] = 0;
	}
	else
	{
		if (overwrite)
		{
			server_list[from_server].port = port;
			if (password || !server_list[from_server].password)
			{
				if (password && *password)
					malloc_strcpy(&(server_list[from_server].password), password);
				else
					new_free(&(server_list[from_server].password));
			}
			if (nick && *nick)
				malloc_strcpy(&(server_list[from_server].nickname), nick);
		}
		if ((int) strlen(server) > (int) strlen(server_list[from_server].name))
			malloc_strcpy(&(server_list[from_server].name), server);
	}
}

extern  void
ctcp_reply_backlog_change(s)
	int	s;
{
	int	i, j, delta;

	if (s <= 0)
		s = 1;
	if (server_list)
	{
		for (i = 0; i < number_of_servers; i++)
		{
			delta = s - server_list[i].ctcp_backlog_size;

			if (delta)
			{
				server_list[i].ctcp_send_size =
					(int *)new_realloc((void *)(server_list[i].ctcp_send_size), s*sizeof(int));
				for(j = server_list[i].ctcp_backlog_size; j < s; j++)
					server_list[i].ctcp_send_size[j] = 0;
				server_list[i].ctcp_backlog_size = s;
			}
		}
	}
}

extern	void
remove_from_server_list(i)
	int	i;
{
	int	old_server = from_server,
		flag = 1;
	Window	*tmp;
/**************************** PATCHED by Flier ******************************/
        struct  nicks *tmpnick,*tmpnickfree;
/****************************************************************************/

	from_server = i;
	clean_whois_queue();
	from_server = old_server;

	close_server(i, (u_char *) 0);

	if (server_list[i].name)
		new_free(&server_list[i].name);
	if (server_list[i].itsname)
		new_free(&server_list[i].itsname);
	if (server_list[i].password)
		new_free(&server_list[i].password);
	if (server_list[i].away)
		new_free(&server_list[i].away);
	if (server_list[i].version_string)
		new_free(&server_list[i].version_string);
	if (server_list[i].nickname)
		new_free(&server_list[i].nickname);
	if (server_list[i].whois_stuff.nick)
		new_free(&server_list[i].whois_stuff.nick);
	if (server_list[i].whois_stuff.user)
		new_free(&server_list[i].whois_stuff.user);
	if (server_list[i].whois_stuff.host)
		new_free(&server_list[i].whois_stuff.host);
	if (server_list[i].whois_stuff.channel)
		new_free(&server_list[i].whois_stuff.channel);
	if (server_list[i].whois_stuff.channels)
		new_free(&server_list[i].whois_stuff.channels);
	if (server_list[i].whois_stuff.name)
		new_free(&server_list[i].whois_stuff.name);
	if (server_list[i].whois_stuff.server)
		new_free(&server_list[i].whois_stuff.server);
	if (server_list[i].whois_stuff.server_stuff)
		new_free(&server_list[i].whois_stuff.server_stuff);
	if (server_list[i].ctcp_send_size)
		new_free(&server_list[i].ctcp_send_size);
/**************************** PATCHED by Flier ******************************/
        if (server_list[i].LastMessage) new_free(&(server_list[i].LastMessage));
        if (server_list[i].LastNotice) new_free(&(server_list[i].LastNotice));
        if (server_list[i].LastMessageSent) new_free(&(server_list[i].LastMessageSent));
        if (server_list[i].LastNoticeSent) new_free(&(server_list[i].LastNoticeSent));
        if (server_list[i].LastJoin) new_free(&(server_list[i].LastJoin));
        for (tmpnick=server_list[i].arlist;tmpnick;) {
            tmpnickfree=tmpnick;
            tmpnick=tmpnick->next;
            new_free(&(tmpnickfree->nick));
            new_free(&tmpnickfree);
        }
        for (tmpnick=server_list[i].nicklist;tmpnick;) {
            tmpnickfree=tmpnick;
            tmpnick=tmpnick->next;
            new_free(&(tmpnickfree->nick));
            new_free(&tmpnickfree);
        }
/****************************************************************************/

	/* update all the structs with server in them */
	channel_server_delete(i);	/* fix `higher' servers */
	clear_channel_list(i);
#ifndef _Windows
	exec_server_delete(i);
#endif /* _Windows */
	if (i < primary_server)
		--primary_server;
	if (i < from_server)
		--from_server;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
		if (tmp->server > i)
			tmp->server--;

 	bcopy((char *) &server_list[i + 1], (char *) &server_list[i], (number_of_servers - i - 1) * sizeof(Server));
	server_list = (Server *) new_realloc((char *) server_list, --number_of_servers * sizeof(Server));
/**************************** PATCHED by Flier *******************************/
        if (number_of_servers==0) {
            new_free(&server_list);
            server_list=(Server *) 0;
        }
/****************************************************************************/
}

/*
 * parse_server_info:  This parses a single string of the form
 * "server:portnum:password:nickname".  It the points port to the portnum
 * portion and password to the password portion.  This chews up the original
 * string, so * upon return, name will only point the the name.  If portnum
 * or password are missing or empty,  their respective returned value will
 * point to null.  if extra is non NULL, it is set to anything after the
 * final : after the nickname..
 *
 * Note:  this will set connect_next_as_irc/connect_next_as_icb if it sees
 * the IRC/ or ICB/ at the start of the "name".
 */
void
parse_server_info(name, port, password, nick, extra)
	char	**name,
		**port,
		**password,
		**nick,
		**extra;
{
	char *ptr, *ename, *savename = (char *) 0;

	*port = *password = *nick = *extra = NULL;
	/* check for [i:p:v:6]:port style */
	if (**name == '[')
	{
		if ((ename = index((*name)+1, ']')))
		{
			*ename = '\0';
			savename = *name + 1;
			*name = ename + 1;	/* now points to empty or : we hope */
		}
 	}
	if ((ptr = (char *) index(*name, ':')) != NULL)
	{
		*(ptr++) = (char) 0;
		if (strlen(ptr) == 0)
			*port = (char *) 0;
		else
		{
			*port = ptr;
			if ((ptr = (char *) index(ptr, ':')) != NULL)
			{
				*(ptr++) = (char) 0;
				if (strlen(ptr) == 0)
					*password = '\0';
				else
				{
					*password = ptr;
					if ((ptr = (char *) index(ptr, ':'))
							!= NULL)
					{
						*(ptr++) = '\0';
						if (!strlen(ptr))
							*nick = NULL;
						else
						{
							*nick = ptr;
							if (extra && (ptr = (char *) index(ptr, ':'))
									!= NULL)
							{
								*(ptr++) = '\0';
								if (!strlen(ptr))
									*extra = NULL;
								else
									*extra = ptr;
							}
						}
					}
				}
			}
		}
	}
	if (savename)
		*name = savename;
}

/*
 * build_server_list: given a whitespace separated list of server names this
 * builds a list of those servers using add_to_server_list().  Since
 * add_to_server_list() is used to added each server specification, this can
 * be called many many times to add more servers to the server list.  Each
 * element in the server list case have one of the following forms: 
 *
 * servername 
 *
 * servername:port 
 *
 * servername:port:password 
 *
 * servername::password 
 *
 * Note also that this routine mucks around with the server string passed to it,
 * so make sure this is ok .
 *
 * A new format for ICB and more support is:
 *
 *	type/<type-specifc-format>
 *
 * eg:
 *	IRC/server:port:pass:nick:#foo:#bar:&baz
 * means connect to server on port port with pass and nick, and then to join
 * channels #foo, #bar and &baz.  this is not implemented beyond the nick...
 *
 * or
 *	ICB/server:port:pass:nick:group:mode
 * which is all the things needed at connection startup.  this is done.
 */
void
build_server_list(servers)
	char	*servers;
{
	char	*host,
		*rest,
		*extra,
		*password = (char *) 0,
		*port = (char *) 0,
		*nick = (char *) 0;
	int	port_num;

	if (servers == (char *) 0)
		return;
	while (servers)
	{
		if ((rest = (char *) index(servers, '\n')) != NULL)
			*rest++ = '\0';
		while ((host = next_arg(servers, &servers)) != NULL)
		{
			parse_server_info(&host, &port, &password, &nick, &extra);
			if (port && *port)
			{
				port_num = atoi(port);
				if (!port_num)
					port_num = irc_port;
			}
			else
				port_num = irc_port;
			add_to_server_list(host, port_num, password, nick, 0);
			if (extra)
			{
				/* nothing yet */
			}
		}
		servers = rest;
	}
}

/*
 * connect_to_server_direct: handles the tcp connection to a server.  If
 * successful, the user is disconnected from any previously connected server,
 * the new server is added to the server list, and the user is registered on
 * the new server.  If connection to the server is not successful,  the
 * reason for failure is displayed and the previous server connection is
 * resumed uniterrupted. 
 *
 * This version of connect_to_server() connects directly to a server 
 */
static	int
connect_to_server_direct(server_name, port, nick)
	char	*server_name;
	int	port;
	char	*nick;
{
	int	new_des;
#ifdef INET6
/*
	struct	sockaddr_storage	localaddr;
*/
/* For IPv4 only DCC */
	char	strlhost[1025];
	struct	addrinfo h, *r, *r0;
#else
	struct sockaddr_in localaddr;
	int	address_len;
#endif

	using_server_process = 0;
	oper_command = 0;
	errno = 0;
#ifdef HAVE_SYS_UN_H
	if (*server_name == '/')
		new_des = connect_to_unix(port, server_name);
	else
#endif /* HAVE_SYS_UN_H */
		new_des = connect_by_number(port, server_name, 1);
	if (new_des < 0)
	{
		char *e = NULL;
		switch (new_des)
		{
		default:
		case -2:
			e = "Unknown host";
			errno = 0;
			break;
		case -3:
			e = "socket";
			break;
		case -4:
			e = "connect";
			break;
		}
			
		say("Unable to connect to port %d of server %s: %s%s%s", port, server_name, e,
		    errno ? ": " : "", errno ? strerror(errno) : "");
		if (is_server_open(from_server))
			say("Connection to server %s resumed...", server_list[from_server].name);
		return (-1);
	}
#ifdef HAVE_SYS_UN_H
	if (*server_name != '/')
#endif /* HAVE_SYS_UN_H */
	{
#ifndef INET6
		address_len = sizeof(struct sockaddr_in);
		getsockname(new_des, (struct sockaddr *) &localaddr,
				&address_len);
#endif
	}
	update_all_status();
	add_to_server_list(server_name, port, (char *) 0, nick, 1);
	if (port)
	{
		server_list[from_server].read = new_des;
		server_list[from_server].write = new_des;
	}
	else
		server_list[from_server].read = new_des;
#ifdef INET6
/* DCC works _only_ via IPv4, so we put IPv4 address here */
	gethostname(strlhost, sizeof(strlhost));
	memset(&h, 0, sizeof(h));
	h.ai_family = AF_INET;
	h.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(strlhost, "0", &h, &r0) == 0)
	{
		struct sockaddr_in tmps;
		for (r = r0; r; r = r->ai_next) {
			memcpy(&tmps, r->ai_addr, r->ai_addrlen);
			server_list[from_server].local_addr.s_addr = tmps.sin_addr.s_addr;
			freeaddrinfo(r0);
			break;
		}
	}
#else
	server_list[from_server].local_addr.s_addr = localaddr.sin_addr.s_addr;
#endif
	server_list[from_server].operator = 0;
	return (0);
}

/*
 * connect_to_server_process: handles the tcp connection to a server.  If
 * successful, the user is disconnected from any previously connected server,
 * the new server is added to the server list, and the user is registered on
 * the new server.  If connection to the server is not successful,  the
 * reason for failure is displayed and the previous server connection is
 * resumed uniterrupted. 
 *
 * This version of connect_to_server() uses the ircio process to talk to a
 * server 
 */
static	int
connect_to_server_process(server_name, port, nick)
	char	*server_name;
	int	port;
	char	*nick;
{
#ifdef _Windows
	return -1;
#else
	int	write_des[2],
		read_des[2],
		pid,
		c;
	char	*path,
		*name = (char *) 0,
		*s;
	char	buffer[BIG_BUFFER_SIZE+1];
	int	old_timeout;

	path = IRCIO_PATH;
	if ((s = rindex(path, '/')) != NULL)
		malloc_strcpy(&name, s + 1);
	if (!name)
		name = path;
	if (*path == '\0')
		return (connect_to_server_direct(server_name, port, nick));
	using_server_process = 1;
	oper_command = 0;
	write_des[0] = -1;
	write_des[1] = -1;
	if (pipe(write_des) || pipe(read_des))
	{
		if (write_des[0] != -1)
		{
			new_close(write_des[0]);
			new_close(write_des[1]);
		}
		say("Couldn't start new process: %s", strerror(errno));
		return (connect_to_server_direct(server_name, port, nick));
	}
	switch (pid = fork())
	{
	case -1:
		say("Couldn't start new process: %s\n", strerror(errno));
		return (-1);
	case 0:
		(void) MY_SIGNAL(SIGINT, (sigfunc *)SIG_IGN, 0);
		dup2(read_des[1], 1);
		dup2(write_des[0], 0);
		new_close(read_des[0]);
		new_close(read_des[1]);
		new_close(write_des[0]);
		new_close(write_des[1]);
		sprintf(buffer, "%u", port);
		setuid(getuid());
		execl(path, name, server_name, buffer, (char *) 0);
		printf("-5 0\n"); /* -1 - -4 returned by connect_by_number() */
		fflush(stdout);
		_exit(1);
	default:
		new_close(read_des[1]);
		new_close(write_des[0]);
		break;
	}
	old_timeout = dgets_timeout(3);
/**************************** PATCHED by Flier ******************************/
	/*c = dgets(buffer, BIG_BUFFER_SIZE, read_des[0], (char *) 0);*/
#ifdef BNCCRYPT
	c = dgets(buffer, BIG_BUFFER_SIZE, read_des[0], (char *) 0, 1);
#else
	c = dgets(buffer, BIG_BUFFER_SIZE, read_des[0], (char *) 0, 0);
#endif
/****************************************************************************/
	(void) dgets_timeout(old_timeout);
	if ((c == 0) || ((c = atoi(buffer)) != 0))
	{
		if (c == -5)
			return (connect_to_server_direct(server_name, port, nick));
		else
		{
			char *ptr;

			if ((ptr = (char *) index(buffer, ' ')) != NULL)
			{
				ptr++;
				if (atoi(ptr) > 0)
		say("Unable to connect to port %d of server %s: %s",
			port, server_name, strerror(atoi(ptr)));
				else
		say("Unable to connect to port %d of server %s: Unknown host",
							port, server_name);
			}
			else
		say("Unable to connect to port %d of server %s: Unknown host",
							port, server_name);
			if (is_server_open(from_server))
				say("Connection to server %s resumed...",
						server_list[from_server].name);
			new_close(read_des[0]);
			new_close(write_des[1]);
			return (-1);
		}
	}
	update_all_status();
	add_to_server_list(server_name, port, (char *) 0, nick, 1);
	server_list[from_server].read = read_des[0];
	server_list[from_server].write = write_des[1];
	server_list[from_server].pid = pid;
	server_list[from_server].operator = 0;
	return (0);
#endif /* _Windows */
}

/*
 * connect_to_server: Given a name and portnumber, this will attempt to
 * connect to that server using either a direct connection or process
 * connection, depending on the value of using_server_process.  If connection
 * is successful, the proper NICK, USER, and PASS commands are sent to the
 * server.  If the c_server parameter is not -1, then the server with that
 * index will be closed upon successful connection here. Also, if connection
 * is successful, the attempting_to_connect variable is incremented.  This is
 * checked in the notice.c routines to make sure that connection was truely
 * successful (and not closed immediately by the server). 
 */
int
connect_to_server(server_name, port, nick, c_server)
	char	*server_name;
	int	port;
	char	*nick;
	int	c_server;
{
	int	server_index;
#ifdef INET6
	struct	sockaddr_storage	sa;
	int salen = sizeof( struct sockaddr_storage );
#else
	struct sockaddr_in	sa;
	int salen = sizeof( struct sockaddr_in );
#endif
/**************************** PATCHED by Flier ******************************/
        int i;

        LastServer=time((time_t *) 0);
/****************************************************************************/
 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);
	server_index = find_in_server_list(server_name, port, nick);
/**************************** PATCHED by Flier ******************************/
        /* Fix for ircII bug where client believes it is connected to
         * two servers if following sequence is executed:
         * 1) connect to server
         * 2) /s server which is firewalled or timeouts
         * 3) /s new_server while 2) is still in progress
         * Now client is confused about servers. */
        for (i=0;i<number_of_servers;i++) {
            if ((server_list[i].flags)&CLEAR_PENDING)
                clear_channel_list(i);
            if ((server_list[i].flags)&CLOSE_PENDING) {
		server_list[i].close_serv=-1;
		server_list[i].flags&=~(CLOSE_PENDING|CLEAR_PENDING);
                close_server(i,"broken connect");
            }
        }
/****************************************************************************/
	attempting_to_connect = 1;
	/*
	 * check if the server doesn't exist, or that we're not already
	 * connected to it.
         */
	if (!is_server_connected(server_index))
	{
		if (is_server_open(server_index))
			close_server(server_index, "retrying connect");
		if (port == -1)
		{
			if (server_index != -1)
				port = server_list[server_index].port;
			else
				port = irc_port;
		}
		say("Connecting to port %d of server %s", port, server_name);

		load_ircquick();

/**************************** PATCHED by Flier ******************************/
                /* transfer auto-reply and tabkey lists */
                if (CheckServer(c_server) && server_index>=0) {
                    struct nicks *tmp;
                    struct nicks *tmpfree;

                    if (server_list[server_index].LastMessage)
                        new_free(&(server_list[server_index].LastMessage));
                    if (server_list[server_index].LastNotice)
                        new_free(&(server_list[server_index].LastNotice));
                    if (server_list[server_index].LastMessageSent)
                        new_free(&(server_list[server_index].LastMessageSent));
                    if (server_list[server_index].LastNoticeSent)
                        new_free(&(server_list[server_index].LastNoticeSent));
                    if (server_list[c_server].LastMessage)
                        malloc_strcpy(&(server_list[server_index].LastMessage),
                                      server_list[c_server].LastMessage);
                    if (server_list[c_server].LastNotice)
                        malloc_strcpy(&(server_list[server_index].LastNotice),
                                      server_list[c_server].LastNotice);
                    if (server_list[c_server].LastMessageSent)
                        malloc_strcpy(&(server_list[server_index].LastMessageSent),
                                      server_list[c_server].LastMessageSent);
                    if (server_list[c_server].LastNoticeSent)
                        malloc_strcpy(&(server_list[server_index].LastNoticeSent),
                                      server_list[c_server].LastNoticeSent);
                    for (tmp=server_list[server_index].arlist;tmp;) {
                        tmpfree=tmp;
                        tmp=tmp->next;
                        new_free(&(tmpfree->nick));
                        new_free(&tmpfree);
                    }
                    server_list[server_index].arcur=server_list[c_server].arcur;
                    server_list[server_index].arlist=server_list[c_server].arlist;
                    for (tmp=server_list[server_index].nicklist;tmp;) {
                        tmpfree=tmp;
                        tmp=tmp->next;
                        new_free(&(tmpfree->nick));
                        new_free(&tmpfree);
                    }
                    server_list[server_index].nickcur=server_list[c_server].nickcur;
                    server_list[server_index].nicklist=server_list[c_server].nicklist;
                }
/****************************************************************************/
		if (using_server_process)
			server_index = connect_to_server_process(server_name, port, nick);
		else
			server_index = connect_to_server_direct(server_name, port, nick);
		if (server_index)
		{
			attempting_to_connect = 0;
 			restore_message_from();
			return -1;
		}
		if ((c_server != -1) && (c_server != from_server))
		{
#ifdef NON_BLOCKING_CONNECTS
			/*
			 * this sucks.. we can't keep our channels if the
			 * new server connect() fails.
			 */
#if defined(GKM)
			say("--- server %s will be closed when we connect", server_list[c_server].name);
			if (server_list[c_server].flags & CLOSE_PENDING)
				say("--- why are we flagging this for closing a second time?");
#endif
/**************************** PATCHED by Flier ******************************/
                        say("Server %s:%d will be closed when we connect",
                            server_list[c_server].name,
                            server_list[c_server].port);
/****************************************************************************/
			server_list[from_server].close_serv = c_server;
			server_list[c_server].flags |= CLOSE_PENDING;
			server_list[c_server].connected = 0;
#else
			close_server(c_server, "changing servers");
#endif /* NON_BLOCKING_CONNECTS */
		}
		else
		{
			server_list[from_server].close_serv = -1;
		}
		if (connect_next_nick)
		{
			if (*connect_next_nick)
				malloc_strcpy(&(server_list[from_server].nickname), connect_next_nick);
			new_free(&connect_next_nick);
		}
		if (connect_next_password)
		{
			if (*connect_next_password)
			malloc_strcpy(&(server_list[from_server].password),
					connect_next_password);
			new_free(&connect_next_password);
		}
		if (server_list[from_server].nickname == (char *) 0)
			malloc_strcpy(&(server_list[from_server].nickname),
					nickname);
		server_list[from_server].flags &= ~LOGGED_IN;
		/*
		 * this used to be an ifndef NON_BLOCKING_CONNECTS .. we want to do this
		 * whenever the connection is valid, it's possible for a connect to be
		 * "immediate".
		 */
		if (is_server_open(from_server) &&
			getpeername(server_list[from_server].read, (struct sockaddr *) &sa, &salen) != -1)
			login_to_server(from_server);
	}
	else
	{
		if (port == -1)
		{
			if (server_index != -1)
				port = server_list[server_index].port;
			else
				port = irc_port;
		}
		say("Connected to port %d of server %s", port, server_name);
		from_server = server_index;
		if ((c_server != -1) && (c_server != from_server))
			close_server(c_server, "changing servers");
	}
	update_all_status();
 	restore_message_from();
	return 0;
}

static	void
login_to_server(server)
	int server;
{
#ifdef NON_BLOCKING_CONNECTS
	int	old_serv = server_list[server].close_serv;
#endif

/**************************** PATCHED by Flier ******************************/
	server_list[server].SZWI=0;
	server_list[server].SZWho=0;
	server_list[server].SZUnban=0;
/****************************************************************************/
	server_list[server].flags |= LOGGED_IN;
#ifdef NON_BLOCKING_CONNECTS
	set_blocking(server_list[server].read);
	if (server_list[server].read != server_list[server].write)
		set_blocking(server_list[server].write);
	if (old_serv != -1)
	{
#if defined(GKM)
		say("--- closing server %s - changing servers", server_list[server_list[server].close_serv].name);
		if (!(server_list[server_list[server].close_serv].flags & CLOSE_PENDING))
			say("--- uh oh. closing a server that wasn't CLOSE_PENDING");
#endif
		if (server_list[old_serv].flags & CLEAR_PENDING)
			clear_channel_list(old_serv);   /* Channels were
							   transfered -Sol */
		server_list[old_serv].flags &= ~(CLOSE_PENDING|CLEAR_PENDING);
		close_server(old_serv, "changing servers");
		server_list[server].close_serv = -1;
		/* should we pause here to let the net catch up with us? */
	}
#if defined(GKM)
	else
	{
		say("--- no server to close in login_to_server()");
	}
#endif
#endif /* NON_BLOCKING_CONNECTS */
	irc2_login_to_server(server);
}

static	void
irc2_login_to_server(server)
	int	server;
{

	if (server_list[server].password)
		send_to_server("PASS %s", server_list[server].password);
	send_to_server("NICK %s", server_list[server].nickname);
	send_to_server("USER %s %s %s :%s", username,
		(send_umode && *send_umode) ? send_umode : hostname,
		server_list[server].name, realname);
}

/*
 * get_connected: This function connects the primary server for IRCII.  It
 * attempts to connect to the given server.  If this isn't possible, it
 * traverses the server list trying to keep the user connected at all cost.  
 * oldconn is set if this connection is really an old connection being
 * resurected (eg. connection to server failed).
 */
void
get_connected(server)
	int	server;
{
	int	s,
		ret = -1;

        if (server_list)
	{
		int	already_connected = 0;

/**************************** PATCHED by Flier ******************************/
		/*if (server == number_of_servers)*/
                if (server>=number_of_servers)
/****************************************************************************/
			server = 0;
		else if (server < 0)
			server = number_of_servers - 1;
		s = server;
		if (connect_to_server(server_list[server].name, server_list[server].port, server_list[server].nickname, primary_server))
		{
			while (server_list[server].read == -1)
			{
				server++;
				if (server == number_of_servers)
					server = 0;
				if (server == s)
				{
					clean_whois_queue();
					say("Use /SERVER to connect to a server");
					break;
				}
				from_server = server;
				already_connected = is_server_connected(server);
				ret = connect_to_server(server_list[server].name, server_list[server].port, server_list[server].nickname, primary_server);
			}
			if (!ret)
				from_server = server;
			else
				from_server = -1;
		}
 		if (from_server != -1) {
 			int flags;

			flags = (already_connected ? 0 : WIN_TRANSFER);
			window_set_server(-1, from_server, flags);
 		}
	}
	else
	{
		clean_whois_queue();
		say("Use /SERVER to connect to a server");
	}
}

#ifdef SERVERS_FILE
/*
 * read_server_file: reads hostname:portnum:password server information from
 * a file and adds this stuff to the server list.  See build_server_list()/ 
 */
int
read_server_file()
{
	FILE *fp;
	char format[11];
	char *file_path = (char *) 0;
	char	buffer[BIG_BUFFER_SIZE+1];

	malloc_strcpy(&file_path, irc_lib);
	malloc_strcat(&file_path, SERVERS_FILE);
	sprintf(format, "%%%ds", BIG_BUFFER_SIZE);
	fp = fopen(file_path, "r");
	new_free(&file_path);
	if ((FILE *) 0 != fp)
	{
		while (fscanf(fp, format, buffer) != EOF)
			build_server_list(buffer);
		fclose(fp);
		return (0);
	}
	return (1);
}
#endif

/* display_server_list: just guess what this does */
void
display_server_list()
{
	int	i;

	if (server_list)
	{
		if (from_server != -1)
			say("Current server: %s %d",
					server_list[from_server].name,
					server_list[from_server].port);
		else
			say("Current server: <None>");
		if (primary_server != -1)
			say("Primary server: %s %d",
				server_list[primary_server].name,
				server_list[primary_server].port);
		else
			say("Primary server: <None>");
		say("Server list:");
		for (i = 0; i < number_of_servers; i++)
		{
			if (!server_list[i].nickname)
			{
				say("\t%d) %s %d%s", i,
					server_list[i].name,
					server_list[i].port,
					server_list[i].read == -1 ? " (not connected)" : "");
			}
			else
			{
				if (server_list[i].read == -1)
					say("\t%d) %s %d (was %s)", i,
						server_list[i].name,
						server_list[i].port,
						server_list[i].nickname);
				else
					say("\t%d) %s %d (%s)", i,
						server_list[i].name,
						server_list[i].port,
						server_list[i].nickname);
			}
#ifdef GKM
/**************************** PATCHED by Flier ******************************/
			say("\t\tflags: %s%s%s",
				server_list[i].flags & SERVER_2_6_2 ? "SERVER_2_6_2 " : "",
				server_list[i].flags & CLOSE_PENDING ? "CLOSE_PENDING " : "",
				server_list[i].flags & LOGGED_IN ? "LOGGED_IN " : "" );
/****************************************************************************/
			say("\t\tclose_serv=%d, connected=%d, read=%d, eof=%d", server_list[i].close_serv, server_list[i].connected, server_list[i].read, server_list[i].eof);
#endif
		}
	}
	else
		say("The server list is empty");
}

void
MarkAllAway(command, message)
	char	*command;
	char	*message;
{
	int	old_server;

	old_server = from_server;
	for (from_server=0; from_server<number_of_servers; from_server++)
	{
/**************************** PATCHED by Flier ******************************/
		/*if (is_server_connected(from_server))
			send_to_server("%s :%s", command, message);*/
            	if (message && *message) {
                    malloc_strcpy(&(server_list[from_server].away),message);
                    if (is_server_connected(from_server)) {
                        send_to_server("%s :%s", command, message);
#ifdef CELE
                        SentAway++;
#endif
                    }
                }
                else if (is_server_connected(from_server))
                    send_to_server("%s", command);
/****************************************************************************/
	}
	from_server = old_server;
}


/*
 * set_server_password: this sets the password for the server with the given
 * index.  If password is null, the password for the given server is returned 
 */
char	*
set_server_password(server_index, password)
	int	server_index;
	char	*password;
{

	if (server_list)
	{
		if (password)
			malloc_strcpy(&(server_list[server_index].password), password);
		return (server_list[server_index].password);
	}
	else
		return ((char *) 0);
}

/*
 * server: the /SERVER command. Read the SERVER help page about 
 */
/*ARGSUSED*/
void
servercmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*server,
		*port,
		*extra,
		*password = (char *) 0,
		*nick = (char *) 0;
	int	port_num,
		i,
		new_server_flags;

	if ((server = next_arg(args, &args)) != NULL)
	{
		while (*server == '-')
		{
 			size_t	len;

			/*
			 * old usage of `/server -' handled here.
			 */
			if (*++server == '\0')
			{
				get_connected(primary_server - 1);
				return;
			}
			upper(server);
			len = strlen(server);
			/*
			 * just don't return if you want to perform some action in one of
			 * the flag handling sections.
			 */
			if (!strncmp(server, "DELETE", len))
			{
				if ((server = next_arg(args, &args)) != NULL)
				{
					if ((i = parse_server_index(server)) == -1)
					{
						if (-1 == (i = find_in_server_list(server, 0, 0)))
						{
							say("No such server in list");
							return;
						}
					}
					if (server_list[i].connected)
					{
						say("Can not delete server that is already open");
						return;
					}
					remove_from_server_list(i);
					return;
				}
				say("Need server number for -DELETE");
				return;
			}
			else
			{
				say("SERVER: %s is an unknown flag", server);
				return;
			}
			if ((server = next_arg(args, &args)) == NULL)
			{
				say("SERVER: need a server name");
				return;
			}
		}

		if (index(server, ':') != NULL)
		{
			parse_server_info(&server, &port, &password, &nick, &extra);
			if (!strlen(server))
			{
				say("Server name required");
				return;
			}
			if (port && *port) {
				port_num = atoi(port);
				if (!port_num)
					port_num = irc_port;
			} else
				port_num = irc_port;
		}
		else
		{
			if ((port = next_arg(args, &args)) != NULL)
			{
				port_num = atoi(port);
				if (!port_num)
					port_num = irc_port;
				if ((password = next_arg(args, &args)) != NULL)
					nick = next_arg(args, &args);
			}
			else
				port_num = -1;

			extra = (char *) 0;
		}
 
		if (nick && *nick)
			malloc_strcpy(&connect_next_nick, nick);
		if (password && *password)
			malloc_strcpy(&connect_next_password, password);

		if (*server == '+' || *server == '=' || *server == '~')
		{
			if (*(server+1))
			{
				char	servinfo[INPUT_BUFFER_SIZE+1];

				if (*server == '+')
					server++;
				/* Reconstitute whole server info so
				  window_get_connected can parse it -Sol */
				sprintf(servinfo, "%s:%d:%s:%s",
					server, port_num,
					password ? password : empty_string,
					nick ? nick : empty_string);
				window_get_connected(curr_scr_win, servinfo, -1, 0, (char *) 0);
			}
			else
/**************************** PATCHED by Flier ******************************/
				/*get_connected(primary_server + 1);*/
                                get_connected(primary_server+1+
                                              primary_server==-1?1+(rand()%(number_of_servers?number_of_servers:1)):0);
/****************************************************************************/
			return;
		}
		/*
		 * work in progress.. window->prev_server needs to be set for
		 * all windows that used to be associated with a server as it
		 * switches [successfully] to a new server.
		 * this'll be fun since that can happen in server.c and
		 * window.c and non-blocking-connects will throw yet another
		 * wrench into things since we only want it to happen on
		 * a successful connect. - gkm
		 */
		else if (*server == '.')
		{
			if (*(++server))
			{
				say("syntax error - nothing may be specified after the '.'");
				return;
			}
			if (current_screen && curr_scr_win && curr_scr_win->prev_server != -1)
			{
				window_restore_server(curr_scr_win->prev_server);
				window_get_connected(curr_scr_win, NULL, curr_scr_win->server, 0, (char *) 0);
			}
			else
				say("No server previously in use in this window");
			return;
		}
		if ((i = parse_server_index(server)) != -1)
		{
			server = server_list[i].name;
			if (server_list[i].port != -1)
				port_num = server_list[i].port;
			if (server_list[i].nickname && !nick)
				nick = server_list[i].nickname;
		}
		else
			i = find_in_server_list(server, port_num, nick);
		if (is_server_connected(i))
		{
			/*
			 * We reset the log level only if the "new" server
			 * already has windows associated with it : here it's
			 * equivalent to its already being connected. -Sol
			 */
			new_server_flags = 0;
		}
		else
			new_server_flags = WIN_TRANSFER;
		if (connect_to_server(server, port_num, nick, primary_server) != -1)
		{
			if (primary_server > -1 && from_server != primary_server &&
			    !server_list[from_server].away && server_list[primary_server].away)
				malloc_strcpy(&server_list[from_server].away, server_list[primary_server].away);
			window_set_server(-1, from_server, new_server_flags);
		}
	}
	else
		display_server_list();
}

/*
 * flush_server: eats all output from server, until there is at least a
 * second delay between bits of servers crap... useful to abort a /links. 
 */
void
flush_server()
{
	fd_set rd;
	struct timeval time_out;
	int	flushing = 1;
	int	des;
	int	old_timeout;
	char	buffer[BIG_BUFFER_SIZE+1];

	if ((des = server_list[from_server].read) == -1)
		return;
	time_out.tv_usec = 0;
	time_out.tv_sec = 1;
	old_timeout = dgets_timeout(1);
	while (flushing)
	{
		FD_ZERO(&rd);
		FD_SET(des, &rd);
		switch (new_select(&rd, (fd_set *) 0, &time_out))
		{
		case -1:
		case 0:
			flushing = 0;
			break;
		default:
			if (FD_ISSET(des, &rd))
			{
/**************************** PATCHED by Flier ******************************/
				/*if (0 == dgets(buffer, BIG_BUFFER_SIZE, des,
						(char *) 0))*/
#ifdef BNCCRYPT
				if (0 == dgets(buffer, BIG_BUFFER_SIZE, des,
						(char *) 0, 1))
#else
				if (0 == dgets(buffer, BIG_BUFFER_SIZE, des,
						(char *) 0, 0))
#endif
/****************************************************************************/
					flushing = 0;
				
			}
			break;
		}
	}
	/* make sure we've read a full line from server */
	FD_ZERO(&rd);
	FD_SET(des, &rd);
	if (new_select(&rd, (fd_set *) 0, &time_out) > 0)
/**************************** PATCHED by Flier ******************************/
		/*dgets(buffer, BIG_BUFFER_SIZE, des, (char *) 0);*/
#ifdef BNCCRYPT
		dgets(buffer, BIG_BUFFER_SIZE, des, (char *) 0, 1);
#else
		dgets(buffer, BIG_BUFFER_SIZE, des, (char *) 0, 0);
#endif
/****************************************************************************/
	(void) dgets_timeout(old_timeout);
}

/*
 * set_server_whois: sets the whois value for the given server index.  If the
 * whois value is 0, it assumes the server doesn't send End of WHOIS commands
 * and the whois.c routines use the old fashion way of getting whois info. If
 * the whois value is non-zero, then the server sends End of WHOIS and things
 * can be done more effienciently 
 */
void
set_server_whois(server_index, value)
	int	server_index,
		value;
{
	server_list[server_index].whois = value;
}

/* get_server_whois: Returns the whois value for the given server index */
int
get_server_whois(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	return (server_list[server_index].whois);
}


void
set_server_2_6_2(server_index, value)
	int	server_index,
		value;
{
	set_server_flag(server_index, SERVER_2_6_2, value);
}

int
get_server_2_6_2(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	return (get_server_flag(server_index, SERVER_2_6_2));
}

void
set_server_flag(server_index, flag, value)
	int	server_index;
	int	flag;
	int	value;
{
	if (server_index == -1)
		server_index = primary_server;
	if (value)
		server_list[server_index].flags |= flag;
	else
		server_list[server_index].flags &= ~flag;
}

int
get_server_flag(server_index, value)
	int	server_index;
	int	value;
{
	if (server_index == -1)
		server_index = primary_server;
	return server_list[server_index].flags & value;
}

/**************************** PATCHED by Flier ******************************/
void set_server_umode_flag(server_index,flag,add)
int server_index;
char flag;
int add;
{
    int flagvalue=(1 << (tolower(flag)-'`')) >> 1;

    if (server_index==-1) server_index=primary_server;
    if (add) server_list[server_index].umodeflags|=flagvalue;
    else server_list[server_index].umodeflags&=~flagvalue;
}

int get_server_umode_flag(server_index,flag)
int server_index;
char flag;
{
    int flagvalue=(1 << (tolower(flag)-'`')) >> 1;

    if (server_index==-1) server_index=primary_server;
    return(server_list[server_index].umodeflags&flagvalue);
}
/****************************************************************************/

/*
 * get_server_password: get the passwor for this server.
 */
char *
get_server_password(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	return (server_list[server_index].password);
}

/*
 * set_server_version: Sets the server version for the given server type.  A
 * zero version means pre 2.6, a one version means 2.6 aso. (look server.h
 * for typedef)
 */
void
set_server_version(server_index, version)
	int	server_index;
	int	version;
{
	if (server_index == -1)
		server_index = primary_server;
	server_list[server_index].version = version;
}

/*
 * get_server_version: returns the server version value for the given server
 * index 
 */
int
get_server_version(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	if (server_index == -1)
		return DEFAULT_SERVER_VER;
	else
		return (server_list[server_index].version);
}

/* get_server_name: returns the name for the given server index */
char	*
get_server_name(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	return (server_list[server_index].name);
}

/* set_server_itsname: returns the server's idea of its name */
char	*
get_server_itsname(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	if (server_list[server_index].itsname)
		return server_list[server_index].itsname;
	else if (server_list[server_index].name)
		return server_list[server_index].name;
	else
		return "<None>";
}

void
set_server_itsname(server_index, name)
	int	server_index;
	char	*name;
{
	if (server_index == -1)
		server_index = primary_server;
	malloc_strcpy(&server_list[server_index].itsname, name);
}

/*
 * is_server_open: Returns true if the given server index represents a server
 * with a live connection, returns false otherwise 
 */
int
is_server_open(server_index)
	int	server_index;
{
	if (server_index < 0)
		return (0);
	return (server_list[server_index].read != -1);
}

/*
 * is_server_connected: returns true if the given server is connected.  This
 * means that both the tcp connection is open and the user is properly
 * registered 
 */
int
is_server_connected(server_index)
	int	server_index;
{
/**************************** PATCHED by Flier ******************************/
	/*if (server_index < 0)
		return (0);*/
        if (server_index<0 || server_index>=number_of_servers) return(0);
/****************************************************************************/
	return (server_list[server_index].connected && (server_list[server_index].flags & LOGGED_IN));
}

/* get_server_port: Returns the connection port for the given server index */
int
get_server_port(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	return (server_list[server_index].port);
}

/*
 * get_server_nickname: returns the current nickname for the given server
 * index 
 */
char	*
get_server_nickname(server_index)
	int	server_index;
{
	if ((server_index != -1) && server_list[server_index].nickname)
		return (server_list[server_index].nickname);
	else
		return (nickname);
}



/* get_server_qhead - get the head of the whois queue */
WhoisQueue *
get_server_qhead(server_index)
	int	server_index;
{
	if (server_index != -1)
		return server_list[server_index].WQ_head;
	else
		return WQ_head;
}

/* get_server_whois_stuff */
WhoisStuff *
get_server_whois_stuff(server_index)
	int	server_index;
{
	if (server_index == -1)
		server_index = primary_server;
	return &server_list[server_index].whois_stuff;
}

/* get_server_qtail - get the tail of the whois queue */
WhoisQueue *
get_server_qtail(server_index)
	int	server_index;
{
	if (server_index !=-1)
		return server_list[server_index].WQ_tail;
	else
		return WQ_tail;
}



/* set_server_qhead - set the head of the whois queue */
void
set_server_qhead(server_index, value)
	int	server_index;
	WhoisQueue *value;
{
	if (server_index != -1)
		server_list[server_index].WQ_head = value;
	else
		WQ_head = value;
}

/* set_server_qtail - set the tail of the whois queue */
void
set_server_qtail(server_index, value)
	int	server_index;
	WhoisQueue *value;
{
	if (server_index !=-1)
		server_list[server_index].WQ_tail = value;
	else
		WQ_tail = value;
}



/*
 * get_server_operator: returns true if the user has op privs on the server,
 * false otherwise 
 */
int
get_server_operator(server_index)
	int	server_index;
{
	return (server_list[server_index].operator);
}

/*
 * set_server_operator: If flag is non-zero, marks the user as having op
 * privs on the given server.  
 */
void
set_server_operator(server_index, flag)
	int	server_index;
	int	flag;
{
	server_list[server_index].operator = flag;
}

/*
 * set_server_nickname: sets the nickname for the given server to nickname.
 * This nickname is then used for all future connections to that server
 * (unless changed with NICK while connected to the server 
 */
void
set_server_nickname(server_index, nick)
	int	server_index;
	char	*nick;
{
	if (server_index != -1)
	{
		malloc_strcpy(&(server_list[server_index].nickname), nick);
		if (server_index == primary_server)
 			malloc_strcpy(&nickname, nick);
	}
	update_all_status();
}

void
set_server_motd(server_index, flag)
	int	server_index;
	int	flag;
{
	if (server_index != -1)
		server_list[server_index].motd = flag;
}

int
get_server_motd(server_index)
	int	server_index;
{
	if (server_index != -1)
		return(server_list[server_index].motd);
	return (0);
}

void
server_is_connected(server_index, value)
	int	server_index,
		value;
{
	server_list[server_index].connected = value;
	if (value)
		server_list[server_index].eof = 0;
}

extern int in_redirect;
/* send_to_server: sends the given info the the server */
void
#ifdef HAVE_STDARG_H
send_to_server(char *format, ...)
#else
send_to_server(format, arg1, arg2, arg3, arg4, arg5,
		arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
#endif
{
	static	int	in_send_to_server = 0;
	char	lbuf[BIG_BUFFER_SIZE + 1];	/* make this buffer *much*
						 * bigger than needed */
	char	*buf = lbuf;
	int	des;
	size_t	len;
	int	server = from_server;
#ifdef HAVE_STDARG_H
	va_list vlist;

	va_start(vlist, format);
#endif

	if (in_send_to_server)
		return;
	bzero(lbuf, sizeof(lbuf));
	in_send_to_server = 1;
	if (server == -1)
		server = primary_server;
	if (server != -1 && ((des = server_list[server].write) != -1) &&
	    (server_list[server].flags & LOGGED_IN) )
	{
		server_list[server].sent = 1;
#ifdef HAVE_STDARG_H
		vsprintf(buf, format, vlist);
		va_end(vlist);
#else
			     
		sprintf(buf, format, arg1, arg2, arg3, arg4, arg5,
		    arg6, arg7, arg8, arg9, arg10);
#endif
 		len = strlen(lbuf);
		if (len > (IRCD_BUFFER_SIZE - 2))
 			lbuf[IRCD_BUFFER_SIZE - 2] = (char) 0;
/**************************** PATCHED by Flier ******************************/
#ifdef BNCCRYPT
                EncryptBNC(lbuf,strlen(lbuf));
#endif
/****************************************************************************/
 		strmcat(lbuf, "\n", IRCD_BUFFER_SIZE);
 		send(des, lbuf, strlen(lbuf), 0);
	}
	else if (!in_redirect && !connected_to_server)
/**************************** PATCHED by Flier ******************************/
        {
/****************************************************************************/
		say("You are not connected to a server, use /SERVER to connect.");
/**************************** PATCHED by Flier ******************************/
                inSZNotify=0;
                inSZLinks=0;
                inSZFKill=0;
                inSZTrace=0;
                inSZNickCompl=0;
        }
/****************************************************************************/
	in_send_to_server = 0;
}

#ifdef HAVE_SYS_UN_H
/*
 * Connect to a UNIX domain socket. Only works for servers.
 * submitted by Avalon for use with server 2.7.2 and beyond.
 */
int
connect_to_unix(port, path)
	int	port;
	char	*path;
{
	struct	sockaddr_un un;
	int	    sock;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);

	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "%-.100s/%-.6d", path, port);

 	if (connect(sock, (struct sockaddr *)&un, (int)strlen(path)+2) == -1)
	{
		new_close(sock);
		return -1;
	}
	return sock;
}
#endif /* HAVE_SYS_UN_H */

/*
 * close_all_server: Used whn creating new screens to close all the open
 * server connections in the child process...
 */
extern	void
close_all_server()
{
	int	i;

	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].read != -1)
			new_close(server_list[i].read);
		if (server_list[i].write != -1)
			new_close(server_list[i].write);
	}
}

extern	char	*
create_server_list()
{
	int	i;
	char	*value = (char *) 0;
	char	buffer[BIG_BUFFER_SIZE+1];

	*buffer = '\0';
	for (i = 0; i < number_of_servers; i++)
		if (server_list[i].read != -1)
		{
			strcat(buffer, get_server_itsname(i));
			strcat(buffer, " ");
		}
	malloc_strcpy(&value, buffer);

	return value;
}

static	void
add_to_server_buffer(server, buf)
	int	server;
	char	*buf;
{
	if (buf && *buf)
	{
		if (server_list[server].buffer)
			malloc_strcat(&server_list[server].buffer, buf);
		else
			malloc_strcpy(&server_list[server].buffer, buf);
	}
}

void
disconnectcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*server;
	char	*message;
	int	i;

	if ((server = next_arg(args, &args)) != NULL && server[0] != '*' && server[1] != '\0')
	{
		i = parse_server_index(server);
		if (-1 == i)
		{
			say("No such server!");
			return;
		}
	}
	else
		i = get_window_server(0);
	/*
	 * XXX - this is a major kludge.  i should never equal -1 at
	 * this point.  we only do this because something has gotten
	 * *really* confused at this point.  .mrg.
	 */
	if (i == -1)
	{
		for (i = 0; i < number_of_servers; i++)
		{
			server_list[i].eof = -1;
			new_close(server_list[i].read);
			new_close(server_list[i].write);
		}
		goto done;
	}
	if (!args || !*args)
		message = "Disconnecting";
	else
		message = args;
	if (-1 == server_list[i].write)
	{
		say("That server isn't connected!");
		return;
	}
	server = server_list[i].itsname ? server_list[i].itsname :
		server_list[i].name ? server_list[i].name : "unknown?";
	say("Disconnecting from server %s", server);
	close_server(i, message);
	server_list[i].eof = 1;
done:
	clean_whois_queue();
	window_check_servers();
	if (!connected_to_server)
		say("You are not connected to a server. Use /SERVER to connect.");
}

int
find_server_group(group, add)
	char	*group;
	int	add;
{
	static	int	next = 1;
	SGroup *g = (SGroup *) find_in_list((List **) &server_group_list, group, 0);

	if (g)
		goto end;

	if (!add)
		return 0;

	g = (SGroup *) new_malloc(sizeof(SGroup));
	g->name = (char *) 0;
	malloc_strcpy(&g->name, group);
	g->number = next++;
	add_to_list((List **) &server_group_list, (List *) g);
end:
	return g->number;
}

char	*
find_server_group_name(number)
	int	number;
{
	SGroup *g = server_group_list;

	for (; g; g = g->next)
		if (g->number == number)
			return g->name;
	return empty_string;
}
