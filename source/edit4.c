/******************************************************************************
 Functions coded by Flier (THANX to Sheik!!)

 PrintMessage        Prints received message
 HandleSplit         Handles net splits
 CheckJoin           Checks the joined person
 HandleJoin          Handles net joins
 ChannelJoin         This executes when you join a channel
 HandleInvite        Handles invite requests
 HandleKills         Handles operator kills
 HandleNickChange    Handles nick change
 HandleMyKick        Handles my kicks
 HandleKick          Handles kicks
 HandleTabNext       Insert next tab key entry
 HandleTabPrev       Insert previous tab key entry
 EndOfBans           Handles end of bans reply
 HandleNotice        Handles received notice
 HandleClosedConn    Handles closed connections from server
 HandleRename        Handles on the fly renaming of files
 HandleFlood         Handles flooding
 ModeLocked          Locks channel mode
 CheckLock           Checks if there is a need to lock mode back
 HandleEndOfWho      Handles end of who reply
 CTCPCloakingToggle  Toggles CTCP cloaking on/off
 Check4WordKick      Checks if wordkick should take place
 AddBan              This adds ban to ban list
 RemoveBan           This removes ban from ban list
 UnbanIt             Really unbans user
 TBan                Shows all bans and asks which one you wanna remove
 HandleFakes         Handles fakes
 ModeUnlocked        Unlocks channel mode
 ScatterKick         Kicks nick with funny message
 RandomScatterKick   Randomly picks funny message and kicks nick
 LastNoticeKick      Kicks nick with last notice
 NickStat            Prints some statistics about nick
 AddNotify           Adds user to notify list
 RemoveNotify        Removes user from notify list
 HandleNotifyOn      Handles notify
 HandleUserhost      Adds userhost info to notify list
 HandleNotifyOff     Handles notify off
 HandleNotifyOffUh   Handles notify userhost off
 ListNotify          Lists all users on notify list
 ClearBans           Clears my stuff in ChannelList (bans and modelock)
 HandleGotOps        Handles all things when you get ops
 MyQuit              This executes when you quit from IRC
 AddDelayOp          Adds user to delay op list
 AddDelayNotify      Adds nick to delay notify list
 AddServer           Adds given server to server list
 RemoveServer        Removes given server from server list
 ListServers         Lists all servers on server list
 IsBanned            Returns 1 if user is banned on channel, else 0
 PickSignOff         Randomly picks sign off message
 PickScatterKick     Randomly picks scatter kick
 ClearTab            Clears tabkey list
 HandleLinks         This handles links reply from server
 ListSplitedServers  Prints all servers missing from links info
 ShowKill            This will print out the sucker that last killed you
******************************************************************************/

/*
 * $Id: edit4.c,v 1.33 1999-09-26 16:26:37 f Exp $
 */

#include "irc.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "translat.h"
#include "output.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "screen.h"
#include "server.h"
#include "edit.h"
#include "ircterm.h"
#include "list.h"
#include "struct.h"
#include "myvars.h"
#include "whowas.h"

void   ListBansPage _((char *));
void   ListBansPrompt _((char *, char *));
void   MyQuitPrompt _((char *, char *));
void   AddDelayOp _((char *, char *, int));
void   AddDelayNotify _((char *));
int    IsBanned _((char *, char *, int, ChannelList *));
char   *PickSignOff _((void));
#ifdef SCKICKS
char   *PickScatterKick _((int));
#endif
#ifdef CELE
char   *CeleSignoff _((void));
#endif
extern char *CelerityNtfy;

extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern struct words *CheckLine _((char *, char *));
extern void AwaySave _((char *, int));
extern void AddNick2List _((char *, int));
extern int  AddSplitter _((char *, char *, char *));
extern int  Check4Fake _((char *));
extern struct friends *CheckUsers _((char *, char *));
extern void Check4Join _((char *, char *, char *));
extern int  CheckChannel _((char *, char *));
extern void NickNext _((void));
#ifdef EXTRAS
extern void FixName _((char *));
#endif
extern void ClearKey _((char *, char *, char *));
extern void Cdcc _((char *, char *, char *));
extern int  readln _((FILE *, char *));
extern void NextArg _((char *, char **, char *));
extern void PrintSetting _((char *, char *, char *, char *));
extern char *OpenCreateFile _((char *, int));
extern void PrintSynch _((ChannelList *));
extern void StripAnsi _((char *, char *, int));
extern int  matchmcommand _((char *, int));
extern int  GrabURL _((char *, char *, char *));
extern void Ignore _((char *, char *, char *));
extern void CheckPermBans _((ChannelList *));
#ifdef EXTRAS
void CheckLock _((char *, int, ChannelList *));
#endif
#if defined(EXTRAS) || defined(FLIER)
extern void CheckInvite _((char *, char *, int));
#endif
extern void NotChanOp _((char *));
extern void NoWindowChannel _((void));
extern void PrintUsage _((char *));
extern void ColorUserHost _((char *, char *, char *, int));
extern int  AddLast _((List *, List *));
extern NickList *find_in_hash _((ChannelList *, char *));
extern void HandleDelayOp _((void));
extern void HandleDelayNotify _((void));
extern void BanIt _((char *, char *, char *, int, ChannelList *));
#ifdef WANTANSI
extern void FixColorAnsi _((char *));
#endif
extern int  CheckServer _((int));
extern void CdccQueueNickChange _((char *, char *));
extern char *FormatTime _((int));
extern void AddJoinKey _((int, char *));

extern void e_channel _((char *, char *, char *));
extern void timercmd _((char *, char *, char *));
extern void me _((char *, char *, char *));
extern void waitcmd _((char *, char *, char *));
extern char *recreate_mode _((ChannelList *));

static struct bans *tmpbn;
static struct bans *tmpbanlist;
static int listcount;

extern NotifyList *notify_list;
extern DCC_list *ClientList;

extern char *chars;
extern NickList *tabnickcompl;

/* Prints received message */
void PrintMessage(nick,userhost,msg,print)
char *nick;
char *userhost;
char *msg;
int  print;
{
    int  numurl=0;
    char thing;
    char *message;
    char *filepath=NULL;
    char tmpbuf1[mybufsize];
#ifdef WANTANSI
    char tmpbuf2[mybufsize];
#endif
    char tmpbuf3[mybufsize];
    char tmpbuf4[mybufsize];

    if (!(userhost && *userhost)) userhost=(char *) 0;
    if (get_int_var(HIGH_ASCII_VAR)) thing='ù';
    else thing='*';
    if (URLCatch) {
        filepath=OpenCreateFile("ScrollZ.notepad",1);
        numurl=GrabURL(msg,tmpbuf4,filepath);
        message=tmpbuf4;
    }
    else message=msg;
#ifdef WANTANSI
#ifdef CELECOSM
    if (ExtMes && userhost)
        ColorUserHost(userhost,CmdsColors[COLMSG].color2,tmpbuf2,1);
    else *tmpbuf2='\0';
    sprintf(tmpbuf1,"%s[%s%s%s%s%s%s]%s %s%s%s",
            CmdsColors[COLMSG].color5,Colors[COLOFF],
            CmdsColors[COLMSG].color1,nick,Colors[COLOFF],tmpbuf2,
            CmdsColors[COLMSG].color5,Colors[COLOFF],
            CmdsColors[COLMSG].color3,message,Colors[COLOFF]);
    sprintf(tmpbuf3,"  <%s%s%s>",CmdsColors[COLMSG].color4,update_clock(GET_TIME),Colors[COLOFF]);
#else  /* CELECOSM */
    sprintf(tmpbuf1,"%c%s%s%s%c %s%s%s",
            thing,CmdsColors[COLMSG].color1,nick,Colors[COLOFF],thing,
            CmdsColors[COLMSG].color3,message,Colors[COLOFF]);
    if (ExtMes && userhost) {
        ColorUserHost(userhost,CmdsColors[COLMSG].color2,tmpbuf2,1);
#ifdef TDF
        sprintf(tmpbuf3,"  <[%s%s%s]%s>",
                CmdsColors[COLMSG].color4,update_clock(GET_TIME),Colors[COLOFF],tmpbuf2);
#else  /* TDF */
        sprintf(tmpbuf3,"  %s [%s%s%s]",tmpbuf2,
                CmdsColors[COLMSG].color4,update_clock(GET_TIME),Colors[COLOFF]);
#endif /* TDF */
    }
    else *tmpbuf3='\0';
#endif /* CELECOSM */
#else  /* WANTANSI */
    sprintf(tmpbuf1,"%c%s%c %s",thing,nick,thing,message);
    if (ExtMes && userhost) sprintf(tmpbuf3,"  (%s) [%s]",userhost,update_clock(GET_TIME));
    else *tmpbuf3='\0';
#endif /* WANTANSI */
    if (print) put_it("%s%s",tmpbuf1,tmpbuf3);
    StripAnsi(message,tmpbuf3,2);
    if (!(userhost && *userhost)) userhost=empty_string;
    sprintf(tmpbuf1,"*%s* %s  (%s [%s])",nick,tmpbuf3,userhost,update_clock(GET_TIME));
    malloc_strcpy(&(server_list[from_server].LastMessage),tmpbuf1);
    sprintf(tmpbuf1,"*%s* %s  (%s)",nick,tmpbuf3,userhost);
    if (away_set || LogOn) {
       AwaySave(tmpbuf1,SAVEMSG);
       AwayMsgNum++;
       update_all_status();
    }
    AddNick2List(nick,from_server);
    if (URLCatch && URLCatch<3 && numurl)
        say("Added %d URL%s to NotePad (%c%s%c)",numurl,numurl==1?"":"s",
            bold,filepath,bold);
}

/* Handles net splits */
void HandleSplit(reason,nick,channel,netsplit)
char *reason;
char *nick;
char *channel;
int  *netsplit;
{
    *netsplit=0;
    if (!NHDisp) return;
    if (wild_match("*.* *.*",reason) && !Check4Fake(reason)) {
        if (AddSplitter(nick,channel,reason)) *netsplit=2;
        else *netsplit=1;
    }
}

/* Checks joined person */
NickList *CheckJoin(nick,userhost,channel,server,tmpchan)
char *nick;
char *userhost;
char *channel;
int  server;
ChannelList *tmpchan;
{
    int  privs;
    int  voice=-1;
    int  ischanop;
    char *comment;
    char *tmpignore;
    char tmpbuf[mybufsize/4];
    NickList *tmpjoiner;
    ChannelList *chan;
    struct autobankicks *tmpabk;

    if (tmpchan) chan=tmpchan;
    else chan=lookup_channel(channel,server,0);
    if (!chan) return(NULL);
    tmpjoiner=CheckJoiners(nick,channel,server,chan);
    if (tmpjoiner && tmpjoiner->frlist && !(tmpjoiner->frlist->passwd))
        privs=tmpjoiner->frlist->privs;
    else privs=0;
    ischanop=(chan->status)&CHAN_CHOP;
    sprintf(tmpbuf,"%s!%s",nick,userhost);
    if (!privs && ischanop && chan->KickOnBan && IsBanned(tmpbuf,channel,server,chan))
#ifdef CELE
        send_to_server("KICK %s %s :Banned %s",channel,nick,CelerityL);
#else  /* CELE */
        send_to_server("KICK %s %s :Banned",channel,nick);
#endif /* CELE */
    else if (ischanop && (tmpabk=tmpjoiner->shitlist)) {
        if ((tmpabk->shit)&SLBAN)
            send_to_server("MODE %s -o+b %s %s",channel,nick,tmpabk->userhost);
        if ((tmpabk->shit)&SLKICK) {
            if (tmpabk->reason[0]) comment=tmpabk->reason;
            else comment=DefaultABK;
#ifdef CELE
            send_to_server("KICK %s %s :%s %s>",channel,nick,comment,CelerityL);
#else  /* CELE */
            send_to_server("KICK %s %s :%s",channel,nick,comment);
#endif /* CELE */
        }
        if ((tmpabk->shit)&SLIGNORE) {
            tmpignore=index(tmpabk->userhost,'!');
            if (tmpignore) tmpignore++;
            else tmpignore=tmpabk->userhost;
            sprintf(tmpbuf,"%s ALL",tmpignore);
            Ignore(NULL,tmpbuf,tmpbuf);
        }
    }
    if (ischanop && tmpjoiner && chan->FriendList &&
        ((privs&FLAUTOOP)|(privs&FLINSTANT))) {
        if (privs&FLOP) voice=0;
        else if (privs&FLVOICE) voice=1;
        if (voice>=0) {
            if ((privs&FLINSTANT))
                send_to_server("MODE %s +%c %s",channel,voice?'v':'o',nick);
            if (AutoOpDelay) AddDelayOp(channel,nick,voice);
            else if (ischanop)
                send_to_server("MODE %s +%c %s",channel,voice?'v':'o',nick);
        }
    }
    return(tmpjoiner);
}

