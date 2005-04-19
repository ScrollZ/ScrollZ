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
 * $Id: edit3.c,v 1.85 2005-04-19 15:23:30 f Exp $
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
#include "parse.h"
#include "myvars.h"
#include "whowas.h"

#include <sys/stat.h> /* for umask() */

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
extern void PrintUsage _((char *));
extern void EncryptString _((char *, char *, char *, int, int));
extern int  AddLast _((List *, List *));
extern int  CheckPrivs _((char *, char *));
extern int  CheckShit _((char *, char *));
extern NickList * find_in_hash _((ChannelList *, char *));

extern void dcc_chat _((char *));
extern void dcc_close _((char *));

extern char *ScrollZver1;
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
        if (is_channel(channel)) strmcpy(tmpbuf1,channel,sizeof(tmpbuf1));
        else snprintf(tmpbuf1,sizeof(tmpbuf1),"#%s",channel);
        channel=tmpbuf1;
    }
    else if ((channel=get_channel_by_refnum(0))==NULL) {
        NoWindowChannel();
        return;
    }
    if (is_chanop(channel,get_server_nickname(from_server))) {
        if ((curmode=get_channel_mode(channel,from_server))!=NULL) {
            strmcpy(tmpbuf2,curmode,sizeof(tmpbuf2));
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
    put_it("");
    put_it("");
    put_it("      .dSSSSSSb..sSSs.  .SSSSb.  .sSb. SSS'.SS' .dSSS.");
    put_it("    .SSSP'  `SSS. `SSS .S' `SSSS.S `SSS S'.SS .SSS'`SS.");
    put_it("   .SSP'   .SS'SS  `SS SS    `SSSb  `SSS  SS.dSS'   XSS");
    put_it("   SS'   .dSS' `S.  `S SS     SSSS  d`SS .SSSSP    .SS'");
    put_it("   SS.   SSS'    .   ` SS    dSSP'  S SS SSSP     .SS'");
    put_it("   `SSSb.~SS     .sSSb.SS .sSSP'S   S SS `SS     .SS'");
    put_it("    `SSSSISS       `SSSSSSSSP'  S   S SS  SS    .SS'");
    put_it("       `SSSSSS       `SSSS`SS. SS   S SS  SS  .SSS'                .S");
    put_it("         s.XSSSS      `SS  `SS SS   S SS  SS  SSS'              .sSS");
    put_it("    .    SSS `SSS      SS   `S.SS   S.SS  SS  SSS.   .       .sSS'");
    put_it("    Sb   `SS  `SS  .SS SS    Sb`S   `SSS  S'.. SSS..s      .sSSS'");
    put_it("    SSSb  `XS dSS  SSS SS    `Sb`bs.dSS.  dSS  `XSSb.    .sSSS'");
    put_it("    `SSSSb. .dSS .dSS' SS     SSb.SSSS SSbSS SS SS`XSSb.dSSS'");
    put_it("      `SSSSSSSS SSSS'.SS'   .dSSSS`SS' `SS'   SS    `SSSSS'");
    put_it("                    dSS'   dSSSSS'    juice@3sheep");
    put_it("                .sSSSS'   `juice'");
    put_it("               SSSSSS      `SSP'         %s",ScrollZver1);
    put_it("               `SSS'");
    put_it("    coding   : flier (flier@globecom.net)");
    put_it("    celerity : Zakath (jeremy@3Sheep.COM)");
    put_it("    patches  : acidflash (acid@hostu.net)");
    put_it("    type /SZINFO for more information");
    put_it("");
    put_it("");
}

/* Saves message into ScrollZ.save file */
void AwaySave(message,type)
char *message;
int  type;
{
    int  oldumask;
    char *filepath;
    char *filename;
    char tmpbuf1[2 * mybufsize];
    char tmpbuf2[2 * mybufsize];
    FILE *awayfile;
    time_t now;

    if (type && !(type & AwaySaveSet)) return;
    oldumask = umask(0177);
    filename = get_string_var(AWAY_FILE_VAR);
    filepath = OpenCreateFile(filename, 1);
    if (filepath && (awayfile = fopen(filepath, "a")) != NULL) {
        now = time((time_t *) 0);
        if (type & SAVEMSG)          snprintf(tmpbuf1,sizeof(tmpbuf1), "%cMSG   %c", REV_TOG, REV_TOG);
        else if (type & SAVENOTICE)  strmcpy(tmpbuf1, "NOTICE", sizeof(tmpbuf1));
        else if (type & SAVEMASS)    strmcpy(tmpbuf1, "MASS  ", sizeof(tmpbuf1));
        else if (type & SAVECOLL)    strmcpy(tmpbuf1, "COLL  ", sizeof(tmpbuf1));
        else if (type & SAVECDCC)    strmcpy(tmpbuf1, "CDCC  ", sizeof(tmpbuf1));
        else if (type & SAVEDCC)     strmcpy(tmpbuf1, "DCC   ", sizeof(tmpbuf1));
        else if (type & SAVEPROT)    strmcpy(tmpbuf1, "PROT  ", sizeof(tmpbuf1));
        else if (type & SAVEHACK)    strmcpy(tmpbuf1, "HACK  ", sizeof(tmpbuf1));
        else if (type & SAVESRVM)    strmcpy(tmpbuf1, "SRVM  ", sizeof(tmpbuf1));
        else if (type & SAVECTCP)    strmcpy(tmpbuf1, "CTCP  ", sizeof(tmpbuf1));
        else if (type & SAVEFLOOD)   strmcpy(tmpbuf1, "FLOOD ", sizeof(tmpbuf1));
        else if (type & SAVEINVITE)  strmcpy(tmpbuf1, "INVITE", sizeof(tmpbuf1));
        else if (type & SAVEKILL)    strmcpy(tmpbuf1, "KILL  ", sizeof(tmpbuf1));
        else if (type & SAVEKICK)    strmcpy(tmpbuf1, "KICK  ", sizeof(tmpbuf1));
        else if (type & SAVESERVER)  strmcpy(tmpbuf1, "SERVER", sizeof(tmpbuf1));
        else if (type & SAVEFAKE)    strmcpy(tmpbuf1, "FAKE  ", sizeof(tmpbuf1));
        else if (type & SAVEAREPLY)  strmcpy(tmpbuf1, "AREPLY", sizeof(tmpbuf1));
        else if (type & SAVENOTIFY)  strmcpy(tmpbuf1, "NOTIFY", sizeof(tmpbuf1));
        else if (type & SAVESENTMSG) strmcpy(tmpbuf1, "SENTMSG", sizeof(tmpbuf1));
        else if (type & SAVEAWAY)    strmcpy(tmpbuf1, "AWAY", sizeof(tmpbuf1));
        else *tmpbuf1 = '\0';
        if (*tmpbuf1) snprintf(tmpbuf2, sizeof(tmpbuf2), "[%.24s] %s: %s", ctime(&now), tmpbuf1, message);
        else snprintf(tmpbuf2, sizeof(tmpbuf2), "[%.24s] %s", ctime(&now), message);
        StripAnsi(tmpbuf2, tmpbuf1, 2);
        if (AwayEncrypt && EncryptPassword) EncryptString(tmpbuf2, tmpbuf1, EncryptPassword, mybufsize, 0);
        else strmcpy(tmpbuf2, tmpbuf1, sizeof(tmpbuf2));
        fprintf(awayfile, "%s\n", tmpbuf2);
        fclose(awayfile);
    }
    umask(oldumask);
}

#if !defined(WANTANSI) || defined(MGS)
/* Returns user@domain if user@host given 
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
}*/
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

    strmcpy(tmpbuf,userhost,sizeof(tmpbuf));
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
        snprintf(tmpbuf2,sizeof(tmpbuf2),"ON%s for channels : %s%s%s",Colors[COLOFF],
                CmdsColors[COLSETTING].color5,NHProtChannels,Colors[COLOFF]);
        strmcpy(tmpbuf3,CmdsColors[COLSETTING].color2,sizeof(tmpbuf3));
#else
        snprintf(tmpbuf2,sizeof(tmpbuf2),"ON for channels : %c%s%c",bold,NHProtChannels,
                bold);
#endif
        switch (NHDisp) {
            case 0:
                strmcat(tmpbuf3,"QUIET",sizeof(tmpbuf3));
                break;
            case 1:
                strmcat(tmpbuf3,"MEDIUM",sizeof(tmpbuf3));
                break;
            case 2:
                strmcat(tmpbuf3,"FULL",sizeof(tmpbuf3));
                break;
        }
        PrintSetting(tmpbuf1,tmpbuf2,", display is",tmpbuf3);
    }
    else {
#ifdef WANTANSI
        strmcpy(tmpbuf3,CmdsColors[COLSETTING].color2,sizeof(tmpbuf3));
#endif
        switch (NHDisp) {
            case 0:
                strmcat(tmpbuf3,"QUIET",sizeof(tmpbuf3));
                break;
            case 1:
                strmcat(tmpbuf3,"MEDIUM",sizeof(tmpbuf3));
                break;
            case 2:
                strmcat(tmpbuf3,"FULL",sizeof(tmpbuf3));
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
    int  halfops=0;
    int  voice=0;
    char *channel;
    char tmpbuf[mybufsize/4];
    NickList *tmpnick;
    ChannelList *tmpchan;
    struct bans *tmpban;

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (!(tmpchan=lookup_channel(channel,curr_scr_win->server,0))) return;
        for (tmpban=tmpchan->banlist;tmpban;tmpban=tmpban->next)
            if (!(tmpban->exception)) bancount++;
        for (tmpnick=tmpchan->nicks;tmpnick;tmpnick=tmpnick->next) {
            users++;
            if (tmpnick->chanop) ops++;
	    if (tmpnick->halfop) halfops++;
            if (tmpnick->hasvoice) voice++;
        }
#ifdef WANTANSI
        say("Statistics for channel %s%s%s :",
            CmdsColors[COLSETTING].color5,tmpchan->channel,Colors[COLOFF]);
        say("Channel created in memory at %s%.24s%s",
            CmdsColors[COLSETTING].color2,ctime(&(tmpchan->creationtime)),Colors[COLOFF]);
	snprintf(tmpbuf, sizeof(tmpbuf), "Ops     : %s%-5d%s Deops    : %s%-5d%s Servops    : %s%-5d%s Servdeops    : %s%-5d%s", CmdsColors[COLSETTING].color2, tmpchan->pluso, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->minuso, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->servpluso, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->servminuso, Colors[COLOFF]);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Halfops : %s%-5d%s Dehalfops: %s%-5d%s Servhalfops: %s%-5d%s Servdehalfops: %s%-5d%s", CmdsColors[COLSETTING].color2, tmpchan->plush, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->minush, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->servplush, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->servminush, Colors[COLOFF]);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Bans    : %s%-5d%s Unbans   : %s%-5d%s Servbans   : %s%-5d%s Servunbans   : %s%-5d%s", CmdsColors[COLSETTING].color2, tmpchan->plusb, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->minusb, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->servplusb, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->servminusb, Colors[COLOFF]);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Bans set: %s%-5d%s Kicks    : %s%-5d%s Topics     : %s%-5d%s Publics      : %s%-5d%s", CmdsColors[COLSETTING].color2, bancount, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->kick, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->topic, Colors[COLOFF], CmdsColors[COLSETTING].color2, tmpchan->pub, Colors[COLOFF]);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Opped   : %s%-5d%s Halfopped: %s%-5d%s Unopped    : %s%-5d%s Voiced       : %s%-5d%s", CmdsColors[COLSETTING].color2, ops, Colors[COLOFF], CmdsColors[COLSETTING].color2, halfops, Colors[COLOFF], CmdsColors[COLSETTING].color2, users - ops - halfops, Colors[COLOFF], CmdsColors[COLSETTING].color2, voice, Colors[COLOFF]);
	say("%s", tmpbuf);
	say("Total   : %s%-5d%s", CmdsColors[COLSETTING].color2, users, Colors[COLOFF]);
