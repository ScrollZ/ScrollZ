/*************************************************************************
 *  Various Functions   Coded  By  Sheik and Scrubbs                     *
 *  Goal: To Create a Toolz-like atmosphere, in C.                       *
 *                                                                       *
 *  ChanWallOp       msg all the ops on current channel - /wallmsg       *
 *  MegaDeop         mega deops the current channel - /mdop              *
 *  LameKick         Kicks all unopped people from current channel - /lk *
 *  DoDops           Deops a line of nicks, whatever the length          *
 *  NewUser          Lets you fake your UserName - /newuser              *
 *  ReconnectServer  Reconnects to Current Server - /reconnect           *
 *************************************************************************/

/******************************************************************************
 Functions coded by Flier (THANX to Sheik!!)
 The same goal as 12 lines up ! :))

 ReconnectOnKill      Reconnects you to server on kills
 Kick                 Kicks nick from channel
 BanType              Sets ban type (normal,hostname,screw)
 Ban                  Bans nick from channel
 BanKick              Kicks and bans user from channel
 Op                   Ops nick
 Dop                  Deops nick
 About                Tells you some info on ScrollZ
 Leave                Leave channel
 Invite               Invites nick to channel
 ModeClear            Does a -ntsilpmk on channel
 Chops                Shows all ops on current channel
 ChannelScan          Shows all nicks on current channel
 ListFriends          Lists all the friends on your friends list
 ListAutoBanKicks     Lists all the people on your auto (ban) kick list
 Topic                Shows or sets topic on channel
 FServer              Connects to specified server, else to default one
 Cycle                Leaves and joins channel
 ScrollZSave          Saves ScrollZ.save file
 CheckUsers           Returns user's privileges or 0 if he/she is not registered
 CheckABKs            Returns user's shit-level or 0 if he/she is not on ABK list
 CheckJoiners         Returns pointer if it finds nick on your joinlist, else adds him to your joinlist
 DirLM                Redirects last message/notice you have received
 AddFriend            Adds user to your friends list
 AddAutoBanKick       Adds user to your auto (ban) kick list
 RemoveFriend         Removes user from your friends list
 RemoveAutoBanKick    Removes user from your auto (ban) kick list
 ShowBans             Shows bans on a channel
 Version              Does /CTCP nick VERSION
 Unban                Unbans user
 OnBans               Handles ban messages from server
 CdBan                Clears channel of bans
 NoIgnore             Unignores user
 Ignore               Ignores user
 SetAway              Marks you as being away
 SetBack              Marks you as not being away
 OnWho                Handles who reply from server
 Check4Ban            Checks if ban on you is detected
 AddWord              Adds word to your wordkick list
 RemoveWord           Removes word from your wordkick list
 ListWords            Lists all the words from your wordkick list
 CheckLine            Checks line for wordkick, 1 - if match found, 0 - otherwise
 AddNick2List         Adds nick to list of people who have messaged you
 NickNext             Sets nickcur to the next leaf on linked list nicks (when TAB pressed)
 AddSplitter          Adds splitter to appropriate server
 Check4Join           Checks if net has joined and returns server name if so
 WhoLeft              Tells you all the nicks that went away with the split
 FilterKick           Kicks all the people that match filter
 DoBans               Bans all the people that are shitlisted
 NewHost              Virtual Hosting, patched in by Zakath
******************************************************************************/

/*
 * $Id: edit2.c,v 1.8 1998-10-21 17:35:09 f Exp $
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
#include "list.h"
#include "struct.h"
#include "myvars.h"
#include "whowas.h"

#ifdef JIMMIE
#include <sys/utsname.h>
#endif

extern NotifyList *notify_list;
extern int CO;
extern time_t start_time;

NickList *CheckJoiners _((char *, char *, int , ChannelList *));
void DoDops _((char *, char *, int));
void ReconnectServer _((char *, char *, char *));
void AddFriend2List _((char *, char *, char *));
void AddBK2List _((char *, char *, char *));
void RemoveFriendFromList _((char *));
void RemoveAutoBanKickFromList _((char *));
void SetBack2 _((char *, char *));
void SetBackDelete _((char *, char *));
void ListFriendsPage _((char *));
void ListFriendsKey _((char *, char *));
void ListABKsPage _((char *));
void ListABKsKey _((char *, char *));
void AddNick2List _((char *, int));
struct words *CheckLine _((char *, char *));
void NotChanOp _((char *));
void NoWindowChannel _((void));
void BanNew _((WhoisStuff *, char *, char *));
void BanKickNew _((WhoisStuff *, char *, char *));
void AddFriendNew _((WhoisStuff *, char *, char *));
void AddBKNew _((WhoisStuff *, char *, char *));
void RemoveFriendNew _((WhoisStuff *, char *));
void RemoveAutoBanKickNew _((WhoisStuff *, char *));
void UnbanNew _((WhoisStuff *, char *, char *));
void IgnoreNew _((WhoisStuff *, char *, char *));
int  AddLast _((List *, List *));

extern struct friends *FindMatch _((char *, char *));
extern void ClearKey _((char *, char *, char *));
extern int  CheckChannel _((char *, char *));
extern int  CheckChannel2 _((char *, char *));
extern void UnbanIt _((char *, char *, int));
extern int  AddBan _((char *, char *, int, char *, time_t, ChannelList *));
extern void AwaySave _((char *, int));
extern void UserDomainList _((char *));
#ifdef WANTANSI
extern void SaveColors _((FILE *));
#endif
extern void PrintSetting _((char *, char *, char *, char *));
extern char *OpenCreateFile _((char *, int));
extern void CreateBan _((char *, char *, char *));
extern void StripAnsi _((char *, char *, int));
extern void BuildPrivs _((struct friends *, char *));
#ifdef WANTANSI
extern int  CountAnsi _((char *, int));
#endif
extern struct autobankicks *FindShit _((char *, char *));
extern void PlayBack _((char *, char *, char *));
extern void PrintUsage _((char *));
extern void CheckJoinChannel _((WhoisStuff *, char *, char *));
extern void EncryptString _((char *, char *, char *, int));
extern int  CheckServer _((int));
extern int  matchmcommand _((char *, int));
extern NickList *find_in_hash _((ChannelList *, char *));
extern void add_nick_to_hash _((ChannelList *, NickList *));
extern void remove_nick_from_hash _((ChannelList *, NickList *));

extern void e_channel _((char *, char *, char *));
extern void timercmd _((char *, char *, char *));
extern void away _((char *, char *, char *));
extern void describe _((char *, char *, char *));
extern void swap_window _((Window *, Window *));

static char *tmpbuflist=(char *) 0;
static int  listcount;
static int  countall;
static int  DontHold;
static struct friends *tmpfriendlist;
static struct autobankicks *tmpabklist;
/* Patched by Zakath */
static char timereturn[mybufsize/16];
#if defined(CELE) && !defined(VILAS)
static char *celeawaystr=(char *) 0;
static time_t CeleAwayTime;
#endif
extern char *CelerityNtfy;

/* Kicks all unopped people from current channel */
void LameKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  all=0;
    int  count=0;
    char *channel;
    char *comment=(char *) 0;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize/4];
    NickList *tmp;
    ChannelList *chan;

    if ((channel=get_channel_by_refnum(0))) {
        if (args && *args && !my_strnicmp(args,"-ALL",4)) {
            all=1;
            new_next_arg(args,&args);
        }
        chan=lookup_channel(channel,curr_scr_win->server,0);
        if (chan && ((chan->status)&CHAN_CHOP)) {
            if (!chan) return;
            *tmpbuf1='\0';
            if (args && *args) comment=args;
            else comment=DefaultLK;
            for (tmp=chan->nicks;tmp;tmp=tmp->next) {
                if (tmp->chanop) continue;
                if (tmp->voice && !all) continue;
#if defined(VILAS)
                sprintf(tmpbuf2,"KICK %s %s :%s\r\n",channel,tmp->nick,comment);
#elif defined(CELE)
                sprintf(tmpbuf2,"KICK %s %s :%s %s\r\n",channel,tmp->nick,
                        comment,CelerityL);
#else  /* CELE */
                sprintf(tmpbuf2,"KICK %s %s :<ScrollZ-LK> %s\r\n",channel,tmp->nick,
                        comment);
#endif /* VILAS */
                if (strlen(tmpbuf1)+strlen(tmpbuf2)>=IRCD_BUFFER_SIZE-150) {
                    send_to_server("%s",tmpbuf1);
                    *tmpbuf1='\0';
                    count=0;
                }
                count++;
                strcat(tmpbuf1,tmpbuf2);
            }
            if (count) send_to_server("%s",tmpbuf1);
            say("Sent the server all the lamer kicks, sit back and watch !");
        }
        else NotChanOp(channel);
    }
    else NoWindowChannel();
}

/* Sends NOTICE to all ops of Current Channel!       */
void ChanWallOp(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  server=0;
    int  maxnicks=get_int_var(MAX_WALLOP_NICKS_VAR);
    int  opscount=0;
    int  totalcount=0;
    int  includecount=0;
    char *ptr;
    char *word;
    char *channel;
    char *alllist=(char *) 0;
    char *allinclist=(char *) 0;
    char *allexclist=(char *) 0;
    char *fulllistptr;
    char tmpnick[100];
    char fulllist[mybufsize*2];
    char excludelist[mybufsize];
    char includelist[mybufsize];
    char currentlist[mybufsize];
    NickList *tmp;
    ChannelList *chan=NULL;

    /* set all strings to null */
    *fulllist='\0';
    *excludelist='\0';
    *includelist='\0';
    *currentlist='\0';
    if (!(args && *args)) {
        PrintUsage("WALL [#channel] [+nick] [-nick] message");
        return;
    }
    server=curr_scr_win->server;
    while (*args && isspace(*args)) args++;
    if (*args=='#') {
        word=next_arg(args,&args);
        if ((chan=lookup_channel(word,server,0))==NULL) return;
    }
    else if (!(channel=get_channel_by_refnum(0)) ||
             (chan=lookup_channel(channel,server,0))==NULL) {
        NoWindowChannel();
        return;
    }
    /* work along line till we find a word that doesn't start with - or +  */
    while (args) {
        /* peek at the first char, before detaching */
        if ((*args!='-') && (*args!='+')) break; /* no special flags to parse out */
        word=next_arg(args,&args);
        if (!word || !*(word+1)) {
            say("Error: no message/channel/nicks found");
            return;
        }
        else if (*word=='-') {
            *word++='\0';
            /* exclude list will be like " sheik  blah  lame " */
            if ((tmp=find_in_hash(chan,word))) {
                strcat(excludelist," ");
                strcat(excludelist,tmp->nick);
                strcat(excludelist," ");
                if (allexclist) malloc_strcat(&allexclist," ");
                malloc_strcat(&allexclist,tmp->nick);
            }
        }
        else if (*word=='+') {
            *word++='\0';
            if ((tmp=find_in_hash(chan,word))) {
                strcat(includelist," ");
                strcat(includelist,tmp->nick);
                strcat(includelist," ");
                includecount++;
                if (allinclist) malloc_strcat(&allinclist," ");
                malloc_strcat(&allinclist,tmp->nick);
            }
        }
    }
    /* strip all spaces in front of message */
    while (args && isspace(*args)) *args++='\0';
    if (!args || !(*args)) {
        say("Error: no message found");
        return;
    }
    for (tmp=chan->nicks;tmp;tmp=tmp->next) {
        if (tmp->chanop) {
            /* create a string like  " sheik ", then strstr()
             * on the exclude list, does not allow wildcards
             * in the exclude list! EXPENSIVE!
             */
            sprintf(tmpnick," %s ",tmp->nick);
            if (strstr(excludelist,tmpnick)) continue;
            strcat(fulllist,tmpnick);
            opscount++;
        }
    }
    if (includecount) strcat(fulllist,includelist);
    fulllistptr=fulllist;
    for (ptr=new_next_arg(fulllistptr,&fulllistptr);ptr;
         ptr=new_next_arg(fulllistptr,&fulllistptr)) {
        if (*currentlist) strcat(currentlist,",");
        strcat(currentlist,ptr);
        totalcount++;
        if (totalcount>=maxnicks) {
            sprintf(buffer,"[S-WallOp/%s] %s",chan->channel,args);
            send_to_server("NOTICE %s :%s",currentlist,buffer);
            totalcount=0;
            *currentlist='\0';
        }
        if (alllist) malloc_strcat(&alllist," ");
        malloc_strcat(&alllist,ptr);
    }
    if (totalcount) {
        sprintf(buffer,"[S-WallOp/%s] %s",chan->channel,args);
        send_to_server("NOTICE %s :%s",currentlist,buffer);
    }
    if (ShowWallop) {
        if (*excludelist) say("Excluding %s from wallmsg",allexclist);
        if (includecount) say("Including %s to receive wallmsg",allinclist);
        say("Sent wallmsg on %s to %s",chan->channel,alllist);
    }
    new_free(&alllist);
    new_free(&allinclist);
    new_free(&allexclist);
    if (!opscount) say("There are no channel operators in %s",chan->channel);
}

/* Mega ops/deops/voices/devoices channel */
void MegaMode(args,type)
char *args;
int type;
{
    int  all=1;
    int  isfriend;
    char *chops=(char *) 0;
    char *tmpstr;
    char *mynick=get_server_nickname(from_server);
    char *channel;
    NickList *tmp;
    ChannelList *chan;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr && !my_stricmp(tmpstr,"-O")) {
        all=0;
        tmpstr=new_next_arg(args,&args);
    }
    if (tmpstr && is_channel(tmpstr)) channel=tmpstr;
    else if (!(channel=get_channel_by_refnum(0))) {
        NoWindowChannel();
        return;
    }
    chan=lookup_channel(channel,curr_scr_win->server,0);
    if (!chan) return;
    if ((chan->status)&CHAN_CHOP) {
        for (tmp=chan->nicks;tmp;tmp=tmp->next) {
            if (type==1 || type==4) isfriend=tmp->frlist && ((tmp->frlist->privs)&FLOP);
            else isfriend=tmp->frlist &&
                    ((tmp->frlist->privs)&FLOP || (tmp->frlist->privs)&FLAUTOOP);
            if (type<2 && !(tmp->chanop)) continue;
            if (type==4 && tmp->chanop) continue;
            if ((all || (!all && (type%2?!isfriend:isfriend))) &&
                 my_stricmp(tmp->nick,mynick)) {
                if (chops) malloc_strcat(&chops," ");
                malloc_strcat(&chops,tmp->nick);
            }
        }
        if (chops) {
            /* type=0 -> reop */
            /* type=1 -> deop */
            /* type=2 -> voice      -DEXTRAS */
            /* type=3 -> devoice    -DEXTRAS */
            /* type=4 -> op */
            if (!all) say("%susers on %s",
                          type==0?"Mass reopping ":
                          type==1?"Mass deopping l":
#ifdef EXTRAS
                          type==2?"Mass voicing ":
                          type==3?"Mass devoicing l":
#endif
                          "Mass opping ",channel);
            else say("Mass %s %s",
                     type==0?"reopping":
                     type==1?"deopping":
#ifdef EXTRAS
                     type==2?"voicing":
                     type==3?"devoicing":
#endif
                     "opping",channel);
            DoDops(chops,channel,type);
            new_free(&chops);
        }
        else say ("No users to %s on %s",
                  type==0?"reop":
                  type==1?"deop":
#ifdef EXTRAS
                  type==2?"voice":
                  type==3?"devoice":
#endif
                  "op",channel);
    }
    else NotChanOp(channel);
}

/* MegaDeops channel */
void MegaDeop(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    MegaMode(args,1);
}

/* MegaReops channel */
void MegaReop(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    MegaMode(args,0);
}

/* MassOp channel */
void MassOp(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    MegaMode(args,4);
}

#ifdef EXTRAS
/* MegaVoice channel */
void MegaVoice(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    MegaMode(args,2);
}

/* MegaDeVoice channel */
void MegaDeVoice(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    MegaMode(args,3);
}
#endif

