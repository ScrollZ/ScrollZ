/*
 * ircsig.c: has a `my_signal()' that uses sigaction().
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

/*
 * written by matthew green, 1993.
 *
 * i stole bits of this from w. richard stevens' `advanced programming
 * in the unix environment' -mrg
 */

/**************************** PATCHED by Flier ******************************
#ifndef lint
static	char	rcsid[] = "@(#)$Id: ircsig.c,v 1.1 1998-09-10 17:31:13 f Exp $";
#endif
****************************************************************************/

#include "irc.h"
#include "irc_std.h"

#ifdef USE_SIGACTION
sigfunc *
my_signal(sig_no, sig_handler, misc_flags)
	int sig_no;
	sigfunc *sig_handler;
	int misc_flags;
{
        /*
         * misc_flags is unused currently.  it's planned to be used
         * to use some of the doovier bits of sigaction(), if at
         * some point we need them, -mrg
         */

        struct sigaction sa, osa;

	/*
	 * have to (void *) this because of strict type checking
	 * on some systems -glen
	 */
        sa.sa_handler = (void *)sig_handler;

        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, sig_no);

        /* this is ugly, but the `correct' way.  i hate c. -mrg */
        sa.sa_flags = 0;
#if defined(SA_RESTART) || defined(SA_INTERRUPT)
        if (SIGALRM == sig_no)
        {
# if defined(SA_INTERRUPT)
                sa.sa_flags |= SA_INTERRUPT;
# endif /* SA_INTERRUPT */
        }
        else
        {
# if defined(SA_RESTART)
                sa.sa_flags |= SA_RESTART;
# endif /* SA_RESTART */
        }
#endif /* SA_RESTART || SA_INTERRUPT */

        if (0 > sigaction(sig_no, &sa, &osa))
                return ((sigfunc *)SIG_ERR);

        return ((sigfunc *)osa.sa_handler);
}
#endif /* USE_SIGACTION */
