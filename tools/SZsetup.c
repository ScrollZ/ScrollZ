/*
 * SZsetup
 *
 * Small add-on for ScrollZ that will make life easier for all those people
 * that complain about default colors. :-)
 * I hope this will generate tons of different color schemes
 *
 * Copyright (C) July 1997 by Flier
 *
 * Requires ansi capable terminal (Linux console, xterm_color or rxvt should
 * work O.K.). If it causes problems, contact the author via e-mail:
 *    flier@scrollz.com
 *
 * Compile : cc -o SZsetup SZsetup.c
 *
 * Usage : SZsetup [-l filename] savefile
 *         -l will load settings from file filename, you can load ScrollZ.save
 *         here (only COLOR section will be used, rest is ignored)
 *         savefile is the name of the file results will be saved to (you can
 *         append this to your ScrollZ.save with text editor)
 *
 * Oh yeah, use this at your own risk, if it formats your hard disk it's your
 * own problem. If it fucks up your ScrollZ it's your problem too. In another
 * word, author is not responsible for anything caused by this code.
 *
 * $Id: SZsetup.c,v 1.4 2002-03-05 17:50:07 f Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "SZsetup.h"

char *Colors[NUMCOLORS]={
 /*  off  */
 "\033[m" ,
 /*  bold       underline  flash      reverse */
 "\033[1m" , "\033[4m"  , "\033[5m"  , "\033[7m"  ,
 /*  black      red        green      yellow     blue          */
 "\033[30m", "\033[31m" , "\033[32m" , "\033[33m" , "\033[34m" ,
 /*  purple     cyan       white      blackbg    redbg         */
 "\033[35m", "\033[36m" , "\033[37m" , "\033[40m" , "\033[41m" ,
 /*  greenbg    yellowbg   bluebg     purplebg   cyanbg        */
 "\033[42m", "\033[43m" , "\033[44m" , "\033[45m" , "\033[46m" ,
 /*  whitebg                                                   */
 "\033[47m"
};
char *ColorNames[NUMCOLORS]={
    "Off",    "Bold",    "Underline","Flash",   "Reverse",
    "Black",  "Red",     "Green",    "Yellow",  "Blue",
    "Purple", "Cyan",    "White",    "BlackBG", "RedBG",
    "GreenBG","YellowBG","BlueBG",   "PurpleBG","CyanBG",
    "WhiteBG"
};
struct setstr SettingNames[NUMCMDCOLORS]={
    { "Warning",  disp_warning },
    { "Join",     disp_join },
    { "Msg",      disp_msg },
    { "Notice",   disp_notice },
    { "Netsplit", disp_netsplit },
    { "Invite",   disp_invite },
    { "Mode",     disp_mode },
    { "Setting",  disp_setting },
    { "Help",     disp_help },
    { "Leave",    disp_leave },
    { "Notify",   disp_notify },
    { "Ctcp",     disp_ctcp },
    { "Kick",     disp_kick },
    { "Dcc",      disp_dcc },
    { "Who",      disp_who },
    { "Whois",    disp_whois },
    { "Public",   disp_public },
    { "Cdcc",     disp_cdcc },
    { "Links",    disp_links },
    { "DccChat",  disp_dccchat },
    { "Cscan",    disp_cscan },
    { "Nick",     disp_nick },
    { "Me",       disp_me }
};
struct colorstr CmdsColors[NUMCMDCOLORS];
struct termios old_tc; /* to store terminal settings on entry */
struct termios new_tc;
char *filename=NULL;   /* filename to load from */
char *savefile=NULL;   /* filename to save to */
int color=1;           /* current color */
int setting=0;         /* current setting */
int startset=0;        /* offset for setting */
int scrcolor=0;        /* current ScrollZ color 1-6 */
int lastcolor=2;       /* last color cursor was on */
int lastsetting=1;     /* last setting cursor was on */
int lastscrcolor=0;    /* last ScrollZ color cursor was on 1-6 */
int where=0;           /* 0=setting  1=color */
int message=0;         /* whether there was a message displayed */

void main(argc,argv)
int argc;
char **argv;
{
    int i;
    FILE *fp;

    if (argc<2) {
        printf("Usage : %s [-l filename] savefile\n",argv[0]);
        return;
    }
    for (i=1;i<argc;i++) {
        if (!strcasecmp(argv[i],"-l")) {
            if (i<argc-1) {
                filename=argv[i+1];
                i++;
            }
            else {
                printf("Usage : %s [-l filename] savefile\n",argv[0]);
                return;
            }
        }
        else savefile=argv[i];
        if (savefile && filename) break;
    }
    if (!savefile) {
        printf("Usage : %s [-l filename] savefile\n",argv[0]);
        return;
    }
    if (filename) {
        if ((fp=fopen(filename,"r"))==NULL) {
            printf("Error, can't open %s\n",filename);
            return;
        }
        fclose(fp);
    }
    init_sz_colors();
    if (filename) read_file();
    set_term();
    draw_screen();
    do_it();
    move(24,0);
    attrset(COLWHITE);
    printf("[?25h\n");
    reset_term();
}

void set_term() {
    tcgetattr(0,&old_tc);
    new_tc=old_tc;
    new_tc.c_lflag&=~(ECHO|ICANON); /* raw output */
    new_tc.c_cc[VMIN]=0;            /* don't wait for keypresses */
    new_tc.c_cc[VTIME]=1;
    tcsetattr(0,TCSANOW,&new_tc);
}

void reset_term() {
    tcsetattr(0, TCSANOW, &old_tc);
}			

int set_color(color,string)
char *color;
char *string;
{
    char *tmpstr;
    char *tmpstr1;
    char tmpbuf[mybufsize];

    strcpy(string,"");
    tmpstr=color;
    while (*tmpstr && isspace(*tmpstr)) tmpstr++;
    while (*tmpstr && !isspace(*tmpstr)) {
        tmpstr1=tmpbuf;
        while (*tmpstr && !(*tmpstr==',' || isspace(*tmpstr))) {
            *tmpstr1=*tmpstr;
            tmpstr1++;
            tmpstr++;
        }
        *tmpstr1='\0';
        if (!strcasecmp(tmpbuf,"BOLD")) strcat(string,Colors[COLBOLD]);
        else if (!strcasecmp(tmpbuf,"UNDERLINE")) strcat(string,Colors[COLUNDERLINE]);
        else if (!strcasecmp(tmpbuf,"FLASH")) strcat(string,Colors[COLFLASH]);
        else if (!strcasecmp(tmpbuf,"REVERSE")) strcat(string,Colors[COLREV]);
        else if (!strcasecmp(tmpbuf,"BLACK")) strcat(string,Colors[COLBLACK]);
        else if (!strcasecmp(tmpbuf,"RED")) strcat(string,Colors[COLRED]);
        else if (!strcasecmp(tmpbuf,"GREEN")) strcat(string,Colors[COLGREEN]);
        else if (!strcasecmp(tmpbuf,"YELLOW")) strcat(string,Colors[COLYELLOW]);
        else if (!strcasecmp(tmpbuf,"BLUE")) strcat(string,Colors[COLBLUE]);
        else if (!strcasecmp(tmpbuf,"PURPLE")) strcat(string,Colors[COLPURPLE]);
        else if (!strcasecmp(tmpbuf,"CYAN")) strcat(string,Colors[COLCYAN]);
        else if (!strcasecmp(tmpbuf,"WHITE")) strcat(string,Colors[COLWHITE]);
        else if (!strcasecmp(tmpbuf,"BLACKBG")) strcat(string,Colors[COLBLACKBG]);
        else if (!strcasecmp(tmpbuf,"REDBG")) strcat(string,Colors[COLREDBG]);
        else if (!strcasecmp(tmpbuf,"GREENBG")) strcat(string,Colors[COLGREENBG]);
        else if (!strcasecmp(tmpbuf,"YELLOWBG")) strcat(string,Colors[COLYELLOWBG]);
        else if (!strcasecmp(tmpbuf,"BLUEBG")) strcat(string,Colors[COLBLUEBG]);
        else if (!strcasecmp(tmpbuf,"PURPLEBG")) strcat(string,Colors[COLPURPLEBG]);
        else if (!strcasecmp(tmpbuf,"CYANBG")) strcat(string,Colors[COLCYANBG]);
        else if (!strcasecmp(tmpbuf,"WHITEBG")) strcat(string,Colors[COLWHITEBG]);
        else return(0);
        if (*tmpstr==',') tmpstr++;
    }
    return(1);
}

