/*
 * edit.h: header for edit.c 
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
 * $Id: edit.h,v 1.5 2002-01-21 22:18:17 f Exp $
 */

#ifndef __edit_h_
#define __edit_h_

extern	u_char	*sent_nick;
extern	u_char	*sent_body;
extern	u_char	*recv_nick;

	void	load _((char *, char *, char *));
	void	send_text _((char *, char *, char *));
	void	eval_inputlist _((char *, char *));
	void	parse_command _((char *, int, char *));
	void	parse_line _((char *, char *, char *, int, int, int));
	void	edit_char _((u_int));
	void	execute_timer _((void));
	void	ison_now _((WhoisStuff *, char *, char *));
	void	redirect_msg _((char *, char *));
	void	query _((char *, char *, char *));
	void	forward_character _((u_int, char *));
	void	backward_character _((u_int, char *));
	void	forward_history _((u_int, char *));
	void	backward_history _((u_int, char *));
	void	toggle_insert_mode _((u_int, char *));
	void	send_line _((u_int, char *));
	void	meta1_char _((u_int, char *));
	void	meta2_char _((u_int, char *));
	void	meta3_char _((u_int, char *));
	void	meta4_char _((u_int, char *));
	void	meta5_char _((u_int, char *));
	void	meta6_char _((u_int, char *));
	void	meta7_char _((u_int, char *));
	void	meta8_char _((u_int, char *));
	void	quote_char _((u_int, char *));
	void	type_text _((u_int, char *));
	void	parse_text _((u_int, char *));
	void	irc_clear_screen _((u_int, char *));
	void	command_completion _((u_int, char *));
	void	e_quit _((char *, char *, char *));
	int	check_wait_command _((char *));
	
#define AWAY_ONE 0
#define AWAY_ALL 1

#define STACK_POP 0
#define STACK_PUSH 1
#define STACK_SWAP 2

/* a structure for the timer list */
typedef struct	timerlist_stru
{
	int	ref;
	int	in_on_who;
	time_t	time;
	unsigned microseconds;
/**************************** PATCHED by Flier ******************************/
        int     visible;
        void    (*func)();
/****************************************************************************/
        int     server;
	char	*command;
	struct	timerlist_stru *next;
}	TimerList;

extern TimerList *PendingTimers;

#endif /* __edit_h_ */
