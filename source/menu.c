/*
 * menu.c: stuff for the menu's..
 *
 * Written By Troy Rollo <troy@cbme.unsw.oz.au>
 *
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
 * $Id: menu.c,v 1.6 2000-09-24 17:10:34 f Exp $
 */

#include "irc.h"

#include "menu.h"
#include "list.h"
#include "ircaux.h"
#include "ircterm.h"
#include "window.h"
#include "screen.h"
#include "input.h"
#include "vars.h"
#include "output.h"

#ifndef LITE

#ifdef lines
#undef lines
#endif

Menu	*MenuList = (Menu *) 0;

struct	OptionTableTag
{
	char	*name;
	void	(*func) _((char *));
};

typedef	struct	OptionTableTag OptionTable;

static	OptionTable	OptionsList[] =
{
	{ "PREVIOUS",	menu_previous },
	{ "MENU",	menu_submenu },
	{ "EXIT",	menu_exit },
	{ "CHANNELS",	menu_channels },
	{ "COMMAND",	menu_command },
	{ (char *) 0,	(void (*) _((char *))) 0 }
};

static	MenuOption * create_new_option _((void));
static	Menu	* create_new_menu _((void));
static	void	install_menu _((Menu *, int));

static	MenuOption *
create_new_option()
{
	MenuOption *NewOption;

	NewOption = (MenuOption *) new_malloc(sizeof(MenuOption));
	NewOption->Name = (char *) 0;
	NewOption->Arguments = (char *) 0;
	NewOption->Func = (void (*) _((char *))) 0;
	return NewOption;
}

static	Menu	*
create_new_menu()
{
	Menu	*NewMenu;

	NewMenu = (Menu *) new_malloc(sizeof(Menu));
	NewMenu->Name = (char *) 0;
	NewMenu->TotalOptions = 0;
	NewMenu->Options = (MenuOption **) 0;
	return NewMenu;
}


static	void
install_menu(NewMenu, TotalMade)
	Menu	*NewMenu;
	int	TotalMade;
{
	MenuOption **NewOptions;
	int	i;

	if (!NewMenu)
		return;
	if (TotalMade != NewMenu->TotalOptions)
	{
		NewOptions = (MenuOption **) malloc(TotalMade *
							sizeof(MenuOption *));
		for (i = 0; i < TotalMade; i++)
			NewOptions[i] = NewMenu->Options[i];
		new_free(&NewMenu->Options);
		NewMenu->Options = NewOptions;
		NewMenu->TotalOptions = TotalMade;
	}
	add_to_list((List **) &MenuList, (List *) NewMenu);
	say("Menu \"%s\" added", NewMenu->Name);
}