/* Handles net joins */
int HandleJoin(tmpnick,nick,userhost,channel)
NickList *tmpnick;
char *nick;
char *userhost;
char *channel;
{
    char *servername;
    char *tmpstr;
    char *server;
#ifdef WANTANSI
    char *colnick;
#endif
    char tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/4];
#endif

    servername=tmpbuf1;
    Check4Join(userhost,servername,channel);
    if (!strcmp(servername,"000")) {
#ifdef WANTANSI
        if (tmpnick && tmpnick->shitlist && tmpnick->shitlist->shit)
            colnick=CmdsColors[COLJOIN].color6;
        else if (tmpnick && tmpnick->frlist && tmpnick->frlist->privs)
            colnick=CmdsColors[COLJOIN].color5;
        else colnick=CmdsColors[COLJOIN].color1;
        ColorUserHost(userhost,CmdsColors[COLJOIN].color2,tmpbuf1,1);
        say("%s%s%s %s has joined channel %s%s%s",
            colnick,nick,Colors[COLOFF],tmpbuf1,
            CmdsColors[COLJOIN].color3,channel,Colors[COLOFF]);
#else
        say("%s (%s) has joined channel %s",nick,userhost,channel);
#endif
        return(1);
    }
    else if (strcmp(servername,"111")) {
        if (NHDisp) {
            tmpstr=tmpbuf1;
            server=new_next_arg(tmpstr,&tmpstr);
#ifdef WANTANSI
            sprintf(tmpbuf2,"%sNetjoined%s at %s%s%s : ",
                    CmdsColors[COLNETSPLIT].color1,Colors[COLOFF],
                    CmdsColors[COLNETSPLIT].color2,update_clock(GET_TIME),Colors[COLOFF]);
            say("%s[%s%s%s %s<-%s %s%s%s]",tmpbuf2,
                CmdsColors[COLNETSPLIT].color3,tmpstr,Colors[COLOFF],
                CmdsColors[COLNETSPLIT].color6,Colors[COLOFF],
                CmdsColors[COLNETSPLIT].color3,server,Colors[COLOFF]);
#else
            say("Netjoined at %s : [%s <- %s]",
                update_clock(GET_TIME),tmpstr,server);
#endif
        }
    }
    return(0);
}

/* This executes when you join a channel */
NickList *ChannelJoin(nick,channel,chan)
char *nick;
char *channel;
ChannelList *chan;
{
    char tmpbuf[mybufsize/4+1];
    NickList *tmpjoiner;

    if (unban<2) unban=2;
    else unban++;
    inFlierWho++;
    tmpjoiner=CheckJoiners(nick,channel,from_server,chan);
    strmcpy(tmpbuf,nick,mybufsize/4);
    strmcat(tmpbuf,"/",mybufsize/4);
    strmcat(tmpbuf,channel,mybufsize/4);
#ifdef WANTANSI
    FixColorAnsi(tmpbuf);
#endif
    malloc_strcpy(&(server_list[from_server].LastJoin),tmpbuf);
#ifndef CELEHOOK
    do_hook(JOIN_ME_LIST,"%s",channel);
#endif
#ifdef ACID
    chan->TryRejoin=0;
#endif
    return(tmpjoiner);
}

/* Handles invite requests */
void HandleInvite(nick,userhost,channel)
char *nick;
char *userhost;
char *channel;
{
    int  isfake=0;
    int  printinv=1;
    char *tmpstr;
    char tmpbuf1[mybufsize];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/4];
#endif
    struct friends *tmpfriend;
    ChannelList *chan;

#ifdef WANTANSI
    ColorUserHost(userhost,CmdsColors[COLINVITE].color2,tmpbuf2,1);
#endif
    for (tmpstr=channel;*tmpstr;tmpstr++)
        if ((unsigned char)*tmpstr<' ') {
            isfake=1;
            break;
        }
#ifdef WANTANSI
    sprintf(tmpbuf1,"%s%s%s %s invites you to channel %s%s%s",
            CmdsColors[COLINVITE].color1,nick,Colors[COLOFF],tmpbuf2,
            CmdsColors[COLINVITE].color3,channel,Colors[COLOFF]);
    if (isfake) {
        sprintf(tmpbuf2," - %sfake%s",CmdsColors[COLINVITE].color4,Colors[COLOFF]);
        strcat(tmpbuf1,tmpbuf2);
    }
#else
    sprintf(tmpbuf1,"%s (%s) invites you to channel %s",nick,userhost,channel);
    if (isfake) strcat(tmpbuf1," - fake");
#endif
    if (CompressModes && AutoJoinOnInv && (chan=lookup_channel(channel,from_server,0))) {
        if (chan->connected==CHAN_JOINING) printinv=0;
    }
    if (printinv) say("%s",tmpbuf1);
    if (away_set || LogOn) AwaySave(tmpbuf1,SAVEINVITE);
    if (isfake) return;
    sprintf(tmpbuf1,"%s!%s",nick,userhost);
    tmpfriend=CheckUsers(tmpbuf1,NULL);
    if ((AutoJoinOnInv && CheckChannel(channel,AutoJoinChannels)) ||
        (tmpfriend && ((tmpfriend->privs)&FLJOIN))) {
        e_channel("JOIN",channel,NULL);
        if (printinv) {
#ifdef WANTANSI
            say("Auto joining %s%s%s",CmdsColors[COLINVITE].color3,channel,Colors[COLOFF]);
#else
            say("Auto joining %s",channel);
#endif
        }
    }
}

/* Handles operator kills */
void HandleKills(server,nick,userhost,reason)
int server;
char *nick;
char *userhost;
char *reason;
{
    char tmpbuf[mybufsize/2];

    sprintf(tmpbuf,"You have been killed by operator %s (%s) %s",nick,userhost,reason);
    malloc_strcpy(&WhoKilled,tmpbuf);
    if (away_set || LogOn) AwaySave(tmpbuf,SAVEKILL);
}

/* Handles nick change */
void HandleNickChange(oldnick,newnick,userhost,server)
char *oldnick;
char *newnick;
char *userhost;
int  server;
{
    int  privs=0;
    int  printed=0;
    int  curserv=from_server;
    char *mynick;
    char tmpbuf[mybufsize/2];
    time_t timenow;
    NickList *tmp;
    ChannelList *chan;
    struct nicks *nickstr;

    mynick=get_server_nickname(server);
    timenow=time((time_t *) 0);
    for (chan=server_list[server].chan_list;chan;chan=chan->next)
        if (chan->NickWatch) {
            if ((tmp=find_in_hash(chan,oldnick))) {
                tmp->nickc++;
                if (!(!my_stricmp(oldnick,mynick) || !my_stricmp(newnick,mynick))) {
                    if (tmp->frlist) privs=tmp->frlist->privs;
                    else privs=0;
                    if (!(privs&(FLPROT | FLGOD))) {
                        if (timenow-tmp->nickt>=NickTimer) {
                            tmp->curn=1;
                            tmp->nickp=0;
                            tmp->nickt=timenow;
                        }
                        else tmp->curn++;
                        if (!tmp->nickp && tmp->curn>=NickSensor && ((chan->status)&CHAN_CHOP)) {
#ifdef CELE
                            send_to_server("KICK %s %s :Nick flood detected %s",
                                           chan->channel,oldnick,CelerityL);
#else  /* CELE */
                            send_to_server("KICK %s %s :Nick flood detected",
                                           chan->channel,oldnick);
#endif /* CELE */
#ifdef WANTANSI
                            if (!printed) {
                                say("%sNick flood%s detected by %s%s%s",
                                    CmdsColors[COLWARNING].color1,Colors[COLOFF],
                                    CmdsColors[COLWARNING].color2,newnick,Colors[COLOFF]);
                                printed=1;
                            }
#else  /* WANTANSI */
                            if (!printed) {
                                say("%cNick flood%c detected by %s",bold,bold,newnick);
                                printed=1;
                            }
#endif /* WANTANSI */
                            if (away_set || LogOn) {
                                sprintf(tmpbuf,"Nick flood detected by %s (%s)",newnick,userhost);
                                AwaySave(tmpbuf,SAVEMASS);
                            }
                            tmp->nickp=1;
                        }
                    }
                }
            }
        }
    if (CheckServer(curserv)) {
        for (nickstr=server_list[curserv].nicklist;nickstr;nickstr=nickstr->next)
            if (!my_stricmp(nickstr->nick,oldnick))
                malloc_strcpy(&(nickstr->nick),newnick);
        for (nickstr=server_list[curserv].arlist;nickstr;nickstr=nickstr->next)
            if (!my_stricmp(nickstr->nick,oldnick))
                malloc_strcpy(&(nickstr->nick),newnick);
    }
    CdccQueueNickChange(oldnick,newnick);
}

/* Adds comment to buffer */
void AddComment(buffer,comment)
char *buffer;
char *comment;
{
    char tmpbuf[mybufsize/2];

    if (comment && *comment) {
        sprintf(tmpbuf,"%s  (%s)",buffer,comment);
        strcpy(buffer,tmpbuf);
    }
}

/* Handles my kicks */
void HandleMyKick(mynick,nick,userhost,channel,comment,frkick)
char *mynick;
char *nick;
char *userhost;
char *channel;
char *comment;
int  *frkick;
{
    char tmpbuf[mybufsize/2];

    if (away_set || LogOn) {
        sprintf(tmpbuf,"You have been kicked off channel %s by %s (%s)",
                channel,nick,userhost);
        AddComment(tmpbuf,comment);
        AwaySave(tmpbuf,SAVEKICK);
    }
}

