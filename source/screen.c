/*
 * screen.c
 *
 * Written By Matthew Green, based on portions of window.c
 * by Michael Sandrof.
 *
 * Copyright (c) 1990 Michael Sandrof.
 * Copyright (c) 1993-1998 Matthew R. Green.
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
 * $Id: screen.c,v 1.3 1998-10-07 20:01:30 f Exp $
 */

#include "irc.h"

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif /* HAVE_SYS_UN_H */

#include "screen.h"
#include "menu.h"
#include "window.h"
#include "output.h"
#include "vars.h"
#include "server.h"
#include "list.h"
#include "ircterm.h"
#include "names.h"
#include "ircaux.h"
#include "input.h"
#include "log.h"
#include "hook.h"
#include "dcc.h"
#include "translat.h"
#include "exec.h"
#include "newio.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

#ifdef WANTANSI
extern  int  CountAnsi _((char *, int));
extern  int  vt100Decode _((char));
extern  void FixColorAnsi _((char *));
extern  void ConvertmIRC _((char *, char *));
#endif
extern  void StripAnsi _((char *, char *, int));
/****************************************************************************/

#ifdef lines
#undef lines
#undef columns
#endif

	Window	*to_window;
	Screen	*current_screen;
	Screen	*main_screen;
	Screen	*last_input_screen;

extern	int	in_help;	/* Used to suppress holding of help text */
extern	char	*redirect_nick;

/* Our list of screens */
Screen	*screen_list = NULL;

static	void	display_lastlog_lines _((int, int, Window *));
static	Screen	* create_new_screen _((void));
static	char	** split_up_line _((char *));
static	void	scrollback_backwards_lines _((int));
static	void	scrollback_forwards_lines _((int));
static	char	display_highlight _((int));
static	char	display_bold _((int));
static	void	add_to_window _((Window *, char *));
static	u_int	create_refnum _((void));
static	char	*next_line_back _((Window *, int));

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
void my_addstr(str,len)
char *str;
int len;
{
    if (len) {
#ifdef WANTANSI
        int i,j,fg=7,bg=0,cnt=0;
        static char buf[2*BIG_BUFFER_SIZE+1];

        for (i=0;i<len && str[i];i++)
            if (str[i]!='\033') buf[cnt++]=str[i];
            else {
                i++;
                if (str[i]!='[') {
                    /* buffered output because of lame addch() */
                    buf[cnt++]=str[i];
                    continue;
                }
                /* output buffer before we set new color */
                buf[cnt]='\0';
                addstr(buf);
                cnt=0;
                i++;
                while (i<len && str[i] && str[i]!='m') {
                    j=atoi(&str[i]);
                    if (j>=30 && j<=37) fg=j-30;
                    else if (j>=40 && j<=47) bg=j-40;
                    else if (j==1) attron(A_BOLD);
                    else if (j==5) attron(A_BLINK);
                    else if (j==7) attron(A_REVERSE);
                    else if (j==22) attroff(A_BOLD|A_BLINK|A_REVERSE);
                    else if (j==0) {
                        fg=7;
                        bg=0;
                        attroff(A_BOLD|A_BLINK|A_REVERSE);
                    }
                    while (str[i] && isdigit(str[i])) i++;
                    if (str[i]!='m') while (i<len && str[i] && !isdigit(str[i])) i++;
                }
                attron(COLOR_PAIR((8*bg+fg)));
            }
        if (cnt) {
            buf[cnt]='\0';
            addstr(buf);
        }
#else
        char *c;

        c=new_malloc(len+1);
        strncpy(c,str,len);
        c[len]=0;
        addstr(c);
        new_free(&c);
#endif /* WANTANSI */
    }
}
#endif /* SZNCURSES */
/****************************************************************************/

/*
 * create_new_screen creates a new screen structure. with the help of
 * this structure we maintain ircII windows that cross screen window
 * boundaries.
 */
static	Screen	*
create_new_screen()
{
	Screen	*new = NULL,
		**list;
	static	int	refnumber = 0;

	for (list = &screen_list; *list; list = &((*list)->next))
	{
		if (!(*list)->alive)
		{
			new = *list;
			break;
		}
	}
	if (!new)
	{
		new = (Screen *) malloc(sizeof(Screen));
		new->screennum = ++refnumber;
		new->next = screen_list;
		if (screen_list)
			screen_list->prev = new;
		screen_list = new;
	}
	new->last_window_refnum = 1;
	new->window_list = NULL;
	new->window_list_end = NULL;
	new->cursor_window = NULL;
	new->current_window = NULL;
	new->visible_windows = 0;
	new->window_stack = NULL;
        new->meta1_hit = new->meta2_hit = new->meta3_hit = new->meta4_hit = 0;
/**************************** PATCHED by Flier ******************************/
        new->meta5_hit=0;
/****************************************************************************/
	new->quote_hit = new->digraph_hit = new->inside_menu = 0;
	new->buffer_pos = new->buffer_min_pos = 0;
	new->input_buffer[0] = '\0';
	new->fdout = 1;
	new->fpout = stdout;
	new->fdin = 0;
	new->fpin = stdin;
	new->alive = 1;
	new->promptlist = NULL;
	new->redirect_name = NULL;
	new->redirect_token = NULL;
	new->tty_name = (char *) 0;
	new->li = 24;
	new->co = 79;
	new->redirect_server = -1;
	last_input_screen = new;
	return new;
}

/* 
 * add_wait_prompt:  Given a prompt string, a function to call when
 * the prompt is entered.. some other data to pass to the function,
 * and the type of prompt..  either for a line, or a key, we add 
 * this to the prompt_list for the current screen..  and set the
 * input prompt accordingly.
 */
void
add_wait_prompt(prompt, func, data, type)
	char	*prompt;
	void	(*func) _((char *, char *));
	char	*data;
	int	type;
{
	WaitPrompt **AddLoc,
		   *New;

	New = (WaitPrompt *) new_malloc(sizeof(WaitPrompt));
	New->prompt = NULL;
	malloc_strcpy(&New->prompt, prompt);
	New->data = NULL;
	malloc_strcpy(&New->data, data);
	New->type = type;
	New->func = func;
	New->next = NULL;
	for (AddLoc = &current_screen->promptlist; *AddLoc;
			AddLoc = &(*AddLoc)->next);
	*AddLoc = New;
	if (AddLoc == &current_screen->promptlist)
		change_input_prompt(1);
}

void
set_current_screen(screen)
	Screen	*screen;
{
	if (screen->alive)
		current_screen = screen;
	else
		current_screen = screen_list;
}

/*
 * window_redirect: Setting who to non null will cause IRCII to echo all
 * commands and output from that server (including output generated by IRCII)
 * to who.  Setting who to null disables this 
 */
void
window_redirect(who, server)
	char	*who;
	int	server;
{
	char	buf[BIG_BUFFER_SIZE];

	if (who)
		sprintf(buf, "%04d%s", server, who);
	else
		sprintf(buf, "%04d#LAME", server);
	malloc_strcpy(&current_screen->redirect_token, buf);
	malloc_strcpy(&current_screen->redirect_name, who);
	current_screen->redirect_server = server;
}

int
check_screen_redirect(nick)
	char	*nick;
{
	Screen	*screen,
		*tmp_screen;
	
	for (screen = screen_list; screen; screen = screen->next)
	{
		if (!screen->redirect_token)
			continue;
                if (!strcmp(nick, screen->redirect_token))
		{
			tmp_screen = current_screen;
			current_screen = screen;
			window_redirect(NULL, from_server);
			current_screen = tmp_screen;
			return 1;
		}
	}
	return 0;
}

/* Old screens never die. They just fade away. */

