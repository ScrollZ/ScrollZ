/******************************************************************************
 Functions coded by Flier (THANX to Sheik!!)
 
 ClearKey             Clears current channel key
 Logo                 Prints out ScrollZ logo
 AwaySave             Saves message into ScrollZ.away file
 UserDomain           Returns pointer to user@domain if given user@host
 UserDomainList       Returns pointer to user@domain useful for lists
 NHProtToggle         Toggles nethack protection on/off
 ChanStat             Prints out some channel statistics
 Ls                   Does the ls command
 SZHelp               Displays help
 Chat                 Does /DCC CHAT nick
 NoChat               Does /DCC CLOSE CHAT nick
 Finger               Launches finger on nick
 Check4Fake           Returns 1 if netsplit seems to be fake, 0 - otherwise
 UserMode             Sets your mode
 AutoJoinOnInvToggle  Sets auto join on invite toggle
 Settings             Prints all ScrollZ related settings
 FloodProtToggle      Sets flood protection toggle
 CheckChannel         Checks if a channel is on your friend's channels list
 CodeIt               Codes an array
 FixName              Fixes filename
 AddFriendFilter      Adds friend with filter to your friend's list
 AddABKFilter         Adds auto (ban) kicker with filter your abk list
 Net                  Like /net in phoenix.irc
 PrintError           Prints error in ScrollZ.save
 ScrollZLoad          Loads ScrollZ.save file
 Reset                Resets your friends + auto (ban) kicks list and reloads ScrollZ.save
 InitVars             Initializes ScrollZ related variables
 LastMessageKick      Kicks user with last message you have received
 RandomLamerKick      Kicks a random lamer (non-op) from your current channel
 ReplyWord            Change AutoReply Buffer from IRC
******************************************************************************/

/*
 * $Id: edit3.c,v 1.5 1998-10-07 16:51:10 f Exp $
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
#include "list.h"
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
#include "exec.h"
#include "struct.h"
#include "myvars.h"
#include "whowas.h"

void ShowSZHelpPage _((char *));
void ShowSZHelp _((char *, char *));
void ShowSZCommandPage _((char *));
void ShowSZCommand _((char *, char *));
void NextArg _((char *, char **, char *));
void FingerNew _((WhoisStuff *, char *));
int  CheckChannel _((char *, char *));
int  readln _((FILE *, char *));

NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern void HandleUserhost _((WhoisStuff *, char *, char *));
extern void helpmcommand _((char *));
#ifdef WANTANSI
extern void SetColors _((int, char **, int *, int));
#endif
extern void PrintSetting _((char *, char *, char *, char *));
extern char *OpenCreateFile _((char *, int));
extern void StripAnsi _((char *, char *, int));
extern void InitKeysColors _((void));
extern void NotChanOp _((char *));
extern void NoWindowChannel _((void));
extern void ShowHelpLine _((char *));
extern void PrintUsage _((char *));
extern void MangleString _((char *, char *, int));
extern void MangleVersion _((char *));
extern void EncryptString _((char *, char *, char *, int));
extern int  AddLast _((List *, List *));
extern int  CheckPrivs _((char *, char *));
extern int  CheckShit _((char *, char *));
extern NickList * find_in_hash _((ChannelList *, char *));

extern void dcc_chat _((char *));
extern void dcc_close _((char *));

extern char *ScrollZlame1;
extern char *chars;
#ifdef IPCHECKING
extern char global_track[];
#endif

static FILE *helpfile;
static char tmpbufhlp[mybufsize/32];
static int  DontHold=0;
static int  FoundCommand=0;
extern char *CelerityNtfy;

/* Removes key for channel */
void ClearKey(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *curmode;
    char *tmpmode;
    char *channel;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (args && *args) {
        channel=new_next_arg(args,&args);
        if (is_channel(channel)) strcpy(tmpbuf1,channel);
        else sprintf(tmpbuf1,"#%s",channel);
        channel=tmpbuf1;
    }
    else if ((channel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    if (is_chanop(channel,get_server_nickname(from_server))) {
        if ((curmode=get_channel_mode(channel,from_server))!=NULL) {
            strcpy(tmpbuf2,curmode);
            curmode=tmpbuf2;
            while (*curmode && *curmode!='k') curmode++;
            if (*curmode && *curmode=='k') {
                while (*curmode && *curmode!=' ') curmode++;
                curmode++;
                tmpmode=curmode;
                while (*tmpmode && *tmpmode!=' ') tmpmode++;
                *tmpmode=0;
                send_to_server("MODE %s -k %s",channel,curmode);
            }
        }
    }
    else NotChanOp(channel);
}

/* Prints out ScrollZ logo :) */
void Logo(command,args,subargs)
char *command;
char *args;
char *subargs;
{
#ifdef __linux__
    put_it("[0m[0;25;37;40m");
    put_it("               [1;30mÜ[0m                                    [34m [0m    [1;30m [0;36;40m    [0m     [36mÞ[0m");
    put_it("             [1;30mÜÛÝ[0m      [1;30mßßßß  ÛÛÛÜÜÜÜ[0m   [1;30mÜÜÜÜÜÜ[0;33;40m°°[0m [1;32;46m°²[40mÛ[33mÛ[0m [34m [0m    [36m [1;33mÛÛ[32mÛ[46m²[0;36;40m    [1mÜ[46m°[0m");
    put_it("         [1;30mÜÜÛÛÛÛ   ÜÜÜÜÛÛÛÛß ÜÛ[43m²[40m  ßÛÛÛÜ  ßßÛÛ[43m²²[40mÜ[0;36;40m ß[1;32;46m²[40mÛ[0m       [1;32mÛÛ[46m²°[0;36;40m  [1mÜÛ[46m²°[0;36;40mßßßßÛÛÛÛÜ[0m");
    put_it("    [1;30mÜÜÛÛÛÛßßÜÛÝ ÜÛÛß  ÛÛÛ  ÞÛÛ[43m²[0m  [1;30m ÞÛÛÛÝ[0m   [1;30m ÛÛÛÛÝ[0;36;40m Þ[1;32;46m²[0m     [36mÞ [1;32mÛ[46m²°[0;36;40mÛ  [1;37mß[36mßß[0;36;40mß[0m    [36mÞÛ[1;46m°°[0;36;40mÛÝ[0m");
    put_it("  [1;30m ÞÛÛÛß[0m   [1;30mÞÛ[47m²[40m ÞÛÛÛ [0m  [1;30m  ÜÜ [0m [1;30mßÛ[43m²[0m  [1;30mÜÛ[47m²²[40mß[0m    [1;30mÞÛÛÛß[0m [36mÜÛ[1;32;46m°[0;36;40m  [34m [36m ÜÛ [1;32;46m²°[0;36;40mÛÛ  [1;32mÛÛ[46m²°[0;36;40m ÜÜÜÛÛÛÛß[0m");
    put_it("   [1;30mßÛÛÛÜÜÜÜÜ  [0m [1;30mÛÛÛÝ [0m [1;30m ßÛÛÛÜ[0m  [1;30mÛ ßßß[0m°°    [1;30m [43m²²[40mÛÝ[0m [36mÞÛÛÛ  ÜÛÛÛ [1;32;46m°[0;36;40mÛÛÛ  [1;32mÛ[46m²°[0;36;40mÛ[0m    [1;33mÜ[32mÜÜ[0;36;40mÜ[0m");
    put_it(" [1;30mÜ ÜÝÜÜ [0m   [1;30mßÛÛÜ[0m  [1;30mßÛÝ[0m  [1;30m ÞÛÛÛÛÝ[0m [1;30mÞ [0m [1;30mÛÛÜÜ    ÛÛÛÛ  [0;36;40mÛÛÛ[1;32;46m°[0;36;40m ÞÛÛÛÛ ÛÛÛÛ  [1;32;46m²°[0;36;40mÛÛ[0m    [1;32;46mÛÛ²°[0;36;40m [0m");
    put_it(" [1;30m  ÞÛÛÛÝ[0m   [1;30mÛÛÛÛÝ[0m  [1;30mÞÛÜÜ[47m²²[40mÛÛÛß[0m  [1;30mÛ [0m [1;30m ßÛÛÛ ßÛÛßß   [0;36;40mßßßßßßÛÛ[1;46m°°[0;36;40m ßßßßßß[1;32;46m°[0;36;40mÛÛÛ    [1;32;46mÛ²°[0;36;40mÛ  [0m");
    put_it("sub [1;30mß[47m²²[40mÛÜÜÛÛÛÛß[0m     [1;30mßßßßß[0m   [1;30m ßß[0m  [1;30m  ÞÛ[47m²²[0m                [36m°°[0m       [36mÜÜÜÜ[0m    [1;32;46m²°[0;36;40mÛÛ  [0m");
    put_it("       [1;30mßßßßß  [0m                    [1;30m   ÛÛÛÝ[0m                        [36mßßßßßßßß[1;32;46m°[0;36;40mÛÛÛ[0m");
    put_it("           S c r o l l Z           [1;30mÜÜÜ[0m");
    put_it("                                            original ansi by subsonic of cia");
    put_it("[0m  [1mÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜ[0mÜ");
#ifdef CELE
    put_it("  [1mÛ[0;30;47m     [0;30;47m- [1;34mScrollZ[0;30;47m [1;34mGraphical IRC client[0;30;47m [1mver[0;30;47m [35m1[1;37m.[0;35;47m8i[30m +[1;34m %s [0;30;47m-     [1;30mÛ[0m",CelerityVersion);
#else
    put_it("  [1mÛ[0;30;47m             [0;30;47m- [1;37mScrollZ[0;30;47m [1;37mGraphical IRC client[0;30;47m [1mversion[0;30;47m [35m1[1;37m.[0;35;47m8i[30m -               [1;30mÛ[0m");
#endif
    put_it("  [1;37mÛ[0;35;47m               coding[1;30m : [33mFlier[0;30;47m [1;37m([33mflier[1;37m@[33mglobecom[1;37m.[33mnet[1;37m)[30m                       [1;30mÛ[0m");
    put_it("  [1;37mÛ[0;35;47m              patches[1;30m : [32;47mZakath[1;37m ([30mxanth[37m@[30m3Sheep[37m.[30mCOM[37m)[0;30;47m                        [1;30mÛ[0m");
    put_it("  [1;37mÛ[0;35;47m  distro head/patches[1;30m : [32;47macidflash[1;37m ([30macid[37m@[30mhostu[37m.[30mnet[37m)                       [1;30mÛ[0m");
    put_it("  [1;37mÛ[0;35;47m                        [1;34mType /SZINFO for more information                [1;30mÛ[0m");
    put_it("  ß[1;30mßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß[0m");
#else
    put_it("");
    put_it("");
    put_it("                                                             :");
    put_it("                                                            ,i   :");
    put_it("                           $#ss,_  _,,ss#$$$$  ScrollZ  _,sS$$   i");
    put_it("         _,s#S$$$7 _,ss###S$$$$$7 $$7'  $$$$$ ,,,,,__  `'7$$$$ ,s$ $S#ss,._");
    put_it("    _,s#P'$$$$$7',d$$$7'^`$$$$7',s$i    $$$$$ $$$$$$$S#s,`7$$$ $$$ $$`^''7$Sb");
    put_it(" ,s$$$P' :$$$$$; $$$$$   ;$$$i d$$$;    i$$$$ $$$$$$$$$$$b `$$ $$$ $7'    `$$7");
    put_it("  `7$$Ss,.    ss$$$$$i   i$$$$,`$$$    ,d$$$y $$ $$7'$$$$$; $$ $$$      ,s#7'`");
    put_it("     `'7Y$Ss,.`'7$$$$;       sss$$$ _,s$$$7',d$$ $$  $$$$$i $$ $$$   ,s$$$7");
    put_it("  ,ds,    `7$$$s,`7$$        $$$$$$`'7S#s,.`7$$$s$$  $$$$$l $$ $$$  d$$$$i");
    put_it(" d$$$$$7    `$$$$b `$;    `7s,_`'7$   `?$$$s,`''?$$s#$$$$$; $$ $$$ d$$$$$;");
    put_it(":$$$$$7     ,$$$$$7 di     i$$$$b $    ;$$$$$$7,$$$$$$$$$7 d$$ $$$ $$$$$$b,");
    put_it("`^'7$7S#ssssS$$$7'`,$$S#ssS$$$$7 d$    i$$$$$7 ,$$$$$$7#',s$$$ $$$ $$$$$$$$b,.");
    put_it("    `Y$$$$$$$7'      `Y$$$$$7'         ,$$$$7' '^^^``");
    put_it("    ScrollZ 1.8i                      `7$$$s,  mydknight>>");
    put_it("    coding  : Flier (flier@globecom.net)   `^' *");
    put_it("    patches : Zakath (xanth@3Sheep.COM)");
    put_it("    patches : acidflash (acid@hostu.net)");
    put_it("    type /SZINFO for more information");
    put_it("");
    put_it("");
#endif /* __linux__ */
}

/* Saves message into ScrollZ.save file */
void AwaySave(message,type)
char *message;
int  type;
{
    char *filepath;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize];
    FILE *awayfile;
    time_t now;

    if (type && !(type&AwaySaveSet)) return;
    filepath=OpenCreateFile("ScrollZ.away",1);
    if (filepath && (awayfile=fopen(filepath,"a"))!=NULL) {
        now=time((time_t *) 0);
        if (type&SAVEMSG)         sprintf(tmpbuf1,"%cMSG   %c",REV_TOG,REV_TOG);
        else if (type&SAVENOTICE) strcpy(tmpbuf1,"NOTICE");
        else if (type&SAVEMASS)   strcpy(tmpbuf1,"MASS  ");
        else if (type&SAVECOLL)   strcpy(tmpbuf1,"COLL  ");
        else if (type&SAVECDCC)   strcpy(tmpbuf1,"CDCC  ");
        else if (type&SAVEDCC)    strcpy(tmpbuf1,"DCC   ");
        else if (type&SAVEPROT)   strcpy(tmpbuf1,"PROT  ");
        else if (type&SAVEHACK)   strcpy(tmpbuf1,"HACK  ");
        else if (type&SAVESRVM)   strcpy(tmpbuf1,"SRVM  ");
        else if (type&SAVECTCP)   strcpy(tmpbuf1,"CTCP  ");
        else if (type&SAVEFLOOD)  strcpy(tmpbuf1,"FLOOD ");
        else if (type&SAVEINVITE) strcpy(tmpbuf1,"INVITE");
        else if (type&SAVEKILL)   strcpy(tmpbuf1,"KILL  ");
        else if (type&SAVEKICK)   strcpy(tmpbuf1,"KICK  ");
        else if (type&SAVESERVER) strcpy(tmpbuf1,"SERVER");
        else if (type&SAVEFAKE)   strcpy(tmpbuf1,"FAKE  ");
        else if (type&SAVEAREPLY) strcpy(tmpbuf1,"AREPLY");
        else if (type&SAVENOTIFY) strcpy(tmpbuf1,"NOTIFY");
        else *tmpbuf1='\0';
        if (*tmpbuf1) sprintf(tmpbuf2,"[%.24s] %s: %s",ctime(&now),tmpbuf1,message);
        else sprintf(tmpbuf2,"[%.24s] %s",ctime(&now),message);
        StripAnsi(tmpbuf2,tmpbuf1,2);
        if (EncryptPassword) EncryptString(tmpbuf2,tmpbuf1,EncryptPassword,mybufsize);
        else strcpy(tmpbuf2,tmpbuf1);
        fprintf(awayfile,"%s\n",tmpbuf2);
        fclose(awayfile);
    }
}

#if !defined(WANTANSI) || defined(MGS)
/* Returns user@domain if user@host given */
void UserDomain(userhost)
char *userhost;
{
    int  count=0;
    int  origcount;
    int  isip=2;
    char *string;
    char *afna;
    char tmpbuf[mybufsize/4];

    for (string=userhost;*string;string++) {
        if (*string=='@' && isip==2) isip=1;
        else if (isip!=2) isip&=(*string=='.' || isdigit(*string));
        if (*string=='.') count++;
    }
    if (isip==1) return;
    if (count>1) {
        strcpy(tmpbuf,userhost);
        origcount=count;
        afna=strchr(userhost,'@');
        afna++;
        string=strchr(tmpbuf,'@');
        while (*string && count==origcount) {
            if (*string=='.') count--;
            string++;
        }
        strcpy(afna,string);
    }
}
#endif

/* Returns user@domain useful for list */
void UserDomainList(userhost)
char *userhost;
{
    int  isip=2;
    int  count=0;
    char tmpbuf[mybufsize/4];
    char *tmpstr;
    char *tilda=tmpbuf;
    char *string=NULL;

    strcpy(tmpbuf,userhost);
    if (*tilda=='~') tilda++;
    for (tmpstr=tilda;*tmpstr;tmpstr++) {
        if (*tmpstr=='@') {
            if (isip==2) isip=1;
            string=tmpstr;
        }
        else if (isip!=2) isip&=(*tmpstr=='.' || isdigit(*tmpstr));
        if (string && *tmpstr=='.') count++;
    }
    tmpstr--;
    if (isip==1) {
        while (*tmpstr && *tmpstr!='.') tmpstr--;
        if (*tmpstr=='.') *tmpstr='\0';
        sprintf(userhost,"*%s.*",tilda);
        return;
    }
    if (count>1) {
        *string++='\0';
        if (count>2) count--;
        for (;*string && count>1;string++)
            if (*string=='.') count--;
        sprintf(userhost,"*%s@*.%s",tilda,string);
    }
    else sprintf(userhost,"*%s",tilda);
}

/* Toggles nethack protection on/off */
void NHProtToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  i;
    int  oldnh;
    char *tmpstr=(char *) 0;
    char *tmpchan=(char *) 0;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
    ChannelList *chan;
    WhowasChanList *whowas;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr && *tmpstr) {
        oldnh=NHProt;
        if (!my_stricmp(tmpstr,"ON")) NHProt=1;
        else if (!my_stricmp(tmpstr,"OFF")) NHProt=0;
        else {
            PrintUsage("NHPROT on channels/off quiet/medium/full");
            return;
        }
        tmpchan=tmpstr;
        tmpstr=new_next_arg(args,&args);
        if (!my_stricmp(tmpchan,"OFF") && tmpstr && *tmpstr) {
            if (!my_stricmp(tmpstr,"QUIET")) NHDisp=0;
            else if (!my_stricmp(tmpstr,"MEDIUM")) NHDisp=1;
            else if (!my_stricmp(tmpstr,"FULL")) NHDisp=2;
            else {
                NHProt=oldnh;
                PrintUsage("NHPROT on channels/off quiet/medium/full");
                return;
            }
        }
        else if (tmpstr && *tmpstr && args && *args) {
            tmpchan=tmpstr;
            tmpstr=new_next_arg(args,&args);
            if (!my_stricmp(tmpstr,"QUIET")) NHDisp=0;
            else if (!my_stricmp(tmpstr,"MEDIUM")) NHDisp=1;
            else if (!my_stricmp(tmpstr,"FULL")) NHDisp=2;
            else {
                NHProt=oldnh;
                PrintUsage("NHPROT on channels/off quiet/medium/full");
                return;
            }
            malloc_strcpy(&NHProtChannels,tmpchan);
        }
        else {
            PrintUsage("NHPROT on channels/off quiet/medium/full");
            return;
        }
        for (i=0;i<number_of_servers;i++)
            for (chan=server_list[i].chan_list;chan;chan=chan->next)
                chan->NHProt=NHProt?CheckChannel(chan->channel,NHProtChannels):0;
        for (whowas=whowas_chan_list;whowas;whowas=whowas->next)
            whowas->channellist->NHProt=
                NHProt?CheckChannel(whowas->channellist->channel,NHProtChannels):0;
        update_all_status();
    }
    strcpy(tmpbuf1,"Nethack protection");
    *tmpbuf3='\0';
    if (NHProt) {
#ifdef WANTANSI
        sprintf(tmpbuf2,"ON%s for channels : %s%s%s",Colors[COLOFF],
                CmdsColors[COLSETTING].color5,NHProtChannels,Colors[COLOFF]);
        strcpy(tmpbuf3,CmdsColors[COLSETTING].color2);
#else
        sprintf(tmpbuf2,"ON%c for channels : %c%s%c",bold,bold,NHProtChannels,
                bold);
#endif
        switch (NHDisp) {
            case 0:
                strcat(tmpbuf3,"QUIET");
                break;
            case 1:
                strcat(tmpbuf3,"MEDIUM");
                break;
            case 2:
                strcat(tmpbuf3,"FULL");
                break;
        }
        PrintSetting(tmpbuf1,tmpbuf2,", display is",tmpbuf3);
    }
    else {
#ifdef WANTANSI
        strcpy(tmpbuf3,CmdsColors[COLSETTING].color2);
#endif
        switch (NHDisp) {
            case 0:
                strcat(tmpbuf3,"QUIET");
                break;
            case 1:
                strcat(tmpbuf3,"MEDIUM");
                break;
            case 2:
                strcat(tmpbuf3,"FULL");
                break;
        }
        PrintSetting(tmpbuf1,"OFF",", display is",tmpbuf3);
    }
}