/* Change modes for number of users */
/* type=0 -> reop */
/* type=1 -> deop */
/* type=2 -> voice      -DEXTRAS */
/* type=3 -> devoice    -DEXTRAS */
/* type=4 -> op */
void DoDops(chops,channel,type)
char *chops;
char *channel;
int type;
{
    int  send=0;
    int  count=0;
    int  max=get_int_var(MAX_MODES_VAR);
    char sign=' ';
    char *tmp=(char *) 0;
    char *modetype="";
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];

    *tmpbuf1='\0';
    *tmpbuf2='\0';
    *tmpbuf3='\0';
    switch (type) {
        case 0:
        case 4:
            sign='+';
            modetype="o";
            break;
        case 1:
            sign='-';
            modetype="o";
            break;
#ifdef EXTRAS
        case 2:
            sign='+';
            modetype="v";
            break;
        case 3:
            sign='-';
            modetype="v";
            break;
#endif
    }
    sprintf(tmpbuf2,"MODE %s %c",channel,sign);
    for (tmp=new_next_arg(chops,&chops);tmp;tmp=new_next_arg(chops,&chops)) {
        count++;
        strcat(tmpbuf2,modetype);
	strcat(tmpbuf3," ");
        strcat(tmpbuf3,tmp);
        send=1;
        if (count==max) {
            strcat(tmpbuf1,tmpbuf2);
            strcat(tmpbuf1,tmpbuf3);
            strcat(tmpbuf1,"\r\n");
            sprintf(tmpbuf2,"MODE %s %c",channel,sign);
            *tmpbuf3='\0';
            count=0;
        }
        if (strlen(tmpbuf1)>=IRCD_BUFFER_SIZE-150) {
            send_to_server("%s",tmpbuf1);
            *tmpbuf1='\0';
            *tmpbuf2='\0';
            *tmpbuf3='\0';
            send=0;
        }
    }
    if (count) {
        strcat(tmpbuf1,tmpbuf2);
        strcat(tmpbuf1,tmpbuf3);
        strcat(tmpbuf1,"\r\n");
        send=1;
    }
    if (send) send_to_server("%s",tmpbuf1);
}

/* Newuser, stolen from Hendrix, and fixed up */
void NewUser(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmp;

    if (!(tmp=new_next_arg(args,&args))) {
        PrintUsage("NEWUSER username [realname]");
        PrintSetting("Current username",username,empty_string,empty_string);
        PrintSetting("Current realname",realname,empty_string,empty_string);
        return;
    }
    strmcpy(username,tmp,NAME_LEN);
    if (args && *args) strmcpy(realname,args,REALNAME_LEN);
    ReconnectServer(NULL,NULL,NULL);
}

/* Reconnects you to server, storing all current info */
void ReconnectServer(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int port;
    int servernum=from_server;
    char tmpbuf[mybufsize/8];

    if (servernum!=-1) {
        port=get_server_port(servernum);
        say("Reconnecting to server %s port %d...",get_server_name(servernum),port);
        sprintf(tmpbuf,"%d %d",servernum,port);
        close_server(servernum,"reconnecting to server");
        clean_whois_queue();
        window_check_servers();
        server(NULL,tmpbuf,NULL);
    }
    else say("You are not connected to a server");
}

/* Reconnects you to server on kill */
void ReconnectOnKill(servernum)
int servernum;
{
    int flag=1;
    int change=0;
    char tmpbuf[mybufsize/16];
    Window *tmp=(Window *) 0;
    Window *oldwindow;

    oldwindow=curr_scr_win;
    if (curr_scr_win->server!=servernum) {
        change=1;
        while ((tmp=traverse_all_windows(&flag)))
            if (tmp->server==servernum) break;
    }
    if (change && tmp) swap_window(oldwindow,tmp);
    sprintf(tmpbuf,"SERVER %d",servernum);
    window(NULL,tmpbuf,NULL);
    if (change && tmp) {
        swap_window(oldwindow,tmp);
        redraw_window(oldwindow,0);
    }
}

/* Kicks nick from the current channel */
void Kick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *tmpnick=(char *) 0;
    char *comment=(char *) 0;
    NickList *joiner;
    ChannelList *chan;

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (args && *args) {
            chan=lookup_channel(channel,curr_scr_win->server,0);
            if (chan && ((chan->status)&CHAN_CHOP)) {
                tmpnick=new_next_arg(args,&args);
                if (args && *args) comment=args;
                else comment=DefaultK;
                joiner=CheckJoiners(tmpnick,channel,curr_scr_win->server,chan);
#if defined(VILAS)
                if (joiner) send_to_server("KICK %s %s :%s",channel,tmpnick,comment);
#elif defined(CELE)
                if (joiner) send_to_server("KICK %s %s :%s %s",channel,tmpnick,
                                           comment,CelerityL);
#else  /* CELE */
                if (joiner) send_to_server("KICK %s %s :<ScrollZ-K> %s",channel,tmpnick,
                                           comment);
#endif /* VILAS */
                else say("Can't find %s on %s",tmpnick,channel);
            }
            else NotChanOp(channel);
        }
        else PrintUsage("K nick [reason]");
    }
    else NoWindowChannel();
}

/* Changes default ban type */
void BanType(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmptype;
    char tmpbuf[mybufsize/4];

    if (*args) {
        tmptype=new_next_arg(args,&args);
        switch (tmptype[0]) {
            case 'N':
            case 'n':
                defban='N';
                break;
            case 'B':
            case 'b':
                defban='B';
                break;
            case 'E':
            case 'e':
                defban='E';
                break;
            case 'H':
            case 'h':
                defban='H';
                break;
            case 'S':
            case 's':
                defban='S';
                break;
            default:
                PrintUsage("BANTYPE type (N, B, E, H, or S)");
                say("          N=Normal, B=Better, E=Elite, H=Host, S=Screw");
                break;
        }
    }
    strcpy(tmpbuf,"Ban type");
    switch (defban) {
        case 'N':
            PrintSetting(tmpbuf,"Normal",empty_string,empty_string);
            break;
        case 'B':
            PrintSetting(tmpbuf,"Better",empty_string,empty_string);
            break;
        case 'E':
            PrintSetting(tmpbuf,"Elite",empty_string,empty_string);
            break;
        case 'H':
            PrintSetting(tmpbuf,"Host",empty_string,empty_string);
            break;
        case 'S':
            PrintSetting(tmpbuf,"Screw",empty_string,empty_string);
            break;
    }
}

/* Bans nick on specified channel */
void BanIt(channel,nick,userhost,deop,chan)
char *channel;
char *nick;
char *userhost;
int  deop;
ChannelList *chan;
{
    ChannelList *tmpchan;
    char tmpbuf[mybufsize/4];

    if (channel && chan) tmpchan=chan;
    else tmpchan=lookup_channel(channel,curr_scr_win->server,0);
    CreateBan(nick,userhost,tmpbuf);
    if (tmpchan && ((tmpchan->status)&CHAN_CHOP)) {
        if (deop) send_to_server("MODE %s -o+b %s %s",channel,nick,tmpbuf);
        else send_to_server("MODE %s +b %s",channel,tmpbuf);
    }
    else NotChanOp(channel);
}

/* Bans nick on the current channel */
void Ban(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel = (char *) 0;
    char *tmpnick;
    void (*func)();
    NickList *joiner;
    ChannelList *chan;

    channel=get_channel_by_refnum(0);
    if (channel) {
        chan=lookup_channel(channel,curr_scr_win->server,0);
        if (!chan) return;
        if (*args) {
            if ((chan->status)&CHAN_CHOP) {
                while ((tmpnick=new_next_arg(args,&args))) {
                    if (strchr(tmpnick,'!') || strchr(tmpnick,'@') || strchr(tmpnick,'*') ||
                        strchr(tmpnick,'.') || strchr(tmpnick,'?')) {
                        send_to_server("MODE %s +b %s",channel,tmpnick);
                        continue;
                    }
                    joiner=CheckJoiners(tmpnick,chan->channel,from_server,chan);
                    if (joiner) BanIt(channel,joiner->nick,joiner->userhost,1,chan);
                    else {
                        joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
                        if (joiner) BanIt(channel,joiner->nick,joiner->userhost,0,NULL);
                        else {
                            func=(void(*)())BanNew;
                            inFlierWI++;
                            add_to_whois_queue(tmpnick,func,"%s",channel);
                        }
                    }
                }
            }
            else NotChanOp(channel);
        }
        else PrintUsage("BAN nick");
    }
    else NoWindowChannel();
}

/* Bans user if he/she is not on joinlist */
void BanNew(wistuff, tmpnick, text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    char tmpbuf[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    sprintf(tmpbuf,"%s@%s",wistuff->user,wistuff->host);
    BanIt(text,wistuff->nick,tmpbuf,0,NULL);
}

/* Bans and kicks nick from the current channel */
/* Also handles ignore and timed ban */
void BanKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel=(char *) 0;
    char *comment=(char *) 0;
    char *tmpnick;
    char tmpbuf1[mybufsize/4];
#ifdef EXTRAS
    char tmpbuf2[mybufsize/4];
#endif
    void (*func)();
    NickList *joiner;
    ChannelList *chan;

    channel=get_channel_by_refnum(0);
    if (channel) {
        chan=lookup_channel(channel,curr_scr_win->server,0);
        if (!chan) return;
        if (*args && args) {
            if ((chan->status)&CHAN_CHOP) {
                tmpnick=new_next_arg(args,&args);
                if (args && *args) comment=args;
                else {
                    if (!my_stricmp(command,"BK")) comment=DefaultBK;
#ifdef EXTRAS
                    else if (!my_stricmp(command,"BKI")) comment=DefaultBKI;
                    else comment=DefaultBKT;
#endif
                }
                joiner=CheckJoiners(tmpnick,channel,from_server,chan);
                if (joiner) {
                    BanIt(channel,joiner->nick,joiner->userhost,1,chan);
#if defined(VILAS)
                    send_to_server("KICK %s %s :%s",channel,joiner->nick,comment);
#elif defined(CELE)
                    send_to_server("KICK %s %s :%s %s",channel,joiner->nick,
                                   comment,CelerityL);
#else  /* CELE */
                    send_to_server("KICK %s %s :<ScrollZ-%s> %s",channel,joiner->nick,
                                   command,comment);
#endif /* VILAS */
                }
                else {
                    joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
                    if (joiner) BanIt(channel,joiner->nick,joiner->userhost,0,NULL);
                    else {
                        func=(void(*)())BanKickNew;
                        inFlierWI++;
                        add_to_whois_queue(tmpnick,func,"%s %s",command,channel);
                    }
                }
#ifdef EXTRAS
                if (joiner) {
                    if (index(command,'I')) {
                        send_text(joiner->nick,"You're now being ignored","NOTICE");
                        sprintf(tmpbuf1,"%s ALL",joiner->userhost);
                        sprintf(tmpbuf2,"%d",IgnoreTime);
                        ignore(NULL,tmpbuf1,tmpbuf2);
                        sprintf(tmpbuf1,"-INV %d IGNORE %s NONE",IgnoreTime,joiner->userhost);
                        timercmd("TIMER",tmpbuf1,NULL);
                    }
                    else if (index(command,'T')) {
                        CreateBan(joiner->nick,joiner->userhost,tmpbuf2);
                        sprintf(tmpbuf1,"-INV 6 MODE %s -b %s",channel,tmpbuf2);
                        timercmd("TIMER",tmpbuf1,NULL);
                    }
                }
#endif
            }
            else NotChanOp(channel);
        }
        else {
            sprintf(tmpbuf1,"%s nick [reason]",command);
            PrintUsage(tmpbuf1);
        }
    }
    else NoWindowChannel();
}

/* Bans & kicks user if he/she is not on joinlist */
void BanKickNew(wistuff, tmpnick, text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    char *command;
    char *channel;
    char tmpbuf1[mybufsize/4];
#ifdef EXTRAS
    char tmpbuf2[mybufsize/4];
#endif

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    command=new_next_arg(text,&text);
    channel=new_next_arg(text,&text);
    if (!command || !channel) return;
    sprintf(tmpbuf1,"%s@%s",wistuff->user,wistuff->host);
    BanIt(channel,wistuff->nick,tmpbuf1,0,NULL);
#ifdef EXTRAS
    if (index(command,'I')) {
        strcat(tmpbuf1," ALL");
        sprintf(tmpbuf2,"%d",IgnoreTime);
        ignore(NULL,tmpbuf1,tmpbuf2);
        sprintf(tmpbuf1,"-INV %d IGNORE %s@%s NONE",IgnoreTime,wistuff->user,wistuff->host);
        timercmd("TIMER",tmpbuf1,NULL);
    }
    else if (index(command,'T')) {
        CreateBan(wistuff->nick,tmpbuf1,tmpbuf2);
        sprintf(tmpbuf1,"-INV 6 MODE %s -b %s",channel,tmpbuf2);
        timercmd("TIMER",tmpbuf1,NULL);
    }
#endif
}

/* Ops nick on the current channel */
void Op(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  op=!my_stricmp(command,"OP")?1:0;
    int  count=0;
    int  usage=0;
    int  max=get_int_var(MAX_MODES_VAR);
    char *tmpnick;
    char *channel=(char *) 0;
    char tmpbuf[mybufsize/2];
    char modebuf[mybufsize/32];
    register NickList *tmp;
    ChannelList *chan;

    if (args && *args) {
        if (is_channel(args)) channel=new_next_arg(args,&args);
        else if (!(channel=get_channel_by_refnum(0))) {
            NoWindowChannel();
            return;
        }
        chan=lookup_channel(channel,curr_scr_win->server,0);
        if (!chan) return;
        if (*args) {
            if ((chan->status)&CHAN_CHOP) {
                strcpy(modebuf,"+oooooooo");
                if (!op) modebuf[0]='-';
                *tmpbuf='\0';
                while ((tmpnick=new_next_arg(args,&args))) {
                    if ((tmp=find_in_hash(chan,tmpnick))) {
                        strcat(tmpbuf," ");
                        strcat(tmpbuf,tmpnick);
                        count++;
                    }
                    else say("Can't find %s on %s",tmpnick,channel);
                    if (count==max) {
                        modebuf[count+1]='\0';
                        send_to_server("MODE %s %s %s",channel,modebuf,tmpbuf);
                        count=0;
                        *tmpbuf='\0';
                    }
                }
                if (count) {
                    modebuf[count+1]='\0';
                    send_to_server("MODE %s %s %s",channel,modebuf,tmpbuf);
                }
            }
            else NotChanOp(channel);
        }
        else usage=1;
    }
    else usage=1;
    if (usage) {
        sprintf(tmpbuf,"%s nicks",op?"OP":"DOP");
        PrintUsage(tmpbuf);
    }
}

/* Invites nick to your current channel */
void Invite(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;
    char *tmpchannel=(char *) 0;
    char tmpbuf[mybufsize/4];

    if (*args) {
        tmpnick=new_next_arg(args,&args);
        if (args && *args) {
            tmpchannel=new_next_arg(args,&args);
            if (!is_channel(tmpchannel)) sprintf(tmpbuf,"#%s",tmpchannel);
            else strcpy(tmpbuf,tmpchannel);
            tmpchannel=tmpbuf;
        }
        else if ((tmpchannel=get_channel_by_refnum(0))==NULL) {
            NoWindowChannel();
            return;
        }
        if (is_chanop(tmpchannel,get_server_nickname(from_server)))
            send_to_server("INVITE %s %s",tmpnick,tmpchannel);
        else NotChanOp(tmpchannel);
    }
    else PrintUsage("INV nick [[#]channel]");
}

