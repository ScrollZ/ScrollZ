/*
 * keys.c: Does command line parsing, etc 
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
 * $Id: keys.c,v 1.6 2000-08-14 20:38:13 f Exp $
 */

#include "irc.h"

#include "output.h"
#include "keys.h"
#include "names.h"
#include "ircaux.h"
#include "window.h"
#include "edit.h"
#include "vars.h"
#include "translat.h"
#include "ircterm.h"
#include "input.h"
#include "screen.h"
#include "menu.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

extern void InsertAutoReply _((u_int, char *));
extern void InsertNick _((u_int, char *));
extern void LastJoinerKick _((u_int, char *));
extern void AcceptLastChat _((u_int, char *));
extern void HandleTabNext _((u_int, char *));
extern void HandleTabPrev _((u_int, char *));
/****************************************************************************/

static	int	lookup_function _((char *, int *));
static	u_char	* display_key _((u_int));
static	void	show_binding _((u_int, int));
static	int	parse_key _((u_char *));
static	void	write_binding _((u_int, u_int, FILE *, int));
static	void	bind_it _((char *, char *, u_int, int));

/*
 * lookup_function: looks up an irc function by name, and returns the
 * number of functions that match the name, and sets where index points
 * to to be the index of the (first) function found.
 */
static int
lookup_function(name, func_index)
	char	*name;
	int	*func_index;
{
 	size_t	len;
 	int	cnt,
		i;

	if (name)
	{
		upper(name);
		len = strlen(name);
		cnt = 0;
		*func_index = -1;
		for (i = 0; i < NUMBER_OF_FUNCTIONS; i++)
		{
			if (strncmp(name, key_names[i].name, len) == 0)
			{
				cnt++;
				if (*func_index == -1)
					*func_index = i;
			}
		}
		if (*func_index == -1)
			return (0);
		if (strcmp(name, key_names[*func_index].name) == 0)
			return (1);
		else
			return (cnt);
	}
	return (0);
}

/*
 * display_key: converts the character c to a displayable form and returns
 * it.  Very simple indeed 
 */
static	u_char	*
display_key(c)
	u_int c;
{
	static	u_char key[3];

	key[2] = (u_char) 0;
	if (c < 32)
	{
		key[0] = '^';
		key[1] = c + 64;
	}
	else if (c == '\177')
	{
		key[0] = '^';
		key[1] = '?';
	}
	else
	{
		key[0] = c;
		key[1] = (u_char) 0;
	}
	return (key);
}

/*
 * show_binding: given the ascii value of a key and a meta key status (1 for
 * meta1 keys, 2 for meta2 keys, anything else for normal keys), this will
 * display the key binding for the key in a nice way
 */
static void
show_binding(c, meta)
	u_int	c;
	int	meta;
{
	KeyMap	*map;
	char	*meta_str;

	switch (meta)
	{
	case 1:
		map = meta1_keys;
		meta_str = "META1-";
		break;
	case 2:
		map = meta2_keys;
		meta_str = "META2-";
		break;
	case 3:
		map = meta3_keys;
		meta_str = "META3-";
		break;
	case 4:
		map = meta4_keys;
		meta_str = "META4-";
		break;
	case 5:
		map = meta5_keys;
		meta_str = "META5-";
		break;
	case 6:
		map = meta6_keys;
		meta_str = "META6-";
		break;
	case 7:
		map = meta7_keys;
		meta_str = "META7-";
		break;
	case 8:
		map = meta8_keys;
		meta_str = "META8-";
		break;
	default:
		map = keys;
		meta_str = empty_string;
		break;
	}
	say("%s%s is bound to %s %s", meta_str, display_key(c),
		key_names[map[c].index].name, (map[c].stuff &&
		(*(map[c].stuff))) ? map[c].stuff : empty_string);
}

/*
 * parse_key: converts a key string. Accepts any key, or ^c where c is any
 * key (representing control characters), or META1- or META2- for meta1 or
 * meta2 keys respectively.  The string itself is converted to true ascii
 * value, thus "^A" is converted to 1, etc.  Meta key info is removed and
 * returned as the function value, 0 for no meta key, 1 for meta1, and 2 for
 * meta2.  Thus, "META1-a" is converted to "a" and a 1 is returned.
 * Furthermore, if ^X is bound to META2_CHARACTER, and "^Xa" is passed to
 * parse_key(), it is converted to "a" and 2 is returned.  Do ya understand
 * this? 
 */
