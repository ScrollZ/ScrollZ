/*
 * term.c: termcap stuff... 
 *
 * Written By Michael Sandrof
 * HP-UX modifications by Mark T. Dame (Mark.Dame@uc.edu) 
 * Termio modifications by Stellan Klebom (d88-skl@nada.kth.se) 
 * Many, many cleanups, modifications, and some new broken code
 * added by Scott Reynolds (scottr@edsi.org), June 1995.
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
 * $Id: term.c,v 1.5 1999-04-25 14:41:28 f Exp $
 */

#include "irc.h"

#ifdef ESIX
# include <lan/net_types.h>
# include <lan/net_ioctl.h>
# include <sgtty.h>
# include <termio.h>
#endif /* ESIX */

#ifdef DGUX
# define _SYSV3_BAUD_RATE_FLAVOR
#endif /* DGUX */

#ifdef SVR3
# include <sys/stat.h>
# include <sgtty.h>
# include <sys/stream.h>
# ifdef HAVE_SYS_PTEM_H
#  include <sys/ptem.h>
# endif
# define CBREAK RAW
#endif /* SVR3 */

#ifdef __SVR4
# include <sys/stat.h>
# include <termios.h>
# include <sys/ttold.h>
# include <sys/stream.h>
# include <sys/ttcompat.h>
# define CBREAK RAW
#endif /* __SVR4 */

#ifdef M_UNIX
# include <sys/stat.h>
# include <sys/stream.h>
# include <sys/ptem.h>
# include <termio.h>
#endif /* M_UNIX */

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */
#ifdef HAVE_TERMIOS_H
# ifdef HPUX
#  define _INCLUDE_POSIX_SOURCE
# endif /* HPUX */
# include <termios.h>
#else
# ifdef ISC
#  include <sys/sioctl.h>
# endif /* ISC */
# ifdef HAVE_SGTTY_H
#   include <sgtty.h>
#   define USE_SGTTY
# else
#  ifdef HAVE_TERMIO_H
#   include <termio.h>
#   define termios termio
#  else
#   define USE_SGTTY
#  endif /* HAVE_TERMIO_H */
# endif /* HAVE_SGTTY_H */
#endif /* HPUX */

#include "ircterm.h"
#include "translat.h"

#ifdef ISC22
# undef TIOCSETC
# undef TIOCGETC
#endif /* ISC22 */

#if defined(POSIX) && !defined(sun) && !defined(_IBMR2)
# ifdef HAVE_SYS_TTOLD_H
#  include <sys/ttold.h>
# endif /* HAVE_SYS_TTOLD_H */
#endif /* POSIX && !sun && !_IBMR2 */

/* Missing on ConvexOS, idea picked from SunOS */
#if defined(__convex__) && !defined(LPASS8)
# define LPASS8 (L004000>>16)
#endif

#include "window.h"
#include "screen.h"

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
#include <ncurses.h>
extern void my_addstr _((char *, int));
#endif /* SZNCURSES */

#ifdef SZ32
#include <windows.h>
#endif /* SZ32 */

#include "vars.h"
/****************************************************************************/

#ifndef	STTY_ONLY
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
static int	term_curs_clear_to_eol();
static int      term_curs_scroll();
static int	term_curs_insert(char);
static int	term_curs_delete();
static int	term_curs_cursor_left();
static int	term_curs_cursor_right();
#else
/****************************************************************************/
static int	term_CE_clear_to_eol _((void));
static int	term_CS_scroll _((int, int, int));
static int	term_ALDL_scroll _((int, int, int));
static int	term_param_ALDL_scroll _((int, int, int));
static int	term_IC_insert _((u_int));
static int	term_IMEI_insert _((u_int));
static int	term_DC_delete _((void));
static int	term_null_function _((void));
static int	term_BS_cursor_left _((void));
static int	term_LE_cursor_left _((void));
static int	term_ND_cursor_right _((void));
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/
#endif /* STTY_ONLY */

static	int	tty_des;		/* descriptor for the tty */

#ifdef USE_SGTTY
static struct tchars	oldtchars,
			newtchars = { '\003', -1, -1, -1, -1, -1};
