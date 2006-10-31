/*
 * input.c: does the actual input line stuff... keeps the appropriate stuff
 * on the input line, handles insert/delete of characters/words... the whole
 * ball o wax 
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
 * $Id: input.c,v 1.24 2006-10-31 12:31:27 f Exp $
 */

#include "irc.h"

#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif /* HAVE ICONV_H */

#include "input.h"
#include "ircterm.h"
#include "alias.h"
#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "screen.h"
#include "exec.h"
#include "output.h"
#include "translat.h"

#include "debug.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

#ifdef WANTANSI
extern int CountAnsiInput _((char *, int));
#endif
extern void StripAnsi _((char *, char *, int));
extern NickList *tabnickcompl;
/****************************************************************************/

#define WIDTH 10

/* input_prompt: contains the current, unexpanded input prompt */
static	char	*input_prompt = (char *) 0;

static struct mb_data mbdata;
static int mbdata_ok = 0;

/**************************** PATCHED by Flier ******************************/
static void ResetNickCompletion() {
    tabnickcompl = NULL;
}
/****************************************************************************/

/* cursor_to_input: move the cursor to the input line, if not there already */
void
cursor_to_input(void)
{
	Screen *old_current_screen, *screen;

	old_current_screen = current_screen;
	for (screen = screen_list; screen; screen = screen->next)
	{
		set_current_screen(screen);
		if (screen->alive && is_cursor_in_display())
		{
			term_move_cursor(screen->inputdata.cursor_x, screen->inputdata.cursor_y);
			Debug((3, "cursor_to_input: moving cursor to input for screen %d", screen->screennum));
			Debug((3, "- x %u; y %u", screen->inputdata.cursor_x, screen->inputdata.cursor_y));
			cursor_not_in_display();
			term_flush();
		}
	}
	set_current_screen(old_current_screen);
}

static unsigned
input_do_calculate_width(unsigned unival, iconv_t conv)
{
	if (!displayable_unival(unival, conv))
	{
		/* undisplayable values are printed raw */
		if (unival < 0x80)
			return 1;
		if (unival < 0x800)
			return 2;
		if (unival < 0x10000)
			return 3;
		return 4;
	}
	return calc_unival_width(unival);
}

static void
input_do_set_cursor_pos(unsigned pos)
{
	cursor_to_input();
	current_screen->inputdata.buffer.pos = pos;
	update_input(UPDATE_JUST_CURSOR);
}

static void
input_do_insert_raw(char* source)
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	unsigned max = sizeof(current_screen->inputdata.buffer.buf);
	unsigned inslen = strlen(source);
	
	/* This function inserts the given substring of bytes
	 * to the input line at the current editing position.
	 * Cursor is moved to point to the end of the substring.
	 */
	
	if (pos + inslen > max)
	{
		inslen = max-pos;
	}
	
	/* Move the tail out of way */
	memmove(buf + pos + inslen,
	        buf + pos,
	        max - pos - inslen);
	/* Then put the substring in */
	memcpy(buf + pos,
	       source,
	       inslen);
	/* Ensure the buffer is terminated */
	buf[max-1] = '\0';

	pos += inslen;
	if (pos > max)
		pos = max;
	
	/* Update the screen from the old cursor position */
	cursor_to_input();
	update_input(UPDATE_FROM_CURSOR);
	/* Then place the cursor correctly */
	input_do_set_cursor_pos(pos);
}

static void
input_do_delete_raw(int n, int do_save_cut)
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	unsigned max = sizeof(current_screen->inputdata.buffer.buf);

	cursor_to_input();