void read_file() {
    FILE *fp;
    char tmpbuf[mybufsize];
    char tmpbuf2[mybufsize];
    char *tmpstr;
    char *tmpstr1;
    int  i;
    int  number;

    fp=fopen(filename,"r");
    while ((fgets(tmpbuf,mybufsize,fp))) {
        if (tmpbuf[strlen(tmpbuf)-1]=='\n') tmpbuf[strlen(tmpbuf)-1]='\0';
        tmpstr=tmpbuf;
        while (*tmpstr && isspace(*tmpstr)) tmpstr++;
        if (!strncasecmp(tmpstr,"COLOR",5)) {
            while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
            while (*tmpstr && isspace(*tmpstr)) tmpstr++;
            tmpstr1=tmpbuf2;
            while (*tmpstr && !isspace(*tmpstr)) {
                *tmpstr1=*tmpstr;
                tmpstr++;
                tmpstr1++;
            }
            *tmpstr1='\0';
            number=-1;
            for (i=0;i<NUMCMDCOLORS;i++) {
                if (!strcasecmp(tmpbuf2,SettingNames[i].name)) {
                    number=i;
                    break;
                }
            }
            if (number==-1) continue;
            for (i=1;number<NUMCMDCOLORS && *tmpstr && i<7;i++) {
                while (*tmpstr && isspace(*tmpstr)) tmpstr++;
                if (set_color(tmpstr,tmpbuf2)) {
                    switch (i) {
                        case 1:strcpy(CmdsColors[number].color1,tmpbuf2);
                               break;
                        case 2:strcpy(CmdsColors[number].color2,tmpbuf2);
                               break;
                        case 3:strcpy(CmdsColors[number].color3,tmpbuf2);
                               break;
                        case 4:strcpy(CmdsColors[number].color4,tmpbuf2);
                               break;
                        case 5:strcpy(CmdsColors[number].color5,tmpbuf2);
                               break;
                        case 6:strcpy(CmdsColors[number].color6,tmpbuf2);
                               break;
                    }
                }
                while (*tmpstr && !isspace(*tmpstr)) tmpstr++;
            }
        }
    }
    fclose(fp);
}

char *get_color_name(number)
int number;
{
    static char tmpbuf[mybufsize];
    char *tmpstr;

    strcpy(tmpbuf,ColorNames[number]);
    tmpstr=tmpbuf;
    while (*tmpstr && *tmpstr!=' ') {
        *tmpstr=toupper(*tmpstr);
        tmpstr++;
    }
    *tmpstr='\0';
    return(tmpbuf);
}

void get_colors(number,buffer)
int  number;
char *buffer;
{
    int i;

    strcpy(buffer,"");
    for (i=0;i<NUMCOLORS;i++) {
        if (strstr(CmdsColors[number].color1,Colors[i])) {
            strcat(buffer,get_color_name(i));
            strcat(buffer,",");
        }
    }
    if (buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1]='\0';
    strcat(buffer,"  ");
    for (i=0;i<NUMCOLORS;i++) {
        if (strstr(CmdsColors[number].color2,Colors[i])) {
            strcat(buffer,get_color_name(i));
            strcat(buffer,",");
        }
    }
    if (buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1]='\0';
    strcat(buffer,"  ");
    for (i=0;i<NUMCOLORS;i++) {
        if (strstr(CmdsColors[number].color3,Colors[i])) {
            strcat(buffer,get_color_name(i));
            strcat(buffer,",");
        }
    }
    if (buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1]='\0';
    strcat(buffer,"  ");
    for (i=0;i<NUMCOLORS;i++) {
        if (strstr(CmdsColors[number].color4,Colors[i])) {
            strcat(buffer,get_color_name(i));
            strcat(buffer,",");
        }
    }
    if (buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1]='\0';
    strcat(buffer,"  ");
    for (i=0;i<NUMCOLORS;i++) {
        if (strstr(CmdsColors[number].color5,Colors[i])) {
            strcat(buffer,get_color_name(i));
            strcat(buffer,",");
        }
    }
    if (buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1]='\0';
    strcat(buffer,"  ");
    for (i=0;i<NUMCOLORS;i++) {
        if (strstr(CmdsColors[number].color6,Colors[i])) {
            strcat(buffer,get_color_name(i));
            strcat(buffer,",");
        }
    }
    if (buffer[strlen(buffer)-1]==',') buffer[strlen(buffer)-1]='\0';
}

void save_colors(fp)
FILE *fp;
{
    int  i;
    char tmpbuf[mybufsize];

    for (i=0;i<NUMCMDCOLORS;i++) {
        get_colors(i,tmpbuf);
        switch (i) {
            case COLWARNING:
                fprintf(fp,"COLOR  WARNING   %s\n",tmpbuf);
                break;
            case COLJOIN:
                fprintf(fp,"COLOR  JOIN      %s\n",tmpbuf);
                break;
            case COLMSG:
                fprintf(fp,"COLOR  MSG       %s\n",tmpbuf);
                break;
            case COLNOTICE:
                fprintf(fp,"COLOR  NOTICE    %s\n",tmpbuf);
                break;
            case COLNETSPLIT:
                fprintf(fp,"COLOR  NETSPLIT  %s\n",tmpbuf);
                break;
            case COLINVITE:
                fprintf(fp,"COLOR  INVITE    %s\n",tmpbuf);
                break;
            case COLMODE:
                fprintf(fp,"COLOR  MODE      %s\n",tmpbuf);
                break;
            case COLSETTING:
                fprintf(fp,"COLOR  SETTING   %s\n",tmpbuf);
                break;
            case COLLEAVE:
                fprintf(fp,"COLOR  LEAVE     %s\n",tmpbuf);
                break;
            case COLNOTIFY:
                fprintf(fp,"COLOR  NOTIFY    %s\n",tmpbuf);
                break;
            case COLCTCP:
                fprintf(fp,"COLOR  CTCP      %s\n",tmpbuf);
                break;
            case COLKICK:
                fprintf(fp,"COLOR  KICK      %s\n",tmpbuf);
                break;
            case COLDCC:
                fprintf(fp,"COLOR  DCC       %s\n",tmpbuf);
                break;
            case COLWHO:
                fprintf(fp,"COLOR  WHO       %s\n",tmpbuf);
                break;
            case COLWHOIS:
                fprintf(fp,"COLOR  WHOIS     %s\n",tmpbuf);
                break;
            case COLPUBLIC:
                fprintf(fp,"COLOR  PUBLIC    %s\n",tmpbuf);
                break;
            case COLCDCC:
                fprintf(fp,"COLOR  CDCC      %s\n",tmpbuf);
                break;
            case COLLINKS:
                fprintf(fp,"COLOR  LINKS     %s\n",tmpbuf);
                break;
            case COLDCCCHAT:
                fprintf(fp,"COLOR  DCCCHAT   %s\n",tmpbuf);
                break;
            case COLCSCAN:
                fprintf(fp,"COLOR  CSCAN     %s\n",tmpbuf);
                break;
            case COLNICK:
                fprintf(fp,"COLOR  NICK      %s\n",tmpbuf);
                break;
            case COLME:
                fprintf(fp,"COLOR  ME        %s\n",tmpbuf);
                break;
        }
    }
    
}

void move(x,y)
int x;
int y;
{
    printf("[%d;%dH",x+1,y+1);
}

void colorset(color)
char *color;
{
    printf("%s",Colors[COLOFF]);
    printf("%s",color);
}

void attrset(color)
int color;
{
    if (color!=COLOFF) printf("%s",Colors[COLOFF]);
    printf("%s",Colors[color]);
}

void attradd(color)
int color;
{
    printf("%s",Colors[color]);
}

void display_settings() {
    int i;

    /* Settings */
    attrset(COLWHITE);
    for (i=0;i<20;i++) {
        move(YSETTINGS+i,XSETTINGS);
        printf("%-9s",SettingNames[i+startset].name);
    }
}

