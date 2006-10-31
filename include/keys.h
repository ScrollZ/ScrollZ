/*
 * keys.h: header for keys.c 
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
 * $Id: keys.h,v 1.1 2006-10-31 12:31:27 f Exp $
 */

#ifndef __keys_h_
#define __keys_h_

/* KeyMap: the structure of the irc keymaps */
typedef struct
{
	int	index;
	char	changed;
	int	global;
	char	*stuff;
}	KeyMap;

/* KeyMapNames: the structure of the keymap to realname array */
typedef struct
{
	char	*name;
 	void	(*func) _((u_int, char *));
}	KeyMapNames;

extern	KeyMap	keys[],
		meta1_keys[],
		meta2_keys[],
		meta3_keys[],
		meta4_keys[],
		meta5_keys[],
		meta6_keys[],
		meta7_keys[],
		meta8_keys[];
extern	KeyMapNames key_names[];

 	void	(* get_send_line _((void))) _((u_int, char*));
	void	save_bindings _((FILE *, int));
 	void	change_send_line _((void (*)(u_int, char *)));
	void	bindcmd _((char *, char *, char *));
	void	rbindcmd _((char *, char *, char *));
	void	parsekeycmd _((char *, char *, char *));
	void	typecmd _((char *, char *, char *));

enum {
	BACKSPACE = 0,
	BACKWARD_CHARACTER,
	BACKWARD_HISTORY,
	BACKWARD_WORD,
	BEGINNING_OF_LINE,
	CLEAR_SCREEN,
	COMMAND_COMPLETION,
	DELETE_CHARACTER,
	DELETE_NEXT_WORD,
	DELETE_PREVIOUS_WORD,
	END_OF_LINE,
	ENTER_DIGRAPH,
	ENTER_MENU,
	ERASE_LINE,
	ERASE_TO_BEG_OF_LINE,
	ERASE_TO_END_OF_LINE,
	FORWARD_CHARACTER,
	FORWARD_HISTORY,
	FORWARD_WORD,
	META1_CHARACTER,
	META2_CHARACTER,
	META3_CHARACTER,
	META4_CHARACTER,
	META5_CHARACTER,
	META6_CHARACTER,
	META7_CHARACTER,
	META8_CHARACTER,
	NEXT_WINDOW,
	NOTHING,
	PARSE_COMMAND,
	PREVIOUS_WINDOW,
	QUIT_IRC,
	QUOTE_CHARACTER,
	REFRESH_INPUTLINE,
	REFRESH_SCREEN,
	SCROLL_BACKWARD,
	SCROLL_END,
	SCROLL_FORWARD,
	SCROLL_START,
	SELF_INSERT,
	SEND_LINE,
	STOP_IRC,
	SWAP_LAST_WINDOW,
	SWAP_NEXT_WINDOW,
	SWAP_PREVIOUS_WINDOW,
	SWITCH_CHANNELS,
	TOGGLE_INSERT_MODE,
	TOGGLE_STOP_SCREEN,
	TRANSPOSE_CHARACTERS,
	TYPE_TEXT,
	UNSTOP_ALL_WINDOWS,
	YANK_FROM_CUTBUFFER,
/**************************** PATCHED by Flier ******************************/
	INSERT_AUTOREPLY,
	LASTJOINER_KICK,
	ACCEPT_LAST_CHAT,
	INSERT_TABKEY_NEXT,
	INSERT_TABKEY_PREV,
/****************************************************************************/
	NUMBER_OF_FUNCTIONS
};
#endif /* __keys_h_ */
