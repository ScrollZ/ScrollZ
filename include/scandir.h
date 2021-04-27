/*
 * scandir.h: header for scandir routine.
 *
 * Copyright (c) 1998 glen mccready.
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
 * $Id: scandir.h,v 1.5 2021-04-26 20:48:16 t Exp $
 */

#ifndef __scandir_h__
#define __scandir_h__

/* stuff from gnu autoconf docs */

#if defined(HAVE_DIRENT_H) || defined(_POSIX_SOURCE)
# include <dirent.h>
# define NLENGTH(d) (strlen((d)->d_name)
#else /* DIRENT || _POSIX_SOURCE */
# define dirent direct
# define NLENGTH(d) ((d)->d_namlen)
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H || _POSIX_VERSION */

#include <sys/stat.h>

#ifndef HAVE_SCANDIR

#include <sys/types.h>
#include <sys/file.h>
#include <newio.h>

#if (!defined(ultrix) && !defined(__386BSD__) && !defined(_HPUX_SOURCE))
# if defined(__SVR4) || defined(POSIX) || defined(__linux__) || defined(SVR3) \
  || defined(__osf__) || defined(M_UNIX) || defined(_SEQUENT_) \
    || defined(__QNX__)

# include <stdio.h>
# include <dirent.h>
# include <unistd.h>

/* Initial guess at directory size. */
# define INITIAL_SIZE	30

# ifndef DIRSIZ
#  define DIRSIZ(d) (sizeof(struct dirent) + strlen(d->d_name) + 1) 
# endif


#if defined(__linux__) || defined(__sgi)
int scandir _((const char *, struct dirent ***, int (*)(), int (*)()));
#else
int scandir _((char *, struct dirent ***, int (*)(), int (*)()));
#ifndef alphasort
int alphasort _((struct dirent **, struct dirent **)); 
#endif
#endif /* __linux__ || __sgi */

#else /* __SVR4 || POSIX || __linux__ || SVR3 || __osf__ || ... */

# include <sys/stat.h>
# include "irc.h"
# include <sys/dir.h>

#ifdef NeXT
int scandir _((const char *, struct direct ***, int (*)(), int (*)()));
#else
int scandir _((char *, struct direct ***, int (*)(), int (*)()));
#ifndef alphasort
int alphasort _((struct direct **, struct direct **)); 
#endif
#endif /* NeXT */
#endif /* (!ultrix && !__386BSD__ && !_HPUX_SOURCE) */
#endif	/* ultrix || __386BSD__ || BSD */

#endif /* !HAVE_SCANDIR */
#endif /* __scandir_h__ */
