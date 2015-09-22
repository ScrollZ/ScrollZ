/******************************************************************************
 Functions coded by Flier (THANX to Sheik!!)

 FindMatch           Returns pointer to friends list with matching entry
 HandleNickCollision Handles nick collisions
 TimeReply           Handles reply number 333 from server
 TagNick             Tags nick for nick watcher
 WhereIs             Try to dig out users new nick
 WhereList           This will list all users on nick watcher list
 UnFlash             Unflashes your terminal
 vt100Decode         Checks if character is printable
 FixColorAnsi        Checks if ansi string only sets colors/attributes and if
                     not it changes escape character (0x1B) to space
 CountAnsi           How many ansi chars are there in string
 ColorName           Returns the name of the color
 GetColors           Decodes colors and returns string for save
 SaveColors          Saves color settings to ScrollZ.save
 BuildColor          Parses color string and puts color codes in target string
 SetColors           Sets colors loaded from ScrollZ.save
 PrintSetting        Prints setting
 StripAnsi           Strips out ANSI codes for logging
 StripCrap           Strips out non printable codes for logging
 SplitPrint          Prints netsplit stuff
 ModePrint           Prints mode stuff
 KickPrint           Prints kick stuff
 Password            Sets password for log file encryption
 PrintWho            Prints who reply
 PrintWhoIsUser      Prints whois user reply
 PrintWhoIsChannels  Prints whois channel reply
 PrintWhoIsServers   Prints whois server reply
 PrintPublic         Prints public messages
 OpenCreateFile      Opens file in path and creates one if necessary
 PrintLinks          Prints coloured links
 PlayBack            Shows messages from ScrollZ.away w/o setting you back
 AwaySaveToggle      Defines what to save to ScrollZ.away
 StatsKFilter        Does /STATS k with filter
 HandleStatsK        Handles STATS k reply from server
 CurrentChaneMode    Sets mode for your current channel
 Nslookup            Looks up DNS entry
 DoNslookup          This does the real DNS look-up
 InsertAutoReply     Inserts nickname of person that triggered auto reply
 Dump                Dumps variables, aliases, ons or all of them
 AddNick2AutoReply   Adds nick to auto reply nick
 RemoveLog           Deletes ScrollZ.away file
 AutoChangeNick      Automatically changes nick
 PrintChatMsg        Colored chat
 PrintMyChatMsg      Prints my chat messages
 MSay                Outputs line to all channels
 CreateBan           Creates ban according to ban type
 Terminate           Quits with 152 errorlevel
 PrintSynch          Checks whether join is synched
 DirLSM              Redirects last message/notice you've sent
 ShowUser            Like internal WHO
 SetColor            Sets colors interactively
 NotifyModeToggle    Toggles notify mode brief/verbose
 PingMe              Pings yourself
 NotePad             Lets you write down into a file
 URLCatchToggle      Toggles URL catcher on/off
 PrintLinksHead      Prints head of /LINKS
 CountAnsiInput      How many ansi chars are there in input line
 SortedCmp           Used to insert into sorted linked list
 LastJoinerKick      BanKicks the last person to join
 URLSave             Saves URLs to notepad (coded by Zakath)
 AcceptLastChat      Establishes dcc chat with user that last requested chat
 GrabURL             This bolds out URLs and copies them to buffer
 LogOnToggle         Toggles whether to save to ScrollZ.away if not away
 PrintNames          Prints names on channel join
 InitKeysColors      Initializes keys and default color scheme
 MultiKick           Kicks nick from multiple channel
 WhoKill             Kills users matching filter with WHO
 DoKill              Does the actual killing
 HandleEndOfKill     Reports statistics
 BuildColorNew       Returns terminal sequence to set given color
 InitColor           Builds color strings for default theme
******************************************************************************/

/*
 * $Id: edit5.c,v 1.126 2009-12-21 14:39:21 f Exp $
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
#include "list.h"
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
#include "struct.h"
#include "parse.h"
#include "myvars.h" 
#include "whowas.h"

#include <sys/stat.h> /* for umask() */
#include <term.h>     /* for tparm() */

void PlayBack2 _((char *, char *));
void AddNick2AutoReply _((char *));
void DoNslookup _((char *));
void NotePad _((char *, char *, char *));
char *OpenCreateFile _((char *, int));
void PrintLinksHead _((void));
void URLSave _((char *, char *, char *)); /* /URL coded by Zakath */
void URLSave2 _((char *, char *));
void URLSave3 _((char *, char *));
int  GrabURL _((char *, char *, size_t, char *, char *, char *));
void MasterPassword _((char *, char *));
void MasterPasswordOld _((char *, char *));

extern int  CheckChannel _((char *, char *));
extern void AwaySave _((char *, int));
extern void NextArg _((char *, char **, char *));
extern void PrintError _((char *, char *, int));
extern void DumpAliases _((int));
extern void DumpOn _((void));
extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern int  readln _((FILE *, char *));
extern void UserDomainList _((char *));
#ifdef MGS
extern void MyQuit _((char *));
#endif
extern void HandleGotOps _((char *, ChannelList *));
extern void AddNick2List _((char *, int));
extern struct friends *CheckUsers _((char *, char *));
extern void BanKick _((char *, char *, char *));
extern void BuildPrivs _((struct friends *, char *));
extern void SetBackDelete _((char *, char *));
extern void Check4WordKick _((char *, NickList *, int, ChannelList *));
extern void NotChanOp _((char *));
extern void NoWindowChannel _((void));
extern void PrintUsage _((char *));
extern void ColorUserHost _((char *, char *, char *, int));
extern NickList *find_in_hash _((ChannelList *, char *));
extern int  AddLast _((List *, List *)); /* needed for GrabURL, by Zakath */
extern int  CheckServer _((int));
extern char *TimeStamp _((int));
extern void ChannelLogSave _((char *, ChannelList *));
extern int  EncryptString _((char *, char *, char *, int, int, int));
extern int  DecryptString _((char *, char *, char *, int, int));
extern int  RateLimitJoin _((int));

#ifdef CELE
/*extern void Cstatusupd _((int, int));
extern int  Cstatusnum;*/
#endif /* CELE */
#ifdef CELECOSM
struct friends *whoisfriend;
#endif

#ifdef COUNTRY
extern char *function_country _((char *));
#endif

extern void dcc_chat _((char *));
extern void reset_nickname _((char *, char **));
extern void nickname_sendline _((char *, char *));
extern char *function_match _((char *));

/* Needed for URL, by Zakath */
int URLnum=0;
static char *urlbuf=(char *) 0;

#ifdef OPER
int StatskNumber;
static int  WhoKillNum;
static char *wkillpattern=(char *) 0;
static char *wkillreason=(char *) 0;
#endif
static char *playpattern=(char *) 0;
static int  filesize;
static int  DontHold;
static int  PlayReverse;
static FILE *awayfile=NULL;
#ifdef ACID
static int listcount=0;
#endif

/* Returns pointer to friends list with matching entry */
struct friends *FindMatch(userhost,channel)
char *userhost;
char *channel;
{
    struct friends *tmpfriend;

    for (tmpfriend=frlist;tmpfriend;tmpfriend=tmpfriend->next)
        if ((wild_match(tmpfriend->userhost,userhost) ||
             wild_match(userhost,tmpfriend->userhost)) &&
            CheckChannel(channel,tmpfriend->channels))
            return(tmpfriend);
    return(NULL);
}

/* Handles nick collision */
void HandleNickCollision()
{
    if (away_set || LogOn) AwaySave("Nick collision",SAVECOLL);
}

/* Handles reply number 333 from server */
void TimeReply(from,ArgList)
char *from;
char **ArgList;
{
    char *channel;
    ChannelList *chan;

    if (ArgList[1] && is_channel(ArgList[0]) && ArgList[2]) {
        channel = ArgList[0];
        if ((chan = lookup_channel(channel, parsing_server_index, 0))) {
            chan->topicwhen = atoi(ArgList[2]);
            malloc_strcpy(&(chan->topicwho), ArgList[1]);
            save_message_from();
            message_from(channel, LOG_CRAP);
            put_it("%sSet by %s on %.24s", numeric_banner(), chan->topicwho,
                   ctime(&(chan->topicwhen)));
            restore_message_from();
            if (chan->ChanLog) {
                char tmpbuf[mybufsize];

                snprintf(tmpbuf, sizeof(tmpbuf), "Set by %s on %.24s", chan->topicwho, ctime(&(chan->topicwhen)));
                ChannelLogSave(tmpbuf, chan);
            }
        }
    }
}

#ifdef ACID
/* Just a declaration , needed to shut up the compiler */
void TagNickNew();

/* Tags nick for nick watcher */
void TagNick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;
    void (*func)();

    if (*args) {
        tmpnick=new_next_arg(args,&args);
        func=(void(*)())TagNickNew;
        server_list[from_server].SZWI++;
        add_to_whois_queue(tmpnick,func,"%s",tmpnick);
    }
    else PrintUsage("TAG nick");
}

/* This stores user to nick watch list */
void TagNickNew(wistuff,tmpnick,text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    char tmpbuf1[mybufsize/4];
    struct list *tmp;

    if (server_list[from_server].SZWI) server_list[from_server].SZWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s@%s",wistuff->user,wistuff->server);
    tmp=nickwatchlist;
    while (tmp && tmp->next) tmp=tmp->next;
    tmpnickwatch=(struct list *) new_malloc(sizeof(struct list));
    tmpnickwatch->nick=NULL;
    tmpnickwatch->userhost=NULL;
    malloc_strcpy(&(tmpnickwatch->nick),wistuff->nick);
    malloc_strcpy(&(tmpnickwatch->userhost),tmpbuf1);
    tmpnickwatch->next=NULL;
    if (nickwatchlist) tmp->next=tmpnickwatch;
    else nickwatchlist=tmpnickwatch;
    say("%s added to nick watcher",tmpnickwatch->nick);
}

/* This will dig out users new nickname */
void WhereIs(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpnick;
    int  found=0;

    if (*args) {
        tmpnick=new_next_arg(args,&args);
        tmpnickwatch=nickwatchlist;
        while (tmpnickwatch && !found) { 
            if (!my_stricmp(tmpnick,tmpnickwatch->nick)) found=1;
            else tmpnickwatch=tmpnickwatch->next;
        }
        if (found) {
            say("You will get VERSION reply from person under their new nick");
            say("if they didn't change their server or username. This will fail");
            say("if two users share the same username on target server.");
            send_to_server("PRIVMSG %s :%cVERSION%c",tmpnickwatch->userhost,CTCP_DELIM_CHAR,CTCP_DELIM_CHAR);
        }
        else say("%s is not on nick watcher list, use /TAG to add user",tmpnick);
    }
    else PrintUsage("WHEREIS nick");
}

/* Just a declaration , needed to shut up the compiler */
void WhereListPage();

/* This will list all users on nick watcher list */
void WhereList(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    listcount=0;
    tmpnickwatch=nickwatchlist;
    say("The following people are on nick watch list :");
    WhereListPage(command);
}

/* Just a declaration , needed to shut up the compiler */
void WhereListKey();

/* Lists one page of nick watch list */
void WhereListPage(line)
char *line;
{
    int  count=3;

    while (count<curr_scr_win->display_size && tmpnickwatch!=NULL) {
        count++;
        listcount++;
        say("#%-2d %-12s %s",listcount,tmpnickwatch->nick,tmpnickwatch->userhost);
        tmpnickwatch=tmpnickwatch->next;
    }
    if (!tmpnickwatch) WhereListKey(line,"E");
    else if (count>=curr_scr_win->display_size) add_wait_prompt("Press any key to continue, 'q' to quit",WhereListKey,line,WAIT_PROMPT_KEY);
}

/* This waits for key press */
void WhereListKey(stuff,line)
char *stuff;
char *line;
{
    if (line && (*line=='q' || *line=='Q' || *line=='E')) {
        if (*line=='E') say("Total of %d people on nick watch list",listcount);
        return;
    }
    else WhereListPage(stuff);
}
#endif

/* Unflashes your terminal */
void UnFlash(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    fwrite("\033c",2,1,stdout);
    refresh_screen(0,(char *) 0);
}

#ifdef WANTANSI
/* Checks if character is printable */
int vt100Decode(chr)
register char chr;
{
    static enum {
        Normal, Escape, SCS, CSI, DCS, DCSData, DCSEscape, DCS2, DCS3, DCS4
    } vtstate = Normal;
    int iso2022 = get_int_var(ISO2022_SUPPORT_VAR);

    if (chr == 0x1B)
        if (vtstate == DCSData || vtstate == DCSEscape) vtstate = DCSEscape;
        else {
            vtstate = Escape;
            return(1);
        }
    else if (chr == 0x18 || chr == 0x1A) vtstate = Normal;
    else if (chr == 0xE || chr == 0xF) ;
    else if (chr < 0x20) return(0);
    switch (vtstate) {
    case Normal:
        return 0;
        break;
    case Escape:
        switch (chr) {
        case '[':
            vtstate = CSI;
            break;
        case 'P':
            vtstate = DCS;
            break;
        case '(':
        case ')':
            if (iso2022 && chr == '(') {
                vtstate = DCS2;
                return(2);
            }
            else vtstate = SCS;
            break;
        case '$':
            if (iso2022) {
                vtstate = DCS3;
                return(2);
            }
            else {
                vtstate = Normal;
                return(0);
            }
            break;
        default:
            vtstate = Normal;
        }
        return(1);
        break;
    case SCS:
        vtstate = Normal;
        break;
    case CSI:
        if (isalpha(chr)) vtstate = Normal;
        break;
    case DCS:
        if (chr >= 0x40 && chr <= 0x7E) vtstate = DCSData;
        break;
    case DCSData:
        break;
    case DCSEscape:
        vtstate = Normal;
        break;
    case DCS2:
        vtstate = Normal;
        if (chr == 'J' || chr == 'I' || chr == 'B') return(2);
        else return(0);
        break;
    case DCS3:
        if (chr == '(') {
            vtstate = DCS4;
            return(2);
        }
        vtstate = Normal;
        if (chr == '@' || chr == 'B') return(2);
        else return(0);
        break;
    case DCS4:
        vtstate = Normal;
        if (chr == 'D' || chr == 'O' || chr == 'P' || chr == 'Q') return(2);
        else return(0);
        break;
    }
    return(1);
}

/* Checks if ansi string only sets colors/attributes */
void FixColorAnsi(str)
char *str;
{
    int  val = 0;
    int  what = 0;
    int  numbers = 0;
    int  iso2022 = get_int_var(ISO2022_SUPPORT_VAR);
    char *tmpstr;
    char *tmpstr1 = NULL;

    for (tmpstr = str; *tmpstr; tmpstr++) {
        if ((*tmpstr >= '0' && *tmpstr <= '9')) {
            numbers = 1;
            val = val * 10 + (*tmpstr - '0');
        }
        else if (*tmpstr == ';') {
            numbers = 1;
            val = 0;
        }
        /*else if (!(*tmpstr=='m' || *tmpstr=='C')) numbers=val=0;*/
        else if (!(*tmpstr == 'm')) numbers = val = 0;
        if (*tmpstr == 0x1B) {
            if (what && tmpstr1) *tmpstr1 += 64;
            what = 1;
            tmpstr1 = tmpstr;
            numbers = val = 0;
        }
        /*if (what && (*tmpstr=='m' || *tmpstr=='C')) {*/
        if (what && *tmpstr == 'm') {
            if (!numbers || val == 12) {
                *tmpstr1 += 64;
                tmpstr1 = tmpstr;
            }
            what = 0;
            numbers = val = 0;
        }
        else if (what == 1 && *tmpstr == '(') what = 2;
        else if (what == 2 && *tmpstr == 'U') what = 0;
        /* ISO-2022-JP */
        else if (what == 2 && iso2022 && *tmpstr == 'B') what = 0;
        else if (what == 2 && iso2022 && *tmpstr == 'J') what = 0;
        else if (what == 2 && iso2022 && *tmpstr == 'I') what = 0;
        else if (what == 1 && iso2022 && *tmpstr == '$') what = 3;
        else if (what == 3 && iso2022 && *tmpstr == '@') what = 0;
        else if (what == 3 && iso2022 && *tmpstr == 'B') what = 0;
        else if (what == 3 && iso2022 && *tmpstr == '(') what = 4;
        else if (what == 4 && iso2022 && *tmpstr == 'D') what = 0;
        else if (what == 4 && iso2022 && *tmpstr == 'O') what = 0;
        else if (what == 4 && iso2022 && *tmpstr == 'P') what = 0;
        else if (what == 4 && iso2022 && *tmpstr == 'Q') what = 0;
        else if (what && isalpha(*tmpstr)) {
            *tmpstr1 += 64;
            what = 0;
        }
    }
    if (what && tmpstr1) *tmpstr1 += 64;
}

/* Counts ansi chars in string */
int CountAnsi(str,len)
char *str;
int len;
{
    register int  count=0;
    register int  x=0;
    register char *ptr;

    if (!str) return(0);
    FixColorAnsi(str);
    if (len==-1) {
        for (ptr=str;*ptr;ptr++)
            if (vt100Decode(*ptr)) count++;
    }
    else {
        for (ptr=str;*ptr && x<len;ptr++,x++)
            if (vt100Decode(*ptr)) count++;
    }
    return(count);
}

/* Returns the name of the color */
char *ColorName(number)
int number;
{
    switch (number) {
        case COLBOLD      :return("BOLD");
        case COLUNDERLINE :return("UNDERLINE");
        case COLFLASH     :return("FLASH");
        case COLREV       :return("REVERSE");
        case COLBLACK     :return("BLACK");
        case COLRED       :return("RED");
        case COLGREEN     :return("GREEN");
        case COLYELLOW    :return("YELLOW");
        case COLBLUE      :return("BLUE");
        case COLPURPLE    :return("PURPLE");
        case COLCYAN      :return("CYAN");
        case COLWHITE     :return("WHITE");
        case COLBLACKBG   :return("BLACKBG");
        case COLREDBG     :return("REDBG");
        case COLGREENBG   :return("GREENBG");
        case COLYELLOWBG  :return("YELLOWBG");
        case COLBLUEBG    :return("BLUEBG");
        case COLPURPLEBG  :return("PURPLEBG");
        case COLCYANBG    :return("CYANBG");
        case COLWHITEBG   :return("WHITEBG");
        case COLNOBOLD    :return("NOBOLD");
    }
    return("WHITE");
}

/* Decodes colors and returns string for save */
void GetColors(number, buffer)
int  number;
char *buffer;
{
    int i;

    *buffer = '\0';
    if (CmdsColors[number].color1_str) strcat(buffer, CmdsColors[number].color1_str);
    else strcat(buffer, "WHITE");
    strcat(buffer,"  ");
    if (CmdsColors[number].color2_str) strcat(buffer, CmdsColors[number].color2_str);
    else strcat(buffer, "WHITE");
    strcat(buffer,"  ");
    if (CmdsColors[number].color3_str) strcat(buffer, CmdsColors[number].color3_str);
    else strcat(buffer, "WHITE");
    strcat(buffer,"  ");
    if (CmdsColors[number].color4_str) strcat(buffer, CmdsColors[number].color4_str);
    else strcat(buffer, "WHITE");
    strcat(buffer,"  ");
    if (CmdsColors[number].color5_str) strcat(buffer, CmdsColors[number].color5_str);
    else strcat(buffer, "WHITE");
    strcat(buffer,"  ");
    if (CmdsColors[number].color6_str) strcat(buffer, CmdsColors[number].color6_str);
    else strcat(buffer, "WHITE");
}

/* Saves color settings to ScrollZ.save */
void SaveColors(usfile)
FILE *usfile;
{
    int  i;
    char tmpbuf[mybufsize/4];

    fprintf(usfile,"#\n");
    fprintf(usfile,"# Colors settings\n");
    fprintf(usfile,"# COLOR   Code    Col1  Col2  Col3  Col4  Col5  Col6\n");
    fprintf(usfile,"# Codes:    WARNING,JOIN,MSG,NOTICE,NETSPLIT,INVITE,MODE,\n");
    fprintf(usfile,"#           SETTING,LEAVE,NOTIFY,CTCP,KICK,DCC,WHO,\n");
    fprintf(usfile,"#           WHOIS,PUBLIC,CDCC,LINKS,DCCCHAT,CSCAN,NICK,\n");
    fprintf(usfile,"#           ME,SBAR,SBAR2 and OV\n");
    fprintf(usfile,"# Colors:   OFF,BOLD,UNDERLINE,FLASH,REVERSE,NOBOLD\n");
    fprintf(usfile,"#           BLACK,RED,GREEN,YELLOW,BLUE,PURPLE,CYAN,WHITE\n");
    fprintf(usfile,"#           BLACKBG,REDBG,GREENBG,YELLOWBG,BLUEBG,PURPLEBG,CYANBG,WHITEBG\n");
    fprintf(usfile,"#\n");
    for (i=0;i<NUMCMDCOLORS;i++) {
        GetColors(i,tmpbuf);
        switch (i) {
            case COLWARNING:
                fprintf(usfile,"COLOR  WARNING   %s\n",tmpbuf);
                break;
            case COLJOIN:
                fprintf(usfile,"COLOR  JOIN      %s\n",tmpbuf);
                break;
            case COLMSG:
                fprintf(usfile,"COLOR  MSG       %s\n",tmpbuf);
                break;
            case COLNOTICE:
                fprintf(usfile,"COLOR  NOTICE    %s\n",tmpbuf);
                break;
            case COLNETSPLIT:
                fprintf(usfile,"COLOR  NETSPLIT  %s\n",tmpbuf);
                break;
            case COLINVITE:
                fprintf(usfile,"COLOR  INVITE    %s\n",tmpbuf);
                break;
            case COLMODE:
                fprintf(usfile,"COLOR  MODE      %s\n",tmpbuf);
                break;
            case COLSETTING:
                fprintf(usfile,"COLOR  SETTING   %s\n",tmpbuf);
                break;
            case COLLEAVE:
                fprintf(usfile,"COLOR  LEAVE     %s\n",tmpbuf);
                break;
            case COLNOTIFY:
                fprintf(usfile,"COLOR  NOTIFY    %s\n",tmpbuf);
                break;
            case COLCTCP:
                fprintf(usfile,"COLOR  CTCP      %s\n",tmpbuf);
                break;
            case COLKICK:
                fprintf(usfile,"COLOR  KICK      %s\n",tmpbuf);
                break;
            case COLDCC:
                fprintf(usfile,"COLOR  DCC       %s\n",tmpbuf);
                break;
            case COLWHO:
                fprintf(usfile,"COLOR  WHO       %s\n",tmpbuf);
                break;
            case COLWHOIS:
                fprintf(usfile,"COLOR  WHOIS     %s\n",tmpbuf);
                break;
            case COLPUBLIC:
                fprintf(usfile,"COLOR  PUBLIC    %s\n",tmpbuf);
                break;
            case COLCDCC:
                fprintf(usfile,"COLOR  CDCC      %s\n",tmpbuf);
                break;
            case COLLINKS:
                fprintf(usfile,"COLOR  LINKS     %s\n",tmpbuf);
                break;
            case COLDCCCHAT:
                fprintf(usfile,"COLOR  DCCCHAT   %s\n",tmpbuf);
                break;
            case COLCSCAN:
                fprintf(usfile,"COLOR  CSCAN     %s\n",tmpbuf);
                break;
            case COLNICK:
                fprintf(usfile,"COLOR  NICK      %s\n",tmpbuf);
                break;
            case COLME:
                fprintf(usfile,"COLOR  ME        %s\n",tmpbuf);
                break;
            case COLMISC:
                fprintf(usfile,"COLOR  MISC      %s\n",tmpbuf);
                break;
            case COLSBAR1:
                fprintf(usfile,"COLOR  SBAR      %s\n",tmpbuf);
                break;
            case COLSBAR2:
                fprintf(usfile,"COLOR  SBAR2     %s\n",tmpbuf);
                break;
#ifdef CELECOSM
            case COLCELE:
                fprintf(usfile,"COLOR  CELE      %s\n",tmpbuf);
                break;
#endif
#ifdef OPERVISION
            case COLOV:
                fprintf(usfile,"COLOR  OV        %s\n",tmpbuf);
                break;
#endif
        }
    }
}