/*
	fprintf(stderr, "pos(%u)max(%u)buf(%s)do_delete_raw(%d, %d)\n",
	    pos,max,buf, n, do_save_cut);
*/
	/* If n>0, deletes from front
	 * if n<0, deletes from back & moves cursor
	 */
	if (n < 0)
	{
		unsigned limit = current_screen->inputdata.buffer.minpos;
		/* Number of bytes LEFT from the cursor (prompt excluding) */
		unsigned oldbytes = pos-limit;
		unsigned erasebytes = -n;

		/* Don't delete more than we can */
		if (erasebytes > oldbytes)
			erasebytes = oldbytes;

		/* Move cursor backward */
		pos -= erasebytes;
		input_do_set_cursor_pos(pos);

		/* Then delete from forward */
		n = erasebytes;
	}
	if (n > 0)
	{
		unsigned oldbytes = max-pos;
		unsigned erasebytes = n > oldbytes ? oldbytes : n;
		unsigned newbytes = oldbytes - erasebytes;
		
		if (do_save_cut)
		{
			if (cut_buffer) new_free(&cut_buffer);
			cut_buffer = new_malloc(erasebytes+1);
			memcpy(cut_buffer, buf+pos, erasebytes);
			cut_buffer[erasebytes] = '\0';
		}
		
		memmove(buf+pos,
		        buf+pos+erasebytes,
		        newbytes);
		buf[pos+newbytes] = '\0';
	}
/*
	fprintf(stderr, "-> pos(%u)max(%u)buf(%s)\n",
	    pos,max,buf);
*/
	/* Now update the right side from cursor */
	update_input(UPDATE_FROM_CURSOR);
}

static void
input_do_delete_chars(int n, int do_save_cut)
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;

	/* The usage of this function is identical to input_do_delete_raw(),
	 * but instead of bytes, n means the number of characters.
	 * Since characters may consist of 1-4 bytes, this
	 * function converts the number to bytes and then
	 * calls input_do_delete_raw().
	 */
	if (n > 0)
	{
		int bytes;
		for (bytes = 0; buf[pos] != '\0' && n > 0; --n)
		{
			unsigned length = calc_unival_length(buf+pos);
			bytes += length;
			pos   += length;
		}
		input_do_delete_raw(bytes, do_save_cut);
	}
	else if (n < 0)
	{
		unsigned limit = current_screen->inputdata.buffer.minpos;
		
		int bytes;
		for(bytes=0; n<0 && pos > limit; ++n)
		{
			/* Go back until we reach a beginning of a character */
			for(;;)
			{
				unsigned length = calc_unival_length(buf + --pos);
				if (length) { bytes += length; break; }
				/* But don't go more than we're allowed to */
				if (pos == limit) break;
			}
		}
		input_do_delete_raw(-bytes, do_save_cut);
	}
}

static void
input_do_replace_prompt(u_char *newprompt)
{
	ScreenInputBufferData* bufdata = &current_screen->inputdata.buffer;
	char* buf  = bufdata->buf;
	unsigned oldlen = bufdata->minpos;
	unsigned newlen = strlen(newprompt);
	unsigned max = sizeof(bufdata->buf);
	unsigned saved_len = max - (oldlen > newlen ? oldlen : newlen);
/*
	fprintf(stderr, "oldlen=%u, newlen=%u, max=%u, saved_len=%u\n",
		oldlen,newlen, max, saved_len);
*/
	memmove(buf+newlen,
	        buf+oldlen,
	        saved_len);
	memcpy(buf,
	       newprompt,
	       newlen);
	buf[max-1] = '\0'; /* prevent dragons */
	
	bufdata->minpos = newlen;
	bufdata->pos	= bufdata->pos - oldlen + newlen;
	
	if (bufdata->pos < newlen)
		bufdata->pos = newlen;	
}

static int
input_is_password_prompt(void)
{
	char* buf    = current_screen->inputdata.buffer.buf;
	unsigned limit = current_screen->inputdata.buffer.minpos;
	
	if (limit < 9)
		return 0;
	
	/* If the prompt ends with "Password:", it is a password prompt. */
	/* This includes the following known prompts:
	 *   "Password:"
	 *   "Operator Password:"
	 *   "Server Password:"
         * PATCHED by Flier
	 *   "Master Password:"
	 *   "Old Master Password:"
	 */
	return my_strnicmp(buf+limit-9, "Password:", 9) == 0;
}