#ifdef WINDOW_CREATE
void
kill_screen(screen)
	Screen	*screen;
{
	Window	*window;

	if (main_screen == screen)
	{
		say("You may not kill the main screen");
		return;
	}
	if (screen->fdin)
	{
		new_close(screen->fdout);
		new_close(screen->fdin);
	}
	while ((window = screen->window_list))
	{
		screen->window_list = window->next;
		add_to_invisible_list(window);
	}

	if (last_input_screen == screen)
		last_input_screen = screen_list;
	screen->alive = 0;
}

int
is_main_screen(screen)
	Screen	*screen;
{
	return (screen == main_screen);
}

#else

/**************************** PATCHED by Flier ******************************/
/*static	int*/
int
/****************************************************************************/
is_main_screen(screen)
	Screen	*screen;
{
	return 1;
}

#endif /* WINDOW_CREATE */


/*
 * scroll_window: Given a pointer to a window, this determines if that window
 * should be scrolled, or the cursor moved to the top of the screen again, or
 * if it should be left alone. 
 */
void
scroll_window(window)
	Window	*window;
{
	if (dumb)
		return;
	if (window->cursor == window->display_size)
	{
		if (window->scroll)
		{
			int	scroll,
			i;

			if ((scroll = get_int_var(SCROLL_LINES_VAR)) <= 0)
				scroll = 1;

			for (i = 0; i < scroll; i++)
			{
				new_free (&window->top_of_display->line);
				window->top_of_display =
					window->top_of_display->next;
			}
			if (window->visible)
			{
				if (term_scroll(window->top + window->menu.lines, window->top + window->menu.lines + window->cursor - 1, scroll))
				{
					if(current_screen->visible_windows == 1)
					{
						int	i;
						Window	*tmp;

			/*
			 * this method of sim-u-scroll seems to work fairly
			 * well. The penalty is that you can only have one
			 * window, and of course you can't use the scrollback
			 * buffer either. Or menus. Actually, this method
			 * really doesn't work very well at all anymore.
			 */
						tmp = current_screen->cursor_window;
						term_move_cursor(0, LI - 2);
						if (term_clear_to_eol())
							term_space_erase(0);
						term_cr();
						term_newline();
						for (i = 0; i < scroll; i++)
						{
							term_cr();
							term_newline();
						}
						update_window_status(window, 1);
						update_input(UPDATE_ALL);
						current_screen->cursor_window = tmp;
					}
					else
						redraw_window(window, 1);
				}
				window->cursor -= scroll;
				term_move_cursor(0, window->cursor + window->top + window->menu.lines);
			}
			else
				window->cursor -= scroll;
		}
		else
		{
			window->cursor = 0;
			if (window->visible)
				term_move_cursor(0, window->top + window->menu.lines);
		}
	}
	else if (window->visible && current_screen->cursor_window == window)
	{
		term_cr();
		term_newline();
	}
	if (window->visible && current_screen->cursor_window)
	{
		if (term_clear_to_eol()) /* && !window->hold_mode && !window->hold_on_next_rite) */
		{
			term_space_erase(0);
			term_cr();
		}
		term_flush();
	}
}

/* display_highlight: turns off and on the display highlight.  */
static	char
display_highlight(flag)
	int	flag;
{
	static	int	highlight = OFF;

	fflush(current_screen->fpout);
	if (flag == highlight)
		return (flag);
	switch (flag)
	{
	case ON:
		highlight = ON;
		if (get_int_var(INVERSE_VIDEO_VAR))
			term_standout_on();
		return (OFF);
	case OFF:
		highlight = OFF;
		if (get_int_var(INVERSE_VIDEO_VAR))
			term_standout_off();
		return (ON);
	case TOGGLE:
		if (highlight == ON)
		{
			highlight = OFF;
			if (get_int_var(INVERSE_VIDEO_VAR))
				term_standout_off();
			return (ON);
		}
		else
		{
			highlight = ON;
			if (get_int_var(INVERSE_VIDEO_VAR))
				term_standout_on();
			return (OFF);
		}
	}
	return flag;
}

/* display_bold: turns off and on the display bolding.  */
static	char
display_bold(flag)
	int	flag;
{
	static	int	bold = OFF;

	fflush(current_screen->fpout);
	if (flag == bold)
		return (flag);
	switch (flag)
	{
	case ON:
		bold = ON;
		if (get_int_var(BOLD_VIDEO_VAR))
			term_bold_on();
		return (OFF);
	case OFF:
		bold = OFF;
		if (get_int_var(BOLD_VIDEO_VAR))
			term_bold_off();
		return (ON);
	case TOGGLE:
		if (bold == ON)
		{
			bold = OFF;
			if (get_int_var(BOLD_VIDEO_VAR))
				term_bold_off();
			return (ON);
		}
		else
		{
			bold = ON;
			if (get_int_var(BOLD_VIDEO_VAR))
				term_bold_on();
			return (OFF);
		}
	}
	return OFF;
}

/*
 * output_line prints the given string at the current screen position,
 * performing adjustments for ^_, ^B, ^V, and ^O
 */
int
output_line(str, result, startpos)
	char	*str;
	char	**result;
	int	startpos;
{
	static	int	high = OFF, bold = OFF;
	int	rev_tog, und_tog, bld_tog, all_off;
	char	*ptr;
	int	len;
	int	written = 0;
	char	c;
	char	*original;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
        int	ansi_count=0;
#endif
/****************************************************************************/

	original = str;
	ptr = str;
	display_highlight(high);
	display_bold(bold);
	/* do processing on the string, handle inverse and bells */
	while (*ptr)
	{
		switch (*ptr)
		{
		case REV_TOG:
		case UND_TOG:
		case BOLD_TOG:
		case ALL_OFF:
/**************************** PATCHED by Flier ******************************/
                    {
/****************************************************************************/
			len = ptr - str;
			written += len;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                        ansi_count=CountAnsi(str,ptr-str);
                        written-=ansi_count;
#endif
/****************************************************************************/
			if (startpos)
			{
				if (ptr - original > startpos)
				{
					str += len - (ptr - original - startpos);
					len = ptr - original - startpos;
					startpos = 0;
				}
			}
			if (written > CO)
				len = len - (written - CO);
			if (!startpos)
/**************************** PATCHED by Flier ******************************/
				/*fwrite(str, len, 1, current_screen->fpout);*/
#ifdef SZNCURSES
                        	my_addstr(str,len);
#else
                                fwrite(str, len, 1, current_screen->fpout);
#endif /* SZNCURSES */
#ifdef WANTANSI
                        ansi_count=0;
#endif
/****************************************************************************/
			rev_tog = und_tog = bld_tog = all_off = 0;
			do
			{
				switch(*ptr)

				{
				case REV_TOG:
					rev_tog = 1 - rev_tog;
					break;
				case UND_TOG:
					und_tog = 1 - und_tog;
					break;
				case BOLD_TOG:
					bld_tog = 1 - bld_tog;
					break;
				case ALL_OFF:
					all_off = 1;
					und_tog = rev_tog = bld_tog = 0;
					break;
				}
			} while ((ptr[1] == REV_TOG || ptr[1] == UND_TOG ||
			    ptr[1] == BOLD_TOG || ptr[1] == ALL_OFF) && ptr++);
			if (all_off)
			{
				if (!underline)
				{
					term_underline_off();
					underline = 1;
				}
				display_highlight(OFF);
				display_bold(OFF);
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
                                attrset(A_NORMAL);
#endif /* SZNCURSES */
/****************************************************************************/
				high = 0;
				bold = 0;
			}
			if (und_tog && get_int_var(UNDERLINE_VIDEO_VAR))
			{
				if ((underline = 1 - underline) != 0)
					term_underline_off();
				else
					term_underline_on();
			}
			if (rev_tog)
			{
				high = display_highlight(TOGGLE);
				high = 1 - high;
			}
			if (bld_tog)
			{
				bold = display_bold(TOGGLE);
				bold = 1 - bold;
			}
			str = ++ptr;
			break;
/**************************** PATCHED by Flier ******************************/
                    }
/****************************************************************************/
		case '\007':
		/*
		 * same as above, except after we display everything
		 * so far, we beep the terminal 
		 */
			c = *ptr;
			*ptr = '\0';
			len = strlen(str);
			written += len;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                        ansi_count=CountAnsi(str,-1);
                        written-=ansi_count;
#endif
/****************************************************************************/
			if (startpos)
			{
				if (ptr - original > startpos)
				{
					str += len - (ptr - original - startpos);
					len = ptr - original - startpos;
					startpos = 0;
				}
			}
			if (written > CO)
				len = len - (written - CO);
			if (!startpos)
/**************************** PATCHED by Flier ******************************/
				/*fwrite(str, len, 1, current_screen->fpout);*/
#ifdef SZNCURSES
 			  	my_addstr(str,len);
#else
				fwrite(str, len, 1, current_screen->fpout);
#endif /* SZNCURSES */
/****************************************************************************/
			term_beep();
			*ptr = c;
			str = ++ptr;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                        ansi_count=0;
#endif
/****************************************************************************/
			break;
		default:
			ptr++;
			break;
		}
	}
	if (result)
		*result = str;
	return written;
}

