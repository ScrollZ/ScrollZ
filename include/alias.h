/*
 * alias.h: header for alias.c 
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
 * $Id: alias.h,v 1.3 2000-08-09 19:31:20 f Exp $
 */

#ifndef __alias_h_
#define __alias_h_

#define COMMAND_ALIAS 0
#define VAR_ALIAS 1

#define LEFT_BRACE '{'
#define RIGHT_BRACE '}'
#define LEFT_BRACKET '['
#define RIGHT_BRACKET ']'
#define LEFT_PAREN '('
#define RIGHT_PAREN ')'
#define DOUBLE_QUOTE '"'

	void	add_alias _((int, char *, char *));
	char	*get_alias _((int, char *, int *, char **));
	char	*expand_alias _((char *, char *, char *, int *, char **));
	void	execute_alias _((char *, char *, char *));
	void	list_aliases _((int, char *));
	int	mark_alias _((char *, int));
	void	delete_alias _((int, char *));
	char	**match_alias _((char *, int *, int));
	void	alias _((char *, char *, char *));
	char	*parse_inline _((char *, char *, int *));
 	char	*MatchingBracket _((char *, int, int));
	void	save_aliases _((FILE *, int));
	int	word_count _((char *));

extern	char	alias_illegals[];
extern	char	*command_line;

struct	ArgPosTag
{
	char *ArgStart;
	int ArgLen;
	char *FirstComp;
};

typedef	struct ArgPosTag	ArgPos;

/* Alias: structure of each alias entry */
typedef	struct	AliasStru
{
	char	*name;			/* name of alias */
	char	*stuff;			/* what the alias is */
	int	mark;			/* used to prevent recursive aliasing */
	int	global;			/* set if loaded from `global' */
	struct	AliasStru *next;	/* pointer to next alias in list */
}	Alias;

#define MAX_CMD_ARGS 5

#endif /* __alias_h_ */