/* Handles kicks */
void HandleKick(nick,who,userhost,channel,comment,frkick)
char *nick;
char *who;
char *userhost;
char *channel;
char *comment;
int  *frkick;
{
    int  privs=0;
    int  tmplevel=0;
    char *userh;
    char *mynick;
    char tmpbuf[mybufsize/2];
    time_t timenow;
    NickList *joiner;
    ChannelList *chan;

    chan=lookup_channel(channel,from_server,0);
    joiner=CheckJoiners(who,channel,from_server,chan);
    if (chan) chan->kick++;
    timenow=time((time_t *) 0);
    if (joiner && joiner->userhost) {
        userh=joiner->userhost;
        if (joiner->frlist) tmplevel=joiner->frlist->privs;
        else tmplevel=0;
        mynick=get_server_nickname(from_server);
        joiner=CheckJoiners(nick,channel,from_server,chan);
        if (joiner) {
            if (joiner->curk==0) joiner->kickp=0;
            if (timenow-joiner->kickt>=KickTimer) {
                joiner->curk=1;
                joiner->kickp=0;
                joiner->kickt=timenow;
            }
            else joiner->curk++;
            joiner->kick++;
        }
        if (my_stricmp(nick,mynick) && my_stricmp(nick,who)) {
            if (joiner && joiner->userhost && chan) {
                if (chan->FriendList && joiner->frlist) privs=joiner->frlist->privs;
                else privs=0;
                if (!(privs&(FLPROT | FLGOD))) {
                    if (joiner->curk>=KickSensor && joiner->curk<KickSensor*2) {
                        if (!(joiner->kickp) && chan->KickWatch) {
                            if ((chan->status)&CHAN_CHOP)
                                send_to_server("MODE %s -o %s",channel,nick);
#ifdef WANTANSI
                            say("%sMass kick%s detected on %s%s%s by %s%s%s",
                                CmdsColors[COLWARNING].color1,Colors[COLOFF],
                                CmdsColors[COLWARNING].color4,channel,Colors[COLOFF],
                                CmdsColors[COLWARNING].color2,nick,Colors[COLOFF]);
#else  /* WANTANSI */
                            say("%cMass kick%c detected on %s by %s",bold,bold,channel,
                                nick);
#endif /* WANTANSI */
                            if (away_set || LogOn) {
                                sprintf(tmpbuf,"Mass kick detected on %s by %s (%s)",
                                        channel,nick,userhost);
                                AwaySave(tmpbuf,SAVEMASS);
                            }
                        }
                        joiner->kickp=1;
                    }
                    if (joiner->curk>=KickSensor*2) {
                        if (joiner->kickp<2 && chan->KickWatch && chan->KickOnFlood) {
                            if ((chan->status)&CHAN_CHOP)
#ifdef CELE
                                send_to_server("KICK %s %s :Kick flood detected %s",
                                               channel,nick,CelerityL);
#else  /* CELE */
                                send_to_server("KICK %s %s :Kick flood detected",
                                               channel,nick);
#endif /* CELE */
#ifdef WANTANSI
                            say("%sKick flood%s detected on %s%s%s by %s%s%s",
                                CmdsColors[COLWARNING].color1,Colors[COLOFF],
                                CmdsColors[COLWARNING].color4,channel,Colors[COLOFF],
                                CmdsColors[COLWARNING].color2,nick,Colors[COLOFF]);
#else  /* WANTANSI */
                            say("%cKick flood%c detected on %s by %s",bold,bold,channel,
                                nick);
#endif /* WANTANSI */
                            if (away_set || LogOn) {
                                sprintf(tmpbuf,"Kick flood detected on %s by %s (%s)",
                                        channel,nick,userhost);
                                AwaySave(tmpbuf,SAVEMASS);
                            }
                        }
                        joiner->kickp=2;
                    }
                }
            }
            if (chan && chan->FriendList && (tmplevel&(FLPROT | FLGOD))) {
                if ((chan->status)&CHAN_CHOP) {
                    if ((tmplevel&FLGOD) && !(privs&FLGOD))
                        send_to_server("MODE %s -o %s",channel,nick);
                    if ((tmplevel&1) && ((tmplevel&(FLPROT | FLGOD)) ||
                                         ((tmplevel&FLPROT) && !(privs&FLGOD))))
                        send_to_server("INVITE %s %s",who,channel);
                }
                if (away_set || LogOn) {
                    sprintf(tmpbuf,"%s (%s) has been kicked off channel %s by %s (%s)",
                            who,userh,channel,nick,userhost);
                    AddComment(tmpbuf,comment);
                    AwaySave(tmpbuf,SAVEPROT);
                }
            }
        }
    }
    *frkick=tmplevel;
}

/* Inserts nick from tabkey list */
void InsertTabNick() {
    int curserv=from_server;
    char *nickstr;
    char *cmdchars;

    if (server_list[curserv].nickcur) {
        input_clear_line(0,(char *) 0);
        if (!(cmdchars=get_string_var(CMDCHARS_VAR))) cmdchars=DEFAULT_CMDCHARS;
        input_add_character(*cmdchars,NULL);
        input_add_character('m',NULL);
        input_add_character(' ',NULL);
        for (nickstr=(server_list[curserv].nickcur)->nick;nickstr && *nickstr;nickstr++)
            input_add_character(*nickstr,NULL);
        input_add_character(' ',NULL);
    }
}

/* Handles tab key */
void HandleTabNext() {
    int  len;
    int  curserv=from_server;
    int  dotabcompl=0;
    char *tmpstr;
    char *nickstr;
    char *cmdchars;
    char tmpbuf[mybufsize/8];
    static char *tabnick=NULL;
    Screen *curscr=current_screen;
    NickList *tmpnick;
    ChannelList *chan;
    static ChannelList *origchan=NULL;

    tmpstr=&(curscr->input_buffer[curscr->buffer_min_pos]);
    if (*tmpstr && (!my_strnicmp(tmpstr,"/m ",3) || !my_strnicmp(tmpstr,"/msg ",5))) {
        /* skip /m or /msg */
        tmpstr+=2;
        while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
        /* skip space */
        tmpstr++;
        /* skip nick */
        if (*tmpstr) {
            nickstr=tmpstr;
            while (*nickstr && !isspace(*nickstr)) nickstr++;
            if (*nickstr=='\0' || !isspace(*nickstr)) dotabcompl=1;
        }
    }
    if (dotabcompl) {
        if (*tmpstr) {
            /* option to complete channel name */
            if (*tmpstr=='#') {
                for (nickstr=tmpbuf;*tmpstr;) *nickstr++=*tmpstr++;
                *nickstr=0;
                len=strlen(tmpbuf);
                chan=server_list[curr_scr_win->server].chan_list;
                while (chan && my_strnicmp(tmpbuf,chan->channel,len)) chan=chan->next;
                if (chan && !my_strnicmp(tmpbuf,chan->channel,len)) {
                    input_clear_line(0,(char *) 0);
                    if (!(cmdchars=get_string_var(CMDCHARS_VAR)))
                        cmdchars=DEFAULT_CMDCHARS;
                    input_add_character(*cmdchars,NULL);
                    input_add_character('m',NULL);
                    input_add_character(' ',NULL);
                    for (tmpstr=chan->channel;tmpstr && *tmpstr;tmpstr++)
                        input_add_character(*tmpstr,NULL);
                }
                return;
            }
            else if (*tmpstr) {
                int nicklen;
                int sameloop=0;

                if (origchan) chan=origchan;
                else chan=server_list[from_server].chan_list;
                if (!chan) return;
                for (;chan;) {
                    if (!sameloop && tabnickcompl && tabnick) {
                        tmpnick=tabnickcompl->next;
                        len=strlen(tabnick);
                    }
                    else if (sameloop) {
                        tmpnick=chan->nicks;
                        len=tabnick?strlen(tabnick):0;
                    }
                    else {
                        char *tempnick=tmpstr;

                        for (nickstr=tmpbuf;*tempnick && !isspace(*tempnick);)
                            *nickstr++=*tempnick++;
                        *nickstr=0;
                        len=strlen(tmpbuf);
                        tmpnick=chan->nicks;
                        malloc_strcpy(&tabnick,tmpbuf);
                    }
                    for (;tmpnick;tmpnick=tmpnick->next)
                        if (!my_strnicmp(tmpnick->nick,tabnick,len)) break;
                    if (!tmpnick) {
                        if (!sameloop && !(chan->next))
                            chan=server_list[from_server].chan_list;
                        else chan=chan->next;
                        sameloop=1;
                        continue;
                    }
                    origchan=chan;
                    nicklen=strlen(tabnick);
                    if (tmpnick && strcmp(tabnick,tmpnick->nick)) {
                        input_clear_line(0,(char *) 0);
                        if (!(cmdchars=get_string_var(CMDCHARS_VAR)))
                            cmdchars=DEFAULT_CMDCHARS;
                        input_add_character(*cmdchars,NULL);
                        input_add_character('m',NULL);
                        input_add_character(' ',NULL);
                        for (tmpstr=tmpnick->nick;tmpstr && *tmpstr;tmpstr++)
                            input_add_character(*tmpstr,NULL);
                        tabnickcompl=tmpnick;
                        return;
                    }
                    chan=chan->next;
                }
            }
        }
        return;
    }
    new_free(&tabnick);
    tabnickcompl=NULL;
    if (CheckServer(curserv)) {
        if (!(server_list[curserv].nickcur))
            server_list[curserv].nickcur=server_list[curserv].nicklist;
        else NickNext();
        InsertTabNick();
    }
}

/* Handles alt-i (prev nick in nick list) */
void HandleTabPrev() {
    int curserv=from_server;
    struct nicks *tmpnick;

    if (CheckServer(curserv) && server_list[curserv].nicklist) {
        if (server_list[curserv].nickcur==server_list[curserv].nicklist) {
            for (tmpnick=(server_list[curserv].nicklist)->next;tmpnick && tmpnick->next;)
                tmpnick=tmpnick->next;
        }
        else {
            for (tmpnick=server_list[curserv].nicklist;
                 tmpnick && tmpnick->next!=server_list[curserv].nickcur;)
                tmpnick=tmpnick->next;
        }
        if (tmpnick) server_list[curserv].nickcur=tmpnick;
        InsertTabNick();
    }
}

/* Handles end of bans reply */
void EndOfBans(channel,server)
char *channel;
int  server;
{
    ChannelList *chan;

    if (unban>2) unban--;
    else unban=0;
    chan=lookup_channel(channel,server,0);
    if (!chan) return;
    if (!chan->gotbans) chan->gotbans=1;
    if (chan->gotbans && chan->gotwho) PrintSynch(chan);
}

/* Handles received notice */
int HandleNotice(nick,notice,userhost,print)
char *nick;
char *notice;
char *userhost;
int  print;
{
    int  hooked=1;
    int  savemessage=0;
    char *tmp;
    char *wallop;
    char *wallchan;
    char tmpbuf[mybufsize/2];
#ifdef CELECOSM
    char tmpbuf1[mybufsize/2];
#endif
    char tmpbuf2[mybufsize/8];
    ChannelList *chan;
    ChannelList *foundchan=NULL;

    strmcpy(tmpbuf,notice,mybufsize/16);
    upper(tmpbuf);
    if ((wallop=strstr(tmpbuf,"WALL")) && (wallchan=index(tmpbuf,'#')) &&
        wallchan-wallop<32) {
        wallchan++;
        tmp=tmpbuf2;
        *tmp++='#';
        for (wallop=wallchan;*wallop && *wallop!=' ' &&
             wallop-wallchan<32;wallop++) {
            *tmp++=*wallop;
            *tmp='\0';
            if ((chan=lookup_channel(tmpbuf2,parsing_server_index,CHAN_NOUNLINK)))
                foundchan=chan;
        }
        if (foundchan) {
            save_message_from();
            message_from(foundchan->channel,LOG_NOTICE);
            savemessage=1;
        }
    }
    /* Check for invite notices which might hold key */
    if (!print) {
        if (!strncmp(notice,"You have been ctcp invited to ",30)) AddJoinKey(1,notice);
        else if (!strncmp(notice,"Use channel key ",16)) AddJoinKey(2,notice);
        else if (!strncmp(notice,"Channel key for ",17)) AddJoinKey(3,notice);
        else if (!strncmp(notice,"Ctcp-inviting you to ",22)) AddJoinKey(4,notice);
        else if (!strncmp(notice,"The channel ",12)) AddJoinKey(5,notice);
    }
    if (!foundchan ||
        (foundchan && do_hook(CHANNEL_WALLOP,"%s %s %s",foundchan->channel,nick,
                              notice+(wallop-tmpbuf+1)))) {
        if (foundchan || (!foundchan && !print)) hooked=0;
        if (print) {
#ifdef WANTANSI
#ifdef CELECOSM
            if (ExtMes && userhost)
                ColorUserHost(userhost,CmdsColors[COLNOTICE].color6,tmpbuf2,1);
            else *tmpbuf2='\0';
            sprintf(tmpbuf,"%s-%s",CmdsColors[COLNOTICE].color5,Colors[COLOFF]);
            sprintf(tmpbuf1,"%s%s%s%s%s%s",tmpbuf,
                    CmdsColors[COLNOTICE].color1,nick,Colors[COLOFF],tmpbuf2,tmpbuf);
            put_it("%s %s%s%s",tmpbuf1,CmdsColors[COLNOTICE].color3,notice,Colors[COLOFF]);
#else  /* CELECOSM */
            sprintf(tmpbuf,"%s-%s",CmdsColors[COLNOTICE].color5,Colors[COLOFF]);
            put_it("%s%s%s%s%s %s%s%s",tmpbuf,
                   CmdsColors[COLNOTICE].color1,nick,Colors[COLOFF],tmpbuf,
                   CmdsColors[COLNOTICE].color3,notice,Colors[COLOFF]);
#endif /* CELECOSM */
#else  /* WANTANSI */
            put_it("-%s- %s",nick,notice);
#endif /* WANTANSI */
        }
    }
    sprintf(tmpbuf,"-%s- %s",nick,notice);
    malloc_strcpy(&(server_list[from_server].LastNotice),tmpbuf);
    if (!print && (away_set || LogOn))
        AwaySave(server_list[from_server].LastNotice,SAVENOTICE);
    if (savemessage) restore_message_from();
    return(!hooked);
}

