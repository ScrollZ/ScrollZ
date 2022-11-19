/*
 * window.c: Handles the Main Window stuff for irc.  This includes proper
 * scrolling, saving of screen memory, refreshing, clearing, etc. 
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
 * $Id: window.c,v 1.50 2009-08-03 15:57:22 f Exp $
 */

#include "irc.h"

#include "screen.h"
#include "menu.h"
#include "window.h"
#include "vars.h"
#include "server.h"
#include "list.h"
#include "ircterm.h"
#include "names.h"
#include "ircaux.h"
#include "input.h"
#include "status.h"
#include "output.h"
#include "log.h"
#include "hook.h"
#include "dcc.h"
#include "translat.h"
#include "parse.h"
#include "whowas.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"
#include "trace.h"

#ifdef SZ32
#include <windows.h>
#endif
/****************************************************************************/

extern void Trace(long area, char *format, char *arg1, char *arg2, char *arg3, char *arg4, char *arg5, char *arg6, char *arg7, char *arg8, char *arg9, char *arg10);

/* value for underline mode, is 0 when on!  -lynx */
int	underline = 1;

/*
 * The following should synthesize MAXINT on any machine with an 8 bit
 * word.
 */
#define	MAXINT (-1&~(1<<(sizeof(int)*8-1)))

Window	*invisible_list = (Window *) 0; /* list of hidden windows */
char	*who_from = (char *) 0;		/* nick of person who's message
 					 * is being displayed */
int	who_level = LOG_CRAP;	/* Log level of message being displayed */

int	in_window_command = 0;	/* set to true if we are in window().  This
				 * is used if a put_it() is called within the
				 * window() command.  We make sure all
				 * windows are fully updated before doing the
				 * put_it(). */

extern	char	*redirect_nick;

/**************************** PATCHED by Flier ******************************/
extern NickList *tabnickcompl;
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
extern int SSLconnect;
#endif
/****************************************************************************/

/*
 * window_display: this controls the display, 1 being ON, 0 being OFF.  The
 * DISPLAY var sets this. 
 */
unsigned int	window_display = 1;

/*
 * status_update_flag: if 1, the status is updated as normal.  If 0, then all
 * status updating is suppressed 
 */
	int	status_update_flag = 1;

#ifdef lines
#undef lines
#endif /* lines */


static	void	realloc_channels _((Window *));
static	void	remove_from_window_list _((Window *));
static	void	hide_window _((Window *));
static	void	hide_other_windows _((void));
static	void	free_hold _((Window *));
static	void	free_lastlog _((Window *));
static	void	free_display _((Window *));
static	void	free_nicks _((Window *));
static	void	remove_from_invisible_list _((Window *));
static	void	revamp_window_levels _((Window *));
static	void	swap_channels_win_ptr _((Window *, Window *));
/**************************** PATCHED by Flier ******************************/
/*static	void	swap_window _((Window *, Window *));*/
void	swap_window _((Window *, Window *));
/****************************************************************************/
static	void	move_window _((Window *, int));
static	void	grow_window _((Window *, int));
/**************************** Patched by Flier ******************************/
static	void	size_window _((Window *, int));
/****************************************************************************/
static	Window	*get_next_window _((void));
static	Window	*get_previous_window _((void));
static	void	delete_other_windows _((void));
static	void	bind_channel _((char *, Window *));
static	void	unbind_channel _((char *, Window *));
/**************************** PATCHED by Flier ******************************/
/*static	void	irc_goto_window _((int));*/
void	irc_goto_window _((int));
/****************************************************************************/
static	void	list_a_window _((Window *, int, int));
static	void	list_windows _((void));
static	void	show_window _((Window *));
#ifndef LITE
static	void	push_window_by_refnum _((u_int));
static	void	pop_window _((void));
static	void	show_stack _((void));
#endif
static	int	is_window_name_unique _((char *));
static	void	add_nicks_by_refnum _((u_int, char *, int));
static	Window	*get_window _((char *, char **));
static	Window	*get_invisible_window _((char *, char **));
static	int	get_number _((char *, char **));
static	int	get_boolean _((char *, char **, int *));
static	void	win_list_channels _((Window *));

/**************************** PATCHED by Flier ******************************/
extern int      AddLast _((List *, List *));
/****************************************************************************/

/*
 * traverse_all_windows: This will do as the name implies, traverse every
 * window (visible and invisible) and return a pointer to each window on
 * subsequent calls.  If flag points to a non-zero value, then the traversal
 * in started from the beginning again, and flag is set to point to 0.  This
 * returns all visible windows first, then all invisible windows.  It returns
 * null after all windows have been returned.  It should generally be used as
 * follows: 
 *
 * flag = 1; while(tmp = traverse_all_windows(&flag)) { code here }
 *
 * Major revamp by phone (phone@coombs.anu.edu.au), December 1992.
 */
Window	*
traverse_all_windows(flag)
	int	*flag;
{
	static	Window	*which;
	static	Screen	*screen;
	static	char	visible = 1;
	int	foo = 1;

	/* First call, return the current window basically */
	if (*flag)
	{
		*flag = 0;
		visible = 1;
		if (!screen_list)
			return (Window *) 0;
		screen = screen_list;
		which = screen->window_list;
		if (which)
			return (which);
		else
			foo = 0;
	}

	/*
	 * foo is used to indicate the the current screen has no windows.
	 * This happens when we create a new screen..  either way, we want
	 * to go on to the next screen, so if foo isn't set, then if which
	 * is already null, return it again (this should never happen, if
	 * traverse_all_windows()'s is called properly), else move on to
	 * the next window
	 */
	if (foo)
	{
		if (!which)
			return (Window *) 0;
		else
			which = which->next;
 	}

	if (!which)
	{
		while (screen)
		{
			screen = screen->next;
			if (screen && screen->alive)
				break;
		}
		if (screen)
			which = screen->window_list;
	}

	if (which)
		return (which);
	/* 
	 * Got to the end of the visible list..  so we do the invisible list..
	 * Should also mean, that we've got to the end of all the visible
	 * screen..
	 */
	if (visible)
	{
		visible = 0;
		which = invisible_list;
		return (which);
	}
	return ((Window *) 0);
}

/*
 * window_traverse: This will do as the name implies, traverse every
 * window (visible and invisible) and return a pointer to each window on
 * subsequent calls.  If flag points to a non-zero value, then the traversal
 * in started from the beginning again, and flag is set to point to 0.  This
 * returns all visible windows first, then all invisible windows.  It returns
 * null after all windows have been returned.  It should generally be used as
 * follows: 
 *
 * Win_Trav stuff;
 * Window *tmp;
 *
 * stuff.flag = 1;
 * while ((tmp = window_traverse(&stuff)))
 *	{ code here } 
 *
 * this version is recursive.
 */
Window	*
window_traverse(stuff)
	Win_Trav *stuff;
{
	int	foo = 1;

	/* First call, return the current window basically */
	if (stuff->flag)
	{
		stuff->flag = 0;
		stuff->visible = 1;
		if (!screen_list)
			return (Window *) 0;
		stuff->screen = screen_list;
		stuff->which = stuff->screen->window_list;
		if (stuff->which)
			return (stuff->which);
		else
			foo = 0;
	}

	/*
	 * foo is used to indicate the the current screen has no windows.
	 * This happens when we create a new screen..  either way, we want
	 * to go on to the next screen, so if foo isn't set, then if which
	 * is already null, return it again (this should never happen, if
	 * traverse_all_windows()'s is called properly), else move on to
	 * the next window
	 */
	if (foo)
	{
		if (!stuff->which)
			return (Window *) 0;
		else
			stuff->which = stuff->which->next;
	}

	if (!stuff->which)
	{
		while (stuff->screen)
		{
			stuff->screen = stuff->screen->next;
			if (stuff->screen && stuff->screen->alive)
				break;
		}
		if (stuff->screen)
			stuff->which = stuff->screen->window_list;
	}

	if (stuff->which)
		return (stuff->which);
	/* 
	 * Got to the end of the visible list..  so we do the invisible list..
	 * Should also mean, that we've got to the end of all the visible
	 * screen..
	 */
	if (stuff->visible)
	{
		stuff->visible = 0;
		stuff->which = invisible_list;
		return (stuff->which);
	}
	return ((Window *) 0);
}

void
add_window_to_server_group(window, group)
	Window	*window;
	char	*group;
{
	int	i = find_server_group(group, 1);
	int	flag = 1;
	Window	*tmp;

	while ((tmp = traverse_all_windows(&flag)) != (Window *) 0)
		if ((tmp->server_group == i) && (tmp->server != window->server))
		{
			say("Group %s already contains a different server", group);
			return;
		}
	window->server_group = i;
	say("Window's server group is now %s", group);
        Trace(SZ_TRACE_WINDOW, "window %d set server group %s", (char *)window->refnum, (char *)group, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        TraceWindowInfo(2, window);
	update_window_status(window, 1);
}

/*
 * set_scroll_lines: called by /SET SCROLL_LINES to check the scroll lines
 * value 
 */
void 
set_scroll_lines (int size)
{
	if (size == 0)
	{
		set_var_value(SCROLL_VAR, var_settings[0]);
		if (curr_scr_win)
			curr_scr_win->scroll = 0;
	}
	else if (size > curr_scr_win->display_size)
	{
		say("Maximum lines that may be scrolled is %d", 
		    curr_scr_win->display_size);
 		set_int_var(SCROLL_LINES_VAR, (u_int)curr_scr_win->display_size);
	}
}

/*
 * set_scroll: called by /SET SCROLL to make sure the SCROLL_LINES variable
 * is set correctly 
 */
void 
set_scroll (int value)
{
	if (value && (get_int_var(SCROLL_LINES_VAR) == 0))
	{
		put_it("You must set SCROLL_LINES to a positive value first!");
		if (curr_scr_win)
			curr_scr_win->scroll = 0;
	}
	else
	{
		if (curr_scr_win)
		{
			int	old_value = curr_scr_win->scroll;

			old_value = curr_scr_win->scroll;
			curr_scr_win->scroll = value;
			if (old_value != value)
				scroll_window(curr_scr_win);
		}
	}
}

/*
 * reset_line_cnt: called by /SET HOLD_MODE to reset the line counter so we
 * always get a held screen after the proper number of lines 
 */
void 
reset_line_cnt (int value)
{
	curr_scr_win->hold_mode = value;
	curr_scr_win->hold_on_next_rite = 0;
	curr_scr_win->line_cnt = 0;
}

/*
 * set_continued_line: checks the value of CONTINUED_LINE for validity,
 * altering it if its no good 
 */
void 
set_continued_line (char *value)
{
	if (value && ((int) strlen(value) > (current_screen->co / 2)))
		value[current_screen->co / 2] = '\0';
}

/*
 * free_hold: This frees all the data and structures associated with the hold
 * list for the given window 
 */
static	void
free_hold(window)
	Window	*window;
{
	Hold *tmp,
	    *next;

	for (tmp = window->hold_head; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->str));
		new_free(&tmp);
	}
}

/*
 * free_lastlog: This frees all data and structures associated with the
 * lastlog of the given window 
 */
static	void
free_lastlog(window)
	Window	*window;
{
	Lastlog *tmp,
	    *next;

	for (tmp = window->lastlog_head; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->msg));
		new_free(&tmp);
	}
}

/*
 * free_display: This frees all memory for the display list for a given
 * window.  It resets all of the structures related to the display list
 * appropriately as well 
 */
static	void
free_display(window)
	Window	*window;
{
	Display *tmp,
	    *next;
	int	i;

	if (window == (Window *) 0)
		window = curr_scr_win;
	for (tmp = window->top_of_display, i = 0; i < window->display_size; i++, tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->line));
		new_free(&tmp);
        }
	window->top_of_display = (Display *) 0;
	window->display_ip = (Display *) 0;
	window->display_size = 0;
}

static	void
free_nicks(window)
	Window	*window;
{
	NickList *tmp,
	    *next;

	for (tmp = window->nicks; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->nick));
/**************************** PATCHED by Flier ******************************/
                new_free(&(tmp->userhost));
/****************************************************************************/
		new_free(&tmp);
	}
}

/*
 * erase_display: This effectively causes all members of the display list for
 * a window to be set to empty strings, thus "clearing" a window.  It sets
 * the cursor to the top of the window, and the display insertion point to
 * the top of the display. Note, this doesn't actually refresh the screen,
 * just cleans out the display list 
 */
