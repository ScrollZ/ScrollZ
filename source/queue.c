/*
 * queue.c - The queue command
 *
 * Queues allow for future batch processing
 *
 * Syntax:  /QUEUE -DO -SHOW -LIST -NO_FLUSH -DELETE -FLUSH <name> {commands}
 *
 * Written by Jeremy Nelson
 *
 * Copyright (c) 1993 Jeremy Nelson.
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
 */

/**************************** PATCHED by Flier ******************************
#ifndef lint
static	char	rcsid[] = "@(#)$Id: queue.c,v 1.1 1998-09-10 17:31:13 f Exp $";
#endif
****************************************************************************/

#include "irc.h"
#include "alias.h"
#include "ircaux.h"
#include "debug.h"
#include "output.h"
#include "edit.h"
#include "if.h"
#include "queue.h"

typedef	struct CmdListT
{
        struct CmdListT	*next;
        char		*what;
} CmdList;

typedef	struct	QueueT
{
        struct QueueT   *next;
        struct CmdListT *first;
        char     *name;
} Queue;

	void	queuecmd _((char *, char *, char *));
static	Queue	*lookup_queue _((Queue *, char *));
static	CmdList	*walk_commands _((Queue *));
static	Queue	*make_new_queue _((Queue *, char *));
static	int	add_commands_to_queue _((Queue *, char *what, char *));
static	int	delete_commands_from_queue _((Queue *, int));
static	Queue	*remove_a_queue _((Queue *));
static	void	flush_queue _((Queue *));
static	Queue	*do_queue _((Queue *, int));
static	void	display_all_queues _((Queue *));
static	void	print_queue _((Queue *));
static	int	num_entries _((Queue *));

extern	void 
queuecmd(cmd, args, subargs)
	char	*cmd,
		*args,
		*subargs;
{
	Queue	*tmp;
	char	*arg = (char *) 0,
		*name = (char *) 0,
		*startcmds = (char *) 0,
		*cmds = (char *) 0;
	int	noflush = 0,
		runit = 0, 
		list = 0,
		flush = 0,
		remove_by_number = 0,
		commands = 1,
		number = 0;
 	static	Queue	*Queuelist = NULL;

	/* If the queue list is empty, make an entry */
 	if (Queuelist == (Queue *) 0) {
 		char blah_c_sucks[4] = "Top";
 		Queuelist = make_new_queue((Queue *) 0, blah_c_sucks);
 	}
	if (Queuelist == (Queue *) 0)

	if ((startcmds = index(args, '{')) == (char *) 0) /* } */
		commands = 0;
	else
		*(startcmds-1) = '\0';

	while ((arg = upper(next_arg(args, &args))) != (char *) 0)
	{
		if (*arg == '-')
		{
			*arg++ = '\0';
			if (!strcmp(arg, "NO_FLUSH"))
				noflush = 1;
			else
			if (!strcmp(arg, "SHOW"))
			{
				display_all_queues(Queuelist);
				return;
			}
			else
			if (!strcmp(arg, "LIST"))
				list = 1;
			else
			if (!strcmp(arg, "DO"))
				runit = 1;
			else
			if (!strcmp(arg, "DELETE"))
				remove_by_number = 1;
			else
			if (!strcmp(arg, "FLUSH"))
				flush = 1;
		}
		else
		{
			if (name)
				number = atoi(arg);
			else
				name = arg;
		}
	}

	if (name == (char *) 0)
		return;

	/* Find the queue based upon the previous queue */
	tmp = lookup_queue(Queuelist, name);

	/* if the next queue is empty, then we need to see if 
	   we should make it or output an error */
	if ((tmp->next) == (Queue *) 0)
	{
		if (commands)
			tmp->next = make_new_queue((Queue *) 0, name);
		else
		{
			yell ("QUEUE: (%s) no such queue",name);
			return;
		}
	}
	if (remove_by_number == 1)
	{
		if (delete_commands_from_queue(tmp->next,number))
			tmp->next = remove_a_queue(tmp->next);
	}
	if (list == 1)
	{
		print_queue(tmp->next);
	}
	if (runit == 1)
	{
		tmp->next = do_queue(tmp->next, noflush);
	}
	if (flush == 1)
	{
		tmp->next = remove_a_queue(tmp->next);
	}
	if (startcmds)
	{
		int booya;

		if ((cmds = next_expr(&startcmds, '{')) == (char *) 0) /* } */
		{
			yell ("QUEUE: missing closing brace");
			return;
		}
		booya = add_commands_to_queue (tmp->next, cmds, subargs);
		say ("QUEUED: %s now has %d entries",name, booya);
	}
}

/*
 * returns the queue BEFORE the queue we are looking for
 * returns the last queue if no match
 */
static	Queue	*
lookup_queue(queue, what)
	Queue	*queue;
	char	*what;
{
	Queue	*tmp = queue;

	upper(what);

	while (tmp->next)
	{
		if (!strcmp(tmp->next->name, what))
			return tmp;
		else
			if (tmp->next)
				tmp = tmp->next;
			else
				break;
	}
	return tmp;
}

