
/* C-OperVision, for ScrollZ. Coded by Zakath. */
/* Thanks to Sheik and Flier for assistance.   */
/*
 * Comments by Flier:
 * Implemented caching for OVgetword, fixed for various ircd versions.
 * There seems to be a problem when you have OperVision turned ON and
 * either server closes connection or you reconnect yourself.  Client
 * resets window levels and by doing that, messes things up (not just
 * OperVision, messages go to wrong window too). I have fixed that by
 * patching calling OperVisionReinit() which reinstates window levels.
 * I also had to patch my functions that take care of joining channels
 * to deal with OperVision correctly. Channels were going to wrong wi-
 * ndow when OperVision was active. I just changed the way I send JOIN
 * command to server (I'm using ircII function now) and it seems to be
 * working as expected.
 * When user chooses to kill OperVision window with ^WK or WINDOW KILL
 * command, we disable OperVision since they probably wanted that.
 *
 * $Id: operv.c,v 1.43 2002-02-23 10:23:39 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "window.h"
#include "edit.h"
#include "output.h"
#include "server.h"
#include "screen.h"
#include "status.h"
#include "myvars.h"

#if defined(OPERVISION) && defined(WANTANSI)

extern void PrintUsage _((char *));

/* Variables needed for caching */
static  int  NewNotice; /* 1 if we are parsing new notice, 0 otherwise */
static  int  OldWord;   /* holds number  for previous word, if NewNotice is 0 */
static  int  OVTS=1;    /* 1 if time stamping is enabled */
static  char *OldPtr;   /* holds pointer for previous word, if NewNotice is 0 */

void CreateMode(tmpbuf,sizeofbuf)
char *tmpbuf;
int  sizeofbuf;
{
    /* we need to send aditional usermodes (+swfuckrn), for ircd 2.9/2.10 only send +w */
    if (get_server_version(from_server)==Server2_9 || 
        get_server_version(from_server)==Server2_10)
        strmcpy(tmpbuf,"w",sizeofbuf);
    else strmcpy(tmpbuf,"swfuckrn",sizeofbuf);
}

void OperVision(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int incurwin=0;
    int sendmodes=1;
    char *tmp=(char *) 0;
    char *ovts=(char *) 0;
    char *nomodes=(char *) 0;
    char tmpbuf[mybufsize/4+1];
    unsigned int display;

    tmp=next_arg(args,&args);
    ovts=next_arg(args,&args);
    if (tmp) {
	if (!my_stricmp("ON",tmp) || !my_stricmp("HERE",tmp)) {
            if (!my_stricmp("HERE",tmp)) incurwin=1;
            if (ovts && !my_stricmp("NOTS",ovts)) OVTS=0;
            else OVTS=1;
            if (ovts && !my_stricmp("NOMODES",ovts)) nomodes=ovts;
            else nomodes=next_arg(args,&args);
            if (nomodes && !my_stricmp("NOMODES",nomodes)) sendmodes=0;
	    if (OperV && !incurwin)
                say("OperVision is already turned on");
            else {
		OperV=1;
                ServerNotice=1;
                /* turn on additional user modes */
                if (sendmodes) {
                    CreateMode(tmpbuf,mybufsize/4);
                    send_to_server("MODE %s :+%s",get_server_nickname(from_server),tmpbuf);
                }
                /* made one window command, made it jump back to current window when
                   it's done, all output from /WINDOW command is supressed   -Flier */
                if (incurwin)
                    strcpy(tmpbuf,"NAME OV DOUBLE OFF LEVEL +OPNOTE,SNOTE,WALLOP");
                else
                    snprintf(tmpbuf,sizeof(tmpbuf),"NEW NAME OV DOUBLE OFF LEVEL OPNOTE,SNOTE,WALLOP REFNUM %d GROW 6",curr_scr_win->refnum);
                display=window_display;
                window_display=0;
                windowcmd(NULL,tmpbuf,NULL);
                window_display=display;
	    }
            say("OperVision is now enabled%s, time stamping is %sabled",
                incurwin?" in current window":"",
                OVTS?"en":"dis");
	}
	else if (!my_stricmp("OFF",tmp)) {
	    if (!OperV) say("OperVision is not currently active");
	    else {
		OperV=0;
                nomodes=ovts;
                if (nomodes && !my_stricmp("NOMODES",nomodes)) sendmodes=0;
                /* we need to undo aditional usermodes (-swfuckrn) */
                if (sendmodes) {
                    CreateMode(tmpbuf,mybufsize/4);
                    send_to_server("MODE %s :-%s",get_server_nickname(from_server),tmpbuf);
                }
                /* made one window command, all output from /WINDOW command is
                   supressed   -Flier */
		strcpy(tmpbuf,"REFNUM OV KILL");
                display=window_display;
                window_display=0;
		windowcmd(NULL,tmpbuf,NULL);
                window_display=display;
		say("OperVision is now disabled");
	    }
	}
	else PrintUsage("OV on [nots] [nomodes]/here [nots] [nomodes]/off [nomodes]");
    }
    else PrintUsage("OV on [nots] [nomodes]/here [nots] [nomodes]/off [nomodes]");
}