static char
input_do_check_prompt(int update)
{
	char	*prompt;
	
	char changed = 0;

	if (current_screen->promptlist)
		prompt = current_screen->promptlist->prompt;
	else
		prompt = input_prompt ? input_prompt : empty_string;
	if (prompt)
	{
		if (update != NO_UPDATE)
		{
			int free_it = 1;
			unsigned len;
			char	*ptr;
			int	args_used;	/* this isn't used here but is
						 * passed to expand_alias()
						 */
			if (is_process(get_target_by_refnum(0)))
			{
				ptr = get_prompt_by_refnum(0);
				free_it = 0;
			}
			else
				ptr = expand_alias((u_char *) 0, prompt, empty_string, &args_used, NULL);

			len = strlen(ptr);
			if (strncmp(ptr, current_screen->inputdata.buffer.buf, len) || !len)
			{
				input_do_replace_prompt(ptr);
				changed = 1;
			}
			if (free_it)
				new_free(&ptr);
		}
	}
	return changed;
}

static char
input_check_resized(void)
{
	ScreenInputData* inputdata = &current_screen->inputdata;

	if (inputdata->old_li == current_screen->li
	    && inputdata->old_co == current_screen->co)
		return 0;

	/* resized?  Keep it simple and reset everything */
	inputdata->cursor_x = 0;
	inputdata->cursor_y = current_screen->li - 1;
	inputdata->old_li   = current_screen->li;
	inputdata->old_co   = current_screen->co;
	
	inputdata->zone     = current_screen->co;
	if (inputdata->zone > WIDTH)
		inputdata->zone -= WIDTH;
	return 1;
}

/*
 * update_input: does varying amount of updating on the input line depending
 * upon the position of the cursor and the update flag.  If the cursor has
 * move toward one of the edge boundaries on the screen, update_cursor()
 * flips the input line to the next (previous) line of text. The update flag
 * may be: 
 *
 * NO_UPDATE - only do the above bounds checking. 
 *
 * UPDATE_JUST_CURSOR - do bounds checking and position cursor where is should
 * be. 
 *
 * UPDATE_FROM_CURSOR - does all of the above, and makes sure everything from
 * the cursor to the right edge of the screen is current (by redrawing it). 
 *
 * UPDATE_ALL - redraws the entire line 
 */
