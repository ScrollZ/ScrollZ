
/* C-OperVision, for ScrollZ. Coded by Zakath. */
/* Thanks to Sheik and Flier for assistance.   */
/*
 * Implemented caching for OVgetword, fixed for various ircd versions   -Flier
 *
 * There seems to be a problem when you have OperVision turned ON and
 * either server closes connection or you reconnect yourself.  Client
 * resets window levels and by doing that, messes things up (not just
 * OperVision, messages go to wrong window too). I have fixed that by
 * patching /SERVER. It checks whether OperVision was turned ON prior
 * to reconnect, and if it was it is turned OFF and immediately after
 * that it is turned back ON. This seems to fix the problem.            -Flier
 *
 * I also had to patch my functions that take care of joining channels
 * to deal with OperVision correctly. Channels were going to wrong wi-
 * ndow when OperVision was active. I just changed the way I send JOIN
 * command to server (I'm using ircII function now) and it seems to be
 * working as expected.                                                 -Flier
 *
 * When user chooses to kill OperVision window with ^WK or WINDOW KILL
 * command, we disable OperVision since they probably wanted that.      -Flier
 *
 * $Id: operv.c,v 1.2 1998-09-10 17:45:58 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "window.h"
#include "edit.h"
#include "output.h"
#include "server.h"
#include "screen.h"
#include "myvars.h"

#if defined(OPERVISION) && defined(WANTANSI)

extern void PrintUsage _((char *));

/* Variables needed for caching */
static  int  NewNotice;   /* 1 if we are parsing new notice, 0 otherwise */
static  int  OldWord;     /* holds number  for previous word, if NewNotice is 0 */
static  char *OldPtr;     /* holds pointer for previous word, if NewNotice is 0 */

/* Exceptions for my ircd that required slight changes to the code:
   Link with IRC1.FR[@127.0.0.1.6669] established.   no word #5
   Client exiting: I2 [~i2@beavis.leet.com]          no word #5 and using []'s
   Client connecting: I2 [~i2@beavis.leet.com]       using []'s */

void OperVision(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmp=(char *) 0;
    char tmpbuf[mybufsize/4];
    unsigned int display;

    if (args && *args) {
	tmp=next_arg(args,&args);
	if (!my_stricmp("ON",tmp)) {
	    if (OperV) say("OperVision is already turned on");
            else {
		OperV=1;
                ServerNotice=1;
                /* we need to send aditional usermodes (+swfuckrn) */
                send_to_server("MODE %s :+swfuckrn",get_server_nickname(from_server));
                /* made one window command, made it jump back to current window when
                   it's done, all output from /WINDOW command is supressed   -Flier */
                strcpy(tmpbuf,"NEW NAME OV DOUBLE OFF LEVEL OPNOTE,SNOTE,WALLOP REFNUM 1 GROW 6");
                display=window_display;
                window_display=0;
                window(NULL,tmpbuf,NULL);
                window_display=display;
                say("OperVision is now enabled");
	    }
	}
	else if (!my_stricmp("OFF",tmp)) {
	    if (!OperV) say("OperVision is not currently active");
	    else {
		OperV=0;
                /* we need to undo aditional usermodes (-swfuckrn) */
                send_to_server("MODE %s :-swfuckrn",get_server_nickname(from_server));
                /* made one window command, all output from /WINDOW command is
                   supressed   -Flier */
		strcpy(tmpbuf,"REFNUM OV KILL");
                display=window_display;
                window_display=0;
		window(NULL,tmpbuf,NULL);
                window_display=display;
		say("OperVision is now disabled");
	    }
	}
	else PrintUsage("OV on/off");
    }
    else PrintUsage("OV on/off");
}

