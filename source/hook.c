/*
 * hook.c: Does those naughty hook functions. 
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
 * $Id: hook.c,v 1.12 2002-01-21 22:23:51 f Exp $
 */

#include "irc.h"

#include "hook.h"
#include "vars.h"
#include "ircaux.h"
#include "alias.h"
#include "list.h"
#include "window.h"
#include "server.h"
#include "output.h"
#include "edit.h"

#include "buffer.h"

#ifdef	INCLUDE_UNUSED_FUNCTIONS
static  void	flush_on_hooks _((void));
#endif /* INCLUDE_UNUSED_FUNCTIONS */
static	char	*fill_it_out _((char *, int));
static	void	setup_struct _((int, int, int, int));
static	int	Add_Remove_Check _((List *, char *));
static	int	Add_Remove_Check_List _((List *, List*));
static	void	add_numeric_hook _((int, char *, char *, int, int, int, int));
static	void	add_hook _((int, char *, char *, int, int, int, int));
static	int	show_numeric_list _((int));
static	int	show_list _((int));
static	void	remove_numeric_hook _((int, char *, int, int, int));
static	void	write_hook _((FILE *, Hook *, char *));

#define SILENT 0
#define QUIET 1
#define NORMAL 2
#define NOISY 3

/*
 * The various ON levels: SILENT means the DISPLAY will be OFF and it will
 * suppress the default action of the event, QUIET means the display will be
 * OFF but the default action will still take place, NORMAL means you will be
 * notified when an action takes place and the default action still occurs,
 * NOISY means you are notified when an action occur plus you see the action
 * in the display and the default actions still occurs 
 */
static	char	*noise_level[] = { "SILENT", "QUIET", "NORMAL", "NOISY" };

#define	HS_NOGENERIC	0x1000
#define	HF_LOOKONLY	0x0001
#define HF_NORECURSE	0x0002
#define HF_GLOBAL	0x0004

extern	int	load_depth;

	int	in_on_who = 0;

	NumericList *numeric_list = (NumericList *) 0;