void
erase_display(window)
	Window	*window;
{
	int	i;
	Display *tmp;

	if (dumb)
		return;
	if (window == (Window *) 0)
		window = curr_scr_win;
	for (tmp = window->top_of_display, i = 0; i < window->display_size;
			i++, tmp = tmp->next)
		new_free(&(tmp->line));
	window->cursor = 0;
	window->line_cnt = 0;
	window->hold_on_next_rite = 0;
	window->display_ip = window->top_of_display;
}

static	void
remove_from_invisible_list(window)
	Window	*window;
{
	window->visible = 1;
	window->screen = current_screen;
	window->miscflags &= ~WINDOW_NOTIFIED;
	window->miscflags &= ~WINDOW_REPWORD;
	if (window->prev)
		window->prev->next = window->next;
	else
		invisible_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
}

extern	void
add_to_invisible_list(window)
	Window	*window;
{
	if ((window->next = invisible_list) != NULL)
		invisible_list->prev = window;
	invisible_list = window;
	window->prev = (Window *) 0;
	window->visible = 0;
	window->screen = (Screen *) 0;
}

/* swap_channels_win_ptr: must be called because swap_window modifies the
 * content of *v_window and *window instead of swapping pointers
 */
static	void
swap_channels_win_ptr(v_window, window)
Window	*v_window,
        *window;
{
    int i;
    ChannelList	*chan;

    if ((v_window->server != -1) && server_list && (v_window->server < number_of_servers)) {
        for (chan = server_list[v_window->server].chan_list; chan; chan = chan->next) {
            if (chan->window == v_window) {
                Trace(SZ_TRACE_CHANNEL, "channel %s swap window %p -> %p",
                      (char *)chan->channel, (char *)chan->window, (char *)window, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                chan->window = window;
            }
            else if (chan->window == window) {
                Trace(SZ_TRACE_CHANNEL, "channel %s swap window %p -> %p",
                      (char *)chan->channel, (char *)chan->window, (char *)v_window, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                chan->window = v_window;
            }
        }
    }
    /* added by flier
     * The code did not cover scenario: have three windows, window 1 with
     * channel #chan1, window 2 with channel #chan2 and window 3 with
     * channel #chan3. Windows 1 and 2 are connected to server 1 and
     * window 3 is connected to server 2. Make window 2 visible window and
     * disconnect from server. Make window 3 visible window, make window
     * 1 visible window. Reconnect to server 1. Channels #chan1 and #chan2
     * are rejoined in window 1.
     * The solution is to swap window pointers in channels on all servers. */
    for (i = 0; i < number_of_servers; i++) {
        if ((i == window->server) || (i == v_window->server))
            continue;

        for (chan = server_list[i].chan_list; chan; chan = chan->next) {
            if (chan->window == v_window) {
                Trace(SZ_TRACE_CHANNEL, "channel %s, server %d, swap window %p -> %p",
                      (char *)chan->channel, (char *)i, (char *)chan->window, (char *)window, NULL, NULL, NULL, NULL, NULL, NULL);
                chan->window = window;
            }
            else if (chan->window == window) {
                Trace(SZ_TRACE_CHANNEL, "channel %s, server %d, swap window %p -> %p",
                      (char *)chan->channel, (char *)i, (char *)chan->window, (char *)v_window, NULL, NULL, NULL, NULL, NULL, NULL);
                chan->window = v_window;
            }
        }
    }

    if (window->server == v_window->server)
        return;
    if ((window->server != -1) && server_list && (window->server < number_of_servers)) {
        for (chan = server_list[window->server].chan_list; chan; chan = chan->next) {
            if (chan->window == window) {
                Trace(SZ_TRACE_CHANNEL, "channel %s swap window %p -> %p",
                      (char *)chan->channel, (char *)chan->window, (char *)v_window, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                chan->window = v_window;
            }
        }
    }
}

/*
 * swap_window: This swaps the given window with the current window.  The
 * window passed must be invisible.  Swapping retains the positions of both
 * windows in their respective window lists, and retains the dimensions of
 * the windows as well, but update them depending on the number of status
 * lines displayed
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
swap_window(v_window, window)
	Window	*v_window;
	Window	*window;
{
	Window tmp, *prev, *next;
	int	top, bottom, size;

	if (window->visible || !v_window->visible)
	{
		say("You can only SWAP a hidden window with a visible window.");
		return;
	}

/**************************** Patched by Flier ******************************/
        /* fix terminal resize bug with hidden windows */
        if (window->display_size != v_window->display_size)
            window->display_size = v_window->display_size;
        Trace(SZ_TRACE_WINDOW, "swapping window %d (%s) %p with window %d (%s) %p",
              (char *)v_window->refnum, EMPTY_STR(v_window->name), (char *)v_window,
              (char *)window->refnum, EMPTY_STR(window->name), (char *)window, NULL, NULL, NULL, NULL);
        swap_whowas_chan_win_ptr(v_window, window);
/****************************************************************************/

	swap_channels_win_ptr(v_window, window);

	prev = v_window->prev;
	next = v_window->next;

	current_screen->last_window_refnum = v_window->refnum;
	current_screen->last_window_refnum = v_window->refnum;
	remove_from_invisible_list(window);

	tmp = *v_window;
	*v_window = *window;
	v_window->top = tmp.top;
	v_window->bottom = tmp.bottom + tmp.double_status - 
		v_window->double_status;
	v_window->display_size = tmp.display_size + tmp.menu.lines +
		tmp.double_status -
#ifdef SCROLL_AFTER_DISPLAY
		v_window->menu.lines - v_window->double_status - 1;
#else
		v_window->menu.lines - v_window->double_status;
#endif /* SCROLL_AFTER_DISPLAY */
	v_window->prev = prev;
	v_window->next = next;

	/* I don't understand the use of the following, I'll ignore it
	 * If double status screws window sizes, I should look here
	 * again - krys
	 */
	top = window->top;
	bottom = window->bottom;
	size = window->display_size;
	*window = tmp;
	window->top = top;
	window->bottom = bottom - tmp.double_status;
#ifdef SCROLL_AFTER_DISPLAY
	window->display_size = size - 1;
#else
	window->display_size = size;
#endif /* SCROLL_AFTER_DISPLAY */

	add_to_invisible_list(window);

	v_window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;

/**************************** PATCHED by Flier ******************************/
        if (v_window && window && v_window->server!=window->server)
            tabnickcompl=NULL;
/****************************************************************************/
	do_hook(WINDOW_SWAP_LIST, "%d %d", v_window->refnum, window->refnum);
}

/*
 * move_window: This moves a window offset positions in the window list. This
 * means, of course, that the window will move on the screen as well 
 */
static	void
move_window(window, offset)
	Window	*window;
	int	offset;
{
	Window	*tmp,
	    *last;
	int	win_pos,
	pos;

	if (offset == 0)
		return;
	last = (Window *) 0;
	for (win_pos = 0, tmp = current_screen->window_list; tmp;
	    tmp = tmp->next, win_pos++)
	{
		if (window == tmp)
			break;
		last = tmp;
	}
	if (tmp == (Window *) 0)
		return;
	if (last == (Window *) 0)
		current_screen->window_list = tmp->next;
	else
		last->next = tmp->next;
	if (tmp->next)
		tmp->next->prev = last;
	else
		current_screen->window_list_end = last;
	win_pos = (offset + win_pos) % window->screen->visible_windows;
	if (win_pos < 0)
		win_pos = window->screen->visible_windows + win_pos;
	last = (Window *) 0;
	for (pos = 0, tmp = current_screen->window_list;
	    pos != win_pos; tmp = tmp->next, pos++)
		last = tmp;
	if (last == (Window *) 0)
		current_screen->window_list = window;
	else
		last->next = window;
	if (tmp)
		tmp->prev = window;
	else
		current_screen->window_list_end = window;
	window->prev = last;
	window->next = tmp;
	recalculate_window_positions();
}

/*
 * grow_window: This will increase or descrease the size of the given window
 * by offset lines (positive offset increases, negative decreases).
 * Obviously, with a fixed terminal size, this means that some other window
 * is going to have to change size as well.  Normally, this is the next
 * window in the window list (the window below the one being changed) unless
 * the window is the last in the window list, then the previous window is
 * changed as well 
 */
static	void
grow_window(window, offset)
	Window	*window;
	int	offset;
{
	Window	*other,
	    *tmp;
	int	after,
	window_size,
	other_size;

	if (window == (Window *) 0)
		window = curr_scr_win;
	if (!window->visible)
	{
		say("You cannot change the size of hidden windows!");
		return;
	}
	if (window->next)
	{
		other = window->next;
		after = 1;
	}
	else
	{
		other = (Window *) 0;
		for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		{
			if (tmp == window)
				break;
			other = tmp;
		}
		if (other == (Window *) 0)
		{
			say("Can't change the size of this window!");
			return;
		}
		after = 0;
	}
	window_size = window->display_size + offset;
	other_size = other->display_size - offset;
	if ((window_size < 4) ||
	    (other_size < 4))
	{
		say("Not enough room to resize this window!");
		return;
	}
	if (after)
	{
		window->bottom += offset;
		other->top += offset;
	}
	else
	{
		window->top -= offset;
		other->bottom -= offset;
	}
#ifdef SCROLL_AFTER_DISPLAY
	window->display_size = window_size - 1;
	other->display_size = other_size - 1;
#else
	window->display_size = window_size;
	other->display_size = other_size;
#endif /* SCROLL_AFTER_DISPLAY */
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	other->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	term_flush();
}

/**************************** Patched by Flier ******************************/
/*
 * size_window: Set window size. See grow_window() for details.
 */
static void size_window(window,newsize)
Window *window;
int newsize;
{
    if (!window) window=curr_scr_win;
    grow_window(window,newsize-window->display_size);
}
/****************************************************************************/

/*
 * the message_from stack structure.
 */
struct mfstack
{
	char	*who_from;	/* saved from */
 	int	who_level;	/* saved level */
 	struct mfstack *next;	/* next in the list */
} mfstack_head = { NULL, 0, NULL };

/*
 * save_message_from: this is used to save (for later restoration) the
 * who_from variable.  This comes in handy very often when a routine might
 * call another routine that might change who_from.   Note that if you
 * call this routine, you *must* call restore_message_from().
 */
void 
save_message_from (void)
{
 	struct mfstack *mfs;

 	mfs = (struct mfstack *)new_malloc(sizeof *mfs);

 	mfs->who_from = NULL;
 	malloc_strcpy(&(mfs->who_from), who_from);
 	mfs->who_level = who_level;
 	mfs->next = mfstack_head.next;

 	mfstack_head.next = mfs;
}

/* restore_message_from: restores a previously saved who_from variable */
void 
restore_message_from (void)
{
 	struct mfstack *mfs = mfstack_head.next;

	if (mfs == NULL)
	{
 		/*yell("--- restore_message_from: NULL next pointer, fudging..");*/
 		malloc_strcpy(&who_from, NULL);
 		who_level = 0;
	}
	else
	{
 		malloc_strcpy(&who_from, mfs->who_from);
 		who_level = mfs->who_level;
 		mfstack_head.next = mfs->next;
 		new_free(&mfs->who_from);
 		new_free(&mfs);
 	}
}

/*
 * message_from: With this you can the who_from variable and the who_level
 * variable, used by the display routines to decide which window messages
 * should go to.  
 */
void 
message_from (char *who, int level)
{
	malloc_strcpy(&who_from, who);
	who_level = level;
}

/*
 * message_from_level: Like set_lastlog_msg_level, except for message_from.
 * this is needed by XECHO, because we could want to output things in more
 * than one level.
 */
int 
message_from_level (int level)
{
	int	temp;

	temp = who_level;
	who_level = level;
	return temp;
}

/*
 * get_window_by_refnum: Given a reference number to a window, this returns a
 * pointer to that window if a window exists with that refnum, null is
 * returned otherwise.  The "safe" way to reference a window is throught the
 * refnum, since a window might be delete behind your back and and Window
 * pointers might become invalid.  
 */
Window	*
get_window_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;
	int	flag = 1;

	if (refnum)
	{
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->refnum == refnum)
				return (tmp);
		}
	}
	else
		return (curr_scr_win);
	return ((Window *) 0);
}

