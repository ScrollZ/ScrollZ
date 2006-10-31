/*
 * term.h: header file for term.c 
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
 * $Id: ircterm.h,v 1.9 2006-10-31 12:31:27 f Exp $
 */

#ifndef __ircterm_h_
# define __ircterm_h_

#ifdef INCLUDE_CURSES_H
# include <curses.h>
#endif /* INCLUDE_CURSES_H */

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
#include <ncurses.h>
#endif /* SZNCURSES */
/****************************************************************************/

/* this is really busted on solaris -mrg */
#if 0
#ifdef INCLUDE_TERM_H
# include <term.h>
# ifdef lines
#  undef lines
# endif /* lines */
# ifdef columns
#  undef columns
# endif /* columns */
#endif /* INCLUDE_TERM_H */
#endif /* 0 */

#ifdef MUNIX
# include <sys/ttold.h>
#endif /* MUNIX */

extern	int	term_reset_flag;
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
extern	char	*CM,
		*DO,
		*CE,
		*CL,
		*CR,
		*NL,
		*SO,
		*SE,
		*US,
		*UE,
		*MD,
		*ME,
		*BL,
		*TI,
		*TE;
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/
extern	int	SG;

#ifdef NCURSES_VERSION
# define TPUTSRETVAL int
# define TPUTSARGVAL int
#else
# ifdef HPUX
#  define TPUTSRETVAL int
#  define TPUTSARGVAL char
# else /* HPUX */
#  ifdef __sgi
#   include <sys/param.h>
#   define TPUTSRETVAL int
#   define TPUTSARGVAL char
#  endif
#  if (defined(__sgi) && defined(SEEKLIMIT32)) || defined(__osf__) || defined(__SVR4)
/*
 * XXX
 *
 * if this causes your compile to fail, then just delete it.  and
 * please tell me (use the `ircbug' command).  thanks.
 */
char *tgetstr(char *, char **);
#  endif
#  ifndef __sgi
#   if defined(__linux__) || defined(_AIX) || defined(__GNU__) || defined(__FreeBSD__) || (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 104100000)
#    define TPUTSRETVAL int
#    define TPUTSARGVAL int
#   else
#    define TPUTSVOIDRET 1
#    define TPUTSRETVAL void
#    define TPUTSARGVAL int
#   endif /* __linux || _AIX */
#  endif /* __sgi */
# endif /* HPUX */
#endif /* NCURSES_VERSION */

	TPUTSRETVAL putchar_x _((TPUTSARGVAL));

#define tputs_x(s)		(tputs(s, 0, putchar_x))

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
#define term_underline_on()	(attron(A_UNDERLINE))
#define term_underline_off()	(attroff(A_UNDERLINE))
#define term_standout_on()	(attron(A_REVERSE))
#define term_standout_off()	(attroff(A_REVERSE))
#define term_clear_screen()	(clear())
#define term_move_cursor(c, r)	(move(r,c))
#define term_cr()               {}
#define term_newline()		(addch('\n'))
#define term_beep()		(beep())
#define	term_bold_on()		(attron(A_BOLD))
#define	term_bold_off()		(attroff(A_BOLD))

#else  /* SZNCURSES */
/****************************************************************************/

#define term_underline_on()	(tputs_x(US))
#define term_underline_off()	(tputs_x(UE))
#define term_standout_on()	(tputs_x(SO))
#define term_standout_off()	(tputs_x(SE))
#define term_clear_screen()	(tputs_x(CL))
#define term_move_cursor(c, r)	(tputs_x(tgoto(CM, (c), (r))))
#define term_cr()		(tputs_x(CR))
#define term_newline()		(tputs_x(NL))
#define term_beep()		(tputs_x(BL),fflush(current_screen ? \
					current_screen->fpout : stdout))