void
update_input(update)
	int	update;
{
	ScreenInputData* inputdata = &current_screen->inputdata;
	iconv_t display_conv = NULL;
	
	int term_echo = 1;

	if (dumb)
		return;

	cursor_to_input();

	if (input_do_check_prompt(update))
		update = UPDATE_ALL;
	
	term_echo = !input_is_password_prompt();

	if (input_check_resized())
		update = UPDATE_ALL;
	
	if (update != NO_UPDATE)
	{
#ifdef HAVE_ICONV_OPEN
		const char *enc = display_encoding;
		if (!enc)
			enc = "ISO-8859-1";
		display_conv = iconv_open(enc, "UTF-8");
#endif /* HAVE_ICONV_OPEN */
	}
	
/*
	{
		unsigned a;
		u_char* buf  = inputdata->buffer.buf;
		fputc('"', stderr);
		for (a = 0; buf[a]; ++a)
			fputc(buf[a], stderr);
		fprintf(stderr, "\"\n");
		fputc('>', stderr);
		for (a = 0; buf[a]; ++a)
			if (a == inputdata->buffer.pos)
				fputc('^', stderr);
			else if (a == inputdata->buffer.minpos)
				fputc('|', stderr);
			else
				fputc(' ', stderr);
		fprintf(stderr, "\n");
	}
*/
	if (update == UPDATE_JUST_CURSOR || update == UPDATE_ALL)
	{
		char* buf  = inputdata->buffer.buf;
		unsigned pos = inputdata->buffer.pos;
		unsigned limit = inputdata->buffer.minpos;
		unsigned column, ptr;
		
		unsigned window;
		
		/* Recalculate pos_column */
		for (column = ptr = 0; ptr < pos; )
		{
			unsigned unival = calc_unival(buf+ptr);

			if (!term_echo && ptr >= limit)
				unival = ' ';

			column += input_do_calculate_width(unival, display_conv);
			ptr    += calc_unival_length(buf+ptr);
		}
		inputdata->pos_column = column;

		window = column - (column % inputdata->zone);
		
		/* Recalculate left_ptr */
		for (column = ptr = 0; column < window; )
		{
			unsigned unival = calc_unival(buf+ptr);
			
			if (!term_echo && ptr >= limit)
				unival = ' ';
			
			column += input_do_calculate_width(unival, display_conv);
			ptr    += calc_unival_length(buf+ptr);
		}
		/* If the left edge has been moved, redraw the input line */
		if (ptr != inputdata->left_ptr)
			update = UPDATE_ALL;
		inputdata->left_ptr = ptr;
		
		/* Recalculate cursor_x */
		inputdata->cursor_x = inputdata->pos_column - column;
	}
	
	if (update == UPDATE_FROM_CURSOR || update == UPDATE_ALL)
	{
		unsigned limit = inputdata->buffer.minpos;
		char *buf = inputdata->buffer.buf;
		char VisBuf[BIG_BUFFER_SIZE];
		unsigned column, iptr, optr;
		int written;
		
		for (column = 0, optr = 0, iptr = inputdata->left_ptr;
			buf[iptr] != '\0' && column < current_screen->co; )
		{
			unsigned unival = calc_unival(buf+iptr);
			unsigned len = calc_unival_length(buf+iptr);
			
			if (!term_echo && iptr >= limit)
				unival = ' ';
			
			if (displayable_unival(unival, display_conv))
			{
				column += calc_unival_width(unival);
				if (!term_echo && unival == ' ')
				{
					memcpy(VisBuf+optr, " ", 1);
					optr += 1;
				}
				else
				{
					memcpy(VisBuf+optr, buf+iptr, len);
					optr += len;
				}
				iptr += len;
			}
			else
			{
				unsigned n;
				VisBuf[optr++] = REV_TOG;
				for (n = 0; n < len; ++n)
					VisBuf[optr++] = (buf[iptr++] & 127) | 64;
				VisBuf[optr++] = REV_TOG;
				column += len;
			}
		}
		VisBuf[optr] = '\0';
		term_move_cursor(0, inputdata->cursor_y);
		written = output_line(VisBuf, 0);
		if (term_clear_to_eol() && written < current_screen->co)
		{
			/* EOL wasn't implemented, so do it with spaces */
			term_space_erase(current_screen->co - written);
		}
	}
	
	if (update != NO_UPDATE)
	{
		/* Always update cursor */
		term_move_cursor(inputdata->cursor_x, inputdata->cursor_y);
	}
	
	if (update != NO_UPDATE)
	{
#ifdef HAVE_ICONV_OPEN
		iconv_close(display_conv);
#endif /* HAVE_ICONV_OPEN */
	}

	term_flush();
}

void
refresh_inputline(key, ptr)
 	u_int	key;
	char *	ptr;
{
	update_input(UPDATE_ALL);
}

void
change_input_prompt(direction)
	int	direction;
{
	if (!current_screen->promptlist)
	{
		/* Restore stuff */
		memcpy(&current_screen->inputdata.saved_buffer,
		       &current_screen->inputdata.buffer,
		       sizeof(current_screen->inputdata.buffer));
	}
	else if (direction == -1)
	{
		/* Just update whatever should be on screen */
	}
	else if (!current_screen->promptlist->next)
	{
		/* Save stuff */
		memcpy(&current_screen->inputdata.buffer,
		       &current_screen->inputdata.saved_buffer,
		       sizeof(current_screen->inputdata.buffer));
		
		/* Erase input line */
		*current_screen->inputdata.buffer.buf = '\0';
		current_screen->inputdata.buffer.pos =
				current_screen->inputdata.buffer.minpos = 0;
	}
	update_input(UPDATE_ALL);
}

/* input_move_cursor: moves the cursor left or right... got it? */
/* zero=left, nonzero=right */
void
input_move_cursor(dir)
	int	dir;
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	if (dir)
	{
		/* if there are still chars remaining */
		if (buf[pos] != '\0')
		{
			/* Skip over 1 character */
			pos += calc_unival_length(buf+pos);
			
			input_do_set_cursor_pos(pos);
		}
	}
	else
	{
		unsigned limit = current_screen->inputdata.buffer.minpos;

		if (pos > limit)
		{
			/* Go back until we reach a beginning of a character */
			while(!calc_unival_length(buf + --pos))
			{
				/* But don't go more than we're allowed to */
				if (pos == limit) break;
			}
			input_do_set_cursor_pos(pos);
		}
	}
}

