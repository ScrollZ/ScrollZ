/*
 * funny.c: Well, I put some stuff here and called it funny.  So sue me. 
 *
 * Written by Michael Sandrof
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
 * $Id: funny.c,v 1.26 2008-12-22 10:53:39 f Exp $
 */

#include "irc.h"

#include "ircaux.h"
#include "hook.h"
#include "vars.h"
#include "funny.h"
#include "names.h"
#include "server.h"
#include "lastlog.h"
#include "ircterm.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "screen.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

extern void PrintNames _((char *, char *, ChannelList *));
extern void PrintSynch _((ChannelList *));
extern int  IsIrcNetOperChannel _((char *));
#if defined(OPERVISION) && defined(WANTANSI)
extern void OperVisionReinit _((void));
#endif
extern void ChannelLogSave _((char *, ChannelList *));
/****************************************************************************/

static	char	*match_str = (char *) 0;

static	int	funny_min;
static	int	funny_max;
static	int	funny_flags;

void 
funny_match (char *stuff)
{
	malloc_strcpy(&match_str, stuff);
}

void 
set_funny_flags (int min, int max, int flags)
{
	funny_min = min;
	funny_max = max;
	funny_flags = flags;
}

struct	WideListInfoStru
{
	char	*channel;
	int	users;
};

typedef	struct WideListInfoStru WideList;

static	WideList **wide_list = (WideList **) 0;
static	int	wl_size = 0;
static	size_t	wl_elements = 0;

static	int	funny_widelist_users _((WideList **, WideList **));
static	int	funny_widelist_names _((WideList **, WideList **));

static int 
funny_widelist_users (WideList **left, WideList **right)
{
	if ((**left).users > (**right).users)
		return -1;
	else if ((**right).users > (**left).users)
		return 1;
	else
		return my_stricmp((**left).channel, (**right).channel);
}

static int 
funny_widelist_names (WideList **left, WideList **right)
{
	int	comp;

	if ((comp = my_stricmp((**left).channel, (**right).channel)) != 0)
		return comp;
	else if ((**left).users > (**right).users)
		return -1;
	else if ((**right).users > (**left).users)
		return 1;
	else
		return 0;
}


void 
funny_print_widelist (void)
{
	int	i;
	char	buffer1[BIG_BUFFER_SIZE+1];
	char	buffer2[BIG_BUFFER_SIZE+1];
	char	*ptr;

	if (!wide_list)
		return;

	if (funny_flags & FUNNY_NAME)
		qsort((void *) wide_list, wl_elements, sizeof(WideList *),
 			(int (*) _((const void *, const void *))) funny_widelist_names);
	else if (funny_flags & FUNNY_USERS)
		qsort((void *) wide_list, wl_elements, sizeof(WideList *),
 			(int (*) _((const void *, const void *))) funny_widelist_users);

	*buffer1 = '\0';
	for (i = 0; i < wl_elements; i++)
	{
		snprintf(buffer2, sizeof buffer2, "%s(%d) ", wide_list[i]->channel,
				wide_list[i]->users);
		ptr = index(buffer1, '\0');
		if (strlen(buffer1) + strlen(buffer2) > current_screen->co - 5)
		{
			if (do_hook(WIDELIST_LIST, "%s", buffer1))
				say("%s", buffer1);
			*buffer1 = '\0';
/**************************** Patched by Flier ******************************/
			/*strcat(buffer1, buffer2);*/
			strmcat(buffer1, buffer2, sizeof(buffer1));
/****************************************************************************/
		}
		else
/**************************** Patched by Flier ******************************/
			/*strcpy(ptr, buffer2);*/
			strmcpy(ptr, buffer2, sizeof(buffer1) - (ptr - buffer1));
/****************************************************************************/
	}
	if (*buffer1 && do_hook(WIDELIST_LIST, "%s", buffer1))
		say("%s" , buffer1);
	for (i = 0; i < wl_elements; i++)
	{
		new_free(&wide_list[i]->channel);
		new_free(&wide_list[i]);
	}
	new_free(&wide_list);
	wl_elements = wl_size = 0;
}