/* Prints out some channel statistics */
void ChanStat(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  bancount=0;
    int  users=0;
    int  ops=0;
    int  voice=0;
    char *channel;
    char tmpbuf[mybufsize/4];
    NickList *tmpnick;
    ChannelList *tmpchan;
    struct bans *tmpban;

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (!(tmpchan=lookup_channel(channel,curr_scr_win->server,0))) return;
        for (tmpban=tmpchan->banlist;tmpban;tmpban=tmpban->next) bancount++;
        for (tmpnick=tmpchan->nicks;tmpnick;tmpnick=tmpnick->next) {
            users++;
            if (tmpnick->chanop) ops++;
            if (tmpnick->voice) voice++;
        }
#ifdef WANTANSI
        say("Statistics for channel %s%s%s :",
            CmdsColors[COLSETTING].color5,tmpchan->channel,Colors[COLOFF]);
#ifdef HAVETIMEOFDAY
        say("Channel created in memory at %s%.24s%s",
            CmdsColors[COLSETTING].color2,ctime((time_t *) &(tmpchan->time.tv_sec)),
            Colors[COLOFF]);
#else  /* HAVETIMEOFDAY */
        say("Channel created in memory at %s%.24%s",
            CmdsColors[COLSETTING].color2,ctime(&(tmpchan->time)),Colors[COLOFF]);
#endif /* HAVETIMEOFDAY */
        sprintf(tmpbuf,"Ops         : %s%-5d%sDeops      : %s%-5d%sServops    : ",
                CmdsColors[COLSETTING].color2,tmpchan->pluso,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,tmpchan->minuso,Colors[COLOFF]);
        say("%s%s%-5d%sServdeops  : %s%d%s",tmpbuf,
            CmdsColors[COLSETTING].color2,tmpchan->servpluso,Colors[COLOFF],
            CmdsColors[COLSETTING].color2,tmpchan->servminuso,Colors[COLOFF]);
        sprintf(tmpbuf,"Bans        : %s%-5d%sUnbans     : %s%-5d%sServbans   : %s%-5d",
                CmdsColors[COLSETTING].color2,tmpchan->plusb,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,tmpchan->minusb,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,tmpchan->servplusb);
        say("%s%sServunbans : %s%d%s",tmpbuf,Colors[COLOFF],
            CmdsColors[COLSETTING].color2,tmpchan->servminusb,Colors[COLOFF]);
        sprintf(tmpbuf,"Topics      : %s%-5d%sKicks      : %s%-5d%sBans set   : %s%-5d",
                CmdsColors[COLSETTING].color2,tmpchan->topic,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,tmpchan->kick,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,bancount);
        say("%s%sPublics    : %s%-5d%s",tmpbuf,Colors[COLOFF],
            CmdsColors[COLSETTING].color2,tmpchan->pub,Colors[COLOFF]);
        sprintf(tmpbuf,"Total users : %s%-5d%sOpped      : %s%-5d%sUnopped    : %s%-5d",
                CmdsColors[COLSETTING].color2,users,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,ops,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,users-ops);
        say("%s%sVoiced     : %s%-5d%s",tmpbuf,Colors[COLOFF],
            CmdsColors[COLSETTING].color2,voice,Colors[COLOFF]);
#else  /* WANTANSI */
        say("Statistics for channel %s :",tmpchan->channel);
#ifdef HAVETIMEOFDAY
        say("Channel created in memory at %c%.24%c",
            bold,ctime((time_t *) &(tmpchan->time.tv_sec)),bold);
#else  /* HAVETIMEOFDAY */
        say("Channel created in memory at %c%.24%c",
            bold,ctime(&(tmpchan->time)),bold);
#endif /* HAVETIMEOFDAY */
        sprintf(tmpbuf,"Ops         : %c%-5d%cDeops      : %c%-5d%cServops    : ",
                bold,tmpchan->pluso,bold,bold,tmpchan->minuso,bold);
        say("%s%c%-5d%cServdeops  : %c%d%c",tmpbuf,
            bold,tmpchan->servpluso,bold,bold,tmpchan->servminuso,bold);
        sprintf(tmpbuf,"Bans        : %c%-5d%cUnbans     : %c%-5d%cServbans   : %c%-5d",
                bold,tmpchan->plusb,bold,bold,tmpchan->minusb,bold,
                bold,tmpchan->servplusb);
        say("%s%cServunbans : %c%d%c",tmpbuf,bold,bold,tmpchan->servminusb,bold);
        sprintf(tmpbuf,"Topics      : %c%-5d%cKicks      : %c%-5d%cBans set   : %c%-5d",
                bold,tmpchan->topic,bold,bold,tmpchan->kick,bold,
                bold,bancount);
        say("%s%cPublics    : %c%-5d%c",tmpbuf,bold,bold,tmpchan->pub,bold);
        sprintf(tmpbuf,"Total users : %c%-5d%cOpped      : %c%-5d%cUnopped    : %c%-5d",
                bold,users,bold,bold,ops,bold,bold,users-ops);
        say("%s%cVoiced     : %c%-5d%c",tmpbuf,bold,bold,voice,bold);
#endif /* WANTANSI */
    }
    else NoWindowChannel();
}

/* Does the ls command */
void Ls(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char tmpbuf[mybufsize/4];

    if (args && *args) sprintf(tmpbuf,"ls %s",args);
    else sprintf(tmpbuf,"ls");
    execcmd(NULL,tmpbuf,NULL);
}