/* Takes (u@h), removes (), colorizes, returns u@h */
/* Also works with [u@h]   -Flier */
/* Also works with n!u@h   -Pier  */
char *OVuh(word)
char *word;
{
    int i;
    int sht=1;
    char *tmpstr;
    char *tmphost;
    char tmpbuf1[mybufsize/4];
    static char tmpbuf2[mybufsize/4];

    /* I added sht (number-of-chars-to-short the line) -Pier */
    /* Remove the ()'s from *word (pointer +1, cat length -1) */
    tmphost=index(word,'(');
    /* We need to check for []'s if there are no ()'s since ircd that I run on
       my box reports client connecting/exiting with [] ?????   -Flier */
    if (!tmphost) tmphost=index(word,'[');
    /* Hybrid 5.1b26 seems to use n!u@h in /quote HTM notices -Pier */
    if (!tmphost) {
        tmphost=index(word,'!');
        sht=0;
    }
    if (tmphost) tmphost++;
    else return(word);
    i=strlen(tmphost);
    strmcpy(tmpbuf1,tmphost,i-sht);
    /* tmpbuf1 is the u@h, now colorize it */
    if ((tmpstr=index(tmpbuf1,'@'))) {
	*tmpstr='\0';
	tmpstr++;
	sprintf(tmpbuf2,"%s(%s%s%s%s%s@%s%s%s%s%s)%s",
	CmdsColors[COLMISC].color2,Colors[COLOFF],
	CmdsColors[COLSETTING].color4,tmpbuf1,Colors[COLOFF],
	CmdsColors[COLMISC].color1,Colors[COLOFF],
	CmdsColors[COLSETTING].color4,tmpstr,Colors[COLOFF],
	CmdsColors[COLMISC].color2,Colors[COLOFF]);
    }
    return(tmpbuf2);
}

/* Returns domain, minus host and top */
/* fixed by Flier to work on hostname.domain (like irc.net) */
char *OVsvdmn(string)
char *string;
{
    int  i,l;
    char *c;
    char *d;
    char tmpstr[mybufsize/8];
    static char tmpbuf[mybufsize/4];

    c=rindex(string,'.');
    if (!c) return(string);
    i=strlen(string);
    l=strlen(c);
    i-=l;  /* Length of top */
    strmcpy(tmpstr,string,i);
    if (!(d=rindex(tmpstr,'.'))) d=c; /* Extract domain */
    d++;
    sprintf(tmpbuf,"%s",d);
    return(tmpbuf);
}

/* Gets word(s) from string. Similar to $word() in IrcII */
/* Added caching   -Flier */
char *OVgetword(a,b,string)
int a;
int b;
char *string;
{
    int  i=1;
    static char tmpbuf1[mybufsize/4];
    static char tmpbuf2[mybufsize/4];
    char *tmpstr=tmpbuf1;
    char *tmpbuf=tmpbuf1;

    /* Caching works like this:
       You have to call this function with incrementing a or b, i.e.:
       OVgetword(0,2,blah); OVgetword(0,4,blah); OVgetword(5,0,blah);
       This should speed things up and reduce CPU usage.
       First check if this is new notice, and if it is copy entire string to buffer.
       Else, copy old pointer and work from there on, using new indexes   -Flier */
    if (NewNotice) strcpy(tmpbuf1,string);
    else {
        strcpy(tmpbuf1,OldPtr);
        i=OldWord+1;
    }
    /* If a=0, find and return word #b */
    if ((a==0) && (b>0)) {
	for(;i<=b;i++) tmpstr=next_arg(tmpbuf,&tmpbuf);
        /* Made it crash proof since my ircd formats some messages differently */
        if (tmpstr) strcpy(tmpbuf2,tmpstr);
        /* so if there is no word #b we copy empty string   -Flier */
        else *tmpbuf2='\0';
        /* Store current word number */
        OldWord=b;
        /* Store current word pointer */
        OldPtr=tmpbuf;
    }
    /* If a>0 and b=0, return from word #a to end */
    else if ((a>0) && (b==0)) {
        for(;i<a;i++) {
            tmpstr=index(tmpstr,' ');
            if (tmpstr) tmpstr++;
        }
        /* Made it crash proof since my ircd formats some messages differently */
	if (tmpstr) strcpy(tmpbuf2,tmpstr);
        /* so if there is no word #a we copy empty string   -Flier */
        else *tmpbuf2='\0';
        /* Store current word number */
        OldWord=a;
        /* Store current word pointer */
        OldPtr=tmpstr;
    }
    /* Update caching variables
       If there was no word #a or #b start from scratch since we're at the end of
       the string   -Flier */
    if (!OldPtr) NewNotice=1; 
    else NewNotice=0;
    return(tmpbuf2);
}