/* Returns corresponding index into Colors for given color name */
int ColorNumber(colname)
char *colname;
{
    int  i;
    char tmpbuf[mybufsize / 4];
    char *colornames[SZNUMCOLORS]={
        "NORMAL", "BOLD", "UNDERLINE", "FLASH", "REVERSE",
        "BLACK", "RED", "GREEN", "YELLOW", "BLUE", "PURPLE", "CYAN", "WHITE",
        "BLACKBG", "REDBG", "GREENBG", "YELLOWBG", "BLUEBG", "PURPLEBG", "CYANBG", "WHITEBG",
        "NOBOLD"
    };

    if (colname && *colname) {
        strmcpy(tmpbuf, colname, sizeof(tmpbuf));
        for (i = 0; i < SZNUMCOLORS; i++) if (!strcmp(tmpbuf, colornames[i])) break;
        return(i < SZNUMCOLORS ? i : -1);
    }
    return(-1);
}

/* Parses color string and puts color codes in target string */
int BuildColor(color, string, lineno)
char *color;
char *string;
int  lineno;
{
    int  colindex;
    char *tmpstr;
    char *tmpstr1;
    char tmpbuf[mybufsize / 4];

    strcpy(string, empty_string);
    tmpstr = color;
    while (*tmpstr) {
        tmpstr1 = tmpbuf;
        while (*tmpstr && *tmpstr != ',') *tmpstr1++ = *tmpstr++;
        *tmpstr1 = '\0';
        if ((colindex = ColorNumber(tmpbuf)) != -1) strcat(string, Colors[colindex]);
        else if (!BuildColorNew(tmpbuf, string, lineno)) return(0);
        if (*tmpstr == ',') tmpstr++;
    }
    return(1);
}

/* Sets colors loaded from ScrollZ.save */
void SetColors(number, string, error, lineno)
int  number;
char **string;
int  *error;
int  lineno;
{
    int  i;
    char tmpbuf1[mybufsize / 4];
    char tmpbuf2[mybufsize / 4];

    for (i = 1; i < 7; i++) {
        NextArg(*string, string, tmpbuf1);
        if (tmpbuf1[0]) {
            upper(tmpbuf1);
            *tmpbuf2 = '\0';
            if (BuildColor(tmpbuf1, tmpbuf2, lineno)) {
                switch (i) {
                    case 1:
                        malloc_strcpy(&(CmdsColors[number].color1), tmpbuf2);
                        malloc_strcpy(&(CmdsColors[number].color1_str), tmpbuf1);
                        break;
                    case 2:
                        malloc_strcpy(&(CmdsColors[number].color2), tmpbuf2);
                        malloc_strcpy(&(CmdsColors[number].color2_str), tmpbuf1);
                        break;
                    case 3:
                        malloc_strcpy(&(CmdsColors[number].color3), tmpbuf2);
                        malloc_strcpy(&(CmdsColors[number].color3_str), tmpbuf1);
                        break;
                    case 4:
                        malloc_strcpy(&(CmdsColors[number].color4), tmpbuf2);
                        malloc_strcpy(&(CmdsColors[number].color4_str), tmpbuf1);
                        break;
                    case 5:
                        malloc_strcpy(&(CmdsColors[number].color5), tmpbuf2);
                        malloc_strcpy(&(CmdsColors[number].color5_str), tmpbuf1);
                        break;
                    case 6:
                        malloc_strcpy(&(CmdsColors[number].color6), tmpbuf2);
                        malloc_strcpy(&(CmdsColors[number].color6_str), tmpbuf1);
                        break;
                }
            }
            else {
                if (lineno) PrintError("wrong color", empty_string, lineno);
                *error = 1;
                return;
            }
        }
        else {
            if (lineno) PrintError("there should be 6 colors", empty_string, lineno);
            *error = 1;
            return;
        }
    }
    if (number == COLSBAR1 || number == COLSBAR2) build_status(NULL);
}
#endif

/* Prints settings */
void PrintSetting(string1,setting1,string2,setting2)
char *string1;
char *setting1;
char *string2;
char *setting2;
{
    int doorignick=0;
    char quietstr[mybufsize/8];

    if (OrigNickChange && OrigNickQuiet && !strcmp(string1,"Reverting to original nick")) {
        doorignick=1;
        strcpy(quietstr," quietly");
    }
#ifdef WANTANSI
    say("%s is %s%s%s%s%s %s%s%s",string1,
        CmdsColors[COLSETTING].color2,setting1,
        doorignick?quietstr:"",Colors[COLOFF],string2,
        CmdsColors[COLSETTING].color5,setting2,Colors[COLOFF]);
#else
    say("%s is %c%s%s%c%s %c%s%c",string1,
        bold,setting1,doorignick?quietstr:"",
        bold,string2,bold,setting2,bold);
#endif
}

/* append given characters to string */
void append_esc(c1, c2, c3, dst)
char c1;
char c2;
char c3;
unsigned char **dst;
{
    **dst = 0x1B;
    (*dst)++;
    **dst = c1;
    (*dst)++;
    **dst = c2;
    (*dst)++;
    if (c3 != 0) {
        **dst = c3;
        (*dst)++;
    }
}

/* Strips ANSI codes from string */
/* printonly = 0 ... don't strip non-printable characters
               1 ... strip non-printable characters
               2 ... same as 1, but don't strip BOLD, REVERSE and UNDERLINE
                     or characters >=128 (national characters)
               3 ... same as 2, but also strip BOLD, REVERSE and UNDERLINE
               4 ... same as 1 but convert non-printable characters to reverse */
void StripAnsi(line, destline, printonly)
char *line;
char *destline;
int  printonly;
{
    int what = 0;
    int isattr;
    int iso2022 = get_int_var(ISO2022_SUPPORT_VAR);
    unsigned char *tmpstr;
    unsigned char *newstr = destline;

    for (tmpstr = line; *tmpstr; tmpstr++) {
        if (*tmpstr == 0x1B) what = 1;
        /* ISO-2022-JP */
        else if (what == 1 && iso2022 && *tmpstr == '(') what = 2;
        else if (what == 2 && iso2022 && *tmpstr == 'U') {
            append_esc('(', 'U', 0, &newstr);
            what = 0;
        }
        else if (what == 2 && iso2022 && *tmpstr == 'B') {
            append_esc('(', 'B', 0, &newstr);
            what = 0;
        }
        else if (what == 2 && iso2022 && *tmpstr == 'J') {
            append_esc('(', 'J', 0, &newstr);
            what = 0;
        }
        else if (what == 2 && iso2022 && *tmpstr == 'I') {
            append_esc('(', 'I', 0, &newstr);
            what = 0;
        }
        else if (what == 1 && iso2022 && *tmpstr == '$') what = 3;
        else if (what == 3 && iso2022 && *tmpstr == '@') {
            append_esc('$', '@', 0, &newstr);
            what = 0;
        }
        else if (what == 3 && iso2022 && *tmpstr == 'B') {
            append_esc('$', 'B', 0, &newstr);
            what = 0;
        }
        else if (what == 3 && iso2022 && *tmpstr == '(') what = 4;
        else if (what == 4 && iso2022 && *tmpstr == 'D') {
            append_esc('$', '(', 'D', &newstr);
            what = 0;
        }
        else if (what == 4 && iso2022 && *tmpstr == 'O') {
            append_esc('$', '(', 'O', &newstr);
            what = 0;
        }
        else if (what == 4 && iso2022 && *tmpstr == 'P') {
            append_esc('$', '(', 'P', &newstr);
            what = 0;
        }
        else if (what == 4 && iso2022 && *tmpstr == 'Q') {
            append_esc('$', '(', 'Q', &newstr);
            what = 0;
        }
        else if (what && isalpha(*tmpstr)) what = 0;
        else if (!what) {
            if (printonly && (*tmpstr < ' ' || *tmpstr > '~')) {
                if (printonly == 4 && *tmpstr < ' ') {
                    *newstr ++= REV_TOG;
                    *newstr ++= (*tmpstr & 127) | 64;
                    *newstr ++= REV_TOG;
                    continue;
                }
                if (printonly == 1) continue;
                if (*tmpstr == 0x9B || *tmpstr == 0x84) continue;
                isattr = (*tmpstr == BOLD_TOG || *tmpstr == REV_TOG ||
                          *tmpstr == UND_TOG  || *tmpstr == ALL_OFF);
                if (printonly == 3 && isattr) continue;
                if (!(*tmpstr > '~' || isattr)) continue;
            }
            *newstr ++= *tmpstr;
        }
    }
    *newstr = '\0';
}

/* Prints netsplit stuff */
void SplitPrint(reason,nick,channel,netsplit)
char *reason;
char *nick;
char *channel;
int  netsplit;
{
    char *tmpstr;
    char *server;
    char tmpbuf1[mybufsize];
#ifdef WANTANSI
    char tmpbuf2[mybufsize];
#endif

    if (NHDisp && netsplit==2) {
        strmcpy(tmpbuf1, reason, sizeof(tmpbuf1));
        tmpstr = tmpbuf1;
        server = new_next_arg(tmpstr,&tmpstr);
#ifdef WANTANSI
#ifdef CELECOSM
        if (Stamp < 2) snprintf(tmpbuf2,sizeof(tmpbuf2), "%snetsplit%s %s[%s%s%s%s%s]%s - ",
                               CmdsColors[COLNETSPLIT].color1, Colors[COLOFF],
                               CmdsColors[COLNETSPLIT].color3, Colors[COLOFF],
                               CmdsColors[COLNETSPLIT].color2, update_clock(0, 0, GET_TIME), Colors[COLOFF],
                               CmdsColors[COLNETSPLIT].color3, Colors[COLOFF]);
        else snprintf(tmpbuf2,sizeof(tmpbuf2), "%snetsplit%s - ", CmdsColors[COLNETSPLIT].color1, Colors[COLOFF]);
#else  /* CELECOSM */
        snprintf(tmpbuf2,sizeof(tmpbuf2), "%sNetsplit detected%s%s%s%s%s : ",
                CmdsColors[COLNETSPLIT].color1, Colors[COLOFF],
                Stamp < 2 ? " at " : empty_string,
                CmdsColors[COLNETSPLIT].color2,
                Stamp < 2 ? update_clock(0, 0, GET_TIME) : empty_string,
                Colors[COLOFF]);
#endif /* CELECOSM */
        say("%s[%s%s%s %s<-%s %s%s%s]", tmpbuf2,
            CmdsColors[COLNETSPLIT].color3, tmpstr, Colors[COLOFF],
            CmdsColors[COLNETSPLIT].color6, Colors[COLOFF],
            CmdsColors[COLNETSPLIT].color3, server, Colors[COLOFF]);
#else  /* WANTANSI */
        say("Netsplit detected%s%s : [%s <- %s]",
            Stamp < 2 ? " at " : empty_string,
            Stamp < 2 ? update_clock(0, 0, GET_TIME) : empty_string, tmpstr, server);
#endif /* WANTANSI */
    }
}

/* Prints mode stuff */
void ModePrint(line,channel,nick,userhost,nethacks,servmode)
char *line;
char *channel;
char *nick;
char *userhost;
char *nethacks;
char *servmode;
{
    int  isitserver;
#ifdef WANTANSI
    char tmpbuf1[mybufsize/2];
#endif
    char tmpbuf2[mybufsize];
    ChannelList *chan = NULL;

    if (ChanLog) chan = lookup_channel(channel, parsing_server_index, 0);
    isitserver=index(nick,'.')?1:0;
    if (isitserver) {
        if (NHDisp==2 && nethacks[0]) {
#ifdef WANTANSI
#ifdef CELECOSM
            snprintf(tmpbuf1,sizeof(tmpbuf1),"/%s%s%s - %s%s%s/",
#else
            snprintf(tmpbuf1,sizeof(tmpbuf1),"[%s%s%s] on %s%s%s",
#endif /* CELECOSM */
                    CmdsColors[COLNETSPLIT].color3,nick,Colors[COLOFF],
                    CmdsColors[COLNETSPLIT].color4,channel,Colors[COLOFF]);
#ifdef CELECOSM
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%snethack%s %s  %s%s%s",
#else
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%sNetsplit hack%s %s by : %s%s%s",
#endif /* CELECOSM */
                    CmdsColors[COLNETSPLIT].color1,Colors[COLOFF],tmpbuf1,
                    CmdsColors[COLNETSPLIT].color5,nethacks,Colors[COLOFF]);
#else  /* WANTANSI */
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%cNetsplit hack%c [%s] detected on %s by : %s",
                    bold,bold,nick,channel,nethacks);
#endif /* WANTANSI */
            say("%s",tmpbuf2);
            if (away_set || LogOn) AwaySave(tmpbuf2,SAVEHACK);
            if (chan && chan->ChanLog) ChannelLogSave(tmpbuf2, chan);
        }
        if (servmode[0]) {
#ifdef WANTANSI
#ifdef CELECOSM
            snprintf(tmpbuf1,sizeof(tmpbuf1),"%sserver modes%s in %s%s%s: \"%s%s%s\" by ",
                    CmdsColors[COLMODE].color5,Colors[COLOFF],
                    CmdsColors[COLMODE].color3,channel,Colors[COLOFF],
                    CmdsColors[COLMODE].color4,servmode,Colors[COLOFF]);
#else  /* CELECOSM */
            snprintf(tmpbuf1,sizeof(tmpbuf1),"%sServer modes%s \"%s%s%s\" detected on %s%s%s by : ",
                    CmdsColors[COLMODE].color5,Colors[COLOFF],
                    CmdsColors[COLMODE].color4,servmode,Colors[COLOFF],
                    CmdsColors[COLMODE].color3,channel,Colors[COLOFF]);
#endif /* CELECOSM */
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%s%s%s",
                    tmpbuf1,CmdsColors[COLMODE].color1,nick,Colors[COLOFF]);
#else  /* WANTANSI */
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%cServer modes%c \"%s\" detected on %s by : %s",
                    bold,bold,servmode,channel,nick);
#endif /* WANTANSI */
            say("%s",tmpbuf2);
            if (away_set || LogOn) AwaySave(tmpbuf2,SAVESRVM);
            if (chan && chan->ChanLog) ChannelLogSave(tmpbuf2, chan);
        }
    }
    else {
#ifdef WANTANSI
#ifdef CELECOSM
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%smode%s in %s%s%s: \"%s%s%s\" by ",
                CmdsColors[COLMODE].color5,Colors[COLOFF],
                CmdsColors[COLMODE].color3,channel,Colors[COLOFF],
                CmdsColors[COLMODE].color4,line,Colors[COLOFF]);
#else  /* CELECOSM */
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%sMode change%s \"%s%s%s\" on channel %s%s%s by ",
                CmdsColors[COLMODE].color5,Colors[COLOFF],
                CmdsColors[COLMODE].color4,line,Colors[COLOFF],
                CmdsColors[COLMODE].color3,channel,Colors[COLOFF]);
#endif /* CELECOSM */
        say("%s%s%s%s",tmpbuf1,CmdsColors[COLMODE].color1,nick,Colors[COLOFF]);
#else  /* WANTANSI */
        say("Mode change \"%s\" on channel %s by %s",line, channel, nick);
#endif /* WANTANSI */
        if (chan && chan->ChanLog) {
            snprintf(tmpbuf2, sizeof(tmpbuf2), "Mode change \"%s\" on channel %s by %s",line, channel, nick);
            ChannelLogSave(tmpbuf2, chan);
        }
    }
}

/* Prints kick stuff */
void KickPrint(who,word,from,channel,comment,rejoin,frkick)
char *who;
char *word;
char *from;
char *channel;
char *comment;
int  rejoin;
int  frkick;
{
#ifdef WANTANSI
    char *colnick;
    char tmpbuf[mybufsize/2];

    if (frkick) colnick=CmdsColors[COLKICK].color6;
    else colnick=CmdsColors[COLKICK].color1;
#ifdef CELECOSM
    snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s %s %skicked%s from %s%s%s by",
            colnick,who,Colors[COLOFF],word,
            CmdsColors[COLKICK].color5,Colors[COLOFF],
            CmdsColors[COLKICK].color3,channel,Colors[COLOFF]);
#else  /* CELECOSM */
    snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s %s been %skicked%s from channel %s%s%s by",
            colnick,who,Colors[COLOFF],word,
            CmdsColors[COLKICK].color5,Colors[COLOFF],
            CmdsColors[COLKICK].color3,channel,Colors[COLOFF]);
#endif /* CELECOSM */
    if (comment && *comment)
        say("%s %s%s%s (%s%s%s)",tmpbuf,
            CmdsColors[COLKICK].color2,from,Colors[COLOFF],
            CmdsColors[COLKICK].color4,comment,Colors[COLOFF]);
    else say("%s %s%s%s",tmpbuf,
             CmdsColors[COLKICK].color2,from,Colors[COLOFF]);
    if (rejoin) {
        save_message_from();
	message_from(channel,LOG_CRAP);
        say("Auto rejoining %s%s%s",CmdsColors[COLJOIN].color3,channel,Colors[COLOFF]);
        restore_message_from();
    }
#else
    if (comment && *comment) say("%s %s been kicked from channel %s by %s (%s)",
                                 who,word,channel,from,comment);
    else say("%s %s been kicked from channel %s by %s (%s)",
             who,word,channel,from);
    if (rejoin) {
        save_message_from();
	message_from(channel,LOG_CRAP);
        say("Auto rejoining %s",channel);
        restore_message_from();
    }
#endif
}

/* Sets master password for ENCRMSG and log file encryption */
void Password(command, args, subargs)
char *command;
char *args;
char *subargs;
{
    char *oldpwd;
    char *newpwd;

    newpwd = new_next_arg(args, &args);
    if (newpwd) {
        int pwlen = 2 * strlen(newpwd) + 16;
        char *newpass = (char *) new_malloc(pwlen + 1);

        /* a-la master password in Mozilla - store encrypted password */
        EncryptString(newpass, newpwd, newpwd, pwlen, 0, SZ_ENCR_OTHER);
        if (!(args && *args)) {
            if (EncryptPassword) {
                if (!strcmp(newpass, EncryptPassword)) {
                    if (encrlist) {
                        say("Encryption list not empty - master password NOT deleted!");
                        return;
                    }
                    say("Master password deleted, encryption is now disabled!");
                    new_free(&EncryptPassword);
                }
                else say("Incorrect master password, not deleted!");
            }
            else {
                malloc_strcpy(&EncryptPassword, newpass);
                say("Remember your master password!");
            }
        }
        else {
            int oldpwlen;
            char *oldpass = NULL;

            oldpwd = new_next_arg(args, &args);
            if (!oldpwd) return;
            if (oldpwd) {
                oldpwlen = 2 * strlen(oldpwd) + 16;
                oldpass = (char *) new_malloc(oldpwlen + 1);
                EncryptString(oldpass, oldpwd, oldpwd, oldpwlen, 0, SZ_ENCR_OTHER);
                bzero(oldpwd, strlen(oldpwd));
            }
            if (!EncryptPassword)
                PrintUsage("PASSWD [password [old password]]");
            else {
                if (strcmp(EncryptPassword, newpass))
                    say("Incorrect master password!");
                else {
                    malloc_strcpy(&EncryptPassword, newpass);
                    say("Remember your master password!");
                }
            }
            new_free(&oldpass);
        }
        new_free(&newpass);
        bzero(newpwd, strlen(newpwd));
    }
    else {
        /* no master password - ask for one */
        if (!EncryptPassword) add_wait_prompt("Master Password:", MasterPassword, NULL, WAIT_PROMPT_LINE);
        else add_wait_prompt("Old Master Password:", MasterPasswordOld, NULL, WAIT_PROMPT_LINE);
    }
}

