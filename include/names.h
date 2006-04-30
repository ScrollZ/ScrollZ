/*
 * names.h: Header for names.c
 *
 * Written By Michael Sandrof
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
 * $Id: names.h,v 1.12 2006-04-30 14:15:43 f Exp $
 */

#ifndef __names_h_
#define __names_h_

#include "window.h"
#include "irc.h"

/**************************** PATCHED by Flier ******************************/
#include "mystructs.h"
/****************************************************************************/

/*
 * MODE_STRING and other MODE_FOO defines need to be kept in sync.
 * MODE_STRING refers to the bits in the mode bitmask.  the letters
 * and numbers need to match up.
 */
#define MODE_STRING	"aciklmnpqrstRSgQz"
#define MODE_ANONYMOUS	((u_long) 0x0001)
#define MODE_COLOURLESS	((u_long) 0x0002)
#define MODE_INVITE	((u_long) 0x0004)
#define MODE_KEY	((u_long) 0x0008)
#define MODE_LIMIT	((u_long) 0x0010)
#define MODE_MODERATED	((u_long) 0x0020)
#define MODE_MSGS	((u_long) 0x0040)
#define MODE_PRIVATE	((u_long) 0x0080)
#define MODE_QUIET	((u_long) 0x0100)
#define MODE_REOP	((u_long) 0x0200)
#define MODE_SECRET	((u_long) 0x0400)
#define MODE_TOPIC	((u_long) 0x0800)
#define MODE_REGONLY	((u_long) 0x1000)
/**************************** Patched by Flier ******************************/
#define MODE_SSLONLY	((u_long) 0x2000)
#define MODE_ALLINVITE	((u_long) 0x4000)
#define MODE_NOFORWARD	((u_long) 0x8000)
#define MODE_REDUCEMODERATED	((u_long) 0x10000)
/****************************************************************************/

/**************************** Patched by Flier ******************************/
#define HAS_OPS(channel) ((channel & CHAN_CHOP) || (channel & CHAN_HALFOP))
/****************************************************************************/

/* for lookup_channel() */
#define	CHAN_NOUNLINK	1
#define CHAN_UNLINK	2

	int	is_channel_mode _((char *, int, int));
	int	is_chanop _((char *, char *));
	int	has_voice _((char *, char *, int));
	ChannelList	*lookup_channel _((char *, int, int));
/**************************** PATCHED by Flier ******************************/
        void    rename_channel _((char *, char *));
/****************************************************************************/
	char	*get_channel_mode _((char *, int));
#ifdef	INCLUDE_UNUSED_FUNCTIONS
	void	set_channel_mode _((char *, int, char *));
#endif /* INCLUDE_UNUSED_FUNCTIONS */
/**************************** PATCHED by Flier ******************************/
	/*void	add_channel _((char *, int, int, ChannelList *));
	void	add_to_channel _((char *, char *, int, int, int));*/
	void	add_channel _((char *, int, int, ChannelList *, char *, int));
        ChannelList *add_to_channel _((char *, char *, int, int, int, int, char *, ChannelList *));
/****************************************************************************/
	void	remove_channel _((char *, int));
	void	remove_from_channel _((char *, char *, int));
	int	is_on_channel _((char *, int, char *));
	void	list_channels _((void));
	void	reconnect_all_channels _((int));
 	void	switch_channels _((u_int, char *));
	char	*what_channel _((char *, int));
	char	*walk_channels _((char *, int, int));
	void	rename_nick _((char *, char *, int));
/**************************** PATCHED by Flier ******************************/
        /*void	update_channel_mode _((char *, int, char *));*/
        void	update_channel_mode _((char *, int, char *, int, char *, char *, char *, char *, ChannelList *));
/****************************************************************************/
	void	set_channel_window _((Window *, char *, int));
	char	*create_channel_list _((Window *));
	int	get_channel_oper _((char *, int));
	void	channel_server_delete _((int));
	void	change_server_channels _((int, int));
	void	clear_channel_list _((int));
	void	set_waiting_channel _((int));
	int	chan_is_connected _((char *, int));
	void	mark_not_connected _((int));

#endif /* __names_h_ */