/*
 * rite: this routine displays a line to the screen adding bold facing when
 * specified by ^Bs, etc.  It also does handles scrolling and paging, if
 * SCROLL is on, and HOLD_MODE is on, etc.  This routine assumes that str
 * already fits on one screen line.  If show is true, str is displayed
 * regardless of the hold mode state.  If redraw is true, it is assumed we a
 * redrawing the screen from the display_ip list, and we should not add what
 * we are displaying back to the display_ip list again. 
 *
 * Note that rite sets display_highlight() to what it was at then end of the
 * last rite().  Also, before returning, it sets display_highlight() to OFF.
 * This way, between susequent rites(), you can be assured that the state of
 * bold face will remain the same and the it won't interfere with anything
 * else (i.e. status line, input line). 
 */
int
rite(window, str, show, redraw, backscroll, logged)
	Window	*window;
	char	*str;
	int	show,
		redraw,
		backscroll,
		logged;
{
	static	int	high = OFF;
	int	written = 0,
		len;
	Screen	*old_current_screen = current_screen;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
	int	ansi_count=0;
#endif
/****************************************************************************/

	if (!backscroll && window->scrolled_lines)
		window->new_scrolled_lines++;
#if 0
	if (window->hold_mode && window->hold_on_next_rite && !redraw && !backscroll && window->line_cnt >= window->display_size)
#endif
	if (window->hold_mode && window->hold_on_next_rite && !redraw && !backscroll)
	{
		/* stops output from going to the window */
		window->hold_on_next_rite = 0;
		hold_mode(window, ON, 1);
		if (show)
			return (1);
	}
	/*
	 * Don't need to worry about the current_screen if the window isn't
	 * visible, as hidden windows aren't attached to a screen anyway
	 */
	if (window->visible)
	{
		old_current_screen = current_screen;
		set_current_screen(window->screen);
	}
	if (!show && (hold_output(window) || hold_queue(window)) && !in_help && !redraw && !backscroll)
		/* sends window output to the hold list for that window */
		add_to_hold_list(window, str, logged);
	else
	{
		if (!redraw && !backscroll)
		{
		/*
		 * This isn't a screen refresh, so add the line to the display
		 * list for the window 
		 */
			if (window->scroll)
				scroll_window(window);
			malloc_strcpy(&(window->display_ip->line), str);
			window->display_ip->linetype = logged;
			window->display_ip = window->display_ip->next;
			if (!window->scroll)
				new_free(&window->display_ip->line);
		}
		if (window->visible)
		{
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
 		  	char *newstr;
#endif /* SZNCURSES */
/****************************************************************************/
			/* make sure the cursor is in the appropriate window */
			if (current_screen->cursor_window != window &&
					!redraw && !backscroll)
			{
				current_screen->cursor_window = window;
				term_move_cursor(0, window->cursor +
					window->top + window->menu.lines);
			}
/**************************** PATCHED by Flier ******************************/
			/*written = output_line(str, &str, 0);
			len = strlen(str);*/
#ifdef SZNCURSES
                        written = output_line(str,&newstr,0);
			len=strlen(newstr);
#else
                        written = output_line(str, &str, 0);
			len = strlen(str);
#endif /* SZNCURSES */
/****************************************************************************/
			written += len;
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                        ansi_count=CountAnsi(str,-1);
                        written-=ansi_count;
#endif
/****************************************************************************/
			if (written > CO)
					len = len - (written - CO);
			if (len > 0)
/**************************** PATCHED by Flier ******************************/
				/*fwrite(str, len, 1, current_screen->fpout);*/
#ifdef SZNCURSES
 			  	my_addstr(newstr,len);
#else
				fwrite(str, len, 1, current_screen->fpout);
#endif /* SZNCURSES */
/****************************************************************************/
			if (term_clear_to_eol())
					term_space_erase(written);
		}
		else if (!(window->miscflags & WINDOW_NOTIFIED))
		{
			if ((who_level & window->notify_level)
			    || ((window->notify_level & LOG_BEEP)
				&& strchr (str, '\007')))
			{
				window->miscflags |= WINDOW_NOTIFIED;
				if (window->miscflags & WINDOW_NOTIFY)
				{
					Window	*old_to_window;
					int	lastlog_level;

					lastlog_level =
						set_lastlog_msg_level(LOG_CRAP);
					old_to_window = to_window;
					to_window = curr_scr_win;
					say("Activity in window %d",
						window->refnum);
					to_window = old_to_window;
					set_lastlog_msg_level(lastlog_level);
				}
				update_all_status();
			}
		}
		if (!redraw && !backscroll)
		{
			window->cursor++;
			window->line_cnt++;
			if (window->scroll)
			{
				if (window->line_cnt >= window->display_size)
				{
					window->hold_on_next_rite = 1;
					window->line_cnt = 0;
				}
			}
			else
			{
				scroll_window(window);
				if (window->cursor == (window->display_size -1))
					window->hold_on_next_rite = 1;
			}
		}
		else if (window->visible)
		{
			term_cr();
			term_newline();
		}
		if (window->visible)
		{
			high = display_highlight(OFF);
			term_flush();
		}
	}
	if (window->visible)
		set_current_screen(old_current_screen);
	return (0);
}

/*
 * cursor_not_in_display: This forces the cursor out of the display by
 * setting the cursor window to null.  This doesn't actually change the
 * physical position of the cursor, but it will force rite() to reset the
 * cursor upon its next call 
 */
void
cursor_not_in_display()
{
	current_screen->cursor_window = NULL;
}

/*
 * cursor_in_display: this forces the cursor_window to be the
 * current_screen->current_window. 
 * It is actually only used in hold.c to trick the system into thinking the
 * cursor is in a window, thus letting the input updating routines move the
 * cursor down to the input line.  Dumb dumb dumb 
 */
void
cursor_in_display()
{
	current_screen->cursor_window = curr_scr_win;
}

/*
 * is_cursor_in_display: returns true if the cursor is in one of the windows
 * (cursor_window is not null), false otherwise 
 */
int
is_cursor_in_display()
{
	if  (current_screen->cursor_window)
		return (1);
	else
		return (0);
}

void
redraw_resized(window, Info, AnchorTop)
	Window	*window;
	ShrinkInfo Info;
	int	AnchorTop;
{
	if (!AnchorTop)
	{
		if (Info.bottom < 0)
			term_scroll(window->top+window->menu.lines+Info.bottom,
				window->top + window->menu.lines +
				window->display_size - 1,
				Info.bottom);
		else if (Info.bottom)
			term_scroll(window->top+window->menu.lines,
				window->top + window->menu.lines +
				window->display_size -1, Info.bottom);
	}
}