static struct sgttyb	oldb,
			newb;
# ifdef TIOCLSET
static	int	old_local_modes,
		new_local_modes;
# endif /* TIOCLSET */
#else
static struct termios	oldb,
			newb;
#endif /* USE_SGTTY */

#ifndef STTY_ONLY

/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
static	char	termcap[1024];
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/

/*
 * Function variables: each returns 1 if the function is not supported on the
 * current term type, otherwise they do their thing and return 0 
 */
int	(*term_scroll) _((int, int, int)); /* best scroll available */
int	(*term_insert) _((u_int));	/* best insert available */
int	(*term_delete) _((void));	/* best delete available */
int	(*term_cursor_left) _((void));	/* best left available */
int	(*term_cursor_right) _((void));	/* best right available */
int	(*term_clear_to_eol) _((void));	/* figure it out */

/* The termcap variables */
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
char	*CM,
	*CE,
	*CL,
	*CR,
	*NL,
	*AL,
	*DL,
	*CS,
	*DC,
	*IC,
	*IM,
	*EI,
	*SO,
	*SE,
	*US,
	*UE,
	*MD,
	*ME,
	*SF,
	*SR,
	*ND,
	*LE,
	*BL;
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/
int	CO = 79,
	LI = 24,
	SG;

/*
 * term_reset_flag: set to true whenever the terminal is reset, thus letting
 * the calling program work out what to do 
 */
int	term_reset_flag = 0;

static	int	term_echo_flag = 1;
static	int	li,
		co;

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
/* reads as much as possible < count from the keyboard */
int term_read(char *buf, size_t count) {
    int i=0;
    int c;

    while ((c=getch())!=ERR && (i<count)) {
        if (c<0 || c>255) fprintf(stderr,"ircpanic");
        buf[i++]=c;
    }
    buf[i]=0;
    return(i);
}
#endif /* SZNCURSES */
/****************************************************************************/

/*
 * term_echo: if 0, echo is turned off (all characters appear as blanks), if
 * non-zero, all is normal.  The function returns the old value of the
 * term_echo_flag 
 */
int
term_echo(flag)
	int	flag;
{
	int	old_echo;

	old_echo = term_echo_flag;
	term_echo_flag = flag;
	return (old_echo);
}

/*
 * term_putchar: puts a character to the screen, and displays control
 * characters as inverse video uppercase letters.  NOTE:  Dont use this to
 * display termcap control sequences!  It won't work! 
 */
void
term_putchar(i)
 	u_int	i;
{
/**************************** PATCHED by Flier ******************************/
 	/*char	c = (char)i;*/
        /* We need this for high ascii characters */
 	u_char c=i;
/****************************************************************************/

	if (term_echo_flag)
	{
		if (translation)
 			c = transToClient[(int)c];
/**************************** PATCHED by Flier ******************************/
		/*if (c < 32)*/
#ifdef WANTANSI
		if (c==0x1B) fputc(c, (current_screen?current_screen->fpout:stdout));
                else if (c < 32)
#else
                if (c < 32)
#endif
/****************************************************************************/
		{
			term_standout_on();
			c = (c & 127) | 64;
/**************************** PATCHED by Flier ******************************/
			/*fputc(c, (current_screen?current_screen->fpout:stdout));*/
#ifdef SZNCURSES
 			addch(c);
#else
			fputc(c, (current_screen?current_screen->fpout:stdout));
#endif /* SZNCURSES */
/****************************************************************************/
			term_standout_off();
		}
		else if (c == '\177')
		{
			term_standout_on();
			c = '?';
/**************************** PATCHED by Flier ******************************/
			/*fputc(c, (current_screen?current_screen->fpout:stdout));*/
#ifdef SZNCURSES
 			addch(c);
#else
			fputc(c, (current_screen?current_screen->fpout:stdout));
#endif /* SZNCURSES */
/****************************************************************************/
			term_standout_off();
		}
		else
/**************************** PATCHED by Flier ******************************/
			/*fputc(c, (current_screen?current_screen->fpout:stdout));*/
#ifdef SZNCURSES
 			addch(c);
#else
			fputc(c, (current_screen?current_screen->fpout:stdout));
#endif /* SZNCURSES */
/****************************************************************************/
	}
	else
	{
		c = ' ';
/**************************** PATCHED by Flier ******************************/
		/*fputc(c, (current_screen?current_screen->fpout:stdout));*/
#ifdef SZNCURSES
 		addch(c);
#else
		fputc(c, (current_screen?current_screen->fpout:stdout));
#endif /* SZNCURSES */
/****************************************************************************/
	}
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
        refresh();
#endif /* SZNCURSES */
/****************************************************************************/
}