/* Handle old master password */
void MasterPasswordOld(char *x, char *pass)
{
    int pwlen;
    char *mastpass;

    pwlen = 2 * strlen(pass) + 16;
    mastpass = (char *) new_malloc(pwlen + 1);
    EncryptString(mastpass, pass, pass, pwlen, 0, SZ_ENCR_OTHER);
    if (strcmp(EncryptPassword, mastpass)) {
        say("Incorrect master password!");
        new_free(&mastpass);
        return;
    }
    new_free(&mastpass);
    add_wait_prompt("Master Password:", MasterPassword, pass, WAIT_PROMPT_LINE);
}

/* Handle master password */
void MasterPassword(char *x, char *pass)
{
    char tmpbuf[mybufsize / 2 + 1];

    *tmpbuf = '\0';
    if (x) {
        strmcpy(tmpbuf, x, mybufsize / 2);
        strmcat(tmpbuf, " ", mybufsize / 2);
    }
    strmcat(tmpbuf, pass, mybufsize / 2);
    Password(NULL, tmpbuf, NULL);
}

/* Prints who reply */
void PrintWho(channel,nick,stat,user,host,name,server,show_server)
char *channel;
char *nick;
char *stat;
char *user;
char *host;
char *name;
char *server;
int  show_server;
{
    int  width=-1;
    char format[40];
    char *stampbuf=TimeStamp(2);
#ifdef WANTANSI
    char tmpbuf1[mybufsize/2];
    char tmpbuf2[mybufsize/2];

    if ((width=get_int_var(CHANNEL_NAME_WIDTH_VAR))!=0)
        snprintf(format,sizeof(format),"%%s%%-%u.%us%%s %%s%%-9s%%s",
                (unsigned char) width,(unsigned char) width);
    else strcpy(format,"%s%s%s\t%s%-9s%s");
    snprintf(tmpbuf1,sizeof(tmpbuf1),format,
            CmdsColors[COLWHO].color3,channel,Colors[COLOFF],
            CmdsColors[COLWHO].color1,nick,Colors[COLOFF]);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"%s %s%-3s%s %s%s%s%s@%s%s%s%s",tmpbuf1,
            CmdsColors[COLWHO].color4,stat,Colors[COLOFF],
            CmdsColors[COLWHO].color2,user,Colors[COLOFF],
            CmdsColors[COLMISC].color1,Colors[COLOFF],
            CmdsColors[COLWHO].color2,host,Colors[COLOFF]);
    put_it("%s%s %c%s%s%s%c",stampbuf,tmpbuf2,
           show_server?'[':'(',
           CmdsColors[COLWHO].color5,show_server?server:name,Colors[COLOFF],
           show_server?']':')');
#else
    if ((width=get_int_var(CHANNEL_NAME_WIDTH_VAR))!=0)
        snprintf(format,sizeof(format),"%s%%-%u.%us %%-9s %%-3s %%s@%%s (%%s)",
                (unsigned char) width,(unsigned char) width);
    else strcpy(format,"%s%s\t%-9s %-3s %s@%s (%s)");
    put_it(format,stampbuf,channel,nick,stat,user,host,show_server?server:name);
#endif
}

/* Prints whois user reply */
void PrintWhoIsUser(banner,word,nick,user,host,name,channel)
char *banner;
char *word;
char *nick;
char *user;
char *host;
char *name;
char *channel;
{
    char tmpbuf1[mybufsize/2];
    char *country;
    struct friends *tmpfriend;
#ifdef GENX
    char tmpbuf2[mybufsize/2];
#endif /* GENX */

    country=empty_string;
#ifdef COUNTRY
    if ((country=rindex(host,'.'))) country++;
    country=function_country(country);
    snprintf(tmpbuf1,sizeof(tmpbuf1)," [%s]",country);
    new_free(&country);
    malloc_strcpy(&country,tmpbuf1);
#endif /* COUNTRY */
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s!%s@%s",nick,user,host);
    tmpfriend=CheckUsers(tmpbuf1,get_channel_by_refnum(0));
#ifdef WANTANSI
#ifdef GENX
    if (!my_stricmp(word,"is")) word="address";
    else word="was";
    put_it("%sÚÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ",banner);
    snprintf(tmpbuf2,sizeof(tmpbuf2), "%s³ %s%8s%s ³ %s%s%s%s!%s%s%s%s%s@%s%s%s%s",banner,
            CmdsColors[COLWHOIS].color5,word,Colors[COLOFF],
            CmdsColors[COLWHOIS].color1,nick,Colors[COLOFF],
            CmdsColors[COLMISC].color1,Colors[COLOFF],
            CmdsColors[COLWHOIS].color2,user,Colors[COLOFF],
 	    CmdsColors[COLMISC].color1,Colors[COLOFF],
            CmdsColors[COLWHOIS].color2,host,Colors[COLOFF]);
    put_it("%s", tmpbuf2);
    put_it("%s³ %sirc name%s ³ %s%s",banner,
           CmdsColors[COLWHOIS].color5,Colors[COLOFF],name,Colors[COLOFF]);
    if (tmpfriend && tmpfriend->privs) {
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%s³   %sfriend%s ³ filt: %s%s%s",banner,
               CmdsColors[COLWHOIS].color5,Colors[COLOFF],
               CmdsColors[COLWHOIS].color4,tmpfriend->userhost,Colors[COLOFF]);
        snprintf(tmpbuf2,sizeof(tmpbuf2),"  acs: %s",CmdsColors[COLWHOIS].color4);
        BuildPrivs(tmpfriend,tmpbuf2);
        put_it("%s%s%s  chnl: %s%s%s",tmpbuf1,tmpbuf2,Colors[COLOFF],
               CmdsColors[COLWHOIS].color5,tmpfriend->channels,Colors[COLOFF]);
    }
#elif defined(CELECOSM)
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%s%s%s@%s%s%s%s",
            CmdsColors[COLWHOIS].color2,user,Colors[COLOFF],
            CmdsColors[COLMISC].color1,Colors[COLOFF],
            CmdsColors[COLWHOIS].color2,host,Colors[COLOFF]);
    put_it("%s%s%s%s is %s",banner,
           CmdsColors[COLWHOIS].color1,nick,Colors[COLOFF],tmpbuf1);
    put_it("%s%sircname%s:    %s%s%s",banner,
           CmdsColors[COLWHOIS].color5,Colors[COLOFF],
           CmdsColors[COLWHOIS].color3,name,Colors[COLOFF]);
    whoisfriend=tmpfriend;
    if (channel && *channel)
        put_it("%s%schannels%s:   %s%s%s",banner,
               CmdsColors[COLWHOIS].color5,Colors[COLOFF],
               CmdsColors[COLWHOIS].color3,channel,Colors[COLOFF]);
#else /* GENX & CELE */
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%s%s%s@%s%s%s%s",
            CmdsColors[COLWHOIS].color2,user,Colors[COLOFF],
 	    CmdsColors[COLMISC].color1,Colors[COLOFF],
            CmdsColors[COLWHOIS].color2,host,Colors[COLOFF]);
    put_it("%s%s%-9s%s : %s (%s%s)%s",banner,
           CmdsColors[COLWHOIS].color1,nick,Colors[COLOFF],tmpbuf1,name,
           Colors[COLOFF],country);
#ifdef COUNTRY
    new_free(&country);
#endif /* COUNTRY */
    if (tmpfriend && tmpfriend->privs) {
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%sFriend%s    : [Filt] #%d %s%s%s  [Acs] %s",
                banner,CmdsColors[COLWHOIS].color5,Colors[COLOFF],
                tmpfriend->number,
                CmdsColors[COLWHOIS].color4,tmpfriend->userhost,Colors[COLOFF],
                CmdsColors[COLWHOIS].color4);
        BuildPrivs(tmpfriend,tmpbuf1);
        put_it("%s%s  [Chnl] %s%s%s",tmpbuf1,Colors[COLOFF],
               CmdsColors[COLWHOIS].color6,tmpfriend->channels,Colors[COLOFF]);
    }
    if (channel && *channel)
        put_it("%s%sChannels%s  : %s%s%s",banner,
               CmdsColors[COLWHOIS].color5,Colors[COLOFF],
               CmdsColors[COLWHOIS].color3,channel,Colors[COLOFF]);
#endif /* GENX */
#else  /* WANTANSI */
    put_it("%s%-9s : %s@%s (%s)%s",banner,nick,user,host,name,country);
#ifdef COUNTRY
    new_free(&country);
#endif /* COUNTRY */
    if (tmpfriend && tmpfriend->privs) {
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%sFriend    : [Filt] #%d %s  [Acs] ",
                banner,tmpfriend->number,tmpfriend->userhost);
        BuildPrivs(tmpfriend,tmpbuf1);
        put_it("%s  [Chnl] %s",tmpbuf1,tmpfriend->channels);
    }
    if (channel && *channel) put_it("%sChannels  : %s",banner,channel);
#endif /* WANTANSI */
}

/* Prints whois channels reply */
void PrintWhoIsChannels(banner, channels)
char *banner;
char *channels;
{
    char tmpbuf1[2 * mybufsize];
    char tmpbuf2[2 * mybufsize];

    snprintf(tmpbuf1, sizeof(tmpbuf1), "%s", channels);
    StripAnsi(tmpbuf1, tmpbuf2, 4);
#ifdef WANTANSI
#ifdef GENX
    put_it("%s³ %schannels%s ³ %s%s%s", banner,
           CmdsColors[COLWHOIS].color5, Colors[COLOFF],
           CmdsColors[COLWHOIS].color3, tmpbuf2, Colors[COLOFF]);
#elif defined(CELECOSM)
    put_it("%s%schannels%s:   %s%s%s", banner,
           CmdsColors[COLWHOIS].color5, Colors[COLOFF],
           CmdsColors[COLWHOIS].color3, tmpbuf2, Colors[COLOFF]);
#else
    put_it("%s%sChannels%s  : %s%s%s", banner,
           CmdsColors[COLWHOIS].color5, Colors[COLOFF],
           CmdsColors[COLWHOIS].color3, tmpbuf2, Colors[COLOFF]);
#endif /* GENX */
#else  /* WANTANSI */
    put_it("%sChannels  : %s", banner, tmpbuf2);
#endif /* WANTANSI */
}

/* Prints whois servers reply */
void PrintWhoIsServer(banner,server,line)
char *banner;
char *server;
char *line;
{
#ifdef WANTANSI
#ifdef GENX
    char *uplink=NULL;
    char *tmpstr;
    char tmpbuf[mybufsize/2];

    if (line && *line && wild_match("*.*",line)) {
        uplink=line;
        if (*uplink=='[') uplink++;
        tmpstr=index(uplink,']');
        if (tmpstr) *tmpstr='\0';
        snprintf(tmpbuf,sizeof(tmpbuf),"%c:%c%s%s%s",bold,bold,
                CmdsColors[COLWHOIS].color4,uplink,Colors[COLOFF]);
    }
    if (uplink) put_it("%s³   %sserver%s ³ %s%s%s UpLink%s",banner,
                       CmdsColors[COLWHOIS].color5,Colors[COLOFF],
                       CmdsColors[COLWHOIS].color3,server,Colors[COLOFF],tmpbuf);
    else put_it("%s³   %sserver%s ³ %s%s%s",banner,
                CmdsColors[COLWHOIS].color5,Colors[COLOFF],
                CmdsColors[COLWHOIS].color3,server,Colors[COLOFF]);
#elif defined(CELECOSM)
    put_it("%s%sserver%s:     %s%s%s (%s)",banner,
           CmdsColors[COLWHOIS].color5,Colors[COLOFF],
           CmdsColors[COLWHOIS].color3,server,Colors[COLOFF],line);
#else
    put_it("%s%sServer%s    : %s%s%s (%s)",banner,
           CmdsColors[COLWHOIS].color5,Colors[COLOFF],
           CmdsColors[COLWHOIS].color3,server,Colors[COLOFF],line);
#endif /* GENX */
#else  /* WANTANSI */
    put_it("%sServer    : %s (%s)",banner,server,line);
#endif /* WANTANSI */
}

/* Returns true if line matches auto reply buffer */
int AutoReplyMatch(line)
char *line;
{
    int foundar = 0;
    int invmatch;
    char *newar;
    char *currentar;
    char tmpbuf1[mybufsize / 2];
    char tmpbuf2[2 * mybufsize];

    strmcpy(tmpbuf1, AutoReplyBuffer, sizeof(tmpbuf1));
    currentar = tmpbuf1;
    do {
        invmatch = 0;
        if ((newar = strchr(currentar, ','))) *newar++ = '\0';
        if (*currentar == '-') {
            currentar++;
            invmatch = 1;
        }
        snprintf(tmpbuf2, sizeof(tmpbuf2), "%s %s", currentar, line);
        currentar = function_match(tmpbuf2);
        if (atoi(currentar) > 0) {
            if (!invmatch) foundar = 1;
            else foundar = 0;
        }
        new_free(&currentar);
        currentar = newar;
    } while (currentar);
    return(foundar);
}

/* Prints public message */
void PrintPublic(nick,col,channel,line,print,iscrypted)
char *nick;
char *col;
char *channel;
char *line;
int  print;
int  iscrypted;
{
    int  foundar=0;
    int  isfriend=0;
    int  isshit=0;
    int  isitme;
    int  numurl=0;
    int  isopped=0;
    int  ishalfopped=0;
    int  isvoiced=0;
#ifdef WANTANSI
    char *coln=CmdsColors[COLCSCAN].color2;
#endif
    char *mynick;
    char *newcol;
    char *newchan;
    char *filepath=NULL;
    char tmpbuf1[mybufsize/8];
    char tmpbuf2[mybufsize/2];
    char tmpbuf3[2*mybufsize];
    char tmpbuf4[2*mybufsize];
#ifdef WANTANSI
    char tmpbuf5[mybufsize/16];
#endif
    char *stampbuf=TimeStamp(1);
#ifdef WANTANSI
    Window *oldwin;
#endif
    NickList *joiner;
    ChannelList *chan=NULL;
#ifdef CELE
    int truncate=get_int_var(TRUNCATE_PUBLIC_CHANNEL_VAR);
    char channel1[mybufsize/16];
#endif
#ifdef ALTERNATIVE_PUBLICS
    char pubschar='(';
    char pubechar=')';
#else
    char pubschar='<';
    char pubechar='>';
#endif
    char *cstr = empty_string;
    Window *w = NULL;

    if (iscrypted == 2) cstr = "[*]";
    else if (iscrypted) cstr = "[!]";

    chan=lookup_channel(channel,from_server,0);
    if (chan) {
        chan->pub++;
    }
    joiner=CheckJoiners(nick,channel,from_server,chan);
    if (joiner) {
        joiner->publics++;
        joiner->lastmsg=time((time_t *) 0);
        isshit=joiner->shitlist?joiner->shitlist->shit:0;
        isfriend=(!isshit && joiner->frlist)?joiner->frlist->privs:0;
        isopped=joiner->chanop;
	ishalfopped=joiner->halfop;
        isvoiced=joiner->hasvoice;
    }
    if (!col) {
        newcol=empty_string;
        newchan=empty_string;
    }
    else {
        newcol=col;
        newchan=channel;
    }
    mynick=get_server_nickname(from_server);
    isitme=!my_stricmp(mynick,nick);
    if (!isitme && URLCatch) {
        char tmpbuf5[mybufsize/2];

        snprintf(tmpbuf5,sizeof(tmpbuf5),"%s %s",nick,channel);
        filepath=OpenCreateFile("ScrollZ.notepad",1);
        numurl=GrabURL(line,tmpbuf4,sizeof(tmpbuf4),filepath,tmpbuf5,
#ifdef WANTANSI
                       CmdsColors[COLPUBLIC].color5
#else
                       NULL
#endif
                       );
    }
    else strmcpy(tmpbuf4,line,sizeof(tmpbuf4));
#ifdef WANTANSI
    if (!isitme || ShowNick) {
#ifdef CELECOSM
        if (isfriend && !isshit && isitme) coln=CmdsColors[COLMISC].color4;
        else
#endif /* CELECOSM */
        if (isitme) coln=CmdsColors[COLMISC].color5;
        else if (isshit && !isfriend) coln=CmdsColors[COLCSCAN].color6;
        else if (isfriend) coln=CmdsColors[COLMISC].color4;
        else coln=CmdsColors[COLPUBLIC].color2;
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%c%s",coln,pubschar,Colors[COLOFF]);
        snprintf(tmpbuf5,sizeof(tmpbuf5),"%s%c%s",coln,pubechar,Colors[COLOFF]);
        *tmpbuf3='\0';
        if (ExtPub) {
            strmcpy(tmpbuf3,CmdsColors[COLPUBLIC].color6,sizeof(tmpbuf3));
            if (isopped) strmcat(tmpbuf3,"@",sizeof(tmpbuf3));
	    else if (ishalfopped) strmcat(tmpbuf3,"%",sizeof(tmpbuf3));
            else if (isvoiced) strmcat(tmpbuf3,"+",sizeof(tmpbuf3));
            strmcat(tmpbuf3,Colors[COLOFF],sizeof(tmpbuf3));
            strmcat(tmpbuf1,tmpbuf3,sizeof(tmpbuf1));
        }
        if (!isitme && AutoReplyBuffer)
            foundar=AutoReplyMatch(tmpbuf4);
        if (!foundar || isitme) {
            if (Ego && isitme) strmcat(tmpbuf1,CmdsColors[COLPUBLIC].color6,sizeof(tmpbuf1));
            else strmcat(tmpbuf1,CmdsColors[COLPUBLIC].color1,sizeof(tmpbuf1));
        }
        else if (foundar) {
            strmcat(tmpbuf1,CmdsColors[COLPUBLIC].color4,sizeof(tmpbuf1));
            /* Display AR in current screen window too
               if it's different from orig channel window  -Pier */
            if (ARinWindow==3 || (ARinWindow && chan->window!=curr_scr_win)) {
                oldwin=to_window;
                if (ARinWindow==1) /* ON */
                    to_window=curr_scr_win;
                else { /* USER/BOTH */
                    to_window=get_window_by_level(LOG_USER4);
                    if (to_window==NULL)
                        to_window=curr_scr_win;
                }
#ifdef CELE
                if (truncate && strlen(newchan)>5) strmcpy(channel1,channel,5);
                else strmcpy(channel1,channel,sizeof(channel1));
#endif
                *tmpbuf3='\0';
                if (ExtPub) {
                    strmcpy(tmpbuf3,CmdsColors[COLPUBLIC].color6,sizeof(tmpbuf3));
                    if (isopped) strmcat(tmpbuf3,"@",sizeof(tmpbuf3));
                    else if (isvoiced) strmcat(tmpbuf3,"+",sizeof(tmpbuf3));
                    strmcat(tmpbuf3,Colors[COLOFF],sizeof(tmpbuf3));
                }
                snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%c%s%s%s%s%s:%s%s%s%s%c%s ",
                        CmdsColors[COLPUBLIC].color2,pubschar,Colors[COLOFF],
                        tmpbuf3,
                        CmdsColors[COLPUBLIC].color4,nick,Colors[COLOFF],
#ifdef CELE
                        CmdsColors[COLPUBLIC].color3,channel1,Colors[COLOFF],
#else
                        CmdsColors[COLPUBLIC].color3,channel,Colors[COLOFF],
#endif
                        CmdsColors[COLPUBLIC].color2,pubechar,Colors[COLOFF]);
                w = put_it("%s%s%s%s%s%s",cstr,stampbuf,tmpbuf2,
                           CmdsColors[COLPUBLIC].color5,tmpbuf4,Colors[COLOFF]);
                to_window=oldwin; 
            }
        }
        if (print) {
#ifdef CELE
            if (truncate && strlen(newchan)>5) strmcpy(channel1,newchan,5);
            else strmcpy(channel1,newchan,sizeof(channel1));
#endif
            snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%s%s%s%s%s%s",tmpbuf1,
                    nick,Colors[COLOFF],newcol,
#ifdef CELE
                    CmdsColors[COLPUBLIC].color3,channel1,Colors[COLOFF]
#else
                    CmdsColors[COLPUBLIC].color3,newchan,Colors[COLOFF]
#endif
                    );
            w = put_it("%s%s%s%s %s%s%s",cstr,stampbuf,tmpbuf2,tmpbuf5,
                       CmdsColors[COLPUBLIC].color5,tmpbuf4,Colors[COLOFF]);
        }
	/* AwaySave needs to go after the put_it(). Two if's doesn't hurt */
	if (foundar && (away_set || LogOn)) {
            snprintf(tmpbuf3,sizeof(tmpbuf3),"<%s:%s> %s",nick,channel,tmpbuf4);
            AwaySave(tmpbuf3,SAVEAREPLY);
	}
    }
    else {
        if (isitme) coln=CmdsColors[COLMISC].color5;
        else if (isshit && !isfriend) coln=CmdsColors[COLCSCAN].color6;
        else if (isfriend) coln=CmdsColors[COLMISC].color4;
        else coln=CmdsColors[COLPUBLIC].color2;
        if (!col && print)
            w = put_it("%s%s%s%c%s %s%s%s",cstr,stampbuf,
                       coln,pubechar,Colors[COLOFF],
                       CmdsColors[COLPUBLIC].color5,tmpbuf4,Colors[COLOFF]);
        else if (print) {
            snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%c%s%s%s%s%s>%s ",
                    coln,pubechar,Colors[COLOFF],
                    CmdsColors[COLPUBLIC].color3,newchan,Colors[COLOFF],
                    CmdsColors[COLPUBLIC].color2,Colors[COLOFF]);
            w = put_it("%s%s%s%s%s%s",cstr,stampbuf,tmpbuf1,
                       CmdsColors[COLPUBLIC].color5,tmpbuf4,Colors[COLOFF]);
        }
    }
