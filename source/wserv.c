/*
 * wserv.c - little program to be a pipe between a screen or
 * xterm window to the calling ircII process.
 *
 * Copyright (c) 1992 Troy Rollo.
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
 * $Id: wserv.c,v 1.2 1998-09-10 17:46:33 f Exp $
 */

/*
 * Works by opening up the unix domain socket that ircII bind's
 * before calling wserv, and which ircII also deleted after the
 * connection has been made.
 */

#include "defs.h"

#ifdef HAVE_SYS_UN_H

# include "irc.h"
# include "ircterm.h"

# include <sys/un.h>

/*
 * Holds the parent process id, as taken from the command line arguement.
 * We can't use getppid() here, because the parent for screen is the
 * underlying screen process, and for xterm's, it the xterm itself.
 */
static	pid_t	ircIIpid;

/* declare the signal handler */
# if !defined(_RT) && defined(SIGWINCH)
#  if defined(_POSIX_VERSION)
RETSIGTYPE	got_sigwinch _((int));
#  else
RETSIGTYPE	got_sigwinch _((void));
#  endif
# endif /* _RT */

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	char	buffer[1024];
	struct	sockaddr_un *addr = (struct sockaddr_un *) buffer;
	int	nread;
	fd_set	reads;
	int	s;
	char	*path,
		*tmp;

	/* Set up the signal hander to pass SIGWINCH to ircII */
# if !defined(_RT) && defined(SIGWINCH)
	(void) MY_SIGNAL(SIGWINCH, (sigfunc *)got_sigwinch, 0);
# endif /* _RT */

	if (2 != argc)    /* no socket is passed */
 		return 0;
#ifdef	SOCKS
	SOCKSinit(*argv);
#endif /* SOCKS */

	/*
	 * First thing we do here is grab the parent pid from the command
	 * line arguements, because getppid() returns the wrong pid in
	 * all cases.. is comes in via the command line arguement in the
	 * for of irc_xxxxxxxx .. as an 8 digit decimal number..  if we
	 * can't get the pid from the command line arg, then we set it
	 * to -1, which is used later to ignore them..
	 */

	path = (char *) malloc(strlen(argv[1]) + 1);
	strcpy(path, argv[1]);
	if ((char *) 0 != (tmp = (char *) index(path, '_')))
		ircIIpid = atoi(++tmp);
	else
		ircIIpid = -1;

	/*
	 * Set up the socket, from the path passed, connect it.. all that
	 * stuff..  And initalise the term settings for the window.
	 */
	addr->sun_family = AF_UNIX;
	strcpy(addr->sun_path, argv[1]);
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (0 > connect(s, (struct sockaddr *) addr, sizeof(addr->sun_family) +
						strlen(addr->sun_path)))
 		return 0;

	/*
	 * first line to for a wserv program is the tty.  this is so ircii
	 * can grab the size of the tty, and have it changed.
	 */

	tmp = ttyname(0);
	write(s, tmp, strlen(tmp));
	write(s, "\n", 1);
	perror(tmp);

	term_init();

	/*
	 * The select call..  reads from the socket, and from the window..
	 * and pipes the output from out to the other..  nice and simple
	 */
	while (1)
	{
		FD_ZERO(&reads);
		FD_SET(0, &reads);
		FD_SET(s, &reads);
		select(s + 1, &reads, (fd_set *) 0, (fd_set *) 0,
			(struct timeval *) 0);
		if (FD_ISSET(0, &reads))
		{
			if (0 != (nread = read(0, buffer, sizeof(buffer))))
				write(s, buffer, nread);
			else
 				return 0;
		}
		if (FD_ISSET(s, &reads))
		{
			if (0 != (nread = read(s, buffer, sizeof(buffer))))
				write(0, buffer, nread);
			else
 				return 0;
		}
	}
}

/* got_sigwinch: we got a SIGWINCH, so we send it back to ircII */
# if !defined(_RT) && defined(SIGWINCH)
RETSIGTYPE
#  if defined(_POSIX_VERSION)
got_sigwinch(sig)
#  else
got_sigwinch(sig)
#  endif
{
#  ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGWINCH, got_sigwinch, 0);
#  endif
	if (-1 != ircIIpid)
		kill(ircIIpid, SIGWINCH);
}
# endif /* _RT */

#else

void
main()
{
 	return 0;
}

#endif /* HAVE_SYS_UH_H */