/* Prints out ScrollZ help */
void SZHelp(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  noteof=1;
    char *filepath;
    char tmpbuf[mybufsize];

    strcpy(tmpbufhlp,"####");
    if (!(*args)) {
        strcpy(tmpbufhlp,"!MAIN");
        args=tmpbufhlp;
    }
    if (!my_stricmp(args,"CDCC")) {
        helpmcommand(empty_string);
        return;
    }
    filepath=OpenCreateFile("ScrollZ.help",0);
    if (!filepath || (helpfile=fopen(filepath,"r"))==NULL) {
        say("Can't open file ScrollZ.help");
        return;
    }
    if (!my_stricmp(args,"USER")) strcpy(tmpbufhlp,"!USER");
    if (!my_stricmp(args,"OPS")) strcpy(tmpbufhlp,"!OPS");
    if (!my_stricmp(args,"MISC")) strcpy(tmpbufhlp,"!MISC");
    if (!my_stricmp(args,"SETTINGS")) strcpy(tmpbufhlp,"!SETTINGS");
    if (my_stricmp(tmpbufhlp,"####")) {
        *tmpbuf='\0';
        noteof=1;
        say("------------------------------------------------------------------------");
        DontHold=0;
        while (noteof && my_strnicmp(tmpbuf,tmpbufhlp,strlen(tmpbufhlp)))
            noteof=readln(helpfile,tmpbuf);
        if (noteof) {
            say("%s",&tmpbuf[strlen(tmpbufhlp)+1]);
            ShowSZHelpPage(tmpbufhlp);
        }
        else fclose(helpfile);
        if (!my_stricmp(args,"!MAIN")) {
            say("Try %cSHELP Topic%c to get a list of topic related commands",bold,bold);
            say("or  %cSHELP Command%c to get help on using command",bold,bold);
        }
    }
    else {
        DontHold=0;
        FoundCommand=0;
        ShowSZCommandPage(args);
    }
}

/* Display one page from ScrollZ.help file */
void ShowSZHelpPage(line)
char *line;
{
    int  noteof=1;
    int  lineno=3;
    char tmpbuf1[mybufsize];

    while (lineno<curr_scr_win->display_size && noteof) {
        noteof=readln(helpfile,tmpbuf1);
        if (*tmpbuf1=='!') noteof=0;
        else if (noteof && *tmpbuf1==':') {
            ShowHelpLine(&tmpbuf1[1]);
            lineno++;
        }
    }
    if (!noteof) ShowSZHelp(line,"q");
    else if (DontHold) ShowSZHelpPage(line);
    else if (lineno>=curr_scr_win->display_size) add_wait_prompt("Press any key to continue, 'c' for continuous, 'q' to quit",ShowSZHelp,line,WAIT_PROMPT_KEY);
}

/* This waits for key press */
void ShowSZHelp(stuff,line)
char *stuff;
char *line;
{
    if (line && (*line=='q' || *line=='Q')) {
        fclose(helpfile);
        return;
    }
    else {
        if (line && (*line=='c' || *line=='C')) DontHold=1;
        ShowSZHelpPage(NULL);
    }
}

/* Display one page from ScrollZ.help file for one command */
void ShowSZCommandPage(line)
char *line;
{
    int  noteof=1;
    int  lineno=1;
    static int incommand=0;
    char *tmpstr;
    char *helpcmd;
    char tmpbuf[mybufsize];

    while (helpfile && lineno<curr_scr_win->display_size && noteof) {
        noteof=readln(helpfile,tmpbuf);
        if (*tmpbuf==':' || *tmpbuf=='!') {
            helpcmd=tmpbuf;
            tmpstr=tmpbuf;
            if (*helpcmd && *tmpbuf==':') {
                helpcmd=&tmpbuf[13];
                while (isspace(*helpcmd)) helpcmd--;
                helpcmd++;
                *helpcmd='\0';
                helpcmd=&tmpbuf[1];
                tmpstr=&tmpbuf[14];
                while (*tmpstr && isspace(*tmpstr)) tmpstr++;
            }
            incommand=0;
            if (*tmpbuf==':' && wild_match(line,helpcmd)) {
                say("------------------------------------------------------------------------");
#ifdef WANTANSI
                say("Help for command %s%s%s",
                    CmdsColors[COLHELP].color1,&tmpbuf[1],Colors[COLOFF]);
#else
                say("Help for command %c%s%c",22,&tmpbuf[1],22);
#endif
                say("%s",tmpstr);
                incommand=1;
                FoundCommand=1;
            }
            if (incommand) lineno+=3;
        }
        else if (incommand) {
            ShowHelpLine(tmpbuf);
            lineno++;
        }
    }
    if (helpfile) {
        if (!noteof) ShowSZHelp(tmpbuf,"q");
        else if (DontHold) ShowSZCommandPage(line);
        else if (lineno>=curr_scr_win->display_size) add_wait_prompt("Press any key to continue, 'c' for continuous, 'q' to quit",ShowSZCommand,line,WAIT_PROMPT_KEY);
    }
    if (!FoundCommand) {
        say("------------------------------------------------------------------------");
#ifdef WANTANSI
        say("Help for %s not found -  try %sSHELP INDEX%s for list of all commands",
            line,CmdsColors[COLHELP].color1,Colors[COLOFF]);
#else
        say("Help for %s not found - try SHELP INDEX for list of all commands",line);
#endif
    }
}

/* This waits for key press */
void ShowSZCommand(stuff,line)
char *stuff;
char *line;
{
    if (line && (*line=='q' || *line=='Q')) {
        fclose(helpfile);
        helpfile=NULL;
        return;
    }
    else {
        if (line && (*line=='c' || *line=='C')) DontHold=1;
        ShowSZCommandPage(stuff);
    }
}

/* Does /DCC CHAT nick */
void Chat(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    if (args && *args) dcc_chat(new_next_arg(args,&args));
    else if (LastChat) dcc_chat(LastChat);
    else say("No chat request so far");
}

/* Does /DCC CLOSE CHAT nick */
void NoChat(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *who;
    char tmpbuf[mybufsize/4];

    if (args && *args) who=new_next_arg(args,&args);
    else if (LastChat) who=LastChat;
    else {
        say("No chat request so far");
        return;
    }
    sprintf(tmpbuf,"CHAT %s",who);
    dcc_close(tmpbuf);
}

/* Launches finger on nick */
void Finger(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;
    char *tmpstr;
    char tmpbuf[mybufsize/4];
    void (*func)();
    NickList *joiner;

    if (*args) {
        tmpnick=new_next_arg(args,&args);
        if (strchr(tmpnick,'@')) {
            sprintf(tmpbuf,"finger %s",tmpnick);
            execcmd(NULL,tmpbuf,NULL);
            say("Launching finger on %s",tmpnick);
        }
        else {
            joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
            if (joiner && joiner->userhost) {
                tmpstr=joiner->userhost;
                if (*tmpstr=='~' || *tmpstr=='+') tmpstr++;
                sprintf(tmpbuf,"finger %s",tmpstr);
                execcmd(NULL,tmpbuf,NULL);
                say("Launching finger on %s",tmpstr);
            }
            else {
                func=(void(*)())FingerNew;
                inFlierWI++;
                add_userhost_to_whois(tmpnick,func);
            }
        }
    }
    else PrintUsage("FINGER nick or host");
}

/* Launches finger on nick if he/she is not on your joinlist */
void FingerNew(wistuff, tmpnick)
WhoisStuff *wistuff;
char *tmpnick;
{
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (inFlierWI) inFlierWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    if (*(wistuff->user)!='~') sprintf(tmpbuf1,"%s@%s",wistuff->user,wistuff->host);
    else sprintf(tmpbuf1,"%s@%s",&(wistuff->user[1]),wistuff->host);
    sprintf(tmpbuf2,"finger %s",tmpbuf1);
    execcmd(NULL,tmpbuf2,NULL);
    say("Launching finger on %s",tmpbuf1);
}

/* Checks for fake netsplit */
int Check4Fake(splitserver)
char *splitserver;
{
    char *tmpstr;
    int  length;

    tmpstr=splitserver;
    if (*tmpstr=='*') {
        tmpstr++;
        if (*tmpstr=='.') tmpstr++;
    }
    if (*tmpstr<'A') return(1);
    if (*tmpstr>'z') return(1);
    if (*tmpstr>'Z' && *tmpstr<'a') return(1);
    tmpstr=strchr(splitserver,' ');
    tmpstr++;
    if (strchr(tmpstr,' ')) return(1);
    if (*tmpstr=='*') {
        tmpstr++;
        if (*tmpstr=='.') tmpstr++;
    }
    if (*tmpstr<'A') return(1);
    if (*tmpstr>'z') return(1);
    if (*tmpstr>'Z' && *tmpstr<'a') return(1);
    length=strlen(splitserver)-1;
    if (splitserver[length]<'A') return(1);
    if (splitserver[length]>'z') return(1);
    if (splitserver[length]>'Z' && splitserver[length]<'a') return(1);
    return(0);
}

/* Sets your user mode */
void UserMode(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *line=new_next_arg(args,&args);

    send_to_server("MODE %s %s",get_server_nickname(from_server),
                   (line && *line)?line:empty_string);
}

/* Sets auto join on invite toggle */
void AutoJoinOnInvToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr=(char *) 0;
    char *tmpchan=(char *) 0;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr) {
        if ((!my_stricmp("ON",tmpstr)) || (!my_stricmp("AUTO",tmpstr))) {
            tmpchan=new_next_arg(args,&args);
            if (tmpchan) malloc_strcpy(&AutoJoinChannels,tmpchan);
            else {
                PrintUsage("AJOIN on/auto channels/off");
                return;
            }
            if (!my_stricmp("AUTO",tmpstr)) AutoJoinOnInv=2;
            else AutoJoinOnInv=1;
        }
        else if (!my_stricmp("OFF",tmpstr)) AutoJoinOnInv=0;
        else {
            PrintUsage("AJOIN on/auto channels/off");
            return;
        }
    }
    if (AutoJoinOnInv==2) PrintSetting("Auto join on invite","AUTO",
                                       " for channels :",AutoJoinChannels);
    else if (AutoJoinOnInv) PrintSetting("Auto join on invite","ON",
                                         " for channels :",AutoJoinChannels);
    else PrintSetting("Auto join on invite","OFF",empty_string,empty_string);
}

/* Prints all ScrollZ related settings */
void Settings(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];