/*
 * resize_display: After determining that the screen has changed sizes, this
 * resizes all the internal stuff.  If the screen grew, this will add extra
 * empty display entries to the end of the display list.  If the screen
 * shrank, this will remove entries from the end of the display list.  By
 * doing this, we try to maintain as much of the display as possible. 
 *
 * This has now been improved so that it returns enough information for
 * redraw_resized to redisplay the contents of the window without having
 * to redraw too much.
 */
ShrinkInfo
resize_display(window)
	Window	*window;
{
	int	cnt,
		i;
	Display *tmp, *pre_ip;
	int	Wrapped = 0;
	ShrinkInfo Result;

	Result.top = Result.bottom = 0;
	Result.position = window->cursor;
	if (dumb)
	{
		return Result;
	}
	if (!window->top_of_display)
	{
		window->top_of_display = (Display *)new_malloc(sizeof(Display));
		window->top_of_display->line = NULL;
		window->top_of_display->linetype = LT_UNLOGGED;
		window->top_of_display->next = window->top_of_display;
		window->display_ip = window->top_of_display;
		window->old_size = 1;
	}
	/* cnt = size - window->display_size; */
	cnt = window->display_size - window->old_size;
	if (cnt > 0)
	{
		Display *new = NULL;

	/*
	 * screen got bigger: got to last display entry and link in new
	 * entries 
	 */
		for (tmp = window->top_of_display, i = 0;
		    i < (window->old_size - 1);
		    i++, tmp = tmp->next);
		for (i = 0; i < cnt; i++)
		{
			new = (Display *) new_malloc(sizeof(Display));
			new->line = NULL;
			new->linetype = LT_UNLOGGED;
			new->next = tmp->next;
			tmp->next = new;
		}
		if (window->display_ip == window->top_of_display &&
		    window->top_of_display->line)
			window->display_ip = new;
		Result.top = 0;
		Result.bottom = cnt;
		Result.position = 0;
	}
	else if (cnt < 0)

	{
		Display *ptr;

	/*
	 * screen shrank: find last display entry we want to keep, and remove
	 * all after that point 
	 */
		cnt = -cnt;
		for (pre_ip = window->top_of_display;
		    pre_ip->next != window->display_ip;
		    pre_ip = pre_ip->next);
		for (tmp = pre_ip->next, i =0; i < cnt; i++, tmp = ptr)
		{
			ptr = tmp->next;
			if (tmp == window->top_of_display)

			{
				if (tmp->line)
					Wrapped = 1;
				window->top_of_display = ptr;
			}
			if (Wrapped)
				Result.top--;
			else
				Result.bottom--;
			new_free(&(tmp->line));
			new_free(&tmp);
		}
		window->display_ip = pre_ip->next = tmp;
		window->cursor += Result.top;
		if (!window->scroll)
		{
			if (window->cursor == window->display_size)
				window->cursor = 0;
			new_free(&window->display_ip->line);
		}
	}
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->old_size = window->display_size;
	return Result;
}

/*
 * recalculate_windows: this is called when the terminal size changes (as
 * when an xterm window size is changed).  It recalculates the sized and
 * positions of all the windows.  Currently, all windows are rebalanced and
 * window size proportionality is lost 
 */
void
recalculate_windows()
{
	int	base_size,
	size,
	top,
	extra;
	Window	*tmp;
	
	if (dumb)
		return;

	base_size = ((LI - 1) / current_screen->visible_windows) - 1;
	extra = (LI - 1) - ((base_size + 1)*current_screen->visible_windows);
	top = 0;
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		tmp->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		if (extra)
		{
			extra--;
			size = base_size + 1;
		}
		else
			size = base_size;
#ifdef SCROLL_AFTER_DISPLAY
		tmp->display_size = size - tmp->menu.lines
			- tmp->double_status - 1;
#else
		tmp->display_size = size - tmp->menu.lines
			- tmp->double_status;
#endif /* SCROLL_AFTER_DISPLAY */
		if (tmp->display_size<=0)
			tmp->display_size = 1;
		tmp->top = top;
		tmp->bottom = top + size - tmp->double_status;
		top += size + 1;
	}
}

/*
 * clear_window: This clears the display list for the given window, or
 * current window if null is given.  
 */
void
clear_window(window)
	Window	*window;
{
	int	i,
		cnt;

	if (dumb)
		return;
	if (window == NULL)
		window = curr_scr_win;
	erase_display(window);
	term_move_cursor(0, window->top + window->menu.lines);
	cnt = window->bottom - window->top - window->menu.lines;
	for (i = 0; i < cnt; i++)
	{
		if (term_clear_to_eol())
			term_space_erase(0);
		term_newline();
	}
	term_flush();
}

/* clear_all_windows: This clears all *visible* windows */
void
clear_all_windows(unhold)
	int	unhold;
{
	Window	*tmp;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (unhold)
			hold_mode(tmp, OFF, 1);
		clear_window(tmp);
	}
}

/*
 * redraw_window: This redraws the display list for the given window. Some
 * special considerations are made if you are just redrawing one window as
 * opposed to using this routine to redraw the whole screen one window at a
 * time 
 *
 * A negative value in just_one indicates not all of the window needs to
 * be redrawn.
 */
void
redraw_window(window, just_one)
	Window	*window;
	int	just_one;
{
	Display *tmp;
	int	i;
	int	StartPoint;
	int	yScr;

	if (dumb || !window->visible)
		return;
	window = window ? window : curr_scr_win;
	if (just_one < 0)

	{
	/* This part of the window is scrolling into view */
		StartPoint = -just_one;
		just_one = 0;
	}
	else
	{
		StartPoint = 0;
		if (window->scrolled_lines)
			display_lastlog_lines(window->scrolled_lines-window->display_size + 1, window->scrolled_lines, window);
	}
	if (window->menu.menu)
		ShowMenuByWindow(window, just_one ? SMF_ERASE : 0);
	if (window->scrolled_lines + StartPoint < window->display_size)
		yScr = window->scrolled_lines + StartPoint;
	else
		yScr = 0;
	term_move_cursor(0, window->top+window->menu.lines+yScr);
	/*
	 * if (term_clear_to_eol())
	 *	{ term_space_erase(0); term_cr(); } 
	 */
	for (tmp = window->top_of_display, i = 0; i < window->display_size-window->scrolled_lines; i++, tmp = tmp->next)
	{
		if (i < StartPoint)
			continue;
		if (tmp->line)
			rite(window, tmp->line, 1, 1, 0, 0);
		else
		{
			if (just_one)
			{
				if (term_clear_to_eol())
					term_space_erase(0);
			}
			term_newline();
		}
	}
	term_flush();
}

/*
 * recalculate_window_positions: This runs through the window list and
 * re-adjusts the top and bottom fields of the windows according to their
 * current positions in the window list.  This doesn't change any sizes of
 * the windows 
 */
void
recalculate_window_positions()
{
	Window	*tmp;
	int	top;

	top = 0;
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		tmp->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		tmp->top = top;
		tmp->bottom = top + tmp->display_size + tmp->menu.lines;
		top += tmp->display_size + tmp->menu.lines + 1 +
			tmp->double_status;
	}
}

/*
 * redraw_all_windows: This basically clears and redraws the entire display
 * portion of the screen.  All windows and status lines are draws.  This does
 * nothing for the input line of the screen.  Only visible windows are drawn 
 */
void
redraw_all_windows()
{
	Window	*tmp;

	if (dumb)
		return;
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		tmp->update |= REDRAW_STATUS | REDRAW_DISPLAY_FAST;
}

