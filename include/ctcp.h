/*
 * ctcp.h: header file for ctcp.c
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
 * $Id: ctcp.h,v 1.2 1999-02-15 21:18:19 f Exp $
 */

#ifndef __ctcp_h_
#define __ctcp_h_

#define CTCP_DELIM_CHAR '\001'
#define CTCP_DELIM_STR "\001"
#define CTCP_QUOTE_CHAR '\\'
#define CTCP_QUOTE_STR "\\"

#define CTCP_QUOTE_EM "\n\r\001\\"

#define CTCP_PRIVMSG 0
#define CTCP_NOTICE 1

/**************************** PATCHED by Flier ******************************/
#define	CTCP_SED 0
#define CTCP_VERSION 1
#define CTCP_CLIENTINFO 2
#define	CTCP_USERINFO 3
#define	CTCP_ERRMSG 4
#define	CTCP_FINGER 5
#define	CTCP_TIME 6
#define CTCP_ACTION 7
#define	CTCP_DCC_CHAT 8
#define	CTCP_UCT 9
#define CTCP_PING 10
#define CTCP_ECHO 11
#define CTCP_INVITE 12
#define CTCP_OP 13
#define CTCP_UNBAN 14
#define CTCP_CHOPS 15
#define CTCP_VOICE 16
#define CTCP_WHOAMI 17
#define CTCP_HELP 18
#define CTCP_VER 19
#define CTCP_CDCC 20
#define CTCP_XDCC 21
#define CTCP_OPEN 22
#ifdef CTCPPAGE
#define CTCP_PAGE 23
#define	NUMBER_OF_CTCPS 24
#else
#define	NUMBER_OF_CTCPS 23
#endif
/****************************************************************************/

#define CTCP_CRYPTO_TYPE "SED"
#define CTCP_CRYPTO_NAME "contains simple encrypted data"
#define CTCP_CRYPTO_LEN  4

extern	char	*ctcp_type[];
extern	int	sed;

	char	*do_ctcp _((char *, char *, char *));
 	char	*ctcp_quote_it _((char *, size_t));
 	char	*ctcp_unquote_it _((char *, size_t *));
	char	*do_notice_ctcp _((char *, char *, char *));
	int	in_ctcp _((void));
#ifdef HAVE_STDARG_H
	void    send_ctcp_reply _((char *, char *, char *, ...));
	void    send_ctcp _((char *, char *, char *, char *, ...));
#else
	void    send_ctcp_reply _(());
	void    send_ctcp _(());
#endif /* HAVE_STDARG_H */

#endif /* __ctcp_h_ */