void
load_menu(FileName)
	char	*FileName;
{
	FILE	*fp;
	Menu	*NewMenu = NULL;
	MenuOption **NewOptions;
	char	*line, *command;
	char	*name, *func;
	char	buffer[BIG_BUFFER_SIZE+1];
	int	linenum = 0;
	int	CurTotal = 0;
	int	FuncNum;
	int	i;

	if ((fp = fopen(FileName, "r")) == (FILE *) 0)
	{
		say("Unable to open %s", FileName);
		return;
	}
	while (fgets(buffer, BIG_BUFFER_SIZE, fp))
	{
		buffer[strlen(buffer)-1] = '\0';
		linenum++;
		line = buffer;
		while (isspace(*line))
			line++;
		if (*line == '#' || !(command = next_arg(line, &line)))
			continue;
		if (!my_stricmp(command, "MENU"))
		{
			if (!line || !*line)
			{
				put_it("Missing argument in line %d of %s",
						linenum, FileName);
				continue;
			}
			install_menu(NewMenu, CurTotal);
			NewMenu = create_new_menu();
			malloc_strcpy(&NewMenu->Name, line);
		}
		else if (!my_stricmp(command, "OPTION"))
		{
			if (!(name = new_next_arg(line, &line)) ||
			    !(func = next_arg(line, &line)))
			{
				say("Missing argument in line %d of %s",
						linenum, FileName);
				continue;
			}
			for (i=0; OptionsList[i].name; i++)
				if (!my_stricmp(func, OptionsList[i].name))
					break;
			if (!OptionsList[i].name)
			{
				say("Unknown menu function \"%s\" in line %d of %s",
					func, linenum, FileName);
				continue;
			}
			FuncNum = i;
			if (++CurTotal > NewMenu->TotalOptions)
			{
				NewOptions = (MenuOption **)
					new_malloc(sizeof(MenuOption *) *
						(CurTotal+4));
				for (i = 0; i < NewMenu->TotalOptions; i++)
					NewOptions[i] = NewMenu->Options[i];
				new_free(&NewMenu->Options);
				NewMenu->Options = NewOptions;
				NewMenu->TotalOptions = CurTotal + 5;
			}
			NewMenu->Options[CurTotal-1] = create_new_option();
			malloc_strcpy(&NewMenu->Options[CurTotal-1]->Name,
				name);
			malloc_strcpy(&NewMenu->Options[CurTotal-1]->Arguments,
				line);
			NewMenu->Options[CurTotal-1]->Func =
					OptionsList[FuncNum].func;
		}
		else
			say("Unkown menu command in line %d of %s",
				linenum, FileName);
	}
	install_menu(NewMenu, CurTotal);
	fclose(fp);
}

int
ShowMenuByWindow(window, flags)
	Window	*window;
	int	flags;
{
	int	i;
	int	largest;
	int	NumPerLine;
	int	len;
	WindowMenu *menu_info;
	Menu	*ThisMenu;
	int	CursorLoc;

	menu_info = &window->menu;
	ThisMenu = menu_info->menu;
	CursorLoc = menu_info->cursor;
	largest = 0;
	for (i = 0; i < ThisMenu->TotalOptions; i++)
		if ((len = strlen(ThisMenu->Options[i]->Name))>largest)
			largest = len;
	NumPerLine = (CO - CO % (largest + 3)) / (largest + 3);
	menu_info->items_per_line = NumPerLine;
	menu_info->lines = 0;
	for (i = 0; i < ThisMenu->TotalOptions; i++)
	{
		if ((flags & SMF_ERASE) && !(i % NumPerLine) &&
		    !(flags & SMF_CALCONLY))
		{
			term_move_cursor(0, window->top + menu_info->lines);
			term_clear_to_eol();
		}
		if ((i == CursorLoc || !(flags&SMF_CURSONLY)) &&
		    !(flags & SMF_CALCONLY))
		{
			if (i == CursorLoc && !(flags & SMF_NOCURSOR) &&
					current_screen->inside_menu == 1)
				term_standout_on();
			else
				term_bold_on();
			term_move_cursor((i % NumPerLine) * (largest + 3),
				window->top+menu_info->lines);
			fwrite(ThisMenu->Options[i]->Name,
				strlen(ThisMenu->Options[i]->Name), 1, stdout);
			if (i == CursorLoc && !(flags & SMF_NOCURSOR))
				term_standout_off();
			else
				term_bold_off();
		}
		if (!((i + 1) % NumPerLine))
			menu_info->lines++;
	}
	if (i % NumPerLine)
		menu_info->lines++;
	window->display_size = window->bottom - window->top - menu_info->lines;
	if (window->cursor < 0)
		window->cursor = 0;
	fflush(stdout);
	update_input(UPDATE_JUST_CURSOR);
	return ThisMenu->TotalOptions;
}