/* Leaves current or specified channel */
void Leave(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *comment=(char *) 0;
    char *tmpchannel=(char *) 0;
    char tmpbuf[mybufsize/4];

    if (args && *args=='*') {
        tmpchannel=new_next_arg(args,&args);
        tmpchannel=get_channel_by_refnum(0);
    }
    else if (args && *args) {
        tmpchannel=new_next_arg(args,&args);
        if (!is_channel(tmpchannel)) sprintf(tmpbuf,"#%s",tmpchannel);
        else strcpy(tmpbuf,tmpchannel);
        tmpchannel=tmpbuf;
    }
    comment=args;
    if (!tmpchannel && (tmpchannel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    if (comment && *comment) send_to_server("PART %s :%s",tmpchannel,comment);
    else send_to_server("PART %s",tmpchannel);
}

/* Does a -ntislmpk on your current channel */
void ModeClear(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel=(char *) 0;
    char tmpbuf[mybufsize/2];
    ChannelList *chan;

    channel=get_channel_by_refnum(0);
    if (channel && (chan=lookup_channel(channel,from_server,0))) {
        if ((chan->status)&CHAN_CHOP) {
            sprintf(tmpbuf,"MODE %s -ntislmp",channel);
            if ((chan->mode)&MODE_KEY && chan->key) {
                strcat(tmpbuf,"k ");
                strcat(tmpbuf,chan->key);
            }
            send_to_server("%s",tmpbuf);
        }
        else NotChanOp(channel);
    }
    else NoWindowChannel();
}

/* Prints out all nicks on your current or specified channel */
void ChannelScan(command,args,subargs)
char *command;
char *args;
char *subargs;
{
#ifdef NEWCSCAN
    int  count=0;
    int  width=0;
    int  len=0;
#else
    int  buflen;
    int  szlen;
#endif
    int  gotchannel=0;
    int  shitted;
    int  reverse=2;
    int  friendok;
    int  hierarchy; /* 1-shitted, 2-normal, 3-voiced, 4-ops, 5-friends */
    char *channel=(char *) 0;
    char *users=(char *) 0;
#ifdef WANTANSI
    char *colorstr=(char *) 0;
#endif
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
#ifndef NEWCSCAN
    char tmpbuf3[mybufsize/4];
#endif
#ifdef WANTANSI
    char tmpbuf4[mybufsize/4];
#endif
    NickList *tmp;
    ChannelList *chan;

    channel=new_next_arg(args,&args);
    if (channel) {
        gotchannel=1;
        if (!my_stricmp(channel,"-H")) {
            reverse=0;
            gotchannel=0;
        }
        else if (!my_stricmp(channel,"-R")) {
            reverse=1;
            gotchannel=0;
        }
        if (!gotchannel && (channel=new_next_arg(args,&args))) gotchannel=1;
        if (gotchannel) {
            if (is_channel(channel)) strcpy(tmpbuf1,channel);
            else sprintf(tmpbuf1,"#%s",channel);
            channel=tmpbuf1;
        }
        else channel=(char *) 0;
    }
    if (!channel && (channel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    hierarchy=reverse?5:1;
    if (!(chan=lookup_channel(channel,curr_scr_win->server,0))) return;
#ifdef WANTANSI
#ifndef NEWCSCAN
    szlen=strlen(ScrollZstr)-CountAnsi(ScrollZstr,-1);
    sprintf(tmpbuf3,"%%%ds",szlen);
#endif /* NEWCSCAN */
    sprintf(tmpbuf1,"%s Users on %s%s%s :",ScrollZstr,
            CmdsColors[COLCSCAN].color1,chan->channel,Colors[COLOFF]);
#else /* WANTANSI */
    sprintf(tmpbuf1,"%s Users on %s :",ScrollZstr,chan->channel);
#ifndef NEWCSCAN
    szlen=strlen(ScrollZstr);
    sprintf(tmpbuf3,"%%%ds",szlen);
#endif /* NEWCSCAN */
#endif /* WANTANSI */

#ifdef NEWCSCAN
    put_it("%s",tmpbuf1);
#else
    buflen=strlen(tmpbuf1);
    malloc_strcpy(&users,tmpbuf1);
#endif /* NEWCSCAN */

#ifdef NEWCSCAN
    for (tmp=chan->nicks;tmp;tmp=tmp->next)
        if(strlen(tmp->nick)>len) len=strlen(tmp->nick);
    if (len<=9) len=9;
#ifdef WANTANSI
    width=(CO-(strlen(ScrollZstr)-CountAnsi(ScrollZstr,-1)+7))/(len+2);
#else
    width=(CO-(strlen(ScrollZstr)+7))/(len+2);
#endif /* WANTANSI */
    sprintf(tmpbuf1,"%%-%ds",len);
#endif /* NEWCSCAN */

    tmp=chan->nicks;
    for (;;) {
        if (!tmp) {
            if (reverse==2) break;
            else if (reverse) {
                hierarchy--;
                if (hierarchy<1) break;
            }
            else {
                hierarchy++;
                if (hierarchy>5) break;
            }
            tmp=chan->nicks;
        }
        shitted=tmp->shitlist?tmp->shitlist->shit:0;
        friendok=(!shitted && tmp->frlist)?tmp->frlist->privs:0;
        if (reverse!=2) {
            switch (hierarchy) {
            case 1: /* shitlisted */
                if (shitted)
#ifdef WANTANSI
                    colorstr=CmdsColors[COLCSCAN].color6;
#else
                ;
#endif
                else {
                    tmp=tmp->next;
                    continue;
                }
                break;
            case 2: /* normal */
                if (!(tmp->voice) && !(tmp->chanop) && !friendok && !shitted)
#ifdef WANTANSI
                    colorstr=CmdsColors[COLCSCAN].color5;
#else
                ;
#endif
                else {
                    tmp=tmp->next;
                    continue;
                }
                break;
            case 3: /* voiced */
                if (tmp->voice && !(tmp->chanop) && !friendok && !shitted)
#ifdef WANTANSI
                    colorstr=CmdsColors[COLCSCAN].color4;
#else
                ;
#endif
                else {
                    tmp=tmp->next;
                    continue;
                }
                break;
            case 4: /* ops */
                if (tmp->chanop && !friendok && !shitted)
#ifdef WANTANSI
                    colorstr=CmdsColors[COLCSCAN].color3;
#else
                ;
#endif
                else {
                    tmp=tmp->next;
                    continue;
                }
                break;
            case 5: /* friends */
                if (friendok)
#ifdef WANTANSI
                    colorstr=CmdsColors[COLCSCAN].color2;
#else
                ;
#endif
                else {
                    tmp=tmp->next;
                    continue;
                }
                break;
            }
        }

#ifdef WANTANSI
        else {
            if (shitted) colorstr=CmdsColors[COLCSCAN].color6;
            else if (friendok) colorstr=CmdsColors[COLCSCAN].color2;
            else if (tmp->chanop) colorstr=CmdsColors[COLCSCAN].color3;
            else if (tmp->voice) colorstr=CmdsColors[COLCSCAN].color4;
            else colorstr=CmdsColors[COLCSCAN].color5;
        }
#endif /* WANTANSI */

        if (users) malloc_strcat(&users," ");

#ifdef NEWCSCAN
        else malloc_strcat(&users,"  ");
#endif /* NEWCSCAN */

#ifdef WANTANSI
        if (tmp->chanop) {
            sprintf(tmpbuf4,"%s@%s",CmdsColors[COLNICK].color4,Colors[COLOFF]);
            malloc_strcat(&users,tmpbuf4);
#ifndef NEWCSCAN
            buflen+=strlen(tmpbuf4)+1;
#endif /* NEWCSCAN */
        }
        else if (tmp->voice) {
            sprintf(tmpbuf4,"%s+%s",CmdsColors[COLNICK].color5,Colors[COLOFF]);
            malloc_strcat(&users,tmpbuf4);
#ifndef NEWCSCAN
            buflen+=strlen(tmpbuf4)+1;
#endif /* NEWCSCAN */
        }
#else  /* WANTANSI */
        if (tmp->chanop) malloc_strcat(&users,"@");
        else if (tmp->voice) malloc_strcat(&users,"+");
#ifndef NEWCSCAN
        buflen+=2;
#endif /* NEWCSCAN */
#endif /* WANTANSI */

#ifdef NEWCSCAN
        else malloc_strcat(&users," ");
#endif /* NEWCSCAN */

#ifdef WANTANSI
        malloc_strcat(&users,colorstr);
#ifndef NEWCSCAN
        buflen+=strlen(colorstr);
#endif /* NEWCSCAN */
#endif /* WANTANSI */

#ifndef NEWCSCAN
        malloc_strcat(&users,tmp->nick);
        buflen+=strlen(tmp->nick);
#else
        sprintf(tmpbuf2,tmpbuf1,tmp->nick);
        malloc_strcat(&users,tmpbuf2);
#endif /* NEWCSCAN */

#ifdef WANTANSI
        malloc_strcat(&users,Colors[COLOFF]);
#ifndef NEWCSCAN
        buflen+=4;
#endif /* NEWCSCAN */
#endif /* WANTANSI */

#ifndef NEWCSCAN
        if (buflen>=BIG_BUFFER_SIZE-100) {
            put_it("%s",users);
            buflen=0;
            sprintf(tmpbuf2,tmpbuf3," ");
            malloc_strcpy(&users,tmpbuf2);
        }
#else
        count++;
        if ((count%width)==0) {
            say("%s",users);
            count=0;
            new_free(&users);
        }
#endif /* NEWCSCAN */
        tmp=tmp->next;
    }
#ifndef NEWCSCAN
    if (users && buflen) put_it("%s",users);
#else
    if (count) say("%s",users);
#endif
    new_free(&users);
}

/* Lists all the friends you have on your friends list */
void ListFriends(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    DontHold=0;
    if (args && *args) malloc_strcpy(&tmpbuflist,args);
    else malloc_strcpy(&tmpbuflist,"*");
    tmpfriendlist=frlist;
    countall=0;
    listcount=0;
    say("No.  %-38s Access      P Channels","User");
    ListFriendsPage(tmpbuflist);
}

/* Lists one page of friends list */
void ListFriendsPage(line)
char *line;
{
    int  count=3;
#ifdef WANTANSI
    char *tmpstr;
    int  i;
#endif
    char tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
#endif

    while (count<curr_scr_win->display_size && tmpfriendlist!=NULL) {
        countall++;
        if ((*line=='#' && CheckChannel2(line,tmpfriendlist->channels)) ||
            wild_match(line,tmpfriendlist->userhost) ||
            wild_match(tmpfriendlist->userhost,line)) {
            listcount++;
            count++;
            *tmpbuf1='\0';
            BuildPrivs(tmpfriendlist,tmpbuf1);
#ifdef WANTANSI
            sprintf(tmpbuf2,"%s%-11s%s",
                    CmdsColors[COLSETTING].color2,tmpbuf1,Colors[COLOFF]);
            strcpy(tmpbuf1,tmpfriendlist->userhost);
            i=strlen(tmpbuf1);
            if ((tmpstr=index(tmpbuf1,'@'))) {
                *tmpstr='\0';
                tmpstr++;
                sprintf(tmpbuf3,"%s%s%s%s@%s%s%s%s",
                        CmdsColors[COLSETTING].color4,tmpbuf1,Colors[COLOFF],
                        CmdsColors[COLMISC].color1,Colors[COLOFF],
                        CmdsColors[COLSETTING].color4,tmpstr,Colors[COLOFF]);
                strcpy(tmpbuf1,tmpbuf3);
            }
            else
                sprintf(tmpbuf1,"%s%s%s",
                        CmdsColors[COLSETTING].color4,tmpfriendlist->userhost,Colors[COLOFF]);
            for (;i<38;i++) strcat(tmpbuf1," ");
            sprintf(tmpbuf3,"#%-3d %s %s %s",tmpfriendlist->number,tmpbuf1,tmpbuf2,
                    tmpfriendlist->passwd?"Y":"N");
            say("%s %s%s%s",tmpbuf3,
                CmdsColors[COLSETTING].color5,tmpfriendlist->channels,Colors[COLOFF]);
#else
            say("#%-3d %-38s %-11s %s %s",countall,tmpfriendlist->userhost,
                tmpbuf1,tmpfriendlist->passwd?"Y":"N",tmpfriendlist->channels);
#endif
        }
        tmpfriendlist=tmpfriendlist->next;
    }
    if (!tmpfriendlist) ListFriendsKey(line,"E");
    else if (DontHold) ListFriendsPage(line);
    else if (count>=curr_scr_win->display_size) add_wait_prompt("Press any key to continue, 'c' for continuous, 'q' to quit",ListFriendsKey,line,WAIT_PROMPT_KEY);
}

/* This waits for key press */
void ListFriendsKey(stuff,line)
char *stuff;
char *line;
{
    if (line && (*line=='q' || *line=='Q' || *line=='E')) {
        if (*line=='E') say("%d out of %d entries matched your filter",listcount,
                            countall);
        return;
    }
    else {
        if (line && (*line=='c' || *line=='C')) DontHold=1;
        ListFriendsPage(stuff);
    }
}

/* Lists all the people you have on your auto (ban) kick list */
void ListAutoBanKicks(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    DontHold=0;
    if (args && *args) malloc_strcpy(&tmpbuflist,args);
    else malloc_strcpy(&tmpbuflist,"*");
    tmpabklist=abklist;
    listcount=0;
    countall=0;
    say("The following people match %s on shit list :",tmpbuflist);
    ListABKsPage(tmpbuflist);
}


/* Returns shit level as flags */
void BuildShit(shitentry,buffer)
struct autobankicks *shitentry;
char *buffer;
{
    if (shitentry && shitentry->shit) {
        if ((shitentry->shit)&SLKICK) strcat(buffer,"K");
        if ((shitentry->shit)&SLBAN) strcat(buffer,"B");
        if ((shitentry->shit)&SLIGNORE) strcat(buffer,"I");
        if ((shitentry->shit)&SLPERMBAN) strcat(buffer,"P");
        if ((shitentry->shit)&SLDEOP) strcat(buffer,"D");
    }
}

/* This lists one page of auto (ban) kicks list */
void ListABKsPage(line)
char *line;
{
    int  count=2;
#ifdef WANTANSI
    char *tmpstr;
#endif
    char tmpbuf1[mybufsize/4];
#ifdef WANTANSI
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
#endif

    while (count<curr_scr_win->display_size && tmpabklist) {
        countall++;
        if ((*line=='#' && CheckChannel(line,tmpabklist->channels)) ||
            wild_match(line,tmpabklist->userhost) ||
            wild_match(tmpabklist->userhost,line)) {
            listcount++;
            count++;
            count++;
            *tmpbuf1='\0';
            BuildShit(tmpabklist,tmpbuf1);
#ifdef WANTANSI
            strcpy(tmpbuf2,tmpabklist->userhost);
            if ((tmpstr=index(tmpbuf2,'@'))) {
                *tmpstr='\0';
                tmpstr++;
                sprintf(tmpbuf3,"%s%s%s%s@%s%s%s%s",
                        CmdsColors[COLSETTING].color4,tmpbuf2,Colors[COLOFF],
                        CmdsColors[COLMISC].color1,Colors[COLOFF],
                        CmdsColors[COLSETTING].color4,tmpstr,Colors[COLOFF]);
            }
            else
                sprintf(tmpbuf3,"%s%s%s",
                        CmdsColors[COLSETTING].color4,tmpabklist->userhost,Colors[COLOFF]);
            say("#%-2d %s%-3s%s %s",countall,
                CmdsColors[COLSETTING].color2,tmpbuf1,Colors[COLOFF],tmpbuf3);
            say("    on channels %s%s%s : %s%s%s",
                CmdsColors[COLSETTING].color5,tmpabklist->channels,
                Colors[COLOFF],CmdsColors[COLSETTING].color3,
                tmpabklist->reason,Colors[COLOFF]);
#else
            say("#%-2d %-3s  %-45s",countall,tmpbuf1,tmpabklist->userhost);
            say("    on channels %s : %s",tmpabklist->channels,tmpabklist->reason);
#endif
        }
        tmpabklist=tmpabklist->next;
    }
    if (!tmpabklist) ListABKsKey(NULL,"E");
    else if (DontHold) ListABKsPage(line);
    else if (count>=curr_scr_win->display_size) add_wait_prompt("Press any key to continue, 'c' for continuous, 'q' to quit",ListABKsKey,line,WAIT_PROMPT_KEY);
}

/* This waits for key press */
void ListABKsKey(stuff,line)
char *stuff;
char *line;
{
    if (line && (*line=='q' || *line=='Q' || *line=='E')) {
        if (*line=='E') say("%d out of %d entries matched your filter",listcount,
                            countall);
        return;
    }
    else {
        if (line && (*line=='c' || *line=='C')) DontHold=1;
        ListABKsPage(stuff);
    }
}

/* Shows or sets topic for current or specified channel */
void Topic(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *tmparg;
    ChannelList *chan;

    channel=get_channel_by_refnum(0);
    if (!(args && *args)) {
        if (channel && (chan=lookup_channel(channel,from_server,0))) {
            say("Topic for %s: %s",chan->channel,
                chan->topicstr?chan->topicstr:"No topic is set.");
            if (chan->topicwho)
                say("%s by %s on %.19s",chan->topicstr?"Set":"Unset",
                    chan->topicwho,ctime(&(chan->topicwhen)));
        }
        else NoWindowChannel();
    }
    else {
        if (!is_channel(args)) {
            if (channel) {
                if ((chan=lookup_channel(channel,from_server,0))) {
                    if (((chan->mode)&MODE_TOPIC) && ((chan->status)&CHAN_CHOP))
                        send_to_server("TOPIC %s :%s",channel,args);
                    else if ((chan->mode)&MODE_TOPIC) NotChanOp(channel);
                    else send_to_server("TOPIC %s :%s",channel,args);
                }
            }
            else NoWindowChannel();
        }
        else {
            tmparg=new_next_arg(args,&args);
            if ((chan=lookup_channel(tmparg,from_server,0))) {
                if (((chan->mode)&MODE_TOPIC) && ((chan->status)&CHAN_CHOP))
                    send_to_server("TOPIC %s :%s",chan->channel,args);
                else if ((chan->mode)&MODE_TOPIC) NotChanOp(chan->channel);
                else send_to_server("TOPIC %s :%s",chan->channel,args);
            }
            else say("You are not on channel %s",tmparg);
        }
    }
}

/* Connects to default or specified server */
void FServer(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    if (args && *args) server(NULL,args,NULL);
    else if (my_stricmp(DefaultServer,"NONE")) server(NULL,DefaultServer,NULL);
    else say("No default server specified");
}

/* Leaves and joins current or specified channel */
void Cycle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpchannel=(char *) 0;
    char tmpbuf[mybufsize/4];
    ChannelList *chan;

    if (args && *args) {
        tmpchannel=new_next_arg(args,&args);
        if (!is_channel(tmpchannel)) sprintf(tmpbuf,"#%s",tmpchannel);
        else strcpy(tmpbuf,tmpchannel);
        tmpchannel=tmpbuf;
    }
    else if ((tmpchannel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    if ((chan=lookup_channel(tmpchannel,from_server,0))) {
        strcpy(tmpbuf,tmpchannel);
        if (chan->key) {
            strcat(tmpbuf," :");
            strcat(tmpbuf,chan->key);
        }
        send_to_server("PART %s",tmpchannel);
        e_channel("JOIN",tmpbuf,tmpbuf);
    }
}

/* Saves friends into ScrollZ.save file */
void ScrollZSave(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  ulistcount;
    int  slistcount;
    int  nlistcount;
    int  wlistcount;
    char *filepath;
    char *filebak;
    char tmpbuf1[mybufsize/4];
    FILE *usfile;
    NotifyList *notify;
    struct words *tmpword;
    struct friends *tmpfriend;
    struct autobankicks *tmpabk;

    if ((filebak=OpenCreateFile("ScrollZ.bak",1))) remove(filebak);
    if ((filepath=OpenCreateFile("ScrollZ.save",0))) rename(filepath,filebak);
    if (!(filepath=OpenCreateFile("ScrollZ.save",1)) ||
        (usfile=fopen(filepath,"w"))==NULL) {
        say("Can't open file ScrollZ.save for writing !");
        return;
    }
    say("Saving all ScrollZ related stuff into ScrollZ.save");
    fprintf(usfile,"#\n");
    fprintf(usfile,"# This is ScrollZ save file\n");
#ifdef CELE
    fprintf(usfile,"# Some information below is used only by %s.\n",CelerityVersion);
#endif
    fprintf(usfile,"#\n");
    fprintf(usfile,"#\n");
    fprintf(usfile,"# Friends list\n");
    fprintf(usfile,"# ADDF   Nick!User@Host    Flags         Channels      [Password]\n");
    fprintf(usfile,"# where flags can be :\n");
    fprintf(usfile,"#       I=INVITE   C=CHOPS   O=OP      A=AUTO   U=UNBAN   P=PROT\n");
    fprintf(usfile,"#       D=CDCC     G=GOD     V=VOICE   J=JOIN   F=NOFLOOD\n");
    fprintf(usfile,"# channels is a list of channels separated by ','  like #sex*,#test,#chan\n");
    fprintf(usfile,"#\n");
    for (tmpfriend=frlist,ulistcount=0;tmpfriend;tmpfriend=tmpfriend->next) {
        *tmpbuf1='\0';
        BuildPrivs(tmpfriend,tmpbuf1);
        fprintf(usfile,"ADDF  %-40s   %-11s   %-20s",
                tmpfriend->userhost,tmpbuf1,tmpfriend->channels);
        if (tmpfriend->passwd) fprintf(usfile," %s",tmpfriend->passwd);
        fprintf(usfile,"\n");
        ulistcount++;
    }
    fprintf(usfile,"#\n");
    fprintf(usfile,"# Shit list\n");
    fprintf(usfile,"# ADDBK   Nick!User@Host    Shit-flags    Channels    Comment\n");
    fprintf(usfile,"# where shit-flags can be :\n");
    fprintf(usfile,"#         K=KICK   B=BAN   I=IGNORE   P=PERMBAN   D=DEOP\n");
    fprintf(usfile,"# channels is a list of channels separated by ','  like #sex*,#test,#chan\n");
    fprintf(usfile,"#\n");
    for (tmpabk=abklist,slistcount=0;tmpabk;tmpabk=tmpabk->next) {
        *tmpbuf1='\0';
        BuildShit(tmpabk,tmpbuf1);
        fprintf(usfile,"ADDBK %-40s   %-5s   %-20s   %s\n",
                tmpabk->userhost,tmpbuf1,tmpabk->channels,tmpabk->reason);
        slistcount++;
    }
    fprintf(usfile,"#\n");
    fprintf(usfile,"# Notify list\n");
    fprintf(usfile,"# ADDN   Nick1   Nick2   Nick3\n");
    fprintf(usfile,"#\n");
    nlistcount=0;
    *tmpbuf1='\0';
    for (notify=notify_list;notify;notify=notify->next) {
        if (*tmpbuf1) strcat(tmpbuf1,"  ");
        strcat(tmpbuf1,notify->nick);
        if (notify->mask) {
            strcat(tmpbuf1,"!");
            strcat(tmpbuf1,notify->mask);
        }
        nlistcount++;
        if ((nlistcount%3)==0) {
            fprintf(usfile,"ADDN  %s\n",tmpbuf1);
            *tmpbuf1='\0';
        }
    }
    if (nlistcount && (nlistcount%3)) fprintf(usfile,"ADDN  %s\n",tmpbuf1);
    fprintf(usfile,"#\n");
    fprintf(usfile,"# Word kick list\n");
    fprintf(usfile,"# ADDW   channels word comment\n");
    fprintf(usfile,"#\n");
    for (tmpword=wordlist,wlistcount=0;tmpword;tmpword=tmpword->next) {
        fprintf(usfile,"ADDW %15s %20s %s\n",
                tmpword->channels,tmpword->word,tmpword->reason);
        wlistcount++;
    }
    say("Saved %d fl, %d sl, %d nl and %d wk entries",ulistcount,slistcount,nlistcount,
        wlistcount);
    fprintf(usfile,"#\n");
    fprintf(usfile,"# Various user definable settings\n");
    fprintf(usfile,"#\n");
    fprintf(usfile,"EXTMES          ");
    if (ExtMes) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"NHPROT          ");
    if (NHProt) fprintf(usfile,"ON %s ",NHProtChannels);
    else fprintf(usfile,"OFF ");
    if (NHDisp==0) fprintf(usfile,"QUIET\n");
    if (NHDisp==1) fprintf(usfile,"MEDIUM\n");
    if (NHDisp==2) fprintf(usfile,"FULL\n");
    fprintf(usfile,"BANTYPE         ");
    if (defban=='N') fprintf(usfile,"NORMAL\n");
    else if (defban=='B') fprintf(usfile,"BETTER\n");
    else if (defban=='E') fprintf(usfile,"ELITE\n");
    else if (defban=='H') fprintf(usfile,"HOST\n");
    else if (defban=='S') fprintf(usfile,"SCREW\n");
    fprintf(usfile,"AUTOAWTIME      %d\n",AutoAwayTime);
    fprintf(usfile,"IGNORETIME      %d\n",IgnoreTime);
    fprintf(usfile,"SHITIGNORETIME  %d\n",ShitIgnoreTime);
    fprintf(usfile,"MDOPWATCH       ");
    if (MDopWatch) fprintf(usfile,"ON %s\n",MDopWatchChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"KICKWATCH       ");
    if (KickWatch) fprintf(usfile,"ON %s\n",KickWatchChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"NICKWATCH       ");
    if (NickWatch) fprintf(usfile,"ON %s\n",NickWatchChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"AUTOREJOIN      ");
    if (AutoRejoin) fprintf(usfile,"ON %s\n",AutoRejoinChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"AUTOJOININV     ");
    if (AutoJoinOnInv==2) fprintf(usfile,"AUTO %s\n",AutoJoinChannels);
    else if (AutoJoinOnInv) fprintf(usfile,"ON %s\n",AutoJoinChannels);
    else fprintf(usfile,"OFF\n");
#if defined(EXTRAS) || defined(FLIER)
    fprintf(usfile,"AUTOINV         ");
    if (AutoInv) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
#endif
    fprintf(usfile,"FLOODPROT       ");
    if (FloodProt>1) fprintf(usfile,"MAX %d %d\n",FloodMessages,FloodSeconds);
    else if (FloodProt) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"SERVNOTICE      ");
    if (ServerNotice) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"CTCPCLOAKING    ");
    if (CTCPCloaking==1) fprintf(usfile,"ON\n");
    else if (CTCPCloaking==2) fprintf(usfile,"HIDE\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"SHOWFAKES       ");
    if (ShowFakes) fprintf(usfile,"ON %s\n",ShowFakesChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"SHOWAWAY        ");
    if (ShowAway) fprintf(usfile,"ON %s\n",ShowAwayChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"KICKOPS         ");
    if (KickOps) fprintf(usfile,"ON %s\n",KickOpsChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"KICKONFLOOD     ");
    if (KickOnFlood) fprintf(usfile,"ON %s\n",KickOnFloodChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"SHOWNICK        ");
    if (ShowNick) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"KICKONBAN       ");
    if (KickOnBan) fprintf(usfile,"ON %s\n",KickOnBanChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"BITCHMODE       ");
    if (Bitch) fprintf(usfile,"ON %s\n",BitchChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"FRIENDLIST      ");
    if (FriendList) fprintf(usfile,"ON %s\n",FriendListChannels);
    else fprintf(usfile,"OFF\n");
#ifdef EXTRAS
    fprintf(usfile,"IDLEKICK        ");
    if (IdleKick==1) fprintf(usfile,"ON %s\n",IdleKickChannels);
    else if (IdleKick==2) fprintf(usfile,"AUTO %s\n",IdleKickChannels);
    else fprintf(usfile,"OFF\n");
#endif
    fprintf(usfile,"COMPRESSMODES   ");
    if (CompressModes) fprintf(usfile,"ON %s\n",CompressModesChannels);
    else fprintf(usfile,"OFF\n");
#ifdef EXTRAS
    fprintf(usfile,"SIGNOFFCHANNELS ");
    if (ShowSignoffChan) fprintf(usfile,"ON %s\n",SignoffChannels);
    else fprintf(usfile,"OFF\n");
#endif
    fprintf(usfile,"STAMP           ");
    if (Stamp) fprintf(usfile,"ON %s\n",StampChannels);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"DEOPSENSOR      %d\n",DeopSensor);
    fprintf(usfile,"KICKSENSOR      %d\n",KickSensor);
    fprintf(usfile,"NICKSENSOR      %d\n",NickSensor);
    fprintf(usfile,"DEOPTIMER       %d\n",MDopTimer);
    fprintf(usfile,"KICKTIMER       %d\n",KickTimer);
    fprintf(usfile,"NICKTIMER       %d\n",NickTimer);
    fprintf(usfile,"AUTOOPTIME      %d\n",AutoOpDelay);
    fprintf(usfile,"ORIGNICKTIME    %d\n",OrigNickDelay);
#ifdef EXTRAS
    fprintf(usfile,"IDLETIME        %d\n",IdleTime);
#endif
    fprintf(usfile,"DEFSERVER       %s\n",DefaultServer);
    fprintf(usfile,"DEFSIGNOFF      %s\n",DefaultSignOff);
    fprintf(usfile,"DEFSETAWAY      %s\n",DefaultSetAway);
    fprintf(usfile,"DEFSETBACK      %s\n",DefaultSetBack);
    fprintf(usfile,"DEFUSERINFO     %s\n",DefaultUserinfo);
    fprintf(usfile,"DEFFINGER       %s\n",DefaultFinger);
    fprintf(usfile,"DEFK            %s\n",DefaultK);
    fprintf(usfile,"DEFBK           %s\n",DefaultBK);
    fprintf(usfile,"DEFBKI          %s\n",DefaultBKI);
    fprintf(usfile,"DEFBKT          %s\n",DefaultBKT);
    fprintf(usfile,"DEFFK           %s\n",DefaultFK);
    fprintf(usfile,"DEFLK           %s\n",DefaultLK);
    fprintf(usfile,"DEFABK          %s\n",DefaultABK);
    fprintf(usfile,"DEFSK           %s\n",DefaultSK);
#ifdef ACID
    fprintf(usfile,"DEFKILL         %s\n",DefaultKill);
#endif
    fprintf(usfile,"SHOWWALLOP      ");
    if (ShowWallop) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    if (AutoReplyBuffer) fprintf(usfile,"AUTOREPLY       %s\n",AutoReplyBuffer);
    fprintf(usfile,"ORIGNICK        ");
    if (OrigNickChange) fprintf(usfile,"ON %s\n",OrigNick);
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"NOTIFYMODE      ");
    if (NotifyMode==2) fprintf(usfile,"Verbose\n");
    else fprintf(usfile,"Brief\n");
    fprintf(usfile,"URLCATCHER      ");
    if (URLCatch==3) fprintf(usfile,"QUIET\n");
    else if (URLCatch==2) fprintf(usfile,"AUTO\n");
    else if (URLCatch) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"EGO             ");
    if (Ego) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"AUTOCOMPLETION  ");
    if (AutoNickCompl) fprintf(usfile,"%s %s\n",AutoNickCompl==1?"ON":"AUTO",
                               AutoReplyString);
    else fprintf(usfile,"OFF\n");