/*
 * get_window_by_level: Given a level, this returns a pointer to that
 * window if a window exists with that level, null is returned otherwise. 
 */
Window	*
get_window_by_level(level)
u_int level;
{
    Window *tmp;
    int flag=1;

    while ((tmp=traverse_all_windows(&flag))!=NULL) {
        if (((tmp->window_level)&level)==level)
            return(tmp);
    }
    return((Window *) 0);
}

/*
 * clear_window_by_refnum: just like clear_window(), but it uses a refnum. If
 * the refnum is invalid, the current window is cleared. 
 */
void
clear_window_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	clear_window(tmp);
}

/*
 * revamp_window_levels: Given a level setting for the current window, this
 * makes sure that that level setting is unused by any other window. Thus
 * only one window in the system can be set to a given level.  This only
 * revamps levels for windows with servers matching the given window 
 * it also makes sure that only one window has the level `DCC', as this is
 * not dependant on a server.
 */
static	void
revamp_window_levels(window)
	Window	*window;
{
	Window	*tmp;
	int	flag = 1;
	int	got_dcc;

	got_dcc = (LOG_DCC & window->window_level) ? 1 : 0;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp == window)
			continue;
		if (LOG_DCC & tmp->window_level)
		{
			if (0 != got_dcc)
				tmp->window_level &= ~LOG_DCC;
			got_dcc = 1;
		}
		if (window->server == tmp->server)
			tmp->window_level ^= (tmp->window_level & window->window_level);
	}
}

/*
 * set_level_by_refnum: This sets the window level given a refnum.  It
 * revamps the windows levels as well using revamp_window_levels() 
 */
void
set_level_by_refnum(refnum, level)
	u_int	refnum;
	int	level;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	tmp->window_level = level;
	revamp_window_levels(tmp);
}

/*
 * set_prompt_by_refnum: changes the prompt for the given window.  A window
 * prompt will be used as the target in place of the query user or current
 * channel if it is set 
 */
void
set_prompt_by_refnum(refnum, prompt)
	u_int	refnum;
	char	*prompt;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	malloc_strcpy(&(tmp->prompt), prompt);
}

/*
 * message_to: This allows you to specify a window (by refnum) as a
 * destination for messages.  Used by EXEC routines quite nicely 
 */
void
message_to(refnum)
	u_int	refnum;
{
	if (refnum)
		to_window = get_window_by_refnum(refnum);
	else
		to_window = (Window *) NULL;
}

/*
 * get_next_window: This returns a pointer to the next *visible* window in
 * the window list.  It automatically wraps at the end of the list back to
 * the beginning of the list 
 */
static	Window	*
get_next_window()
{
	if (curr_scr_win && curr_scr_win->next)
		return (curr_scr_win->next);
	else
		return (current_screen->window_list);
}

/*
 * get_previous_window: this returns the previous *visible* window in the
 * window list.  This automatically wraps to the last window in the window
 * list 
 */
static	Window	*
get_previous_window()
{
	if (curr_scr_win && curr_scr_win->prev)
		return (curr_scr_win->prev);
	else
		return (current_screen->window_list_end);
}

/*
 * set_current_window: This sets the "current" window to window.  It also
 * keeps track of the last_current_screen->current_window by setting it to the
 * previous current window.  This assures you that the new current window is
 * visible.
 * If not, a new current window is chosen from the window list 
 */
void
set_current_window(window)
	Window	*window;
{
	Window	*tmp;
	unsigned int	refnum;
/**************************** PATCHED by Flier ******************************/
        Window *oldcurw=curr_scr_win;
/****************************************************************************/

	refnum = current_screen->last_window_refnum;
	if (curr_scr_win)
	{
		curr_scr_win->update |= UPDATE_STATUS;
		current_screen->last_window_refnum = curr_scr_win->refnum;
	}
	if ((window == (Window *) 0) || (!window->visible))

	{
		if ((tmp = get_window_by_refnum(refnum)) && (tmp->visible))
			curr_scr_win = tmp;
		else
			curr_scr_win = get_next_window();
	}
	else
		curr_scr_win = window;
	curr_scr_win->update |= UPDATE_STATUS;
/**************************** PATCHED by Flier ******************************/
        if (curr_scr_win && oldcurw && curr_scr_win->server!=oldcurw->server)
            tabnickcompl=NULL;
/****************************************************************************/
}

/*
 * swap_last_window:  This swaps the current window with the last window
 * that was hidden.
 */

void
swap_last_window(key, ptr)
 	u_int	key;
	char *	ptr;
{
	if (invisible_list == (Window *) 0)
	{
		/* say("There are no hidden windows"); */
		/* Not sure if we need to warn   - phone. */
		return;
	}
	swap_window(curr_scr_win, invisible_list);
	update_all_windows();
	cursor_to_input();
}

/*
 * next_window: This switches the current window to the next visible window 
 */
void
next_window(key, ptr)
 	u_int	key;
	char *	ptr;
{
	if (current_screen->visible_windows == 1)
		return;
	set_current_window(get_next_window());
	update_all_windows();
}

/*
 * swap_next_window:  This swaps the current window with the next hidden
 * window.
 */

void
swap_next_window(key, ptr)
 	u_int	key;
	char *	ptr;
{
	int	flag;
	Window	*tmp;
	u_int	next = MAXINT;
	int	smallest;

	if (invisible_list == (Window *) 0)
	{
		say("There are no hidden windows");
		return;
	}
	flag = 1;
	smallest = curr_scr_win->refnum;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (!tmp->visible)
		{
			if (tmp->refnum < smallest)
				smallest = tmp->refnum;
			if ((tmp->refnum > curr_scr_win->refnum)
			    && (next > tmp->refnum))
				next = tmp->refnum;
		}
	}
	if (next != MAXINT)
		tmp = get_window_by_refnum(next);
	else
 		tmp = get_window_by_refnum((u_int)smallest);
	swap_window(curr_scr_win, tmp);
	update_all_windows();
	update_all_status();
	cursor_to_input();
}

/*
 * previous_window: This switches the current window to the previous visible
 * window 
 */
void
previous_window(key, ptr)
	u_int	key;
	char *	ptr;
{
	if (current_screen->visible_windows == 1)
		return;
	set_current_window(get_previous_window());
	update_all_windows();
}

/*
 * swap_previous_window:  This swaps the current window with the next 
 * hidden window.
 */

void
swap_previous_window(key, ptr)
 	u_int	key;
	char *	ptr;
{
	int	flag;
	Window	*tmp;
	int	previous = 0;
	int	largest;

	if (invisible_list == (Window *) 0)
	{
		say("There are no hidden windows");
		return;
	}
	flag = 1;
	largest = curr_scr_win->refnum;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (!tmp->visible)
		{
			if (tmp->refnum > largest)
				largest = tmp->refnum;
			if ((tmp->refnum < curr_scr_win->refnum)
			    && (previous < tmp->refnum))
				previous = tmp->refnum;
		}
	}
	if (previous)
 		tmp = get_window_by_refnum((u_int)previous);
	else
 		tmp = get_window_by_refnum((u_int)largest);
	swap_window(curr_scr_win,tmp);
	update_all_windows();
	update_all_status();
	cursor_to_input();
}

/*
 * back_window:  goes to the last window that was current.  Swapping the
 * current window if the last window was hidden.
 */

void
back_window(key, ptr)
 	u_int	key;
	char *	ptr;
{
	Window	*tmp;

	tmp = get_window_by_refnum(current_screen->last_window_refnum);
/**************************** PATCHED by Flier ******************************/
        /* tmp is NULL after we do /WINDOW KILL */
        if (!tmp) return;
/****************************************************************************/
	if (tmp->visible)
		set_current_window(tmp);
	else
	{
		swap_window(curr_scr_win, tmp);
		update_all_windows();
		update_all_status();
		cursor_to_input();
	}
}

/*
 * add_to_window_list: This inserts the given window into the visible window
 * list (and thus adds it to the displayed windows on the screen).  The
 * window is added by splitting the current window.  If the current window is
 * too small, the next largest window is used.  The added window is returned
 * as the function value or null is returned if the window couldn't be added 
 */
extern	Window	*
add_to_window_list(new)
	Window	*new;
{
	Window	*biggest = (Window *) 0,
		*tmp;

	current_screen->visible_windows++;
	if (curr_scr_win == (Window *) 0)
	{
		current_screen->window_list_end =
				current_screen->window_list = new;
		if (dumb)
		{
#ifdef SCROLL_AFTER_DISPLAY
			new->display_size = 24 - 1;
#else
			new->display_size = 24;	/* what the hell */
#endif /* SCROLL_AFTER_DISPLAY */
			set_current_window(new);
			return (new);
		}
		recalculate_windows();
	}
	else
	{
		/* split current window, or find a better window to split */
		if ((curr_scr_win->display_size < 4) ||
				get_int_var(ALWAYS_SPLIT_BIGGEST_VAR))
		{
			int	size = 0;

			for (tmp = current_screen->window_list; tmp;
					tmp = tmp->next)
			{
				if (tmp->display_size > size)
				{
					size = tmp->display_size;
					biggest = tmp;
				}
			}
			if ((biggest == (Window *) 0) || (size < 4))
			{
				say("Not enough room for another window!");
				/* Probably a source of memory leaks */
				new_free(&new);
				current_screen->visible_windows--;
				return ((Window *) 0);
			}
		}
		else
			biggest = curr_scr_win;
		if ((new->prev = biggest->prev) != NULL)
			new->prev->next = new;
		else
			current_screen->window_list = new;
		new->next = biggest;
		biggest->prev = new;
		new->top = biggest->top;
		new->bottom = (biggest->top + biggest->bottom) / 2 -
			new->double_status;
		biggest->top = new->bottom + new->double_status + 1;
#ifdef SCROLL_AFTER_DISPLAY
		new->display_size = new->bottom - new->top - 1;
		biggest->display_size = biggest->bottom - biggest->top -
			biggest->menu.lines - 1;
#else
		new->display_size = new->bottom - new->top;
		biggest->display_size = biggest->bottom - biggest->top -
			biggest->menu.lines;
#endif /* SCROLL_AFTER_DISPLAY */
		new->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		biggest->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	}
	return (new);
}

/*
 * remove_from_window_list: this removes the given window from the list of
 * visible windows.  It closes up the hole created by the windows absense in
 * a nice way 
 */
static void
remove_from_window_list(window)
	Window	*window;
{
	Window	*other;

	/* find adjacent visible window to close up the screen */
	for (other = window->next; other; other = other->next)
	{
		if (other->visible)
		{
			other->top = window->top;
			break;
		}
	}
	if (other == (Window *) 0)
	{
		for (other = window->prev; other; other = other->prev)
		{
			if (other->visible)
			{
 				other->bottom = window->bottom + window->double_status - other->double_status;
				break;
			}
		}
	}
	/* remove window from window list */
	if (window->prev)
		window->prev->next = window->next;
	else
		current_screen->window_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
	else
		current_screen->window_list_end = window->prev;
	if (window->visible)
	{
		current_screen->visible_windows--;
#ifdef SCROLL_AFTER_DISPLAY
 		other->display_size = other->bottom - other->top - other->menu.lines - 1;
#else
 		other->display_size = other->bottom - other->top - other->menu.lines;
#endif /* SCROLL_AFTER_DISPLAY */
                if (window == curr_scr_win)
			set_current_window((Window *) 0);
		if (window->refnum == current_screen->last_window_refnum)
 			current_screen->last_window_refnum = curr_scr_win->refnum;
        }
}

/*
 * window_check_servers: this checks the validity of the open servers vs the
 * current window list.  Every open server must have at least one window
 * associated with it.  If a window is associated with a server that's no
 * longer open, that window's server is set to the primary server.  If an
 * open server has no assicatiate windows, that server is closed.  If the
 * primary server is no more, a new primary server is picked from the open
 * servers 
 */