/* Takes (u@h), removes (), colorizes, returns u@h */
/* Also works with [u@h]   -Flier */
/* Also works with n!u@h   -Pier  */
char *OVuh(word)
char *word;
{
    int i;
    int sht = 1;
    char *tmpstr;
    char *tmphost;
    char tmpbuf1[mybufsize / 4];
    static char tmpbuf2[mybufsize / 4];

    /* I added sht (number-of-chars-to-short the line) -Pier */
    /* Remove the ()'s from *word (pointer +1, cat length -1) */
    tmphost = index(word,'(');
    /* We need to check for []'s if there are no ()'s since ircd that I run on
       my box reports client connecting/exiting with [] ?????   -Flier */
    if (!tmphost) tmphost = index(word,'[');
    /* Hybrid 5.1b26 seems to use n!u@h in /quote HTM notices -Pier */
    if (!tmphost) {
        tmphost = index(word, '!');
        sht = 0;
    }
    if (tmphost) tmphost++;
    else return(word);
    i = strlen(tmphost);
    strmcpy(tmpbuf1, tmphost, i - sht + 1);
    /* tmpbuf1 is the u@h, now colorize it */
    if ((tmpstr = index(tmpbuf1, '@'))) {
	*tmpstr = '\0';
	tmpstr++;
	snprintf(tmpbuf2, sizeof(tmpbuf2), "%s(%s%s%s%s%s@%s%s%s%s%s)%s",
	        CmdsColors[COLMISC].color2, Colors[COLOFF],
	        CmdsColors[COLSETTING].color4, tmpbuf1, Colors[COLOFF],
	        CmdsColors[COLMISC].color1, Colors[COLOFF],
	        CmdsColors[COLSETTING].color4, tmpstr, Colors[COLOFF],
	        CmdsColors[COLMISC].color2, Colors[COLOFF]);
    }
    return(tmpbuf2);
}