#define	MAXIMUM_SPLITS	40
static	char	**
split_up_line(str)
	char	*str;
{
	static	char	*output[MAXIMUM_SPLITS] =
	{ 
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL
	};
	char	buffer[BIG_BUFFER_SIZE + 1];
	unsigned char *ptr;
	char	*cont_ptr,
		*cont = NULL,
		*temp = NULL,
		c;
	int	pos = 0,
		col = 0,
		nd_cnt = 0,
		word_break = 0,
		start = 0,
		i,
		len,
		indent = 0,
		beep_cnt = 0,
		beep_max,
		tab_cnt = 0,
		tab_max,
		line = 0;

	bzero(buffer, sizeof(buffer));
	for (i = 0; i < MAXIMUM_SPLITS; i++)
		new_free(&output[i]);
	if (!*str)
		malloc_strcpy(&str, " ");	/* special case to make blank lines show up */

	beep_max = get_int_var(BEEP_VAR) ? get_int_var(BEEP_MAX_VAR) : -1;
	tab_max = get_int_var(TAB_VAR) ? get_int_var(TAB_MAX_VAR) : -1;
	for (ptr = (u_char *) str; *ptr && (pos < BIG_BUFFER_SIZE - 8); ptr++)
	{
		if (translation)
			*ptr = transToClient[*ptr];
		if (*ptr <= 32)
		{
			switch (*ptr)
			{
			case '\007':	/* bell */
				if (beep_max == -1)
				{
					buffer[pos++] = REV_TOG;
					buffer[pos++] = (*ptr & 127) | 64;
					buffer[pos++] = REV_TOG;
					nd_cnt += 2;
					col++;
				}
				else if (!beep_max || (++beep_cnt <= beep_max))
				{
					buffer[pos++] = *ptr;
					nd_cnt++;
					col++;
				}
				break;
			case '\011':	/* tab */
				if (tab_max && (++tab_cnt > tab_max))
				{
					buffer[pos++] = REV_TOG;
					buffer[pos++] = (*ptr & 127) | 64;
					buffer[pos++] = REV_TOG;
					nd_cnt += 2;
					col++;
				}
				else
				{
					if (indent == 0)
						indent = -1;
					len = 8 - (col % 8);
					word_break = pos;
					for (i = 0; i < len; i++)
						buffer[pos++] = ' ';
					col += len;
				}
				break;
			case ' ':	/* word break */
				if (indent == 0)
					indent = -1;
				word_break = pos;
				buffer[pos++] = *ptr;
				col++;
				break;
			case UND_TOG:
			case ALL_OFF:
			case REV_TOG:
			case BOLD_TOG:
				buffer[pos++] = *ptr;
				nd_cnt++;
				break;
			default:	/* Anything else, make it displayable */
				if (indent == -1)
					indent = pos - nd_cnt;
/**************************** PATCHED by Flier ******************************/
				/*buffer[pos++] = REV_TOG;
				buffer[pos++] = (*ptr & 127) | 64;
				buffer[pos++] = REV_TOG;
				nd_cnt += 2;
				col++;*/
#ifdef WANTANSI
                                if (vt100Decode(*ptr)) {
                                    buffer[pos++] = *ptr;
                                    nd_cnt++;
                                }
                                else {
                                    buffer[pos++]=REV_TOG;
                                    buffer[pos++]=(*ptr&127)|64;
                                    buffer[pos++]=REV_TOG;
                                    nd_cnt+=2;
                                    col++;
                                }
#else
                                buffer[pos++] = REV_TOG;
                                buffer[pos++] = (*ptr & 127) | 64;
                                buffer[pos++] = REV_TOG;
                                nd_cnt += 2;
                                col++;
#endif
/****************************************************************************/
				break;
			}
		}
		else
		{
			if (indent == -1)
				indent = pos - nd_cnt;
			buffer[pos++] = *ptr;
/**************************** PATCHED by Flier ******************************/
			/*col++;*/
#ifdef WANTANSI
                        if (vt100Decode(*ptr)) nd_cnt++;
                        else col++;
#else
                        col++;
#endif
/****************************************************************************/
		}
		if (pos == BIG_BUFFER_SIZE)
			*ptr = '\0';
		if (col >= CO)
		{
			/* one big long line, no word breaks */
			if (word_break == 0)
				word_break = pos - (col - CO);
			c = buffer[word_break];
			buffer[word_break] = '\0';
			if (cont)
			{
				malloc_strcpy(&temp, cont);
				malloc_strcat(&temp, &(buffer[start]));
			}
			else
				malloc_strcpy(&temp, &(buffer[start]));
			malloc_strcpy(&output[line++], temp);
			buffer[word_break] = c;
			start = word_break;
			word_break = 0;
			while (buffer[start] == ' ')
				start++;
			if (start > pos)
				start = pos;
			if (!(cont_ptr = get_string_var(CONTINUED_LINE_VAR)))
					
				cont_ptr = empty_string;
			if (get_int_var(INDENT_VAR) && (indent < CO / 3))
			{
		/*
		 * INDENT thanks to Carlo "lynx" v. Loesch
		 * - hehe, nice to see this is still here... -lynx 91
		 */
				if (!cont)
				{
					if ((len = strlen(cont_ptr)) > indent)
					{
						cont = (char *) new_malloc(len
							+ 1);
						strcpy(cont, cont_ptr);
					}
					else
					{
						cont = (char *)
							new_malloc(indent + 1);
						strcpy(cont, cont_ptr);
						for (i = len; i < indent; i++)
							cont[i] = ' ';
						cont[indent] = '\0';
					}
				}
			}
			else
				malloc_strcpy(&cont, cont_ptr);
			col = strlen(cont) + (pos - start);
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                        col-=CountAnsi(&buffer[start],pos-start);
#endif
/****************************************************************************/
		}
	}
	buffer[pos] = '\0';
	if (buffer[start])
	{
		if (cont)
		{
			malloc_strcpy(&temp, cont);
			malloc_strcat(&temp, &(buffer[start]));
		}
		else
			malloc_strcpy(&temp, &(buffer[start]));
		malloc_strcpy(&output[line++], temp);
	}
	new_free(&cont);
	new_free(&temp);
	return output;
}

/*
 * add_to_window: adds the given string to the display.  No kidding. This
 * routine handles the whole ball of wax.  It keeps track of what's on the
 * screen, does the scrolling, everything... well, not quite everything...
 * The CONTINUED_LINE idea thanks to Jan L. Peterson (jlp@hamblin.byu.edu)  
 *
 * At least it used to. Now most of this is done by split_up_line, and this
 * function just dumps it onto the screen. This is because the scrollback
 * functions need to be able to figure out how to split things up too.
 */
static	void
add_to_window(window, str)
	Window	*window;
	char	*str;
{
	int flag;

	flag = do_hook(WINDOW_LIST, "%u %s", window->refnum, str);

	if (flag)
	{
		char	**lines;
		int	logged;

		add_to_log(window->log_fp, str);
		add_to_lastlog(window, str);
		display_highlight(OFF);
		display_bold(OFF);
		strmcat(str, global_all_off, BIG_BUFFER_SIZE - 1);
		logged = islogged(window);
		for (lines = split_up_line(str); *lines; lines++)
		{
			rite(window, *lines, 0, 0, 0, logged);
			if (logged == 1)
				logged = 2;
		}
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
 		attroff(A_BOLD);
 		attrset(A_NORMAL);
#endif /* SZNCURSES */
/****************************************************************************/
		term_flush();
	}
}

/*
 * XXX  this is a temporary internal interface that will go
 * away in the near future.
 */
int in_redirect;

/*
 * add_to_screen: This adds the given null terminated buffer to the screen.
 * That is, it determines which window the information should go to, which
 * lastlog the information should be added to, which log the information
 * should be sent to, etc 
 */