/* returns the last CmdList in a queue, useful for appending commands */
static	CmdList	*
walk_commands(queue)
	Queue 	*queue;
{
	CmdList	*ctmp;
	
	if (!queue)
		return (CmdList *) 0;

	ctmp = queue->first;
	if (ctmp)
	{
		while (ctmp->next)
			ctmp = ctmp->next;
		return ctmp;
	}
	return (CmdList *) 0;
}

/*----------------------------------------------------------------*/
/* Make a new queue, link it in, and return it. */
static	Queue	*
make_new_queue(afterqueue, name)
	Queue	*afterqueue;
	char	*name;
{
	Queue	*tmp;

	if (!name)
		return (Queue *) 0;
	tmp = (Queue *) new_malloc(sizeof(Queue));
	upper(name);

	tmp->next = afterqueue;
	tmp->first = (CmdList *) 0;
	tmp->name = (char *) 0;
	malloc_strcpy(&tmp->name, name);
	return tmp;
}
	
/* add a command to a queue, at the end of the list */
/* expands the whole thing once and stores it */
static	int
add_commands_to_queue(queue, what, subargs)
	Queue	*queue;
	char	*what;
	char	*subargs;
{
	CmdList *ctmp = walk_commands(queue);
	char	*list = (char *) 0,
		*sa;
	int 	args_flag = 0;
	
	sa = subargs ? subargs : " ";
	list = expand_alias((char *) 0, what, sa, &args_flag, (char **) 0);
	if (!ctmp)
	{
		queue->first = (CmdList *) new_malloc(sizeof(CmdList));
		ctmp = queue->first;
	}
	else
	{
		ctmp->next = (CmdList *) new_malloc(sizeof(CmdList));
		ctmp = ctmp->next;
	}
 	ctmp->what = (char *) 0;
	malloc_strcpy(&ctmp->what, list);
	ctmp->next = (CmdList *) 0;
	return num_entries(queue);
}


/* remove the Xth command from the queue */
static	int
delete_commands_from_queue(queue, which)
	Queue	*queue;
	int	which;
{
	CmdList *ctmp = queue->first;
	CmdList *blah;
	int x;

	if (which == 1)
		queue->first = ctmp->next;
	else
	{
		for (x=1;x<which-1;x++)
		{
			if (ctmp->next) 
				ctmp = ctmp->next;
			else 
				return 0;
		}
		blah = ctmp->next;
		ctmp->next = ctmp->next->next;
		ctmp = blah;
	}
	new_free(&ctmp->what);
	new_free(&ctmp);
	if (queue->first == (CmdList *) 0)
		return 1;
	else
		return 0;
}

/*-------------------------------------------------------------------*/
/* flush a queue, deallocate the memory, and return the next in line */
static	Queue	*
remove_a_queue(queue)
	Queue	*queue;
{
	Queue *tmp;

	tmp = queue->next;
	flush_queue(queue);
	new_free(&queue);
	return tmp;
}

/* walk through a queue, deallocating the entries */
static	void
flush_queue(queue)
	Queue	*queue;
{
	CmdList	*tmp,
		*tmp2;

	tmp = queue->first;

	while (tmp != (CmdList *) 0)
	{
		tmp2 = tmp;
		tmp = tmp2->next;
		if (tmp2->what != (char *) 0)
			new_free(&tmp2->what);
		if (tmp2)
			new_free(&tmp2);
	}
}

/*------------------------------------------------------------------------*/
/* run the queue, and if noflush, then return the queue, else return the
   next queue */
static	Queue	*
do_queue(queue, noflush)
	Queue	*queue;
	int	noflush;
{
	CmdList	*tmp;
	
	tmp = queue->first;
	
	do
	{
		if (tmp->what != (char *) 0)
			parse_line((char *) 0, tmp->what, empty_string, 0, 0);
		tmp = tmp->next;
	}
	while (tmp != (CmdList *) 0);

	if (!noflush) 
		return remove_a_queue(queue);
	else
		return queue;
}

/* ---------------------------------------------------------------------- */
/* output the contents of all the queues to the screen */
static	void
display_all_queues(queue)
	Queue	*queue;
{
	Queue *tmp;

	if (!queue)
		return;

	tmp = queue->next;
	while (tmp)
	{
		print_queue(tmp);
		if (tmp->next == (Queue *) 0)
			return;
		else
			tmp = tmp->next;
	}
	yell("QUEUE: No more queues");
}

/* output the contents of a queue to the screen */
static	void
print_queue(queue)
	Queue	*queue;
{
	CmdList *tmp;
	int 	x = 0;
	
	tmp = queue->first;
	while (tmp != (CmdList *) 0)
	{
		if (tmp->what)
			say ("<%s:%2d> %s",queue->name,++x,tmp->what);
		tmp = tmp->next;
	}
	say ("<%s> End of queue",queue->name);
}

/* return the number of entries in a queue */
static	int
num_entries(queue)
	Queue	*queue;
{
	int x = 1;
	CmdList *tmp;

	if ((tmp = queue->first) == (CmdList *) 0) 
		return 0;
	while (tmp->next)
	{
		x++;
		tmp = tmp->next;
	}
	return x;
}