void 
window_check_servers (void)
{
	Window	*tmp;
	int	flag, cnt, max, i, not_connected,
	prime = -1;
/**************************** PATCHED by Flier ******************************/
        int primary_grp = 0;
/****************************************************************************/

	connected_to_server = 0;
	max = number_of_servers;
	for (i = 0; i < max; i++)
	{
		not_connected = !is_server_open(i);
		flag = 1;
		cnt = 0;
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->server == i)
			{
				if (not_connected)
				{
					tmp->server = primary_server;
					if (tmp->current_channel)
						new_free(&tmp->current_channel);
				}
				else
				{
/**************************** PATCHED by Flier ******************************/
                                    if (primary_grp == 0)
                                        primary_grp = tmp->server;
/****************************************************************************/
					prime = tmp->server;
					cnt++;
				}
			}
		}
                if (cnt == 0)
                {
#ifdef NON_BLOCKING_CONNECTS
			if (!(server_list[i].flags & CLOSE_PENDING))
#endif /* NON_BLOCKING_CONNECTS */
			close_server(i, empty_string);
                }
		else
			connected_to_server++;
	}

	if (!is_server_open(primary_server))
	{
		flag = 1;
		while ((tmp = traverse_all_windows(&flag)) != NULL)
/**************************** PATCHED by Flier ******************************/
			/*if (tmp->server == primary_server)*/
                        if (tmp->server == primary_server &&
                            (tmp->server_group == 0 ||
                             tmp->server_group == primary_grp))
/****************************************************************************/
			{
				tmp->server = prime;
			}
		primary_server = prime;
	}
	update_all_status();
	cursor_to_input();
}

/*
 * restore_previous_server: Attempts to restore all windows that were
 * associated with `server', that currently contain nothing, to said
 * server.
 */
void 
window_restore_server (int server)
{
	Window	*tmp;
	int	max = number_of_servers,
		i,
		flag = 1;

	for (i = 0; i < max; i++)
	{
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->server == primary_server &&
			    tmp->prev_server == server)
			{
				int t = tmp->server;

				tmp->server = tmp->prev_server;
				tmp->prev_server = t;
				realloc_channels(tmp);
			}
		}
	}
}

/*
 * realloc_channels: Attempts to reallocate a channel to a window of the
 * same server
 *
 * XXX mrg this looks broken to me but i'm not sure.  blah.
 */
static	void
realloc_channels(window)
	Window	*window;
{
	Window	*tmp;
	int	flag = 1;
	ChannelList	*chan;

	while ((tmp = traverse_all_windows(&flag)))
		if (window != tmp && tmp->server == window->server)
		{
/**************************** Patched by Flier ******************************/
                        /* unset IRCSERVER
                         * scrollz
                         * /window new
                         * /window kill
                         * client dies */
                        if (window->server < 0)
                            continue;
/****************************************************************************/
			for (chan = server_list[window->server].chan_list; chan; chan = chan->next)
				if (chan->window == window)
				{
					chan->window = tmp;
					if (!tmp->current_channel)
						set_channel_by_refnum(tmp->refnum, chan->channel);
				}
			return;
		}
/**************************** Patched by Flier ******************************/
        if (window->server < 0)
            return;
/****************************************************************************/
	for (chan = server_list[window->server].chan_list; chan; chan = chan->next)
	{
		chan->window = (Window *) 0;
	}
}

/*
 * delete_window: This deletes the given window.  It frees all data and
 * structures associated with the window, and it adjusts the other windows so
 * they will display correctly on the screen. 
 */
void
delete_window(window)
	Window	*window;
{
	char	*tmp = (char *) 0;
	char	buffer[BIG_BUFFER_SIZE+1];

	if (window == (Window *) 0)
		window = curr_scr_win;
	if (window->visible && (current_screen->visible_windows == 1))
	{
		if (invisible_list)
		{
			swap_window(window, invisible_list);
			window = invisible_list;
		}
		else
		{
			say("You can't kill the last window!");
			return;
		}
	}
	if (window->name)
		strmcpy(buffer, window->name, BIG_BUFFER_SIZE);
	else
		snprintf(buffer, sizeof buffer, "%u", window->refnum);
	malloc_strcpy(&tmp, buffer);
	realloc_channels(window);
/**************************** PATCHED by Flier ******************************/
#if defined(OPERVISION) && defined(WANTANSI)
        if (OperV && !strcmp(buffer,"OV")) OperV=0;
#endif
/****************************************************************************/
	new_free(&window->status_line[0]);
	new_free(&window->status_line[1]);
/**************************** PATCHED by Flier ******************************/
	new_free(&window->status_line[2]);
/****************************************************************************/
	new_free(&window->query_nick);
	new_free(&window->current_channel);
	new_free(&window->logfile);
	new_free(&window->name);
        free_display(window);
	free_hold(window);
	free_lastlog(window);
	free_nicks(window);
	if (window->visible)
		remove_from_window_list(window);
	else
		remove_from_invisible_list(window);
	new_free(&window);
	window_check_servers();
	do_hook(WINDOW_KILL_LIST, "%s", tmp);
	new_free(&tmp);
}

/**************************** PATCHED by Flier ******************************/
/*
 * free_bound_channels: This deletes the list of bound channels
 * for given window.
 */
void free_bound_channels(window)
Window *window;
{
    struct channels *chan, *next;

    for (chan = window->bound_chans; chan; chan = next) {
        next = chan->next;
        new_free(&chan->channel);
        new_free(&chan);
    }
}

/*
 * cleanup_window: This deletes the given window.  It frees all data and
 * structures associated with the window.
 */
void cleanup_window(window)
Window *window;
{
    new_free(&window->status_line[0]);
    new_free(&window->status_line[1]);
    new_free(&window->status_line[2]);
    new_free(&window->query_nick);
    new_free(&window->current_channel);
    new_free(&window->logfile);
    new_free(&window->name);
    if (window->top_of_display)
        free_display(window);
    free_hold(window);
    free_lastlog(window);
    free_nicks(window);
    free_bound_channels(window);
    new_free(&window);
}
/****************************************************************************/

/* delete_other_windows: zaps all visible windows except the current one */
static void 
delete_other_windows (void)
{
	Window	*tmp,
		*cur,
		*next;

	cur = curr_scr_win;
	tmp = current_screen->window_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != cur)
		{
			delete_window(tmp);
			update_all_windows();
		}
		tmp = next;
	}
}

/*
 * window_kill_swap:  Swaps with the last window that was hidden, then
 * kills the window that was swapped.  Give the effect of replacing the
 * current window with the last one, and removing it at the same time.
 */

void 
window_kill_swap (void)
{
	if (invisible_list != (Window *) 0)
	{
		swap_last_window(0, (char *) 0);
		delete_window(get_window_by_refnum(current_screen->last_window_refnum));
	}
	else
		say("There are no hidden windows!");
}

/*
 * unhold_windows: This is used by the main io loop to display held
 * information at an appropriate time.  Each time this is called, each
 * windows hold list is checked.  If there is info in the hold list and the
 * window is not held, the first line is displayed and removed from the hold
 * list.  Zero is returned if no infomation is displayed 
 */
int 
unhold_windows (void)
{
	Window	*tmp;
	char	*stuff;
	int	hold_flag = 0,
		flag = 1;
	int	logged;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (!hold_output(tmp) && (stuff = hold_queue(tmp)))
		{
			logged = hold_queue_logged(tmp);
			if (rite(tmp, stuff, 1, 0, 0, logged) == 0)
			{
				remove_from_hold_list(tmp);
				hold_flag = 1;
			}
		}
	}
	return (hold_flag);
}

/*
 * update_window_status: This updates the status line for the given window.
 * If the refresh flag is true, the entire status line is redrawn.  If not,
 * only some of the changed portions are redrawn 
 */
void
update_window_status(window, refreshit)
	Window	*window;
	int	refreshit;
{
	if (dumb || (!window->visible) || !status_update_flag || never_connected)
		return;
	if (window == (Window *) 0)
		window = curr_scr_win;
	if (refreshit)
	{
		new_free(&window->status_line[0]);
		new_free(&window->status_line[1]);
/**************************** PATCHED by Flier ******************************/
		new_free(&window->status_line[2]);
/****************************************************************************/
	}
	make_status(window);
}

/*
 * redraw_all_status: This redraws all of the status lines for all of the
 * windows. 
 */
void 
redraw_all_status (void)
{
	Window	*tmp;

	if (dumb)
		return;
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		new_free(&tmp->status_line[0]);
		new_free(&tmp->status_line[1]);
/**************************** PATCHED by Flier ******************************/
		new_free(&tmp->status_line[2]);
/****************************************************************************/
		make_status(tmp);
	}
	update_input(UPDATE_JUST_CURSOR);
	term_flush();
}

/*
 * update_all_status: This updates all of the status lines for all of the
 * windows.  By updating, it only draws from changed portions of the status
 * line to the right edge of the screen 
 */
/*ARGSUSED*/
void 
update_all_status (void)
{
	Window	*window;
	Screen	*screen;

	if (dumb || !status_update_flag || never_connected)
		return;
	for (screen = screen_list; screen; screen = screen->next)
	{
		if (!screen->alive)
			continue;
		for (window = screen->window_list;window; window = window->next)
			if (window->visible)
				make_status(window);
	}
	update_input(UPDATE_JUST_CURSOR);
	term_flush();
}

/*
 * status_update: sets the status_update_flag to whatever flag is.  This also
 * calls update_all_status(), which will update the status line if the flag
 * was true, otherwise it's just ignored 
 */
void 
status_update (int flag)
{
	status_update_flag = flag;
	update_all_status();
	cursor_to_input();
}

/*
 * is_current_channel: Returns true is channel is a current channel for any
 * window.  If the delete flag is not 0, then unset channel as the current
 * channel and attempt to replace it by a non-current channel or the 
 * current_channel of window specified by value of delete
 */
int 
is_current_channel (char *channel, int server, int delete)
{
	Window	*tmp,
		*found_window = (Window *) 0;
	int	found = 0,
		flag = 1;

/**************************** PATCHED by Flier ******************************/
        if (!channel) {
            found=1;
            found_window=curr_scr_win;
            delete=curr_scr_win->refnum;
            channel="";
        }
        else
/****************************************************************************/
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		char *	c = tmp->current_channel;

		if (c && !my_stricmp(channel, c) && tmp->server == server)
		{
			found_window = tmp;
			found = 1;
			if (delete)
			{
				new_free(&(tmp->current_channel));
				tmp->update |= UPDATE_STATUS;
			}
		}
	}
	if (found && delete)
	{
		ChannelList	*chan;
                char		*delete_channel;
		ChannelList	*possible = (ChannelList *) 0;

		for (chan = server_list[server].chan_list; chan; chan = chan->next)
		{

                        if (!my_stricmp(chan->channel, channel))
				continue;
			if (chan->window == found_window)
			{
				set_channel_by_refnum(found_window->refnum,
							chan->channel);
				return 1;
			}
                        if (!get_int_var(SAME_WINDOW_ONLY_VAR))
				if (!is_bound(chan->channel, server)
					&& (chan->window != found_window))
			{
				int	is_current = 0;

				flag = 1;
                                while ((tmp = traverse_all_windows(&flag)))
				{
 					if ((tmp->server != server))
						continue;
                                        if (tmp->current_channel
                                            && !my_stricmp(chan->channel,
                                                           tmp->current_channel))
                                            is_current = 1;
				}
				if (!is_current)
					possible = chan;
			}
		}
		if (!get_int_var(SAME_WINDOW_ONLY_VAR))
		{
			if (possible)
			{
				set_channel_by_refnum(found_window->refnum,
							possible->channel);
				return 1;
			}
 			delete_channel = get_channel_by_refnum((u_int)delete);
			if (delete_channel &&
			   !(is_bound(delete_channel, server) &&
		     	    found_window->refnum != delete))
				set_channel_by_refnum(found_window->refnum,
 					get_channel_by_refnum((u_int)delete));
		}
	}
	return (found);
}

extern	Window *
is_bound(channel, server)
	char	*channel;
	int	server;
{
	Win_Trav stuff;
	Window *tmp;

	stuff.flag = 1;
	while ((tmp = window_traverse(&stuff)))
	{
/**************************** PATCHED by Flier ******************************/
		/*if (tmp->server == server && tmp->bound_channel &&
		    !my_stricmp(channel, tmp->bound_channel))
			return tmp;*/
		if (tmp->server==server) {
                    if (find_in_list((List **) &(tmp->bound_chans), channel, 0))
                        return(tmp);
                }
/****************************************************************************/
	}

	return (Window *) 0;
}