#else  /* WANTANSI */
        say("Statistics for channel %s :",tmpchan->channel);
        say("Channel created in memory at %c%.24s%c",
            bold,ctime(&(tmpchan->creationtime)),bold);
	snprintf(tmpbuf, sizeof(tmpbuf), "Ops     : %c%-5d%c Deops    : %c%-5d%c Servops    : %c%-5d%c Servdeops    : %c%-5d%c", bold, tmpchan->pluso, bold, bold, tmpchan->minuso, bold, bold, tmpchan->servpluso, bold, bold, tmpchan->servminuso, bold);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Halfops : %c%-5d%c Dehalfops: %c%-5d%c Servhalfops: %c%-5d%c Servdehalfops: %c%-5d%c", bold, tmpchan->plush, bold, bold, tmpchan->minush, bold, bold, tmpchan->servplush, bold, bold, tmpchan->servminush, bold);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Bans    : %c%-5d%c Unbans   : %c%-5d%c Servbans   : %c%-5d%c Servunbans   : %c%-5d%c", bold, tmpchan->plusb, bold, bold, tmpchan->minusb, bold, bold, tmpchan->servplusb, bold, bold, tmpchan->servminusb, bold);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Bans set: %c%-5d%c Kicks    : %c%-5d%c Topics     : %c%-5d%c Publics      : %c%-5d%c", bold, bancount, bold, bold, tmpchan->kick, bold, bold, tmpchan->topic, bold, bold, tmpchan->pub, bold);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Opped   : %c%-5d%c Halfopped: %c%-5d%c Unopped    : %c%-5d%c Voiced       : %c%-5d%c", bold, ops, bold, bold, halfops, bold, bold, users - ops - halfops, bold, bold, voice, bold);
	say("%s", tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "Total   : %c%-5d%c", bold, users, bold);
#endif /* WANTANSI */
    }
    else NoWindowChannel();
}

/* Does the ls command */
#ifndef LITE
void Ls(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char tmpbuf[mybufsize/2+1];

    strcpy(tmpbuf,"ls ");
    if (args && *args) strmcat(tmpbuf,args,mybufsize/2);
    execcmd(NULL,tmpbuf,NULL);
}
#endif

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
    snprintf(tmpbuf,sizeof(tmpbuf),"CHAT %s",who);
    dcc_close(tmpbuf);
}