/*
 * input_forward_word: move the input cursor forward one word in the input
 * line 
 */
void
input_forward_word(key, ptr)
 	u_int	key;
	char *	ptr;
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	
	/* Skip while definitely space, then skip while definitely nonspace */ 
	/* For nontypical characters, does nothing */
	
	while (buf[pos] != '\0')
	{
		unsigned unival = calc_unival(buf+pos);

		if (!isspace(unival) && !ispunct(unival))
			break;
		pos += calc_unival_length(buf+pos);
	}
	while (buf[pos] != '\0')
	{
		unsigned unival = calc_unival(buf+pos);
		if (isspace(unival) || ispunct(unival) || unival >= 0x300)
			break;
		pos += calc_unival_length(buf+pos);
	}
	input_do_set_cursor_pos(pos);
}

/* input_backward_word: move the cursor left on word in the input line */
void
input_backward_word(key, ptr)
 	u_int	key;
	char *	ptr;
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	unsigned limit = current_screen->inputdata.buffer.minpos;
	
	/* Skip while previous is definitely space,         */
	/* then skip while previous is definitely nonspace. */
	/* For nontypical characters, does nothing */
	while (pos > limit)
	{
		unsigned prevpos = pos, unival;

		while (!calc_unival_length(buf + --prevpos))
			if (prevpos <= limit)
				break;
		unival = calc_unival(buf + prevpos);
		if (!isspace(unival) && !ispunct(unival))
			break;
		pos = prevpos;
	}
	while (pos > limit)
	{
		unsigned prevpos = pos, unival;
		while (!calc_unival_length(buf + --prevpos))
			if (prevpos <= limit)
				break;
		unival = calc_unival(buf + prevpos);
		if (isspace(unival) || ispunct(unival) || unival >= 0x300)
			break;
		pos = prevpos;
	}
	input_do_set_cursor_pos(pos);
}

/* input_delete_character: deletes a character from the input line */
void
input_delete_character(key, ptr)
 	u_int	key;
	char *	ptr;
{
	input_do_delete_chars(1, 0);
/**************************** PATCHED by Flier ******************************/
        ResetNickCompletion();
/****************************************************************************/
}

/* input_backspace: does a backspace in the input buffer */
/*ARGSUSED*/
void
input_backspace(key, ptr)
 	u_int	key;
	char *	ptr;
{
	input_do_delete_chars(-1, 0);
/**************************** PATCHED by Flier ******************************/
        ResetNickCompletion();
/****************************************************************************/
}

/*
 * input_beginning_of_line: moves the input cursor to the first character in
 * the input buffer 
 */
void
input_beginning_of_line(key, ptr)
 	u_int	key;
	char	*ptr;
{
	input_do_set_cursor_pos(current_screen->inputdata.buffer.minpos);
}

/*
 * input_end_of_line: moves the input cursor to the last character in the
 * input buffer 
 */
void
input_end_of_line(key, ptr)
 	u_int	key;
	char	*ptr;
{
	input_do_set_cursor_pos(strlen(current_screen->inputdata.buffer.buf));
}

/*
 * input_delete_previous_word: deletes from the cursor backwards to the next
 * space character. 
 */
void
input_delete_previous_word(key, ptr)
 	u_int	key;
	char *	ptr;
{
	unsigned pos = current_screen->inputdata.buffer.pos;
	int bytes;

	/* moves cursor backwards */
	input_backward_word(key,ptr);
	
	bytes = pos - current_screen->inputdata.buffer.pos;
	
	/* now that we're back, delete next n bytes */
	input_do_delete_raw(bytes, 1);
/**************************** PATCHED by Flier ******************************/
        tabnickcompl = NULL;
/****************************************************************************/
}

/*
 * input_delete_next_word: deletes from the cursor to the end of the next
 * word 
 */
