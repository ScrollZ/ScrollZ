/*
 * file.c contains file open/read/write routines to be used to open
 * files with the access permissions of the real UID instead of the
 * effective UID. This allows IRCII to be run setuid->root. If it
 * has effective UID == root, it will then use privileged ports to
 * connect to servers, allowing the servers, some day, to take
 * advantage of this information to ensure that the user names are
 * who they claim to be.
 *
 * It can also be run setuid->something else, with ircio being
 * setuid->root and only runable by the given GID.
 *
 * Copyright (c) 1991, 1992 Troy Rollo.
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
 * $Id: file.c,v 1.2 1998-09-10 17:45:00 f Exp $
 */

#include "irc.h"

#include <sys/stat.h>

#ifdef PRIV_PORT
# undef	PRIV_PORT
#endif /* PRIV_PORT */

int
directory_writeable(file)
	char	*file;
{
	char	dir[BIG_BUFFER_SIZE+1];
	char	*ptr;

	strmcpy(dir, file, BIG_BUFFER_SIZE);
	if (ptr = rindex(dir, '/'))
	{
		if (ptr == dir
#ifdef APOLLO
		    || (ptr == dir+1 && *dir == '/')
#endif /*APOLLO*/
		    )
			ptr++;
		*ptr = '\0';
	}
	else
		strcpy(dir, ".");
	return (!access(dir, W_OK|X_OK));
}

int
ruid_open(filename, flags, mode)
	char	*filename;
	int	flags;
	int	mode;
{
	int	access_flags;
	int	fd;

	switch(flags&(O_RDONLY|O_WRONLY|O_RDWR))
	{
	case O_RDWR:
		access_flags = R_OK|W_OK;
		break;
	case O_RDONLY:
		access_flags = R_OK;
		break;
	case O_WRONLY:
		access_flags = W_OK;
		break;
	}
	if (!access(filename, access_flags))
		return open(filename, flags, mode);
	else if ((flags&O_CREAT) == O_CREAT && directory_writeable(filename))
	{
		fd = open(filename, flags, mode);
		chown(filename, getuid(), getgid());
		return fd;
	}
	else
		return -1;
}

FILE	*
ruid_fopen(filename, mode)
	char	*filename;
	char	*mode;
{
	int	access_flags;
	FILE	*fp;
	char	*tm;

	access_flags = 0;
	for (tm = mode; *tm != '\0'; tm++)
	{
		switch (*tm)
		{
		case '+':
			access_flags |= W_OK|R_OK;
			break;
		case 'r':
			access_flags |= R_OK;
			break;
		case 'w':
		case 'a':
			access_flags |= W_OK;
			break;
		case 't':	/* Text and binary - harmless */
		case 'b':
			break;
		default:	 /* Calls are guilty unless proven innocent */
			return NULL; /* :P to all those who think otherwise! */
		}
	}
	if (!access(filename, access_flags))
		return fopen(filename, mode);
	else if ((access_flags&W_OK) == W_OK && directory_writeable(filename))
	{
		fp = fopen(filename, mode);
		chown(filename, getuid(), getgid());
		return fp;
	}
	else
		return NULL;
}

int
ruid_unlink(filename)
	char	*filename;
{
	if (!access(filename, W_OK) && directory_writeable(filename))
		unlink(filename);
}

int
ruid_system(command)
	char	*command;
{
	int	pid;

	switch (pid = fork())
	{
	case 0:
		setuid(getuid());
		setgid(getgid());
		system(command);
		_exit(0);
		break;
	case -1:
		return -1;
	default:
		while(wait(0) != pid);
		return 0;
	}
}

int
ruid_stat(path, buf)
	char	*path;
	struct	stat *buf;
{
	if (!access(path, 0))
		return -1;
	else
		return stat(path, buf);
}
