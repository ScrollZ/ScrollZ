#ifndef _myvars_h_
#define _myvars_h_

/*
 * My variables
 *
 * $Id: myvars.h,v 1.27 2001-08-30 17:46:43 f Exp $
 */

#include "mystructs.h"

#ifdef WANTANSI
/* Index of colors in Colors table */
#define COLOFF       0
#define COLBOLD      1
#define COLUNDERLINE 2
#define COLFLASH     3
#define COLREV       4
#define COLBLACK     5
#define COLRED       6
#define COLGREEN     7
#define COLYELLOW    8
#define COLBLUE      9
#define COLPURPLE    10
#define COLCYAN      11
#define COLWHITE     12
#define COLBLACKBG   13
#define COLREDBG     14
#define COLGREENBG   15
#define COLYELLOWBG  16
#define COLBLUEBG    17
#define COLPURPLEBG  18
#define COLCYANBG    19
#define COLWHITEBG   20
#define COLNOBOLD    21
/* Index of commands in CmdsColors table */
#define COLWARNING   0
#define COLJOIN      1
#define COLMSG       2
#define COLNOTICE    3
#define COLNETSPLIT  4
#define COLINVITE    5
#define COLMODE      6
#define COLSETTING   7
#define COLHELP      8
#define COLLEAVE     9
#define COLNOTIFY    10
#define COLCTCP      11
#define COLKICK      12
#define COLDCC       13
#define COLWHO       14
#define COLWHOIS     15
#define COLPUBLIC    16
#define COLCDCC      17
#define COLLINKS     18
#define COLDCCCHAT   19
#define COLCSCAN     20
#define COLNICK      21
#define COLME        22
#define COLMISC      23
#define COLSBAR1     24
#define COLSBAR2     25
#if defined(CELECOSM) && defined(OPERVISION)
#define COLCELE      26
#define COLOV        27
#elif defined(CELECOSM) && !defined(OPERVISION)
#define COLCELE      26
#elif defined(OPERVISION)
#define COLOV        26
#endif /* CELECOSM && OPERVISION */
#endif /* WANTANSI */

#define SAVEMSG           1
#define SAVENOTICE        2
#define SAVEMASS          4
#define SAVECOLL          8
#define SAVECDCC         16
#define SAVEDCC          32
#define SAVEPROT         64
#define SAVEHACK        128
#define SAVESRVM        256
#define SAVECTCP        512
#define SAVEFLOOD      1024
#define SAVEINVITE     2048
#define SAVEKILL       4096
#define SAVEKICK       8192
#define SAVESERVER    16384
#define SAVEFAKE      32768
#define SAVEAREPLY    65536
#define SAVECHAT     131072
#define SAVENOTIFY   262144
#define SAVESENTMSG  524288
#define SAVEALL     1048575

