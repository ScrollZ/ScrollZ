/*
 * alias.c Handles command aliases for irc.c 
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
 * $Id: alias.c,v 1.51 2008-12-09 15:58:11 f Exp $
 */

#include "irc.h"

#include "alias.h"
#include "dcc.h"
#include "status.h"
#include "edit.h"
#include "history.h"
#include "vars.h"
#include "ircaux.h"
#include "server.h"
#include "screen.h"
#include "window.h"
#include "input.h"
#include "names.h"
#include "server.h"
#include "output.h"
#include "names.h"
#include "parse.h"
#include "notify.h"
#include "ignore.h"
#include "exec.h"
#include "ircterm.h"

#include "sys/stat.h"

/*
 * define this to use the old way of managing allocations
 * inside the guts of alias handling (static buffers, not
 * malloc & realloc'ed ones).
 */
#undef USE_OLD_ALIAS_ALLOC

/**************************** Patched by Flier ******************************/
#ifdef HAVE_REGCOMP
#include <regex.h>
#endif

#include "myvars.h"

extern int  OpenFileRead _((char *));
extern int  OpenFileWrite _((char *, char *));
extern int  FileClose _((int));
extern int  FileEof _((int));
extern int  FileWrite _((int, char *));
extern char *FileRead _((int));
extern struct friends *CheckUsers _((char *, char *));
extern void StripAnsi _((char *, char *, int));
extern struct autobankicks *CheckABKs _((char *, char *));
extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern void BuildPrivs _((struct friends *, char *));
extern void PrintUsage _((char *));
extern int  ColorNumber  _((char *));
extern char *TimeStamp _((int));
#ifndef CELESCRP
extern int  TotalSendDcc _((void));
extern int  TotalQueue _((void));
extern int  is_voiced _((char *, char *));
#endif

extern char VersionInfo[];
extern char *ScrollZver1;
/****************************************************************************/

extern	int	parse_number _((char **));
static	char	*next_unit _((char *, char *, int *, int));
/**************************** PATCHED by Flier ******************************/
/*static	long	randm _((long));*/
/****************************************************************************/
static	char	*lastop _((char *));
static	char	*arg_number _((int, int, char *));
static	void	do_alias_string _((void));
static	Alias	*find_alias _((Alias **, char *, int, int *));
static	void	insert_alias _((Alias **, Alias *));
static	char	*alias_arg _((char **, u_int *));
static	char	*find_inline _((char *));
static	u_char	*built_in_alias _((int));
#ifndef USE_OLD_ALIAS_ALLOC
static	void	expander_addition _((char **, char *, int, char *));
static	char	*alias_special_char _((char *, char **, char *, char *, char *, int *));
#else /* USE_OLD_ALIAS_ALLOC */
static	void	expander_addition _((char *, char *, int, char *));
static	char	*alias_special_char _((char *, char *, char *, char *, char *, int *));
#endif /* USE_OLD_ALIAS_ALLOC */

static	u_char	*alias_detected _((void));
static	u_char	*alias_sent_nick _((void));
static	u_char	*alias_recv_nick _((void));
#ifndef LITE
static	u_char	*alias_msg_body _((void));
#endif
static	u_char	*alias_joined_nick _((void));
static	u_char	*alias_public_nick _((void));
static	u_char	*alias_dollar _((void));
static	u_char	*alias_channel _((void));
static	u_char	*alias_server _((void));
static	u_char	*alias_query_nick _((void));
static	u_char	*alias_target _((void));
static	u_char	*alias_nick _((void));
static	u_char	*alias_invite _((void));
static	u_char	*alias_cmdchar _((void));
#ifndef LITE
static	u_char	*alias_line _((void));
#endif
static	u_char	*alias_away _((void));
static	u_char	*alias_oper _((void));
static	u_char	*alias_chanop _((void));
static	u_char	*alias_modes _((void));
static	u_char	*alias_buffer _((void));
static	u_char	*alias_time _((void));
static	u_char	*alias_version _((void));
static	u_char	*alias_currdir _((void));
static	u_char	*alias_current_numeric _((void));
static	u_char	*alias_server_version _((void));

/**************************** Patched by Flier ******************************/
static	u_char	*alias_ScrollZ_version _((void));
/*static	char	*function_pattern _((void));
static	char	*function_chops _((void));
static	char	*function_chnops _((void));*/
/* Patched by Zakath */
#ifdef CELE
static	u_char	*alias_Celerity_version _((void));
#endif
/* ***************** */
/****************************************************************************/

typedef struct
{
	char	name;
 	u_char	*(*func) _((void));
}	BuiltIns;

static	BuiltIns built_in[] =
{
	{ '.',		alias_sent_nick },
	{ ',',		alias_recv_nick },
	{ ':',		alias_joined_nick },
	{ ';',		alias_public_nick },
	{ '$',		alias_dollar },
	{ 'A',		alias_away },
#ifndef LITE
	{ 'B',		alias_msg_body },
#endif
	{ 'C',		alias_channel },
	{ 'D',		alias_detected },
/*
	{ 'E' },
	{ 'F' },
	{ 'G' },
*/
	{ 'H', 		alias_current_numeric },
	{ 'I',		alias_invite },
/**************************** PATCHED by Flier ******************************/
/*
	{ 'J' },
*/
        { 'J',          alias_ScrollZ_version },
/****************************************************************************/
	{ 'K',		alias_cmdchar },
#ifndef LITE
	{ 'L',		alias_line },
#endif
	{ 'M',		alias_modes },
	{ 'N',		alias_nick },
	{ 'O',		alias_oper },
	{ 'P',		alias_chanop },
	{ 'Q',		alias_query_nick },
	{ 'R',		alias_server_version },
	{ 'S',		alias_server },
	{ 'T',		alias_target },
	{ 'U',		alias_buffer },
	{ 'V',		alias_version },
	{ 'W',		alias_currdir },
/**************************** PATCHED by Flier ******************************/
/*
	{ 'X' },
	{ 'Y' },
*/
#ifdef CELE
        { 'Y',		alias_Celerity_version },
#endif
/****************************************************************************/
	{ 'Z',		alias_time },
	{ (char) 0,	 NULL }
};

/**************************** PATCHED by Flier ******************************/
 	/*char	*command_line = (char *) 0;*/
/****************************************************************************/

	u_char	*function_left _((u_char *));
	u_char	*function_right _((u_char *));
	u_char	*function_mid _((u_char *));
	u_char	*function_rand _((u_char *));
	u_char	*function_srand _((u_char *));
	u_char	*function_time _((u_char *));
	u_char	*function_stime _((u_char *));
	u_char	*function_index _((u_char *));
	u_char	*function_rindex _((u_char *));
	u_char	*function_match _((u_char *));
	u_char	*function_rmatch _((u_char *));
	u_char	*function_userhost _((u_char *));
	u_char	*function_strip _((u_char *));
	u_char	*function_encode _((u_char *));
	u_char	*function_decode _((u_char *));
	u_char	*function_ischannel _((u_char *));
	u_char	*function_ischanop _((u_char *));
#ifdef HAVE_CRYPT
	u_char	*function_crypt _((u_char *));
#endif /* HAVE_CRYPT */
	u_char	*function_hasvoice _((u_char *));
	u_char	*function_dcclist _((u_char *));
	u_char	*function_chatpeers _((u_char *));
	u_char	*function_word _((u_char *));
	u_char	*function_winnum _((u_char *));
	u_char	*function_winnam _((u_char *));
	u_char	*function_winrows _((u_char *));
	u_char	*function_wincols _((u_char *));
	u_char	*function_winvisible _((u_char *));
	u_char	*function_winserver _((u_char *));
	u_char	*function_winservergroup _((u_char *));
	u_char	*function_connect _((u_char *));
	u_char	*function_listen _((u_char *));
	u_char	*function_tdiff _((u_char *));
	u_char	*function_toupper _((u_char *));
	u_char	*function_tolower _((u_char *));
	u_char	*function_channels _((u_char *));
	u_char	*function_servers _((u_char *));
	u_char	*function_servertype _((u_char *));
	u_char	*function_curpos _((u_char *));
	u_char	*function_onchannel _((u_char *));
	u_char	*function_pid _((u_char *));
	u_char	*function_ppid _((u_char *));
/**************************** PATCHED by Flier ******************************/
#ifndef CELESCRP
	u_char	*function_chanusers _((u_char *));
#endif
/****************************************************************************/
	u_char	*function_idle _((u_char *));
	u_char	*function_querynick _((u_char *));
#ifdef HAVE_STRFTIME
	u_char	*function_strftime _((u_char *));
#endif /* HAVE_STRFTIME */
/**************************** PATCHED by Flier ******************************/
#ifndef LITE
	u_char	*function_windows _((u_char *));
	u_char	*function_screens _((u_char *));
	u_char	*function_notify _((u_char *));
	u_char	*function_ignored _((u_char *));
	u_char	*function_urlencode _((u_char *));
	u_char	*function_chr _((u_char *));
	u_char	*function_shellfix _((u_char *));
	u_char	*function_filestat _((u_char *));
#endif
        u_char  *function_open _((u_char *));
        u_char  *function_close _((u_char *));
        u_char  *function_write _((u_char *));
        u_char  *function_read _((u_char *));
        u_char  *function_eof _((u_char *));
        u_char  *function_rename _((u_char *));
        u_char  *function_intuhost _((u_char *));
        u_char  *function_checkuser _((u_char *));
        u_char  *function_checkshit _((u_char *));
        u_char  *function_stripansi _((u_char *));
        u_char  *function_stripcrap _((u_char *));
#ifndef CELESCRP
	u_char	*function_isvoiced _((u_char *));
        u_char	*function_uhost _((u_char *));
	u_char	*function_hhost _((u_char *));
	u_char	*function_topic _((u_char *));
	u_char	*function_strlen _((u_char *));
        u_char	*function_strnum _((u_char *));
#endif
        u_char	*function_fsize _((u_char *));
        u_char	*function_chankey _((u_char *));
#ifndef CELESCRP
        u_char	*function_sar _((u_char *));
	u_char	*function_tr _((u_char *));
        u_char	*function_cdccslots _((u_char *));
        u_char	*function_cdccqslots _((u_char *));
        u_char	*function_url _((u_char *));
        u_char	*function_strstr _((u_char *));
        u_char	*function_szvar _((u_char *));
        u_char	*function_stamp _((u_char *));
#endif
#ifdef HAVE_REGCOMP
        u_char	*function_regexp _((u_char *));
        u_char	*function_regexpreplace _((u_char *));
#endif /* REGCOMP */
#ifdef COUNTRY
        u_char	*function_country _((u_char *));
#endif
#ifdef WANTANSI
        u_char  *function_color _((u_char *));
#endif
        u_char  *function_pattern _((u_char *));
    /*  u_char  *function_chops _((u_char *));
        u_char  *function_chnops _((u_char *));  */
/****************************************************************************/

typedef struct
{
	char	*name;
	u_char	*(*func) _((u_char *));
}	BuiltInFunctions;

static BuiltInFunctions	built_in_functions[] =
{
/**************************** Patched by Flier ******************************/
	{ "UH",                 function_intuhost },
#ifndef CELE
	{ "STAMP",              function_stamp },
#endif
/****************************************************************************/
	{ "LEFT",		function_left },
	{ "RIGHT",		function_right },
	{ "MID",		function_mid },
	{ "RAND",		function_rand },
	{ "SRAND",		function_srand },
	{ "TIME",		function_time },
#ifndef LITE
	{ "TDIFF",		function_tdiff },
	{ "STIME",		function_stime },
#endif
	{ "INDEX",		function_index },
	{ "RINDEX",		function_rindex },
/**************************** PATCHED by Flier ******************************/
#ifdef HAVE_REGCOMP
	{ "REGEXP",             function_regexp },
	{ "REGEXPREP",          function_regexpreplace },
#endif /* HAVE_REGCOMP */
/****************************************************************************/
	{ "MATCH",		function_match },
#ifndef LITE
	{ "RMATCH",		function_rmatch },
#endif
	{ "USERHOST",		function_userhost },
	{ "STRIP",		function_strip },
	{ "ENCODE",		function_encode },
	{ "DECODE",		function_decode },
/**************************** Patched by Flier ******************************/
	{ "STRIPANSI",          function_stripansi },
	{ "STRIPCRAP",          function_stripcrap },
#if !defined(CELESCRP) && !defined(LITE)
	{ "STRSTR",             function_strstr },
	{ "STRLEN",             function_strlen },
	{ "STRNUM",             function_strnum },
        { "UHOST",              function_uhost },
        { "HHOST",              function_hhost },
#endif
        { "CHECKUSER",          function_checkuser },
	{ "CHECKSHIT",          function_checkshit },
/****************************************************************************/
	{ "ISCHANNEL",		function_ischannel },
	{ "ISCHANOP",		function_ischanop },
#if defined(HAVE_CRYPT) && !defined(LITE)
	{ "CRYPT",		function_crypt },
#endif /* HAVE_CRYPT */
/**************************** PATCHED by Flier ******************************/
#if !defined(CELESCRP) && !defined(LITE)
 	{ "HASVOICE",		function_hasvoice },
#endif
/****************************************************************************/
#ifndef LITE
 	{ "DCCLIST",		function_dcclist },
 	{ "CHATPEERS",		function_chatpeers },
#endif
	{ "WORD",		function_word },
#ifndef LITE
	{ "WINNUM",		function_winnum },
	{ "WINNAM",		function_winnam },
 	{ "WINVIS",		function_winvisible },
	{ "WINROWS",		function_winrows },
	{ "WINCOLS",		function_wincols },
 	{ "WINSERVER",		function_winserver },
#endif
 	{ "WINSERVERGROUP",	function_winservergroup },
#ifndef LITE
	{ "CONNECT",		function_connect },
	{ "LISTEN",		function_listen },
#endif
	{ "TOUPPER",		function_toupper },
	{ "TOLOWER",		function_tolower },
	{ "MYCHANNELS",		function_channels },
#ifndef LITE
	{ "MYSERVERS",		function_servers },
	{ "SERVERTYPE",		function_servertype },
	{ "CURPOS",		function_curpos },
#endif
/**************************** PATCHED by Flier ******************************/
#if defined(WANTANSI) && !defined(LITE)
        { "COLOR",              function_color },
#endif
/****************************************************************************/
	{ "ONCHANNEL",		function_onchannel },
#ifndef LITE
	{ "PID",		function_pid },
	{ "PPID",		function_ppid },
#endif
/**************************** PATCHED by Flier ******************************/
#ifndef CELESCRP
        { "CHANUSERS",		function_chanusers },
#endif
/****************************************************************************/
#ifdef HAVE_STRFTIME
	{ "STRFTIME",		function_strftime },
#endif
	{ "IDLE",		function_idle },
 	{ "QUERYNICK",		function_querynick },
/**************************** PATCHED by Flier ******************************/
#ifndef LITE
	{ "WINDOWS",		function_windows },
	{ "SCREENS",		function_screens },
	{ "NOTIFY",		function_notify },
	{ "IGNORED",		function_ignored },
	{ "URLENCODE",		function_urlencode },
	{ "CHR",		function_chr },
	{ "SHELLFIX",		function_shellfix },
	{ "FILESTAT",		function_filestat },
#endif
	{ "PATTERN",            function_pattern },
	/*{ "CHOPS",              function_chops },
	{ "CHNOPS",             function_chnops },*/
#if !defined(CELESCRP) && !defined(LITE)
        { "SZVAR",              function_szvar },
        { "TOPIC",              function_topic },
        { "SAR",                function_sar },
        { "TR",                 function_tr },
#endif
#ifdef COUNTRY
        { "COUNTRY",            function_country },
#endif
#if !defined(CELESCRP) && !defined(LITE)
        { "URL",                function_url },
        { "CDCCSLOTS",          function_cdccslots },
        { "CDCCQSLOTS",         function_cdccqslots },
#endif
        { "CHANKEY",            function_chankey },
#ifndef LITE
        { "OPEN",               function_open },
        { "FSIZE",              function_fsize },
	{ "CLOSE",              function_close },
	{ "READ",               function_read },
	{ "WRITE",              function_write },
        { "EOF",                function_eof },
        { "RENAME",             function_rename },
#endif
/****************************************************************************/
	{ (char *) 0,		NULL }
};

/* alias_illegals: characters that are illegal in alias names */
	char	alias_illegals[] = " #+-*/\\()={}[]<>!@$%^~`,?;:|'\"";

static	Alias	*alias_list[] =
{
	(Alias *) 0,
	(Alias *) 0
};

/* alias_string: the thing that gets replaced by the $"..." construct */
static	char	*alias_string = (char *) 0;

static	int	eval_args;

/* function_stack and function_stkptr - hold the return values from functions */
static	u_char	* function_stack[128] =
{ 
	(u_char *) 0
};
static	int	function_stkptr = 0;

/*
 * find_alias: looks up name in in alias list.  Returns the Alias	entry if
 * found, or null if not found.   If do_unlink is set, the found entry is
 * removed from the list as well.  If match is null, only perfect matches
 * will return anything.  Otherwise, the number of matches will be returned. 
 */
static	Alias	*
find_alias(list, name, do_unlink, match)
	Alias	**list;
	char	*name;
	int	do_unlink;
	int	*match;
{
	Alias	*tmp,
		*last = (Alias *) 0;
	int	cmp;
	size_t	len;
 	int	(*cmp_func) _((char *, char *, size_t));

	if (match)
	{
		*match = 0;
		cmp_func = my_strnicmp;
	}
	else
 		cmp_func = (int (*) _((char *, char *, size_t))) my_stricmp;
	if (name)
	{
		len = strlen(name);
		for (tmp = *list; tmp; tmp = tmp->next)
		{
			if ((cmp = cmp_func(name, tmp->name, len)) == 0)
			{
				if (do_unlink)
				{
					if (last)
						last->next = tmp->next;
					else
						*list = tmp->next;
				}
				if (match)
				{
					(*match)++;
					if (strlen(tmp->name) == len)
					{
						*match = 0;
						return (tmp);
					}
				}
				else
					return (tmp);
			}
			else if (cmp < 0)
				break;
			last = tmp;
		}
	}
	if (match && (*match == 1))
		return (last);
	else
		return ((Alias *) 0);
}