/* Handles closed connections from server */
void HandleClosedConn(server,reason)
int server;
char *reason;
{
    if (away_set || LogOn) AwaySave(reason,SAVESERVER);
}

#ifdef EXTRA_STUFF
/* Handles on the fly renaming of files */
void HandleRename(dccstuff)
char **dccstuff;
{
    char tmpbuf[mybufsize/8];

    if (RenameFiles) {
        FixName(tmpbuf);
        new_free(dccstuff);
        malloc_strcpy(dccstuff,tmpbuf);
    }
}
#endif

/* Handles flooding */
void HandleFlood(nick,userhost,ignoretype)
char *nick;
char *userhost;
char *ignoretype;
{
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    struct friends *tmpfriend;

    if (!userhost) return;
    sprintf(tmpbuf1,"%s!%s",nick,userhost);
    tmpfriend=CheckUsers(tmpbuf1,NULL);
    if (!tmpfriend || !((tmpfriend->privs)&FLNOFLOOD)) {
#ifdef WANTANSI
        ColorUserHost(userhost,CmdsColors[COLWARNING].color3,tmpbuf2,1);
        sprintf(tmpbuf1,"%s%s flooding%s detected from %s%s%s %s",
                CmdsColors[COLWARNING].color1,ignoretype,Colors[COLOFF],
                CmdsColors[COLWARNING].color2,nick,Colors[COLOFF],tmpbuf2);
#else
        sprintf(tmpbuf1,"%c%s flooding%c detected from %s (%s)",
                bold,ignoretype,bold,nick,userhost);
#endif
        if (away_set || LogOn) AwaySave(tmpbuf1,SAVEFLOOD);
        if (FloodProt) {
            say("%s",tmpbuf1);
            sprintf(tmpbuf1,"%s %s",userhost,ignoretype);
            sprintf(tmpbuf2,"%d",IgnoreTime);
            ignore(NULL,tmpbuf1,tmpbuf2);
            sprintf(tmpbuf1,"-INV %d IGNORE %s NONE",IgnoreTime,userhost);
            timercmd("TIMER",tmpbuf1,NULL);
        }
    }
}

#ifdef EXTRAS
/* Locks channel mode */
void ModeLocked(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *mode=(char *) 0;
    char *tmpchannel=(char *) 0;
    ChannelList *chan;

    tmpchannel=new_next_arg(args,&args);
    mode=new_next_arg(args,&args);
    if (tmpchannel && mode) {
        for (chan=server_list[curr_scr_win->server].chan_list;chan;chan=chan->next) {
            if (CheckChannel(chan->channel,tmpchannel)) {
                malloc_strcpy(&(chan->modelock),mode);
                CheckLock(chan->channel,curr_scr_win->server,chan);
                say("Mode %s is now locked for channel %s",mode,chan->channel);
            }
        }
    }
    else PrintUsage("MODELOCK #channel mode");
}

/* Checks if there is a need to put lock mode back */
void CheckLock(channel,server,tmpchan)
char *channel;
int  server;
ChannelList *tmpchan;
{
    int  plus;
    int  count;
    char *mode;
    char *lockmode;
    char *tmpmode;
    char *tmplmode;
    char *inmode;
    char tmpbuf1[mybufsize/8];
    char tmpbuf2[mybufsize/8];
    ChannelList *chan;

    if (!tmpchan) chan=lookup_channel(channel,server,0);
    else chan=tmpchan;
    *tmpbuf1='\0';
    plus=0;
    if (chan && chan->modelock) {
        mode=recreate_mode(chan);
        lockmode=chan->modelock;
        while (*lockmode) {
            if (*lockmode=='+') plus=1;
            else if (*lockmode=='-') plus=0;
            else {
                inmode=strchr(mode,*lockmode);
                if (!plus && inmode) {
                    if (*lockmode!='k') {
                        sprintf(tmpbuf2,"-%c",*lockmode);
                        strcat(tmpbuf1,tmpbuf2);
                    }
                    else ClearKey(NULL,channel,NULL);
                }
                if (plus && !inmode) {
                    if (*lockmode!='l') {
                        sprintf(tmpbuf2,"+%c",*lockmode);
                        strcat(tmpbuf1,tmpbuf2);
                    }
                    else {
                        tmpmode=lockmode;
                        while (*tmpmode && *tmpmode!=' ') tmpmode++;
                        if (*tmpmode==' ') tmpmode++;
                        sprintf(tmpbuf2,"+l %s",tmpmode);
                        strcat(tmpbuf1,tmpbuf2);
                    }
                }
                if (plus && *lockmode=='l' && inmode) {
                    tmpmode=mode;
                    count=1;
                    while (*tmpmode!='l') {
                        if (*tmpmode=='l' || *tmpmode=='k') count++;
                        tmpmode++;
                    }
                    tmpmode=mode;
                    while (count) {
                        while (*tmpmode!=' ') tmpmode++;
                        tmpmode++;
                        count--;
                    }
                    tmplmode=lockmode;
                    while (*tmplmode!=' ') tmplmode++;
                    if (*tmplmode==' ') tmplmode++;
                    inmode=tmplmode;
                    while (*tmplmode==*tmpmode) {
                        tmplmode++;
                        tmpmode++;
                    }
                    if (*tmplmode && *tmplmode!=*tmpmode) {
                        sprintf(tmpbuf2,"+l %s",inmode);
                        strcat(tmpbuf1,tmpbuf2);
                    }
                }
            }
            lockmode++;
            if (*lockmode==' ') break;
        }
        if (chan && ((chan->status)&CHAN_CHOP) && tmpbuf1[0])
            send_to_server("MODE %s %s",channel,tmpbuf1);
    }
}
#endif

/* Handles end of who reply */
void HandleEndOfWho(channel,server)
char *channel;
int  server;
{
    ChannelList *chan;

    if (!(chan=lookup_channel(channel,server,0))) return;
    if (!chan->gotwho) chan->gotwho=1;
    if (chan->gotwho && chan->gotbans) PrintSynch(chan);
}

/* Toggles CTCP cloaking on/off */
void CTCPCloakingToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    if (*args) {
        if (!my_stricmp(args,"ON")) CTCPCloaking=1;
        else if (!my_stricmp(args,"HIDE")) CTCPCloaking=2;
        else if (!my_stricmp(args,"OFF")) CTCPCloaking=0;
        else {
            PrintUsage("CTCPCLOAK on/off/hide");
            return;
        }
    }
    if (CTCPCloaking==1) PrintSetting("CTCP cloaking","ON",empty_string,empty_string);
    else if (CTCPCloaking==2) PrintSetting("CTCP cloaking","HIDE",empty_string,empty_string);
    else PrintSetting("CTCP cloaking","OFF",empty_string,empty_string);
    update_all_status();
}

void Check4WordKick(line,joiner,privs,chan)
char *line;
NickList *joiner;
int  privs;
ChannelList *chan;
{
    char *reason;
    struct words *tmpword;

    if (chan && ((chan->status)&CHAN_CHOP) && !(privs&FLNOFLOOD) && joiner &&
        (chan->KickOps || !(joiner->chanop)) && (tmpword=CheckLine(chan->channel,line))) {
        reason=tmpword->reason;
        if (joiner->userhost && *reason=='B') {
            reason++;
            BanIt(chan->channel,joiner->nick,joiner->userhost,1,chan);
        }
#ifdef CELE
        send_to_server("KICK %s %s :%s %s",chan->channel,joiner->nick,reason,CelerityL);
#else  /* CELE */
        send_to_server("KICK %s %s :%s",chan->channel,joiner->nick,reason);
#endif /* CELE */
    }
}

/* Compare function for list.c */
int AddFirst(element,toadd)
List *element;
List *toadd;
{
    return(1);
}

/* This adds ban to ban list */
int AddBan(ban,channel,server,nick,exception,when,tmpchan)
char *ban;
char *channel;
int  server;
char *nick;
int  exception;
time_t when;
ChannelList *tmpchan;
{
    ChannelList *chan;
    struct bans *tmpban;

    if (channel && tmpchan) chan=tmpchan;
    else chan=lookup_channel(channel,server,0);
    if (chan) {
        for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next)
            if (tmpban->exception==exception && !strcmp(ban,tmpban->ban))
                break;
        if (tmpban) return(0);
        tmpban=(struct bans *) new_malloc(sizeof(struct bans));
        tmpban->ban=(char *) 0;
        tmpban->who=(char *) 0;
        malloc_strcpy(&(tmpban->ban),ban);
        if (nick) malloc_strcpy(&(tmpban->who),nick);
        tmpban->exception=exception;
        tmpban->when=when;
        tmpban->next=NULL;
        add_to_list_ext((List **) &(chan->banlist),(List *) tmpban,
                        (int (*) _((List *, List *))) AddFirst);
    }
    return(1);
}

/* This removes ban from ban list */
int RemoveBan(ban,exception,chan)
char *ban;
int exception;
ChannelList *chan;
{
    struct bans *tmpban=NULL;
    struct bans *tmpbanprev=NULL;

    if (!chan) return(1);
    if (chan) {
        for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next) {
            if (tmpban->exception==exception && !strcmp(ban,tmpban->ban))
                break;
            tmpbanprev=tmpban;
        }
        if (tmpban) {
            if (tmpbanprev) tmpbanprev->next=tmpban->next;
            else chan->banlist=tmpban->next;
        }
    }
    if (tmpbn==tmpban) tmpbn=NULL;
    if (tmpban) {
        new_free(&(tmpban->ban));
        new_free(&(tmpban->who));
        new_free(&tmpban);
        return(1);
    }
    return(0);
}

/* This really unbans user */
void UnbanIt(pattern,channel,server)
char *pattern;
char *channel;
int  server;
{
    int  count=0;
    int  max=get_int_var(MAX_MODES_VAR);
    char *banmodes=(char *) 0;
    char tmpbuf[mybufsize/2];
    char modebuf[mybufsize/32];
    ChannelList *chan;
    struct bans *tmpban;

    chan=lookup_channel(channel,server,0);
    if (!chan) return;
    strcpy(modebuf,"-bbbbbbbb");
    for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next) {
        if (tmpban->exception) continue;
        if (wild_match(pattern,tmpban->ban) || wild_match(tmpban->ban,pattern)) {
            sprintf(tmpbuf," %s",tmpban->ban);
            malloc_strcat(&banmodes,tmpbuf);
            count++;
        }
        if (count==max) {
            modebuf[count+1]='\0';
            send_to_server("MODE %s %s %s",channel,modebuf,banmodes);
            new_free(&banmodes);
            count=0;
        }
    }
    if (count) {
        modebuf[count+1]='\0';
        send_to_server("MODE %s %s %s",channel,modebuf,banmodes);
        new_free(&banmodes);
    }
}

/* Shows all bans and asks which one you wanna remove */
void TBan(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char tmpbuf[mybufsize/8];
    ChannelList *chan;

    if ((channel=new_next_arg(args,&args))) {
        if (is_channel(channel)) strcpy(tmpbuf,channel);
        else sprintf(tmpbuf,"#%s",channel);
        channel=tmpbuf;
    }
    else if (!(channel=get_channel_by_refnum(0))) {
        NoWindowChannel();
        return;
    }
    if (!(chan=lookup_channel(channel,from_server,0))) {
        say("You are not on channel %s",channel);
        return;
    }
    if (chan && ((chan->status)&CHAN_CHOP)) {
        tmpbanlist=chan->banlist;
        tmpbn=tmpbanlist;
        listcount=0;
        ListBansPage(chan->channel);
    }
    else NotChanOp(chan->channel);
}