#ifdef WANTANSI
    fprintf(usfile,"MIRCCOLORS      ");
    if (DisplaymIRC) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
#endif
    if (PermUserMode) fprintf(usfile,"USERMODE        %s\n",PermUserMode);
    fprintf(usfile,"#\n");
    fprintf(usfile,"NOTIFYSTR       %s\n",CelerityNtfy);
    fprintf(usfile,"#\n");
    fprintf(usfile,"# ScrollZ.away defines\n");
    fprintf(usfile,"# AWAYSAVE      type\n");
    fprintf(usfile,"# where type can be : ALL or MSG, NOTICE, MASS, COLL, CDCC, DCC, PROT, HACK, SRVMODE\n");
    fprintf(usfile,"#                     CTCP, FLOOD, INVITE, KILL, KICK, SERVER or FAKE\n");
    fprintf(usfile,"#        ALL=524287   MSG=1   NOTICE=2   MASS=4    COLL=8    CDCC=16    DCC=32    PROT=64    HACK=128\n");
    fprintf(usfile,"#                     SRVMODE=256   CTCP=512   FLOOD=1024  INVITE=2048  KILL=4096  KICK=8192  SERVER=16384\n");
    fprintf(usfile,"#                     FAKE=32768   AREPLY=65536   CHAT=131072   NOTIFY=262144\n");
    fprintf(usfile,"#\n");
    fprintf(usfile,"AWAYSAVE        %d\n",AwaySaveSet);
    fprintf(usfile,"#\n");
    fprintf(usfile,"# Cdcc related settings\n");
    fprintf(usfile,"#\n");
    fprintf(usfile,"CDCC LIMIT      %d %d\n",CdccLimit,CdccQueueLimit);
    fprintf(usfile,"CDCC IDLE       %d\n",CdccIdle);
    fprintf(usfile,"CDCC AUTOGET    ");
    if (AutoGet) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"CDCC SECURE     ");
    if (Security) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    if (CdccChannels) fprintf(usfile,"CDCC CHANNELS   %s\n",CdccChannels);
    fprintf(usfile,"CDCC PTIME      %d\n",PlistTime);
    fprintf(usfile,"CDCC NTIME      %d\n",NlistTime);
    fprintf(usfile,"CDCC LONGSTATUS ");
    if (LongStatus) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"CDCC OVERWRITE  ");
    if (CdccOverWrite) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"CDCC STATUS     ");
    if (ShowDCCStatus) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"CDCC STATS      ");
    if (CdccStats) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    fprintf(usfile,"CDCC WARNING    ");
    if (DCCWarning) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
    if (CdccUlDir) fprintf(usfile,"CDCC ULDIR      %s\n",CdccUlDir);
    if (CdccDlDir) fprintf(usfile,"CDCC DLDIR      %s\n",CdccDlDir);