/*ARGSUSED*/
void 
funny_list (char *from, char **ArgList)
{
	char	*channel,
		*user_cnt,
		*line;
	WideList **new_list;
	int	cnt;
	static	char	format[25];
 	static	int	last_width = -1;

	if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR))
	{
/**************************** PATCHED by Flier ******************************/
		/*if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
			snprintf(CP(format), sizeof format, "*** %%-%u.%us %%-5s  %%s",
				(unsigned char) last_width,
				(unsigned char) last_width);
		else
			strcpy(format, "*** %s\t%-5s  %s");*/
		if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
			snprintf(format, sizeof format, "*** %%-%u.%us %%-5s  %%s",
				(unsigned char) last_width,
				(unsigned char) last_width);
		else
			strcpy(format, "%s\t%-5s  %s");
/****************************************************************************/
	}
	channel = ArgList[0];
	user_cnt = ArgList[1];
	line = PasteArgs(ArgList, 2);
	if (funny_flags & FUNNY_TOPIC && !(line && *line))
			return;
	cnt = atoi(user_cnt);
	if (funny_min && (cnt < funny_min))
		return;
	if (funny_max && (cnt > funny_max))
		return;
	if ((funny_flags & FUNNY_PRIVATE) && (*channel != '*'))
		return;
	if ((funny_flags & FUNNY_PUBLIC) && (*channel == '*'))
		return;
	if (match_str)
	{
		if (wild_match(match_str, channel) == 0)
			return;
	}
	if (funny_flags & FUNNY_WIDE)
	{
		if (wl_elements >= wl_size)
		{
			new_list = (WideList **) new_malloc(sizeof(WideList *) *
			    (wl_size + 50));
			bzero(new_list, sizeof(WideList *) * (wl_size + 50));
			if (wl_size)
				bcopy(wide_list, new_list, sizeof(WideList *)
					* wl_size);
			wl_size += 50;
			new_free(&wide_list);
			wide_list = new_list;
		}
		wide_list[wl_elements] = (WideList *)
			new_malloc(sizeof(WideList));
		wide_list[wl_elements]->channel = (char *) 0;
		wide_list[wl_elements]->users = cnt;
		malloc_strcpy(&wide_list[wl_elements]->channel,
				(*channel != '*') ? channel : "Prv");
		wl_elements++;
		return;
	}
	if (do_hook(current_numeric, "%s %s %s %s", from,  channel, user_cnt,
	    line) && do_hook(LIST_LIST, "%s %s %s", channel, user_cnt, line))
	{
		if (channel && user_cnt)
		{
			if (*channel == '*')
/**************************** PATCHED by Flier ******************************/
				/*put_it(format, "Prv", user_cnt, line);*/
				say(format, "Prv", user_cnt, line);
/****************************************************************************/
			else
/**************************** PATCHED by Flier ******************************/
				/*put_it(format, channel, user_cnt, line);*/
				say(format, channel, user_cnt, line);
/****************************************************************************/
		}
	}
}

void 
funny_namreply (char *from, char **Args)
{
	char	*type,
		*nick,
		*channel;
	static	char	format[40];
 	static	int	last_width = -1;
	int	cnt;
	char	*ptr;
	char	*line;
	ChannelList	*tmp = (ChannelList *) 0;

	PasteArgs(Args, 2);
	type = Args[0];
	channel = Args[1];
	line = Args[2];
 	save_message_from();
	message_from(channel, LOG_CRAP);
	if ((tmp = lookup_channel(channel, parsing_server_index, CHAN_NOUNLINK)) && !((tmp->status & CHAN_NAMES) && (tmp->status & CHAN_MODE)))
	{
		if (do_hook(current_numeric, "%s %s %s %s", from, type, channel,
			line) && get_int_var(SHOW_CHANNEL_NAMES_VAR))
/**************************** PATCHED by Flier ******************************/
			/*say("Users on %s: %s", channel, line);
		while ((nick = next_arg(line, &line)) != NULL)
			add_to_channel(channel, nick, parsing_server_index, 0, 0);*/
			PrintNames(channel, line, tmp);
                while ((nick = next_arg(line, &line))!=NULL)
                    add_to_channel(channel, nick, parsing_server_index, 0, 0, 0, NULL, tmp);
/****************************************************************************/
		tmp->status |= CHAN_NAMES;
 		goto out;
	}
	if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR))
	{
		if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
			snprintf(format, sizeof format, "%%s: %%-%u.%us %%s",
				(unsigned char) last_width,
				(unsigned char) last_width);
		else
			strcpy(format, "%s: %s\t%s");
	}
	ptr = line;
	for (cnt = -1; ptr; cnt++)
	{
		if ((ptr = index(ptr, ' ')) != NULL)
			ptr++;
	}
	if (funny_min && (cnt < funny_min))
		return;
	else if (funny_max && (cnt > funny_max))
		return;
	if ((funny_flags & FUNNY_PRIVATE) && (*type == '='))
		return;
	if ((funny_flags & FUNNY_PUBLIC) && (*type == '*'))
		return;
	if (type && channel)
	{
		if (match_str)
		{
			if (wild_match(match_str, channel) == 0)
				return;
		}
		if (do_hook(current_numeric, "%s %s %s %s", from, type, channel,
			line) && do_hook(NAMES_LIST, "%s %s", channel, line))
		{
			switch (*type)
			{
			case '=':
				if (last_width &&(strlen(channel) > last_width))
				{
					channel[last_width-1] = '>';
					channel[last_width] = (char) 0;
				}
/**************************** PATCHED by Flier ******************************/
				/*put_it(format, "Pub", channel, line);*/
				say("Users on %s are : %s", channel, line);
/****************************************************************************/				
				break;
			case '*':
/**************************** PATCHED by Flier ******************************/
				/*put_it(format, "Prv", channel, line);*/
				say("Users on %s are : %s", channel, line);
/****************************************************************************/				
				break;
			case '@':
/**************************** PATCHED by Flier ******************************/
				/*put_it(format, "Sec", channel, line);*/
				say("Users on %s are : %s", channel, line);
/****************************************************************************/				
				break;
			}
		}
	}