#ifdef WANTANSI
    say("-----------------------= %sScrollZ settings%s =-----------------------",
        CmdsColors[COLSETTING].color1,Colors[COLOFF]);
    sprintf(tmpbuf1,"A-setaway time   : %s%-3d%sm ",
            CmdsColors[COLSETTING].color2,AutoAwayTime,Colors[COLOFF]);
    sprintf(tmpbuf2,"| A-join on invite : ");
    if (AutoJoinOnInv==2)
        sprintf(tmpbuf3,"%sAUTO%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,AutoJoinChannels,
                Colors[COLOFF]);
    else if (AutoJoinOnInv)
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,AutoJoinChannels,
                Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Ban type         : %s%c%s  ",
            CmdsColors[COLSETTING].color2,defban,Colors[COLOFF]);
    sprintf(tmpbuf2,"  | A-rejoin on kick : ");
    if (AutoRejoin)
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,AutoRejoinChannels,
                Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Ignore time      : %s%-3d%ss",
            CmdsColors[COLSETTING].color2,IgnoreTime,Colors[COLOFF]);
    strcat(tmpbuf1," | Fake modes disp  : ");
    if (ShowFakes) 
        sprintf(tmpbuf2,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                ShowFakesChannels,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Server notices   : ");
    if (ServerNotice) sprintf(tmpbuf2,"%sON%s ",CmdsColors[COLSETTING].color2,
                              Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Notify on away   : ");
    if (ShowAway) 
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                ShowAwayChannels,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"CTCP cloaking    : ");
    if (CTCPCloaking==1) sprintf(tmpbuf2,"%sON%s  ",CmdsColors[COLSETTING].color2,
                                 Colors[COLOFF]);
    else if (CTCPCloaking==2) sprintf(tmpbuf2,"%sHIDE%s",CmdsColors[COLSETTING].color2,
                                      Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2," | Kick ops         : ");
    if (KickOps)
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                KickOpsChannels,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"Ext msgs display : ");
    if (ExtMes) sprintf(tmpbuf2,"%sON%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Kick on flood    : ");
    if (KickOnFlood) 
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                KickOnFloodChannels,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"Show nick on pub : ");
    if (ShowNick) sprintf(tmpbuf2,"%sON%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strcat(tmpbuf2,"  | Kick on ban      : ");
    if (KickOnBan)
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                KickOnBanChannels,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"URL Catcher      : ");
    if (URLCatch==3) sprintf(tmpbuf2,"%sQUIET%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else if (URLCatch==2) sprintf(tmpbuf2,"%sAUTO%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else if (URLCatch) sprintf(tmpbuf2,"%sON%s   ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s  ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strcat(tmpbuf2,"| Bitch mode       : ");
    if (Bitch)
        sprintf(tmpbuf3,"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                BitchChannels,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    say("-------------------------= %sCdcc settings%s =------------------------",
        CmdsColors[COLSETTING].color1,Colors[COLOFF]);
    strcpy(tmpbuf1,"Cdcc auto get    : ");
    if (AutoGet) sprintf(tmpbuf2,"%sON%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Cdcc security    : ");
    if (Security) sprintf(tmpbuf3,"%sON%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Cdcc limit       : %s%-2d%s",
            CmdsColors[COLSETTING].color2,CdccLimit,Colors[COLOFF]);
    sprintf(tmpbuf2,"   | A-close idle send: %s%d%ss",
            CmdsColors[COLSETTING].color2,CdccIdle,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    sprintf(tmpbuf1,"Cdcc ptime       : %s%-3d%ss",
            CmdsColors[COLSETTING].color2,PlistTime,Colors[COLOFF]);
    sprintf(tmpbuf2," | Cdcc channels    : ");
    if (CdccChannels)
        sprintf(tmpbuf3,"%s%s%s",CmdsColors[COLSETTING].color5,CdccChannels,Colors[COLOFF]);
    else sprintf(tmpbuf3,"%sNone%s",CmdsColors[COLSETTING].color5,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Cdcc ntime       : %s%-3d%ss",
            CmdsColors[COLSETTING].color2,NlistTime,Colors[COLOFF]);
    say("%s",tmpbuf1);
    strcpy(tmpbuf2,"Cdcc long status : ");
    if (LongStatus) sprintf(tmpbuf1,"%s%sON%s ",tmpbuf2,
                            CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf1,"%s%sOFF%s",tmpbuf2,CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    sprintf(tmpbuf3,"  | Dcc on status bar: ");
    if (ShowDCCStatus) sprintf(tmpbuf2,"%s%sON%s ",tmpbuf3,
                            CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%s%sOFF%s",tmpbuf3,CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    if (CdccUlDir) sprintf(tmpbuf1,"Cdcc uldir       : %s%s%s",
                           CmdsColors[COLSETTING].color2,CdccUlDir,Colors[COLOFF]);
    else {
        getcwd(tmpbuf2,mybufsize);
        sprintf(tmpbuf1,"Cdcc uldir       : %s%s%s - current dir",
                CmdsColors[COLSETTING].color2,tmpbuf2,Colors[COLOFF]);
    }
    say("%s",tmpbuf1);
    if (CdccDlDir) sprintf(tmpbuf1,"Cdcc dldir       : %s%s%s",
                           CmdsColors[COLSETTING].color2,CdccDlDir,Colors[COLOFF]);
    else {
        getcwd(tmpbuf2,mybufsize);
        sprintf(tmpbuf1,"Cdcc dldir       : %s%s%s - current dir",
                CmdsColors[COLSETTING].color2,tmpbuf2,Colors[COLOFF]);
    }
    say("%s",tmpbuf1);
    say("----------------------= %sProtection settings%s =---------------------",
        CmdsColors[COLSETTING].color1,Colors[COLOFF]);
    strcpy(tmpbuf1,"Mass deop prot   : ");
    if (MDopWatch) {
        sprintf(tmpbuf2,"%sON%s for %s%s%s, %s%d%s deops in ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,MDopWatchChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,DeopSensor,Colors[COLOFF]);
        sprintf(tmpbuf3,"%s%2d%ss",CmdsColors[COLSETTING].color2,
                MDopTimer,Colors[COLOFF]);
        strcat(tmpbuf2,tmpbuf3);
    }
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Mass kick prot   : ");
    if (KickWatch) {
        sprintf(tmpbuf2,"%sON%s for %s%s%s, %s%d%s kicks in ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,KickWatchChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,KickSensor,Colors[COLOFF]);
        sprintf(tmpbuf3,"%s%2d%ss",CmdsColors[COLSETTING].color2,
                KickTimer,Colors[COLOFF]);
        strcat(tmpbuf2,tmpbuf3);
    }
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Nick flood prot  : ");
    if (NickWatch) {
        sprintf(tmpbuf2,"%sON%s for %s%s%s, %s%d%s nicks in ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,NickWatchChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,NickSensor,Colors[COLOFF]);
        sprintf(tmpbuf3,"%s%2d%ss",CmdsColors[COLSETTING].color2,
                NickTimer,Colors[COLOFF]);
        strcat(tmpbuf2,tmpbuf3);
    }
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Flood prot       : ");
    if (FloodProt>1)
        sprintf(tmpbuf2,"%sMAX%s, activates with %s%d%s messages in %s%d%s seconds",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,FloodMessages,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,FloodSeconds,Colors[COLOFF]);
    else if (FloodProt) sprintf(tmpbuf2,"%sON%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Nethack prot     : ");
    if (NHProt)
        sprintf(tmpbuf2,"%sON%s for %s%s%s, display is ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,NHProtChannels,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s, display is ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    switch (NHDisp) {
        case 0: 
            sprintf(tmpbuf3,"%sQUIET%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
            break;
        case 1:
            sprintf(tmpbuf3,"%sMEDIUM%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
            break;
        case 2:
            sprintf(tmpbuf3,"%sFULL%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
            break;
    }
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
#ifdef EXTRAS
    strcpy(tmpbuf1,"Idle kicks       : ");
    if (IdleKick)
        sprintf(tmpbuf2,"%s%s%s for %s%s%s after %s%d%s minutes",
                CmdsColors[COLSETTING].color2,IdleKick==1?"ON":"AUTO",Colors[COLOFF],
                CmdsColors[COLSETTING].color5,IdleKickChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,IdleTime,Colors[COLOFF]);
    else sprintf(tmpbuf2,"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
#endif /* EXTRAS */
#else  /* WANTANSI */
    say("-----------------------= %cScrollZ settings%c =-----------------------",
        bold,bold);
    sprintf(tmpbuf1,"A-setaway time   : %c%-3d%cm ",bold,AutoAwayTime,bold);
    sprintf(tmpbuf2,"| A-join on invite : ");
    if (AutoJoinOnInv==2)
        sprintf(tmpbuf3,"%cAUTO%c for %c%s%c",bold,bold,bold,AutoJoinChannels,bold);
    else if (AutoJoinOnInv)
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,AutoJoinChannels,bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Ban type         : %c%c%c  ",bold,defban,bold);
    sprintf(tmpbuf2,"  | A-rejoin on kick : ");
    if (AutoRejoin)
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,AutoRejoinChannels,bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Ignore time      : %c%-3d%cs",bold,IgnoreTime,bold);
    strcat(tmpbuf1," | Fake modes disp  : ");
    if (ShowFakes) 
        sprintf(tmpbuf2,"%cON%c for %c%s%c",bold,bold,bold,ShowFakesChannels,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Server notices   : ");
    if (ServerNotice) sprintf(tmpbuf2,"%cON%c ",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Notify on away   : ");
    if (ShowAway) 
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,ShowAwayChannels,bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"CTCP cloaking    : ");
    if (CTCPCloaking==1) sprintf(tmpbuf2,"%cON%c  ",bold,bold);
    else if (CTCPCloaking==2) sprintf(tmpbuf2,"%cHIDE%c",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c ",bold,bold);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2," | Kick ops         : ");
    if (KickOps)
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,KickOpsChannels,
                bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"Ext msgs display : ");
    if (ExtMes) sprintf(tmpbuf2,"%cON%c ",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Kick on flood    : ");
    if (KickOnFlood) 
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,KickOnFloodChannels,
                bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"Show nick on pub : ");
    if (ShowNick) sprintf(tmpbuf2,"%cON%c ",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Kick on ban      : ");
    if (KickOnBan)
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,KickOnBanChannels,
                bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strcpy(tmpbuf1,"URL Catcher      : ");
    if (URLCatch==2) sprintf(tmpbuf2,"%cAUTO%c",bold,bold);
    else if (URLCatch) sprintf(tmpbuf2,"%cON%c  ",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c ",bold,bold);
    strcat(tmpbuf2," | Bitch mode       : ");
    if (Bitch)
        sprintf(tmpbuf3,"%cON%c for %c%s%c",bold,bold,bold,BitchChannels,
                bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    say("-------------------------= %cCdcc settings%c =------------------------",
        bold,bold);
    strcpy(tmpbuf1,"Cdcc auto get    : ");
    if (AutoGet) sprintf(tmpbuf2,"%cON%c ",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    strcat(tmpbuf1,tmpbuf2);
    sprintf(tmpbuf2,"  | Cdcc security    : ");
    if (Security) sprintf(tmpbuf3,"%cON%c",bold,bold);
    else sprintf(tmpbuf3,"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Cdcc limit       : %c%-2d%c",bold,CdccLimit,bold);
    sprintf(tmpbuf2,"   | A-close idle send: %c%d%cs",bold,CdccIdle,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    sprintf(tmpbuf1,"Cdcc ptime       : %c%-3d%cs",bold,PlistTime,bold);
    sprintf(tmpbuf2," | Cdcc channels    : ");
    if (CdccChannels) sprintf(tmpbuf3,"%c%s%c",bold,CdccChannels,bold);
    else sprintf(tmpbuf3,"%cNone%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    sprintf(tmpbuf1,"Cdcc ptime       : %c%-3d%cs",bold,PlistTime,bold);
    say("%s",tmpbuf1);
    strcpy(tmpbuf2,"Cdcc long status : ");
    if (LongStatus) sprintf(tmpbuf1,"%s%cON%c ",tmpbuf2,bold,bold);
    else sprintf(tmpbuf1,"%s%cOFF%c",tmpbuf2,bold,bold);
    sprintf(tmpbuf3,"  | Dcc on status bar: ");
    if (ShowDCCStatus) sprintf(tmpbuf2,"%s%cON%c ",tmpbuf3,bold,bold);
    else sprintf(tmpbuf2,"%s%cOFF%c",tmpbuf3,bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    if (CdccUlDir) sprintf(tmpbuf1,"Cdcc uldir       : %c%s%c",bold,CdccUlDir,bold);
    else {
        getcwd(tmpbuf2,mybufsize);
        sprintf(tmpbuf1,"Cdcc uldir       : %c%s%c - current dir",bold,tmpbuf2,bold);
    }
    say("%s",tmpbuf1);
    if (CdccDlDir) sprintf(tmpbuf1,"Cdcc dldir       : %c%s%c",bold,CdccDlDir,bold);
    else {
        getcwd(tmpbuf2,mybufsize);
        sprintf(tmpbuf1,"Cdcc dldir       : %c%s%c - current dir",bold,tmpbuf2,bold);
    }
    say("%s",tmpbuf1);
    say("----------------------= %cProtection settings%c =---------------------",
        bold,bold);
    strcpy(tmpbuf1,"Mass deop prot   : ");
    if (MDopWatch) {
        sprintf(tmpbuf2,"%cON%c for %c%s%c, %c%d%c deops in ",bold,bold,bold,
                MDopWatchChannels,bold,bold,DeopSensor,bold);
        sprintf(tmpbuf3,"%c%2d%cs",bold,MDopTimer,bold);
        strcat(tmpbuf2,tmpbuf3);
    }
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Mass kick prot   : ");
    if (KickWatch) {
        sprintf(tmpbuf2,"%cON%c for %c%s%c, %c%d%c kicks in ",bold,bold,bold,
                KickWatchChannels,bold,bold,KickSensor,bold);
        sprintf(tmpbuf3,"%c%2d%cs",bold,KickTimer,bold);
        strcat(tmpbuf2,tmpbuf3);
    }
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Nick flood prot  : ");
    if (NickWatch) {
        sprintf(tmpbuf2,"%cON%c for %c%s%c, %c%d%c nicks in ",bold,bold,bold,
                NickWatchChannels,bold,bold,NickSensor,bold);
        sprintf(tmpbuf3,"%c%2d%cs",bold,NickTimer,bold);
        strcat(tmpbuf2,tmpbuf3);
    }
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Flood prot       : ");
    if (FloodProt>1)
        sprintf(tmpbuf2,"%cMAX%c, activates with %c%d%c messages in %c%d%c seconds",
                bold,bold,bold,FloodMessages,bold,bold,FloodSeconds,bold);
    else if (FloodProt) sprintf(tmpbuf2,"%cON%c ",bold,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strcpy(tmpbuf1,"Nethack prot     : ");
    if (NHProt)
        sprintf(tmpbuf2,"%cON%c for %c%s%c, display is ",bold,bold,bold,
                NHProtChannels,bold);
    else sprintf(tmpbuf2,"%cOFF%c, display is ",bold,bold);
    switch (NHDisp) {
        case 0: 
            sprintf(tmpbuf3,"%cQUIET%c",bold,bold);
            break;
        case 1:
            sprintf(tmpbuf3,"%cMEDIUM%c",bold,bold);
            break;
        case 2:
            sprintf(tmpbuf3,"%cFULL%c",bold,bold);
            break;
    }
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
#ifdef EXTRAS
    strcpy(tmpbuf1,"Idle kicks       : ");
    if (IdleKick)
        sprintf(tmpbuf2,"%c%s%c for %c%s%c after %c%d%c minutes",
                bold,IdleKick==1?"ON":"AUTO",bold,bold,IdleKickChannels,bold,bold,
                IdleTime,bold);
    else sprintf(tmpbuf2,"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
#endif /* EXTRAS */
#endif /* WANTANSI */
}

/* Sets flood protection toggle */
void FloodProtToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmparg;
    char *tmpmsgs;
    char *tmpsecs;
    char tmpbuf[mybufsize/4];

    if ((tmparg=new_next_arg(args,&args))) {
        if (!my_stricmp("MAX",tmparg)) {
            tmpmsgs=new_next_arg(args,&args);
            tmpsecs=new_next_arg(args,&args);
            if (tmpmsgs && tmpsecs && is_number(tmpmsgs) && is_number(tmpsecs) &&
                (FloodMessages=atoi(tmpmsgs))>0 && (FloodSeconds=atoi(tmpsecs))>0)
                FloodProt=2;
            else {
                PrintUsage("FLOODP on/off or max #messages #seconds");
                return;
            }
        }
        else if (!my_stricmp("ON",tmparg)) FloodProt=1;
        else if (!my_stricmp("OFF",tmparg)) FloodProt=0;
        else {
            PrintUsage("FLOODP on/off or max #messages #seconds");
            return;
        }
    }
    if (FloodProt>1) {
        sprintf(tmpbuf,"%d messages in %d seconds",FloodMessages,FloodSeconds);
        PrintSetting("Flood protection","MAX",", activates with",tmpbuf);
    }
    else if (FloodProt) PrintSetting("Flood protection","ON",empty_string,empty_string);
    else PrintSetting("Flood protection","OFF",empty_string,empty_string);
    update_all_status();
}

/* Checks if channel is on your friend's channels list */
int CheckChannel(channels,chanlist)
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
                if (minus) found=0;
                else found=1;
            }
            if (*tmpchan1==',') tmpchan1++;
        }
    }
    return(found);
}

/* Same as above except that channels is used for matching and chanlist is not */
int CheckChannel2(channels,chanlist)
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
            else {
                tmpstr2=tmpbuf2;
                minus=0;
            }
            if (*tmpbuf1=='-') {
                tmpstr1=&tmpbuf1[1];
                minus=1;
            }
            else tmpstr1=tmpbuf1;
            if (wild_match(tmpstr1,tmpstr2)) {
                if (minus) found=0;
                else found=1;
            }
            if (*tmpchan1==',') tmpchan1++;
        }
    }
    return(found);
}

#ifdef EXTRA_STUFF
/* Codes an array */
void CodeIt(file,tmp,length)
int file;
char *tmp;
int length;
{
    int count=0,code=0,what;
    char *tmpstr,c;

    tmpstr=EString;
    if (my_stricmp(EString,"NONE")) code=1;
    if (code) {
        while (count<length) {
            c=tmp[count];
            tmp[count]=(c^(*tmpstr))^105;
            tmpstr++;
            if (!(*tmpstr)) tmpstr=EString;
            count++;
        }
        what=1000000-length;
        write(file,&what,sizeof(int));
    }
}

/* Fixes filename */
void FixName(newname)
char *newname;
{
    char c;
    char tmpbuf[mybufsize/4];
    FILE *fp;

    c='a';
    if (CdccDlDir) sprintf(tmpbuf,"%s/ca%c",CdccDlDir,c);
    else sprintf(tmpbuf,"ca%c",c);
    while ((fp=fopen(tmpbuf,"r"))!=NULL) {
        fclose(fp);
        c++;
        if (CdccDlDir) sprintf(tmpbuf,"%s/ca%c",CdccDlDir,c);
        else sprintf(tmpbuf,"ca%c",c);
    }
    fclose(fp);
    sprintf(newname,"ca%c",c);
}
#endif

/* Like /NET in PhoEniX.irc */
void Net(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  servid;
    char *server=(char *) 0;
    char *port=(char *) 0;
    char *newnick=(char *) 0;
    char *newuser=(char *) 0;
    char *newreal=(char *) 0;
    char tmpbuf[mybufsize/4];

    if (!(server=new_next_arg(args,&args))) PrintUsage("NET server [port] [nick] [username] [realname]");
    else {
        if ((port=index(server,':'))) *port++='\0';
        else port=new_next_arg(args,&args);
        if (!port) port="6667";
        servid=find_in_server_list(server,atoi(port));
        if (servid>-1 && is_server_connected(servid)) {
            say("Already connected to server %s port %s",server,port);
            return;
        }
        newnick=new_next_arg(args,&args);
        newuser=new_next_arg(args,&args);
        newreal=new_next_arg(args,&args);
        if (!newnick) newnick=get_server_nickname(from_server);
        if (newuser) strmcpy(username,newuser,NAME_LEN);
        if (newreal) strmcpy(realname,newreal,REALNAME_LEN);
        strcpy(tmpbuf,"NEW");
        window(NULL,tmpbuf,NULL);
        sprintf(tmpbuf,"SERVER %s:%s::%s",server,port,newnick);
        say("Creating new window on server %s port %s",server,port);
        say("Hit CONTROL-W then ? for help on window commands");
        window(NULL,tmpbuf,NULL);
    }
}

/* Reads one line from file */
int readln(usfile,buffer)
FILE *usfile;
char *buffer;
{
    char *ok;

    *buffer='\0';
    ok=fgets(buffer,mybufsize,usfile);
    if (ok && buffer[strlen(buffer)-1]=='\n') buffer[strlen(buffer)-1]='\0';
    return(ok?1:0);
}

/* Scans string and returns one argument */
void NextArg(arg,pointer,retstr)
char *arg;
char **pointer;
char *retstr;
{
    char *newstr;

    while (isspace(*arg)) arg++;
    newstr=retstr;
    while (*arg && !isspace(*arg)) {
        *newstr=*arg;
        arg++;
        newstr++;
    };
    *newstr=0;
    *pointer=arg;
};

/* Prints error in ScrollZ.save */
void PrintError(string1,string2,lineno)
char *string1;
char *string2;
int  lineno;
{
#ifdef WANTANSI
    say("%sError%s in ScrollZ.save : %s%s%s %s, %sline %d%s",
        CmdsColors[COLWARNING].color1,Colors[COLOFF],
        CmdsColors[COLWARNING].color2,string1,Colors[COLOFF],string2,
        CmdsColors[COLWARNING].color3,lineno,Colors[COLOFF]);
#else
    say("%cError%c in ScrollZ.save : %s %s, line %d",bold,bold,string1,string2,lineno);
#endif
}

/* Sets on/off values from ScrollZ.save */
void OnOffSet(pointer,variable,error,lineno,command)
char **pointer;
int  *variable;
int  *error;
int  lineno;
char *command;
{
    char tmpbuf[mybufsize/4];

    NextArg(*pointer,pointer,tmpbuf);
    if (!my_stricmp(tmpbuf,"ON")) *variable=1;
    else if (!my_stricmp(tmpbuf,"OFF")) *variable=0;
    else {
        sprintf(tmpbuf,"in %s",command);
        PrintError("must be ON/OFF",tmpbuf,lineno);
        *error=1;
    }
}

/* Sets number values from ScrollZ.save */
void NumberSet(pointer,variable,error,lineno,command)
char **pointer;
int  *variable;
int  *error;
int  lineno;
char *command;
{
    int number=0;
    char tmpbuf[mybufsize/4];

    NextArg(*pointer,pointer,tmpbuf);
    number=atoi(tmpbuf);
    if (*tmpbuf && is_number(tmpbuf)) *variable=number;
    else {
        sprintf(tmpbuf,"in %s",command);
        PrintError("must be NUMBER",tmpbuf,lineno);
        *error=1;
    }
}

/* Sets dir values from ScrollZ.save */
void DirSet(pointer,variable,error,lineno,message,command)
char **pointer;
char **variable;
int  *error;
int  lineno;
char *message;
char *command;
{
    char tmpbuf[mybufsize/4];

    if (!message) message="must be DIR";
    NextArg(*pointer,pointer,tmpbuf);
    if (*tmpbuf) malloc_strcpy(variable,tmpbuf);
    else {
        sprintf(tmpbuf,"in %s",command);
        PrintError(message,tmpbuf,lineno);
        *error=1;
    }
}

/* Sets string values from ScrollZ.save */
void StringSet(pointer,variable,error,lineno,command)
char *pointer;
char **variable;
int  *error;
int  lineno;
char *command;
{
    char tmpbuf[mybufsize/4];

    while (*pointer && isspace(*pointer)) pointer++;
    if (*pointer) malloc_strcpy(variable,pointer);
    else {
        sprintf(tmpbuf,"in %s",command);
        PrintError("must be STRING",tmpbuf,lineno);
        *error=1;
    }
}

/* Sets on channels/off values from ScrollZ.save */
void ChannelsSet(pointer,variable,strvar,error,lineno,command,message)
char **pointer;
int  *variable;
char **strvar;
int  *error;
int  lineno;
char *command;
char *message;
{
    char tmpbuf[mybufsize/4];

    NextArg(*pointer,pointer,tmpbuf);
    if (!my_stricmp(tmpbuf,"ON")) {
        NextArg(*pointer,pointer,tmpbuf);
        if (*tmpbuf) {
            *variable=1;
            malloc_strcpy(strvar,tmpbuf);
        }
        else {
            if (!message) message="must be ON CHANLIST";
            sprintf(tmpbuf,"in %s",command);
            PrintError(message,tmpbuf,lineno);
            *error=1;
        }
    }
    else if (!my_stricmp(tmpbuf,"OFF")) {
        *variable=0;
        new_free(strvar);
    }
    else {
        sprintf(tmpbuf,"in %s",command);
        PrintError("must be OFF",tmpbuf,lineno);
        *error=1;
    }
}

/* Loads ScrollZ.save file */
int ScrollZLoad()
{
    int  i;
    int  error=0;
    int  lineno;
    int  number;
    int  ulnumber=0;
    char *pointer;
    char *chanlist=NULL;
    char *filepath;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
    FILE *usfile=NULL;
    struct friends *tmpfriend;
    struct friends *friendnew;
    struct autobankicks *abknew;
    struct words *wordnew;

    if (!OrigNick) malloc_strcpy(&OrigNick,nickname);
    if (!AutoReplyBuffer) {
        i=strlen(nickname)>3?3:strlen(nickname);
        AutoReplyBuffer=(char *) new_malloc(i+1);
        strmcpy(AutoReplyBuffer,nickname,i);
        AutoReplyBuffer[i]='\0';
    }
    say("Loading ScrollZ.save file...");
    filepath=OpenCreateFile("ScrollZ.save",0);
    if (!filepath || (usfile=fopen(filepath,"r"))==NULL) {
#ifdef WANTANSI
        say("%sError%s: Can't open file ScrollZ.save !",
            CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
        say("Can't open file ScrollZ.save");
#endif
        usersloaded=1;
        return(1);
    };
    lineno=0;
    while (readln(usfile,tmpbuf1)) {
        lineno++;
        pointer=tmpbuf1;
        NextArg(pointer,&pointer,tmpbuf3);
        if (!(*tmpbuf3)) continue;
        upper(tmpbuf3);
        if (!strcmp("ADDF",tmpbuf3)) {
            if ((friendnew=(struct friends *) new_malloc(sizeof(struct friends)))==NULL) {
#ifdef WANTANSI
                say("%sError%s: Not enough memory to load friend list !",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
                say("Not enough memory to load friend list !");
#endif
                fclose(usfile);
                usersloaded=1;
                return(1);
            };
            friendnew->privs=0;
            friendnew->userhost=(char *) 0;
            friendnew->channels=(char *) 0;
            friendnew->passwd=(char *) 0;
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(friendnew->userhost),tmpbuf3);
            else {
                new_free(&friendnew);
                PrintError("missing USERHOST","in ADDF",lineno);
                error=1;
                continue;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (!(*tmpbuf3)) {
                new_free(&friendnew);
                PrintError("missing ACCESS","in ADDF",lineno);
                error=1;
                continue;
            }
            if (is_number(tmpbuf3)) {
                number=0;
                number=atoi(tmpbuf3);
                if (number>0) friendnew->privs=number;
            }
            else friendnew->privs=CheckPrivs(tmpbuf3,NULL);
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(friendnew->channels),tmpbuf3);
            else {
                new_free(&friendnew);
                PrintError("missing CHANLIST","in ADDF",lineno);
                error=1;
                continue;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(friendnew->passwd),tmpbuf3);
            friendnew->next=NULL;
            tmpfriend=frlist;
            while (tmpfriend && tmpfriend->next) tmpfriend=tmpfriend->next;
            if (!frlist && tmpfriend==frlist) frlist=friendnew;
            else tmpfriend->next=friendnew;
            ulnumber++;
            friendnew->number=ulnumber;
            synch_whowas_adduser(friendnew);
        }
        else if (!strcmp("ADDBK",tmpbuf3)) {
            if ((abknew=(struct autobankicks *) new_malloc(sizeof(struct autobankicks)))==NULL) {
#ifdef WANTANSI
                say("%sError%s: Not enough memory to load shit list !",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
                say("Not enough memory to load shit list !",lineno);
#endif
                fclose(usfile);
                usersloaded=1;
                return(1);
            };
            abknew->shit=0;
            abknew->userhost=(char *) 0;
            abknew->reason=(char *) 0;
            abknew->channels=(char *) 0;
            abknew->next=NULL;
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(abknew->userhost),tmpbuf3);
            else {
                new_free(&abknew);
                PrintError("missing USERHOST","in ADDBK",lineno);
                error=1;
                continue;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (!(*tmpbuf3)) {
                new_free(&abknew);
                PrintError("missing SHIT","in ADDBK",lineno);
                error=1;
                continue;
            }
            if (is_number(tmpbuf3)) {
                number=0;
                number=atoi(tmpbuf3);
                if (number>0) abknew->shit=number;
            }
            else abknew->shit=CheckShit(tmpbuf3,NULL);
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(abknew->channels),tmpbuf3);
            else {
                new_free(&abknew);
                PrintError("missing CHANLIST","in ADDBK",lineno);
                error=1;
                continue;
            }
            while (*pointer==' ') pointer++;
            malloc_strcpy(&(abknew->reason),pointer);
            add_to_list_ext((List **) &abklist,(List *) abknew,
                            (int (*) _((List *, List *))) AddLast);
            synch_whowas_addshit(abknew);
        }
        else if (!strcmp("ADDN",tmpbuf3)) {
            while (pointer && *pointer && isspace(*pointer)) pointer++;
            if (pointer && *pointer) {
                inFlierNotify=1;
                strcpy(tmpbuf2,pointer);
                notify(NULL,tmpbuf2,NULL);
                inFlierNotify=0;
            }
            else {
                PrintError("missing NICK(s)","in ADDN",lineno);
                error=1;
            }
        }
        else if (!strcmp("ADDW",tmpbuf3)) {
            if ((wordnew=(struct words *) new_malloc(sizeof(struct words)))==NULL) {
#ifdef WANTANSI
                say("%sError%s: Not enough memory to load word kick list !",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
                say("Not enough memory to load word kick list !");
#endif
                fclose(usfile);
                usersloaded=1;
                return(1);
            };
            wordnew->channels=(char *) 0;
            wordnew->word=(char *) 0;
            wordnew->reason=(char *) 0;
            wordnew->next=NULL;
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(wordnew->channels),tmpbuf3);
            else {
                PrintError("missing CHANLIST","in ADDW",lineno);
                continue;
                error=1;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(wordnew->word),tmpbuf3);
            else {
                PrintError("missing WORD","in ADDW",lineno);
                continue;
                error=1;
            }
            while (pointer && *pointer && isspace(*pointer)) pointer++;
            if (pointer && *pointer) malloc_strcpy(&(wordnew->reason),pointer);
            else {
                sprintf(tmpbuf2,"You said %s",wordnew->word);
                malloc_strcpy(&(wordnew->reason),tmpbuf2);
            }
            add_to_list_ext((List **) &wordlist,(List *) wordnew,
                            (int (*) _((List *, List *))) AddLast);
        }
        else if (!strcmp("EXTMES",tmpbuf3))
            OnOffSet(&pointer,&ExtMes,&error,lineno,"EXTMES");
        else if (!strcmp("NHPROT",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            number=NHProt;
            if (!my_stricmp(tmpbuf3,"ON")) NHProt=1;
            else if (!my_stricmp(tmpbuf3,"OFF")) NHProt=0;
            else error=2;
            if (error!=2) {
                if (NHProt) {
                    NextArg(pointer,&pointer,tmpbuf2);
                    if (*tmpbuf2) chanlist=tmpbuf2;
                    else error=2;
                }
                if (error!=2) {
                    NextArg(pointer,&pointer,tmpbuf3);
                    if (*tmpbuf3) {
                        if (!my_stricmp(tmpbuf3,"QUIET")) NHDisp=0;
                        else if (!my_stricmp(tmpbuf3,"MEDIUM")) NHDisp=1;
                        else if (!my_stricmp(tmpbuf3,"FULL")) NHDisp=2;
                        else error=2;
                    }
                    else error=2;
                }
                if (error!=2) malloc_strcpy(&NHProtChannels,chanlist);
            }
            if (error==2) {
                NHProt=number;
                PrintError("must be ON CHANLIST/OFF QUIET/MEDIUM/FULL","in NHPROT",lineno);
            }
        }
        else if (!strcmp("BANTYPE",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"NORMAL")) defban='N';
            else if (!my_stricmp(tmpbuf3,"BETTER")) defban='B';
            else if (!my_stricmp(tmpbuf3,"ELITE")) defban='E';
            else if (!my_stricmp(tmpbuf3,"HOST")) defban='H';
            else if (!my_stricmp(tmpbuf3,"SCREW")) defban='S';
            else {
                PrintError("must be NORMAL/BETTER/HOST/SCREW","in BANTYPE",lineno);
                error=1;
            }
        }
        else if (!strcmp("CDCC",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            upper(tmpbuf3);
            if (!strcmp("LIMIT",tmpbuf3)) {
                int number=0;
                char tmpbuf[mybufsize/4];

                NextArg(pointer,&pointer,tmpbuf);
                number=atoi(tmpbuf);
                if (*tmpbuf && is_number(tmpbuf)) CdccLimit=number;
                else {
                    PrintError("must be NUMBER","in CDCC LIMIT",lineno);
                    error=1;
                }
                NextArg(pointer,&pointer,tmpbuf);
                number=atoi(tmpbuf);
                if (*tmpbuf && is_number(tmpbuf)) CdccQueueLimit=number;
            }
            else if (!strcmp("IDLE",tmpbuf3))
                NumberSet(&pointer,&CdccIdle,&error,lineno,"CDCC IDLE");
            else if (!strcmp("AUTOGET",tmpbuf3))
                OnOffSet(&pointer,&AutoGet,&error,lineno,"CDCC AUTOGET");
            else if (!strcmp("SECURE",tmpbuf3))
                OnOffSet(&pointer,&Security,&error,lineno,"CDCC SECURE");
            else if (!strcmp("PTIME",tmpbuf3))
                NumberSet(&pointer,&PlistTime,&error,lineno,"CDCC PTIME");
            else if (!strcmp("NTIME",tmpbuf3))
                NumberSet(&pointer,&NlistTime,&error,lineno,"CDCC NTIME");
            else if (!strcmp("CHANNELS",tmpbuf3))
                StringSet(pointer,&CdccChannels,&error,lineno,"CDCC CHANNELS");
            else if (!strcmp("LONGSTATUS",tmpbuf3))
                OnOffSet(&pointer,&LongStatus,&error,lineno,"CDCC LONGSTATUS");
            else if (!strcmp("OVERWRITE",tmpbuf3))
                OnOffSet(&pointer,&CdccOverWrite,&error,lineno,"CDCC OVERWRITE");
            else if (!strcmp("STATUS",tmpbuf3))
                OnOffSet(&pointer,&ShowDCCStatus,&error,lineno,"CDCC STATUS");
            else if (!strcmp("STATS",tmpbuf3))
                OnOffSet(&pointer,&CdccStats,&error,lineno,"CDCC STATS");
            else if (!strcmp("WARNING",tmpbuf3))
                OnOffSet(&pointer,&DCCWarning,&error,lineno,"CDCC WARNING");
            else if (!strcmp("ULDIR",tmpbuf3))
                DirSet(&pointer,&CdccUlDir,&error,lineno,NULL,"CDCC ULDIR");
            else if (!strcmp("DLDIR",tmpbuf3))
                DirSet(&pointer,&CdccDlDir,&error,lineno,NULL,"CDCC DLDIR");
#ifdef EXTRA_STUFF
            else if (!strcmp("E",tmpbuf3))
                DirSet(&pointer,&EString,&error,lineno,"missing STRING","CDCC E");
            else if (!strcmp("M",tmpbuf3))
                OnOffSet(&pointer,&RenameFiles,&error,lineno,"CDCC M");
#endif
        }
        else if (!strcmp("DEOPSENSOR",tmpbuf3))
            NumberSet(&pointer,&DeopSensor,&error,lineno,"DEOPSENSOR");
        else if (!strcmp("KICKSENSOR",tmpbuf3))
            NumberSet(&pointer,&KickSensor,&error,lineno,"KICKSENSOR");
        else if (!strcmp("NICKSENSOR",tmpbuf3))
            NumberSet(&pointer,&NickSensor,&error,lineno,"NICKSENSOR");
        else if (!strcmp("DEOPTIMER",tmpbuf3))
            NumberSet(&pointer,&MDopTimer,&error,lineno,"DEOPTIMER");
        else if (!strcmp("KICKTIMER",tmpbuf3))
            NumberSet(&pointer,&KickTimer,&error,lineno,"KICKTIMER");
        else if (!strcmp("NICKTIMER",tmpbuf3))
            NumberSet(&pointer,&NickTimer,&error,lineno,"NICKTIMER");
        else if (!strcmp("AUTOAWTIME",tmpbuf3))
            NumberSet(&pointer,&AutoAwayTime,&error,lineno,"AUTOAWTIME");
        else if (!strcmp("IGNORETIME",tmpbuf3))
            NumberSet(&pointer,&IgnoreTime,&error,lineno,"IGNORETIME");
        else if (!strcmp("SHITIGNORETIME",tmpbuf3))
            NumberSet(&pointer,&ShitIgnoreTime,&error,lineno,"SHITIGNORETIME");
#ifdef EXTRAS
        else if (!strcmp("IDLETIME",tmpbuf3))
            NumberSet(&pointer,&IdleTime,&error,lineno,"IDLETIME");
#endif
        else if (!strcmp("NICKWATCH",tmpbuf3))
            ChannelsSet(&pointer,&NickWatch,&NickWatchChannels,&error,lineno,"NICKWATCH",NULL);
        else if (!strcmp("MDOPWATCH",tmpbuf3))
            ChannelsSet(&pointer,&MDopWatch,&MDopWatchChannels,&error,lineno,"MDOPWATCH",NULL);
        else if (!strcmp("KICKWATCH",tmpbuf3))
            ChannelsSet(&pointer,&KickWatch,&KickWatchChannels,&error,lineno,"KICKWATCH",NULL);
        else if (!strcmp("AUTOREJOIN",tmpbuf3))
            ChannelsSet(&pointer,&AutoRejoin,&AutoRejoinChannels,&error,lineno,"AUTOREJOIN",NULL);
        else if (!strcmp("AUTOJOININV",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if ((!my_stricmp(tmpbuf3,"ON")) || (!my_stricmp(tmpbuf3,"AUTO"))) {
                if (!my_stricmp(tmpbuf3,"AUTO")) AutoJoinOnInv=2;
                else AutoJoinOnInv=1;
                NextArg(pointer,&pointer,tmpbuf3);
                if (*tmpbuf3) malloc_strcpy(&AutoJoinChannels,tmpbuf3);
                else {
                    PrintError("must be ON/AUTO CHANLIST","in AUTOJOININV",lineno);
                    error=1;
                }
            }
            else if (!my_stricmp(tmpbuf3,"OFF")) AutoJoinOnInv=0;
            else {
                PrintError("must be OFF","in AUTOJOINONINV",lineno);
                error=1;
            }
        }
#if defined(EXTRAS) || defined(FLIER)
        else if (!strcmp("AUTOINV",tmpbuf3))
            OnOffSet(&pointer,&AutoInv,&error,lineno,"AUTOINV");
#endif
        else if (!strcmp("FLOODPROT",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"MAX")) {
                NextArg(pointer,&pointer,tmpbuf3);
                if (!is_number(tmpbuf3) || (FloodMessages=atoi(tmpbuf3))<0) error=3;
                NextArg(pointer,&pointer,tmpbuf3);
                if (!is_number(tmpbuf3) || (FloodSeconds=atoi(tmpbuf3))<0) error=3;
                if (error!=3) FloodProt=2;
                else {
                    PrintError("must be ON/OFF or MAX #messages #seconds","in FLOODPROT",lineno);
                    error=1;
                }
            }
            else if (!my_stricmp(tmpbuf3,"ON")) FloodProt=1;
            else if (!my_stricmp(tmpbuf3,"OFF")) FloodProt=0;
            else {
                PrintError("must be ON/OFF or MAX #messages #seconds","in FLOODPROT",lineno);
                error=1;
            }
        }
        else if (!strcmp("SERVNOTICE",tmpbuf3))
            OnOffSet(&pointer,&ServerNotice,&error,lineno,"SERVNOTICE");
        else if (!strcmp("CTCPCLOAKING",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"ON")) CTCPCloaking=1;
            else if (!my_stricmp(tmpbuf3,"HIDE")) CTCPCloaking=2;
            else if (!my_stricmp(tmpbuf3,"OFF")) CTCPCloaking=0;
            else {
                PrintError("must be ON/OFF/HIDE","in CTCPCLOAKING",lineno);
                error=1;
            }
        }
        else if (!strcmp("SHOWFAKES",tmpbuf3))
            ChannelsSet(&pointer,&ShowFakes,&ShowFakesChannels,&error,lineno,"SHOWFAKES",NULL);
        else if (!strcmp("SHOWAWAY",tmpbuf3))
            ChannelsSet(&pointer,&ShowAway,&ShowAwayChannels,&error,lineno,"SHOWAWAY",NULL);
        else if (!strcmp("DEFSERVER",tmpbuf3))
            StringSet(pointer,&DefaultServer,&error,lineno,"DEFSERVER");
        else if (!strcmp("DEFSIGNOFF",tmpbuf3))
            StringSet(pointer,&DefaultSignOff,&error,lineno,"DEFSIGNOFF");
        else if (!strcmp("DEFSETAWAY",tmpbuf3))
            StringSet(pointer,&DefaultSetAway,&error,lineno,"DEFSETAWAY");
        else if (!strcmp("DEFSETBACK",tmpbuf3))
            StringSet(pointer,&DefaultSetBack,&error,lineno,"DEFSETBACK");
        else if (!strcmp("DEFUSERINFO",tmpbuf3))
            StringSet(pointer,&DefaultUserinfo,&error,lineno,"DEFUSERINFO");
        else if (!strcmp("DEFFINGER",tmpbuf3))
            StringSet(pointer,&DefaultFinger,&error,lineno,"DEFFINGER");
        else if (!strcmp("AUTOOPTIME",tmpbuf3))
            NumberSet(&pointer,&AutoOpDelay,&error,lineno,"AUTOOPTIME");
        else if (!strcmp("KICKOPS",tmpbuf3))
            ChannelsSet(&pointer,&KickOps,&KickOpsChannels,&error,lineno,"KICKOPS",NULL);
        else if (!strcmp("KICKONFLOOD",tmpbuf3))
            ChannelsSet(&pointer,&KickOnFlood,&KickOnFloodChannels,&error,lineno,"KICKONFLOOD",NULL);
        else if (!strcmp("SHOWNICK",tmpbuf3))
            OnOffSet(&pointer,&ShowNick,&error,lineno,"SHOWNICK");
        else if (!strcmp("KICKONBAN",tmpbuf3))
            ChannelsSet(&pointer,&KickOnBan,&KickOnBanChannels,&error,lineno,"KICKONBAN",NULL);
        else if (!strcmp("AWAYSAVE",tmpbuf3))
            NumberSet(&pointer,&AwaySaveSet,&error,lineno,"AWAYSAVE");
        else if (!strcmp("SHOWWALLOP",tmpbuf3))
            OnOffSet(&pointer,&ShowWallop,&error,lineno,"SHOWWALLOP");
        else if (!strcmp("AUTOREPLY",tmpbuf3))
            StringSet(pointer,&AutoReplyBuffer,&error,lineno,"AUTOREPLY");
        else if (!strcmp("ORIGNICK",tmpbuf3))
            ChannelsSet(&pointer,&OrigNickChange,&OrigNick,&error,lineno,"ORIGNICK","must be ON NICK");
        else if (!strcmp("NOTIFYMODE",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"BRIEF")) NotifyMode=1;
            else if (!my_stricmp(tmpbuf3,"VERBOSE")) NotifyMode=2;
            else {
                PrintError("must be Brief/Verbose","in NOTIFYMODE",lineno);
                error=1;
            }
        }
        else if (!strcmp("URLCATCHER",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"QUIET")) URLCatch=3;
            else if (!my_stricmp(tmpbuf3,"AUTO")) URLCatch=2;
            else if (!my_stricmp(tmpbuf3,"ON")) URLCatch=1;
            else if (!my_stricmp(tmpbuf3,"OFF")) URLCatch=0;
            else {
                PrintError("must be ON/AUTO/QUIET/OFF","in URLCATCHER",lineno);
                error=1;
            }
        }
        else if (!strcmp("EGO",tmpbuf3))
            OnOffSet(&pointer,&Ego,&error,lineno,"EGO");
        else if (!strcmp("AUTOCOMPLETION",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"ON") || !my_stricmp(tmpbuf3,"AUTO")) {
                if (!my_stricmp(tmpbuf3,"ON")) AutoNickCompl=1;
                else AutoNickCompl=2;
                if (pointer && *pointer) {
                    pointer++;
                    malloc_strcpy(&AutoReplyString,pointer);
                }
            }
            else if (!my_stricmp(tmpbuf3,"OFF")) AutoNickCompl=0;
            else {
                PrintError("must be ON/AUTO string/OFF","in AUTOCOMPLETION",lineno);
                error=1;
            }
        }
#ifdef EXTRAS
        else if (!strcmp("IDLEKICK",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"ON") || !my_stricmp(tmpbuf3,"AUTO")) {
                int newset;

                if (!my_stricmp(tmpbuf3,"ON")) newset=1;
                else newset=2;
                NextArg(pointer,&pointer,tmpbuf3);
                if (*tmpbuf3) {
                    IdleKick=newset;
                    malloc_strcpy(&IdleKickChannels,tmpbuf3);
                }
                else {
                    PrintError("must be ON/AUTO channels/OFF","in IDLEKICK",lineno);
                    error=1;
                }
            }
            else if (!my_stricmp(tmpbuf3,"OFF")) IdleKick=0;
            else {
                PrintError("must be ON/AUTO channels/OFF","in IDLEKICK",lineno);
                error=1;
            }
        }
#endif
        else if (!strcmp("DEFK",tmpbuf3))
            StringSet(pointer,&DefaultK,&error,lineno,"DEFK");
        else if (!strcmp("DEFBK",tmpbuf3))
            StringSet(pointer,&DefaultBK,&error,lineno,"DEFBK");
        else if (!strcmp("DEFBKI",tmpbuf3))
            StringSet(pointer,&DefaultBKI,&error,lineno,"DEFBKI");
        else if (!strcmp("DEFBKT",tmpbuf3))
            StringSet(pointer,&DefaultBKT,&error,lineno,"DEFBKT");
        else if (!strcmp("DEFFK",tmpbuf3))
            StringSet(pointer,&DefaultFK,&error,lineno,"DEFFK");
        else if (!strcmp("DEFLK",tmpbuf3))
            StringSet(pointer,&DefaultLK,&error,lineno,"DEFLK");
        else if (!strcmp("DEFABK",tmpbuf3))
            StringSet(pointer,&DefaultABK,&error,lineno,"DEFABK");
        else if (!strcmp("DEFSK",tmpbuf3))
            StringSet(pointer,&DefaultSK,&error,lineno,"DEFSK");
#ifdef ACID
	else if (!strcmp("DEFKILL",tmpbuf3))
	    StringSet(pointer,&DefaultKill,&error,lineno,"DEFKILL");
#endif
        else if (!strcmp("USERMODE",tmpbuf3))
            StringSet(pointer,&PermUserMode,&error,lineno,"USERMODE");
        else if (!strcmp("BITCHMODE",tmpbuf3))
            ChannelsSet(&pointer,&Bitch,&BitchChannels,&error,lineno,"BITCHMODE",NULL);
        else if (!strcmp("FRIENDLIST",tmpbuf3))
            ChannelsSet(&pointer,&FriendList,&FriendListChannels,&error,lineno,"FRIENDLIST",NULL);
        else if (!strcmp("COMPRESSMODES",tmpbuf3))
            ChannelsSet(&pointer,&CompressModes,&CompressModesChannels,&error,lineno,"COMPRESSMODES",NULL);
#ifdef EXTRAS
        else if (!strcmp("SIGNOFFCHANNELS",tmpbuf3))
            ChannelsSet(&pointer,&ShowSignoffChan,&SignoffChannels,&error,lineno,"SIGNOFFCHANNELS",NULL);
#endif
        else if (!strcmp("STAMP",tmpbuf3))
            ChannelsSet(&pointer,&Stamp,&StampChannels,&error,lineno,"STAMP",NULL);
        else if (!strcmp("NOTIFYSTR",tmpbuf3)) {
            StringSet(pointer,&CelerityNtfy,&error,lineno,"NOTIFYSTR");
            set_string_var(NOTIFY_STRING_VAR,CelerityNtfy);
        }
        else if (!strcmp("ORIGNICKTIME",tmpbuf3))
            NumberSet(&pointer,&OrigNickDelay,&error,lineno,"ORIGNICKTIME");
#ifdef WANTANSI
        else if (!strcmp("MIRCCOLORS",tmpbuf3))
            OnOffSet(&pointer,&DisplaymIRC,&error,lineno,"MIRCCOLORS");
        else if (!strcmp("COLOR",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            upper(tmpbuf3);
            if (!strcmp(tmpbuf3,"WARNING")) SetColors(COLWARNING,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"JOIN")) SetColors(COLJOIN,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"MSG")) SetColors(COLMSG,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"NOTICE")) SetColors(COLNOTICE,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"NETSPLIT")) SetColors(COLNETSPLIT,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"INVITE")) SetColors(COLINVITE,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"MODE")) SetColors(COLMODE,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"SETTING")) SetColors(COLSETTING,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"HELP")) SetColors(COLHELP,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"LEAVE")) SetColors(COLLEAVE,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"NOTIFY")) SetColors(COLNOTIFY,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"CTCP")) SetColors(COLCTCP,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"KICK")) SetColors(COLKICK,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"DCC")) SetColors(COLDCC,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"WHO")) SetColors(COLWHO,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"WHOIS")) SetColors(COLWHOIS,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"PUBLIC")) SetColors(COLPUBLIC,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"CDCC")) SetColors(COLCDCC,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"LINKS")) SetColors(COLLINKS,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"DCCCHAT")) SetColors(COLDCCCHAT,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"CSCAN")) SetColors(COLCSCAN,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"NICK")) SetColors(COLNICK,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"ME")) SetColors(COLME,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"MISC")) SetColors(COLMISC,&pointer,&error,lineno);
            else if (!strcmp(tmpbuf3,"SBAR")) SetColors(COLSBAR,&pointer,&error,lineno);