/* Lists one page of bans list */
void ListBansPage(line)
char *line;
{
    int count=1;

    while (count<curr_scr_win->display_size && tmpbanlist) {
        if (!(tmpbanlist->exception)) {
            count++;
            if (!listcount) say("Listing all bans on channel %s",line);
            listcount++;
            if (tmpbanlist->who && tmpbanlist->when)
                say("#%-2d %s by %s %s",listcount,tmpbanlist->ban,tmpbanlist->who,
                        FormatTime(time((time_t *) 0)-tmpbanlist->when));
            else say("#%-2d %s",listcount,tmpbanlist->ban);
        }
        tmpbanlist=tmpbanlist->next;
    }
    if (!tmpbanlist) {
        if (listcount) {
            say("Total of %d bans",listcount);
            add_wait_prompt("Press ENTER to quit or filter (-2,4,6-8,10- or *) : ",ListBansPrompt,line,WAIT_PROMPT_LINE);
        }
        else say("There are no bans on channel %s",line);
    }
    else if (count>=curr_scr_win->display_size) add_wait_prompt("Press ENTER to quit or filter (-2,4,6-8,10- or *) : ",ListBansPrompt,line,WAIT_PROMPT_LINE);
}

/* This waits for line input */
void ListBansPrompt(stuff,line)
char *stuff;
char *line;
{
    int  max=get_int_var(MAX_MODES_VAR);
    int  count=0;
    int  number=0;
    char *banmodes=(char *) 0;
    char tmpbuf[mybufsize/2];
    char modebuf[mybufsize/32];

    if (line && *line) {
        strcpy(modebuf,"-bbbbbbbb");
        for (tmpbanlist=tmpbn;tmpbanlist;tmpbanlist=tmpbanlist->next) {
            if (tmpbanlist->exception) continue;
            count++;
            if (matchmcommand(line,count)) {
                sprintf(tmpbuf," %s",tmpbanlist->ban);
                malloc_strcat(&banmodes,tmpbuf);
                number++;
            }
            if (number==max) {
                modebuf[number+1]='\0';
                send_to_server("MODE %s %s %s",stuff,modebuf,banmodes);
                new_free(&banmodes);
                number=0;
            }
        }
        if (number) {
            modebuf[number+1]='\0';
            send_to_server("MODE %s %s %s",stuff,modebuf,banmodes);
            new_free(&banmodes);
        }
    }
    else if (tmpbanlist) ListBansPage(stuff);
}

/* Handles fake modes */
void HandleFakes(line,from,server)
char *line;
char *from;
int  server;
{
    char *tmpstr=(char *) 0;
    char *tmpnick=(char *) 0;
    char *tmpchan=(char *) 0;
    char *tmpwhat=(char *) 0;
    char *tmparg;
    char tmpbuf1[mybufsize/2];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/2];
#endif
    ChannelList *chan;

    strcpy(tmpbuf1,line);
    tmparg=tmpbuf1;
    tmpstr=new_next_arg(tmparg,&tmparg);
    tmpstr=new_next_arg(tmparg,&tmparg);
    tmpstr=new_next_arg(tmparg,&tmparg);
    tmpstr=new_next_arg(tmparg,&tmparg);
    tmpnick=new_next_arg(tmparg,&tmparg);
    tmpstr=new_next_arg(tmparg,&tmparg);
    tmpchan=new_next_arg(tmparg,&tmparg);
    tmpwhat=tmpchan;
    while (*tmpwhat) tmpwhat++;
    tmpwhat++;
    tmparg=tmpwhat;
    while (*tmparg) tmparg++;
    tmparg--;
    *tmparg=0;
    for (chan=server_list[server].chan_list;chan;chan=chan->next)
        if (chan->ShowFakes && server==chan->server && !my_stricmp(tmpchan,chan->channel)) {
#ifdef WANTANSI
            sprintf(tmpbuf2,"%sFake%s %s%s%s %sMODE%s %s",
                    CmdsColors[COLMODE].color6,Colors[COLOFF],
                    CmdsColors[COLMODE].color1,tmpnick,Colors[COLOFF],
                    CmdsColors[COLMODE].color5,Colors[COLOFF],
                    CmdsColors[COLMODE].color3);
            sprintf(tmpbuf1,"%s%s%s \"%s%s%s\" from %s%s%s",
                    tmpbuf2,tmpchan,Colors[COLOFF],
                    CmdsColors[COLMODE].color4,tmpwhat,Colors[COLOFF],
                    CmdsColors[COLMODE].color2,from,Colors[COLOFF]);
#else
            sprintf(tmpbuf1,"%cFake%c %s MODE %s %s from %s",
                    bold,bold,tmpnick,tmpchan,tmpwhat,from);
#endif
            say("%s",tmpbuf1);
            if (away_set || LogOn) AwaySave(tmpbuf1,SAVEFAKE);
            return;
        }
}

#ifdef EXTRAS
/* Resets channel's lock mode */
void ModeUnlocked(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char tmpbuf[mybufsize/4];
    ChannelList *chan;

    if ((channel=new_next_arg(args,&args))) {
        if (is_channel(channel)) strcpy(tmpbuf,channel);
        else sprintf(tmpbuf,"#%s",channel);
        channel=tmpbuf;
    }
    else if ((channel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    chan=lookup_channel(channel,curr_scr_win->server,0);
    if (!chan) {
        say("You're not on channel %s on this server",channel);
        return;
    }
    if (chan->modelock) {
        say("Mode lock %s has been removed for channel %s",chan->modelock,channel);
        new_free(&(chan->modelock));
    }
}
#endif

#ifdef SCKICKS
/* Kicks nick with funny message */
void ScatterKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  number=0;
    int  kicked=0;
    char *channel;
    char *tmpnick=(char *) 0;
    char *tmpkick=(char *) 0;
    char *tmpcommand=(char *) 0;
    char *tmpstr;
    char *tmpstr1;
    char *comment;
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/2];
    char tmpbuf3[mybufsize/2];
    NickList *joiner;

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (NumberOfScatterKicks) {
            tmpkick=new_next_arg(args,&args);
            tmpnick=new_next_arg(args,&args);
            if (tmpnick && tmpkick) {
                if (is_chanop(channel,get_server_nickname(from_server))) {
                    number=atoi(tmpkick);
                    if (number>0 && number<=NumberOfScatterKicks) {
                        number--;
                        strcpy(tmpbuf1,PickScatterKick(number));
                        tmpstr=tmpbuf1;
                        joiner=CheckJoiners(tmpnick,channel,from_server,NULL);
                        if (joiner) {
                            while (1) {
                                tmpcommand=new_next_arg(tmpstr,&tmpstr);
                                if (!tmpcommand) break;
                                *tmpbuf2='\0';
                                if (!my_stricmp("$M",tmpcommand)) {
                                    do {
                                        tmpstr1=new_next_arg(tmpstr,&tmpstr);
                                        if (!tmpstr1) break;
                                        if (*tmpstr1=='$' && *(tmpstr1+1)=='0') {
                                            tmpstr1++;
                                            tmpstr1++;
                                            if (!joiner) strcat(tmpbuf2,tmpnick);
                                            else strcat(tmpbuf2,joiner->nick);
                                            strcat(tmpbuf2,tmpstr1);
                                        }
                                        else if (!my_stricmp("$C",tmpstr1)) strcat(tmpbuf2,channel);
                                        else if (*tmpstr1=='$' && *(tmpstr1+1)=='N') {
                                            tmpstr1++;
                                            tmpstr1++;
                                            strcat(tmpbuf2,get_server_nickname(from_server));
                                            strcat(tmpbuf2,tmpstr1);
                                        }
                                        else if (*tmpstr1!=';') strcat(tmpbuf2,tmpstr1);
                                        strcat(tmpbuf2," ");
                                    } while (tmpstr1 && *tmpstr1!=';');
                                    if (!kicked) me(NULL,tmpbuf2,NULL);
                                    else {
                                        sprintf(tmpbuf3,"-CMD ME %s",tmpbuf2);
                                        waitcmd(NULL,tmpbuf3,NULL);
                                    }
                                }
                                if (!my_stricmp("$S",tmpcommand)) {
                                    do {
                                        tmpstr1=new_next_arg(tmpstr,&tmpstr);
                                        if (!tmpstr1) break;
                                        if (*tmpstr1=='$' && *(tmpstr1+1)=='0') {
                                            tmpstr1++;
                                            tmpstr1++;
                                            if (!joiner) strcat(tmpbuf2,tmpnick);
                                            else strcat(tmpbuf2,joiner->nick);
                                            strcat(tmpbuf2,tmpstr1);
                                        }
                                        else if (!my_stricmp("$C",tmpstr1)) strcat(tmpbuf2,channel);
                                        else if (*tmpstr1=='$' && *(tmpstr1+1)=='N') {
                                            tmpstr1++;
                                            tmpstr1++;
                                            strcat(tmpbuf2,get_server_nickname(from_server));
                                            strcat(tmpbuf2,tmpstr1);
                                        }
                                        else if (*tmpstr1!=';') strcat(tmpbuf2,tmpstr1);
                                        strcat(tmpbuf2," ");
                                    } while (tmpstr1 && *tmpstr1!=';');
                                    if (!kicked) send_text(channel,tmpbuf2,"PRIVMSG");
                                    else {
                                        sprintf(tmpbuf3,"-CMD MSG %s %s",channel,tmpbuf2);
                                        waitcmd(NULL,tmpbuf3,NULL);
                                    }
                                }
                                if (!my_stricmp("$K",tmpcommand)) {
                                    do {
                                        tmpstr1=new_next_arg(tmpstr,&tmpstr);
                                        if (!tmpstr1) break;
                                        if (*tmpstr1=='$' && *(tmpstr1+1)=='0') {
                                            tmpstr1++;
                                            tmpstr1++;
                                            if (!joiner) strcat(tmpbuf2,tmpnick);
                                            else strcat(tmpbuf2,joiner->nick);
                                            strcat(tmpbuf2,tmpstr1);
                                        }
                                        else if (!my_stricmp("$C",tmpstr1)) strcat(tmpbuf2,channel);
                                        else if (*tmpstr1=='$' && *(tmpstr1+1)=='N') {
                                            tmpstr1++;
                                            tmpstr1++;
                                            strcat(tmpbuf2,get_server_nickname(from_server));
                                            strcat(tmpbuf2,tmpstr1);
                                        }
                                        else if (*tmpstr1!=';') strcat(tmpbuf2,tmpstr1);
                                        strcat(tmpbuf2," ");
                                    } while (tmpstr1 && *tmpstr1!=';');
                                    while (tmpbuf2[strlen(tmpbuf2)-1]==' ') tmpbuf2[strlen(tmpbuf2)-1]=0;
                                    if (tmpbuf2[0]) comment=tmpbuf2;
                                    else comment=DefaultSK;
#ifdef CELE
                                    send_to_server("KICK %s %s :%s %s",channel,
                                                   joiner->nick,comment,CelerityL);
#else  /* CELE */
                                    send_to_server("KICK %s %s :%s",channel,
                                                   joiner->nick,comment);
#endif /* CELE */
                                    kicked=1;
                                }
                            }
                        }
                        else say("Can't find %s on %s",tmpnick,channel);
                    }
                    else {
                        sprintf(tmpbuf1,"SK # nick (# is from 1 to %d)",NumberOfScatterKicks);
                        PrintUsage(tmpbuf1);
                    }
                }
                else NotChanOp(channel);
            }
            else {
                sprintf(tmpbuf1,"SK # nick (# is from 1 to %d)",NumberOfScatterKicks);
                PrintUsage(tmpbuf1);
            }
        }
        else say("No scatter kicks defined");
    }
    else NoWindowChannel();
}