void draw_screen() {
    int i;

    printf("[2J[?25l");
    move(0,0);
    attrset(COLBLUEBG);
    attradd(COLWHITE);
    attradd(COLBOLD);
    printf("                      ScrollZ color setup v1.2 by Flier                        ");
    attrset(COLWHITE);
    /* Colors */
    for (i=1;i<NUMCOLORS;i++) {
        attrset(i);
        move(YCOLORS+i,XCOLORS);
        printf("%s",ColorNames[i]);
    }
    display_settings();
    /* Description of keys */
    move(24,2);
    attrset(COLYELLOW);
    attradd(COLBOLD);
    printf("C");
    attrset(COLWHITE);
    printf(" Clear  ");
    attrset(COLYELLOW);
    attradd(COLBOLD);
    printf("Q");
    attrset(COLWHITE);
    printf(" Quit   ");
    attrset(COLYELLOW);
    attradd(COLBOLD);
    printf("S");
    attrset(COLWHITE);
    printf(" Save   ");
    attrset(COLYELLOW);
    attradd(COLBOLD);
    printf("Cursors");
    attrset(COLWHITE);
    printf(" Move   ");
    attrset(COLYELLOW);
    attradd(COLBOLD);
    printf("Space");
    attrset(COLWHITE);
    printf(" Select   ");
    attrset(COLYELLOW);
    attradd(COLBOLD);
    printf("1-6");
    attrset(COLWHITE);
    printf(" Color");
    attrset(COLYELLOW);
    attradd(COLBOLD);
    move(YSCRCOLORS+7,XSCRCOLORS);
    printf("Example :");
    attrset(COLCYAN);
    attradd(COLBOLD);
    move(YSCRCOLORS+scrcolor,XSCRCOLORS+9);
    printf("<--");
}

void fix_attr(colbuf,lowcolor,highcolor,curcolor)
char *colbuf;
int  lowcolor;
int  highcolor;
int  curcolor;
{
    int  i;
    char tmpbuf[mybufsize];

    strcpy(tmpbuf,"");
    for (i=COLOFF;i<NUMCOLORS;i++) {
        if (i>=lowcolor && i<=highcolor) {
            if (i!=curcolor && strstr(colbuf,Colors[i])) strcat(tmpbuf,Colors[i]);
        }
        else if (strstr(colbuf,Colors[i])) strcat(tmpbuf,Colors[i]);
    }
    if (!strstr(colbuf,Colors[curcolor])) strcat(tmpbuf,Colors[curcolor]);
    strcpy(colbuf,tmpbuf);
}

void fix_color(colbuf,lowcolor,highcolor,curcolor)
char *colbuf;
int  lowcolor;
int  highcolor;
int  curcolor;
{
    int  i;
    char tmpbuf[mybufsize];

    strcpy(tmpbuf,"");
    for (i=COLOFF;i<NUMCOLORS;i++) {
        if ((i<lowcolor || i>highcolor) && strstr(colbuf,Colors[i]))
            strcat(tmpbuf,Colors[i]);
    }
    strcat(tmpbuf,Colors[curcolor]);
    strcpy(colbuf,tmpbuf);
}

void do_it() {
    int  i;
    int  disp=3;
    char key;
    char colbuf[mybufsize];
    FILE *fp;

    while (1) {
        if (disp) {
            if (disp>1 || setting!=lastsetting) {
                /* we need to redisplay settings */
                if (!startset) {
                    attrset(COLWHITE);
                    move(YSETTINGS+lastsetting,XSETTINGS);  /* previous active setting */
                    printf("%-9s",SettingNames[lastsetting].name);
                }
                if (where) {
                    attrset(COLBLUEBG);
                    attradd(COLWHITE);
                    attradd(COLBOLD);
                }
                else {
                    attrset(COLCYANBG);
                    attradd(COLWHITE);
                    attradd(COLBOLD);
                }
                move(YSETTINGS+setting,XSETTINGS);      /* current active setting */
                printf("%-9s",SettingNames[setting+startset].name);
                lastsetting=setting;
            }
            if (disp>1 || color!=lastcolor) {       /* we need to redisplay colors */
                move(YCOLORS+lastcolor,XCOLORS-1);  /* previous active color */
                attrset(COLWHITE);
                printf(" ");
                attrset(lastcolor);
                printf("%-10s",ColorNames[lastcolor]);
                attrset(COLWHITE);
                printf(" ");
                move(YCOLORS+color,XCOLORS-1);      /* active color */
                if (!where) {
                    attrset(COLBLUEBG);
                    attradd(COLWHITE);
                    attradd(COLBOLD);
                }
                else {
                    attrset(COLCYANBG);
                    attradd(COLWHITE);
                    attradd(COLBOLD);
                }
                printf(" ");
                printf("%-10s",ColorNames[color]);
                printf(" ");
                lastcolor=color;
            }
            if (scrcolor!=lastscrcolor) { /* we need to redisplay ScrollZ colors */
                attrset(COLWHITE);
                move(YSCRCOLORS+lastscrcolor,XSCRCOLORS+9);
                printf("   ");
                attrset(COLCYAN);
                attradd(COLBOLD);
                move(YSCRCOLORS+scrcolor,XSCRCOLORS+9);
                printf("<--");
                lastscrcolor=scrcolor;
            }
            if (disp==3) {
                attrset(COLWHITE);
                for (i=0;i<13;i++) {
                    move(YSCRCOLORS+8+i,XSCRCOLORS);
                    printf("                                                      ");
                }
                SettingNames[setting+startset].func();
                print_colors();
            }
            disp=0;
        }
        key=getchar();
        if (key>=0) {
            if (message) {
                attrset(COLWHITE);
                move(YMESSAGE,XMESSAGE);
                printf("                                                        ");
                message=0;
            }
            if (key=='c' || key=='C') {
                switch (scrcolor) {
                    case 0:strcpy(CmdsColors[setting+startset].color1,Colors[COLWHITE]);
                           break;
                    case 1:strcpy(CmdsColors[setting+startset].color2,Colors[COLWHITE]);
                           break;
                    case 2:strcpy(CmdsColors[setting+startset].color3,Colors[COLWHITE]);
                           break;
                    case 3:strcpy(CmdsColors[setting+startset].color4,Colors[COLWHITE]);
                           break;
                    case 4:strcpy(CmdsColors[setting+startset].color5,Colors[COLWHITE]);
                           break;
                    case 5:strcpy(CmdsColors[setting+startset].color6,Colors[COLWHITE]);
                           break;
                }
                disp=3;
            }
            if (key=='q' || key=='Q') break;
            if (key=='s' || key=='S') {
                if ((fp=fopen(savefile,"w"))==NULL) {
                    message=1;
                    attrset(COLREDBG);
                    attradd(COLWHITE);
                    attradd(COLBOLD);
                    move(YMESSAGE,XMESSAGE);
                    printf("    Can't save to %s     ",savefile);
                }
                else {
                    save_colors(fp);
                    fclose(fp);
                    message=1;
                    attrset(COLBLUEBG);
                    attradd(COLWHITE);
                    attradd(COLBOLD);
                    move(YMESSAGE,XMESSAGE);
                    printf("    Save completed to %s     ",savefile);
                }
            }
            if (key>='1' && key<='6') {
                lastscrcolor=scrcolor;
                scrcolor=key-'1';
                disp=1;
            }
            move(2,40);
            if (key==' ' || key=='\n') {
                switch (scrcolor) {
                    case 0:strcpy(colbuf,CmdsColors[setting+startset].color1);
                           break;
                    case 1:strcpy(colbuf,CmdsColors[setting+startset].color2);
                           break;
                    case 2:strcpy(colbuf,CmdsColors[setting+startset].color3);
                           break;
                    case 3:strcpy(colbuf,CmdsColors[setting+startset].color4);
                           break;
                    case 4:strcpy(colbuf,CmdsColors[setting+startset].color5);
                           break;
                    case 5:strcpy(colbuf,CmdsColors[setting+startset].color6);
                           break;
                }
                if (color>=1 && color<=4) fix_attr(colbuf,1,4,color);
                if (color>=5 && color<=12) fix_color(colbuf,5,12,color);
                if (color>12) fix_color(colbuf,13,20,color);
                switch (scrcolor) {
                    case 0:strcpy(CmdsColors[setting+startset].color1,colbuf);
                           break;
                    case 1:strcpy(CmdsColors[setting+startset].color2,colbuf);
                           break;
                    case 2:strcpy(CmdsColors[setting+startset].color3,colbuf);
                           break;
                    case 3:strcpy(CmdsColors[setting+startset].color4,colbuf);
                           break;
                    case 4:strcpy(CmdsColors[setting+startset].color5,colbuf);
                           break;
                    case 5:strcpy(CmdsColors[setting+startset].color6,colbuf);
                           break;
                }
                disp=3;
            }
            if (key=='\033') {
                key=getchar();
                if (key=='[') {
                    key=getchar();
                    if (key=='B') {  /* cursor down */
                        switch (where) {
                            case 0:lastsetting=setting;
                                   if (setting<19) {
                                       setting++;
                                       display_settings();
                                       disp=3;
                                   }
                                   else if (startset+setting<NUMCMDCOLORS-1) {
                                       startset++;
                                       display_settings();
                                       disp=3;
                                   }
                                   break;
                            case 1:lastcolor=color;
                                   color++;
                                   if (color==NUMCOLORS) color=1;
                                   disp=1;
                                   break;
                        }
                    }
                    if (key=='A') {  /* cursor up */
                        switch (where) {
                            case 0:lastsetting=setting;
                                   if (setting) {
                                       disp=3;
                                       attrset(COLWHITE);
                                       move(YSETTINGS+setting,XSETTINGS);  /* previous active setting */
                                       printf("%-9s",SettingNames[setting+startset].name);
                                       setting--;
                                   }
                                   else if (startset) {
                                       startset--;
                                       display_settings();
                                       disp=3;
                                   }
                                   break;
                            case 1:lastcolor=color;
                                   color--;
                                   if (color<1) color=NUMCOLORS-1;
                                   disp=1;
                                   break;
                        }
                    }
                    if (key=='C') { /* cursor right */
                        where++;
                        if (where>1) where=0;
                        disp=2;
                    }
                    if (key=='D') { /* cursor left */
                        where--;
                        if (where<0) where=1;
                        disp=2;
                    }
                }
            }
        }
    }
}