static int
parse_key(key_str)
	u_char	*key_str;
{
	u_char	*ptr1, *ptr2;
	u_char	c;
	int	meta = 0;

	ptr2 = ptr1 = key_str;
	while (*ptr1)
	{
		if (*ptr1 == '^')
		{
			ptr1++;
			switch (*ptr1)
			{
			case 0:
				*(ptr2++) = '^';
				break;
			case '?':
				*(ptr2++) = '\177';
				ptr1++;
				break;
			default:
				c = *(ptr1++);
				if (islower(c))
					c = toupper(c);
				if (c < 64)
				{
					say("Illegal key sequence: ^%c", c);
					return (-1);
				}
				*(ptr2++) = c - 64;
			}
		}
		else
			*(ptr2++) = *(ptr1++);
	}
	*ptr2 = (char) 0;
	if ((int) strlen((char *) key_str) > 1)
	{
		char	*cmd = NULL;

		malloc_strcpy(&cmd, (char *)key_str);
		upper(cmd);
		if (strncmp(cmd, "META1-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 1;
		}
		else if (strncmp(cmd, "META2-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 2;
		}
		else if (strncmp(cmd, "META3-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 3;
		}
		else if (strncmp(cmd, "META4-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 4;
		}
		else if (strncmp(cmd, "META5-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 5;
		}
		else if (strncmp(cmd, "META6-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 6;
		}
		else if (strncmp(cmd, "META7-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 7;
		}
		else if (strncmp(cmd, "META8-", 6) == 0)
		{
			strcpy((char *) key_str, (char *) key_str + 6);
			meta = 8;
		}
		else if (keys[(u_char) *key_str].index == META1_CHARACTER)
		{
			meta = 1;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META2_CHARACTER)
		{
			meta = 2;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META3_CHARACTER)
		{
			meta = 3;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META4_CHARACTER)
		{
			meta = 4;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META5_CHARACTER)
		{
			meta = 5;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META6_CHARACTER)
		{
			meta = 6;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META7_CHARACTER)
		{
			meta = 7;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else if (keys[(u_char) *key_str].index == META8_CHARACTER)
		{
			meta = 8;
			strcpy((char *) key_str, (char *) key_str + 1);
		}
		else
		{
			say("Illegal key sequence: %s is not a meta-key", display_key(*key_str));
			return (-1);
		}
 		new_free(&cmd);
	}
	return (meta);
}

/*
 * bind_it: does the actually binding of the function to the key with the
 * given meta modifier
 */
static	void
bind_it(function, string, key, meta)
	char	*function,
		*string;
	u_int	key;
	int	meta;
{
	KeyMap	*km;
	int	cnt,
		func_index,
		i;

	switch (meta)
	{
	case 0:
		km = keys;
		break;
	case 1:
		km = meta1_keys;
		break;
	case 2:
		km = meta2_keys;
		break;
	case 3:
		km = meta3_keys;
		break;
	case 4:
		km = meta4_keys;
		break;
	case 5:
		km = meta5_keys;
		break;
	case 6:
		km = meta6_keys;
		break;
	case 7:
		km = meta7_keys;
		break;
	case 8:
		km = meta8_keys;
		break;
	default:
		km = keys;
	}
	if (*string == (char) 0)
		string = (char *) 0;
	switch (cnt = lookup_function(function, &func_index))
	{
	case 0:
		say("No such function: %s", function);
		break;
	case 1:
		if (! km[key].changed)
		{
			if ((km[key].index != func_index) ||
					((string == (char *) 0) &&
					km[key].stuff) ||
					((km[key].stuff == (char *) 0) &&
					string) || (string && km[key].stuff &&
					strcmp(km[key].stuff,string)))
/**************************** PATCHED by Flier ******************************/
				/*km[key].changed = 1;*/
				if (usersloaded) km[key].changed = 1;
/****************************************************************************/
		}
		km[key].index = func_index;
		km[key].global = loading_global;
		malloc_strcpy(&(km[key].stuff), string);
/**************************** PATCHED by Flier ******************************/
		/*show_binding(key, meta);*/
		if (usersloaded) show_binding(key, meta);
/****************************************************************************/
		break;
	default:
		say("Ambiguous function name: %s", function);
		for (i = 0; i < cnt; i++, func_index++)
			put_it("%s", key_names[func_index].name);
		break;
	}
}

/* parsekeycmd: does the PARSEKEY command.  */
void
parsekeycmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	i;
	char	*arg;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		switch (lookup_function(arg, &i))
		{
		case 0:
			say("No such function %s", arg);
			return;
		case 1:
 			key_names[i].func(0, args);
			break;
		default:
			say("Ambigious function %s", arg);
			break;
		}
	}
}

/*
 * bindcmd: the bind command, takes a key sequence followed by a function
 * name followed by option arguments (used depending on the function) and
 * binds a key.  If no function is specified, the current binding for that
 * key is shown 
 */
/*ARGSUSED*/
void
bindcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	u_char	*key;
	char	*function;
	int	meta;
/**************************** PATCHED by Flier ******************************/
        char    tmpbuf[mybufsize/64];
/****************************************************************************/

	if ((key = (unsigned char *) next_arg(args, &args)) != NULL)
	{
/**************************** PATCHED by Flier ******************************/
                if (!my_stricmp(key,"F1")) {
                    strcpy(tmpbuf,"meta5-A");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F2")) {
                    strcpy(tmpbuf,"meta5-B");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F3")) {
                    strcpy(tmpbuf,"meta5-C");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F4")) {
                    strcpy(tmpbuf,"meta5-D");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F5")) {
                    strcpy(tmpbuf,"meta5-E");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F6")) {
                    strcpy(tmpbuf,"meta5-7");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F7")) {
                    strcpy(tmpbuf,"meta5-8");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F8")) {
                    strcpy(tmpbuf,"meta5-9");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F9")) {
                    strcpy(tmpbuf,"meta5-0");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F10")) {
                    strcpy(tmpbuf,"meta5-1");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F11")) {
                    strcpy(tmpbuf,"meta5-3");
                    key=tmpbuf;
                }
                else if (!my_stricmp(key,"F12")) {
                    strcpy(tmpbuf,"meta5-4");
                    key=tmpbuf;
                }
/****************************************************************************/
		if ((meta = parse_key(key)) == -1)
			return;
		if ((int) strlen((char *) key) > 1)
		{
			say("Key sequences may not contain more than two keys");
			return;
		}
		if ((function = next_arg(args, &args)) != NULL)
			bind_it(function, args, *key, meta);
		else
			show_binding(*key, meta);
	}
	else
	{
 		u_int	i;
		int	charsize = charset_size();

		for (i = 0; i < charsize; i++)
		{
			if ((keys[i].index != NOTHING) && (keys[i].index !=
					SELF_INSERT))
				show_binding(i, 0);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta1_keys[i].index != NOTHING) &&
					(meta1_keys[i].index != SELF_INSERT))
				show_binding(i, 1);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta2_keys[i].index != NOTHING) &&
					(meta2_keys[i].index != SELF_INSERT))
				show_binding(i, 2);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta3_keys[i].index != NOTHING) &&
					(meta3_keys[i].index != SELF_INSERT))
				show_binding(i, 3);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta4_keys[i].index != NOTHING) &&
					(meta4_keys[i].index != SELF_INSERT))
				show_binding(i, 4);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta5_keys[i].index != NOTHING) &&
					(meta5_keys[i].index != SELF_INSERT))
				show_binding(i, 5);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta6_keys[i].index != NOTHING) &&
					(meta6_keys[i].index != SELF_INSERT))
				show_binding(i, 6);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta7_keys[i].index != NOTHING) &&
					(meta7_keys[i].index != SELF_INSERT))
				show_binding(i, 7);
		}
		for (i = 0; i < charsize; i++)
		{
			if ((meta8_keys[i].index != NOTHING) &&
					(meta8_keys[i].index != SELF_INSERT))
				show_binding(i, 8);
		}
	}
}