/* Returns domain, minus host and top */
/* fixed by Flier to work on hostname.domain (like irc.net) */
char *OVsvdmn(string)
char *string;
{
    int  i, l;
    char *c;
    char *d;
    char tmpstr[mybufsize / 8];
    static char tmpbuf[mybufsize / 4];

    c = rindex(string, '.');
    if (!c) return(string);
    i = strlen(string);
    l = strlen(c);
    i -= l;  /* Length of top */
    strmcpy(tmpstr, string, i+1);
    if (!(d = rindex(tmpstr, '.'))) d = c; /* Extract domain */
    d++;
    snprintf(tmpbuf, sizeof(tmpbuf), "%s", d);
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
    static char tmpbuf1[mybufsize/2];
    static char tmpbuf2[mybufsize/2];
    char *tmpstr=tmpbuf1;
    char *tmpbuf=tmpbuf1;

    /* Caching works like this:
       You have to call this function with incrementing a or b, i.e.:
       OVgetword(0,2,blah); OVgetword(0,4,blah); OVgetword(5,0,blah);
       This should speed things up and reduce CPU usage.
       First check if this is new notice, and if it is copy entire string to buffer.
       Else, copy old pointer and work from there on, using new indexes   -Flier */
    if (NewNotice) strmcpy(tmpbuf1,string,sizeof(tmpbuf1));
    else {
        strmcpy(tmpbuf1,OldPtr,sizeof(tmpbuf1));
        i=OldWord+1;
    }
    /* If a=0, find and return word #b */
    if ((a==0) && (b>0)) {
	for(;i<=b;i++) tmpstr=next_arg(tmpbuf,&tmpbuf);
        /* Made it crash proof since my ircd formats some messages differently */
        if (tmpstr) strmcpy(tmpbuf2,tmpstr,sizeof(tmpbuf2));
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
	if (tmpstr) strmcpy(tmpbuf2,tmpstr,sizeof(tmpbuf2));
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
    static char tmpbuf[mybufsize/4];

    strmcpy(tmpbuf,nuh,sizeof(tmpbuf));
    if ((tmpstr=index(tmpbuf,'!'))) *tmpstr='\0';
    return(tmpbuf);
}

void OVformat(line,from)
char *line;
char *from;
{
    char *tmp;
    char *tmpline;
    char *curtime;
    char *origfrom;
    char *servername;
    char word1[mybufsize];
    char word2[mybufsize];
    char word3[mybufsize];
    char word4[mybufsize];
    char tmpbuf[mybufsize];

    /* Set up tmpline to be just the message to parse */
    if (!strncmp(line,"*** Notice -- ",14)) tmpline=line+14;
    /* SZNet support */
    else if (!strncmp(line,"*** Global -- ",14)) tmpline=line+14;
    else if (!strncmp(line,"***",4)) tmpline=line+4; 
    else {
        tmpline=index(line,' ');
        if (!tmpline) tmpline=line;
        else {
            tmpline++;
            if (!strncmp(tmpline,"*** Notice -- ",14)) tmpline+=14;
            else tmpline=line;
        }
    }
    strmcpy(tmpbuf,tmpline,sizeof(tmpbuf)); /* Default if no match is found */
    tmpline=tmpbuf;
    /* If from has '.' in it is is server */
    origfrom=from;
    if (from && index(from,'.')) from=(char *) 0;
    /* We got new notice, needed for caching */
    NewNotice=1;
    /* Now we got the message, use strstr() to match it up */
    /* OVgetword() and OVgetnick() return max mybufsize/2 so we are safe */
    if (!strncmp(tmpline,"Connecting to",12)) {
        strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] connecting to %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Connecting to %s%s%s",CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Entering high-traffic mode: Forced by",36)) {
        strcpy(word2,OVgetword(0,6,tmpline));  /* Nick!User@Host */
        strcpy(word1,OVgetnick(word2));        /* Nick */
#ifdef OGRE
	snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] high traffic mode: forced by %s%s%s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else		  
	snprintf(tmpbuf,sizeof(tmpbuf),"Entering high traffic mode: forced by %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Entering high-traffic mode",25)) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* High speed */
        strcpy(word2,OVgetword(0,7,tmpline));  /* Low speed */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"high-traffic mode: %s%s%s ò %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] high traffic mode %s%s%s > %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Entering high traffic mode: %s%s%s > %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Resuming standard operation: Forced by",37)) {
        strcpy(word2,OVgetword(0,6,tmpline));  /* Nick!User@Host */
        strcpy(word1,OVgetnick(word2));        /* Nick */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] standard operation: forced by %s%s%s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Resuming standard operation: forced by %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Resuming standard operation",26)) {
        strcpy(word1,OVgetword(0,4,tmpline));  /* Low speed */
        strcpy(word2,OVgetword(0,6,tmpline));  /* High speed */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"standard-traffic mode: %s%s%s %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] standard traffic mode: %s%s%s %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Entering standard traffic mode: %s%s%s  %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Client exiting",14)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
        if (strstr(tmpline+14," from ")) {
            /* ircd 2.9/2.10 */
            strcpy(word3,OVgetword(0,5,tmpline));  /* user */
	    snprintf(word2,sizeof(word2),"(%s@%s)",word3,
                    OVgetword(0,7,tmpline));       /* host */
            strcpy(word3,OVgetword(9,0));
        }
        else {
	    strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	    strcpy(word3,OVgetword(5,0,tmpline));  /* Reason */
        }
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"clnt/%sexit%s  %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %sexit%s] %s%s%s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Client %sexiting%s: %s%s%s %s %s",
		CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
#endif
    }
    else if (!strncmp(tmpline,"Client connecting on port",25)) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* port */
        if ((tmp=index(word1,':'))) *tmp='\0';
	strcpy(word2,OVgetword(0,6,tmpline));  /* Nick */
	strcpy(word3,OVgetword(0,7,tmpline));  /* user@host */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"clnt/%sconnect%s [p%s%s%s]  %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color5,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[   %sconnect%s] [p%s%s%s]  %s%s%s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Client %sconnecting%s on port %s%s%s: %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color5,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
#endif
    }
    else if (!strncmp(tmpline,"Client connecting",17)) {
        strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
        if (strstr(tmpline+17," from ")) {
            /* ircd 2.9/2.10 */
            strcpy(word3,OVgetword(0,5,tmpline));  /* user */
	    snprintf(word2,sizeof(word2),"(%s@%s)",word3,
                    OVgetword(0,7,tmpline));       /* host */
        }
        else strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"clnt/%sconnect%s  %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[   %sconnect%s] %s%s%s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Client %sconnecting%s: %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"LINKS requested by",18) ||
             !strncmp(tmpline,"LINKS '",7)) {
        if (*(tmpline+6)=='\'') {
	    snprintf(word3,sizeof(word3)," %s",OVgetword(0,2,tmpline));  /* filter */
            strcpy(word1,OVgetword(0,5,tmpline));  /* Nick */
	    strcpy(word2,OVgetword(0,6,tmpline));  /* user@host */
        }
        else {
            strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	    strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
            *word3='\0';
        }
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"%slinks%s%s from %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],word3,
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %slinks%s]%s from %s%s%s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],word3,
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"%sLinks%s%s request from %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],word3,
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"TRACE requested by",18)) {
        strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"%strace%s from %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %strace%s] from %s%s%s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"%sTrace%s request from %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Rejecting vlad",14)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bot nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* user@host */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sreject%s] vlad/joh/com bot: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Rejecting vlad/joh/com bot: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Rejecting clonebot",17)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Bot nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
