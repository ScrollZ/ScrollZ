/*
 * ircaux.h: header file for ircaux.c 
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
 * $Id: ircaux.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

#ifndef __ircaux_h_
#define __ircaux_h_

#include <stdio.h>

	char	*check_nickname _((char *));
	char	*next_arg _((char *, char **));
	char	*new_next_arg _((char *, char **));
	char	*expand_twiddle _((char *));
	char	*upper _((char *));
	char	*lower _((char *));
	char	*sindex _((char *, char *));
 	char	*srindex _((char *, char *));
	char	*rfgets _((char *, int, FILE *));
	char	*path_search _((char *, char *));
	char	*double_quote _((char *, char *));
	char	*new_malloc _((int));
#ifdef ALLOC_DEBUG
	void	alloc_cmd _((char *, char *, char *));
#endif
	char	*new_realloc _((char *, int));
	void	malloc_strcpy _((char **, char *));
	void	malloc_strcat _((char **, char *));
	void	new_free _((void *));
	void	wait_new_free _((char **));
	FILE	*zcat _((char *));
	int	is_number _((char *));
/**************************** PATCHED by Flier ******************************/
	/*int	connect_by_number _((int, char *, int));*/
	int	connect_by_number _((int, char *, int, int));
/****************************************************************************/
	int	my_stricmp _((char *, char *));
	int	my_strnicmp _((char *, char *, int));
	int	set_non_blocking _((int));
	int	set_blocking _((int));
	int	scanstr _((char *, char *));
	void	really_free _((int));
	void	strmcpy _((char *, char *, int));
	void	strmcat _((char *, char *, int));
	void	strmcat_ue _((char *, char *, int));

#endif /* __ircaux_h_ */
