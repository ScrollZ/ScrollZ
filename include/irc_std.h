/*
 * irc_std.h: header to define things used in all the programs ircii
 * comes with
 *
 * hacked together from various other files by matthew green
 *
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
 * $Id: irc_std.h,v 1.4 1999-10-04 19:21:37 f Exp $
 */

#ifndef __irc_std_h
#define __irc_std_h

#undef _
#if defined(__STDC__) || defined(__BORLANDC__)
# define _(a) a
#else
# define _(a) ()
# ifdef const
#  undef const
# endif /* cost */
# define const
#endif  /* __STDC__ */

#if !defined(__GNUC__) || __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define __attribute__(x)        /* delete __attribute__ if non-gcc or gcc1 */
#endif

#ifndef lint
#define IRCII_RCSID(x) static const char rcsid[] __attribute__((__unused__)) = x;
#else
#define IRCII_RCSID(x)
#endif

#ifdef _IBMR2
# include <sys/errno.h>
# include <sys/select.h>
#else
# include <errno.h>
#ifndef ERRNO_DECLARED
extern	int	errno;
#endif
#endif /* _IBMR2 */

#ifndef NBBY
# define NBBY	8		/* number of bits in a byte */
#endif /* NBBY */

#ifndef NFDBITS
# define NFDBITS	(sizeof(long) * NBBY)	/* bits per mask */
#endif /* NFDBITS */

/**************************** PATCHED by Flier ******************************/
/*#ifndef FD_SET
# define FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#endif  FD_SET

#ifndef FD_CLR
# define FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#endif  FD_CLR

#ifndef FD_ISSET
# define FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#endif  FD_ISSET

#ifndef FD_ZERO
# define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif  FD_ZERO

#ifndef	FD_SETSIZE
# define FD_SETSIZE	32
#endif*/

#ifndef OPEN_MAX
# define OPEN_MAX 64
# undef FD_SETSIZE
# define FD_SETSIZE 64
#endif

#ifndef FD_SET
# ifdef __FD_SET
#  define FD_SET __FD_SET
# else
#  define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
# endif
#endif

#ifndef FD_CLR
# ifdef __FD_CLR
#  define FD_CLR __FD_CLR
# else
#  define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
# endif
#endif

#ifndef FD_ISSET
# ifdef __FD_ISSET
#  define FD_ISSET __FD_ISSET
# else
#  define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
# endif
#endif

#ifndef FD_ZERO
# ifdef __FD_ZERO
#  define FD_ZERO __FD_ZERO
# else
#  define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
# endif
#endif
/****************************************************************************/

typedef RETSIGTYPE sigfunc _((void));

#ifdef USE_SIGACTION
sigfunc *my_signal _((int, sigfunc *, int));
# define MY_SIGNAL(s_n, s_h, m_f) my_signal(s_n, s_h, m_f)
#else
# if USE_SIGSET
#  define MY_SIGNAL(s_n, s_h, m_f) sigset(s_n, s_h)
# else
#  define MY_SIGNAL(s_n, s_h, m_f) signal(s_n, s_h)
# endif /* USE_SIGSET */
#endif /* USE_SIGACTION */

#if defined(USE_SIGACTION) || defined(USE_SIGSET)
# undef SYSVSIGNALS
#endif

#if defined(__svr4__) || defined(SVR4) || defined(__SVR4)
# if !defined(__svr4__)
#  define __svr4__
# endif /* __svr4__ */
# if !defined(SVR4)
#  define SVR4
# endif /* SVR4 */
# if !defined(__SVR4)
#  define __SVR4
# endif /* __SVR4 */
#endif /* __svr4__ || SVR4 || __SVR4 */

#ifdef _SEQUENT_
# define	u_short	ushort
# define	u_char	unchar
# define	u_long	ulong
# define	u_int	uint
# define	USE_TERMIO
# ifndef POSIX
#  define POSIX
# endif
#endif /* _SEQUENT_ */

#ifndef NeXT
# if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#  include <string.h>
#  if defined(STDC_HEADERS)
#   include <stdlib.h>
#  endif /* HAVE_STDLIB_H */
#  if defined(HAVE_MEMORY_H)
#   include <memory.h>
#  endif /* HAVE_MEMORY_H */
#  undef index
#  undef rindex
#  undef bcopy
#  undef bzero
#  undef bcmp
#  define index strchr
#  define rindex strrchr
#  ifdef HAVE_MEMMOVE
#   define bcopy(s, d, n) memmove((d), (s), (n))
#  else
#   define bcopy(s, d, n) memcpy ((d), (s), (n))
#  endif
#  define bcmp(s, t, n) memcmp ((s), (t), (n))
#  define bzero(s, n) memset ((s), 0, (n))
# else /* STDC_HEADERS || HAVE_STRING_H */
#  include <strings.h>
# endif /* STDC_HEADERS || HAVE_STRING_H */
#endif /* !NeXT */

/**************************** PATCHED by Flier ******************************/
#if defined(_Windows) || defined(SZ32)
/****************************************************************************/
# define IS_ABSOLUTE_PATH(file) ((file)[0] == '/' || (file)[0] == '\\' || ((file)[0] && (file)[1] == ':'))
#else
# define IS_ABSOLUTE_PATH(file) ((file)[0] == '/')
#endif

#if !defined(SYS_ERRLIST_DECLARED) && !defined(_Windows)
extern	char	*sys_errlist[];
extern	int	sys_nerr;
#endif

#ifdef _Windows
extern char FAR * FAR winsock_errors[];
# define strerror(e) ((e) >= 0 && (e) < sys_nerr ? sys_errlist[e] : ((e) >=WSABASEERR ? winsock_errors[(e) - WSABASEERR] : "(unknown)"))
#else
# ifdef NEED_STRERROR
#  undef strerror
#  define strerror(e) ((e) < 0 || (e) >= sys_nerr ? "(unknown)" : sys_errlist[e])
# endif
#endif

/* we need an unsigned 32 bit integer for dcc, how lame */

#ifdef UNSIGNED_LONG32

typedef		unsigned long		u_32int;

#else
# ifdef UNSIGNED_INT32

typedef		unsigned int		u_32int;

# else

typedef		unsigned long		u_32int;

# endif /* UNSIGNED_INT32 */
#endif /* UNSIGNED_LONG32 */

#endif /* __irc_std_h */