/* Launches finger on nick */
#ifndef LITE
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
            snprintf(tmpbuf,sizeof(tmpbuf),"finger %s",tmpnick);
            execcmd(NULL,tmpbuf,NULL);
            say("Launching finger on %s",tmpnick);
        }
        else {
            joiner=CheckJoiners(tmpnick,NULL,from_server,NULL);
            if (joiner && joiner->userhost) {
                tmpstr=joiner->userhost;
                if (*tmpstr=='~' || *tmpstr=='+') tmpstr++;
                snprintf(tmpbuf,sizeof(tmpbuf),"finger %s",tmpstr);
                execcmd(NULL,tmpbuf,NULL);
                say("Launching finger on %s",tmpstr);
            }
            else {
                func=(void(*)())FingerNew;
                server_list[from_server].SZWI++;
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

    if (server_list[from_server].SZWI) server_list[from_server].SZWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    if (*(wistuff->user)!='~') snprintf(tmpbuf1,sizeof(tmpbuf1),"%s@%s",wistuff->user,wistuff->host);
    else snprintf(tmpbuf1,sizeof(tmpbuf1),"%s@%s",&(wistuff->user[1]),wistuff->host);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"finger %s",tmpbuf1);
    execcmd(NULL,tmpbuf2,NULL);
    say("Launching finger on %s",tmpbuf1);
}
#endif /* LITE */

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
    char *opt=new_next_arg(args,&args);

    send_to_server("MODE %s %s %s",get_server_nickname(from_server),
                   (line && *line)?line:empty_string,
                   (opt && *opt)?opt:empty_string);
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
#ifndef LITE
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
    snprintf(tmpbuf1,sizeof(tmpbuf1),"A-setaway time   : %s%-3d%sm   ",
            CmdsColors[COLSETTING].color2,AutoAwayTime,Colors[COLOFF]);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| A-join on invite  : ");
    if (AutoJoinOnInv==2)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sAUTO%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,AutoJoinChannels,
                Colors[COLOFF]);
    else if (AutoJoinOnInv)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,AutoJoinChannels,
                Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Ban type         : %s%c%s      ",
            CmdsColors[COLSETTING].color2,defban,Colors[COLOFF]);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| A-rejoin on kick  : ");
    if (AutoRejoin)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,AutoRejoinChannels,
                Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Ignore time      : %s%-3d%ss   ",
            CmdsColors[COLSETTING].color2,IgnoreTime,Colors[COLOFF]);
    strmcat(tmpbuf1,"| Fake modes disp   : ",sizeof(tmpbuf1));
    if (ShowFakes) 
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                ShowFakesChannels,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Server notices   : ",sizeof(tmpbuf1));
    if (ServerNotice) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,
                              Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Notify on away    : ");
    if (ShowAway) 
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                ShowAwayChannels,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"CTCP cloaking    : ",sizeof(tmpbuf1));
    if (CTCPCloaking==1) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,
                                 Colors[COLOFF]);
    else if (CTCPCloaking==2) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sHIDE%s   ",CmdsColors[COLSETTING].color2,
                                      Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Kick ops          : ");
    if (KickOps)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                KickOpsChannels,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Ext msgs display : ",sizeof(tmpbuf1));
    if (ExtMes) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Kick on flood     : ");
    if (KickOnFlood) 
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                KickOnFloodChannels,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Show nick on pub : ",sizeof(tmpbuf1));
    if (ShowNick) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf2,"| Kick on ban       : ",sizeof(tmpbuf2));
    if (KickOnBan)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                KickOnBanChannels,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"URL Catcher      : ",sizeof(tmpbuf1));
    if (URLCatch==3) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sQUIET%s  ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else if (URLCatch==2) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sAUTO%s   ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else if (URLCatch) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf2,"| Bitch mode        : ",sizeof(tmpbuf2));
    if (Bitch)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                BitchChannels,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Auto reconnect   : ",sizeof(tmpbuf1));
    if (get_int_var(AUTO_RECONNECT_VAR))
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf2,"| Extended publics  : ",sizeof(tmpbuf2));
    if (ExtPub)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    say("-------------------------= %sCdcc settings%s =------------------------",
        CmdsColors[COLSETTING].color1,Colors[COLOFF]);
    strmcpy(tmpbuf1,"Cdcc auto get    : ",sizeof(tmpbuf1));
    if (AutoGet==1) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s     ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else if (AutoGet==2) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sALWAYS%s ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s    ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Cdcc security     : ");
    if (Security) snprintf(tmpbuf3,sizeof(tmpbuf3),"%sON%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc limit       : %s%-2d%s     ",
            CmdsColors[COLSETTING].color2,CdccLimit,Colors[COLOFF]);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| A-close idle send : %s%d%ss",
            CmdsColors[COLSETTING].color2,CdccIdle,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc ptime       : %s%-4d%ss  ",
            CmdsColors[COLSETTING].color2,PlistTime,Colors[COLOFF]);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Cdcc channels     : ");
    if (CdccChannels)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%s%s",CmdsColors[COLSETTING].color5,CdccChannels,Colors[COLOFF]);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%sNone%s",CmdsColors[COLSETTING].color5,Colors[COLOFF]);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc ntime       : %s%-3d%ss",
            CmdsColors[COLSETTING].color2,NlistTime,Colors[COLOFF]);
    say("%s",tmpbuf1);
    strmcpy(tmpbuf2,"Cdcc long status : ",sizeof(tmpbuf2));
    if (LongStatus) snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%sON%s     ",tmpbuf2,
                            CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%sOFF%s    ",tmpbuf2,CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    snprintf(tmpbuf3,sizeof(tmpbuf3),"| Dcc on status bar : ");
    if (ShowDCCStatus) snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%sON%s",tmpbuf3,
                            CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%sOFF%s",tmpbuf3,CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    if (CdccUlDir) snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc uldir       : %s%s%s",
                           CmdsColors[COLSETTING].color2,CdccUlDir,Colors[COLOFF]);
    else {
        getcwd(tmpbuf2,mybufsize);
        snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc uldir       : %s%s%s - current dir",
                CmdsColors[COLSETTING].color2,tmpbuf2,Colors[COLOFF]);
    }
    say("%s",tmpbuf1);
    if (CdccDlDir) snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc dldir       : %s%s%s",
                           CmdsColors[COLSETTING].color2,CdccDlDir,Colors[COLOFF]);
    else {
        getcwd(tmpbuf2,mybufsize);
        snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc dldir       : %s%s%s - current dir",
                CmdsColors[COLSETTING].color2,tmpbuf2,Colors[COLOFF]);
    }
    say("%s",tmpbuf1);
    say("----------------------= %sProtection settings%s =---------------------",
        CmdsColors[COLSETTING].color1,Colors[COLOFF]);
    strmcpy(tmpbuf1,"Mass deop prot   : ",sizeof(tmpbuf1));
    if (MDopWatch) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s, %s%d%s deops in ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,MDopWatchChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,DeopSensor,Colors[COLOFF]);
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%2d%ss",CmdsColors[COLSETTING].color2,
                MDopTimer,Colors[COLOFF]);
        strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
    }
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Mass kick prot   : ",sizeof(tmpbuf1));
    if (KickWatch) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s, %s%d%s kicks in ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,KickWatchChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,KickSensor,Colors[COLOFF]);
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%2d%ss",CmdsColors[COLSETTING].color2,
                KickTimer,Colors[COLOFF]);
        strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
    }
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Nick flood prot  : ",sizeof(tmpbuf1));
    if (NickWatch) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s, %s%d%s nicks in ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,NickWatchChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,NickSensor,Colors[COLOFF]);
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%2d%ss",CmdsColors[COLSETTING].color2,
                NickTimer,Colors[COLOFF]);
        strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
    }
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Flood prot       : ",sizeof(tmpbuf1));
    if (FloodProt>1)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sMAX%s, activates with %s%d%s messages in %s%d%s seconds",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,FloodMessages,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,FloodSeconds,Colors[COLOFF]);
    else if (FloodProt) snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Nethack prot     : ",sizeof(tmpbuf1));
    if (NHProt)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s, display is ",
                CmdsColors[COLSETTING].color2,Colors[COLOFF],
                CmdsColors[COLSETTING].color5,NHProtChannels,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s, display is ",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    switch (NHDisp) {
        case 0: 
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%sQUIET%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
            break;
        case 1:
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%sMEDIUM%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
            break;
        case 2:
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%sFULL%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
            break;
    }
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Friend list      : ",sizeof(tmpbuf1));
    if (FriendList)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                FriendListChannels,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Shit list        : ",sizeof(tmpbuf1));
    if (BKList)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sON%s for %s%s%s",CmdsColors[COLSETTING].color2,
                Colors[COLOFF],CmdsColors[COLSETTING].color5,
                BKChannels,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
#ifdef EXTRAS
    strmcpy(tmpbuf1,"Idle kicks       : ",sizeof(tmpbuf1));
    if (IdleKick)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%s%s for %s%s%s after %s%d%s minutes",
                CmdsColors[COLSETTING].color2,IdleKick==1?"ON":"AUTO",Colors[COLOFF],
                CmdsColors[COLSETTING].color5,IdleKickChannels,Colors[COLOFF],
                CmdsColors[COLSETTING].color2,IdleTime,Colors[COLOFF]);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%sOFF%s",CmdsColors[COLSETTING].color2,Colors[COLOFF]);
    say("%s%s",tmpbuf1,tmpbuf2);
#endif /* EXTRAS */
#else  /* WANTANSI */
    say("-----------------------= %cScrollZ settings%c =-----------------------",
        bold,bold);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"A-setaway time   : %c%-3d%cm   ",bold,AutoAwayTime,bold);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| A-join on invite  : ");
    if (AutoJoinOnInv==2)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cAUTO%c for %c%s%c",bold,bold,bold,AutoJoinChannels,bold);
    else if (AutoJoinOnInv)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,AutoJoinChannels,bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Ban type         : %c%c%c      ",bold,defban,bold);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| A-rejoin on kick  : ");
    if (AutoRejoin)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,AutoRejoinChannels,bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Ignore time      : %c%-3d%cs   ",bold,IgnoreTime,bold);
    strmcat(tmpbuf1,"| Fake modes disp   : ",sizeof(tmpbuf1));
    if (ShowFakes) 
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c",bold,bold,bold,ShowFakesChannels,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Server notices   : ",sizeof(tmpbuf1));
    if (ServerNotice) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Notify on away    : ");
    if (ShowAway) 
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,ShowAwayChannels,bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"CTCP cloaking    : ",sizeof(tmpbuf1));
    if (CTCPCloaking==1) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else if (CTCPCloaking==2) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cHIDE%c   ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Kick ops          : ");
    if (KickOps)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,KickOpsChannels,
                bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Ext msgs display : ",sizeof(tmpbuf1));
    if (ExtMes) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Kick on flood     : ");
    if (KickOnFlood) 
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,KickOnFloodChannels,
                bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Show nick on pub : ",sizeof(tmpbuf1));
    if (ShowNick) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Kick on ban       : ");
    if (KickOnBan)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,KickOnBanChannels,
                bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"URL Catcher      : ",sizeof(tmpbuf1));
    if (URLCatch==2) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cAUTO%c   ",bold,bold);
    else if (URLCatch) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf2,"| Bitch mode        : ",sizeof(tmpbuf2));
    if (Bitch)
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c for %c%s%c",bold,bold,bold,BitchChannels,
                bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Auto reconnect   : ",sizeof(tmpbuf1));
    if (get_int_var(AUTO_RECONNECT_VAR))
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf2,"| Extended publics  : ",sizeof(tmpbuf2));
    if (ExtPub) snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c     ",bold,bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c    ",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    say("-------------------------= %cCdcc settings%c =------------------------",
        bold,bold);
    strmcpy(tmpbuf1,"Cdcc auto get    : ",sizeof(tmpbuf1));
    if (AutoGet==1) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c     ",bold,bold);
    else if (AutoGet==2) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cALWAYS%c ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c    ",bold,bold);
    strmcat(tmpbuf1,tmpbuf2,sizeof(tmpbuf1));
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Cdcc security     : ");
    if (Security) snprintf(tmpbuf3,sizeof(tmpbuf3),"%cON%c",bold,bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cOFF%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc limit       : %c%-2d%c     ",bold,CdccLimit,bold);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| A-close idle send : %c%d%cs",bold,CdccIdle,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc ptime       : %c%-4d%cs  ",bold,PlistTime,bold);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"| Cdcc channels     : ");
    if (CdccChannels) snprintf(tmpbuf3,sizeof(tmpbuf3),"%c%s%c",bold,CdccChannels,bold);
    else snprintf(tmpbuf3,sizeof(tmpbuf3),"%cNone%c",bold,bold);
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc ptime       : %c%-3d%cs   ",bold,PlistTime,bold);
    say("%s",tmpbuf1);
    strmcpy(tmpbuf2,"Cdcc long status : ",sizeof(tmpbuf2));
    if (LongStatus) snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%cON%c     ",tmpbuf2,bold,bold);
    else snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%cOFF%c    ",tmpbuf2,bold,bold);
    snprintf(tmpbuf3,sizeof(tmpbuf3),"| Dcc on status bar : ");
    if (ShowDCCStatus) snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%cON%c ",tmpbuf3,bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%cOFF%c",tmpbuf3,bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    if (CdccUlDir) snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc uldir       : %c%s%c",bold,CdccUlDir,bold);
    else {
        getcwd(tmpbuf2,mybufsize);
        snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc uldir       : %c%s%c - current dir",bold,tmpbuf2,bold);
    }
    say("%s",tmpbuf1);
    if (CdccDlDir) snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc dldir       : %c%s%c",bold,CdccDlDir,bold);
    else {
        getcwd(tmpbuf2,mybufsize);
        snprintf(tmpbuf1,sizeof(tmpbuf1),"Cdcc dldir       : %c%s%c - current dir",bold,tmpbuf2,bold);
    }
    say("%s",tmpbuf1);
    say("----------------------= %cProtection settings%c =---------------------",
        bold,bold);
    strmcpy(tmpbuf1,"Mass deop prot   : ",sizeof(tmpbuf1));
    if (MDopWatch) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c, %c%d%c deops in ",bold,bold,bold,
                MDopWatchChannels,bold,bold,DeopSensor,bold);
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%c%2d%cs",bold,MDopTimer,bold);
        strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
    }
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Mass kick prot   : ",sizeof(tmpbuf1));
    if (KickWatch) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c, %c%d%c kicks in ",bold,bold,bold,
                KickWatchChannels,bold,bold,KickSensor,bold);
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%c%2d%cs",bold,KickTimer,bold);
        strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
    }
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Nick flood prot  : ",sizeof(tmpbuf1));
    if (NickWatch) {
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c, %c%d%c nicks in ",bold,bold,bold,
                NickWatchChannels,bold,bold,NickSensor,bold);
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%c%2d%cs",bold,NickTimer,bold);
        strmcat(tmpbuf2,tmpbuf3,sizeof(tmpbuf2));
    }
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Flood prot       : ",sizeof(tmpbuf1));
    if (FloodProt>1)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cMAX%c, activates with %c%d%c messages in %c%d%c seconds",
                bold,bold,bold,FloodMessages,bold,bold,FloodSeconds,bold);
    else if (FloodProt) snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c ",bold,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Nethack prot     : ",sizeof(tmpbuf1));
    if (NHProt)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c, display is ",bold,bold,bold,
                NHProtChannels,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c, display is ",bold,bold);
    switch (NHDisp) {
        case 0: 
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%cQUIET%c",bold,bold);
            break;
        case 1:
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%cMEDIUM%c",bold,bold);
            break;
        case 2:
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%cFULL%c",bold,bold);
            break;
    }
    say("%s%s%s",tmpbuf1,tmpbuf2,tmpbuf3);
    strmcpy(tmpbuf1,"Friend list      : ",sizeof(tmpbuf1));
    if (FriendList)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c",bold,bold,bold,
                FriendListChannels,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
    strmcpy(tmpbuf1,"Shit list        : ",sizeof(tmpbuf1));
    if (BKList)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%cON%c for %c%s%c",bold,bold,bold,
                BKChannels,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