void print_colors() {
    int i;

    attrset(COLWHITE);
    for (i=0;i<6;i++) {
        move(YSCRCOLORS+i,XSCRCOLORS);
        printf("      ");
    }
    for (i=0;i<6;i++) {
        move(YSCRCOLORS+i,XSCRCOLORS);
        attrset(COLWHITE);
        switch (i) {
            case 0: printf("%s",CmdsColors[setting+startset].color1);
                    break;
            case 1: printf("%s",CmdsColors[setting+startset].color2);
                    break;
            case 2: printf("%s",CmdsColors[setting+startset].color3);
                    break;
            case 3: printf("%s",CmdsColors[setting+startset].color4);
                    break;
            case 4: printf("%s",CmdsColors[setting+startset].color5);
                    break;
            case 5: printf("%s",CmdsColors[setting+startset].color6);
                    break;
        }
        printf("Color%d",i+1);
    }
}

void init_sz_colors() {
    int i;

    for (i=0;i<NUMCMDCOLORS;i++) {
        strcpy(CmdsColors[i].color1,Colors[COLWHITE]);
        strcpy(CmdsColors[i].color2,Colors[COLWHITE]);
        strcpy(CmdsColors[i].color3,Colors[COLWHITE]);
        strcpy(CmdsColors[i].color4,Colors[COLWHITE]);
        strcpy(CmdsColors[i].color5,Colors[COLWHITE]);
        strcpy(CmdsColors[i].color6,Colors[COLWHITE]);
    }
    /* Warnings - floods, errors in C-Toolz.save, mass commands, */
    /*            protection violations */
    /* Warning itself */
    strcpy(CmdsColors[COLWARNING].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLWARNING].color1,Colors[COLRED]);
    /* Nick */
    strcpy(CmdsColors[COLWARNING].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLWARNING].color2,Colors[COLWHITE]);
    /* Userhost */
    strcpy(CmdsColors[COLWARNING].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLWARNING].color3,Colors[COLYELLOW]);
    /* Channel */
    strcpy(CmdsColors[COLWARNING].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLWARNING].color4,Colors[COLCYAN]);

    /* Joins */
    /* Nick */
    strcpy(CmdsColors[COLJOIN].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLJOIN].color2,Colors[COLPURPLE]);
    /* Channel */
    strcpy(CmdsColors[COLJOIN].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLJOIN].color3,Colors[COLCYAN]);
    /* Synched */
    strcpy(CmdsColors[COLJOIN].color4,Colors[COLWHITE]);
    /* Friends */
    strcpy(CmdsColors[COLJOIN].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLJOIN].color5,Colors[COLCYAN]);
    /* Shitted */
    strcpy(CmdsColors[COLJOIN].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLJOIN].color6,Colors[COLRED]);

    /* MSGs */
    /* Nick */
    strcpy(CmdsColors[COLMSG].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLMSG].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLMSG].color2,Colors[COLPURPLE]);
    /* Message */
    strcpy(CmdsColors[COLMSG].color3,Colors[COLWHITE]);
    /* Time */
    strcpy(CmdsColors[COLMSG].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLMSG].color4,Colors[COLBLACK]);
    /* [] */
    strcpy(CmdsColors[COLMSG].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLMSG].color5,Colors[COLCYAN]);
    /* Nick you sent message to */
    strcpy(CmdsColors[COLMSG].color6,Colors[COLCYAN]);

    /* Notices */
    /* Nick */
    strcpy(CmdsColors[COLNOTICE].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTICE].color1,Colors[COLGREEN]);
    /* Nick you send notice to */
    strcpy(CmdsColors[COLNOTICE].color2,Colors[COLGREEN]);
    /* Message */
    strcpy(CmdsColors[COLNOTICE].color3,Colors[COLWHITE]);
    /* <> */
    strcpy(CmdsColors[COLNOTICE].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTICE].color4,Colors[COLGREEN]);
    /* - in received notice */
    strcpy(CmdsColors[COLNOTICE].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTICE].color5,Colors[COLWHITE]);

    /* Netsplits, netjoins */
    /* Message */
    strcpy(CmdsColors[COLNETSPLIT].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLNETSPLIT].color1,Colors[COLWHITE]);
    /* Time */
    strcpy(CmdsColors[COLNETSPLIT].color2,Colors[COLWHITE]);
    /* Servers */
    strcpy(CmdsColors[COLNETSPLIT].color3,Colors[COLWHITE]);
    /* Channel */
    strcpy(CmdsColors[COLNETSPLIT].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLNETSPLIT].color4,Colors[COLCYAN]);
    /* Nicks */
    strcpy(CmdsColors[COLNETSPLIT].color5,Colors[COLCYAN]);
    /* <- */
    strcpy(CmdsColors[COLNETSPLIT].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLNETSPLIT].color6,Colors[COLYELLOW]);

    /* Invites */
    /* Nick */
    strcpy(CmdsColors[COLINVITE].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLINVITE].color2,Colors[COLPURPLE]);
    /* Channel */
    strcpy(CmdsColors[COLINVITE].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLINVITE].color3,Colors[COLCYAN]);
    /* fake word */
    strcpy(CmdsColors[COLINVITE].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLINVITE].color4,Colors[COLRED]);

    /* Mode changes */
    /* Nick */
    strcpy(CmdsColors[COLMODE].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLMODE].color2,Colors[COLWHITE]);
    /* Channel */
    strcpy(CmdsColors[COLMODE].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLMODE].color3,Colors[COLCYAN]);
    /* Mode */
    strcpy(CmdsColors[COLMODE].color4,Colors[COLWHITE]);
    /* Message */
    strcpy(CmdsColors[COLMODE].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLMODE].color5,Colors[COLWHITE]);
    /* Fake word */
    strcpy(CmdsColors[COLMODE].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLMODE].color6,Colors[COLRED]);

    /* Settings */
    /* Header */
    strcpy(CmdsColors[COLSETTING].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLSETTING].color1,Colors[COLWHITE]);
    /* Setting - ON,OFF,5,#te,... */
    strcpy(CmdsColors[COLSETTING].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLSETTING].color2,Colors[COLPURPLE]);
    /* Comment for shit list */
    strcpy(CmdsColors[COLSETTING].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLSETTING].color3,Colors[COLYELLOW]);
    /* Userhost */
    strcpy(CmdsColors[COLSETTING].color4,Colors[COLPURPLE]);
    /* Channels */
    strcpy(CmdsColors[COLSETTING].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLSETTING].color5,Colors[COLCYAN]);

    /* Leaves */
    /* Nick */
    strcpy(CmdsColors[COLLEAVE].color1,Colors[COLCYAN]);
    /* Channel */
    strcpy(CmdsColors[COLLEAVE].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLLEAVE].color2,Colors[COLCYAN]);
    /* Reason */
    strcpy(CmdsColors[COLLEAVE].color3,Colors[COLWHITE]);
    /* Friends */
    strcpy(CmdsColors[COLLEAVE].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLLEAVE].color4,Colors[COLCYAN]);
    /* Shitted */
    strcpy(CmdsColors[COLLEAVE].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLLEAVE].color5,Colors[COLRED]);
    /* Reason (for 2.9 servers) */
    strcpy(CmdsColors[COLLEAVE].color6,Colors[COLWHITE]);

    /* Notify */
    /* Nick */
    strcpy(CmdsColors[COLNOTIFY].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLNOTIFY].color2,Colors[COLPURPLE]);
    /* Time */
    strcpy(CmdsColors[COLNOTIFY].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTIFY].color3,Colors[COLBLACK]);
    /* Message */
    strcpy(CmdsColors[COLNOTIFY].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTIFY].color4,Colors[COLWHITE]);
    /* Signon-ed nicks */
    strcpy(CmdsColors[COLNOTIFY].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTIFY].color5,Colors[COLCYAN]);
    /* Signon-ed friends */
    strcpy(CmdsColors[COLNOTIFY].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLNOTIFY].color6,Colors[COLCYAN]);

    /* CTCPs */
    /* Nick */
    strcpy(CmdsColors[COLCTCP].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLCTCP].color2,Colors[COLPURPLE]);
    /* Channel */
    strcpy(CmdsColors[COLCTCP].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLCTCP].color3,Colors[COLCYAN]);
    /* Command */
    strcpy(CmdsColors[COLCTCP].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLCTCP].color4,Colors[COLCYAN]);

    /* Kicks */
    /* Nick */
    strcpy(CmdsColors[COLKICK].color1,Colors[COLCYAN]);
    /* Who */
    strcpy(CmdsColors[COLKICK].color2,Colors[COLCYAN]);
    /* Channel */
    strcpy(CmdsColors[COLKICK].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLKICK].color3,Colors[COLCYAN]);
    /* Comment */
    strcpy(CmdsColors[COLKICK].color4,Colors[COLWHITE]);
    /* Kick */
    strcpy(CmdsColors[COLKICK].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLKICK].color5,Colors[COLWHITE]);
    /* Friends */
    strcpy(CmdsColors[COLKICK].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLKICK].color6,Colors[COLCYAN]);

    /* DCCs */
    /* Nick */
    strcpy(CmdsColors[COLDCC].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLDCC].color2,Colors[COLPURPLE]);
    /* Command */
    strcpy(CmdsColors[COLDCC].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLDCC].color3,Colors[COLWHITE]);
    /* What */
    strcpy(CmdsColors[COLDCC].color4,Colors[COLCYAN]);
    /* Dcc */
    strcpy(CmdsColors[COLDCC].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLDCC].color5,Colors[COLYELLOW]);
    /* Warning */
    strcpy(CmdsColors[COLDCC].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLDCC].color6,Colors[COLRED]);

    /* WHO */
    /* Nick */
    strcpy(CmdsColors[COLWHO].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLWHO].color2,Colors[COLPURPLE]);
    /* Channel */
    strcpy(CmdsColors[COLWHO].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLWHO].color3,Colors[COLCYAN]);
    /* Mode */
    strcpy(CmdsColors[COLWHO].color4,Colors[COLWHITE]);
    /* Name */
    strcpy(CmdsColors[COLWHO].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLWHO].color5,Colors[COLWHITE]);

    /* WHOIS */
    /* Nick */
    strcpy(CmdsColors[COLWHOIS].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLWHOIS].color2,Colors[COLPURPLE]);
    /* Channels */
    strcpy(CmdsColors[COLWHOIS].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLWHOIS].color3,Colors[COLCYAN]);
    /* Server */
    strcpy(CmdsColors[COLWHOIS].color4,Colors[COLWHITE]);
    /* Channels,Server,SignOn,Idle,IrcOp */
    strcpy(CmdsColors[COLWHOIS].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLWHOIS].color5,Colors[COLBLUE]);
    /* Channels in friend */
    strcpy(CmdsColors[COLWHOIS].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLWHOIS].color6,Colors[COLRED]);

    /* Public MSGs */
    /* Nick */
    strcpy(CmdsColors[COLPUBLIC].color1,Colors[COLWHITE]);
    /* < and > */
    strcpy(CmdsColors[COLPUBLIC].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLPUBLIC].color2,Colors[COLBLUE]);
    /* Channel */
    strcpy(CmdsColors[COLPUBLIC].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLPUBLIC].color3,Colors[COLCYAN]);
    /* Auto reply */
    strcpy(CmdsColors[COLPUBLIC].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLPUBLIC].color4,Colors[COLCYAN]);
    /* Line */
    strcpy(CmdsColors[COLPUBLIC].color5,Colors[COLWHITE]);
    /* Your nick if Ego is on */
    strcpy(CmdsColors[COLPUBLIC].color6,Colors[COLCYAN]);

    /* Cdcc */
    /* Nick */
    strcpy(CmdsColors[COLCDCC].color1,Colors[COLCYAN]);
    /* Userhost */
    strcpy(CmdsColors[COLCDCC].color2,Colors[COLPURPLE]);
    /* What */
    strcpy(CmdsColors[COLCDCC].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLCDCC].color3,Colors[COLWHITE]);
    /* Line */
    strcpy(CmdsColors[COLCDCC].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLCDCC].color4,Colors[COLYELLOW]);
    /* Files/bytes */
    strcpy(CmdsColors[COLCDCC].color5,Colors[COLCYAN]);
    /* Channel */
    strcpy(CmdsColors[COLCDCC].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLCDCC].color6,Colors[COLCYAN]);

    /* Links */
    /* Server */
    strcpy(CmdsColors[COLLINKS].color1,Colors[COLCYAN]);
    /* Uplink */
    strcpy(CmdsColors[COLLINKS].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLLINKS].color2,Colors[COLCYAN]);
    /* Distance */
    strcpy(CmdsColors[COLLINKS].color3,Colors[COLBOLD]);
    strcat(CmdsColors[COLLINKS].color3,Colors[COLYELLOW]);
    /* > */
    strcpy(CmdsColors[COLLINKS].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLLINKS].color4,Colors[COLWHITE]);
    /* Border */
    strcpy(CmdsColors[COLLINKS].color5,Colors[COLPURPLE]);

    /* DCC CHAT */
    /* Nick */
    strcpy(CmdsColors[COLDCCCHAT].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLDCCCHAT].color1,Colors[COLRED]);
    /* = */
    strcpy(CmdsColors[COLDCCCHAT].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLDCCCHAT].color2,Colors[COLWHITE]);
    /* Line */
    strcpy(CmdsColors[COLDCCCHAT].color3,Colors[COLWHITE]);
    /* [ */
    strcpy(CmdsColors[COLDCCCHAT].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLDCCCHAT].color4,Colors[COLRED]);
    /* Nick you sent chat message to */
    strcpy(CmdsColors[COLDCCCHAT].color5,Colors[COLRED]);
    
    /* CSCAN */
    /* Channel */
    strcpy(CmdsColors[COLCSCAN].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLCSCAN].color1,Colors[COLCYAN]);
    /* Friends */
    strcpy(CmdsColors[COLCSCAN].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLCSCAN].color2,Colors[COLCYAN]);
    /* Ops */
    strcpy(CmdsColors[COLCSCAN].color3,Colors[COLCYAN]);
    /* Voiced */
    strcpy(CmdsColors[COLCSCAN].color4,Colors[COLPURPLE]);
    /* Normal */
    strcpy(CmdsColors[COLCSCAN].color5,Colors[COLWHITE]);
    /* Shitted */
    strcpy(CmdsColors[COLCSCAN].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLCSCAN].color6,Colors[COLRED]);

    /* Nick change */
    /* Old nick */
    strcpy(CmdsColors[COLNICK].color1,Colors[COLCYAN]);
    /* known */
    strcpy(CmdsColors[COLNICK].color2,Colors[COLWHITE]);
    /* New nick */
    strcpy(CmdsColors[COLNICK].color3,Colors[COLCYAN]);
    /* @ in Cscan */
    strcpy(CmdsColors[COLNICK].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLNICK].color4,Colors[COLGREEN]);
    /* + in Cscan */
    strcpy(CmdsColors[COLNICK].color5,Colors[COLBOLD]);
    strcat(CmdsColors[COLNICK].color5,Colors[COLPURPLE]);

    /* /ME */
    /* * or ì */
    strcpy(CmdsColors[COLME].color1,Colors[COLBOLD]);
    strcat(CmdsColors[COLME].color1,Colors[COLWHITE]);
    /* Your nick */
    strcpy(CmdsColors[COLME].color2,Colors[COLBOLD]);
    strcat(CmdsColors[COLME].color2,Colors[COLCYAN]);
    /* Nick */
    strcpy(CmdsColors[COLME].color3,Colors[COLCYAN]);
    /* Target */
    strcpy(CmdsColors[COLME].color4,Colors[COLBOLD]);
    strcat(CmdsColors[COLME].color4,Colors[COLCYAN]);
    /* Line */
    strcpy(CmdsColors[COLME].color5,Colors[COLWHITE]);
    /* Auto Reply */
    strcpy(CmdsColors[COLME].color6,Colors[COLBOLD]);
    strcat(CmdsColors[COLME].color6,Colors[COLCYAN]);
}

