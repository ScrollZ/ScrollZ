/*
 * newio.c: This is some handy stuff to deal with file descriptors in a way
 * much like stdio's FILE pointers 
 *
 * IMPORTANT NOTE:  If you use the routines here-in, you shouldn't switch to
 * using normal reads() on the descriptors cause that will cause bad things
 * to happen.  If using any of these routines, use them all 
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
 * $Id: newio.c,v 1.6 2000-08-18 17:21:49 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "newio.h"

#ifdef ISC22
# include <sys/bsdtypes.h>
#endif /* ISC22 */

#ifdef ESIX
# include <lan/net_types.h>
#endif /* ESIX */

#include "irc_std.h"

#define IO_BUFFER_SIZE 512

#define	WAIT_NL ((unsigned) 0x0001)

/**************************** Patched by Flier ******************************/
/*#ifdef FDSETSIZE
# define IO_ARRAYLEN FDSETSIZE
#else
# ifdef FD_SETSIZE
#  define IO_ARRAYLEN FD_SETSIZE
# else
#  define IO_ARRAYLEN NFDBITS
# endif
#endif*/
#ifdef OPEN_MAX
# define IO_ARRAYLEN OPEN_MAX
# else
# ifdef FDSETSIZE
#  define IO_ARRAYLEN FDSETSIZE
# else
#  ifdef FD_SETSIZE
#   define IO_ARRAYLEN FD_SETSIZE
#  else
#   define IO_ARRAYLEN NFDBITS
#  endif /* FD_SETSIZE */
# endif /* FDSETSIZE */
#endif /* OPEN_MAX */

#ifdef BNCCRYPT
extern void DecryptBNC _((char *, int));
#endif
/****************************************************************************/

typedef	struct	myio_struct
{
	char	buffer[IO_BUFFER_SIZE + 1];
	unsigned int	read_pos,
			write_pos;
	unsigned	misc_flags;
#if defined(ESIX) || defined(_Windows)
	unsigned	flags;
#endif /* ESIX */
#ifdef _Windows
	int		fd;
#endif /* _Windows */
}           MyIO;

#define IO_SOCKET 1

static	struct	timeval	right_away = { 0L, 0L };
static	MyIO	*io_rec[IO_ARRAYLEN];

static	struct	timeval	dgets_timer;
static	struct	timeval	*timer;
	int	dgets_errno = 0;

static	void	init_io _((void));

#ifdef ESIX
/* Esix must know if it is a socket or not. */
void
mark_socket(int des)
{

	if (first)
	{
		int	c;
		for (c = 0; c < FD_SETSIZE; c++)
			io_rec[c] = (MyIO *) 0;
		first = 0;
	}
	if (io_rec[des] == (MyIO *) 0)
	{
		io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
		io_rec[des]->flags = 0;
	}
	io_rec[des]->flags |= IO_SOCKET;
}

void
unmark_socket(int des)
{

	if (first)
	{
		int	c;
		for (c = 0; c < FD_SETSIZE; c++)
			io_rec[c] = (MyIO *) 0;
		first = 0;
	}
	if (io_rec[des] == (MyIO *) 0)
	{
		io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
		io_rec[des]->flags = 0;
	}
	io_rec[des]->flags &= ~IO_SOCKET;
}
#endif /* ESIX */

/*
 * dgets_timeout: does what you'd expect.  Sets a timeout in seconds for
 * dgets to read a line.  if second is -1, then make it a poll.
 */
extern	time_t
dgets_timeout(sec)
	int	sec;
{
	time_t	old_timeout = dgets_timer.tv_sec;

	if (sec)
	{
		dgets_timer.tv_sec = (sec == -1) ? 0 : sec;
		dgets_timer.tv_usec = 0;
		timer = &dgets_timer;
	}
	else
		timer = (struct timeval *) 0;
	return old_timeout;
}

static	void
init_io()
{
	static	int	first = 1;

	if (first)
	{
		int	c;

		for (c = 0; c < IO_ARRAYLEN; c++)
			io_rec[c] = (MyIO *) 0;
		(void) dgets_timeout(-1);
		first = 0;
	}
}

/*
 * dgets: works much like fgets except on descriptor rather than file
 * pointers.  Returns the number of character read in.  Returns 0 on EOF and
 * -1 on a timeout (see dgets_timeout()) 
 */