/* Gets nick form nick!user@host */
char *OVgetnick(nuh)
char *nuh;
{
    char *tmpstr;
    static char tmpbuf[mybufsize/4+1];

    strmcpy(tmpbuf,nuh,mybufsize/4);
    if ((tmpstr=index(tmpbuf,'!'))) *tmpstr='\0';
    return(tmpbuf);
}

void OVformat(line)
char *line;
{
    char *tmp;
    char *tmpline;
    char *servername;
    char word1[mybufsize/4];
    char word2[mybufsize/4];
    char word3[mybufsize/2];
    char word4[mybufsize/4];
    char tmpbuf[mybufsize];

    /* Set up tmpline to be just the message to parse */
    if (strstr(line,"Notice --")) tmpline=line+14;
    else if (strstr(line,"***")) tmpline=line+4; 
    else tmpline=line;
    strcpy(tmpbuf,tmpline); /* Default if no match is found */
    tmpline=tmpbuf;
    /* We got new notice, needed for caching */
    NewNotice=1;
    /* Now we got the message, use strstr() to match it up */
    if (strstr(tmpbuf,"whois on you")) {
        strcpy(word1,OVgetword(0,1,tmpline));  /* nick */
        strcpy(word2,OVgetword(0,2,tmpline));  /* user@host */
	sprintf(tmpbuf,"%s%s%s %s is doing a %sWhois%s on you.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
		CmdsColors[COLOV].color4,Colors[COLOFF]);
    }
    else if (strstr(tmpbuf,"Connecting to")) {
        strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
        sprintf(tmpbuf,"Connecting to %s%s%s",CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (strstr(tmpbuf,"Entering high-traffic mode: Forced by")) {
        strcpy(word2,OVgetword(0,6,tmpline));  /* Nick!User@Host */
        strcpy(word1,OVgetnick(word2));        /* Nick */
        sprintf(tmpbuf,"Entering high traffic mode: Forced by %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"Entering high-traffic mode")) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* High speed */
        strcpy(word2,OVgetword(0,7,tmpline));  /* Low speed */
#ifdef CELECOSM
        sprintf(tmpbuf,"high-traffic mode: %s%s%s ò %s%s%s",
#else
        sprintf(tmpbuf,"Entering high traffic mode: %s%s%s > %s%s%s",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
    }
    else if (strstr(tmpbuf,"Resuming standard operation: Forced by")) {
        strcpy(word2,OVgetword(0,6,tmpline));  /* Nick!User@Host */
        strcpy(word1,OVgetnick(word2));        /* Nick */
	sprintf(tmpbuf,"Resuming standard operation: Forced by %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"Resuming standard operation")) {
	strcpy(word1,OVgetword(0,5,tmpline));  /* Low speed */
	strcpy(word2,OVgetword(0,7,tmpline));  /* High speed */
#ifdef CELECOSM
        sprintf(tmpbuf,"standard-traffic mode: %s%s%s %s%s%s",
#else
        sprintf(tmpbuf,"Entering standard traffic mode: %s%s%s  %s%s%s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
    }
    else if (strstr(tmpbuf,"Rejecting vlad")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bot nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
	sprintf(tmpbuf,"Rejecting vlad/joh/com bot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"Rejecting clonebot")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Bot nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	sprintf(tmpbuf,"Rejecting clonebot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"Identd response differs")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* Attemtped IRCUSER */
	sprintf(tmpbuf,"Fault identd response for %s%s%s %c%s%c",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],
		bold,word2,bold);
    }
    else if (strstr(tmpbuf,"Kill line active for")) {
	strcpy(word1,OVgetword(0,5,tmpline));  /* Banned client */
#ifdef CELECOSM
        sprintf(tmpbuf,"k-line active: %s%s%s",
#else
        sprintf(tmpbuf,"Active K-Line for %s%s%s",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (strstr(tmpbuf,"LINKS requested by")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
#ifdef CELECOSM
        sprintf(tmpbuf,"%slinks%s from %s%s%s %s",
#else
        sprintf(tmpbuf,"%sLinks%s request from %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"is now operator")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(0,2,tmpline));  /* user@host */
        strcpy(word3,OVgetword(0,6,tmpline));  /* o/O */
#ifdef CELECOSM
        sprintf(tmpbuf,"%s%s%s %s is an IRC warrior %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                *word3?(*(word3+1)=='O'?"(global)":"(local)"):"");
#else
	sprintf(tmpbuf,"%s%s%s %s is now %sIRC Operator.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                *word3?(*(word3+1)=='O'?"global ":"local "):"");
#endif
    }
    else if (strstr(tmpbuf,"Client exiting")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	strcpy(word3,OVgetword(5,0,tmpline));  /* Reason */
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sexit%s  %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	sprintf(tmpbuf,"Client %sexiting%s: %s%s%s %s %s",
		CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
#endif
    }
    else if (strstr(tmpbuf,"Client connecting on port")) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* port */
        if ((tmp=index(word1,':'))) *tmp='\0';
	strcpy(word2,OVgetword(0,6,tmpline));  /* Nick */
	strcpy(word3,OVgetword(0,7,tmpline));  /* user@host */
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sconnect%s [p%s%s%s]  %s%s%s %s",
#else
        sprintf(tmpbuf,"Client %sconnecting%s on port %s%s%s: %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color5,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
    }
    else if (strstr(tmpbuf,"Client connecting")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
#ifdef CELECOSM
        sprintf(tmpbuf,"clnt/%sconnect%s  %s%s%s %s",
#else
        sprintf(tmpbuf,"Client %sconnecting%s: %s%s%s %s",
#endif
		CmdsColors[COLOV].color4,Colors[COLOFF],
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"STATS")) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Stat type */
	strcpy(word2,OVgetword(0,5,tmpline));  /* Nick */
	strcpy(word3,OVgetword(0,6,tmpline));  /* user@host */
	strcpy(word4,OVgetword(0,7,tmpline));  /* Server */
#ifdef CELECOSM
        sprintf(tmpbuf,"stats %s%s%s from %s%s%s %s %s",
#else
        sprintf(tmpbuf,"Stats %s%s%s request from %s%s%s %s %s",
#endif
		CmdsColors[COLOV].color4,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3),word4);
    }
    else if (strstr(tmpbuf,"Fake")) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Nick/Server */
	strcpy(word2,OVgetword(0,4,tmpline));  /* channel */
	strcpy(word3,OVgetword(5,0,tmpline));  /* fake modes */
#ifdef CELECOSM
        sprintf(tmpbuf,"fake mode in %s%s%s: \"%s%s%s\" by %s",
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],
                CmdsColors[COLOV].color4,word3,Colors[COLOFF],word1);
#else
        sprintf(tmpbuf,"Fake Mode: \"%s%s%s\" in %s%s%s by %s",
                CmdsColors[COLOV].color4,word3,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],word1);
#endif
    }
    else if (strstr(tmpbuf,"Nick change collision")) {
	strcpy(word1,OVgetword(0,5,tmpline));
	strcpy(word2,OVgetword(0,7,tmpline));
	strcpy(word3,OVgetword(8,0,tmpline));
#ifdef CELECOSM
        sprintf(tmpbuf,"nick collide: %s%s%s [%s] %s",
#else
        sprintf(tmpbuf,"Nick Change Collision: [%s%s%s] [%s] %s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
    }
    else if (strstr(tmpbuf,"ick collision")) {
	strcpy(word1,OVgetword(4,0,tmpline));
#ifdef CELECOSM
        sprintf(tmpbuf,"nick collide: %s%s%s",
#else
        sprintf(tmpbuf,"Nick Collide: %s%s%s",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (strstr(tmpbuf,"Too many connect")) strcpy(tmpbuf,line);
    else if (strstr(tmpbuf,"Possible bot")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Botnick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	sprintf(tmpbuf,"Possible Bot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(tmpbuf,"Link with")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	strcpy(word2,OVgetword(4,0,tmpline));  /* Connect info */
#ifdef CELECOSM
        sprintf(tmpbuf,"link/%sconnect%s  %s%s%s (%s)",
                CmdsColors[COLOV].color3,Colors[COLOFF],
#else
        sprintf(tmpbuf,"Link: Connected to %s%s%s (%s)",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2);
    }
    else if (strstr(tmpbuf,"Write error to")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Server */
#ifdef CELECOSM
        sprintf(tmpbuf,"link/%sw.error%s  %s%s%s (closing)",
                CmdsColors[COLOV].color3,Colors[COLOFF],
#else
        sprintf(tmpbuf,"Link: Write error to %s%s%s - closing.",
#endif
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    if (strstr(line,"Message")) strcpy(tmpbuf,line);
    else if (strstr(line,"is rehashing Server config")) {
        strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
#ifdef CELECOSM
        sprintf(tmpbuf,"config rehash by %s%s%s",
#else
        sprintf(tmpbuf,"%s%s%s is rehashing the server config file.",
#endif
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (strstr(line,"Received SQUIT")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	strcpy(word2,OVgetword(0,5,tmpline));  /* SQUITer */
        strcpy(word3,OVgetword(6,0,tmpline));  /* Reason */
#ifdef CELECOSM
        sprintf(tmpbuf,"link/%ssquit%s  %s%s%s from %s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#else
	sprintf(tmpbuf,"Link: %s%s%s recieved %sSQUIT%s from %s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color4,Colors[COLOFF],word2,word3);
#endif
    }

/*#^on ^window "? ??? *Resuming standard*" if ([$1]==[***]) {^xecho -window OV [$clr(yellow $tlz.sv
 *dmn($0))] Server Resuming Standard Operation.} 
*/

/*
*[irc.cs.rpi.edu] Entering high traffic mode: (18.2k/s > 18k/s)
*/
    else if (strstr(line,"Received KILL message for")) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* nick */
        if (strlen(word1) && word1[strlen(word1)-1]=='.')
            word1[strlen(word1)-1]='\0';
        strcpy(word2,OVgetword(0,7,tmpline));  /* killer  */
        strcpy(word3,OVgetword(0,7,tmpline));  /* path  */
        strcpy(word4,OVgetword(10,0));         /* reason */
        /* check for server kill first */
        if (index(word2,'.'))
            sprintf(tmpbuf,"Server kill received for %s%s%s from %s%s%s (%s)",
                    CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                    CmdsColors[COLOV].color2,word2,Colors[COLOFF],word4);
        else {
            int foreign=0;
            char *tmp=word3;
            char *tmpuh=NULL;
            char *userhost=word3;

            /* check for foreing kill (more than one !) */
            while (*tmp && (tmp=index(tmp,'!'))) {
                foreign++;
                *tmp++='\0';
                userhost=tmpuh;
                tmpuh=tmp;
            }
            if (foreign>1)
                sprintf(tmpbuf,"Foreign kill received for %s%s%s from %s%s%s %s",
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
            else {
                int locop=strstr(tmpuh,"(L")?1:0;

                sprintf(tmpbuf,"Local kill received for %s%s%s from %s%s%s%s %s",
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        locop?"local operator ":"",
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
            }
        }
    }
    else if (strstr(line,"Possible Eggdrop:")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* BotNick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	strcpy(word3,OVgetword(0,5,tmpline));  /* b-line notice */
	sprintf(tmpbuf,"Possible Eggdrop: %s%s%s %s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
    }
    else if (strstr(line,"tried to msg")) {
        strcpy(word1,OVgetword(0,2,tmpline));  /* nick */
        strcpy(word2,OVgetword(0,3,tmpline));  /* user@host */
        strcpy(word3,OVgetword(0,7,tmpline));  /* number */
        sprintf(tmpbuf,"User %s%s%s %s tried to message %s%s%s users",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color2,word3,Colors[COLOFF]);
    }
    else if (strstr(line,"Nick change: From")) {
        strcpy(word1,OVgetword(0,4,tmpline));  /* oldnick */
	strcpy(word2,OVgetword(0,6,tmpline));  /* newnick */
        strcpy(word3,OVgetword(0,7,tmpline));  /* user@host */
	sprintf(tmpbuf,"Nick change: %s%s%s to %s%s%s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
    }
    else if (strstr(line,"added K-Line")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(5,0,tmpline));  /* K-line */
        sprintf(tmpbuf,"K-Line added by %s%s%s - %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    else if (strstr(line,"Added K-Line [")) return;
    else if (strstr(line,"Bogus server name")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bogus name */
	strcpy(word2,OVgetword(0,6,tmpline));  /* Nick */
	sprintf(tmpbuf,"Bogus server name %s%s%s from %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
    }
    else if (strstr(line,"No response from")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Server */
	sprintf(tmpbuf,"Link Error: %s%s%s is not responding",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }

/*^on ^window "? % Connect: Server % already exists*" {^xecho -window OV [$clr(yellow $tlz.sv
*dmn($S))] Link: $clr(blue1 $4) already exists ù \($clr(white $left($rindex(. $8) $8))\)}
*^on ^notice "*.* Connect: *already exists*" {^xecho -window OV [$clr(yellow $tlz.sv
*dmn($0))] Link: $clr(blue1 $3) already exists ù \($clr(white $left($rindex(. $7) $7))\)}
*/

/* [irc.magg.net] Remote CONNECT alternet.one.se 4110 from Zakath */

    else if (strstr(line,"IP# Mismatch")) {
	strcpy(word1,OVgetword(0,5,tmpline));  /* Mismatched IP */
	strcpy(word2,OVgetword(0,3,tmpline));  /* Real IP */
	sprintf(tmpbuf,"IP Mismatch detected: %s%s%s != %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
    }
    else if (strstr(line,"Unauthorized connection from")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick!user@host */
	sprintf(tmpbuf,"Unauthorized connect from %s%s%s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
    }
    else if (strstr(line,"Invalid userna")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* username */
	sprintf(tmpbuf,"Invalid username: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    else if ((strstr(line,"Possible")) && (strstr(line,"bot"))) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bot Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* User@Host */
	sprintf(tmpbuf,"Possible IrcBot: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }
    else if (strstr(line,"Cannot accept connect")) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick? */
	strcpy(word2,OVgetword(5,0,tmpline));  /* Stuff? */
	sprintf(tmpbuf,"Unlinkable connection: %s%s%s [%s]",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
    }
    else if (strstr(line,"ERROR :from")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Error Source */
	strcpy(word2,OVgetword(0,7,tmpline));  /* ? */
	strcpy(word3,OVgetword(8,0,tmpline));  /* Error */
	sprintf(tmpbuf,"Error: %s%s%s - close link %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],
		CmdsColors[COLOV].color2,word2,Colors[COLOFF],word3);
    }
    else if (strstr(line,"closed the connection")) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Server */
	sprintf(tmpbuf,"Link Error: %s%s%s closed the connection.",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if ((strstr(line,"Connection to")) && (strstr(line,"activated"))) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	sprintf(tmpbuf,"Link: Connecting to %s%s%s",CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (strstr(line,"Lost connection to")) {
	strcpy(word1,OVgetword(4,0,tmpline));  /* Server */
	sprintf(tmpbuf,"Link: Lost connection to %s%s%s",CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
    }
    else if (strstr(line,"connect failure:")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Failed Server */
	strcpy(word2,OVgetword(4,0,tmpline));  /* Reason */
	sprintf(tmpbuf,"Failed connect from %s%s%s [%s]",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2);
    }
/* [rpi] Failed OPER attempt: by (gemini--) (gemini@pm1-2.slo.silcom.com) */
    else if (strstr(line,"Failed OPER attempt")) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,6,tmpline));  /* user@host */
	strcpy(word3,OVgetword(0,7,tmpline));  /* OPER nick */
	sprintf(tmpbuf,"Failed OPER attempt: %s%s%s %s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
    }
    else if (strstr(line,"Global -- Failed OPER attempt")) {
        strcpy(word1,OVgetword(0,7,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,8,tmpline));  /* user@host */
	sprintf(tmpbuf,"Failed OPER attempt: %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
    }

/*Added by Flier:
*
*[OV] ERROR :from IRC1.FR[127.0.0.1] -- Closing Link: irc.net[127.0.0.1] irc.fr (6667 blah)
*/
    servername=server_list[from_server].itsname;
    if (!servername) servername=server_list[from_server].name;
    put_it("[%s%s%s] %s",CmdsColors[COLOV].color6,OVsvdmn(servername),Colors[COLOFF],tmpbuf);
}
#endif
