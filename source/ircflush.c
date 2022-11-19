/*
 * Flush: A little program that tricks another program into line buffering
 * its output. 
 *
 * By Michael Sandrof 
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
 * $Id: ircflush.c,v 1.5 2003-01-08 20:00:54 f Exp $
 */

#include "irc.h"
#include <sys/wait.h>

#ifndef __linux__
# if defined(__SVR4) || defined(HAVE_TERMIOS_H)
#  include <termios.h>
# else
#  include <sgtty.h>	/* SVR4 => sgtty = yuk */
# endif /* SOLARIS */
#endif /* __linux__ */


#define BUFFER_SIZE 1024

/* descriptors of the tty and pty */
	int	master,
		slave;
	pid_t	pid;

RETSIGTYPE death _((void));
static	void	setup_master_slave _((void));

RETSIGTYPE
death()
{
	close(0);
	close(master);
	kill(pid, SIGKILL);
	wait(0);
	exit(0);
}

/*
 * setup_master_slave: this searches for an open tty/pty pair, opening the
 * pty as the master device and the tty as the slace device 
 */
static void 
setup_master_slave (void)
{
	char	line[11];
	char	linec;
	int	linen;

	for (linec = 'p'; linec <= 's'; linec++)
	{
		snprintf(line, sizeof line, "/dev/pty%c0", linec);
		if (access(line, 0) != 0)
			break;
		for (linen = 0; linen < 16; linen++)
		{
			snprintf(line, sizeof line, "/dev/pty%c%1x", linec, linen);
			if ((master = open(line, O_RDWR)) >= 0)
			{
				snprintf(line, sizeof line, "/dev/tty%c%1x", linec, linen);
				if (access(line, R_OK | W_OK) == 0)
				{
					if ((slave = open(line, O_RDWR)) >= 0)
						return;
				}
				close(master);
			}
		}
	}
	fprintf(stderr, "flush: Can't find a pty\n");
	exit(0);
}

/*
 * What's the deal here?  Well, it's like this.  First we find an open
 * tty/pty pair.  Then we fork three processes.  The first reads from stdin
 * and sends the info to the master device.  The next process reads from the
 * master device and sends stuff to stdout.  The last processes is the rest
 * of the command line arguments exec'd.  By doing all this, the exec'd
 * process is fooled into flushing each line of output as it occurs.  
 */
int 
main (int argc, char **argv)
{
	char	buffer[BUFFER_SIZE];
	int	cnt;
	fd_set	rd;

	if (argc < 2)
	{
	    fprintf(stderr, "Usage: %s [program] [arguments to program]\n", argv[0]);
	    exit(1);
	}
	pid = open("/dev/tty", O_RDWR);
#ifdef HAVE_SETSID
	setsid();
#else
	ioctl(pid, TIOCNOTTY, 0);
#endif /* HAVE_SETSID */
	setup_master_slave();
	switch (pid = fork())
	{
	case -1:
		fprintf(stderr, "flush: Unable to fork process!\n");
		exit(1);
	case 0:
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		close(master);
		setuid(getuid());
		setgid(getgid());
		execvp(argv[1], &(argv[1]));
		fprintf(stderr, "flush: Error exec'ing process!\n");
		exit(1);
		break;
	default:
		(void) MY_SIGNAL(SIGCHLD, death, 0);
		close(slave);
		while (1)
		{
			FD_ZERO(&rd);
			FD_SET(master, &rd);
			FD_SET(0, &rd);
			switch (select(NFDBITS, &rd, 0, 0, 0))
			{
			case -1:
			case 0:
				break;
			default:
				if (FD_ISSET(0, &rd))
				{
				    if ((cnt = read(0, buffer,BUFFER_SIZE)) > 0)
					write(master, buffer, cnt);
				    else
					death();
				}
				if (FD_ISSET(master, &rd))
				{
					if ((cnt = read(master, buffer,
							BUFFER_SIZE)) > 0)
						write(1, buffer, cnt);
					else
						death();
				}
			}
		}
		break;
	}
 	return 0;
}