#ifdef EXTRA_STUFF
    fprintf(usfile,"CDCC E          %s\n",EString);
    fprintf(usfile,"CDCC M          ");
    if (RenameFiles) fprintf(usfile,"ON\n");
    else fprintf(usfile,"OFF\n");
#endif
#ifdef WANTANSI
    SaveColors(usfile);
#endif
    fclose(usfile);
    say("ScrollZ.save succesfully saved !");
}

/* Returns friend's privilege (if it finds him/her on your friends list, else 0 */
struct friends *CheckUsers(userhost,channels)
char *userhost;
char *channels;
{
    struct friends *tmpfriend;

    if (!userhost) return(NULL);
    tmpfriend=frlist;
    if (!channels) {
        for (;tmpfriend;tmpfriend=tmpfriend->next)
            if (wild_match(tmpfriend->userhost,userhost) ||
                wild_match(userhost,tmpfriend->userhost))
                return(tmpfriend);
    }
    else {
        for (;tmpfriend;tmpfriend=tmpfriend->next)
            if ((wild_match(tmpfriend->userhost,userhost) ||
                 wild_match(userhost,tmpfriend->userhost))
                && CheckChannel(channels,tmpfriend->channels))
                return(tmpfriend);
    }
    return(NULL);
}

/* Returns autobankicker's shit-level (if it finds him/her on your abks list, else 0 */
struct autobankicks *CheckABKs(userhost,channel)
char *userhost;
char *channel;
{
    struct autobankicks *tmpabk;

    tmpabk=abklist;
    if (!channel) {
        while (tmpabk) {
            if (wild_match(tmpabk->userhost,userhost) ||
                wild_match(userhost,tmpabk->userhost))
                break;
            tmpabk=tmpabk->next;
        }
    }
    else {
        while (tmpabk) {
            if ((wild_match(tmpabk->userhost,userhost) ||
                 wild_match(userhost,tmpabk->userhost))
                && CheckChannel(channel,tmpabk->channels)) break;
            tmpabk=tmpabk->next;
        }
    }
    return(tmpabk);
}

/* Finds nick on given channel and server or just server if no channel */
NickList *CheckJoiners(nick,channel,server,tmpchan)
char *nick;
char *channel;
int  server;
ChannelList *tmpchan;
{
    NickList *tmp=NULL;
    register ChannelList *chan;

    if (channel) {
        if (tmpchan) chan=tmpchan;
        else chan=lookup_channel(channel,server,0);
        if (chan) tmp=find_in_hash(chan,nick);
        return(tmp);
    }
    else {
        for (chan=server_list[server].chan_list;chan;chan=chan->next) {
            tmp=find_in_hash(chan,nick);
            if (tmp) return(tmp);
        }
    }
    return((NickList *) 0);
}

/* Redirects last message/notice to current or specified channel */
void DirLM(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  msg=1;
    char *tmparg;
    char *message=LastMessage;
    char *channel=(char *) 0;

    if (!my_stricmp(command,"DIRLN")) {
        message=LastNotice;
        msg=0;
    }
    if (message) {
        if (!(*args)) {
            channel=get_channel_by_refnum(0);
            if (channel) send_text(channel,message,"PRIVMSG");
            else NoWindowChannel();
        }
        else {
            tmparg=new_next_arg(args,&args);
            send_text(tmparg,message,"PRIVMSG");
            AddNick2List(tmparg,from_server);
        }
    }
    else say("You haven't received any %s so far",msg?"message":"notice");
}

/* Adds friend to your friends list with specified privileges */
void AddFriend(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick=(char *) 0;
    char *tmpchan=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
    void (*func)();
    NickList *joiner;

    tmpnick=new_next_arg(args,&args);
    tmpchan=new_next_arg(args,&args);
    if (!(args && *args)) {
        PrintUsage("ADDF nick/filter channels privileges [password]");
        return;
    }
    sprintf(tmpbuf1,"%s %s",tmpchan,args);
    if (strchr(tmpnick,'!') || strchr(tmpnick,'@') || strchr(tmpnick,'*') ||
        strchr(tmpnick,'.') || strchr(tmpnick,'?'))
        AddFriend2List(NULL,tmpnick,tmpbuf1);
    else {
        joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
        if (joiner && joiner->userhost) {
            strcpy(tmpbuf2,joiner->userhost);
            UserDomainList(tmpbuf2);
            sprintf(tmpbuf3,"*!%s",tmpbuf2);
            AddFriend2List(joiner->nick,tmpbuf3,tmpbuf1);
        }
        else {
            func=(void(*)())AddFriendNew;
            inFlierWI++;
            add_to_whois_queue(tmpnick,func,"%s",tmpbuf1);
        }
    }
}

/* Adds friend to your friends list if he/she is not on joinlist */
void AddFriendNew(wistuff, tmpnick, text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    sprintf(tmpbuf1,"%s@%s",wistuff->user,wistuff->host);
    UserDomainList(tmpbuf1);
    sprintf(tmpbuf2,"*!%s",tmpbuf1);
    AddFriend2List(wistuff->nick,tmpbuf2,text);
}

/* Returns the integer from specified privileges */
int CheckPrivs(privs,buffer)
char *privs;
char *buffer;
{
    int  i=0;
    char *tmpprivs=(char *) 0;

    if (buffer) *buffer='\0';
    tmpprivs=privs;
    if (tmpprivs && *tmpprivs && !my_stricmp(tmpprivs,"ALL")) {
        i=FLINVITE | FLCHOPS | FLOP | FLAUTOOP | FLUNBAN | FLPROT | FLCDCC | FLNOFLOOD;
        if (buffer) strcpy(buffer,"INVITE CHOPS OP UNBAN CDCC");
        return(i);
    }
    for (;tmpprivs && *tmpprivs;tmpprivs++) {
        switch (*tmpprivs) {
            case 'i':
            case 'I':
                if (i&FLINVITE) break;
                i|=FLINVITE;
                if (buffer) strcat(buffer,"INVITE ");
                break;
            case 'c':
            case 'C':
                if (i&FLCHOPS) break;
                i|=FLCHOPS;
                if (buffer) strcat(buffer,"CHOPS ");
                break;
            case 'o':
            case 'O':
                if (i&FLOP) break;
                i|=FLOP;
                if (buffer) strcat(buffer,"OP ");
                break;
            case 'a':
            case 'A':
                i|=FLAUTOOP;
                break;
            case 'u':
            case 'U':
                if (i&FLUNBAN) break;
                i|=FLUNBAN;
                if (buffer) strcat(buffer,"UNBAN ");
                break;
            case 'p':
            case 'P':
                i|=FLPROT;
                break;
            case 'd':
            case 'D':
                if (i&FLCDCC) break;
                i|=FLCDCC;
                if (buffer) strcat(buffer,"CDCC ");
                break;
            case 'g':
            case 'G':
                i|=FLGOD;
                break;
            case 'v':
            case 'V':
                if (i&FLVOICE) break;
                i|=FLVOICE;
                if (buffer) strcat(buffer,"VOICE ");
                break;
            case 'j':
            case 'J':
                i|=FLJOIN;
                break;
            case 'f':
            case 'F':
                i|=FLNOFLOOD;
                break;
        }
    }
    return(i);
}

/* Returns Y/N */
char *YNreply(what)
int what;
{
    if (what) return("Y");
    else return("N");
}

/* This really adds him/her to your friends list */
void AddFriend2List(nick,userhost,buffer)
char *nick;
char *userhost;
char *buffer;
{
    int  i=0;
    int  number=1;
    char *tmpstr=(char *) 0;
    char *chanlist=(char *) 0;
    char *privs=(char *) 0;
    char *passwd=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    NickList *tmp;
    ChannelList *tmpchan;
    struct friends *tmpfriend;
    struct friends *tmpfriend1;

    tmpstr=buffer;
    chanlist=new_next_arg(tmpstr,&tmpstr);
    privs=new_next_arg(tmpstr,&tmpstr);
    passwd=new_next_arg(tmpstr,&tmpstr);
    i=CheckPrivs(privs,tmpbuf2);
    if (i) {
        if (CheckUsers(userhost,chanlist)) {
            for (tmpfriend=frlist;tmpfriend;tmpfriend=tmpfriend->next)
                if ((wild_match(tmpfriend->userhost,userhost) ||
                     wild_match(userhost,tmpfriend->userhost))
                    && CheckChannel(chanlist,tmpfriend->channels)) break;
        }
        else {
            tmpfriend=(struct friends *) new_malloc(sizeof(struct friends));
            tmpfriend->userhost=(char *) 0;
            tmpfriend->channels=(char *) 0;
            tmpfriend->passwd=(char *) 0;
            tmpfriend->next=NULL;
            for (tmpfriend1=frlist;tmpfriend1 && tmpfriend1->next;tmpfriend1=tmpfriend1->next)
                number++;
            if (tmpfriend1) {
                tmpfriend1->next=tmpfriend;
                number++;
            }
            else frlist=tmpfriend;
            tmpfriend->number=number;
        }
        for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
             tmpchan=tmpchan->next)
            if (CheckChannel(tmpchan->channel,chanlist)) {
                for (tmp=tmpchan->nicks;tmp;tmp=tmp->next) {
                    if (tmp->userhost) sprintf(tmpbuf1,"%s!%s",tmp->nick,tmp->userhost);
                    else strcpy(tmpbuf1,tmp->nick);
                    if (wild_match(userhost,tmpbuf1)) tmp->frlist=tmpfriend;
                }
            }
        malloc_strcpy(&(tmpfriend->userhost),userhost);
        malloc_strcpy(&(tmpfriend->channels),chanlist);
        if (passwd && *passwd) {
            EncryptString(tmpbuf1,passwd,passwd,mybufsize/16);
            malloc_strcpy(&(tmpfriend->passwd),tmpbuf1);
        }
        else new_free(&(tmpfriend->passwd));
        tmpfriend->privs=i;
        synch_whowas_adduser(tmpfriend);
        sprintf(tmpbuf1,"Auto:%s  Prot:%s  No flood:%s  God:%s",
                YNreply(i&FLAUTOOP),YNreply(i&FLPROT),YNreply(i&FLNOFLOOD),
                YNreply(i&FLGOD));
        if (nick) {
            send_to_server("NOTICE %s :-ScrollZ- You have been added to my friends list with",nick);
            send_to_server("NOTICE %s :-ScrollZ- CTCP access of : %s",nick,tmpbuf2);
            send_to_server("NOTICE %s :-ScrollZ- Commands are valid on channel(s) : %s",nick,chanlist);
            send_to_server("NOTICE %s :-ScrollZ- %s",nick,tmpbuf1);
            if (i&FLJOIN) send_to_server("NOTICE %s :-ScrollZ- I will auto-join above channel(s) when invited by you.",nick);
            if (tmpfriend->passwd)
                send_to_server("NOTICE %s :-ScrollZ- Your password is %s",nick,passwd);
        }
#ifdef WANTANSI
        say("Added %s%s%s to your friends list",
            CmdsColors[COLSETTING].color4,userhost,Colors[COLOFF]);
        say("with CTCP access of : %s%s%s",
            CmdsColors[COLSETTING].color2,tmpbuf2,Colors[COLOFF]);
        say("on channels %s%s%s",
            CmdsColors[COLSETTING].color5,chanlist,Colors[COLOFF]);
        sprintf(tmpbuf1,"Auto:%s%s%s  Prot:%s%s%s",
                CmdsColors[COLSETTING].color2,YNreply(i&FLAUTOOP),Colors[COLOFF],
                CmdsColors[COLSETTING].color2,YNreply(i&FLPROT),Colors[COLOFF]);
        say("%s  No flood:%s%s%s  God:%s%s%s",tmpbuf1,
            CmdsColors[COLSETTING].color2,YNreply(i&FLNOFLOOD),Colors[COLOFF],
            CmdsColors[COLSETTING].color2,YNreply(i&FLGOD),Colors[COLOFF]);
        if (i&FLJOIN) say("You will join channels %s%s%s invites you to",
                       CmdsColors[COLSETTING].color4,userhost,Colors[COLOFF]);
#else
        say("Added %s to your friends list",userhost);
        say("with CTCP access of : %s",tmpbuf2);
        say("on channels %s",chanlist);
        say("%s",tmpbuf1);
        if (i&FLJOIN) say("You will join channels %s invites you to",userhost);
#endif
        if (tmpfriend->passwd)
            say("Password is set to %s",passwd);
    }
    else PrintUsage("ADDF nick/filter channels privileges [password]");
}

/* Adds someone on your auto-ban-kick list */
void AddAutoBanKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;
    char *tmpchan;
    char *tmpshit;
    char *userhost;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
    void (*func)();
    NickList *joiner;

    tmpnick=new_next_arg(args,&args);
    tmpchan=new_next_arg(args,&args);
    tmpshit=new_next_arg(args,&args);
    if (!(tmpnick && tmpshit && tmpchan)) {
        PrintUsage("ADDBK nick/filter channels shit-level [comment]");
        return;
    }
    sprintf(tmpbuf1,"%s %s",tmpchan,args);
    if (strchr(tmpnick,'!') || strchr(tmpnick,'@') || strchr(tmpnick,'*') ||
        strchr(tmpnick,'.') || strchr(tmpnick,'?'))
        AddBK2List(tmpnick,tmpshit,tmpbuf1);
    else {
        joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
        if (joiner && joiner->userhost) {
            userhost=(joiner->userhost)+1;
            if (*userhost=='@') userhost--;
            strcpy(tmpbuf2,userhost);
            UserDomainList(tmpbuf2);
            sprintf(tmpbuf3,"*!%s",tmpbuf2);
            AddBK2List(tmpbuf3,tmpshit,tmpbuf1);
        }
        else {
            func=(void(*)())AddBKNew;
            inFlierWI++;
            sprintf(tmpbuf1,"%s %s %s",tmpshit,tmpchan,args);
            add_to_whois_queue(tmpnick,func,"%s",tmpbuf1);
        }
    }
}

/* This adds him/her on auto (ban) kick list if he/she is not on your joinlist */
void AddBKNew(wistuff, tmpnick, text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    char *tmpshit;
    char *username;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    tmpshit=new_next_arg(text,&text);
    username=(wistuff->user)+1;
    if (!(*username)) username--;
    sprintf(tmpbuf1,"%s@%s",username,wistuff->host);
    UserDomainList(tmpbuf1);
    sprintf(tmpbuf2,"*!%s",tmpbuf1);
    AddBK2List(tmpbuf2,tmpshit,text);
}