/*
 * rbindcmd: does the rbind command.  you give it a string that something
 * is bound to and it tells you all the things that are bound to that
 * functions
 */
void
rbindcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	f;
	char	*arg;

	if ((arg = next_arg(args, &args)) != NULL)
	{
 		u_int	i;
		int	charsize = charset_size();

		switch (lookup_function(arg, &f))
		{
		case 0:
			say("No such function %s", arg);
			return;

		case 1:
			break;

		default:
			say("Ambigious function %s", arg);
			return;
		}

		for (i = 0; i < charsize; i++)
			if (f == keys[i].index)
				show_binding(i, 0);
		for (i = 0; i < charsize; i++)
			if (f == meta1_keys[i].index)
				show_binding(i, 1);
		for (i = 0; i < charsize; i++)
			if (f == meta2_keys[i].index)
				show_binding(i, 2);
		for (i = 0; i < charsize; i++)
			if (f == meta3_keys[i].index)
				show_binding(i, 3);
		for (i = 0; i < charsize; i++)
			if (f == meta4_keys[i].index)
				show_binding(i, 4);
		for (i = 0; i < charsize; i++)
			if (f == meta5_keys[i].index)
				show_binding(i, 5);
		for (i = 0; i < charsize; i++)
			if (f == meta6_keys[i].index)
				show_binding(i, 6);
		for (i = 0; i < charsize; i++)
			if (f == meta7_keys[i].index)
				show_binding(i, 7);
		for (i = 0; i < charsize; i++)
			if (f == meta8_keys[i].index)
				show_binding(i, 8);
	}
}

