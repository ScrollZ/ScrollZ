/*
 * $Id: SZsetup.h,v 1.3 2002-03-05 17:50:07 f Exp $
 */

#define mybufsize     1024
#define NUMCMDCOLORS  23
#define NUMCOLORS     21
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
/* Index of commands in CmdsColors table */
#define COLWARNING   0
#define COLJOIN      1
#define COLMSG       2
#define COLNOTICE    3
#define COLNETSPLIT  4
#define COLINVITE    5
#define COLMODE      6
#define COLSETTING   7
#define COLLEAVE     8
#define COLNOTIFY    9
#define COLCTCP      10
#define COLKICK      11
#define COLDCC       12
#define COLWHO       13
#define COLWHOIS     14
#define COLPUBLIC    15
#define COLCDCC      16
#define COLLINKS     17
#define COLDCCCHAT   18
#define COLCSCAN     19
#define COLNICK      20
#define COLME        21
#define COLMISC      22

#define COLOR1 CmdsColors[setting+startset].color1
#define COLOR2 CmdsColors[setting+startset].color2
#define COLOR3 CmdsColors[setting+startset].color3
#define COLOR4 CmdsColors[setting+startset].color4
#define COLOR5 CmdsColors[setting+startset].color5
#define COLOR6 CmdsColors[setting+startset].color6

#define XSETTINGS  1
#define YSETTINGS  2
#define XCOLORS    12
#define YCOLORS    1
#define XSCRCOLORS 25
#define YSCRCOLORS 2
#define XMESSAGE   1
#define YMESSAGE   23
#define COLSELECT  COLCYANBG

struct colorstr {
    char color1[64];
    char color2[64];
    char color3[64];
    char color4[64];
    char color5[64];
    char color6[64];
};

struct setstr {
    char *name;
    void (*func)();
};

int  set_color();
void  set_term();
void  reset_term();
void read_file();
void init_sz_colors();
void move();
void addstr();
void attrset();
void attradd();
void draw_screen();
char *get_color_name();
void get_colors();
void save_colors();
void print_colors();
void fix_attr();
void fix_color();
void do_it();
void disp_warning();
void disp_join();
void disp_msg();
void disp_notice();
void disp_netsplit();
void disp_invite();
void disp_mode();
void disp_setting();
void disp_help();
void disp_leave();
void disp_notify();
void disp_ctcp();
void disp_kick();
void disp_dcc();
void disp_who();
void disp_whois();
void disp_public();
void disp_cdcc();
void disp_links();
void disp_dccchat();
void disp_cscan();
void disp_nick();
void disp_me();
void disp_misc();
