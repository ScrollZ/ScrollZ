/*
 * cdcc.c xdcc in C.  This file tries to recreate the script xdcc in C,
 * this file contains all the functions needed to implement this.
 *
 * Written by Scott H Kilau
 *
 * Heavily modified by Flier
 *
 * Copyright(c) 1995
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 *
 * $Id: cdcc.c,v 1.46 2002-03-03 11:03:23 f Exp $
 */

/* uncomment this if compiling on BSD */
/*#include <db.h>*/
#include "irc.h"
#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "list.h"
#include "server.h"
#include "vars.h"
#include "ircaux.h"
#include "input.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "edit.h"
#include "dcc.h"
#include "parse.h"
#if !defined(CELEHOOK) && !defined(LITE)
#include "hook.h"
#endif
#include "cdcc.h"
#include "myvars.h"

#define LIST_DELAY 4

void Cdcc _((char *, char *, char *));
void CheckAutoGet _((char *, char *, char *, char *));
void CheckCdccTimers _((void));
void RemoveFromQueue _((int));
int  TotalSendDcc _((void));
int  TotalQueue _((void));
void helpmcommand _((char *));
int  matchmcommand _((char *, int));
static void formatstats _((char *, int));
static void sendmcommand _((char *));
static void send2mcommand _((char *, char *));
static void send3mcommand _((char *, char *));
static void resendmcommand _((char *));
static void resend2mcommand _((char *, char *));
static void resend3mcommand _((char *, char *));
static void getmcommand _((char *));
static void get2mcommand _((char *, char *));
static void doffermcommand _((char *));
static void doffer2mcommand _((char *, char *));
static void offermcommand _((char *));
static void offer2mcommand _((char *, char *));
static void offer3mcommand _((char *, char *));
static void offer4mcommand _((char *, char *));
#ifdef EXTRAS
static void renamepackmcommand _((char *));
#endif
static void listmcommand _((char *));
static void plistmcommand _((char *));
static void noticemcommand _((char *));
static void listcommand _((char *, char *));
static void helpcommand _((char *, char *));
static void sendcommand _((char *, char *, int, int));
static void versioncommand _((char *, char *));
static void queuecommand _((char *, char *));
#ifndef LITE
static void psendmcommand _((char *));
static void psend2mcommand _((char *, char *));
static void psend3mcommand _((char *, char *));
#endif
static void autogetmcommand _((char *));
static void securemcommand _((char *));
static void closemcommand _((char *));
static void close2mcommand _((char *, char *));
#ifdef EXTRA_STUFF
static void emcommand _((char *));
static void mmcommand _((char *));
#endif
/****** Coded by Zakath ******/
#ifndef LITE
static void requestmcommand _((char *));
#endif
/*****************************/
static void idlemcommand _((char *));
static void limitmcommand _((char *));
static void channelsmcommand _((char *));
static void ptimemcommand _((char *));
static void ntimemcommand _((char *));
static void longstatusmcommand _((char *));
static void overwritemcommand _((char *));
static void uldirmcommand _((char *));
static void dldirmcommand _((char *));
static void showdccsmcommand _((int));
#ifndef LITE
static void loadmcommand _((char *));
static void savemcommand _((char *));
#endif
static void statusmcommand _((char *));
static void statsmcommand _((char *));
static void warningmcommand _((char *));
static void verbosemcommand _((char *));
void queuemcommand _((char *));
static void GetDir _((char *));
static void CleanList _((void));
static void AddFileToList _((char *, char *, int));
static int  AddFiles2List _((char *));
static void AddToOfferList _((char *, char *));
static void ShowPacks _((char *));
static void DeleteSend _((void));
static int  AddToQueue _((Files *, char *, int));
static int  SeedFiles _((char *, int));
static int  GetSize _((char *, char *));
static int  compar _((struct dirent **, struct dirent **));
static int  selectent _((struct dirent *));

extern void AwaySave _((char *, int));
extern void PrintSetting _((char *, char *, char *, char *));
extern void NoWindowChannel _((void));
extern void PrintUsage _((char *));
extern void NumberCommand _((char *, char *, char *));
extern void OnOffCommand _((char *, char *, char *));
extern int  CheckChannel _((char *, char *));
extern char *OpenCreateFile _((char *, int));
extern struct friends *CheckUsers _((char *, char *));
extern void ColorUserHost _((char *, char *, char *, int));
extern int  CheckServer _((int));

extern void dcc_close _((char *));
extern void dcc_getfile _((char *));
#ifdef BROKEN_MIRC_RESUME
extern void dcc_getfile_resume _((char *));
#endif /* BROKEN_MIRC_RESUME */
extern void dcc_regetfile _((char *));
extern void dcc_filesend _((char *));
extern void dcc_resend _((char *));
extern void timercmd _((char *, char *, char *));

static Packs *packs=(Packs *) 0;
static Files *files=(Files *) 0;
static FileQueue *queuelist=(FileQueue *) 0;
static struct dirent **CdccFileNames=NULL;
static struct stat CdccStatBuf;
static int CdccEntries;
static char *CdccString="[S+Z]";
static time_t LastIdleCheck=0;
static time_t LastList=0;
/****** Coded by Zakath ******/
#ifndef LITE
static int  CdccReqTog=0;
static char *CdccRequest=(char *) 0;
#endif
/*****************************/

static CdccCom CdccCommands[]={
    { "HELP",        helpmcommand },
    { "GET",         getmcommand },
    { "SEND",        sendmcommand },
    { "CLOSE",       closemcommand },
    { "RESEND",      resendmcommand },
    { "OFFER",       offermcommand },
    { "DOFFER",      doffermcommand },
    { "PLIST",       plistmcommand },
    { "LIST",        listmcommand },
    { "NOTICE",      noticemcommand },
#ifdef EXTRAS
    { "RENPACK",     renamepackmcommand },
#endif
/****** Coded by Zakath ******/
#ifndef LITE
    { "REQUEST",     requestmcommand },
#endif
/*****************************/
    { "QUEUE",       queuemcommand },
#ifndef LITE
    { "LOAD",        loadmcommand },
    { "SAVE",        savemcommand },
#endif
    { "LIMIT",       limitmcommand },
    { "CHANNELS",    channelsmcommand },
    { "PTIME",       ptimemcommand },
    { "NTIME",       ntimemcommand },
    { "LONGSTATUS",  longstatusmcommand },
    { "OVERWRITE",   overwritemcommand },
#ifndef LITE
    { "PSEND",       psendmcommand },
#endif
    { "IDLE",        idlemcommand },
    { "AUTOGET",     autogetmcommand },
    { "SECURE",      securemcommand },
    { "STATUS",      statusmcommand },
    { "STATS",       statsmcommand },
    { "VERBOSE",     verbosemcommand },
    { "WARNING",     warningmcommand },
    { "ULDIR",       uldirmcommand },
    { "DLDIR",       dldirmcommand },
#ifdef EXTRA_STUFF
    { "E",           emcommand },
    { "M",           mmcommand },
#endif
    { NULL,          NULL }
};

extern char *dcc_types[];

/************************************************************************
 * Cdcc: parse cdcc command line, and send off to correct function      *
 ************************************************************************/
void Cdcc(command, args, subargs)
char *command;
char *args;
char *subargs;
{
    char *word=(char *) 0;
    char *tmpstr;
    int  len=0;
    int  found=0;
    int  i;
    int  com=0;

    if (!(word=new_next_arg(args,&args))) {
        showdccsmcommand(15);
        return;
    }
    len=strlen(word);
    for (i=0;CdccCommands[i].command && CdccCommands[i].function;i++)
        if (!my_strnicmp(CdccCommands[i].command,word,len)) {
            found++;
            if (found>1) break;
            if (!com) com=i;
        }
    if (found>1) {
        for (tmpstr=word;*tmpstr;tmpstr++)
            if (*tmpstr>='a' && *tmpstr<='z') *tmpstr-=' ';
        say("CDCC %s is ambiguous",word);
        return;
    }
    if (found) CdccCommands[com].function(args);
    else say("Try /CDCC HELP");
}

/*********************************************************************
 * Gives user help                                                   *
 *********************************************************************/
void helpmcommand(line)
char *line;
{
    PrintUsage("CDCC command where command is one of :");
    say("AUTOGET  CHAN     CLOSE   DLDIR    DOFFER  GET     IDLE   LIMIT   LIST");
    say("LOAD     LONGST   OVERWR  NOTICE   OFFER   PLIST   PTIME  NTIME   PSEND");
    say("QUEUE    RENPACK  RESEND  REQUEST  SAVE    SECURE  SEND   STATUS  ULDIR");
    say("VERBOSE  WARNING");
    say("For more help on command do /HELP CDCC command");
}

/***********************************************************************
 * listmcommand: List packs                                            *
 ***********************************************************************/
static void listmcommand(line)
char *line;
{
    ShowPacks(line);
}

/***********************************************************************
 * Sets CDCC limit                                                     *
 ***********************************************************************/
static void limitmcommand(line)
char *line;
{
    int number;
    int isnumber=1;
    char *word;
    char *tmpstr;
    char tmpbuf1[mybufsize/16];
    char tmpbuf2[mybufsize/16];

    if ((word=new_next_arg(line,&line))) {
        for (tmpstr=word;*tmpstr;tmpstr++) isnumber&=isdigit(*tmpstr)?1:0;
        number=atoi(word);
        if (isnumber && number>-1) CdccLimit=number;
        else {
            PrintUsage("CDCC LIMIT limit [queue limit]");
            return;
        }
        if ((word=new_next_arg(line,&line))) {
            for (tmpstr=word;*tmpstr;tmpstr++) isnumber&=isdigit(*tmpstr)?1:0;
            number=atoi(word);
            if (isnumber && number>-1) CdccQueueLimit=number;
            else {
                PrintUsage("CDCC LIMIT limit [queue limit]");
                return;
            }
        }
    }
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%d",CdccLimit);
    if (CdccQueueLimit) snprintf(tmpbuf2,sizeof(tmpbuf2),"%d",CdccQueueLimit);
    else strcpy(tmpbuf2,"unlimited");
    PrintSetting("Cdcc limit",tmpbuf1,", queue limit is",tmpbuf2);
    RemoveFromQueue(0);
}

/***********************************************************************
 * Sets idle seconds before auto-close                                 *
 ***********************************************************************/
static void idlemcommand(line)
char *line;
{
    NumberCommand("IDLE",line,NULL);
}

/***********************************************************************
 * Sets CDCC autoget on or off                                         *
 ***********************************************************************/
static void autogetmcommand(line)
char *line;
{
    OnOffCommand("AUTOGET",line,NULL);
    update_all_status();
}

/***********************************************************************
 * Sets CDCC security on or off                                        *
 ***********************************************************************/
static void securemcommand(line)
char *line;
{
    OnOffCommand("SECURE",line,NULL);
    update_all_status();
}

#ifdef EXTRA_STUFF
/**********************************************************************
* Sets CDCC encode string                                             *
***********************************************************************/
static void emcommand(line)
char *line;
{
    if (*line) malloc_strcpy(&EString,line);
    PrintSetting("Cdcc E",EString,empty_string,empty_string);
}

/**********************************************************************
* Sets CDCC renaming on or off                                        *
***********************************************************************/
static void mmcommand(line)
char *line;
{
    OnOffCommand("M",line,NULL);
}
#endif

/**********************************************************************
* Sets CDCC channels for PLIST                                        *
***********************************************************************/
static void channelsmcommand(line)
char *line;
{
    char *channels;

    if ((channels=new_next_arg(line,&line))) malloc_strcpy(&CdccChannels,channels);
    if (CdccChannels) {
        if (!my_stricmp(CdccChannels,"current"))
            PrintSetting("Cdcc channels","current channel",empty_string,empty_string);
        else PrintSetting("Cdcc channels",CdccChannels,empty_string,empty_string);
    }
    else PrintSetting("Cdcc channels","none",empty_string,empty_string);
}

/***********************************************************************
 * Sets seconds to pass before repeating PLIST                         *
 ***********************************************************************/
static void ptimemcommand(line)
char *line;
{
    NumberCommand("PTIME",line,NULL);
}

/***********************************************************************
 * Sets seconds to pass before repeating NOTICE                        *
 ***********************************************************************/
static void ntimemcommand(line)
char *line;
{
    NumberCommand("NTIME",line,NULL);
}

/***********************************************************************
 * Sets CDCC long status bar on or off                                 *
 ***********************************************************************/
static void longstatusmcommand(line)
char *line;
{
    OnOffCommand("LONGSTATUS",line,NULL);
}

/***********************************************************************
 * Sets CDCC overwrite on or off                                       *
 ***********************************************************************/
static void overwritemcommand(line)
char *line;
{
    OnOffCommand("OVERWRITE",line,NULL);
}

/***********************************************************************
 * Sets showing DCC status on status bar on or off                     *
 ***********************************************************************/
static void statusmcommand(line)
char *line;
{
    OnOffCommand("STATUS",line,NULL);
    DCCDone=0;
    new_free(&CurrentDCC);
    update_all_status();
}

/***********************************************************************
 * Sets showing received/sent kB in PLIST on or off                    *
 ***********************************************************************/