#else  /* WANTANSI */
    if (!isitme || ShowNick) {
        if (!isitme && AutoReplyBuffer)
            foundar=AutoReplyMatch(tmpbuf4);
        *tmpbuf2='\0';
        if (ExtPub) {
            strmcpy(tmpbuf2,"",sizeof(tmpbuf2));
            if (isopped) strmcat(tmpbuf2,"@",sizeof(tmpbuf2));
	    else if (ishalfopped) strmcat(tmpbuf2,"%",sizeof(tmpbuf2));
            else if (isvoiced) strmcat(tmpbuf2,"+",sizeof(tmpbuf2));
            strmcat(tmpbuf2,"",sizeof(tmpbuf2));
        }
        if (print && (!foundar || isitme)) {
            snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%s%c%s",cstr,stampbuf,pubschar,tmpbuf2);
            w = put_it("%s%s%s%s%s%s%c %s",tmpbuf3,
                       (isitme && Ego)?"":"",nick,(isitme && Ego)?"":"",newcol,newchan,pubechar,tmpbuf4);
        }
        else {
            if (print)
                w = put_it("%s%s%c%s%s%s%s%c %s",cstr,
                           stampbuf,pubschar,tmpbuf2,nick,newcol,newchan,pubechar,tmpbuf4);
            if (away_set || LogOn) {
                snprintf(tmpbuf2,sizeof(tmpbuf2),"<%s:%s> %s",nick,channel,tmpbuf4);
                AwaySave(tmpbuf2,SAVEAREPLY);
            }
        }
    }
    else {
        if (!col && print) w = put_it("%s%s%c %s",cstr,stampbuf,pubechar,tmpbuf4);
        else if (print) w = put_it("%s%s%c%s> %s",cstr,stampbuf,pubechar,newchan,tmpbuf4);
    }
#endif /* WANTANSI */
    if (URLCatch && URLCatch<3 && numurl)
        say("Added %d URL%s to NotePad (%c%s%c)",numurl,numurl==1?"":"s",bold,filepath,
            bold);
    if (foundar) AddNick2AutoReply(nick);
    if (!isitme) Check4WordKick(line,joiner,isfriend,chan);
    if (chan && chan->ChanLog) {
        snprintf(tmpbuf3,sizeof(tmpbuf3),"<%s> %s", nick, line);
        ChannelLogSave(tmpbuf3, chan);
    }
    if (foundar && w && (w != curr_scr_win)) {
        w->miscflags |= WINDOW_REPWORD;
        update_all_status();
    }
}

/* Returns path to filename */
char *OpenCreateFile(filename, create)
char *filename;
int  create;
{
    char *path;
    char *filepath;
    char tmpbuf[mybufsize / 4];
    static char newfilepath[mybufsize / 8];

    if (!filename) return(NULL);
    if (*filename == '/') path = empty_string;
    else if (!(path = get_string_var(LOAD_PATH_VAR))) path = ".";
    if ((filepath = path_search(filename, path))) return(filepath);
    if (!create) return(NULL);
    strmcpy(tmpbuf, path, sizeof(tmpbuf));
    if ((filepath = index(tmpbuf, ':'))) *filepath = '\0';
    if (*tmpbuf && tmpbuf[strlen(tmpbuf) - 1] != '/') strmcat(tmpbuf, "/", sizeof(tmpbuf));
    strmcat(tmpbuf, filename, sizeof(tmpbuf));
    filepath = expand_twiddle(tmpbuf);
    strmcpy(newfilepath, filepath, sizeof(newfilepath));
    new_free(&filepath);
    return(newfilepath);
}

/* Prints links info */
void PrintLinks(server,uplink,distance)
char *server;
char *uplink;
char *distance;
{
    int  dist;
#ifdef WANTANSI
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
#endif

    dist=atoi(distance);
#ifdef WANTANSI
    if (!LinksNumber) PrintLinksHead();
#ifdef HAVE_ICONV_H
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%s%s",CmdsColors[COLLINKS].color5,
            get_int_var(HIGH_ASCII_VAR)?"\342\224\202":"|",Colors[COLOFF]);
#else
    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s%c%s",CmdsColors[COLLINKS].color5,
            get_int_var(HIGH_ASCII_VAR)?'³':'|',Colors[COLOFF]);
#endif /* HAVE_ICONV_H */
    LinksNumber++;
    snprintf(tmpbuf2,sizeof(tmpbuf2),"%s%s%3d%s %s %s%s%26s%s%s",
            tmpbuf1,CmdsColors[COLLINKS].color1,LinksNumber,Colors[COLOFF],tmpbuf1,
            tmpbuf1,CmdsColors[COLLINKS].color1,server,Colors[COLOFF],tmpbuf1);
    snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%s%3d%s %s %s>%s %s%s%25s%s%s",
            tmpbuf1,CmdsColors[COLLINKS].color3,dist,Colors[COLOFF],tmpbuf1,
            CmdsColors[COLLINKS].color4,Colors[COLOFF],tmpbuf1,
            CmdsColors[COLLINKS].color2,uplink,Colors[COLOFF],tmpbuf1);
    say("%s %s",tmpbuf2,tmpbuf3);
/*    snprintf(tmpbuf1,sizeof(tmpbuf1),"%s[%s",CmdsColors[COLLINKS].color4,Colors[COLOFF]);
    snprintf(tmpbuf2,sizeof(tmpbuf2),"%s]%s",CmdsColors[COLLINKS].color4,Colors[COLOFF]);
    if (!LinksNumber) {
        snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%sNo.%s%s %s          %sServer%s         %s",
                tmpbuf1,CmdsColors[COLLINKS].color1,Colors[COLOFF],tmpbuf2,
                tmpbuf1,CmdsColors[COLLINKS].color1,Colors[COLOFF],tmpbuf2);
        snprintf(tmpbuf4,sizeof(tmpbuf4),"%s%sDis%s%s    %s          %sUplink%s         %s",
                tmpbuf1,CmdsColors[COLLINKS].color2,Colors[COLOFF],tmpbuf2,
                tmpbuf1,CmdsColors[COLLINKS].color2,Colors[COLOFF],tmpbuf2);
        say("%s %s",tmpbuf3,tmpbuf4);
    }
    LinksNumber++;
    snprintf(tmpbuf3,sizeof(tmpbuf3),"%s%s%3d%s%s %s%s%25s%s%s",
            tmpbuf1,CmdsColors[COLLINKS].color1,LinksNumber,Colors[COLOFF],tmpbuf2,
            tmpbuf1,CmdsColors[COLLINKS].color1,server,Colors[COLOFF],tmpbuf2);
    snprintf(tmpbuf4,sizeof(tmpbuf4),"%s%s%3d%s%s %s->%s %s%s%25s%s%s",
            tmpbuf1,CmdsColors[COLLINKS].color3,dist,Colors[COLOFF],tmpbuf2,
            CmdsColors[COLLINKS].color3,Colors[COLOFF],
            tmpbuf1,CmdsColors[COLLINKS].color2,uplink,Colors[COLOFF],tmpbuf2);
    say("%s %s",tmpbuf3,tmpbuf4); */
#else
    if (!LinksNumber) say("[No.] [Dis] %-27s %-27s","Server","Uplink");
    LinksNumber++;
    say("[%3d] [%3d] %-27s %-27s",LinksNumber,dist,server,uplink);
#endif
}

/* This checks given password against master password */
void EncryptMasterPlayBack(stuff, line)
char *stuff;
char *line;
{
    int pwlen;
    char *mastpass;

    pwlen = 2 * strlen(line) + 16;
    mastpass = (char *) new_malloc(pwlen + 1);
    EncryptString(mastpass, line, line, pwlen, 0, SZ_ENCR_OTHER);
    if (strcmp(EncryptPassword, mastpass)) {
        say("Incorrect master password!");
        new_free(&mastpass);
        return;
    }
    new_free(&mastpass);
    PlayBack2(stuff, empty_string);
}

/* Shows stored messages from ScrollZ.away w/o setting you back */
void PlayBack(command, args, subargs)
char *command;
char *args;
char *subargs;
{
    char *pattern;
    char *filepath;
    char *filename;
    char tmpbuf[mybufsize / 4];

    new_free(&playpattern);
    filename = get_string_var(AWAY_FILE_VAR);
    filepath = OpenCreateFile(filename, 1);
    if (filepath && (awayfile = fopen(filepath, "r"))) {
        if (!my_strnicmp(args, "-R", 2)) {
            pattern = new_next_arg(args, &args);
            pattern = new_next_arg(args, &args);
            PlayReverse = 1;
        }
        else {
            PlayReverse = 0;
            pattern = args;
        }
        fseek(awayfile, 0, 2);
        filesize = ftell(awayfile);
        if (!PlayReverse) fseek(awayfile, 0, 0);
        if (!filesize) filesize = 1;
        DontHold = 0;
        if (command) subargs = NULL;
        if (pattern && *pattern) {
            if (*pattern != '*') snprintf(tmpbuf, sizeof(tmpbuf), "*%s*", pattern);
            else strmcpy(tmpbuf, pattern, sizeof(tmpbuf));
            malloc_strcpy(&playpattern, tmpbuf);
        }
        if (AwayEncrypt && EncryptPassword)
            add_wait_prompt("Master Password:", EncryptMasterPlayBack, subargs, WAIT_PROMPT_LINE);
        else
            PlayBack2(subargs, empty_string);
    }
    else {
        say("Can't open file %s", filename);
        PlayBack2(subargs, "q");
    }
}

/* Shows one page from ScrollZ.away file */
void PlayBack2(stuff,line)
char *stuff;
char *line;
{
    int  count = 2;
    int  readline = 1;
    char tmpbuf[2 * mybufsize + 1];
    char tmpbuf2[2 * mybufsize + 1];

    if (line && (*line == 'q' || *line == 'Q')) {
        if (awayfile) fclose(awayfile);
        add_wait_prompt("Delete message file (y/n) ? ", SetBackDelete, stuff, WAIT_PROMPT_KEY);
        return;
    }
    else if (line && (*line == 'c' || *line == 'C')) DontHold = 1;
    while (count < curr_scr_win->display_size && readline) {
        if (PlayReverse) {
            readline = rfgets(tmpbuf,mybufsize,awayfile) ? 1 : 0;
            if (readline && tmpbuf[strlen(tmpbuf) - 1] == '\n')
                tmpbuf[strlen(tmpbuf) - 1] = '\0';
        }
        else readline = readln(awayfile, tmpbuf);
        if (readline) {
            if (playpattern && !wild_match(playpattern, tmpbuf)) continue;
#ifdef WANTANSI
            count += (strlen(tmpbuf) - CountAnsi(tmpbuf, -1) + 6) / current_screen->co + 1;
#else
            count += (strlen(tmpbuf) + 6) / current_screen->co + 1;
#endif
            if (AwayEncrypt && EncryptPassword) DecryptString(tmpbuf2, tmpbuf, EncryptPassword, 2 * mybufsize, 0);
            else strcpy(tmpbuf2, tmpbuf);
            say("%s", tmpbuf2);
        }
    }
    if (!readline) {
        if (awayfile) fclose(awayfile);
        add_wait_prompt("Delete message file (y/n) ? ", SetBackDelete, stuff, WAIT_PROMPT_KEY);
        return;
    }
    if (DontHold) PlayBack2(stuff, line);
    else if (count >= curr_scr_win->display_size) {
        readline = ((float) (100 * ftell(awayfile)) / (float) filesize);
        if (PlayReverse) readline = 100 - readline;
        snprintf(tmpbuf, sizeof(tmpbuf), "[%2d%%] Press any key to continue, 'c' for continuous, 'q' to quit",
                readline);
        add_wait_prompt(tmpbuf, PlayBack2, stuff, WAIT_PROMPT_KEY);
    }
}

/* Defines what to save to ScrollZ.away */
void AwaySaveToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  newtype=AwaySaveSet;
    int  curval;
    int  minus=0;
    char *tmpstr=(char *) 0;

    if (args && *args) {
        while ((tmpstr=new_next_arg(args,&args))!=NULL) {
            minus=0;
            curval=0;
            if (*tmpstr=='-') {
                minus=1;
                tmpstr++;
            }
            if (!my_stricmp(tmpstr,"MSG")) curval=SAVEMSG;
            else if (!my_stricmp(tmpstr,"NOTICE")) curval=SAVENOTICE;
            else if (!my_stricmp(tmpstr,"MASS")) curval=SAVEMASS;
            else if (!my_stricmp(tmpstr,"COLL")) curval=SAVECOLL;
            else if (!my_stricmp(tmpstr,"CDCC")) curval=SAVECDCC;
            else if (!my_stricmp(tmpstr,"DCC")) curval=SAVEDCC;
            else if (!my_stricmp(tmpstr,"PROT")) curval=SAVEPROT;
            else if (!my_stricmp(tmpstr,"HACK")) curval=SAVEHACK;
            else if (!my_stricmp(tmpstr,"SRVMODE")) curval=SAVESRVM;
            else if (!my_stricmp(tmpstr,"CTCP")) curval=SAVECTCP;
            else if (!my_stricmp(tmpstr,"FLOOD")) curval=SAVEFLOOD;
            else if (!my_stricmp(tmpstr,"INVITE")) curval=SAVEINVITE;
            else if (!my_stricmp(tmpstr,"KILL")) curval=SAVEKILL;
            else if (!my_stricmp(tmpstr,"KICK")) curval=SAVEKICK;
            else if (!my_stricmp(tmpstr,"SERVER")) curval=SAVESERVER;
            else if (!my_stricmp(tmpstr,"FAKE")) curval=SAVEFAKE;
            else if (!my_stricmp(tmpstr,"AREPLY")) curval=SAVEAREPLY;
            else if (!my_stricmp(tmpstr,"CHAT")) curval=SAVECHAT;
            else if (!my_stricmp(tmpstr,"NOTIFY")) curval=SAVENOTIFY;
            else if (!my_stricmp(tmpstr,"SENTMSG")) curval=SAVESENTMSG;
            else if (!my_stricmp(tmpstr,"AWAY")) curval=SAVEAWAY;
            else if (!my_stricmp(tmpstr,"ALL")) newtype=SAVEALL;
            else if (!my_stricmp(tmpstr,"NONE")) newtype=0;
            else say("Unknown type %s",tmpstr);
            if (curval) {
                if (minus) newtype&=(~curval);
                else newtype|=curval;
            }
        }
        AwaySaveSet=newtype;
    }
    tmpstr=NULL;
    if ((AwaySaveSet&SAVEALL)==SAVEALL) malloc_strcpy(&tmpstr,"ALL");
    else {
        if (AwaySaveSet&SAVEMSG) malloc_strcat(&tmpstr,"MSG ");
        if (AwaySaveSet&SAVENOTICE) malloc_strcat(&tmpstr,"NOTICE ");
        if (AwaySaveSet&SAVEMASS) malloc_strcat(&tmpstr,"MASS ");
        if (AwaySaveSet&SAVECOLL) malloc_strcat(&tmpstr,"COLL ");
        if (AwaySaveSet&SAVECDCC) malloc_strcat(&tmpstr,"CDCC ");
        if (AwaySaveSet&SAVEDCC) malloc_strcat(&tmpstr,"DCC ");
        if (AwaySaveSet&SAVEPROT) malloc_strcat(&tmpstr,"PROT ");
        if (AwaySaveSet&SAVEHACK) malloc_strcat(&tmpstr,"HACK ");
        if (AwaySaveSet&SAVESRVM) malloc_strcat(&tmpstr,"SRVMODE ");
        if (AwaySaveSet&SAVECTCP) malloc_strcat(&tmpstr,"CTCP ");
        if (AwaySaveSet&SAVEFLOOD) malloc_strcat(&tmpstr,"FLOOD ");
        if (AwaySaveSet&SAVEINVITE) malloc_strcat(&tmpstr,"INVITE ");
        if (AwaySaveSet&SAVEKILL) malloc_strcat(&tmpstr,"KILL ");
        if (AwaySaveSet&SAVEKICK) malloc_strcat(&tmpstr,"KICK ");
        if (AwaySaveSet&SAVESERVER) malloc_strcat(&tmpstr,"SERVER ");
        if (AwaySaveSet&SAVEFAKE) malloc_strcat(&tmpstr,"FAKE ");
        if (AwaySaveSet&SAVEAREPLY) malloc_strcat(&tmpstr,"AREPLY ");
        if (AwaySaveSet&SAVECHAT) malloc_strcat(&tmpstr,"CHAT ");
        if (AwaySaveSet&SAVENOTIFY) malloc_strcat(&tmpstr,"NOTIFY ");
        if (AwaySaveSet&SAVESENTMSG) malloc_strcat(&tmpstr,"SENTMSG ");
        if (AwaySaveSet&SAVEAWAY) malloc_strcat(&tmpstr,"AWAY");
    }
    if (tmpstr) PrintSetting("Away saving",tmpstr,empty_string,empty_string);
    else PrintSetting("Away saving","NONE",empty_string,empty_string);
    new_free(&tmpstr);
}

/* Does /STATS k with filter */
#ifdef OPER
void StatsKFilter(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int doall=1;
    char *tmpstr;
    char *server=NULL;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr && *tmpstr) {
        if (!my_strnicmp(tmpstr,"-TEMP",5)) {
            doall=0;
            tmpstr=new_next_arg(args,&args);
        }
        if (tmpstr && *tmpstr) {
            malloc_strcpy(&StatskFilter,tmpstr);
            StatskNumber=0;
            server=new_next_arg(args,&args);
            send_to_server("STATS %c %s",doall?'K':'k',server?server:"");
            return;
        }
    }
    PrintUsage("FKLINE [-TEMP] filter [server]");
}

/* Parses STATS k reply from server */
void HandleStatsK(kline,rest)
char *kline;
char *rest;
{
    char *blah;
    char *tmpstr;
    char *format="%s@%s";
    char *comment=NULL;
    char *username=NULL;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/4];
    struct nicks *tmp;
    struct nicks *args=NULL;
    struct nicks *tmpnew;

    if (rest && *rest=='*') {
        /* it's ircd 2.8/th.v5.3a */
        blah=new_next_arg(rest,&rest);
        username=new_next_arg(rest,&rest);
        comment=rest;
    }
    else if (rest) {
        /* plain ircd 2.8 or 2.9 */
        username=tmpbuf2;
        comment=tmpbuf3;
        strmcpy(tmpbuf1,rest,sizeof(tmpbuf1));
        blah=tmpbuf1;
        /* let's dig out username and comment */
        while ((tmpstr=new_next_arg(blah,&blah))) {
            tmpnew=(struct nicks *) new_malloc(sizeof(struct nicks));
            tmpnew->nick=NULL;
            tmpnew->next=NULL;
            malloc_strcpy(&(tmpnew->nick),tmpstr);
            /* let's add new element at the end of the list */
            if (args) {
                for (tmp=args;tmp->next;) tmp=tmp->next;
                tmp->next=tmpnew;
            }
            else args=tmpnew;
        }
        /* element that is previous to previous to last now holds username */
        tmp=NULL;
        for (tmpnew=args;tmpnew;tmpnew=tmpnew->next)
            if (tmpnew->next && tmpnew->next->next) tmp=tmpnew;
        if (tmp) {
            strmcpy(username,tmp->nick,sizeof(tmpbuf2));
            /* elements before it are comment */
            strmcpy(tmpbuf1,rest,sizeof(tmpbuf1));
            tmpstr=tmpbuf1;
            while ((blah=strstr(tmpstr,username)))
                tmpstr=blah+1;
            /* blah now pointes to username, let's copy string before username */
            tmpstr--;
            tmpstr--;
            while (isspace(*tmpstr) && tmpstr>tmpbuf1) tmpstr--;
            if (!isspace(*tmpstr)) tmpstr++;
            *tmpstr='\0';
            strmcpy(comment,tmpbuf1,sizeof(tmpbuf3));
        }
        else say("Something went wrong while parsing K-line");
        /* let's free the list */
        for (tmp=args;args;tmp=args) {
            args=args->next;
            new_free(&(tmp->nick));
            new_free(&tmp);
        }
    }
    if (!username) username="*";
    if (!(comment && *comment)) comment="Empty";
    if (!StatskNumber) say("%-30s Comment","K-Line");
    if (kline && strchr(kline,'@')) {
        format="%s%s";
        username=empty_string;
    }
    snprintf(tmpbuf1,sizeof(tmpbuf1),format,username,kline);
    if ((StatskFilter && wild_match(StatskFilter,tmpbuf1)) || !StatskFilter)
        put_it("%s%-30s %s",numeric_banner(),tmpbuf1,comment);
    StatskNumber++;
}
#endif /* OPER */

/* Sets mode for your current channel */
void CurrentChanMode(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *channel=(char *) 0;
    ChannelList *chan;

    channel=get_channel_by_refnum(0);
    if (channel) {
        if (*args) {
            if ((chan=lookup_channel(channel,from_server,0)) &&
                HAS_OPS(chan->status))
                send_to_server("MODE %s %s",chan->channel,args);
            else NotChanOp(channel);
        }
        else PrintUsage("C chanmode");
    }
    else NoWindowChannel();
}

/* Just a declaration , needed to shut up the compiler */
void NslookupNew();

/* Looks up DNS entry */
#ifndef LITE
void Nslookup(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr;
    void (*func)();
    NickList *joiner;

    if (args && *args) {
        if (strchr(args,'.')) {
            DoNslookup(args);
            return;
        }
        else if ((joiner=CheckJoiners(args,NULL,from_server,NULL)) && joiner->userhost) {
            tmpstr=strchr(joiner->userhost,'@');
            if (tmpstr) {
                tmpstr++;
                DoNslookup(tmpstr);
                return;
            }
        }
        func=(void(*)())NslookupNew;
        server_list[from_server].SZWI++;
        add_userhost_to_whois(args,func);
    }
    else PrintUsage("NSLOOKUP entry");
}

/* Just calls DoNslookup with correct parameters */
void NslookupNew(wistuff,tmpnick,text)
WhoisStuff *wistuff;
char *tmpnick;
char *text;
{
    if (server_list[from_server].SZWI) server_list[from_server].SZWI--;
    if (wistuff->not_on || !wistuff->nick || my_stricmp(wistuff->nick,tmpnick)) {
        say("Can't find %s on IRC",tmpnick);
        return;
    }
    DoNslookup(wistuff->host);
}