extern struct friends *frlist;
extern struct autobankicks *abklist;
extern struct words *wordlist;
extern struct wholeftstr *wholist;
extern struct splitstr *splitlist,*splitlist1;
#ifdef ACID
extern struct list *nickwatchlist,*tmpnickwatch;
#endif
extern struct spingstr *spinglist;
extern struct encrstr *encrlist;
#ifdef WANTANSI
extern struct colorstr CmdsColors[NUMCMDCOLORS];
extern char   *Colors[SZNUMCOLORS];
#endif
extern char   defban;
extern char   bold;
/****** Coded by Zakath ******/
#ifdef CELE
extern char   *CelerityVersion;
extern char   *CelerityL;
#endif
extern struct urlstr *urllist;
/*****************************/
extern char   *mydefaultserver;
extern char   *ScrollZstr;
extern char   *ScrollZver;
#ifdef EXTRA_STUFF
extern char   *EString;
#endif
extern char   *DefaultServer;
extern char   *DefaultSignOff;
extern char   *DefaultSetAway;
extern char   *DefaultSetBack;
extern char   *DefaultUserinfo;
extern char   *DefaultFinger;
extern char   *AutoJoinChannels;
extern char   *ModeLockString;
extern char   *CdccUlDir;
extern char   *CdccDlDir;
extern char   *WhoKilled;
extern char   *InviteChannels;
extern char   *CdccChannels;
extern char   *AutoRejoinChannels;
extern char   *MDopWatchChannels;
extern char   *ShowFakesChannels;
extern char   *KickOnFloodChannels;
extern char   *KickWatchChannels;
extern char   *NHProtChannels;
extern char   *NickWatchChannels;
extern char   *ShowAwayChannels;
extern char   *KickOpsChannels;
extern char   *KickOnBanChannels;
extern char   *BitchChannels;
extern char   *FriendListChannels;
extern char   *IdleKickChannels;
extern char   *CompressModesChannels;
extern char   *SignoffChannels;
extern char   *BKChannels;
#if defined(EXTRAS) || defined(FLIER)
extern char   *AutoInvChannels;
#endif
extern char   *EncryptPassword;
#ifdef OPER
extern char   *StatskFilter;
extern char   *StatsiFilter;
extern char   *StatscFilter;
extern char   *StatslFilter;
#endif
extern char   *AutoReplyBuffer;
extern char   *OrigNick;
extern char   *LastChat;
extern char   *CurrentDCC;
extern char   *DefaultK;
extern char   *DefaultBK;
extern char   *DefaultBKI;
extern char   *DefaultBKT;
extern char   *DefaultFK;
extern char   *DefaultLK;
extern char   *DefaultABK;
extern char   *DefaultSK;
#ifdef OPER
extern char   *DefaultKill;
#endif
#ifdef ACID
extern char   *ForceJoinChannels;
#endif
extern char   *PermUserMode;
extern char   *AutoReplyString;
extern int    usersloaded;
extern int    inSZNotify;
extern int    inSZLinks;
extern int    inSZFKill;
extern int    inSZTrace;
extern int    ExtMes;
extern int    NHProt;
extern int    NHDisp;
extern int    AutoGet;
extern int    DeopSensor;
extern int    KickSensor;
extern int    NickSensor;
extern int    AutoAwayTime;
extern int    NickWatch;
extern int    MDopWatch;
extern int    KickWatch;
extern int    NickTimer;
extern int    MDopTimer;
extern int    KickTimer;
extern int    IgnoreTime;
extern int    ShitIgnoreTime;
extern int    AutoRejoin;
extern int    AutoJoinOnInv;
extern int    FloodProt;
extern int    FloodMessages;
extern int    FloodSeconds;
extern int    CdccIdle;
extern int    CdccLimit;
extern int    CdccQueueLimit;
extern int    CdccOverWrite;
#ifdef EXTRA_STUFF
extern int    RenameFiles;
#endif
extern int    Security;
extern int    ServerNotice;
extern int    CTCPCloaking;
extern int    ShowFakes;
extern int    ShowAway;
extern int    AutoOpDelay;
#if defined(HAVETIMEOFDAY) && defined(CELE)
extern struct timeval LagTimer;
#else
extern int    LagTimer;
#endif
extern int    KickOps;
extern int    KickOnFlood;
extern int    KickOnBan;
extern int    NickChange;
#ifdef SCKICKS
extern int    NumberOfScatterKicks;
#endif
extern int    NumberOfSignOffMsgs;
extern int    ShowNick;
extern int    PlistTime;
extern int    NlistTime;
extern int    LinksNumber;
extern int    AwaySaveSet;
extern int    ShowWallop;
extern int    LongStatus;
extern double BytesReceived;
extern double BytesSent;
extern int    FriendList;
extern int    OrigNickChange;
extern int    IRCQuit;
extern int    NotifyMode;
extern int    URLCatch;
extern int    Ego;
extern int    LogOn;
extern int    ShowDCCStatus;
extern int    DCCDone;
extern int    AutoNickCompl;
extern int    Bitch;
extern int    IdleKick;
extern int    IdleTime;
extern int    CdccStats;
extern int    CompressModes;
extern int    OrigNickDelay;
#ifdef WANTANSI
extern int    DisplaymIRC;
#endif
extern int    DCCWarning;
extern int    ShowSignoffChan;
extern int    Stamp;
extern int    CdccVerbose;
extern int    ARinWindow;
extern int    BKList;
extern int    OrigNickQuiet;
extern int    OrigNickSent;
#ifdef EXTRAS
extern int    ShowSignAllChan;
#endif
extern int    ExtPub;
/****** Coded by Zakath ******/
extern char   *SPingServers;
extern int    CdccPackNum;
extern int    CdccSendNum;
extern int    CdccRecvNum;
extern int    AwayMsgNum;
extern char   *URLBuffer;
#if defined(EXTRAS) || defined(FLIER)
extern int    AutoInv;
#endif
#if defined(OPERVISION) && defined(WANTANSI)
extern int    OperV;
#endif
#ifdef CELE
extern int    SentAway;
#endif
#ifdef ACID
extern int    ForceJoin;
#endif
/*****************************/
extern time_t LastCheck;
extern time_t LastPlist;
extern time_t LastNlist;
extern time_t LastServer;
extern time_t LastNick;
extern time_t LastLinks;
#ifdef CELE
extern struct friends *whoisfriend;
#endif

extern int  from_server;
extern char *FromUserHost;
extern int  set_away;
extern struct in_addr local_ip_address;
extern char *channel_join;

#endif /* _myvars_h_ */