#ifdef EXTRAS
    strmcpy(tmpbuf1,"Idle kicks       : ",sizeof(tmpbuf1));
    if (IdleKick)
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%c%s%c for %c%s%c after %c%d%c minutes",
                bold,IdleKick==1?"ON":"AUTO",bold,bold,IdleKickChannels,bold,bold,
                IdleTime,bold);
    else snprintf(tmpbuf2,sizeof(tmpbuf2),"%cOFF%c",bold,bold);
    say("%s%s",tmpbuf1,tmpbuf2);
#endif /* EXTRAS */
#endif /* WANTANSI */
}
#endif

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
        snprintf(tmpbuf,sizeof(tmpbuf),"%d messages in %d seconds",FloodMessages,FloodSeconds);
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
    if (CdccDlDir) snprintf(tmpbuf,sizeof(tmpbuf),"%s/ca%c",CdccDlDir,c);
    else snprintf(tmpbuf,sizeof(tmpbuf),"ca%c",c);
    while ((fp=fopen(tmpbuf,"r"))!=NULL) {
        fclose(fp);
        c++;
        if (CdccDlDir) snprintf(tmpbuf,sizeof(tmpbuf),"%s/ca%c",CdccDlDir,c);
        else snprintf(tmpbuf,sizeof(tmpbuf),"ca%c",c);
    }
    fclose(fp);
    snprintf(newname,sizeof(newname),"ca%c",c);
}
#endif

/* Like /NET in PhoEniX.irc */
void Net(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  servid;
    static int firsttime=1;
    char *server=(char *) 0;
    char *port=(char *) 0;
    char *newnick=(char *) 0;
    char *newuser=(char *) 0;
    char *newreal=(char *) 0;
    char tmpbuf[mybufsize/2];

    if (!(server=new_next_arg(args,&args))) PrintUsage("NET server [port] [nick] [username] [realname]");
    else {
        if ((port=index(server,':'))) *port++='\0';
        else port=new_next_arg(args,&args);
        if (!port) port="6667";
        newnick=new_next_arg(args,&args);
        newuser=new_next_arg(args,&args);
        newreal=new_next_arg(args,&args);
        if (!newnick) newnick=get_server_nickname(from_server);
        servid=find_in_server_list(server,atoi(port),newnick);
        if (servid>-1 && is_server_connected(servid)) {
            say("Already connected to server %s port %s",server,port);
            return;
        }
        if (newuser) strmcpy(username,newuser,NAME_LEN);
        if (newreal) strmcpy(realname,newreal,REALNAME_LEN);
        say("Creating new window on server %s port %s",server,port);
        if (firsttime) {
            say("Hit CONTROL-W then ? for help on window commands");
            firsttime=0;
        }
        snprintf(tmpbuf,sizeof(tmpbuf),"NEW SERVER %s:%s::%s HIDE",server,port,newnick);
        windowcmd(NULL,tmpbuf,NULL);
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
    }
    *newstr=0;
    *pointer=arg;
}

/* Prints error in ScrollZ.save */
void PrintError(string1,string2,lineno)
char *string1;
char *string2;
int  lineno;
{
#ifdef WANTANSI
    say("%sError%s in ScrollZ.save: %s%s%s %s, %sline %d%s",
        CmdsColors[COLWARNING].color1,Colors[COLOFF],
        CmdsColors[COLWARNING].color2,string1,Colors[COLOFF],string2,
        CmdsColors[COLWARNING].color3,lineno,Colors[COLOFF]);
#else
    say("%cError%c in ScrollZ.save: %s %s, line %d",bold,bold,string1,string2,lineno);
#endif
}

/* Sets on/off values from ScrollZ.save */
void OnOffSet(pointer,variable,loaderror,lineno,command)
char **pointer;
int  *variable;
int  *loaderror;
int  lineno;
char *command;
{
    char tmpbuf[mybufsize/4];

    NextArg(*pointer,pointer,tmpbuf);
    if (!my_stricmp(tmpbuf,"ON")) *variable=1;
    else if (!my_stricmp(tmpbuf,"OFF")) *variable=0;
    else if (!strcmp(command,"MIRCCOLORS") && !my_stricmp(tmpbuf,"STRIP")) *variable=2;
    else if (!strcmp(command,"CDCC VERBOSE") && !my_stricmp(tmpbuf,"QUIET")) *variable=2;
    else if (!strcmp(command,"STAMP") && !my_stricmp(tmpbuf,"MAX")) *variable=2;
    else if (!strcmp(command,"CDCC AUTOGET") && !my_stricmp(tmpbuf,"ALWAYS")) *variable=2;
    else {
        snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
        if (!strcmp(command,"MIRCCOLORS"))
            PrintError("must be ON/OFF/STRIP",tmpbuf,lineno);
        else if (!strcmp(command,"CDCC VERBOSE"))
            PrintError("must be ON/OFF/VERBOSE",tmpbuf,lineno);
        else if (!strcmp(command,"STAMP"))
            PrintError("must be ON/OFF/MAX",tmpbuf,lineno);
        else if (!strcmp(command,"CDCC AUTOGET"))
            PrintError("must be ON/OFF/ALWAYS",tmpbuf,lineno);
        else PrintError("must be ON/OFF",tmpbuf,lineno);
        *loaderror=1;
    }
}

/* Sets number values from ScrollZ.save */
void NumberSet(pointer,variable,loaderror,lineno,command)
char **pointer;
int  *variable;
int  *loaderror;
int  lineno;
char *command;
{
    int number=0;
    char tmpbuf[mybufsize/4];

    NextArg(*pointer,pointer,tmpbuf);
    number=atoi(tmpbuf);
    if (*tmpbuf && is_number(tmpbuf)) *variable=number;
    else {
        snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
        PrintError("must be NUMBER",tmpbuf,lineno);
        *loaderror=1;
    }
}

/* Sets dir values from ScrollZ.save */
void DirSet(pointer,variable,loaderror,lineno,message,command)
char **pointer;
char **variable;
int  *loaderror;
int  lineno;
char *message;
char *command;
{
    char tmpbuf[mybufsize/4];

    if (!message) message="must be DIR";
    NextArg(*pointer,pointer,tmpbuf);
    if (*tmpbuf) malloc_strcpy(variable,tmpbuf);
    else {
        snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
        PrintError(message,tmpbuf,lineno);
        *loaderror=1;
    }
}