static void statsmcommand(line)
char *line;
{
    OnOffCommand("STATS",line,NULL);
}

/***********************************************************************
 * Sets check on incoming DCCs on or off                               *
 ***********************************************************************/
static void warningmcommand(line)
char *line;
{
    OnOffCommand("WARNING",line,NULL);
}

/***********************************************************************
 * Sets verbose mode on or off                                         *
 ***********************************************************************/
static void verbosemcommand(line)
char *line;
{
    OnOffCommand("VERBOSE",line,NULL);
}

/***********************************************************************
 * Sets CDCC upload dir                                                *
 ***********************************************************************/
static void uldirmcommand(line)
char *line;
{
    char *newdir;
    char *fullname=(char *) 0;
    char tmpbuf[mybufsize/4];

    getcwd(tmpbuf,mybufsize);
    if ((newdir=new_next_arg(line,&line))) {
        fullname=expand_twiddle(newdir);
        if (!chdir(fullname)) malloc_strcpy(&CdccUlDir,fullname);
        else {
#ifdef WANTANSI
            say("%sError%s, can't cd into %s%s%s",
                CmdsColors[COLWARNING].color1,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,line,Colors[COLOFF]);
#else
            say("Error, can't cd into %s",line);
#endif
        }
        new_free(&fullname);
        chdir(tmpbuf);
    }
    if (CdccUlDir) PrintSetting("Cdcc upload dir",CdccUlDir,empty_string,empty_string);
    else PrintSetting("Cdcc upload dir",tmpbuf," - your current dir",empty_string);
}

/***********************************************************************
 * Sets CDCC download dir                                              *
 ***********************************************************************/
static void dldirmcommand(line)
char *line;
{
    char *newdir;
    char *fullname=(char *) 0;
    char tmpbuf[mybufsize/4];

    getcwd(tmpbuf,mybufsize);
    if ((newdir=new_next_arg(line,&line))) {
        fullname=expand_twiddle(newdir);
        if (!chdir(fullname)) malloc_strcpy(&CdccDlDir,fullname);
        else {
#ifdef WANTANSI
            say("%sError%s, can't cd into %s%s%s",
                CmdsColors[COLWARNING].color1,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,line,Colors[COLOFF]);
#else
            say("Error, can't cd into %s",line);
#endif
        }
        new_free(&fullname);
        chdir(tmpbuf);
    }
    if (CdccDlDir) PrintSetting("Cdcc download dir",CdccDlDir,empty_string,empty_string);
    else PrintSetting("Cdcc download dir",tmpbuf," - your current dir",empty_string);
}

/* returns DCC status as one character */
static char dccstatus(flags)
int flags;
{
    return(flags&DCC_OFFER?'O' :
           flags&DCC_DELETE?'D' :
           flags&DCC_ACTIVE?'A' :
           flags&DCC_WAIT?'W' :
#ifdef DCC_CNCT_PEND
           flags&DCC_CNCT_PEND?'C' :
#endif
           'U');
}

/***********************************************************************
 * showdccscommand: Lists all dccs                                     *
 ***********************************************************************/
static void showdccsmcommand(type)
int type;
{
    int   fills;
    int   count=1,i,j;
    int   highascii=get_int_var(HIGH_ASCII_VAR);
    long  completed;
    char  *filename;
    char  *format;
    char  tmpbuf1[mybufsize/4];
    char  tmpbuf2[mybufsize/4];
    char  tmpbuf3[mybufsize/4];
    char  tmpbuf4[mybufsize/32];
    float rate;
    time_t timenow;
    time_t etatime;
    unsigned flags;
    DCC_list *Client;

    if (LongStatus) format="%s %-6.6s %-9.9s %c %7.7s %s %s";
    else format="%-2s %-6.6s %-9.9s %c %7.7s %s %s %s";
#if !defined(CELEHOOK) && !defined(LITE)
    if (do_hook(DCC_LIST_HEADER,"%d",LongStatus))
#endif
    {
        if (LongStatus) say(format,"# ","Type","Nick",'S',"kb/s","   ETA ","Arguments");
        else say(format,"# ","Type","Nick",'S',"kb/s","Completed    ","     ETA ","Arguments");
    }
    for (Client=ClientList;Client;Client=Client->next) {
        flags=Client->flags;
        if (type==15 || (flags&type)==type) {
            *tmpbuf1='\0';
            *tmpbuf2='\0';
            timenow=time((time_t *) 0);
            completed=0;
            flags&=DCC_TYPES;
            if (flags==DCC_FILEREAD) completed=Client->bytes_read;
            else if (flags==DCC_FILEREGET)
                completed=Client->bytes_read+Client->resendoffset;
            else if (flags==DCC_FILEOFFER) completed=Client->bytes_sent;
            else if (flags==DCC_RESENDOFFER)
                completed=Client->bytes_sent+Client->resendoffset;
            filename=rindex(Client->description,'/');
            if (!filename) filename=Client->description;
            else filename++;
            if (completed && Client->filesize>0) {
                if (Client->filesize>=10000000) fills=completed/(Client->filesize/100);
                else fills=completed*100/Client->filesize;
            }
            else fills=0;
            if (completed && (Client->flags)&DCC_ACTIVE && timenow-Client->starttime>0) {
                if (flags==DCC_FILEREGET || flags==DCC_RESENDOFFER)
                    completed-=Client->resendoffset;
                rate=(float) (completed)/(float)(timenow-Client->starttime);
                if (rate>0.0 && completed<=Client->filesize)
                    etatime=(float) (((float) (Client->filesize)-(float) completed)/(float) rate);
                else etatime=0;
            }
            else {
                rate=0.0;
                etatime=0;
            }
#if !defined(CELEHOOK) && !defined(LITE)
            snprintf(tmpbuf1,sizeof(tmpbuf1),"%.2f",rate);
            if (do_hook(DCC_LIST,"%d %s %s %c %s %ld %s %d %ld",count,
                        dcc_types[flags],Client->user,dccstatus(Client->flags),
                        tmpbuf1,etatime,filename,fills,(long) Client->filesize))
#endif
            {
                if (completed) {
                    if (LongStatus) {
                        strmcpy(tmpbuf1,"\026    ",sizeof(tmpbuf1));
                        for (i=1;i<7;i++) strmcat(tmpbuf1,"          ",sizeof(tmpbuf1));
                        snprintf(tmpbuf2,sizeof(tmpbuf2),"%3d%%  (%ld of %ld bytes)",fills,completed,
                                (long) Client->filesize);
                        fills=(fills+1)*63/100;
                        for (i=0,j=0;i<=fills;i++)
                            if (tmpbuf2[j] && i>14) {
                                tmpbuf1[i+1]=tmpbuf2[j];
                                j++;
                            }
                            else tmpbuf1[i+1]=' ';
                        if (!i) i++;
                        if (tmpbuf1[i]!=' ') tmpbuf1[i+1]=tmpbuf1[i];
                        tmpbuf1[i]='\026';
                        i++;
                        if (i<16) i=16;
                        while (tmpbuf2[j]) {
                            tmpbuf1[i+1]=tmpbuf2[j];
                            i++;
                            j++;
                        }
                    }
                    else {
#define BARSIZE 10
                        char *fillchar=highascii?"²":" ";
                        char *emptychar=highascii?"°":"_";

                        snprintf(tmpbuf2,sizeof(tmpbuf2)," %3d%%",fills);
                        fills=((fills+3)*BARSIZE)/100;
                        if (highascii) *tmpbuf1='\0';
                        else strmcpy(tmpbuf1,"\026",sizeof(tmpbuf1));
                        for (i=0;i<fills;i++) strmcat(tmpbuf1,fillchar,sizeof(tmpbuf1));
                        if (!highascii) strmcat(tmpbuf1,"\026",sizeof(tmpbuf1));
                        for (;i<BARSIZE;i++) strmcat(tmpbuf1,emptychar,sizeof(tmpbuf1));
                        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
                    }
                    strmcpy(tmpbuf2,"    N/A",sizeof(tmpbuf2));
                    flags=Client->flags;
                    if (flags&DCC_ACTIVE && timenow-Client->starttime>0) {
                        flags&=DCC_TYPES;
                        if (flags==DCC_FILEREGET || flags==DCC_RESENDOFFER)
                            completed-=Client->resendoffset;
                        rate=(float) (completed)/(float)(timenow-Client->starttime);
                        snprintf(tmpbuf2,sizeof(tmpbuf2),"%6.2f",rate/1024.0);
                        if (rate>0.0 && completed<=Client->filesize) {
                            etatime=(float) (((float) (Client->filesize)-(float) completed)/(float) rate);
                            snprintf(tmpbuf3,sizeof(tmpbuf3),"%3ld:%02ld ",etatime/60,etatime%60);
                        }
                    }
                    else strmcpy(tmpbuf3,"        N/A ",sizeof(tmpbuf3));
                }
                else {
                    if (LongStatus) {
                        strmcpy(tmpbuf2,"  N/A",sizeof(tmpbuf2));
                        strmcpy(tmpbuf3,"   N/A ",sizeof(tmpbuf3));
                    }
                    else {
                        strmcpy(tmpbuf1,"      N/A ",sizeof(tmpbuf1));
                        strmcpy(tmpbuf2,"  N/A",sizeof(tmpbuf2));
                        strmcpy(tmpbuf3,"        N/A ",sizeof(tmpbuf3));
                    }
                }
                snprintf(tmpbuf4,sizeof(tmpbuf4),"%-2d",count);
                if (!(*tmpbuf2)) strmcpy(tmpbuf2,"      ",sizeof(tmpbuf2));
                if (LongStatus) {
                    flags=Client->flags;
                    say(format,tmpbuf4,dcc_types[flags&DCC_TYPES],Client->user,
                        dccstatus(flags),tmpbuf2,tmpbuf3,filename);
                    flags&=DCC_TYPES;
                    if (completed &&
                        (flags==DCC_FILEREAD  || flags==DCC_FILEREGET ||
                         flags==DCC_FILEOFFER || flags==DCC_RESENDOFFER))
                        say("[%s]",tmpbuf1);
                }
                else {
                    flags=Client->flags;
                    say(format,tmpbuf4,dcc_types[flags&DCC_TYPES],Client->user,
                        dccstatus(flags),tmpbuf2,tmpbuf1,tmpbuf3,filename);
                }
            }
            count++;
        }
    }
#if !defined(CELEHOOK) && !defined(LITE)
    do_hook(DCC_LIST_FOOTER,"%d",count-1);
#endif
}

/***********************************************************************
 * matchmcommand: Returns true if count matches line                   *
 ***********************************************************************/
int matchmcommand(origline,count)
char *origline;
int  count;
{
    int  startnum=0;
    int  endnum=0;
    register char *tmpstr;

    tmpstr=origline;
    if (!tmpstr) return(0);
    if (*tmpstr=='#') tmpstr++;
    if (*tmpstr=='*') return(1);
    while (*tmpstr) {
        startnum=0;
        endnum=0;
        if (*tmpstr=='-') {
            while (*tmpstr && !isdigit(*tmpstr)) tmpstr++;
            endnum=atoi(tmpstr);
            startnum=1;
            while (*tmpstr && isdigit(*tmpstr)) tmpstr++;
        }
        else {
            while (*tmpstr && !isdigit(*tmpstr)) tmpstr++;
            startnum=atoi(tmpstr);
            while (*tmpstr && isdigit(*tmpstr)) tmpstr++;
            if (*tmpstr=='-') {
                while (*tmpstr && !isdigit(*tmpstr)) tmpstr++;
                endnum=atoi(tmpstr);
                if (!endnum) endnum=1000;
                while (*tmpstr && isdigit(*tmpstr)) tmpstr++;
            }
        }
        if (count==startnum || (count>=startnum && count<=endnum)) return(1);
    }
    if (count==startnum || (count>=startnum && count<=endnum)) return(1);
    return(0);
}

/**********************************************************************
* closemcommand: Prompt User for type of dccs he wants to close       *
***********************************************************************/
static void closemcommand(line)
char *line;
{
    char *word;

    if (!ClientList) {
        say("No dccs to close");
        return;
    }
    if ((word=new_next_arg(line,&line))) close2mcommand(NULL,word);
    else {
        showdccsmcommand(15);
        add_wait_prompt("What to close (1-6,3 or *) ? ",
                        close2mcommand,line,WAIT_PROMPT_LINE);
    }
}

/**********************************************************************
* close2mcommand  This closes all user specified dccs                 *
***********************************************************************/
static void close2mcommand(blah,line)
char *blah;
char *line;
{
    int  count=0;
    int  packcount=0;
    char tmpbuf[mybufsize];
    DCC_list *Client;
    unsigned flags;

    if (line && *line) {
        for (Client=ClientList;Client;Client=Client->next) {
            packcount++;
            flags=Client->flags;
            if (matchmcommand(line,packcount)) {
                count++;
                snprintf(tmpbuf,sizeof(tmpbuf),"%s %s \"%s\"",dcc_types[flags&DCC_TYPES],Client->user,
                        Client->description);
                dcc_close(tmpbuf);
            }
        }
        say("Total of %d dccs closed",count);
        RemoveFromQueue(0);
    }
    else say("You must specify what to close");
}

