/*
 * input.h: header for input.c 
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
 * $Id: input.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

#ifndef __input_h_
#define __input_h_

	char	input_pause _((char *));
	void	set_input _((char *));
	void	set_input_prompt _((char *));
	char	*get_input_prompt _((void));
	char	*get_input _((void));
	void	update_input _((int));
	void	init_input _((void));
	void	input_move_cursor _((int));
	void	change_input_prompt _((int));
	void	cursor_to_input _((void));
	void	input_add_character _((unsigned char, char *));
	void	input_backward_word _((unsigned char, char *));
	void	input_forward_word _((unsigned char, char *));
	void	input_delete_previous_word _((unsigned char, char *));
	void	input_delete_next_word _((unsigned char, char *));
	void	input_clear_to_bol _((unsigned char, char *));
	void	input_clear_line _((unsigned char, char *));
	void	input_end_of_line _((unsigned char, char *));
	void	input_clear_to_eol _((unsigned char, char *));
	void	input_beginning_of_line _((unsigned char, char *));
	void	refresh_inputline _((unsigned char, char *));
	void	input_delete_character _((unsigned char, char *));
	void	input_backspace _((unsigned char, char *));
	void	input_transpose_characters _((unsigned char, char *));
	void	input_yank_cut_buffer _((unsigned char, char *));

extern	int	str_start;
extern	int	input_line;

/* used by update_input */
#define NO_UPDATE 0
#define UPDATE_ALL 1
#define UPDATE_FROM_CURSOR 2
#define UPDATE_JUST_CURSOR 3

#endif /* __input_h_ */