out:
 	restore_message_from();
}

void 
funny_mode (char *from, char **ArgList)
{
	char	*mode, *channel;
	ChannelList	*tmp = (ChannelList *) 0;

	if (!ArgList[0])
		return;
	if (get_server_version(parsing_server_index) < Server2_6)
	{
		channel = (char *) 0;
		mode = ArgList[0];
		PasteArgs(ArgList, 0);
	}
	else
	{
		channel = ArgList[0];
		mode = ArgList[1];
		PasteArgs(ArgList, 1);
	}
	/* if (ignore_mode) */
	if (channel && (tmp = lookup_channel(channel, parsing_server_index, CHAN_NOUNLINK)) && !((tmp->status & CHAN_NAMES) && (tmp->status & CHAN_MODE)))
	{
/**************************** PATCHED by Flier ******************************/
		/*update_channel_mode(channel, parsing_server_index, mode);*/
		update_channel_mode(channel, parsing_server_index, mode, strlen(mode),
                                    NULL, NULL, NULL, NULL, tmp);
                if ((get_server_version(from_server) == Server2_11) &&
                    IsIrcNetOperChannel(channel)) {
                    tmp->gotbans = 1;
                    tmp->gotwho = 1;
                    if (server_list[from_server].SZUnban > 2)
                        server_list[from_server].SZUnban--;
                    else server_list[from_server].SZUnban = 0;
                    server_list[from_server].SZWho--;
                    PrintSynch(tmp);
                }
                if (tmp && tmp->ChanLog) {
                    char tmpbuf[mybufsize];

                    snprintf(tmpbuf, sizeof(tmpbuf), "Mode for channel %s is %s", channel, mode);
                    ChannelLogSave(tmpbuf, tmp);
                }
/****************************************************************************/
		tmp->status |= CHAN_MODE;
		update_all_status();
	}
	else
	{
 		save_message_from();
 		message_from(channel, LOG_CRAP);
		if (channel)
		{
			if (do_hook(current_numeric, "%s %s %s", from,
					channel, mode))
/**************************** Patched by Flier ******************************/
				/*put_it("%s Mode for channel %s is \"%s\"",*/
#ifdef WANTANSI
				put_it("%sMode for channel %s%s%s is \"%s%s%s\"",
					numeric_banner(),
					CmdsColors[COLMODE].color3, channel,
					Colors[COLOFF],
                                        CmdsColors[COLMODE].color4, mode, Colors[COLOFF]);
#else
				put_it("%sMode for channel %s is \"%s\"",
/****************************************************************************/
					numeric_banner(), channel, mode);
#endif
		}
		else
		{
			if (do_hook(current_numeric, "%s %s", from, mode))
/**************************** Patched by Flier ******************************/
				/*put_it("%s Channel mode is \"%s\"",*/
				put_it("%sChannel mode is \"%s\"",
/****************************************************************************/
					numeric_banner(), mode);
		}
 		restore_message_from();
	}
}

/**************************** PATCHED by Flier ******************************/
void 
update_user_mode (char *modes)
{
    int	onoff = 1;

    while (*modes) {
        if (*modes == '-') onoff = 0;
        else if (*modes == '+') onoff = 1;
        else {
            if (*modes == 'o' || *modes == 'O')
                set_server_operator(parsing_server_index, onoff);
            set_server_umode_flag(parsing_server_index, *modes, onoff);
        }
        modes++;
    }
}

void 
reinstate_user_modes (void)
{
    int  i;
    char modes[64]; /* more than enough */
    char *c;

    if (get_server_version(parsing_server_index) < Server2_7) return;
    c = modes;
    for (i = 0; i < 25; i++) {
        if ('a' + i != 'o' && get_server_umode_flag(parsing_server_index, 'a' + i))
            *c ++= 'a' + i;
    }
    for (i = 0; i < 25; i++) {
        if ('A' + i != 'O' && get_server_umode_flag(parsing_server_index, 'A' + i))
            *c ++= 'A' + i;
    }
    *c = '\0';
    if (PermUserMode)
        send_to_server("MODE %s %s", get_server_nickname(parsing_server_index), PermUserMode);
    else if (c != modes) send_to_server("MODE %s +%s", get_server_nickname(parsing_server_index), modes);
#if defined(OPERVISION) && defined(WANTANSI)
    if (OperV) OperVisionReinit();
#endif
}
/****************************************************************************/