#ifdef CELECOSM
	    else if (!strcmp(tmpbuf3,"CELE")) SetColors(COLCELE,&pointer,&error,lineno);
#endif
#ifdef OPERVISION
            else if (!strcmp(tmpbuf3,"OV")) SetColors(COLOV,&pointer,&error,lineno);
#endif
            else {
                PrintError("missing or wrong CODE","in COLOR",lineno);
                error=1;
            }
        }
#endif
        else if (*tmpbuf3!='#') PrintError("unknown command",empty_string,lineno);
    };
    fclose(usfile);
    if (error) say("There were errors in ScrollZ.save");
    else say("ScrollZ.save sucessfully loaded");
    usersloaded=1;
    filepath=OpenCreateFile("ScrollZ.addon",0);
    if (filepath && (usfile=fopen(filepath,"r"))!=NULL) {
        while (readln(usfile,tmpbuf1)) {
            pointer=tmpbuf1;
            NextArg(pointer,&pointer,tmpbuf3);
#ifdef SCKICKS
            if (!my_stricmp("SKICK",tmpbuf3)) NumberOfScatterKicks++;
#endif
#ifndef CELE
            if (!my_stricmp("SIOFF",tmpbuf3)) NumberOfSignOffMsgs++;
#endif
        }
        fclose(usfile);
#ifdef SCKICKS
        if (NumberOfScatterKicks) NumberOfScatterKicks++;
#endif
#ifndef CELE
        if (NumberOfSignOffMsgs) NumberOfSignOffMsgs++;
#endif
    };
    if (PermUserMode) send_to_server("MODE %s %s",get_server_nickname(from_server),
                                     PermUserMode);
    return(0);
};