#ifdef OGRE
	snprintf(tmpbuf,sizeof(tmpbuf),"[    %sreject%s] clonebot: %s%s%s %s",
		CmdsColors[COLOV].color2,Colors[COLOFF],
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Rejecting clonebot: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Identd response differs",22)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* Attemtped IRCUSER */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %sfault%s] fake identd: %s%s%s %c%s%c",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                bold,word2,bold);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Fault identd response for %s%s%s %c%s%c",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                bold,word2,bold);
#endif
    }
    else if (!strncmp(tmpline,"Kill line active for",19)) {
	strcpy(word1,OVgetword(0,5,tmpline));  /* Banned client */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"k-line active: %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %skline%s] active: %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Active K-Line for %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"K-lined ",8)) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Banned client */
        if (*word1) {
            tmp=word1+strlen(word1)-1;
            if (*tmp=='.') *tmp='\0';
        }
	strcpy(word2,OVgetword(4,0,tmpline));  /* Banned client */
        if (*word2) snprintf(word3,sizeof(word3)," (%s)",word2);
        else *word3='\0';
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"k-line active: %s%s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word3);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %skline%s] active: %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Active K-line for %s%s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word3);
#endif
    }
    else if (!strncmp(tmpline,"I-line is full for",18)) {
	strcpy(word1,OVgetword(0,5,tmpline));  /* nick[user@host] */
	strcpy(word2,word1);
        /* strip [ from nick */
        if ((tmp=index(word1,'['))) *tmp='\0';
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"i-line full: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %siline%s] full: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"I-Line full for %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Quarantined nick",16)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* nick */
	strcpy(word2,OVgetword(0,6,tmpline));  /* user@host */
        tmp=word1;
        /* strip [ from nick */
        if (*tmp=='[') {
            if ((tmp=rindex(word1,']'))) *tmp='\0';
            tmp=word1+1;
        }
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"quarantined nick: %s%s%s %s",
                CmdsColors[COLOV].color1,tmp,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %snick%s] quarantined: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,tmp,Colors[COLOFF],OVuh(word2));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Quarantined nick %s%s%s %s",
                CmdsColors[COLOV].color1,tmp,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"X-line Warning",14)) {
	strcpy(word1,OVgetword(0,9,tmpline));  /* nick[user@host] */
	strcpy(word2,word1);
        /* strip [ from nick */
        if ((tmp=index(word1,'['))) *tmp='\0';
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"spambot warning 1: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] spambot warning 1: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Spambot warning 1 for %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!my_strnicmp(tmpline,"stats ",6)) {
        strcpy(word1,OVgetword(0,2,tmpline));  /* Stat type */
        if (strstr(tmpline+6," from ")) {
            /* ircd 2.9/2.10 */
	    strcpy(word2,OVgetword(0,4,tmpline));  /* Nick */
            strcpy(word4,OVgetword(0,6,tmpline));  /* user */
	    snprintf(word3,sizeof(word3),"(%s@%s)",word4,
                    OVgetword(0,8,tmpline));       /* host */
            *word4='\0';
        }
        else {
	    strcpy(word2,OVgetword(0,5,tmpline));  /* Nick */
	    strcpy(word3,OVgetword(0,6,tmpline));  /* user@host */
	    strcpy(word4,OVgetword(0,7,tmpline));  /* Server */
        }
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"stats %s%s%s from %s%s%s %s %s%s%s",
		CmdsColors[COLOV].color4,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3),
                CmdsColors[COLOV].color3,word4,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %sstats%s] %s%s%s from %s%s%s %s %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3),
                CmdsColors[COLOV].color1,word4,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Stats %s%s%s request from %s%s%s %s %s%s%s",
		CmdsColors[COLOV].color4,word1,Colors[COLOFF],
		CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3),
                CmdsColors[COLOV].color3,word4,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Nick change collision",21)) {
	strcpy(word1,OVgetword(0,5,tmpline));
	strcpy(word2,OVgetword(0,7,tmpline));
	strcpy(word3,OVgetword(8,0,tmpline));
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"nick collide: %s%s%s [%s] %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[ %scollision%s] %s%s%s [%s] %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2,word3);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Nick change collision: [%s%s%s] [%s] %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#endif
    }
    else if (!strncmp(tmpline,"Nick collision on",17)) {
	strcpy(word1,OVgetword(4,0,tmpline));
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"nick collide: %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[ %scollision%s] %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Nick collision: %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Fake ",5)) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Nick/Server */
	strcpy(word2,OVgetword(0,4,tmpline));  /* channel */
	strcpy(word3,OVgetword(5,0,tmpline));  /* fake modes */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"fake mode in %s%s%s: \"%s%s%s\" by %s",
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],
                CmdsColors[COLOV].color4,word3,Colors[COLOFF],word1);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %sfake%s] mode in %s%s%s: \"%s%s%s\" by %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word3,Colors[COLOFF],word1);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Fake mode: \"%s%s%s\" in %s%s%s by %s",
                CmdsColors[COLOV].color4,word3,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],word1);