/* Returns integer from specified shit level */
int CheckShit(shit,buffer)
char *shit;
char *buffer;
{
    int i=0;
    char *tmpshit=(char *) 0;

    if (buffer) *buffer='\0';
    for (tmpshit=new_next_arg(shit,&shit);tmpshit && *tmpshit;tmpshit++) {
        switch (*tmpshit) {
            case 'k':
            case 'K':
                i|=SLKICK;
                if (buffer) strcat(buffer,"KICK ");
                break;
            case 'b':
            case 'B':
                i|=SLBAN;
                if (buffer) strcat(buffer,"BAN ");
                break;
            case 'i':
            case 'I':
                i|=SLIGNORE;
                if (buffer) strcat(buffer,"IGNORE ");
                break;
            case 'p':
            case 'P':
                i|=SLPERMBAN;
                if (buffer) strcat(buffer,"PERMBAN ");
                break;
            case 'd':
            case 'D':
                i|=SLDEOP;
                if (buffer) strcat(buffer,"DEOP ");
                break;
        }
    }
    return(i);
}

/* This really adds user to shitlist */
void AddBK2List(userhost,shit,reason)
char *userhost;
char *shit;
char *reason;
{
    int i=0;
    char *tmpchan=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    NickList *tmp;
    ChannelList *chan;
    struct autobankicks *tmpabk;

    tmpchan=new_next_arg(reason,&reason);
    i=CheckShit(shit,tmpbuf2);
    if (i) {
        if (!(tmpabk=CheckABKs(userhost,tmpchan))) {
            tmpabk=(struct autobankicks *) new_malloc(sizeof(struct autobankicks));
            tmpabk->userhost=(char *) 0;
            tmpabk->channels=(char *) 0;
            tmpabk->reason=(char *) 0;
            tmpabk->next=NULL;
            add_to_list_ext((List **) &abklist,(List *) tmpabk,
                            (int (*) _((List *, List *))) AddLast);
        }
        for (chan=server_list[curr_scr_win->server].chan_list;chan;chan=chan->next)
            if (CheckChannel(chan->channel,tmpchan)) {
                for (tmp=chan->nicks;tmp;tmp=tmp->next) {
                    if (tmp->userhost) sprintf(tmpbuf1,"%s!%s",tmp->nick,tmp->userhost);
                    else strcpy(tmpbuf1,tmp->nick);
                    if (wild_match(userhost,tmpbuf1)) tmp->shitlist=tmpabk;
                }
            }
        malloc_strcpy(&(tmpabk->userhost),userhost);
        malloc_strcpy(&(tmpabk->reason),reason);
        malloc_strcpy(&(tmpabk->channels),tmpchan);
        tmpabk->shit=i;
        synch_whowas_addshit(tmpabk);
#ifdef WANTANSI
        say("Added %s%s%s to your shit list",
            CmdsColors[COLSETTING].color4,userhost,Colors[COLOFF]);
        say("with shit type %s%s%son channels %s%s%s",
            CmdsColors[COLSETTING].color2,tmpbuf2,Colors[COLOFF],
            CmdsColors[COLSETTING].color5,tmpchan,Colors[COLOFF]);
#else
        say("Added %s to your shit list",userhost);
        say("with shit type %son channels %s",tmpbuf2,tmpchan);
#endif
    }
    else PrintUsage("ADDBK nick/filter channels shit-level [comment]");
}

/* Show bans on current or specified channel */
void ShowBans(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  count=0;
    char *channel=(char *) 0;
    char tmpbuf[mybufsize/4];
    ChannelList *chan;
    struct bans *tmpban;

    if (args && *args) {
        channel=new_next_arg(args,&args);
        if (!is_channel(channel)) sprintf(tmpbuf,"#%s",channel);
        else strcpy(tmpbuf,channel);
        channel=tmpbuf;
    }
    else if ((channel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    chan=lookup_channel(channel,from_server,0);
    if (chan) {
        for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next) {
            if (tmpban->who && tmpban->when)
                say("%s %-29s %s on %.19s",chan->channel,tmpban->ban,tmpban->who,
                    ctime(&(tmpban->when)));
            else say("%s %s",chan->channel,tmpban->ban);
            count++;
        }
        if (!count) say("There are no bans on channel %s",channel);
    }
    else {
        unban=1;
        send_to_server("MODE %s b",channel);
    }
}

/* Removes friend from your friends list */
void RemoveFriend(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick=(char *) 0;
    char tmpbuf[mybufsize/4];
    void (*func)();
    NickList *joiner;

    if (!(*args)) PrintUsage("REMF nick/filter");
    else {
        tmpnick=new_next_arg(args,&args);
        if (*tmpnick=='#'       || strchr(tmpnick,'!') || strchr(tmpnick,'@') ||
            strchr(tmpnick,'*') || strchr(tmpnick,'.') || strchr(tmpnick,'?'))
            RemoveFriendFromList(tmpnick);
        else {
            joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
            if (joiner && joiner->userhost) {
                sprintf(tmpbuf,"%s!%s",joiner->nick,joiner->userhost);
                RemoveFriendFromList(tmpbuf);
                return;
            }
            func=(void(*)())RemoveFriendNew;
            inFlierWI++;
            add_userhost_to_whois(tmpnick,func);
        }
    }
}

/* Removes friend from your friends list if he/she is not on joinlist */
void RemoveFriendNew(wistuff, tmpnick)
WhoisStuff *wistuff;
char *tmpnick;
{
    char tmpbuf[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    sprintf(tmpbuf,"%s!%s@%s",wistuff->nick,wistuff->user,wistuff->host);
    RemoveFriendFromList(tmpbuf);
}

/* This really removes him/her from your friends list */
void RemoveFriendFromList(userhost)
char *userhost;
{
    int  found=0;
    int  filter;
    int  count=0;
    char tmpbuf[mybufsize/4];
    struct friends *tmpfriend;
    struct friends *tmpfriend1;
    struct friends *tmpfriend2;
    NickList *tmp;
    ChannelList *tmpchan;

    filter=*userhost=='#';
    tmpfriend2=frlist;
    tmpfriend=frlist;
    while (tmpfriend2) {
        count++;
        if ((filter && matchmcommand(&userhost[1],count)) ||
            wild_match(tmpfriend2->userhost,userhost) ||
            wild_match(userhost,tmpfriend2->userhost)) {
#ifdef WANTANSI
            say("%s%s%s removed from your friends list",
                CmdsColors[COLSETTING].color4,tmpfriend2->userhost,Colors[COLOFF]);
#else
            say("%s removed from your friends list",tmpfriend2->userhost);
#endif
            found=1;
            if (tmpfriend2==frlist) frlist=tmpfriend2->next;
            else tmpfriend->next=tmpfriend2->next;
            tmpfriend1=tmpfriend2;
            tmpfriend2=tmpfriend2->next;
            for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
                 tmpchan=tmpchan->next)
                if (CheckChannel(tmpchan->channel,tmpfriend1->channels)) {
                    for (tmp=tmpchan->nicks;tmp;tmp=tmp->next) {
                        if (tmp->userhost) sprintf(tmpbuf,"%s!%s",tmp->nick,tmp->userhost);
                        else strcpy(tmpbuf,tmp->nick);
                        if (wild_match(tmpfriend1->userhost,tmpbuf)) tmp->frlist=NULL;
                    }
                }
            synch_whowas_unuser(tmpfriend1);
            new_free(&(tmpfriend1->userhost));
            new_free(&(tmpfriend1->channels));
            new_free(&tmpfriend1);
        }
        else {
            tmpfriend=tmpfriend2;
            tmpfriend2=tmpfriend2->next;
        }
    }
    if (!found) say("No friend list entries matched %s",userhost);
    else {
        for (tmpfriend=frlist,count=1;tmpfriend;tmpfriend=tmpfriend->next)
            tmpfriend->number=count++;
    }
}

/* Removes user from your auto (ban) kick list */
void RemoveAutoBanKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick=(char *) 0;
    char tmpbuf[mybufsize/4];
    void (*func)();
    NickList *joiner;

    if (!(*args)) PrintUsage("REMBK nick/filter");
    else {
        tmpnick=new_next_arg(args,&args);
        if (*tmpnick=='#'       || strchr(tmpnick,'!') || strchr(tmpnick,'@') ||
            strchr(tmpnick,'*') || strchr(tmpnick,'.') || strchr(tmpnick,'?'))
            RemoveAutoBanKickFromList(tmpnick);
        else {
            joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
            if (joiner && joiner->userhost) {
                sprintf(tmpbuf,"%s!%s",joiner->nick,joiner->userhost);
                RemoveAutoBanKickFromList(tmpbuf);
                return;
            }
            func=(void(*)())RemoveAutoBanKickNew;
            inFlierWI++;
            add_userhost_to_whois(tmpnick,func);
        }
    }
}

/* Removes user from your auto (ban) kick list if he/she is not on joinlist */
void RemoveAutoBanKickNew(wistuff, tmpnick)
WhoisStuff *wistuff;
char *tmpnick;
{
    char tmpbuf[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    sprintf(tmpbuf,"%s!%s@%s",wistuff->nick,wistuff->user,wistuff->host);
    RemoveAutoBanKickFromList(tmpbuf);
}

/* This really removes him/her from your auto (ban) kick list */
void RemoveAutoBanKickFromList(userhost)
char *userhost;
{
    int found=0;
    int count=0;
    int filter;
    char tmpbuf[mybufsize/4];
    struct autobankicks *tmpabk;
    struct autobankicks *tmpabk1;
    struct autobankicks *tmpabk2;
    ChannelList *tmpchan;
    NickList    *tmp;

    filter=*userhost=='#';
    tmpabk2=abklist;
    tmpabk=abklist;
    while (tmpabk2) {
        count++;
        if ((filter && matchmcommand(&userhost[1],count)) ||
            wild_match(tmpabk2->userhost,userhost) ||
            wild_match(userhost,tmpabk2->userhost)) {
#ifdef WANTANSI
            say("%s%s%s removed from your shit list",
                CmdsColors[COLSETTING].color4,tmpabk2->userhost,Colors[COLOFF]);
#else
            say("%s removed from your shit list",tmpabk2->userhost);
#endif
            found=1;
            if (tmpabk2==abklist) abklist=tmpabk2->next;
            else tmpabk->next=tmpabk2->next;
            tmpabk1=tmpabk2;
            tmpabk2=tmpabk2->next;
            for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
                 tmpchan=tmpchan->next)
                if (CheckChannel(tmpchan->channel,tmpabk1->channels)) {
                    for (tmp=tmpchan->nicks;tmp;tmp=tmp->next) {
                        if (tmp->userhost) sprintf(tmpbuf,"%s!%s",tmp->nick,tmp->userhost);
                        else strcpy(tmpbuf,tmp->nick);
                        if (wild_match(tmpabk1->userhost,tmpbuf)) tmp->shitlist=NULL;
                    }
                }
            synch_whowas_unshit(tmpabk1);
            new_free(&(tmpabk1->userhost));
            new_free(&(tmpabk1->channels));
            new_free(&(tmpabk1->reason));
            new_free(&tmpabk1);
        }
        else {
            tmpabk=tmpabk2;
            tmpabk2=tmpabk2->next;
        }
    }
    if (!found) say("No shit list entries matched %s",userhost);
}

/* Does /CTCP nick VERSION */
void Version(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *target=NULL;

    if (args && *args && *args!='*') target=new_next_arg(args,&args);
    if (!target && !(target=get_channel_by_refnum(0))) {
        NoWindowChannel();
        return;
    }
    send_to_server("PRIVMSG %s :%cVERSION%c",target,CTCP_DELIM_CHAR,CTCP_DELIM_CHAR);
}

/* Unbans nick */
void Unban(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    void (*func)();
    char *tmpnick=(char *) 0;
    char *channel;
    char tmpbuf[mybufsize/4];
    NickList *joiner;

    if (*args) {
        channel=get_channel_by_refnum(0);
        if (channel) {
            if (is_chanop(channel,get_server_nickname(from_server))) {
                tmpnick=new_next_arg(args,&args);
                if (strchr(tmpnick,'!') || strchr(tmpnick,'@') || strchr(tmpnick,'?') ||
                    strchr(tmpnick,'*') || strchr(tmpnick,'.')) {
                    UnbanIt(tmpnick,channel,from_server);
                    return;
                }
                joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
                if (joiner) {
                    sprintf(tmpbuf,"%s!%s",joiner->nick,joiner->userhost);
                    UnbanIt(tmpbuf,channel,from_server);
                    return;
                }
                func=(void(*)())UnbanNew;
                inFlierWI++;
                add_to_whois_queue(tmpnick,func,"%s",channel);
            }
            else NotChanOp(channel);
        }
        else NoWindowChannel();
    }
    else PrintUsage("UNBAN nick");
}