/* Sets string values from ScrollZ.save */
void StringSet(pointer,variable,loaderror,lineno,command)
char *pointer;
char **variable;
int  *loaderror;
int  lineno;
char *command;
{
    char tmpbuf[mybufsize/4];

    while (*pointer && isspace(*pointer)) pointer++;
    if (*pointer) malloc_strcpy(variable,pointer);
    else {
        snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
        PrintError("must be STRING",tmpbuf,lineno);
        *loaderror=1;
    }
}

/* Sets on channels/off values from ScrollZ.save */
void ChannelsSet(pointer,variable,strvar,loaderror,lineno,command,message)
char **pointer;
int  *variable;
char **strvar;
int  *loaderror;
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
            snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
            PrintError(message,tmpbuf,lineno);
            *loaderror=1;
        }
    }
    else if (!strcmp(command,"ORIGNICK") && !my_stricmp(tmpbuf,"QUIET")) {
        NextArg(*pointer,pointer,tmpbuf);
        if (*tmpbuf) {
            *variable=1;
            OrigNickQuiet=1;
            malloc_strcpy(strvar,tmpbuf);
        }
        else {
            snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
            PrintError(message,tmpbuf,lineno);
            *loaderror=1;
        }
    }
    else if (!my_stricmp(tmpbuf,"OFF")) {
        *variable=0;
        new_free(strvar);
    }
    else {
        snprintf(tmpbuf,sizeof(tmpbuf),"in %s",command);
        PrintError("must be OFF",tmpbuf,lineno);
        *loaderror=1;
    }
}