void disp_warning() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Error");
    attrset(COLWHITE);
    printf(" can't open file ScrollZ.save");
    colorset(COLOR1);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Error");
    attrset(COLWHITE);
    printf(" in ScrollZ.save, ");
    colorset(COLOR2);
    printf("unknown command");
    attrset(COLWHITE);
    printf(" , ");
    colorset(COLOR3);
    printf("line 7");
}

void disp_join() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" has joined channel ");
    colorset(COLOR3);
    printf("#butt");
    attrset(COLWHITE);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Join to ");
    colorset(COLOR3);
    printf("#butt");
    attrset(COLWHITE);
    printf(" is now ");
    colorset(COLOR4);
    printf("synched");
    attrset(COLWHITE);
    printf(" (0.666 seconds)");
    attrset(COLWHITE);
    move(YSCRCOLORS+10,XSCRCOLORS);
    colorset(COLOR5);
    printf("color5");
    attrset(COLWHITE);
    printf(" is for friends, ");
    colorset(COLOR1);
    printf("color1");
    attrset(COLWHITE);
    printf(" is for normal users");
    move(YSCRCOLORS+11,XSCRCOLORS);
    colorset(COLOR6);
    printf("color6");
    attrset(COLWHITE);
    printf(" is for shitted users");
}

void disp_msg() {
    move(YSCRCOLORS+8,XSCRCOLORS);
    colorset(COLOR5);
    printf("[");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR6);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR5);
    printf("]");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("Toilets rule");
    move(YSCRCOLORS+9,XSCRCOLORS);
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("*");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("* ");
    colorset(COLOR3);
    printf("Toilets rule");
    attrset(COLWHITE);
    printf("  ");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLWHITE);
    printf(" [");
    colorset(COLOR4);
    printf("12:04");
    attrset(COLWHITE);
    printf("]");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
}

