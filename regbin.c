/*
 * $Id: regbin.c,v 1.3 1998-10-24 10:30:27 f Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define bufsize 65536

char *mask="V.XTjHwjCuRS7U";
char *int_ver="19980912";
char *ver="ircII 4.4A+ScrollZ v1.8i2 (12.09.98)+Cdcc v1.7";
char *chars="ABCDEFGHIJRSTUVWXYZ.*[]0123|_-{}/=+klmnopqabcdefghijrstuvwxyz456789KLMNOPQ!#$^?():'% ";

#define SearchNum 5
#define FoundNum 5
#define ValidNum 4
#define IdNum 4
#define DoneNum 4
#define ErrorNum 5
#define WrongNum 4

char *Search[]={
    "Search fer code in progress, wait till end of the century\n",
    "Looking up code, go out for a smoke\n",
    "Digging out code, go take a dump\n",
    "Locating code, wait another 1.345E02 seconds\n",
    "Searching for code, go get a beer\n"
};
char *Found[]={
    "Found at %d GigaBytes\n",
    "Located at %d ButtBytes\n",
    "Digged out at %d microBytes\n",
    "Found at -%d bits\n",
    "I think it's at %d bytez\n"
};
char *Valid[]={
    "Valid IP numbahs : %s (yeah right)\n",
    "This copy will roll on : %s\n",
    "This copy is gonna work on : %s\n",
    "Let's hope this won't crash on : %s\n"
};
char *Done[]={
    "Done, %s can now rock with ScrollZ%c\n",
    "Game over, %s is ready for ScrollZ%c\n",
    "Ready, %s will now be jammin' wif ScrollZ%c\n",
    "Finished, you can now send ScrollZ to %s%c\n"
};
char *Error[]={
    "Error writing phile %s, check space and ownership!\n",
    "Go check yer disk space and ownership, couldn't write to phile %s!\n",
    "Uhm, can't write phile %s, can I rm -rf / ?\n",
    "Gimme some disk space or change permissions, can't write to phile %s!\n"
    "Are you sure you own %s, I can't seem to write to it ? (Fewl)\n"
};
char *Wrong[]={
    "You gave muh wrong phile, butt-munch!\n",
    "Are you sure this was ScrollZ binary ? (Dumbass)\n",
    "You fucked up, this ain't ScrollZ binary\n",
    "No luck, gimme ScrollZ binary next time\n"
};

char buffer[bufsize];
char tmpbuf1[512],tmpbuf2[512],tmpbuf3[512],format[512],password[512];
char *tmp1,*tmp2,*tmp3,*tmp4,*tmpstr,*tmpip,*verstr,*verstr2;
int  i,j,k,l,found,pos,oldpos,gotit=0;
FILE *fp;

int myrand(high)
int high;
{
    int tmp;

    tmp=rand()%5;
    while (tmp--) rand();
    tmp=rand()/100;
    return(tmp%high);
}

void locatelog(pathbuf,fname)
char *pathbuf;
char *fname;
{
    char *curpath,*tmpp;
    char fpath[512],cpath[512];
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
    char pathbuf[512];
    FILE *fp1;

    strcpy(format,"KBdAAIFeHRY0COtvT5UM0|-PO=_^R$t");
    if (!(argc==2 || argc==3 || argc==4 || argc==5)) {
        printf("Usage : regbin filename regname valid_IP [password] or  regbin string [password]\n");
        return;
    }
    locatelog(pathbuf,"regbin");
    strcpy(tmpbuf2,ver);
    for (l=0,tmp2=tmpbuf2;l<2;tmp2++) if (*tmp2==' ') l++;
    for (l=0,tmp1=tmp2;l<2;tmp1++) if (*tmp1==' ') l++;
    tmp1--;
    *tmp1='\0';
    sprintf(tmpbuf3,"%s%s",int_ver,tmp2);
    verstr=tmpbuf3;
    if (argc==2 || argc==3) {
        printf("Encoding in progress....wait a century <G>\n");
        strcpy(tmpbuf1,argv[1]);
        l=strlen(chars);
        if (argc==2) strcpy(password,verstr);
        else strcpy(password,argv[2]);
        tmp2=password;
        if (argc==3) {
            k=-77;
            for (tmp1=tmp2;*tmp1;tmp1++) k+=*tmp1;
            k+=strlen(tmp2);
        }
        else {
            for (k=-33,tmp1=int_ver;*tmp1;tmp1++) k+=*tmp1;
            k+=strlen(int_ver);
        }
        tmp1=tmpbuf1;
        tmp3=tmpbuf2;
        while (*tmp1) {
            for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
            if (!(*tmp4)) {
                printf("Character %c (%d/%X) is wrong!\n",*tmp1,*tmp1,*tmp1);
                exit(1);
            }
            if (argc==3) j=k+l-i+(tmp1-tmpbuf1)-1;
            else j=k-2*l-i-(tmp1-tmpbuf1);
            while (j<0) j+=l;
            while (j>=l) j-=l;
            if (argc==2) {
                j+=27;
                if (j>=l) j-=l;
            }
            *tmp3=chars[j];
            tmp1++;
            tmp3++;
        }
        *tmp3='\0';
        tmp1=tmpbuf2;
        tmp3=tmpbuf1;
        while (*tmp1) {
            for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
            if (!(*tmp4)) {
                printf("Character %c (%d/%X) is illegal!\n",*tmp1,*tmp1,*tmp1);
                exit(1);
            }
            for (tmp4=chars,j=0;*tmp4;tmp4++,j++) if (*tmp4==*tmp2) break;
            if (!(*tmp4)) {
                printf("Uhm something is fucked up, tell Flier about it :P   Character %c (%d/%X) is illegal!\n",
                       *tmp2,*tmp2,*tmp2);
                exit(1);
            }
            *tmp3=chars[(i+j)%l];
            tmp1++;
            tmp2++;
            tmp3++;
            if (!(*tmp2)) tmp2=password;
        }
        *tmp3='\0';
        printf("Done, here is yer string: [%s]\n",tmpbuf1);
        return;
    }
    if (argc<5) {
        printf("Enter password:");
        fgets(password,512,stdin);
        if (strlen(password) && password[strlen(password)-1]=='\n')
            password[strlen(password)-1]='\0';
    }
    else strcpy(password,argv[4]);
    fp=fopen(argv[1],"r+b");
    if (fp==NULL) {
        printf("Can't open phile %s\n",argv[1]);
        return;
    }
    tmpstr=argv[2];
    if (index(tmpstr,'#')) {
        printf("You can't use character # in regname!\n");
        return;
    }
    tmpip=argv[3];
    if (index(tmpip,'#')) {
        printf("You can't use character # in IP!\n");
        return;
    }
    found=0;
    pos=0;
    l=strlen(mask);
    srand(time(NULL)+(rand()%200000)+l);
    printf("\n");
    printf(Search[myrand(SearchNum)]);
    while (!found && !feof(fp)) {
        oldpos=ftell(fp);
        i=fread(buffer,1,bufsize,fp);
        k=0;
        for (j=0;j<i;j++) if (mask[0]==buffer[j] && mask[1]==buffer[j+1]) {
            for (k=0;k<l;k++) if (mask[k]!=buffer[j+k]) break;
            if (k==l && mask[k-1]==buffer[j+k-1]) {
                found=1;
                break;
            }
        }
        if (!found) {
            if (i<bufsize) break;
            pos+=(i-1000);
            fseek(fp,pos,0);
        }
    }
    if (found) {
        printf(Found[myrand(FoundNum)],pos);
        srand(time(NULL)+(rand()%200000)+l);
        printf(Valid[myrand(ValidNum)],tmpip);
        fseek(fp,pos+j,0);
        l=strlen(chars);
        tmp2=password;
        k=-77;
        for (tmp1=tmp2;*tmp1;tmp1++) k+=*tmp1;
        k+=strlen(tmp2);
        tmp1=format;
        tmp3=tmpbuf1;
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
        tmp1=tmpbuf1;
        tmp3=format;
        while (*tmp1) {
            for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
            if (!(*tmp4)) {
                printf("Character %c (%d/%X) is illegal in format!\n",*tmp1,*tmp1,*tmp1);
                exit(1);
            }
            j=l-i+(tmp1-tmpbuf1)-1+k;
            while (j<0) j+=l;
            while (j>=l) j-=l;
            *tmp3=chars[j];
            tmp1++;
            tmp3++;
        }
        *tmp3='\0';
        tmp1=index(format,'%');
        if (tmp1 && *(tmp1-1)==' ' && *(tmp1+1)=='s' && *(tmp1+2)==' ' && *(tmp1+3)=='#' &&
            *(tmp1+4)=='%' && *(tmp1+5)=='s' && !(*(tmp1+6))) gotit=1;
        i=1;
        for (tmp1=format,tmp2=tmpbuf2;*tmp1;tmp1++)
            if (*tmp1=='%' && i) {
                for (tmp3=tmpstr;*tmp3;tmp3++)
                    *tmp2++=*tmp3;
                tmp1++;
                i=0;
            }
            else *tmp2++=*tmp1;
        *tmp2='\0';
        for (tmp1=tmpbuf2,tmp2=tmpbuf1;*tmp1;tmp1++)
            if (*tmp1=='%') {
                for (tmp3=tmpip;*tmp3;tmp3++)
                    *tmp2++=*tmp3;
                tmp1++;
            }
            else *tmp2++=*tmp1;
        *tmp2='\0';
        for (k=-33,tmp1=int_ver;*tmp1;tmp1++) k+=*tmp1;
        k+=strlen(int_ver);
        l=strlen(chars);
        tmp1=tmpbuf1;
        tmp3=tmpbuf2;
        while (*tmp1) {
            for (tmp4=chars,i=0;*tmp4;tmp4++,i++) if (*tmp4==*tmp1) break;
            if (!(*tmp4)) {
                printf("Character %c (%d/%X) is wrong!\n",*tmp1,*tmp1,*tmp1);
                exit(1);
            }
            j=k-2*l-i-(tmp1-tmpbuf1);
            while (j<0) j+=l;
            while (j>=l) j-=l;
            j+=27;
            if (j>=l) j-=l;
            *tmp3=chars[j];
            tmp1++;
            tmp3++;
        }
        *tmp3='\0';
        tmp1=tmpbuf2;
        tmp2=verstr;
        tmp3=tmpbuf1;
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
        srand(time(NULL)+(rand()%200000)+l);
        printf(Done[myrand(DoneNum)],tmpstr,gotit?'!':' ');
        i=fwrite(tmpbuf1,1,128,fp);
        if (i!=128) printf(Error[myrand(ErrorNum)],argv[1]);
        sprintf(tmpbuf1,"%sregbin.log",pathbuf);
        if ((fp1=fopen(tmpbuf1,"a"))) {
            time_t timenow=time((time_t *) 0);

            fprintf(fp1,"[%.24s] Registered to %s : #%s\n",ctime(&timenow),tmpstr,tmpip);
            fclose(fp1);
        }
    }
    else printf(Wrong[myrand(WrongNum)]);
    fclose(fp);
    printf("\n");
}