/**********************************************************************
* doffermcommand: Prompt User for removing pack                       *
***********************************************************************/
static void doffermcommand(line)
char *line;
{
    char *word;

    if (packs) {
        if ((word=new_next_arg(line,&line))) doffer2mcommand(NULL,word);
        else {
            ShowPacks(NULL);
            add_wait_prompt("Doffer what pack (1-6,3 or * for all) ? ",
                            doffer2mcommand,line,WAIT_PROMPT_LINE);
        }
    }
    else say("No packs created");
}

/**********************************************************************
* doffer2mcommand  This parses offer file list                        *
*                  And puts in Files Linked List                      *
***********************************************************************/
static void doffer2mcommand(blah,line)
char *blah;
char *line;
{
    int   packcount=0;
    Files *tmp2;
    Files *next;
    Packs *tmp;
    Packs *tmp1;
    Packs *tmp3;
    char  *tmpstr=(char *) 0;

    if ((tmpstr=new_next_arg(line,&line))) {
        for (tmp=packs;tmp;tmp=tmp3) {
            tmp3=tmp->next;
            packcount++;
            if (matchmcommand(tmpstr,packcount)) {
                if ((tmp1=(Packs *) list_lookup((List **) &packs,tmp->description,
                                                !USE_WILDCARDS,REMOVE_FROM_LIST))!=NULL) {
                    say("Removing pack %-2d : %s",packcount,tmp1->description);
                    for (tmp2=tmp1->files;tmp2;tmp2=next) {
                        next=tmp2->next;
                        new_free(&(tmp2->file));
                        new_free(&(tmp2->path));
                        new_free(&tmp2);
                    }
                    tmp1->files=NULL;
                    new_free(&(tmp1->description));
                    new_free(&tmp1);
/****** Coded by Zakath ******/
                    CdccPackNum--;
                    update_all_status();
/*****************************/
                }
                else say("DOH ERROR !");
            }
        }
    }
    else say("You must specify what packs to remove");
}

/* Formats Received/Sent */
static void formatstats(buffer,cdccstuff)
char *buffer;
int cdccstuff;
{
    char byteschar='k';
    char tmpbuf[mybufsize/4];
    double mult=1.0;

    if (BytesReceived>1073741823.0) {
        byteschar='G';
        mult=1048576.0;
    }
    else if (BytesReceived>1048575.0) {
        byteschar='M';
        mult=1024.0;
    }
    if (cdccstuff) snprintf(tmpbuf,sizeof(tmpbuf),"%s Received %.2f %cB",CdccString,
                           BytesReceived/(1024.0*mult),byteschar);
    else snprintf(tmpbuf,sizeof(tmpbuf),"Received %.2f %cB",BytesReceived/(1024.0*mult),byteschar);
    if (BytesSent>1073741823.0) {
        byteschar='G';
        mult=1048576.0;
    }
    else if (BytesSent>1048575.0) {
        byteschar='M';
        mult=1024.0;
    }
    else {
        byteschar='k';
        mult=1.0;
    }
    sprintf(buffer,"%s  Sent %.2f %cB",tmpbuf,BytesSent/(1024.0*mult),byteschar);
}

/* Like CheckChannel but + before channel has special meaning */
static int CheckCdccChannel(channels,chanlist)
char *channels;
char *chanlist;
{
    int found=0;
    int minus=0;
    char *tmpstr1;
    char *tmpstr2;
    char *tmpchan1;
    char *tmpchan2;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize];

    if (!channels || !chanlist) return(0);
    tmpchan2=chanlist;
    while (*tmpchan2) {
        tmpstr2=tmpbuf2;
        while (*tmpchan2 && *tmpchan2!=',') *tmpstr2++=*tmpchan2++;
        *tmpstr2='\0';
        if (*tmpchan2==',') tmpchan2++;
        tmpchan1=channels;
        while (*tmpchan1) {
            tmpstr1=tmpbuf1;
            while (*tmpchan1 && *tmpchan1!=',') *tmpstr1++=*tmpchan1++;
            *tmpstr1='\0';
            if (*tmpbuf2=='-') {
                tmpstr2=&tmpbuf2[1];
                minus=1;
            }
            else if (*tmpbuf2=='+') {
                tmpstr2=&tmpbuf2[1];
                minus=2;
            }
            else {
                tmpstr2=tmpbuf2;
                minus=0;
            }
            if (*tmpbuf1=='-') {
                tmpstr1=&tmpbuf1[1];
                minus=1;
            }
            else tmpstr1=tmpbuf1;
            if (wild_match(tmpstr2,tmpstr1) || wild_match(tmpstr1,tmpstr2)) {
                if (minus==1) found=0;
                else if (minus==2) found=2;
                else found=1;
            }
            if (*tmpchan1==',') tmpchan1++;
        }
    }
    return(found);
}

/* Sends output from plist to server (when called by timer) */
static void sendplist(text)
char *text;
{
    char *msg;
    char *channel;

    channel=index(text,' ');
    channel++;
    msg=index(channel,' ');
    *msg++='\0';
    msg++;
    send_text(channel,msg,"PRIVMSG");
}

/* Does actual plist */
static void doplist(tmpchan,current,chan)
char *tmpchan;
int current;
ChannelList *chan;
{
    int count;
    int delay=-1;
    int number;
    int donlist;
    int oldserver;
    char byteschar;
    char *mynick=get_server_nickname(from_server);
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize/2];
    float mult;
    Packs *tmp;
    void (*func)()=(void(*)()) sendplist;

    for (tmp=packs,count=0;tmp;tmp=tmp->next) count++;
    oldserver=from_server;
    for (;chan;chan=chan->next) {
        donlist=0;
        if (current || (donlist=CheckCdccChannel(chan->channel,tmpchan))) {
            from_server=chan->server;
#if !defined(CELEHOOK) && !defined(LITE)
            if (do_hook(CDCC_PLIST_HEADER,"%s %d %s",chan->channel,count,mynick))
#endif
            {
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%s    %d PACK%s OFFERED   /CTCP %s CDCC SEND N for pack N",
                        CdccString,count,count==1?empty_string:"S",mynick);
                send_text(chan->channel,tmpbuf1,"PRIVMSG");
            }
            if (donlist==2) continue;
            number=1;
            for (tmp=packs;tmp;tmp=tmp->next) {
#if !defined(CELEHOOK) && !defined(LITE)
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%.2f",tmp->minspeed);
                if (do_hook(CDCC_PLIST,"%d %d %d %s %d %s",number,tmp->totalfiles,
                            tmp->totalbytes,tmpbuf2,tmp->gets,tmp->description))
#endif
                {
                    if (delay==-1) delay=0;
                    snprintf(tmpbuf2,sizeof(tmpbuf2),"%d/%dx",number,tmp->gets);
                    if (tmp->totalbytes>1048575) {
                        byteschar='M';
                        mult=1024.0;
                    }
                    else {
                        byteschar='k';
                        mult=1.0;
                    }
                    snprintf(tmpbuf1,sizeof(tmpbuf1),"-INV %d PRIVMSG %s :#%-6s %s  [%d file%s/%.2f %cB",
                            delay,chan->channel,tmpbuf2,tmp->description,tmp->totalfiles,
                            tmp->totalfiles==1?"":"s",
                            (float) (tmp->totalbytes)/(1024.0*mult),byteschar);
                    if (tmp->minspeed>0.0) {
                        snprintf(tmpbuf2,sizeof(tmpbuf2),"/min %.2f kB/s",tmp->minspeed);
                        strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
                    }
                    strmcat(tmpbuf1,"]",sizeof(tmpbuf1));
                    timercmd("FTIMER",tmpbuf1,(char *) func);
                    if ((tmp->next || chan->next) && !(number%3)) delay+=LIST_DELAY;
                }
                number++;
            }
#if !defined(CELEHOOK) && !defined(LITE)
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%.0f %.0f",BytesReceived,BytesSent);
            if (do_hook(CDCC_PLIST_FOOTER,"%d %d %s",count,CdccStats,tmpbuf2))
#endif
            {
                if (CdccStats) {
                    formatstats(tmpbuf2,1);
                    if (delay==-1) send_text(chan->channel,tmpbuf2,"PRIVMSG");
                    else {
                        snprintf(tmpbuf1,sizeof(tmpbuf1),"-INV %d PRIVMSG %s :%s",delay,chan->channel,
                                tmpbuf2);
                        timercmd("FTIMER",tmpbuf1,(char *) func);
                    }
                }
            }
            from_server=oldserver;
        }
        if (current) break;
    }
    LastPlist=time((time_t *) 0);
}

/**********************************************************************
* plistmcommand: Puts list to current channel                         *
***********************************************************************/
static void plistmcommand(line)
char *line;
{
    int   current=0;
    char  *tmpchan;
    ChannelList *chan=server_list[curr_scr_win->server].chan_list;

    if (packs) {
        if (!(tmpchan=new_next_arg(line,&line))) tmpchan=CdccChannels;
        if (!tmpchan) {
            say("You must set CDCC CHANNELS first");
            return;
        }
        if (!my_stricmp(tmpchan,"current")) {
            tmpchan=get_channel_by_refnum(0);
            if (!tmpchan) {
                NoWindowChannel();
                return;
            }
            current=1;
            chan=lookup_channel(tmpchan,from_server,0);
        }
        doplist(tmpchan,current,chan);
    }
    else say("No packs created");
}

/* Does actual notice */
static void donotice(tmpchan,current,chan)
char *tmpchan;
int current;
ChannelList *chan;
{
    int count;
    int oldserver;
    char *mynick=get_server_nickname(from_server);
    char tmpbuf[mybufsize/4];
    Packs *tmp;

    for (tmp=packs,count=0;tmp;tmp=tmp->next) count++;
    oldserver=from_server;
    for (;chan;chan=chan->next) {
        if (current || CheckCdccChannel(chan->channel,tmpchan)) {
            snprintf(tmpbuf,sizeof(tmpbuf),"%s  %d PACK%s OFFERED   /CTCP %s CDCC LIST",
                    CdccString,count,count==1?empty_string:"S",mynick);
            from_server=chan->server;
            send_text(chan->channel,tmpbuf,"PRIVMSG");
            from_server=oldserver;
        }
        if (current) break;
    }
    LastNlist=time((time_t *) 0);
}

/**********************************************************************
* noticemcommand: yells about offer to current channel                *
***********************************************************************/
static void noticemcommand(line)
char *line;
{
    int   current=0;
    char  *tmpchan;
    ChannelList *chan=server_list[curr_scr_win->server].chan_list;

    if (packs) {
        if (!(tmpchan=new_next_arg(line,&line))) tmpchan=CdccChannels;
        if (!tmpchan) {
            say("You must set CDCC CHANNELS first");
            return;
        }
        if (!my_stricmp(tmpchan,"current")) {
            tmpchan=get_channel_by_refnum(0);
            if (!tmpchan) {
                NoWindowChannel();
                return;
            }
            current=1;
            chan=lookup_channel(tmpchan,from_server,0);
        }
        donotice(tmpchan,current,chan);
    }
    else say("No packs created");
}

/***********************************************************************
 * Sets new pack description                                           *
 ***********************************************************************/
#ifdef EXTRAS
static void renamepackmcommand(line)
char *line;
{
    int  number;
    int  i;
    char *pack;
    char *desc;
    Packs *tmp;

    if (packs) {
        if ((pack=new_next_arg(line,&line))) {
            if (pack && *pack=='#') pack++;
            number=atoi(pack);
            desc=line;
            for (tmp=packs,i=1;tmp;tmp=tmp->next,i++) {
                if (i==number) {
                    say("Renamed pack #%d from %s to %s",number,tmp->description,desc);
                    malloc_strcpy(&tmp->description,desc);
                    return;
                }
            }
            say("Invalid pack number %d",number);
        }
        else PrintUsage("CDCC RENPACK #packno new description");
    }
    else say("No packs created");
}
#endif

/***********************************************************************
 * Lists files in queue                                                *
 ***********************************************************************/
