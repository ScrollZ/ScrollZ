/*
 * screen.h: header for screen.c
 *
 * written by matthew green.
 *
 * Copyright (c) 1993-2003 Matthew R. Green.
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
 * see the copyright file, or type help ircii copyright
 *
 * $Id: screen.h,v 1.7 2008-12-01 15:41:36 f Exp $
 */

#ifndef __screen_h_
#define __screen_h_

#include "window.h"

#define WAIT_PROMPT_LINE        0x01
#define WAIT_PROMPT_KEY         0x02

/* Stuff for the screen/xterm junk */

#define ST_NOTHING      -1
#define ST_SCREEN       0
#define ST_XTERM        1

/* This is here because it happens in so many places */
#define curr_scr_win	current_screen->current_window

	void	clear_window _((Window *));
	void	recalculate_window_positions _((void));
	int	output_line _((char *, int));
	void	recalculate_windows _((void));
	Window	*create_additional_screen _((void));
	void	scroll_window _((Window *));
	Window	*new_window _((void));
	void	update_all_windows _((void));
 	void	add_wait_prompt _((char *, void (*)(char *, char *), char *, int));
	void	clear_all_windows _((int));
	void	cursor_in_display _((void));
	int	is_cursor_in_display _((void));
	void	cursor_not_in_display _((void));
	void	set_current_screen _((Screen *));
	void	window_redirect _((char *, int));
	void	redraw_resized _((Window *, ShrinkInfo, int));
	void	close_all_screen _((void));
 	void	scrollback_forwards _((u_int, char *));
 	void	scrollback_backwards _((u_int, char *));
 	void	scrollback_end _((u_int, char *));
 	void	scrollback_start _((u_int, char *));
	int	check_screen_redirect _((char *));
	void	kill_screen _((Screen *));
	int	is_main_screen _((Screen *));
	int	rite _((Window *, char *, int, int, int, int));
	ShrinkInfo	resize_display _((Window *));
	void	redraw_all_windows _((void));
	Window *add_to_screen _((char *));
	void	screen_wserv_message _((Screen *screen));
/**************************** PATCHED by Flier ******************************/
	void	redraw_window _((Window *, int, int));
/****************************************************************************/

	int	my_strlen_i _((char *));
	int	my_strlen_c _((char *));
	int	my_strlen_ci _((char *));
	void	my_strcpy_ci _((char *, char *));

extern	Window	*to_window;
extern	Screen	*current_screen;
extern	Screen	*main_screen;
extern	Screen	*last_input_screen;
extern	Screen	*screen_list;

#endif /* __screen_h_ */