#define	term_bold_on()		(tputs_x(MD))
#define	term_bold_off()		(tputs_x(ME))
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/

	RETSIGTYPE	term_cont _((void));
 	void	term_set_fp _((FILE *));
	void	term_init _((void));
	int	term_resize _((void));
 	void	term_pause _((u_int, char *));
        int     tputs_s _((char *, size_t));
	void	term_flush _((void));
	void	term_space_erase _((int));
	void	term_reset _((void));
	void    copy_window_size _((int *, int *));
	int	term_eight_bit _((void));
	void	set_term_eight_bit _((int));
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
        int     term_read _((char *buf, size_t count));
#endif /* SZNCURSES */
/****************************************************************************/

extern int	(*term_scroll) _((int, int, int));
extern int	(*term_insert) _((u_int));
#if 0
extern int	(*term_insert_kanji) _((u_int, u_int));
#endif
extern int	(*term_delete) _((void));
extern int	(*term_cursor_right) _((void));
extern int	(*term_cursor_left) _((void));
extern int	(*term_clear_to_eol) _((void));

#if defined(ISC22) || defined(MUNIX)
/* Structure for terminal special characters */
struct	tchars
{
	char	t_intrc;	/* Interrupt			*/
	char	t_quitc;	/* Quit 			*/
	char	t_startc;	/* Start output 		*/
	char	t_stopc;	/* Stop output			*/
	char	t_eofc;		/* End-of-file (EOF)		*/
	char	t_brkc;		/* Input delimiter (like nl)	*/
}

struct ltchars
{
	char	t_suspc;	/* stop process signal		*/
	char	t_dsuspc;	/* delayed stop process signal	*/
	char	t_rprntc;	/* reprint line			*/
	char	t_flushc;	/* flush output (toggles)	*/
	char	t_werasc;	/* word erase			*/
	char	t_lnextc;	/* literal next character	*/
};
#endif /* ISC22 || MUNIX */

#if defined(_HPUX_SOURCE)

#ifndef _TTY_CHARS_ST_
#define _TTY_CHARS_ST_

/* Structure for terminal special characters */
struct tchars
{
	char	t_intrc;	/* Interrupt			*/
	char	t_quitc;	/* Quit 			*/
	char	t_startc;	/* Start output 		*/
	char	t_stopc;	/* Stop output			*/
	char	t_eofc;		/* End-of-file (EOF)		*/
	char	t_brkc;		/* Input delimiter (like nl)	*/
};

#endif /* _TTY_CHARS_ST_ */

#ifndef TIOCSETC
# define TIOCSETC	_IOW('t', 17, struct tchars)	/* set special chars */
#endif /* TIOCSETC */

#ifndef TIOCGETC
# define TIOCGETC	_IOR('t', 18, struct tchars)	/* get special chars */
#endif /* TIOCGETC */

#ifndef CBREAK
# define CBREAK		0x02	/* Half-cooked mode */
#endif /* CBREAK */

#ifndef SIGWINCH
# define    SIGWINCH    SIGWINDOW
#endif /* SIGWINCH */

#endif /* _HPUX_SOURCE */

/* well, it works */
#ifdef mips
# ifndef HAVE_FPUTC
#  define fputc(c,f) do { char x = (c); write(fileno(f),&x,1); } while (0)
# endif /* HAVE_FPUTC */
# ifndef HAVE_FWRITE
#  define fwrite(buffer,len,cnt,f) write(fileno(f),(buffer),(len)*(cnt))
# endif /* HAVE_FWRITE */
#endif /* mips */

/**************************** PATCHED by Flier ******************************/
/****** Patched by Zakath ******/
/* Linux/Alpha has a problem with incompat betw termio/termios */
#if defined(linux) && defined(__alpha__)
# undef TCGETA
# define TCGETA TCGETS
# undef TCSETA
# define TCSETA TCSETS
# undef TCSETAW 
# define TCSETAW TCSETSW
# undef termio
# define termio termios
#endif /* linux && __alpha__ */
/****** ***************** ******/
/****************************************************************************/

#endif /* __ircterm_h_ */
