/*
 *   Celerity C-Script
 *
 *  code:      jeremy roethel [xanth@3sheep.COM]
 *  cosmetics: sasha eysymontt [sage@3Sheep.COM]
 *
 *  $Id: celerity.c,v 1.4 2000-08-15 16:31:40 f Exp $
 */
/*
 * Cstatusbar()		- /set STATUSBAR #
 * Cquickstat()		- /QuickStat command 
 */

#include "defs.h"
#ifdef CELE
#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "screen.h"
#include "status.h"
#include "myvars.h" 
#include "vars.h"

#undef DEBUG

/* Externalities */
/* handled in include files above */

/* Global Stuff */
void Cstatusbar _((char *,char *,char *));
void Cquickstat _((char *,char *,char *));
extern void Cquick_status _((char *, int));

char *CelerityVersion=" celerity v1.1";
char *CelerityL="\002/cy/\002";

static int Cqstat=0; /* Status of QuickStat: 1=on, 0=off */

/* Internal Stuff */
#define Cstatusnum 2


/******************** Beginning of code section ********************/

/* Change the status bar [/set STATUSBAR] */
void Cstatusbar(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int n=0;
    u_int old_display;
    char *sbar=(char *) 0;
    char *newbar0;
    char *newbar1;
    char *newbar2;
    char *newbar3;
    char tmpbuf[mybufsize/2];

    sbar=new_next_arg(args,&args);
    if (sbar) n=atoi(sbar);
    if ((n>0) && (n<=Cstatusnum)) {
	if (n==2) {
	    newbar0="%y1 [OperVision] %> %!S ";
	    newbar1="%y1 %T %*%@%y3%N %# %C%+%A %Q %> %M ";
	    newbar2="%y1 %1 %> %S%H%B%I%O%F%W l/%2 ";
	    newbar3="%y1 %U [lj:%3] %> [DCC:s%6:r%5:o%4] ";
	}
	else { 		/* bar 1 is the default */
	    newbar0="%y1 [OperVision] %> %!S ";
	    newbar1="%y1 %T %*%@%y3%N %#%A %Q %> %M %1 ";
	    newbar2="%y1 %C%+ %U %> %S%H%B%I%O%F %W l/%2 ";
	    newbar3="%y1 QuickStat¯ %> [lj:%3] [DCC:s%6:r%5:o%4] ";
	}
	old_display=window_display;
	window_display=0;
	sprintf(tmpbuf,"STATUS_FORMAT %s",newbar0);
	set_variable(NULL,tmpbuf,NULL);
	sprintf(tmpbuf,"STATUS_FORMAT1 %s",newbar1);
	set_variable(NULL,tmpbuf,NULL);
	sprintf(tmpbuf,"STATUS_FORMAT2 %s",newbar2);
	set_variable(NULL,tmpbuf,NULL);
	sprintf(tmpbuf,"STATUS_FORMAT3 %s",newbar3);
	set_variable(NULL,tmpbuf,NULL);
	window_display=old_display;
    }
    else say("Error: Invalid status bar number (1 - %d)",Cstatusnum);
}

void Cquickstat(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char tmpbuf[mybufsize/16];

    if (Cqstat) {	/* Turn off */
	Cqstat=0;
        strcpy(tmpbuf,"DOUBLE 2");
    }
    else {		/* Turn on */
	Cqstat=1;
        strcpy(tmpbuf,"DOUBLE 3");
    }
    Cquick_status((char *) 0,Cqstat);
    windowcmd(NULL,tmpbuf,NULL);
    update_all_status();
}
#endif