void queuemcommand(line)
char *line;
{
    int  count=0;
    int  countdel=0;
    char *tmpstr;
    char *files=(char *) 0;
    char *file=(char *) 0;
    char *nick=(char *) 0;
    FileQueue *tmp;
    FileQueue *prev=(FileQueue *) 0;
    FileQueue *tmpdel;

    tmpstr=new_next_arg(line,&line);
    if (!queuelist) {
        if (!tmpstr || my_stricmp(tmpstr,"FLUSH"))
            say("No files in queue");
        return;
    }
    if (tmpstr && !my_stricmp(tmpstr,"LIST")) {
        tmpstr=new_next_arg(line,&line);
        if (tmpstr && *tmpstr) nick=tmpstr;
        if (nick) say("Listing all files in queue for %s",nick);
        else say("Listing all files in queue");
        for (tmp=queuelist;tmp;tmp=tmp->next)
            if (!nick || wild_match(nick,tmp->nick)) {
                count++;
                if (nick) {
                    if (!(file=rindex(tmp->file,'/'))) file=tmp->file;
                    else file++;
                    if (files) malloc_strcat(&files," ");
                    malloc_strcat(&files,file);
                    if (count && (count%70)==0) {
                        say("%s",files);
                        new_free(&files);
                    }
                }
                else say("#%-2d %s to %s",count,tmp->file,tmp->nick);
            }
        if (nick) {
            if (files) {
                say("%s",files);
                new_free(&files);
            }
            say("Total of %d file%s in queue for %s",count,count==1?"":"s",nick);
        }
        else say("Total of %d file%s in queue",count,count==1?"":"s");
    }
    else if (tmpstr && !my_stricmp(tmpstr,"REMOVE")) {
        tmpstr=new_next_arg(line,&line);
        if (tmpstr && *tmpstr) {
            for (tmp=queuelist;tmp;) {
                count++;
                tmpdel=tmp;
                tmp=tmp->next;
                if (matchmcommand(tmpstr,count)) {
                    if (prev) prev->next=tmpdel->next;
                    else queuelist=tmpdel->next;
                    new_free(&(tmpdel->file));
                    new_free(&(tmpdel->nick));
                    new_free(&tmpdel);
                    countdel++;
                }
                else prev=tmpdel;
            }
            say("Total of %d files removed from queue",countdel);
        }
        else PrintUsage("CDCC QUEUE REMOVE filter");
    }
    else if (tmpstr && !my_stricmp(tmpstr,"FLUSH")) 
        RemoveFromQueue(0);
    else {
        for (tmp=queuelist;tmp;tmp=tmp->next) count++;
        say("Total of %d file%s in queue",count,count==1?"":"s");
    }
}

/**********************************************************************
* loadmcommand: loads packs from file                                 *
***********************************************************************/
#ifndef LITE
static void loadmcommand(line)
char *line;
{
    int   count=0;
    int   lineno=0;
    char  *file;
    char  *filepath;
    char  *tmpstr;
    char  *tmpstr1;
    char  tmpbuf[mybufsize/2];
    FILE  *fp;
    Packs *tmp=NULL;
    Packs *last;
    Files *tmpfile;
    Files *lastfile;

    file=new_next_arg(line,&line);
    if (!file) file="ScrollZ.offer";
    if (!(filepath=OpenCreateFile(file,1)) || (fp=fopen(filepath,"r"))==NULL) {
#ifdef WANTANSI
        say("%sError%s: Can't open file %s !",
            CmdsColors[COLWARNING].color1,Colors[COLOFF],file);
#else
        say("Can't open file %s",file);
#endif
        return;
    }
    last=packs;
    while (last && last->next) last=last->next;
    while (fgets(tmpbuf,mybufsize/2,fp)) {
        lineno++;
        if (tmpbuf[0]=='#') continue;
        if (tmpbuf[strlen(tmpbuf)-1]=='\n') tmpbuf[strlen(tmpbuf)-1]='\0';
        tmpstr=tmpbuf;
        tmpstr1=new_next_arg(tmpstr,&tmpstr);
        if (tmpstr1 && *tmpstr1 && !my_strnicmp(tmpstr1,"PACK",4)) {
            tmp=(Packs *) new_malloc(sizeof(Packs));
            tmp->description=NULL;
            while (tmpstr && *tmpstr && isspace(*tmpstr)) tmpstr++;
            malloc_strcpy(&tmp->description,tmpstr);
            tmp->totalfiles=0;
            tmp->totalbytes=0;
            tmp->gets=0;
            tmp->minspeed=0.0;
            tmp->files=NULL;
            tmp->next=NULL;
            if (last) last->next=tmp;
            else packs=tmp;
            last=tmp;
            count++;
        }
        else if (tmpstr1 && *tmpstr1 && !my_strnicmp(tmpstr1,"SPEED",5)) {
            if (tmp) tmp->minspeed=atof(tmpstr);
            else {
#ifdef WANTANSI
                say("%sError%s in %s, %sline %d%s (SPEED should follow PACK)",
                     CmdsColors[COLWARNING].color1,Colors[COLOFF],file,
                     CmdsColors[COLWARNING].color3,lineno,Colors[COLOFF]);
#else
                say("Error in %s, line %d (SPEED should follow PACK)",file,lineno);
#endif
                continue;
            }
        }
        else if (tmpstr1 && *tmpstr1 && !my_strnicmp(tmpstr1,"GETS",4)) {
            if (tmp) tmp->gets=atoi(tmpstr);
            else {
#ifdef WANTANSI
                say("%sError%s in %s, %sline %d%s (GETS should follow PACK)",
                     CmdsColors[COLWARNING].color1,Colors[COLOFF],file,
                     CmdsColors[COLWARNING].color3,lineno,Colors[COLOFF]);
#else
                say("Error in %s, line %d (GETS should follow PACK)",file,lineno);
#endif
                continue;
            }
        }
        else if (tmpstr1 && *tmpstr1 && !my_strnicmp(tmpstr1,"FILE",4)) {
            if (tmp) {
                for (tmpstr1=new_next_arg(tmpstr,&tmpstr);tmpstr1;tmpstr1=new_next_arg(tmpstr,&tmpstr))
                    if (SeedFiles(tmpstr1,0)) {
                        lastfile=tmp->files;
                        while (lastfile && lastfile->next) lastfile=lastfile->next;
                        if (lastfile) lastfile->next=files;
                        else tmp->files=files;
                        files=NULL;
                    }
                    else {
#ifdef WANTANSI
                        say("%sError%s in %s, %sline %d%s (can't stat %s)",
                            CmdsColors[COLWARNING].color1,Colors[COLOFF],file,
                            CmdsColors[COLWARNING].color3,lineno,Colors[COLOFF],tmpstr1);
#else
                        say("Error in %s, line %d (can't stat %s)",file,lineno,tmpstr1);
#endif
                        continue;
                    }
            }
            else {
#ifdef WANTANSI
                say("%sError%s in %s, %sline %d%s (FILE should follow PACK)",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF],file,
                    CmdsColors[COLWARNING].color3,lineno,Colors[COLOFF]);
#else
                say("Error in %s, line %d (FILE should follow PACK)",file,lineno);
#endif
                continue;
            }
        }
    }
    fclose(fp);
    for (tmp=packs;tmp;tmp=tmp->next) {
        tmp->totalbytes=0;
        tmp->totalfiles=0;
        for (tmpfile=tmp->files;tmpfile;tmpfile=tmpfile->next) {
            tmp->totalbytes+=tmpfile->size;
            tmp->totalfiles++;
        }
    }
    tmp=packs;
    last=packs;
    while (tmp) {
        if (!(tmp->totalfiles)) {
            count--;
            if (tmp==packs) {
                packs=tmp->next;
                last=packs;
            }
            else last->next=tmp->next;
            say("No files in pack %s, deleting...",tmp->description);
            new_free(&tmp->description);
            for (tmpfile=tmp->files;tmpfile;tmpfile=lastfile) {
                lastfile=tmpfile->next;
                new_free(&tmpfile->path);
                new_free(&tmpfile->file);
                new_free(&tmpfile);
            }
            new_free(&tmp);
            tmp=last;
        }
        else {
            last=tmp;
            tmp=tmp->next;
        }
    }
    say("Loaded %d pack%s from %s",count,count==1?empty_string:"s",file);
    CdccPackNum+=count;
}

/**********************************************************************
* savemcommand: saves packs to file                                   *
***********************************************************************/
static void savemcommand(line)
char *line;
{
    int   count=0;
    int   oldumask=umask(0177);
    char  *file;
    char  *filepath;
    FILE  *fp;
    Packs *tmp;
    Files *tmpfile;

    if (packs) {
        file=new_next_arg(line,&line);
        if (!file) file="ScrollZ.offer";
        if (!(filepath=OpenCreateFile(file,1)) || (fp=fopen(filepath,"w"))==NULL) {
#ifdef WANTANSI
            say("%sError%s: Can't open file %s !",
                CmdsColors[COLWARNING].color1,Colors[COLOFF],file);
#else
            say("Can't open file %s",file);
#endif
            umask(oldumask);
            return;
        }
        for (tmp=packs;tmp;tmp=tmp->next) {
            fprintf(fp,"PACK %s\n",tmp->description);
            if (tmp->minspeed>0.0) fprintf(fp,"SPEED %.2f\n",tmp->minspeed);
            fprintf(fp,"GETS %d\n",tmp->gets);
            for (tmpfile=tmp->files;tmpfile;tmpfile=tmpfile->next)
                fprintf(fp,"FILE \"%s/%s\"\n",tmpfile->path,tmpfile->file);
            count++;
        }
        fclose(fp);
        say("Saved %d pack%s to %s",count,count==1?empty_string:"s",file);
    }
    else say("No packs created");
    umask(oldumask);
}

/***********************************************************************
 * requestmcommand: Lets User tell leechers what he needs. By Zakath   *
 ***********************************************************************/
static void requestmcommand(line)
char *line;
{
    if (line && *line) {
        CdccReqTog=1;    /* Assume request will be turned on, unless "OFF" */
        if (!my_stricmp(line,"OFF")) {
            CdccReqTog=0;
            new_free(&CdccRequest);   /* Not sure if new_free() is correct */
        }
        else malloc_strcpy(&CdccRequest,line);    /* It works */
    }
    if (!CdccReqTog) PrintSetting("Cdcc request","OFF",empty_string,
                                  empty_string);
    else if (CdccRequest && CdccReqTog) PrintSetting("Cdcc request",CdccRequest,
                                                     empty_string,empty_string);
}
#endif /* LITE */

/**********************************************************************
* offermcommand: Prompt User for files                                *
***********************************************************************/
static void offermcommand(line)
char *line;
{
    char *speed;
    char *desc;

    if (line && *line) {
        speed=index(line,',');
        if (!speed) {
            PrintUsage("CDCC OFFER pattern1 pattern2 , speed , description");
            return;
        }
        *speed='\0';
        speed++;
        while (isspace(*speed)) speed++;
        desc=index(speed,',');
        if (!desc) {
            PrintUsage("CDCC OFFER pattern1 pattern2 , speed , description");
            return;
        }
        *desc='\0';
        desc++;
        if (!(*desc)) {
            PrintUsage("CDCC OFFER pattern1 pattern2 , speed , description");
            return;
        }
        if (AddFiles2List(line)) offer4mcommand(speed,desc);
    }
    else add_wait_prompt("Add what files to pack ? ",offer2mcommand,line,WAIT_PROMPT_LINE);
}

/**********************************************************************
* offer2mcommand  This parses offer file list                         *
*                 And puts in Files Linked List                       *
***********************************************************************/
static void offer2mcommand(blah,line)
char *blah;
char *line;
{
    if (line && *line) {
        if (AddFiles2List(line))
            add_wait_prompt("Min speed for pack ? ",offer3mcommand,line,WAIT_PROMPT_LINE);
    }
    else say("You must specify file(s) to add to pack");
}

/**********************************************************************
* offer3mcommand  This asks for description                           *
***********************************************************************/
static void offer3mcommand(blah,line)
char *blah;
char *line;
{
    add_wait_prompt("Pack description ? ",offer4mcommand,line,WAIT_PROMPT_LINE);
}

/**********************************************************************
* offer4mcommand  Final part of pack creation                         *
***********************************************************************/
static void offer4mcommand(blah,line)
char *blah;
char *line;
{
    if (line && *line) AddToOfferList(blah,line);
    else say("You must specify pack description");
    DeleteSend();
}

/**********************************************************************
* AddToOfferList:  Adds Files to offer List                           *
***********************************************************************/
static void AddToOfferList(speed,desc)
char *speed;
char *desc;
{
    int   totalfiles=0;
    int   totalbytes=0;
    char  byteschar;
    char  tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char  tmpbuf2[mybufsize/8];
#endif
    float mult;
    Packs *new=(Packs *) 0;
    Packs *tmppack;
    Files *tmp=(Files *) 0;
    Files *tmp2=(Files *) 0;
    Files *tmp3=(Files *) 0;

    new=(Packs *) new_malloc(sizeof(Packs));
    new->description=(char *) 0;
    new->minspeed=(speed && *speed)?atof(speed):0.0;
    new->gets=0;
    malloc_strcpy(&(new->description),desc);
    new->files=NULL;
    new->next=NULL;
    for (tmppack=packs;tmppack && tmppack->next;tmppack=tmppack->next);
    if (tmppack) tmppack->next=new;
    else packs=new;
    if (files) {
        tmp=files;
        for (;;) {
            tmp2=tmp->next;
            for (tmp3=new->files;tmp3 && tmp3->next;) tmp3=tmp3->next;
            if (tmp3) tmp3->next=tmp;
            else new->files=tmp;
            tmp->next=(Files *) 0;
            totalfiles++;
            totalbytes=totalbytes+tmp->size;
            if (!tmp2) break;
            tmp=tmp2;
        }
        files=(Files *) 0;
    }
    else files=(Files *) 0;
    new->totalbytes=totalbytes;
    new->totalfiles=totalfiles;
    if (totalbytes>1048575) {
        byteschar='M';
        mult=1024.0;
    }
    else {
        byteschar='k';
        mult=1.0;
    }
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%.2f %cB/%d file%s",(float) (totalbytes)/(1024.0*mult),byteschar,
            totalfiles,totalfiles==1?empty_string:"s");