void
add_to_screen(incoming)
	char	*incoming;
{
	int	flag;
	Window	*tmp;
/**************************** PATCHED by Flier ******************************/
        char tmpbuf[BIG_BUFFER_SIZE+1];
#ifdef WANTANSI
        char tmpbuf1[BIG_BUFFER_SIZE+1];
#endif

        if (!get_int_var(DISPLAY_ANSI_VAR)) {
            StripAnsi(incoming,buffer,0);
            strcpy(incoming,buffer);
        }
#ifdef WANTANSI
        else {
            if (DisplaymIRC) {
                ConvertmIRC(incoming,tmpbuf1);
                incoming=tmpbuf1;
            }
            FixColorAnsi(incoming);
        }
#endif
/****************************************************************************/

	/* Handles output redirection first */
	if (!in_redirect && current_screen->redirect_name &&
	    from_server == current_screen->redirect_server)
	{
		int	i;

		in_redirect = 1;
		if (*current_screen->redirect_name == '%')
		{	/* redirection to a process */
			if (is_number(current_screen->redirect_name + 1))
				i = atoi(current_screen->redirect_name + 1);
			else
			    i = logical_to_index(current_screen->redirect_name
				+ 1);
			if (is_process_running(i))
				text_to_process(i, incoming, 0);
		}
		else if (*current_screen->redirect_name == '=')
			dcc_message_transmit(current_screen->redirect_name + 1,
				incoming, DCC_CHAT, 0);
		/*
		 * shouldn't this be a NOTICE?  -lynx
		 * agreed - phone, jan 1993
		 */
/**************************** PATCHED by Flier ******************************/
                /*else
			send_to_server("PRIVMSG %s :%s",
				current_screen->redirect_name, incoming);*/
                else {
                    if (get_int_var(LASTLOG_ANSI_VAR)) StripAnsi(incoming,tmpbuf,0);
                    else strcpy(tmpbuf,incoming);
                    send_to_server("PRIVMSG %s :%s",current_screen->redirect_name,tmpbuf);
                }
/****************************************************************************/
		in_redirect = 0;
	}
	if (dumb)
	{
		add_to_lastlog(curr_scr_win, incoming);
		if (do_hook(WINDOW_LIST, "%u %s", curr_scr_win->refnum, incoming))
			puts(incoming);
		fflush(current_screen->fpout);
		return;
	}
	if (in_window_command)
		update_all_windows();
	if ((who_level == LOG_CURRENT) && (curr_scr_win->server == from_server))
        {
		add_to_window(curr_scr_win, incoming);
		return;
	}
	if (to_window)
	{
		add_to_window(to_window, incoming);
		return;
	}
	if (who_from)
	{
		if (is_channel(who_from))
		{
			ChannelList	*chan;

			if ((chan = lookup_channel(who_from, from_server, CHAN_NOUNLINK)))
			{
				add_to_window(chan->window, incoming);
				return;
			}
			if (who_level == LOG_DCC)
			{
				strcpy(buffer, "=");
				strmcat(buffer, who_from, BIG_BUFFER_SIZE);
				if ((chan = lookup_channel(buffer, from_server, CHAN_NOUNLINK)))
				{
					add_to_window(chan->window, incoming);
					return;
				}
			}
		}
		else
		{
			flag = 1;
			while ((tmp = traverse_all_windows(&flag)) != NULL)
			{
				if (tmp->query_nick &&
					(((who_level == LOG_MSG || who_level == LOG_NOTICE)
					&& !my_stricmp(who_from, tmp->query_nick) &&
					from_server == tmp->server) ||
					(who_level == LOG_DCC &&
					(*tmp->query_nick == '=' || *tmp->query_nick == '@') &&
					my_stricmp(who_from, tmp->query_nick + 1) == 0)))
				{
					add_to_window(tmp, incoming);
					return;
				}
			}
			flag = 1;
			while ((tmp = traverse_all_windows(&flag)) != NULL)
			{
				if (from_server == tmp->server)
				{
					if (find_in_list((List **) &(tmp->nicks), who_from, !USE_WILDCARDS))
					{
						add_to_window(tmp, incoming);
						return;
					}
				}
			}
		}
	}
	flag = 1;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (((from_server == tmp->server) || (from_server == -1)) &&
		    (who_level & tmp->window_level))
		{
			add_to_window(tmp, incoming);
			return;
		}
	}
	if (from_server == curr_scr_win->server)
		tmp = curr_scr_win;
	else
	{
		flag = 1;
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->server == from_server)
				break;
		}
		if (!tmp)
			tmp = curr_scr_win;
	}
	add_to_window(tmp, incoming);
}

/*
 * update_all_windows: This goes through each visible window and draws the
 * necessary portions according the the update field of the window. 
 */
void
update_all_windows()
{
	Window	*tmp;
	int	fast_window,
		full_window,
		r_status,
		u_status;

        for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->display_size != tmp->old_size)
			resize_display(tmp);
		if (tmp->update)
		{
			fast_window = tmp->update & REDRAW_DISPLAY_FAST;
			full_window = tmp->update & REDRAW_DISPLAY_FULL;
			r_status = tmp->update & REDRAW_STATUS;
			u_status = tmp->update & UPDATE_STATUS;
			if (full_window)
				redraw_window(tmp, 1);
			else if (fast_window)
				redraw_window(tmp, 0);
			if (r_status)
				update_window_status(tmp, 1);
			else if (u_status)
				update_window_status(tmp, 0);
		}
		tmp->update = 0;
	}
	for (tmp = invisible_list; tmp; tmp = tmp->next)
	{
		if (tmp->display_size != tmp->old_size)
			resize_display(tmp);
		tmp->update = 0;
	}
	update_input(UPDATE_JUST_CURSOR);
	term_flush();
}

/*
 * create_refnum: this generates a reference number for a new window that is
 * not currently is use by another window.  A refnum of 0 is reserved (and
 * never returned by this routine).  Using a refnum of 0 in the message_to()
 * routine means no particular window (stuff goes to CRAP) 
 */
static	u_int
create_refnum()
{
	unsigned int	new_refnum = 1;
	Window	*tmp;
	int	done = 0,
		flag;

	while (!done)
	{
		done = 1;
		if (new_refnum == 0)
			new_refnum++;

		flag = 1;
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->refnum == new_refnum)
			{
				done = 0;
				new_refnum++;
				break;
			}
		}
	}
	return (new_refnum);
}

/*
 * new_window: This creates a new window on the screen.  It does so by either
 * splitting the current window, or if it can't do that, it splits the
 * largest window.  The new window is added to the window list and made the
 * current window 
 */
Window	*
new_window()
{
	Window	*new;
	static	int	no_screens = 1;

	if (no_screens)

	{
		set_current_screen(create_new_screen());
		main_screen = current_screen;
		no_screens = 0;
	}
	if (dumb && (current_screen->visible_windows == 1))
		return NULL;
	new = (Window *) new_malloc(sizeof(Window));
	new->refnum = create_refnum();
	if (curr_scr_win)
		new->server = curr_scr_win->server;
	else
		new->server = primary_server;
	new->prev_server = -1;
	new->line_cnt = 0;
	if (current_screen->visible_windows == 0)
		new->window_level = LOG_DEFAULT;
	else
		new->window_level = LOG_NONE;
	new->hold_mode = get_int_var(HOLD_MODE_VAR);
	new->scroll = get_int_var(SCROLL_VAR);
	new->lastlog_head = 0;
	new->lastlog_tail = 0;
	new->nicks = 0;
	new->lastlog_level = real_lastlog_level();
	new->name = 0;
	new->prompt = 0;
	new->lastlog_size = 0;
	new->held = OFF;
	new->last_held = OFF;
	new->current_channel = 0;
	new->query_nick = 0;
	new->hold_on_next_rite = 0;
	new->status_line[0] = NULL;
	new->status_line[1] = NULL;
/**************************** PATCHED by Flier ******************************/
	new->status_line[2] = NULL;
        /*new->double_status = 0;*/
        new->double_status = 1;
/****************************************************************************/
	new->top_of_display = 0;
	new->display_ip = 0;
	new->display_size = 1;
	new->old_size = 1;
	new->hold_head = 0;
	new->hold_tail = 0;
	new->held_lines = 0;
	new->scrolled_lines = 0;
	new->new_scrolled_lines = 0;
	new->next = 0;
	new->prev = 0;
	new->cursor = 0;
	new->visible = 1;
	new->screen = current_screen;
	new->logfile = 0;
	new->log = 0;
	new->log_fp = 0;
	new->miscflags = 0;
	new->update = 0;
	new->menu.lines = 0;
	new->menu.menu = 0;
	new->notify_level = real_notify_level();
	new->server_group = 0;
	new->sticky = 1;
	resize_display(new);
	if (add_to_window_list(new))
		set_current_window(new);
	term_flush();
	return (new);
}