/* term_puts: uses term_putchar to print text */
int
term_puts(str, len)
	char	*str;
 	size_t	len;
{
 	size_t	i;

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
        my_addstr(str,len);
        i=len;
#else
/****************************************************************************/
	for (i = 0; *str && (i < len); str++, i++)
/**************************** PATCHED by Flier ******************************/
 		/*term_putchar((u_int)*str);*/
                /* We need this for high ascii characters! */
 		term_putchar((u_char) *str);
#endif /* SZNCURSES */
/****************************************************************************/
	return (i);
}

/* putchar_x: the putchar function used by tputs */
TPUTSRETVAL
putchar_x(c)
	TPUTSARGVAL	c;
{
/**************************** PATCHED by Flier ******************************/
	/*fputc(c, (current_screen?current_screen->fpout:stdout));*/
#ifdef SZNCURSES
        addch(c);
        refresh();
#else
        fputc(c, (current_screen?current_screen->fpout:stdout));
#endif /* SZNCURSES */

#ifdef __linux__
        return(1);
#endif
/****************************************************************************/
}

void
term_flush()
{
/**************************** PATCHED by Flier ******************************/
	/*fflush((current_screen?current_screen->fpout:stdout));*/
#ifdef SZNCURSES
 	refresh();
#else
	fflush((current_screen?current_screen->fpout:stdout));
#endif /* SZNCURSES */
/****************************************************************************/
}

/*
 * term_reset: sets terminal attributed back to what they were before the
 * program started 
 */
void
term_reset()
{
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
#ifdef HAVE_TERMIOS_H
	tcsetattr(tty_des, TCSADRAIN, &oldb);
#else
# ifdef USE_SGTTY
	ioctl(tty_des, TIOCSETC, &oldtchars);
	ioctl(tty_des, TIOCSETP, &oldb);
# ifdef TIOCLSET
	ioctl(tty_des, TIOCLSET, &old_local_modes);
# endif
# else
	ioctl(tty_des, TCSETA, &oldb);
# endif /* USE_SGTTY */
#endif /* HAVE_TERMIOS_H */

#if (defined(mips) && !defined(ultrix)) || defined(ISC22)
	new_stty("cooked");
#endif /* mips */

	if (CS)
		tputs_x(tgoto(CS, LI - 1, 0));
	term_move_cursor(0, LI - 1);
	term_reset_flag = 1;
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/
	term_flush();
}

/*
 * term_cont: sets the terminal back to IRCII stuff when it is restarted
 * after a SIGSTOP.  Somewhere, this must be used in a signal() call 
 */
RETSIGTYPE
term_cont()
{
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
#ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGCONT, term_cont, 0); /* sysv has dumb signals */
#endif /* SYSVSIGNALS */

#if defined(SIGSTOP) && defined(SIGTSTP) /* munix has no sigstop, sigtstp */

# ifdef HAVE_TERMIOS_H
	tcsetattr(tty_des, TCSADRAIN, &newb);
# else
#  ifdef USE_SGTTY
	ioctl(tty_des, TIOCSETC, &newtchars);
	ioctl(tty_des, TIOCSETP, &newb);
#   ifdef TIOCLSET
	ioctl(tty_des, TIOCLSET, &new_local_modes);
#   endif
#  else
	ioctl(tty_des, TCSETA, &newb);
#  endif /* USE_SGTTY */
# endif /* HAVE_TERMIOS_H */

# if defined(ISC22)
	new_stty("opost");
# endif /* ISC22 */
# if defined(mips) && !defined(ultrix) /*ultrix/mips silliness*/
	new_stty("raw -echo");
# endif /* mips */
#endif /* SIGSTOP && SIGTSTP */
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/
}

