/*
 * alias.c Handles command aliases for irc.c 
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
 * $Id: alias.c,v 1.3 1998-10-11 12:31:03 f Exp $
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

/**************************** Patched by Flier ******************************/
#include "sys/stat.h"
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
#ifndef CELESCRP
extern int  TotalSendDcc _((void));
extern int  TotalQueue _((void));
extern int  is_voiced _((char *, char *));
#endif

extern char *ScrollZlame1;
extern char VersionInfo[];
/****************************************************************************/

extern	char	*FromUserHost;

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
static	char	*built_in_alias _((char));
static	char	*find_inline _((char *));
static	char	*call_function _((char *, char *, char *, int *));
static	void	expander_addition _((char *, char *, int, char *));
static	char	*alias_special_char _((char *, char *, char *, char *, char *, int *));

static	char	*alias_detected _((void));
static	char	*alias_sent_nick _((void));
static	char	*alias_recv_nick _((void));
static	char	*alias_msg_body _((void));
static	char	*alias_joined_nick _((void));
static	char	*alias_public_nick _((void));
static	char	*alias_dollar _((void));
static	char	*alias_channel _((void));
static	char	*alias_server _((void));
static	char	*alias_query_nick _((void));
static	char	*alias_target _((void));
static	char	*alias_nick _((void));
static	char	*alias_invite _((void));
static	char	*alias_cmdchar _((void));
static	char	*alias_line _((void));
static	char	*alias_away _((void));
static	char	*alias_oper _((void));
static	char	*alias_chanop _((void));
static	char	*alias_modes _((void));
static	char	*alias_buffer _((void));
static	char	*alias_time _((void));
static	char	*alias_version _((void));
static	char	*alias_currdir _((void));
static	char	*alias_current_numeric _((void));
static	char	*alias_server_version _((void));

/**************************** Patched by Flier ******************************/
static	char	*alias_ScrollZ_version _((void));
/*static	char	*function_pattern _((void));
static	char	*function_chops _((void));
static	char	*function_chnops _((void));*/
/* Patched by Zakath */
#ifdef CELE
static	char	*alias_Celerity_version _((void));
#endif
/* ***************** */
/****************************************************************************/

typedef struct
{
	char	name;
	char	*(*func) _((void));
}	BuiltIns;