#ifdef WANTANSI
    if (new->minspeed>0.0)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s/min %.2f kB/s%s",
                CmdsColors[COLCDCC].color5,new->minspeed,Colors[COLOFF]);
    else *tmpbuf2='\0';
    say("%sCdcc%s %screated new pack%s : [%s%s%s%s] ",
        CmdsColors[COLCDCC].color4,Colors[COLOFF],
        CmdsColors[COLCDCC].color3,Colors[COLOFF],
        CmdsColors[COLCDCC].color5,tmpbuf1,Colors[COLOFF],tmpbuf2);

#else
    if (new->minspeed>0.0) 
        say("Cdcc created new pack : [%s/min %.2f kB/s]",tmpbuf1,new->minspeed);
    else say("Cdcc created new pack : [%s]",tmpbuf1);
#endif
/****** Coded by Zakath ******/
    CdccPackNum++;
    update_all_status();
/*****************************/
}

/**********************************************************************
* sendmcommand: Prompt User for files                                 *
***********************************************************************/
static void sendmcommand(line)
char *line;
{
    char *comma;

    if (line && *line) {
        comma=strchr(line,',');
        if (!comma) {
            PrintUsage("CDCC SEND pattern1 pattern2 , nick1 nick2");
            return;
        }
        *comma='\0';
        comma++;
        if (AddFiles2List(line)) send3mcommand(NULL,comma);
    }
    else add_wait_prompt("Files to send ? ",send2mcommand,line,WAIT_PROMPT_LINE);
}

/**********************************************************************
* send2mcommand  This parses file send list                           *
*                Files Linked list                                    *
***********************************************************************/
static void send2mcommand(blah,line)
char *blah;
char *line;
{
    if (line && *line) {
        if (AddFiles2List(line))
            add_wait_prompt("Send to whom ? (ie. nick1 nick2 nick3) ",send3mcommand,line,WAIT_PROMPT_LINE);
    }
    else say("You must specify file(s) to send");
}

/**********************************************************************
* send3mcommand  This parses nick send list, and sends all files in   *
*                Files Linked list                                    *
***********************************************************************/
static void send3mcommand(blah,line)
char *blah;
char *line;
{
    int  count;
    int  total;
    int  queue=0;
    int  queueret;
    int  queuesay=0;
    int  queuesent=0;
    char byteschar;
    char *nick=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/8];
    float mult;
    Files *tmp;
    unsigned int display;

    if (line && *line) {
        for (nick=new_next_arg(line,&line);nick;nick=new_next_arg(line,&line)) {
            total=0;
            count=0;
            display=window_display;
            window_display=0;
            for (tmp=files;tmp;tmp=tmp->next) {
                if (TotalSendDcc()<CdccLimit) {
                    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s \"%s/%s\"",nick,tmp->path,tmp->file);
                    dcc_filesend(tmpbuf1);
                    count++;
                    total+=tmp->size;
                }
                else {
                    if ((queueret=AddToQueue(tmp,nick,1))>0) {
                        queue++;
                        count++;
                        total+=tmp->size;
                    }
                    else if (!queuesent && queueret==0) {
                        queuesent=1;
                        if (!CTCPCloaking)
                            send_to_server("NOTICE %s :Sorry, my Cdcc queue is full",nick);
                    }
                    if (!queuesay && queueret==-1) queuesay=1;
                }
            }
            window_display=display;
            if (total>1048575) {
                byteschar='M';
                mult=1024.0;
            }
            else {
                byteschar='k';
                mult=1.0;
            }
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%.2f %cB/%d file%s",(float) (total)/(1024.0*mult),byteschar,
                    count,count==1?empty_string:"s");
            if (queue) snprintf(tmpbuf1,sizeof(tmpbuf1),", %d file%s in queue",queue,
                               queue==1?"":"s");
            else *tmpbuf1='\0';
            if (count || queue) {
                if (!CTCPCloaking)
                    send_to_server("NOTICE %s :Sent : [%s]%s",nick,tmpbuf2,tmpbuf1);
#ifdef WANTANSI
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%sCdcc%s %ssending%s %s%s%s : ",
                        CmdsColors[COLCDCC].color4,Colors[COLOFF],
                        CmdsColors[COLCDCC].color3,Colors[COLOFF],
                        CmdsColors[COLCDCC].color1,nick,Colors[COLOFF]);
                if (!queue) say("%s[%s%s%s]",tmpbuf1,
                                CmdsColors[COLCDCC].color5,tmpbuf2,Colors[COLOFF]);
                else say("%s[%s%s%s], %d file%s in queue",tmpbuf1,
                         CmdsColors[COLCDCC].color5,tmpbuf2,Colors[COLOFF],queue,
                         queue==1?"":"s");
                if (queuesent) say("%sCdcc%s %squeue%s is full",
                                   CmdsColors[COLCDCC].color4,Colors[COLOFF],
                                   CmdsColors[COLCDCC].color3,Colors[COLOFF]);
#else
                if (!queue) say("Cdcc sending %s : [%s]",nick,tmpbuf2);
                else say("Cdcc sending %s : [%s], %d file%s in queue",nick,tmpbuf2,queue,
                         queue==1?"":"s");
                if (queuesent) say("Cdcc queue is full");
#endif
            }
            if (queuesay) say("Duplicate files were not put in queue");
        }
    }
    else say("You must specify nick(s) to send file(s) to");
    DeleteSend();
}

/**********************************************************************
* resendmcommand: Prompt User for files                               *
***********************************************************************/
static void resendmcommand(line)
char *line;
{
    char *comma;

    if (line && *line) {
        comma=strchr(line,',');
        if (!comma) {
            PrintUsage("CDCC RESEND pattern1 pattern2 , nick1 nick2");
            return;
        }
        *comma='\0';
        comma++;
        while (*comma==' ') comma++;
        if (AddFiles2List(line)) resend3mcommand(NULL,comma);
    }
    else add_wait_prompt("Files to resend ? ",resend2mcommand,line,WAIT_PROMPT_LINE);
}

/**********************************************************************
* resend2mcommand  This parses file resend list                       *
*                  Files Linked list                                  *
***********************************************************************/
static void resend2mcommand(blah,line)
char *blah;
char *line;
{
    if (line && *line) {
        if (AddFiles2List(line))
            add_wait_prompt("Resend to whom ? (ie. nick1 nick2 nick3) ",resend3mcommand,line,WAIT_PROMPT_LINE);
    }
    else say("You must specify file(s) to resend");
}

/*************************************************************************
* resend3mcommand  This parses nick resend list, and resends all files   *
*                  in Files Linked list                                  *
**************************************************************************/
static void resend3mcommand(blah,line)
char *blah;
char *line;
{
    int  count=0;
    int  total=0;
    char byteschar;
    char *nick=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/8];
    float mult;
    Files *tmp;
    unsigned int display;

    if (line && *line) {
        for (nick=new_next_arg(line,&line);nick;nick=new_next_arg(line,&line)) {
            display=window_display;
            window_display=0;
            for (tmp=files;tmp;tmp=tmp->next) {
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%s \"%s/%s\"",nick,tmp->path,tmp->file);
                dcc_resend(tmpbuf1);
                count++;
                total=total+tmp->size;
            }
            window_display=display;
            if (total>1048575) {
                byteschar='M';
                mult=1024.0;
            }
            else {
                byteschar='k';
                mult=1.0;
            }
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%.2f %cB/%d file%s",(float) (total)/(1024.0*mult),byteschar,
                    count,count==1?empty_string:"s");
            if (!CTCPCloaking) send_to_server("NOTICE %s :Resent : [%s]",nick,tmpbuf2);
#ifdef WANTANSI
            snprintf(tmpbuf1,sizeof(tmpbuf1),"%sCdcc%s %sresending%s %s%s%s : ",
                    CmdsColors[COLCDCC].color4,Colors[COLOFF],
                    CmdsColors[COLCDCC].color3,Colors[COLOFF],
                    CmdsColors[COLCDCC].color1,nick,Colors[COLOFF]);
            say("%s[%s%s%s]",tmpbuf1,CmdsColors[COLCDCC].color5,tmpbuf2,Colors[COLOFF]);
#else
            say("Cdcc resending %s : [%s]",nick,tmpbuf2);
#endif
            total=0;
            count=0;
        }
    }
    else say("You must specify nick(s) to resend file(s) to");
    DeleteSend();
}

/**********************************************************************
* psendmcommand: Prompt User for pack                                 *
***********************************************************************/
#ifndef LITE
static void psendmcommand(line)
char *line;
{
    char *comma;

    if (packs) {
        if (line && *line) {
            comma=strchr(line,',');
            if (!comma) {
                PrintUsage("CDCC PSEND pattern , nick1 nick2");
                return;
            }
            *comma='\0';
            comma++;
            while (*comma==' ') comma++;
            psend3mcommand(line,comma);
        }
        else {
            ShowPacks(NULL);
            add_wait_prompt("Packs to send (1-6,3 or * for all) ? ",psend2mcommand,line,WAIT_PROMPT_LINE);
        }
    }
    else say("No packs created");
}

/**********************************************************************
* psend2mcommand  This parses nick send list, and sends all files in  *
*                Pack Linked list                                     *
***********************************************************************/
static void psend2mcommand(blah,line)
char *blah;
char *line;
{
    if (line && *line)
        add_wait_prompt("Send to whom ? (ie. nick1 nick2 nick3) ",psend3mcommand,line,WAIT_PROMPT_LINE);
    else say("You must specify pack(s) to send");
}

/**********************************************************************
* psend3mcommand  This parses pack send list                          *
***********************************************************************/
static void psend3mcommand(blah,line)
char *blah;
char *line;
{
    int  packcount;
    char *nick;
    char *tmpstr;
    char tmpbuf[mybufsize/32];
    Packs *tmp;

    if (line && *line) {
        tmpstr=line;
        for (nick=new_next_arg(tmpstr,&tmpstr);nick;nick=new_next_arg(tmpstr,&tmpstr)) {
            packcount=1;
            for (tmp=packs;tmp;tmp=tmp->next) {
                if (matchmcommand(blah,packcount)) {
                    snprintf(tmpbuf,sizeof(tmpbuf),"%d",packcount);
                    /* if user is sending manually avoid queue/min speed */
                    sendcommand(nick,tmpbuf,0, 1);
                }
                packcount++;
            }
        }
    }
    else say("You must specify nicks(s) to send pack(s) to");
    DeleteSend();
}
#endif /* LITE */

/**********************************************************************
* SeedFiles: Gets Users Line of Files with path, and rips em apart    *
* Puts path, file, and size into Files linked list.                   *
***********************************************************************/ 
static int SeedFiles(line,error)
char *line;
int  error;
{
    int  i=0;
    int  size=0;
    int  count=0;
    char *file;
    char *rest;
    char *string;
    char *fullname;
    char  tmpbuf1[mybufsize/2+1];
    char  tmpbuf2[mybufsize/4+1];
    struct stat tmpstat;

    file=line;
    if (*file=='/') {
        if (strlen(file)>1 && rindex(file,'/')==file) snprintf(tmpbuf1,sizeof(tmpbuf1),"/ %s",&file[1]);
        else strmcpy(tmpbuf1,file,sizeof(tmpbuf1));
    }
    else if (*file=='~') {
        if (0 == (fullname=expand_twiddle(file))) {
            if (error) yell("Unable to expand %s!",file);
            return(0);
        }
        strmcpy(tmpbuf1,fullname,sizeof(tmpbuf1));
        new_free(&fullname);
    }
    else {
        if (CdccUlDir) strmcpy(tmpbuf1,CdccUlDir,sizeof(tmpbuf1));
        else getcwd(tmpbuf1,sizeof(tmpbuf1)-1);
        strmcat(tmpbuf1,"/",sizeof(tmpbuf1));
        strmcat(tmpbuf1,file,sizeof(tmpbuf1));
    }
    rest=rindex(tmpbuf1,'/');
    if (rest==tmpbuf1) rest++;
    if (rest) *rest++='\0';
    if (access(tmpbuf1, R_OK)!=0) {
        if (error) say("Can't access %s",tmpbuf1);
        return(0);
    }
    GetDir(tmpbuf1);
    for (i=0;i<CdccEntries;i++) {
        strmcpy(tmpbuf2,CdccFileNames[i]->d_name,sizeof(tmpbuf2));
        string=tmpbuf2;
#ifdef lame_dgux
        string=string-2;
        if (string[0]=='.') continue;
#endif
        if (wild_match(rest,string)) {
            char tmpbuf3[mybufsize/2+1];

            strmcpy(tmpbuf3,tmpbuf1,mybufsize/2);
            strmcat(tmpbuf3,"/",mybufsize/2);
            strmcat(tmpbuf3,tmpbuf2,mybufsize/2);
            tmpstat.st_mode=0;
            size=stat(tmpbuf3,&tmpstat);
            if (tmpstat.st_mode & S_IFDIR) {
                if (error)
                    say("You tried to send a dir %s, please do a %s/* to send a whole dir",
                        tmpbuf2,tmpbuf2);
                continue;
            }
            if ((size=GetSize(tmpbuf1,string))!=-1) {
                AddFileToList(tmpbuf1,string,size);
                count++;
            }
        }
    }
    return(count);
}