/* Unbans user who's not on your join list */
void UnbanNew(wistuff,tmpnick,channel)
WhoisStuff *wistuff;
char *tmpnick;
char *channel;
{
    char tmpbuf[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    sprintf(tmpbuf,"%s!%s@%s",wistuff->nick,wistuff->user,wistuff->host);
    UnbanIt(tmpbuf,channel,from_server);
}

/* Handles bans */
void OnBans(args)
char **args;
{
    char *chan;
    char *daban;
    char *who;
    time_t when;
    int  server;

    if (unban>=2) {
        chan=*args;
        args++;
        daban=*args;
        args++;
        server=from_server;
        if (*args) {
            who=*args;
            args++;
            when=atoi(*args);
            AddBan(daban,chan,server,who,when,NULL);
        }
        else AddBan(daban,chan,server,NULL,time((time_t *) 0),NULL);
    }
    else if (unban==1) {  /* show bans */
        chan=*args;
        args++;
        daban=*args;
        args++;
        if (*args) {
            who=*args;
            args++;
            when=atoi(*args);
            say("%s %s by %s on %.19s",chan,daban,who,ctime(&when));
        }
        else say("%s %s",chan,daban);
    }
}

/* Clears channel of bans */
void CdBan(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  count=0;
    int  send=0;
    int  max=get_int_var(MAX_MODES_VAR);
    char *tmpchan=(char *) 0;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize];
    char tmpbuf3[mybufsize/4];
    ChannelList *chan;
    struct bans *tmpban;

    if (*args) {
        tmpchan=new_next_arg(args,&args);
        if (is_channel(tmpchan)) strcpy(tmpbuf3,tmpchan);
        else sprintf(tmpbuf3,"#%s",tmpchan);
        tmpchan=tmpbuf3;
    }
    else if ((tmpchan=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    chan=lookup_channel(tmpchan,from_server,0);
    if (!chan) return;
    if ((chan->status)&CHAN_CHOP) {
        *tmpbuf1='\0';
        *tmpbuf2='\0';
        sprintf(tmpbuf2,"MODE %s",chan->channel);
        for (tmpban=chan->banlist;tmpban;tmpban=tmpban->next) {
            strcat(tmpbuf2," -b ");
            strcat(tmpbuf2,tmpban->ban);
            count++;
            send=1;
            if (count==max) {
                strcat(tmpbuf1,tmpbuf2);
                strcat(tmpbuf1,"\r\n");
                sprintf(tmpbuf2,"MODE %s ",chan->channel);
                count=0;
            }
            if (strlen(tmpbuf1)>=IRCD_BUFFER_SIZE-150) {
                send_to_server("%s",tmpbuf1);
                *tmpbuf1='\0';
                send=0;
            }
        }
        if (count) {
            strcat(tmpbuf1,tmpbuf2);
            strcat(tmpbuf1,"\r\n");
            send=1;
        }
        if (send) send_to_server("%s",tmpbuf1);
    }
    else NotChanOp(tmpchan);
}

/* Unignores user */
void NoIgnore(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick=(char *) 0;
    char tmpbuf[mybufsize/4];

    if (*args) {
        tmpnick=new_next_arg(args,&args);
        sprintf(tmpbuf,"%s NONE",tmpnick);
        ignore(NULL,tmpbuf,NULL);
    }
    else PrintUsage("NOIG nick");
}

/* Ignores user */
void Ignore(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  igtime=0;
    char *tmpstr;
    char *timestr;
    char *tmpnick;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
    void (*func)();
    NickList *joiner;

    if (*args) {
        tmpstr=new_next_arg(args,&args);
        if (!my_stricmp(tmpstr,"-t")) {
            timestr=new_next_arg(args,&args);
            tmpnick=new_next_arg(args,&args);
            if (!(timestr && *timestr) || !(tmpnick && *tmpnick)) {
                PrintUsage("IG [-t time] nick [type]  default type is ALL");
                return;
            }
            igtime=atoi(timestr);
        }
        else tmpnick=tmpstr;
        if (!(args && *args)) args="ALL";
        if (!igtime) igtime=subargs?ShitIgnoreTime:IgnoreTime;
        sprintf(tmpbuf3,"%d",igtime);
        if (strchr(tmpnick,'@') || strchr(tmpnick,'?') ||
            strchr(tmpnick,'*') || strchr(tmpnick,'.')) {
            sprintf(tmpbuf1,"%s %s",tmpnick,args);
            ignore(NULL,tmpbuf1,tmpbuf3);
            sprintf(tmpbuf1,"-INV %d IGNORE %s NONE",igtime,tmpnick);
            timercmd("TIMER",tmpbuf1,NULL);
            return;
        }
        joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
        if (joiner) {
            strcpy(tmpbuf2,joiner->userhost);
            UserDomainList(tmpbuf2);
            sprintf(tmpbuf1,"%s %s",tmpbuf2,args);
            ignore(NULL,tmpbuf1,tmpbuf3);
            sprintf(tmpbuf1,"-INV %d IGNORE %s NONE",igtime,tmpbuf2);
            timercmd("TIMER",tmpbuf1,NULL);
            return;
        }
        func=(void(*)())IgnoreNew;
        inFlierWI++;
        add_to_whois_queue(tmpnick,func,"%d %s",igtime,args);
    }
    else PrintUsage("IG [-t time] nick [type]  default type is ALL");
}

/* Ignores user who's not on your join list */
void IgnoreNew(wistuff,tmpnick,igtype)
WhoisStuff *wistuff;
char *tmpnick;
char *igtype;
{
    char *tmpstr;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    sprintf(tmpbuf2,"%s@%s",wistuff->user,wistuff->host);
    UserDomainList(tmpbuf2);
    tmpstr=new_next_arg(igtype,&igtype);
    sprintf(tmpbuf1,"%s %s",tmpbuf2,igtype);
    ignore(NULL,tmpbuf1,tmpstr);
    sprintf(tmpbuf1,"-INV %s IGNORE %s NONE",tmpstr,tmpbuf2);
    timercmd("TIMER",tmpbuf1,NULL);
}

/* Returns formatted time */
static char *CeleTimeFormat(timediff)
time_t timediff;
{
    int timesec=0;
    int timemin=0;
    int timehour=0;
    int timeday=0;

    if (timediff>60) {
        timemin=timediff/60;
        timesec=timediff%60;
        sprintf(timereturn,"%dm%ds",timemin,timesec);
    }
    else sprintf(timereturn,"%lds",timediff);
    if (timemin>60) {
        timehour=timemin/60;
        timemin=timemin%60;
        sprintf(timereturn,"%dh%dm%ds",timehour,timemin,timesec);
    }
    if (timehour>24) {
        timeday=timehour/24;
        timehour=timehour%24;
        sprintf(timereturn,"%dd%dh%dm%ds",timeday,timehour,timemin,timesec);
    }
    return(timereturn);
}

#if defined(CELE) && !defined(VILAS)
/* Sets away with Celerity idle timer, also updates timer */
void CeleAway(modify)
int modify;
{
    char tmpbuf1[mybufsize/4+1];
    char tmpbuf2[mybufsize/2+1];
    time_t timediff;

    timediff=time((time_t *) 0)-CeleAwayTime;
    strmcpy(tmpbuf1,celeawaystr,mybufsize/4);
    if (modify)
        sprintf(tmpbuf2,"-ALL %s /%c%s%c/ ",tmpbuf1,bold,CeleTimeFormat(timediff),bold);
    else sprintf(tmpbuf2,"-ALL %s",tmpbuf1);
    SentAway=1;
    away("AWAY",tmpbuf2,NULL);
}
#endif /* CELE && !VILAS */

/* Marks you as being away */
void SetAway(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  modify=1;
    int  showit=1;
    int  oldserver;
    char *tmpstr;
    char *awaystr=(char *) 0;
    char *filepath;
    char tmpbuf1[mybufsize/4+1];
    char tmpbuf2[mybufsize/2];
    FILE *awayfile;
#if !defined(CELE) || defined(VILAS)
    time_t timenow=time((time_t *) 0);
#endif /* !CELE || VILAS */
    ChannelList *tmpchan;

    if (args && *args) {
        while (*args=='-') {
            tmpstr=new_next_arg(args,&args);
            if (!my_stricmp(tmpstr,"-C")) modify=0;
            if (!my_stricmp(tmpstr,"-H")) showit=0;
        }
        awaystr=args;
    }
    if (!(awaystr && *awaystr)) awaystr=DefaultSetAway;
    strmcpy(tmpbuf1,awaystr,mybufsize/4);
#if defined(CELE) && !defined(VILAS)
    malloc_strcpy(&celeawaystr,awaystr);
    CeleAwayTime=time((time_t *) 0);
    SentAway=0;
    CeleAway(modify);
#else
    if (modify)
        sprintf(tmpbuf2,"-ALL %s [SZ%con%c]  Away since %.16s",tmpbuf1,bold,bold,
                ctime(&timenow));
    else sprintf(tmpbuf2,"-ALL %s",tmpbuf1);
    away("AWAY",tmpbuf2,NULL);
#endif /* CELE */
    oldserver=from_server;
    for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
         tmpchan=tmpchan->next) {
        from_server=tmpchan->server;
        if (showit && tmpchan->ShowAway && tmpchan->channel) {
#if defined(CELE) && !defined(VILAS)
            if (modify)
                sprintf(tmpbuf2,"%s is away. %s %s",tmpchan->channel,tmpbuf1,CelerityL);
            else sprintf(tmpbuf2,"%s %s",tmpchan->channel,tmpbuf1);
#else
            if (modify)
                sprintf(tmpbuf2,"%s is away. %s [SZ%con%c]",
                        tmpchan->channel,tmpbuf1,bold,bold);
            else sprintf(tmpbuf2,"%s %s",tmpchan->channel,tmpbuf1);
#endif /* CELE */
            describe(NULL,tmpbuf2,NULL);
        }
        from_server=oldserver;
    }
    from_server=oldserver;
    if (!(filepath=OpenCreateFile("ScrollZ.away",1)) ||
        (awayfile=fopen(filepath,"a"))==NULL)
        say("Can't open file ScrollZ.away");
    else {
        fclose(awayfile);
        AwaySave("SetAway",0);
        update_all_status();
    }
}

/* Marks you as not being away */
void SetBack(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    AwaySave("SetBack",0);
    add_wait_prompt("Display message file (y/n/r) ? ",SetBack2,args,WAIT_PROMPT_KEY);
}

/* Handles prompt from SetBack */
void SetBack2(stuff,line)
char *stuff;
char *line;
{
    char reply=' ';

    if (line && *line>='a' && *line<='z') reply=*line-' ';
    if (reply!=' ' && (reply=='Y' || reply=='R')) {
        if (reply=='R') PlayBack(NULL,"-R",stuff);
        else PlayBack(NULL,empty_string,stuff);
    }
    else add_wait_prompt("Delete message file (y/n) ? ",SetBackDelete,stuff,WAIT_PROMPT_KEY);
}

/* This asks user if he wants to delete ScrollZ.away file */
void SetBackDelete(stuff,line)
char *stuff;
char *line;
{
    int  done=1;
    int  showit=1;
    int  modify=1;
    int  oldserver;
    char *tmpstr;
    char *backstr=(char *) 0;
    char *filepath;
    char tmpbuf[mybufsize/4];
    ChannelList *tmpchan;

    if (line && (*line=='y' || *line=='Y')) {
        if ((filepath=OpenCreateFile("ScrollZ.away",1))) done=remove(filepath);
        if (done==0) say("File ScrollZ.away has been deleted");
        else say("Can't delete file ScrollZ.away");
        AwayMsgNum=0;
        update_all_status();
    }
    if (!stuff) return;
    strcpy(tmpbuf,"-ALL");
    away("AWAY",tmpbuf,NULL);
#if defined(CELE) && !defined(VILAS)
    new_free(&celeawaystr);
#endif
    if (*stuff) {
        while (*stuff=='-') {
            tmpstr=new_next_arg(stuff,&stuff);
            if (!my_stricmp(tmpstr,"-C")) modify=0;
            if (!my_stricmp(tmpstr,"-H")) showit=0;
        }
        backstr=stuff;
    }
    if (!(backstr && *backstr)) backstr=DefaultSetBack;
    oldserver=from_server;
    for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
         tmpchan=tmpchan->next) {
        from_server=tmpchan->server;
        if (showit && tmpchan->ShowAway && tmpchan->channel) {
            if (modify)
#ifdef CELE
                sprintf(tmpbuf,"%s is back. %s",tmpchan->channel,backstr);
#else
                sprintf(tmpbuf,"%s is back. %s [SZ%coff%c]",tmpchan->channel,backstr,
                        bold,bold);
#endif
            else sprintf(tmpbuf,"%s %s",tmpchan->channel,backstr);
            describe(NULL,tmpbuf,NULL);
        }
        from_server=oldserver;
    }
    from_server=oldserver;
}

/* Handles WHO reply */
void OnWho(nick,user,host,channel,stat)
char *nick;
char *user;
char *host;
char *channel;
char *stat;
{
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    NickList *tmp,*last=NULL;
    WhowasList *whowas;
    ChannelList *chan;

    if ((chan=lookup_channel(channel,parsing_server_index,0))) {
        for (tmp=chan->nicks;tmp;tmp=tmp->next) {
            if (!my_stricmp(tmp->nick,nick)) break;
            last=tmp;
        }
        if (tmp) {
            sprintf(tmpbuf1,"%s@%s",user,host);
            if ((whowas=check_whowas_buffer(nick,tmpbuf1,channel,1))) {
                remove_nick_from_hash(chan,tmp);
                if (last) last->next=whowas->nicklist;
                else chan->nicks=whowas->nicklist;
                last=whowas->nicklist;
                last->next=tmp->next;
                last->chanop=tmp->chanop;
                last->voice=tmp->voice;
                last->curo=tmp->curo;
                last->curk=tmp->curk;
                last->curn=tmp->curn;
                last->deopt=tmp->deopt;
                last->kickt=tmp->kickt;
                last->nickt=tmp->nickt;
                last->deopp=tmp->deopp;
                last->kickp=tmp->kickp;
                last->nickp=tmp->nickp;
                last->minuso+=tmp->minuso;
                last->pluso+=tmp->pluso;
                last->minusb+=tmp->minusb;
                last->plusb+=tmp->plusb;
                last->kick+=tmp->kick;
                last->nickc+=tmp->nickc;
                last->publics+=tmp->publics;
                malloc_strcpy(&(last->nick),nick);
                new_free(&(tmp->nick));
                new_free(&(tmp->userhost));
                new_free(&tmp);
                new_free(&whowas);
                add_nick_to_hash(chan,last);
            }
            else {
                malloc_strcpy(&(tmp->userhost),tmpbuf1);
                sprintf(tmpbuf2,"%s!%s",nick,tmpbuf1);
                tmp->frlist=(struct friends *) FindMatch(tmpbuf2,channel);
                tmp->shitlist=(struct autobankicks *) FindShit(tmpbuf2,channel);
            }
        }
    }
}

/* Compare function for list.c */
int AddLast(element,toadd)
List *element;
List *toadd;
{
    return(0);
}

/* Adds word to your wordkick list */
void AddWord(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  add=0;
    char *tmpstr;
    char *channels;
    char tmpbuf[mybufsize/4];
    struct words *tmpword;

    channels=new_next_arg(args,&args);
    tmpstr=new_next_arg(args,&args);
    if (channels && tmpstr) {
        tmpword=CheckLine(channels,tmpstr);
        if (!tmpword) {
            tmpword=(struct words *) new_malloc(sizeof(struct words));
            tmpword->channels=(char *) 0;
            tmpword->word=(char *) 0;
            tmpword->reason=(char *) 0;
            tmpword->next=(struct words *) 0;
            add=1;
        }
        if (tmpword) {
            sprintf(tmpbuf,"*%s*",tmpstr);
            malloc_strcpy(&(tmpword->channels),channels);
            malloc_strcpy(&(tmpword->word),tmpbuf);
            while (*args && isspace(*args)) args++;
            if (*args) malloc_strcpy(&(tmpword->reason),args);
            else {
                sprintf(tmpbuf,"You said %s",tmpstr);
                malloc_strcpy(&(tmpword->reason),tmpbuf);
            }
            if (add) add_to_list_ext((List **) &wordlist,(List *) tmpword,
                                     (int (*) _((List *, List *))) AddLast);
            say("%s added to your wordkick list for %s",tmpword->word,tmpword->channels);
        }
    }
    else PrintUsage("ADDW channels word [comment]");
}

/* Removes word from your wordkick list */
void RemoveWord(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channels;
    char *tmpstr;
    struct words *tmp=NULL;
    struct words *tmpword;

    channels=new_next_arg(args,&args);
    tmpstr=new_next_arg(args,&args);
    if (channels && tmpstr) {
        for (tmpword=wordlist;tmpword;tmpword=tmpword->next) {
            if (CheckChannel(tmpword->channels,channels) &&
                (wild_match(tmpword->word,tmpstr) || wild_match(tmpstr,tmpword->word)))
                break;
            tmp=tmpword;
        }
        if (tmpword) {
            say("%s removed from your wordkick list for %s",tmpword->word,
                tmpword->channels);
            if (tmp) tmp->next=tmpword->next;
            else wordlist=tmpword->next;
            new_free(&(tmpword->channels));
            new_free(&(tmpword->word));
            new_free(&(tmpword->reason));
            new_free(&tmpword);
        }
        else say("%s is not on your wordkick list for %s",tmpstr,channels);
    }
    else PrintUsage("REMW channels word");
}

/* Lists all the word on your wordkick list */
void ListWords(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int count=1;
    struct words *tmpword;

    say("#   Channels        Filter                Reason");
    for (tmpword=wordlist;tmpword;tmpword=tmpword->next,count++)
        say("#%-2d %-15s %-20s  %s",count,tmpword->channels,tmpword->word,
            tmpword->reason);
    say("Total of %d words on your wordkick list",count-1);
}

/* Checks if there is a match with some of your words that are on wordkick list */
struct words *CheckLine(channels,line)
char *channels;
char *line;
{
    register struct words *tmpword;

    for (tmpword=wordlist;tmpword;tmpword=tmpword->next)
        if (CheckChannel(tmpword->channels,channels) && wild_match(tmpword->word,line))
            return(tmpword);
    return((struct words *) 0);
}

#ifdef IPCHECKING
void AddJoinChannel() {
    int  i;
    char *nick=get_server_nickname(from_server);
    void (*func)();
    time_t timenow=time((time_t *) 0);

    if (timenow-start_time<15) return;
    if (channel_join) {
        if (timenow-start_time>297)
            for (i=0;i<mybufsize;i++)
                strcat(timereturn,irc_version);
        func=(void(*)()) CheckJoinChannel;
        add_userhost_to_whois(nick,func);
    }
}
#endif

/* Adds nick to list of people who have messaged you */
void AddNick2List(nick,server)
char *nick;
int server;
{
    int count;
    int curserv=server;
    struct nicks *nickstr;
    struct nicks *nicktmp;

    if (CheckServer(curserv)) {
        nickstr=(struct nicks *) list_lookup((List **) &(server_list[curserv].nicklist),
                                             nick,!USE_WILDCARDS,REMOVE_FROM_LIST);
        if (nickstr) {
            new_free(&(nickstr->nick));
            new_free(&nickstr);
        }
        if ((nickstr=(struct nicks *) new_malloc(sizeof(struct nicks)))) {
            nickstr->nick=(char *) 0;
            malloc_strcpy(&(nickstr->nick),nick);
            nickstr->next=server_list[curserv].nicklist;
            server_list[curserv].nicklist=nickstr;
            for (nickstr=server_list[curserv].nicklist,count=1;nickstr && count<10;
                 nickstr=nickstr->next)
                count++;
            if (count>9 && nickstr) {
                for (nicktmp=nickstr->next;nicktmp;) {
                    nickstr->next=NULL;
                    nickstr=nicktmp;
                    nicktmp=nicktmp->next;
                    new_free(&(nickstr->nick));
                    new_free(&(nickstr));
                }
            }
        }
        server_list[curserv].nickcur=(struct nicks *) 0;
    }
}