static void
bind_channel(channel, window)
	char	*channel;
	Window	*window;
{
	Win_Trav stuff;
	Window *tmp;
/**************************** PATCHED by Flier ******************************/
        char tmpbuf[mybufsize / 2 + 1];
        struct channels *chan;
/****************************************************************************/

	/* check it isn't bound on this server elsewhere */
	stuff.flag = 1;
	while ((tmp = window_traverse(&stuff)))
	{
		if (tmp->server != window->server && tmp == window)
			continue;
/**************************** PATCHED by Flier ******************************/
		/*if (!my_stricmp(tmp->bound_channel, channel))*/
                chan=(struct channels *) find_in_list((List **) &(tmp->bound_chans), channel, 0);
                if (chan)
/****************************************************************************/
		{
/**************************** PATCHED by Flier ******************************/
			/*say("Channel %s is already bound to window %d", channel, window->refnum);*/
                        if (tmp->name) strmcpy(tmpbuf, tmp->name, sizeof(tmpbuf) - 1);
                        else snprintf(tmpbuf, sizeof(tmpbuf), "%d", tmp->refnum);
			say("Channel %s is already bound to window %s", channel, tmpbuf);
/****************************************************************************/
			return;
		}
	}
	if (is_on_channel(channel, window->server, get_server_nickname(window->server)))
	{
		is_current_channel(channel, window->server, (int)window->refnum);
		set_channel_by_refnum(0, channel);
	}
	/* XXX fix this */
#if 0
	else
	{
		int	server, sg = -1, fsg = -2;	/* different */

		server = from_server;
		from_server = window->server;

		if (server_list[server].server_group)
			sg = find_server_group(server_list[server].server_group, 0);
		if (server_list[from_server].server_group)
			fsg = find_server_group(server_list[server].server_group, 0);

		if (sg == 0 || fsg == 0)
			yell("--- huh. coudn't find server groups");

		if (sg == fsg)
		{
			switch (get_server_version(window->server)) {
			/* XXX make this use a key? */
			/* XXX by factoring out the "JOIN/CHANNEL" code to a module */
			case Server2_5:
				send_to_server("CHANNEL %s", channel);
				break;
			default:
				send_to_server("JOIN %s", channel);
			}
			add_channel(channel, from_server, CHAN_JOINING, (ChannelList *) 0);
			from_server = server;
		}
	}
#endif
/**************************** PATCHED by Flier ******************************/
	/*malloc_strcpy(&window->bound_channel, channel);
	say("Channel %s bound to window %d", channel, window->refnum);*/
        chan = (struct channels *) new_malloc(sizeof(struct channels));
        chan->channel = NULL;
        malloc_strcpy(&(chan->channel),channel);
        chan->next = NULL;
        add_to_list_ext((List **) &(window->bound_chans), (List *) chan,
                        (int (*) _((List *, List *))) AddLast);
        if (window->name) strmcpy(tmpbuf, window->name, sizeof(tmpbuf) - 1);
        else snprintf(tmpbuf, sizeof(tmpbuf), "%d", window->refnum);
        say("Channel %s bound to window %s", channel, tmpbuf);
        Trace(SZ_TRACE_WINDOW, "bind channel %s", (char *)channel, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        TraceWindowInfo(2, window);
/****************************************************************************/
}

static void
unbind_channel(channel, window)
	char	*channel;
	Window	*window;
{
/**************************** PATCHED by Flier ******************************/
        struct channels *chan;
/****************************************************************************/

	window = is_bound(channel, window->server);
	if (!window)
		return;
/**************************** PATCHED by Flier ******************************/
	/*new_free(&window->bound_channel);*/
        chan=(struct channels *) remove_from_list((List **) &(window->bound_chans), channel);
        if (chan) {
            new_free(&(chan->channel));
            new_free(&chan);
        }
        Trace(SZ_TRACE_WINDOW, "unbind channel %s", (char *)channel, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        TraceWindowInfo(2, window);
/****************************************************************************/
}

/*
 * get_window_server: returns the server index for the window with the given
 * refnum 
 */
int 
get_window_server (unsigned int refnum)
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	return (tmp->server);
}

/*
 * window_set_server:  This sets the server of the given window to server.
 * If refnum is -1 then we are setting the primary server and all windows
 * that are set to the current primary server are changed to server.  The misc
 * flag is ignored in this case.  If refnum is not -1, then that window is
 * set to the given server.  If WIN_ALL is set in misc, then all windows
 * with the same server as refnum are set to the new server as well.
 * If the window is in a group, all the group is set to the new server.
 * WIN_TRANSFER will move the channels to the new server, WIN_FORCE will
 * force a 'sticky' behaviour of the window. -Sol
 */
void 
window_set_server (int refnum, int server, int misc)
{
	int	old_serv;
	Window	*window = 0, *ptr, *new_win = (Window *) 0;
	ChannelList *tmp;
	int	moved = 0;
	int	flag = 1;

	if (refnum == -1)
	{
		old_serv = primary_server;
		primary_server = server;
		misc |= WIN_ALL;
	}
	else
	{
		window = get_window_by_refnum((u_int)refnum);
		old_serv = window->server;
	}

	if (server == old_serv)
		return;

	/* Moving all windows associated with old_serv -Sol */
	if (misc & WIN_ALL)
	{
		if ((misc & WIN_TRANSFER) && (old_serv >= 0))
		{
/**************************** PATCHED by Flier ******************************/
                        /* Don't transfer channels if WIN_OLDCONN is set here
                         * because it means connection to old server failed
                         * and if we transfer channels we lose information
                         * on nicks in channels! */
                        if (misc&WIN_OLDCONN)
/****************************************************************************/
			for (tmp = server_list[old_serv].chan_list; tmp; tmp = tmp->next)
			{
				/* XXX: moved is always 0 at this point */
				if (!moved)	/* If we're here, it means
						   we're going to transfer
						   channels to the new server,
						   so we dump old channels
						   first, but only once -Sol */
				{
					moved++;
					clear_channel_list(server);
				}
/**************************** Patched by Flier ******************************/
				/*add_channel(tmp->channel, server, CHAN_LIMBO, tmp);*/
				add_channel(tmp->channel, server, CHAN_LIMBO, tmp, NULL, 0);
/****************************************************************************/
			}
#ifdef NON_BLOCKING_CONNECTS
			if (server_list[old_serv].flags & CLOSE_PENDING)
				server_list[old_serv].flags |= CLEAR_PENDING;
			else
#endif /* NON_BLOCKING_CONNECTS */
				clear_channel_list(old_serv);
		}
		while ((ptr = traverse_all_windows(&flag)) != (Window *) 0)
			if (ptr->server == old_serv)
			{
				ptr->prev_server = ptr->server;
				ptr->server = server;
				/*
				 * XXX we could save this to old_current_channel and use
				 * that after other checks to decide where a channel should
				 * go, maybe??
				 */
				if (ptr->current_channel)
					new_free(&ptr->current_channel);
			}
		window_check_servers();
		return;
	}

	/*
	 * We are setting only some windows of the old server : let's look
	 * for a window of that server that is not being moved.
	 * refnum == -1 has been dealt with above so window is defined. -Sol
	 */

	flag = 1;
	while ((ptr = traverse_all_windows(&flag)) != (Window *) 0)
		if ((ptr != window) && (!ptr->server_group || (ptr->server_group != window->server_group)) && (ptr->server == old_serv))
		{
			/* Possible relocation -Sol */
			new_win = ptr;

			/* Immediately retain window if no group -Sol */
			if (!ptr->server_group)
				break;
		}

	/* No relocation : we're closing last windows for old_serv -Sol */
	if (!new_win)
	{
		window_set_server(refnum, server, misc | WIN_ALL);
		return;
	}

	/*
	 * Now that we know that server still has at least one window open,
	 * move what we're supposed to -Sol
	 */

	if ((misc & WIN_TRANSFER) && (old_serv >= 0))
		for (tmp = server_list[old_serv].chan_list; tmp; tmp = tmp->next)
			if ((tmp->window == window) || (window->server_group && (tmp->window->server_group == window->server_group)))
			{
				/* Found a channel to be relocated -Sol */
				if (tmp->window->sticky || (misc & WIN_FORCE))
				{	/* This channel moves -Sol */
					int	old = from_server;

					if (!moved)
					{
						moved++;
						clear_channel_list(server);
					}
/**************************** Patched by Flier ******************************/
					/*add_channel(tmp->channel, server, CHAN_LIMBO, tmp);*/ /* Copy it -Sol */
					add_channel(tmp->channel, server, CHAN_LIMBO, tmp, NULL, 0);
/****************************************************************************/
					from_server = old_serv; /* On old_serv,
								   leave it
								   -Sol */
					send_to_server("PART %s", tmp->channel);
					from_server = old;
					remove_channel(tmp->channel, old_serv);
				}
				else
					tmp->window = new_win;
			}

	flag = 1;
	while ((ptr = traverse_all_windows(&flag)) != (Window *) 0)
		if ((ptr == window) || (ptr->server_group && (ptr->server_group == window->server_group)))
		{
			ptr->server = server;
			if (ptr->current_channel)
				new_free(&ptr->current_channel);
		}
		window_check_servers();
}

/*
 * set_channel_by_refnum: This sets the current channel for the current
 * window. It returns the current channel as it's value.  If channel is null,
 * the * current channel is not changed, but simply reported by the function
 * result.  This treats as a special case setting the current channel to
 * channel "0".  This frees the current_channel for the
 * current_screen->current_window, * setting it to null 
 */
char *
set_channel_by_refnum (unsigned int refnum, char *channel)
{
	Window	*tmp;
	Window *tmp2;
	Win_Trav stuff;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (channel && strcmp(channel, irczero) == 0)
		channel = (char *) 0;

	stuff.flag = 1;
	while ((tmp2 = window_traverse(&stuff)))
		if (tmp2->server == tmp->server && my_stricmp(tmp2->current_channel, channel) == 0)
			new_free(&tmp2->current_channel);

	malloc_strcpy(&tmp->current_channel, channel);
	tmp->update |= UPDATE_STATUS;
	set_channel_window(tmp, channel, tmp->server);
        Trace(SZ_TRACE_WINDOW, "set channel %s", (char *)channel, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        TraceWindowInfo(2, tmp);
	return (channel);
}

/* get_channel_by_refnum: returns the current channel for window refnum */
char	*
get_channel_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	return (tmp->current_channel);
}

/* current_refnum: returns the reference number for the current window */
unsigned int 
current_refnum (void)
{
	return (curr_scr_win->refnum);
}

/* query_nick: Returns the query nick for the current channel */
char *
query_nick (void)
{
	return (curr_scr_win->query_nick);
}

/* get_prompt_by_refnum: returns the prompt for the given window refnum */
char	*
get_prompt_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (tmp->prompt)
		return (tmp->prompt);
	else
		return (empty_string);
}

/*
 * get_target_by_refnum: returns the target for the window with the given
 * refnum (or for the current window).  The target is either the query nick
 * or current channel for the window 
 */
char	*
get_target_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (tmp->query_nick)
		return (tmp->query_nick);
	else if (tmp->current_channel)
		return (tmp->current_channel);
	else
		return ((char *) 0);
}

/* set_query_nick: sets the query nick for the current channel to nick */
void 
set_query_nick (char *nick)
{
	char	*ptr;
	NickList *tmp;

	if (curr_scr_win->query_nick)
	{
 		char	*oldnick;

		oldnick = curr_scr_win->query_nick;
		while (oldnick)
		{
			if ((ptr = (char *) index(oldnick,',')) != NULL)
				*(ptr++) = '\0';
			if ((tmp = (NickList *) remove_from_list((List **) &(curr_scr_win->nicks), oldnick)) != NULL)
			{
				new_free(&tmp->nick);
/**************************** PATCHED by Flier ******************************/
				new_free(&tmp->userhost);
/****************************************************************************/
				new_free(&tmp);
			}
			oldnick = ptr;
		}
		new_free(&curr_scr_win->query_nick);
	}
	if (nick)
	{
		malloc_strcpy(&(curr_scr_win->query_nick), nick);
		curr_scr_win->update |= UPDATE_STATUS;
		while (nick)
		{
			if ((ptr = (char *) index(nick,',')) != NULL)
				*(ptr++) = '\0';
			tmp = (NickList *) new_malloc(sizeof(NickList));
			tmp->nick = (char *) 0;
/**************************** PATCHED by Flier ******************************/
			tmp->userhost=(char *) 0;
/****************************************************************************/
			malloc_strcpy(&tmp->nick, nick);
			add_to_list((List **) &(curr_scr_win->nicks), (List *) tmp);
			nick = ptr;
		}
	}
	update_window_status(curr_scr_win,0);
}