/**********************************************************************
* Add File to  FILES linked list                                      *
***********************************************************************/
static void AddFileToList(path,file,size)
char *path;
char *file;
int  size;
{
    Files *new;
    Files *tmp;

    new=(Files *) new_malloc(sizeof(Files));
    new->path=(char *) 0;
    new->file=(char *) 0;
    new->next=(Files *) 0;
    malloc_strcpy(&(new->path),path);
    malloc_strcpy(&(new->file),file);
    new->size=size;
    for (tmp=files;tmp && tmp->next;) tmp=tmp->next;
    if (tmp) tmp->next=new;
    else files=new;
}

/**********************************************************************
* Returns number of files added to list                               *
***********************************************************************/
static int AddFiles2List(line)
char *line;
{
    int   count=0;
    char  *file=(char *) 0;
    char  tmpbuf[mybufsize*2];
    Files *tmpfile;

    for (file=new_next_arg(line,&line);file;file=new_next_arg(line,&line))
        count+=SeedFiles(file,1);
    if (count) {
#ifdef WANTANSI
        snprintf(tmpbuf,sizeof(tmpbuf),"%sCdcc%s %sadded%s %s%d%s file%s (",
                CmdsColors[COLCDCC].color4,Colors[COLOFF],
                CmdsColors[COLCDCC].color3,Colors[COLOFF],
                CmdsColors[COLCDCC].color5,count,Colors[COLOFF],
                count==1?empty_string:"s");
#else
        snprintf(tmpbuf,sizeof(tmpbuf),"Added %d file%s (",count,count==1?empty_string:"s");
#endif
        for (tmpfile=files;tmpfile;tmpfile=tmpfile->next) {
#ifdef WANTANSI
            strmcat(tmpbuf,CmdsColors[COLCDCC].color5,sizeof(tmpbuf));
#endif
            strmcat(tmpbuf,tmpfile->file,sizeof(tmpbuf));
            if (tmpfile->next) strmcat(tmpbuf," ",sizeof(tmpbuf));
            else {
#ifdef WANTANSI
                strmcat(tmpbuf,Colors[COLOFF],sizeof(tmpbuf));
#endif
                strmcat(tmpbuf,")",sizeof(tmpbuf));
            }
            if (strlen(tmpbuf)>mybufsize*2-mybufsize/4) {
                say("%s",tmpbuf);
                *tmpbuf='\0';
            }
        }
        if (*tmpbuf) say("%s",tmpbuf);
    }
    else {
        say("No files found, aborting...");
        DeleteSend();
    }
    return(count);
}

/*********************************************************************
* Deletes the file link list                                         *
**********************************************************************/
static void DeleteSend()
{
    Files *tmp;
    Files *next;

    for (tmp=files;tmp;tmp=next) {
        next=tmp->next;
        new_free(&(tmp->path));
        new_free(&(tmp->file));
        new_free(&tmp);
    }
    files=(Files *) 0;
}

/*********************************************************************
* Shows the offer link list                                          *
**********************************************************************/
static void ShowPacks(args)
char *args;
{
    int   packcount=1;
    char  byteschar;
    char  *word=(char *) 0;
    char  tmpbuf1[mybufsize/4];
    char  tmpbuf2[mybufsize/32];
    float mult;
    Packs *tmp;
    Files *tmp1;

    if (args && *args) word=new_next_arg(args,&args);
    if (word) {
        if (packs) {
            for (tmp=packs;tmp;tmp=tmp->next) {
                if (matchmcommand(word,packcount)) {
                    if (tmp->minspeed>0.0) 
                        say("Pack: %d  Description: %s  Min: %.2f kB/s",packcount,
                            tmp->description,tmp->minspeed);
                    else say("Pack: %d  Description: %s",packcount,tmp->description);
                    say("kBytes      File");
                    for (tmp1=tmp->files;tmp1;tmp1=tmp1->next) {
                        snprintf(tmpbuf1,sizeof(tmpbuf1),"%-11.2f",(float) (tmp1->size)/1024.0);
                        say("%s %s",tmpbuf1,tmp1->file);
                    }
                    say("----------- ---------------");
                }
                packcount++;
            }
            formatstats(tmpbuf1,0);
            say("%s",tmpbuf1);
        }
        else say("No packs created");
    }
    else {
        if (packs) {
            for (tmp=packs;tmp;tmp=tmp->next) {
                if (tmp->minspeed>0.0)
                    snprintf(tmpbuf2,sizeof(tmpbuf2),"/min %.2f kB/s]",tmp->minspeed);
                else strmcpy(tmpbuf2,"]",sizeof(tmpbuf2));
                if (tmp->totalbytes>1048575) {
                    byteschar='M';
                    mult=1024.0;
                }
                else {
                    byteschar='k';
                    mult=1.0;
                }
                snprintf(tmpbuf1,sizeof(tmpbuf1),"[%d file%s/%.2f %cB%s",
                        tmp->totalfiles,tmp->totalfiles==1?empty_string:"s",
                        (float) (tmp->totalbytes)/(1024.0*mult),byteschar,tmpbuf2);
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%d/%d",packcount,tmp->gets);
                say("#%-4s %s  %s",tmpbuf2,tmp->description,tmpbuf1);
                packcount++;
            }
            formatstats(tmpbuf1,0);
            say("%s",tmpbuf1);
        }
        else say("No packs created");
    }
}

/*********************************************************************
* GetDir:  This gets the listing of all the files in *ptr dir.       *
**********************************************************************/
static void GetDir(path)
char *path;
{
    CleanList();
    CdccEntries=scandir(path, &CdccFileNames,
                        (const void *)(int (*) (const struct dirent *)) selectent,
                        (const void *)(int (*) (const struct dirent *, const struct dirent *)) compar);
}

/*********************************************************************
* CleanList: Cleans up global dirent                                 *
**********************************************************************/
static void CleanList()
{
    int i;

    if (CdccFileNames) {
        for (i=0;i<CdccEntries;i++) new_free(&(CdccFileNames[i]));     
        new_free(&CdccFileNames);
        CdccEntries=0;
    }
}

/*********************************************************************
* compar: used by scandir to alphabetize files in dir                *
**********************************************************************/
static int compar(e1,e2)
struct dirent **e1;
struct dirent **e2;
{
    return (my_stricmp((*e1)->d_name,(*e2)->d_name));
}

/***************************************************************************
* selectent: used by scandir to decide which entries to include in the dir *
*            listing.  Ignores ., includes all other files                 *
****************************************************************************/
static int selectent(entry)
struct dirent *entry;
{
    if (*(entry->d_name)=='.') return (0);
    else return(1);
}

/***********************************************************************
* GetSize: takes path, and filename, cats them together, and returns   *
*          filesize of file, if dir, returns -1                        *
************************************************************************/
static int GetSize(path,file)
char *path;
char *file;
{
    char  tmpbuf[mybufsize/2];

    snprintf(tmpbuf,sizeof(tmpbuf),"%s/%s",path,file);
    stat(tmpbuf,&CdccStatBuf);
    if (CdccStatBuf.st_mode & S_IFDIR) return(-1);
    return(CdccStatBuf.st_size);
}

/**********************************************************************
* getmcommand: Prompt User for nick (* for all)                       *
***********************************************************************/
static void getmcommand(line)
char *line;
{
    char *word;

    if ((word=new_next_arg(line,&line))) get2mcommand(NULL,word);
    else {
        showdccsmcommand(66);
        add_wait_prompt("What to get (1-6,3 or * for all) ",get2mcommand,
                        line,WAIT_PROMPT_LINE);
    }
}

/**********************************************************************
* get2mcommand:          This will parse your dcc list, and get files *
*                        Specified by your filter.                    *
***********************************************************************/
static void get2mcommand(data,line)
char *data;
char *line;
{
    int  count=0;
    int  mode;
    char *tmp=(char *) 0;
    char  tmpbuf[mybufsize/4];
    DCC_list *Client;
    unsigned flags;

    if (!(tmp=new_next_arg(line,&line))) {
        say("You must specify what to get");
        return;
    }
    for (Client=ClientList;Client;Client=Client->next) {
        flags=Client->flags;
        if ((flags&DCC_OFFER)==DCC_OFFER) {
            mode=0;
            if ((flags&DCC_TYPES)==DCC_FILEREAD) mode=1;
            else if ((flags&DCC_TYPES)==DCC_FILEREGET) mode=2;
            count++;
            if (matchmcommand(tmp,count)) {
                snprintf(tmpbuf,sizeof(tmpbuf),"%s %s",Client->user,Client->description);
                if (mode==1) dcc_getfile(tmpbuf);
                else if (mode==2) dcc_regetfile(tmpbuf);
            }
        }
    }
}

/* Parse message to see what they want. */
void CheckCdcc(nick,args,to,msg)
char *nick;
char *args;
char *to;
int  msg;
{
    int  level=0;
    char *command=(char *) 0;
#ifdef WANTANSI
    char  tmpbuf1[mybufsize/2];
#endif
#ifdef WANTANSI
    char  tmpbuf2[mybufsize/8];
    char  tmpbuf3[mybufsize/8];
#endif
    char  tmpbuf4[mybufsize];
    time_t timenow;
    static time_t lastcheck=0;
    struct friends *tmpfriend=NULL;

    if (msg) {
        timenow=time((time_t *) 0);
        if (timenow-lastcheck<3) return;
        lastcheck=timenow;
    }
    snprintf(tmpbuf4,sizeof(tmpbuf4),"%s!%s",nick,FromUserHost);
    if ((tmpfriend=CheckUsers(tmpbuf4,NULL))) level=tmpfriend->privs;
    if (msg) command=new_next_arg(args,&args);
    else command="CDCC";
#ifdef WANTANSI
    if (args && *args)
        snprintf(tmpbuf1,sizeof(tmpbuf1)," %s%s%s",
                CmdsColors[COLCDCC].color3,args,Colors[COLOFF]);
    else *tmpbuf1='\0';
    ColorUserHost(FromUserHost,CmdsColors[COLCTCP].color2,tmpbuf2,1);
    snprintf(tmpbuf4,sizeof(tmpbuf4),"%sCdcc%s%s request received from %s%s%s %s",
            CmdsColors[COLCDCC].color4,Colors[COLOFF],tmpbuf1,
            CmdsColors[COLCDCC].color1,nick,Colors[COLOFF],tmpbuf2);
    if (to && is_channel(to)) {
        snprintf(tmpbuf3,sizeof(tmpbuf3)," to %s%s%s",CmdsColors[COLCDCC].color6,to,Colors[COLOFF]);
        strmcat(tmpbuf4,tmpbuf3,sizeof(tmpbuf4));
    }
#else
    snprintf(tmpbuf4,sizeof(tmpbuf4),"Cdcc%s request received from %s (%s)",args,nick,FromUserHost);
    if (to && is_channel(to)) {
        char buf[mybufsize];

        snprintf(buf,sizeof(buf)," to %s",to);
        strmcat(tmpbuf4,buf,sizeof(tmpbuf4));
    }
#endif
    if (away_set || LogOn) AwaySave(tmpbuf4,SAVECTCP);
    if (Security && !(level&FLCDCC)) {
        strmcat(tmpbuf4,", no access",sizeof(tmpbuf4));
        if (!CTCPCloaking)
            send_to_server("NOTICE %s :You do not have access...  -ScrollZ-",nick);
        say("%s",tmpbuf4);
        return;
    }
    if (CdccVerbose==1) say("%s",tmpbuf4);
    if (command && (!my_stricmp(command,"CDCC") || !my_stricmp(command,"XDCC")))
        command=new_next_arg(args,&args);
    if (command) {
        if (!my_stricmp(command,"HELP")) helpcommand(nick,args);
        else if (!my_stricmp(command,"SEND")) sendcommand(nick,args,0,level&FLCDCC);
        else if (!my_stricmp(command,"RESEND")) sendcommand(nick,args,1,level&FLCDCC);
        else if (!my_stricmp(command,"LIST")) listcommand(nick,args);
        else if (!my_stricmp(command,"VERSION")) versioncommand(nick,args);
        else if (!my_stricmp(command,"QUEUE")) queuecommand(nick,args);
        else if (!CTCPCloaking) send_to_server("NOTICE %s :Try /CTCP %s CDCC HELP to get Cdcc help",nick,get_server_nickname(from_server));
    }
    else if (!CTCPCloaking) send_to_server("NOTICE %s :Try /CTCP %s CDCC HELP to get Cdcc help",nick,get_server_nickname(from_server));
}

