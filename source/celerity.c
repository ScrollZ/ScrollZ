/*
 *   Celerity C-Script
 *
 *  code:      jeremy roethel [xanth@3sheep.COM]
 *  cosmetics: sasha eysymontt [sage@3Sheep.COM]
 *
 *  $Id: celerity.c,v 1.3 2000-08-15 16:03:07 f Exp $
 */
/*
 * Cstatusbar()		- /set STATUSBAR #
 * Cquickstat()		- /QuickStat command 
 */

#include "defs.h"
#ifdef CELE
#include "irc.h"
#include "ircaux.h"
#include "screen.h"
#include "status.h"
#include "myvars.h" 
#include "vars.h"

#undef DEBUG

/* Externalities */
/* handled in include files above */

/* Global Stuff */
void Cstatusbar _((int));
void Cquickstat _((char *,char *,char *));
extern void Cquick_status _((char *, int));

char *CelerityVersion=" celerity v1.1";
char *CelerityL="\002/cy/\002";

int Cqstat=0; /* Status of QuickStat: 1=on, 0=off */

/* Internal Stuff */
static int Cstatusnum=2;


/******************** Beginning of code section ********************/

/* Change the status bar [/set STATUSBAR] 
void Cstatusbar(n)
int n;
{
    unsigned int display;
    char tmpbuf[mybufsize/4];
    char newbar0[mybufsize];
    char newbar1[mybufsize];
    char newbar2[mybufsize];
    char newbar3[mybufsize];

    if ((n > 0) && (n <= Cstatusnum)) {
	if (n==2) {
	    sprintf(newbar0,"%y1 [OperVision] %> %%!S ");
	    sprintf(newbar1,"%y1 %%T %%*%@%y3%%N %%# %%C%%+%%A %%Q %%> %%M ");
	    sprintf(newbar2,"%y1 %%1 %%> %%S%%H%%B%%I%%O%%F%%W l/%%2 ");
	    sprintf(newbar3,"%y1 %%U [lj:%%3] %%> [DCC:s%%6:r%%5:o%%4] ");
	}
	else { 		*//* bar 1 is the default 
	    sprintf(newbar0,"%y1 [OperVision] %%> %%!S ");
	    sprintf(newbar1,"%y1 %%T %%*%%@%y3%%N %%#%%A %%Q %%> %%M %%1 ");
	    sprintf(newbar2,"%y1 %%C%%+ %%U %%> %%S%%H%%B%%I%%O%%F %%W l/%%2 ");
	    sprintf(newbar3,"%y1 QuickStat¯ %%> [lj:%%3] [DCC:s%%6:r%%5:o%%4] ");
	}

	display=window_display;
	window_display=0;
	sprintf(tmpbuf,"STATUS_FORMAT %s",newbar0);
	set_variable(NULL,tmpbuf,NULL);
	sprintf(tmpbuf,"STATUS_FORMAT1 %s",newbar1);
	set_variable(NULL,tmpbuf,NULL);
	sprintf(tmpbuf,"STATUS_FORMAT2 %s",newbar2);
	set_variable(NULL,tmpbuf,NULL);
	sprintf(tmpbuf,"STATUS_FORMAT3 %s",newbar3);
	set_variable(NULL,tmpbuf,NULL);
	window_display=display;

	set_int_var(STATUSBAR_VAR,n);
    } else if (n < 0 || n > Cstatusnum) 
	say("Error: Invalid status bar number (1 - %d)",Cstatusnum);
}
*/
void Cquickstat(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    Window *window;
    int current;

    window = curr_scr_win;
    current = window->double_status;

    if (Cqstat==1) {	/* Turn off */
	Cqstat=0;
	window->double_status=1;
	window->display_size+=current-window->double_status;
	recalculate_window_positions();
	update_all_windows();
	build_status((char *) 0);
    }
    else {		/* Turn on */
	Cqstat=1;
	Cquick_status((char *) 0,Cqstat);
	window->double_status=2;
	window->display_size+=current-window->double_status;
	recalculate_window_positions();
	update_all_windows();
    }
}
#endif