/*
 * irc_goto_window: This will switch the current window to the window numbered
 * "which", where which is 0 through the number of visible windows on the
 * screen.  The which has nothing to do with the windows refnum. 
 */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void 
irc_goto_window (int which)
{
	Window	*tmp;
	int	i;


	if (which == 0)
		return;
	if ((which < 0) || (which > current_screen->visible_windows))
	{
		say("GOTO: Illegal value");
		return;
	}
	tmp = current_screen->window_list;
	for (i = 1; tmp && (i != which); tmp = tmp->next, i++)
		;
	set_current_window(tmp);
}

/*
 * hide_window: sets the given window to invisible and recalculates remaing
 * windows to fill the entire screen 
 */
static void
hide_window(window)
	Window	*window;
{
	if (current_screen->visible_windows == 1)
	{
		say("You can't hide the last window.");
		return;
	}
	if (window->visible)
	{
		remove_from_window_list(window);
		add_to_invisible_list(window);
#ifdef SCROLL_AFTER_DISPLAY
/**************************** PATCHED by Flier ******************************/
/*#ifdef SCROLL_AFTER_DISPLAY
		window->display_size = current_screen->li - 3;
#else
		window->display_size = current_screen->li - 2;*/
		window->display_size = current_screen->li - 3 - window->double_status;
#else
		window->display_size = current_screen->li - 2 - window->double_status;
/****************************************************************************/
#endif /* SCROLL_AFTER_DISPLAY */
		set_current_window((Window *) 0);
        }
}

/* hide_other_windows: makes all visible windows but the current one hidden */
static void 
hide_other_windows (void)
{
	Window	*tmp,
		*cur,
		*next;

	cur = curr_scr_win;
	tmp = current_screen->window_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != cur)
			hide_window(tmp);
		tmp = next;
	}
}

#define WIN_FORM "%%-4s %%-%u.%us %%-%u.%us  %%-%u.%us %%-9.9s %%-10.10s %%s%%s"
static	void
list_a_window(window, len, clen)
	Window	*window;
	int	len;
	int	clen;
{
	char	tmp[10];
	char	buffer[BIG_BUFFER_SIZE+1];

	snprintf(tmp, sizeof tmp, "%-4u", window->refnum);
	snprintf(buffer, sizeof buffer, WIN_FORM, 9, 9, len,	/* XXX: 9 is old NICKNAME_LEN */
			len, clen, clen);
/**************************** PATCHED by Flier ******************************/
	/*say(buffer, tmp, get_server_nickname(window->server),
			window->name?window->name:"<None>",
			window->current_channel ?
				window->current_channel : "<None>",
			window->query_nick ? window->query_nick : "<None>",
			window->server != -1 ? get_server_itsname(window->server) : "<None>",
			bits_to_lastlog_level(window->window_level),
			(window->visible) ? "" : " Hidden");*/
        say(buffer, tmp, get_server_nickname(window->server),
			window->name?window->name:"None",
			window->current_channel ?
				window->current_channel : "None",
                        window->query_nick ? window->query_nick : "None",
                        window->server != -1 ? get_server_itsname(window->server) : "None",
			bits_to_lastlog_level(window->window_level),
			(window->visible) ? "" : " Hidden");
/****************************************************************************/
}

/*
 * list_windows: This Gives a terse list of all the windows, visible or not,
 * by displaying their refnums, current channel, and current nick 
 */
static void 
list_windows (void)
{
	Window	*tmp;
	char	buffer[BIG_BUFFER_SIZE+1];
	int	flag = 1;
	int	len = 4;
	int	clen = get_int_var(CHANNEL_NAME_WIDTH_VAR);
	int	check_clen = clen == 0;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp->name && ((int) strlen(tmp->name) > len))
			len = strlen(tmp->name);
		if (check_clen == 0)
			continue;
		if (tmp->current_channel && ((int) strlen(tmp->current_channel) > clen))
			clen = strlen(tmp->current_channel);
	}
	snprintf(buffer, sizeof buffer, WIN_FORM, 9, 9, len, len,	/* XXX: 9 is old NICKNAME_LEN */
		clen, clen);
	say(buffer, "Ref", "Nick", "Name", "Channel", "Query", "Server",
		"Level", empty_string);
	flag = 1;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
		list_a_window(tmp, len, clen);
}

/* show_window: This makes the given window visible.  */
static	void
show_window(window)
	Window	*window;
{
	if (window->visible)
	{
		set_current_window(window);
		return;
	}
	remove_from_invisible_list(window);
	if (add_to_window_list(window))
		set_current_window(window);
	else
		add_to_invisible_list(window);
}

/* push_window_by_refnum:  This pushes the given refnum onto the window stack */
#ifndef LITE
static	void
push_window_by_refnum(refnum)
	u_int	refnum;
{
	WindowStack *new;

	new = (WindowStack *) new_malloc(sizeof(WindowStack));
	new->refnum = refnum;
	new->next = current_screen->window_stack;
	current_screen->window_stack = new;
}

/*
 * pop_window: this pops the top entry off the window stack and sets the
 * current window to that window.  If the top of the window stack is no
 * longer a valid window, then that entry is discarded and the next entry
 * down is tried (as so on).  If the stack is empty, the current window is
 * left unchanged 
 */
static void 
pop_window (void)
{
	int	refnum;
	WindowStack *tmp;
	Window	*win;

	while (1)
	{
		if (current_screen->window_stack)
		{
			refnum = current_screen->window_stack->refnum;
			tmp = current_screen->window_stack->next;
			new_free(&current_screen->window_stack);
			current_screen->window_stack = tmp;
 			if ((win = get_window_by_refnum((u_int)refnum)) != NULL)
			{
				if (!win->visible)
					show_window(win);
				else
					set_current_window(win);
				break;
			}
		}
		else
		{
			say("The window stack is empty!");
			break;
		}
	}
}

/*
 * show_stack: displays the current window stack.  This also purges out of
 * the stack any window refnums that are no longer valid 
 */
static void 
show_stack (void)
{
	WindowStack *last = (WindowStack *) 0,
	    *tmp, *crap;
	Window	*win;
	int	flag = 1;
	int	len = 4;
	int	clen = get_int_var(CHANNEL_NAME_WIDTH_VAR);
	int	check_clen = clen == 0;

	while ((win = traverse_all_windows(&flag)) != NULL)
	{
		if (win->name && ((int) strlen(win->name) > len))
			len = strlen(win->name);
		if (check_clen == 0)
			continue;
		if (win->current_channel && ((int) strlen(win->current_channel) > clen))
			clen = strlen(win->current_channel);
	}
	say("Window stack:");
	tmp = current_screen->window_stack;
	while (tmp)
	{
		if ((win = get_window_by_refnum(tmp->refnum)) != NULL)
		{
			list_a_window(win, len, clen);
			tmp = tmp->next;
		}
		else
		{
			crap = tmp->next;
			new_free(&tmp);
			if (last)
				last->next = crap;
			else
				current_screen->window_stack = crap;
			tmp = crap;
		}
	}
}
#endif

/*
 * is_window_name_unique: checks the given name vs the names of all the
 * windows and returns true if the given name is unique, false otherwise 
 */
static int 
is_window_name_unique (char *name)
{
	Window	*tmp;
	int	flag = 1;

	if (name)
	{
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->name && (my_stricmp(tmp->name, name) == 0))
				return (0);
		}
	}
	return (1);
}

/*
 * add_nicks_by_refnum:  This adds the given str to the nicklist of the
 * window refnum.  Only unique names are added to the list.  If add is zero
 * or the name is preceeded by a ^ it is removed from the list.  The str
 * may be a comma separated list of nicks, channels, etc .
 */
static	void
add_nicks_by_refnum(refnum, str, add)
	u_int	refnum;
	char	*str;
	int	add;
{
	Window	*tmp;
	char	*ptr;
	NickList *new;

	if ((tmp = get_window_by_refnum(refnum)) != NULL)
	{
		while (str)
		{
			if ((ptr = (char *) index(str, ',')) != NULL)
				*(ptr++) = '\0';
			if (add == 0 || *str == '^')
			{
 				if (add == 0 && *str == '^')
 					str++;
 				if ((new = (NickList *) remove_from_list((List **) &(tmp->nicks), str)) != NULL)
				{
					say("%s removed from window name list", new->nick);
					new_free(&new->nick);
/**************************** PATCHED by Flier ******************************/
					new_free(&new->userhost);
/****************************************************************************/
					new_free(&new);
				}
				else
 					say("%s is not on the list for this window!", str);
			}
			else
			{
				if (!find_in_list((List **) &(tmp->nicks), str, !USE_WILDCARDS))
				{
/**************************** PATCHED by Flier ******************************/
					/*say("%s add to window name list", str);*/
					say("%s added to window name list", str);
/****************************************************************************/
					new = (NickList *) new_malloc(sizeof(NickList));
					new->nick = (char *) 0;
					malloc_strcpy(&new->nick, str);
/**************************** PATCHED by Flier ******************************/
			                new->userhost = NULL;
/****************************************************************************/
					add_to_list((List **) &(tmp->nicks), (List *) new);
				}
				else
					say("%s already on window name list", str);
			}
			str = ptr;
		}
	}
	else
		say("No such window!");
}

/* below is stuff used for parsing of WINDOW command */

/*
 * get_window_by_name: returns a pointer to a window with a matching logical
 * name or null if no window matches 
 */
Window	*
get_window_by_name(name)
	char	*name;
{
	Window	*tmp;
	int	flag = 1;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp->name && (my_stricmp(tmp->name, name) == 0))
			return (tmp);
	}
	return ((Window *) 0);
}

/*
 * get_window: this parses out any window (visible or not) and returns a
 * pointer to it 
 */
static	Window	*
get_window(name, args)
	char	*name;
	char	**args;
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(*args, args)) != NULL)
	{
		if (is_number(arg))
		{
 			if ((tmp = get_window_by_refnum((u_int)atoi(arg))) != NULL)
				return (tmp);
		}
		if ((tmp = get_window_by_name(arg)) != NULL)
			return (tmp);
		say("%s: No such window: %s", name, arg);
	}
	else
		say("%s: Please specify a window refnum or name", name);
	return ((Window *) 0);
}

/*
 * get_invisible_window: parses out an invisible window by reference number.
 * Returns the pointer to the window, or null.  The args can also be "LAST"
 * indicating the top of the invisible window list (and thus the last window
 * made invisible) 
 */
static	Window	*
get_invisible_window(name, args)
	char	*name;
	char	**args;
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(*args, args)) != NULL)
	{
		if (my_strnicmp(arg, "LAST", strlen(arg)) == 0)
		{
			if (invisible_list == (Window *) 0)
				say("%s: There are no hidden windows", name);
			return (invisible_list);
		}
		if ((tmp = get_window(name, &arg)) != NULL)
		{
			if (!tmp->visible)
				return (tmp);
			else
			{
				if (tmp->name)
					say("%s: Window %s is not hidden!",
						name, tmp->name);
				else
					say("%s: Window %d is not hidden!",
						name, tmp->refnum);
			}
		}
	}
	else
		say("%s: Please specify a window refnum or LAST", name);
	return ((Window *) 0);
}

/* get_number: parses out an integer number and returns it */
static int 
get_number (char *name, char **args)
{
	char	*arg;

	if ((arg = next_arg(*args, args)) != NULL)
		return (atoi(arg));
	else
		say("%s: You must specify the number of lines", name);
	return (0);
}

/*
 * get_boolean: parses either ON, OFF, or TOGGLE and sets the var
 * accordingly.  Returns 0 if all went well, -1 if a bogus or missing value
 * was specified 
 */
static int 
get_boolean (char *name, char **args, int *var)
{
	char	*arg;

	if (((arg = next_arg(*args, args)) == (char *) 0) ||
	    do_boolean(arg, var))
	{
		say("Value for %s must be ON, OFF, or TOGGLE", name);
		return (-1);
	}
	else
	{
		say("Window %s is %s", name, var_settings[*var]);
		return (0);
	}
}