void
input_delete_next_word(key, ptr)
 	u_int	key;
	char *	ptr;
{
	unsigned pos = current_screen->inputdata.buffer.pos;
	int bytes;

	/* moves cursor forward */
	input_forward_word(key,ptr);
	
	bytes = current_screen->inputdata.buffer.pos - pos;
	
	/* now that we're after the word, delete past n bytes */
	input_do_delete_raw(-bytes, 1);
}

/*
 * input_add_character: adds the byte c to the input buffer, repecting
 * the current overwrite/insert mode status, etc 
 */
/*ARGSUSED*/
void
input_add_character(key, ptr)
 	u_int	key;
	char *	ptr;
{
	u_char output_buffer[8];
	
#ifdef HAVE_ICONV_OPEN
	static u_char   input_buffer[32]="";
	static unsigned input_pos=0;
	size_t retval;
	
	iconv_const char *iptr;
	char *optr;
	size_t isize, osize;
	
	input_buffer[input_pos++] = key;

	if (!mbdata_ok)
	{
		mbdata_init(&mbdata, input_encoding);
		mbdata_ok = 1;
	}

re_encode:
	if (!input_pos)
	{
		return;
	}
	iptr = (iconv_const char *)input_buffer;
	isize = input_pos;
	optr = (char *)output_buffer;
	osize = sizeof output_buffer;
	
	retval = iconv(mbdata.conv_in,
		       &iptr, &isize,
		       (char **)&optr, &osize);
	
	if (retval == (size_t)-1)
	{
		switch(errno)
		{
			case EINVAL:
			{
				/* User didn't give enough bytes. Must give more later. */
				return;
			}
			case EILSEQ:
			{
				/* User gave a bad byte. Ignore bad bytes! */
				memmove(input_buffer+1,
				        input_buffer,
				        --input_pos);
				goto re_encode;
			}
		}
	}
	*optr = '\0';
	input_pos = 0;
#else
	/* no iconv, assume in=iso-8859-1 */
	utf8_sequence(key, output_buffer);
#endif /* HAVE_ICONV_OPEN */

	if (!get_int_var(INSERT_MODE_VAR))
	{
		/* Delete the next character */
		input_do_delete_chars(1, 0);
	}
	input_do_insert_raw(output_buffer);
/**************************** PATCHED by Flier ******************************/
        if (key == ' ') ResetNickCompletion();
/****************************************************************************/
}

/*
 * set_input: sets the input buffer to the given string, discarding whatever
 * was in the input buffer before 
 */
void
set_input(str)
	char	*str;
{
	u_char converted_input[INPUT_BUFFER_SIZE];
	struct mb_data mbdata1;
	unsigned dest;
#ifdef HAVE_ICONV_OPEN
	mbdata_init(&mbdata1, irc_encoding);
#else
	mbdata_init(&mbdata1, NULL);
#endif /* HAVE_ICONV_OPEN */
	for (dest = 0; *str != '\0'; )
	{
		if (dest + 8 >= sizeof(converted_input))
			break;

		decode_mb(str, converted_input+dest, &mbdata1);
		dest += mbdata1.output_bytes;
		str  += mbdata1.input_bytes;
	}
	converted_input[dest] = '\0';

	set_input_raw(converted_input);

/**************************** PATCHED by Flier ******************************/
        /* fix memory leak */
        mbdata_done(&mbdata1);
/****************************************************************************/
}

void
set_input_raw(str)
	char* str;
{
	char* buf    = current_screen->inputdata.buffer.buf;
	unsigned pos   = current_screen->inputdata.buffer.pos;
	unsigned limit = current_screen->inputdata.buffer.minpos;

	strmcpy(buf + limit,
		str,
		(size_t)sizeof(current_screen->inputdata.buffer.buf) - limit);
	
	pos = strlen(buf);
	
	current_screen->inputdata.buffer.pos = pos;
		
	if (mbdata_ok)
	{
		mbdata_done(&mbdata);
		mbdata_ok = 0;
	}
}

/*
 * get_input: returns a pointer to the input buffer.  Changing this will
 * actually change the input buffer.  This is a bad way to change the input
 * buffer tho, cause no bounds checking won't be done 
 */