int
ShowMenu(Name)
	char	*Name;
{
	Menu	*ThisMenu;
	Window	*window;
	WindowMenu *menu_info;

	window = curr_scr_win;
	menu_info = &window->menu;
	ThisMenu = (Menu *) find_in_list((List **) &MenuList, Name, 0);
	if (!ThisMenu)
	{
		say("No such menu \"%s\"", Name);
		return -1;
	}
	menu_info->cursor = 0;
	menu_info->menu = ThisMenu;
	return ShowMenuByWindow(window, SMF_CALCONLY);
}

void
set_menu(Value)
	char	*Value;
{
	Window	*window;
	WindowMenu *menu_info;
	ShrinkInfo SizeInfo;

	window = curr_scr_win;
	menu_info = &window->menu;
	if (!Value)
	{
		window->display_size = window->bottom - window->top;
		menu_info->menu = (Menu *) 0;
		menu_info->lines = 0;
		SizeInfo = resize_display(window);
		redraw_resized(window, SizeInfo, 0);
		update_input(UPDATE_JUST_CURSOR);
		current_screen->inside_menu = -1;
	}
	else
	{
		if (ShowMenu(Value) == -1)
		{
			set_string_var(MENU_VAR, NULL);
			return;
		}
		SizeInfo = resize_display(window);
		redraw_resized(window, SizeInfo, 0);
		ShowMenuByWindow(window, SMF_ERASE);
/*
		redraw_window(window);
 */
	}
}

void
enter_menu(key, ptr)
 	u_int	key;
	char *	ptr;
{
	if (!curr_scr_win->menu.menu)
		return;
	current_screen->inside_menu = 1;
	ShowMenuByWindow(curr_scr_win, SMF_CURSONLY);
}


void
menu_previous(args)
	char	*args;
{
}

void
menu_submenu(args)
	char	*args;
{
}

void
menu_exit(args)
	char	*args;
{
	current_screen->inside_menu = -1;
}

void
menu_channels(args)
	char	*args;
{
}

void
menu_key(ikey)
 	u_int	ikey;
{
	Window *window;
	WindowMenu *menu_info;
	Menu	*ThisMenu;
 	u_char	key = (u_char)ikey;

	window = curr_scr_win;
	menu_info = &window->menu;
	ThisMenu = menu_info->menu;
	ShowMenuByWindow(window, SMF_CURSONLY | SMF_NOCURSOR);
	switch(key)
	{
	case 'U':
	case 'u':
	case 'P':
	case 'p':
	case 'k':
	case 'K':
	case 'U' - '@':
	case 'P' - '@':
		menu_info->cursor-=menu_info->items_per_line;
		break;
	case 'n':
	case 'd':
	case 'j':
	case 'N':
	case 'D':
	case 'J':
	case 'D' - '@':
	case 'N' - '@':
		menu_info->cursor+=menu_info->items_per_line;
		break;
	case 'b':
	case 'h':
	case 'B':
	case 'H':
	case 'B' - '@':
		menu_info->cursor--;
		break;
	case 'f':
	case 'l':
	case 'F':
	case 'L':
	case 'F' - '@':
		menu_info->cursor++;
		break;
	case '\033':
		break;
	case '\r':
	case '\n':
		hold_mode((Window *) 0, OFF, 1);
		break;
	case ' ':
	case '.':
		current_screen->inside_menu = 0;
		ThisMenu->Options[menu_info->cursor]->Func(
			ThisMenu->Options[menu_info->cursor]->Arguments);
		if (current_screen->inside_menu != -1)
			current_screen->inside_menu = 1;
		else
			current_screen->inside_menu = 0;
		return; /* The menu may not be here any more */
	}
	if (menu_info->cursor>=menu_info->menu->TotalOptions)
		menu_info->cursor = menu_info->menu->TotalOptions - 1;
	if (menu_info->cursor < 0)
		menu_info->cursor = 0;
	if (current_screen->inside_menu)
		ShowMenuByWindow(window, SMF_CURSONLY);
}

void
menu_command(args)
	char	*args;
{
	parse_line((char *) 0, args, empty_string, 0, 0);
}

#endif /* LITE */
