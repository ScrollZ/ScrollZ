/*
 * stack.c - does the handling of stack functions
 *
 * written by matthew green
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
 * $Id: stack.c,v 1.7 2003-01-08 20:00:54 f Exp $
 */

#include "irc.h"

#include "stack.h"
#include "window.h"
#include "hook.h"
#include "ircaux.h"
#include "output.h"
#include "list.h"

#ifndef LITE

static	OnStack	*on_stack = NULL;
static  AliasStack *alias_stack = NULL;
static	AliasStack *assign_stack = NULL;
static	SetStack *set_stack = NULL;

static	AliasStack *alias_get _((char *, int));
static	AliasStack *alias_stack_find _((char *, int));
static	void	alias_stack_add _((AliasStack *, int));
static	void	do_stack_on _((int, char *));
static	void	do_stack_alias _((int, char *, int));
static	void	do_stack_set _((int, char *));

static void 
do_stack_on (int type, char *args)
{
	char	foo[4];
 	int	cnt, i, which = 0;
 	size_t	len;
	Hook	*list;
	NumericList	*nhook,
			*nptr,
			*ntmp = NULL;

	if (!on_stack && (type == STACK_POP || type == STACK_LIST))
	{
		say("ON stack is empty!");
		return;
	}
	if (!args || !*args)
	{
		say("Missing event type for STACK ON");
		return;
	}
	len = strlen(args);
	for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
	{
		if (!my_strnicmp(args, hook_functions[i].name, len))
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
	if (!cnt)
	{
		if (is_number(args))
		{
			which = atoi(args);
			if (which < 1 || which > 999)
			{
				say("Numerics must be between 001 and 999");
				return;
			}
			which = -which;
		}
		else
		{
			say("No such ON function: %s", args);
			return;
		}
	}
	if (which < 0)
	{
		/* XXX: check cnt != 1 */
		snprintf(foo, sizeof foo, "%3.3u", -which);
		if ((nhook = (NumericList *) find_in_list((List **) &numeric_list, foo, 0)) != NULL)
			list = nhook->list;
		else
			list = NULL;
	}
	else
		list = hook_functions[which].list;
	if (type == STACK_PUSH)
	{
		OnStack	*new;

		if (list == NULL)
		{
			say("The ON %s list is empty", args);
			return;
		}
		new = (OnStack *) new_malloc(sizeof(OnStack));
		new->next = on_stack;
		on_stack = new;
		new->which = which;
		new->list = list;
		if (which < 0)
		{
			if (nhook == numeric_list)
			{
				numeric_list = nhook->next;
				new_free(&nhook->name);
				new_free(&nhook);
				return;
			}
			for (nptr = numeric_list; nptr;
					ntmp = nptr, nptr = nptr->next)
			{
				if (nptr == nhook)
				{
					ntmp->next = nptr->next;
					new_free(&nptr->name);
					new_free(&nptr);
					return;
				}
			}
		}
		else
			hook_functions[which].list = NULL;
		return;
	}
	else if (type == STACK_POP)
	{
		OnStack	*p,
			*tmp = NULL;

		for (p = on_stack; p; tmp = p, p = p->next)
		{
			if (p->which == which)
			{
				if (p == on_stack)
					on_stack = p->next;
				else
					tmp->next = p->next;
				break;
			}
		}
		if (!p)
		{
			say("No %s on the stack", args);
			return;
		}
		if ((which < 0 && nhook) || hook_functions[which].list)
			remove_hook(which, NULL, 0, 0, 1);	/* free hooks */
		if (which < 0)
		{
			if ((nptr = (NumericList *) find_in_list((List **) &numeric_list, foo, 0)) == NULL)
			{
				nptr = (NumericList *) new_malloc(sizeof(NumericList));
				nptr->name = NULL;
				nptr->list = p->list;
				malloc_strcpy(&nptr->name, foo);
				add_to_list((List **) &numeric_list, (List *) nptr);
			}
			else
				add_to_list((List **) &numeric_list->list, (List *) p->list);
		}
		else
			hook_functions[which].list = p->list;
		return;
	}
	else if (type == STACK_LIST)
	{
		int	slevel = 0;
		OnStack	*osptr;

		for (osptr = on_stack; osptr; osptr = osptr->next)
			if (osptr->which == which)
			{
				Hook	*hptr;

				slevel++;
				say("Level %d stack", slevel);
				for (hptr = osptr->list; hptr; hptr = hptr->next)
					show_hook(hptr, args);
			}

		if (!slevel)
			say("The STACK ON %s list is empty", args);
		return;
	}
	say("Unknown STACK ON type ??");
}

static void 
do_stack_alias (int type, char *args, int which)
{
	char	*name;
	AliasStack	*aptr,
			**aptrptr;

	if (which == STACK_DO_ALIAS)
	{
		name = "ALIAS";
		aptrptr = &alias_stack;
	}
	else
	{
		name = "ASSIGN";
		aptrptr = &assign_stack;
	}
	if (!*aptrptr && (type == STACK_POP || type == STACK_LIST))
	{
		say("%s stack is empty!", name);
		return;
	}

	if (STACK_PUSH == type)
	{
		aptr = alias_get(args, which);
		if ((AliasStack *) 0 == aptr)
		{
			say("No such %s %s", name, args);
			return;
		}
		if (aptrptr)
			aptr->next = *aptrptr;
		*aptrptr = aptr;
		return;
	}
	if (STACK_POP == type)
	{
		aptr = alias_stack_find(args, which);
		if ((AliasStack *) 0 == aptr)
		{
			say("%s is not on the %s stack!", args, name);
			return;
		}
		alias_stack_add(aptr, which);
		return;
	}
	if (STACK_LIST == type)
	{
		say("stack list is not implimented yet");
		return;
	}
	say("Unknown STACK type ??");
}

static void 
do_stack_set (int type, char *args)
{
	SetStack *tmp = set_stack;

	tmp++;
}

/*
 * alias_get: this returns a point to an `AliasStack' structure that
 * has be extracted from the current aliases, and removed from that
 * list.
 */
static	AliasStack* alias_get(args, which)
	char	*args;
	int	which;
{
	return (AliasStack *) 0;
}

/*
 * alias_stack_find: this returns the pointer to the struct with the
 * most recent alias for `args' in the stack.
 */
static	AliasStack*
alias_stack_find(args, which)
	char	*args;
	int	which;
{
	return (AliasStack *) 0;
}

/*
 * alias_stack_add: this adds `aptr' to the alias/assign stack.
 */
static	void
alias_stack_add(aptr, which)
	AliasStack *aptr;
	int which;
{
	return;
}

void 
stackcmd (char *command, char *args, char *subargs)
{
	char	*arg,
		*cmd = NULL;
 	int	type;
 	size_t	len;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		malloc_strcpy(&cmd, arg);
		upper(cmd);
		if (!strncmp(cmd, "PUSH", len))
			type = STACK_PUSH;
		else if (!strncmp(cmd, "POP", len))
			type = STACK_POP;
		else if (!strncmp(cmd, "LIST", len))
			type = STACK_LIST;
		else
		{
			say("%s is unknown stack type", arg);
			new_free(&cmd);
			return;
		}
		new_free(&cmd);
	}
	else
	{
		say("Need operation for STACK");
		return;
	}
	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		malloc_strcpy(&cmd, arg);
		upper(cmd);
		if (!strncmp(cmd, "ON", len))
			do_stack_on(type, args);
		else if (!strncmp(cmd, "ALIAS", len))
			do_stack_alias(type, args, STACK_DO_ALIAS);
		else if (!strncmp(cmd, "ASSIGN", len))
			do_stack_alias(type, args, STACK_DO_ASSIGN);
		else if (!strncmp(cmd, "SET", len))
			do_stack_set(type, args);
		else
		{
			say("%s is not a valid STACK type");
			new_free(&cmd);
			return;
		}
		new_free(&cmd);
	}
	else
	{
		say("Need stack type for STACK");
		return;
	}
}

#endif /* LITE */
