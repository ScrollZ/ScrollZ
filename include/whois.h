/*
 * whois.h: header for whois.c 
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
 * $Id: whois.h,v 1.3 2001-08-02 18:34:52 f Exp $
 */

#ifndef __whois_h_
# define __whois_h_

#ifdef HAVE_STDARG_H
	void	add_to_whois_queue _((char *, void (*) (WhoisStuff *, char *, char *), char *, ...));
# else
	void	add_to_whois_queue _(());
#endif /* HAVE_STDARG_H */
	void	add_ison_to_whois _((char *, void (*) (WhoisStuff *, char *, char *)));
/**************************** PATCHED by Flier ******************************/
	void	add_userhost_to_whois _((char *, void (*) (WhoisStuff *, char *, char *)));
/****************************************************************************/
        void	whois_name _((char *, char **));
	void	whowas_name _((char *, char **));
	void	whois_channels _((char *, char **));
	void	whois_server _((char *, char **));
	void	whois_oper _((char *, char **));
/**************************** Patched by Flier ******************************/
	void	whois_admin _((char *, char **));
/****************************************************************************/
	void	whois_lastcom _((char *, char **));
	void	whois_nickname _((WhoisStuff *, char *, char *));
	void	whois_ignore_msgs _((WhoisStuff *, char *, char *));
	void	whois_ignore_notices _((WhoisStuff *, char *, char *));
	void	whois_ignore_walls _((WhoisStuff *, char *, char *));
	void	whois_ignore_invites _((WhoisStuff *, char *, char *));
	void	whois_notify _((WhoisStuff *, char *, char *));
	void	whois_new_wallops _((WhoisStuff *, char *, char *));
	void	clean_whois_queue _((void));
	void	set_beep_on_msg _((char *));
	void    userhost_cmd_returned _((WhoisStuff *, char *, char *));
	void	user_is_away _((char *, char **));
	void	userhost_returned _((char *, char **));
	void	ison_returned _((char *, char **));
	void	whois_chop _((char *, char **));
	void	end_of_whois _((char *, char **));
	void	whoreply _((char *, char **));
	void	convert_to_whois _((void));
	void	ison_notify _((WhoisStuff *, char *, char *));
/**************************** PATCHED by Flier ******************************/
	/*void	no_such_nickname _((char *, char **));*/
	void	no_such_nickname _((char *, char **, int));
/****************************************************************************/

extern	int	beep_on_level;
extern	char	*redirect_format;

#define	WHOIS_WHOIS	0x01
#define	WHOIS_USERHOST	0x02
#define	WHOIS_ISON	0x04
#define WHOIS_ISON2	0x08

#define	USERHOST_USERHOST ((void (*)_((WhoisStuff *, char *, char *))) 1)

#endif /* __whois_h_ */