/* send msg'er help menu */
static void helpcommand(from,args)
char *from;
char *args;
{
    if (CTCPCloaking) return;
    if (args && *args) {
        if (!my_stricmp(args,"VERSION")) send_to_server("NOTICE %s :Cdcc VERSION will report Cdcc version",from);
        else if (!my_stricmp(args,"HELP")) send_to_server("NOTICE %s :Cdcc HELP will give you Cdcc help",from);
        else if (!my_stricmp(args,"LIST"))
            send_to_server("NOTICE %s :Cdcc LIST will list offered packs",from);
        else if (!my_stricmp(args,"SEND") || !my_stricmp(args,"RESEND")) {
            upper(args);
            send_to_server("NOTICE %s :Cdcc %s filter will %ssend packs matching filter",
                           from,args,!strcmp(args,"RESEND")?"re":"");
            send_to_server("NOTICE %s :filter can be : -2,4,6-8,10- or * for all",from);
        }
        else if (!my_stricmp(args,"QUEUE"))
            send_to_server("NOTICE %s :Cdcc QUEUE will show your queue position",from);
    }
    else send_to_server("NOTICE %s :/CTCP %s CDCC HELP command where command is one of the following : HELP VERSION LIST QUEUE SEND or RESEND",from,get_server_nickname(from_server));
}

/* send msg'er version reply */
static void versioncommand(from, args)
char *from;
char *args;
{
    if (CTCPCloaking) return;
    send_to_server("NOTICE %s :Cdcc v" CDCC_VERSION " written by Sheik & Flier",from);
    send_to_server("NOTICE %s :IRC's first XDCC clone in C !",from);
}

/* Sends to server w/o print */
static void sendlist(text)
char *text;
{
    send_to_server("%s",text);
}

/* Check what listing type they want. */
static void listcommand(from,args)
char *from;
char *args;
{
    int   count=0;
    int   multdelay;
    int   packcount=1;
    char  byteschar;
    char  *mynick=get_server_nickname(from_server);
    char  tmpbuf1[mybufsize];
    char  tmpbuf2[mybufsize/2];
    char  tmpbuf3[mybufsize/16];
    float mult;
    Packs *tmp;
    void (*func)()=(void(*)()) sendlist;
    time_t timenow=time((time_t *) 0);
    static int delay=0;

    if (packs) {
        for (tmp=packs;tmp;tmp=tmp->next) count++;
        multdelay=count/3*LIST_DELAY;
        if (timenow>LastList+multdelay+10) delay=0;
        else delay+=5;
        LastList=timenow;
#if !defined(CELEHOOK) && !defined(LITE)
        if (do_hook(CDCC_PLIST_HEADER,"%s %d %s",from,count,mynick)) 
#endif
        {
            snprintf(tmpbuf1,sizeof(tmpbuf1),"-INV %d NOTICE %s :%s    %d PACK%s OFFERED   /CTCP %s CDCC SEND N for pack N",
                    delay,from,CdccString,count,count==1?empty_string:"S",
                    mynick);
            timercmd("FTIMER",tmpbuf1,(char *) func);
            if (delay) delay++;
        }
        for (tmp=packs;tmp;tmp=tmp->next) {
            if (tmp->minspeed>0.0)
                snprintf(tmpbuf1,sizeof(tmpbuf1),"/min %.2f kB/s]",tmp->minspeed);
            else strmcpy(tmpbuf1,"]",sizeof(tmpbuf1));
            if (tmp->totalbytes>1048575) {
                byteschar='M';
                mult=1024.0;
            }
            else {
                byteschar='k';
                mult=1.0;
            }
            snprintf(tmpbuf2,sizeof(tmpbuf2),"[%d file%s/%.2f %cB%s",
                    tmp->totalfiles,tmp->totalfiles==1?empty_string:"s",
                    (float) (tmp->totalbytes)/(1024.0*mult),byteschar,tmpbuf1);
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%d/%dx",packcount,tmp->gets);

            snprintf(tmpbuf1,sizeof(tmpbuf1),"%.2f",tmp->minspeed);
#if !defined(CELEHOOK) && !defined(LITE)
            if (do_hook(CDCC_PLIST,"%d %d %d %s %d %s",packcount,
                        tmp->totalfiles,tmp->totalbytes,tmpbuf1,tmp->gets,
                        tmp->description))
#endif
            {
                snprintf(tmpbuf1,sizeof(tmpbuf1),"-INV %d NOTICE %s :#%-6s %s  %s",delay,
                        from,tmpbuf3,tmp->description,tmpbuf2);
                timercmd("FTIMER",tmpbuf1,(char *) func);
                if (tmp->next && !(packcount%3)) delay+=LIST_DELAY;
            }

            packcount++;
        }
#if !defined(CELEHOOK) && !defined(LITE)
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%.0f %.0f",BytesReceived,BytesSent);
        if (do_hook(CDCC_PLIST_FOOTER,"%d %d %s",count,CdccStats,tmpbuf1))
#endif
        {
            if (CdccStats) {
                formatstats(tmpbuf2,1);
                snprintf(tmpbuf1,sizeof(tmpbuf1),"-INV %d NOTICE %s :%s",delay,from,tmpbuf2);
                timercmd("FTIMER",tmpbuf1,(char *) func);
                if (delay) delay++;
            }
        }
    }
    else if (!CTCPCloaking)
        send_to_server("NOTICE %s :Sorry, there are no files offered",from);
}

/* List files in queue */
static void queuecommand(from,args)
char *from;
char *args;
{
    int i=1;
    int cnt=0;
    char *tmpstr=(char *) 0;
    char tmpbuf[mybufsize/64];
    FileQueue *tmp=queuelist;

    if (!queuelist) {
        if (!CTCPCloaking)
            send_to_server("NOTICE %s :There are no files in queue",from);
        return;
    }
    for (;tmp;tmp=tmp->next) {
        if (tmp->server==parsing_server_index && !my_stricmp(tmp->nick,from)) {
            if (cnt) malloc_strcat(&tmpstr,",");
            snprintf(tmpbuf,sizeof(tmpbuf),"%d",i);
            malloc_strcat(&tmpstr,tmpbuf);
            if (cnt>=10) {
                malloc_strcat(&tmpstr,"...");
                break;
            }
            cnt++;
        }
        i++;
    }
    if (cnt) {
        send_to_server("NOTICE %s :Your queue position: %s",from,tmpstr);
        new_free(&tmpstr);
    }
}

/* Send pack that they request */
static void sendcommand(from,args,resend,level)
char *from;
char *args;
int  resend;
int  level;
{
    int   packcount=1;
    int   totalfiles=0;
    int   totalbytes=0;
    int   sent=0;
    int   queue=0;
    int   packsent;
    int   queueret;
    int   queuesay=0;
    int   queuesent=0;
    char  byteschar;
    char  *word;
    char  tmpbuf1[mybufsize/4];
    char  tmpbuf2[mybufsize/4];
    char  tmpbuf3[mybufsize/4];
    float mult;
    Packs *tmp;
    Files *tmp1;
    unsigned int display;
    DCC_list *tmpdcc;

    word=new_next_arg(args,&args);
    if (word) {
        *tmpbuf3='\0';
        if (*word=='#') word++;
        if (packs) {
            for (tmp=packs;tmp;tmp=tmp->next) {
                if (matchmcommand(word,packcount)) {
                    tmp->gets++;
                    packsent=0;
                    display=window_display;
                    window_display=0;
                    sent++;
                    for (tmp1=tmp->files;tmp1;tmp1=tmp1->next) {
                        if (level || TotalSendDcc()<CdccLimit) {
                            int dccflag;

                            packsent=1;
                            snprintf(tmpbuf2,sizeof(tmpbuf2),"%s/%s",tmp1->path,tmp1->file);
                            snprintf(tmpbuf1,sizeof(tmpbuf1),"%s \"%s\"",from,tmpbuf2);
                            if (resend) {
                                dccflag=DCC_RESENDOFFER;
                                dcc_resend(tmpbuf1);
                            }
                            else {
                                dccflag=DCC_FILEOFFER;
                                dcc_filesend(tmpbuf1);
                            }
                            tmpdcc=dcc_searchlist(NULL,from,dccflag,0,tmpbuf2);
                            if (tmpdcc) {
                                if (!level) tmpdcc->minspeed=tmp->minspeed;
                                else tmpdcc->minspeed=0.0;
                            }
                            totalbytes+=tmp1->size;
                            totalfiles++;
                        }
                        else {
                            if ((queueret=AddToQueue(tmp1,from,resend+1))>0) {
                                packsent=1;
                                queue++;
                                totalbytes+=tmp1->size;
                                totalfiles++;
                            }
                            else if (!queuesent && queueret==0) {
                                queuesent=1;
                                if (!CTCPCloaking)
                                    send_to_server("NOTICE %s :Sorry, my Cdcc queue is full",from);
                            }
                            if (!queuesay && queueret==-1) queuesay=1;
                        }
                    }
                    window_display=display;
                    if (packsent) {
                        snprintf(tmpbuf1,sizeof(tmpbuf1),"%d",packcount);
                        if (*tmpbuf3) strmcat(tmpbuf3,",",sizeof(tmpbuf3));
                        strmcat(tmpbuf3,tmpbuf1,sizeof(tmpbuf3));
                    }
                }
                packcount++;
            }
            if (totalfiles) {
                if (totalbytes>1048575) {
                    byteschar='M';
                    mult=1024.0;
                }
                else {
                    byteschar='k';
                    mult=1.0;
                }
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%.2f %cB/%d file%s",
                        (float) (totalbytes)/(1024.0*mult),byteschar,
                        totalfiles,totalfiles==1?empty_string:"s");
                if (!queue) {
                    if (!CTCPCloaking) send_to_server("NOTICE %s :%sent packs %s : %s",from,
                                                      resend?"Res":"S",tmpbuf3,tmpbuf2);
                }
                else if (!CTCPCloaking) send_to_server("NOTICE %s :%sent packs %s : %s, %d file%s in queue",
                                                       from,resend?"Res":"S",tmpbuf3,tmpbuf2,queue,
                                                       queue==1?"":"s");
#ifdef WANTANSI
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%sCdcc%s %s%ssent%s %s%s%s packs %s : ",
                        CmdsColors[COLCDCC].color4,Colors[COLOFF],
                        CmdsColors[COLCDCC].color3,resend?"re":"",Colors[COLOFF],
                        CmdsColors[COLCDCC].color1,from,Colors[COLOFF],tmpbuf3);
                if (CdccVerbose==1) {
                    if (!queue) say("%s[%s%s%s]",tmpbuf1,
                                    CmdsColors[COLCDCC].color5,tmpbuf2,Colors[COLOFF]);
                    else say("%s[%s%s%s], %s%d%s file%s in queue",tmpbuf1,
                             CmdsColors[COLCDCC].color5,tmpbuf2,Colors[COLOFF],
                             CmdsColors[COLCDCC].color5,queue,Colors[COLOFF],
                             queue==1?"":"s");
                    if (queuesent) say("%sCdcc%s %squeue%s is full",
                                       CmdsColors[COLCDCC].color4,Colors[COLOFF],
                                       CmdsColors[COLCDCC].color3,Colors[COLOFF]);
                }
#else
                if (CdccVerbose==1) {
                    if (!queue) say("Cdcc %ssent %s %s : [%s]",resend?"re":"",
                                    from,tmpbuf3,tmpbuf2);
                    else say("Cdcc %ssent %s %s : [%s], %d file%s in queue",
                             resend?"re":"",from,tmpbuf3,tmpbuf2,queue,queue==1?"":"s");
                    if (queuesent) say("Cdcc queue is full");
                }
#endif
/****** Coded by Zakath ******/
#ifndef LITE
                if (CdccReqTog && CdccRequest)
                    send_to_server("NOTICE %s :I need %s",from,CdccRequest);
#endif
/*****************************/
            }
            if (queuesay && CdccVerbose==1)
                say("Duplicate files were not put in queue");
            if (!sent && !CTCPCloaking) send_to_server("NOTICE %s :No packs found matching %s",from,word);
        }
        else if (!CTCPCloaking) send_to_server("NOTICE %s :Sorry, there are no files offered",from);
    }
    else if (!CTCPCloaking) send_to_server("NOTICE %s :Try /CTCP %s CDCC SEND N",from,get_server_nickname(from_server));
}

/**********************************************************************
* TotalSendDcc:          Return Total Dcc Sends Currently on Dcc      *
*                        Linked List.                                 *
***********************************************************************/
int TotalSendDcc()
{
    DCC_list *Client;
    unsigned flags;
    int counter=0;

    for (Client=ClientList;Client;Client=Client->next) {
        flags=Client->flags;
        if (flags&DCC_DELETE) continue;
        if (((flags&DCC_TYPES)==DCC_FILEOFFER) ||
            ((flags&DCC_TYPES)==DCC_RESENDOFFER)) counter++;
    }
    return(counter);
}

/* return number of files in queue */
int TotalQueue()
{
    int count=0;
    FileQueue *tmp;

    for (tmp=queuelist;tmp;tmp=tmp->next) count++;
    return(count);
}

