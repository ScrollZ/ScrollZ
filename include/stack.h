/*
 * stack.h - header for stack.c
 *
 * written by matthew green
 *
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
 * $Id: stack.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

#ifndef __stack_h_
# define __stack_h_

#include "hook.h"
#include "alias.h"

	void	stackcmd  _((char *, char *, char *));

#define STACK_POP 0
#define STACK_PUSH 1
#define STACK_SWAP 2
#define STACK_LIST 3

#define STACK_DO_ALIAS	0x0001
#define STACK_DO_ASSIGN	0x0002

typedef	struct	setstacklist
{
	int	which;
	Hook	*list;
	struct setstacklist *next;
}	SetStack;

typedef	struct	aliasstacklist
{
	int	which;
	Alias	*list;
	struct aliasstacklist *next;
}	AliasStack;

typedef	struct	onstacklist
{
	int	which;
	Hook	*list;
	struct onstacklist *next;
}	OnStack;

#endif /* __stack_h_ */