void disp_notice() {
    colorset(COLOR4);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("<");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR4);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("I am the great Cornholio");
    colorset(COLOR5);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("-");
    colorset(COLOR1);
    printf("Beavis");
    colorset(COLOR5);
    printf("-");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("I am the great Cornholio");
}

void disp_netsplit() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Netsplit detected");
    attrset(COLWHITE);
    printf(" at ");
    colorset(COLOR2);
    printf("11:03");
    attrset(COLWHITE);
    printf(" : [");
    colorset(COLOR3);
    printf("irc.dumb");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR6);
    printf("<-");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("irc.cool");
    attrset(COLWHITE);
    printf("]");
    attrset(COLWHITE);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Logged netsplit information");
    colorset(COLOR4);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("Channel");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" : ");
    colorset(COLOR5);
    printf("Nicks");
    attrset(COLWHITE);
    printf("       [");
    colorset(COLOR3);
    printf("irc.dumb");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR6);
    printf("<-");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("irc.cool");
    attrset(COLWHITE);
    printf("] : [");
    colorset(COLOR2);
    printf("23");
    attrset(COLWHITE);
    printf("]");
    colorset(COLOR4);
    move(YSCRCOLORS+11,XSCRCOLORS);
    printf("#butt");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("   : ");
    colorset(COLOR5);
    printf("Beavis Butt-head");
    attrset(COLWHITE);
    move(YSCRCOLORS+12,XSCRCOLORS);
    printf("End of netsplit information");
    colorset(COLOR1);
    move(YSCRCOLORS+13,XSCRCOLORS);
    printf("Netjoined");
    attrset(COLWHITE);
    printf(" at ");
    colorset(COLOR2);
    printf("11:05");
    attrset(COLWHITE);
    printf(" : [");
    colorset(COLOR3);
    printf("irc.dumb");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR6);
    printf("<-");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("irc.cool");
    attrset(COLWHITE);
    printf("]");
    colorset(COLOR1);
    move(YSCRCOLORS+14,XSCRCOLORS);
    printf("Netsplit hack");
    attrset(COLWHITE);
    printf(" [");
    colorset(COLOR3);
    printf("irc.cool");
    attrset(COLWHITE);
    printf("] on ");
    colorset(COLOR4);
    printf("#butt");
    attrset(COLWHITE);
    printf(" by : ");
    colorset(COLOR5);
    printf("Butt-head");
}

void disp_invite() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Bat");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("bat");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("leet.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" invites you to channel ");
    colorset(COLOR3);
    printf("#te");
    colorset(COLOR1);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Bat");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("bat");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("leet.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" invites you to channel ");
    colorset(COLOR3);
    printf("#te");
    attrset(COLWHITE);
    printf(" - ");
    colorset(COLOR4);
    printf("fake");
}

void disp_mode() {
    colorset(COLOR5);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Mode change");
    attrset(COLWHITE);
    printf(" \"");
    colorset(COLOR4);
    printf("+o Beavis");
    attrset(COLWHITE);
    printf("\" on channel ");
    colorset(COLOR3);
    printf("#butt");
    attrset(COLWHITE);
    printf(" by ");
    colorset(COLOR1);
    printf("Butt-head");
    colorset(COLOR5);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Server modes");
    attrset(COLWHITE);
    printf(" \"");
    colorset(COLOR4);
    printf("+nst");
    attrset(COLWHITE);
    printf("\" on channel ");
    colorset(COLOR3);
    printf("#butt");
    attrset(COLWHITE);
    printf(" by ");
    colorset(COLOR1);
    printf("irc.cool");
    colorset(COLOR6);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("Fake");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("MODE");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("#butt");
    attrset(COLWHITE);
    printf(" \"");
    colorset(COLOR4);
    printf("-o Butt-head");
    attrset(COLWHITE);
    printf("\" from ");
    colorset(COLOR2);
    printf("irc.cool");
}