int
/**************************** Patched by Flier ******************************
dgets(str, len, des, specials)
	char	*str;
	int	len;
	int	des;
	char	*specials;
****************************************************************************/
dgets(str, len, des, specials, decrypt)
	char	*str;
	int	len;
	int	des;
	char	*specials;
	int	decrypt;
{
	char	*ptr, ch;
 	size_t	cnt = 0;
 	int	c;
	fd_set	rd;
	int	WantNewLine = 0;
	int	BufferEmpty;
	int	i,
		j;

	init_io();
	if (io_rec[des] == (MyIO *) 0)
	{
		io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
		io_rec[des]->misc_flags = 0;
#ifdef ESIX
		io_rec[des]->flags = 0;
#endif /* ESIX */	
	}
	if (len < 0)
	{
		WantNewLine = 1;
		len = (-len);
		io_rec[des]->misc_flags |= WAIT_NL;
	}
	while (1)
	{
		if ((BufferEmpty = (io_rec[des]->read_pos ==
				io_rec[des]->write_pos)) || WantNewLine)
		{
			if(BufferEmpty)
			{
				io_rec[des]->read_pos = 0;
				io_rec[des]->write_pos = 0;
			}
			FD_ZERO(&rd);
			FD_SET(des, &rd);
			switch (select(des + 1, &rd, 0, 0, timer))
			{
			case 0:
				str[cnt] = (char) 0;
				dgets_errno = 0;
				return (-1);
			default:
#ifdef ESIX
				if (io_rec[des]->flags & IO_SOCKET)
					c = recv(des, io_rec[des]->buffer +
					  io_rec[des]->write_pos,
					  IO_BUFFER_SIZE-io_rec[des]->write_pos,
					  0);
				else
#endif /* ESIX */
					c = read(des, io_rec[des]->buffer +
					 io_rec[des]->write_pos,
					 IO_BUFFER_SIZE-io_rec[des]->write_pos);
/**************************** PATCHED by Flier ******************************/
#ifdef BNCCRYPT
                                if (decrypt) DecryptBNC(io_rec[des]->buffer+io_rec[des]->write_pos,c);
#endif
/****************************************************************************/
				if (c <= 0)
				{
					if (c == 0)
						dgets_errno = -1;
					else
						dgets_errno = errno;
					return 0;
				}
				if (WantNewLine && specials)
				{
					ptr = io_rec[des]->buffer;
					for (i = io_rec[des]->write_pos; i < io_rec[des]->write_pos + c; i++)
					{
						if ((ch = ptr[i]) == specials[0])
						{
							if (i > 0)
							{
								bcopy(ptr + i - 1, ptr + i + 1, io_rec[des]->write_pos + c - i - 1);
								i -= 2;
								c -= 2;
							}
							else
							{
								bcopy(ptr, ptr + 1, io_rec[des]->write_pos + c - 1);
								i--;
								c--;
							}
						}
						else if (ch == specials[2])
						{
							for (j = i - 1; j >= 0 && isspace(ptr[j]); j--)
								;
							for (; j >= 0 && !isspace(ptr[j]); j--)
								;
							bcopy(ptr + j + 1, ptr + i + 1, io_rec[des]->write_pos + c - i - 1);
							c -= i - j;
							i = j;
						}
						else if (ch == specials[1])
						{
							for (j = i - 1; j >= 0 && ptr[j] != '\n'; j--)
								;
							bcopy(ptr + j + 1, ptr + i + 1, io_rec[des]->write_pos + c - i - 1);
							c -= i - j;
							i = j;
						}
					}
				}
				io_rec[des]->write_pos += c;
				break;
			}
		}
		ptr = io_rec[des]->buffer;
		if (WantNewLine)
		{
			for (cnt = io_rec[des]->write_pos; cnt > 0;cnt--, ptr++)
			{
				if (*ptr == '\n' || cnt == len-1)
				{
					*ptr = '\0';
					(void) strcpy(str, io_rec[des]->buffer);
					io_rec[des]->write_pos = cnt-1;
					bcopy(io_rec[des]->buffer, ptr, cnt);
					dgets_errno = 0;
					return 1;
				}
			}
			return -2;
		}
		while (io_rec[des]->read_pos < io_rec[des]->write_pos)
		{
			if (((str[cnt++] = ptr[(io_rec[des]->read_pos)++]) == '\n') || (cnt == len))
			{
				dgets_errno = 0;
				str[cnt] = (char) 0;
				return (cnt);
			}
		}
	}
}

/*
 * new_select: works just like select(), execpt I trimmed out the excess
 * parameters I didn't need.  
 */
int
new_select(rd, wd, time_out)
	fd_set	*rd,
		*wd;
	struct	timeval	*time_out;
{
	int	i,
		set = 0;
		fd_set new;
	struct	timeval	*newtimeout,
			thetimeout;
	int	max_fd = -1;

	if (time_out)
	{
		newtimeout = &thetimeout;
		bcopy(time_out, newtimeout, sizeof(struct timeval));
	}
	else
		newtimeout = NULL;
	init_io();
	FD_ZERO(&new);
	for (i = 0; i < IO_ARRAYLEN; i++)
	{
		if (i > max_fd && ((rd && FD_ISSET(i, rd)) || (wd && FD_ISSET(i, wd))))
			max_fd = i;
		if (io_rec[i] && !(io_rec[i]->misc_flags&WAIT_NL))
		{
			if (io_rec[i]->read_pos < io_rec[i]->write_pos)
			{
				FD_SET(i, &new);
				set = 1;
			}
		}
	}
	if (set)
	{
		set = 0;
		if (!(select(max_fd + 1, rd, wd, NULL, &right_away) > 0))
			FD_ZERO(rd);
		for (i = 0; i < IO_ARRAYLEN; i++)
		{
			if ((FD_ISSET(i, rd)) || (FD_ISSET(i, &new)))
			{
				set++;
				FD_SET(i, rd);
			}
			else
				FD_CLR(i, rd);
		}
		return (set);
	}
	return (select(max_fd + 1, rd, wd, NULL, newtimeout));
}

/* new_close: works just like close */
void
new_close(des)
	int	des;
{
	if (des < 0)
		return;
#ifdef ESIX
	if (io_rec[des]->flags & IO_SOCKET)
		t_close(des);
#endif /* ESIX */
	new_free(&(io_rec[des])); /* gkm */
	close(des);
}

/* set's socket options */
extern	void
set_socket_options(s)
	int	s;
{
#if defined(ESIX) || defined(_Windows)
	mark_socket(s);
#else
#ifndef NO_STRUCT_LINGER
	struct linger	lin;
#endif
	int	opt = 1;
	int	optlen = sizeof(opt);

#ifdef USE_SO_REUSEADDR
	(void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, optlen);
	opt = 1;
#endif /* USE_SO_REUSEADDR */
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, optlen);
#ifndef NO_STRUCT_LINGER
	lin.l_onoff = lin.l_linger = 0;
	(void) setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &lin, optlen);
#endif /* NO_STRUCT_LINGER */
#endif /* ESIX */
}
