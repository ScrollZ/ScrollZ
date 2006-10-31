/*
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
 * $Id: hook.h,v 1.1 2006-10-31 12:31:27 f Exp $
 */

#ifndef __hook_h_
# define __hook_h_

/**************************** PATCHED by Flier ******************************/
#include "defs.h"
/****************************************************************************/

/* Hook: The structure of the entries of the hook functions lists */
typedef struct	hook_stru
{
	struct	hook_stru *next;	/* pointer to next element in list */
	char	*nick;			/* The Nickname */
	int	not;			/* If true, this entry should be
					 * ignored when matched, otherwise it
					 * is a normal entry */
	int	noisy;			/* flag indicating how much output
					 * should be given */
	int	server;			/* the server in which this hook
					 * applies. (-1 if none). If bit 0x1000
					 * is set, then no other hooks are
					 * tried in the given server if all the
					 * server specific ones fail
					 */
	int	sernum;			/* The serial number for this hook. This
					 * is used for hooks which will be
					 * concurrent with others of the same
					 * pattern. The default is 0, which
					 * means, of course, no special
					 * behaviour. If any 1 hook suppresses
					 * the * default output, output will be
					 * suppressed.
					 */
	char	*stuff;			/* The this that gets done */
	int	global;			/* set if loaded from `global' */
}	Hook;

/* HookFunc: A little structure to keep track of the various hook functions */
typedef struct
{
	char	*name;			/* name of the function */
	Hook	*list;			/* pointer to head of the list for this
					 * function */
	int	params;			/* number of parameters expected */
	int	mark;
	unsigned flags;
}	HookFunc;

/*
 * NumericList: a special list type to dynamically handle numeric hook
 * requests 
 */
typedef struct numericlist_stru
{
	struct	numericlist_stru *next;
	char	*name;
	Hook	*list;
}	NumericList;

#ifdef HAVE_STDARG_H
	int	do_hook _((int, char *, ...));
#else
	int	do_hook _(());
#endif /* HAVE_STDARG_H */
	void	on _((char *, char *, char *));
	void	save_hooks _((FILE *, int));
	void	remove_hook _((int, char *, int, int, int));
	void	show_hook _((Hook *, char *));

extern	char	*hook_info;
extern	NumericList *numeric_list;
extern	HookFunc hook_functions[];

extern	int	in_on_who;

enum {
	ACTION_LIST = 0,
/**************************** PATCHED by Flier ******************************/
	CDCC_PLIST,
	CDCC_PLIST_FOOTER,
	CDCC_PLIST_HEADER,
/****************************************************************************/
	CHANNEL_NICK_LIST,
	CHANNEL_SIGNOFF_LIST,
/**************************** PATCHED by Flier ******************************/
	CHANNEL_SYNCH_LIST,
	CHANNEL_WALLOP_LIST,
/****************************************************************************/
	CONNECT_LIST,
	CTCP_LIST,
	CTCP_REPLY_LIST,
	DCC_CHAT_LIST,
	DCC_CONNECT_LIST,
	DCC_ERROR_LIST,
/**************************** PATCHED by Flier ******************************/
	DCC_LIST,
	DCC_LIST_FOOTER,
	DCC_LIST_HEADER,
/****************************************************************************/
	DCC_LOST_LIST,
	DCC_RAW_LIST,
	DCC_REQUEST_LIST,
	DISCONNECT_LIST,
	ENCRYPTED_NOTICE_LIST,
	ENCRYPTED_PRIVMSG_LIST,
	EXEC_LIST,
	EXEC_ERRORS_LIST,
	EXEC_EXIT_LIST,
	EXEC_PROMPT_LIST,
	EXIT_LIST,
	FLOOD_LIST,
	HELP_LIST,
	HOOK_LIST,
	IDLE_LIST,
	INPUT_LIST,
	INVITE_LIST,
	JOIN_LIST,
/**************************** PATCHED by Flier ******************************/
	JOIN_ME_LIST,
/****************************************************************************/
	KICK_LIST,
	LEAVE_LIST,
	LIST_LIST,
	MAIL_LIST,
	MODE_LIST,
	MSG_LIST,
	MSG_GROUP_LIST,
	NAMES_LIST,
	NICKNAME_LIST,
	NOTE_LIST,
	NOTICE_LIST,
	NOTIFY_SIGNOFF_LIST,
/**************************** PATCHED by Flier ******************************/
	NOTIFY_SIGNOFF_UH_LIST,
/****************************************************************************/
	NOTIFY_SIGNON_LIST,
/**************************** PATCHED by Flier ******************************/
	NOTIFY_SIGNON_UH_LIST,
/****************************************************************************/
	PUBLIC_LIST,
	PUBLIC_MSG_LIST,
	PUBLIC_NOTICE_LIST,
	PUBLIC_OTHER_LIST,
	RAW_IRC_LIST,
	RAW_SEND_LIST,
	SEND_ACTION_LIST,
/**************************** PATCHED by Flier ******************************/
	SEND_CTCP_LIST,
/****************************************************************************/
	SEND_DCC_CHAT_LIST,
	SEND_MSG_LIST,
	SEND_NOTICE_LIST,
	SEND_PUBLIC_LIST,
	SERVER_NOTICE_LIST,
	SIGNOFF_LIST,
	TIMER_LIST,
	TOPIC_LIST,
	WALL_LIST,
	WALLOP_LIST,
	WHO_LIST,
	WIDELIST_LIST,
	WINDOW_LIST,
	WINDOW_KILL_LIST,
	WINDOW_SWAP_LIST,
	NUMBER_OF_LISTS
};
#endif /* __hook_h_ */