/* This really does nslookup */
void DoNslookup(host)
char *host;
{
    int    isip=1;
    char   *tmpstr;
    struct hostent *hostaddr;
    uint32_t ipnum;

    for (tmpstr=host;tmpstr && *tmpstr;tmpstr++)
        isip&=(*tmpstr=='.' || isdigit(*tmpstr));
    say("Checking tables...");
    if (!isip) hostaddr=gethostbyname(host);
    else {
        ipnum=inet_addr(host);
        hostaddr=gethostbyaddr((char *) &ipnum,sizeof(ipnum),AF_INET);
    }
    if (!hostaddr) say("No entry found for %s",host);
    else say("%s is %s (%s)",host,hostaddr->h_name,
             inet_ntoa(*(struct in_addr *) hostaddr->h_addr));
}
#endif /* LITE */

/* Inserts nickname of the person that triggered auto reply */
void InsertAutoReply() {
    int curserv=from_server;
    char *tmpstr;

    if (CheckServer(curserv) && server_list[curserv].arcur) {
        input_clear_line(0,(char *) 0);
        tmpstr=(server_list[curserv].arcur)->nick;
        while (*tmpstr) {
            input_add_character(*tmpstr,NULL);
            tmpstr++;
        }
        tmpstr=AutoReplyString;
        while (tmpstr && *tmpstr) {
            input_add_character(*tmpstr,NULL);
            tmpstr++;
        }
        if (server_list[curserv].arcur)
            server_list[curserv].arcur=(server_list[curserv].arcur)->next;
        if (!(server_list[curserv].arcur))
            server_list[curserv].arcur=server_list[curserv].arlist;
    }
}

/* Dumps variables, aliases, ons or all of them */
void Dump(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    if (args && *args) {
        if (!my_stricmp(args,"ALIAS")) DumpAliases(COMMAND_ALIAS);
        else if (!my_stricmp(args,"VAR")) DumpAliases(VAR_ALIAS);
        else if (!my_stricmp(args,"ON")) DumpOn();
        else if (!my_stricmp(args,"ALL")) {
            DumpAliases(COMMAND_ALIAS);
            DumpAliases(VAR_ALIAS);
            DumpOn();
        }
    }
    else PrintUsage("DUMP ALIAS | ON | VAR | ALL");
}

/* Adds nick that triggered auto reply to list */
void AddNick2AutoReply(nick)
char *nick;
{
    int found=0;
    int numentr=0;
    int curserv=from_server;
    struct nicks *artmp;
    struct nicks *arstr;
    struct nicks *arcnt;

    if (CheckServer(curserv)) {
        artmp=server_list[curserv].arlist;
        for (arcnt=artmp;arcnt;arcnt=arcnt->next) {
            if (!found) {
                if (!my_stricmp(nick,arcnt->nick)) {
                    found=1;
                    arstr=arcnt;
                }
                else artmp=arcnt;
            }
            numentr++;
        }
        if (found) {
            if (arstr==server_list[curserv].arlist)
                server_list[curserv].arlist=arstr->next;
            else artmp->next=arstr->next;
            new_free(&(arstr->nick));
            new_free(&arstr);
            if (numentr) numentr--;
        }
        else if (numentr==AUTOREPLYSIZE) {
            artmp=server_list[curserv].arlist;
            while (artmp && artmp->next && artmp->next->next) artmp=artmp->next;
            if (artmp && artmp->next) {
                new_free(&(artmp->next->nick));
                new_free(&(artmp->next));
                artmp->next=NULL;
            }
        }
        if ((arstr=(struct nicks *) new_malloc(sizeof(struct nicks)))) {
            arstr->nick=(char *) 0;
            malloc_strcpy(&(arstr->nick),nick);
            arstr->next=server_list[curserv].arlist;
            server_list[curserv].arlist=arstr;
        }
        server_list[curserv].arcur=server_list[curserv].arlist;
    }
}

/* Deletes ScrollZ.away file */
void RemoveLog(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  done;
    char *filepath;
    char *filename;

    filename=get_string_var(AWAY_FILE_VAR);
    if ((filepath=OpenCreateFile(filename,0))) {
        done=remove(filepath);
        if (args && *args && !my_stricmp(args,"OFF")) goto out;
        if (done==0) say("File %s has been deleted",filename);
        else say("Can't delete file %s",filename);
    }
out:
    AwayMsgNum=0;
    update_all_status();
}

/* Automatically changes nick */
void AutoChangeNick(nick)
char *nick;
{
    int nicklen;
    char *tmpstr;
    char *newnick;
    char tmpbuf[mybufsize / 8 + 1];
    char nickbuf[mybufsize / 8 + 1];
    static int  fudge_index = 0;
    static char oldnick[mybufsize / 8 + 1] = {0};

    strmcpy(tmpbuf, nick, sizeof(tmpbuf));
    tmpstr = tmpbuf;
    newnick = new_next_arg(tmpstr, &tmpstr);
    strmcpy(nickbuf, newnick, sizeof(nickbuf));
    if (!(*oldnick)) {
        strmcpy(oldnick, nickbuf, sizeof(oldnick));
        fudge_index = strlen(nickbuf);
    }
    else {
        /* Same nick as the last one we convoluted? */
        if (!my_stricmp(oldnick, nickbuf)) {
            fudge_index++;
            if (fudge_index == 17) {
                char *arglist = NULL;

                reset_nickname(NULL, &arglist);
                fudge_index = 0;
                return;
            }
        }
        else {
            strmcpy(oldnick, nickbuf, sizeof(oldnick));
            fudge_index = strlen(nickbuf);
        }
    }
    if ((nicklen = strlen(nickbuf)) < 9) strmcat(nickbuf, "_", sizeof(nickbuf));
    else {
        char tmp;

        strmcpy(tmpbuf, nickbuf, sizeof(tmpbuf));
        tmp = tmpbuf[8];
        tmpbuf[8] = tmpbuf[7];
        tmpbuf[7] = tmpbuf[6];
        tmpbuf[6] = tmpbuf[5];
        tmpbuf[5] = tmpbuf[4];
        tmpbuf[4] = tmpbuf[3];
        tmpbuf[3] = tmpbuf[2];
        tmpbuf[2] = tmpbuf[1];
        tmpbuf[1] = tmpbuf[0];
        tmpbuf[0] = tmp;
        /* be smart about this -> don't create illegal nick */
        if (check_nickname(tmpbuf)) strmcpy(nickbuf, tmpbuf, sizeof(nickbuf));
        else {
            srand(time(NULL));
            nickbuf[rand() % nicklen] = '_';
        }
    }
    snprintf(tmpbuf, sizeof(tmpbuf), "%d", parsing_server_index);
    strmcpy(oldnick, nickbuf, sizeof(nickbuf));
    nickname_sendline(tmpbuf, nickbuf);
}

/* Colored chat */
void PrintChatMsg(client,line,bytes,iscrypted)
DCC_list *client;
char *line;
int  bytes;
int  iscrypted;
{
    char *thing;
    char tmpbuf[mybufsize/4];
#ifdef WANTANSI
#ifdef TDF
    char tmpbuf2[mybufsize/4];
#endif
#endif
    char thingleft[mybufsize/128];
    char thingright[mybufsize/128];
    void (*func)()=(void(*)())put_it;

    if (Stamp==2) func=(void(*)())say;
#ifdef HAVE_ICONV_H
    if (get_int_var(HIGH_ASCII_VAR)) thing="\342\211\241";
#else
    if (get_int_var(HIGH_ASCII_VAR)) thing="ð";
#endif
    else thing="=";
    snprintf(thingleft,sizeof(thingleft),"%s%s",iscrypted?"*":"",thing);
    snprintf(thingright,sizeof(thingright),"%s%s",thing,iscrypted?"*":"");
#ifdef WANTANSI
    snprintf(tmpbuf,sizeof(tmpbuf),"%s%s%s%s%s%s%s%s%s",
            CmdsColors[COLDCCCHAT].color2,thingleft,Colors[COLOFF],
            CmdsColors[COLDCCCHAT].color1,client->user,Colors[COLOFF],
            CmdsColors[COLDCCCHAT].color2,thingright,Colors[COLOFF]);
#ifdef TDF
    snprintf(tmpbuf2,sizeof(tmpbuf2),"<[%s%s%s]%s%s,%s%s>",
            CmdsColors[COLMSG].color4,update_clock(0,0,GET_TIME),Colors[COLOFF],
            CmdsColors[COLDCC].color4,client->addr,client->port,Colors[COLOFF]);
    func("%s %s%s%s %s",tmpbuf,CmdsColors[COLDCCCHAT].color3,line,Colors[COLOFF],
         tmpbuf2);
#else  /* TDF */
    func("%s %s%s%s",tmpbuf,CmdsColors[COLDCCCHAT].color3,line,Colors[COLOFF]);
#endif /* TDF */
#else  /* WANTANSI */
    func("%s%s%s %s",thingleft,client->user,thingright,line);
#endif /* WANTANSI */
    if (bytes>512 && (away_set || LogOn)) {
        snprintf(tmpbuf,sizeof(tmpbuf),"DCC CHAT from %s is %d bytes long",client->user,bytes);
        AwaySave(tmpbuf,SAVEDCC);
    }
}

/* Prints my chat messages */
void PrintMyChatMsg(nick,line,iscrypted)
char *nick;
char *line;
int  iscrypted;
{
    char *thing;
#ifdef WANTANSI
    char tmpbuf[mybufsize/4];
#endif
    char thingleft[mybufsize/128];
    char thingright[mybufsize/128];
    void (*func)()=(void(*)())put_it;

    if (Stamp==2) func=(void(*)())say;
#ifdef HAVE_ICONV_H
    if (get_int_var(HIGH_ASCII_VAR)) thing="\342\211\241";
#else
    if (get_int_var(HIGH_ASCII_VAR)) thing="ð";
#endif /* HAVE_ICONV_H */
    else thing="=";
    snprintf(thingleft,sizeof(thingleft),"%s%s",iscrypted?"*":"",thing);
    snprintf(thingright,sizeof(thingright),"%s%s",thing,iscrypted?"*":"");
#ifdef WANTANSI
    snprintf(tmpbuf,sizeof(tmpbuf),"%s[%s%s%s%s%s%s%s%s%s%s%s]%s",
            CmdsColors[COLDCCCHAT].color4,Colors[COLOFF],
            CmdsColors[COLDCCCHAT].color2,thingleft,Colors[COLOFF],
            CmdsColors[COLDCCCHAT].color5,nick,Colors[COLOFF],
            CmdsColors[COLDCCCHAT].color2,thingright,Colors[COLOFF],
            CmdsColors[COLDCCCHAT].color4,Colors[COLOFF]);
    func("%s %s%s%s",tmpbuf,CmdsColors[COLDCCCHAT].color3,line,Colors[COLOFF]);
#else
    func("[%s%s%s] %s",thingleft,nick,thingright,line);
#endif
}

#ifdef EXTRAS
/* Outputs line to all channels */
void MSay(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  server;
    char *channels=(char *) 0;
    ChannelList *chan;

    if (args && *args) {
        if (is_channel(args)) channels=new_next_arg(args,&args);
        else channels="*";
        for (chan=server_list[from_server].chan_list;chan;chan=chan->next) {
            server=from_server;
            from_server=chan->server;
            if (CheckChannel(chan->channel,channels))
                send_text(chan->channel,args,NULL);
            from_server=server;
        }
    }
    else PrintUsage("MSAY [#channels] line");
}
#endif

/* Creates ban according to ban type */
void CreateBan(nick, userhost, banstr)
char *nick;
char *userhost;
char *banstr;
{
    int  found = 0;
    int  rate1;
    int  rate2;
    char *tmpstr;
    char *tmpstr2;
    char tmpbuf[mybufsize / 4 + 1];
    char tmpbuf2[mybufsize / 4 + 1];

    if (!userhost) snprintf(tmpbuf2, sizeof(tmpbuf2), "*%s*!*@*", nick);
    else if (defban=='N') snprintf(tmpbuf2, sizeof(tmpbuf2), "%s!%s", nick, userhost);
    else {
        userhost++;
        if (*userhost == '@') userhost--;
        if (defban == 'B') {
            strmcpy(tmpbuf, userhost, sizeof(tmpbuf));
            UserDomainList(tmpbuf);
            snprintf(tmpbuf2, sizeof(tmpbuf2), "*!%s", tmpbuf);
        }
        else if (defban == 'E') {
            srand(time((time_t *) 0));
            strmcpy(tmpbuf, userhost, sizeof(tmpbuf));
            UserDomainList(tmpbuf);
            snprintf(tmpbuf2, sizeof(tmpbuf2), "*!%s", tmpbuf);
            rate1 = 350;
            rate2 = 1;
            for (tmpstr = tmpbuf2; *tmpstr; tmpstr++) {
                if (*tmpstr == '@') {
                    found = 1;
                    rate1 = 500;
                    rate2 = 2;
                }
                if (*tmpstr != '.' && *tmpstr != '@' && *tmpstr != '*' &&
                    *tmpstr != '!' && *tmpstr != ':' && ((rand() % 1000) < 650)) {
                    if (((rand() % 1000) < rate1) || (*(tmpstr - (1 + rand() % rate2)) != '?'))
                        *tmpstr = '?';
                }
            }
        }
        else if (defban == 'H') snprintf(tmpbuf2, sizeof(tmpbuf2), "*!*%s", index(userhost,'@'));
        else if (defban == 'S') {
            tmpstr2 = userhost;
            while ((tmpstr = index(tmpstr2, '.'))) {
                tmpstr2 = tmpstr + 1;
                found++;
            }
            if (found > 1) {
                tmpstr2 = userhost;
                while (found-- > 1) {
                    tmpstr = index(tmpstr2, '.');
                    tmpstr2 = tmpstr + 1;
                }
            }
            else tmpstr = index(userhost, '@') + 1;
            snprintf(tmpbuf2, sizeof(tmpbuf2), "*!*@*%s", tmpstr);
        }
    }
    strcpy(banstr, tmpbuf2);
}

#ifdef MGS_
/* Quits IRC with error level 152 */
void Terminate(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr=NULL;

    IRCQuit=152;
    if (args && *args) tmpstr=args;
    MyQuit(tmpstr);
}
#endif

/* Checks whether join is synched */
void PrintSynch(chan)
ChannelList *chan;
{
    char tmpbuf1[mybufsize / 16];
    char tmpbuf2[mybufsize / 32];
    struct timeval timenow;

    if (!chan->gotbans || !chan->gotwho) return;
    gettimeofday(&timenow, NULL);
    timenow.tv_sec -= chan->time.tv_sec;
    if (timenow.tv_usec >= chan->time.tv_usec) timenow.tv_usec -= chan->time.tv_usec;
    else {
        timenow.tv_usec = timenow.tv_usec-chan->time.tv_usec + 1000000;
        timenow.tv_sec--;
    }
    snprintf(tmpbuf2,sizeof(tmpbuf2), "%06ld", timenow.tv_usec);
    tmpbuf2[3] = '\0';
    snprintf(tmpbuf1,sizeof(tmpbuf1), "%ld.%s", timenow.tv_sec, tmpbuf2);
#ifndef CELEHOOK
    if (do_hook(CHANNEL_SYNCH_LIST, "%s %s", chan->channel, tmpbuf1))
#endif /* CELEHOOK */
#ifdef WANTANSI
        say("Join to %s%s%s is now %ssynched%s (%s seconds)",
            CmdsColors[COLJOIN].color3, chan->channel, Colors[COLOFF],
            CmdsColors[COLJOIN].color4, Colors[COLOFF], tmpbuf1);
#else  /* WANTANSI */
        say("Join to %s is now %csynched%c (%s seconds)",
            chan->channel, bold, bold, tmpbuf1);
#endif /* WANTANSI */
    if (HAS_OPS(chan->status) && (chan->FriendList || chan->BKList))
        HandleGotOps(get_server_nickname(from_server), chan);
    if (chan && chan->ChanLog) {
        char tmpbuf3[mybufsize];

        snprintf(tmpbuf3,sizeof(tmpbuf3), "Join to %s is now synched (%s seconds)", chan->channel, tmpbuf1);
        ChannelLogSave(tmpbuf3, chan);
    }
    /* join next channel in the list */
    if (RateLimitJoin(from_server)) {
        ChannelList *tmpchan = server_list[from_server].ChanPendingList;
        if (tmpchan) {
            server_list[from_server].ChanPendingList = tmpchan->next;
            send_to_server("%s %s %s %s", EMPTY_STR(tmpchan->topicstr),
                           EMPTY_STR(tmpchan->channel), EMPTY_STR(tmpchan->s_mode),
                           EMPTY_STR(tmpchan->key));
            new_free(&tmpchan->channel);
            new_free(&tmpchan->key);
            new_free(&tmpchan->s_mode);
            new_free(&tmpchan->topicstr);
            new_free(&tmpchan);
        }
    }
}

/* Redirects last message/notice you have sent to current or specified channel */
void DirLSM(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  message=!my_stricmp(command,"DIRLSM");
    char *target;
    char *msgsent=server_list[from_server].LastMessageSent;
    char *ntcsent=server_list[from_server].LastNoticeSent;

    if ((message && msgsent) || (!message && ntcsent)) {
        if (!(target=new_next_arg(args,&args))) {
            target=get_channel_by_refnum(0);
            if (!target) {
                NoWindowChannel();
                return;
            }
        }
        if (message) send_text(target,msgsent,"PRIVMSG");
        else send_text(target,ntcsent,"PRIVMSG");
        AddNick2List(target,from_server);
    }
    else say("You haven't sent any %s so far",message?"message":"notice");
}

/* Colorize joiner for /SHOWUSER */
#ifdef WANTANSI
void ColorizeJoiner(joiner,buffer)
NickList *joiner;
char *buffer;
{
    int count=2;
    char *colnick=CmdsColors[COLCSCAN].color5;
    char thing[mybufsize/16];

    if (joiner->shitlist && joiner->shitlist->shit) colnick=CmdsColors[COLCSCAN].color6;
    else if (joiner->frlist && joiner->frlist->privs) colnick=CmdsColors[COLCSCAN].color2;
    else if (joiner->chanop || joiner->halfop) colnick=CmdsColors[COLCSCAN].color3;
    else if (joiner->hasvoice) colnick=CmdsColors[COLCSCAN].color4;
    *thing='\0';
    if (joiner->hasvoice) {
        snprintf(thing,sizeof(thing),"%s+%s",CmdsColors[COLNICK].color5,Colors[COLOFF]);
        count--;
    }
    if (joiner->chanop || joiner->halfop) {
        snprintf(&thing[strlen(thing)],sizeof(thing),"%s%c%s",CmdsColors[COLNICK].color4,joiner->chanop?'@':'%',Colors[COLOFF]);
        count--;
    }
    while (count) {
        sprintf(buffer," %s",thing);
        strmcpy(thing,buffer,sizeof(thing));
        count--;
    }
    sprintf(buffer,"%s%s%-9s%s",thing,colnick,joiner->nick,Colors[COLOFF]);
}
#endif

#define SLSTALL		1
#define SLSTFRND	2
#define SLSTNFRND	3
#define SLSTSHIT	8
#define SLSTNSHIT	16
#define SLSTOP		32
#define SLSTNOP		64
#define SLSTVOIC	128
#define SLSTNVOIC	256
#define SLSTHLFO	512
#define SLSTNHLFO	1024