/*
 * term_pause: sets terminal back to pre-program days, then SIGSTOPs itself. 
 */
void
term_pause(key, ptr)
 	u_int	key;
	char *	ptr;
{
#if !defined(SIGSTOP) || !defined(SIGTSTP) || defined(_RT) || defined(ESIX)
	say("The STOP_IRC function does not work on this system type.");
#else
	term_reset();
	kill(getpid(), SIGSTOP);
#endif /* MUNIX */
}
#endif /* STTY_ONLY */


/*
 * term_init: does all terminal initialization... reads termcap info, sets
 * the terminal to CBREAK, no ECHO mode.   Chooses the best of the terminal
 * attributes to use ..  for the version of this function that is called for
 * wserv, we set the termial to RAW, no ECHO, so that all the signals are
 * ignored.. fixes quite a few problems...  -phone, jan 1993..
 */
void
term_init()
{
#ifndef	STTY_ONLY
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
        /* ncurses init and allocation of colormap */
        initscr();
        nonl();
        intrflush(stdscr,FALSE);
        keypad(stdscr,FALSE);
        nodelay(stdscr,TRUE);
        noecho();
        cbreak();
        if (has_colors()) {
            int j,k;
            start_color();
            /* index is (bg%8)*8 + fg */
            for (j=0;j<8;j++) /* j = bg, k = fg */
                for (k=0;k<8;k++) init_pair(8*j+k,k,j);
        }
        co=COLS-1; /* a lucky guess */
        li=LINES;
        CO=co;
#else  /* SZNCURSES */
	/*char	bp[1024],*/
	char	bp[2048],
/****************************************************************************/
		*term,
		*ptr;

	if ((term = getenv("TERM")) == (char *) 0)
	{
		fprintf(stderr, "irc: No TERM variable set!\n");
		fprintf(stderr,"irc: You may still run irc by using the -d switch\n");
		exit(1);
	}
	if (tgetent(bp, term) < 1)
	{
		fprintf(stderr, "irc: No termcap entry for %s.\n", term);
		fprintf(stderr,"irc: You may still run irc by using the -d switch\n");
		exit(1);
	}
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
#ifdef SZ32
        SetConsoleTitle("ScrollZ/32");
#endif /* SZ32 */
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
 	term_clear_to_eol=term_curs_clear_to_eol;
 	term_cursor_right=term_curs_cursor_right;
 	term_cursor_left=term_curs_cursor_left;
 	term_scroll=term_curs_scroll;
 	term_insert=term_curs_insert;
 	term_delete=term_curs_delete;
#else
/****************************************************************************/
	if ((co = tgetnum("co")) == -1)
		co = 80;
	if ((li = tgetnum("li")) == -1)
		li = 24;
	ptr = termcap;

	/*
	 * Thanks to Max Bell (mbell@cie.uoregon.edu) for info about TVI
	 * terminals and the sg terminal capability 
	 */
	SG = tgetnum("sg");
	CM = tgetstr("cm", &ptr);
	CL = tgetstr("cl", &ptr);
	if ((CM == (char *) 0) ||
	    (CL == (char *) 0))
	{
		fprintf(stderr, "This terminal does not have the necessary capabilities to run IRCII\nin full screen mode. You may still run irc by using the -d switch\n");
		exit(1);
	}
	if ((CR = tgetstr("cr", &ptr)) == (char *) 0)
		CR = "\r";
	if ((NL = tgetstr("nl", &ptr)) == (char *) 0)
		NL = "\n";

	if ((CE = tgetstr("ce", &ptr)) != NULL)
		term_clear_to_eol = term_CE_clear_to_eol;
	else
		term_clear_to_eol = term_null_function;


	/* if ((ND = tgetstr("nd", &ptr)) || (ND = tgetstr("kr", &ptr))) */
	if ((ND = tgetstr("nd", &ptr)) != NULL)
		term_cursor_right = term_ND_cursor_right;
	else
		term_cursor_right = term_null_function;

	/* if ((LE = tgetstr("le", &ptr)) || (LE = tgetstr("kl", &ptr))) */
	if ((LE = tgetstr("le", &ptr)) != NULL)
		term_cursor_left = term_LE_cursor_left;
	else if (tgetflag("bs"))
		term_cursor_left = term_BS_cursor_left;
	else
		term_cursor_left = term_null_function;

	SF = tgetstr("sf", &ptr);
	SR = tgetstr("sr", &ptr);

	if ((CS = tgetstr("cs", &ptr)) != NULL)
		term_scroll = term_CS_scroll;
	else if ((AL = tgetstr("AL", &ptr)) && (DL = tgetstr("DL", &ptr)))
		term_scroll = term_param_ALDL_scroll;
	else if ((AL = tgetstr("al", &ptr)) && (DL = tgetstr("dl", &ptr)))
		term_scroll = term_ALDL_scroll;
	else
		term_scroll = (int (*) _((int, int, int))) term_null_function;

	if ((IC = tgetstr("ic", &ptr)) != NULL)
		term_insert = term_IC_insert;
	else
	{
		if ((IM = tgetstr("im", &ptr)) && (EI = tgetstr("ei", &ptr)))
			term_insert = term_IMEI_insert;
		else
 			term_insert = (int (*) _((u_int))) term_null_function;
	}

	if ((DC = tgetstr("dc", &ptr)) != NULL)
		term_delete = term_DC_delete;
	else
		term_delete = term_null_function;

	SO = tgetstr("so", &ptr);
	SE = tgetstr("se", &ptr);
	if ((SO == (char *) 0) || (SE == (char *) 0))
	{
		SO = empty_string;
		SE = empty_string;
	}
	US = tgetstr("us", &ptr);
	UE = tgetstr("ue", &ptr);
	if ((US == (char *) 0) || (UE == (char *) 0))
	{
		US = empty_string;
		UE = empty_string;
	}
	MD = tgetstr("md", &ptr);
	ME = tgetstr("me", &ptr);
	if ((MD == (char *) 0) || (ME == (char *) 0))
	{
		MD = empty_string;
		ME = empty_string;
	}
	if ((BL = tgetstr("bl", &ptr)) == (char *) 0)
		BL = "\007";
#endif /* STTY_ONLY */

	if (getenv("IRC_DEBUG")|| (tty_des = open("/dev/tty", O_RDWR, 0)) == -1)
		tty_des = 0;

#ifdef HAVE_TERMIOS_H
	tcgetattr(tty_des, &oldb);
#else
# ifdef USE_SGTTY
	ioctl(tty_des, TIOCGETC, &oldtchars);
	ioctl(tty_des, TIOCGETP, &oldb);
# else
	ioctl(tty_des, TCGETA, &oldb);
# endif /* USE_SGTTY */
#endif /* HAVE_TERMIOS_H */

#ifdef USE_SGTTY
	newb = oldb;
	newb.sg_flags &= ~CRMOD;
# ifdef TIOCLSET
	ioctl(tty_des, TIOCLGET, &old_local_modes);
	new_local_modes = old_local_modes | LDECCTQ | LLITOUT | LNOFLSH;
	ioctl(tty_des, TIOCLSET, &new_local_modes);
# endif


# ifndef STTY_ONLY
	if (use_flow_control)
	{
		newtchars.t_startc = oldtchars.t_startc;
		newtchars.t_stopc = oldtchars.t_stopc;
	}
	newb.sg_flags |= CBREAK;
# else
	newb.sg_flags |= RAW;
# endif /* STTY_ONLY */

# if !defined(_HPUX_SOURCE)
	newb.sg_flags &= (~ECHO);
# endif /* _HPUX_SOURCE */

#else /* USE_SGTTY */

	newb = oldb;
	newb.c_lflag &= ~(ICANON | ECHO);	/* set equivalent of
						 * CBREAK and no ECHO
						 */
	newb.c_cc[VMIN] = 1;	/* read() satified after 1 char */
	newb.c_cc[VTIME] = 0;	/* No timer */

#ifndef _POSIX_VDISABLE
# define _POSIX_VDISABLE 0
#endif
	newb.c_cc[VQUIT] = _POSIX_VDISABLE;
# ifdef VDISCARD
	newb.c_cc[VDISCARD] = _POSIX_VDISABLE;
# endif
# ifdef VDSUSP
	newb.c_cc[VDSUSP] = _POSIX_VDISABLE;
# endif
# ifdef VSUSP
	newb.c_cc[VSUSP] = _POSIX_VDISABLE;
# endif

# ifndef STTY_ONLY
	if (!use_flow_control)
		newb.c_iflag &= ~IXON;	/* No XON/XOFF */
# endif /* STTY_ONLY */

#endif /* USE_SGTTY */

#ifdef HAVE_TERMIOS_H
	tcsetattr(tty_des, TCSADRAIN, &newb);
#else
# ifdef USE_SGTTY
	ioctl(tty_des, TIOCSETC, &newtchars);
	ioctl(tty_des, TIOCSETP, &newb);
# else
	ioctl(tty_des, TCSETA, &newb);
# endif /* USE_SGTTY */
#endif /* HAVE_TERMIOS_H */

#ifndef STTY_ONLY
#if defined(mips) && !defined(ultrix)
	new_stty("raw -echo");
#endif /* mips */
#endif /* STTY_ONLY */
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/
}


