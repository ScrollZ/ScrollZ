/*
 * newio.h - header for newio.c
 *
 * written by matthew green
 *
 * Copyright (c) 1995-2003 Matthew R. Green.
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
 * $Id: newio.h,v 1.6 2007-03-30 15:27:36 f Exp $
 */

#ifndef __newio_h_
# define __newio_h_

/**************************** Patched by Flier ******************************/
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
#include "myssl.h"
#endif
/****************************************************************************/

#ifdef ESIX
	void	mark_socket _((int));
	void	unmark_socket _((int));
#endif
	time_t	dgets_timeout _((int));
        int	dgets _((char *, int, int, char *));
/**************************** PATCHED by Flier ******************************/
#if defined(HAVE_SSL)
        int     SSL_dgets _((char *, int, int, gnutls_session_t *));
#elif defined(HAVE_OPENSSL)
        int     SSL_dgets _((char *, int, int, SSL *));
#endif
/****************************************************************************/
	int	new_select _((fd_set *, fd_set *, struct timeval *));
	void	new_close _((int));
	void	set_socket_options _((int));

#endif /* __newio_h_ */
