/*
 * This little piece of code should make life easier for ScrollZ distro team.
 *
 * Be sure to follow next steps, otherwise things won't work !!!
 * First of all, run configure and compile client with your defaults. Next,
 * compile this program and put it in main ScrollZ dir (this *IS* important).
 * It needs include/defs.h and include/defs.h.in to operate plus working rm
 * (yes, the UN*X command).
 *
 * Once you qualify for requirements, you're ready to use it. Just run it and
 * you'll be present with current ScrollZ setup (read from include/defs.h).
 * Follow on-screen instructions and make your choices. Either use 'Q' to quit
 * or press 'S' to save. If you opted for latter, just type make irc and in a
 * matter of minutes you will have binary of your choice.
 *
 * Now that ScrollZ has a lot of compile-time options, we needed this so you
 * can simply ask user what options he wants, run this program, select those
 * options and in a few minutes you can send them their own personalized copy
 * of ScrollZ.
 *
 * Note that this should reduce compile time too since it only recompiles
 * files that are affected by selected options. If you need more info or you
 * have comments on this code, send e-mail to:
 * flier@globecom.net
 * flier@3sheep.com or
 * 
 * $Id: SZdist.c,v 1.5 1998-10-24 10:30:04 f Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>

#define WANTANSI 1
#define EXTRAS 2
#define BETTERTIMER 4
#define GENX 8
#define NEWCSCAN 16
#define ACID 32
#define MGS 64
#define SCKICKS 128
#define OPERVISION 256
#define CELE 512
#define HYPERDCC 1024
#define VILAS 2048
#define JIMMIE 4096
#define CTCPPAGE 8192
#define TDF 16384
#define COUNTRY 32768
#define SZ32 65536
#define SZNCURSES 131072
#define IPCHECKING 262144
#define NUMDEFS 262144

#define mybufsize 1024

char *int_ver="19980912";
char *ver="ircII 4.4A+ScrollZ v1.8i2 (12.09.98)+Cdcc v1.7";
char *chars="ABCDEFGHIJRSTUVWXYZ.*[]0123|_-{}/=+klmnopqabcdefghijrstuvwxyz456789KLMNOPQ!#$^?():'% ";
char *defsfile="include/defs.h";
char *defsoldfile="include/defs.h.old";
char *ircfile="source/files.c";
char *ircoldfile="source/files.c.old";
char *irctmpfile="source/files.c.tmp";

char *WANTANSIfiles="alias.o cdcc.o dcc.o edit.o edit2.o edit3.o edit4.o edit5.o\
 edit6.o input.o log.o names.o numbers.o parse.o screen.o status.o term.o vars.o\
 whois.o";
char *EXTRASfiles="alias.o cdcc.o edit.o edit2.o edit3.o edit4.o edit5.o edit6.o\
 list.o names.o numbers.o parse.o whowas.o";
char *BETTERTIMERfiles="edit.o";
char *GENXfiles="edit5.o whois.o";
char *NEWCSCANfiles="edit2.o";
char *ACIDfiles="edit.o edit2.o edit3.o edit5.o edit6.o";
char *MGSfiles="edit.o edit3.o edit5.o names.o";
char *SCKICKSfiles="edit.o edit3.o edit4.o";
char *OPERVISIONfiles="edit.o edit3.o edit5.o funny.o notice.o operv.o parse.o\
 window.o";
char *CELEfiles="alias.o celerity.o edit.o edit2.o edit3.o edit4.o edit5.o edit6.o\
 numbers.o parse.o server.o vars.o whois.o";
char *HYPERDCCfiles="cdcc.o dcc.o edit.o edit4.o edit6.o";
char *VILASfiles="edit.o edit2.o edit3.o edit4.o edit5.o edit6.o names.o\
 numbers.o server.o";
char *JIMMIEfiles="edit2.o";
char *CTCPPAGEfiles="ctcp.o";
char *TDFfiles="cdcc.o dcc.o edit.o edit4.o edit5.o edit6.o status.o";
char *COUNTRYfiles="alias.o";
char *SZ32files="edit.o help.o irc.o ircaux.o scandir.o server.o term.o window.o";
char *SZNCURSESfiles="edit.o edit4.o input.o irc.o menu.o output.o parse.o screen.o\
 status.o term.o";
char *IPCHECKINGfiles="edit2.o edit3.o edit6.o files.o parse.o";

char format[mybufsize];

void reg(regname,ip)
char *regname;
char *ip;
{
    char tstr[mybufsize];
    char tmpstr[mybufsize];
    char ipstr[mybufsize];
    char tmpbuf[mybufsize];
    char tmpbuf1[mybufsize];
    int  i,j,k,l,found;
    char *tmp1,*tmp2,*tmp3,*tmp4,*verstr;
    FILE *fp,*inf;

    inf=fopen(ircfile,"r");
    if (inf==NULL) {
        printf("Can't open phile %s for reading!\n",ircfile);
        printf("Press any key to continue\n");
        getchar();
        return;
    }
    fclose(inf);
    unlink(ircoldfile);
    link(ircfile,ircoldfile);
    if ((inf=fopen(ircoldfile,"r"))==NULL) {
        printf("Can't open phile %s for reading!\n",ircoldfile);
        printf("Press any key to continue\n");
        getchar();
        return;
    }
    if ((fp=fopen(irctmpfile,"w"))==NULL) {
        printf("Can't open phile %s for writing!\n",irctmpfile);
        printf("Press any key to continue\n");
        getchar();
        fclose(inf);
        return;
    }
    printf("Enter registration name : ");
    getchar();
    fgets(tmpstr,mybufsize,stdin);
    if (strlen(tmpstr) && tmpstr[strlen(tmpstr)-1]=='\n') tmpstr[strlen(tmpstr)-1]='\0';
    if (index(tmpstr,'#')) {
        printf("You can't use character # in registration name!\n");
        printf("Press any key to continue\n");
        getchar();
        fclose(inf);
        fclose(fp);
        return;
    }
    printf("Enter valid IP : ");
    fgets(ipstr,mybufsize,stdin);
    if (strlen(ipstr) && ipstr[strlen(ipstr)-1]=='\n') ipstr[strlen(ipstr)-1]='\0';
    if (index(ipstr,'#')) {
        printf("You can't use character # in IP!\n");
        printf("Press any key to continue\n");
        getchar();
        fclose(inf);
        fclose(fp);
        return;
    }
    strcpy(regname,tmpstr);
    strcpy(ip,ipstr);
    i=1;
    strcpy(tmpbuf,ver);
    for (l=0,tmp2=tmpbuf;l<2;tmp2++) if (*tmp2==' ') l++;
    for (l=0,tmp1=tmp2;l<2;tmp1++) if (*tmp1==' ') l++;
    tmp1--;
    *tmp1='\0';
    sprintf(tstr,"%s%s",int_ver,tmp2);
    verstr=tstr;
    for (tmp1=format,tmp2=tmpbuf1;*tmp1;tmp1++)
        if (*tmp1=='%' && i) {
            for (tmp3=tmpstr;*tmp3;tmp3++)
                *tmp2++=*tmp3;
            tmp1++;
            i=0;
        }
        else *tmp2++=*tmp1;
    *tmp2='\0';
    for (tmp1=tmpbuf1,tmp2=tmpbuf;*tmp1;tmp1++)
        if (*tmp1=='%') {
            for (tmp3=ipstr;*tmp3;tmp3++)
                *tmp2++=*tmp3;
            tmp1++;
        }
        else *tmp2++=*tmp1;
    *tmp2='\0';
    for (k=-33,tmp1=int_ver;*tmp1;tmp1++) k+=*tmp1;
    k+=strlen(int_ver);
    l=strlen(chars);
    tmp1=tmpbuf;
    tmp3=tmpbuf1;
    while (*tmp1) {
        for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
        if (!(*tmp4)) {
            printf("Character %c (%d/%X) is wrong!\n",*tmp1,*tmp1,*tmp1);
            exit(1);
        }
        j=k-2*l-i-(tmp1-tmpbuf);
        while (j<0) j+=l;
        while (j>=l) j-=l;
        j+=27;
        if (j>=l) j-=l;
        *tmp3=chars[j];
        tmp1++;
        tmp3++;
    }
    *tmp3='\0';
    tmp1=tmpbuf1;
    tmp2=verstr;
    tmp3=tmpbuf;
    while (*tmp1) {
        for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
        if (!(*tmp4)) {
            printf("\nCharacter %c (%d/%X) is wrong!\n",*tmp1,*tmp1,*tmp1);
            exit(1);
        }
        for (tmp4=chars,j=0;*tmp4;tmp4++,j++) if (*tmp4==*tmp2) break;
        if (!(*tmp4)) {
            printf("\nUhm something went wrong, tell Flier about it :P   Character %c (%d/%X) is illegal!\n",*tmp1,*tmp1,*tmp1);
            exit(1);
        }
        *tmp3=chars[(i+j)%l];
        tmp1++;
        tmp2++;
        tmp3++;
        if (!(*tmp2)) tmp2=verstr;
    }
    *tmp3='\0';
    tmp3++;
    l++;
    for (;l<256;l++,tmp3++) {
        *tmp3=rand()%256;
        if (*tmp3<0) *tmp3+=128;
    }
    found=0;
    while (!found && fgets(tstr,mybufsize,inf)) {
        tstr[strlen(tstr)-1]='\0';
        if (!strncmp(tstr,"char    global_track[256]={",27)) found=1;
        else fprintf(fp,"%s\n",tstr);
    }
    if (!found) {
        fclose(fp);
        fclose(inf);
        printf("You gave muh wrong file fartknocker!\n");
        printf("Press any key to continue\n");
        getchar();
        return;
    }
    fprintf(fp,"char    global_track[256]={");
    l=27;
    for (i=1;i<257;i++) {
        l++;
        fprintf(fp,"%d",tmpbuf[i-1]);
        if (i<256) fprintf(fp,",");
        if (tmpbuf[i-1]<10) l++;
        else if (tmpbuf[i-1]<100) l+=2;
        else l+=3;
        if (l>=85) {
            fprintf(fp,"\n        ");
            l=9;
        }
    }
    fprintf(fp,"\n        };\n");
    found=0;
    while (!found && fgets(tstr,mybufsize,inf)) {
        tstr[strlen(tstr)-1]='\0';
        if (!strcmp(tstr,"        };")) found=1;
    }
    while (fgets(tstr,mybufsize,inf)) {
        tstr[strlen(tstr)-1]='\0';
        fprintf(fp,"%s\n",tstr);
    }
    fclose(fp);
    fclose(inf);
    unlink(ircfile);
    rename(irctmpfile,ircfile);
}

char *onoffstr(type,onoffbuf)
int type;
char *onoffbuf;
{
    strcpy(onoffbuf,"[1m");
    if (type) strcat(onoffbuf,"ON ");
    else strcat(onoffbuf,"OFF");
    strcat(onoffbuf,"[0m");
    return(onoffbuf);
}

void addtobuf(files,buffer,choice,oldchoice,what)
char *files;
char *buffer;
int  choice;
int  oldchoice;
int  what;
{
    char buf[32];
    char tmpbuf[mybufsize];
    char filebuf[64];
    char *tmpstr1=tmpbuf;
    char *tmpstr2;

    if ((choice&what)==(oldchoice&what)) return;
    strcpy(tmpbuf,files);
    tmpstr2=strtok(tmpstr1," ");
    do {
        sprintf(filebuf," %s",tmpstr2);
        if (!strstr(buffer,filebuf)) {
            if (strstr(tmpstr2,".c")) {
                printf("Not deleting %s, because it is .c file !!\n",tmpstr2);
                getchar();
            }
            else {
                sprintf(buf," source/%s",tmpstr2);
                strcat(buffer,buf);
            }
        }
    } while ((tmpstr2=strtok(NULL," ")));
}

void touchfile(file,statbuf)
char *file;
struct stat *statbuf;
{
    struct utimbuf utimbuf;

    utimbuf.actime=statbuf->st_atime;
    utimbuf.modtime=statbuf->st_mtime;
    if ((utime(file,&utimbuf))!=0)
        printf("Error, couldn't set time on file %s\n",file);
}

void locatelog(pathbuf,fname)
char *pathbuf;
char *fname;
{
    char *curpath,*tmpp;
    char fpath[mybufsize],cpath[mybufsize];
    FILE *fp;

    *pathbuf='\0';
    curpath=getenv("PATH");
    if (curpath==NULL) return;
    strcpy(fpath,curpath);
    curpath=fpath;
    do {
        tmpp=index(curpath,':');
	if (tmpp) *tmpp++='\0';
        sprintf(cpath,"%s/%s",curpath,fname);
        if ((fp=fopen(cpath,"r"))) {
            strcpy(pathbuf,curpath);
            strcat(pathbuf,"/");
            return;
	}
	curpath=tmpp;
    } while (curpath);
}

void main(argc,argv)
int argc;
char **argv;
{
    int  choice=0,oldchoice=0;
    int  end=0,gotit=0;
    int  i,j,k,l;
    char c;
    char tmpbuf[2*mybufsize];
    char onoffbuf[32];
    char buf[mybufsize],pathbuf[mybufsize],filebuf[mybufsize];
    char regname[mybufsize],ip[mybufsize];
    char password[mybufsize];
    char *tmp1,*tmp2,*tmp3,*tmp4;
    FILE *fpin=NULL,*fpout;
    time_t timenow;
    struct stat statbuf;

    strcpy(format,"KBdAAIFeHRY0COtvT5UM0|-PO=_^R$t");
    locatelog(pathbuf,"SZdist");
    if ((fpin=fopen(defsfile,"r"))==NULL || stat(defsfile,&statbuf)!=0) {
        printf("Error, couldn't open %s for reading\n",defsfile);
        if (fpin) fclose(fpin);
        return;
    }
    printf("Enter password:");
    fgets(password,mybufsize,stdin);
    if (strlen(password) && password[strlen(password)-1]=='\n')
        password[strlen(password)-1]='\0';
    l=strlen(chars);
    if ((strlen(password))) {
        tmp2=password;
        k=-77;
        for (tmp1=tmp2;*tmp1;tmp1++) k+=*tmp1;
        k+=strlen(tmp2);
        tmp1=format;
        tmp3=tmpbuf;
        while (*tmp1) {
            for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
            if (!(*tmp4)) {
                printf("Character %c (%d/%X) is wrong in format!\n",*tmp1,*tmp1,*tmp1);
                exit(1);
            }
            for (tmp4=chars,j=0;*tmp4;tmp4++,j++) if (*tmp4==*tmp2) break;
            if (!(*tmp4)) {
                printf("Uhm something went wrong with format, tell Flier about it :P   Character %c (%d/%X) is illegal!\n",
                       *tmp2,*tmp2,*tmp2);
                exit(1);
            }
            i-=j;
            if (i<0) i+=l;
            *tmp3=chars[i];
            tmp1++;
            tmp2++;
            tmp3++;
            if (!(*tmp2)) tmp2=password;
        }
        *tmp3='\0';
        tmp1=tmpbuf;
        tmp3=format;
        while (*tmp1) {
            for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
            if (!(*tmp4)) {
                printf("Character %c (%d/%X) is illegal in format!\n",*tmp1,*tmp1,*tmp1);
                exit(1);
            }
            j=l-i+(tmp1-tmpbuf)-1+k;
            while (j<0) j+=l;
            while (j>=l) j-=l;
            *tmp3=chars[j];
            tmp1++;
            tmp3++;
        }
        *tmp3='\0';
    }
    tmp1=index(format,'%');
    if (tmp1 && *(tmp1-1)==' ' && *(tmp1+1)=='s' && *(tmp1+2)==' ' && *(tmp1+3)=='#' &&
        *(tmp1+4)=='%' && *(tmp1+5)=='s' && !(*(tmp1+6))) gotit=1;
    while (fgets(buf,1024,fpin))
        if (strstr(buf,"#define")) {
            if (strstr(buf,"WANTANSI")) choice|=WANTANSI;
            else if (strstr(buf,"EXTRAS")) choice|=EXTRAS;
            else if (strstr(buf,"BETTERTIMER")) choice|=BETTERTIMER;
            else if (strstr(buf,"GENX")) choice|=GENX;
            else if (strstr(buf,"NEWCSCAN")) choice|=NEWCSCAN;
            else if (strstr(buf,"ACID")) choice|=ACID;
            else if (strstr(buf,"MGS")) choice|=MGS;
            else if (strstr(buf,"SCKICKS")) choice|=SCKICKS;
            else if (strstr(buf,"OPERVISION")) choice|=OPERVISION;
	    else if (strstr(buf,"CELE")) choice|=CELE;
	    else if (strstr(buf,"HYPERDCC")) choice|=HYPERDCC;
	    else if (strstr(buf,"VILAS")) choice|=VILAS;
	    else if (strstr(buf,"JIMMIE")) choice|=JIMMIE;
	    else if (strstr(buf,"CTCPPAGE")) choice|=CTCPPAGE;
	    else if (strstr(buf,"TDF")) choice|=TDF;
	    else if (strstr(buf,"COUNTRY")) choice|=COUNTRY;
	    else if (strstr(buf,"SZ32")) choice|=SZ32;
	    else if (strstr(buf,"SZNCURSES")) choice|=SZNCURSES;
	    else if (strstr(buf,"IPCHECKING")) choice|=IPCHECKING;
        }
    fclose(fpin);
    oldchoice=choice;
    if (argc==2) {
        for (tmp1=argv[1];*tmp1 && *tmp1!=' ';tmp1++) {
            if (*tmp1=='A') choice|=WANTANSI;
            else if (*tmp1=='a') choice&=~WANTANSI;
            if (*tmp1=='E') choice|=EXTRAS;
            else if (*tmp1=='e') choice&=~EXTRAS;
            if (*tmp1=='C') choice|=NEWCSCAN;
            else if (*tmp1=='c') choice&=~NEWCSCAN;
            if (*tmp1=='T') choice|=BETTERTIMER;
            else if (*tmp1=='t') choice&=~BETTERTIMER;
            if (*tmp1=='S') choice|=SCKICKS;
            else if (*tmp1=='s') choice&=~SCKICKS;
            if (*tmp1=='D') choice|=HYPERDCC;
            else if (*tmp1=='d') choice&=~HYPERDCC;
            if (*tmp1=='P') choice|=CTCPPAGE;
            else if (*tmp1=='p') choice&=~CTCPPAGE;
            if (*tmp1=='Y') choice|=COUNTRY;
            else if (*tmp1=='y') choice&=~COUNTRY;
            if (*tmp1=='X') choice|=IPCHECKING;
            else if (*tmp1=='x') choice&=~IPCHECKING;
        }
        if (*tmp1==' ' && *(tmp1+1)=='O' && *(tmp1+2)=='V') {
            choice|=OPERVISION;
            tmp1++;
            tmp1++;
            tmp1++;
        }
        else choice&=~OPERVISION;
        /*if (*tmp1==' ' && *(tmp1+1)=='c' && *(tmp1+2)=='y') {
            choice|=CELE;
            tmp1++;
            tmp1++;
            tmp1++;
        }
        else choice&=~CELE;*/
        if (*tmp1==' ') {
            tmp1++;
            for (;*tmp1 && *tmp1!=' ';tmp1++) {
                if (*tmp1=='G') choice|=GENX;
                else if (*tmp1=='g') choice&=~GENX;
                if (*tmp1=='I') choice|=ACID;
                else if (*tmp1=='i') choice&=~ACID;
                if (*tmp1=='V') choice|=VILAS;
                else if (*tmp1=='v') choice&=~VILAS;
                if (*tmp1=='J') choice|=JIMMIE;
                else if (*tmp1=='j') choice&=~JIMMIE;
                if (*tmp1=='X') choice|=TDF;
                else if (*tmp1=='x') choice&=~TDF;
            }
        }
    }
    strcpy(tmpbuf,"rm source/irc.o source/ctcp.o");
    strcpy(regname,"None");
    strcpy(ip,"0.0.0.0");
    do {
        printf("[2J[1;1H");
        printf("Enter [1mletter[0m to toggle option on/off, '[1mQ[0m' to quit, '[1mS[0m' to save and quit%c\n",
               gotit?'.':' ');
        printf("-------------------------------------------------------------------------------\n");
        printf(" [1mA[0m - WANTANSI      %s - color capable client\n",
               onoffstr(choice&WANTANSI,onoffbuf));
        printf(" [1mB[0m - EXTRAS        %s - enables commands like AUTOINV, BKT, DIRLMK...\n",
               onoffstr(choice&EXTRAS,onoffbuf));
        printf(" [1mC[0m - BETTERTIMER   %s - TIMER that's accurate to 0.01s \n",
               onoffstr(choice&BETTERTIMER,onoffbuf));
        printf(" [1mD[0m - GENX          %s - GenX's nifty WHOIS, req. WANTANSI\n",
               onoffstr(choice&GENX,onoffbuf));
        printf(" [1mE[0m - NEWCSCAN      %s - formatted CSCAN, similar to BitchX\n",
               onoffstr(choice&NEWCSCAN,onoffbuf));
        printf(" [1mF[0m - ACID          %s - invite on notify for non +i channels, req. EXTRAS\n",
               onoffstr(choice&ACID,onoffbuf));
        printf(" [1mG[0m - MGS           %s - sorted nicks in CSCAN and TERMINATE\n",
               onoffstr(choice&MGS,onoffbuf));
        printf(" [1mH[0m - SCKICKS       %s - scatter (funny) kicks\n",
               onoffstr(choice&SCKICKS,onoffbuf));
        printf(" [1mI[0m - OPERVISION    %s - for IRC Operators, req. WANTANSI\n",
               onoffstr(choice&OPERVISION,onoffbuf));
	printf(" [1mJ[0m - CELE          %s - Compile with Celerity C-script\n",
	       onoffstr(choice&CELE,onoffbuf));
	printf(" [1mK[0m - HYPERDCC      %s - Compile with HyperDCC by Annatar\n",
	       onoffstr(choice&HYPERDCC,onoffbuf));
	printf(" [1mL[0m - VILAS         %s - No ScrollZ trademarks in kicks and away msgs\n",
	       onoffstr(choice&VILAS,onoffbuf));
	printf(" [1mM[0m - JIMMIE        %s - Better NEWHOST (lists all hostnames)\n",
	       onoffstr(choice&JIMMIE,onoffbuf));
	printf(" [1mN[0m - CTCP PAGE     %s - CTCP PAGE for friends by BiGhEaD\n",
	       onoffstr(choice&CTCPPAGE,onoffbuf));
	printf(" [1mO[0m - TDF           %s - different msgs, chat msgs and CDCC\n",
	       onoffstr(choice&TDF,onoffbuf));
	printf(" [1mP[0m - COUNTRY       %s - compile with $country()\n",
	       onoffstr(choice&COUNTRY,onoffbuf));
	printf(" [1m3[0m - SZ32          %s - compile for Win32 (NT+95)\n",
	       onoffstr(choice&SZ32,onoffbuf));
	printf(" [1mU[0m - SZNCURSES     %s - compile with ncurses colors (W95)\n",
	       onoffstr(choice&SZNCURSES,onoffbuf));
	printf(" [1mX[0m - IPCHECKING    %s - compile with IP checking\n",
	       onoffstr(choice&IPCHECKING,onoffbuf));
        printf(" [1mR[0m - REGISTER      Name: [1m%s[0m   IP: [1m%s[0m\n",regname,ip);
        printf(" [1mQ[0m - QUIT          [1mS[0m - SAVE & QUIT\n");
        printf("Enter your choice: ");
        c=getchar();
        if (c>='a' && c<='z') c-=32;
        if (c=='Q') end=1;
        else if (c=='S') end=2;
        else {
            switch (c) {
                case 'A': if ((choice&WANTANSI)) choice&=~WANTANSI;
                          else choice|=WANTANSI;
                          break;
                case 'B': if ((choice&EXTRAS)) choice&=~EXTRAS;
                          else choice|=EXTRAS;
                          break;
                case 'C': if ((choice&BETTERTIMER)) choice&=~BETTERTIMER;
                          else choice|=BETTERTIMER;
                          break;
                case 'D': if ((choice&GENX)) choice&=~GENX;
                          else choice|=GENX;
                          break;
                case 'E': if ((choice&NEWCSCAN)) choice&=~NEWCSCAN;
                          else choice|=NEWCSCAN;
                          break;
                case 'F': if ((choice&ACID)) choice&=~ACID;
                          else choice|=ACID;
                          break;
                case 'G': if ((choice&MGS)) choice&=~MGS;
                          else choice|=MGS;
                          break;
                case 'H': if ((choice&SCKICKS)) choice&=~SCKICKS;
                          else choice|=SCKICKS;
                          break;
                case 'I': if ((choice&OPERVISION)) choice&=~OPERVISION;
                          else choice|=OPERVISION;
                          break;
		/*case 'J': if ((choice&CELE)) choice&=~CELE;
			  else choice|=CELE;
			  break;*/
		case 'K': if ((choice&HYPERDCC)) choice&=~HYPERDCC;
			  else choice|=HYPERDCC;
			  break;
		case 'L': if ((choice&VILAS)) choice&=~VILAS;
			  else choice|=VILAS;
			  break;
		case 'M': if ((choice&JIMMIE)) choice&=~JIMMIE;
			  else choice|=JIMMIE;
			  break;
		case 'N': if ((choice&CTCPPAGE)) choice&=~CTCPPAGE;
			  else choice|=CTCPPAGE;
			  break;
		case 'O': if ((choice&TDF)) choice&=~TDF;
			  else choice|=TDF;
			  break;
		case 'P': if ((choice&COUNTRY)) choice&=~COUNTRY;
			  else choice|=COUNTRY;
			  break;
		case '3': if ((choice&SZ32)) choice&=~SZ32;
			  else choice|=SZ32;
			  break;
		case 'U': if ((choice&SZNCURSES)) choice&=~SZNCURSES;
			  else choice|=SZNCURSES;
			  break;
		case 'X': if ((choice&IPCHECKING)) choice&=~IPCHECKING;
			  else choice|=IPCHECKING;
			  break;
                case 'R': if ((choice&IPCHECKING)) reg(regname,ip);
                          break;

            }
        }
        if (!(choice&WANTANSI)) choice&=~OPERVISION;
        if (!(choice&WANTANSI)) choice&=~GENX;
        if (!(choice&WANTANSI)) choice&=~TDF;
        if (!(choice&EXTRAS)) choice&=~ACID;
    } while (!end);
    if (end==2) {
        for (i=1;i<=NUMDEFS;i*=2) {
            if (i==WANTANSI) addtobuf(WANTANSIfiles,tmpbuf,choice,oldchoice,i);
            else if (i==EXTRAS) addtobuf(EXTRASfiles,tmpbuf,choice,oldchoice,i);
            else if (i==BETTERTIMER) addtobuf(BETTERTIMERfiles,tmpbuf,choice,oldchoice,i);
            else if (i==GENX) addtobuf(GENXfiles,tmpbuf,choice,oldchoice,i);
            else if (i==NEWCSCAN) addtobuf(NEWCSCANfiles,tmpbuf,choice,oldchoice,i);
            else if (i==ACID) addtobuf(ACIDfiles,tmpbuf,choice,oldchoice,i);
            else if (i==MGS) addtobuf(MGSfiles,tmpbuf,choice,oldchoice,i);
            else if (i==SCKICKS) addtobuf(SCKICKSfiles,tmpbuf,choice,oldchoice,i);
            else if (i==OPERVISION) addtobuf(OPERVISIONfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==CELE) addtobuf(CELEfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==HYPERDCC) addtobuf(HYPERDCCfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==VILAS) addtobuf(VILASfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==JIMMIE) addtobuf(JIMMIEfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==CTCPPAGE) addtobuf(CTCPPAGEfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==TDF) addtobuf(TDFfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==COUNTRY) addtobuf(COUNTRYfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==SZ32) addtobuf(SZ32files,tmpbuf,choice,oldchoice,i);
	    else if (i==SZNCURSES) addtobuf(SZNCURSESfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==IPCHECKING) addtobuf(IPCHECKINGfiles,tmpbuf,choice,oldchoice,i);
        }
        if (rename(defsfile,defsoldfile)<0) {
            printf("Error, couldn't rename %s to %s\n",defsfile,defsoldfile);
            return;
        }
        if ((fpin=fopen(defsoldfile,"r"))==NULL) {
            printf("Error, couldn't open %s for reading\n",defsoldfile);
            return;
        }
        if ((fpout=fopen(defsfile,"w"))==NULL) {
            printf("Error, couldn't open %s for writing\n",defsfile);
            fclose(fpin);
            return;
        }
        while (fgets(buf,1024,fpin)) {
            if (buf[strlen(buf)-1]=='\n') buf[strlen(buf)-1]='\0';
            if (!strcmp(buf,"/* Define this if you want IP checking */"))
                break;
            fprintf(fpout,"%s\n",buf);
        }
        fprintf(fpout,"/* Define this if you want IP checking */\n");
        if (choice&IPCHECKING) fprintf(fpout,"#define IPCHECKING\n");
        else fprintf(fpout,"#undef IPCHECKING\n");
        fprintf(fpout,"\n/* Define this if you want client with ANSI (color) support */\n");
        if (choice&WANTANSI) fprintf(fpout,"#define WANTANSI\n");
        else fprintf(fpout,"#undef WANTANSI\n");
        fprintf(fpout,"\n/* Define this if you want OperVision in the client */\n");
        if (choice&OPERVISION) fprintf(fpout,"#define OPERVISION\n");
        else fprintf(fpout,"#undef OPERVISION\n");
        fprintf(fpout,"\n/* Define this if you want following optional stuff:\n");
        fprintf(fpout," - /AUTOINV   - Invites on Notify    - /LOGO\n");
        fprintf(fpout," - /FIND      - /SVER                - /BKT\n");
        fprintf(fpout," - /DIRLMK    - /DIRLNK              - /BKI\n");
        fprintf(fpout," - /MODELOCK  - /MODEUNLOCK          - Mode Lock Checking\n");
        fprintf(fpout," - /LLOOKUP   - /LLOOK               - /RANLK\n");
        fprintf(fpout," - /MN        - /ML                  - /TERMINATE\n");
        fprintf(fpout," - /MSAY      - /DOBANS */\n");
        if (choice&EXTRAS) fprintf(fpout,"#define EXTRAS\n");
        else fprintf(fpout,"#undef EXTRAS\n");
        fprintf(fpout,"\n/* Define this if you want more accurate timer, NOT WORKING OK YET !!! */\n");
        if (choice&BETTERTIMER) fprintf(fpout,"#define BETTERTIMER\n");
        else fprintf(fpout,"#undef BETTERTIMER\n");
        fprintf(fpout,"\n/* Defines this if you want GenX's nifty /WHOIS */\n");
        if (choice&GENX) fprintf(fpout,"#define GENX\n");
        else fprintf(fpout,"#undef GENX\n");
        fprintf(fpout,"\n/* Define this if you want formatted /CSCAN */\n");
        if (choice&NEWCSCAN) fprintf(fpout,"#define NEWCSCAN\n");
        else fprintf(fpout,"#undef NEWCSCAN\n");
        fprintf(fpout,"\n/* Define this if you feel users should be invited to non +i channels on\n");
        fprintf(fpout,"   notify signon */\n");
        if (choice&ACID) fprintf(fpout,"#define ACID\n");
        else fprintf(fpout,"#undef ACID\n");
        fprintf(fpout,"\n/* Define this if you want sorted nicks in /CSCAN and old style of MSGs */\n");
        if (choice&MGS) fprintf(fpout,"#define MGS\n");
        else fprintf(fpout,"#undef MGS\n");
        fprintf(fpout,"\n/* Define this if you want scatter kicks */\n");
        if (choice&SCKICKS) fprintf(fpout,"#define SCKICKS\n");
        else fprintf(fpout,"#undef SCKICKS\n");
	fprintf(fpout,"\n/* Define this if you want to compile with Celerity C-Script */\n");
	if (choice&CELE) fprintf(fpout,"#define CELE\n");
	else fprintf(fpout,"#undef CELE\n");
	fprintf(fpout,"\n/* Define this if you want HyperDCC by Annatar in the client */\n");
	if (choice&HYPERDCC) fprintf(fpout,"#define HYPERDCC\n");
	else fprintf(fpout,"#undef HYPERDCC\n");
	fprintf(fpout,"\n/* Define this if you don't want ScrollZ trademarks in KICKs and away messages*/\n");
	if (choice&VILAS) fprintf(fpout,"#define VILAS\n");
	else fprintf(fpout,"#undef VILAS\n");
	fprintf(fpout,"\n/* Define this if you want better /NEWHOST */\n");
	if (choice&JIMMIE) fprintf(fpout,"#define JIMMIE\n");
	else fprintf(fpout,"#undef JIMMIE\n");
	fprintf(fpout,"\n/* Define this if you want CTCP PAGE by BiGhEaD */\n");
	if (choice&CTCPPAGE) fprintf(fpout,"#define CTCPPAGE\n");
	else fprintf(fpout,"#undef CTCPPAGE\n");
	fprintf(fpout,"\n/* Define this if you want different messages, chat messages and CDCC */\n");
	if (choice&TDF) fprintf(fpout,"#define TDF\n");
	else fprintf(fpout,"#undef TDF\n");
        fprintf(fpout,"\n/* Define this if you want $country() in the client */\n");
	if (choice&COUNTRY) fprintf(fpout,"#define COUNTRY\n");
	else fprintf(fpout,"#undef COUNTRY\n");
        fprintf(fpout,"\n/* Define this if you want client with Win32 support */\n");
	if (choice&SZ32) fprintf(fpout,"#define SZ32\n");
	else fprintf(fpout,"#undef SZ32\n");
        fprintf(fpout,"\n/* Define this if you want client with ncurses support */\n");
	if (choice&SZNCURSES) fprintf(fpout,"#define SZNCURSES\n");
	else fprintf(fpout,"#undef SZNCURSES\n");
        fprintf(fpout,"/****************************************************************************/\n");
        fclose(fpin);
        fclose(fpout);
        if (strcmp(tmpbuf,"rm")) {
            strcat(tmpbuf," >/dev/null 2>&1");
            system(tmpbuf);
        }
        sprintf(filebuf,"%sSZdist.log",pathbuf);
        if ((fpout=fopen(filebuf,"a"))) {
            char flagbuf[32];
            char *flags=flagbuf;

            timenow=time((time_t *) 0);
            if (choice&WANTANSI) *flags++='A';
            else *flags++='a';
            if (choice&EXTRAS) *flags++='E';
            else *flags++='e';
            if (choice&NEWCSCAN) *flags++='C';
            else *flags++='c';
            if (choice&BETTERTIMER) *flags++='T';
            else *flags++='t';
            if (choice&SCKICKS) *flags++='S';
            else *flags++='s';
            if (choice&HYPERDCC) *flags++='D';
            else *flags++='d';
            if (choice&CTCPPAGE) *flags++='P';
            else *flags++='p';
            if (choice&COUNTRY) *flags++='Y';
            else *flags++='y';
            if (choice&OPERVISION) {
                *flags++=' ';
                *flags++='O';
                *flags++='V';
            }
            if (choice&CELE) {
                *flags++=' ';
                *flags++='c';
                *flags++='y';
            }
            *flags++=' ';
            if (choice&GENX) *flags++='G';
            else *flags++='g';
            if (choice&ACID) *flags++='I';
            else *flags++='i';
            if (choice&VILAS) *flags++='V';
            else *flags++='v';
            if (choice&JIMMIE) *flags++='J';
            else *flags++='j';
            if (choice&TDF) *flags++='X';
            else *flags++='x';
            *flags='\0';
            if (choice&IPCHECKING)
                fprintf(fpout,"[%.24s] Registered to %s : #%s [%s]\n",ctime(&timenow),
                        regname,ip,flagbuf);
            else
                fprintf(fpout,"[%.24s] Public binary : [%s]\n",ctime(&timenow),flagbuf);
            fclose(fpout);
        }
        touchfile(defsfile,&statbuf);
    }
}