void (*
get_send_line _((void))) _((u_int, char *))
{
 	return (void (*) _((u_int, char *))) key_names[SEND_LINE].func;
}

/*
 * change_send_line: Allows you to change the everything bound to SENDLINE in
 * one fell swoop.  Used by the various functions that gather input using the
 * normal irc interface but dont wish to parse it and send it to the server.
 * Sending NULL resets it to send_line()
 */
void
change_send_line(func)
 	void	(*func) _((u_int, char *));
{
	if (func)
		key_names[SEND_LINE].func = func;
	else
		key_names[SEND_LINE].func = send_line;
}

/*
 * type: The TYPE command.  This parses the given string and treats each
 * character as tho it were typed in by the user.  Thus key bindings are used
 * for each character parsed.  Special case characters are control character
 * sequences, specified by a ^ follow by a legal control key.  Thus doing
 * "/TYPE ^B" will be as tho ^B were hit at the keyboard, probably moving the
 * cursor backward one character.
 */
/*ARGSUSED*/
void
type(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	c;
	char	key;

	while (*args)
	{
		if (*args == '^')
		{
			switch (*(++args))
			{
			case '?':
				key = '\177';
				args++;
				break;
			default:
				c = *(args++);
				if (islower(c))
					c = toupper(c);
				if (c < 64)
				{
					say("Illegal key sequence: ^%c", c);
					return;
				}
				key = c - 64;
				break;
			}
		}
		else if (*args == '\\')
		{
			key = *++args;
			args++;
		}
		else
			key = *(args++);
 		edit_char((u_int)key);
	}
}