/* Like internal WHO */
void ShowUser(command,args,subargs)
char *command;
char *args;
char *subargs;
{
#ifdef WANTANSI
    int  len;
#else
    int  count;
#endif
    int  listfl = SLSTALL;
    char *channels = NULL;
    char *tmpstr = NULL;
    char tmpbuf1[mybufsize / 4];
    char tmpbuf2[mybufsize / 4];
    NickList *joiner;
    ChannelList *chan;
    struct nicks *filtlist = NULL, *tmpfilt;

    while ((tmpstr = new_next_arg(args, &args))) {
        if (*tmpstr == '-') {
            tmpstr++;
            if (!(*tmpstr)) continue;
            upper(tmpstr);
            if (*tmpstr == 'O') {
                listfl &= (~SLSTALL);
                listfl |= SLSTOP;
            }
	    if (*tmpstr == 'H') {
		listfl &= (~SLSTALL);
		listfl |= SLSTHLFO;
	    }
            if (*tmpstr == 'F') {
                listfl &= (~SLSTALL);
                listfl |= SLSTFRND;
            }
            if (*tmpstr == 'S') {
                listfl &= (~SLSTALL);
                listfl |= SLSTSHIT;
            }
            if (*tmpstr == 'V') {
                listfl &= (~SLSTALL);
                listfl |= SLSTVOIC;
            }
            if (*tmpstr == 'N') {
                tmpstr++;
                if (!(*tmpstr)) continue;
                if (*tmpstr == 'O') {
                    listfl &= (~SLSTALL);
                    listfl |= SLSTNOP;
                }
		if (*tmpstr == 'H') {
		    listfl &= (~SLSTALL);
		    listfl |= SLSTNHLFO;
		}
                if (*tmpstr == 'F') {
                    listfl &= (~SLSTALL);
                    listfl |= SLSTNFRND;
                }
                if (*tmpstr == 'S') {
                    listfl &= (~SLSTALL);
                    listfl |= SLSTNSHIT;
                }
                if (*tmpstr == 'V') {
                    listfl &= (~SLSTALL);
                    listfl |= SLSTNVOIC;
                }
            }
        }
        else if (*tmpstr == '#') channels=tmpstr;
        else {
            if ((tmpfilt = (struct nicks *) new_malloc(sizeof(struct nicks)))) {
                tmpfilt->nick = (char *) 0;
                tmpfilt->next = filtlist;
                malloc_strcpy(&(tmpfilt->nick), tmpstr);
                filtlist = tmpfilt;
            }
            else say("Malloc failed");
        }
    }
    if (!filtlist) {
        if ((filtlist = (struct nicks *) new_malloc(sizeof(struct nicks)))) {
            filtlist->nick = (char *) 0;
            filtlist->next = (struct nicks *) 0;
            malloc_strcpy(&(filtlist->nick), "*");
        }
        else say("Malloc failed");
    }
    if (!channels) channels = get_channel_by_refnum(0);
    say("%-3s %-15s %-10s %-34s Friend","UL","Channel"," Nick","Userhost");
    for (chan = server_list[curr_scr_win->server].chan_list; chan; chan = chan->next) {
        if (chan->channel && CheckChannel(chan->channel, channels)) {
            for (joiner = chan->nicks; joiner; joiner = joiner->next) {
                if (joiner->userhost) snprintf(tmpbuf1, sizeof(tmpbuf1), "%s!%s",
                                               joiner->nick, joiner->userhost);
                else strmcpy(tmpbuf1, joiner->nick, sizeof(tmpbuf1));
                /* Check for u@h match */
                for (tmpfilt = filtlist; tmpfilt; tmpfilt = tmpfilt->next)
                    if (wild_match(tmpfilt->nick, tmpbuf1)) break;
                if (!tmpfilt) continue;
                /* Check if flags match */
                if (((listfl & SLSTOP) && joiner->chanop) ||
                    ((listfl & SLSTNOP) && !(joiner->chanop)) ||
		    ((listfl & SLSTHLFO) && joiner->halfop) ||
		    ((listfl & SLSTNHLFO) && !(joiner->halfop)) ||
                    ((listfl & SLSTFRND) && joiner->frlist && joiner->frlist->privs) ||
                    ((listfl & SLSTNFRND) && (!(joiner->frlist) || !(joiner->frlist->privs))) ||
                    ((listfl & SLSTVOIC) && joiner->hasvoice) ||
                    ((listfl & SLSTNVOIC) && !(joiner->hasvoice) && !(joiner->chanop)) ||
                    ((listfl & SLSTSHIT) && joiner->shitlist && joiner->shitlist->shit) ||
                    ((listfl & SLSTNSHIT) && (!(joiner->shitlist) || !(joiner->shitlist->shit))) ||
                    ((listfl & SLSTALL))) {
#ifdef WANTANSI
                    ColorizeJoiner(joiner, tmpbuf1);
                    if (joiner->userhost) {
                        ColorUserHost(joiner->userhost, CmdsColors[COLWHO].color2, tmpbuf2, 0);
                        len = strlen(joiner->userhost);
                    }
                    else {
                        len = 0;
                        *tmpbuf2 = '\0';
                    }
                    if (len < 35) {
                        for (; len < 35; len++) strmcat(tmpbuf2, " ", sizeof(tmpbuf2));
                    }
                    /* ensure we have at least one space before flags */
                    else strcat(tmpbuf2, " ");
                    strmcat(tmpbuf2, CmdsColors[COLCSCAN].color2, sizeof(tmpbuf2));
                    BuildPrivs(joiner->frlist, tmpbuf2);
                    strmcat(tmpbuf2,Colors[COLOFF], sizeof(tmpbuf2));
                    say("%-3d %s%-14s%s %s %s",
                        joiner->frlist ? joiner->frlist->number : 0,
                        CmdsColors[COLWHO].color3, chan->channel, Colors[COLOFF],
                        tmpbuf1, tmpbuf2);
#else
                    count = 0;
                    strmcpy(tmpbuf2, "  ", sizeof(tmpbuf2));
                    if (joiner->hasvoice) {
                        tmpbuf2[1] = '+';
                        count++;
                    }
                    if (joiner->chanop || joiner->halfop) {
                        if (count) {
                            tmpbuf2[0] = '+';
                            count = 0;
                        }
			tmpbuf2[1 - count] = joiner->chanop ? '@' : '%';
                    }
                    snprintf(tmpbuf1, sizeof(tmpbuf1), "%s%-9s", tmpbuf2, joiner->nick);
                    *tmpbuf2 = '\0';
                    BuildPrivs(joiner->frlist, tmpbuf2);
                    say("%-3d %-14s %s %-34s %s",
                        joiner->frlist ? joiner->frlist->number : 0, chan->channel,
                        tmpbuf1, joiner->userhost, tmpbuf2);
#endif
                }
            }
        }
    }
    for (; filtlist ;) {
        tmpfilt = filtlist;
        filtlist = filtlist->next;
        new_free(&(tmpfilt->nick));
        new_free(&tmpfilt);
    }
}

#ifdef WANTANSI
/* Sets color interactively */
void SetColor(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  error=0;
    int  colsetting=0;
    char *tmpstr;
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];
    char tmpbuf3[mybufsize/32];

    if (args && *args) {
        tmpstr=new_next_arg(args,&args);
        if (!my_stricmp(tmpstr,"WARNING")) colsetting=COLWARNING;
        else if (!my_stricmp(tmpstr,"JOIN")) colsetting=COLJOIN;
        else if (!my_stricmp(tmpstr,"MSG")) colsetting=COLMSG;
        else if (!my_stricmp(tmpstr,"NOTICE")) colsetting=COLNOTICE;
        else if (!my_stricmp(tmpstr,"NETSPLIT")) colsetting=COLNETSPLIT;
        else if (!my_stricmp(tmpstr,"INVITE")) colsetting=COLINVITE;
        else if (!my_stricmp(tmpstr,"MODE")) colsetting=COLMODE;
        else if (!my_stricmp(tmpstr,"SETTING")) colsetting=COLSETTING;
        else if (!my_stricmp(tmpstr,"LEAVE")) colsetting=COLLEAVE;
        else if (!my_stricmp(tmpstr,"NOTIFY")) colsetting=COLNOTIFY;
        else if (!my_stricmp(tmpstr,"CTCP")) colsetting=COLCTCP;
        else if (!my_stricmp(tmpstr,"KICK")) colsetting=COLKICK;
        else if (!my_stricmp(tmpstr,"DCC")) colsetting=COLDCC;
        else if (!my_stricmp(tmpstr,"WHO")) colsetting=COLWHO;
        else if (!my_stricmp(tmpstr,"WHOIS")) colsetting=COLWHOIS;
        else if (!my_stricmp(tmpstr,"PUBLIC")) colsetting=COLPUBLIC;
        else if (!my_stricmp(tmpstr,"CDCC")) colsetting=COLCDCC;
        else if (!my_stricmp(tmpstr,"LINKS")) colsetting=COLLINKS;
        else if (!my_stricmp(tmpstr,"DCCCHAT")) colsetting=COLDCCCHAT;
        else if (!my_stricmp(tmpstr,"CSCAN")) colsetting=COLCSCAN;
        else if (!my_stricmp(tmpstr,"NICK")) colsetting=COLNICK;
        else if (!my_stricmp(tmpstr,"ME")) colsetting=COLME;
        else if (!my_stricmp(tmpstr,"MISC")) colsetting=COLMISC;
        else if (!my_stricmp(tmpstr,"SBAR")) colsetting=COLSBAR1;
        else if (!my_stricmp(tmpstr,"SBAR2")) colsetting=COLSBAR2;
#ifdef CELECOSM
        else if (!my_stricmp(tmpstr,"CELE")) colsetting=COLCELE;
#endif
#ifdef OPERVISION
        else if (!my_stricmp(tmpstr,"OV")) colsetting=COLOV;
#endif
        else error=1;
        if (error) PrintUsage("COLOR SETTING COLOR1 COLOR2 COLOR3 COLOR4 COLOR5");
        else {
            if (args && *args) SetColors(colsetting,&args,&error,0);
            snprintf(tmpbuf1,sizeof(tmpbuf1),"Color for %s",tmpstr);
            GetColors(colsetting,tmpbuf2);
            snprintf(tmpbuf3,sizeof(tmpbuf3),"  [%d]",colsetting+1);
            PrintSetting(tmpbuf1,tmpbuf2,tmpbuf3,empty_string);
            if (colsetting==COLSBAR1 || colsetting==COLSBAR2) build_status((char *) 0);
        }
    }
    else PrintUsage("COLOR SETTING COLOR1 COLOR2 COLOR3 COLOR4 COLOR5");
}
#endif

/*
void GroupServers(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr;
    int  isnumber;
    int  low;
    int  high;

    if (args && *args) {
        while (*args && isspace(*args)) args++;
        if (*args) {
            tmpstr=args;
            while (*tmpstr) {
                isnumber=1;
                low=0;
                while (*tmpstr && *tmpstr!='-') {
                    isnumber=isnumber?isdigit(*tmpstr):0;
                    low=low*10+(*tmpstr-'0');
                    tmpstr++;
                }
                if (!isnumber) {
                    say("First part of the group should be number");
                    return;
                }
                if (*tmpstr=='-') tmpstr++;
                else {
                    say("- should follow first number");
                    return;
                }
                isnumber=1;
                high=0;
                while (*tmpstr && !isspace(*tmpstr)) {
                    isnumber=isnumber?isdigit(*tmpstr):0;
                    high=high*10+(*tmpstr-'0');
                    tmpstr++;
                }
                if (!isnumber) {
                    say("Second part of the group should be number");
                    return;
                }
                if (low>high) {
                    say("Second part of the group should be larger than first part");
                    return;
                }
                if (high>=number_of_servers) {
                    say("Second part should be smaller than number of servers");
                    return;
                }
                while (*tmpstr && isspace(*tmpstr)) tmpstr++;
            }
            say("Servers are now grouped as %s",args);
            malloc_strcpy(&ServerGroups,args);
        }
    }
    else PrintSetting("Server group",ServerGroups,empty_string,empty_string);
}

void CheckGroup(servernum)
int servernum;
{
    char *tmpstr;
    char *tmpbuf;
    int  low;
    int  high;
    int  start;
    int  count;
    int  visible;
    Window *tmp;
    Window *old;
    Window *tmp2;
    Screen *screen;

    if (*ServerGroups=='*') {
        FServer(NULL,"+",NULL);
        return;
    }
    strcpy(tmpbuf1,ServerGroups);
    tmpbuf=tmpbuf1;
    while ((tmpstr=new_next_arg(tmpbuf,&tmpbuf))) {
        low=atoi(tmpstr);
        while (*tmpstr && *tmpstr!='-') tmpstr++;
        if (*tmpstr=='-') {
            tmpstr++;
            high=atoi(tmpstr);
            if (low<=servernum && servernum<=high) {
                start=servernum+1;
                if (start>high) start=low;
                count=high-low+1;
                old=current_screen->current_window;
                if ((tmp=find_window(servernum))) {
                    visible=tmp->visible;
                    if (!visible) {
                        snprintf(tmpbuf2,sizeof(tmpbuf2),"SHOW %d",tmp->refnum);
                        window(NULL,tmpbuf2,NULL);
                    }
                    set_current_window(tmp);
                    while (count) {
                        if (!connect_to_server(get_server_name(start),
                                               server_list[start].port,-1)) {
                            for (screen=screen_list;screen;screen=screen->next) {
                                for (tmp2=screen->window_list;tmp2;tmp2=tmp2->next)
                                    if (tmp2->oldserver==servernum) tmp2->server=start;
                            }
                            for (tmp2=invisible_list;tmp2;tmp2=tmp2->next)
                                if (tmp2->oldserver==servernum) tmp2->server=start;
                            new_free(&tmp->current_channel);
                            update_all_status();
                            if (!visible) {
                                snprintf(tmpbuf2,sizeof(tmpbuf2),"HIDE");
                                window(NULL,tmpbuf2,NULL);
                            }
                            set_current_window(old);
                            return;
                        }
                        start++;
                        if (start>high) start=low;
                        count--;
                    }
                    set_current_window(old);
                }
                return;
            }
        }
    }
}
*/

/* Toggles notify mode brief/verbose */
void NotifyModeToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr=(char *) 0;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr) {
        if (!my_strnicmp("B",tmpstr,1)) NotifyMode=1;
        else if (!my_strnicmp("V",tmpstr,1)) NotifyMode=2;
        else {
            PrintUsage("NTFYMODE B[rief]/V[erbose]");
            return;
        }
    }
    if (NotifyMode==1) PrintSetting("Notify mode","Brief",empty_string,empty_string);
    else PrintSetting("Notify mode","Verbose",empty_string,empty_string);
}

/* Pings yourself */
void PingMe(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    struct timeval timenow;

    gettimeofday(&timenow,NULL);
    send_ctcp(ctcp_type[CTCP_PRIVMSG],get_server_nickname(from_server),"PING","\%ld \%ld",
              timenow.tv_sec,timenow.tv_usec);
}

/* Notepad - in case you don't have pen & paper handy... */
#ifndef LITE
void NotePad(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int  oldumask=umask(0177);
    char *filepath;
    FILE *notefile;
    time_t now;
    
    if (args && *args) {
        filepath=OpenCreateFile("ScrollZ.notepad",1);
        if (!filepath || (notefile=fopen(filepath,"a"))==NULL)
            say("Can't open file ScrollZ.notepad");
        else {
            now=time((time_t *) 0);
            fprintf(notefile,"# %s [%.24s]\n",args,ctime(&now));
            fclose(notefile);
            say("Successful addition to NotePad (%c%s%c)",bold,filepath,bold);
        }
    } else PrintUsage("NOTEPAD <Text to NotePad>");
    umask(oldumask);
}
#endif

/* Toggles URL catcher on/off */
void URLCatchToggle(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *tmpstr=(char *) 0;

    tmpstr=new_next_arg(args,&args);
    if (tmpstr) {
        if (!my_stricmp("QUIET",tmpstr)) URLCatch=3;
        else if (!my_stricmp("AUTO",tmpstr)) URLCatch=2;
        else if (!my_stricmp("ON",tmpstr)) URLCatch=1;
        else if (!my_stricmp("OFF",tmpstr)) URLCatch=0;
        else {
            PrintUsage("URLCATCH on/auto/quiet/off");
            return;
        }
    }
    if (URLCatch==3) PrintSetting("URL catcher","QUIET",empty_string,empty_string);
    else if (URLCatch==2) PrintSetting("URL catcher","AUTO",empty_string,empty_string);
    else if (URLCatch) PrintSetting("URL catcher","ON",empty_string,empty_string);
    else PrintSetting("URL catcher","OFF",empty_string,empty_string);
}

/* Prints head of /LINKS */
#ifdef WANTANSI
void PrintLinksHead() {
    char tmpbuf1[mybufsize/4];
    char tmpbuf2[mybufsize/4];

    if (get_int_var(HIGH_ASCII_VAR)) {
#ifdef HAVE_ICONV_H
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%s\342\224\214%s\302\267%sNo%s\302\267%s\342\224\220 \342\224\214\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200%s\302\267%sServer%s\302\267%s\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\220%s",
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color1,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color1,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF]);
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s\342\224\214%s\302\267%sDs%s\302\267%s\342\224\220   \342\224\214\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200%s\302\267%sUplink%s\302\267%s\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\200\342\224\220%s",
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color3,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color2,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF]);
#else
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%sÚ%sú%sNo%sú%s¿ ÚÄÄÄÄÄÄÄÄÄ%sú%sServer%sú%sÄÄÄÄÄÄÄÄÄ¿%s",
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color1,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color1,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF]);
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%sÚ%sú%sDs%sú%s¿   ÚÄÄÄÄÄÄÄÄ%sú%sUplink%sú%sÄÄÄÄÄÄÄÄÄ¿%s",
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color3,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color2,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF]);
#endif /* HAVE_ICONV_H */
    }
    else {
        snprintf(tmpbuf1,sizeof(tmpbuf1),"%s.%s-%sNo%s-%s. .---------%s-%sServer%s-%s---------.%s",
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color1,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color1,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF]);
        snprintf(tmpbuf2,sizeof(tmpbuf2),"%s.%s-%sDs%s-%s.   .--------%s-%sUplink%s-%s---------.%s",
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color3,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF],
                CmdsColors[COLLINKS].color2,Colors[COLOFF],
                CmdsColors[COLLINKS].color5,Colors[COLOFF]);
    }
    say("%s %s",tmpbuf1,tmpbuf2);
}
#endif

/* URL command coded by Zakath */
void URLSave(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    int count=0;
    struct urlstr *tmpurl;

    if (args && *args) {
        if (!my_stricmp(args,"-CLEAR")) {
            for (;urllist;) {
                tmpurl=urllist;
                urllist=urllist->next;
                new_free(&(tmpurl->urls));
                new_free(&(tmpurl->source));
                new_free(&tmpurl);
            }
            return;
        }
        malloc_strcpy(&urlbuf,args);
        add_wait_prompt("Description of URL: ",URLSave3,args,WAIT_PROMPT_LINE);
    }
    else {
 	if (!urllist) say("There are currently no URLs stored in memory.");
 	else {
 	    say("List of all URLs currently stored in memory:");
 	    for (tmpurl=urllist;tmpurl;tmpurl=tmpurl->next,count++) {
 		say("%-2d %c%s%c %s",
                    count,bold,tmpurl->urls,bold,
                    tmpurl->source?tmpurl->source:empty_string);
            }
            say("A total of %d URL%s stored in memory.",count,count==1?"":"s");
 	    add_wait_prompt("Type the # of the URL to save, any other key to abort -> ",URLSave2,args,WAIT_PROMPT_LINE);
 	}
    }
}

/* Prompts for URL description */
void URLSave2(stuff,line)
char *stuff;
char *line;
{
    int found = 0;
    long count = 1;
    long urlnum;
    char *endptr;
    struct urlstr *tmpurl;

    urlnum = strtol(line, &endptr, 10);
    if (line && *line && (endptr != NULL)) {
        for (tmpurl = urllist; tmpurl; tmpurl = tmpurl->next, count++)
            if (count == urlnum) {
                malloc_strcpy(&urlbuf, tmpurl->urls);
                if (tmpurl->source) {
                    malloc_strcat(&urlbuf, " ");
                    malloc_strcat(&urlbuf, tmpurl->source);
                }
                found = 1;
                break;
            }
        if (found) add_wait_prompt("Description of URL: ", URLSave3, stuff, WAIT_PROMPT_LINE);
        else say("Could not find the URL with that number");
    }
}

/* Final part, actually does saving */
void URLSave3(blah,args)
char *blah;
char *args;
{
    int  oldumask=umask(0177);
    char *filepath;
    char tmpbuf[mybufsize/4];
    FILE *notefile;
    time_t now;

    if (!(args && *args)) strcpy(tmpbuf,"None given.");
    else strmcpy(tmpbuf,args,sizeof(tmpbuf));
    filepath=OpenCreateFile("ScrollZ.notepad",1);
    if (!filepath || (notefile=fopen(filepath,"a"))==NULL)
        say("Can't open file ScrollZ.notepad");
    else {
        now=time((time_t *) 0);
        fprintf(notefile,"## %s [%.24s]\n",urlbuf,ctime(&now));
        fprintf(notefile,"# Desc: %s\n",tmpbuf);
        fclose(notefile);
        say("Added URL (%c%s%c) to ScrollZ NotePad.",bold,urlbuf,bold);
    }
    new_free(&urlbuf);
    umask(oldumask);
}

#ifdef WANTANSI
/* Counts ansi chars in string */
int CountAnsiInput(str,len)
char *str;
int len;
{
    register int  count=0;
    register int  x=0;
    register char *ptr;

    if (!str) return(0);
    FixColorAnsi(str);
    for (ptr=str;*ptr && x<len;ptr++)
        if (vt100Decode(*ptr)) count++;
        else x++;
    return(count);
}
#endif

#ifdef SORTED_NICKS
/* Used to insert into sorted linked list */
int SortedCmp(elmt1,elmt2)
List *elmt1;
List *elmt2;
{
    char *nick1;
    char *nick2;
    char char1;
    char char2;
    int  value=0;
    int  xor;

    nick1=elmt1->name;
    nick2=elmt2->name;
    while (*nick1 && *nick2) {
        xor=(*nick1)^(*nick2);
        if (xor && xor!=32) {
            char1=toupper(*nick1);
            char2=toupper(*nick2);
            value=(char1-char2)>0?1:0;
            break;
        }
        nick1++;
        nick2++;
    }
    if (*nick1 && !(*nick2)) value=1;
    return(value);
}
#endif

/* BanKicks the last joined person */
void LastJoinerKick() {
    char *channel;
    char *lastjoin;
    char tmpbuf[mybufsize/4+1];

    if (!CheckServer(from_server)) {
        say("You are not connected to a server. Use /SERVER to connect.");
        return;
    }
    lastjoin=server_list[from_server].LastJoin;
    if (lastjoin && strcmp(lastjoin,"none yet")) {
        strmcpy(tmpbuf,lastjoin,mybufsize/4);
        channel=index(tmpbuf,'/');
        if (channel) {
            *channel=0;
            if (!my_stricmp(tmpbuf,get_server_nickname(from_server))) return;
            *channel=' ';
            BanKick("BK",tmpbuf,NULL);
        }
    }
    else say("No join so far");
}

/* Accepts chat from last user that has requested it */
void AcceptLastChat() {
    if (LastChat) dcc_chat(LastChat);
    else say("No chat request so far");
}

