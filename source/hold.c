/*
 * hold.c: handles buffering of display output.
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
 * $Id: hold.c,v 1.4 2003-01-08 20:00:54 f Exp $
 */

#include "irc.h"

#include "ircaux.h"
#include "window.h"
#include "screen.h"
#include "vars.h"
#include "input.h"

/* reset_hold: Make hold_mode behave like VM CHAT, hold only
 * when there is no user interaction, this should be called
 * whenever the user does something in a window.  -lynx
 */
void
reset_hold(win)
	Window	*win;
{
	if (!win)
		win = curr_scr_win;
	if (!win->scrolled_lines)
		win->line_cnt = 0;
}

/* add_to_hold_list: adds str to the hold list queue */
void
add_to_hold_list(window, str, logged)
	Window	*window;
	char	*str;
	int	logged;
{
	Hold	*new;
	unsigned int	max;

	new = (Hold *) new_malloc(sizeof(Hold));
	new->str = (char *) 0;
	malloc_strcpy(&(new->str), str);
	new->logged = logged;
	window->held_lines++;
	if ((max = get_int_var(HOLD_MODE_MAX_VAR)) != 0)
	{
		if (window->held_lines > max)
			hold_mode(window, OFF, 1);
	}
	new->next = window->hold_head;
	new->prev = (Hold *) 0;
	if (window->hold_tail == (Hold *) 0)
		window->hold_tail = new;
	if (window->hold_head)
		window->hold_head->prev = new;
	window->hold_head = new;
	update_all_status();
}

/* remove_from_hold_list: pops the next element off the hold list queue. */
void
remove_from_hold_list(window)
	Window	*window;
{
	Hold	*crap;

	if (window->hold_tail)
	{
		window->held_lines--;
		new_free(&window->hold_tail->str);
		crap = window->hold_tail;
		window->hold_tail = window->hold_tail->prev;
		if (window->hold_tail == (Hold *) 0)
			window->hold_head = window->hold_tail;
		else
			window->hold_tail->next = (Hold *) 0;
		new_free(&crap);
		update_all_status();
	}
}

/*
 * hold_mode: sets the "hold mode".  Really.  If the update flag is true,
 * this will also update the status line, if needed, to display the hold mode
 * state.  If update is false, only the internal flag is set.  
 */
void
hold_mode(window, flag, update)
	Window	*window;
	int	flag,
		update;
{
	if (window == (Window *) 0)
		window = curr_scr_win;
	if (flag != ON && window->scrolled_lines)
		return;
	if (flag == TOGGLE)
	{
		if (window->held == OFF)
			window->held = ON;
		else
			window->held = OFF;
	}
	else
		window->held = flag;
	if (update)
	{
		if (window->held != window->last_held)
		{
			window->last_held = window->held;
					/* This shouldn't be done
					 * this way */
			update_window_status(window, 0);
			if (window->update | UPDATE_STATUS)
				window->update -= UPDATE_STATUS;
			cursor_in_display();
			update_input(NO_UPDATE);
		}
	}
	else
		window->last_held = -1;
}

/*
 * hold_output: returns the state of the window->held, which is set in the
 * hold_mode() routine. 
 */
int
hold_output(window)
	Window	*window;
{
	if (!window)
		window = curr_scr_win;
	return (window->held == ON) || (window->scrolled_lines != 0);
}

/*
 * hold_queue: returns the string value of the next element on the hold
 * quere.  This does not change the hold queue 
 */
char	*
hold_queue(window)
	Window	*window;
{
	if (window->hold_tail)
		return (window->hold_tail->str);
	else
		return ((char *) 0);
}

int
hold_queue_logged(window)
	Window	*window;
{
	if (window->hold_tail)
		return window->hold_tail->logged;
	else
		return 0;
}

/* toggle_stop_screen: the BIND function TOGGLE_STOP_SCREEN */
void
toggle_stop_screen(key, ptr)
 	u_int	key;
	char *	ptr;
{
	hold_mode((Window *) 0, TOGGLE, 1);
	update_all_windows();
}