/* hook_functions: the list of all hook functions available */
	HookFunc FAR hook_functions[] =
{
	{ "ACTION",		(Hook *) 0,	3,	0,	0 },
/**************************** PATCHED by Flier ******************************/
        { "CDCC_PLIST",         (Hook *) 0,	5,	0,	0 },
        { "CDCC_PLIST_FOOTER",  (Hook *) 0,	4,	0,	0 },
        { "CDCC_PLIST_HEADER",  (Hook *) 0,	3,	0,	0 },
/****************************************************************************/
	{ "CHANNEL_NICK",	(Hook *) 0,	3,	0,	0 },
	{ "CHANNEL_SIGNOFF",	(Hook *) 0,	3,	0,	0 },
/**************************** PATCHED by Flier ******************************/
        { "CHANNEL_SYNCH",      (Hook *) 0,	2,	0,	0 },
        { "CHANNEL_WALLOP",     (Hook *) 0,	3,	0,	0 },
/****************************************************************************/
	{ "CONNECT",		(Hook *) 0,	1,	0,	0 },
	{ "CTCP",		(Hook *) 0,	4,	0,	0 },
	{ "CTCP_REPLY",		(Hook *) 0,	3,	0,	0 },
	{ "DCC_CHAT",		(Hook *) 0,	2,	0,	0 },
        { "DCC_CONNECT",        (Hook *) 0,     2,      0,      0 },
        { "DCC_ERROR",          (Hook *) 0,     6,      0,      0 },
/**************************** PATCHED by Flier ******************************/
        { "DCC_LIST",           (Hook *) 0,	8,	0,	0 },
        { "DCC_LIST_FOOTER",    (Hook *) 0,	1,	0,	0 },
        { "DCC_LIST_HEADER",    (Hook *) 0,	1,	0,	0 },
/****************************************************************************/
        { "DCC_LOST",           (Hook *) 0,     2,      0,      0 },
	{ "DCC_RAW",		(Hook *) 0,	3,	0,	0 },
        { "DCC_REQUEST",        (Hook *) 0,     4,      0,      0 },
	{ "DISCONNECT",		(Hook *) 0,	1,	0,	0 },
        { "ENCRYPTED_NOTICE",   (Hook *) 0,     3,      0,      0 },
        { "ENCRYPTED_PRIVMSG",  (Hook *) 0,     3,      0,      0 },
	{ "EXEC",		(Hook *) 0,	2,	0,	0 },
	{ "EXEC_ERRORS",	(Hook *) 0,	2,	0,	0 },
	{ "EXEC_EXIT",		(Hook *) 0,	3,	0,	0 },
	{ "EXEC_PROMPT",	(Hook *) 0,	2,	0,	0 },
        { "EXIT",               (Hook *) 0,     1,      0,      0 },
	{ "FLOOD",		(Hook *) 0,	3,	0,	0 },
	{ "HELP",		(Hook *) 0,	2,	0,	0 },
	{ "HOOK",		(Hook *) 0,	1,	0,	0 },
	{ "IDLE",		(Hook *) 0,	1,	0,	0 },
	{ "INPUT",		(Hook *) 0,	1,	0,	0 },
	{ "INVITE",		(Hook *) 0,	2,	0,	0 },
	{ "JOIN",		(Hook *) 0,	3,	0,	0 },
/**************************** PATCHED by Flier ******************************/
        { "JOIN_ME",	        (Hook *) 0,	1,	0,	0 },
/****************************************************************************/
	{ "KICK",		(Hook *) 0,	3,	0,	HF_LOOKONLY },
	{ "LEAVE",		(Hook *) 0,	2,	0,	0 },
	{ "LIST",		(Hook *) 0,	3,	0,	HF_LOOKONLY },
	{ "MAIL",		(Hook *) 0,	2,	0,	0 },
	{ "MODE",		(Hook *) 0,	3,	0,	0 },
	{ "MSG",		(Hook *) 0,	2,	0,	0 },
	{ "MSG_GROUP",		(Hook *) 0,	3,	0,	0 },
	{ "NAMES",		(Hook *) 0,	2,	0,	HF_LOOKONLY },
	{ "NICKNAME",		(Hook *) 0,	2,	0,	0 },
	{ "NOTE",		(Hook *) 0,	10,	0,	0 },
	{ "NOTICE",		(Hook *) 0,	2,	0,	0 },
	{ "NOTIFY_SIGNOFF",	(Hook *) 0,	1,	0,	0 },
/**************************** Patched by Flier ******************************/
	{ "NOTIFY_SIGNOFF_UH",	(Hook *) 0,	3,	0,	0 },
/****************************************************************************/
	{ "NOTIFY_SIGNON",	(Hook *) 0,	1,	0,	0 },
/**************************** PATCHED by Flier ******************************/
	{ "NOTIFY_SIGNON_UH",	(Hook *) 0,	3,	0,	0 },
/****************************************************************************/
	{ "PUBLIC",		(Hook *) 0,	3,	0,	0 },
	{ "PUBLIC_MSG",		(Hook *) 0,	3,	0,	0 },
	{ "PUBLIC_NOTICE",	(Hook *) 0,	3,	0,	0 },
	{ "PUBLIC_OTHER",	(Hook *) 0,	3,	0,	0 },
	{ "RAW_IRC",		(Hook *) 0,	1,	0,	0 },
	{ "RAW_SEND",		(Hook *) 0,	1,	0,	0 },
	{ "SEND_ACTION",	(Hook *) 0,	2,	0,	0 },
/**************************** PATCHED by Flier ******************************/
	{ "SEND_CTCP",		(Hook *) 0,	3,	0,	0 },
/****************************************************************************/
	{ "SEND_DCC_CHAT",	(Hook *) 0,	2,	0,	0 },
	{ "SEND_MSG",		(Hook *) 0,	2,	0,	0 },
	{ "SEND_NOTICE",	(Hook *) 0,	2,	0,	0 },
	{ "SEND_PUBLIC",	(Hook *) 0,	2,	0,	0 },
	{ "SERVER_NOTICE",	(Hook *) 0,	1,	0,	0 },
	{ "SIGNOFF",		(Hook *) 0,	1,	0,	0 },
	{ "TIMER",		(Hook *) 0,	1,	0,	0 },
	{ "TOPIC",		(Hook *) 0,	2,	0,	0 },
	{ "WALL",		(Hook *) 0,	2,	0,	HF_LOOKONLY },
	{ "WALLOP",		(Hook *) 0,	3,	0,	HF_LOOKONLY },
	{ "WHO",		(Hook *) 0,	6,	0,	HF_LOOKONLY },
	{ "WIDELIST",		(Hook *) 0,	1,	0,	HF_LOOKONLY },
	{ "WINDOW",		(Hook *) 0,	2,	0,	HF_NORECURSE },
	{ "WINDOW_KILL",	(Hook *) 0,	1,	0,	0 },
	{ "WINDOW_SWAP",	(Hook *) 0,	2,	0,	0 }
};