char	*
get_input()
{
	iconv_const char* source = (iconv_const char*)get_input_raw();
	
	/* The input buffer is UTF-8 encoded. Clients will want irc_encoding instead. */
	static u_char converted_buffer[INPUT_BUFFER_SIZE];

#ifdef HAVE_ICONV_OPEN
	iconv_t conv = iconv_open(irc_encoding, "UTF-8");
	char* dest = (char *)converted_buffer;
	size_t left, space;

	left = strlen(source);
	space  = sizeof(converted_buffer);
	while (*source != '\0')
	{
		size_t retval;

		retval = iconv(conv,
		                &source, &left,
		                &dest, &space);
		if (retval == (size_t)-1)
		{
			if (errno == E2BIG)
				break;
			if (errno == EILSEQ)
			{
				/* Ignore silently 1 illegal byte */
				if (left > 0)
				{
					++source;
					--left;
				}
			}
			if (errno == EINVAL)
			{
				/* Input terminated with a partial byte. */
				/* Ignore the error silently. */
				break;
			}
		}
	}
	/* Reset the converter, create a reset-sequence */
	iconv(conv, NULL, &left, &dest, &space);
	
	/* Ensure null-terminators are where they should be */
	converted_buffer[sizeof(converted_buffer)-1] = '\0';
	*dest = '\0';
	
	iconv_close(conv);
#else
	/* Must convert manually - assume output is ISO-8859-1 */
	unsigned dest = 0;

	while (*source != '\0')
	{
		unsigned len = calc_unival_length(source);
		unsigned unival;

		if (!len)
		{
			/* ignore illegal byte (shouldn't happen) */
			++source;
			continue;
		}
		unival = calc_unival(source);
		if (displayable_unival(unival, NULL))
		{
			converted_buffer[dest++] = unival;
			if (dest+1 >= sizeof(converted_buffer))
				break;
		}
		source += len;
	}
	converted_buffer[dest] = '\0';
#endif /* HAVE_ICONV_OPEN */
	
	return converted_buffer;
}

char	*
get_input_raw()
{
	char* buf    = current_screen->inputdata.buffer.buf;
	unsigned limit = current_screen->inputdata.buffer.minpos;
	
	return buf+limit;
}

/* input_clear_to_eol: erases from the cursor to the end of the input buffer */
void
input_clear_to_eol(key, ptr)
 	u_int	key;
	char *	ptr;
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	int bytes;
	
	for (bytes = 0; buf[pos] != '\0';)
	{
		unsigned length = calc_unival_length(buf+pos);
		bytes += length;
		pos   += length;
	}
	
	input_do_delete_raw(bytes, 1);
}

/*
 * input_clear_to_bol: clears from the cursor to the beginning of the input
 * buffer 
 */
void
input_clear_to_bol(key, ptr)
 	u_int	key;
	char	*ptr;
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	int bytes;
	
	unsigned limit = current_screen->inputdata.buffer.minpos;
	for (bytes = 0; limit < pos; )
	{
		unsigned length = calc_unival_length(buf+limit);
		bytes += length;
		limit += length;
	}
	
	input_do_delete_raw(-bytes, 1);
}

/*
 * input_clear_line: clears entire input line
 */
void
input_clear_line(key, ptr)
 	u_int	key;
	char	*ptr;
{
	input_beginning_of_line(key, ptr);
	input_clear_to_eol(key, ptr);
/**************************** PATCHED by Flier ******************************/
        tabnickcompl = NULL;
/****************************************************************************/
}

/*
 * input_transpose_characters: swaps the positions of the two characters
 * before the cursor position 
 * and moves cursor 1 forward
 */
void
input_transpose_characters(key, ptr)
 	u_int	key;
	char *	ptr;
{
	char* buf  = current_screen->inputdata.buffer.buf;
	unsigned pos = current_screen->inputdata.buffer.pos;
	
	/*
	  delete previous char,
	  then move 1 right,
	  then insert the deleted char
	  
	  example (^ denotes cursor position):
	  
		before:
		  defg
		    ^
		after:
		  dfeg
		     ^
	  
	  this is how bash transposes at least /Bisqwit
	  
	  FIXME: cut-buffer shouldn't be affected when transposing
	*/
	if (!buf[pos])
	{
		/* back off 1 char */
		input_move_cursor(0);
		pos = current_screen->inputdata.buffer.pos;
		/* If we're still at the end of the string,
		 * the string consists of only 1 char.
		 * In which case there's nothing to transpose.
		 */
		if (!buf[pos])
		{
			return;
		}
	}
	input_do_delete_chars(-1, 1);
	input_move_cursor(1);
	input_yank_cut_buffer(key, ptr);
}