/*ARGSUSED*/
void 
windowcmd (char *command, char *args, char *subargs)
{
 	size_t	len;
	char	*arg,
		*cmd = (char *) 0;
	char	buffer[BIG_BUFFER_SIZE+1];
	int	no_args = 1;
	Window	*window,
		*tmp;

	in_window_command = 1;
 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);	/* XXX should remove this */
	window = curr_scr_win;
	while ((arg = next_arg(args, &args)) != NULL)
        {
		no_args = 0;
		len = strlen(arg);
		malloc_strcpy(&cmd, arg);
                upper(cmd);
		if (strncmp("NEW", cmd, len) == 0)
		{
                        Trace(SZ_TRACE_WINDOW, "adding new window", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                        TraceWindowInfo(2, NULL);
			if ((tmp = new_window()) != NULL) {
				window = tmp;
                                Trace(SZ_TRACE_WINDOW, "window added", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                                TraceWindowInfo(2, NULL);
                        }
		}
#ifdef WINDOW_CREATE
		else if (strncmp("CREATE", cmd, len) == 0)
		{
			if ((tmp = create_additional_screen()) != NULL)
				window = tmp;
			else
				say("Cannot create new screen!");
		}
		else if (!strncmp("DELETE", cmd, len))
			kill_screen(current_screen);
#endif /* WINODW_CREATE */
		else if (strncmp("REFNUM", cmd, len) == 0)
		{
			if ((tmp = get_window("REFNUM", &args)) != NULL)
			{
				if (tmp->screen && tmp->screen !=window->screen)
					say("Window in another screen!");
				else if (tmp->visible)
				{ 
					set_current_window(tmp);
					window = tmp;
				}
				else
					say("Window not visible!");
			}
			else
			{
				say("No such window!");
				new_free(&cmd);
				goto out;
			}
		}
		else if (strncmp("KILL", cmd, len) == 0) {
                        Trace(SZ_TRACE_WINDOW, "deleting window", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                        TraceWindowInfo(2, NULL);
			delete_window(window);
                        Trace(SZ_TRACE_WINDOW, "window deleted", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                        TraceWindowInfo(2, NULL);
                }
		else if (strncmp("SHRINK", cmd, len) == 0)
			grow_window(window, -get_number("SHRINK", &args));
		else if (strncmp("GROW", cmd, len) == 0)
/**************************** Patched by Flier ******************************/
			/*grow_window(window, get_number("SHRINK", &args));*/
			grow_window(window, get_number("GROW", &args));
		else if (strncmp("SIZE", cmd, len) == 0)
			size_window(window, get_number("SIZE", &args));
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
		else if (strncmp("SSL", cmd, len) == 0 || strncmp("-SSL", cmd, len) == 0)
			SSLconnect = 1;
#endif
/****************************************************************************/
		else if (strncmp("SCROLL", cmd, len) == 0)
			get_boolean("SCROLL", &args, &(window->scroll));
		else if (strncmp("STICKY", cmd, len) == 0)
			get_boolean("STICKY", &args, &(window->sticky));
		else if (strncmp("LOG", cmd, len) == 0)
		{
			if (get_boolean("LOG", &args, &(window->log)))
				break;
			else
			{
				char	*logfile;
				int	add_ext = 1;

				if ((logfile = window->logfile) != NULL)
					add_ext = 0;
				else if (!(logfile = get_string_var(LOGFILE_VAR)))
					logfile = empty_string;
				if (!add_ext)
					snprintf(buffer, sizeof buffer, "%s", logfile);
				else if (window->current_channel)
					snprintf(buffer, sizeof buffer, "%s.%s", logfile, window->current_channel);
				else if (window->query_nick)
					snprintf(buffer, sizeof buffer, "%s.%s", logfile, window->query_nick);
				else
					snprintf(buffer, sizeof buffer, "%s.Window_%d", logfile, window->refnum);
/**************************** PATCHED by Flier ******************************/
				/*window->log_fp = do_log(window->log, buffer, window->log_fp);*/
				do_log(window->log, buffer, &(window->log_fp));
/****************************************************************************/
				if (window->log_fp == (FILE *) 0)
					window->log = 0;
			}
		}
		else if (strncmp("HOLD_MODE", cmd, len) == 0)
			get_boolean("HOLD_MODE", &args, &(window->hold_mode));
		else if (strncmp("LASTLOG_LEVEL", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				window->lastlog_level = parse_lastlog_level(arg);
				say("Lastlog level is %s",
				  bits_to_lastlog_level(window->lastlog_level));
			}
			else
				say("Level required");
		}
		else if (strncmp("LEVEL", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
/**************************** PATCHED by Flier ******************************/
				/*window->window_level = parse_lastlog_level(arg);*/
                                int new_level = parse_lastlog_level(arg);
                                if (*arg == '+') window->window_level |= new_level;
                                else window->window_level = new_level;
/****************************************************************************/
				say("Window level is %s",
				   bits_to_lastlog_level(window->window_level));
				revamp_window_levels(window);
			}
			else
				say("LEVEL: Level required");
		}
		else if (strncmp("BALANCE", cmd, len) == 0)
			recalculate_windows();
/**************************** PATCHED by Flier ******************************/
/*#if defined(_Windows)*/
/****************************************************************************/
#ifndef LITE
		else if (strncmp("TITLE", cmd, len) == 0)
		{
/**************************** PATCHED by Flier ******************************/
			/*if ((arg = next_arg(args, &args)) != NULL)*/
			if ((arg = new_next_arg(args, &args)) != NULL)
/****************************************************************************/
			{
/**************************** PATCHED by Flier ******************************/
#ifdef SZ32
                                SetConsoleTitle(arg);
#elif defined(_Windows)
/****************************************************************************/
				if (window->visible && window->screen)
				{
					SetWindowText(window->screen->hwnd, arg);
				}
				else
				{
					say("The current window is not attached to a screen");
				}
/**************************** PATCHED by Flier ******************************/
#else
                                fprintf(stderr, "]0;%s", arg);
                                fflush(stderr);
#endif
/****************************************************************************/
			}
			else
			{
				say("You must specify a name for the window!");
			}
		}
#endif
/**************************** PATCHED by Flier ******************************/
/*#endif*/ /* _Windows */
/****************************************************************************/
		else if (strncmp("NAME", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				if (is_window_name_unique(arg))
				{
					malloc_strcpy(&(window->name), arg);
					window->update |= UPDATE_STATUS;
				}
				else
					say("%s is not unique!", arg);
			}
			else
				say("You must specify a name for the window!");
		}
/**************************** PATCHED by Flier ******************************/
                else if (strncmp("UNNAME", cmd, len) == 0)
                {
                    new_free(&(window->name));
                    update_window_status(window, 0);
                }
/****************************************************************************/
		else if (strncmp("PROMPT", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				malloc_strcpy(&(window->prompt), arg);
				window->update |= UPDATE_STATUS;
			}
			else
			    say("You must specify a prompt for the window!");
		}
		else if (strncmp("GOTO", cmd, len) == 0)
		{
			irc_goto_window(get_number("GOTO", &args));
			window = curr_scr_win;
		}
		else if (strncmp("LAST", cmd, len) == 0)
			set_current_window((Window *) 0);
		else if (strncmp("MOVE", cmd, len) == 0)
		{
			move_window(window, get_number("MOVE", &args));
			window = curr_scr_win;
		}
		else if (strncmp("SWAP", cmd, len) == 0)
		{
			if ((tmp = get_invisible_window("SWAP", &args)) != NULL)
				swap_window(window, tmp);
		}
                else if (strncmp("HIDE", cmd, len) == 0)
			hide_window(window);
#ifndef LITE
		else if (strncmp("PUSH", cmd, len) == 0)
			push_window_by_refnum(window->refnum);
		else if (strncmp("POP", cmd, len) == 0)
			pop_window();
#endif
		else if (strncmp("ADD", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
				add_nicks_by_refnum(window->refnum, arg, 1);
			else
				say("ADD: Do something!  Geez!");
		}
		else if (strncmp("REMOVE", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
				add_nicks_by_refnum(window->refnum, arg, 0);
			else
				say("REMOVE: Do something!  Geez!");
		}
#ifndef LITE
		else if (strncmp("STACK", cmd, len) == 0)
			show_stack();
#endif
		else if (strncmp("LIST", cmd, len) == 0)
			list_windows();
		else if (strncmp("SERVER", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL) {
				window_get_connected(window, arg, -1, args);
            }
			else
				say("SERVER: You must specify a server");
		}
		else if (strncmp("SHOW", cmd, len) == 0)
		{
			if ((tmp = get_window("SHOW", &args)) != NULL)
			{
				show_window(tmp);
				window = curr_scr_win;
			}
		}
		else if (strncmp("HIDE_OTHERS", cmd, len) == 0)
			hide_other_windows();
		else if (strncmp("KILL_OTHERS", cmd, len) == 0)
			delete_other_windows();
		else if (strncmp("NOTIFY", cmd, len) == 0)
		{
			window->miscflags ^= WINDOW_NOTIFY;
			say("Notification when hidden set to %s",
			    (window->miscflags & WINDOW_NOTIFY)? "ON" : "OFF");
		}
		else if (strncmp("WHERE", cmd, len) == 0)
			win_list_channels(curr_scr_win);
 		else if (strncmp("QUERY", cmd, len) == 0)
 		{
 			char *a = 0;

 			a = next_arg(args, &args);
 			query(cmd, a, 0);
 		}
		else if (strncmp("CHANNEL", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
 				char *key;

				arg = strtok(arg, ",");
 				if ((key = strtok(0, ", ")) == 0)
 					key = empty_string;

				if (is_bound(arg, window->server))
				{
					say("Channel %s is bound", arg);
				}
				else
				{
 					if (is_on_channel(arg, window->server,
 						get_server_nickname(window->server)))
 					{
 						is_current_channel(arg, window->server,
 							(int)window->refnum);
 						say("You are now talking to channel %s", arg);
 						set_channel_by_refnum(0, arg);
 					}
 					else if (*arg == '0' && !*(args + 1))
 						set_channel_by_refnum(0, NULL);
 					else
 					{
 						int	server;

 						server = from_server;
 						from_server = window->server;
						switch (get_server_version(window->server)) {
						case Server2_5:
							send_to_server("CHANNEL %s%s%s", arg, key ? " " : empty_string, key);
							break;
						default:
							send_to_server("JOIN %s%s%s", arg, key ? " " : empty_string, key);
						}
/**************************** PATCHED by Flier ******************************/
                                                /* from_server can be -1 here
 						add_channel(arg, from_server, CHAN_JOINING, (ChannelList *) 0,);*/
                                                if (from_server >= 0)
                                                    add_channel(arg, from_server, CHAN_JOINING, NULL, key, 0);
                                                Trace(SZ_TRACE_WINDOW, "joining channel %s", (char *)arg, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                                                TraceWindowInfo(2, window);
/****************************************************************************/
 						from_server = server;
 					}
				}
			}
			else
				set_channel_by_refnum(0, irczero);
		}
		else if (strncmp("PREVIOUS", cmd, len) == 0)
		{
			swap_previous_window(0, (char *) 0);
		}
		else if (strncmp("NEXT", cmd, len) == 0)
		{
			swap_next_window(0, (char *) 0);
		}
		else if (strncmp("BACK", cmd, len) == 0)
		{
			back_window(0, (char *) 0);
		}
		else if (strncmp("KILLSWAP", cmd, len) == 0)
		{
			window_kill_swap();
		}
		else if (strncmp("LOGFILE", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				malloc_strcpy(&window->logfile, arg);
				say("Window LOGFILE set to %s", arg);
			}
			else
				say("No LOGFILE given");
		}
		else if (strncmp("NOTIFY_LEVEL", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				window->notify_level = parse_lastlog_level(arg);
				say("Window notify level is %s",
				   bits_to_lastlog_level(window->notify_level));
			}
			else
				say("Level missing");
		}
		else if (strncmp("NUMBER", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
			{
				int	i;
 				Window	*wtmp;

				i = atoi(arg);
				if (i > 0)
				{
					/* check if window number exists */

 					wtmp = get_window_by_refnum((u_int)i);
 					if (!wtmp)
						window->refnum = i;
					else
					{
						wtmp->refnum = window->refnum;
						window->refnum = i;
					}
					update_all_status();
				}
				else
					say("Window number must be greater than 1");
			}
			else
				say("Window number missing");
		}
		else if (strncmp("BIND", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
			{
				if (!is_channel(arg))
					say("BIND: %s is not a valid channel name", arg);
				else
				{
/**************************** PATCHED by Flier ******************************/
                                    char *origarg = arg;

                                    while ((arg = strtok(origarg, ","))) {
                                        origarg = (char *) 0;
/****************************************************************************/
					bind_channel(arg, window);
/**************************** PATCHED by Flier ******************************/
                                    }
/****************************************************************************/
				}
			}
			else
/**************************** PATCHED by Flier ******************************/
                        {
				/*if (window->bound_channel)
					say("Channel %s is bound to window %d", window->bound_channel, window->refnum);*/
                            int count = 0;
                            char *tmpbuf = NULL;
                            struct channels *chan;

                            for (chan = window->bound_chans; chan; chan = chan->next){
                                malloc_strcat(&tmpbuf, chan->channel);
                                if (chan->next) malloc_strcat(&tmpbuf, ",");
                                count++;
                            }
                            if (count) {
                                say("Channel%s %s %s bound to window %d",
                                    count == 1 ? "" : "s", tmpbuf, count == 1 ? "is" : "are",
                                    window->refnum);
                                new_free(&tmpbuf);
                            }
                        }
/****************************************************************************/
		}
		else if (strncmp("UNBIND", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
			{
				if (is_bound(arg, window->server))
				{
					say("Channel %s is no longer bound", arg);
					unbind_channel(arg, window);
				}
				else
					say("Channel %s is not bound", arg);
			}
			else
				say("UNBIND: You must specify a channel name");
		}
		else if (strncmp("ADDGROUP", cmd, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
				add_window_to_server_group(window, arg);
			else
				say("WINDOW ADDGROUP requires a group name");
		}
		else if (strncmp("DELGROUP", cmd, len) == 0)
		{
			window->server_group = 0;
			say("Window no longer has a server group");
			update_window_status(window, 1);
                        Trace(SZ_TRACE_WINDOW, "window %d unset server group", (char *)window->refnum, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                        TraceWindowInfo(2, window);
		}
		else if (strncmp("DOUBLE", cmd, len) == 0)
		{
/**************************** PATCHED by Flier ******************************/
			/*int current = window->double_status;

			if (get_boolean("DOUBLE", &args, &(window->double_status)) == 0)
			{
				window->display_size += current -
					window->double_status;
				recalculate_window_positions();
				update_all_windows();
				build_status((char *) NULL);
			}*/
                        if ((arg = next_arg(args, &args))) {
                            int errcmd = -1;
                            int current = window->double_status;
    
                            upper(arg);
                            if (!strcmp(arg, "ON")) {
                                errcmd = 0;
                                window->double_status = 1;
                            }
                            else if (!strcmp(arg, "OFF")) {
                                errcmd = 0;
                                window->double_status = 0;
                            }
                            else if (!strcmp(arg, "TOGGLE")) {
                                errcmd = 0;
                                if (window->double_status == 1) window->double_status = 0;
                                else if (window->double_status == 0) window->double_status = 1;
                                else errcmd = -1;
                            }
                            else {
                                int i;

                                i = atoi(arg);
                                if (i > 1 && i < 4) {
                                    errcmd = 0;
                                    window->double_status = i - 1;
                                }
                            }
                            if (errcmd == -1) say("Value for DOUBLE must be ON, OFF, TOGGLE, 2 or 3");
                            else {
                                window->display_size += current-window->double_status;
                                recalculate_window_positions();
                                update_all_windows();
                                build_status(NULL);
                            }
                        }
/****************************************************************************/
		}
		else if (strncmp("NOSTATUS", cmd, len) == 0)
		{
			int current = window->double_status;

			window->double_status = -1;
			window->display_size += current - window->double_status;
			recalculate_window_positions();
			update_all_windows();
			build_status((u_char *) NULL);
		}
		else
			say("Unknown WINDOW command: %s", arg);
		new_free(&cmd);
	}
	if (no_args)
	{
		if (window->name)
			say("Window %s (%u)", window->name, window->refnum);
		else
			say("Window %u", window->refnum);
		if (window->server == -1)
			say("\tServer: <None>");
		else
		{
                    char tmpbuf[mybufsize/8];
                    char *group;

/**************************** PATCHED by Flier ******************************/
 			/*say("\tServer: %s", get_server_name(window->server));*/
                    group = get_server_group(window->server);
                    if (group) snprintf(tmpbuf, sizeof(tmpbuf), " [%s]", group);
                    else *tmpbuf = '\0';
                    say("\tServer: %s:%d%s", get_server_name(window->server),
                        get_server_port(window->server), tmpbuf);
/****************************************************************************/
		}
		say("\tCurrent channel: %s", window->current_channel ?  window->current_channel : "<None>");
		say("\tQuery User: %s", (window->query_nick ?  window->query_nick : "<None>"));
		say("\tPrompt: %s", window->prompt ?  window->prompt : "<None>");
/**************************** PATCHED by Flier ******************************/
		/*say("\tSecond status line is %s", var_settings[window->double_status]);*/
                say("\tNumber of status lines: %d",1+window->double_status);
/****************************************************************************/
		say("\tScrolling is %s", var_settings[window->scroll]);
		say("\tLogging is %s", var_settings[window->log]);
		if (window->logfile)
			say("\tLogfile is %s", window->logfile);
		else
			say("\tNo logfile given");
		say("\tNotification is %s", var_settings[window->miscflags & WINDOW_NOTIFY]);
		say("\tHold mode is %s", var_settings[window->hold_mode]);
		say("\tSticky behaviour is %s", var_settings[window->sticky]);
		say("\tWindow level is %s", bits_to_lastlog_level(window->window_level));
		say("\tLastlog level is %s", bits_to_lastlog_level(window->lastlog_level));
		say("\tNotify level is %s", bits_to_lastlog_level(window->notify_level));
		if (window->server_group)
			say("\tServer Group is (%d) %s", window->server_group, find_server_group_name(window->server_group));
/**************************** PATCHED by Flier ******************************/
		/*if (window->bound_channel)
			say("\tBound Channel is %s", window->bound_channel);*/
		if (window->bound_chans) {
                    int count=0;
                    char *tmpbuf=(char *) 0;
                    struct channels *chan;

                    for (chan=window->bound_chans;chan;chan=chan->next) {
                        malloc_strcat(&tmpbuf,chan->channel);
                        if (chan->next) malloc_strcat(&tmpbuf,",");
                        count++;
                    }
                    say("\tBound Channel%s %s %s",
                         count==1?"":"s",count==1?"is":"are",tmpbuf);
                    new_free(&tmpbuf);
                }
/****************************************************************************/
		if (window->nicks)
		{
 			NickList *ntmp;

			say("\tName list:");
			for (ntmp = window->nicks; ntmp; ntmp = ntmp->next)
				say("\t  %s", ntmp->nick);
		}
	}
out:
 	restore_message_from();
	in_window_command = 0;
	update_all_windows();
        cursor_to_input();
/**************************** Patched by Flier ******************************/
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
        SSLconnect = 0;
#endif
/****************************************************************************/
}

int 
number_of_windows (void)
{
	return (current_screen->visible_windows);
}

void
unstop_all_windows(key, ptr)
 	u_int	key;
	char *	ptr;
{
	Window	*tmp;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		hold_mode(tmp, OFF, 1);
}

/* this will make underline toggle between 2 and -1 and never let it get to 0 */
void 
set_underline_video (int value)
{
	if (value == OFF)
		underline = -1;
	else
		underline = 1;
}

void
window_get_connected(window, arg, narg, args)
	Window	*window;
	char	*arg;
	int	narg;
	char	*args;
{
	int	i,
		port_num,
		new_server_flags = WIN_TRANSFER;
	char	*port,
		*password = NULL,
		*nick = NULL,
		*group = NULL;

	if (arg)
	{
		if (*arg == '=')
		{
			new_server_flags |= WIN_ALL;
			arg++;
		}
		else if (*arg == '~')
		{
			new_server_flags |= WIN_FORCE;
			arg++;
		}
		parse_server_info(&arg, &port, &password, &nick, &group);
		if (port)
		{
			port_num = atoi(port);
			if (!port_num)
				port_num = -1;
		}
		else
			port_num = -1;
		/* relies on parse_server_info putting a null in */
		/* This comes first for "/serv +1" -Sol */
		if ((i = parse_server_index(arg)) != -1)
		{
			if (port_num == -1) /* Could be "/serv +1:6664" -Sol */
				port_num = server_list[i].port;
			if (nick == NULL)
				nick = server_list[i].nickname;
		}
		else if ((i = find_in_server_list(arg, port_num, nick)) != -1)
			port_num = server_list[i].port;
	}
	else
	{
		i = narg;
		port_num = server_list[i].port;
		arg = server_list[i].name;
	}

	if (!(new_server_flags & WIN_ALL))
	{	/* Test if last window -Sol */
		int	flag = 1;
		Window	*ptr, *new_win = (Window *) 0;

		while ((ptr = traverse_all_windows(&flag)) != (Window *) 0)
			if ((ptr != window) && (!ptr->server_group || (ptr->server_group != window->server_group)) && (ptr->server == window->server))
			{
				new_win = ptr;
				break;
			}
		if (!new_win)
			new_server_flags |= WIN_ALL;
	}

	if (-1 == i)
	{
		if (nick && *nick)
			malloc_strcpy(&connect_next_nick, nick);
		if (password && *password)
			malloc_strcpy(&connect_next_password, password);
	}
	else
	{
		if (nick && *nick)
			set_server_nickname(i, nick);
		if (password && *password)
			set_server_password(i, password);

		if (((i = find_in_server_list(get_server_name(i), port_num, nick)) != -1) && is_server_connected(i))
			new_server_flags &= ~WIN_TRANSFER;

		arg = get_server_name(i);
		port_num = server_list[i].port;
	}

	if (!connect_to_server(arg, port_num, nick, group, (new_server_flags & WIN_ALL) ? window->server : -1))
	{
		window_set_server((int)window->refnum, from_server, new_server_flags);
		update_all_status();
	}
	window_check_servers();
/**************************** PATCHED by Flier ******************************/
        Trace(SZ_TRACE_WINDOW, "connecting to server", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        TraceWindowInfo(2, window);
        if (curr_scr_win->server >= 0)
            is_current_channel(NULL, curr_scr_win->server, 0);
/****************************************************************************/
}

static	void
win_list_channels(window)
	Window	*window;
{
	ChannelList *tmp;
	int	print_one = 0;

	if (!window)
		return;

	for (tmp = server_list[window->server].chan_list; tmp; tmp = tmp->next)
		if (tmp->window == window)
		{
			if (!print_one)
			{
				say("Channels currently in this window :");
				print_one++;
			}
			say("\t%s", tmp->channel);
		}
	if (!print_one)
		say("There are no channels in this window");
}

/**************************** PATCHED by Flier ******************************/
/* Try to find given window in window list */
Window *FindWindowByPtr(wind)
Window *wind;
{
    Window *tmp;

    if (!wind) return((Window *) 0);
    for (tmp=curr_scr_win->screen->window_list;tmp;tmp=tmp->next)
        if (tmp->server==wind->server && tmp==wind) return(tmp);
    for (tmp=invisible_list;tmp;tmp=tmp->next)
        if (tmp->server==wind->server && tmp==wind) break;
    return(tmp);
}

/* Clean up all memory related to window */
void 
CleanUpWindows (void) {
    Screen *tmpscreen;
    Screen *tmpscreenfree;
    Window *tmpwindow;
    Window *tmpwindowfree;

    for (tmpscreen=screen_list;tmpscreen;) {
        tmpscreenfree=tmpscreen;
        tmpscreen=tmpscreen->next;
        for (tmpwindow=tmpscreenfree->window_list;tmpwindow;) {
            tmpwindowfree=tmpwindow;
            tmpwindow=tmpwindow->next;
            cleanup_window(tmpwindowfree);
        }
        new_free(&(tmpscreenfree->redirect_name));
        new_free(&(tmpscreenfree->redirect_token));
        new_free(&(tmpscreenfree->tty_name));
        new_free(&tmpscreenfree);
    }
    for (tmpwindow=invisible_list;tmpwindow;) {
        tmpwindowfree=tmpwindow;
        tmpwindow=tmpwindow->next;
        cleanup_window(tmpwindowfree);
    }
}
/****************************************************************************/
