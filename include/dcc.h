/*
 * dcc.h: Things dealing client to client connections. 
 *
 * Written By Troy Rollo <troy@plod.cbme.unsw.oz.au> 
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
 * $Id: dcc.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

/*
 * this file must be included after irc.h as i needed <sys/types.h>
 * <netinet/in.h> and <apra/inet.h> and, possibly, <sys/select.h>
 */

#ifndef __dcc_h_
#define __dcc_h_

#define DCC_CHAT	((unsigned) 0x0001)
#define DCC_FILEOFFER	((unsigned) 0x0002)
#define DCC_FILEREAD	((unsigned) 0x0003)
/**************************** PATCHED by Flier ******************************/
/*#define DCC_TALK	((unsigned) 0x0004)
#define DCC_SUMMON	((unsigned) 0x0005)
#define	DCC_RAW_LISTEN	((unsigned) 0x0006)
#define	DCC_RAW		((unsigned) 0x0007)*/
#define	DCC_RAW_LISTEN	((unsigned) 0x0004)
#define	DCC_RAW		((unsigned) 0x0005)
#define DCC_RESENDOFFER ((unsigned) 0x0006)
#define DCC_FILEREGET   ((unsigned) 0x0007)
/****************************************************************************/
#define DCC_TYPES	((unsigned) 0x000f)

#define DCC_WAIT	((unsigned) 0x0010)
#define DCC_ACTIVE	((unsigned) 0x0020)
#define DCC_OFFER	((unsigned) 0x0040)
#define DCC_DELETE	((unsigned) 0x0080)
#define DCC_TWOCLIENTS	((unsigned) 0x0100)
#ifdef NON_BLOCKING_CONNECTS
#define DCC_CNCT_PEND	((unsigned) 0x0200)
#endif /* NON_BLOCKING_CONNECTS */
#define DCC_STATES	((unsigned) 0xfff0)

/**************************** PATCHED by Flier ******************************/
/*#define DCC_TALK_CHECK 0
#define DCC_TALK_INVITE 1
#define DCC_TALK_ANNOUNCE 2
#define DCC_TALK_DELETE_LOCAL 3
#define DCC_TALK_DELETE_REMOTE 4
#define DCC_TALK_SUMMON 5
#define DCC_TALK_DELETE_SUMMON 6*/

#define DCC_PACKETID  0xfeab

struct transfer_struct {
	unsigned short packet_id;
	unsigned char byteorder;
	unsigned long byteoffset;
}; 
/****************************************************************************/

	DCC_list * dcc_searchlist _((char *, char *, int, int, char *));
	void	dcc_erase _((DCC_list *));
	void	register_dcc_offer _((char *, char *, char *, char *, char *, char *));
	void	process_dcc _((char *));
	char	*dcc_raw_connect _((char *, u_short));
	char	*dcc_raw_listen _((u_short));
	void	dcc_list _((char *));
	void	dcc_chat_transmit _((char *, char *));
	void	dcc_message_transmit _((char *, char *, int, int));
/**************************** PATCHED by Flier ******************************/
	/*int	send_talk_control _((DCC_list *, int));*/
/****************************************************************************/
	void	close_all_dcc _((void));
	void	set_dcc_bits _((fd_set *, fd_set *));
	void	dcc_check _((fd_set *, fd_set *));
/**************************** PATCHED by Flier ******************************/
        unsigned char byteordertest _((void));
/****************************************************************************/

#endif /* __dcc_h_ */