/* The string values for these *MUST* be in ALL CAPITALS */
KeyMapNames FAR key_names[] =
{
	{ "BACKSPACE",			input_backspace },
	{ "BACKWARD_CHARACTER",		backward_character },
	{ "BACKWARD_HISTORY",		backward_history },
	{ "BACKWARD_WORD",		input_backward_word },
	{ "BEGINNING_OF_LINE",		input_beginning_of_line },
	{ "CLEAR_SCREEN",		irc_clear_screen },
	{ "COMMAND_COMPLETION",		command_completion },
	{ "DELETE_CHARACTER",		input_delete_character },
	{ "DELETE_NEXT_WORD",		input_delete_next_word },
	{ "DELETE_PREVIOUS_WORD",	input_delete_previous_word },
	{ "END_OF_LINE",		input_end_of_line },
	{ "ENTER_DIGRAPH",		enter_digraph },
	{ "ENTER_MENU",			enter_menu },
	{ "ERASE_LINE",			input_clear_line },
	{ "ERASE_TO_BEG_OF_LINE",	input_clear_to_bol },
	{ "ERASE_TO_END_OF_LINE",	input_clear_to_eol },
	{ "FORWARD_CHARACTER",		forward_character },
	{ "FORWARD_HISTORY",		forward_history },
	{ "FORWARD_WORD",		input_forward_word },
	{ "META1_CHARACTER",		meta1_char },
	{ "META2_CHARACTER",		meta2_char },
	{ "META3_CHARACTER",		meta3_char },
	{ "META4_CHARACTER",		meta4_char },
	{ "META5_CHARACTER",		meta5_char },
	{ "META6_CHARACTER",		meta6_char },
	{ "META7_CHARACTER",		meta7_char },
	{ "META8_CHARACTER",		meta8_char },
	{ "NEXT_WINDOW",		next_window },
	{ "NOTHING",			NULL },
	{ "PARSE_COMMAND",		parse_text },
	{ "PREVIOUS_WINDOW",		previous_window },
	{ "QUIT_IRC",			irc_quit },
	{ "QUOTE_CHARACTER",		quote_char },
	{ "REFRESH_INPUTLINE",		refresh_inputline },
	{ "REFRESH_SCREEN",		refresh_screen },
	{ "SCROLL_BACKWARD",		scrollback_backwards },
	{ "SCROLL_END",			scrollback_end },
	{ "SCROLL_FORWARD",		scrollback_forwards },
	{ "SCROLL_START",		scrollback_start },
	{ "SELF_INSERT",		input_add_character },
	{ "SEND_LINE",			send_line },
	{ "STOP_IRC",			term_pause },
	{ "SWAP_LAST_WINDOW",		swap_last_window },
	{ "SWAP_NEXT_WINDOW",		swap_next_window },
	{ "SWAP_PREVIOUS_WINDOW",	swap_previous_window },
	{ "SWITCH_CHANNELS",		switch_channels },
	{ "TOGGLE_INSERT_MODE",		toggle_insert_mode },
	{ "TOGGLE_STOP_SCREEN",		toggle_stop_screen },
	{ "TRANSPOSE_CHARACTERS",	input_transpose_characters },
	{ "TYPE_TEXT",			type_text },
	{ "UNSTOP_ALL_WINDOWS",		unstop_all_windows },
/**************************** PATCHED by Flier ******************************/
	/*{ "YANK_FROM_CUTBUFFER",	input_yank_cut_buffer }*/
	{ "YANK_FROM_CUTBUFFER",	input_yank_cut_buffer },
	{ "INSERT_AUTOREPLY",	        InsertAutoReply },
        { "NICK_COMPLETION",            InsertNick },
        { "LASTJOINER_KICK",            LastJoinerKick },
        { "ACCEPT_LAST_CHAT",           AcceptLastChat },
	{ "INSERT_TABKEY_NEXT",	        HandleTabNext },
	{ "INSERT_TABKEY_PREV",	        HandleTabPrev }
/****************************************************************************/
};