#ifndef STTY_ONLY
/*
 * term_resize: gets the terminal height and width.  Trys to get the info
 * from the tty driver about size, if it can't... uses the termcap values. If
 * the terminal size has changed since last time term_resize() has been
 * called, 1 is returned.  If it is unchanged, 0 is returned. 
 */
int
term_resize()
{
	static	int	old_li = -1,
			old_co = -1;

/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
 	old_li=li;
 	old_co=co;
 	endwin();
        /* ncurses init and allocation of colormap */
        initscr();
        nonl();
        intrflush(stdscr,FALSE);
        keypad(stdscr,FALSE);
        nodelay(stdscr,TRUE);
        noecho();
        cbreak();
        if (has_colors()) {
            int j,k;
            start_color();
            /* index is (bg%8)*8 + fg */
            for (j=0;j<8;j++) /* j = bg, k = fg */
                for (k=0;k<8;k++) init_pair(8*j+k,k,j);
        }
 	LI=LINES;
        CO=COLS-1;
 	SG=-1; /* what is this ? */
#else
/****************************************************************************/
#ifndef TIOCGWINSZ
	LI = li;
	CO = co;
#else
	struct	winsize window;

	if (ioctl(tty_des, TIOCGWINSZ, &window) < 0)
	{
		LI = li;
		CO = co;
	}
	else
	{
		if ((LI = window.ws_row) == 0)
			LI = li;
		if ((CO = window.ws_col) == 0)
			CO = co;
	}
#endif /* TIOCGWINSZ */

	CO--;
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/
	if ((old_li != LI) || (old_co != CO))
	{
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
		old_li = LI;
		old_co = CO;
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/
		return (1);
	}
	return (0);
}

