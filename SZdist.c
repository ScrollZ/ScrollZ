/*
 * This little piece of code should make life easier for customizing ScrollZ.
 *
 * Be sure to follow next steps, otherwise things won't work !!!
 * First of all, run configure. Next, compile this program and put it in
 * main ScrollZ dir (this *IS* important).
 * It needs include/defs.h and include/defs.h.in to operate plus working rm
 * (yes, the UN*X command).
 *
 * Once you qualify for requirements, you're ready to use it. Just run it and
 * you'll be present with current ScrollZ setup (read from include/defs.h).
 * Follow on-screen instructions and make your choices. Either use 'Q' to quit
 * or press 'S' to save. If you opted for latter, just type make irc and in a
 * matter of minutes you will have binary of your choice.
 *
 * Now that ScrollZ has a lot of compile-time options, we needed this so one
 * can simply feed this program output of /eval echo $J and get bin with
 * exactly the same options.
 *
 * Note that this should reduce compile time too since it only recompiles
 * files that are affected by selected options. If you need more info or you
 * have comments on this code, send e-mail to:
 * flier@scrollz.com
 * 
 * $Id: SZdist.c,v 1.31 2001-07-25 17:48:30 f Exp $
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
#include <sys/ioctl.h>

#define WANTANSI       (1<<0)
#define EXTRAS         (1<<1)
#define BETTERTIMER    (1<<2)
#define GENX           (1<<3)
#define NEWCSCAN       (1<<4)
#define ACID           (1<<5)
#define SORTEDNICKS    (1<<6)
#define SCKICKS        (1<<7)
#define OPERVISION     (1<<8)
#define CELE           (1<<9)
#define HYPERDCC       (1<<10)
#define VILAS          (1<<11)
#define JIMMIE         (1<<12)
#define CTCPPAGE       (1<<13)
#define TDF            (1<<14)
#define COUNTRY        (1<<15)
#define OPER           (1<<16)
#define OGRE           (1<<17)
#define SZ32           (1<<18)
#define LITE           (1<<19)
#define NUMDEFS        (LITE)

#define mybufsize 1024

char *int_ver="20010122";
char *ver="ircII 4.4Z+ScrollZ v1.8k (22.1.2000)+Cdcc v1.8";
char *defsfile="include/defs.h";
char *defsoldfile="include/defs.h.old";

char *WANTANSIfiles="alias.o cdcc.o dcc.o edit.o edit2.o edit3.o edit4.o edit5.o\
 edit6.o input.o log.o names.o numbers.o parse.o screen.o status.o term.o vars.o\
 whois.o";
char *EXTRASfiles="alias.o cdcc.o edit.o edit2.o edit3.o edit4.o edit5.o edit6.o\
 list.o names.o numbers.o parse.o whowas.o";
char *BETTERTIMERfiles="edit.o edit6.o";
char *GENXfiles="edit5.o whois.o";
char *NEWCSCANfiles="edit2.o";
char *ACIDfiles="edit.o edit2.o edit3.o edit5.o edit6.o screen.o vars.o window.o";
char *SORTEDNICKfiles="edit5.o names.o";
char *SCKICKSfiles="edit.o edit3.o edit4.o";
char *OPERVISIONfiles="edit.o edit3.o edit5.o funny.o notice.o operv.o parse.o\
 window.o";
char *CELEfiles="alias.o cdcc.o celerity.o edit.o edit2.o edit3.o edit4.o edit5.o\
 edit6.o input.o numbers.o operv.o parse.o server.o status.o vars.o whois.o";
char *HYPERDCCfiles="cdcc.o dcc.o edit.o edit4.o edit6.o";
char *VILASfiles="edit2.o edit6.o";
char *JIMMIEfiles="edit2.o";
char *CTCPPAGEfiles="ctcp.o";
char *TDFfiles="cdcc.o dcc.o edit.o edit4.o edit5.o edit6.o status.o";
char *COUNTRYfiles="alias.o";
char *SZ32files="*.o";
char *OPERfiles="edit.o edit2.o edit3.o edit5.o edit6.o numbers.o parse.o";
char *OGREfiles="operv.o";
char *LITEfiles="*.o";

char format[mybufsize];

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

int main(argc,argv)
int argc;
char **argv;
{
    int  i,choice=0,oldchoice=0;
    int  end=0;
    char c;
    char tmpbuf[2*mybufsize];
    char onoffbuf[32];
    char buf[mybufsize];
    char *tmp1;
    FILE *fpin=NULL,*fpout;
    struct stat statbuf;

    if ((fpin=fopen(defsfile,"r"))==NULL || stat(defsfile,&statbuf)!=0) {
        printf("Error, couldn't open %s for reading\n",defsfile);
        if (fpin) fclose(fpin);
        return(1);
    }
    while (fgets(buf,1024,fpin))
        if (strstr(buf,"#define")) {
            if (strstr(buf,"WANTANSI")) choice|=WANTANSI;
            else if (strstr(buf,"EXTRAS")) choice|=EXTRAS;
            else if (strstr(buf,"BETTERTIMER")) choice|=BETTERTIMER;
            else if (strstr(buf,"GENX")) choice|=GENX;
            else if (strstr(buf,"NEWCSCAN")) choice|=NEWCSCAN;
            else if (strstr(buf,"ACID")) choice|=ACID;
            else if (strstr(buf,"SORTEDNICKS")) choice|=SORTEDNICKS;
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
	    else if (strstr(buf,"OPER")) choice|=OPER;
	    else if (strstr(buf,"OGRE")) choice|=OGRE;
	    else if (strstr(buf,"LITE")) choice|=LITE;
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
            if (*tmp1=='L') choice|=LITE;
            else if (*tmp1=='l') choice&=~LITE;
            if (*tmp1=='N') choice|=SORTEDNICKS;
            else if (*tmp1=='n') choice&=~SORTEDNICKS;
        }
        if (*tmp1==' ' && *(tmp1+1)=='O' && *(tmp1+2)=='V') {
            choice|=OPERVISION;
            tmp1++;
            tmp1++;
            tmp1++;
        }
        else choice&=~OPERVISION;
        if (*tmp1==' ' && *(tmp1+1)=='c' && *(tmp1+2)=='y') {
            choice|=CELE;
            tmp1++;
            tmp1++;
            tmp1++;
        }
        else choice&=~CELE;
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
                if (*tmp1=='Z') choice|=OGRE;
                else if (*tmp1=='z') choice&=~OGRE;
            }
        }
        if (*tmp1==' ' && *(tmp1+1)=='O' && *(tmp1+2)=='P' && *(tmp1+3)=='E' &&
                          *(tmp1+4)=='R') {
            choice|=OPER;
            tmp1++;
            tmp1++;
            tmp1++;
        }
        else choice&=~OPER;
    }
    strcpy(tmpbuf,"rm source/irc.o source/ctcp.o");
    do {
        printf("[2J[1;1H");
        printf("Enter [1mletter[0m to toggle option on/off, '[1mQ[0m' to quit, '[1mS[0m' to save and quit\n");
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
        printf(" [1mG[0m - SORTEDNICKS   %s - sorted nicks in CSCAN\n",
               onoffstr(choice&SORTEDNICKS,onoffbuf));
        printf(" [1mH[0m - SCKICKS       %s - scatter (funny) kicks\n",
               onoffstr(choice&SCKICKS,onoffbuf));
        printf(" [1mI[0m - OPERVISION    %s - for IRC Operators, req. WANTANSI\n",
               onoffstr(choice&OPERVISION,onoffbuf));
	printf(" [1mJ[0m - CELE          %s - Compile with Celerity C-script\n",
	       onoffstr(choice&CELE,onoffbuf));
	printf(" [1mK[0m - HYPERDCC      %s - Compile with HyperDCC by Annatar\n",
	       onoffstr(choice&HYPERDCC,onoffbuf));
	printf(" [1mL[0m - VILAS         %s - No ScrollZ trademarks in ctcps\n",
	       onoffstr(choice&VILAS,onoffbuf));
	printf(" [1mM[0m - JIMMIE        %s - Better NEWHOST (lists all hostnames)\n",
	       onoffstr(choice&JIMMIE,onoffbuf));
	printf(" [1mN[0m - CTCP PAGE     %s - CTCP PAGE for friends by BiGhEaD\n",
	       onoffstr(choice&CTCPPAGE,onoffbuf));
	printf(" [1mO[0m - TDF           %s - different msgs, chat msgs and CDCC\n",
	       onoffstr(choice&TDF,onoffbuf));
	printf(" [1mP[0m - COUNTRY       %s - compile with $country()\n",
	       onoffstr(choice&COUNTRY,onoffbuf));
	printf(" [1mT[0m - LITE          %s - compile without some functionality\n",
	       onoffstr(choice&LITE,onoffbuf));
	printf(" [1m3[0m - SZ32          %s - compile for Win32 (NT+95)\n",
	       onoffstr(choice&SZ32,onoffbuf));
	printf(" [1mY[0m - OPER          %s - compile with IRC oper stuff\n",
	       onoffstr(choice&OPER,onoffbuf));
	printf(" [1mZ[0m - OGRE          %s - compile with ogre's OperVision cosmetics\n",
	       onoffstr(choice&OGRE,onoffbuf));
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
                case 'G': if ((choice&SORTEDNICKS)) choice&=~SORTEDNICKS;
                          else choice|=SORTEDNICKS;
                          break;
                case 'H': if ((choice&SCKICKS)) choice&=~SCKICKS;
                          else choice|=SCKICKS;
                          break;
                case 'I': if ((choice&OPERVISION)) choice&=~OPERVISION;
                          else choice|=OPERVISION;
                          break;
		case 'J': if ((choice&CELE)) choice&=~CELE;
			  else choice|=CELE;
			  break;
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
		case 'T': if ((choice&LITE)) choice&=~LITE;
			  else choice|=LITE;
			  break;
		case '3': if ((choice&SZ32)) choice&=~SZ32;
			  else choice|=SZ32;
			  break;
		case 'Y': if ((choice&OPER)) choice&=~OPER;
			  else choice|=OPER;
			  break;
		case 'Z': if ((choice&OGRE)) choice&=~OGRE;
			  else choice|=OGRE;
			  break;

            }
        }
        if (!(choice&WANTANSI)) choice&=~OPERVISION;
        if (!(choice&WANTANSI)) choice&=~GENX;
        if (!(choice&WANTANSI)) choice&=~TDF;
        if (!(choice&EXTRAS)) choice&=~ACID;
        if (!(choice&OPERVISION)) choice&=~OGRE;
    } while (!end);
    if (end==2) {
        for (i=1;i<=NUMDEFS;i*=2) {
            if (i==WANTANSI) addtobuf(WANTANSIfiles,tmpbuf,choice,oldchoice,i);
            else if (i==EXTRAS) addtobuf(EXTRASfiles,tmpbuf,choice,oldchoice,i);
            else if (i==BETTERTIMER) addtobuf(BETTERTIMERfiles,tmpbuf,choice,oldchoice,i);
            else if (i==GENX) addtobuf(GENXfiles,tmpbuf,choice,oldchoice,i);
            else if (i==NEWCSCAN) addtobuf(NEWCSCANfiles,tmpbuf,choice,oldchoice,i);
            else if (i==ACID) addtobuf(ACIDfiles,tmpbuf,choice,oldchoice,i);
            else if (i==SORTEDNICKS) addtobuf(SORTEDNICKSfiles,tmpbuf,choice,oldchoice,i);
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
	    else if (i==OPER) addtobuf(OPERfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==OGRE) addtobuf(OGREfiles,tmpbuf,choice,oldchoice,i);
	    else if (i==LITE) addtobuf(LITEfiles,tmpbuf,choice,oldchoice,i);
        }
        if (rename(defsfile,defsoldfile)<0) {
            printf("Error, couldn't rename %s to %s\n",defsfile,defsoldfile);
            return(1);
        }
        if ((fpin=fopen(defsoldfile,"r"))==NULL) {
            printf("Error, couldn't open %s for reading\n",defsoldfile);
            return(1);
        }
        if ((fpout=fopen(defsfile,"w"))==NULL) {
            printf("Error, couldn't open %s for writing\n",defsfile);
            fclose(fpin);
            return(1);
        }
        while (fgets(buf,1024,fpin)) {
            if (buf[strlen(buf)-1]=='\n') buf[strlen(buf)-1]='\0';
            if (!strcmp(buf,"/* Define this if you want client with ANSI (color) support */"))
                break;
            fprintf(fpout,"%s\n",buf);
        }
        fprintf(fpout,"\n/* Define this if you want client with ANSI (color) support */\n");
        if (choice&WANTANSI) fprintf(fpout,"#define WANTANSI\n");
        else fprintf(fpout,"#undef WANTANSI\n");
        fprintf(fpout,"\n/* Define this if you want OperVision support in the client */\n");
        if (choice&OPERVISION) fprintf(fpout,"#define OPERVISION\n");
        else fprintf(fpout,"#undef OPERVISION\n");
        fprintf(fpout,"\n/* Define this if you want following optional stuff:\n");
        fprintf(fpout," - /AUTOINV\n");
        fprintf(fpout," - /BKT       - /BKI         - /CHSIGNOFF\n");
        fprintf(fpout," - /DIRLMK    - /DIRLNK      - /DOBANS\n");
        fprintf(fpout," - /IDLEKICK  - /IDLETIME    - /LLOOK\n");
        fprintf(fpout," - /LLOOKUP   - /MASSDV      - /MASSV\n");
        fprintf(fpout," - /MODELOCK  - /MODEUNLOCK  - /MSAY\n");
        fprintf(fpout," - /RANLK     - /SHOWIDLE */\n");
        if (choice&EXTRAS) fprintf(fpout,"#define EXTRAS\n");
        else fprintf(fpout,"#undef EXTRAS\n");
        fprintf(fpout,"\n/* Define this if you want timer that's accurate to 0.01s */\n");
        if (choice&BETTERTIMER) fprintf(fpout,"#define BETTERTIMER\n");
        else fprintf(fpout,"#undef BETTERTIMER\n");
        fprintf(fpout,"\n/* Defines this if you want GenX's nifty /WHOIS */\n");
        if (choice&GENX) fprintf(fpout,"#define GENX\n");
        else fprintf(fpout,"#undef GENX\n");
        fprintf(fpout,"\n/* Define this if you want formatted /CSCAN */\n");
        if (choice&NEWCSCAN) fprintf(fpout,"#define NEWCSCAN\n");
        else fprintf(fpout,"#undef NEWCSCAN\n");
        fprintf(fpout,"\n/* Define this if you feel users should be invited to non +i channels on\n");
        fprintf(fpout,"   notify signon and +z userflag */\n");
        if (choice&ACID) fprintf(fpout,"#define ACID\n");
        else fprintf(fpout,"#undef ACID\n");
        fprintf(fpout,"\n/* Define this if you want sorted nicks in /CSCAN */\n");
        if (choice&SORTEDNICKS) fprintf(fpout,"#define SORTED_NICKS\n");
        else fprintf(fpout,"#undef SORTED_NICKS\n");
        fprintf(fpout,"\n/* Define this if you want scatter (funny) kicks support */\n");
        if (choice&SCKICKS) fprintf(fpout,"#define SCKICKS\n");
        else fprintf(fpout,"#undef SCKICKS\n");
	fprintf(fpout,"\n/* Define this if you want to compile with Celerity C-Script */\n");
	if (choice&CELE) fprintf(fpout,"#define CELE\n");
	else fprintf(fpout,"#undef CELE\n");
        fprintf(fpout,"#include \"celerity.h\"\n");
	fprintf(fpout,"\n/* Define this if you want HyperDCC by Annatar in the client */\n");
	if (choice&HYPERDCC) fprintf(fpout,"#define HYPERDCC\n");
	else fprintf(fpout,"#undef HYPERDCC\n");
	fprintf(fpout,"\n/* Define this if you don't want ScrollZ trademarks in CTCPs */\n");
	if (choice&VILAS) fprintf(fpout,"#define VILAS\n");
	else fprintf(fpout,"#undef VILAS\n");
	fprintf(fpout,"\n/* Define this if you want better /NEWHOST */\n");
	if (choice&JIMMIE) fprintf(fpout,"#define JIMMIE\n");
	else fprintf(fpout,"#undef JIMMIE\n");
	fprintf(fpout,"\n/* Define this if you want CTCP PAGE by bighead */\n");
	if (choice&CTCPPAGE) fprintf(fpout,"#define CTCPPAGE\n");
	else fprintf(fpout,"#undef CTCPPAGE\n");
	fprintf(fpout,"\n/* Define this if you want different look of messages and DCC chat messages */\n");
	if (choice&TDF) fprintf(fpout,"#define TDF\n");
	else fprintf(fpout,"#undef TDF\n");
        fprintf(fpout,"\n/* Define this if you want $country() in the client */\n");
	if (choice&COUNTRY) fprintf(fpout,"#define COUNTRY\n");
	else fprintf(fpout,"#undef COUNTRY\n");
        fprintf(fpout,"\n/* Define this if you want client with Win32 support */\n");
	if (choice&SZ32) fprintf(fpout,"#define SZ32\n");
	else fprintf(fpout,"#undef SZ32\n");
        fprintf(fpout,"\n/* Define this if you want irc oper stuff (not OperVision!) */\n");
	if (choice&OPER) fprintf(fpout,"#define OPER\n");
	else fprintf(fpout,"#undef OPER\n");
        fprintf(fpout,"\n/* Define this if you want ogre's cosmetics in OperVision */\n");
	if (choice&OGRE) fprintf(fpout,"#define OGRE\n");
	else fprintf(fpout,"#undef OGRE\n");
        fprintf(fpout,"\n/* Define this if you want client w/o certain functionality */\n");
	if (choice&LITE) fprintf(fpout,"#define LITE\n");
	else fprintf(fpout,"#undef LITE\n");
        fprintf(fpout,"/****************************************************************************/\n");
        fclose(fpin);
        fclose(fpout);
        if (strcmp(tmpbuf,"rm")) {
            strcat(tmpbuf," >/dev/null 2>&1");
            system(tmpbuf);
        }
        touchfile(defsfile,&statbuf);
    }
    return(0);
}
