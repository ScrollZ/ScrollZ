/*
 * notify.h: header for notify.c
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
 * $Id: notify.h,v 1.4 2002-05-04 16:43:39 f Exp $
 */

#ifndef __notify_h_
#define __notify_h_

	char *get_notify_list _((int));
#define NOTIFY_LIST_HERE 	0x1
#define NOTIFY_LIST_GONE 	0x2
#define NOTIFY_LIST_ALL		0x3
	void	show_notify_list _((int));
	void	notify _((char *, char *, char *));
	void	do_notify _((void));
	void	notify_mark _((char *, int, int));
	void	save_notify _((FILE *));
	void	set_notify_handler _((char *));

/**************************** PATCHED by Flier ******************************/
/* moved here rather than having it in notify.c - Flier */
/* NotifyList: the structure for the notify stuff */
typedef	struct	notify_stru
{
	struct	notify_stru	*next;	/* pointer to next notify person */
	char	*nick;			/* nickname of person to notify about */
/**************************** PATCHED by Flier ******************************/
        int     server;                 /* server number so we can update u@h */
        int     isfriend;               /* 1 if friend, 0 otherwise */
        int     printed;                /* used when showing users on notify list */
        char    *userhost;              /* their userhost */
        char    *mask;                  /* nick!user@host for user@host based notify */
        char    *group;                 /* for grouping users */
/****************************************************************************/
        int	flag;			/* 1=person on irc, 0=person not on irc */
}	NotifyList;
/****************************************************************************/

#endif /* __notify_h_ */