/*
 * insert_alias: adds the given alias to the alias list.  The alias list is
 * alphabetized by name 
 */
static	void	
insert_alias(list, nalias)
	Alias	**list;
	Alias	*nalias;
{
	Alias	*tmp,
		*last,
		*foo;

	last = (Alias *) 0;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (strcmp(nalias->name, tmp->name) < 0)
			break;
		last = tmp;
	}
	if (last)
	{
		foo = last->next;
		last->next = nalias;
		nalias->next = foo;
	}
	else
	{
		nalias->next = *list;
		*list = nalias;
	}
}

/*
 * add_alias: given the alias name and "stuff" that makes up the alias,
 * add_alias() first checks to see if the alias name is already in use... if
 * so, the old alias is replaced with the new one.  If the alias is not
 * already in use, it is added. 
 */
void	
add_alias(type, name, stuff)
	int	type;
	char	*name,
		*stuff;
{
	Alias	*tmp;
	char	*ptr;

	upper(name);
	if (type == COMMAND_ALIAS)
		say("Alias	%s added", name);
	else
	{
		if (!strcmp(name, "FUNCTION_RETURN"))
		{
			if (function_stack[function_stkptr])
				new_free(&function_stack[function_stkptr]);
			malloc_strcpy((char **) &function_stack[function_stkptr], stuff);
			return;
		}
		if ((ptr = sindex(name, alias_illegals)) != NULL)
		{
			yell("Assign names may not contain '%c'", *ptr);
			return;
		}
		say("Assign %s added", name);
	}
	if ((tmp = find_alias(&(alias_list[type]), name, 1, (int *) 0)) ==
			(Alias *) 0)
	{
		tmp = (Alias *) new_malloc(sizeof(Alias));
		if (tmp == (Alias *) 0)
		{
			yell("Couldn't allocate memory for new alias!");
			return;
		}
		tmp->name = (char *) 0;
		tmp->stuff = (char *) 0;
	}
	malloc_strcpy(&(tmp->name), name);
        malloc_strcpy(&(tmp->stuff), stuff);
	tmp->mark = 0;
	tmp->global = loading_global;
	insert_alias(&(alias_list[type]), tmp);
}

/* alias_arg: a special version of next_arg for aliases */
static	char	*
alias_arg(str, pos)
	char	**str;
	u_int	*pos;
{
	char	*ptr;

	if (!*str)
		return (char *) 0;
	*pos = 0;
	ptr = *str;
	while (' ' == *ptr)
	{
		ptr++;
		(*pos)++;
	}
	if (*ptr == '\0')
	{
		*str = empty_string;
		return ((char *) 0);
	}
	if ((*str = sindex(ptr, " ")) != NULL)
		*((*str)++) = '\0';
	else
		*str = empty_string;
	return (ptr);
}

/* word_count: returns the number of words in the given string */
extern	int	
word_count(str)
	char	*str;
{
	int	cnt = 0;
	char	*ptr;

	while (1)
	{
		if ((ptr = sindex(str, "^ ")) != NULL)
		{
			cnt++;
			if ((str = sindex(ptr, " ")) == (char *) 0)
				return (cnt);
		}
		else
			return (cnt);
	}
}

static	u_char	*
built_in_alias(c)
	int	c;
{
	BuiltIns	*tmp;
	u_char	*ret = (u_char *) 0;

 	for (tmp = built_in; tmp->name; tmp++)
 		if ((char)c == tmp->name)
		{
			malloc_strcpy((char **) &ret, (char *) tmp->func());
			break;
		}
 	return (ret);
}

/*
 * find_inline: This simply looks up the given str.  It first checks to see
 * if its a user variable and returns it if so.  If not, it checks to see if
 * it's an IRC variable and returns it if so.  If not, it checks to see if
 * its and environment variable and returns it if so.  If not, it returns
 * null.  It mallocs the returned string 
 */
static	char	*
find_inline(str)
	char	*str;
{
	Alias	*nalias;
	char	*ret = NULL;
	char	*tmp;

 	if ((nalias = find_alias(&(alias_list[VAR_ALIAS]), str, 0, (int *) NULL))
			!= NULL)
	{
 		malloc_strcpy(&ret, nalias->stuff);
		return (ret);
	}
	if ((strlen(str) == 1) && (ret = (char *) built_in_alias(*str)))
		return(ret);
	if ((ret = make_string_var(str)) != NULL)
		return (ret);
#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	malloc_strcpy(&ret, (getuid() != DAEMON_UID) && (tmp = getenv(str)) ?
				tmp : empty_string);
#else
	malloc_strcpy(&ret, (tmp = getenv(str)) ? tmp : empty_string);
#endif /* DAEMON_UID */
	return (ret);
}

char	*
call_function(name, f_args, args, args_flag)
	char	*name,
		*f_args,
		*args;
	int	*args_flag;
{
	u_char	*tmp;
	u_char	*result = (u_char *) 0;
	char	*sub_buffer = (char *) 0;
	int	builtnum;
	char	*debug_copy = (char *) 0;
	char	*cmd = (char *) 0;

	malloc_strcpy(&cmd, name);
	upper(cmd);
	tmp = (u_char *) expand_alias((char *) 0, f_args, args, args_flag, NULL);
	if (get_int_var(DEBUG_VAR) & DEBUG_FUNCTIONS)
		malloc_strcpy(&debug_copy, (char *) tmp);
	for (builtnum = 0; built_in_functions[builtnum].name != NULL &&
			strcmp((char *) built_in_functions[builtnum].name, cmd);
			builtnum++)
		;
 	new_free(&cmd);
	if (built_in_functions[builtnum].name)
		result = built_in_functions[builtnum].func(tmp);
	else
	{
		sub_buffer = new_malloc(strlen(name)+strlen((char *) tmp)+2);
		strcpy(sub_buffer, name);
		strcat(sub_buffer, " ");
		strcat(sub_buffer, (char *) tmp);
		function_stack[++function_stkptr] = (u_char *) 0;
		parse_command(sub_buffer, 0, empty_string);
		new_free(&sub_buffer);
		eval_args=1;
		result = function_stack[function_stkptr];
		function_stack[function_stkptr] = (u_char *) 0;
		if (!result)
			malloc_strcpy((char **) &result, empty_string);
		function_stkptr--;
	}
	if (debug_copy)
	{
		yell("Function %s(%s) returned %s",
		    name, debug_copy, result);
		new_free(&debug_copy);
	}
	new_free(&tmp);
	return (result);
}


/* Given a pointer to an operator, find the last operator in the string */
static	char	*
lastop(ptr)
	char	*ptr;
{
	while (ptr[1] && index("!=<>&^|#+/-*", ptr[1]))
		ptr++;
	return ptr;
}

#define	NU_EXPR	0
#define	NU_CONJ NU_EXPR
#define	NU_ASSN	1
#define	NU_COMP 2
#define	NU_ADD  3
#define	NU_MULT 4
#define	NU_UNIT 5
#define	NU_TERT 6
#define	NU_BITW 8