/* init_input: initialized the input buffer by clearing it out */
void
init_input()
{
	*current_screen->inputdata.buffer.buf = (char) 0;
	current_screen->inputdata.buffer.pos = current_screen->inputdata.buffer.minpos;
}

/*
 * input_yank_cut_buffer: takes the contents of the cut buffer and inserts it
 * into the input line 
 */
void
input_yank_cut_buffer(key, ptr)
 	u_int	key;
	char *	ptr;
{
	if (cut_buffer)
		input_do_insert_raw(cut_buffer);
}

/* get_input_prompt: returns the current input_prompt */
char	*
get_input_prompt()
{ 
	return (input_prompt); 
}

/*
 * set_input_prompt: sets a prompt that will be displayed in the input
 * buffer.  This prompt cannot be backspaced over, etc.  It's a prompt.
 * Setting the prompt to null uses no prompt 
 */
void
set_input_prompt(prompt)
	char	*prompt;
{
	if (!prompt)
	{
		malloc_strcpy(&input_prompt, empty_string);
	}
	else
	{
		char converted_prompt[INPUT_BUFFER_SIZE];
		struct mb_data mbdata1;
		unsigned dest;
#ifdef HAVE_ICONV_OPEN
		mbdata_init(&mbdata1, irc_encoding);
#else
		mbdata_init(&mbdata1, NULL);
#endif /* HAVE_ICONV_OPEN */
		for (dest = 0; *prompt != '\0'; )
		{
			decode_mb(prompt, converted_prompt+dest, &mbdata1);
			dest   += mbdata1.output_bytes;
			prompt += mbdata1.input_bytes;
		}
		converted_prompt[dest] = '\0';

		if (input_prompt && !strcmp(converted_prompt, input_prompt))
			return;
		malloc_strcpy(&input_prompt, converted_prompt);
	}
	update_input(UPDATE_ALL);
}

/*
 * input_pause: prompt the user with a message then waits for a single
 * keystroke before continuing.  The key hit is returned. 
 *
 * This function is NEVER to be called, once the inital irc_io()
 * loop has been called from main().  Perhaps it would be better
 * to just put this code at the only place it is called, and not
 * have the function.  If you want an input_pause, use add_wait_prompt()
 * with WAIT_PROMPT_KEY.
 */
char
input_pause(msg)
	char	*msg;
{
	char	*ptr = (char *) 0;
	char	c;

	if (dumb)
	{
		puts(msg);
		fflush(stdout);
		if (use_input)
			irc_io(&c, NULL, -1, 1);
	}
	else
	{
		malloc_strcpy(&ptr, get_input());
		set_input(msg);
		update_input(UPDATE_ALL);
		irc_io(&c, NULL, -1, 1);
		set_input(ptr);
		update_input(UPDATE_ALL);
		new_free(&ptr);
	}
	return (c);
}

void
input_reset_screen(Screen* new)
{
	new->inputdata.old_li = -1;
	new->inputdata.old_co = -1;
	new->inputdata.cursor_x = 0;
	new->inputdata.cursor_y = 0;
	new->inputdata.left_ptr = 0;
	new->inputdata.pos_column = 0;
	new->inputdata.buffer.pos = 0;
	new->inputdata.buffer.minpos = 0;
	new->inputdata.buffer.buf[0] = '\0';
}

/* moved from alias.c */
u_char	*
function_curpos(input)
	u_char	*input;
{
	char	*new = (char *) 0,
		pos[8];

	snprintf(pos, sizeof pos, "%d", current_screen->inputdata.buffer.pos);
	malloc_strcpy(&new, pos);
	return new;
}
