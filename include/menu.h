/*
 * Here we define how our menus are held
 *
 * Copyright (c) 1992 Troy Rollo.
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
 * $Id: menu.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

#ifndef __menu_h_
#define __menu_h_

#define IRCII_MENU_H

#define	SMF_ERASE	0x0001
#define	SMF_NOCURSOR	0x0002
#define	SMF_CURSONLY	0x0004
#define	SMF_CALCONLY	0x0008

/* Below are our known menu functions */
	void	menu_previous _((char *));	/* Go to previous menu */
	void	menu_submenu _((char *));	/* Invoke a submenu */
	void	menu_exit _((char *));		/* Exit the menu */
	void	menu_channels _((char *));	/* List of channels menu */
	void	menu_command _((char *));	/* Invoke an IRCII command */
	void	menu_key _((char));
	void	load_menu _((char *));
	int	ShowMenu _((char *));
	int	ShowMenuByWindow _((Window *, int));
	void	enter_menu _((unsigned char, char *));
	void	set_menu _((char *));

#endif /* __menu_h_ */