static char	*
fill_it_out(str, params)
	char	*str;
	int	params;
{
 	char	lbuf[BIG_BUFFER_SIZE + 1];
	char	*arg,
		*free_ptr = (char *) 0,
		*ptr;
	int	i = 0;

	malloc_strcpy(&free_ptr, str);
	ptr = free_ptr;
 	*lbuf = (char) 0;
	while ((arg = next_arg(ptr, &ptr)) != NULL)
	{
 		if (*lbuf)
 			strmcat(lbuf, " ", BIG_BUFFER_SIZE);
 		strmcat(lbuf, arg, BIG_BUFFER_SIZE);
		if (++i == params)
			break;
	}
	for (; i < params; i++)
 		strmcat(lbuf, (i < params-1) ? " %" : " *", BIG_BUFFER_SIZE);
	if (*ptr)
	{
 		strmcat(lbuf, " ", BIG_BUFFER_SIZE);
 		strmcat(lbuf, ptr, BIG_BUFFER_SIZE);
	}
 	malloc_strcpy(&free_ptr, lbuf);
	return (free_ptr);
}


/*
 * A variety of comparison functions used by the hook routines follow.
 */

struct	CmpInfoStruc
{
	int	ServerRequired;
	int	SkipSerialNum;
	int	SerialNumber;
	int	Flags;
}	cmp_info;

#define	CIF_NOSERNUM	0x0001
#define	CIF_SKIP	0x0002

int     cmpinfodone = 0;

static void
setup_struct(ServReq, SkipSer, SerNum, flags)
	int	ServReq;
	int	SkipSer;
	int	SerNum;
	int	flags;
{
	cmp_info.ServerRequired = ServReq;
	cmp_info.SkipSerialNum = SkipSer;
	cmp_info.SerialNumber = SerNum;
	cmp_info.Flags = flags;
}

static	int
Add_Remove_Check(_Item, Name)
 	List	*_Item;
	char	*Name;
{
	int	comp;
 	Hook	*Item = (Hook *)_Item;

	if (cmp_info.SerialNumber != Item->sernum)
		return (Item->sernum > cmp_info.SerialNumber) ? 1 : -1;
	if ((comp = my_stricmp(Item->nick, Name)) != 0)
			return comp;
	if (Item->server != cmp_info.ServerRequired)
		return (Item->server > cmp_info.ServerRequired) ? 1 : -1;
	return 0;
}

static	int
Add_Remove_Check_List(_Item, _Item2)
	List	*_Item;
	List	*_Item2;
{
	return Add_Remove_Check(_Item, _Item->name);
}

static	void
add_numeric_hook(numeric, nick, stuff, noisy, not, server, sernum)
	int	numeric;
	char	*nick,
		*stuff;
	int	noisy,
		not;
	int	server,
		sernum;
{
	NumericList *entry;
	Hook	*new;
	char	buf[4];

	snprintf(buf, sizeof buf, "%3.3u", numeric);
	if ((entry = (NumericList *) find_in_list((List **) &numeric_list, buf, 0)) ==
			(NumericList *) 0)
	{
		entry = (NumericList *) new_malloc(sizeof(NumericList));
		entry->name = (char *) 0;
		entry->list = (Hook *) 0;
		malloc_strcpy(&(entry->name), buf);
		add_to_list((List **) &numeric_list, (List *) entry);
	}

	setup_struct((server==-1) ? -1 : (server & ~HS_NOGENERIC), sernum-1, sernum, 0);
 	if ((new = (Hook *) remove_from_list_ext((List **) &(entry->list), nick, Add_Remove_Check)) != NULL)
	{
		new->not = 1;
		new_free(&(new->nick));
		new_free(&(new->stuff));
		wait_new_free((char **) &new);
	}
	new = (Hook *) new_malloc(sizeof(Hook));
	new->nick = (char *) 0;
	new->noisy = noisy;
	new->server = server;
	new->sernum = sernum;
	new->not = not;
	new->global = loading_global;
	new->stuff = (char *) 0;
	malloc_strcpy(&new->nick, nick);
	malloc_strcpy(&new->stuff, stuff);
	upper(new->nick);
 	add_to_list_ext((List **) &(entry->list), (List *) new, Add_Remove_Check_List);
}