/* Loads ScrollZ.save file */
void ScrollZLoad()
{
    int  lineno;
    int  number;
    int  ulnumber=0;
    int  loaderror=0;
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
    /* if this is a new user (indicated by default LOAD_PATH and
       no ScrollZ.save file) we create .ScrollZ directory for them
       and empty ScrollZ.save file */
    filepath=get_string_var(LOAD_PATH_VAR);
    if (filepath && !strcmp(filepath,IRCPATH)) {
        filepath=OpenCreateFile("ScrollZ.save",0);
        if (!filepath || (usfile=fopen(filepath,"r"))==NULL) {
            say("This seems to be the first time you have run ScrollZ");
            say("Directory .ScrollZ will now be created in your home directory");
            snprintf(tmpbuf1,sizeof(tmpbuf1),"%s/.ScrollZ",my_path);
            if (mkdir(tmpbuf1,0700)<0) {
#ifdef WANTANSI
                say("%sError%s: Can't create directory %s: %s!",
                        CmdsColors[COLWARNING].color1,Colors[COLOFF],tmpbuf1,strerror(errno));
#else
                say("Can't create directory %s: %s!",tmpbuf1,strerror(errno));
#endif
                return;
            }
            else {
                snprintf(tmpbuf1,sizeof(tmpbuf1),"%s/.ScrollZ/ScrollZ.save",my_path);
                usfile=fopen(tmpbuf1,"w");
                if (usfile) fclose(usfile);
                return;
            }
        }
    }
    say("Loading ScrollZ.save file...");
    filepath=OpenCreateFile("ScrollZ.save",0);
    if (!filepath || (usfile=fopen(filepath,"r"))==NULL) {
#ifdef WANTANSI
        say("%sError%s: Can't open file ScrollZ.save!",
            CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
        say("Can't open file ScrollZ.save!");
#endif
        usersloaded=1;
        return;
    }
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
                say("%sError%s: Not enough memory to load friend list!",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
                say("Not enough memory to load friend list!");
#endif
                fclose(usfile);
                usersloaded=1;
                return;
            }
            friendnew->privs=0;
            friendnew->userhost=(char *) 0;
            friendnew->channels=(char *) 0;
            friendnew->passwd=(char *) 0;
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(friendnew->userhost),tmpbuf3);
            else {
                new_free(&friendnew);
                PrintError("missing USERHOST","in ADDF",lineno);
                loaderror=1;
                continue;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (!(*tmpbuf3)) {
                new_free(&friendnew);
                PrintError("missing ACCESS","in ADDF",lineno);
                loaderror=1;
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
                loaderror=1;
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
                say("%sError%s: Not enough memory to load shit list!",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
                say("Not enough memory to load shit list!",lineno);
#endif
                fclose(usfile);
                usersloaded=1;
                return;
            }
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
                loaderror=1;
                continue;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (!(*tmpbuf3)) {
                new_free(&abknew);
                PrintError("missing SHIT","in ADDBK",lineno);
                loaderror=1;
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
                loaderror=1;
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
                inSZNotify=1;
                strmcpy(tmpbuf2,pointer,sizeof(tmpbuf2));
                notify(NULL,tmpbuf2,NULL);
                inSZNotify=0;
            }
            else {
                PrintError("missing NICK(s)","in ADDN",lineno);
                loaderror=1;
            }
        }
        else if (!strcmp("ADDW",tmpbuf3)) {
            if ((wordnew=(struct words *) new_malloc(sizeof(struct words)))==NULL) {
#ifdef WANTANSI
                say("%sError%s: Not enough memory to load word kick list!",
                    CmdsColors[COLWARNING].color1,Colors[COLOFF]);
#else
                say("Not enough memory to load word kick list!");
#endif
                fclose(usfile);
                usersloaded=1;
                return;
            }
            wordnew->channels=NULL;
            wordnew->word=NULL;
            wordnew->reason=NULL;
            wordnew->ban=0;
            wordnew->bantime=0;
            wordnew->next=NULL;
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3 && !my_stricmp(tmpbuf3,"-BAN")) {
                wordnew->ban=1;
                NextArg(pointer,&pointer,tmpbuf3);
            }
            if (*tmpbuf3 && !my_stricmp(tmpbuf3,"-TIME")) {
                NextArg(pointer,&pointer,tmpbuf3);
                if (*tmpbuf3) wordnew->bantime=atoi(tmpbuf3);
                else {
                    PrintError("missing BANTIME","in ADDW",lineno);
                    loaderror=1;
                    continue;
                }
                NextArg(pointer,&pointer,tmpbuf3);
            }
            if (*tmpbuf3) malloc_strcpy(&(wordnew->channels),tmpbuf3);
            else {
                PrintError("missing CHANLIST","in ADDW",lineno);
                loaderror=1;
                continue;
            }
            NextArg(pointer,&pointer,tmpbuf3);
            if (*tmpbuf3) malloc_strcpy(&(wordnew->word),tmpbuf3);
            else {
                PrintError("missing WORD","in ADDW",lineno);
                loaderror=1;
                continue;
            }
            while (pointer && *pointer && isspace(*pointer)) pointer++;
            if (pointer && *pointer) malloc_strcpy(&(wordnew->reason),pointer);
            else {
                snprintf(tmpbuf2,sizeof(tmpbuf2),"You said %s",wordnew->word);
                malloc_strcpy(&(wordnew->reason),tmpbuf2);
            }
            add_to_list_ext((List **) &wordlist,(List *) wordnew,
                            (int (*) _((List *, List *))) AddLast);
        }
        else if (!strcmp("IGN",tmpbuf3)) {
            int display=window_display;
            char *ign=NULL;
            char *tmpstr;

            NextArg(pointer,&pointer,tmpbuf2);
            NextArg(pointer,&pointer,tmpbuf3);
            if (!(*tmpbuf2)) {
                PrintError("missing PATTERN","in IGN",lineno);
                loaderror=1;
                continue;
            }
            if (!(*tmpbuf3)) {
                PrintError("missing IGNORE TYPE","in IGN",lineno);
                loaderror=1;
                continue;
            }
            for (tmpstr=tmpbuf3;*tmpstr;tmpstr++)
                if (*tmpstr==',') *tmpstr=' ';
            malloc_strcpy(&ign,tmpbuf2);
            malloc_strcat(&ign," ");
            malloc_strcat(&ign,tmpbuf3);
            window_display=0;
            ignore(NULL,ign,NULL);
            window_display=display;
            new_free(&ign);
        }
        else if (!strcmp("EXTMES",tmpbuf3))
            OnOffSet(&pointer,&ExtMes,&loaderror,lineno,"EXTMES");
        else if (!strcmp("NHPROT",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            number=NHProt;
            if (!my_stricmp(tmpbuf3,"ON")) NHProt=1;
            else if (!my_stricmp(tmpbuf3,"OFF")) NHProt=0;
            else loaderror=2;
            if (loaderror!=2) {
                if (NHProt) {
                    NextArg(pointer,&pointer,tmpbuf2);
                    if (*tmpbuf2) chanlist=tmpbuf2;
                    else loaderror=2;
                }
                if (loaderror!=2) {
                    NextArg(pointer,&pointer,tmpbuf3);
                    if (*tmpbuf3) {
                        if (!my_stricmp(tmpbuf3,"QUIET")) NHDisp=0;
                        else if (!my_stricmp(tmpbuf3,"MEDIUM")) NHDisp=1;
                        else if (!my_stricmp(tmpbuf3,"FULL")) NHDisp=2;
                        else loaderror=2;
                    }
                    else loaderror=2;
                }
                if (loaderror!=2) malloc_strcpy(&NHProtChannels,chanlist);
            }
            if (loaderror==2) {
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
                loaderror=1;
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
                    loaderror=1;
                }
                NextArg(pointer,&pointer,tmpbuf);
                number=atoi(tmpbuf);
                if (*tmpbuf && is_number(tmpbuf)) CdccQueueLimit=number;
            }
            else if (!strcmp("IDLE",tmpbuf3))
                NumberSet(&pointer,&CdccIdle,&loaderror,lineno,"CDCC IDLE");
            else if (!strcmp("AUTOGET",tmpbuf3))
                OnOffSet(&pointer,&AutoGet,&loaderror,lineno,"CDCC AUTOGET");
            else if (!strcmp("SECURE",tmpbuf3))
                OnOffSet(&pointer,&Security,&loaderror,lineno,"CDCC SECURE");
            else if (!strcmp("PTIME",tmpbuf3))
                NumberSet(&pointer,&PlistTime,&loaderror,lineno,"CDCC PTIME");
            else if (!strcmp("NTIME",tmpbuf3))
                NumberSet(&pointer,&NlistTime,&loaderror,lineno,"CDCC NTIME");
            else if (!strcmp("CHANNELS",tmpbuf3))
                StringSet(pointer,&CdccChannels,&loaderror,lineno,"CDCC CHANNELS");
            else if (!strcmp("LONGSTATUS",tmpbuf3))
                OnOffSet(&pointer,&LongStatus,&loaderror,lineno,"CDCC LONGSTATUS");
            else if (!strcmp("OVERWRITE",tmpbuf3))
                OnOffSet(&pointer,&CdccOverWrite,&loaderror,lineno,"CDCC OVERWRITE");
            else if (!strcmp("STATUS",tmpbuf3))
                OnOffSet(&pointer,&ShowDCCStatus,&loaderror,lineno,"CDCC STATUS");
            else if (!strcmp("STATS",tmpbuf3))
                OnOffSet(&pointer,&CdccStats,&loaderror,lineno,"CDCC STATS");
            else if (!strcmp("VERBOSE",tmpbuf3))
                OnOffSet(&pointer,&CdccVerbose,&loaderror,lineno,"CDCC VERBOSE");
            else if (!strcmp("WARNING",tmpbuf3))
                OnOffSet(&pointer,&DCCWarning,&loaderror,lineno,"CDCC WARNING");
            else if (!strcmp("ULDIR",tmpbuf3))
                DirSet(&pointer,&CdccUlDir,&loaderror,lineno,NULL,"CDCC ULDIR");
            else if (!strcmp("DLDIR",tmpbuf3))
                DirSet(&pointer,&CdccDlDir,&loaderror,lineno,NULL,"CDCC DLDIR");
#ifdef EXTRA_STUFF
            else if (!strcmp("E",tmpbuf3))
                DirSet(&pointer,&EString,&loaderror,lineno,"missing STRING","CDCC E");
            else if (!strcmp("M",tmpbuf3))
                OnOffSet(&pointer,&RenameFiles,&loaderror,lineno,"CDCC M");
#endif
        }
        else if (!strcmp("DEOPSENSOR",tmpbuf3))
            NumberSet(&pointer,&DeopSensor,&loaderror,lineno,"DEOPSENSOR");
        else if (!strcmp("KICKSENSOR",tmpbuf3))
            NumberSet(&pointer,&KickSensor,&loaderror,lineno,"KICKSENSOR");
        else if (!strcmp("NICKSENSOR",tmpbuf3))
            NumberSet(&pointer,&NickSensor,&loaderror,lineno,"NICKSENSOR");
        else if (!strcmp("DEOPTIMER",tmpbuf3))
            NumberSet(&pointer,&MDopTimer,&loaderror,lineno,"DEOPTIMER");
        else if (!strcmp("KICKTIMER",tmpbuf3))
            NumberSet(&pointer,&KickTimer,&loaderror,lineno,"KICKTIMER");
        else if (!strcmp("NICKTIMER",tmpbuf3))
            NumberSet(&pointer,&NickTimer,&loaderror,lineno,"NICKTIMER");
        else if (!strcmp("AUTOAWTIME",tmpbuf3))
            NumberSet(&pointer,&AutoAwayTime,&loaderror,lineno,"AUTOAWTIME");
        else if (!strcmp("IGNORETIME",tmpbuf3))
            NumberSet(&pointer,&IgnoreTime,&loaderror,lineno,"IGNORETIME");
        else if (!strcmp("SHITIGNORETIME",tmpbuf3))
            NumberSet(&pointer,&ShitIgnoreTime,&loaderror,lineno,"SHITIGNORETIME");
#ifdef EXTRAS
        else if (!strcmp("IDLETIME",tmpbuf3))
            NumberSet(&pointer,&IdleTime,&loaderror,lineno,"IDLETIME");
#endif
        else if (!strcmp("NICKWATCH",tmpbuf3))
            ChannelsSet(&pointer,&NickWatch,&NickWatchChannels,&loaderror,lineno,"NICKWATCH",NULL);
        else if (!strcmp("MDOPWATCH",tmpbuf3))
            ChannelsSet(&pointer,&MDopWatch,&MDopWatchChannels,&loaderror,lineno,"MDOPWATCH",NULL);
        else if (!strcmp("KICKWATCH",tmpbuf3))
            ChannelsSet(&pointer,&KickWatch,&KickWatchChannels,&loaderror,lineno,"KICKWATCH",NULL);
        else if (!strcmp("AUTOREJOIN",tmpbuf3))
            ChannelsSet(&pointer,&AutoRejoin,&AutoRejoinChannels,&loaderror,lineno,"AUTOREJOIN",NULL);
        else if (!strcmp("AUTOJOININV",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if ((!my_stricmp(tmpbuf3,"ON")) || (!my_stricmp(tmpbuf3,"AUTO"))) {
                if (!my_stricmp(tmpbuf3,"AUTO")) AutoJoinOnInv=2;
                else AutoJoinOnInv=1;
                NextArg(pointer,&pointer,tmpbuf3);
                if (*tmpbuf3) malloc_strcpy(&AutoJoinChannels,tmpbuf3);
                else {
                    PrintError("must be ON/AUTO CHANLIST","in AUTOJOININV",lineno);
                    loaderror=1;
                }
            }
            else if (!my_stricmp(tmpbuf3,"OFF")) AutoJoinOnInv=0;
            else {
                PrintError("must be OFF","in AUTOJOINONINV",lineno);
                loaderror=1;
            }
        }
#if defined(EXTRAS) || defined(FLIER)
        else if (!strcmp("AUTOINV",tmpbuf3))
            ChannelsSet(&pointer,&AutoInv,&AutoInvChannels,&loaderror,lineno,"AUTOINV",NULL);
#endif
#ifdef ACID
        else if (!strcmp("FORCEJOIN",tmpbuf3))
            ChannelsSet(&pointer,&ForceJoin,&ForceJoinChannels,&loaderror,lineno,"FORCEJOIN",NULL);
#endif
        else if (!strcmp("CHANLOG",tmpbuf3))
            ChannelsSet(&pointer,&ChanLog,&ChanLogChannels,&loaderror,lineno,"CHANLOG",NULL);
        else if (!strcmp("FLOODPROT",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"MAX")) {
                NextArg(pointer,&pointer,tmpbuf3);
                if (!is_number(tmpbuf3) || (FloodMessages=atoi(tmpbuf3))<0) loaderror=3;
                NextArg(pointer,&pointer,tmpbuf3);
                if (!is_number(tmpbuf3) || (FloodSeconds=atoi(tmpbuf3))<0) loaderror=3;
                if (loaderror!=3) FloodProt=2;
                else {
                    PrintError("must be ON/OFF or MAX #messages #seconds","in FLOODPROT",lineno);
                    loaderror=1;
                }
            }
            else if (!my_stricmp(tmpbuf3,"ON")) FloodProt=1;
            else if (!my_stricmp(tmpbuf3,"OFF")) FloodProt=0;
            else {
                PrintError("must be ON/OFF or MAX #messages #seconds","in FLOODPROT",lineno);
                loaderror=1;
            }
        }
        else if (!strcmp("SERVNOTICE",tmpbuf3))
            OnOffSet(&pointer,&ServerNotice,&loaderror,lineno,"SERVNOTICE");
        else if (!strcmp("CTCPCLOAKING",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"ON")) CTCPCloaking=1;
            else if (!my_stricmp(tmpbuf3,"HIDE")) CTCPCloaking=2;
            else if (!my_stricmp(tmpbuf3,"OFF")) CTCPCloaking=0;
            else {
                PrintError("must be ON/OFF/HIDE","in CTCPCLOAKING",lineno);
                loaderror=1;
            }
        }
        else if (!strcmp("SHOWFAKES",tmpbuf3))
            ChannelsSet(&pointer,&ShowFakes,&ShowFakesChannels,&loaderror,lineno,"SHOWFAKES",NULL);
        else if (!strcmp("SHOWAWAY",tmpbuf3))
            ChannelsSet(&pointer,&ShowAway,&ShowAwayChannels,&loaderror,lineno,"SHOWAWAY",NULL);
        else if (!strcmp("DEFSERVER",tmpbuf3))
            StringSet(pointer,&DefaultServer,&loaderror,lineno,"DEFSERVER");
        else if (!strcmp("DEFSIGNOFF",tmpbuf3))
            StringSet(pointer,&DefaultSignOff,&loaderror,lineno,"DEFSIGNOFF");
        else if (!strcmp("DEFSETAWAY",tmpbuf3))
            StringSet(pointer,&DefaultSetAway,&loaderror,lineno,"DEFSETAWAY");
        else if (!strcmp("DEFSETBACK",tmpbuf3))
            StringSet(pointer,&DefaultSetBack,&loaderror,lineno,"DEFSETBACK");
        else if (!strcmp("DEFUSERINFO",tmpbuf3))
            StringSet(pointer,&DefaultUserinfo,&loaderror,lineno,"DEFUSERINFO");
        else if (!strcmp("DEFFINGER",tmpbuf3))
            StringSet(pointer,&DefaultFinger,&loaderror,lineno,"DEFFINGER");
        else if (!strcmp("AUTOOPTIME",tmpbuf3))
            NumberSet(&pointer,&AutoOpDelay,&loaderror,lineno,"AUTOOPTIME");
        else if (!strcmp("KICKOPS",tmpbuf3))
            ChannelsSet(&pointer,&KickOps,&KickOpsChannels,&loaderror,lineno,"KICKOPS",NULL);
        else if (!strcmp("KICKONFLOOD",tmpbuf3))
            ChannelsSet(&pointer,&KickOnFlood,&KickOnFloodChannels,&loaderror,lineno,"KICKONFLOOD",NULL);
        else if (!strcmp("SHOWNICK",tmpbuf3))
            OnOffSet(&pointer,&ShowNick,&loaderror,lineno,"SHOWNICK");
        else if (!strcmp("KICKONBAN",tmpbuf3))
            ChannelsSet(&pointer,&KickOnBan,&KickOnBanChannels,&loaderror,lineno,"KICKONBAN",NULL);
        else if (!strcmp("AWAYSAVE",tmpbuf3))
            NumberSet(&pointer,&AwaySaveSet,&loaderror,lineno,"AWAYSAVE");
        else if (!strcmp("SHOWWALLOP",tmpbuf3))
            OnOffSet(&pointer,&ShowWallop,&loaderror,lineno,"SHOWWALLOP");
        else if (!strcmp("AUTOREPLY",tmpbuf3))
            StringSet(pointer,&AutoReplyBuffer,&loaderror,lineno,"AUTOREPLY");
        else if (!strcmp("ORIGNICK",tmpbuf3))
            ChannelsSet(&pointer,&OrigNickChange,&OrigNick,&loaderror,lineno,"ORIGNICK","must be ON/QUIET NICK");
        else if (!strcmp("NOTIFYMODE",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"BRIEF")) NotifyMode=1;
            else if (!my_stricmp(tmpbuf3,"VERBOSE")) NotifyMode=2;
            else {
                PrintError("must be Brief/Verbose","in NOTIFYMODE",lineno);
                loaderror=1;
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
                loaderror=1;
            }
        }
        else if (!strcmp("EGO",tmpbuf3))
            OnOffSet(&pointer,&Ego,&loaderror,lineno,"EGO");
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
                loaderror=1;
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
                    loaderror=1;
                }
            }
            else if (!my_stricmp(tmpbuf3,"OFF")) IdleKick=0;
            else {
                PrintError("must be ON/AUTO channels/OFF","in IDLEKICK",lineno);
                loaderror=1;
            }
        }