/* Randomly picks a funny message and kicks nick */
void RandomScatterKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  number=0;
    char *channel;
    char *tmpnick=(char *) 0;
    char tmpbuf[mybufsize/16];

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (NumberOfScatterKicks) {
            tmpnick=new_next_arg(args,&args);
            if (tmpnick) {
                srand(time(0));
                number=rand()%NumberOfScatterKicks+1;
                sprintf(tmpbuf,"%d %s",number,tmpnick);
                ScatterKick(NULL,tmpbuf,NULL);
            }
            else PrintUsage("RANSK nick");
        }
        else say("No scatter kicks defined");
    }
    else NoWindowChannel();
}
#endif

#ifdef EXTRAS
/* Kicks nick with last notice */
void LastNoticeKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *tmpnick;
    char *notice=server_list[from_server].LastNotice;
    NickList *joiner;
    ChannelList *chan;

    if (!(*args)) PrintUsage("DIRLNK nick");
    else {
        channel=get_channel_by_refnum(0);
        if (channel) {
            chan=lookup_channel(channel,curr_scr_win->server,0);
            if (chan && ((chan->status)&CHAN_CHOP)) {
                tmpnick=new_next_arg(args,&args);
                joiner=CheckJoiners(tmpnick,channel,curr_scr_win->server,chan);
                if (joiner) {
#ifdef CELE
                    if (notice) send_to_server("KICK %s %s :%s %s",
                                               channel,tmpnick,notice,CelerityL);
                    else send_to_server("KICK %s %s :%s %s",
                                        channel,tmpnick,DefaultK,CelerityL);
#else  /* CELE */
                    if (notice) send_to_server("KICK %s %s :%s",channel,tmpnick,notice);
                    else send_to_server("KICK %s %s :%s",channel,tmpnick,DefaultK);
#endif /* CELE */
                }
                else say("Can't find %s on %s.",tmpnick,channel);
            }
            else NotChanOp(channel);
        }
        else NoWindowChannel();
    }
}
#endif

/* Prints some statistics about nick */
void NickStat(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  found=0;
    char *tmpnick;
    NickList *joiner;
    ChannelList *chan;

    if ((tmpnick=new_next_arg(args,&args))) {
        for (chan=server_list[curr_scr_win->server].chan_list;chan;chan=chan->next) {
            for (joiner=chan->nicks;joiner;joiner=joiner->next) {
                if (wild_match(tmpnick,joiner->nick)) {
                    if (!found) {
                        say("Nick      Channel             +o    -o    +b    -b   kicks  nicks   pub");
                        found=1;
                    }
                    say("%-9.9s %-17.17s %4d  %4d  %4d  %4d   %5d  %5d  %4d",
                        joiner->nick,chan->channel,joiner->pluso,joiner->minuso,
                        joiner->plusb,joiner->minusb,joiner->kick,
                        joiner->nickc,joiner->publics);
                }
            }
        }
        if (!found) say("No nicks matched %s",tmpnick);
    }
    else PrintUsage("NWHOIS filter");
}

/* Adds nick to notify list */
void AddNotify(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;

    if (*args) {
        while ((tmpnick=new_next_arg(args,&args)))
            notify(NULL,tmpnick,NULL);
    }
    else PrintUsage("ADDN nick [nick ...]");
}

/* Removes nick from notify list */
void RemoveNotify(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;
    char tmpbuf[mybufsize/4];

    if (*args) {
        *tmpbuf='\0';
        while ((tmpnick=new_next_arg(args,&args))) {
            strcat(tmpbuf," -");
            strcat(tmpbuf,tmpnick);
        }
        notify(NULL,tmpbuf,NULL);
    }
    else PrintUsage("REMN nick [nick ...]");
}

/* Handles notify reply */
void HandleNotifyOn(nick,server)
char *nick;
int  server;
{
    AddDelayNotify(nick);
}

/* Handles userhost reply */
void HandleUserhost(wistuff,tmpnick,text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    char tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/4];
#endif
    NotifyList *notify;
#ifdef WANTANSI
    char   *colnick;
#else
    time_t timenow;

    timenow=time((time_t *) 0);
#endif
    inFlierNotify--;
    if (inFlierNotify==1) inFlierNotify=0;
    if (!wistuff->nick) return;
    notify_mark(wistuff->nick,2+wistuff->not_on,0);
    if (wistuff->not_on) return;
    sprintf(tmpbuf1,"%s@%s",wistuff->user,wistuff->host);
#if defined(EXTRAS) || defined(FLIER)
    CheckInvite(wistuff->nick,tmpbuf1,parsing_server_index);
#endif
    notify=(NotifyList *) list_lookup((List **) &notify_list,wistuff->nick,
                                      !USE_WILDCARDS,!REMOVE_FROM_LIST);
    if (notify) {
        malloc_strcpy(&(notify->nick),wistuff->nick);
        malloc_strcpy(&(notify->userhost),tmpbuf1);
        if (notify->mask) {
            char tmpbuf3[mybufsize/4];
            char tmpbuf4[mybufsize/4];

            sprintf(tmpbuf3,"%s!%s",notify->nick,notify->mask);
            sprintf(tmpbuf4,"%s!%s",notify->nick,notify->userhost);
            if (!wild_match(tmpbuf3,tmpbuf4)) return;
        }
    }
    if (do_hook(NOTIFY_SIGNON_UH_LIST,"%s %s %s",wistuff->nick,wistuff->user,wistuff->host)) {
#ifdef WANTANSI
        sprintf(tmpbuf2,"%s!%s",wistuff->nick,tmpbuf1);
        if (CheckUsers(tmpbuf2,NULL)) {
            notify->isfriend=1;
            colnick=CmdsColors[COLNOTIFY].color6;
        }
        else colnick=CmdsColors[COLNOTIFY].color1;
        ColorUserHost(tmpbuf1,CmdsColors[COLNOTIFY].color2,tmpbuf2,1);
#ifdef CELECOSM
        sprintf(tmpbuf1,"[sign%son%s]  %s%s%s %s ",
                CmdsColors[COLNOTIFY].color4,Colors[COLOFF],
                colnick,wistuff->nick,Colors[COLOFF],tmpbuf2);
        put_it("%s %s<%s%s%s>",CelerityNtfy,tmpbuf1,
               CmdsColors[COLNOTIFY].color3,update_clock(GET_TIME),Colors[COLOFF]);
#else  /* CELECOSM */
        sprintf(tmpbuf1,"Sign%sOn%s detected: %s%s%s %s ",
                CmdsColors[COLNOTIFY].color4,Colors[COLOFF],
                colnick,wistuff->nick,Colors[COLOFF],tmpbuf2);
        put_it("%s %s[%s%s%s]",CelerityNtfy,tmpbuf1,
               CmdsColors[COLNOTIFY].color3,update_clock(GET_TIME),Colors[COLOFF]);
#endif /* CELECOSM */
#else  /* WANTANSI */
        put_it("%s SignOn detected: %s (%s) [%s]",CelerityNtfy,
               wistuff->nick,tmpbuf1,update_clock(GET_TIME));
#endif /* WANTANSI */
    }
    sprintf(tmpbuf1,"Signon by %s (%s@%s) detected",wistuff->nick,wistuff->user,
            wistuff->host);
    AwaySave(tmpbuf1,SAVENOTIFY);
}

/* Handles notify off reply */
void HandleNotifyOff(nick,timenow)
char *nick;
time_t timenow;
{
    char tmpbuf1[mybufsize/4];

#ifdef WANTANSI
#ifdef CELECOSM
    put_it("%s [sign%soff%s]  %s%s%s <%s%s%s>",CelerityNtfy,
           CmdsColors[COLNOTIFY].color4,Colors[COLOFF],
           CmdsColors[COLNOTIFY].color1,nick,Colors[COLOFF],
           CmdsColors[COLNOTIFY].color3,update_clock(GET_TIME),Colors[COLOFF]);
#else  /* CELECOSM */
    put_it("%s Sign%sOff%s detected: %s%s%s [%s%s%s]",CelerityNtfy,
           CmdsColors[COLNOTIFY].color4,Colors[COLOFF],
           CmdsColors[COLNOTIFY].color1,nick,Colors[COLOFF],
           CmdsColors[COLNOTIFY].color3,update_clock(GET_TIME),Colors[COLOFF]);
#endif /* CELECOSM */
#else  /* WANTANSI */
    put_it("%s SignOff detected: %s [%s]",CelerityNtfy,nick,update_clock(GET_TIME));
#endif /* WANTANSI */
    sprintf(tmpbuf1,"Signoff by %s detected",nick);
    AwaySave(tmpbuf1,SAVENOTIFY);
}

/* Handles notify userhost off reply */
void HandleNotifyOffUh(nick,userhost,mask,timenow,isfriend)
char *nick;
char *userhost;
char *mask;
time_t timenow;
int isfriend;
{
    char tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char *colnick;
    char tmpbuf2[mybufsize/4];
#endif

    if (mask) {
        char tmpbuf3[mybufsize/4];

        sprintf(tmpbuf1,"%s!%s",nick,mask);
        sprintf(tmpbuf3,"%s!%s",nick,userhost);
        if (!wild_match(tmpbuf1,tmpbuf3)) return;
    }
#ifdef WANTANSI
    if (isfriend) colnick=CmdsColors[COLNOTIFY].color6;
    else colnick=CmdsColors[COLNOTIFY].color1;
    ColorUserHost(userhost,CmdsColors[COLNOTIFY].color2,tmpbuf2,1);
#ifdef CELECOSM
    sprintf(tmpbuf1,"%s [sign%soff%s]  %s%s%s %s",CelerityNtfy,
            CmdsColors[COLNOTIFY].color4,Colors[COLOFF],
            colnick,nick,Colors[COLOFF],tmpbuf2);
    put_it("%s <%s%s%s>",tmpbuf1,
           CmdsColors[COLNOTIFY].color3,update_clock(GET_TIME),Colors[COLOFF]);
#else  /* CELECOSM */
    sprintf(tmpbuf1,"%s Sign%sOff%s detected: %s%s%s %s",CelerityNtfy,
            CmdsColors[COLNOTIFY].color4,Colors[COLOFF],
            colnick,nick,Colors[COLOFF],tmpbuf2);
    put_it("%s [%s%s%s]",tmpbuf1,
           CmdsColors[COLNOTIFY].color3,update_clock(GET_TIME),Colors[COLOFF]);
#endif /* CELECOSM */
#else  /* WANTANSI */
    put_it("%s SignOff detected: %s (%s) [%s]",CelerityNtfy,
           nick,userhost,update_clock(GET_TIME));
#endif /* WANTANSI */
    sprintf(tmpbuf1,"Signoff by %s (%s) detected",nick,userhost);
    AwaySave(tmpbuf1,SAVENOTIFY);
}

/* Lists all users on notify list */
void ListNotify(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  count=0;
    int  maskmatch;
    char *on=(char *) 0;
    char *noton=(char *) 0;
#ifdef WANTANSI
    char *ncolor;
#endif
    char tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/4];