/* Lets you change your AutoReply Buffer - by Zakath */
void ReplyWord(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    if (args && *args) malloc_strcpy(&AutoReplyBuffer,new_next_arg(args,&args));
    PrintSetting("AutoReply Buffer",AutoReplyBuffer,empty_string,empty_string);
}

/* Clean friends list, shit list and word-kick list */
void CleanUpLists(void) {
    struct words *tmpword;
    struct friends *tmpfriend;
    struct autobankicks *tmpabk;

    while (frlist) {
        tmpfriend=frlist;
        frlist=tmpfriend->next;
        synch_whowas_unuser(tmpfriend);
        new_free(&(tmpfriend->userhost));
        new_free(&(tmpfriend->channels));
        new_free(&(tmpfriend->passwd));
        new_free(&tmpfriend);
    }
    while (abklist) {
        tmpabk=abklist;
        abklist=tmpabk->next;
        synch_whowas_unshit(tmpabk);
        new_free(&(tmpabk->userhost));
        new_free(&(tmpabk->channels));
        new_free(&(tmpabk->reason));
        new_free(&tmpabk);
    }
    while (wordlist) {
        tmpword=wordlist;
        wordlist=tmpword->next;
        new_free(&(tmpword->word));
        new_free(&(tmpword->reason));
        new_free(&(tmpword->channels));
        new_free(&tmpword);
    }
}