static	char	*
next_unit(str, args, arg_flag, stage)
	char	*str,
		*args;
	int	*arg_flag,
		stage;
{
	char	*ptr,
		*ptr2,
		*right;
	int	got_sloshed = 0;
	char	*lastc;
	char	tmp[40];
	char	*result1 = (char *) 0,
		*result2 = (char *) 0;
	long	value1 = 0,
		value2,
		value3;
	char	op;
	int	display;
	char	*ArrayIndex,
		*EndIndex;

	while (isspace(*str))
		++str;
	if (!*str)
	{
		malloc_strcpy(&result1, empty_string);
		return result1;
	}
	lastc = str+strlen(str)-1;
	while (isspace(*lastc))
		*lastc-- = '\0';
	if (stage == NU_UNIT && *lastc == ')' && *str == '(')
	{
		str++, *lastc-- = '\0';
		return next_unit(str, args, arg_flag, NU_EXPR);
	}
	if (!*str)
	{
		malloc_strcpy(&result1, empty_string);
		return result1;
	}
	for (ptr = str; *ptr; ptr++)
	{
		if (got_sloshed) /* Help! I'm drunk! */
		{
			got_sloshed = 0;
			continue;
		}
		switch(*ptr)
		{
		case '\\':
			got_sloshed = 1;
			continue;
		case '(':
			if (stage != NU_UNIT || ptr == str)
			{
 				if (!(ptr2 = MatchingBracket(ptr+1, (int)'(', (int)')')))
					ptr = ptr+strlen(ptr)-1;
				else
					ptr = ptr2;
				break;
			}
			*ptr++ = '\0';
			right = ptr;
 			ptr = MatchingBracket(right, (int)LEFT_PAREN, (int)RIGHT_PAREN);
			if (ptr)
				*ptr++ = '\0';
			result1 = (char *)call_function(str, right, args, arg_flag);
			if (ptr && *ptr)
			{
				malloc_strcat(&result1, ptr);
				result2 = next_unit(result1, args, arg_flag,
						stage);
				new_free(&result1);
				result1 = result2;
			}
			return result1;
		case '[':
			if (stage != NU_UNIT)
			{
 				if (!(ptr2 = MatchingBracket(ptr+1, (int)'[', (int)']')))
					ptr = ptr+strlen(ptr)-1;
				else
					ptr = ptr2;
				break;
			}
			*ptr++ = '\0';
			right = ptr;
 			ptr = MatchingBracket(right, (int)LEFT_BRACKET, (int)RIGHT_BRACKET);
			if (ptr)
				*ptr++ = '\0';
			result1 = expand_alias((char *) 0, right, args, arg_flag, NULL);
			if (*str)
			{
				result2 = new_malloc(strlen(str)+
						(result1?strlen(result1):0)+
						(ptr?strlen(ptr):0) + 2);
				strcpy(result2, str);
				strcat(result2, ".");
				strcat(result2, result1);
				new_free(&result1);
				if (ptr && *ptr)
				{
					strcat(result2, ptr);
					result1 = next_unit(result2, args,
						arg_flag, stage);
				}
				else
				{
					result1 = find_inline(result2);
					if (!result1)
						malloc_strcpy(&result1,
							empty_string);
				}
				new_free(&result2);
			}
			else if (ptr && *ptr)
			{
				malloc_strcat(&result1, ptr);
				result2 = next_unit(result1, args, arg_flag,
					stage);
				new_free(&result1);
				result1 = result2;
			}
			return result1;
		case '-':
		case '+':
                        if (*(ptr+1) == *(ptr))  /* index operator */
                        {
                                char *tptr;

                                *ptr++ = '\0';
				if (ptr == str + 1)	/* Its a prefix */
                                {
                                        tptr = str + 2;
                                }
				else			/* Its a postfix */
                                {
                                        tptr = str;
                                }
			    	result1 = find_inline(tptr);
                                if (!result1) 
                                        malloc_strcpy(&result1, irczero);

                                {           /* This isnt supposed to be
                                                attached to the if, so
                                                dont "fix" it. */
                                        int r;
                                        r = atoi(result1);
                                        if (*ptr == '+')
                                                r++;
                                        else    r--;
                                        snprintf(tmp, sizeof tmp, "%d", r);
                                        display = window_display;
                                        window_display = 0;
                                        add_alias(VAR_ALIAS, tptr, tmp);
                                        window_display = display;
                                }
		/* A kludge?  Cheating?  Maybe.... */
				if (ptr == str + 1) 
				{
	                                *(ptr-1) = ' ';
					*ptr = ' ';
				} else            
				{
                                        if (*ptr == '+')
					        *(ptr-1) = '-';
                                        else
                                                *(ptr-1) = '+';
					*ptr = '1';
				}
                                ptr = str; 
                                new_free(&result1);
                                break;
                        }
			if (ptr == str) /* It's unary..... do nothing */
				break;
			if (stage != NU_ADD)
			{
				ptr = lastop(ptr);
				break;
			}
			op = *ptr;
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			value1 = atol(result1);
			value2 = atol(result2);
			new_free(&result1);
			new_free(&result2);
			if (op == '-')
				value3 = value1 - value2;
			else
				value3 = value1 + value2;
			snprintf(tmp, sizeof tmp, "%ld", value3);
			malloc_strcpy(&result1, tmp);
			return result1;
		case '/':
		case '*':
  		case '%':
			if (stage != NU_MULT)
			{
				ptr = lastop(ptr);
				break;
			}
			op = *ptr;
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			value1 = atol(result1);
			value2 = atol(result2);
			new_free(&result1);
			new_free(&result2);
			if (op == '/')
			{
				if (value2)
					value3 = value1 / value2;
				else
				{
					value3 = 0;
					say("Division by zero");
				}
			}
			else
                        if (op == '*')
				value3 = value1 * value2;
                        else
                        {
                                if (value2)
                                    value3 = value1 % value2;
                                else
                                {
                                        value3 = 0;
                                        say("Mod by zero");
                                }
                        }
			snprintf(tmp, sizeof tmp, "%ld", value3);
			malloc_strcpy(&result1, tmp);
			return result1;
		case '#':
			if (stage != NU_ADD || ptr[1] != '#')
			{
				ptr = lastop(ptr);
				break;
			}
			*ptr = '\0';
			ptr += 2;
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			malloc_strcat(&result1, result2);
			new_free(&result2);
			return result1;
	/* Reworked - Jeremy Nelson, Feb 1994
	 * & or && should both be supported, each with different
	 * stages, same with || and ^^.  Also, they should be
	 * short-circuit as well.
	 */
		case '&':
			if (ptr[0] == ptr[1])
			{
				if (stage != NU_CONJ)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 2;
				result1 = next_unit(str, args, arg_flag, stage);
				value1 = atol(result1);
				if (value1)
				{
					result2 = next_unit(ptr, args, arg_flag, stage);
					value2 = atol(result2);
					value3 = value1 && value2;
				}
				else
					value3 = 0;
				new_free(&result1);
				new_free(&result2);
                                tmp[0] = '0' + (value3?1:0);
                                tmp[1] = '\0';
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
			else
			{
				if (stage != NU_BITW)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 1;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				new_free(&result1);
				new_free(&result2);
				value3 = value1 & value2;
				snprintf(tmp, sizeof tmp, "%ld", value3);
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
		case '|':
			if (ptr[0] == ptr[1])
			{
				if (stage != NU_CONJ)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 2;
				result1 = next_unit(str, args, arg_flag, stage);
				value1 = atol(result1);
				if (!value1)
				{
					result2 = next_unit(ptr, args, arg_flag, stage);
					value2 = atol(result2);
					value3 = value1 || value2;
				}
				else	
					value3 = 1;
				new_free(&result1);
				new_free(&result2);
				tmp[0] = '0' + (value3 ? 1 : 0);
				tmp[1] = '\0';
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
			else
			{
				if (stage != NU_BITW)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 1;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				new_free(&result1);
				new_free(&result2);
				value3 = value1 | value2;
				snprintf(tmp, sizeof tmp, "%ld", value3);
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
		case '^':
			if (ptr[0] == ptr[1])
			{
				if (stage != NU_CONJ)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 2;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				value1 = value1?1:0;
				value2 = value2?1:0;
				value3 = value1 ^ value2;
				new_free(&result1);
				new_free(&result2);
				tmp[0] = '0' + (value3 ? 1 : 0);
				tmp[1] = '\0';
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
			else
			{
				if (stage != NU_BITW)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr = '\0';
				ptr += 1;
				result1 = next_unit(str, args, arg_flag, stage);
				result2 = next_unit(ptr, args, arg_flag, stage);
				value1 = atol(result1);
				value2 = atol(result2);
				new_free(&result1);
				new_free(&result2);
				value3 = value1 ^ value2;
				snprintf(tmp, sizeof tmp, "%ld", value3);
				malloc_strcpy(&result1, tmp);
 				return result1;
			}
                case '?':
                        if (stage != NU_TERT)
                        {
				ptr = lastop(ptr);
				break;
			}
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			ptr2 = index(ptr, ':');
			*ptr2++ = '\0';
                        right = result1;
                        value1 = parse_number(&right);
                        if ((value1 == -1) && (*right == (char) 0))
                                value1 = 0;
                        if ( value1 == 0 )
                                while (isspace(*right))
                                        *(right++) = '\0';
                        if ( value1 || *right )
				result2 = next_unit(ptr, args, arg_flag, stage);
			else
				result2 = next_unit(ptr2, args, arg_flag, stage);
                        *(ptr2-1) = ':';
			new_free(&result1);
			return result2;
		case '=':
			if (ptr[1] != '=')
			{
				if (stage != NU_ASSN)
				{
					ptr = lastop(ptr);
					break;
				}
				*ptr++ = '\0';
				result1 = expand_alias((char *) 0, str,
					args, arg_flag, NULL);
				result2 = next_unit(ptr, args, arg_flag, stage);
				display = window_display;
				window_display = 0;
				lastc = result1 + strlen(result1) - 1;
				while (lastc > result1 && *lastc == ' ')
					*lastc-- = '\0';
				for (ptr = result1; *ptr == ' '; ptr++);
				while ((ArrayIndex = (char *) index(ptr, '['))
						!= NULL)
				{
				    *ArrayIndex++='.';
				    if ((EndIndex = MatchingBracket(ArrayIndex,
 				        (int)LEFT_BRACKET, (int)RIGHT_BRACKET)) != NULL)
				    {
				        *EndIndex++='\0';
				        strcat(ptr, EndIndex);
				    }
				    else
				        break;
				}
				if (*ptr)
					add_alias(VAR_ALIAS, ptr, result2);
				else
					yell("Invalid assignment expression");
				window_display = display;
				new_free(&result1);
				return result2;
			}
			if (stage != NU_COMP)
			{
				ptr = lastop(ptr);
				break;
			}
			*ptr = '\0';
			ptr += 2;
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			if (!my_stricmp(result1, result2))
				malloc_strcpy(&result1, one);
			else
				malloc_strcpy(&result1, irczero);
			new_free(&result2);
			return result1;
		case '>':
		case '<':
			if (stage != NU_COMP)
			{
				ptr = lastop(ptr);
				break;
			}
			op = *ptr;
			if (ptr[1] == '=')
				value3 = 1, *ptr++ = '\0';
			else
				value3 = 0;
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			if (isdigit(*result1) && isdigit(*result2))
			{
				value1 = atol(result1);
				value2 = atol(result2);
				value1 = (value1 == value2) ? 0 : ((value1 <
					value2) ? -1 : 1);
			}
			else
				value1 = my_stricmp(result1, result2);
			if (value1)
			{
				value2 = (value1 > 0) ? 1 : 0;
				if (op == '<')
					value2 = 1 - value2;
			}
			else
				value2 = value3;
			new_free(&result2);
			snprintf(tmp, sizeof tmp, "%ld", value2);
			malloc_strcpy(&result1, tmp);
			return result1;
		case '~':
			if (ptr == str)
			{
				if (stage != NU_BITW)
					break;
				result1 = next_unit(str+1, args, arg_flag,
					stage);
				if (isdigit(*result1))
				{
					value1 = atol(result1);
					value2 = ~value1;
				}
				else
					value2 = 0;
				snprintf(tmp, sizeof tmp, "%ld", value2);
				malloc_strcpy(&result1, tmp);
				return result1;
			}
                        else
                        {
                                ptr = lastop(ptr);
                                break;
                        }
		case '!':
			if (ptr == str)
			{
				if (stage != NU_UNIT)
					break;
				result1 = next_unit(str+1, args, arg_flag,
					stage);
				if (isdigit(*result1))
				{
					value1 = atol(result1);
					value2 = value1 ? 0 : 1;
				}
				else
				{
					value2 = ((*result1)?0:1);
				}
				snprintf(tmp, sizeof tmp, "%ld", value2);
				malloc_strcpy(&result1, tmp);
				return result1;
			}
			if (stage != NU_COMP || ptr[1] != '=')
			{
				ptr = lastop(ptr);
				break;
			}
			*ptr = '\0';
			ptr += 2;
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			if (!my_stricmp(result1, result2))
				malloc_strcpy(&result1, irczero);
			else
				malloc_strcpy(&result1, one);
			new_free(&result2);
			return result1;
                case ',': 
			/*
			 * this utterly kludge code is needed (?) to get
			 * around bugs introduced from hop's patches to
			 * alias.c.  the $, variable stopped working
			 * because of this.  -mrg, july 94.
			 */
			if (ptr == str || (ptr > str && ptr[-1] == '$'))
				break;
                        if (stage != NU_EXPR)
                        {
				ptr = lastop(ptr);
				break;
			}
			*ptr++ = '\0';
			result1 = next_unit(str, args, arg_flag, stage);
			result2 = next_unit(ptr, args, arg_flag, stage);
			new_free(&result1);
			return result2;
		}
	}
	if (stage != NU_UNIT)
		return next_unit(str, args, arg_flag, stage+1);
	if (isdigit(*str) || *str == '+' || *str == '-')
		malloc_strcpy(&result1, str);
	else
	{
		if (*str == '#' || *str=='@')
			op = *str++;
		else
			op = '\0';
		result1 = find_inline(str);
		if (!result1)
			malloc_strcpy(&result1, empty_string);
		if (op)
		{
			if (op == '#')
				value1 = word_count(result1);
			else if (op == '@')
				value1 = strlen(result1);
			snprintf(tmp, sizeof tmp, "%ld", value1);
			malloc_strcpy(&result1, tmp);
		}
	}
	return result1;
}

/*
 * parse_inline:  This evaluates user-variable expression.  I'll talk more
 * about this at some future date. The ^ function and some fixes by
 * troy@cbme.unsw.EDU.AU (Troy Rollo) 
 */
char	*
parse_inline(str, args, args_flag)
	char	*str;
	char	*args;
	int	*args_flag;
{
	return next_unit(str, args, args_flag, NU_EXPR);
}

/*
 * arg_number: Returns the argument 'num' from 'str', or, if 'num' is
 * negative, returns from argument 'num' to the end of 'str'.  You might be
 * wondering what's going on down there... here goes.  First we copy 'str' to
 * malloced space.  Then, using next_arg(), we strip out each argument ,
 * putting them in arg_list, and putting their position in the original
 * string in arg_list_pos.  Anyway, once parsing is done, the arguments are
 * returned directly from the arg_list array... or in the case of negative
 * 'num', the arg_list_pos is used to return the postion of the rest of the
 * args in the original string... got it?  Anyway, the bad points of the
 * routine:  1) Always parses out everything, even if only one arg is used.
 * 2) The malloced stuff remains around until arg_number is called with a
 * different string. Even then, some malloced stuff remains around.  This can
 * be fixed. 
 */

#define	LAST_ARG 8000

static char *
arg_number(lower_lim, upper_lim, str)
int	lower_lim,
	upper_lim;
char	*str;
{
	char	*ptr,
		*arg,
		c;
	int	use_full = 0;
	unsigned int	pos,
		start_pos;
	static	char	*last_args = (char *) 0;
	static	char	*last_range = (char *) 0;
	static	char	**arg_list = (char **) 0;
	static	unsigned int	*arg_list_pos = (unsigned int *) 0;
	static	unsigned int	*arg_list_end_pos = (unsigned int *) 0;
	static	int	arg_list_size;

	if (eval_args)
	{
		int	arg_list_limit;

		eval_args = 0;
		new_free(&arg_list);
		new_free(&arg_list_pos);
		new_free(&arg_list_end_pos);
		arg_list_size = 0;
		arg_list_limit = 10;
		arg_list = (char **) new_malloc(sizeof(char *) *
			arg_list_limit);
		arg_list_pos = (unsigned int *) new_malloc(sizeof(unsigned int)
			* arg_list_limit);
		arg_list_end_pos = (unsigned int *) new_malloc(sizeof(unsigned
			int) * arg_list_limit);
		malloc_strcpy(&last_args, str);
		ptr = last_args;
		pos = 0;
		while ((arg = alias_arg(&ptr, &start_pos)) != NULL)
		{
			arg_list_pos[arg_list_size] = pos;
			pos += start_pos + strlen(arg);
			arg_list_end_pos[arg_list_size] = pos++;
			arg_list[arg_list_size++] = arg;
			if (arg_list_size == arg_list_limit)
			{
				arg_list_limit += 10;
				arg_list = (char **) new_realloc((char *) arg_list, sizeof(char *) * arg_list_limit);
				arg_list_pos = (unsigned int *) new_realloc((char *) arg_list_pos, sizeof(unsigned int) * arg_list_limit);
				arg_list_end_pos = (unsigned int *) new_realloc((char *) arg_list_end_pos, sizeof(unsigned int) * arg_list_limit);
			}
		}
	}
	if (upper_lim == LAST_ARG && lower_lim == LAST_ARG)
		upper_lim = lower_lim = arg_list_size - 1;
	if (arg_list_size == 0)
		return (empty_string);
	if ((upper_lim >= arg_list_size) || (upper_lim < 0))
	{
		use_full = 1;
		upper_lim = arg_list_size - 1;
	}
	if (upper_lim < lower_lim)
                return (empty_string);
	if (lower_lim >= arg_list_size)
		lower_lim = arg_list_size - 1;
	else if (lower_lim < 0)
		lower_lim = 0;
	if ((use_full == 0) && (lower_lim == upper_lim))
		return (arg_list[lower_lim]);
	c = *(str + arg_list_end_pos[upper_lim]);
	*(str + arg_list_end_pos[upper_lim]) = (char) 0;
	malloc_strcpy(&last_range, str + arg_list_pos[lower_lim]);
	*(str + arg_list_end_pos[upper_lim]) = c;
	return (last_range);
}

/*
 * parse_number: returns the next number found in a string and moves the
 * string pointer beyond that point	in the string.  Here's some examples: 
 *
 * "123harhar"  returns 123 and str as "harhar" 
 *
 * while: 
 *
 * "hoohar"     returns -1  and str as "hoohar" 
 */
extern	int	
parse_number(str)
	char	**str;
{
	int	ret;
	char	*ptr;

	ptr = *str;
	if (isdigit(*ptr))
	{
		ret = atoi(ptr);
		for (; isdigit(*ptr); ptr++);
		*str = ptr;
	}
	else
		ret = -1;
	return (ret);
}

static void	
do_alias_string()
{
	malloc_strcpy(&alias_string, get_input());
	irc_io_loop = 0;
}

/*
 * expander_addition: This handles string width formatting for irc variables
 * when [] is specified.  
 */
static	void	
expander_addition(buff, add, length, quote_em)
#ifndef USE_OLD_ALIAS_ALLOC
	char	**buff,
#else /* USE_OLD_ALIAS_ALLOC */
	char	*buff,
#endif /* USE_OLD_ALIAS_ALLOC */
		*add;
	int	length;
	char	*quote_em;
{
	char	format[40],
		buffer[BIG_BUFFER_SIZE+1],
		*ptr;

	if (length)
	{
		snprintf(format, sizeof format, "%%%d.%ds", -length, (length < 0 ? -length :
			length));
		snprintf(buffer, sizeof buffer, format, add);
		add = buffer;
	}
	if (quote_em)
	{
		ptr = double_quote(add, quote_em);
#ifndef USE_OLD_ALIAS_ALLOC
		malloc_strcat(buff, ptr);
#else /* USE_OLD_ALIAS_ALLOC */
		strmcat(buff, ptr, BIG_BUFFER_SIZE);
#endif /* USE_OLD_ALIAS_ALLOC */
		new_free(&ptr);
	}
#ifndef USE_OLD_ALIAS_ALLOC
	else if (add && *add)
	        malloc_strcat(buff, add);
#else /* USE_OLD_ALIAS_ALLOC */
	else
                if (buff)
		        strmcat(buff, add, BIG_BUFFER_SIZE);
#endif /* USE_OLD_ALIAS_ALLOC */
}

/* MatchingBracket returns the next unescaped bracket of the given type */
char	*
MatchingBracket(string, left, right)
	char	*string;
	int	left;
	int	right;
{
	int	bracket_count = 1;

	while (*string && bracket_count)
	{
 		if (*string == (char)left)
			bracket_count++;
 		else if (*string == (char)right)
		{
			if (!--bracket_count)
				return string;
		}
		else if (*string == '\\' && string[1])
			string++;
		string++;
	}
	return (char *) 0;
}


/*
 * alias_special_char: Here we determin what to do with the character after
 * the $ in a line of text. The special characters are described more fulling
 * in the help/ALIAS file.  But they are all handled here. Paremeters are the
 * name of the alias (if applicable) to prevent deadly recursion, a
 * destination buffer (that we are malloc_str*ing) to which things are appended,
 * a ptr to the string (the first character of which is the special
 * character, the args to the alias, and a character indication what
 * characters in the string should be quoted with a backslash).  It returns a
 * pointer to the character right after the converted alias.

 The args_flag is set to 1 if any of the $n, $n-, $n-m, $-m, $*, or $() is used
 in the alias.  Otherwise it is left unchanged.
 */
/*ARGSUSED*/
static	char	*
alias_special_char(name, lbuf, ptr, args, quote_em, args_flag)
	char	*name;
#ifndef USE_OLD_ALIAS_ALLOC
	char	**lbuf;
#else /* USE_OLD_ALIAS_ALLOC */
 	char	*lbuf;
#endif /* USE_OLD_ALIAS_ALLOC */
	char	*ptr;
	char	*args;
	char	*quote_em;
	int	*args_flag;
{
	char	*tmp,
		c;
 	int	is_upper,
 		is_lower,
 		length;

	length = 0;
	if ((c = *ptr) == LEFT_BRACKET)
	{
		ptr++;
		if ((tmp = (char *) index(ptr, RIGHT_BRACKET)) != NULL)
		{
			*(tmp++) = (char) 0;
			length = atoi(ptr);
#ifdef USE_OLD_ALIAS_ALLOC
 			/* XXX hack to avoid core dumps */
 			if (length > ((BIG_BUFFER_SIZE * 5) / 3))
 				length = ((BIG_BUFFER_SIZE * 5) / 3);
#endif /* USE_OLD_ALIAS_ALLOC */
			ptr = tmp;
			c = *ptr;
		}
		else
		{
			say("Missing %c", RIGHT_BRACKET);
			return (ptr);
		}
	}
	tmp = ptr+1;
	switch (c)
	{
	case LEFT_PAREN:
		{
#ifndef USE_OLD_ALIAS_ALLOC
			char	*sub_buffer = (char *) 0;
#else /* USE_OLD_ALIAS_ALLOC */
			char	sub_buffer[BIG_BUFFER_SIZE+1];
#endif /* USE_OLD_ALIAS_ALLOC */

 			if ((ptr = MatchingBracket(tmp, (int)LEFT_PAREN,
 			    (int)RIGHT_PAREN)) || (ptr = (char *) index(tmp,
			    RIGHT_PAREN)))
				*(ptr++) = (char) 0;
			tmp = expand_alias((char *) 0, tmp, args, args_flag,
				NULL);
#ifndef USE_OLD_ALIAS_ALLOC
			malloc_strcpy(&sub_buffer, empty_string);
			alias_special_char((char *) 0, &sub_buffer, tmp,
#else /* USE_OLD_ALIAS_ALLOC */
			*sub_buffer = (char) 0;
			alias_special_char((char *) 0, sub_buffer, tmp,
#endif /* USE_OLD_ALIAS_ALLOC */
				args, quote_em, args_flag);
 			expander_addition(lbuf, sub_buffer, length, quote_em);
#ifndef USE_OLD_ALIAS_ALLOC
			new_free(&sub_buffer);
#endif /* not USE_OLD_ALIAS_ALLOC */
			new_free(&tmp);
			*args_flag = 1;
		}
		return (ptr);
	case '!':
		if ((ptr = (char *) index(tmp, '!')) != NULL)
			*(ptr++) = (char) 0;
		if ((tmp = do_history(tmp, empty_string)) != NULL)
		{
 			expander_addition(lbuf, tmp, length, quote_em);
			new_free(&tmp);
		}
		return (ptr);
	case LEFT_BRACE:
		if ((ptr = (char *) index(tmp, RIGHT_BRACE)) != NULL)
			*(ptr++) = (char) 0;
		if ((tmp = parse_inline(tmp, args, args_flag)) != NULL)
		{
 			expander_addition(lbuf, tmp, length, quote_em);
			new_free(&tmp);
		}
		return (ptr);
	case DOUBLE_QUOTE:
		if ((ptr = (char *) index(tmp, DOUBLE_QUOTE)) != NULL)
			*(ptr++) = (char) 0;
		alias_string = (char *) 0;
			/* XXX - the cast in the following is an ugly hack! */
 		if (irc_io(tmp, (void (*) _((u_int, char *))) do_alias_string, use_input, 1))
		{
			yell("Illegal recursive edit");
			break;
		}
 		expander_addition(lbuf, alias_string, length, quote_em);
		new_free(&alias_string);
		return (ptr);
	case '*':
 		expander_addition(lbuf, args, length, quote_em);
		*args_flag = 1;
		return (ptr + 1);
	default:
		if (isdigit(c) || (c == '-') || c == '~')
		{
			*args_flag = 1;
			if (*ptr == '~')
			{
 				is_lower = is_upper = LAST_ARG;
				ptr++;
			}
			else
			{
 				is_lower = parse_number(&ptr);
				if (*ptr == '-')
				{
					ptr++;
 					is_upper = parse_number(&ptr);
				}
				else
 					is_upper = is_lower;
			}
 			expander_addition(lbuf, arg_number(is_lower, is_upper,
				args), length, quote_em);
			return (ptr ? ptr : empty_string);
		}
		else
		{
			char	*rest,
 				c2 = (char) 0;

		/*
		 * Why use ptr+1?  Cause try to maintain backward compatability
		 * can be a pain in the butt.  Basically, we don't want any of
		 * the illegal characters in the alias, except that things like
		 * $* and $, were around first, so they must remain legal.  So
		 * we skip the first char after the $.  Does this make sense?
		 */
			/* special case for $ */
			if (*ptr == '$')
			{
				rest = ptr+1;
 				c2 = *rest;
				*rest = (char) 0;
			}
			else if ((rest = sindex(ptr+1, alias_illegals)) != NULL)
			{
				if (isalpha(*ptr) || *ptr == '_')
					while ((*rest == LEFT_BRACKET ||
					    *rest == LEFT_PAREN) &&
					    (tmp = MatchingBracket(rest+1,
 					    (int)*rest, (int)(*rest == LEFT_BRACKET) ?
 					    RIGHT_BRACKET : RIGHT_PAREN)))
						rest = tmp + 1;
				c2 = *rest;
				*rest = (char) 0;
			}
			if ((tmp = parse_inline(ptr, args, args_flag)) != NULL)
			{
 				expander_addition(lbuf, tmp, length,
					quote_em);
				new_free(&tmp);
			}
			if (rest)
				*rest = c2;
			return(rest);
		}
	}
	return NULL;
}


/*
 * expand_alias: Expands inline variables in the given string and returns the
 * expanded string in a new string which is malloced by expand_alias(). 
 *
 * Also unescapes anything that was quoted with a backslash
 *
 * Behaviour is modified by the following:
 *	Anything between brackets (...) {...} is left unmodified.
 *	If more_text is supplied, the text is broken up at
 *		semi-colons and returned one at a time. The unprocessed
 *		portion is written back into more_text.
 *	Backslash escapes are unescaped.
 */

char	*
expand_alias(name, string, args, args_flag, more_text)
	char	*name,
		*string,
		*args;
	int	*args_flag;
	char	**more_text;
{
#ifndef USE_OLD_ALIAS_ALLOC
	char	*lbuf = (char *) 0,
#else /* USE_OLD_ALIAS_ALLOC */
	char	lbuf[BIG_BUFFER_SIZE + 1],
#endif /* USE_OLD_ALIAS_ALLOC */
		*ptr,
		*stuff = (char *) 0,
		*free_stuff;
	char	*quote_em,
		*quote_str = (char *) 0;
	char	ch;
	int	quote_cnt = 0;
	int	is_quote = 0;
#ifndef USE_OLD_ALIAS_ALLOC
	void	(*str_cat) _((char **, char *));
#else /* USE_OLD_ALIAS_ALLOC */
	void	(*str_cat) _((char *, char *, size_t));
#endif /* USE_OLD_ALIAS_ALLOC */

	if (*string == '@' && more_text)
	{
#ifndef USE_OLD_ALIAS_ALLOC
		str_cat = malloc_strcat;
#else /* USE_OLD_ALIAS_ALLOC */
		str_cat = strmcat;
#endif /* USE_OLD_ALIAS_ALLOC */
		*args_flag = 1; /* Stop the @ command from auto appending */
	}
	else
#ifndef USE_OLD_ALIAS_ALLOC
		str_cat = malloc_strcat_ue;
#else /* USE_OLD_ALIAS_ALLOC */
		str_cat = strmcat_ue;
#endif /* USE_OLD_ALIAS_ALLOC */
	malloc_strcpy(&stuff, string);
	free_stuff = stuff;
#ifndef USE_OLD_ALIAS_ALLOC
	malloc_strcpy(&lbuf, empty_string);
#else /* USE_OLD_ALIAS_ALLOC */
	*lbuf = (char) 0;
#endif /* USE_OLD_ALIAS_ALLOC */
	eval_args = 1;
	ptr = stuff;
	if (more_text)
		*more_text = NULL;
	while (ptr && *ptr)
	{
		if (is_quote)
		{
			is_quote = 0;
			++ptr;
			continue;
		}
		switch(*ptr)
		{
		case '$':
	/*
	 * The test here ensures that if we are in the expression
	 * evaluation command, we don't expand $. In this case we
	 * are only coming here to do command separation at ';'s.
	 * If more_text is not defined, and the first character is
	 * '@', we have come here from [] in an expression.
	 */
			if (more_text && *string == '@')
			{
				ptr++;
				break;
			}
			*(ptr++) = (char) 0;
#ifndef USE_OLD_ALIAS_ALLOC
			(*str_cat)(&lbuf, stuff);
#else /* USE_OLD_ALIAS_ALLOC */
			(*str_cat)(lbuf, stuff, BIG_BUFFER_SIZE);
#endif /* USE_OLD_ALIAS_ALLOC */
			while (*ptr == '^')
			{
				ptr++;
				if (quote_str)
					quote_str = (char *)
						new_realloc(quote_str,
						sizeof(char) * (quote_cnt + 2));
				else
					quote_str = (char *)
						new_malloc(sizeof(char) *
						(quote_cnt + 2));
				quote_str[quote_cnt++] = *(ptr++);
				quote_str[quote_cnt] = (char) 0;
			}
			quote_em = quote_str;
#ifndef USE_OLD_ALIAS_ALLOC
			stuff = alias_special_char(name, &lbuf, ptr, args,
#else /* USE_OLD_ALIAS_ALLOC */
			stuff = alias_special_char(name, lbuf, ptr, args,
#endif /* USE_OLD_ALIAS_ALLOC */
				quote_em, args_flag);
			if (stuff)
				new_free(&quote_str);
			quote_cnt = 0;
			ptr = stuff;
			break;
		case ';':
			if (!more_text)
			{
				ptr++;
				break;
			}
			*more_text = string + (ptr - free_stuff) +1;
			*ptr = '\0'; /* To terminate the loop */
			break;
		case LEFT_PAREN:
		case LEFT_BRACE:
			ch = *ptr;
			*ptr = '\0';
#ifndef USE_OLD_ALIAS_ALLOC
			(*str_cat)(&lbuf, stuff);
#else /* USE_OLD_ALIAS_ALLOC */
			(*str_cat)(lbuf, stuff, BIG_BUFFER_SIZE);
#endif /* USE_OLD_ALIAS_ALLOC */
			stuff = ptr;
			*args_flag = 1;
 			if (!(ptr = MatchingBracket(stuff + 1, (int)ch,
 					(int)(ch == LEFT_PAREN) ?
					RIGHT_PAREN : RIGHT_BRACE)))
			{
				yell("Unmatched %c", ch);
				ptr = stuff + strlen(stuff+1)+1;
			}
			else
				ptr++;
			*stuff = ch;
			ch = *ptr;
			*ptr = '\0';
#ifndef USE_OLD_ALIAS_ALLOC
			malloc_strcat(&lbuf, stuff);
#else /* USE_OLD_ALIAS_ALLOC */
			strmcat(lbuf, stuff, BIG_BUFFER_SIZE);
#endif /* USE_OLD_ALIAS_ALLOC */
			stuff = ptr;
			*ptr = ch;
			break;
		case '\\':
			is_quote = 1;
			ptr++;
			break;
		default:
			ptr++;
			break;
		}
	}
	if (stuff)
#ifndef USE_OLD_ALIAS_ALLOC
		(*str_cat)(&lbuf, stuff);
#else /* USE_OLD_ALIAS_ALLOC */
		(*str_cat)(lbuf, stuff, BIG_BUFFER_SIZE);
#endif /* USE_OLD_ALIAS_ALLOC */
	ptr = (char *) 0;
	new_free(&free_stuff);
#ifndef USE_OLD_ALIAS_ALLOC
	ptr = lbuf;
#else /* USE_OLD_ALIAS_ALLOC */
	malloc_strcpy(&ptr, lbuf);
#endif /* USE_OLD_ALIAS_ALLOC */
	if (get_int_var(DEBUG_VAR) & DEBUG_EXPANSIONS)
		yell("Expanded [%s] to [%s]",
			string, ptr);
	return (ptr);
}

/*
 * get_alias: returns the alias matching 'name' as the function value. 'args'
 * are expanded as needed, etc.  If no matching alias is found, null is
 * returned, cnt is 0, and full_name is null.  If one matching alias is
 * found, it is retuned, with cnt set to 1 and full_name set to the full name
 * of the alias.  If more than 1 match are found, null is returned, cnt is
 * set to the number of matches, and fullname is null. NOTE: get_alias()
 * mallocs the space for the full_name, but returns the actual value of the
 * alias if found! 
 */
char	*
get_alias(type, name, cnt, full_name)
	int	type;
	char	*name,
		**full_name;
	int	*cnt;
{
	Alias	*tmp;

	*full_name = (char *) 0;
	if ((name == (char *) 0) || (*name == (char) 0))
	{
		*cnt = 0;
		return ((char *) 0);
	}
	if ((tmp = find_alias(&(alias_list[type]), name, 0, cnt)) != NULL)
	{
		if (*cnt < 2)
		{
			malloc_strcpy(full_name, tmp->name);
			return (tmp->stuff);
		}
	}
	return ((char *) 0);
}

/*
 * match_alias: this returns a list of alias names that match the given name.
 * This is used for command completion etc.  Note that the returned array is
 * malloced in this routine.  Returns null if no matches are found 
 */
char	**
match_alias(name, cnt, type)
	char	*name;
	int	*cnt;
	int	type;
{
	Alias	*tmp;
	char	**matches = (char **) 0;
	int	matches_size = 5;
 	size_t	len;
	char	*last_match = (char *) 0;
	char	*dot;

	len = strlen(name);
	*cnt = 0;
	matches = (char	**) new_malloc(sizeof(char *) * matches_size);
	for (tmp = alias_list[type]; tmp; tmp = tmp->next)
	{
		if (strncmp(name, tmp->name, len) == 0)
		{
			if ((dot = (char *) index(tmp->name+len, '.')) != NULL)
			{
				if (type == COMMAND_ALIAS)
					continue;
				else
				{
					*dot = '\0';
					if (last_match && !strcmp(last_match,
							tmp->name))
					{
						*dot = '.';
						continue;
					}
				}
			}
			matches[*cnt] = (char *) 0;
			malloc_strcpy(&(matches[*cnt]), tmp->name);
			last_match = matches[*cnt];
			if (dot)
				*dot = '.';
			if (++(*cnt) == matches_size)
			{
				matches_size += 5;
				matches = (char	**) new_realloc((char *) matches, sizeof(char *) * matches_size);
			}
		}
		else if (*cnt)
			break;
	}
	if (*cnt)
	{
		matches = (char	**) new_realloc((char *) matches, sizeof(char *) * (*cnt + 1));
		matches[*cnt] = (char *) 0;
	}
	else
		new_free(&matches);
	return (matches);
}

/* delete_alias: The alias name is removed from the alias list. */
void
delete_alias(type, name)
	int	type;
	char	*name;
{
	Alias	*tmp;

	upper(name);
	if ((tmp = find_alias(&(alias_list[type]), name, 1, (int *) NULL))
			!= NULL)
	{
		new_free(&(tmp->name));
		new_free(&(tmp->stuff));
		new_free(&tmp);
		if (type == COMMAND_ALIAS)
			say("Alias	%s removed", name);
		else
			say("Assign %s removed", name);
	}
	else
		say("No such alias: %s", name);
}

/*
 * list_aliases: Lists all aliases matching 'name'.  If name is null, all
 * aliases are listed 
 */
void
list_aliases(type, name)
	int	type;
	char	*name;
{
	Alias	*tmp;
 	size_t	len;
 	int	lastlog_level;
 	size_t	DotLoc,
		LastDotLoc = 0;
	char	*LastStructName = NULL;
	char	*s;

 	lastlog_level = message_from_level(LOG_CRAP);
	if (type == COMMAND_ALIAS)
		say("Aliases:");
	else
		say("Assigns:");
	if (name)
	{
		upper(name);
		len = strlen(name);
	}
	else
		len = 0;
	for (tmp = alias_list[type]; tmp; tmp = tmp->next)

	{
		if (!name || !strncmp(tmp->name, name, len))
		{
			s = index(tmp->name + len, '.');
			if (!s)
				say("\t%s\t%s", tmp->name, tmp->stuff);
			else
			{
				DotLoc = s - tmp->name;
				if (!LastStructName || (DotLoc != LastDotLoc) || strncmp(tmp->name, LastStructName, DotLoc))
				{
					say("\t%*.*s\t<Structure>", DotLoc, DotLoc, tmp->name);
					LastStructName = tmp->name;
					LastDotLoc = DotLoc;
				}
			}
		}
	}
 	(void)message_from_level(lastlog_level);
}

/*
 * mark_alias: sets the mark field of the given alias to 'flag', and returns
 * the previous value of the mark.  If the name is not found, -1 is returned.
 * This is used to prevent recursive aliases by marking and unmarking
 * aliases, and not reusing an alias that has previously been marked.  I'll
 * explain later 
 */
int
mark_alias(name, flag)
	char	*name;
	int	flag;
{
	int	old_mark;
	Alias	*tmp;
	int	match;

	if ((tmp = find_alias(&(alias_list[COMMAND_ALIAS]), name, 0, &match))
			!= NULL)
	{
		if (match < 2)
		{
			old_mark = tmp->mark;
		/* New handling of recursion */
			if (flag)
			{
				int	i;
				/* Count recursion */

				tmp->mark = tmp->mark + flag;
				if ((i = get_int_var(MAX_RECURSIONS_VAR)) > 1)
				{
					if (tmp->mark > i)
					{
						tmp->mark = 0;
						return(1); /* MAX exceeded. */
					}
					else return(0);
				/* In recursion but it's ok */
				}
				else
				{
					if (tmp->mark > 1)
					{
						tmp->mark = 0;
						return(1);
				/* max of 1 here.. exceeded */
					}
					else return(0);
				/* In recursion but it's ok */
				}
			}
			else
		/* Not in recursion at all */
			{
				tmp->mark = 0;
				return(old_mark);
			/* This one gets ignored anyway */
			}
		}
	}
	return (-1);
}

/*
 * execute_alias: After an alias has been identified and expanded, it is sent
 * here for proper execution.  This routine mainly prevents recursive
 * aliasing.  The name is the full name of the alias, and the alias is
 * already expanded alias (both of these parameters are returned by
 * get_alias()) 
 */
void
execute_alias(alias_name, ealias, args)
	char	*alias_name,
		*ealias,
		*args;
{
	if (mark_alias(alias_name, 1))
		say("Maximum recursion count exceeded in: %s", alias_name);
	else
	{
		parse_line(alias_name, ealias, args, 0, 1, 0);
		mark_alias(alias_name, 0);
	}
}

/*
 * save_aliases: This will write all of the aliases to the FILE pointer fp in
 * such a way that they can be read back in using LOAD or the -l switch 
 */
void
save_aliases(fp, do_all)
	FILE	*fp;
	int	do_all;
{
	Alias	*tmp;

	for (tmp = alias_list[VAR_ALIAS]; tmp; tmp = tmp->next)
		if (!tmp->global || do_all)
			fprintf(fp, "ASSIGN %s %s\n", tmp->name, tmp->stuff);
	for (tmp = alias_list[COMMAND_ALIAS]; tmp; tmp = tmp->next)
		if (!tmp->global || do_all)
			fprintf(fp, "ALIAS %s %s\n", tmp->name, tmp->stuff);
}

/* The Built-In Alias expando functions */
#ifndef LITE
static	u_char	*
alias_line()
{
	return ((u_char *) get_input());
}
#endif

static	u_char	*
alias_buffer()
{
	return ((u_char *) cut_buffer);
}

static	u_char	*
alias_time()
{
	static char timestr[16];

	return ((u_char *) update_clock(timestr, 16, GET_TIME));
}

static	u_char	*
alias_dollar()
{
	return ((u_char *) "$");
}

static	u_char	*
alias_detected()
{
	return (last_notify_nick);
}

static	u_char	*
alias_nick()
{
	return (u_char *) ((from_server >= 0) ? get_server_nickname(from_server) : nickname);
}

static	u_char	*
alias_away()
{
	return (u_char *) ((from_server >= 0) ? server_list[from_server].away : empty_string);
}

static	u_char	*
alias_sent_nick()
{
	return (sent_nick) ? sent_nick : (u_char *) empty_string;
}

static	u_char	*
alias_recv_nick()
{
	return (recv_nick) ? recv_nick : (u_char *) empty_string;
}

#ifndef LITE
static	u_char	*
alias_msg_body()
{
	return (sent_body) ? sent_body : (u_char *) empty_string;
}
#endif

static	u_char	*
alias_joined_nick()
{
	return (joined_nick) ? joined_nick : (u_char *) empty_string;
}

static	u_char	*
alias_public_nick()
{
	return (public_nick) ? public_nick : (u_char *) empty_string;
}

static	u_char	*
alias_channel()
{
	char	*tmp;

	return (u_char *) ((tmp = get_channel_by_refnum(0)) ? tmp : irczero);
}

static	u_char	*
alias_server()
{
	return (u_char *) ((parsing_server_index != -1) ?
		get_server_itsname(parsing_server_index) :
		(get_window_server(0) != -1) ?
 			get_server_itsname(get_window_server(0)) : empty_string);
}

static	u_char	*
alias_query_nick()
{
	char	*tmp;

	return (u_char *) ((tmp = query_nick()) ? tmp : empty_string);
}

static	u_char	*
alias_target()
{
	char	*tmp;

	return (u_char *) ((tmp = get_target_by_refnum(0)) ? tmp : empty_string);
}

static	u_char	*
alias_invite()
{
	return (u_char *) ((invite_channel) ? invite_channel : empty_string);
}

static	u_char	*
alias_cmdchar()
{
	static	u_char	thing[2];
	u_char	*cmdu_chars;

	if ((cmdu_chars = get_string_var(CMDCHARS_VAR)) == (u_char *) 0)
		cmdu_chars = DEFAULT_CMDCHARS;
	thing[0] = cmdu_chars[0];
	thing[1] = (u_char) 0;
 	return (thing);
}

static	u_char	*
alias_oper()
{
	return (u_char *) ((from_server >= 0 && get_server_operator(from_server)) ?
		get_string_var(STATUS_OPER_VAR) : empty_string);
}

static	u_char	*
alias_chanop()
{
	u_char	*tmp;

	return (u_char *) ((from_server >= 0 && (tmp = get_channel_by_refnum(0)) &&
 			get_channel_oper(tmp, from_server)) ?
 		"@" : empty_string);
}

static	u_char	*
alias_modes()
{
	u_char	*tmp;

	return (u_char *) ((from_server >= 0 && (tmp = get_channel_by_refnum(0))) ?
		get_channel_mode(tmp, from_server) : empty_string);
}

static	u_char	*
alias_version()
{
	return  (irc_version);
}

static	u_char	*
alias_currdir()
{
	static	u_char	dirbuf[1024];

	getcwd(CP(dirbuf), 1024);
	return (dirbuf);
}

static	u_char	*
alias_current_numeric()
{
	static	u_char	number[4];

	snprintf(CP(number), sizeof number, "%03d", -current_numeric);
	return (number);
}

static	u_char	*
alias_server_version()
{
	char	*s;

	return (u_char *) ((curr_scr_win->server >= 0 &&
 			(s = server_list[curr_scr_win->server].version_string)) ?
		s : empty_string);
}

/**************************** PATCHED by Flier ******************************/
static u_char *alias_ScrollZ_version() {
    static u_char tmpbuf[mybufsize / 16];

    snprintf((char *) tmpbuf, sizeof(tmpbuf), "%s [%s]", ScrollZver1, VersionInfo);
    return(tmpbuf);
}

#ifdef CELE
static u_char *alias_Celerity_version() {
    static u_char tmpbuf[32];

    snprintf((char *) tmpbuf, sizeof(tmpbuf), "%s", CelerityVersion);
    return(tmpbuf);
}
#endif
/****************************************************************************/

/*
 * alias: the /ALIAS command.  Calls the correct alias function depending on
 * the args 
 */
void	
alias(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*name,
		*rest;
	int	type;
	char	*ArrayIndex;
	char	*EndIndex;

	type = *command - 48;	/*
				 * A trick!  Yes, well, what the hell.  Note
                                 * the the command part of ALIAS is "0" and
                                 * the command part of ASSIGN is "1" in the
				 * command array list
				 */
	if ((name = next_arg(args, &rest)) != NULL)
	{
		while ((ArrayIndex = (char *) index(name, '[')) != NULL)
		{
			*ArrayIndex++ = '.';
			if ((EndIndex = MatchingBracket(ArrayIndex,
 					(int)LEFT_BRACKET, (int)RIGHT_BRACKET)) != NULL)
			{
				*EndIndex++ = '\0';
				strcat(name, EndIndex);
			}
			else
				break;
		}
		if (*rest)
		{
			if (*rest == LEFT_BRACE)
			{
				char	*ptr;

				ptr = MatchingBracket(++rest,
 						(int)LEFT_BRACE, (int)RIGHT_BRACE);
				if (!ptr)
				    say("Unmatched brace in ALIAS or ASSIGN");
				else if (ptr[1])
				{
					say("Junk after closing brace in ALIAS or ASSIGN");
				}
				else
				{
					*ptr = '\0';
					add_alias(type, name, rest);
				}
			}
			else
				add_alias(type, name, rest);
		}
		else
		{
			if (*name == '-')
			{
				if (*(name + 1))
					delete_alias(type, name + 1);
				else
					say("You must specify an alias to be removed");
			}
			else
				list_aliases(type, name);
		}
	}
	else
		list_aliases(type, (char *) 0);
}



u_char	*
function_left(input)
 	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*count;
	int	cvalue;

	count = next_arg((char *) input, (char **) &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if ((int) strlen((char *) input) > cvalue)
		input[cvalue] = '\0';
	malloc_strcpy((char **) &result, (char *) input);
	return (result);
}

u_char	*
function_right(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*count;
	int	cvalue;
	int	len;

	count = next_arg((char *) input, (char **) &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if ((len = (int) strlen((char *) input)) > cvalue)
		input += len - cvalue;
	malloc_strcpy((char **) &result, (char *) input);
	return (result);
}

u_char	*
function_mid(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*mid_index;
	int	ivalue;
	char	*count;
	int	cvalue;

	mid_index = next_arg((char *) input, (char **) &input);
	if (mid_index)
		ivalue = atoi(mid_index);
	else
		ivalue = 0;
	count = next_arg((char *) input, (char **) &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (ivalue >= 0 && (int) strlen(CP(input)) > ivalue)
		input += ivalue;
	else
		*input = '\0';
	if (cvalue > 0 && (int) strlen(CP(input)) > cvalue)
		input[cvalue] = '\0';
	malloc_strcpy((char **) &result, (char *) input);
	return (result);
}

/* patch from Sarayan to make $rand() better */

/**************************** PATCHED by Flier ******************************/
/*#define RAND_A 16807L
#define RAND_M 2147483647L
#define RAND_Q 127773L
#define RAND_R 2836L

static	long	
randm(l)
	long	l;
{
	static	u_long	z = 0;
	long	t;

#ifndef __MSDOS__
	if (!z)
		z = (u_long) getuid();
#endif*/ /* __MSDOS__ */
/*	if (!l)
	{
		t = RAND_A * (z % RAND_Q) - RAND_R * (z / RAND_Q);
		if (t > 0)
			z = t;
		else
			z = t + RAND_M;
		return (z >> 8) | ((z & 255) << 23);
	}
	else
	{
		if (l < 0)
#ifdef __MSDOS__
 			z = 0;
#else
  			z = (u_long) getuid();
#endif*/ /* __MSDOS__ */
/*		else
			z = l;
		return 0;
	}
}*/
/****************************************************************************/

u_char	*
function_rand(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	tmp[40];
/**************************** PATCHED by Flier ******************************/
	/*long	tempin;*/
        int     value = 0;
/****************************************************************************/

#ifdef _Windows
	snprintf(tmp, sizeof tmp, "%ld", random(atol(CP(input))));
#else
/**************************** PATCHED by Flier ******************************/
	/*snprintf(CP(tmp), sizeof tmp, "%ld", (tempin = my_atol(input)) ? randm(0L) % tempin : 0);*/
        if (input && * input) value = atoi(input);
        if (!value) value = 1;
        snprintf(tmp, sizeof(tmp), "%d", (input && *input) ? rand() % value : rand());
/****************************************************************************/
#endif /* _Windows */
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

u_char	*
function_srand(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;

/**************************** PATCHED by Flier ******************************/
	/*if (input && *input)
		(void) randm(atol((char *) input));
	else
		(void) randm((long) time(NULL));*/
	if (input && *input) srand(atol((char *) input));
	else srand(time((time_t *) 0));
/****************************************************************************/
	malloc_strcpy((char **) &result, empty_string);
	return (result);
}

/*ARGSUSED*/
u_char	*
function_time(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	time_t	ltime;
	char	tmp[40];

	(void) time(&ltime);
	snprintf(tmp, sizeof tmp, "%ld", (long)ltime);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

#ifndef LITE
u_char	*
function_stime(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	time_t	ltime;

	ltime = atol((char *) input);
	malloc_strcpy((char **) &result, ctime(&ltime));
	result[strlen((char *) result) - 1] = (u_char) 0;
	return (result);
}

u_char	*
function_tdiff(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	time_t	ltime;
	time_t	days,
		hours,
		minutes,
		seconds;
	char	tmp[80];
	char	*tstr;

	ltime = atol((char *) input);
	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime%60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	tstr = tmp;
	if (days)
	{
		snprintf(tstr, sizeof(tmp) - (tstr - tmp), "%ld day%s ", (long) days, (days==1)?"":"s");
		tstr += strlen(tstr);
	}
	if (hours)
	{
		snprintf(tstr, sizeof(tmp) - (tstr - tmp), "%ld hour%s ", (long) hours, (hours==1)?"":"s");
		tstr += strlen(tstr);
	}
	if (minutes)
	{
		snprintf(tstr, sizeof(tmp) - (tstr - tmp), "%ld minute%s ", (long) minutes, (minutes==1)?"":"s");
		tstr += strlen(tstr);
	}
	if (seconds || (!days && !hours && !minutes))
	{
		snprintf(tstr, sizeof(tmp) - (tstr - tmp), "%ld second%s", (long) seconds, (seconds==1)?"":"s");
		tstr += strlen(tstr);
	}
	malloc_strcpy((char **) &result, tmp);
	return (result);
}
#endif

u_char	*
function_index(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*schars;
	char	*iloc;
	int	ival;
	char	tmp[40];

	schars = next_arg((char *) input, (char **) &input);
	iloc = (schars) ? sindex((char *) input, schars) : NULL;
	ival = (iloc) ? iloc - (char *) input : -1;
	snprintf(tmp, sizeof tmp, "%d", ival);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

u_char	*
function_rindex(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*schars;
	char	*iloc;
	int	ival;
	char	tmp[40];

	schars = next_arg((char *) input, (char **) &input);
 	iloc = (schars) ? srindex((char *) input, schars) : NULL;
 	ival = (iloc) ? iloc - (char *) input : -1;
	snprintf(tmp, sizeof tmp, "%d", ival);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

u_char	*
function_match(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*pattern;
	char	*word;
	int	current_match;
	int	best_match = 0;
	int	match = 0;
	int	match_index = 0;
	char	tmp[40];

	if ((pattern = next_arg((char *) input, (char **) &input)) != NULL)
	{
		while ((word = next_arg((char *) input, (char **) &input)) != NULL)
		{
			match_index++;
			if ((current_match = wild_match(pattern, word))
					> best_match)
			{
				match = match_index;
				best_match = current_match;
			}
		}
	}
	snprintf(tmp, sizeof tmp, "%d", match);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

#ifndef LITE
u_char	*
function_rmatch(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*pattern;
	char	*word;
	int	current_match;
	int	best_match = 0;
	int	match = 0;
	int	rmatch_index = 0;
	char	tmp[40];

	if ((pattern = next_arg((char *) input, (char **) &input)) != NULL)
	{
		while ((word = next_arg((char *) input, (char **) &input)) != NULL)
		{
			rmatch_index++;
			if ((current_match = wild_match(word, pattern)) >
					best_match)
			{
				match = rmatch_index;
				best_match = current_match;
			}
		}
	}
	snprintf(tmp, sizeof tmp, "%d", match);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}
#endif

/*ARGSUSED*/
u_char	*
function_userhost(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;

	malloc_strcpy((char **) &result, FromUserHost ? FromUserHost : empty_string);
	return (result);
}

u_char	*
function_strip(input)
	u_char	*input;
{
	char	tmpbuf[128], *result;
	u_char	*retval = (u_char *) 0;
	char	*chars;
	char	*cp, *dp;
	size_t len = 0;

	if ((chars = next_arg((char *) input, (char **) &input)) && input)
	{
		len = strlen((char *) input);
		if (len > 127)
			result = (char *) new_malloc(len + 1);
		else
			result = tmpbuf;

		for (cp = (char *) input, dp = result; *cp; cp++)
		{
			if (!index(chars, *cp))
				*dp++ = *cp;
		}
		*dp = '\0';
	}
	malloc_strcpy((char **) &retval, result);
	if (len > 127)
		new_free(&result);	/* we could use this copy, but it might be extra-long */
	return (retval);
}

u_char	*
function_encode(input)
	u_char	*input;
{
	u_char	*result;
	u_char	*c;
	int	i = 0;

 	result = (u_char *) new_malloc(strlen((char *) input) * 2 + 1);
	for (c = input; *c; c++)
	{
		result[i++] = (*c >> 4) + 0x41;
		result[i++] = (*c & 0x0f) + 0x41;
	}
	result[i] = '\0';
 	return (result);
}


u_char	*
function_decode(input)
	u_char	*input;
{
	u_char	*result;
	u_char	*c;
	u_char	d, e;
	int	i = 0;

	c = input;
	result = (u_char *) new_malloc(strlen((char *) input) / 2 + 2);
	while((d = *c) && (e = *(c+1)))
	{
		result[i] = ((d - 0x41) << 4) | (e - 0x41);
		c += 2;
		i++;
	}
	result[i] = '\0';
	return (result);
}

u_char	*
function_ischannel(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;

	malloc_strcpy((char **) &result, is_channel((char *) input) ? one : irczero);
	return (result);
}

u_char	*
function_ischanop(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*nick;
	char	*channel = NULL;

	if (!(nick = next_arg((char *) input, &channel)))
		malloc_strcpy((char **) &result, irczero);
	else
		malloc_strcpy((char **) &result, is_chanop(channel, nick) ? one : irczero);
	return (result);
}

#ifndef LITE
#ifdef HAVE_CRYPT

u_char *
function_crypt(input)
	u_char *input;
{
	u_char	*result = (char *)0;
	char	*salt;
	char	*key = NULL;

	if (!(salt = next_arg((char *) input, &key)))
		malloc_strcpy((char **) &result, irczero);
	else
		malloc_strcpy((char **) &result, (char *) crypt(key, salt));
	return (result);
}
#endif /* HAVE_CRYPT */

u_char *
function_hasvoice(input)
	u_char *input;
{
	u_char	*result = (u_char *)0;
	char	*nick;
	char	*channel = NULL;

	if (!(nick = next_arg((char *) input, &channel)))
		malloc_strcpy((char **) &result, irczero);
	else
		malloc_strcpy((char **) &result, has_voice(channel, nick, from_server) ? one : irczero);
	return (result);
}

u_char *
function_dcclist(Nick)
	u_char *Nick;
{
	u_char *result;
	DCC_list *Client;
	size_t len = 0;
	int i;
	
	if (!Nick)
	{
		malloc_strcpy((char **)&result, irczero);
		return (result);
	}
	
	for (i = 0, Client = ClientList; Client != NULL; Client = Client->next)
		if (!my_stricmp((char *) Nick, Client->user))
			len += 3;

	result = (u_char *) new_malloc(len + 1);

	for (i = 0, Client = ClientList; Client != NULL; Client = Client->next)
		if (!my_stricmp((char *) Nick, Client->user))
		{
			int b = Client->flags;
			int a = (b & DCC_TYPES);
			
			result[i++] =
					  (a == DCC_CHAT)		? 'C' /* CHAT */
					: (a == DCC_FILEOFFER)		? 'S' /* SEND */
					: (a == DCC_FILEREAD)		? 'G' /* GET */
/**************************** PATCHED by Flier ******************************/
					/*: (a == DCC_TALK)		? 'T' *//* TALK */
					/*: (a == DCC_SUMMON)		? 'U' *//* SUMMON */
/****************************************************************************/
					: (a == DCC_RAW_LISTEN)		? 'L' /* RAW_LISTEN */
					: (a == DCC_RAW)      		? 'R' /* RAW */
					:				  'x';

			result[i++] =
					  (b & DCC_OFFER)		? 'O' /* OFFERED */
					: (b & DCC_DELETE)		? 'C' /* CLOSED */
					: (b & DCC_ACTIVE)		? 'A' /* ACTIVE */
					: (b & DCC_WAIT)		? 'W' /* WAITING */
					:                         	  'x';

			result[i++] = ' ';
		}
	
	result[i] = '\0';
	
	return (result);
}

u_char *
function_chatpeers(dummy)
	u_char *dummy;
{
	u_char *result;
	DCC_list *Client;
	int	notfirst = 0;
	size_t len = 0;
	
	/* calculate size */
	for (Client = ClientList; Client != NULL; Client = Client->next)
		if ((Client->flags & (DCC_CHAT|DCC_ACTIVE)) == (DCC_CHAT|DCC_ACTIVE))
			len += (strlen(Client->user) + 1);
	result = (u_char *) new_malloc(len);
	*result = '\0';

	for (Client = ClientList; Client != NULL; Client = Client->next)
		if ((Client->flags & (DCC_CHAT|DCC_ACTIVE)) == (DCC_CHAT|DCC_ACTIVE))
		{
			if (notfirst)
				strcat((char *) result, ",");
			else
				notfirst = 1;
			strcat((char *) result, Client->user);
		}

	return (result);
}
#endif /* LITE */

u_char	*
function_word(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*count;
	int	cvalue;
	char	*word;

	count = next_arg((char *) input, (char **) &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (cvalue < 0)
		malloc_strcpy((char **) &result, empty_string);
	else
	{
		for (word = next_arg((char *) input, (char **) &input); word && cvalue--;
				word = next_arg((char *) input, (char **) &input))
			;
		malloc_strcpy((char **) &result, (word) ? word : empty_string);
	}
	return (result);
}

u_char	*
function_querynick(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	Window	*win;

	if (input && isdigit(*input))
		win = get_window_by_refnum((u_int)atoi((char *) input));
	else
		win = curr_scr_win;
	malloc_strcpy((char **) &result, win ? win->query_nick : "-1");
	return (result);
}

#ifndef LITE
u_char	*
function_windows(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	Win_Trav stuff;
	Window	*tmp;

	malloc_strcat((char **) &result, empty_string);
	stuff.flag = 1;
	while ((tmp = window_traverse(&stuff)))
	{
		if (tmp->name)
		{
			malloc_strcat((char **) &result, tmp->name);
			malloc_strcat((char **) &result, " ");
		}
		else
		{
			char buf[32];

			snprintf(buf, sizeof buf, "%u ", tmp->refnum);
			malloc_strcat((char **) &result, buf);
		}
	}

	return (result);
}

u_char	*
function_screens(input)
	u_char	*input;
{
	Screen	*list;
	u_char	*result = (u_char *) 0;
	char buf[32];

	malloc_strcat((char **) &result, empty_string);
	for (list = screen_list; list; list = list->next)
	{
		if (list->alive)
		{
			snprintf(buf, sizeof buf, "%u ", list->screennum);
			malloc_strcat((char **) &result, buf);
		}
	}

	return (result);
}

u_char	*
function_notify(input)
	u_char	*input;
{
	u_char	*result;

	if (input && my_stricmp(input, "gone") == 0)
		result = get_notify_list(NOTIFY_LIST_GONE);
	else if (input && my_stricmp(input, "all") == 0)
		result = get_notify_list(NOTIFY_LIST_ALL);
	else
		result = get_notify_list(NOTIFY_LIST_HERE);

	return result;
}

/*
 * $ignored(nick!user@host type) with type from:
 *	ALL MSGS PUBLIC WALLS WALLOPS INVITES NOTICES NOTES CTCP CRAP
 */
u_char	*
function_ignored(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0, *userhost, *nick;
	int type;

	if ((nick = next_arg((char *) input, (char **) &input)) != NULL)
	{
		type = get_ignore_type(input);
		if (type == 0 || type == -1 || type == IGNORE_ALL)
			goto do_zero;

		if ((userhost = index(nick, '!')))
			*userhost++ = 0;
		switch (double_ignore(nick, userhost, type))
		{
		case DONT_IGNORE:
			malloc_strcpy((char **) &result, "dont");
			break;
		case HIGHLIGHTED:
			malloc_strcpy((char **) &result, "highlighted");
			break;
		case IGNORED:
			malloc_strcpy((char **) &result, "ignored");
			break;
		default:
			goto do_zero;
		}
	}
	else
do_zero:
		malloc_strcpy((char **) &result, irczero);
	return (result);
}

u_char	*
function_winserver(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	tmp[10];
	Window	*win;

	if (input && isdigit(*input))
		win = get_window_by_refnum((u_int)atoi((char *) input));
	else
		win = curr_scr_win;
	snprintf(tmp, sizeof tmp, "%d", win ? win->server : -1);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}
#endif

u_char	*
function_winservergroup(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	tmp[10];
	Window	*win;

	if (input && isdigit(*input))
		win = get_window_by_refnum((u_int)atoi((char *) input));
	else
		win = curr_scr_win;
	snprintf(tmp, sizeof tmp, "%d", win ? win->server_group : -1);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

#ifndef LITE
u_char	*
function_winvisible(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	tmp[10];
	Window	*win;

	if (input && isdigit(*input))
		win = get_window_by_refnum((u_int)atoi((char *) input));
	else
		win = curr_scr_win;
	snprintf(tmp, sizeof tmp, "%d", win ? win->visible : -1);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

u_char	*
function_winnum(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	tmp[10];

	snprintf(tmp, sizeof tmp, "%d", curr_scr_win ? (int)curr_scr_win->refnum : -1);
	malloc_strcpy((char **) &result, tmp);
	return (result);
}

u_char	*
function_winnam(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	Window	*win;

	if (input && isdigit(*input))
		win = get_window_by_refnum((u_int)atoi((char *) input));
	else
		win = curr_scr_win;
	malloc_strcpy((char **) &result, (win && win->name) ? win->name : empty_string);
	return (result);
}

/*
 * returns the current window's display (len) size counting for double
 * status bars, etc.. -Toasty
 */
u_char    *
function_winrows(input)
        u_char   *input;
{
	u_char	*result = (u_char *) 0;

	if (curr_scr_win)
	{
		char	tmp[10];

		snprintf(tmp, sizeof tmp, "%d", curr_scr_win->display_size);
		malloc_strcpy((char **) &result, tmp);
	}
	else
		malloc_strcpy((char **) &result, "-1");
	return (result);
}

/*
 * returns the current screen's (since all windows have the same
 * column/width) column value -Toasty
 */
u_char	*
function_wincols(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;

	if (curr_scr_win)
	{
		char	tmp[10];

		snprintf(tmp, sizeof tmp, "%d", current_screen->co);
		malloc_strcpy((char **) &result, tmp);
	}
	else
		malloc_strcpy((char **) &result, "-1");
	return (result);
}

u_char	*
function_connect(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*host;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		put_it("You are not permitted to use CONNECT()");
	else
#endif
		if ((host = next_arg((char *) input, (char **) &input)) != NULL)
			result = (u_char *) dcc_raw_connect(host, (u_int) atoi((char *) input));
	return (result);
}


u_char	*
function_listen(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		malloc_strcpy((char **) &result, irczero);
	else
#endif
		result = (u_char *) dcc_raw_listen((u_int) atoi((char *) input));
	return (result);
}
#endif /* LITE */

u_char	*
function_toupper(input)
	u_char	*input;
{
	u_char	*new = (u_char *) 0,
		*ptr;

	if (!input)
		return (u_char *) empty_string;
	malloc_strcpy((char **) &new, (char *) input);
	for (ptr = new; *ptr; ptr++)
		*ptr = islower(*ptr) ? toupper(*ptr) : *ptr;
	return new;
}

u_char	*
function_tolower(input)
	u_char	*input;
{
	u_char	*new = (u_char *) 0,
		*ptr;

	if (!input)
		return (u_char *) empty_string;
	malloc_strcpy((char **) &new, (char *) input);
	for (ptr = new; *ptr; ptr++)
		*ptr = (isupper(*ptr)) ? tolower(*ptr) : *ptr;
	return new;
}

u_char	*
function_channels(input)
	u_char	*input;
{
	Window	*window;

	if (input)
 		window = isdigit(*input) ? get_window_by_refnum((u_int) atoi((char *) input))
					 : curr_scr_win;
	else
		window = curr_scr_win;

	return (u_char *) create_channel_list(window);
}

/* function_curpos moved to input.c */

#ifndef LITE
u_char	*
function_servers(input)
	u_char	*input;
{
	return (u_char *) create_server_list();
}

u_char	*
function_servertype(input)
	u_char	*input;
{
	int server;
	char *s;
	u_char *result = NULL;

	if (from_server < 0)
		server = primary_server;
	else
		server = from_server;
	if (server < 0)
		s = empty_string;
	else	
		switch (server_list[server].version) {
		case Server2_5:
			s = "IRC2.5";
			break;
		case Server2_6:
			s = "IRC2.6";
			break;
		case Server2_7:
			s = "IRC2.7";
			break;
		case Server2_8:
			s = "IRC2.8";
			break;
		case Server2_9:
			s = "IRC2.9";
			break;
		case Server2_10:
			s = "IRC2.10";
			break;
		case Server2_11:
			s = "IRC2.11";
			break;
/**************************** PATCHED by Flier ******************************/
                case Server2_12:
                        s = "IRC2.12";
                        break;
                case Server2_90:
                        s = "IRC2.90";
                        break;
/****************************************************************************/
		default:
			s = "IRC unknown";
			break;
		}

	malloc_strcpy((char **) &result, s);
	return (result);
}
#endif /* LITE */

u_char	*
function_onchannel(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	char	*nick;
	char	*channel = NULL;

 	if (from_server < 0 || !(nick = next_arg((char *) input, &channel)))
		malloc_strcpy((char **) &result, irczero);
	else
		malloc_strcpy((char **) &result,
			is_on_channel(channel, from_server, nick) ? one : irczero);
	return (result);
}

#ifndef LITE
u_char	*
function_pid(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	u_char	lbuf[32];	/* plenty big enough for %d */

	snprintf(CP(lbuf), sizeof lbuf, "%d", (int) getpid());
	malloc_strcpy((char **) &result, lbuf);
	return (result);
}

u_char	*
function_ppid(input)
	u_char	*input;
{
	u_char	*result = (u_char *) 0;
	u_char	lbuf[32];	/* plenty big enough for %d */

	snprintf(CP(lbuf), sizeof lbuf, "%d", (int) getppid());
	malloc_strcpy((char **) &result, lbuf);
	return (result);
}
#endif /* LITE */

/**************************** PATCHED by Flier ******************************/
/* Modified so if you call it $chanusers(#blah 1) it will return
   @nick if nick is channel operator
   +nick if nick is voiced
   %nick if nick is halfopped
   .nick otherwise */
#ifndef CELESCRP
/****************************************************************************/
u_char	*
function_chanusers(input)
	u_char	*input;
{
	ChannelList	*chan;
	NickList	*nicks;
	u_char	*result = (u_char *) 0;
	int	len = 0;
	int	notfirst = 0;
/**************************** PATCHED by Flier ******************************/
        int  nickst=0;
        char *channel;
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
	/*chan = lookup_channel((char *) input, from_server, CHAN_NOUNLINK);*/
        channel=new_next_arg((char *) input,(char **) &input);
	if (!channel) {
	    channel=get_channel_by_refnum(0);
	    if (!channel) return (u_char *) 0;
	}
        chan=lookup_channel(channel,from_server,CHAN_NOUNLINK);
        if (input && *input=='1') nickst=1;
/****************************************************************************/
	if ((ChannelList *) 0 == chan)
		return (u_char *) 0;

	for (nicks = chan->nicks; nicks; nicks = nicks->next)
/**************************** PATCHED by Flier ******************************/
        {
/****************************************************************************/
		len += (strlen(nicks->nick) + 1);
/**************************** PATCHED by Flier ******************************/
                if (nickst) len++;
        }
/****************************************************************************/
	result = (u_char *) new_malloc(len + 1);
	*result = '\0';

        for (nicks = chan->nicks; nicks; nicks = nicks->next)
	{
		if (notfirst)
			strcat((char *)result, " ");
		else
			notfirst = 1;
/**************************** PATCHED by Flier ******************************/
                if (nickst) {
                    if (nicks->chanop) strcat((char *) result,"@");
                    else if (nicks->halfop) strcat((char *) result,"%");
                    else if (nicks->hasvoice) strcat((char *) result,"+");
                    else strcat((char *) result,".");
                }
/****************************************************************************/
		strcat((char *)result, nicks->nick);
        }

        return (result);
}
/**************************** PATCHED by Flier ******************************/
#endif /* CELESCRP */
/****************************************************************************/

/*
 * strftime() patch from hari (markc@arbld.unimelb.edu.au)
 */
#ifdef HAVE_STRFTIME
u_char	*
function_strftime(input)
	u_char 	*input;
{
	char	result[128];
	time_t	ltime;
	char	*fmt = (char *) 0;

	ltime = atol((char *) input);
	fmt = (char *) input;
	/* skip the time field */
	while (isdigit(*fmt))
		++fmt; 
	if (*fmt && *++fmt)
	{
		struct tm	*tm;

		tm = localtime(&ltime);
		if (strftime(result, 128, fmt, tm))
		{
			u_char	*s = (u_char *) 0;

			malloc_strcpy((char **) &s, result);
			return s;
		}
		else
			return (u_char *) 0;
	}
	else
	{
		return (u_char *) 0;
	}
}
#endif


/*
 * idle() patch from scottr (scott.reynolds@plexus.com)
 */
u_char	*
function_idle(input)
	u_char 	*input;
{
	u_char	*result = (u_char *) 0;
	char	lbuf[20];

	snprintf(lbuf, sizeof lbuf, "%ld", (long)(time(0) - idle_time));
	malloc_strcpy((char **) &result, lbuf);
	return (result);
}

#ifndef LITE
u_char	*
function_urlencode(input)
u_char *input;
{
    int	i = 0;
    u_char *result;
    u_char *c;
    char buf[8];

    for (c = input; *c; c++)
        if (*c < '/' || *c > 'z' || (*c > ':' && *c < 'A') || (*c > 'Z' && *c < 'a'))
            i += 3;
        else
            i++;

    result = (u_char *) new_malloc(i + 15);

    for (i = 0, c = input; *c; c++)
    {
        if (*c < '/' || *c > 'z' || (*c > ':' && *c < 'A') || (*c > 'Z' && *c < 'a'))
        {
            sprintf(buf, "%02X", *c);
            result[i++] = '%';
            result[i++] = buf[0];
            result[i++] = buf[1];
        }
        else
            result[i++] = *c;
    }
    result[i] = '\0';
    return (result);
}

u_char	*
function_chr(input)
u_char *input;
{
    char c;
    char *tmpstr;
    char buf[16];
    u_char *result = NULL;

    if (input && *input) {
        while ((tmpstr = new_next_arg(input, (char **) &input))) {
            c = (char) atoi(tmpstr);
            sprintf(buf, "%c", c);
            malloc_strcat((char **) &result, buf);
        }
    }
    if (!result) malloc_strcpy((char **) &result, empty_string);
    return (result);
}

u_char	*
function_shellfix(input)
	u_char *input;
{
	u_char	*result;
	u_char	*c;
	int	i = 2;
	
	for (c = input; *c; c++)
		if (*c == '\'')
			i += 4;
		else
			++i;
	
	result = (u_char *) new_malloc(i + 1);
	
	i = 0;
	result[i++] = '\'';
	for (c = input; *c; c++)
	{
		if (*c == '\'')
		{
			result[i++] = '\'';
			result[i++] = '\\';
			result[i++] = '\'';
			result[i++] = '\'';
		}
		else
			result[i++] = *c;
	}
	result[i++] = '\'';
	result[i] = '\0';
	return (result);
}

u_char	*
function_filestat(input)
	u_char *input;
{
	struct	stat statbuf;
	u_char	*result = (char *) 0;
	char	lbuf[40];   /* 40 should be enough */


	if (stat(input, &statbuf) == -1)
		malloc_strcpy((char **) &result, empty_string);
	else
	{
		snprintf(lbuf, sizeof lbuf, "%lu,%d,%d,%o,%s",
		    (unsigned long)statbuf.st_size,
		    (int)statbuf.st_uid,
		    (int)statbuf.st_gid,
		    statbuf.st_mode,
		    input);
		malloc_strcpy((char **) &result, lbuf);
	}
	return (result);
}
#endif

/**************************** Patched by Flier ******************************/
#ifndef LITE
u_char *function_open(words)
u_char *words;
{
    u_char *result=(char *) 0;
    char *filename=(char *) 0;
    char locbuf[mybufsize/64];

    filename=next_arg((char *) words,(char **) &words);
    if (words && *words) {
        *(words+1)='\0';
        upper(words);
        if (*words=='R') snprintf(locbuf,sizeof(locbuf),"%d",OpenFileRead(filename));
        else if (*words=='W' || *words=='A')
            snprintf(locbuf,sizeof(locbuf),"%d",OpenFileWrite(filename,words));
    }
    else strcpy(locbuf,"-1");
    malloc_strcpy((char **) &result,locbuf);
    return(result);
}

u_char *function_close(words)
u_char *words;
{
    char *args=(char *) 0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        snprintf(locbuf,sizeof(locbuf),"%d",FileClose(atoi(args)));
    }
    else strcpy(locbuf,"-1");
    malloc_strcpy((char **) &result,locbuf);
    return(result);
}	

u_char *function_read(words)
u_char *words;
{
    char *args=(char *) 0;
    u_char *result=(char *) 0;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        malloc_strcpy((char **) &result,FileRead(atoi(args)));
    }
    else malloc_strcpy((char **) &result,"-1");
    return(result);
}

u_char *function_write(words)
u_char *words;
{
    char *args=(char *) 0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        if (words && *words) snprintf(locbuf,sizeof(locbuf),"%d",FileWrite(atoi(args),words));
        else strcpy(locbuf,"-1");
    }
    else strcpy(locbuf,"-1");
    malloc_strcpy((char **) &result,locbuf);
    return(result);
}

u_char *function_eof(words)
u_char *words;
{
    char *args=(char *) 0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        snprintf(locbuf,sizeof(locbuf),"%d",FileEof(atoi(args)));
    }
    else strcpy(locbuf,"-1");
    malloc_strcpy((char **) &result,locbuf);
    return(result);
}

u_char *function_rename(words)
u_char *words;
{
    char *oldname=(char *) 0;
    char *newname=(char *) 0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];

    if (words && *words) {
        oldname=next_arg((char *) words,(char **) &words);
        newname=next_arg((char *) words,(char **) &words);
        if (newname && *newname) {
            snprintf(locbuf,sizeof(locbuf),"%d",rename(oldname,newname));
            malloc_strcpy((char **) &result,locbuf);
        }
        else malloc_strcpy((char **) &result,"-1");
    }
    else malloc_strcpy((char **) &result,"-1");
    return(result);
}	
#endif

u_char *function_intuhost(words)
u_char *words;
{
    u_char *result=(char *) 0;
    char *args=(char *) 0;
    char locbuf[mybufsize/2];
    NickList *joiner;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        joiner=CheckJoiners(args,0,from_server,NULL);
        if (joiner && joiner->userhost) snprintf(locbuf,sizeof(locbuf),"%s!%s",joiner->nick,joiner->userhost);
        else strcpy(locbuf,"-1");
        malloc_strcpy((char **) &result,locbuf);
    }
    else malloc_strcpy((char **) &result,"-1");
    return(result);
}

u_char *function_checkuser(words)
u_char *words;
{
    u_char *result=(char *) 0;
    char *args=(char *) 0;
    char locbuf[2*mybufsize];
    struct friends *tmpfriend;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        if (words && *words) {
            if ((tmpfriend=CheckUsers(args,words))) {
                *locbuf='\0';
                BuildPrivs(tmpfriend,locbuf);
                strmcat(locbuf," ",sizeof(locbuf));
                strmcat(locbuf,tmpfriend->userhost,sizeof(locbuf));
                strmcat(locbuf," ",sizeof(locbuf));
                strmcat(locbuf,tmpfriend->channels,sizeof(locbuf));
                malloc_strcpy((char **) &result,locbuf);
                return(result);
            }
        }
    }
    malloc_strcpy((char **) &result,"-1");
    return(result);
}

u_char *function_checkshit(words)
u_char *words;
{
    u_char *result=(char *) 0;
    char *args=(char *) 0;
    char locbuf[2*mybufsize];
    struct autobankicks *abk;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        if (words && *words) {
            *locbuf='\0';
            if ((abk=CheckABKs(args,words))!=NULL) {
                if ((abk->shit)&1) strcat(locbuf,"K");
                if ((abk->shit)&2) strcat(locbuf,"B");
                if ((abk->shit)&4) strcat(locbuf,"I");
                if ((abk->shit)&8) strcat(locbuf,"P");
                if ((abk->shit)&16) strcat(locbuf,"D");
                strmcat(locbuf," ",sizeof(locbuf));
                strmcat(locbuf,abk->userhost,sizeof(locbuf));
                strmcat(locbuf," ",sizeof(locbuf));
                strmcat(locbuf,abk->channels,sizeof(locbuf));
                strmcat(locbuf," ",sizeof(locbuf));
                strmcat(locbuf,abk->reason,sizeof(locbuf));
            }
            else strcpy(locbuf,"-1");
            malloc_strcpy((char **) &result,locbuf);
        }
        else malloc_strcpy((char **) &result,"-1");
    }
    else malloc_strcpy((char **) &result,"-1");
    return(result);
}

u_char *function_stripansi(words)
u_char *words;
{
    u_char *result=(char *) 0;
    char locbuf[2*mybufsize];

    if (words && *words) {
        StripAnsi(words,locbuf,0);
        malloc_strcpy((char **) &result,locbuf);
    }
    else malloc_strcpy((char **) &result,empty_string);
    return(result);
}

u_char *function_stripcrap(words)
u_char *words;
{
    u_char *result = NULL;
    char locbuf[2 * mybufsize];

    if (words && *words) {
        StripAnsi(words, locbuf, 1);
        malloc_strcpy((char **) &result, locbuf);
    }
    else malloc_strcpy((char **) &result, empty_string);
    return(result);
}

u_char *function_pattern(words)
u_char *words;
{
    char    *tmpstr;
    char    *pattern;
    u_char  *result = NULL;

    if ((pattern = next_arg((char *) words, (char **) &words))) {
        while (((tmpstr = next_arg((char *) words, (char **) &words)) != NULL)) {
            if (wild_match(pattern, tmpstr)) {
                malloc_strcat((char **) &result, tmpstr);
                malloc_strcat((char **) &result, " ");
            }
        }
    } 
    if (!result) malloc_strcpy((char **) &result, empty_string);
    return(result);
}

/*u_char *function_chops(words)
u_char *words;
{
    u_char *nicks=(char *) 0;
    char *curr_chan;
    int  count=0;
    NickList *tmp;
    ChannelList *chan;

    *tmpbuf2='\0';
    if (words && *words) strmcpy(tmpbuf,next_arg((char *) words,(char **) &words),mybufsize/2);
    else *tmpbuf='\0';
    curr_chan=get_channel_by_refnum(0);
    if (!curr_chan) curr_chan=empty_string;
    if ((tmpbuf[0]==0) || (!strcmp(tmpbuf,"*"))) strmcpy(tmpbuf,curr_chan,mybufsize/2);
    if (tmpbuf[0]==0 || tmpbuf[0]=='0') {
        malloc_strcpy((char **) &nicks,tmpbuf2);
        return(nicks);
    }
    chan=lookup_channel(tmpbuf,curr_scr_win->server,0);
    if (!chan) {
        malloc_strcpy((char **) &nicks,tmpbuf2);
        return(nicks);
    }
    for (tmp=chan->nicks;tmp;tmp=tmp->next) {
        if (tmp->chanop) {
            count++;
            strcat(tmpbuf2,tmp->nick);
            strcat(tmpbuf2," ");
            if (count>50) {
                tmpbuf2[strlen(tmpbuf2)-1]='\0';
                malloc_strcat((char **) &nicks,tmpbuf2);
                *tmpbuf2='\0';
                count=0;
            }
        }
    }
    if (count) {
        tmpbuf2[strlen(tmpbuf2)-1]='\0';
        malloc_strcat((char **) &nicks,tmpbuf2);
    }
    return(nicks);
}

u_char *function_chnops(words)
u_char *words;
{
    u_char *nicks=(char *) 0;
    char *curr_chan;
    int  count=0;
    NickList *tmp;
    ChannelList *chan;

    *tmpbuf2='\0';
    if (words && *words) strmcpy(tmpbuf,next_arg((char *) words,(char **) &words),mybufsize/2);
    else *tmpbuf='\0';
    curr_chan=get_channel_by_refnum(0);
    if (!curr_chan) curr_chan=empty_string;
    if ((tmpbuf[0]==0) || (!strcmp(tmpbuf,"*"))) strmcpy(tmpbuf,curr_chan,mybufsize/2);
    if (tmpbuf[0]==0 || tmpbuf[0]=='0') {
        malloc_strcpy((char **) &nicks,tmpbuf2);
        return(nicks);
    }
    chan=lookup_channel(tmpbuf,curr_scr_win->server,0);
    if (!chan) {
        malloc_strcpy((char **) &nicks,tmpbuf2);
        return(nicks);
    }
    for (tmp=chan->nicks;tmp;tmp=tmp->next) {
        if (!(tmp->chanop)) {
            count++;
            strcat(tmpbuf2,tmp->nick);
            strcat(tmpbuf2," ");
            if (count>50) {
                tmpbuf2[strlen(tmpbuf2)-1]='\0';
                malloc_strcat((char **) &nicks,tmpbuf2);
                *tmpbuf2='\0';
                count=0;
            }
        }
    }
    if (count) {
        tmpbuf2[strlen(tmpbuf2)-1]='\0';
        malloc_strcat((char **) &nicks,tmpbuf2);
    }
    return(nicks);
}*/

#ifndef LITE
u_char *function_color(words)
u_char *words;
{
    u_char *result=(char *) 0;
#ifdef WANTANSI
    int  colnum=0;
    int  eventnum=0;
    char *tmp=(char *) 0;
    char *tmpstr=(char *) 0;
    char locbuf[2*mybufsize];
    
    if (words && *words) {
        tmp=next_arg((char *) words,(char **) &words);
        if (tmp && words && *words) {
            eventnum=atoi(tmp);
            colnum=atoi(words);
        }
        if (eventnum>=1 && eventnum<=NUMCMDCOLORS && colnum>=1 && colnum<=6) {
            eventnum--;
            malloc_strcpy((char **) &result,Colors[COLOFF]);
            switch (colnum) {
                case 1 : malloc_strcat((char **) &result,CmdsColors[eventnum].color1);
                break;
                case 2 : malloc_strcat((char **) &result,CmdsColors[eventnum].color2);
                break;
                case 3 : malloc_strcat((char **) &result,CmdsColors[eventnum].color3);
                break;
                case 4 : malloc_strcat((char **) &result,CmdsColors[eventnum].color4);
                break;
                case 5 : malloc_strcat((char **) &result,CmdsColors[eventnum].color5);
                break;
                case 6 : malloc_strcat((char **) &result,CmdsColors[eventnum].color6);
                break;
            }
        }
        else {
            malloc_strcpy((char **) &result,Colors[COLOFF]);
            strmcpy(locbuf,tmp,sizeof(locbuf));
            tmp=locbuf;
            for (;*tmp;tmp++)
                if (*tmp==',') {
                    *tmp++='\0';
                    if (!tmpstr) tmpstr=locbuf;
                    if ((colnum=ColorNumber(tmpstr))!=-1) 
                        malloc_strcat((char **) &result,Colors[colnum]);
                    tmpstr=tmp;
                }
            if (!tmpstr) tmpstr=locbuf;
            if (tmpstr)
                if ((colnum=ColorNumber(tmpstr))!=-1)
                    malloc_strcat((char **) &result,Colors[colnum]);
        }
    }
    else malloc_strcpy((char **) &result,empty_string);
    return(result);
#else
    malloc_strcpy((char **) &result,empty_string);
    return(result);
#endif
}
#endif /* LITE */

#if !defined(CELESCRP) && !defined(LITE)
u_char *function_uhost(input)
u_char *input;
{
    char *tmp;
    u_char *result=(char *) 0;
    
    if ((tmp=index(input,'!'))) malloc_strcpy((char **) &result,tmp+1);
    else malloc_strcpy((char **) &result,empty_string);
    return(result);
}

u_char *function_hhost(input)
u_char *input;
{
    char *tmp;
    u_char *result=(char *) 0;

    if ((tmp=index(input,'@'))) malloc_strcpy((char **) &result,tmp+1);
    else malloc_strcpy((char **) &result,empty_string);
    return(result);
}

u_char *function_topic(input)
u_char *input;
{
    u_char *result=(char *) 0;
    char *channel;
    char locbuf[2*mybufsize];
    ChannelList *chan;

    if ((channel=next_arg((char *) input,(char **) &channel)) &&
        (chan=lookup_channel(channel,from_server,0)) && chan->topicstr) {
        if (chan->topicwho) snprintf(locbuf,sizeof(locbuf),"%s %ld ",chan->topicwho,chan->topicwhen);
        else strcpy(locbuf,"unknown 0 ");
        strmcat(locbuf,chan->topicstr,sizeof(locbuf));
        malloc_strcpy((char **) &result,locbuf);
    }
    else malloc_strcpy((char **) &result,empty_string);
    return(result);
}

u_char *function_strlen(input)
u_char *input;
{
    u_char *result=(char *) 0;
    char locbuf[2*mybufsize];

    if (input && *input) {
        snprintf(locbuf,sizeof(locbuf),"%d",strlen(input));
        malloc_strcpy((char **) &result,locbuf);
    }
    else malloc_strcpy((char **) &result,"0");
    return(result);
}

u_char *function_strnum(input)
u_char *input;
{
    int  count=0;
    char *tmp=input;
    u_char *result=(char *) 0;
    char locbuf[2*mybufsize];

    if (tmp && *tmp) {
        while (*tmp) {
            while (*tmp && isspace(*tmp)) tmp++;
            if (*tmp) {
                count++;
                while (*tmp && !isspace(*tmp)) tmp++;
            }
            if (*tmp) tmp++;
        }
        snprintf(locbuf,sizeof(locbuf),"%d",count);
        malloc_strcpy((char **) &result,locbuf);
    }
    else malloc_strcpy((char **) &result,"0");
    return(result);
}
#endif /* !CELESCRP && !LITE */

#ifndef LITE
u_char *function_fsize(input)
u_char *input;
{
    int  fexists;
    char *filename=(char *) 0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];
    struct stat statbuf;

    if (input && *input) {
        if (!(filename=expand_twiddle(input))) malloc_strcpy(&filename,input);
        fexists=stat(filename,&statbuf);
        new_free(&filename);
        if (fexists==-1) malloc_strcpy((char **) &result,"-1");
        else {
            snprintf(locbuf,sizeof(locbuf),"%ld",statbuf.st_size);
            malloc_strcpy((char **) &result,locbuf);
        }
    }
    else malloc_strcpy((char **) &result,"-1");
    return(result);
}

/* Search and replace function --
   Usage:   $sar(command/search/replace/data)
   Commands:
		r - treat data as a variable name and 
		    return the replaced data to the variable
		g - Replace all instances, not just the first one
   The delimiter may be any character that is not a command (typically /)
   The delimiter MUST be the first character after the command
   Returns empty string on error
*/
#ifndef CELESCRP
u_char *function_sar(word)
u_char *word;
{
    int	variable=0,global=0,searchlen;
    unsigned int display=window_display;
    char delimiter;
    char *data=(char *) 0;
    char *value=(char *) 0;
    char *svalue;
    char *search=(char *) 0;
    u_char *result=(char *) 0;
    char *replace=(char *) 0;
    char *pointer=(char *) 0;
    Alias *tmp;

    while (((*word=='r') && (variable=1)) || ((*word=='g') && (global=1))) word++;
    if (!(word && *word)) {
        malloc_strcpy((char **) &result,empty_string);
        return(result);
    }
    delimiter=*word;
    search=word+1;
    if (!(replace=strchr(search,delimiter))) {
        malloc_strcpy((char **) &result,empty_string);
        return(result);
    }
    *replace++='\0';
    if (!(data=strchr(replace,delimiter))) {
        malloc_strcpy((char **) &result,empty_string);
        return(result);
    }
    *data++='\0';
    if (variable) {
        if ((tmp=find_alias(&(alias_list[VAR_ALIAS]),data,1,(int *) 0)))
            malloc_strcpy(&value,tmp->stuff);
    }
    else malloc_strcpy(&value,data);
    if (!value || !*value) {
        if (value) new_free(&value);
        malloc_strcpy((char **) &result,empty_string);
        return(result);
    }
    svalue=value;
    pointer=value;
    searchlen=strlen(search)-1;
    if (searchlen<0) searchlen=0;
    if (global) {
        while ((pointer=strstr(pointer,search))) {
            pointer[0]=pointer[searchlen]=0;
            pointer+=searchlen+1;
            malloc_strcat((char **) &result,value);
            malloc_strcat((char **) &result,replace);
            value=pointer;
            if (!*pointer) break;
        }
    } 
    else {
        if ((pointer=strstr(pointer,search))) {
            pointer[0]=pointer[searchlen]=0;
            pointer+=searchlen+1;
            malloc_strcat((char **) &result,value);
            malloc_strcat((char **) &result,replace);
            value=pointer;
        }
    }
    malloc_strcat((char **) &result,value);
    if (variable)  {
        window_display=0;
        add_alias(VAR_ALIAS,data,result);
        window_display=display;
    }
    new_free(&svalue);
    return(result);
}

/* Translate characters
   Usage:   $sar(/set1/set2/data)
   The delimiter may be any character
   Replaces character from set1 with character on the same position in set2 
   Returns empty string on error
*/
u_char *function_tr(input)
u_char *input;
{
    int i;
    char delim;
    char *x, *y;
    char *search = NULL;
    char *replace = NULL;
    char *string = NULL;
    u_char *result = NULL;

    if (input && *input) {
        delim = *input;
        search = input;
        search++;
        replace = strchr(search, delim);
        if (replace && replace != search) {
            *replace++ = '\0';
            if ((string = strchr(replace, delim))) {
                *string++ = '\0';
                if (strlen(search) == strlen(replace)) {
                    malloc_strcpy((char **) &result, string);
                    for (i = 0, x = search; *x; x++, i++)
                        for (y = result; *y; y++)
                            if (*y == *x) {
                                *y = replace[i];
                            }
                }
            }
        }
    }
    if (!result) malloc_strcpy((char **) &result, empty_string);
    return(result);
}
#endif /* CELESCRP */
#endif /* LITE */

#ifdef COUNTRY
u_char *function_country(input)
u_char *input;
{
    int i;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64+1];
    struct domain_str {
        char *id;
        char *description;
    } domain[] = {
        { "AD", "Andorra" },
        { "AE", "United Arab Emirates" },
        { "AF", "Afghanistan" },
        { "AG", "Antigua and Barbuda" },
        { "AI", "Anguilla" },
        { "AL", "Albania" },
        { "AM", "Armenia" },
        { "AO", "Angola" },
        { "AQ", "Antarctica" },
        { "AR", "Argentina" },
        { "AS", "Samoa (American)" },
        { "AT", "Austria" },
        { "AU", "Australia" },
        { "AW", "Aruba" },
        { "AX", "Aaland Islands" },
        { "AZ", "Azerbaijan" },
        { "BA", "Bosnia and Herzegovina" },
        { "BB", "Barbados" },
        { "BD", "Bangladesh" },
        { "BE", "Belgium" },
        { "BF", "Burkina Faso" },
        { "BG", "Bulgaria" },
        { "BH", "Bahrain" },
        { "BI", "Burundi" },
        { "BJ", "Benin" },
        { "BL", "St Barthelemy" },
        { "BM", "Bermuda" },
        { "BN", "Brunei" },
        { "BO", "Bolivia" },
        { "BQ", "Caribbean Netherlands" },
        { "BR", "Brazil" },
        { "BS", "Bahamas" },
        { "BT", "Bhutan" },
        { "BV", "Bouvet Island" },
        { "BW", "Botswana" },
        { "BY", "Belarus" },
        { "BZ", "Belize" },
        { "CA", "Canada" },
        { "CC", "Cocos (Keeling) Islands" },
        { "CD", "Congo (Dem. Rep.)" },
        { "CF", "Central African Rep." },
        { "CG", "Congo (Rep.)" },
        { "CH", "Switzerland" },
        { "CI", "Cote d'Ivoire" },
        { "CK", "Cook Islands" },
        { "CL", "Chile" },
        { "CM", "Cameroon" },
        { "CN", "China" },
        { "CO", "Colombia" },
        { "CR", "Costa Rica" },
        { "CU", "Cuba" },
        { "CV", "Cape Verde" },
        { "CW", "Curacao" },
        { "CX", "Christmas Island" },
        { "CY", "Cyprus" },
        { "CZ", "Czech Republic" },
        { "DE", "Germany" },
        { "DJ", "Djibouti" },
        { "DK", "Denmark" },
        { "DM", "Dominica" },
        { "DO", "Dominican Republic" },
        { "DZ", "Algeria" },
        { "EC", "Ecuador" },
        { "EE", "Estonia" },
        { "EG", "Egypt" },
        { "EH", "Western Sahara" },
        { "ER", "Eritrea" },
        { "ES", "Spain" },
        { "ET", "Ethiopia" },
        { "FI", "Finland" },
        { "FJ", "Fiji" },
        { "FK", "Falkland Islands" },
        { "FM", "Micronesia" },
        { "FO", "Faroe Islands" },
        { "FR", "France" },
        { "GA", "Gabon" },
        { "GB", "Britain (UK)" },
        { "GD", "Grenada" },
        { "GE", "Georgia" },
        { "GF", "French Guiana" },
        { "GG", "Guernsey" },
        { "GH", "Ghana" },
        { "GI", "Gibraltar" },
        { "GL", "Greenland" },
        { "GM", "Gambia" },
        { "GN", "Guinea" },
        { "GP", "Guadeloupe" },
        { "GQ", "Equatorial Guinea" },
        { "GR", "Greece" },
        { "GS", "South Georgia and the South Sandwich Islands" },
        { "GT", "Guatemala" },
        { "GU", "Guam" },
        { "GW", "Guinea-Bissau" },
        { "GY", "Guyana" },
        { "HK", "Hong Kong" },
        { "HM", "Heard Island and McDonald Islands" },
        { "HN", "Honduras" },
        { "HR", "Croatia" },
        { "HT", "Haiti" },
        { "HU", "Hungary" },
        { "ID", "Indonesia" },
        { "IE", "Ireland" },
        { "IL", "Israel" },
        { "IM", "Isle of Man" },
        { "IN", "India" },
        { "IO", "British Indian Ocean Territory" },
        { "IQ", "Iraq" },
        { "IR", "Iran" },
        { "IS", "Iceland" },
        { "IT", "Italy" },
        { "JE", "Jersey" },
        { "JM", "Jamaica" },
        { "JO", "Jordan" },
        { "JP", "Japan" },
        { "KE", "Kenya" },
        { "KG", "Kyrgyzstan" },
        { "KH", "Cambodia" },
        { "KI", "Kiribati" },
        { "KM", "Comoros" },
        { "KN", "St Kitts and Nevis" },
        { "KP", "Korea (North)" },
        { "KR", "Korea (South)" },
        { "KW", "Kuwait" },
        { "KY", "Cayman Islands" },
        { "KZ", "Kazakhstan" },
        { "LA", "Laos" },
        { "LB", "Lebanon" },
        { "LC", "St Lucia" },
        { "LI", "Liechtenstein" },
        { "LK", "Sri Lanka" },
        { "LR", "Liberia" },
        { "LS", "Lesotho" },
        { "LT", "Lithuania" },
        { "LU", "Luxembourg" },
        { "LV", "Latvia" },
        { "LY", "Libya" },
        { "MA", "Morocco" },
        { "MC", "Monaco" },
        { "MD", "Moldova" },
        { "ME", "Montenegro" },
        { "MF", "St Martin (French part)" },
        { "MG", "Madagascar" },
        { "MH", "Marshall Islands" },
        { "MK", "Macedonia" },
        { "ML", "Mali" },
        { "MM", "Myanmar (Burma)" },
        { "MN", "Mongolia" },
        { "MO", "Macau" },
        { "MP", "Northern Mariana Islands" },
        { "MQ", "Martinique" },
        { "MR", "Mauritania" },
        { "MS", "Montserrat" },
        { "MT", "Malta" },
        { "MU", "Mauritius" },
        { "MV", "Maldives" },
        { "MW", "Malawi" },
        { "MX", "Mexico" },
        { "MY", "Malaysia" },
        { "MZ", "Mozambique" },
        { "NA", "Namibia" },
        { "NC", "New Caledonia" },
        { "NE", "Niger" },
        { "NF", "Norfolk Island" },
        { "NG", "Nigeria" },
        { "NI", "Nicaragua" },
        { "NL", "Netherlands" },
        { "NO", "Norway" },
        { "NP", "Nepal" },
        { "NR", "Nauru" },
        { "NU", "Niue" },
        { "NZ", "New Zealand" },
        { "OM", "Oman" },
        { "PA", "Panama" },
        { "PE", "Peru" },
        { "PF", "French Polynesia" },
        { "PG", "Papua New Guinea" },
        { "PH", "Philippines" },
        { "PK", "Pakistan" },
        { "PL", "Poland" },
        { "PM", "St Pierre and Miquelon" },
        { "PN", "Pitcairn" },
        { "PR", "Puerto Rico" },
        { "PS", "Palestine" },
        { "PT", "Portugal" },
        { "PW", "Palau" },
        { "PY", "Paraguay" },
        { "QA", "Qatar" },
        { "RE", "Reunion" },
        { "RO", "Romania" },
        { "RS", "Serbia" },
        { "RU", "Russia" },
        { "RW", "Rwanda" },
        { "SA", "Saudi Arabia" },
        { "SB", "Solomon Islands" },
        { "SC", "Seychelles" },
        { "SD", "Sudan" },
        { "SE", "Sweden" },
        { "SG", "Singapore" },
        { "SH", "St Helena" },
        { "SI", "Slovenia" },
        { "SJ", "Svalbard and Jan Mayen" },
        { "SK", "Slovakia" },
        { "SL", "Sierra Leone" },
        { "SM", "San Marino" },
        { "SN", "Senegal" },
        { "SO", "Somalia" },
        { "SR", "Suriname" },
        { "SS", "South Sudan" },
        { "ST", "Sao Tome and Principe" },
        { "SV", "El Salvador" },
        { "SX", "St Maarten (Dutch part)" },
        { "SY", "Syria" },
        { "SZ", "Swaziland" },
        { "TC", "Turks and Caicos Is" },
        { "TD", "Chad" },
        { "TF", "French Southern and Antarctic Lands" },
        { "TG", "Togo" },
        { "TH", "Thailand" },
        { "TJ", "Tajikistan" },
        { "TK", "Tokelau" },
        { "TL", "East Timor" },
        { "TM", "Turkmenistan" },
        { "TN", "Tunisia" },
        { "TO", "Tonga" },
        { "TR", "Turkey" },
        { "TT", "Trinidad and Tobago" },
        { "TV", "Tuvalu" },
        { "TW", "Taiwan" },
        { "TZ", "Tanzania" },
        { "UA", "Ukraine" },
        { "UG", "Uganda" },
        { "UM", "US minor outlying islands" },
        { "US", "United States" },
        { "UY", "Uruguay" },
        { "UZ", "Uzbekistan" },
        { "VA", "Vatican City" },
        { "VC", "St Vincent" },
        { "VE", "Venezuela" },
        { "VG", "Virgin Islands (UK)" },
        { "VI", "Virgin Islands (US)" },
        { "VN", "Vietnam" },
        { "VU", "Vanuatu" },
        { "WF", "Wallis and Futuna" },
        { "WS", "Samoa (western)" },
        { "YE", "Yemen" },
        { "YT", "Mayotte" },
        { "ZA", "South Africa" },
        { "ZM", "Zambia" },
        { "ZW", "Zimbabwe" },
        { "COM", "Commercial" },
        { "EDU", "Educational Institution" },
        { "GOV", "Government" },
        { "INT", "International" },
        { "MIL", "Military" },
        { "NET", "Network" },
        { "ORG", "Non-Profit Organization" },
        { "RPA", "Old School ARPAnet" },
        { "ATO", "Nato Fiel" },
        { "MED", "United States Medical" },
        { "ARPA", "Reverse DNS" },
        { NULL, NULL }
    };

    if (input && *input) {
        strmcpy(locbuf,input,mybufsize/64);
        upper(locbuf);
        for (i=0;domain[i].id;i++)
            if (!strcmp(domain[i].id,locbuf)) {
                malloc_strcpy((char **) &result,domain[i].description);
                return(result);
            }
    }
    malloc_strcpy((char **) &result,"Unknown");
    return(result);
}
#endif

#if !defined(CELESCRP) && !defined(LITE)
u_char *function_cdccslots(input)
u_char *input;
{
    int  slots=0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];

    slots=CdccLimit-TotalSendDcc();
    if (slots<0) slots=0;
    snprintf(locbuf,sizeof(locbuf),"%d",slots);
    malloc_strcpy((char **) &result,locbuf);
    return(result);
}

u_char *function_cdccqslots(input)
u_char *input;
{
    int  slots=0;
    u_char *result=(char *) 0;
    char locbuf[mybufsize/64];

    if (CdccQueueLimit) slots=CdccQueueLimit-TotalQueue();
    else slots=10;
    if (slots<0) slots=0;
    snprintf(locbuf,sizeof(locbuf),"%d",slots);
    malloc_strcpy((char **) &result,locbuf);
    return(result);
}

u_char *function_url(input)
u_char *input;
{
    int urlnum = 9999;
    int showtarget = 0;
    u_char *result = NULL;
    struct urlstr *tmpurl = urllist, *prevurl = NULL;

    if (!tmpurl) malloc_strcpy((char **) &result, empty_string);
    else {
        if (input && *input) {
            char *tmpstr = new_next_arg(input, (char **) &input);
            urlnum = atoi(tmpstr);
            if (input && *input) showtarget = 1;
        }
        while (tmpurl && urlnum--) {
            prevurl = tmpurl;
            tmpurl = tmpurl->next;
        }
        if (tmpurl || (prevurl && (urlnum > 0))) {
            if (!tmpurl) tmpurl = prevurl;
            malloc_strcpy((char **) &result, tmpurl->urls);
            if (showtarget && tmpurl->source) {
                malloc_strcat((char **) &result, " ");
                malloc_strcat((char **) &result, tmpurl->source);
            }
        }
        else malloc_strcpy((char **) &result, empty_string);
    }
    return(result);
}

u_char *function_strstr(input)
u_char *input;
{
    u_char *result = (char *) 0;
    char *tmpstr = (char *) 0;
    char *findstr = (char *) 0;
    char locbuf[2 * mybufsize + 1];

    if (input && *input) {
        strmcpy(locbuf, &input[1], 2 * mybufsize);
        if ((tmpstr = index(locbuf, *input))) {
            *tmpstr ++= '\0';
            if (*tmpstr == '\0') tmpstr = (char *) 0;
        }
        if (tmpstr) findstr = strstr(locbuf, tmpstr);
    }
    if (!findstr) findstr = empty_string;
    malloc_strcpy((char **) &result, findstr);
    return(result);
}

u_char *function_szvar(input)
u_char *input;
{
    int  i;
    u_char *result=(char *) 0;
    char locbuf[mybufsize+1];
    struct commands {
        char *command;
        int  type;  /* 1=numeric   2=string   3=on channels/off */
        int  *ivar;
        char **svar;
    } command_list[]= {
        { "EXTMES"         , 1, &ExtMes          , NULL                   },
        { "NHPROT"         , 3, &NHProt          , &NHProtChannels        },
        /* defban is character that should be returned as string */
        { "BANTYPE"        , 2, NULL             , NULL                   },
        /* Cdcc limit should return CdccLimit and CdccQueueLimit */
        { "CDCCLIMIT"      , 1, &CdccLimit       , NULL                   },
        { "CDCCIDLE"       , 1, &CdccIdle        , NULL                   },
        { "CDCCAUTOGET"    , 1, &AutoGet         , NULL                   },
        { "CDCCSECURE"     , 1, &Security        , NULL                   },
        { "CDCCPTIME"      , 1, &PlistTime       , NULL                   },
        { "CDCCNTIME"      , 1, &NlistTime       , NULL                   },
        { "CDCCCHANNELS"   , 2, NULL             , &CdccChannels          },
        { "CDCCLONGSTATUS" , 1, &LongStatus      , NULL                   },
        { "CDCCOVERWRITE"  , 1, &CdccOverWrite   , NULL                   },
        { "CDCCSTATUS"     , 1, &ShowDCCStatus   , NULL                   },
        { "CDCCSTATS"      , 1, &CdccStats       , NULL                   },
        { "CDCCWARNING"    , 1, &DCCWarning      , NULL                   },
        { "CDCCULDIR"      , 2, NULL             , &CdccUlDir             },
        { "CDCCDLDIR"      , 2, NULL             , &CdccDlDir             },
        { "DEOPS"          , 1, &DeopSensor      , NULL                   },
        { "KICKS"          , 1, &KickSensor      , NULL                   },
        { "NICKS"          , 1, &NickSensor      , NULL                   },
        { "DEOPT"          , 1, &MDopTimer       , NULL                   },
        { "KICKT"          , 1, &KickTimer       , NULL                   },
        { "NICKT"          , 1, &NickTimer       , NULL                   },
        { "AWAYT"          , 1, &AutoAwayTime    , NULL                   },
        { "IGTIME"         , 1, &IgnoreTime      , NULL                   },
#ifdef EXTRAS
        { "IDLETIME"       , 1, &IdleTime        , NULL                   },
#endif
        { "NPROT"          , 3, &NickWatch       , &NickWatchChannels     },
        { "DPROT"          , 3, &MDopWatch       , &MDopWatchChannels     },
        { "KPROT"          , 3, &KickWatch       , &KickWatchChannels     },
        { "AREJOIN"        , 3, &AutoRejoin      , &AutoRejoinChannels    },
        { "AJOIN"          , 3, &AutoJoinOnInv   , &AutoJoinChannels      },
#if defined(EXTRAS) || defined(FLIER)
        { "AUTOINV"        , 3, &AutoInv         , &AutoInvChannels       },
#endif
        /* Floodp should return FloodProt, FloodMessages and FloodSeconds */
        { "FLOODP"         , 1, &FloodProt       , NULL                   },
        { "SERVNOTICE"     , 1, &ServerNotice    , NULL                   },
        { "CTCPCLOAK"      , 1, &CTCPCloaking    , NULL                   },
        { "FAKE"           , 3, &ShowFakes       , &ShowFakesChannels     },
        { "SHOWAWAY"       , 3, &ShowAway        , &ShowAwayChannels      },
        { "KICKOPS"        , 3, &KickOps         , &KickOpsChannels       },
        { "KICKONFLOOD"    , 3, &KickOnFlood     , &KickOnFloodChannels   },
        { "SHOWNICK"       , 1, &ShowNick        , NULL                   },
        { "KICKONBAN"      , 3, &KickOnBan       , &KickOnBanChannels     },
        { "REPWORD"        , 2, NULL             , &AutoReplyBuffer       },
        { "ORIGNICK"       , 3, &OrigNickChange  , &OrigNick              },
        { "NTFYMODE"       , 1, &NotifyMode      , NULL                   },
        { "URLCATCH"       , 1, &URLCatch        , NULL                   },
        { "EGO"            , 1, &Ego             , NULL                   },
        { "AUTOCOMPL"      , 3, &AutoNickCompl   , &AutoReplyString       },
        { "BITCH"          , 3, &Bitch           , &BitchChannels         },
        { "FRLIST"         , 3, &FriendList      , &FriendListChannels    },
#ifdef EXTRAS
        { "IDLEKICK"       , 3, &IdleKick        , &IdleKickChannels      },
        { "CHSIGNOFF"      , 3, &ShowSignoffChan , &SignoffChannels       },
#endif
        { "COMPRESS"       , 3, &CompressModes   , &CompressModesChannels },
        { "STAMP"          , 1, &Stamp           , NULL                   },
        { "ARINWIN"        , 1, &ARinWindow      , NULL                   },
        { "BKLIST"         , 3, &BKList          , &BKChannels            },
#ifdef WANTANSI
        { "MIRC"           , 1, &DisplaymIRC     , NULL                   },
#endif
#ifdef EXTRAS
        { "SHOWSIGN"       , 1, &ShowSignAllChan , NULL                   },
#endif
        { "EXTPUB"         , 1, &ExtPub          , NULL                   },
        { "CHANLOG"        , 3, &ChanLog         , &ChanLogChannels       },
        { "CHANLOGDEST"    , 2, NULL             , &ChanLogDir            },
        { "CHANLOGPREFIX"  , 2, NULL             , &ChanLogPrefix         },
        { "CHANLOGPOST"    , 2, NULL             , &ChanLogPostfix        },
#ifdef EXTRAS
        { "NICKCHAN"       , 1, &ShowNickAllChan , NULL                   },
#endif
        { "AWAYENCR"       , 1, &AwayEncrypt     , NULL                   },
        { "BANTIME"        , 1, &BanTime         , NULL                   },
        { "SHOWCHAN"       , 3, &ShowChan        , &ShowChanChannels      },
        { NULL             , 0, NULL             , NULL                   }
    };

    if (input && *input) {
        strmcpy(locbuf,(char *) input,mybufsize);
        upper(locbuf);
        for (i=0;command_list[i].command;i++)
            if (!strcmp(command_list[i].command,locbuf)) break;
        if (!(command_list[i].command)) {
            malloc_strcpy((char **) &result,empty_string);
            return(result);
        }
        *locbuf='\0';
        switch (command_list[i].type) {
            case 1:
                /* Cdcc limit is special case */
                if (!strcmp(command_list[i].command,"CDCCLIMIT"))
                    snprintf(locbuf,sizeof(locbuf),"%d %d",*(command_list[i].ivar),CdccQueueLimit);
                /* Floodp is special case */
                else if (!strcmp(command_list[i].command,"FLOODP"))
                    snprintf(locbuf,sizeof(locbuf),"%d %d %d",
                            *(command_list[i].ivar),FloodMessages,FloodSeconds);
                else snprintf(locbuf,sizeof(locbuf),"%d",*(command_list[i].ivar));
                break;
            case 2:
                /* Bantype is special case */
                if (!strcmp(command_list[i].command,"BANTYPE")) {
                    *locbuf=tolower(defban);
                    *(locbuf+1)='\0';
                }
                else if (*(command_list[i].svar)) strmcpy(locbuf,*(command_list[i].svar),sizeof(locbuf));
                break;
            case 3:
                if (*(command_list[i].ivar) && *(command_list[i].svar)) {
                    snprintf(locbuf,sizeof(locbuf),"%d %s",*(command_list[i].ivar),*(command_list[i].svar));
                    /* Orignick is special case */
                    if (OrigNickQuiet && !strcmp(command_list[i].command,"ORIGNICK"))
                        *locbuf='2';
                }
                else strcpy(locbuf,"0");
                /* Nhprot is special case */
                if (!strcmp(command_list[i].command,"NHPROT"))
                    snprintf(locbuf+strlen(locbuf),sizeof(locbuf)-strlen(locbuf)," %s",
                            NHDisp == 0 ? "q" : (NHDisp == 1 ? "m" : "f"));
                break;
        }
        if (*locbuf) {
            malloc_strcpy((char **) &result,locbuf);
            return(result);
        }
    }
    malloc_strcpy((char **) &result,empty_string);
    return(result);
}
#endif /* !CELESCRP && !LITE */

u_char *function_stamp(input)
u_char *input;
{
    u_char *result = NULL;
    char *stampbuf = TimeStamp(0);

    if (stampbuf) malloc_strcpy((char **) &result, stampbuf);
    else malloc_strcpy((char **) &result,empty_string);
    return(result);
}

u_char *function_chankey(input)
u_char *input;
{
    char *channel;
    u_char *result = NULL;
    ChannelList *chan;

    if ((channel = next_arg((char *) input,(char **) &channel)) &&
        (chan = lookup_channel(channel, from_server, 0)) && chan->key) {
        malloc_strcpy((char **) &result, chan->key);
    }
    else malloc_strcpy((char **) &result, empty_string);

    return(result);
}

#ifdef HAVE_REGCOMP
u_char *function_regexp(input)
u_char *input;
{
    char *pattern;
    u_char *result = NULL;
    regex_t regex;

    if (((pattern = new_next_arg((char *) input, (char **) &input)) != NULL) &&
        input && *input) {
        if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0) {
            if (regexec(&regex, input, 0, NULL, 0) == 0) {
                malloc_strcpy((char **) &result, "1");
            }
        }
        regfree(&regex);
    }

    if (result == NULL) malloc_strcpy((char **) &result, "0");
    return(result);
}

#define REGREPL_COUNT 10
u_char *function_regexpreplace(input)
u_char *input;
{
    char *x;
    char *pattern;
    char *search;
    char *replace = NULL;
    u_char *result = NULL;
    regex_t regex;

    if (((pattern = new_next_arg((char *) input, (char **) &input)) != NULL) &&
        input && *input) {
        search = pattern;
        if (*search == '/') {
            search++;
            replace = search;
            while (*replace && *replace != '/') replace++;
            if (*replace) {
                *replace++ = '\0';
                x = replace;
                while (*x && *x != '/') x++;
                if (*x == '/') *x++ = '\0';
            }
        }
        if (replace && *replace) {
            if (regcomp(&regex, search, REG_EXTENDED | REG_ICASE) == 0) {
                regmatch_t pmatch[REGREPL_COUNT];
    
                if (regexec(&regex, input, REGREPL_COUNT, pmatch, 0) == 0) {
                    int i;
                    char *tmp = replace;
                    char *tmpstr = NULL;
                    char *prevtmp = replace;

                    while (*tmp) {
                        if (*tmp == '$') {
                            if (prevtmp != tmp) {
                                tmpstr = (char *) new_malloc(tmp - prevtmp + 2);
                                strmcpy(tmpstr, prevtmp, tmp - prevtmp + 1);
                                malloc_strcat((char **) &result, tmpstr);
                                new_free(&tmpstr);
                            }
                            tmp++;
                            i = atoi(tmp);
                            while (*tmp && isdigit(*tmp)) tmp++;
                            prevtmp = tmp;
                            if (i >=0 && i <= REGREPL_COUNT) {
                                int l = pmatch[i].rm_eo - pmatch[i].rm_so;

                                tmpstr = (char *) new_malloc(l + 2);
                                strmcpy(tmpstr, &input[pmatch[i].rm_so], l + 1);
                                malloc_strcat((char **) &result, tmpstr);
                                new_free(&tmpstr);
                            }
                        }
                        else tmp++;
                    }
                    if (prevtmp != tmp) {
                        tmpstr = (char *) new_malloc(tmp - prevtmp + 2);
                        strmcpy(tmpstr, prevtmp, tmp - prevtmp + 1);
                        malloc_strcat((char **) &result, tmpstr);
                        new_free(&tmpstr);
                    }
                }
            }
            regfree(&regex);
        }
    }

    if (result == NULL) malloc_strcpy((char **) &result, empty_string);
    return(result);
}
#endif /* REGCOMP */


/* Removes all aliases */
void DumpAliases(type)
int type;
{
    Alias *tmp;
    Alias *tmpdel;

    for (tmp=alias_list[type];tmp;) {
        tmpdel=tmp;
        tmp=tmp->next;
        new_free(&(tmpdel->name));
        new_free(&(tmpdel->stuff));
        new_free(&tmpdel);
    }
    alias_list[type]=(Alias *) 0;
}

/* Really removes structure */
void DumpAssign(name)
char *name;
{
    int found=0;
    int namelen;
    char locbuf[mybufsize];
    Alias *tmp;
    Alias *tmprem;

    snprintf(locbuf,sizeof(locbuf),"%s.",name);
    namelen=strlen(locbuf);
    for (tmp=alias_list[VAR_ALIAS];tmp;) {
        tmprem=tmp;
        tmp=tmp->next;
        if (!strncmp(tmprem->name,locbuf,namelen))
            if ((tmprem=find_alias(&(alias_list[VAR_ALIAS]),tmprem->name,1,NULL))) {
                new_free(&(tmprem->name));
                new_free(&(tmprem->stuff));
                new_free(&tmprem);
                found=1;
            }
    }
    if (found) say("Assign structure %s removed",name);
    else say("Assign structure %s not found",name);
}

/* Clears assigned structure */
#ifndef LITE
void Purge(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *name;

    if (!(args && *args)) {
        PrintUsage("PURGE name");
        return;
    }
    name=new_next_arg(args,&args);
    upper(name);
    DumpAssign(name);
}
#endif
/****************************************************************************/