#endif
    NotifyList *notify;

    say("Listing all the people on your notify list");
    for (notify=notify_list;notify;notify=notify->next) {
        if (notify->flag && notify->flag!=2) {
#ifdef WANTANSI
            ncolor=notify->isfriend?
                CmdsColors[COLNOTIFY].color6:CmdsColors[COLNOTIFY].color5;
#endif
            maskmatch=1;
            if (notify->mask) {
                char tmpbuf3[mybufsize/4];

                sprintf(tmpbuf1,"%s!%s",notify->nick,notify->mask);
                sprintf(tmpbuf3,"%s!%s",notify->nick,notify->userhost);
                if (!wild_match(tmpbuf1,tmpbuf3)) maskmatch=0;
            }
            if (NotifyMode==2) {
#ifdef CELECOSM
                if (!count) strcpy(tmpbuf1,"/present/ ");
#else
                if (!count) strcpy(tmpbuf1,"(Present) ");
#endif /* CELECOSM */
                else strcpy(tmpbuf1,"          ");
#ifdef WANTANSI
                if (notify->userhost && maskmatch) {
                    ColorUserHost(notify->userhost,CmdsColors[COLNOTIFY].color2,tmpbuf2,1);
                    say("%s %s%-13s%s %s",tmpbuf1,
                        ncolor,notify->nick,Colors[COLOFF],tmpbuf2);
                }
                else say("%s %s%-13s%s",tmpbuf1,ncolor,notify->nick,Colors[COLOFF]);
#else
                if (notify->userhost && maskmatch)
                    say("%s %c%-13s%c [%s]",tmpbuf1,bold,notify->nick,bold,
                        notify->userhost);
                else say("%s %c%-13s%c",tmpbuf1,bold,notify->nick,bold);
#endif
            }
            else {
#ifdef CELECOSM
                if (!on) malloc_strcpy(&on,"/present/ ");
#else
                if (!on) malloc_strcpy(&on,"(Present) ");
#endif /* CELECOSM */
#ifdef WANTANSI
                sprintf(tmpbuf1," %s%s%s",ncolor,notify->nick,Colors[COLOFF]);
#else
                sprintf(tmpbuf1," %s",notify->nick);
#endif
                malloc_strcat(&on,tmpbuf1);
            }
            count++;
        }
        else {
#ifdef CELECOSM
            if (!noton) malloc_strcpy(&noton,"/missing/ ");
#else
            if (!noton) malloc_strcpy(&noton,"(Absent ) ");
#endif /* CELECOSM */
#ifdef WANTANSI
            sprintf(tmpbuf1," %s%s%s",CmdsColors[COLNOTIFY].color1,
                    notify->nick,Colors[COLOFF]);
#else
            sprintf(tmpbuf1," %s",notify->nick);
#endif
            malloc_strcat(&noton,tmpbuf1);
        }
    }
    if (on) {
        say("%s",on);
        new_free(&on);
    }
    if (noton) {
        say("%s",noton);
        new_free(&noton);
    }
}

/* Clears bans in ChannelList */
/* Also clears hash table */
void ClearBans(chan)
ChannelList *chan;
{
    int i;
    struct bans *tmpban;
    struct bans *tmpbant;
    struct hashstr *tmp;

    for (tmpbant=chan->banlist;tmpbant;) {
        tmpban=tmpbant;
        tmpbant=tmpbant->next;
        new_free(&(tmpban->ban));
        new_free(&(tmpban->who));
        new_free(&tmpban);
    }
    chan->banlist=(struct bans *) 0;
    for (i=0;i<HASHTABLESIZE;i++) {
        for (;chan->nickshash[i];) {
            tmp=chan->nickshash[i];
            chan->nickshash[i]=chan->nickshash[i]->next;
            new_free(&tmp);
        }
        chan->nickshash[i]=(struct hashstr *) 0;
    }
}

/* Handles all things when you get ops */
void HandleGotOps(mynick,chan)
char *mynick;
ChannelList *chan;
{
    int  done;
    int  count=0;
    int  opped;
    int  countv=0;
    int  max=get_int_var(MAX_MODES_VAR);
    char *tmpmode=(char *) 0;
    char *tmpmodev=(char *) 0;
    char *reason;
    char tmpbuf[mybufsize/4];
    char modebuf[mybufsize/32];
    char modebufv[mybufsize/32];
    NickList *tmp;
    WhowasList *whowas;
    struct bans *tmpban;

    if (chan && chan->FriendList) {
        count=0;
        *modebuf='\0';
        *modebufv='\0';
        for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next) {
            if (tmpban->exception) continue;
            for (tmp=chan->nicks;tmp;tmp=tmp->next) {
                if (tmp->frlist && ((tmp->frlist->privs)&(FLPROT | FLGOD)) &&
                    ((tmp->frlist->privs)&FLUNBAN)) {
                    sprintf(tmpbuf,"%s!%s",tmp->nick,tmp->userhost);
                    if (wild_match(tmpban->ban,tmpbuf)) {
                        count++;
                        strcat(modebuf,"-b");
                        sprintf(tmpbuf," %s",tmpban->ban);
                        malloc_strcat(&tmpmode,tmpbuf);
                        break;
                    }
                }
            }
            if (!tmp) {
                for (whowas=whowas_userlist_list;whowas;whowas=whowas->next) {
                    if (whowas->nicklist->frlist && ((whowas->nicklist->frlist->privs)&(FLPROT | FLGOD))
                       && ((whowas->nicklist->frlist->privs)&FLUNBAN)) {
                        if (whowas->nicklist->userhost)
                            sprintf(tmpbuf,"%s!%s",whowas->nicklist->nick,whowas->nicklist->userhost);
                        else strcpy(tmpbuf,whowas->nicklist->nick);
                        if (wild_match(tmpban->ban,tmpbuf)) {
                            count++;
                            strcat(modebuf,"-b");
                            sprintf(tmpbuf," %s",tmpban->ban);
                            malloc_strcat(&tmpmode,tmpbuf);
                        }
                        break;
                    }
                }
            }
            if (count==max) {
                count=0;
                send_to_server("MODE %s %s %s",chan->channel,modebuf,tmpmode);
                new_free(&tmpmode);
                *modebuf='\0';
            }
        }
        if (count) {
            count=0;
            send_to_server("MODE %s %s %s",chan->channel,modebuf,tmpmode);
            new_free(&tmpmode);
            *modebuf='\0';
        }
        for (tmp=chan->nicks;tmp;tmp=tmp->next) {
            opped=0;
            if (!(tmp->chanop)) {
                if (tmp->frlist &&
                   ((tmp->frlist->privs)&FLOP) && !(tmp->frlist->passwd) &&
                   (((tmp->frlist->privs)&FLAUTOOP) || ((tmp->frlist->privs)&FLINSTANT)))
                {
                    count++;
                    opped=1;
                    strcat(modebuf,"+o");
                    sprintf(tmpbuf," %s",tmp->nick);
                    malloc_strcat(&tmpmode,tmpbuf);
                    if (count==max) {
                        count=0;
                        send_to_server("MODE %s %s %s",chan->channel,modebuf,tmpmode);
                        new_free(&tmpmode);
                        *modebuf='\0';
                    }
                }
            }
            if (!opped && !(tmp->chanop) && !(tmp->voice)) {
                if (tmp->frlist && ((tmp->frlist->privs)&FLVOICE) &&
                    (((tmp->frlist->privs)&FLAUTOOP) || ((tmp->frlist->privs)&FLINSTANT)))
                {
                    countv++;
                    strcat(modebufv,"+v");
                    sprintf(tmpbuf," %s",tmp->nick);
                    malloc_strcat(&tmpmodev,tmpbuf);
                    if (countv==max) {
                        countv=0;
                        send_to_server("MODE %s %s %s",chan->channel,modebufv,
                                       tmpmodev);
                        new_free(&tmpmodev);
                        *modebufv='\0';
                    }
                }
            }
        }
        if (count) {
            send_to_server("MODE %s %s %s",chan->channel,modebuf,tmpmode);
            new_free(&tmpmode);
        }
        if (countv) {
            send_to_server("MODE %s %s %s",chan->channel,modebufv,tmpmodev);
            new_free(&tmpmodev);
        }
        *modebuf='\0';
        count=0;
        for (tmp=chan->nicks;tmp;tmp=tmp->next) {
            done=0;
            if (chan->Bitch && my_stricmp(tmp->nick,mynick) && tmp->chanop &&
                (!tmp->frlist || (tmp->frlist &&
                 !((tmp->frlist->privs)&(FLOP|FLAUTOOP|FLINSTANT))))) {
                strcat(modebuf,"-o");
                sprintf(tmpbuf," %s",tmp->nick);
                malloc_strcat(&tmpmode,tmpbuf);
                count++;
                done=1;
            }
            if (tmp->shitlist && tmp->shitlist->shit) {
                reason=(char *) 0;
                if ((tmp->shitlist->shit)&SLDEOP) {
                    if (!done && tmp->chanop) {
                        strcat(modebuf,"-o");
                        sprintf(tmpbuf," %s",tmp->nick);
                        malloc_strcat(&tmpmode,tmpbuf);
                        count++;
                    }
                }
                if ((tmp->shitlist->shit)&SLKICK) {
                    if (tmp->shitlist->reason[0]) reason=tmp->shitlist->reason;
                    else reason=DefaultABK;
#ifdef CELE
                    send_to_server("KICK %s %s :%s %s",chan->channel,tmp->nick,reason,
                                   CelerityL);
#else  /* CELE */
                    send_to_server("KICK %s %s :%s",chan->channel,tmp->nick,reason);
#endif /* CELE */
                }
                if ((tmp->shitlist->shit)&SLBAN)
                    send_to_server("MODE %s -o+b %s %s",chan->channel,
                                   tmp->nick,tmp->shitlist->userhost);

                if ((tmp->shitlist->shit)&SLIGNORE) {
                    sprintf(tmpbuf,"%s ALL",tmp->userhost);
                    Ignore(NULL,tmpbuf,tmpbuf);
                    break;
                }
            }
            if (count==max) {
                send_to_server("MODE %s %s %s",chan->channel,modebuf,tmpmode);
                new_free(&tmpmode);
                count=0;
                *modebuf='\0';
            }
        }
        if (count) {
            send_to_server("MODE %s %s %s",chan->channel,modebuf,tmpmode);
            new_free(&tmpmode);
        }
        CheckPermBans(chan);
#ifdef EXTRAS
        CheckLock(chan->channel,from_server,chan);
#endif
    }
}