/* This bolds out URLs and copies them to buffer */
int GrabURL(line, buffer, bufsize, filepath, source, colour)
char *line;
char *buffer;
size_t bufsize;
char *filepath;
char *source;
char *colour;
{
    int  urlnum = 0;
    int  saveit;
    char store;
    char tmpbuf1[mybufsize];
    char tmpbuf2[mybufsize / 2];
    char *tmpstr1 = tmpbuf1;
    char *tmpstr2 = buffer;
    char *tmpstr3;
    FILE *notefile;
    time_t now;
    struct urlstr *urlnew, *tmpurl, *prevurl = NULL;

    strmcpy(tmpbuf1, line, sizeof(tmpbuf1));
    *tmpstr2 = '\0';
    while (*tmpstr1) {
        while (isspace(*tmpstr1)) *tmpstr2 ++= *tmpstr1++;
        *tmpstr2 = '\0';
        if (*tmpstr1) {
            for (tmpstr3 = tmpstr1; *tmpstr3 && !isspace(*tmpstr3); tmpstr3++);
            store = *tmpstr3;
            *tmpstr3 = '\0';
            if ((!my_strnicmp(tmpstr1, "http://", 7) || !my_strnicmp(tmpstr1, "ftp://", 6) ||
                 !my_strnicmp(tmpstr1, "https://", 8) ||
                 !my_strnicmp(tmpstr1, "www.", 4) || !my_strnicmp(tmpstr1, "ftp.", 4))) {
                saveit = 1;
                prevurl = NULL;
                for (tmpurl = urllist; tmpurl; tmpurl = tmpurl->next) {
                    if (!my_stricmp(tmpurl->urls, tmpstr1)) break;
                    prevurl = tmpurl;
                }
                if (tmpurl) {
                    if (prevurl) prevurl->next = tmpurl->next;
                    else urllist = tmpurl->next;
                    for (urlnew = urllist; urlnew && urlnew->next;) urlnew = urlnew->next;
                    if (urlnew) urlnew->next = tmpurl;
                    else urllist = tmpurl;
                    tmpurl->next = NULL;
                    saveit = 0;
                }
#ifdef WANTANSI
                snprintf(tmpbuf2, sizeof(tmpbuf2), "%s%s%s",
                         CmdsColors[COLMISC].color6, tmpstr1, colour);
#else
                snprintf(tmpbuf2, sizeof(tmpbuf2), "%c%s%c",
                         bold, tmpstr1, bold);
#endif
                strmcat(tmpstr2, tmpbuf2, bufsize - 1);
                tmpstr2 += strlen(tmpbuf2);
                /* Add URL to list */
                if (saveit) {
                    tmpurl = urllist;
                    urlnew = (struct urlstr *) new_malloc(sizeof(struct urlstr));
                    urlnew->urls = NULL;
                    urlnew->source = NULL;
                    urlnew->next = NULL;
                    malloc_strcpy(&(urlnew->urls), tmpstr1);
                    if (source) malloc_strcpy(&(urlnew->source), source);
                    URLnum++;
                    if (URLnum > get_int_var(URL_BUFFER_SIZE_VAR)) {
                        urllist = tmpurl->next;
                        new_free(&(tmpurl->urls));
                        new_free(&(tmpurl->source));
                        new_free(&tmpurl);
                        URLnum--;
                    }
                    add_to_list_ext((List **) &urllist,(List *) urlnew,
                                    (int (*) _((List *, List *))) AddLast);
                    /* if URL Catcher is set to auto... */
                    if (URLCatch >= 2 && filepath) {
                        int oldumask = umask(0177);

                        if ((notefile = fopen(filepath, "a")) != NULL) {
                            now = time(NULL);
                            fprintf(notefile,"## %s %s [%.24s]\n",
                                    urlnew->urls, urlnew->source, ctime(&now));
                            fclose(notefile);
                            urlnum++;
                        }
                        umask(oldumask);
                    }
                }
            }
            else {
                strcat(tmpstr2, tmpstr1);
                tmpstr2 += strlen(tmpstr1);
            }
            tmpstr1 = tmpstr3;
            if (store) *tmpstr1 = store;
        }
    }
    return(urlnum);
}

/* Prints names on channel join */
void PrintNames(channel,nicks,chan)
char *channel;
char *nicks;
ChannelList *chan;
{
#ifdef WANTANSI
    int  addspace = 0;
    char *nick = NULL;
    char *tmpstr = NULL;
    char tmpbuf[mybufsize];
    char buffer[BIG_BUFFER_SIZE + 1];

    strmcpy(tmpbuf, nicks, sizeof(tmpbuf));
    tmpstr = tmpbuf;
    *buffer = '\0';
    while ((nick = new_next_arg(tmpstr, &tmpstr))) {
        if (addspace) strmcat(buffer, " ", sizeof(buffer));
        else addspace = 1;
        if (*nick == '@') {
            strmcat(buffer, CmdsColors[COLNICK].color4, sizeof(buffer));
            strmcat(buffer, "@", sizeof(buffer));
            strmcat(buffer, Colors[COLOFF], sizeof(buffer));
            strmcat(buffer, CmdsColors[COLCSCAN].color3, sizeof(buffer));
            nick++;
        }
	else if (*nick == '%') {
	    strmcat(buffer, CmdsColors[COLNICK].color4, sizeof(buffer));
	    strmcat(buffer, "%", sizeof(buffer));
	    strmcat(buffer, Colors[COLOFF], sizeof(buffer));
	    strmcat(buffer, CmdsColors[COLCSCAN].color3, sizeof(buffer));
	    nick++;
	}
        else if (*nick == '+') {
            strmcat(buffer, CmdsColors[COLNICK].color5, sizeof(buffer));
            strmcat(buffer, "+", sizeof(buffer));
            strmcat(buffer, Colors[COLOFF], sizeof(buffer));
            strmcat(buffer, CmdsColors[COLCSCAN].color4, sizeof(buffer));
            nick++;
        }
        else strmcat(buffer, CmdsColors[COLCSCAN].color5, sizeof(buffer));
        strmcat(buffer, nick, sizeof(buffer));
        strmcat(buffer, Colors[COLOFF], sizeof(buffer));
        if (strlen(buffer) >= BIG_BUFFER_SIZE - 100) {
            say("Users on %s%s%s: %s", CmdsColors[COLCSCAN].color1, channel, Colors[COLOFF], buffer);
            *buffer = '\0';
        }
    }
    if (*buffer)
        say("Users on %s%s%s: %s", CmdsColors[COLCSCAN].color1, channel, Colors[COLOFF], buffer);
#else
    say("Users on %s: %s", channel, nicks);
#endif
    if (chan && chan->ChanLog) {
        char tmpbuf[4 * mybufsize];

        snprintf(tmpbuf,sizeof(tmpbuf), "Users on %s: %s", channel, nicks);
        ChannelLogSave(tmpbuf, chan);
    }
}

/* Initializes keys and default color scheme */
void InitKeysColors() {
#ifdef WANTANSI
    int i;
#endif
    char tmpbuf[mybufsize / 4];

    strcpy(tmpbuf, "^F parse_command wholeft");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F1 parse_command help keys");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F2 parse_command join -invite");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^W? parse_command help net");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wh parse_command window hide");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wn parse_command window next");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wp parse_command window previous");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wk parse_command window kill");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wl parse_command window list");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wc parse_command clear");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^W1 parse_command window goto 1");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^W2 parse_command window goto 2");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^W3 parse_command window goto 3");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^W4 parse_command window goto 4");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^W5 parse_command window goto 5");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-1 parse_command ^window swap 1");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-2 parse_command ^window swap 2");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-3 parse_command ^window swap 3");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-4 parse_command ^window swap 4");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-5 parse_command ^window swap 5");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-6 parse_command ^window swap 6");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-7 parse_command ^window swap 7");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-8 parse_command ^window swap 8");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-9 parse_command ^window swap 9");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wg parse_command window grow 1");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^Wr parse_command window shrink 1");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-i insert_tabkey_prev");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "meta1-q push_line");
    bindcmd(NULL, tmpbuf, NULL);

#ifdef CELE /* Celerity Binds */
    strcpy(tmpbuf, "^K erase_to_end_of_line");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^O parse_command dirlm");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "^D delete_character");
    bindcmd(NULL, tmpbuf, NULL);
/*    strcpy(tmpbuf, "^J enter_digraph");
    bindcmd(NULL, tmpbuf, NULL);*/
    strcpy(tmpbuf, "^T switch_channels");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F2 parse_command cscan");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F3 parse_command clear");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F4 parse_command cdcc");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F5 parse_command digraph");
    bindcmd(NULL, tmpbuf, NULL);
    strcpy(tmpbuf, "F6 parse_command quickstat");
    bindcmd(NULL, tmpbuf, NULL);
#endif /* CELE */

#ifdef WANTANSI
    for (i = 0; i < NUMCMDCOLORS; i++) {
        CmdsColors[i].color1 = CmdsColors[i].color1_str = NULL;
        CmdsColors[i].color2 = CmdsColors[i].color2_str = NULL;
        CmdsColors[i].color3 = CmdsColors[i].color3_str = NULL;
        CmdsColors[i].color4 = CmdsColors[i].color4_str = NULL;
        CmdsColors[i].color5 = CmdsColors[i].color5_str = NULL;
        CmdsColors[i].color6 = CmdsColors[i].color6_str = NULL;
        malloc_strcpy(&CmdsColors[i].color1_str, ColorName(COLWHITE));
        malloc_strcpy(&CmdsColors[i].color2_str, ColorName(COLWHITE));
        malloc_strcpy(&CmdsColors[i].color3_str, ColorName(COLWHITE));
        malloc_strcpy(&CmdsColors[i].color4_str, ColorName(COLWHITE));
        malloc_strcpy(&CmdsColors[i].color5_str, ColorName(COLWHITE));
        malloc_strcpy(&CmdsColors[i].color6_str, ColorName(COLWHITE));
    }
    /* Warnings - floods, errors in C-Toolz.save, mass commands, */
    /*            protection violations */
    /* Warning itself */
    InitColor(&CmdsColors[COLWARNING].color1_str, &CmdsColors[COLWARNING].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLWARNING].color1_str, &CmdsColors[COLWARNING].color1, COLFLASH, 1);
    InitColor(&CmdsColors[COLWARNING].color1_str, &CmdsColors[COLWARNING].color1, COLRED, 1);
    /* Nick */
    InitColor(&CmdsColors[COLWARNING].color2_str, &CmdsColors[COLWARNING].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLWARNING].color2_str, &CmdsColors[COLWARNING].color2, COLWHITE, 1);
    /* Userhost */
    InitColor(&CmdsColors[COLWARNING].color3_str, &CmdsColors[COLWARNING].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLWARNING].color3_str, &CmdsColors[COLWARNING].color3, COLYELLOW, 1);
    /* Channel */
    InitColor(&CmdsColors[COLWARNING].color4_str, &CmdsColors[COLWARNING].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLWARNING].color4_str, &CmdsColors[COLWARNING].color4, COLCYAN, 1);

    /* Joins */
    /* Nick */
    InitColor(&CmdsColors[COLJOIN].color1_str, &CmdsColors[COLJOIN].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLJOIN].color2_str, &CmdsColors[COLJOIN].color2, COLPURPLE, 0);
    /* Channel */
    InitColor(&CmdsColors[COLJOIN].color3_str, &CmdsColors[COLJOIN].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLJOIN].color3_str, &CmdsColors[COLJOIN].color3, COLCYAN, 1);
    /* Synched */
    InitColor(&CmdsColors[COLJOIN].color4_str, &CmdsColors[COLJOIN].color4, COLWHITE, 0);
    /* Friends */
    InitColor(&CmdsColors[COLJOIN].color5_str, &CmdsColors[COLJOIN].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLJOIN].color5_str, &CmdsColors[COLJOIN].color5, COLCYAN, 1);
    /* Shitted */
    InitColor(&CmdsColors[COLJOIN].color6_str, &CmdsColors[COLJOIN].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLJOIN].color6_str, &CmdsColors[COLJOIN].color6, COLRED, 1);

    /* MSGs */
    /* Nick */
    InitColor(&CmdsColors[COLMSG].color1_str, &CmdsColors[COLMSG].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLMSG].color1_str, &CmdsColors[COLMSG].color1, COLCYAN, 1);
    /* Userhost */
    InitColor(&CmdsColors[COLMSG].color2_str, &CmdsColors[COLMSG].color2, COLPURPLE, 0);
    /* Message */
    InitColor(&CmdsColors[COLMSG].color3_str, &CmdsColors[COLMSG].color3, COLWHITE, 0);
    /* Time */
    InitColor(&CmdsColors[COLMSG].color4_str, &CmdsColors[COLMSG].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLMSG].color4_str, &CmdsColors[COLMSG].color4, COLBLACK, 1);
    /* [] */
    InitColor(&CmdsColors[COLMSG].color5_str, &CmdsColors[COLMSG].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLMSG].color5_str, &CmdsColors[COLMSG].color5, COLCYAN, 1);
    /* Nick you sent message to */
    InitColor(&CmdsColors[COLMSG].color6_str, &CmdsColors[COLMSG].color6, COLCYAN, 0);

    /* Notices */
    /* Nick */
    InitColor(&CmdsColors[COLNOTICE].color1_str, &CmdsColors[COLNOTICE].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLNOTICE].color1_str, &CmdsColors[COLNOTICE].color1, COLGREEN, 1);
    /* Nick you send notice to */
    InitColor(&CmdsColors[COLNOTICE].color2_str, &CmdsColors[COLNOTICE].color2, COLGREEN, 0);
    /* Message */
    InitColor(&CmdsColors[COLNOTICE].color3_str, &CmdsColors[COLNOTICE].color3, COLWHITE, 0);
    /* <> */
    InitColor(&CmdsColors[COLNOTICE].color4_str, &CmdsColors[COLNOTICE].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLNOTICE].color4_str, &CmdsColors[COLNOTICE].color4, COLGREEN, 1);
    /* - in received notice */
    InitColor(&CmdsColors[COLNOTICE].color5_str, &CmdsColors[COLNOTICE].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLNOTICE].color5_str, &CmdsColors[COLNOTICE].color5, COLWHITE, 1);
    /* Userhost in Celerity */
    InitColor(&CmdsColors[COLNOTICE].color6_str, &CmdsColors[COLNOTICE].color6, COLCYAN, 0);

    /* Netsplits, netjoins */
    /* Message */
    InitColor(&CmdsColors[COLNETSPLIT].color1_str, &CmdsColors[COLNETSPLIT].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLNETSPLIT].color1_str, &CmdsColors[COLNETSPLIT].color1, COLWHITE, 1);
    /* Time */
    InitColor(&CmdsColors[COLNETSPLIT].color2_str, &CmdsColors[COLNETSPLIT].color2, COLWHITE, 0);
    /* Servers */
    InitColor(&CmdsColors[COLNETSPLIT].color3_str, &CmdsColors[COLNETSPLIT].color3, COLWHITE, 0);
    /* Channel */
    InitColor(&CmdsColors[COLNETSPLIT].color4_str, &CmdsColors[COLNETSPLIT].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLNETSPLIT].color4_str, &CmdsColors[COLNETSPLIT].color4, COLCYAN, 1);
    /* Nicks */
    InitColor(&CmdsColors[COLNETSPLIT].color5_str, &CmdsColors[COLNETSPLIT].color5, COLCYAN, 0);
    /* <- */
    InitColor(&CmdsColors[COLNETSPLIT].color6_str, &CmdsColors[COLNETSPLIT].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLNETSPLIT].color6_str, &CmdsColors[COLNETSPLIT].color6, COLYELLOW, 1);

    /* Invites */
    /* Nick */
    InitColor(&CmdsColors[COLINVITE].color1_str, &CmdsColors[COLINVITE].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLINVITE].color2_str, &CmdsColors[COLINVITE].color2, COLPURPLE, 0);
    /* Channel */
    InitColor(&CmdsColors[COLINVITE].color3_str, &CmdsColors[COLINVITE].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLINVITE].color3_str, &CmdsColors[COLINVITE].color3, COLCYAN, 1);
    /* fake word */
    InitColor(&CmdsColors[COLINVITE].color4_str, &CmdsColors[COLINVITE].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLINVITE].color4_str, &CmdsColors[COLINVITE].color4, COLRED, 1);

    /* Mode changes */
    /* Nick */
    InitColor(&CmdsColors[COLMODE].color1_str, &CmdsColors[COLMODE].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLMODE].color2_str, &CmdsColors[COLMODE].color2, COLWHITE, 0);
    /* Channel */
    InitColor(&CmdsColors[COLMODE].color3_str, &CmdsColors[COLMODE].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLMODE].color3_str, &CmdsColors[COLMODE].color3, COLCYAN, 1);
    /* Mode */
    InitColor(&CmdsColors[COLMODE].color4_str, &CmdsColors[COLMODE].color4, COLWHITE, 0);
    /* Message */
    InitColor(&CmdsColors[COLMODE].color5_str, &CmdsColors[COLMODE].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLMODE].color5_str, &CmdsColors[COLMODE].color5, COLWHITE, 1);
    /* Fake word */
    InitColor(&CmdsColors[COLMODE].color6_str, &CmdsColors[COLMODE].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLMODE].color6_str, &CmdsColors[COLMODE].color6, COLRED, 1);

    /* Settings */
    /* Header */
    InitColor(&CmdsColors[COLSETTING].color1_str, &CmdsColors[COLSETTING].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLSETTING].color1_str, &CmdsColors[COLSETTING].color1, COLWHITE, 1);
    /* Setting - ON,OFF,5,#te,... */
    InitColor(&CmdsColors[COLSETTING].color2_str, &CmdsColors[COLSETTING].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLSETTING].color2_str, &CmdsColors[COLSETTING].color2, COLPURPLE, 1);
    /* Comment for shit list */
    InitColor(&CmdsColors[COLSETTING].color3_str, &CmdsColors[COLSETTING].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLSETTING].color3_str, &CmdsColors[COLSETTING].color3, COLYELLOW, 1);
    /* Userhost */
    InitColor(&CmdsColors[COLSETTING].color4_str, &CmdsColors[COLSETTING].color4, COLPURPLE, 0);
    /* Channels */
    InitColor(&CmdsColors[COLSETTING].color5_str, &CmdsColors[COLSETTING].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLSETTING].color5_str, &CmdsColors[COLSETTING].color5, COLCYAN, 1);

    /* Leaves */
    /* Nick */
    InitColor(&CmdsColors[COLLEAVE].color1_str, &CmdsColors[COLLEAVE].color1, COLCYAN, 0);
    /* Channel */
    InitColor(&CmdsColors[COLLEAVE].color2_str, &CmdsColors[COLLEAVE].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLLEAVE].color2_str, &CmdsColors[COLLEAVE].color2, COLCYAN, 1);
    /* Reason */
    InitColor(&CmdsColors[COLLEAVE].color3_str, &CmdsColors[COLLEAVE].color3, COLWHITE, 0);
    /* Friends */
    InitColor(&CmdsColors[COLLEAVE].color4_str, &CmdsColors[COLLEAVE].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLLEAVE].color4_str, &CmdsColors[COLLEAVE].color4, COLCYAN, 1);
    /* Shitted */
    InitColor(&CmdsColors[COLLEAVE].color5_str, &CmdsColors[COLLEAVE].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLLEAVE].color5_str, &CmdsColors[COLLEAVE].color5, COLRED, 1);

    /* Notify */
    /* Nick */
    InitColor(&CmdsColors[COLNOTIFY].color1_str, &CmdsColors[COLNOTIFY].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLNOTIFY].color2_str, &CmdsColors[COLNOTIFY].color2, COLPURPLE, 0);
    /* Time */
    InitColor(&CmdsColors[COLNOTIFY].color3_str, &CmdsColors[COLNOTIFY].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLNOTIFY].color3_str, &CmdsColors[COLNOTIFY].color3, COLBLACK, 1);
    /* Message */
    InitColor(&CmdsColors[COLNOTIFY].color4_str, &CmdsColors[COLNOTIFY].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLNOTIFY].color4_str, &CmdsColors[COLNOTIFY].color4, COLWHITE, 1);
    /* Signon-ed nicks */
    InitColor(&CmdsColors[COLNOTIFY].color5_str, &CmdsColors[COLNOTIFY].color5, COLCYAN, 0);
    /* Signon-ed friends */
    InitColor(&CmdsColors[COLNOTIFY].color6_str, &CmdsColors[COLNOTIFY].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLNOTIFY].color6_str, &CmdsColors[COLNOTIFY].color6, COLCYAN, 1);

    /* CTCPs */
    /* Nick */
    InitColor(&CmdsColors[COLCTCP].color1_str, &CmdsColors[COLCTCP].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLCTCP].color2_str, &CmdsColors[COLCTCP].color2, COLPURPLE, 0);
    /* Channel */
    InitColor(&CmdsColors[COLCTCP].color3_str, &CmdsColors[COLCTCP].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLCTCP].color3_str, &CmdsColors[COLCTCP].color3, COLCYAN, 1);
    /* Command */
    InitColor(&CmdsColors[COLCTCP].color4_str, &CmdsColors[COLCTCP].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLCTCP].color4_str, &CmdsColors[COLCTCP].color4, COLCYAN, 1);

    /* Kicks */
    /* Nick */
    InitColor(&CmdsColors[COLKICK].color1_str, &CmdsColors[COLKICK].color1, COLCYAN, 0);
    /* Who */
    InitColor(&CmdsColors[COLKICK].color2_str, &CmdsColors[COLKICK].color2, COLCYAN, 0);
    /* Channel */
    InitColor(&CmdsColors[COLKICK].color3_str, &CmdsColors[COLKICK].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLKICK].color3_str, &CmdsColors[COLKICK].color3, COLCYAN, 1);
    /* Comment */
    InitColor(&CmdsColors[COLKICK].color4_str, &CmdsColors[COLKICK].color4, COLWHITE, 0);
    /* Kick */
    InitColor(&CmdsColors[COLKICK].color5_str, &CmdsColors[COLKICK].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLKICK].color5_str, &CmdsColors[COLKICK].color5, COLWHITE, 1);
    /* Friends */
    InitColor(&CmdsColors[COLKICK].color6_str, &CmdsColors[COLKICK].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLKICK].color6_str, &CmdsColors[COLKICK].color6, COLCYAN, 1);

    /* DCCs */
    /* Nick */
    InitColor(&CmdsColors[COLDCC].color1_str, &CmdsColors[COLDCC].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLDCC].color2_str, &CmdsColors[COLDCC].color2, COLPURPLE, 0);
    /* Command */
    InitColor(&CmdsColors[COLDCC].color3_str, &CmdsColors[COLDCC].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLDCC].color3_str, &CmdsColors[COLDCC].color3, COLWHITE, 1);
    /* What */
    InitColor(&CmdsColors[COLDCC].color4_str, &CmdsColors[COLDCC].color4, COLCYAN, 0);
    /* Dcc */
    InitColor(&CmdsColors[COLDCC].color5_str, &CmdsColors[COLDCC].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLDCC].color5_str, &CmdsColors[COLDCC].color5, COLYELLOW, 1);
    /* Warning */
    InitColor(&CmdsColors[COLDCC].color6_str, &CmdsColors[COLDCC].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLDCC].color6_str, &CmdsColors[COLDCC].color6, COLRED, 1);

    /* WHO */
    /* Nick */
    InitColor(&CmdsColors[COLWHO].color1_str, &CmdsColors[COLWHO].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLWHO].color2_str, &CmdsColors[COLWHO].color2, COLPURPLE, 0);
    /* Channel */
    InitColor(&CmdsColors[COLWHO].color3_str, &CmdsColors[COLWHO].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLWHO].color3_str, &CmdsColors[COLWHO].color3, COLCYAN, 1);
    /* Mode */
    InitColor(&CmdsColors[COLWHO].color4_str, &CmdsColors[COLWHO].color4, COLWHITE, 0);
    /* Name */
    InitColor(&CmdsColors[COLWHO].color5_str, &CmdsColors[COLWHO].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLWHO].color5_str, &CmdsColors[COLWHO].color5, COLWHITE, 1);

    /* WHOIS */
    /* Nick */
    InitColor(&CmdsColors[COLWHOIS].color1_str, &CmdsColors[COLWHOIS].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLWHOIS].color2_str, &CmdsColors[COLWHOIS].color2, COLPURPLE, 0);
    /* Channels */
    InitColor(&CmdsColors[COLWHOIS].color3_str, &CmdsColors[COLWHOIS].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLWHOIS].color3_str, &CmdsColors[COLWHOIS].color3, COLCYAN, 1);
    /* Server */
    InitColor(&CmdsColors[COLWHOIS].color4_str, &CmdsColors[COLWHOIS].color4, COLWHITE, 0);
    /* Channels,Server,SignOn,Idle,IrcOp */
    InitColor(&CmdsColors[COLWHOIS].color5_str, &CmdsColors[COLWHOIS].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLWHOIS].color5_str, &CmdsColors[COLWHOIS].color5, COLBLUE, 1);
    /* Channels in friend */
    InitColor(&CmdsColors[COLWHOIS].color6_str, &CmdsColors[COLWHOIS].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLWHOIS].color6_str, &CmdsColors[COLWHOIS].color6, COLRED, 1);

    /* Public MSGs */
    /* Nick */
    InitColor(&CmdsColors[COLPUBLIC].color1_str, &CmdsColors[COLPUBLIC].color1, COLWHITE, 0);
    /* < and > */
    InitColor(&CmdsColors[COLPUBLIC].color2_str, &CmdsColors[COLPUBLIC].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLPUBLIC].color2_str, &CmdsColors[COLPUBLIC].color2, COLCYAN, 0);
    /* Channel */
    InitColor(&CmdsColors[COLPUBLIC].color3_str, &CmdsColors[COLPUBLIC].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLPUBLIC].color3_str, &CmdsColors[COLPUBLIC].color3, COLCYAN, 1);
    /* Auto reply */
    InitColor(&CmdsColors[COLPUBLIC].color4_str, &CmdsColors[COLPUBLIC].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLPUBLIC].color4_str, &CmdsColors[COLPUBLIC].color4, COLCYAN, 1);
    /* Line */
    InitColor(&CmdsColors[COLPUBLIC].color5_str, &CmdsColors[COLPUBLIC].color5, COLWHITE, 0);
    /* Your nick if Ego is on */
    InitColor(&CmdsColors[COLPUBLIC].color6_str, &CmdsColors[COLPUBLIC].color6, COLCYAN, 0);

    /* Cdcc */
    /* Nick */
    InitColor(&CmdsColors[COLCDCC].color1_str, &CmdsColors[COLCDCC].color1, COLCYAN, 0);
    /* Userhost */
    InitColor(&CmdsColors[COLCDCC].color2_str, &CmdsColors[COLCDCC].color2, COLPURPLE, 0);
    /* What */
    InitColor(&CmdsColors[COLCDCC].color3_str, &CmdsColors[COLCDCC].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLCDCC].color3_str, &CmdsColors[COLCDCC].color3, COLWHITE, 1);
    /* Line */
    InitColor(&CmdsColors[COLCDCC].color4_str, &CmdsColors[COLCDCC].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLCDCC].color4_str, &CmdsColors[COLCDCC].color4, COLYELLOW, 1);
    /* Files/bytes */
    InitColor(&CmdsColors[COLCDCC].color5_str, &CmdsColors[COLCDCC].color5, COLCYAN, 0);
    /* Channel */
    InitColor(&CmdsColors[COLCDCC].color6_str, &CmdsColors[COLCDCC].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLCDCC].color6_str, &CmdsColors[COLCDCC].color6, COLCYAN, 1);

    /* Links */
    /* Server */
    InitColor(&CmdsColors[COLLINKS].color1_str, &CmdsColors[COLLINKS].color1, COLCYAN, 0);
    /* Uplink */
    InitColor(&CmdsColors[COLLINKS].color2_str, &CmdsColors[COLLINKS].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLLINKS].color2_str, &CmdsColors[COLLINKS].color2, COLCYAN, 1);
    /* Distance */
    InitColor(&CmdsColors[COLLINKS].color3_str, &CmdsColors[COLLINKS].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLLINKS].color3_str, &CmdsColors[COLLINKS].color3, COLYELLOW, 1);
    /* > */
    InitColor(&CmdsColors[COLLINKS].color4_str, &CmdsColors[COLLINKS].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLLINKS].color4_str, &CmdsColors[COLLINKS].color4, COLWHITE, 1);
    /* Border */
    InitColor(&CmdsColors[COLLINKS].color5_str, &CmdsColors[COLLINKS].color5, COLPURPLE, 0);

    /* DCC CHAT */
    /* Nick */
    InitColor(&CmdsColors[COLDCCCHAT].color1_str, &CmdsColors[COLDCCCHAT].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLDCCCHAT].color1_str, &CmdsColors[COLDCCCHAT].color1, COLRED, 1);
    /* = */
    InitColor(&CmdsColors[COLDCCCHAT].color2_str, &CmdsColors[COLDCCCHAT].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLDCCCHAT].color2_str, &CmdsColors[COLDCCCHAT].color2, COLWHITE, 1);
    /* Line */
    InitColor(&CmdsColors[COLDCCCHAT].color3_str, &CmdsColors[COLDCCCHAT].color3, COLWHITE, 0);
    /* [ */
    InitColor(&CmdsColors[COLDCCCHAT].color4_str, &CmdsColors[COLDCCCHAT].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLDCCCHAT].color4_str, &CmdsColors[COLDCCCHAT].color4, COLRED, 1);
    /* Nick you sent chat message to */
    InitColor(&CmdsColors[COLDCCCHAT].color5_str, &CmdsColors[COLDCCCHAT].color5, COLRED, 0);

    /* CSCAN */
    /* Channel */
    InitColor(&CmdsColors[COLCSCAN].color1_str, &CmdsColors[COLCSCAN].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLCSCAN].color1_str, &CmdsColors[COLCSCAN].color1, COLCYAN, 1);
    /* Friends */
    InitColor(&CmdsColors[COLCSCAN].color2_str, &CmdsColors[COLCSCAN].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLCSCAN].color2_str, &CmdsColors[COLCSCAN].color2, COLCYAN, 1);
    /* Ops */
    InitColor(&CmdsColors[COLCSCAN].color3_str, &CmdsColors[COLCSCAN].color3, COLCYAN, 0);
    /* Voiced */
    InitColor(&CmdsColors[COLCSCAN].color4_str, &CmdsColors[COLCSCAN].color4, COLPURPLE, 0);
    /* Normal */
    InitColor(&CmdsColors[COLCSCAN].color5_str, &CmdsColors[COLCSCAN].color5, COLWHITE, 0);
    /* Shitted */
    InitColor(&CmdsColors[COLCSCAN].color6_str, &CmdsColors[COLCSCAN].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLCSCAN].color6_str, &CmdsColors[COLCSCAN].color6, COLRED, 1);

    /* Nick change */
    /* Old nick */
    InitColor(&CmdsColors[COLNICK].color1_str, &CmdsColors[COLNICK].color1, COLCYAN, 0);
    /* known */
    InitColor(&CmdsColors[COLNICK].color2_str, &CmdsColors[COLNICK].color2, COLWHITE, 0);
    /* New nick */
    InitColor(&CmdsColors[COLNICK].color3_str, &CmdsColors[COLNICK].color3, COLCYAN, 0);
    /* @ in Cscan */
    InitColor(&CmdsColors[COLNICK].color4_str, &CmdsColors[COLNICK].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLNICK].color4_str, &CmdsColors[COLNICK].color4, COLGREEN, 1);
    /* + in Cscan */
    InitColor(&CmdsColors[COLNICK].color5_str, &CmdsColors[COLNICK].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLNICK].color5_str, &CmdsColors[COLNICK].color5, COLPURPLE, 1);

    /* /ME */
    /* * or ì */
    InitColor(&CmdsColors[COLME].color1_str, &CmdsColors[COLME].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLME].color1_str, &CmdsColors[COLME].color1, COLWHITE, 1);
    /* Your nick */
    InitColor(&CmdsColors[COLME].color2_str, &CmdsColors[COLME].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLME].color2_str, &CmdsColors[COLME].color2, COLCYAN, 1);
    /* Nick */
    InitColor(&CmdsColors[COLME].color3_str, &CmdsColors[COLME].color3, COLCYAN, 0);
    /* Target */
    InitColor(&CmdsColors[COLME].color4_str, &CmdsColors[COLME].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLME].color4_str, &CmdsColors[COLME].color4, COLCYAN, 1);
    /* Line */
    InitColor(&CmdsColors[COLME].color5_str, &CmdsColors[COLME].color5, COLWHITE, 0);
    /* Auto reply */
    InitColor(&CmdsColors[COLME].color6_str, &CmdsColors[COLME].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLME].color6_str, &CmdsColors[COLME].color6, COLCYAN, 1);

    /* Misc Colors */
    /* Color of @ in user@host */
    InitColor(&CmdsColors[COLMISC].color1_str, &CmdsColors[COLMISC].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLMISC].color1_str, &CmdsColors[COLMISC].color1, COLWHITE, 1);
    /* Color of ()s arround user@host */
    InitColor(&CmdsColors[COLMISC].color2_str, &CmdsColors[COLMISC].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLMISC].color2_str, &CmdsColors[COLMISC].color2, COLWHITE, 1);
    /* Colors of (msg) etc in CELECOSM */