void disp_setting() {
    attrset(COLWHITE);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Found ");
    printf("@");
    printf("Beavis ");
    colorset(COLOR4);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR4);
    printf("rocks.com");
    attrset(COLWHITE);
    printf(" with access ");
    colorset(COLOR2);
    printf("ICOAUD");
    attrset(COLWHITE);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Found ");
    printf("@");
    printf("Beavis ");
    colorset(COLOR4);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR4);
    printf("bites.com");
    attrset(COLWHITE);
    printf(" with ");
    colorset(COLOR2);
    printf("no access");
    attrset(COLWHITE);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("#1  ");
    colorset(COLOR4);
    printf("Beavis@rocks.com");
    attrset(COLWHITE);
    printf("   ");
    colorset(COLOR2);
    printf("ICOAUP");
    attrset(COLWHITE);
    printf("  N ");
    colorset(COLOR5);
    printf("#butt*");
    attrset(COLWHITE);
    move(YSCRCOLORS+11,XSCRCOLORS);
    printf("#1 ");
    colorset(COLOR2);
    printf("BKI");
    attrset(COLWHITE);
    printf("   ");
    colorset(COLOR4);
    printf("BillG@Micro$oft.com");
    attrset(COLWHITE);
    move(YSCRCOLORS+12,XSCRCOLORS);
    printf("   on channels ");
    colorset(COLOR5);
    printf("*");
    attrset(COLWHITE);
    printf(" : ");
    colorset(COLOR3);
    printf("Go away dork");
    attrset(COLWHITE);
    move(YSCRCOLORS+13,XSCRCOLORS);
    printf("Added ");
    colorset(COLOR4);
    printf("Butt-head@rocks.com");
    attrset(COLWHITE);
    printf(" to your friends list");
    attrset(COLWHITE);
    move(YSCRCOLORS+14,XSCRCOLORS);
    printf("with CTCP access of ");
    colorset(COLOR2);
    printf("INVITE CHOPS OP AUTOOP UNBAN PROT");
    attrset(COLWHITE);
    move(YSCRCOLORS+15,XSCRCOLORS);
    printf("on channels ");
    colorset(COLOR5);
    printf("#butt*");
    colorset(COLOR4);
    move(YSCRCOLORS+16,XSCRCOLORS);
    printf("jay@sucks.com");
    attrset(COLWHITE);
    printf(" removed from your friends list");
    attrset(COLWHITE);
    move(YSCRCOLORS+17,XSCRCOLORS);
    printf("Fake modes display is ");
    colorset(COLOR2);
    printf("ON");
    attrset(COLWHITE);
    printf(" for channels : ");
    colorset(COLOR5);
    printf("#butt*");
    attrset(COLWHITE);
    move(YSCRCOLORS+18,XSCRCOLORS);
    printf("---------------= ");
    colorset(COLOR1);
    printf("ScrollZ settings");
    attrset(COLWHITE);
    printf(" =---------------");
    attrset(COLWHITE);
    move(YSCRCOLORS+19,XSCRCOLORS);
    printf("A-setaway time : ");
    colorset(COLOR2);
    printf("10");
    attrset(COLWHITE);
    printf(" | A-join on invite : ");
    colorset(COLOR2);
    printf("ON");
    attrset(COLWHITE);
    printf(" for ");
    colorset(COLOR5);
    printf("#butt*");
}

void disp_help() {
    move(YSCRCOLORS+8,XSCRCOLORS);
    attrset(COLWHITE);
    printf("Help for command ");
    colorset(COLOR1);
    printf("URL");
    move(YSCRCOLORS+9,XSCRCOLORS);
    attrset(COLWHITE);
    printf("Usage ");
    colorset(COLOR3);
    printf("URL");
    attrset(COLWHITE);
    printf(" [");
    colorset(COLOR3);
    printf("http://...");
    attrset(COLWHITE);
    printf("]");
    move(YSCRCOLORS+10,XSCRCOLORS);
    attrset(COLWHITE);
    printf("When you see highlighted URL, type ");
    colorset(COLOR4);
    printf("URL");
    attrset(COLWHITE);
    printf(" to save it.");
    move(YSCRCOLORS+11,XSCRCOLORS);
    attrset(COLWHITE);
    printf("Also look at ");
    colorset(COLOR2);
    printf("URLCATCH");
}

void disp_leave() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" has left channel ");
    colorset(COLOR2);
    printf("#butt");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR6);
    printf("I didn't do it");
    attrset(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Signoff: ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR3);
    printf("Shut up Butt-head");
    attrset(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    move(YSCRCOLORS+10,XSCRCOLORS);
    colorset(COLOR4);
    printf("color4");
    attrset(COLWHITE);
    printf(" is for friends, ");
    colorset(COLOR1);
    printf("color1");
    attrset(COLWHITE);
    printf(" is for normal users");
    move(YSCRCOLORS+11,XSCRCOLORS);
    colorset(COLOR5);
    printf("color5");
    attrset(COLWHITE);
    printf(" is for shitted people");
}

void disp_notify() {
    attrset(COLWHITE);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Sign");
    colorset(COLOR4);
    printf("On");
    attrset(COLWHITE);
    printf(" detected: ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" [");
    colorset(COLOR3);
    printf("11:39");
    attrset(COLWHITE);
    printf("]");
    attrset(COLWHITE);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Sign");
    colorset(COLOR4);
    printf("Off");
    attrset(COLWHITE);
    printf(" detected: ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" [");
    colorset(COLOR3);
    printf("11:39");
    attrset(COLWHITE);
    printf("]");
    move(YSCRCOLORS+10,XSCRCOLORS);
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("(");
    attrset(COLWHITE);
    printf("Present");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(") ");
    colorset(COLOR5);
    printf("Beavis");
    attrset(COLWHITE);
    printf("      [");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLWHITE);
    printf("]");
    move(YSCRCOLORS+11,XSCRCOLORS);
    attrset(COLWHITE);
    printf("          ");
    colorset(COLOR5);
    printf("Butt-head");
    attrset(COLWHITE);
    printf("   [");
    colorset(COLOR2);
    printf("Butt-head");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("bites.com");
    attrset(COLWHITE);
    printf("]");
    move(YSCRCOLORS+12,XSCRCOLORS);
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("(");
    attrset(COLWHITE);
    printf("Absent");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" ) ");
    colorset(COLOR1);
    printf("Stewart Daria McVicker");
    attrset(COLWHITE);
    move(YSCRCOLORS+13,XSCRCOLORS);
    colorset(COLOR6);
    printf("color6");
    attrset(COLWHITE);
    printf(" is for friends, ");
    colorset(COLOR1);
    printf("color1");
    attrset(COLWHITE);
    printf(" is for normal users");
}

void disp_ctcp() {
    colorset(COLOR4);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("CTCP PING");
    attrset(COLWHITE);
    printf(" reply from ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf(": 0.078 seconds");
    colorset(COLOR4);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("CTCP VERSION");
    attrset(COLWHITE);
    printf(" from ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" to ");
    colorset(COLOR3);
    printf("#butt");
}

void disp_kick() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("You");
    attrset(COLWHITE);
    printf(" have been ");
    colorset(COLOR5);
    printf("kicked");
    attrset(COLWHITE);
    printf(" from channel ");
    colorset(COLOR3);
    printf("#te");
    attrset(COLWHITE);
    printf(" by ");
    colorset(COLOR2);
    printf("Yuk");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR4);
    printf("Bye");
    attrset(COLWHITE);
    printf(")");
    move(YSCRCOLORS+9,XSCRCOLORS);
    colorset(COLOR6);
    printf("color6");
    attrset(COLWHITE);
    printf(" is for friends");
}

void disp_dcc() {
    colorset(COLOR5);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("DCC");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("SEND");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR4);
    printf("butt");
    attrset(COLWHITE);
    printf(") request from ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    move(YSCRCOLORS+9,XSCRCOLORS+4);
    colorset(COLOR6);
    printf("rejected");
    attrset(COLWHITE);
    printf(" [Port=89]");
    colorset(COLOR5);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("DCC");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("SEND");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR4);
    printf("butt 743");
    attrset(COLWHITE);
    printf(") request ");
    colorset(COLOR4);
    printf("received");
    attrset(COLWHITE);
    printf(" from");
    move(YSCRCOLORS+11,XSCRCOLORS+4);
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" (");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    colorset(COLOR6);
    move(YSCRCOLORS+12,XSCRCOLORS);
    printf("Warning");
    attrset(COLWHITE);
    printf(" fake DCC handshake detected");
}

void disp_who() {
    colorset(COLOR3);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("#butt");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf("    ");
    colorset(COLOR4);
    printf("H@");
    attrset(COLWHITE);
    printf("  ");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR5);
    printf("No pain no gain");
    attrset(COLWHITE);
    printf(")");
    colorset(COLOR3);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("#butt");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR1);
    printf("Butt-head");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR4);
    printf("H*@");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR2);
    printf("Butt-head");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("bites.com");
    attrset(COLWHITE);
    printf(" (");
    colorset(COLOR5);
    printf("Cool");
    attrset(COLWHITE);
    printf(")");
}