#endif
    }
    else if (!strncmp(tmpline,"Too many connect",16)) strcpy(tmpbuf,line);
    else if (!strncmp(tmpline,"Possible bot",12)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Botnick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] possible bot: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Possible Bot: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Link with",9)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	strcpy(word2,OVgetword(4,0,tmpline));  /* Connect info */
        if ((tmp=index(word2,'('))) {
            *word2='\0';
        }
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"link/%sconnect%s  %s%s%s%s%s%s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                *word2?" (":"",*word2?word2:"",*word2?")":"");
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sconnect%s %s%s%s%s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                *word2?"(":"",*word2?word2:"",*word2?")":"");
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Link: connected to %s%s%s%s%s%s",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                *word2?" (":"",*word2?word2:"",*word2?")":"");
#endif
    }
    else if (!strncmp(tmpline,"Write error to",14)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Server */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"link/%sw.error%s  %s%s%s (closing)",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sw.error%s %s%s%s (closing)",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Link: Write error to %s%s%s - closing.",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(line,"Message",7)) strcpy(tmpbuf,line);
    else if (!strncmp(tmpline,"Received SQUIT",14)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
	strcpy(word2,OVgetword(0,5,tmpline));  /* SQUITer */
        strcpy(word3,OVgetword(6,0,tmpline));  /* Reason */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"link/%ssquit%s  %s%s%s from %s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %ssquit%s %s%s%s from %s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2,word3);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Link: %s%s%s recieved %sSQUIT%s from %s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color4,Colors[COLOFF],word2,word3);
#endif
    }
    else if (!strncmp(tmpline,"Received KILL message for",25)) {
        strcpy(word1,OVgetword(0,5,tmpline));  /* nick */
        if (strlen(word1) && word1[strlen(word1)-1]=='.')
            word1[strlen(word1)-1]='\0';
        strcpy(word2,OVgetword(0,7,tmpline));  /* killer  */
        /* check for server kill first */
        if (index(word2,'.')) {
            strcpy(word3,OVgetword(9,0,tmpline));  /* path  */
#ifdef OGRE 
            snprintf(tmpbuf,sizeof(tmpbuf),"[      %skill%s] %sserver%s: %s%s%s from %s%s%s (%s)",
                    CmdsColors[COLOV].color2,Colors[COLOFF],
                    CmdsColors[COLOV].color6,Colors[COLOFF],
                    CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                    CmdsColors[COLOV].color5,word2,Colors[COLOFF],word3);
#else
            snprintf(tmpbuf,sizeof(tmpbuf),"Server kill received for %s%s%s from %s%s%s (%s)",
                    CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                    CmdsColors[COLOV].color2,word2,Colors[COLOFF],word3);
#endif
        }
        else {
            int foreign=0;
            char *tmp=word3;
            char *tmpuh=NULL;
            char *userhost=word3;

            strcpy(word3,OVgetword(0,9,tmpline));  /* path  */
            strcpy(word4,OVgetword(10,0,tmpline)); /* reason  */
            /* check for foreing kill (more than one !) */
            while (*tmp && (tmp=index(tmp,'!'))) {
                foreign++;
                *tmp++='\0';
                userhost=tmpuh;
                tmpuh=tmp;
            }
            if (foreign>1) {
#ifdef OGRE
                snprintf(tmpbuf,sizeof(tmpbuf),"[      %skill%s] %sforeign%s: %s%s%s from %s%s%s %s",
                        CmdsColors[COLOV].color2,Colors[COLOFF],
                        CmdsColors[COLOV].color6,Colors[COLOFF],
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
#else
                snprintf(tmpbuf,sizeof(tmpbuf),"Foreign kill received for %s%s%s from %s%s%s %s",
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
#endif
            }
            else {
                int locop=strstr(tmpuh?tmpuh:"","(L")?1:0;

#ifdef OGRE
                snprintf(tmpbuf,sizeof(tmpbuf),"[      %skill%s] %slocal%s: %s%s%s from %s%s%s%s %s",
                        CmdsColors[COLOV].color2,Colors[COLOFF],
                        CmdsColors[COLOV].color6,Colors[COLOFF],
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        locop?"local operator ":"",
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
#else
                snprintf(tmpbuf,sizeof(tmpbuf),"Local kill received for %s%s%s from %s%s%s%s %s",
                        CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                        locop?"local operator ":"",
                        CmdsColors[COLOV].color1,word2,Colors[COLOFF],word4);
#endif
            }
        }
    }
    else if (!strncmp(tmpline,"Received SERVER",15)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* server */
	strcpy(word2,OVgetword(0,5,tmpline));  /* remote */
	strcpy(word4,OVgetword(7,0,tmpline));  /* IP & server name */
        if (*word4) snprintf(word3,sizeof(word3),"(%s",word4);
        else *word3='\0';
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"link/%sserver%s  %s%s%s from %s %s %s",
                CmdsColors[COLOV].color3,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sserver%s %s%s%s from %s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2,word3);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Link: %s%s%s recieved %sSERVER%s from %s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color4,Colors[COLOFF],word2,word3);
#endif
    }
    else if (!strncmp(tmpline,"Possible Eggdrop:",17)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* BotNick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* user@host */
	strcpy(word3,OVgetword(0,5,tmpline));  /* b-line notice */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[   %sclient%s] possible eggdrop: %s%s%s %s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Possible eggdrop: %s%s%s %s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
#endif
    }
    else if (!strncmp(tmpline,"Nick change",11)) {
        if (strstr(tmpline+6," from ")) {
            /* ircd 2.9/2.10 */
	    strcpy(word1,OVgetword(0,3,tmpline));  /* oldnick */
            strcpy(word2,OVgetword(0,5,tmpline));  /* newnick */
            strcpy(word4,OVgetword(0,7,tmpline));  /* user */
	    snprintf(word3,sizeof(word3),"(%s@%s)",word4,
                    OVgetword(0,9,tmpline));       /* host */
        }
        else {
            strcpy(word1,OVgetword(0,4,tmpline));  /* oldnick */
	    strcpy(word2,OVgetword(0,6,tmpline));  /* newnick */
            strcpy(word3,OVgetword(0,7,tmpline));  /* user@host */
        }
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %snick%s] %s%s%s to %s%s%s %s",
                CmdsColors[COLOV].color1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Nick change: %s%s%s to %s%s%s %s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],OVuh(word3));