void
close_all_screen()
{
	Screen *screen;

	for (screen = screen_list; screen && screen != current_screen;
			screen = screen->next)
		if (screen->alive && screen->fdin != 0)
			new_close(screen->fdin);
}

#ifdef WINDOW_CREATE
Window	*
create_additional_screen()
{
	Window	*win;
	Screen	*oldscreen;
	char	*displayvar,
		*termvar;
	int	screen_type = ST_NOTHING;
	struct	sockaddr_un sock, 
			*sockaddr = &sock,
			NewSock;
	int	NsZ;
	int	s;
	fd_set	fd_read;
	struct	timeval	timeout;
	pid_t	child;
	int	old_timeout;
#define IRCXTERM_MAX 10
	char	*ircxterm[IRCXTERM_MAX];
	char	*ircxterm_env;
	char	*xterm = (char *) 0;
	int	ircxterm_num;
	char	*p, *q;
	int	i;

#ifdef DAEMON_UID
	if (DAEMON_UID == getuid())
	{
		say("you are not permitted to use WINDOW CREATE");
		return (Window *) 0;
	}
#endif

	ircxterm_num = 0;
	p = ircxterm_env = getenv("IRCXTERM");
	if (p)
	{
		q = ircxterm_env + strlen(ircxterm_env);
		while (p < q)
		{
			while (':' == *p)
				p++;
			if ('\0' == *p)
				break;
			ircxterm[ircxterm_num++] = p;
			while (':' != *p && '\0' != *p)
				p++;
			if (':' == *p)
			{
				*p = '\0';
				p++;
			}
		}
	}
	else
	{
		ircxterm[ircxterm_num] = "xterm";
		ircxterm_num++;
	}

	/*
	 * Environment variable STY has to be set for screen to work..  so it is
	 * the best way to check screen..  regardless of what TERM is, the 
	 * execpl() for screen won't just open a new window if STY isn't set,
	 * it will open a new screen process, and run the wserv in its first
	 * window, not what we want...  -phone
	 */
	if (0 != getenv("STY"))
	{
		screen_type = ST_SCREEN;
	}
	else if ((char *) 0 != (displayvar = getenv("DISPLAY")))
	{
		if ((char *) 0 == (termvar = getenv("TERM")))
		{
			say("I don't know how to create new windows for this terminal");
			return (Window *) 0;
		}
		screen_type = ST_XTERM;
		if (0 == strncmp(termvar, "sun", 3))
		{
			xterm = "xterm";
		}
		else
		{
			for(; *termvar; termvar++)
			{
				for (i = 0; i < ircxterm_num; i++)
				{
					if (!strncmp(termvar, ircxterm[i], strlen(ircxterm[i])))
					{
						screen_type = ST_XTERM;
						malloc_strcpy(&xterm, ircxterm[i]);
						termvar = empty_string;
						break;
					}
				}
			}
		}
	}

	if (screen_type == ST_NOTHING)
	{
		say("I don't know how to create new windows for this terminal");
		return (Window *) 0;
	}
	say("Opening new %s...",
		screen_type == ST_XTERM ?  "window" :
		screen_type == ST_SCREEN ? "screen" :
					   "wound" );
	sprintf(sock.sun_path, "/tmp/irc_%08d", (int) getpid());
	sock.sun_family = AF_UNIX;
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	bind(s, (struct sockaddr *) &sock, 2 + strlen(sock.sun_path));
	listen(s, 1);
	oldscreen = current_screen;
	set_current_screen(create_new_screen());
	if (0 == (child = fork()))
	{
		setuid(getuid());
		setgid(getgid());
	/*
	 * Unlike most other cases, it is important here to close down
	 * *ALL* unneeded file descriptors. Failure to do so can cause
	 * Things like server and DCC connections to fail to close on
	 * request. This isn't a problem with "screen", but is with X.
	 */
		new_close(s);
		close_all_screen();
		close_all_dcc();
		close_all_exec();
		close_all_server();
		if (screen_type == ST_SCREEN)
		{
			int	i = 0;
			char	*args[64],
				*s,
				*t,
				*opts = NULL;

			args[i++] = "screen";
			if ((s = get_string_var(SCREEN_OPTIONS_VAR)) != NULL)
			{
				malloc_strcpy(&opts, s);
				while ((t = (char *) strtok(opts," ")) != NULL)
				{
					args[i++] = t;
					opts = NULL;
				}
			}
			args[i++] = WSERV_PATH;
			args[i++] = sockaddr->sun_path;
			args[i++] = NULL;
			execvp("screen", args);
		}
		else if (screen_type == ST_XTERM)
		{
			int	lines,
				columns,
				i = 0;
			char	geom[20],
				*args[64],
				*s,
				*t,
				*opts = NULL;

			copy_window_size(&lines, &columns);
			sprintf(geom, "%dx%d", columns, lines);
			args[i++] = xterm;
			args[i++] = "-geom";
			args[i++] = geom;
			if ((s = get_string_var(XTERM_OPTIONS_VAR)) != NULL)
			{
				malloc_strcpy(&opts, s);
				while ((t = (char *) strtok(opts," ")) != NULL)
				{
					args[i++] = t;
					opts = NULL;
				}
			}
			args[i++] = "-e";
			args[i++] = WSERV_PATH;
			args[i++] = sockaddr->sun_path;
			args[i] = NULL;
			execvp(xterm, args);
		}
		perror("execve");
		exit(errno ? errno : -1);
	}
	NsZ = sizeof(NewSock);
	FD_ZERO(&fd_read);
	FD_SET(s, &fd_read);
	timeout.tv_sec = (time_t) 5;
	timeout.tv_usec = 0;
	sleep(1);

	/* using say(), yell() can be bad in this next section of code. */

	switch (select(NFDBITS , &fd_read, NULL, NULL, &timeout))
	{
	case -1:
	case 0:
		errno = get_child_exit(child);
		new_close(s);
		kill_screen(current_screen);
		kill(child, SIGKILL);
		last_input_screen = oldscreen;
		set_current_screen(oldscreen);
		yell("child %s with %d", (errno < 1) ? "signaled" : "exited",
					 (errno < 1) ? -errno : errno);
		return (Window *) 0;
	default:
		current_screen->fdin = current_screen->fdout =
			accept(s, (struct sockaddr *) &NewSock, &NsZ);
		if (current_screen->fdin < 0)
			return (Window *) 0;
		current_screen->fpin = current_screen->fpout =
			fdopen(current_screen->fdin, "r+");
		new_close(s);
		unlink(sockaddr->sun_path);
		old_timeout = dgets_timeout(5);
		/*
		 * dgets returns 0 on EOF and -1 on timeout.  both of these are
		 * error conditions in this case, so we bail out here.
		 */
/**************************** PATCHED by Flier ******************************/
		/*if (dgets(buffer, BIG_BUFFER_SIZE, current_screen->fdin, (char *) 0) < 1)*/
		if (dgets(buffer, BIG_BUFFER_SIZE, current_screen->fdin, (char *) 0, 0) < 1)
/****************************************************************************/
		{
			new_close(current_screen->fdin);
			kill_screen(current_screen);
			kill(child, SIGKILL);
			last_input_screen = oldscreen;
			set_current_screen(oldscreen);
			(void) dgets_timeout(old_timeout);
			return (Window *) 0;
		}
		else
			malloc_strcpy(&current_screen->tty_name, buffer);
		win = new_window();
		(void) refresh_screen(0, NULL);
		set_current_screen(oldscreen);
		(void) dgets_timeout(old_timeout);
		return win;
	}
	/* NOTREACHED */
}
#endif /* WINDOW_CREATE */