/* This executes when you quit from IRC */
void MyQuit(reason)
char *reason;
{
    int active=0;
    DCC_list *client;

    for (client=ClientList;client;client=client->next)
        active|=(client->flags)&DCC_ACTIVE;
    if (active) {
        if (get_int_var(BEEP_VAR)) term_beep();
#ifdef WANTANSI
        say("You have DCCs %sactive%s",CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
        say("You have DCCs %cactive%c",bold,bold);
#endif
        Cdcc(NULL,NULL,NULL);
        add_wait_prompt("Press any key to continue, 'q' to quit from IRC",MyQuitPrompt,reason,WAIT_PROMPT_KEY);
    }
    else MyQuitPrompt(reason,"q");
}

/* Prompt when you quit if you have dccs pending */
void MyQuitPrompt(reason,line)
char *reason;
char *line;
{
    int max;
    int i;
    char *tmpstr=NULL;
    char *mynick;
#ifdef WANTANSI
    char *colnick;
    NickList *joiner;
#endif
    char tmpbuf1[mybufsize/4+1];
    char tmpbuf2[mybufsize/4+1];

    if (reason) tmpstr=reason;
#ifdef CELE
    else tmpstr=CeleSignoff();
#else
    else if (NumberOfSignOffMsgs) tmpstr=PickSignOff();
#endif
    if (!tmpstr) {
        if (DefaultSignOff && *DefaultSignOff) tmpstr=DefaultSignOff;
        else tmpstr=(char *) "Leaving";
    }
    strmcpy(tmpbuf2,tmpstr,mybufsize/4);
    if (line && (*line=='q' || *line=='Q')) {
        max=number_of_servers;
        for (i=0;i<max;i++)
            if (is_server_connected(i)) {
                from_server=i;
                strmcpy(tmpbuf1,tmpbuf2,mybufsize/4+1);
                send_to_server("QUIT :%s",tmpbuf1);
                strmcpy(tmpbuf1,tmpbuf2,mybufsize/4+1);
                mynick=get_server_nickname(from_server);
#ifdef WANTANSI
                joiner=CheckJoiners(mynick,NULL,from_server,NULL);
                if (joiner && joiner->shitlist && joiner->shitlist->shit)
                    colnick=CmdsColors[COLLEAVE].color5;
                else if (joiner && joiner->frlist && joiner->frlist->privs)
                    colnick=CmdsColors[COLLEAVE].color4;
                else colnick=CmdsColors[COLLEAVE].color1;
                say("Signoff: %s%s%s (%s%s%s)",
                    colnick,mynick,Colors[COLOFF],
                    CmdsColors[COLLEAVE].color3,tmpbuf1,Colors[COLOFF]);
#else
                say("Signoff: %s (%s)",mynick,tmpbuf1);
#endif
            }
        irc_exit(IRCQuit);
    }
}

/* Adds nick to delay op list */
void AddDelayOp(channel,nick,voice)
char *channel;
char *nick;
int  voice;
{
    char tmpbuf[mybufsize/4];
    void (*func)()=(void(*)()) HandleDelayOp;

    sprintf(tmpbuf,"-INV %d %s %d %s",AutoOpDelay,channel,voice,nick);
    timercmd("FTIMER",tmpbuf,(char *) func);
}

/* Adds nick to delay notify list */
void AddDelayNotify(nick)
char *nick;
{
    char tmpbuf[mybufsize/4];
    void (*func)()=(void(*)()) HandleDelayNotify;

    sprintf(tmpbuf,"-INV 10 %s",nick);
    timercmd("FTIMER",tmpbuf,(char *) func);
}

/* Adds given server to server list */
void AddServer(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int port=0;
    int old_server=0;
    char *tmp;
    char *server;

    server=new_next_arg(args,&args);
    if (!(server && *server)) {
        PrintUsage("ADDS server[:port] or ADDS server [port]");
        return;
    }
    if ((tmp=index(server,':'))!=(char *) 0) *tmp++='\0';
    else tmp=new_next_arg(args,&args);
    if (!(tmp && *tmp)) port=6667;
    else {
        if (!isdigit(*tmp)) port=6667;
        else {
            port=atoi(tmp);
            if (port<1024) port=6667;
        }
    }
    old_server=from_server;
    add_to_server_list(server,port,(char *) 0,(char *) 0,0);
    from_server=old_server;
    say("%s %d added to your server list",server,port);
}

/* Removes given server from server list */
void RemoveServer(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int port=0;
    int old_server=0;
    int servernum=0;
    char *tmp;
    char *server;

    server=new_next_arg(args,&args);
    if (!(server && *server)) {
        PrintUsage("REMS server[:port] or REMS server [port]");
        return;
    }
    if ((tmp=index(server,':'))!=(char *) 0) *tmp++ = '\0';
    else tmp=new_next_arg(args,&args);
    if (!(tmp && *tmp)) port=6667;
    else {
        if (!isdigit(*tmp)) port=6667;
        else {
            port=atoi(tmp);
            if (port<1024) port=6667;
        }
    }
    old_server=from_server;
    if ((servernum=find_in_server_list(server,port))==-1)
        say("Couldn't find %s %d",server,port);
    else {
        server=server_list[servernum].name;
        if (server_list[servernum].connected)
            say("%s %d is connected. Not removed !",server,port);
        else {
            say("%s %d removed from your server list",server,port);
            remove_from_server_list(servernum);
        }
    }
    from_server=old_server;
}

/* Lists servers on server list */
void ListServers(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    char *server;

    say("List of servers on your server list");
    for (i=0;i<number_of_servers;i++) {
        server=server_list[i].name;
        if (i!=from_server) say("#%-2d %s  %d",i,server,server_list[i].port);
        else say("#%-2d %c%s%c  %d",i,bold,server,bold,server_list[i].port);
    }
}

/* Returns 1 if user is banned on channel, else 0 */
int IsBanned(userhost,channel,server,tmpchan)
char *userhost;
char *channel;
int  server;
ChannelList *tmpchan;
{
    int isbanned=0;
    ChannelList *chan;
    struct bans *tmpban;

    if (tmpchan) chan=tmpchan;
    else chan=lookup_channel(channel,server,0);
    if (chan) {
        for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next)
            if (wild_match(userhost,tmpban->ban) || wild_match(tmpban->ban,userhost)) {
                if (tmpban->exception) return(0);
                isbanned=1;
            }
    }
    return(isbanned);
}

#ifdef CELE
/* Random signoff generator */
char *CeleSignoff()
{
    return("we need some random signoff messages");
/*
 *   int num;
 *
 *   srand(time(0));
 *   num=rand()%9;
 *   if (num==0) return("What do you have in your Toolie Box?");
 *   else if (num==1) return("It's got less backdoors than iNFiNiTY...");
 *   else if (num==2) return("Hope it makes ya feel warm & safe");
 *   else if ((num==3) || (num==8)) return("Complete with new Nerf Jackhammer (c)");
 *   else if ((num==4) || (num==9)) return("Accepteth No Substitutes");
 *   else if (num==5) return("Now available in fire engine red!");
 *   else if (num==6) return("Now includes \"101 ways to BBQ a bison\"");
 *   else if (num==7) return("mmm... toolie, i love you");
 *   else return(NULL);
*/
}
#else
/* Randomly picks sign off message */
char *PickSignOff()
{
    int  number;
    int  count=0;
    char *pointer=(char *) 0;
    char *filepath;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize/2];
    FILE *fp;

    if (!NumberOfSignOffMsgs) return(NULL);
    srand(time((time_t *) 0));
    number=rand()%NumberOfSignOffMsgs+1;
    filepath=OpenCreateFile("ScrollZ.addon",0);
    if (filepath && (fp=fopen(filepath,"r"))) {
        while (readln(fp,tmpbuf1)) {
            pointer=tmpbuf1;
            NextArg(pointer,&pointer,tmpbuf2);
            if (!my_stricmp("SIOFF",tmpbuf2)) count++;
            if (count==number) break;
        }
        fclose(fp);
        while (isspace(*pointer)) pointer++;
        return(pointer);
    }
    return(NULL);
}
#endif

#ifdef SCKICKS
/* Randomly picks scatter kick comment */
char *PickScatterKick(number)
int number;
{
    int  count=0;
    char *pointer=(char *) 0;
    char *filepath;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize/2];
    FILE *fp;

    if (!NumberOfScatterKicks) return(NULL);
    filepath=OpenCreateFile("ScrollZ.addon",0);
    if (filepath && (fp=fopen(filepath,"r"))) {
        while (readln(fp,tmpbuf1)) {
            pointer=tmpbuf1;
            NextArg(pointer,&pointer,tmpbuf2);
            if (!my_stricmp("SKICK",tmpbuf2)) count++;
            if (count==number) break;
        }
        fclose(fp);
        while (isspace(*pointer)) pointer++;
        return(pointer);
    }
    return(NULL);
}
#endif

/* Clears tabkey list */
void ClearTab(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int curserv=from_server;
    struct nicks *nickstr;
    struct nicks *nicklist;

    if (CheckServer(curserv)) {
        nicklist=server_list[curserv].nicklist;
        while (nicklist) {
            nickstr=nicklist;
            nicklist=nickstr->next;
            new_free(&(nickstr->nick));
            new_free(&nickstr);
        }
        server_list[curserv].nicklist=NULL;
        server_list[curserv].nickcur=NULL;
    }
}

#ifdef EXTRAS
/* Stores links info internally */
void LLook(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    struct splitstr *tmpsplit;
    time_t timenow=time((time_t *) 0);

    if (timenow-LastLinks>=120 || !inFlierLinks) {
        LastLinks=timenow;
        while (splitlist) {
            tmpsplit=splitlist;
            splitlist=splitlist->next;
            new_free(&(tmpsplit->servers));
            new_free(&tmpsplit);
        }
        inFlierLinks=1;
        send_to_server("LINKS");
        say("Gathering links info from server");
    }
    else say("Wait till previous LINKS, LLOOK, LLOOKUP or MAP completes");
}

/* Compares links info against one stored internally */
void LLookUp(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    time_t timenow=time((time_t *) 0);
    struct splitstr *tmpsplit;

    if (splitlist) {
        if (timenow-LastLinks>=120 || !inFlierLinks) {
            LastLinks=timenow;
            while (splitlist1) {
                tmpsplit=splitlist1;
                splitlist1=splitlist1->next;
                new_free(&(tmpsplit->servers));
                new_free(&tmpsplit);
            }
            inFlierLinks=2;
            send_to_server("LINKS");
            say("Gathering links info from server");
        }
        else say("Wait till previous LINKS, LLOOK, LLOOKUP or MAP completes");
    }
    else say("Do /LLOOK first");
}

/* This handles links reply from server */
void HandleLinks(servers)
char *servers;
{
    int  found1,found2;
    char *tmpstr=(char *) 0;
    char tmpbuf[mybufsize/4];
    struct splitstr *tmp1,*tmp2,*tmpsplit;

    if (inFlierLinks) {
        tmp1=(struct splitstr *) new_malloc(sizeof(struct splitstr));
        tmp2=(struct splitstr *) new_malloc(sizeof(struct splitstr));
        if (tmp1 && tmp2) {
            tmpstr=servers;
            tmp1->next=tmp2;
            tmp2->next=NULL;
            tmp1->servers=NULL;
            tmp2->servers=NULL;
            NextArg(tmpstr,&tmpstr,tmpbuf);
            malloc_strcpy(&(tmp1->servers),tmpbuf);
            NextArg(tmpstr,&tmpstr,tmpbuf);
            malloc_strcat(&(tmp2->servers),tmpbuf);
            if (inFlierLinks==1) {
                found1=0;
                found2=0;
                tmpsplit=splitlist;
                while (tmpsplit) {
                    if (!my_stricmp(tmp1->servers,tmpsplit->servers)) found1=1;
                    if (!my_stricmp(tmp2->servers,tmpsplit->servers)) found2=1;
                    tmpsplit=tmpsplit->next;
                    if (found1 && found2) break;
                }
                if (found1 && !found2) {
                    new_free(&(tmp1->servers));
                    new_free(&tmp1);
                    tmp1=tmp2;
                    tmp2=NULL;
                }
                if (!found1 && found2) {
                    new_free(&(tmp2->servers));
                    new_free(&tmp2);
                    tmp1->next=NULL;
                }
                if (found1 && found2) {
                    new_free(&(tmp1->servers));
                    new_free(&tmp1);
                    new_free(&(tmp2->servers));
                    new_free(&tmp2);
                    return;
                }
                if (!splitlist) splitlist=tmp1;
                else {
                    tmpsplit=splitlist;
                    while (tmpsplit->next) tmpsplit=tmpsplit->next;
                    tmpsplit->next=tmp1;
                }
            }
            else {
                found1=0;
                found2=0;
                tmpsplit=splitlist1;
                while (tmpsplit) {
                    if (!my_stricmp(tmp1->servers,tmpsplit->servers)) found1=1;
                    if (!my_stricmp(tmp2->servers,tmpsplit->servers)) found2=1;
                    tmpsplit=tmpsplit->next;
                    if (found1 && found2) break;
                }
                if (found1 && !found2) {
                    new_free(&(tmp1->servers));
                    new_free(&tmp1);
                    tmp1=tmp2;
                    tmp2=NULL;
                }
                if (!found1 && found2) {
                    new_free(&(tmp2->servers));
                    new_free(&tmp2);
                    tmp1->next=NULL;
                }
                if (found1 && found2) {
                    new_free(&(tmp1->servers));
                    new_free(&tmp1);
                    new_free(&(tmp2->servers));
                    new_free(&tmp2);
                    return;
                }
                if (!splitlist1) splitlist1=tmp1;
                else {
                    tmpsplit=splitlist1;
                    while (tmpsplit->next) tmpsplit=tmpsplit->next;
                    tmpsplit->next=tmp1;
                }
            }
        }
    }
}

/* Prints all servers missing from links info */
void ListSplitedServers()
{
    int found;
    struct splitstr *tmp,*tmpsplit;

    say("Servers missing from links info");
    tmpsplit=splitlist;
    while (tmpsplit) {
        found=0;
        tmp=splitlist1;
        while (tmp && !found) {
            if (!my_stricmp(tmpsplit->servers,tmp->servers)) found=1;
            tmp=tmp->next;
        }
        if (!found) say("%s",tmpsplit->servers);
        tmpsplit=tmpsplit->next;
    }
    say("End of missing servers list");
}
#endif

/* This will show the sucker that last killed you */
void ShowKill(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    if (WhoKilled) say("%s",WhoKilled);
    else say("You haven't been killed so far");
}