/*
 * term_null_function: used when a terminal is missing a particulary useful
 * feature, such as scrolling, to warn the calling program that no such
 * function exists 
 */
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
static	int
term_null_function()
{
	return (1);
}
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/

/* term_CE_clear_to_eol(): the clear to eol function, right? */
static	int
/**************************** PATCHED by Flier ******************************/
/*term_CE_clear_to_eol()*/
#ifdef SZNCURSES
term_curs_clear_to_eol()
#else
term_CE_clear_to_eol()
#endif
/****************************************************************************/
{
/**************************** PATCHED by Flier ******************************/
	/*tputs_x(CE);*/
#ifdef SZNCURSES
        clrtoeol();
#else
	tputs_x(CE);
#endif
/****************************************************************************/
	return (0);
}

/* * term_space_erase: this can be used if term_CE_clear_to_eol() returns 1.
 * This will erase from x to the end of the screen uses space.  Actually, it
 * doesn't reposition the cursor at all, so the cursor must be in the correct
 * spot at the beginning and you must move it back afterwards 
 */
void
term_space_erase(x)
	int	x;
{
	char	c = ' ';
	int	i,
		cnt;

	cnt = CO - x;
	for (i = 0; i < cnt; i++)
		fputc(c, (current_screen?current_screen->fpout:stdout));
}