static	FAR BuiltIns built_in[] =
{
	{ '.',		alias_sent_nick },
	{ ',',		alias_recv_nick },
	{ ':',		alias_joined_nick },
	{ ';',		alias_public_nick },
	{ '$',		alias_dollar },
	{ 'A',		alias_away },
	{ 'B',		alias_msg_body },
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
	{ 'L',		alias_line },
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

	char	FAR command_line[BIG_BUFFER_SIZE+1];

	char	*function_left _((unsigned char *));
	char	*function_right _((unsigned char *));
	char	*function_mid _((unsigned char *));
	char	*function_rand _((unsigned char *));
	char	*function_srand _((unsigned char *));
	char	*function_time _((unsigned char *));
	char	*function_stime _((unsigned char *));
	char	*function_index _((unsigned char *));
	char	*function_rindex _((unsigned char *));
	char	*function_match _((unsigned char *));
	char	*function_rmatch _((unsigned char *));
	char	*function_userhost _((unsigned char *));
	char	*function_strip _((unsigned char *));
	char	*function_encode _((unsigned char *));
	char	*function_decode _((unsigned char *));
	char	*function_ischannel _((unsigned char *));
	char	*function_ischanop _((unsigned char *));
	char	*function_word _((unsigned char *));
	char	*function_winnum _((unsigned char *));
	char	*function_winnam _((unsigned char *));
	char	*function_connect _((unsigned char *));
	char	*function_listen _((unsigned char *));
	char	*function_tdiff _((unsigned char *));
	char	*function_toupper _((unsigned char *));
	char	*function_tolower _((unsigned char *));
	char	*function_channels _((unsigned char *));
	char	*function_servers _((unsigned char *));
	char	*function_curpos _((unsigned char *));
	char	*function_onchannel _((unsigned char *));
	char	*function_pid _((unsigned char *));
	char	*function_ppid _((unsigned char *));
/**************************** PATCHED by Flier ******************************/
#ifndef CELESCRP
	char	*function_chanusers _((unsigned char *));
#endif
/****************************************************************************/
        char	*function_idle _((unsigned char *));
#ifdef HAVE_STRFTIME
	char	*function_strftime _((unsigned char *));
#endif
/**************************** Patched by Flier ******************************/
        char    *function_open _((unsigned char *));
        char    *function_close _((unsigned char *));
        char    *function_write _((unsigned char *));
        char    *function_read _((unsigned char *));
        char    *function_eof _((unsigned char *));
        char    *function_rename _((unsigned char *));
        char    *function_intuhost _((unsigned char *));
        char    *function_checkuser _((unsigned char *));
        char    *function_checkshit _((unsigned char *));
        char    *function_stripansi _((unsigned char *));
#ifndef CELESCRP
	char	*function_isvoiced _((unsigned char *));
        char	*function_uhost _((unsigned char *));
	char	*function_hhost _((unsigned char *));
	char	*function_topic _((unsigned char *));
	char	*function_strlen _((unsigned char *));
        char	*function_strnum _((unsigned char *));
#endif
        char	*function_fsize _((unsigned char *));
#ifndef CELESCRP
        char	*function_sar _((unsigned char *));
        char	*function_cdccslots _((unsigned char *));
        char	*function_cdccqslots _((unsigned char *));
        char	*function_url _((unsigned char *));
        char	*function_strstr _((unsigned char *));
        char	*function_szvar _((unsigned char *));
#endif
#ifdef COUNTRY
        char	*function_country _((unsigned char *));
#endif
#ifdef WANTANSI
        char    *function_color _((unsigned char *));
#endif
/*        char    *function_pattern _((unsigned char *));
        char    *function_chops _((unsigned char *));
        char    *function_chnops _((unsigned char *));*/
/****************************************************************************/

typedef struct
{
	char	*name;
	char	*(*func) _((unsigned char *));
}	BuiltInFunctions;

static BuiltInFunctions	FAR built_in_functions[] =
{
/**************************** Patched by Flier ******************************/
	{ "UH",                 function_intuhost },
/****************************************************************************/
	{ "LEFT",		function_left },
	{ "RIGHT",		function_right },
	{ "MID",		function_mid },
	{ "RAND",		function_rand },
	{ "SRAND",		function_srand },
	{ "TIME",		function_time },
	{ "TDIFF",		function_tdiff },
	{ "STIME",		function_stime },
	{ "INDEX",		function_index },
	{ "RINDEX",		function_rindex },
	{ "MATCH",		function_match },
	{ "RMATCH",		function_rmatch },
	{ "USERHOST",		function_userhost },
	{ "STRIP",		function_strip },
	{ "ENCODE",		function_encode },
	{ "DECODE",		function_decode },
/**************************** Patched by Flier ******************************/
	{ "STRIPANSI",          function_stripansi },
#ifndef CELESCRP
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
/**************************** PATCHED by Flier ******************************/
#ifndef CELESCRP
        { "ISVOICED",		function_isvoiced },
#endif
/****************************************************************************/
	{ "WORD",		function_word },
	{ "WINNUM",		function_winnum },
	{ "WINNAM",		function_winnam },
	{ "CONNECT",		function_connect },
	{ "LISTEN",		function_listen },
	{ "TOUPPER",		function_toupper },
	{ "TOLOWER",		function_tolower },
	{ "MYCHANNELS",		function_channels },
	{ "MYSERVERS",		function_servers },
	{ "CURPOS",		function_curpos },
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
        { "COLOR",              function_color },
#endif
/****************************************************************************/
	{ "ONCHANNEL",		function_onchannel },
	{ "PID",		function_pid },
	{ "PPID",		function_ppid },
/**************************** PATCHED by Flier ******************************/
#ifndef CELESCRP
        { "CHANUSERS",		function_chanusers },
#endif
/****************************************************************************/
#ifdef HAVE_STRFTIME
	{ "STRFTIME",		function_strftime },
#endif
	{ "IDLE",		function_idle },
/**************************** Patched by Flier ******************************/
	/*{ "PATTERN",            function_pattern },
	{ "CHOPS",              function_chops },
	{ "CHNOPS",             function_chnops },*/
#ifndef CELESCRP
        { "SZVAR",              function_szvar },
        { "TOPIC",              function_topic },
        { "SAR",                function_sar },
#endif
#ifdef COUNTRY
        { "COUNTRY",            function_country },
#endif
#ifndef CELESCRP
        { "URL",                function_url },
        { "CDCCSLOTS",          function_cdccslots },
        { "CDCCQSLOTS",         function_cdccqslots },
#endif
        { "OPEN",               function_open },
        { "FSIZE",              function_fsize },
	{ "CLOSE",              function_close },
	{ "READ",               function_read },
	{ "WRITE",              function_write },
        { "EOF",                function_eof },
        { "RENAME",             function_rename },
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
static	char	* FAR function_stack[128] =
{ 
	(char *) 0
};
static	int	function_stkptr = 0;

/*
 * find_alias: looks up name in in alias list.  Returns the Alias	entry if
 * found, or null if not found.   If unlink is set, the found entry is
 * removed from the list as well.  If match is null, only perfect matches
 * will return anything.  Otherwise, the number of matches will be returned. 
 */
static	Alias	*
find_alias(list, name, unlink, match)
	Alias	**list;
	char	*name;
	int	unlink;
	int	*match;
{
	Alias	*tmp,
		*last = (Alias *) 0;
	int	cmp,
		len;
	int	(*cmp_func) _((char *, char *, int));

	if (match)
	{
		*match = 0;
		cmp_func = my_strnicmp;
	}
	else
		cmp_func = (int (*) _((char *, char *, int))) my_stricmp;
	if (name)
	{
		len = strlen(name);
		for (tmp = *list; tmp; tmp = tmp->next)
		{
			if ((cmp = cmp_func(name, tmp->name, len)) == 0)
			{
				if (unlink)
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
insert_alias(list, alias)
	Alias	**list;
	Alias	*alias;
{
	Alias	*tmp,
		*last,
		*foo;

	last = (Alias *) 0;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (strcmp(alias->name, tmp->name) < 0)
			break;
		last = tmp;
	}
	if (last)
	{
		foo = last->next;
		last->next = alias;
		alias->next = foo;
	}
	else
	{
		alias->next = *list;
		*list = alias;
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
			malloc_strcpy(&function_stack[function_stkptr], stuff);
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

static	char	*
#ifdef __STDC__
built_in_alias(char c)
#else
built_in_alias(c)
	char	c;
#endif
{
	BuiltIns	*tmp;
	char	*ret = (char *) 0;

	for(tmp = built_in;tmp->name;tmp++)
		if (c == tmp->name)
		{
			malloc_strcpy(&ret, tmp->func());
			break;
		}
	return(ret);
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
	Alias	*alias;
	char	*ret = NULL;
	char	*tmp;

	if ((alias = find_alias(&(alias_list[VAR_ALIAS]), str, 0, (int *) NULL))
			!= NULL)
	{
		malloc_strcpy(&ret, alias->stuff);
		return (ret);
	}
	if ((strlen(str) == 1) && (ret = built_in_alias(*str)))
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

static	char	*
call_function(name, f_args, args, args_flag)
	char	*name,
		*f_args,
		*args;
	int	*args_flag;
{
	unsigned char	*tmp;
	char	*result = (char *) 0;
	char	*sub_buffer = (char *) 0;
	int	builtnum;
	char	*debug_copy = (char *) 0;
	char	*cmd = (char *) 0;

	malloc_strcpy(&cmd, name);
	upper(cmd);
	tmp = (unsigned char *) expand_alias((char *) 0, f_args, args, args_flag, NULL);
	if (get_int_var(DEBUG_VAR) & DEBUG_FUNCTIONS)
		malloc_strcpy(&debug_copy, (char *) tmp);
	for (builtnum = 0; built_in_functions[builtnum].name != NULL &&
			strcmp(built_in_functions[builtnum].name, cmd);
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
		function_stack[++function_stkptr] = (char *) 0;
		parse_command(sub_buffer, 0, empty_string);
		new_free(&sub_buffer);
		eval_args=1;
		result = function_stack[function_stkptr];
		function_stack[function_stkptr] = (char *) 0;
		if (!result)
			malloc_strcpy(&result, empty_string);
		function_stkptr--;
	}
	if (debug_copy)
	{
		yell("Function %s(%s) returned %s",
		    name, debug_copy, result);
		new_free(&debug_copy);
	}
	new_free(&tmp);
	return result;
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
				if (!(ptr2 = MatchingBracket(ptr+1, '(', ')')))
					ptr = ptr+strlen(ptr)-1;
				else
					ptr = ptr2;
				break;
			}
			*ptr++ = '\0';
			right = ptr;
			ptr = MatchingBracket(right, LEFT_PAREN, RIGHT_PAREN);
			if (ptr)
				*ptr++ = '\0';
			result1 = call_function(str, right, args, arg_flag);
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
				if (!(ptr2 = MatchingBracket(ptr+1, '[', ']')))
					ptr = ptr+strlen(ptr)-1;
				else
					ptr = ptr2;
				break;
			}
			*ptr++ = '\0';
			right = ptr;
			ptr = MatchingBracket(right, LEFT_BRACKET, RIGHT_BRACKET);
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
                                        malloc_strcpy(&result1,zero);

                                {           /* This isnt supposed to be
                                                attached to the if, so
                                                dont "fix" it. */
                                        int r;
                                        r = atoi(result1);
                                        if (*ptr == '+')
                                                r++;
                                        else    r--;
                                        sprintf(tmp, "%d", r);
                                        display = window_display;
                                        window_display = 0;
                                        add_alias(VAR_ALIAS,tptr,tmp);
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
			sprintf(tmp, "%ld", value3);
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
			sprintf(tmp, "%ld", value3);
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
                                sprintf(tmp, "%ld",value3);
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
                                sprintf(tmp, "%ld",value3);
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
                                sprintf(tmp, "%ld",value3);
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
				        LEFT_BRACKET, RIGHT_BRACKET)) != NULL)
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
				malloc_strcpy(&result1, zero);
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
			sprintf(tmp, "%ld", value2);
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
				sprintf(tmp, "%ld", value2);
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
				sprintf(tmp, "%ld", value2);
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
				malloc_strcpy(&result1, zero);
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
			sprintf(tmp, "%ld", value1);
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
                return empty_string;
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
	char	*buff,
		*add;
	int	length;
	char	*quote_em;
{
	char	format[40],
		*ptr;

	if (length)
	{
		sprintf(format, "%%%d.%ds", -length, (length < 0 ? -length :
			length));
		sprintf(buffer, format, add);
		add = buffer;
	}
	if (quote_em)
	{
		ptr = double_quote(add, quote_em);
		strmcat(buff, ptr, BIG_BUFFER_SIZE);
		new_free(&ptr);
	}
	else
                if (buff)
		        strmcat(buff, add, BIG_BUFFER_SIZE);
}

/* MatchingBracket returns the next unescaped bracket of the given type */
char	*
#ifdef __STDC__
MatchingBracket(char *string, char left, char right)
#else
MatchingBracket(string, left, right)
	char	*string;
	char	left;
	char	right;
#endif
{
	int	bracket_count = 1;

	while (*string && bracket_count)
	{
		if (*string == left)
			bracket_count++;
		else if (*string == right)
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
 * destination buffer (of size BIG_BUFFER_SIZE) to which things are appended,
 * a ptr to the string (the first character of which is the special
 * character, the args to the alias, and a character indication what
 * characters in the string should be quoted with a backslash.  It returns a
 * pointer to the character right after the converted alias.

 The args_flag is set to 1 if any of the $n, $n-, $n-m, $-m, $*, or $() is used
 in the alias.  Otherwise it is left unchanged.
 */
/*ARGSUSED*/
static	char	*
alias_special_char(name, buffer, ptr, args, quote_em,args_flag)
	char	*name;
	char	*buffer;
	char	*ptr;
	char	*args;
	char	*quote_em;
	int	*args_flag;
{
	char	*tmp,
		c;
	int	upper,
	lower,
	length;

	length = 0;
	if ((c = *ptr) == LEFT_BRACKET)
	{
		ptr++;
		if ((tmp = (char *) index(ptr, RIGHT_BRACKET)) != NULL)
		{
			*(tmp++) = (char) 0;
			length = atoi(ptr);
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
			char	sub_buffer[BIG_BUFFER_SIZE+1];

			if ((ptr = MatchingBracket(tmp, LEFT_PAREN,
			    RIGHT_PAREN)) || (ptr = (char *) index(tmp,
			    RIGHT_PAREN)))
				*(ptr++) = (char) 0;
			tmp = expand_alias((char *) 0, tmp, args, args_flag,
				NULL);
			*sub_buffer = (char) 0;
			alias_special_char((char *) 0, sub_buffer, tmp,
				args, quote_em,args_flag);
			expander_addition(buffer, sub_buffer, length, quote_em);
			new_free(&tmp);
			*args_flag = 1;
		}
		return (ptr);
	case '!':
		if ((ptr = (char *) index(tmp, '!')) != NULL)
			*(ptr++) = (char) 0;
		if ((tmp = do_history(tmp, empty_string)) != NULL)
		{
			expander_addition(buffer, tmp, length, quote_em);
			new_free(&tmp);
		}
		return (ptr);
	case LEFT_BRACE:
		if ((ptr = (char *) index(tmp, RIGHT_BRACE)) != NULL)
			*(ptr++) = (char) 0;
		if ((tmp = parse_inline(tmp, args, args_flag)) != NULL)
		{
			expander_addition(buffer, tmp, length, quote_em);
			new_free(&tmp);
		}
		return (ptr);
	case DOUBLE_QUOTE:
		if ((ptr = (char *) index(tmp, DOUBLE_QUOTE)) != NULL)
			*(ptr++) = (char) 0;
		alias_string = (char *) 0;
			/* XXX - the cast in the following is an ugly hack! */
		if (irc_io(tmp, (void (*) _((unsigned char, char *))) do_alias_string, use_input, 1))
		{
			yell("Illegal recursive edit");
			break;
		}
		expander_addition(buffer, alias_string, length, quote_em);
		new_free(&alias_string);
		return (ptr);
	case '*':
		expander_addition(buffer, args, length, quote_em);
		*args_flag = 1;
		return (ptr + 1);
	default:
		if (isdigit(c) || (c == '-') || c == '~')
		{
			*args_flag = 1;
			if (*ptr == '~')
			{
				lower = upper = LAST_ARG;
				ptr++;
			}
			else
			{
				lower = parse_number(&ptr);
				if (*ptr == '-')
				{
					ptr++;
					upper = parse_number(&ptr);
				}
				else
					upper = lower;
			}
			expander_addition(buffer, arg_number(lower, upper,
				args), length, quote_em);
			return (ptr ? ptr : empty_string);
		}
		else
		{
			char	*rest,
				c = (char) 0;

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
				c = *rest;
				*rest = (char) 0;
			}
			else if ((rest = sindex(ptr+1, alias_illegals)) != NULL)
			{
				if (isalpha(*ptr) || *ptr == '_')
					while ((*rest == LEFT_BRACKET ||
					    *rest == LEFT_PAREN) &&
					    (tmp = MatchingBracket(rest+1,
					    *rest, (*rest == LEFT_BRACKET) ?
					    RIGHT_BRACKET: RIGHT_PAREN)))
						rest = tmp + 1;
				c = *rest;
				*rest = (char) 0;
			}
			if ((tmp = parse_inline(ptr, args, args_flag)) != NULL)
			{
				expander_addition(buffer, tmp, length,
					quote_em);
				new_free(&tmp);
			}
			if (rest)
				*rest = c;
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
expand_alias(name, string, args,args_flag, more_text)
	char	*name,
		*string,
		*args;
	int	*args_flag;
	char	**more_text;
{
	char	buffer[BIG_BUFFER_SIZE + 1],
		*ptr,
		*stuff = (char *) 0,
		*free_stuff;
	char	*quote_em,
		*quote_str = (char *) 0;
	char	ch;
	int	quote_cnt = 0;
	int	is_quote = 0;
	void	(*str_cat) _((char *, char *, int));

	if (*string == '@' && more_text)
	{
		str_cat = strmcat;
		*args_flag = 1; /* Stop the @ command from auto appending */
	}
	else
		str_cat = strmcat_ue;
	malloc_strcpy(&stuff, string);
	free_stuff = stuff;
	*buffer = (char) 0;
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
			(*str_cat)(buffer, stuff, BIG_BUFFER_SIZE);
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
			stuff = alias_special_char(name, buffer, ptr, args,
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
			(*str_cat)(buffer, stuff, BIG_BUFFER_SIZE);
			stuff = ptr;
			*args_flag = 1;
			if (!(ptr = MatchingBracket(stuff + 1, ch,
					(ch == LEFT_PAREN) ?
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
			strmcat(buffer, stuff, BIG_BUFFER_SIZE);
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
		(*str_cat)(buffer, stuff, BIG_BUFFER_SIZE);
	ptr = (char *) 0;
	new_free(&free_stuff);
	malloc_strcpy(&ptr, buffer);
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
	int	len;
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
 	int	len, lastlog_level;
	int	DotLoc,
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
 	(void) message_from_level(lastlog_level);
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
execute_alias(alias_name, alias, args)
	char	*alias_name,
		*alias,
		*args;
{
	if (mark_alias(alias_name, 1))
		say("Maximum recursion count exceeded in: %s", alias_name);
	else
	{
		parse_line(alias_name, alias, args, 0,1);
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
static	char	*
alias_line()
{
	return (get_input());
}

static	char	*
alias_buffer()
{
	return (cut_buffer);
}

static	char	*
alias_time()
{
	return (update_clock(GET_TIME));
}

static	char	*
alias_dollar()
{
	return ("$");
}

static	char	*
alias_detected()
{
	return (last_notify_nick);
}

static	char	*
alias_nick()
{
/**************************** PATCHED by Flier ******************************/
	/*return (get_server_nickname(curr_scr_win->server));*/
	return (get_server_nickname(from_server));
/****************************************************************************/
}

static	char	*
alias_away()
{
	return (server_list[curr_scr_win->server].away);
}

static	char	*
alias_sent_nick()
{
	return (sent_nick) ? sent_nick : empty_string;
}

static	char	*
alias_recv_nick()
{
	return (recv_nick) ? recv_nick : empty_string;
}

static	char	*
alias_msg_body()
{
	return (sent_body) ? sent_body : empty_string;
}

static	char	*
alias_joined_nick()
{
	return (joined_nick) ? joined_nick : empty_string;
}

static	char	*
alias_public_nick()
{
	return (public_nick) ? public_nick : empty_string;
}

static	char	*
alias_channel()
{
	char	*tmp;

	return (tmp = get_channel_by_refnum(0)) ? tmp : zero;
}

static	char	*
alias_server()
{
	return (parsing_server_index != -1) ?
		get_server_itsname(parsing_server_index) :
		(get_window_server(0) != -1) ?
			get_server_itsname(get_window_server(0)) : empty_string;
}

static	char	*
alias_query_nick()
{
	char	*tmp;

	return (tmp = query_nick()) ? tmp : empty_string;
}

static	char	*
alias_target()
{
	char	*tmp;

	return (tmp = get_target_by_refnum(0)) ? tmp : empty_string;
}

static	char	*
alias_invite()
{
	return (invite_channel) ? invite_channel : empty_string;
}

static	char	*
alias_cmdchar()
{
	static	char	thing[2];
	char	*cmdchars;

	if ((cmdchars = get_string_var(CMDCHARS_VAR)) == (char *) 0)
		cmdchars = DEFAULT_CMDCHARS;
	thing[0] = cmdchars[0];
	thing[1] = (char) 0;
	return(thing);
}

static	char	*
alias_oper()
{
	return get_server_operator(from_server) ?
		get_string_var(STATUS_OPER_VAR) : empty_string;
}

static	char	*
alias_chanop()
{
	char	*tmp;

	return ((tmp = get_channel_by_refnum(0)) && get_channel_oper(tmp,
			from_server)) ?
		"@" : empty_string;
}

static	char	*
alias_modes()
{
	char	*tmp;

	return (tmp = get_channel_by_refnum(0)) ?
		get_channel_mode(tmp, from_server) : empty_string;
}

static	char	*
alias_version()
{
	return (internal_version);
}

static	char	*
alias_currdir()
{
	static	char	dirbuf[1024];

        getcwd(dirbuf, BIG_BUFFER_SIZE+1);
	return (dirbuf);
}

static	char	*
alias_current_numeric()
{
	static	char	number[4];
	
	sprintf(number, "%03d", -current_numeric);
	return (number);
}

static	char	*
alias_server_version()
{
	char	*s;

	return ((s = server_list[curr_scr_win->server].version_string) ?
	    s : empty_string);
}

/**************************** PATCHED by Flier ******************************/
static char *alias_ScrollZ_version() {
    static char tmpbuf[mybufsize/16];

    sprintf(tmpbuf,"%s [%s]",ScrollZlame1,VersionInfo);
    return(tmpbuf);
}

#ifdef CELE
static char *alias_Celerity_version() {
    static char tmpbuf[32];

    sprintf(tmpbuf,"%s",CelerityVersion);
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
					LEFT_BRACKET, RIGHT_BRACKET)) != NULL)
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
				char	*ptr = MatchingBracket(++rest,
						LEFT_BRACE, RIGHT_BRACE);
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



char	*
function_left(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*count;
	int	cvalue;

	count = next_arg((char *) input, (char **) &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if ((int) strlen((char *) input) > cvalue)
		input[cvalue] = '\0';
	malloc_strcpy(&result, (char *) input);
	return result;
}

char	*
function_right(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
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
	malloc_strcpy(&result, (char *) input);
	return result;
}

char	*
function_mid(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
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
	if ((int) strlen((char *) input) > ivalue)
		input += ivalue;
	else
		*input = '\0';
	if ((int) strlen((char *) input) > cvalue)
		input[cvalue] = '\0';
	malloc_strcpy(&result, (char *) input);
	return result;
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

char	*
function_rand(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	tmp[40];
/**************************** PATCHED by Flier ******************************/
	/*long	tempin;*/
        int     value=0;
/****************************************************************************/

#ifdef _Windows
	sprintf(tmp, "%ld", random(atol(input)));
#else
/**************************** PATCHED by Flier ******************************/
	/*sprintf(tmp, "%ld", (tempin = atol((char *) input)) ? randm(0L) % tempin : 0);*/
        if (input && * input) value=atoi(input);
        if (!value) value=1;
        sprintf(tmp,"%d",(input && *input)?rand()%value:rand());
/****************************************************************************/
#endif /* _Windows */
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_srand(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

/**************************** PATCHED by Flier ******************************/
	/*if (input && *input)
		(void) randm(atol((char *) input));
	else
		(void) randm((time_t) time(NULL));*/
	if (input && *input) srand(atol((char *) input));
	else srand(time((time_t *) 0));
/****************************************************************************/
	malloc_strcpy(&result, empty_string);
	return result;
}

/*ARGSUSED*/
char	*
function_time(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	time_t	ltime;
	char	tmp[40];

	(void) time(&ltime);
	sprintf(tmp, "%ld", ltime);
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_stime(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	time_t	ltime;

	ltime = atol((char *) input);
	malloc_strcpy(&result, ctime(&ltime));
	result[strlen(result) - 1] = (char) 0;
	return result;
}

char	*
function_tdiff(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
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
		sprintf(tstr, "%ld day%s ", (long) days, (days==1)?"":"s");
		tstr += strlen(tstr);
	}
	if (hours)
	{
		sprintf(tstr, "%ld hour%s ", (long) hours, (hours==1)?"":"s");
		tstr += strlen(tstr);
	}
	if (minutes)
	{
		sprintf(tstr, "%ld minute%s ", (long) minutes, (minutes==1)?"":"s");
		tstr += strlen(tstr);
	}
	if (seconds || (!days && !hours && !minutes))
	{
		sprintf(tstr, "%ld second%s", (long) seconds, (seconds==1)?"":"s");
		tstr += strlen(tstr);
	}
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_index(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*schars;
	char	*iloc;
	int	ival;
	char	tmp[40];

	schars = next_arg((char *) input, (char **) &input);
	iloc = (schars) ? sindex((char *) input, schars) : NULL;
	ival = (iloc) ? iloc - (char *) input : -1;
	sprintf(tmp, "%d", ival);
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_rindex(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*schars;
	char	*iloc;
	int	ival;
	char	tmp[40];

	schars = next_arg((char *) input, (char **) &input);
 	iloc = (schars) ? srindex((char *) input, schars) : NULL;
 	ival = (iloc) ? (char *)input + strlen(input) - iloc : -1;
	sprintf(tmp, "%d", ival);
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_match(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
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
	sprintf(tmp, "%d", match);
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_rmatch(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
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
	sprintf(tmp, "%d", match);
	malloc_strcpy(&result, tmp);
	return result;
}

/*ARGSUSED*/
char	*
function_userhost(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, FromUserHost ? FromUserHost : empty_string);
	return result;
}

char	*
function_strip(input)
	unsigned char	*input;
{
	static	char	FAR result[BIG_BUFFER_SIZE+1];
	char	*retval = (char *) 0;
	char	*chars;
	char	*cp, *dp;

	if ((chars = next_arg((char *) input, (char **) &input)) && input)
	{
		for (cp = (char *) input, dp = result; *cp; cp++)
		{
			if (!index(chars, *cp))
				*dp++ = *cp;
		}
		*dp = '\0';
	}
	malloc_strcpy(&retval, result);
	return retval;
}

char	*
function_encode(input)
	unsigned char	*input;
{
	static unsigned char	FAR result[BIG_BUFFER_SIZE+1];
	char	*retval = (char *) 0;
	unsigned char	*c;
	int	i = 0;

	for (c = input; *c; c++)
	{
		result[i++] = (*c >> 4) + 0x41;
		result[i++] = (*c & 0x0f) + 0x41;
	}
	result[i] = '\0';
	malloc_strcpy(&retval, (char *) result);
	return retval;
}


char	*
function_decode(input)
	unsigned char	*input;
{
	static unsigned	char	FAR result[BIG_BUFFER_SIZE+1];
	char	*retval = (char *) 0;
	unsigned char	*c;
	unsigned char	d,e;
	int	i = 0;

	c = input;
	while((d = *c) && (e = *(c+1)))
	{
		result[i] = ((d - 0x41) << 4) | (e - 0x41);
		c += 2;
		i++;
	}
	result[i] = '\0';
	malloc_strcpy(&retval, (char *) result);
	return retval;
}

char	*
function_ischannel(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, is_channel((char *) input) ? one : zero);
	return result;
}

char	*
function_ischanop(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*nick;
	char	*channel = NULL;

	if (!(nick = next_arg((char *) input, &channel)))
		malloc_strcpy(&result, zero);
	else
		malloc_strcpy(&result, is_chanop(channel, nick) ? one : zero);
	return result;
}


char	*
function_word(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*count;
	int	cvalue;
	char	*word;

	count = next_arg((char *) input, (char **) &input);
	if (count)
		cvalue = atoi(count);
	else
		cvalue = 0;
	if (cvalue < 0)
		malloc_strcpy(&result, empty_string);
	else
	{
		for (word = next_arg((char *) input, (char **) &input); word && cvalue--;
				word = next_arg((char *) input, (char **) &input))
			;
		malloc_strcpy(&result, (word) ? word : empty_string);
	}
	return result;
}


char	*
function_winnum(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	tmp[10];

	if (curr_scr_win)
		sprintf(tmp, "%d", curr_scr_win->refnum);
	else
		strcpy(tmp, "-1");
	malloc_strcpy(&result, tmp);
	return result;
}

char	*
function_winnam(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

	malloc_strcpy(&result, (curr_scr_win && curr_scr_win->name) ?
		curr_scr_win->name : empty_string);
	return result;
}

char	*
function_connect(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*host;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		put_it("You are not permitted to use CONNECT()");
	else
#endif
		if ((host = next_arg((char *) input, (char **) &input)) != NULL)
			result = dcc_raw_connect(host, (u_short) atoi((char *) input));
	return result;
}


char	*
function_listen(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		malloc_strcpy(&result, zero);
	else
#endif
		result = dcc_raw_listen((u_short) atoi((char *) input));
	return result;
}

char	*
function_toupper(input)
	unsigned char	*input;
{
	char	*new = (char *) 0,
		*ptr;

	if (!input)
		return empty_string;
	malloc_strcpy(&new, (char *) input);
	for (ptr = new; *ptr; ptr++)
		*ptr = islower(*ptr) ? toupper(*ptr) : *ptr;
	return new;
}

char	*
function_tolower(input)
	unsigned char	*input;
{
	char	*new = (char *) 0,
		*ptr;

	if (!input)
		return empty_string;
	malloc_strcpy(&new, (char *) input);
	for (ptr = new; *ptr; ptr++)
		*ptr = (isupper(*ptr)) ? tolower(*ptr) : *ptr;
	return new;
}

char	*
function_curpos(input)
	unsigned char	*input;
{
	char	*new = (char *) 0,
		pos[4];

	sprintf(pos, "%d", current_screen->buffer_pos);
	malloc_strcpy(&new, pos);
	return new;
}

char	*
function_channels(input)
	unsigned char	*input;
{
	Window	*window;

	if (input)
		window = isdigit(*input) ? get_window_by_refnum(atoi((char *) input))
					 : curr_scr_win;
	else
		window = curr_scr_win;

	return create_channel_list(window);
}

char	*
function_servers(input)
	unsigned char	*input;
{
	return create_server_list();
}

char	*
function_onchannel(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;
	char	*nick;
	char	*channel = NULL;

	if (!(nick = next_arg((char *) input, &channel)))
		malloc_strcpy(&result, zero);
	else
		malloc_strcpy(&result,
			is_on_channel(channel, from_server, nick) ? one : zero);
	return result;
}

char	*
function_pid(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%d", (int) getpid());
	malloc_strcpy(&result, buffer);
	return result;
}

char	*
function_ppid(input)
	unsigned char	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%d", (int) getppid());
	malloc_strcpy(&result, buffer);
	return result;
}

/**************************** PATCHED by Flier ******************************/
/* Modified so if you call it $chanusers(#blah 1) it will return
   @nick if nick is channel operator
   +nick if nick is voiced
   .nick otherwise */
#ifndef CELESCRP
/****************************************************************************/
char	*
function_chanusers(input)
	unsigned char	*input;
{
	ChannelList	*chan;
	NickList	*nicks;
	char	*result = (char *) 0;
	int	len = 0;
	int	notfirst = 0;
/**************************** PATCHED by Flier ******************************/
        int  nickst=0;
        char *channel;
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
	/*chan = lookup_channel((char *) input, from_server, CHAN_NOUNLINK);*/
        channel=new_next_arg(input,(char **) &input);
        chan=lookup_channel(channel,from_server,CHAN_NOUNLINK);
        if (input && *input=='1') nickst=1;
/****************************************************************************/
	if ((ChannelList *) 0 == chan)
		return (char *) 0;

	*buffer = '\0';

	for (nicks = chan->nicks; nicks; nicks = nicks->next)
	{
/**************************** PATCHED by Flier ******************************/
		/*len += strlen(nicks->nick);
		if (len > BIG_BUFFER_SIZE)*/
		len+=strlen(nicks->nick)+1;
                if (nickst) len++;
		if (len>BIG_BUFFER_SIZE-100)
/****************************************************************************/
		{
			malloc_strcat(&result, buffer);
			*buffer = '\0';
			len = 0;
/**************************** PATCHED by Flier ******************************/
                        if (strlen(result)>2*BIG_BUFFER_SIZE-100) break;
/****************************************************************************/
		}
		if (notfirst)
			strcat(buffer, " ");
		else
			notfirst = 1;
/**************************** PATCHED by Flier ******************************/
                if (nickst) {
                    if (nicks->chanop) strcat(buffer,"@");
                    else if (nicks->voice) strcat(buffer,"+");
                    else strcat(buffer,".");
                }
/****************************************************************************/
		strcat(buffer, nicks->nick);
        }
        malloc_strcat(&result, buffer);
	return result;
}
/**************************** PATCHED by Flier ******************************/
#endif /* CELESCRP */
/****************************************************************************/

/*
 * strftime() patch from hari (markc@arbld.unimelb.edu.au)
 */
#ifdef HAVE_STRFTIME
char	*
function_strftime(input)
	unsigned char 	*input;
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
			char *	s = (char *) 0;

			malloc_strcpy(&s, result);
			return s;
		}
		else
			return (char *) 0;
	}
	else
	{
		return (char *) 0;
	}
}
#endif


/*
 * idle() patch from scottr (scott.reynolds@plexus.com)
 */
char	*
function_idle(input)
	unsigned char 	*input;
{
	char	*result = (char *) 0;

	sprintf(buffer, "%ld", (long)(time(0) - idle_time));
	malloc_strcpy(&result, buffer);
	return result;
}

/**************************** Patched by Flier ******************************/
char *function_open(words)
unsigned char *words;
{
    char *result=(char *) 0;
    char *filename=(char *) 0;

    filename=next_arg((char *) words,(char **) &words);
    if (words && *words) {
        *(words+1)='\0';
        upper(words);
        if (*words=='R') sprintf(buffer,"%d",OpenFileRead(filename));
        else if (*words=='W' || *words=='A')
            sprintf(buffer,"%d",OpenFileWrite(filename,words));
    }
    else strcpy(buffer,"-1");
    malloc_strcpy(&result,buffer);
    return(result);
}

char *function_close(words)
unsigned char *words;
{
    char *args=(char *) 0;
    char *result=(char *) 0;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        sprintf(buffer,"%d",FileClose(atoi(args)));
    }
    else strcpy(buffer,"-1");
    malloc_strcpy(&result,buffer);
    return(result);
}	

char *function_read(words)
unsigned char *words;
{
    char *args=(char *) 0;
    char *result=(char *) 0;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        return(FileRead(atoi(args)));
    }
    else {
        malloc_strcpy(&result,"-1");
        return(result);
    }
}	

char *function_write(words)
unsigned char *words;
{
    char *args=(char *) 0;
    char *result=(char *) 0;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        if (words && *words) sprintf(buffer,"%d",FileWrite(atoi(args),words));
        else strcpy(buffer,"-1");
    }
    else strcpy(buffer,"-1");
    malloc_strcpy(&result,buffer);
    return(result);
}

char *function_eof(words)
unsigned char *words;
{
    char *args=(char *) 0;
    char *result=(char *) 0;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        sprintf(buffer,"%d",FileEof(atoi(args)));
    }
    else strcpy(buffer,"-1");
    malloc_strcpy(&result,buffer);
    return(result);
}

char *function_rename(words)
unsigned char *words;
{
    char *oldname=(char *) 0;
    char *newname=(char *) 0;
    char *result=(char *) 0;

    if (words && *words) {
        oldname=next_arg((char *) words,(char **) &words);
        newname=next_arg((char *) words,(char **) &words);
        if (newname && *newname) {
            sprintf(buffer,"%d",rename(oldname,newname));
            malloc_strcpy(&result,buffer);
        }
        else malloc_strcpy(&result,"-1");
    }
    else malloc_strcpy(&result,"-1");
    return(result);
}	

char *function_intuhost(words)
unsigned char *words;
{
    char *result=(char *) 0;
    char *args=(char *) 0;
    NickList *joiner;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        joiner=CheckJoiners(args,0,from_server,NULL);
        if (joiner && joiner->userhost) sprintf(buffer,"%s!%s",joiner->nick,joiner->userhost);
        else strcpy(buffer,"-1");
        malloc_strcpy(&result,buffer);
    }
    else malloc_strcpy(&result,"-1");
    return(result);
}

char *function_checkuser(words)
unsigned char *words;
{
    char *result=(char *) 0;
    char *args=(char *) 0;
    struct friends *tmpfriend;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        if (words && *words) {
            if ((tmpfriend=CheckUsers(args,words))) {
                *buffer='\0';
                BuildPrivs(tmpfriend,buffer);
                strcat(buffer," ");
                strcat(buffer,tmpfriend->userhost);
                strcat(buffer," ");
                strcat(buffer,tmpfriend->channels);
                malloc_strcpy(&result,buffer);
                return(result);
            }
        }
    }
    malloc_strcpy(&result,"-1");
    return(result);
}

char *function_checkshit(words)
unsigned char *words;
{
    char *result=(char *) 0;
    char *args=(char *) 0;
    struct autobankicks *abk;

    if (words && *words) {
        args=next_arg((char *) words,(char **) &words);
        if (words && *words) {
            *buffer='\0';
            if ((abk=CheckABKs(args,words))!=NULL) {
                if ((abk->shit)&1) strcat(buffer,"K");
                if ((abk->shit)&2) strcat(buffer,"B");
                if ((abk->shit)&4) strcat(buffer,"I");
                if ((abk->shit)&8) strcat(buffer,"P");
                if ((abk->shit)&16) strcat(buffer,"D");
                strcat(buffer," ");
                strcat(buffer,abk->userhost);
                strcat(buffer," ");
                strcat(buffer,abk->channels);
                strcat(buffer," ");
                strcat(buffer,abk->reason);
            }
            else strcpy(buffer,"-1");
            malloc_strcpy(&result,buffer);
        }
        else malloc_strcpy(&result,"-1");
    }
    else malloc_strcpy(&result,"-1");
    return(result);
}

char *function_stripansi(words)
unsigned char *words;
{
    char *result=(char *) 0;

    if (words && *words) {
        StripAnsi(words,buffer,0);
        malloc_strcpy(&result,buffer);
    }
    else malloc_strcpy(&result,empty_string);
    return(result);
}

/*char *function_pattern(words)
unsigned char *words;
{
    char    *tmpstr;
    char    *pattern;
    char    *result=(char *) 0;

    *tmpbuf='\0';
    if ((pattern=next_arg((char *) words,(char **) &words))) {
        while (((tmpstr=next_arg((char *) words,(char **) &words))!=NULL)) {
            if (wild_match(pattern,tmpstr)) {
                strcat(tmpbuf,tmpstr);
                strcat(tmpbuf," ");
            }
        }
        tmpbuf[strlen(tmpbuf)-1]='\0';
        malloc_strcpy(&result,tmpbuf);
    } 
    else malloc_strcpy(&result,empty_string);
    return(result);
}

char *function_chops(words)
unsigned char *words;
{
    char *nicks=(char *) 0;
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
        malloc_strcpy(&nicks,tmpbuf2);
        return(nicks);
    }
    chan=lookup_channel(tmpbuf,curr_scr_win->server,0);
    if (!chan) {
        malloc_strcpy(&nicks,tmpbuf2);
        return(nicks);
    }
    for (tmp=chan->nicks;tmp;tmp=tmp->next) {
        if (tmp->chanop) {
            count++;
            strcat(tmpbuf2,tmp->nick);
            strcat(tmpbuf2," ");
            if (count>50) {
                tmpbuf2[strlen(tmpbuf2)-1]='\0';
                malloc_strcat(&nicks,tmpbuf2);
                *tmpbuf2='\0';
                count=0;
            }
        }
    }
    if (count) {
        tmpbuf2[strlen(tmpbuf2)-1]='\0';
        malloc_strcat(&nicks,tmpbuf2);
    }
    return(nicks);
}

char *function_chnops(words)
unsigned char *words;
{
    char *nicks=(char *) 0;
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
        malloc_strcpy(&nicks,tmpbuf2);
        return(nicks);
    }
    chan=lookup_channel(tmpbuf,curr_scr_win->server,0);
    if (!chan) {
        malloc_strcpy(&nicks,tmpbuf2);
        return(nicks);
    }
    for (tmp=chan->nicks;tmp;tmp=tmp->next) {
        if (!(tmp->chanop)) {
            count++;
            strcat(tmpbuf2,tmp->nick);
            strcat(tmpbuf2," ");
            if (count>50) {
                tmpbuf2[strlen(tmpbuf2)-1]='\0';
                malloc_strcat(&nicks,tmpbuf2);
                *tmpbuf2='\0';
                count=0;
            }
        }
    }
    if (count) {
        tmpbuf2[strlen(tmpbuf2)-1]='\0';
        malloc_strcat(&nicks,tmpbuf2);
    }
    return(nicks);
}*/

char *function_color(words)
unsigned char *words;
{
    char *result=(char *) 0;
#ifdef WANTANSI
    int  colnum=0;
    int  eventnum=0;
    char *tmp=(char *) 0;
    char *tmpstr=(char *) 0;
    
    if (words && *words) {
        tmp=next_arg((char *) words,(char **) &words);
        if (tmp && words && *words) {
            eventnum=atoi(tmp);
            colnum=atoi(words);
        }
        if (eventnum>=1 && eventnum<=NUMCMDCOLORS && colnum>=1 && colnum<=6) {
            eventnum--;
            malloc_strcpy(&result,Colors[COLOFF]);
            switch (colnum) {
                case 1 : malloc_strcat(&result,CmdsColors[eventnum].color1);
                break;
                case 2 : malloc_strcat(&result,CmdsColors[eventnum].color2);
                break;
                case 3 : malloc_strcat(&result,CmdsColors[eventnum].color3);
                break;
                case 4 : malloc_strcat(&result,CmdsColors[eventnum].color4);
                break;
                case 5 : malloc_strcat(&result,CmdsColors[eventnum].color5);
                break;
                case 6 : malloc_strcat(&result,CmdsColors[eventnum].color6);
                break;
            }
        }
        else {
            malloc_strcpy(&result,Colors[COLOFF]);
            strcpy(buffer,tmp);
            tmp=buffer;
            for (;*tmp;tmp++)
                if (*tmp==',') {
                    *tmp++='\0';
                    if (!tmpstr) tmpstr=buffer;
                    if ((colnum=ColorNumber(tmpstr))!=-1) 
                        malloc_strcat(&result,Colors[colnum]);
                    tmpstr=tmp;
                }
            if (!tmpstr) tmpstr=buffer;
            if (tmpstr)
                if ((colnum=ColorNumber(tmpstr))!=-1)
                    malloc_strcat(&result,Colors[colnum]);
        }
    }
    else malloc_strcpy(&result,empty_string);
    return(result);
#else
    malloc_strcpy(&result,empty_string);
    return(result);
#endif
}

#ifndef CELESCRP
char *function_uhost(input)
unsigned char *input;
{
    char *tmp;
    char *result=(char *) 0;
    
    if ((tmp=index(input,'!'))) malloc_strcpy(&result,tmp+1);
    else malloc_strcpy(&result,empty_string);
    return(result);
}

char *function_hhost(input)
unsigned char *input;
{
    char *tmp;
    char *result=(char *) 0;

    if ((tmp=index(input,'@'))) malloc_strcpy(&result,tmp+1);
    else malloc_strcpy(&result,empty_string);
    return(result);
}

char *function_topic(input)
unsigned char *input;
{
    char *result=(char *) 0;
    char *channel;
    ChannelList *chan;

    if ((channel=next_arg((char *) input,(char **) &channel)) &&
        (chan=lookup_channel(channel,from_server,0)) && chan->topicstr) {
        if (chan->topicwho) sprintf(buffer,"%s %ld ",chan->topicwho,chan->topicwhen);
        else sprintf(buffer,"unknown 0 ");
        strcat(buffer,chan->topicstr);
        malloc_strcpy(&result,buffer);
    }
    else malloc_strcpy(&result,empty_string);
    return(result);
}

char *function_strlen(input)
unsigned char *input;
{
    char *result=(char *) 0;

    if (input && *input) {
        sprintf(buffer,"%d",strlen(input));
        malloc_strcpy(&result,buffer);
    }
    else malloc_strcpy(&result,"0");
    return(result);
}

char *function_strnum(input)
unsigned char *input;
{
    int  count=0;
    char *tmp=input;
    char *result=(char *) 0;

    if (tmp && *tmp) {
        while (*tmp) {
            while (*tmp && isspace(*tmp)) tmp++;
            if (*tmp) {
                count++;
                while (*tmp && !isspace(*tmp)) tmp++;
            }
            if (*tmp) tmp++;
        }
        sprintf(buffer,"%d",count);
        malloc_strcpy(&result,buffer);
    }
    else malloc_strcpy(&result,"0");
    return(result);
}
#endif /* CELESCRP */

char *function_fsize(input)
unsigned char *input;
{
    int  fexists;
    char *filename=(char *) 0;
    char *result=(char *) 0;
    struct stat statbuf;

    if (input && *input) {
        if (!(filename=expand_twiddle(input))) malloc_strcpy(&filename,input);
        fexists=stat(filename,&statbuf);
        new_free(&filename);
        if (fexists==-1) malloc_strcpy(&result,"-1");
        else {
            sprintf(buffer,"%ld",statbuf.st_size);
            malloc_strcpy(&result,buffer);
        }
    }
    else malloc_strcpy(&result,"-1");
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
char *function_sar(word)
unsigned char *word;
{
    int	variable=0,global=0,searchlen;
    unsigned int display=window_display;
    char delimiter;
    char *data=(char *) 0;
    char *value=(char *) 0;
    char *svalue;
    char *search=(char *) 0;
    char *result=(char *) 0;
    char *replace=(char *) 0;
    char *pointer=(char *) 0;
    Alias *tmp;

    while (((*word=='r') && (variable=1)) || ((*word=='g') && (global=1))) word++;
    if (!(word && *word)) {
        malloc_strcpy(&result,empty_string);
        return(result);
    }
    delimiter=*word;
    search=word+1;
    if (!(replace=strchr(search,delimiter))) {
        malloc_strcpy(&result,empty_string);
        return(result);
    }
    *replace++='\0';
    if (!(data=strchr(replace,delimiter))) {
        malloc_strcpy(&result,empty_string);
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
        malloc_strcpy(&result,empty_string);
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
            malloc_strcat(&result,value);
            malloc_strcat(&result,replace);
            value=pointer;
            if (!*pointer) break;
        }
    } 
    else {
        if ((pointer=strstr(pointer,search))) {
            pointer[0]=pointer[searchlen]=0;
            pointer+=searchlen+1;
            malloc_strcat(&result,value);
            malloc_strcat(&result,replace);
            value=pointer;
        }
    }
    malloc_strcat(&result,value);
    if (variable)  {
        window_display=0;
        add_alias(VAR_ALIAS,data,result);
        window_display=display;
    }
    new_free(&svalue);
    return(result);
}
#endif /* CELESCRP */

#ifdef COUNTRY
char *function_country(input)
unsigned char *input;
{
    int i;
    char *result=(char *) 0;
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
        { "AN", "Netherlands Antilles" },
        { "AO", "Angola" },
        { "AQ", "Antarctica" },
        { "AR", "Argentina" },
        { "AS", "American Samoa" },
        { "AT", "Austria" },
        { "AU", "Australia" },
        { "AW", "Aruba" },
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
        { "BM", "Bermuda" },
        { "BN", "Brunei Darussalam" },
        { "BO", "Bolivia" },
        { "BR", "Brazil" },
        { "BS", "Bahamas" },
        { "BT", "Bhutan" },
        { "BV", "Bouvet Island" },
        { "BW", "Botswana" },
        { "BY", "Belarus" },
        { "BZ", "Belize" },
        { "CA", "Canada" },
        { "CC", "Cocos Islands" },
        { "CF", "Central African Republic" },
        { "CG", "Congo" },
        { "CH", "Switzerland" },
        { "CI", "Cote D'ivoire" },
        { "CK", "Cook Islands" },
        { "CL", "Chile" },
        { "CM", "Cameroon" },
        { "CN", "China" },
        { "CO", "Colombia" },
        { "CR", "Costa Rica" },
        { "CS", "Former Czechoslovakia" },
        { "CU", "Cuba" },
        { "CV", "Cape Verde" },
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
        { "FX", "France, Metropolitan" },
        { "GA", "Gabon" },
        { "GB", "Great Britain" },
        { "GD", "Grenada" },
        { "GE", "Georgia" },
        { "GF", "French Guiana" },
        { "GH", "Ghana" },
        { "GI", "Gibraltar" },
        { "GL", "Greenland" },
        { "GM", "Gambia" },
        { "GN", "Guinea" },
        { "GP", "Guadeloupe" },
        { "GQ", "Equatorial Guinea" },
        { "GR", "Greece" },
        { "GS", "S. Georgia and S. Sandwich Isls." },
        { "GT", "Guatemala" },
        { "GU", "Guam" },
        { "GW", "Guinea-Bissau" },
        { "GY", "Guyana" },
        { "HK", "Hong Kong" },
        { "HM", "Heard and McDonald Islands" },
        { "HN", "Honduras" },
        { "HR", "Croatia" },
        { "HT", "Haiti" },
        { "HU", "Hungary" },
        { "ID", "Indonesia" },
        { "IE", "Ireland" },
        { "IL", "Israel" },
        { "IN", "India" },
        { "IO", "British Indian Ocean Territory" },
        { "IQ", "Iraq" },
        { "IR", "Iran" },
        { "IS", "Iceland" },
        { "IT", "Italy" },
        { "JM", "Jamaica" },
        { "JO", "Jordan" },
        { "JP", "Japan" },
        { "KE", "Kenya" },
        { "KG", "Kyrgyzstan" },
        { "KH", "Cambodia" },
        { "KI", "Kiribati" },
        { "KM", "Comoros" },
        { "KN", "St. Kitts and Nevis" },
        { "KP", "North Korea" },
        { "KR", "South Korea" },
        { "KW", "Kuwait" },
        { "KY", "Cayman Islands" },
        { "KZ", "Kazakhstan" },
        { "LA", "Laos" },
        { "LB", "Lebanon" },
        { "LC", "Saint Lucia" },
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
        { "MG", "Madagascar" },
        { "MH", "Marshall Islands" },
        { "MK", "Macedonia" },
        { "ML", "Mali" },
        { "MM", "Myanmar" },
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
        { "NT", "Neutral Zone" },
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
        { "PM", "St. Pierre and Miquelon" },
        { "PN", "Pitcairn" },
        { "PR", "Puerto Rico" },
        { "PT", "Portugal" },
        { "PW", "Palau" },
        { "PY", "Paraguay" },
        { "QA", "Qatar" },
        { "RE", "Reunion" },
        { "RO", "Romania" },
        { "RU", "Russian Federation" },
        { "RW", "Rwanda" },
        { "SA", "Saudi Arabia" },
        { "SB", "Solomon Islands" },
        { "SC", "Seychelles" },
        { "SD", "Sudan" },
        { "SE", "Sweden" },
        { "SG", "Singapore" },
        { "SH", "St. Helena" },
        { "SI", "Slovenia" },
        { "SJ", "Svalbard and Jan Mayen Islands" },
        { "SK", "Slovak Republic" },
        { "SL", "Sierra Leone" },
        { "SM", "San Marino" },
        { "SN", "Senegal" },
        { "SO", "Somalia" },
        { "SR", "Suriname" },
        { "ST", "Sao Tome and Principe" },
        { "SU", "Former USSR" },
        { "SV", "El Salvador" },
        { "SY", "Syria" },
        { "SZ", "Swaziland" },
        { "TC", "Turks and Caicos Islands" },
        { "TD", "Chad" },
        { "TF", "French Southern Territories" },
        { "TG", "Togo" },
        { "TH", "Thailand" },
        { "TJ", "Tajikistan" },
        { "TK", "Tokelau" },
        { "TM", "Turkmenistan" },
        { "TN", "Tunisia" },
        { "TO", "Tonga" },
        { "TP", "East Timor" },
        { "TR", "Turkey" },
        { "TT", "Trinidad and Tobago" },
        { "TV", "Tuvalu" },
        { "TW", "Taiwan" },
        { "TZ", "Tanzania" },
        { "UA", "Ukraine" },
        { "UG", "Uganda" },
        { "UK", "United Kingdom" },
        { "UM", "US Minor Outlying Islands" },
        { "US", "United States of America" },
        { "UY", "Uruguay" },
        { "UZ", "Uzbekistan" },
        { "VA", "Vatican City State" },
        { "VC", "St. Vincent and the grenadines" },
        { "VE", "Venezuela" },
        { "VG", "British Virgin Islands" },
        { "VI", "US Virgin Islands" },
        { "VN", "Vietnam" },
        { "VU", "Vanuatu" },
        { "WF", "Wallis and Futuna Islands" },
        { "WS", "Samoa" },
        { "YE", "Yemen" },
        { "YT", "Mayotte" },
        { "YU", "Yugoslavia" },
        { "ZA", "South Africa" },
        { "ZM", "Zambia" },
        { "ZR", "Zaire" },
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
        strcpy(buffer,input);
        upper(buffer);
        for (i=0;domain[i].id;i++)
            if (!strcmp(domain[i].id,buffer)) {
                malloc_strcpy(&result,domain[i].description);
                return(result);
            }
    }
    malloc_strcpy(&result,"Unknown");
    return(result);
}
#endif

#ifndef CELESCRP
char *function_cdccslots(input)
unsigned char *input;
{
    int  slots=0;
    char *result=(char *) 0;

    slots=CdccLimit-TotalSendDcc();
    if (slots<0) slots=0;
    sprintf(buffer,"%d",slots);
    malloc_strcpy(&result,buffer);
    return(result);
}

char *function_cdccqslots(input)
unsigned char *input;
{
    int  slots=0;
    char *result=(char *) 0;

    if (CdccQueueLimit) slots=CdccQueueLimit-TotalQueue();
    else slots=10;
    if (slots<0) slots=0;
    sprintf(buffer,"%d",slots);
    malloc_strcpy(&result,buffer);
    return(result);
}

char *function_url(input)
unsigned char *input;
{
    int urlnum=99;
    char *result=(char *) 0;
    struct urlstr *tmpurl=urllist;

    if (!tmpurl) malloc_strcpy(&result,"0");
    else {
        if (input && *input) urlnum=atoi(input);
        while (tmpurl && tmpurl->next && urlnum--) tmpurl=tmpurl->next;
        malloc_strcpy(&result,tmpurl->urls);
    }
    return(result);
}

char *function_strstr(input)
unsigned char *input;
{
    char *result=(char *) 0;
    char *tmpstr=(char *) 0;
    char *findstr=(char *) 0;

    if (input && *input) {
        strcpy(buffer,&input[1]);
        if ((tmpstr=index(buffer,*input))) {
            *tmpstr++='\0';
            if (*tmpstr) tmpstr++;
            else tmpstr=(char *) 0;
        }
        if (tmpstr) findstr=strstr(buffer,tmpstr);
    }
    if (!findstr) findstr=empty_string;
    malloc_strcpy(&result,findstr);
    return(result);
}

char *function_szvar(input)
unsigned char *input;
{
    int  i;
    char *result=(char *) 0;
    struct commands {
        char *command;
        int  type;  /* 1=numeric   2=string   3=on channels/off */
        int  *ivar;
        char **svar;
    } command_list[]= {
        { "EXTMES"         , 1, &ExtMes          , NULL                   },
        { "NHPROT"         , 3, &NHProt          , &NHProtChannels        },
        { "NHDISP"         , 1, &NHDisp          , NULL                   },
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
        { "AUTOINV"        , 1, &AutoInv         , NULL                   },
#endif
        /* Floodp should return FloodProt, FloodMessages and FloodSeconds */
        { "FLOODP"         , 1, &FloodProt       , NULL                   },
        { "SERVNOTICE"     , 1, &ServerNotice    , NULL                   },
        { "CTCPCLOAKING"   , 1, &CTCPCloaking    , NULL                   },
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
        { "STAMP"          , 3, &Stamp           , &StampChannels         },
#ifdef WANTANSI
        { "MIRC"           , 1, &DisplaymIRC     , NULL                   },
#endif
        { NULL             , 0, NULL             , NULL                   }
    };

    if (input && *input) {
        strcpy(buffer,input);
        upper(buffer);
        for (i=0;command_list[i].command;i++)
            if (!strcmp(command_list[i].command,buffer)) break;
        if (!(command_list[i].command)) {
            malloc_strcpy(&result,empty_string);
            return(result);
        }
        *buffer='\0';
        switch (command_list[i].type) {
            case 1:
                /* Cdcc limit is special case */
                if (!strcmp(command_list[i].command,"CDCCLIMIT"))
                    sprintf(buffer,"%d %d",*(command_list[i].ivar),CdccQueueLimit);
                /* Floodp is special case */
                else if (!strcmp(command_list[i].command,"FLOODP"))
                    sprintf(buffer,"%d %d %d",
                            *(command_list[i].ivar),FloodMessages,FloodSeconds);
                else sprintf(buffer,"%d",*(command_list[i].ivar));
                break;
            case 2:
                /* Bantype is special case */
                if (!strcmp(command_list[i].command,"BANTYPE")) {
                    *buffer=defban;
                    *(buffer+1)='\0';
                }
                else if (*(command_list[i].svar)) strcpy(buffer,*(command_list[i].svar));
                break;
            case 3:
                if (*(command_list[i].ivar) && *(command_list[i].svar))
                    sprintf(buffer,"%d %s",*(command_list[i].ivar),*(command_list[i].svar));
                else strcpy(buffer,"0");
                break;
        }
        if (*buffer) {
            malloc_strcpy(&result,buffer);
            return(result);
        }
    }
    malloc_strcpy(&result,empty_string);
    return(result);
}

/* by Zakath */
char *function_isvoiced(input)
unsigned char *input;
{
    char *nick;
    char *result=(char *) 0;
    char *channel=(char *) 0;

    if ((nick=next_arg((char *) input,(char **) &channel)))
        malloc_strcpy(&result,is_voiced(channel,nick)?"1":"0");
    else malloc_strcpy(&result,"0");
    return(result);
}
#endif /* CELESCRP */

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
    Alias *tmp;
    Alias *tmprem;

    sprintf(buffer,"%s.",name);
    namelen=strlen(buffer);
    for (tmp=alias_list[VAR_ALIAS];tmp;) {
        tmprem=tmp;
        tmp=tmp->next;
        if (!strncmp(tmprem->name,buffer,namelen))
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
/****************************************************************************/