#endif
    }
    else if (!strncmp(tmpline,"added K-Line",12)) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(5,0,tmpline));  /* K-line */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %skline%s] added by %s%s%s for %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"K-Line added by %s%s%s - %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
#endif
    }
    else if (!strncmp(tmpline,"Added K-Line [",14)) return;
    else if (!strncmp(tmpline,"Bogus server name",17)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bogus name */
	strcpy(word2,OVgetword(0,6,tmpline));  /* Nick */
#ifdef OGRE	
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sbogus%s:  %s%s%s from %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Bogus server name %s%s%s from %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"No response from",16)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Server */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s]  %serror%s: %s%s%s is not responding",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Link error: %s%s%s is not responding",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"IP# Mismatch",12)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Real IP */
	strcpy(word2,OVgetword(0,5,tmpline));  /* Mismatched IP */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[  %smismatch%s] %s%s%s != %s%s%s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"IP mismatch detected: %s%s%s != %s%s%s",
		CmdsColors[COLOV].color2,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word2,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Unauthorized connection from",28)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick!user@host */
#ifdef OGRE	
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] unauthorized: %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Unauthorized connect from %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Invalid username",16)) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,4,tmpline));  /* username */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] invalid username: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Invalid username: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Cannot accept connect",21)) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Nick? */
	strcpy(word2,OVgetword(5,0,tmpline));  /* Stuff? */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sunlinkable%s: %s%s%s [%s]",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Unlinkable connection: %s%s%s [%s]",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
#endif
    }
    else if (!strncmp(tmpline,"Lost connection to",18)) {
	strcpy(word1,OVgetword(4,0,tmpline));  /* Server */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %slost connection%s: %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Link: lost connection to %s%s%s",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (!my_strnicmp(tmpline,"failed oper",11)) {
        strcpy(word1,OVgetword(0,6,tmpline));  /* n!u@h for ircd 2.9/2.10 */
        if ((tmp=index(word1,'!')) && index(word1,'@')) {
            *tmp++='\0';
            snprintf(word2,sizeof(word2),"(%s)",tmp);
            strcpy(word3,OVgetword(0,3));
        }
        else {
            strcpy(word1,OVgetword(0,5,tmpline));  /* Nick */
            strcpy(word2,OVgetword(0,6,tmpline));  /* user@host */
            strcpy(word3,OVgetword(0,7,tmpline));  /* OPER nick */
        }
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %soper%s] FAILED: %s%s%s %s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color4,word1,Colors[COLOFF],OVuh(word2),word3);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Failed OPER attempt: %s%s%s %s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),word3);
#endif
    }
    else if (!strncmp(tmpline,"Global -- Failed OPER attempt",29)) {
        strcpy(word1,OVgetword(0,7,tmpline));  /* Nick */
	strcpy(word2,OVgetword(0,8,tmpline));  /* user@host */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %soper%s] FAILED: %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color4,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Failed OPER attempt: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (!strncmp(tmpline,"Sending SQUIT ",14) ||
             !strncmp(tmpline,"Sending SERVER ",15)) {
        strcpy(word1,OVgetword(0,2,tmpline));
        strcpy(word2,OVgetword(0,3,tmpline));
        strcpy(word3,OVgetword(4,0,tmpline));
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] sending %s%s%s %s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2,
                !strncmp(tmpline,"Sending SQUIT ",14)?word3:"");
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Sending %s%s%s %s %s",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,
                !strncmp(tmpline,"Sending SQUIT ",14)?word3:"");