/*
 * term_CS_scroll: should be used if the terminal has the CS capability by
 * setting term_scroll equal to it 
 */
static	int
/**************************** PATCHED by Flier ******************************/
/*term_CS_scroll(line1, line2, n)*/
#ifdef SZNCURSES
term_curs_scroll(line1,line2,n)
#else
term_CS_scroll(line1, line2, n)
#endif
/****************************************************************************/
	int	line1,
		line2,
		n;
{
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
 	scrollok(stdscr,TRUE);
 	setscrreg(line1,line2);
 	scrl(n);
 	wrefresh(stdscr);
 	scrollok(stdscr,FALSE);
#else
/**************************** PATCHED by Flier ******************************/
	int	i;
	char	*thing;

	if (n > 0)
		thing = SF ? SF : NL;
	else if (n < 0)
	{
		if (SR)
			thing = SR;
		else
			return 1;
	}
	else
		return 0;
	tputs_x(tgoto(CS, line2, line1));  /* shouldn't do this each time */
	if (n < 0)
	{
		term_move_cursor(0, line1);
		n = -n;
	}
	else
		term_move_cursor(0, line2);
	for (i = 0; i < n; i++)
		tputs_x(thing);
	tputs_x(tgoto(CS, LI - 1, 0));	/* shouldn't do this each time */
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/
	return (0);
}

/*
 * term_ALDL_scroll: should be used for scrolling if the term has AL and DL
 * by setting the term_scroll function to it 
 */
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
static	int
term_ALDL_scroll(line1, line2, n)
	int	line1,
		line2,
		n;
{
	int	i;

	if (n > 0)
	{
		term_move_cursor(0, line1);
		for (i = 0; i < n; i++)
			tputs_x(DL);
		term_move_cursor(0, line2 - n + 1);
		for (i = 0; i < n; i++)
			tputs_x(AL);
	}
	else if (n < 0)
	{
		n = -n;
		term_move_cursor(0, line2-n+1);
		for (i=0; i < n; i++)
			tputs_x(DL);
		term_move_cursor(0, line1);
		for (i=0; i < n; i++)
			tputs_x(AL);
	}
	return (0);
}
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/

/*
 * term_param_ALDL_scroll: Uses the parameterized version of AL and DL 
 */
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
static	int
term_param_ALDL_scroll(line1, line2, n)
	int	line1,
		line2,
		n;
{
	if (n > 0)
	{
		term_move_cursor(0, line1);
		tputs_x(tgoto(DL, n, n));
		term_move_cursor(0, line2 - n + 1);
		tputs_x(tgoto(AL, n, n));
	}
	else if (n < 0)
	{
		n = -n;
		term_move_cursor(0, line2-n+1);
		tputs_x(tgoto(DL, n, n));
		term_move_cursor(0, line1);
		tputs_x(tgoto(AL, n, n));
	}
	return (0);
}
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/

/*
 * term_IC_insert: should be used for character inserts if the term has IC by
 * setting term_insert to it. 
 */
static	int
/**************************** PATCHED by Flier ******************************/
/*term_IC_insert(c)
 	u_int	c;*/
#ifdef SZNCURSES
term_curs_insert(c)
char c;
#else  /* SZNCURSES */
term_IC_insert(c)
u_int c;
#endif /* SZNCURSES */
/****************************************************************************/
{
/**************************** PATCHED by Flier ******************************/
	/*tputs_x(IC);
	term_putchar(c);*/
#ifdef SZNCURSES
 	int y,x;

 	insch(c);
 	getyx(stdscr,y,x);
 	term_move_cursor(x+1,y);
        refresh();
#else
	tputs_x(IC);
	term_putchar(c);
#endif /* SZNCURSES */
/****************************************************************************/
	return (0);
}