#ifdef CELECOSM
    InitColor(&CmdsColors[COLMISC].color3_str, &CmdsColors[COLMISC].color3, COLBLUE, 1);
#endif
    /* Color of <>s for friends in publics */
    InitColor(&CmdsColors[COLMISC].color4_str, &CmdsColors[COLMISC].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLMISC].color4_str, &CmdsColors[COLMISC].color4, COLCYAN, 1);
    /* Color of <>s for you in publics */
    InitColor(&CmdsColors[COLMISC].color5_str, &CmdsColors[COLMISC].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLMISC].color5_str, &CmdsColors[COLMISC].color5, COLBLUE, 1);

    /* Color1 of the statusbar */
#ifndef CELE
    /* BAR */
    InitColor(&CmdsColors[COLSBAR1].color1_str, &CmdsColors[COLSBAR1].color1, COLBLUEBG, 0);
    /* Window refnum, ircop (*), away and server in OV */
    InitColor(&CmdsColors[COLSBAR1].color2_str, &CmdsColors[COLSBAR1].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color2_str, &CmdsColors[COLSBAR1].color2, COLGREEN, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color2_str, &CmdsColors[COLSBAR1].color2, COLBLUEBG, 1);
#endif
    /* ChanOp (@) and via server */
    InitColor(&CmdsColors[COLSBAR1].color3_str, &CmdsColors[COLSBAR1].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color3_str, &CmdsColors[COLSBAR1].color3, COLPURPLE, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color3_str, &CmdsColors[COLSBAR1].color3, COLBLUEBG, 1);
#endif
    /* Usermode and query */
    InitColor(&CmdsColors[COLSBAR1].color4_str, &CmdsColors[COLSBAR1].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color4_str, &CmdsColors[COLSBAR1].color4, COLRED, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color4_str, &CmdsColors[COLSBAR1].color4, COLBLUEBG, 1);
#endif
    /* Channel and lastjoin */
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLYELLOW, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLBLUEBG, 1);
#endif
    /* Turn all attributes (bold) off */
#ifndef SZ32
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLNOBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLWHITE, 1);
#else
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLWHITE, 0);
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLBLUEBG, 1);
#endif
#else  /* CELE */
    /* BAR */
    InitColor(&CmdsColors[COLSBAR1].color1_str, &CmdsColors[COLSBAR1].color1, COLBLUEBG, 0);
    /* Secondary Bar */
    InitColor(&CmdsColors[COLSBAR1].color2_str, &CmdsColors[COLSBAR1].color2, COLBLUEBG, 0);
    /* Level 1 stuff (nick) */
    InitColor(&CmdsColors[COLSBAR1].color3_str, &CmdsColors[COLSBAR1].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color3_str, &CmdsColors[COLSBAR1].color3, COLCYAN, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color3_str, &CmdsColors[COLSBAR1].color3, COLBLUEBG, 1);
#endif
    /* Level 2 stuff (modes, names) */
    InitColor(&CmdsColors[COLSBAR1].color4_str, &CmdsColors[COLSBAR1].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color4_str, &CmdsColors[COLSBAR1].color4, COLRED, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color4_str, &CmdsColors[COLSBAR1].color4, COLBLUEBG, 1);
#endif
    /* Level 3 stuff (info) */
#ifndef SZ32
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLNOBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLGREEN, 1);
#else
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLGREEN, 0);
    InitColor(&CmdsColors[COLSBAR1].color5_str, &CmdsColors[COLSBAR1].color5, COLBLUEBG, 1);
#endif
    /* level 4 stuff (brackets) */
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLNOBOLD, 0);
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLWHITE, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR1].color6_str, &CmdsColors[COLSBAR1].color6, COLBLUEBG, 1);
#endif
#endif /* CELE */

    /* Color2 of the statusbar */
    /* Nickname and time */
    InitColor(&CmdsColors[COLSBAR2].color1_str, &CmdsColors[COLSBAR2].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR2].color1_str, &CmdsColors[COLSBAR2].color1, COLCYAN, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR2].color1_str, &CmdsColors[COLSBAR2].color1, COLBLUEBG, 1);
#endif
    /* Various stuff (uptime, lag, status flags) */
    InitColor(&CmdsColors[COLSBAR2].color2_str, &CmdsColors[COLSBAR2].color2, COLCYAN, 0);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR2].color2_str, &CmdsColors[COLSBAR2].color2, COLBLUEBG, 1);
#endif
    /* Hold */
    InitColor(&CmdsColors[COLSBAR2].color3_str, &CmdsColors[COLSBAR2].color3, COLBOLD, 0);
    InitColor(&CmdsColors[COLSBAR2].color3_str, &CmdsColors[COLSBAR2].color3, COLWHITE, 1);
#ifdef SZ32
    InitColor(&CmdsColors[COLSBAR2].color3_str, &CmdsColors[COLSBAR2].color3, COLBLUEBG, 1);
#endif
    /* Window activity */
    InitColor(&CmdsColors[COLSBAR2].color4_str, &CmdsColors[COLSBAR2].color4, COLYELLOW, 0);
    InitColor(&CmdsColors[COLSBAR2].color4_str, &CmdsColors[COLSBAR2].color4, COLBOLD, 1);
    InitColor(&CmdsColors[COLSBAR2].color4_str, &CmdsColors[COLSBAR2].color4, COLREDBG, 1);

#ifdef CELECOSM
    /* Celerity Colors - Used in cosmetics */
    /* Color of (msg) */
    InitColor(&CmdsColors[COLCELE].color1_str, &CmdsColors[COLCELE].color1, COLRED, 0);
    /* Color of (notice) */
    InitColor(&CmdsColors[COLCELE].color2_str, &CmdsColors[COLCELE].color2, COLRED, 0);
    /* Color of Mail Notice */
    InitColor(&CmdsColors[COLCELE].color3_str, &CmdsColors[COLCELE].color3, COLPURPLE, 0);
#endif

#ifdef OPERVISION
    /* OperVision */
    /* Nick / Single Server */
    InitColor(&CmdsColors[COLOV].color1_str, &CmdsColors[COLOV].color1, COLBOLD, 0);
    InitColor(&CmdsColors[COLOV].color1_str, &CmdsColors[COLOV].color1, COLBLUE, 1);
    /* Main Server */
    InitColor(&CmdsColors[COLOV].color2_str, &CmdsColors[COLOV].color2, COLBOLD, 0);
    InitColor(&CmdsColors[COLOV].color2_str, &CmdsColors[COLOV].color2, COLWHITE, 1);
    /* Server */
    InitColor(&CmdsColors[COLOV].color3_str, &CmdsColors[COLOV].color3, COLGREEN, 0);
    /* Command */
    InitColor(&CmdsColors[COLOV].color4_str, &CmdsColors[COLOV].color4, COLBOLD, 0);
    InitColor(&CmdsColors[COLOV].color4_str, &CmdsColors[COLOV].color4, COLWHITE, 0);
    /* Secondary Info */
    InitColor(&CmdsColors[COLOV].color5_str, &CmdsColors[COLOV].color5, COLWHITE, 0);
    /* OV String */
    InitColor(&CmdsColors[COLOV].color6_str, &CmdsColors[COLOV].color6, COLBOLD, 0);
    InitColor(&CmdsColors[COLOV].color6_str, &CmdsColors[COLOV].color6, COLYELLOW, 1);
#endif
#endif
}

/* Kicks nick from multiple channel */
#ifndef LITE
void MultiKick(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *nick=(char *) 0;
    char *channels=(char *) 0;
    char *comment=(char *) 0;
    NickList *tmpnick;
    ChannelList *tmpchan;

    channels=new_next_arg(args,&args);
    if (channels) {
        if (!(*channels=='#')) {
            nick=channels;
            channels="*";
        }
        else nick=new_next_arg(args,&args);
        if (!(nick && *nick)) {
            PrintUsage("MULTK [#channels] nick [comment]");
            return;
        }
        if (args && *args) comment=args;
        else comment=DefaultK;
        for (tmpchan=server_list[curr_scr_win->server].chan_list;tmpchan;
             tmpchan=tmpchan->next)
            if (HAS_OPS(tmpchan->status) && CheckChannel(tmpchan->channel,channels))
            {
                tmpnick=find_in_hash(tmpchan,nick);
                if (tmpnick) send_to_server("KICK %s %s :%s",
                                            tmpchan->channel,nick,comment);
            }
    }
    else PrintUsage("MULTK [#channels] nick [comment]");
}
#endif

/* Kills users matching filter with WHO */
#ifdef OPER
void WhoKill(command,args,subargs)
char *command;
char *args;
char *subargs;
{
    char *filter=(char *) 0;
    char *pattern=(char *) 0;

    if (inSZFKill) {
        say("Already doing filter kill or trace kill");
        return;
    }
    filter=new_next_arg(args,&args);
    pattern=new_next_arg(args,&args);
    if (filter && pattern) {
        malloc_strcpy(&wkillpattern,pattern);
        if (args && *args) malloc_strcpy(&wkillreason,args);
        else new_free(&wkillreason);
        WhoKillNum=0;
        server_list[from_server].SZWho++;
        inSZFKill=1;
        send_to_server("WHO %s",filter);
    }
    else PrintUsage("WKILL who_filter kill_pattern [reason]");
}

/* Does the actual killing */
void DoKill(nick,user,host)
char *nick;
char *user;
char *host;
{
    char tmpbuf[mybufsize/4];

    if (!my_stricmp(nick,get_server_nickname(from_server))) return;
    snprintf(tmpbuf,sizeof(tmpbuf),"%s!%s@%s",nick,user,host);
    if (!wild_match(wkillpattern,tmpbuf)) return;
    WhoKillNum++;
    if (wkillreason) strmcpy(tmpbuf,wkillreason,sizeof(tmpbuf));
    else strmcpy(tmpbuf,nick,sizeof(tmpbuf));
    send_to_server("KILL %s :%s (%d)",nick,tmpbuf,WhoKillNum);
}

/* Reports statistics for filter kill */
void HandleEndOfKill() {
    say("Total of %d users were killed",WhoKillNum);
    inSZFKill=0;
    new_free(&wkillpattern);
    new_free(&wkillreason);
}
#endif /* OPER */

#ifdef WANTANSI
/* Returns terminal sequence to set given color */
int BuildColorNew(color, dest, lineno)
char *color;
char *dest;
int  lineno;
{
    int  colnum;
    char *tmpstr = color;
    char *tmpstr1;
    char tmpbuf[mybufsize / 4];

    while (*tmpstr) {
        tmpstr1 = tmpbuf;
        while (*tmpstr && *tmpstr != ',') *tmpstr1++ = *tmpstr++;
        *tmpstr1 = '\0';

        if ((!strncmp(tmpbuf, "FG", 2) || !strncmp(tmpbuf, "BG", 2)) &&
             is_number(tmpbuf + 2)) {
            char *tcap;

            if (!SETAF || !SETAB) {
                put_it("Warning: terminal lacks support for 'setaf' or 'setab' capability (try infocmp -1), can't use this color (line %d in ScrollZ.save)", NUMCOLORS - 1, lineno);
                return(0);
            }

            colnum = atoi(tmpbuf + 2);
            if (colnum >= NUMCOLORS) {
                put_it("Warning: color %d is outside the range terminal supports for colors: 0 - %d (try infocmp -1), can't use this color (line %d in ScrollZ.save)", colnum, NUMCOLORS - 1, lineno);
                return(0);
            }

            if (*tmpbuf=='F') tcap = SETAF;
            else tcap = SETAB;

            strcat(dest, tparm(tcap, colnum));
        }
        else return(0);

        if (*tmpstr == ',') tmpstr++;
    }
    return(1);
}

/* Builds color strings for default theme */
int InitColor(colorstr, colorseq, colornum, append)
char **colorstr;
char **colorseq;
int colornum;
int append;
{
    char *colorname = ColorName(colornum);

    if (append) {
        malloc_strcat(colorstr, ",");
        malloc_strcat(colorstr, colorname);
        malloc_strcat(colorseq, Colors[colornum]);
    }
    else {
        malloc_strcpy(colorstr, colorname);
        malloc_strcpy(colorseq, Colors[colornum]);
    }
}
#endif /* WANTANSI */