/* get offer files if auto-get is on */
void CheckAutoGet(nick,userhost,file,type)
char *nick;
char *userhost;
char *file;
char *type;
{
    char tmpbuf[mybufsize/4];
    struct friends *tmpfriend;

    if (Security) {
        snprintf(tmpbuf,sizeof(tmpbuf),"%s!%s",nick,userhost);
        tmpfriend=CheckUsers(tmpbuf,NULL);
        if (!tmpfriend) return;
        if (!((tmpfriend->privs)&FLCDCC)) return;
    }
#ifdef WANTANSI
    snprintf(tmpbuf,sizeof(tmpbuf),"%sCdcc%s %sauto getting%s %s%s%s from ",
            CmdsColors[COLCDCC].color4,Colors[COLOFF],
            CmdsColors[COLCDCC].color3,Colors[COLOFF],
            CmdsColors[COLCDCC].color5,file,Colors[COLOFF]);
    say("%s%s%s%s",tmpbuf,CmdsColors[COLCDCC].color1,nick,Colors[COLOFF]);
#else
    say("Cdcc auto getting %s from %s",file,nick);
#endif
    if (away_set || LogOn) {
        snprintf(tmpbuf,sizeof(tmpbuf),"Cdcc auto getting %s from %s (%s)",file,nick,FromUserHost);
        AwaySave(tmpbuf,SAVECDCC);
    }
    snprintf(tmpbuf,sizeof(tmpbuf),"%s \"%s\"",nick,file);
    if (!my_stricmp(type,"SEND")) dcc_getfile(tmpbuf);
    else if (!my_stricmp(type,"RESEND")) dcc_regetfile(tmpbuf);
#ifdef BROKEN_MIRC_RESUME
    else if (!my_stricmp(type,"RESUME")) dcc_getfile_resume(tmpbuf);
#endif /* BROKEN_MIRC_RESUME */
}


/**********************************************************************
* CheckCdccTimer:        Checks pending Cdcc timer stuff              *
***********************************************************************/
void CheckCdccTimers()
{
    int  current=0;
    int  oldserver;
    char *tmpstr;
    char *channel;
#ifdef WANTANSI
    char  tmpbuf1[mybufsize];
    char  tmpbuf2[mybufsize];
#endif
    time_t timenow;
    unsigned flags;
    DCC_list *Client;
    ChannelList *chan;

    timenow=time((time_t *) 0);
    if (timenow-LastIdleCheck>=60) {
        LastIdleCheck=timenow;
        for (Client=ClientList;CdccIdle && Client;Client=Client->next) {
            flags=Client->flags;
            if (!(flags&DCC_ACTIVE)) {
                tmpstr=rindex(Client->description,'/');
                if (tmpstr) tmpstr++;
                else tmpstr=Client->description;
                if ((flags&DCC_TYPES)==DCC_FILEOFFER || (flags&DCC_TYPES)==DCC_RESENDOFFER) {
                    if (timenow-Client->CdccTime>CdccIdle) {
#ifdef WANTANSI
                        snprintf(tmpbuf1,sizeof(tmpbuf1),"%sCdcc%s %sclosing%s idle dcc %s%s%s",
                                CmdsColors[COLCDCC].color4,Colors[COLOFF],
                                CmdsColors[COLCDCC].color3,Colors[COLOFF],
                                CmdsColors[COLCDCC].color3,dcc_types[flags&DCC_TYPES],
                                Colors[COLOFF]);
                        snprintf(tmpbuf2,sizeof(tmpbuf2)," %s%s%s to %s%s%s",
                                CmdsColors[COLCDCC].color5,tmpstr,Colors[COLOFF],
                                CmdsColors[COLCDCC].color1,Client->user,Colors[COLOFF]);
                        say("%s%s",tmpbuf1,tmpbuf2);
#else
                        say("Cdcc closing idle dcc %s %s to %s",
                            dcc_types[flags&DCC_TYPES],tmpstr,Client->user);
#endif
                        if (!CTCPCloaking) {
                            oldserver=from_server;
                            from_server=Client->server;
                            if (CheckServer(from_server))
                                send_to_server("NOTICE %s :Cdcc %s %s auto closed.",
                                               Client->user,dcc_types[flags&DCC_TYPES],
                                               tmpstr);
                            from_server=oldserver;
                        }
                        dcc_erase(Client);
                    }
                }
                else if (timenow-Client->CdccTime>3*CdccIdle) {
#ifdef WANTANSI
                    snprintf(tmpbuf1,sizeof(tmpbuf1),"%sCdcc%s %sclosing%s idle dcc %s%s%s",
                            CmdsColors[COLCDCC].color4,Colors[COLOFF],
                            CmdsColors[COLCDCC].color3,Colors[COLOFF],
                            CmdsColors[COLCDCC].color3,dcc_types[flags&DCC_TYPES],
                            Colors[COLOFF]);
                    snprintf(tmpbuf2,sizeof(tmpbuf2)," %s%s%s from %s%s%s",
                            CmdsColors[COLCDCC].color5,tmpstr,Colors[COLOFF],
                            CmdsColors[COLCDCC].color1,Client->user,Colors[COLOFF]);
                    say("%s%s",tmpbuf1,tmpbuf2);
#else
                    say("Cdcc closing idle dcc %s %s from %s",dcc_types[flags&DCC_TYPES],
                        tmpstr,Client->user);
#endif
                    dcc_erase(Client);
                }
            }
        }
    }
    if (curr_scr_win->server!=-1) {
        chan=server_list[curr_scr_win->server].chan_list;
        if (packs && CdccChannels && PlistTime && timenow-LastPlist>PlistTime) {
            if (!my_stricmp(CdccChannels,"current")) {
                channel=get_channel_by_refnum(0);
                if (!channel) current=-1;
                else {
                    current=1;
                    chan=lookup_channel(channel,from_server,0);
                }
            }
            if (current!=-1) doplist(CdccChannels,current,chan);
        }
        if (packs && CdccChannels && NlistTime && timenow-LastNlist>NlistTime) {
            if (!my_stricmp(CdccChannels,"current")) {
                channel=get_channel_by_refnum(0);
                if (!channel) current=-1;
                else {
                    current=1;
                    chan=lookup_channel(channel,from_server,0);
                }
            }
            if (current!=-1) donotice(CdccChannels,current,chan);
        }
    }
}

/**********************************************************************
* CheckDCCSpeed:         Checks the pending DCC speed                 *
***********************************************************************/
void CheckDCCSpeed(Client,timenow)
DCC_list *Client;
time_t timenow;
{
    int    oldserver;
    float  rate=-1.0;
    char   *tmpstr;
#ifdef WANTANSI
    char   tmpbuf1[mybufsize/4];
    char   tmpbuf2[mybufsize/4];
#endif

    if (timenow-Client->starttime)
        rate=((float) (Client->bytes_sent)/(float)(timenow-Client->starttime))/1024.0;
    if (rate>=0.0 && rate<Client->minspeed) {
        tmpstr=rindex(Client->description,'/');
        if (tmpstr) tmpstr++;
#ifdef WANTANSI
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%sCdcc%s %sclosing%s slow dcc %s%s%s ",
                CmdsColors[COLCDCC].color4,Colors[COLOFF],
                CmdsColors[COLCDCC].color3,Colors[COLOFF],
                CmdsColors[COLCDCC].color3,dcc_types[Client->flags&DCC_TYPES],
                Colors[COLOFF]);
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%s%s%s to %s%s%s (%srate %.2f",
                tmpbuf1,CmdsColors[COLCDCC].color5,tmpstr,Colors[COLOFF],
                CmdsColors[COLCDCC].color1,Client->user,Colors[COLOFF],
                CmdsColors[COLCDCC].color5,rate);
        say("%skB/s%s/%smin %.2f kB/s%s)",tmpbuf2,Colors[COLOFF],
            CmdsColors[COLCDCC].color5,Client->minspeed,Colors[COLOFF]);
#else
        say("Cdcc closing slow dcc %s %s to %s (rate %.2f kB/s/min %.2f kB/s)",
            dcc_types[Client->flags&DCC_TYPES],tmpstr,Client->user,rate,
            Client->minspeed);
#endif
        if (!CTCPCloaking) {
            oldserver=from_server;
            from_server=Client->server;
            if (CheckServer(from_server))
                send_to_server("NOTICE %s :Cdcc %s %s auto closed, dcc was too slow (rate %.2f kB/s/min %.2f kB/s)",
                               Client->user,dcc_types[Client->flags&DCC_TYPES],tmpstr,
                               rate,Client->minspeed);
            from_server=oldserver;
        }
        Client->flags|=DCC_DELETE;
    }
    else {
        if (rate>4*Client->minspeed) Client->CdccTime+=25L;
        else if (rate>2*Client->minspeed) Client->CdccTime+=20L;
        else Client->CdccTime+=15L;
    }
}

/**********************************************************************
* AddToQueue:            Adds file to queue                           *
***********************************************************************/
static int AddToQueue(tmpfile,nick,flag)
Files *tmpfile;
char  *nick;
int   flag;
{
    int count=0;
    char *spath;
    char tmpbuf[mybufsize/4];
    DCC_list *Client;
    FileQueue *tmp;
    FileQueue *newfile;

    count=TotalQueue();
    if (CdccQueueLimit>0 && count>=CdccQueueLimit) return(0);
    snprintf(tmpbuf,sizeof(tmpbuf),"%s/%s",tmpfile->path,tmpfile->file);
    /* check if nick/file combination is already in DCC list */
    for (Client=ClientList;Client;Client=Client->next) {
        /* we're only interested in filename so we strip path */
        if ((spath=rindex(Client->description,'/'))) spath++;
        else spath=Client->description;
        if (!strcmp(tmpfile->file,spath) && !my_stricmp(nick,Client->user))
            return(-1);
    }
    /* check if nick/file combination is already in queue */
    for (tmp=queuelist;tmp;tmp=tmp->next)
        if (!strcmp(tmpbuf,tmp->file) && !my_stricmp(nick,tmp->nick))
            return(-1);
    newfile=(FileQueue *) new_malloc(sizeof(FileQueue));
    newfile->server=from_server;
    newfile->file=(char *) 0;
    newfile->nick=(char *) 0;
    newfile->next=(FileQueue *) 0;
    malloc_strcpy(&(newfile->file),tmpbuf);
    malloc_strcpy(&(newfile->nick),nick);
    newfile->flag=flag;
    if (!queuelist) queuelist=newfile;
    else {
        for (tmp=queuelist;tmp && tmp->next;) tmp=tmp->next;
        tmp->next=newfile;
    }
    return(1);
}

/**********************************************************************
* RemoveFromQueue:       Removes file from queue                      *
***********************************************************************/
void RemoveFromQueue(removed)
int removed;
{
    int active=TotalSendDcc()-removed;
    int oldserver;
    char tmpbuf[mybufsize/4];
    unsigned int display;
    FileQueue *tmp=queuelist;

    while (tmp && active<CdccLimit) {
        queuelist=queuelist->next;
        snprintf(tmpbuf,sizeof(tmpbuf),"%s \"%s\"",tmp->nick,tmp->file);
        display=window_display;
        window_display=0;
        oldserver=from_server;
        from_server=tmp->server;
        if (CheckServer(from_server)) {
            if (tmp->flag==1) dcc_filesend(tmpbuf);
            else dcc_resend(tmpbuf);
            active++;
        }
        from_server=oldserver;
        window_display=display;
        new_free(&(tmp->nick));
        new_free(&(tmp->file));
        new_free(&tmp);
        tmp=queuelist;
    }
}

/**********************************************************************
* CdccQueueNickChange:   Handle nick change                           *
***********************************************************************/
void CdccQueueNickChange(oldnick,newnick)
char *oldnick;
char *newnick;
{
    FileQueue *tmp=queuelist;

    for (tmp=queuelist;tmp;tmp=tmp->next) {
        if (tmp->server!=from_server) continue;
        if (!my_stricmp(tmp->nick,oldnick)) malloc_strcpy(&(tmp->nick),newnick);
    }
}

/**********************************************************************
* CleanUpCdcc:           Free all allocated memory                    *
***********************************************************************/
void CleanUpCdcc() {
    Files *tmpfile;
    Files *tmpfilefree;
    Packs *tmppack;
    Packs *tmppackfree;
    FileQueue *tmpqueue;
    FileQueue *tmpqueuefree;

    for (tmppack=packs;tmppack;) {
        tmppackfree=tmppack;
        tmppack=tmppack->next;
        for (tmpfile=tmppackfree->files;tmpfile;) {
            tmpfilefree=tmpfile;
            tmpfile=tmpfile->next;
            new_free(&(tmpfilefree->path));
            new_free(&(tmpfilefree->file));
            new_free(&tmpfilefree);
        }
        new_free(&(tmppackfree->description));
        new_free(&tmppackfree);
    }
    for (tmpqueue=queuelist;tmpqueue;) {
        tmpqueuefree=tmpqueue;
        tmpqueue=tmpqueue->next;
        new_free(&(tmpqueuefree->nick));
        new_free(&(tmpqueuefree->file));
        new_free(&tmpqueuefree);
    }
}

void CdccTimeWarning() {
    say("Warning: your CDCC PLIST or NLIST timer is set below two hours.");
    say("Due to a lot of complaints about this the above timers have been");
    say("increased to two hours. If you feel your values are apropriate");
    say("enter the following commands:");
    if (PlistTime<7200) {
        say("/CDCC PTIME %d",PlistTime);
        PlistTime=7200;
    }
    if (NlistTime<7200) {
        say("/CDCC NTIME %d",NlistTime);
        NlistTime=7200;
    }
}