#endif
        else if (!strcmp("DEFK",tmpbuf3))
            StringSet(pointer,&DefaultK,&loaderror,lineno,"DEFK");
        else if (!strcmp("DEFBK",tmpbuf3))
            StringSet(pointer,&DefaultBK,&loaderror,lineno,"DEFBK");
        else if (!strcmp("DEFBKI",tmpbuf3))
            StringSet(pointer,&DefaultBKI,&loaderror,lineno,"DEFBKI");
        else if (!strcmp("DEFBKT",tmpbuf3))
            StringSet(pointer,&DefaultBKT,&loaderror,lineno,"DEFBKT");
        else if (!strcmp("DEFFK",tmpbuf3))
            StringSet(pointer,&DefaultFK,&loaderror,lineno,"DEFFK");
        else if (!strcmp("DEFLK",tmpbuf3))
            StringSet(pointer,&DefaultLK,&loaderror,lineno,"DEFLK");
        else if (!strcmp("DEFABK",tmpbuf3))
            StringSet(pointer,&DefaultABK,&loaderror,lineno,"DEFABK");
        else if (!strcmp("DEFSK",tmpbuf3))
            StringSet(pointer,&DefaultSK,&loaderror,lineno,"DEFSK");
#ifdef OPER
	else if (!strcmp("DEFKILL",tmpbuf3))
	    StringSet(pointer,&DefaultKill,&loaderror,lineno,"DEFKILL");
#endif
        else if (!strcmp("USERMODE",tmpbuf3))
            StringSet(pointer,&PermUserMode,&loaderror,lineno,"USERMODE");
        else if (!strcmp("BITCHMODE",tmpbuf3))
            ChannelsSet(&pointer,&Bitch,&BitchChannels,&loaderror,lineno,"BITCHMODE",NULL);
        else if (!strcmp("FRIENDLIST",tmpbuf3))
            ChannelsSet(&pointer,&FriendList,&FriendListChannels,&loaderror,lineno,"FRIENDLIST",NULL);
        else if (!strcmp("COMPRESSMODES",tmpbuf3))
            ChannelsSet(&pointer,&CompressModes,&CompressModesChannels,&loaderror,lineno,"COMPRESSMODES",NULL);
#ifdef CELE
        else if (!strcmp("TRUNCATE",tmpbuf3)) {
            set_int_var(TRUNCATE_PUBLIC_CHANNEL_VAR,atoi(pointer));
        }
        else if (!strcmp("SCROLLZSTR",tmpbuf3)) {
            StringSet(pointer,&ScrollZstr,&loaderror,lineno,"SCROLLZSTR");
            set_string_var(SCROLLZ_STRING_VAR,ScrollZstr);
        }
#endif
#ifdef EXTRAS
        else if (!strcmp("SIGNOFFCHANNELS",tmpbuf3))
            ChannelsSet(&pointer,&ShowSignoffChan,&SignoffChannels,&loaderror,lineno,"SIGNOFFCHANNELS",NULL);
#endif
        else if (!strcmp("STAMP",tmpbuf3))
            OnOffSet(&pointer,&Stamp,&loaderror,lineno,"STAMP");
        else if (!strcmp("BANKICKLIST",tmpbuf3))
            ChannelsSet(&pointer,&BKList,&BKChannels,&loaderror,lineno,"BANKICKLIST",NULL);
        else if (!strcmp("NOTIFYSTR",tmpbuf3)) {
            StringSet(pointer,&CelerityNtfy,&loaderror,lineno,"NOTIFYSTR");
            set_string_var(NOTIFY_STRING_VAR,CelerityNtfy);
        }
        else if (!strcmp("CHANLOGDIR",tmpbuf3))
            StringSet(pointer,&ChanLogDir,&loaderror,lineno,"CHANLOGDIR");
        else if (!strcmp("CHANLOGPREFIX",tmpbuf3))
            StringSet(pointer,&ChanLogPrefix,&loaderror,lineno,"CHANLOGPREFIX");
        else if (!strcmp("CHANLOGPOST",tmpbuf3))
            StringSet(pointer,&ChanLogPostfix,&loaderror,lineno,"CHANLOGPOST");
        else if (!strcmp("ORIGNICKTIME",tmpbuf3))
            NumberSet(&pointer,&OrigNickDelay,&loaderror,lineno,"ORIGNICKTIME");
        else if (!strcmp("LOGON",tmpbuf3))
            OnOffSet(&pointer,&LogOn,&loaderror,lineno,"LOGON");
        else if (!strcmp("ARINWINDOW",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            if (!my_stricmp(tmpbuf3,"ON")) ARinWindow=1;
            else if (!my_stricmp(tmpbuf3,"USER")) ARinWindow=2;
            else if (!my_stricmp(tmpbuf3,"BOTH")) ARinWindow=3;
            else if (!my_stricmp(tmpbuf3,"OFF")) ARinWindow=0;
            else {
                PrintError("must be ON/OFF/USER/BOTH","in ARINWINDOW",lineno);
                loaderror=1;
            }
        }
#ifdef EXTRAS
        else if (!strcmp("SIGNOFFALLCHAN",tmpbuf3))
            OnOffSet(&pointer,&ShowSignAllChan,&loaderror,lineno,"SIGNOFFALLCHAN");
        else if (!strcmp("NICKCHGALLCHAN",tmpbuf3))
            OnOffSet(&pointer,&ShowNickAllChan,&loaderror,lineno,"NICKCHGALLCHAN");
#endif
        else if (!strcmp("EXTPUB",tmpbuf3))
            OnOffSet(&pointer,&ExtPub,&loaderror,lineno,"EXTPUB");
        else if (!strcmp("AWAYENCRYPT",tmpbuf3))
            OnOffSet(&pointer,&AwayEncrypt,&loaderror,lineno,"AWAYENCRYPT");
        else if (!strcmp("ETOPICDELIM",tmpbuf3))
            StringSet(pointer,&ExtTopicDelimiter,&loaderror,lineno,"ETOPICDELIM");
#ifdef WANTANSI
        else if (!strcmp("MIRCCOLORS",tmpbuf3))
            OnOffSet(&pointer,&DisplaymIRC,&loaderror,lineno,"MIRCCOLORS");
        else if (!strcmp("COLOR",tmpbuf3)) {
            NextArg(pointer,&pointer,tmpbuf3);
            upper(tmpbuf3);
            if (!strcmp(tmpbuf3,"WARNING")) SetColors(COLWARNING,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"JOIN")) SetColors(COLJOIN,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"MSG")) SetColors(COLMSG,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"NOTICE")) SetColors(COLNOTICE,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"NETSPLIT")) SetColors(COLNETSPLIT,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"INVITE")) SetColors(COLINVITE,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"MODE")) SetColors(COLMODE,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"SETTING")) SetColors(COLSETTING,&pointer,&loaderror,lineno);
            /* XXX: don't report error for HELP - just ignore it (this has to be removed one day) */
            else if (!strcmp(tmpbuf3,"HELP")) ;
            else if (!strcmp(tmpbuf3,"LEAVE")) SetColors(COLLEAVE,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"NOTIFY")) SetColors(COLNOTIFY,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"CTCP")) SetColors(COLCTCP,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"KICK")) SetColors(COLKICK,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"DCC")) SetColors(COLDCC,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"WHO")) SetColors(COLWHO,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"WHOIS")) SetColors(COLWHOIS,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"PUBLIC")) SetColors(COLPUBLIC,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"CDCC")) SetColors(COLCDCC,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"LINKS")) SetColors(COLLINKS,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"DCCCHAT")) SetColors(COLDCCCHAT,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"CSCAN")) SetColors(COLCSCAN,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"NICK")) SetColors(COLNICK,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"ME")) SetColors(COLME,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"MISC")) SetColors(COLMISC,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"SBAR")) SetColors(COLSBAR1,&pointer,&loaderror,lineno);
            else if (!strcmp(tmpbuf3,"SBAR2")) SetColors(COLSBAR2,&pointer,&loaderror,lineno);