KeyMap	FAR keys[] =
{
	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 0 */
	{ BEGINNING_OF_LINE,	0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ BACKWARD_CHARACTER,	0, 0,	(char *) 0 },*/
	{ SELF_INSERT,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ QUIT_IRC,		0, 0,	(char *) 0 },
	{ DELETE_CHARACTER,	0, 0,	(char *) 0 },
	{ END_OF_LINE,		0, 0,	(char *) 0 },
	{ FORWARD_CHARACTER,	0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ BACKSPACE,		0, 0,	(char *) 0 },	/* 8 */
/**************************** PATCHED by Flier ******************************/
	/*{ TOGGLE_INSERT_MODE,	0, 0,	(char *) 0 },*/
	{ INSERT_TABKEY_NEXT,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ SEND_LINE,		0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ ERASE_TO_END_OF_LINE,	0, 0,	(char *) 0 },*/
	{ LASTJOINER_KICK,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ REFRESH_SCREEN,	0, 0,	(char *) 0 },
	{ SEND_LINE,		0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ FORWARD_HISTORY,	0, 0,	(char *) 0 },*/
	{ ACCEPT_LAST_CHAT,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ BACKWARD_HISTORY,	0, 0,	(char *) 0 },	/* 16 */
	{ QUOTE_CHARACTER,	0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ ENTER_MENU,		0, 0,	(char *) 0 },*/
	{ INSERT_AUTOREPLY,    	0, 0,	(char *) 0 },
/****************************************************************************/
	{ TOGGLE_STOP_SCREEN,	0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ TRANSPOSE_CHARACTERS,	0, 0,	(char *) 0 },*/
	{ NICK_COMPLETION,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ ERASE_LINE,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ SELF_INSERT,		0, 0,	(char *) 0 },*/
	{ META3_CHARACTER,     	0, 0,	(char *) 0 },
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
	/*{ META2_CHARACTER,	0, 0,	(char *) 0 },*/	/* 24 */
	{ SWITCH_CHANNELS,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ YANK_FROM_CUTBUFFER,	0, 0,	(char *) 0 },
			/* And I moved STOP_IRC to META1 26 */
/**************************** PATCHED by Flier ******************************/
 	/*{ ENTER_DIGRAPH,	0, 0,	(char *) 0 },*/
#ifndef SZ32
        { STOP_IRC, 		0, 0,	(char *) 0 },
#else
        { NOTHING, 		0, 0,	(char *) 0 },
#endif
/****************************************************************************/
	{ META1_CHARACTER,	0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 32 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 40 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 48 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 56 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 64 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 72 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 80 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 88 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 96 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 104 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
 
	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 112 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 120 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ BACKSPACE,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 128 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 136 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 144 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 152 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 160 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 168 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 176 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 184 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/*	{ SCROLL_START,		0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/*	{ SCROLL_END,		0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 192 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
 
	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 200 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 208 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 216 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 224 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/*	{ BACKWARD_WORD,	0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/*	{ DELETE_NEXT_WORD,	0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/*	{ SCROLL_END,		0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
/*	{ FORWARD_WORD,		0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },

/*	{ DELETE_PREVIOUS_WORD,	0, 0,	(char *) 0 }, */
	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 232 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 240 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },

	{ SELF_INSERT,		0, 0,	(char *) 0 },	/* 248 */
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 },
	{ SELF_INSERT,		0, 0,	(char *) 0 }
/*	{ DELETE_PREVIOUS_WORD,	0, 0,	(char *) 0 } */
};

KeyMap	FAR meta1_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	
	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ COMMAND_COMPLETION,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ CLEAR_SCREEN,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ SCROLL_START,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ SCROLL_END,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
        /*{ META3_CHARACTER,	0, 0,	(char *) 0 },*/
	{ META2_CHARACTER,	0, 0,	(char *) 0 },
/****************************************************************************/
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ BACKWARD_WORD,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ DELETE_NEXT_WORD,	0, 0,	(char *) 0 },
	{ SCROLL_END,		0, 0,	(char *) 0 },
	{ FORWARD_WORD,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ DELETE_PREVIOUS_WORD,	0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ SCROLL_FORWARD,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ SCROLL_BACKWARD,	0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ DELETE_PREVIOUS_WORD,	0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

KeyMap	FAR meta2_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
#ifdef ALLOW_STOP_IRC
	{ STOP_IRC,		0, 0,	(char *) 0 },
#else
	{ NOTHING,		0, 0,	(char *) 0 },
#endif
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
/**************************** PATCHED by Flier ******************************/
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ META5_CHARACTER,     	0, 0,	(char *) 0 },
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ META5_CHARACTER,     	0, 0,	(char *) 0 },
/****************************************************************************/
	{ NOTHING,		0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ SCROLL_END,		0, 0,	(char *) 0 },
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ SCROLL_BACKWARD,     	0, 0,	(char *) 0 },
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ SCROLL_FORWARD,      	0, 0,	(char *) 0 },
/****************************************************************************/
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
/**************************** PATCHED by Flier ******************************/
        /*{ NOTHING,		0, 0,	(char *) 0 },*/
        { BACKWARD_HISTORY,    	0, 0,	(char *) 0 },
        /*{ NOTHING,		0, 0,	(char *) 0 },*/
        { FORWARD_HISTORY,     	0, 0,	(char *) 0 },
        /*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ FORWARD_CHARACTER,   	0, 0,	(char *) 0 },
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ BACKWARD_CHARACTER,  	0, 0,	(char *) 0 },
/****************************************************************************/
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
/**************************** PATCHED by Flier ******************************/
	/*{ NOTHING,		0, 0,	(char *) 0 },*/
	{ META5_CHARACTER,     	0, 0,	(char *) 0 },