void disp_whois() {
    colorset(COLOR1);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("   : ");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLWHITE);
    printf(" (No pain no gain)");
    colorset(COLOR5);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Friend");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("   : ");
    attrset(COLWHITE);
    printf("[Filt] ");
    colorset(COLOR4);
    printf("Beav*@*.com");
    attrset(COLWHITE);
    printf("  [Acs] ");
    colorset(COLOR4);
    printf("ICV");
    attrset(COLWHITE);
    printf("  [Chnl] ");
    colorset(COLOR6);
    printf("#but*");
    colorset(COLOR5);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("Channels");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" : ");
    colorset(COLOR3);
    printf("#butt @#Beavis&Butt-head");
    colorset(COLOR5);
    move(YSCRCOLORS+11,XSCRCOLORS);
    printf("Server");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("   : ");
    colorset(COLOR3);
    printf("irc.cool");
    attrset(COLWHITE);
    printf(" (Cool)");
    colorset(COLOR5);
    move(YSCRCOLORS+12,XSCRCOLORS);
    printf("SetAway");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("  : ");
    attrset(COLWHITE);
    printf("(Beavis) Let's burn something");
    colorset(COLOR5);
    move(YSCRCOLORS+13,XSCRCOLORS);
    printf("IrcOp");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("    : ");
    attrset(COLWHITE);
    printf("Beavis is an IRC Operator");
    colorset(COLOR5);
    move(16,XSCRCOLORS);
    printf("SignOn");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("   : ");
    colorset(COLOR5);
    printf("Sep  8 18:52:49");
    attrset(COLWHITE);
    printf("   ");
    colorset(COLOR5);
    printf("Idle");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(" : ");
    attrset(COLWHITE);
    printf("163s (2 minutes)");
}

void disp_public() {
    move(YSCRCOLORS+8,XSCRCOLORS);
    colorset(COLOR2);
    printf("<");
    colorset(COLOR6);
    printf("Beavis");
    colorset(COLOR2);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("That was cool");
    move(YSCRCOLORS+9,XSCRCOLORS);
    colorset(COLOR2);
    printf("<");
    colorset(COLOR4);
    printf("Butt-head");
    colorset(COLOR2);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("Hey Beavis check this out");
    move(YSCRCOLORS+10,XSCRCOLORS);
    colorset(COLOR2);
    printf("<");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf(":");
    colorset(COLOR3);
    printf("#butt");
    colorset(COLOR2);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("Let's burn something");
}

void disp_cdcc() {
    colorset(COLOR4);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Cdcc");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("created new pack");
    attrset(COLWHITE);
    printf(" : [");
    colorset(COLOR5);
    printf("17.69 kB/3 files");
    attrset(COLWHITE);
    printf("]");
    colorset(COLOR4);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Cdcc");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("sending");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" : [");
    colorset(COLOR5);
    printf("17.69 kB/3 files");
    attrset(COLWHITE);
    printf("]");
    colorset(COLOR4);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("Cdcc");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("list");
    attrset(COLWHITE);
    printf(" request received from ");
    colorset(COLOR1);
    printf("Beavis");
    move(YSCRCOLORS+11,XSCRCOLORS+5);
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("(");
    colorset(COLOR2);
    printf("Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    colorset(COLOR2);
    printf("rocks.com");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf(")");
    attrset(COLWHITE);
    printf(" to ");
    colorset(COLOR6);
    printf("#butt");
    colorset(COLOR4);
    move(YSCRCOLORS+12,XSCRCOLORS);
    printf("Cdcc");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("SEND");
    attrset(COLWHITE);
    printf(" to ");
    colorset(COLOR1);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" added to queue at position ");
    colorset(COLOR5);
    printf("5");
}

void disp_links() {
    colorset(COLOR5);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf(".");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR1);
    printf("No");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR5);
    printf(". .---");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR1);
    printf("Server");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR5);
    printf("---.");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf(".");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR3);
    printf("Ds");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR5);
    printf(".   .---");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR2);
    printf("Uplink");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("-");
    colorset(COLOR5);
    printf("---.");
    colorset(COLOR5);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("|");
    colorset(COLOR1);
    printf("  1");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR1);
    printf("      irc.cool");
    colorset(COLOR5);
    printf("|");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR3);
    printf("  0");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR4);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR2);
    printf("      irc.dumb");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR5);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("|");
    colorset(COLOR1);
    printf("  2");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR1);
    printf("irc.global.net");
    colorset(COLOR5);
    printf("|");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR3);
    printf("  1");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR4);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR2);
    printf("   irc.bgx.com");
    colorset(COLOR5);
    printf("|");
    colorset(COLOR5);
    move(YSCRCOLORS+11,XSCRCOLORS);
    printf("`----' `--------------' `----'   `--------------'");
}

void disp_dccchat() {
    colorset(COLOR4);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("[");
    colorset(COLOR2);
    printf("=");
    colorset(COLOR5);
    printf("Beavis");
    colorset(COLOR2);
    printf("=");
    colorset(COLOR4);
    printf("]");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("Settle down Beavis");
    colorset(COLOR2);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("=");
    colorset(COLOR1);
    printf("Beavis");
    colorset(COLOR2);
    printf("=");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("No pain no gain");
}

void disp_cscan() {
    attrset(COLWHITE);
    move(YSCRCOLORS+8,XSCRCOLORS);
    printf("Users on ");
    colorset(COLOR1);
    printf("#butt");
    attrset(COLWHITE);
    printf(" are : @");
    colorset(COLOR2);
    printf("Butt-head");
    attrset(COLWHITE);
    printf(" @");
    colorset(COLOR3);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" +");
    colorset(COLOR4);
    printf("Tod");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("Stewart");
    attrset(COLWHITE);
    move(YSCRCOLORS+9,XSCRCOLORS);
    printf("Found @");
    colorset(COLOR2);
    printf("Butt-head");
    attrset(COLWHITE);
    printf(" Butt-head");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    attrset(COLWHITE);
    printf("leet.com with access ICOAUP");
    attrset(COLWHITE);
    move(YSCRCOLORS+10,XSCRCOLORS);
    printf("Found @");
    colorset(COLOR3);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" Beavis");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    attrset(COLWHITE);
    printf("rocks.com with no access");
    attrset(COLWHITE);
    move(YSCRCOLORS+11,XSCRCOLORS);
    printf("Found +");
    colorset(COLOR4);
    printf("Tod");
    attrset(COLWHITE);
    printf(" Tod");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    attrset(COLWHITE);
    printf("rocks.com with no access");
    attrset(COLWHITE);
    move(YSCRCOLORS+12,XSCRCOLORS);
    printf("Found ");
    colorset(COLOR5);
    printf("Stewart");
    attrset(COLWHITE);
    printf(" Stewart");
    attrset(COLBOLD);
    attradd(COLWHITE);
    printf("@");
    attrset(COLWHITE);
    printf("sucks.com with no access");
    move(YSCRCOLORS+13,XSCRCOLORS);
    colorset(COLOR6);
    printf("color6");
    attrset(COLWHITE);
    printf(" is for shitted people");
}

void disp_nick() {
    move(YSCRCOLORS+8,XSCRCOLORS);
    colorset(COLOR1);
    printf("Butt-head");
    attrset(COLWHITE);
    printf(" is now ");
    colorset(COLOR2);
    printf("known");
    attrset(COLWHITE);
    printf(" as ");
    colorset(COLOR3);
    printf("Butt-cool");
    move(YSCRCOLORS+9,XSCRCOLORS);
    attrset(COLWHITE);
    printf("Users on #butt are : ");
    colorset(COLOR4);
    printf("@");
    attrset(COLWHITE);
    printf("Butt-head ");
    colorset(COLOR5);
    printf("+");
    attrset(COLWHITE);
    printf("Beavis");
}

void disp_me() {
    move(YSCRCOLORS+8,XSCRCOLORS);
    colorset(COLOR1);
    printf("*");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR3);
    printf("Beavis");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("rulez the world");
    move(YSCRCOLORS+9,XSCRCOLORS);
    attrset(COLWHITE);
    printf("<");
    colorset(COLOR4);
    printf("#butt");
    attrset(COLWHITE);
    printf(">");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR1);
    printf("*");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR2);
    printf("Butt-head");
    attrset(COLWHITE);
    printf(" ");
    colorset(COLOR5);
    printf("has lots of chicks");
}