#endif
    }
    else if (!strncmp(tmpline,"Rejecting connection from ",26)) {
        strcpy(word1,OVgetword(0,4,tmpline));
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sreject%s] connection: %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Rejecting connection from %s%s%s",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"Remote CONNECT",14)) {
        strcpy(word1,OVgetword(0,3,tmpline)); /* target */
        strcpy(word2,OVgetword(0,4,tmpline)); /* port */
        strcpy(word3,OVgetword(0,6,tmpline)); /* nick */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sconnect%s %s%s%s %s <- %s from %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color5,word1,Colors[COLOFF],word2,origfrom,
                CmdsColors[COLOV].color2,word3,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Remote connect %s%s%s %s <- %s from %s%s%s",
                CmdsColors[COLOV].color5,word1,Colors[COLOFF],word2,origfrom,
                CmdsColors[COLOV].color2,word3,Colors[COLOFF]);
#endif
    }
    else if (!strncmp(tmpline,"TS for ",7)) {
        strcpy(word1,OVgetword(0,3,tmpline)); /* channel */
        strcpy(word2,OVgetword(0,6,tmpline)); /* TS1 */
        strcpy(word3,OVgetword(0,8,tmpline)); /* TS2 */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %schannel%s] %sTS%s %s%s%s %s -> %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2,word3);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"%sTS%s for %s%s%s changed %s -> %s",
                CmdsColors[COLOV].color1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2,word3);
#endif
    }
    else if (strstr(tmpline,"whois on you") ||
             strstr(tmpline,"WHOIS on YOU")) {
        strcpy(word1,OVgetword(0,1,tmpline));  /* nick */
        strcpy(word2,OVgetword(0,2,tmpline));  /* user@host */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %swhois%s] on you by %s%s%s %s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s %s is doing a %sWhois%s on you.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color4,Colors[COLOFF]);
#endif
    }
    else if (strstr(tmpline,"Flooder") && strstr(tmpline,"target")) {
        strcpy(word1,OVgetword(0,2,tmpline)); /* nick */
        strcpy(word2,OVgetword(0,3,tmpline)); /* user@host */
        strcpy(word3,OVgetword(0,5,tmpline)); /* Server */
        strcpy(word4,OVgetword(0,7,tmpline)); /* Channel */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"clnt/%sflood%s %s%s%s -> %s%s%s [%s]",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word4,Colors[COLOFF],word3);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] flooder: %s%s%s %s -> %s%s%s [%s]",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color1,word4,Colors[COLOFF],word3);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Flooder %s%s%s %s -> %s%s%s [%s]",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color2,word4,Colors[COLOFF],word3);
#endif
    }
    else if (strstr(tmpline,"is now operator")) {
        int opertype;
        char *operstr;

	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(0,2,tmpline));  /* user@host */
        strcpy(word3,OVgetword(0,6,tmpline));  /* o/O */
        tmp=word3;
        if (*tmp) tmp++;
        if (get_server_version(from_server)==Server2_9 || get_server_version(from_server)==Server2_10) {
            opertype=*tmp?(*tmp=='o'?2:1):0;
        }
        else {
            opertype=*tmp?(*tmp=='O'?2:1):0;
        }
        operstr=opertype?(opertype==2?"global":"local"):"";
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s %s is an IRC warrior %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                *operstr?"(":"",operstr,operstr?")":"");
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %soper%s] %s%s%s %s is now a %s oper",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                operstr);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s %s is now %s IRC Operator.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                operstr);
#endif
    }
    else if (strstr(tmpline,"is rehashing Server config")) {
        strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
#ifdef CELECOSM
        snprintf(tmpbuf,sizeof(tmpbuf),"config rehash by %s%s%s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#elif defined(OGRE)
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %srehash%s] by %s%s%s",
                CmdsColors[COLOV].color4,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s is rehashing the server config file.",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#endif
    }
    else if (strstr(tmpline,"tried to msg")) {
        strcpy(word1,OVgetword(0,2,tmpline));  /* nick */
        strcpy(word2,OVgetword(0,3,tmpline));  /* user@host */
        strcpy(word3,OVgetword(0,7,tmpline));  /* number */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] %s%s%s %s tried to message %s%s%s users",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color1,word3,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"User %s%s%s %s tried to message %s%s%s users",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2),
                CmdsColors[COLOV].color2,word3,Colors[COLOFF]);