/*
 * add_hook: Given an index into the hook_functions array, this adds a new
 * entry to the list as specified by the rest of the parameters.  The new
 * entry is added in alphabetical order (by nick). 
 */
static	void
add_hook(which, nick, stuff, noisy, not, server, sernum)
	int	which;
	char	*nick,
		*stuff;
	int	noisy,
		not;
	int	server,
		sernum;
{
	Hook	*new;

	if (which < 0)
	{
		add_numeric_hook(-which, nick, stuff, noisy, not, server,
			sernum);
		return;
	}
	setup_struct((server == -1) ? -1 : (server & ~HS_NOGENERIC), sernum-1, sernum, 0);
 	if ((new = (Hook *) remove_from_list_ext((List **) &(hook_functions[which].list), nick, Add_Remove_Check)) != NULL)
	{
		new->not = 1;
		new_free(&(new->nick));
		new_free(&(new->stuff));
		wait_new_free((char **) &new);
	}
	new = (Hook *) new_malloc(sizeof(Hook));
	new->nick = (char *) 0;
	new->noisy = noisy;
	new->server = server;
	new->sernum = sernum;
	new->not = not;
	new->stuff = (char *) 0;
	new->global = loading_global;
	malloc_strcpy(&new->nick, nick);
	malloc_strcpy(&new->stuff, stuff);
	upper(new->nick);
 	add_to_list_ext((List **) &(hook_functions[which].list), (List *) new, Add_Remove_Check_List);
}

/* show_hook shows a single hook */
extern	void
show_hook(list, name)
	Hook	*list;
	char	*name;
{
	if (list->server != -1)
		say("On %s from \"%s\" do %s [%s] <%d> (Server %d)%s",
		    name, list->nick,
		    (list->not ? "nothing" : list->stuff),
		    noise_level[list->noisy], list->sernum,
		    list->server&~HS_NOGENERIC,
		    (list->server&HS_NOGENERIC) ? " Exclusive" : empty_string);
	else
		say("On %s from \"%s\" do %s [%s] <%d>",
		    name, list->nick,
		    (list->not ? "nothing" : list->stuff),
		    noise_level[list->noisy],
		    list->sernum);
}

/*
 * show_numeric_list: If numeric is 0, then all numeric lists are displayed.
 * If numeric is non-zero, then that particular list is displayed.  The total
 * number of entries displayed is returned 
 */
static	int
show_numeric_list(numeric)
	int	numeric;
{
	NumericList *tmp;
	Hook	*list;
	char	buf[4];
	int	cnt = 0;

	if (numeric)
	{
		snprintf(buf, sizeof buf, "%3.3u", numeric);
		if ((tmp = (NumericList *) find_in_list((List **) &numeric_list, buf, 0))
				!= NULL)
		{
			for (list = tmp->list; list; list = list->next, cnt++)
				show_hook(list, tmp->name);
		}
	}
	else
	{
		for (tmp = numeric_list; tmp; tmp = tmp->next)
		{
			for (list = tmp->list; list; list = list->next, cnt++)
				show_hook(list, tmp->name);
		}
	}
	return (cnt);
}

/*
 * show_list: Displays the contents of the list specified by the index into
 * the hook_functions array.  This function returns the number of entries in
 * the list displayed 
 */
static	int
show_list(which)
	int	which;
{
	Hook	*list;
	int	cnt = 0;

	/* Less garbage when issueing /on without args. (lynx) */
	for (list = hook_functions[which].list; list; list = list->next, cnt++)
		show_hook(list, hook_functions[which].name);
	return (cnt);
}