/* Resets your friends and auto (ban) kicks list and reloads ScrollZ.save */
void Reset(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int i;
    char tmpbuf[mybufsize/4];
    NickList *tmp;
    ChannelList *tmpchan;
    WhowasChanList *whowas;
    struct friends *tmpfriend;
    struct autobankicks *tmpabk;

    inFlierNotify=1;
    strcpy(tmpbuf,"-");
    notify(NULL,tmpbuf,NULL);
    inFlierNotify=0;
    CleanUpLists();
    ScrollZLoad();
    for (i=0;i<number_of_servers;i++)
        for (tmpchan=server_list[i].chan_list;tmpchan;tmpchan=tmpchan->next) {
            for (tmp=tmpchan->nicks;tmp;tmp=tmp->next) {
                if (tmp->userhost) sprintf(tmpbuf,"%s!%s",tmp->nick,tmp->userhost);
                else strcpy(tmpbuf,tmp->nick);
                for (tmpfriend=frlist;tmpfriend;tmpfriend=tmpfriend->next)
                    if (CheckChannel(tmpchan->channel,tmpfriend->channels) &&
                        wild_match(tmpfriend->userhost,tmpbuf))
                        tmp->frlist=tmpfriend;
                for (tmpabk=abklist;tmpabk;tmpabk=tmpabk->next)
                    if (CheckChannel(tmpchan->channel,tmpabk->channels) &&
                        wild_match(tmpabk->userhost,tmpbuf))
                        tmp->shitlist=tmpabk;
            }
            tmpchan->AutoRejoin=AutoRejoin?CheckChannel(tmpchan->channel,AutoRejoinChannels):0;
            tmpchan->MDopWatch=MDopWatch?CheckChannel(tmpchan->channel,MDopWatchChannels):0;
            tmpchan->ShowFakes=ShowFakes?CheckChannel(tmpchan->channel,ShowFakesChannels):0;
            tmpchan->KickOnFlood=KickOnFlood?CheckChannel(tmpchan->channel,KickOnFloodChannels):0;
            tmpchan->KickWatch=KickWatch?CheckChannel(tmpchan->channel,KickWatchChannels):0;
            tmpchan->NHProt=NHProt?CheckChannel(tmpchan->channel,NHProtChannels):0;
            tmpchan->NickWatch=NickWatch?CheckChannel(tmpchan->channel,NickWatchChannels):0;
            tmpchan->ShowAway=ShowAway?CheckChannel(tmpchan->channel,ShowAwayChannels):0;
            tmpchan->KickOps=KickOps?CheckChannel(tmpchan->channel,KickOpsChannels):0;
            tmpchan->KickOnBan=KickOnBan?CheckChannel(tmpchan->channel,KickOnBanChannels):0;
            tmpchan->Bitch=Bitch?CheckChannel(tmpchan->channel,BitchChannels):0;
            tmpchan->FriendList=FriendList?CheckChannel(tmpchan->channel,FriendListChannels):0;
#ifdef EXTRAS
            tmpchan->IdleKick=IdleKick?CheckChannel(tmpchan->channel,IdleKickChannels):0;
            if (tmpchan->IdleKick) tmpchan->IdleKick=IdleKick;
#endif
            tmpchan->CompressModes=CompressModes?CheckChannel(tmpchan->channel,CompressModesChannels):0;
            tmpchan->Stamp=Stamp?CheckChannel(tmpchan->channel,StampChannels):0;
        }
    for (whowas=whowas_chan_list;whowas;whowas=whowas->next) {
        whowas->channellist->AutoRejoin=
            AutoRejoin?CheckChannel(whowas->channellist->channel,AutoRejoinChannels):0;
        whowas->channellist->MDopWatch=
            MDopWatch?CheckChannel(whowas->channellist->channel,MDopWatchChannels):0;
        whowas->channellist->ShowFakes=
            ShowFakes?CheckChannel(whowas->channellist->channel,ShowFakesChannels):0;
        whowas->channellist->KickOnFlood=
            KickOnFlood?CheckChannel(whowas->channellist->channel,KickOnFloodChannels):0;
        whowas->channellist->KickWatch=
            KickWatch?CheckChannel(whowas->channellist->channel,KickWatchChannels):0;
        whowas->channellist->NHProt=
            NHProt?CheckChannel(whowas->channellist->channel,NHProtChannels):0;
        whowas->channellist->NickWatch=
            NickWatch?CheckChannel(whowas->channellist->channel,NickWatchChannels):0;
        whowas->channellist->ShowAway=
            ShowAway?CheckChannel(whowas->channellist->channel,ShowAwayChannels):0;
        whowas->channellist->KickOps=
            KickOps?CheckChannel(whowas->channellist->channel,KickOpsChannels):0;
        whowas->channellist->KickOnBan=
            KickOnBan?CheckChannel(whowas->channellist->channel,KickOnBanChannels):0;
        whowas->channellist->Bitch=
            Bitch?CheckChannel(whowas->channellist->channel,BitchChannels):0;
        whowas->channellist->FriendList=
            FriendList?CheckChannel(whowas->channellist->channel,FriendListChannels):0;
#ifdef EXTRAS
        whowas->channellist->IdleKick=
            IdleKick?CheckChannel(whowas->channellist->channel,IdleKickChannels):0;
        if (whowas->channellist->IdleKick) whowas->channellist->IdleKick=IdleKick;
#endif
        whowas->channellist->CompressModes=
            CompressModes?CheckChannel(whowas->channellist->channel,CompressModesChannels):0;
        whowas->channellist->Stamp=
            Stamp?CheckChannel(whowas->channellist->channel,StampChannels):0;
    }
}

