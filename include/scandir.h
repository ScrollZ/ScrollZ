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
 * $Id: scandir.h,v 1.2 1999-03-04 22:06:04 f Exp $
 */

#ifndef __scandir_h__
#define __scandir_h__
#ifndef HAVE_SCANDIR

#if (!defined(ultrix) && !defined(__386BSD__) && !defined(_HPUX_SOURCE)) || defined(HPUX7)
# if defined(XD88) || defined(__SVR4) || defined(POSIX) || defined(__linux__) \
  || defined(SVR3) || defined(__osf__) || defined(M_UNIX) || defined(_SEQUENT_) \
  || defined(__QNX__)

#if defined(__linux__) || defined(__sgi)
int scandir _((const char *, struct dirent ***, int (*)(), int (*)()));
#else
int scandir _((char *, struct dirent ***, int (*)(), int (*)()));
#endif /* __linux__ || __sgi */
#else
#ifdef NeXT
int scandir _((const char *, struct direct ***, int (*)(), int (*)()));
#else
int scandir _((char *, struct direct ***, int (*)(), int (*)()));
#endif /* NeXT */
#endif /* (!ultrix && !__386BSD__ && !_HPUX_SOURCE) || HPUX7 */
#endif	/* ultrix || __386BSD__ || BSD */

#endif /* !HAVE_SCANDIR */
#endif /* __scandir_h__ */