/*
 * term_IMEI_insert: should be used for character inserts if the term has IM
 * and EI by setting term_insert to it 
 */
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
static	int
term_IMEI_insert(c)
 	u_int	c;
{
	tputs_x(IM);
	term_putchar(c);
	tputs_x(EI);
	return (0);
}
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/

/*
 * term_DC_delete: should be used for character deletes if the term has DC by
 * setting term_delete to it 
 */
static	int
/**************************** PATCHED by Flier ******************************/
/*term_DC_delete()*/
#ifdef SZNCURSES
term_curs_delete()
#else
term_DC_delete()
#endif /* SZNCURSES */
/****************************************************************************/
{
/**************************** PATCHED by Flier ******************************/
	/*tputs_x(DC);*/
#ifdef SZNCURSES
 	delch();
 	refresh();
#else
        tputs_x(DC);
#endif /* SZNCURSES */
/****************************************************************************/
	return (0);
}

/* term_ND_cursor_right: got it yet? */
static	int
/**************************** PATCHED by Flier ******************************/
/*term_ND_cursor_right()*/
#ifdef SZNCURSES
term_curs_cursor_right()
#else
term_ND_cursor_right()
#endif /* SZNCURSES */
/****************************************************************************/
{
/**************************** PATCHED by Flier ******************************/
	/*tputs_x(ND);*/
#ifdef SZNCURSES
 	int y,x;

 	getyx(stdscr,y,x);
 	term_move_cursor(x+1,y);
#else
	tputs_x(ND);
#endif /* SZNCURSES */
/****************************************************************************/
	return (0);
}

/* term_LE_cursor_left:  shouldn't you move on to something else? */
static	int
/**************************** PATCHED by Flier ******************************/
/*term_LE_cursor_left()*/
#ifdef SZNCURSES
term_curs_cursor_left()
#else
term_LE_cursor_left()
#endif /* SZNCURSES */
/****************************************************************************/
{
/**************************** PATCHED by Flier ******************************/
	/*tputs_x(LE);*/
#ifdef SZNCURSES
 	int y,x;

 	getyx(stdscr,y,x);
 	term_move_cursor(x-1,y);
#else
	tputs_x(LE);
#endif /* SZNCURSES */
/****************************************************************************/
	return (0);
}

/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
static	int
term_BS_cursor_left()
{
	char	c = '\010';

	fputc(c, (current_screen ? current_screen->fpout : stdout));
	return (0);
}

extern	void
copy_window_size(nlines, cols)
	int	*nlines,
		*cols;
{
	*nlines = li;
	*cols = co;
}
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/

extern	int
term_eight_bit()
{
/**************************** PATCHED by Flier ******************************/
#ifdef SZNCURSES
        return(1);
#else
/****************************************************************************/
#ifdef USE_SGTTY
	return (old_local_modes & LPASS8) ? 1 : 0;
#else
	return (((oldb.c_cflag) & CSIZE) == CS8) ? 1 : 0;
#endif /* USE_SGTTY */
/**************************** PATCHED by Flier ******************************/
#endif /* SZNCURSES */
/****************************************************************************/
}
#endif /* STTY_ONLY */


extern	void
set_term_eight_bit(value)
	int	value;
{
/**************************** PATCHED by Flier ******************************/
#ifndef SZNCURSES
/****************************************************************************/
#ifdef USE_SGTTY
	if (value == ON)
		new_local_modes |= LPASS8;
	else
		new_local_modes &= ~LPASS8;
	ioctl(tty_des, TIOCLSET, &new_local_modes);
#else
	if (value == ON)
	{
		newb.c_cflag |= CS8;
		newb.c_iflag &= ~ISTRIP;
	}
	else
	{
		newb.c_cflag &= ~CS8;
		newb.c_iflag |= ISTRIP;
	}
# ifdef HAVE_TERMIOS_H
	tcsetattr(tty_des, TCSADRAIN, &newb);
# else
	ioctl(tty_des, TCSETA, &newb);
# endif /* HAVE_TERMIOS_H */
#endif /* USE_SGTTY */
/**************************** PATCHED by Flier ******************************/
#endif /* ! SZNCURSES */
/****************************************************************************/
}