/* Initializes ScrollZ related variables */
void InitVars() {
    int  i;
    char *tmpstr1;
    char *tmpstr2;
    char tmpbuf[mybufsize/4];

    frlist=NULL;
    abklist=NULL;
    wordlist=NULL;
    wholist=NULL;
    splitlist=NULL;
    splitlist1=NULL;
#ifdef ACID
    nickwatchlist=NULL;
#endif
    spinglist=NULL;
    MDopTimer=30;
    KickTimer=30;
    NickTimer=30;
    IgnoreTime=20;
    ShitIgnoreTime=120;
#ifdef EXTRA_STUFF
    malloc_strcpy(&EString,"NONE");
#endif
#ifdef CELE
    malloc_strcpy(&DefaultSignOff,"we need a default signoff message");
    malloc_strcpy(&DefaultSetAway,"mmm.. something better");
    malloc_strcpy(&DefaultSetBack,"are you there?");
    malloc_strcpy(&ScrollZstr,"[/]");
    malloc_strcpy(&DefaultK,"you == chump");
    malloc_strcpy(&DefaultBK,"you == bison");
    malloc_strcpy(&DefaultBKI,"you == bison");
    malloc_strcpy(&DefaultFK,"your domain == sucks");
    malloc_strcpy(&DefaultLK,"you == twit");
    malloc_strcpy(&DefaultABK,"a bison, a barrel, and a smoking gun");
    malloc_strcpy(&DefaultSK,"no chumps allowed");
    malloc_strcpy(&DefaultUserinfo,"userinfo");
    malloc_strcpy(&DefaultFinger,"finger info");
    malloc_strcpy(&DefaultBKT,"a bison, a barrel, and a smoking gun");
    AutoOpDelay=0;
#else
    malloc_strcpy(&DefaultServer,"NONE");
    malloc_strcpy(&DefaultSignOff,"That's it for today");
    malloc_strcpy(&DefaultSetAway,"Not here");
    malloc_strcpy(&DefaultSetBack,"What was going on ?");
    malloc_strcpy(&ScrollZstr,"[S+Z]");
    malloc_strcpy(&DefaultK,"I don't like you or something");
    malloc_strcpy(&DefaultBK,"I think you suck");
    malloc_strcpy(&DefaultBKI,"You are pain in the ass");
    malloc_strcpy(&DefaultBKT,"Getting rid of you is fun");
    malloc_strcpy(&DefaultFK,"You are annoying me");
    malloc_strcpy(&DefaultLK,"Cleaning the house");
    malloc_strcpy(&DefaultABK,"Going somewhere ?");
    malloc_strcpy(&DefaultSK,"Go away");
    malloc_strcpy(&DefaultUserinfo,"Are you from the FBI ?");
    malloc_strcpy(&DefaultFinger,"Are you from the CIA ?");    
    AutoOpDelay=4;
#endif
#ifdef ACID
    malloc_strcpy(&DefaultKill,"Go play somewhere else...");
#endif
    malloc_strcpy(&NickWatchChannels,"*");
    malloc_strcpy(&MDopWatchChannels,"*");
    malloc_strcpy(&KickWatchChannels,"*");
    malloc_strcpy(&ShowFakesChannels,"*");
    malloc_strcpy(&ShowAwayChannels,"*");
    malloc_strcpy(&KickOnFloodChannels,"*");
    malloc_strcpy(&KickOnBanChannels,"*");
    malloc_strcpy(&FriendListChannels,"*");
    malloc_strcpy(&LastJoin,"none yet");
    malloc_strcpy(&AutoReplyString,": ");
    malloc_strcpy(&CelerityNtfy,ScrollZstr);
    defban='B';
    inFlierWI=0;
    inFlierWho=0;
    inFlierNotify=0;
    inFlierLinks=0;
    inFlierFKill=0;
    inFlierNickCompl=0;
    inFlierTrace=0;
    ExtMes=1;
    NHProt=0;
    NHDisp=2;
    AutoGet=0;
    DeopSensor=4;
    KickSensor=4;
    NickSensor=4;
    AutoAwayTime=10;
    NickWatch=1;
    MDopWatch=1;
    KickWatch=1;
    AutoRejoin=0;
    AutoJoinOnInv=0;
#if defined(EXTRAS) || defined(FLIER)
    AutoInv=0;
#endif
    FloodProt=1;
    CdccIdle=60;
    CdccLimit=6;
    CdccQueueLimit=0;
    CdccOverWrite=1;
#ifdef EXTRA_STUFF
    RenameFiles=0;
#endif
    Security=0;
    ServerNotice=0;
    CTCPCloaking=0;
    ShowFakes=1;
    ShowAway=1;
#if defined(HAVETIMEOFDAY) && defined(CELE)
    LagTimer.tv_sec=0;
    LagTimer.tv_usec=0;
#else
    LagTimer=0;
#endif
    KickOps=0;
    KickOnFlood=1;
    KickOnBan=1;
#ifdef SCKICKS
    NumberOfScatterKicks=0;
#endif
#ifndef CELE
    NumberOfSignOffMsgs=0;
#endif
    ShowNick=1;
    PlistTime=600;
    NlistTime=600;
    LastCheck=time((time_t *) 0);
    LastPlist=LastCheck+360000L;
    LastNlist=LastCheck+360000L;
    LastServer=LastCheck+120L;
    LastNick=LastCheck;
    LastLinks=LastCheck;
    LinksNumber=0;
    AwaySaveSet=SAVEALL&(~SAVENOTIFY);
    ShowWallop=1;
    AutoReplyEntries=0;
    LongStatus=0;
    BytesReceived=0.0;
    BytesSent=0.0;
    FriendList=1;
    OrigNickChange=0;
    IRCQuit=0;
    NotifyMode=2;
    URLCatch=1;
    Ego=1;
    LogOn=0;
    ShowDCCStatus=0;
    DCCDone=0;
    AutoNickCompl=0;
#if defined(OPERVISION) && defined(WANTANSI)
    OperV=0;
#endif
    Bitch=0;
#ifdef EXTRAS
    IdleKick=0;
    IdleTime=10;
    ShowSignoffChan=0;
#endif
    CdccStats=1;
    CompressModes=0;
    OrigNickDelay=500;
#ifdef WANTANSI
    DisplaymIRC=0;
#endif
    DCCWarning=1;
    Stamp=0;
    usersloaded=0;
    unban=0;
    MangleVersion(tmpbuf);
    for (i=0,tmpstr1=tmpbuf;i<2;tmpstr1++) if (*tmpstr1==' ') i++;
    for (i=0,tmpstr2=tmpstr1;i<1;tmpstr2++) if (*tmpstr2==' ') i++;
    tmpstr2--;
    *tmpstr2='\0';
    malloc_strcpy(&ScrollZlame1,tmpstr1);
#ifdef IPCHECKING
    MangleString(global_track,tmpbuf,1);
    if (!(*tmpbuf)) strcpy(tmpbuf,ScrollZlame);
    malloc_strcpy(&channel_join,tmpbuf);
#endif
    InitKeysColors();
}

#ifdef EXTRAS
void LastMessageKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *tmpnick;
    ChannelList *chan;
    NickList *tmp;

    if (!(*args)) PrintUsage("DIRLMK nick");
    else {
        channel=get_channel_by_refnum(0);
        if (channel) {
            chan=lookup_channel(channel,curr_scr_win->server,0);
            if (chan && ((chan->status)&CHAN_CHOP)) {
                tmpnick=new_next_arg(args,&args);
                tmp=find_in_hash(chan,tmpnick);
                if (tmp) {
#if defined(VILAS)
                    if (LastMessage)
                        send_to_server("KICK %s %s :%s",channel,tmpnick,LastMessage);
                    else send_to_server("KICK %s %s :%s",channel,tmpnick,DefaultK);
#elif defined(CELE)
                    if (LastMessage) send_to_server("KICK %s %s :%s %s",channel,tmpnick,
                                                    LastMessage,CelerityL);
                    else send_to_server("KICK %s %s :%s %s",channel,tmpnick,
                                        DefaultK,CelerityL);
#else  /* CELE */
                    if (LastMessage) send_to_server("KICK %s %s :<ScrollZ-LMK> %s",
                                                    channel,tmpnick,LastMessage);
                    else send_to_server("KICK %s %s :<ScrollZ-K> %s",channel,tmpnick,
                                       DefaultK);
#endif /* VILAS */
                } 
                else say("Can't find %s on %s",tmpnick,channel);
            }
            else NotChanOp(channel);
        }
        else NoWindowChannel();
    }
}

void RandomLamerKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel;
    char *comment;
    ChannelList *chan;
    NickList *tmp;
    int count,found,random;

    if (args && *args) comment=args;
    else comment=DefaultLK;
    channel=get_channel_by_refnum(0);
    if (channel) {
        chan=lookup_channel(channel,curr_scr_win->server,0);
        if (chan && ((chan->status)&CHAN_CHOP)) {
            count=0;
            found=0;
            for (tmp=chan->nicks;tmp;tmp=tmp->next)
                if (!(tmp->chanop || tmp->voice)) count++;
            if (count) {
                srand(time(0));
                random=rand()%count+1;
                for (tmp=chan->nicks;tmp;tmp=tmp->next) {
                    if (!(tmp->chanop || tmp->voice)) found++;
                    if (found==random) break;
                }
                if (tmp) {
#if defined(VILAS)
                    send_to_server("KICK %s %s :%s",channel,tmp->nick,comment);
#elif defined(CELE)
                    send_to_server("KICK %s %s :%s %s",channel,tmp->nick,comment,CelerityL);
#else  /* CELE */
                    send_to_server("KICK %s %s :<ScrollZ-RLK> %s",channel,tmp->nick,comment);
#endif /* VILAS */
                }
            }
            else say("No lamers (?) in %s",channel);
        }
        else NotChanOp(channel);
    }
    else NoWindowChannel();
}
#endif