static	char	*
next_line_back(window, skip)
	Window	*window;
 	int	skip;
{
	static	int	row;
	static	Lastlog	*LogLine;
	char	**TheirLines;
	static	char	*ScreenLines[MAXIMUM_SPLITS] =
	{ 
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL
	};

	if (window)
	{
		LogLine = window->lastlog_head;
		row = -1;
	}
 	if (skip)
 	{
 		if (LogLine)
 			LogLine = LogLine->next;
 		row = -1;
 		return NULL;
 	}
	if (row <= 0)
	{
		for (row = 0; ScreenLines[row]; row++)
			new_free(&ScreenLines[row]);
		if (!window && LogLine)
			LogLine = LogLine->next;
		if (!LogLine)
			return NULL;
		TheirLines = split_up_line(LogLine->msg);
		for (row = 0; TheirLines[row]; row++)
		{
			ScreenLines[row] = TheirLines[row];
			TheirLines[row] = NULL;
		}
		if (window)
			return NULL;
	}
	return ScreenLines[--row];
}

static	void
display_lastlog_lines(start, end, window)
	int	start,
		end;
	Window	*window;
{
	Display	*Disp;
	char	*Line;
	int	i;

	(void)next_line_back(window, 0);

	for (i = window->new_scrolled_lines; i--;)
		(void)next_line_back(NULL, 1);

 	/* WTF is this? -krys */
	for (i = 0, Disp = window->top_of_display; i < window->display_size;
			Disp = Disp->next, i++)
		if (Disp->linetype)
			(void)next_line_back(NULL, 0);

	for (i = 0; i < start; i++)
		(void)next_line_back(NULL, 1);

	for (; i < end; i++)
	{
		if (!(Line = next_line_back(NULL, 0)))
			break;
		term_move_cursor(0, window->top + window->menu.lines +
			window->scrolled_lines - i - 1);
		rite(window, Line, 0, 0, 1, 0);
	}
}

static	void
scrollback_backwards_lines(ScrollDist)
	int	ScrollDist;
{
	Window	*window;

	Debug((3, "scrollback_backwards_lines(%d)", ScrollDist));
	window = curr_scr_win;
	if (!window->scrolled_lines && !window->scroll)
	{
		term_beep();
		return;
	}
	Debug((3, "scrolled_lines = %d", window->scrolled_lines));
	window->scrolled_lines += ScrollDist;

	Debug((3, "going to term_scroll(%d, %d, %d)",window->top + window->menu.lines,
			window->top + window->menu.lines + window->display_size - 1, -ScrollDist));
	term_scroll(window->top + window->menu.lines, window->top + window->menu.lines + window->display_size - 1, -ScrollDist);

	Debug((3, "scrolled_lines = %d, new_scrolled_lines = %d, display_size = %d", window->scrolled_lines,
			window->new_scrolled_lines, window->display_size));

	Debug((3, "going to display_lastlog_lines(%d, %d, %s)", window->scrolled_lines - ScrollDist,
			window->scrolled_lines, window->name));
	display_lastlog_lines(window->scrolled_lines - ScrollDist, window->scrolled_lines, window);
	cursor_not_in_display();
	update_input(UPDATE_JUST_CURSOR);
}

static	void
scrollback_forwards_lines(ScrollDist)
	int	ScrollDist;
{
	Window	*window;

	Debug((3, "scrollback_forward_lines(%d)", ScrollDist));
	window = curr_scr_win;
	if (!window->scrolled_lines)
	{
		term_beep();
		return;
	}
	if (ScrollDist > window->scrolled_lines)
		ScrollDist = window->scrolled_lines;

	Debug((3, "scrolled_lines = %d", window->scrolled_lines));
	window->scrolled_lines -= ScrollDist;
	Debug((3, "going to term_scroll(%d, %d, %d)", window->top + window->menu.lines,
			window->top + window->menu.lines + window->display_size - 1, ScrollDist));
	term_scroll(window->top + window->menu.lines, window->top + window->menu.lines + window->display_size - 1, ScrollDist);

	Debug((3, "scrolled_lines = %d, new_scrolled_lines = %d, display_size = %d", window->scrolled_lines,
			window->new_scrolled_lines, window->display_size));
	if (window->scrolled_lines < window->display_size)
		redraw_window(window, ScrollDist + window->scrolled_lines - window->display_size);

	Debug((3, "going to display_lastlog_lines(%d, %d, %s)", window->scrolled_lines - window->display_size,
			window->scrolled_lines - window->display_size + ScrollDist, window->name));
	display_lastlog_lines(window->scrolled_lines - window->display_size,
			window->scrolled_lines - window->display_size + ScrollDist, window);
	cursor_not_in_display();
	update_input(UPDATE_JUST_CURSOR);

	if (!window->scrolled_lines)
	{
		window->new_scrolled_lines = 0;
		if (window->hold_mode)
			hold_mode(window, ON, 1);
		else
			hold_mode(window, OFF, 0);
	}
}

void
#ifdef __STDC__
scrollback_forwards(unsigned char key, char *ptr)
#else
scrollback_forwards(key, ptr)
	unsigned char	key;
	char *	ptr;
#endif
{
/**************************** PATCHED by Flier ******************************/
	/*scrollback_forwards_lines(curr_scr_win->display_size/2);*/
	scrollback_forwards_lines(curr_scr_win->display_size);
/****************************************************************************/
}

void
#ifdef __STDC__
scrollback_backwards(unsigned char key, char *ptr)
#else
scrollback_backwards(key, ptr)
	unsigned char	key;
	char *	ptr;
#endif
{
/**************************** PATCHED by Flier ******************************/
	/*scrollback_backwards_lines(curr_scr_win->display_size/2);*/
	scrollback_backwards_lines(curr_scr_win->display_size);
/****************************************************************************/
}


void
#ifdef __STDC__
scrollback_end(unsigned char key, char *ptr)
#else
scrollback_end(key, ptr)
	unsigned char	key;
	char *	ptr;
#endif
{
	Window	*window;

	window = curr_scr_win;
	window->new_scrolled_lines = 0;

	if (!window->scrolled_lines)
	{
		term_beep();
		return;
	}
	if (window->scrolled_lines < window->display_size)
		scrollback_forwards_lines(window->scrolled_lines);
	else
	{
		window->scrolled_lines = 0;
		redraw_window(window, 1);
		cursor_not_in_display();
		update_input(UPDATE_JUST_CURSOR);
		if (window->hold_mode)
			hold_mode(window, ON, 1);
		else
			hold_mode(window, OFF, 0);
	}
}

/*
 * scrollback_start: moves the current screen back to the start of
 * the scrollback buffer..  there are probably cases this doesn't
 * quite work.. -phone, april 1993.
 */
void
#ifdef __STDC__
scrollback_start(unsigned char key, char *ptr)
#else
scrollback_start(key, ptr)
	unsigned char	key;
	char *	ptr;
#endif
{
	Window	*window;

	window = curr_scr_win;
	if (!window->lastlog_size)
	{
		term_beep();
		return;
	}

	if (window->lastlog_size < window->display_size)
		scrollback_backwards_lines(window->lastlog_size);
	else
	{
		window->scrolled_lines = window->lastlog_size;
		display_lastlog_lines(window->scrolled_lines -
			window->display_size, window->scrolled_lines, window);
		cursor_not_in_display();
		update_input(UPDATE_JUST_CURSOR);
		window->new_scrolled_lines = 0;
		if (window->hold_mode)
			hold_mode(window, ON, 1);
		else
			hold_mode(window, OFF, 0);
	}
}

RETSIGTYPE
sig_refresh_screen()
{
	refresh_screen(0, (char *) 0);
}