#ifdef CELECOSM
	    else if (!strcmp(tmpbuf3,"CELE")) SetColors(COLCELE,&pointer,&loaderror,lineno);
#endif
#ifdef OPERVISION
            else if (!strcmp(tmpbuf3,"OV")) SetColors(COLOV,&pointer,&loaderror,lineno);
#endif
            else {
                PrintError("missing or wrong CODE","in COLOR",lineno);
                loaderror=1;
            }
        }
#endif
        else if (*tmpbuf3!='#') PrintError("unknown command",tmpbuf3,lineno);
    }
    fclose(usfile);
    if (loaderror) say("There were errors in ScrollZ.save");
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
    }
    if (PermUserMode) send_to_server("MODE %s %s",get_server_nickname(from_server),
                                     PermUserMode);
    return;
}

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
void CleanUpLists() {
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

    inSZNotify=1;
    strcpy(tmpbuf,"-");
    notify(NULL,tmpbuf,NULL);
    inSZNotify=0;
    CleanUpLists();
    ScrollZLoad();
    for (i=0;i<number_of_servers;i++)
        for (tmpchan=server_list[i].chan_list;tmpchan;tmpchan=tmpchan->next) {
            for (tmp=tmpchan->nicks;tmp;tmp=tmp->next) {
                if (tmp->userhost) snprintf(tmpbuf,sizeof(tmpbuf),"%s!%s",tmp->nick,tmp->userhost);
                else strmcpy(tmpbuf,tmp->nick,sizeof(tmpbuf));
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
            tmpchan->BKList=BKList?CheckChannel(tmpchan->channel,BKChannels):0;
            tmpchan->ChanLog=ChanLog?CheckChannel(tmpchan->channel,ChanLogChannels):0;
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
        whowas->channellist->BKList=
            BKList?CheckChannel(whowas->channellist->channel,BKChannels):0;
        whowas->channellist->ChanLog=
            ChanLog?CheckChannel(whowas->channellist->channel,ChanLogChannels):0;
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
    encrlist=NULL;
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
    malloc_strcpy(&ScrollZstr,"[1m[30m·[0m[37m·[1m[37m·[0m[37m");
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
#ifdef OPER
    malloc_strcpy(&DefaultKill,"Go play somewhere else...");
#endif
    malloc_strcpy(&ShowFakesChannels,"*");
    malloc_strcpy(&FriendListChannels,"*");
    malloc_strcpy(&BKChannels,"*");
    malloc_strcpy(&AutoReplyString,": ");
    malloc_strcpy(&CelerityNtfy,ScrollZstr);
    malloc_strcpy(&ExtTopicDelimiter,"|");
    set_string_var(AWAY_FILE_VAR,DEFAULT_AWAY_FILE);
    if (!AutoReplyBuffer) {
        i=strlen(nickname)>3?3:strlen(nickname);
        AutoReplyBuffer=(char *) new_malloc(i+1);
        strmcpy(AutoReplyBuffer,nickname,i);
    }
    defban='B';
    inSZNotify=0;
    inSZLinks=0;
    inSZFKill=0;
    inSZTrace=0;
    ExtMes=1;
    NHProt=0;
    NHDisp=2;
    AutoGet=0;
    DeopSensor=4;
    KickSensor=4;
    NickSensor=4;
    AutoAwayTime=10;
    NickWatch=0;
    MDopWatch=0;
    KickWatch=0;
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
    ShowAway=0;
#if defined(CELE)
    LagTimer.tv_sec=0;
    LagTimer.tv_usec=0;
#else
    LagTimer=0;
#endif
    KickOps=0;
    KickOnFlood=0;
    KickOnBan=0;
#ifdef SCKICKS
    NumberOfScatterKicks=0;
#endif
#ifndef CELE
    NumberOfSignOffMsgs=0;
#endif
    ShowNick=1;
    PlistTime=7200;
    NlistTime=7200;
    LastCheck=time((time_t *) 0);
    LastPlist=LastCheck+360000L;
    LastNlist=LastCheck+360000L;
    LastServer=LastCheck+120L;
    LastNick=LastCheck;
    LastLinks=LastCheck;
    LinksNumber=0;
    AwaySaveSet=SAVEALL&(~SAVENOTIFY);
    ShowWallop=1;
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
#ifdef ACID
    ForceJoin=0;
#endif
    DCCWarning=1;
    Stamp=0;
    BKList=1;
    CdccVerbose=1;
    ARinWindow=0;
    OrigNickQuiet=0;
    OrigNickSent=0;
    OrigNickNumber=0;
#ifdef EXTRAS
    ShowSignAllChan=0;
    ShowNickAllChan=0;
#endif
    ExtPub=0;
    ChanLog=0;
    AwayEncrypt=0;
    usersloaded=0;
    strmcpy(tmpbuf,ScrollZver,sizeof(tmpbuf));
    for (i=0,tmpstr1=tmpbuf;i<2;tmpstr1++) if (*tmpstr1==' ') i++;
    for (i=0,tmpstr2=tmpstr1;i<1;tmpstr2++) if (*tmpstr2==' ') i++;
    tmpstr2--;
    *tmpstr2='\0';
    malloc_strcpy(&ScrollZver1,tmpstr1);
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
    char *message=server_list[from_server].LastMessage;
    ChannelList *chan;
    NickList *tmp;

    if (!(*args)) PrintUsage("DIRLMK nick");
    else {
        channel=get_channel_by_refnum(0);
        if (channel) {
            chan=lookup_channel(channel,curr_scr_win->server,0);
            if (chan && HAS_OPS(chan->status)) {
                tmpnick=new_next_arg(args,&args);
                tmp=find_in_hash(chan,tmpnick);
                if (tmp) {
#ifdef CELE
                    if (message) send_to_server("KICK %s %s :%s %s",channel,tmpnick,
                                                message,CelerityL);
                    else send_to_server("KICK %s %s :%s %s",channel,tmpnick,
                                        DefaultK,CelerityL);
#else  /* CELE */
                    if (message) send_to_server("KICK %s %s :%s",channel,tmpnick,message);
                    else send_to_server("KICK %s %s :%s",channel,tmpnick,DefaultK);
#endif /* CELE */
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
        if (chan && HAS_OPS(chan->status)) {
            count=0;
            found=0;
            for (tmp=chan->nicks;tmp;tmp=tmp->next)
                if (!(tmp->chanop || tmp->halfop || tmp->hasvoice)) count++;
            if (count) {
                srand(time(0));
                random=rand()%count+1;
                for (tmp=chan->nicks;tmp;tmp=tmp->next) {
                    if (!(tmp->chanop || tmp->halfop || tmp->hasvoice)) found++;
                    if (found==random) break;
                }
                if (tmp) {
#ifdef CELE
                    send_to_server("KICK %s %s :%s %s",channel,tmp->nick,comment,
                                   CelerityL);
#else  /* CELE */
                    send_to_server("KICK %s %s :%s",channel,tmp->nick,comment);
#endif /* CELE */
                }
            }
            else say("No lamers (?) in %s",channel);
        }
        else NotChanOp(channel);
    }
    else NoWindowChannel();
}
#endif