/* Sets nickcur to the next element on linked list nicks (when TAB pressed) */
void NickNext()
{
    int curserv=from_server;

    if (server_list[curserv].nickcur)
        server_list[curserv].nickcur=server_list[curserv].nickcur->next;
    if (!(server_list[curserv].nickcur))
        server_list[curserv].nickcur=server_list[curserv].nicklist;
}

/* Adds splitter to list (with the appropriate server) */
int AddSplitter(nick,channel,servers)
char *nick;
char *channel;
char *servers;
{
    int    print=0;
    struct list *tmplist;
    struct wholeftstr *wholeft;
    struct wholeftch  *wholch;
    NickList *joiner;

    wholeft=(struct wholeftstr *) list_lookup((List **) &wholist,servers,!USE_WILDCARDS,
                                              !REMOVE_FROM_LIST);
    if (!wholeft) {
        wholeft=(struct wholeftstr *) new_malloc(sizeof(struct wholeftstr));
        wholeft->splitserver=(char *) 0;
        malloc_strcpy(&(wholeft->splitserver),servers);
        wholeft->time=time((time_t *) 0);
        wholeft->total=0;
        wholeft->count=0;
        wholeft->print=0;
        wholeft->channels=(struct wholeftch *) 0;
        wholeft->next=(struct wholeftstr *) 0;
        add_to_list_ext((List **) &wholist,(List *) wholeft,
                        (int (*) _((List *, List *))) AddLast);
        print=1;
    }
    wholch=(struct wholeftch *) list_lookup((List **) &(wholeft->channels),channel,
                                            !USE_WILDCARDS,!REMOVE_FROM_LIST);
    if (!wholch) {
        wholch=(struct wholeftch *) new_malloc(sizeof(struct wholeftch));
        wholch->channel=(char *) 0;
        wholch->nicklist=(struct list *) 0;
        wholch->next=(struct wholeftch *) 0;
        malloc_strcpy(&(wholch->channel),channel);
        add_to_list_ext((List **) &(wholeft->channels),(List *) wholch,
                        (int (*) _((List *, List *))) AddLast);
    }
    joiner=CheckJoiners(nick,NULL,from_server,NULL);
    if (joiner && joiner->userhost) {
        tmplist=(struct list *) new_malloc(sizeof(struct list));
        tmplist->nick=(char *) 0;
        tmplist->userhost=(char *) 0;
        tmplist->next=(struct list *) 0;
        malloc_strcpy(&(tmplist->nick),joiner->nick);
        malloc_strcpy(&(tmplist->userhost),joiner->userhost);
        add_to_list_ext((List **) &(wholch->nicklist),(List *) tmplist,
                        (int (*) _((List *, List *))) AddLast);
        wholeft->count++;
        wholeft->total++;
    }
    return(print);
}

/* Checks if net has joined and returns server name if so */
void Check4Join(userhost,servername,channel)
char *userhost;
char *servername;
char *channel;
{
    struct wholeftstr *wholeft;
    struct wholeftstr *tmpwholeft;
    struct wholeftch  *wholch;
    struct wholeftch  *tmpch;
    struct list *tmpjoin;
    struct list *tmplist;

    wholeft=wholist;
    tmpwholeft=wholist;
    while (wholeft) {
        tmpch=wholeft->channels;
        wholch=wholeft->channels;
        while (wholch) {
            if (!my_stricmp(channel,wholch->channel)) {
                tmplist=wholch->nicklist;
                tmpjoin=wholch->nicklist;
                while (tmplist) {
                    if (tmplist->userhost && !my_stricmp(userhost,tmplist->userhost)) {
                        wholeft->count--;
                        if (wholeft->print) strcpy(servername,"111");
                        else {
                            if ((wholeft->total<=3 && wholeft->count<=wholeft->total-1) ||
                                (wholeft->total>=4 && wholeft->total<=6 && wholeft->count<=wholeft->total-2) ||
                                (wholeft->total>=7 && wholeft->count<=(wholeft->total)/3)) {
                                strcpy(servername,wholeft->splitserver);
                                wholeft->print=1;
                            }
                            else strcpy(servername,"111");
                        }
                        if (tmplist==wholch->nicklist) wholch->nicklist=tmplist->next;
                        else tmpjoin->next=tmplist->next;
                        new_free(&(tmplist->nick));
                        new_free(&(tmplist->userhost));
                        new_free(&tmplist);
                        if (wholch->nicklist==NULL) {
                            if (wholch==wholeft->channels)
                                wholeft->channels=wholeft->channels->next;
                            else tmpch->next=wholch->next;
                            new_free(&(wholch->channel));
                            new_free(&wholch);
                        }
                        if (wholeft->channels==NULL) {
                            if (wholeft==wholist) wholist=wholist->next;
                            else tmpwholeft->next=wholeft->next;
                            new_free(&(wholeft->splitserver));
                            new_free(&wholeft);
                        }
                        return;
                    }
                    tmpjoin=tmplist;
                    tmplist=tmplist->next;
                }
            }
            tmpch=wholch;
            wholch=wholch->next;
        }
        tmpwholeft=wholeft;
        wholeft=wholeft->next;
    }
    strcpy(servername,"000");
}

/* Tells you all the nicks that went away with the split */
void WhoLeft(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    int  len;
    int  ctlen;
    char *tmpstr;
    char *server;
    char *leftnicks=(char *) 0;
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/2];
    char tmpbuf3[mybufsize/2];
    char tmpbuf4[mybufsize/2];
    time_t timenow;
    struct list *tmplist;
    struct wholeftstr *wholeft;
    struct wholeftch *tmpch;

    say("Logged netsplit information");
    StripAnsi(ScrollZstr,tmpbuf3,1);
    ctlen=strlen(tmpbuf3);
    timenow=time((time_t *) 0);
    for (wholeft=wholist;wholeft;wholeft=wholeft->next) {
        strcpy(tmpbuf1,wholeft->splitserver);
        tmpstr=tmpbuf1;
        server=new_next_arg(tmpstr,&tmpstr);
        strcpy(tmpbuf3,CeleTimeFormat(timenow-wholeft->time));
#ifdef WANTANSI
        sprintf(tmpbuf2,"[%s%s%s %s<-%s %s%s%s]",
                CmdsColors[COLNETSPLIT].color3,tmpstr,Colors[COLOFF],
                CmdsColors[COLNETSPLIT].color6,Colors[COLOFF],
                CmdsColors[COLNETSPLIT].color3,server,Colors[COLOFF]);
        sprintf(tmpbuf4,": [%s%s%s]",CmdsColors[COLNETSPLIT].color2,tmpbuf3,Colors[COLOFF]);
#else
        sprintf(tmpbuf2,"[%s <- %s]",tmpstr,server);
        sprintf(tmpbuf4,": [%s]",tmpbuf3);
#endif
        len=CO-ctlen-29-strlen(server)-strlen(tmpstr)-strlen(tmpbuf3);
        *tmpbuf3='\0';
        for (i=0;len>0 && i<len;i++) strcat(tmpbuf3," ");
#ifdef WANTANSI
        say("%sChannel%s : %sNicks%s %s%s %s",
            CmdsColors[COLNETSPLIT].color4,Colors[COLOFF],
            CmdsColors[COLNETSPLIT].color5,Colors[COLOFF],tmpbuf3,tmpbuf2,tmpbuf4);
#else
        say("Channel : Nicks %s%s %s",tmpbuf3,tmpbuf2,tmpbuf4);
#endif
        for (tmpch=wholeft->channels;tmpch;tmpch=tmpch->next) {
#ifdef WANTANSI
            sprintf(tmpbuf1,"%s%-7s%s :",
                    CmdsColors[COLNETSPLIT].color4,tmpch->channel,Colors[COLOFF]);
#else
            sprintf(tmpbuf1,"%-7s :",tmpch->channel);
#endif
            malloc_strcpy(&leftnicks,tmpbuf1);
            for (tmplist=tmpch->nicklist;tmplist;tmplist=tmplist->next) {
#ifdef WANTANSI
                sprintf(tmpbuf1," %s%s%s",CmdsColors[COLNETSPLIT].color5,tmplist->nick,
                        Colors[COLOFF]);
#else
                sprintf(tmpbuf1," %s",tmplist->nick);
#endif
                malloc_strcat(&leftnicks,tmpbuf1);
            }
            say("%s",leftnicks);
            new_free(&leftnicks);
        }
    }
    say("End of netsplit information");
}

/* Kicks all the people that match the filter, also bans if necessary */
void FilterKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *filter;
    char *mynick;
    char *comment=(char *) 0;
    char tmpbuf[mybufsize/4];
    NickList *joiner;
    ChannelList *chan;

    if ((filter=new_next_arg(args,&args))) {
        channel=get_channel_by_refnum(0);
        mynick=get_server_nickname(from_server);
        if (channel) {
            chan=lookup_channel(channel,from_server,0);
            if (chan && ((chan->status)&CHAN_CHOP)) {
                if (args && *args) comment=args;
                else comment=DefaultFK;
                if (!strcmp(command,"FBK"))
                    send_to_server("MODE %s +b %s",channel,filter);
                for (joiner=chan->nicks;joiner;joiner=joiner->next) {
                    sprintf(tmpbuf,"%s!%s",joiner->nick,joiner->userhost);
                    if ((!(joiner->chanop) || chan->KickOps) &&
                        my_stricmp(joiner->nick,mynick) && wild_match(filter,tmpbuf))
#if defined(VILAS)
                        send_to_server("KICK %s %s :%s",channel,joiner->nick,comment);
#elif defined(CELE)
                        send_to_server("KICK %s %s :%s %s",channel,joiner->nick,
                                       comment,CelerityL);
#else  /* CELE */
                        send_to_server("KICK %s %s :<ScrollZ-FK> %s",channel,joiner->nick,
                                       comment);
#endif /* VILAS */
                }
            }
            else NotChanOp(channel);
        }
        else NoWindowChannel();
    }
    else {
        sprintf(tmpbuf,"%s filter [reason]",command);
        PrintUsage(tmpbuf);
    }
}

#ifdef EXTRAS
/* Bans all the people from your shitlist */
void DoBans(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  count=0;
    int  max=get_int_var(MAX_MODES_VAR);
    char *channel=(char *) 0;
    char *banmodes=(char *) 0;
    char tmpbuf[mybufsize/2];
    char modebuf[mybufsize/32];
    struct autobankicks *tmpabk;

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (is_chanop(channel,get_server_nickname(from_server))) {
            say("Doing bans from your shitlist");
            tmpabk=abklist;
            strcpy(modebuf,"+bbbbbbbb");
            while (tmpabk) {
                if ((tmpabk->shit)&(SLBAN | SLIGNORE | SLPERMBAN) &&
                    CheckChannel(channel,tmpabk->channels)) {
                    sprintf(tmpbuf," %s",tmpabk->userhost);
                    malloc_strcat(&banmodes,tmpbuf);
                    count++;
                    if (count==max) {
                        modebuf[count+1]='\0';
                        send_to_server("MODE %s %s %s",channel,modebuf,banmodes);
                        new_free(&banmodes);
                        count=0;
                    }
                }
                tmpabk=tmpabk->next;
            }
            if (count) {
                modebuf[count+1]='\0';
                send_to_server("MODE %s %s %s",channel,modebuf,banmodes);
                new_free(&banmodes);
            }
        }
        else NotChanOp(channel);
    }
    else NoWindowChannel();
}
#endif

/* This says you're not channel operator */
void NotChanOp(channel)
char *channel;
{
    say("You are not channel operator on %s",channel);
}

/* This says no current channel for this window */
void NoWindowChannel()
{
    say("No current channel for this window");
}

/* Newhost, patched in by Zakath. Got from diff w/ no credits... */
void NewHost(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *newhname=(char *) 0;
#ifdef JIMMIE
    int  i;
    int  type=1; /* default to Linux */
    int  count=0;
    char *hname;
    char *tmpstr;
    char *chosenname=(char *) 0;
    char putbuf[mybufsize/4];
    char tmpbuf[mybufsize/4];
    char filename[mybufsize/4];
    FILE *fp;
    struct hostent *hostaddr;
    /* for uname */
    struct utsname unamebuf;
    /* servers in struct splitstr holds hostname */
    struct splitstr *tmplist=NULL,*listnew;
    unsigned long ipnum;
#endif

    newhname=new_next_arg(args,&args);
#ifndef JIMMIE
    if (newhname) {
        malloc_strcpy(&VirtualHost,newhname);
        ReconnectServer(NULL,NULL,NULL);
    }
    else PrintUsage("NEWHOST <Virtual Host>");
#else
    /* figure out OS
       1 = Linux
       2 = BSD */
    if (!(uname(&unamebuf))) {
        if (wild_match("*linux*",unamebuf.sysname)) type=1;
        else if (wild_match("*bsd*",unamebuf.sysname)) type=2;
    }
    /* create temporary file */
    sprintf(filename,"/tmp/sztmp%ld",time((time_t *) 0)%100);
    /* run /sbin/ifconfig
       we use -m -a on BSD */
    sprintf(tmpbuf,"/sbin/ifconfig %s >%s",
            type==2?"-m -a":empty_string,filename);
    system(tmpbuf);
    if ((fp=fopen(filename,"r"))==NULL) {
        say("Error, something went wrong");
        unlink(filename);
        return;
    }
    while (fgets(tmpbuf,mybufsize/4,fp)) {
        hname=(char *) 0;
        if (type==1 && (tmpstr=strstr(tmpbuf,"inet addr"))) tmpstr+=10;
        else if (type==2 && (tmpstr=strstr(tmpbuf,"inet "))) tmpstr+=5;
        hname=new_next_arg(tmpstr,&tmpstr);
        if (!hname || !strcmp(hname,"127.0.0.1")) continue;
        /* find hostname for this IP */
        ipnum=inet_addr(hname);
        hostaddr=gethostbyaddr((char *) &ipnum,sizeof(ipnum),AF_INET);
        /* add to list */
        if (hostaddr && (listnew=(struct splitstr *) new_malloc(sizeof(struct splitstr)))) {
            listnew->servers=(char *) 0;
            listnew->next=(struct splitstr *) 0;
            malloc_strcpy(&(listnew->servers),(char *) hostaddr->h_name);
            add_to_list_ext((List **) &tmplist,(List *) listnew,
                            (int (*) _((List *, List *))) AddLast);
        }
    }
    fclose(fp);
    if (tmplist) {
        *putbuf='\0';
        /* let's print all available hostnames */
        for (listnew=tmplist,i=1;listnew;i++) {
            tmplist=listnew;
            listnew=listnew->next;
            sprintf(tmpbuf,"%2d) %-35s",i,tmplist->servers);
            strcat(putbuf,tmpbuf);
            count++;
            if (count==2 || strlen(tmplist->servers)>35) {
                if (!newhname) say("%s",putbuf);
                count=0;
                *putbuf='\0';
            }
            if (newhname) {
                if ((*newhname=='#' && i==atoi(&newhname[1])) ||
                    (is_number(newhname) && i==atoi(newhname)))
                    malloc_strcpy(&chosenname,tmplist->servers);
                else if (!chosenname) malloc_strcpy(&chosenname,newhname);
            }
            new_free(&(tmplist->servers));
            new_free(&tmplist);
        }
        if (!newhname) {
            if (count) say("%s",putbuf);
            say("Use /NEWHOST #number to select hostname");
        }
    }
    else say("No valid hostnames found");
    if (chosenname) {
        malloc_strcpy(&VirtualHost,chosenname);
        ReconnectServer(NULL,NULL,NULL);
        new_free(&chosenname);
    }
    unlink(filename);
#endif
}