/****************************************************************************/
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NEXT_WINDOW,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ PREVIOUS_WINDOW,	0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

KeyMap	FAR meta3_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

KeyMap	FAR meta4_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ BACKWARD_CHARACTER,	0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ FORWARD_CHARACTER,	0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ META4_CHARACTER,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ BACKWARD_CHARACTER,	0, 0,	(char *) 0 },	/* 72 */
	{ META4_CHARACTER,	0, 0,	(char *) 0 },
	{ FORWARD_HISTORY,	0, 0,	(char *) 0 },
	{ BACKWARD_HISTORY,	0, 0,	(char *) 0 },
	{ FORWARD_CHARACTER,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ DELETE_CHARACTER,	0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ META4_CHARACTER,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ BACKWARD_CHARACTER,	0, 0,	(char *) 0 },	/* 104 */
	{ META4_CHARACTER,	0, 0,	(char *) 0 },
	{ FORWARD_HISTORY,	0, 0,	(char *) 0 },
	{ BACKWARD_HISTORY,	0, 0,	(char *) 0 },
	{ FORWARD_CHARACTER,	0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ DELETE_CHARACTER,	0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

/**************************** PATCHED by Flier ******************************/
KeyMap	FAR meta5_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,	        0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

KeyMap	FAR meta6_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,	        0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

KeyMap	FAR meta7_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,	        0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};

KeyMap	FAR meta8_keys[] =
{
	{ NOTHING,		0, 0,	(char *) 0 },	/* 0 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 8 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 16 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 24 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 32 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 40 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 48 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 56 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 64 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 72 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 80 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 88 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 96 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 104 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,	        0, 0,	(char *) 0 },	/* 112 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 120 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 128 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 136 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 144 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 152 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 160 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 168 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 176 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 184 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 192 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 200 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 208 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 216 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 224 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 232 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 240 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },

	{ NOTHING,		0, 0,	(char *) 0 },	/* 248 */
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 },
	{ NOTHING,		0, 0,	(char *) 0 }
};
/****************************************************************************/

/*
 * write_binding: This will write to the given FILE pointer the information
 * about the specified key binding.  The format it writes it out is such that
 * it can be parsed back in later using LOAD or with the -l switch 
 */
static	void
write_binding(c, meta, fp, do_all)
 	u_int	c,
		meta;
	FILE	*fp;
	int	do_all;
{
	KeyMap	*map;
	char	*meta_str;

	if (c == 32)
		return;
	switch (meta)
	{
	case 1:
		map = meta1_keys;
		meta_str = "META1-";
		break;
	case 2:
		map = meta2_keys;
		meta_str = "META2-";
		break;
	case 3:
		map = meta3_keys;
		meta_str = "META3-";
		break;
	case 4:
		map = meta4_keys;
		meta_str = "META4-";
		break;
	case 5:
		map = meta5_keys;
		meta_str = "META5-";
		break;
	case 6:
		map = meta6_keys;
		meta_str = "META6-";
		break;
	case 7:
		map = meta7_keys;
		meta_str = "META7-";
		break;
	case 8:
		map = meta8_keys;
		meta_str = "META8-";
		break;
	default:
		map = keys;
		meta_str = empty_string;
		break;
	}
	if (map[c].changed)
	{
		fprintf(fp, "BIND %s%s %s", meta_str, display_key(c),
			key_names[map[c].index].name);
		if (map[c].stuff && (*(map[c].stuff)))
		{
			fprintf(fp, " %s\n", map[c].stuff);
		}
		else
			fprintf(fp, "\n");
	}
}

/*
 * save_bindings: this writes all the keys bindings for IRCII to the given
 * FILE pointer using the write_binding function 
 */
void
save_bindings(fp, do_all)
	FILE	*fp;
	int	do_all;
{
	int	i;
	int	charsize = charset_size();

	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 0, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 1, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 2, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 3, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 4, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 5, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 6, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 7, fp, do_all);
	for (i = 0; i < charsize; i++)
		write_binding((u_char) i, 8, fp, do_all);
}