#endif
    }
    else if ((strstr(tmpline,"Possible")) && (strstr(tmpline,"bot"))) {
	strcpy(word1,OVgetword(0,4,tmpline));  /* Bot Nick */
	strcpy(word2,OVgetword(0,5,tmpline));  /* User@Host */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[    %sclient%s] possible bot: %s%s%s %s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Possible IrcBot: %s%s%s %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],OVuh(word2));
#endif
    }
    else if (strstr(tmpline,"ERROR :from")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Error Source */
	strcpy(word2,OVgetword(0,7,tmpline));  /* ? */
	strcpy(word3,OVgetword(8,0,tmpline));  /* Error */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %serror%s: %s%s%s - close link %s%s%s %s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF],word3);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Error: %s%s%s - close link %s%s%s %s",
		CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color2,word2,Colors[COLOFF],word3);
#endif
    }
    else if (strstr(tmpline,"closed the connection")) {
	strcpy(word1,OVgetword(0,2,tmpline));  /* Server */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %slink error%s: %s%s%s closed the connection",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Link Error: %s%s%s closed the connection.",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if ((strstr(tmpline,"Connection to")) && (strstr(tmpline,"activated"))) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Server */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sconnecting%s to %s%s%s",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Link: Connecting to %s%s%s",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF]);
#endif
    }
    else if (strstr(tmpline,"connect failure:")) {
	strcpy(word1,OVgetword(0,3,tmpline));  /* Failed Server */
	strcpy(word2,OVgetword(4,0,tmpline));  /* Reason */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[      %slink%s] %sfailed connect%s from %s%s%s [%s]",
                CmdsColors[COLOV].color5,Colors[COLOFF],
                CmdsColors[COLOV].color6,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word4);
#else
	snprintf(tmpbuf,sizeof(tmpbuf),"Failed connect from %s%s%s [%s]",
                CmdsColors[COLOV].color2,word1,Colors[COLOFF],word2);
#endif
    }
    else if ((tmp=strstr(tmpline," added a ")) && strstr(tmp,"kline")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(6,0,tmpline));  /* K-line */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %skline%s] added by %s%s%s for %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"K-Line added by %s%s%s - %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
#endif
    }
    else if (strstr(tmpline," added K-Line ")) {
	strcpy(word1,OVgetword(0,1,tmpline));  /* Nick */
        strcpy(word2,OVgetword(5,0,tmpline));  /* K-line */
#ifdef OGRE
        snprintf(tmpbuf,sizeof(tmpbuf),"[     %skline%s] added by %s%s%s for %s%s%s",
                CmdsColors[COLOV].color2,Colors[COLOFF],
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],
                CmdsColors[COLOV].color1,word2,Colors[COLOFF]);
#else			   
        snprintf(tmpbuf,sizeof(tmpbuf),"K-Line added by %s%s%s - %s",
                CmdsColors[COLOV].color1,word1,Colors[COLOFF],word2);
#endif
    }
    servername=server_list[from_server].itsname;
    if (!servername) servername=server_list[from_server].name;
    if (OVTS) curtime=update_clock(0,0,GET_TIME);
    else curtime=empty_string;
    if (from)
        put_it("[%s%s%s%s%s%s%s] Opermsg from %s%s%s: %s",
                CmdsColors[COLOV].color1,curtime,Colors[COLOFF],
                OVTS?"|":empty_string,
                CmdsColors[COLOV].color6,OVsvdmn(servername),Colors[COLOFF],
                CmdsColors[COLOV].color1,from,Colors[COLOFF],tmpbuf);
    else put_it("[%s%s%s%s%s%s%s] %s",
                CmdsColors[COLOV].color1,curtime,Colors[COLOFF],
                OVTS?"|":empty_string,
                CmdsColors[COLOV].color6,OVsvdmn(servername),Colors[COLOFF],tmpbuf);
}

void OperVisionReinit(void) {
    int ovwinref;
    int curwinref;
    unsigned int display;
    char tmpbuf[mybufsize/4+1];
    Window *ovwin;

    /* turn on additional user modes */
    CreateMode(tmpbuf,mybufsize/4);
    send_to_server("MODE %s :+%s",get_server_nickname(from_server),tmpbuf);
    curwinref=curr_scr_win->refnum;
    ovwin=get_window_by_name("OV");
    /* if we can't locate OperV window silently ignore */
    if (!ovwin) return;
    ovwinref=ovwin->refnum;
    snprintf(tmpbuf,sizeof(tmpbuf),"REFNUM %d LEVEL OPNOTE,SNOTE,WALLOP REFNUM %d",
            ovwinref,curwinref);
    display=window_display;
    window_display=0;
    windowcmd(NULL,tmpbuf,NULL);
    window_display=display;
}
#endif