/*
 * do_hook: This is what gets called whenever a MSG, INVITES, WALL, (you get
 * the idea) occurs.  The nick is looked up in the appropriate list. If a
 * match is found, the stuff field from that entry in the list is treated as
 * if it were a command. First it gets expanded as though it were an alias
 * (with the args parameter used as the arguments to the alias).  After it
 * gets expanded, it gets parsed as a command.  This will return as its value
 * the value of the noisy field of the found entry, or -1 if not found. 
 */
/* huh-huh.. this sucks.. im going to re-write it so that it works */
/*VARARGS*/
int
#ifdef HAVE_STDARG_H
do_hook(int which, char *format, ...)
{
	va_list vl;
#else
/**************************** Patched by Flier ******************************/
/*do_hook(which, format, arg1, arg2, arg3, arg4, arg5, arg6)*/
do_hook(which, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
/****************************************************************************/
	int	which;
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
/**************************** Patched by Flier ******************************/
		/**arg6;*/
		*arg6,
                *arg7,
                *arg8,
                *arg9;
/****************************************************************************/
{
#endif
	Hook	*tmp, **list;
	char	*name = (char *) 0;
	int	RetVal = 1;
	unsigned int	display;
	int	i,
		old_in_on_who;
	Hook	*hook_array[2048];
	int	hook_num = 0;
	static	int hook_level = 0;
 	size_t	len;
	char	*foo;
#ifdef NEED_PUTBUF_DECLARED
	/* make this buffer *much* bigger than needed */
	u_char	putbuf[2*BIG_BUFFER_SIZE + 1] = "";
#endif
	PUTBUF_INIT

	hook_level++;

#ifdef HAVE_STDARG_H
	va_start(vl, format);
	PUTBUF_SPRINTF(format, vl);
	va_end(vl);
#else
/**************************** Patched by Flier ******************************/
	/*snprintf(CP(putbuf), sizeof putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6);*/
	snprintf(putbuf, sizeof putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
/****************************************************************************/
#endif
	if (which < 0)
	{
		NumericList *hook;
		char	buf[4];

		snprintf(buf, sizeof buf, "%3.3u", -which);
		if ((hook = (NumericList *) find_in_list((List **) &numeric_list, buf, 0))
				!= NULL)
		{
			name = hook->name;
			list = &hook->list;
		}
		else
			list = (Hook **) 0;
	}
	else
	{
		if (hook_functions[which].mark && (hook_functions[which].flags & HF_NORECURSE))
			list = (Hook **) 0;
		else
		{
			list = &(hook_functions[which].list);
			name = hook_functions[which].name;
		}
	}
	if (!list)
	{
		RetVal = 1;
		goto out;
	}

	if (which >= 0)
		hook_functions[which].mark++;
			/* not attached, so dont "fix" it */
	{
		int currser = 0, oldser = 0;
		int currmatch = 0, oldmatch = 0;
		Hook *bestmatch = (Hook *) 0;
		int nomorethisserial = 0;

		for (tmp = *list;tmp;tmp = tmp->next)
		{
			currser = tmp->sernum;
			if (currser != oldser)      /* new serial number */
			{
                        	oldser = currser;
				currmatch = oldmatch = nomorethisserial = 0;
				if (bestmatch)
					hook_array[hook_num++] = bestmatch;
				bestmatch = (Hook *) 0;
			}

			if (nomorethisserial) 
				continue;
					/* if there is a specific server
			   		hook and it doesnt match, then
			   		we make sure nothing from
			   		this serial number gets hooked */
			if ((tmp->server != -1) &&
 		   	    (tmp->server & HS_NOGENERIC) &&
 		   	    (tmp->server != (from_server & HS_NOGENERIC)))
			{
				nomorethisserial = 1;
				bestmatch = (Hook *) 0;
				continue;
			}
			currmatch = wild_match(tmp->nick, putbuf);
			if (currmatch > oldmatch)
			{
				oldmatch = currmatch;
				bestmatch = tmp;
			}
		}
		if (bestmatch)
			hook_array[hook_num++] = bestmatch;
	}

	for (i = 0; i < hook_num; i++)
	{
		tmp = hook_array[i];
		if (!tmp)
		{
			if (which >= 0)
				hook_functions[which].mark--;
			goto out;
		}
		if (tmp->not)
			continue;
		send_text_flag = which;
		if (tmp->noisy > QUIET)
			say("%s activated by \"%s\"", name, putbuf);
		display = window_display;
		if (tmp->noisy < NOISY)
			window_display = 0;

		save_message_from();
		old_in_on_who = in_on_who;
		if (which == WHO_LIST || (which <= -311 && which >= -318))
			in_on_who = 1;
		len = strlen(tmp->stuff) + 1; 
		foo = new_malloc(len);
		bcopy(tmp->stuff, foo, len);
		parse_line((char *) 0, foo, putbuf, 0, 0, 1);
		new_free(&foo);
		in_on_who = old_in_on_who;
		window_display = display;
		send_text_flag = -1;
		restore_message_from();
		if (!tmp->noisy && !tmp->sernum)
			RetVal = 0;
	}
	if (which >= 0)
		hook_functions[which].mark--;
out:
	PUTBUF_END
	return really_free(--hook_level), RetVal;
}

static	void
remove_numeric_hook(numeric, nick, server, sernum, quiet)
	int	numeric;
	char	*nick;
	int	server;
	int	sernum;
	int	quiet;
{
	NumericList *hook;
	Hook	*tmp,
		*next;
	char	buf[4];

	snprintf(buf, sizeof buf, "%3.3u", numeric);
	if ((hook = (NumericList *) find_in_list((List **) &numeric_list, buf,0)) != NULL)
	{
		if (nick)
		{
			setup_struct((server == -1) ? -1 :
			    (server & ~HS_NOGENERIC), sernum - 1, sernum, 0);
			if ((tmp = (Hook *) remove_from_list((List **) &(hook->list), nick)) != NULL)
			{
				if (!quiet)
					say("\"%s\" removed from %s list", nick, buf);
				tmp->not = 1;
				new_free(&(tmp->nick));
				new_free(&(tmp->stuff));
				wait_new_free((char **) &tmp);
				if (hook->list == (Hook *) 0)
				{
					if ((hook = (NumericList *) remove_from_list((List **) &numeric_list, buf)) != NULL)
					{
						new_free(&(hook->name));
						new_free(&hook);
					}
				}
				return;
			}
		}
		else
		{
			for (tmp = hook->list; tmp; tmp = next)
			{
				next = tmp->next;
				tmp->not = 1;
				new_free(&(tmp->nick));
				new_free(&(tmp->stuff));
				wait_new_free((char **) &tmp);
			}
			hook->list = (Hook *) 0;
			if (!quiet)
				say("The %s list is empty", buf);
			return;
		}
	}
	if (quiet)
		return;
	if (nick)
		say("\"%s\" is not on the %s list", nick, buf);
	else
		say("The %s list is empty", buf);
}
 
#ifdef	INCLUDE_UNUSED_FUNCTIONS
static  void   
flush_on_hooks()
{
        int	x;
        int	old_display = window_display;
        
        window_display = 0;
        for (x = 100 ; x < 999; x++)
                remove_numeric_hook(x, (char *) 0, 1, x, 0);
        for (x = 0 ; x < NUMBER_OF_LISTS; x++)
                remove_hook(x, (char *) 0, 1, x, 0);
        window_display = old_display;
}
#endif /* INCLUDE_UNUSED_FUNCTIONS */

extern	void
remove_hook(which, nick, server, sernum, quiet)
	int	which;
	char	*nick;
	int	server,
		sernum,
		quiet;
{
	Hook	*tmp,
		*next;

	if (which < 0)
	{
		remove_numeric_hook(-which, nick, server, sernum, quiet);
		return;
	}
	if (nick)
	{
		setup_struct((server == -1) ? -1 : (server & ~HS_NOGENERIC),
			sernum-1, sernum, 0);
 		if ((tmp = (Hook *) remove_from_list_ext((List **) &hook_functions[which].list, nick, Add_Remove_Check)) != NULL)
		{
			if (!quiet)
				say("\"%s\" removed from %s list", nick, hook_functions[which].name);
			tmp->not = 1;
			new_free(&(tmp->nick));
			new_free(&(tmp->stuff));
			wait_new_free((char **) &tmp);
		}
		else if (!quiet)
			say("\"%s\" is not on the %s list", nick, hook_functions[which].name);
	}
	else
	{
		for(tmp = hook_functions[which].list; tmp; tmp=next)
		{
			next = tmp->next;
			tmp->not = 1;
			new_free(&(tmp->nick));
			new_free(&(tmp->stuff));
			wait_new_free((char **) &tmp);
		}
		hook_functions[which].list = (Hook *) 0;
		if (!quiet)
			say("The %s list is empty", hook_functions[which].name);
	}
}

/* on: The ON command */
/*ARGSUSED*/
void
on(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*func,
		*nick,
		*serial,
		*cmd = (char *) 0;
 	/* int noisy = NORMAL, not = 0, do_remove = 0, -not used */
	int	noisy,
		not,
		server,
		sernum,
		do_remove,
		which = 0,
		cnt,
		i;
 	size_t	len;

	if (get_int_var(NOVICE_VAR) && !load_depth)
	{
	    yell("*** You may not type ON commands when you have the NOVICE");
	    yell("*** variable set to ON. Some ON commands may cause a");
	    yell("*** security breach on your machine, or enable another");
	    yell("*** user to control your IRC session. Read the help files");
	    yell("*** in /HELP ON before using ON");
	    return;
	}
	if ((func = next_arg(args, &args)) != NULL)
	{
		if (*func == '#')
		{
			if (!(serial = next_arg(args, &args)))
			{
				say("No serial number specified");
				return;
			}
			sernum = atoi(serial);
			func++;
		}
		else
			sernum = 0;
		switch (*func)
		{
		case '&':
			server = from_server;
			func++;
			break;
		case '@':
			server = from_server|HS_NOGENERIC;
			func++;
			break;
		default:
			server = -1;
			break;
		}
		switch (*func)
		{
		case '-':
			noisy = QUIET;
			func++;
			break;
		case '^':
			noisy = SILENT;
			func++;
			break;
		case '+':
			noisy = NOISY;
			func++;
			break;
		default:
			noisy = NORMAL;
			break;
		}
		if ((len = strlen(func)) == 0)
		{
			say("You must specify an event type!");
			return;
		}
		malloc_strcpy(&cmd, func);
		upper(cmd);
		for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
		{
			if (!strncmp(cmd, hook_functions[i].name, len))
			{
				if (strlen(hook_functions[i].name) == len)
				{
					cnt = 1;
					which = i;
					break;
				}
				else
				{
					cnt++;
					which = i;
				}
			}
			else if (cnt)
				break;
		}
		if (cnt == 0)
		{
			if (is_number(cmd))
			{
				which = atoi(cmd);
				if ((which < 0) || (which > 999))
				{
					say("Numerics must be between 001 and 999");
					goto out;
				}
				which = -which;
			}
			else
			{
				say("No such ON function: %s", func);
				goto out;
			}
		}
		else if (cnt > 1)
		{
			say("Ambiguous ON function: %s", func);
			goto out;
		}
		else 
		{
			if (get_int_var(INPUT_PROTECTION_VAR) && !my_strnicmp(hook_functions[which].name, "INPUT", 5))
			{
				say("You cannot use /ON INPUT with INPUT_PROTECTION set");
				say("Please read /HELP ON INPUT, and /HELP SET INPUT_PROTECTION");
				goto out;
			}
		}
		do_remove = 0;
		not = 0;
		switch (*args)
		{
		case '-':
			do_remove = 1;
			args++;
			break;
		case '^':
			not = 1;
			args++;
			break;
		}
		if ((nick = new_next_arg(args, &args)) != NULL)
		{
			if (which < 0)
				nick = fill_it_out(nick, 1);
			else
				nick = fill_it_out(nick,
					hook_functions[which].params);
			if (do_remove)
			{
				if (strlen(nick) == 0)
					say("No expression specified");
				else
					remove_hook(which, nick, server,
						sernum, 0);
			}
			else
			{
				if (not)
					args = empty_string;
				if (*nick)
				{
					if (*args == LEFT_BRACE)
					{
						char	*ptr;

						ptr = MatchingBracket(++args,
								LEFT_BRACE, RIGHT_BRACE);
						if (!ptr)
						{
							say("Unmatched brace in ON");
							new_free(&nick);
							goto out;
						}
						else if (ptr[1])
						{
							say("Junk after closing brace in ON");
							new_free(&nick);
							goto out;
						}
						else
							*ptr = '\0';
					}
					add_hook(which, nick, args, noisy, not, server, sernum);
					if (which < 0)
						say("On %3.3u from \"%s\" do %s [%s] <%d>",
						    -which, nick, (not ? "nothing" : args),
						    noise_level[noisy], sernum);
					else
						say("On %s from \"%s\" do %s [%s] <%d>",
							hook_functions[which].name, nick,
							(not ? "nothing" : args),
							noise_level[noisy], sernum);
				}
			}
			new_free(&nick);
		}
		else
		{
			if (do_remove)
				remove_hook(which, (char *) 0, server,
					sernum, 0);
			else
			{
				if (which < 0)
				{
					if (show_numeric_list(-which) == 0)
						say("The %3.3u list is empty.",
							-which);
				}
				else if (show_list(which) == 0)
					say("The %s list is empty.",
						hook_functions[which].name);
			}
		}
	}
	else
	{
		int	total = 0;

		say("ON listings:");
		for (which = 0; which < NUMBER_OF_LISTS; which++)
			total += show_list(which);
		total += show_numeric_list(0);
		if (total == 0)
			say("All ON lists are empty.");
	}
out:
	new_free(&cmd);
}

static	void
write_hook(fp, hook, name)
	FILE	*fp;
	Hook	*hook;
	char	*name;
{
	char	*stuff = (char *) 0;

	if (hook->server!=-1)
		return;
	switch (hook->noisy)
	{
	case SILENT:
		stuff = "^";
		break;
	case QUIET:
		stuff = "-";
		break;
	case NORMAL:
		stuff = empty_string;
		break;
	case NOISY:
		stuff = "+";
		break;
	}
	if (hook->sernum)
		fprintf(fp, "ON #%s%s %d \"%s\"", stuff, name, hook->sernum,
			hook->nick);
	else
		fprintf(fp, "ON %s%s \"%s\"", stuff, name, hook->nick);
	fprintf(fp, " %s\n", hook->stuff);
}

/*
 * save_hooks: for use by the SAVE command to write the hooks to a file so it
 * can be interpreted by the LOAD command 
 */
void
save_hooks(fp, do_all)
	FILE	*fp;
	int	do_all;
{
	Hook	*list;
	NumericList *numeric;
	int	which;

	for (which = 0; which < NUMBER_OF_LISTS; which++)
	{
		for (list = hook_functions[which].list; list; list = list->next)
			if (!list->global || do_all)
				write_hook(fp,list, hook_functions[which].name);
	}
	for (numeric = numeric_list; numeric; numeric = numeric->next)
	{
		for (list = numeric->list; list; list = list->next)
			if (!list->global)
				write_hook(fp, list, numeric->name);
	}
}

/**************************** Patched by Flier ******************************/
void DumpOn() {
    int  i;
    Hook *tmp;
    Hook *tmpdel;
    NumericList *hook;
    NumericList *hookdel;

    for (i=0;i<NUMBER_OF_LISTS;i++) {
        for (tmp=hook_functions[i].list;tmp;) {
            tmpdel=tmp;
            tmp=tmp->next;
            tmpdel->not=1;
            new_free(&(tmpdel->nick));
            new_free(&(tmpdel->stuff));
            wait_new_free((char **) &tmpdel);
        }
        hook_functions[i].list=(Hook *) 0;
    }
    for (hook=numeric_list;hook;) {
        hookdel=hook;
        hook=hook->next;
        for(tmp=hookdel->list;tmp;) {
            tmpdel=tmp;
            tmp=tmp->next;
            tmpdel->not = 1;
            new_free(&(tmpdel->nick));
            new_free(&(tmpdel->stuff));
            wait_new_free((char **) &tmpdel);
        }
        hookdel->list=(Hook *) 0;
    }
}
/****************************************************************************/
