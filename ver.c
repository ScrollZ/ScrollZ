/*
 * $Id: ver.c,v 1.2 1998-09-10 17:34:49 f Exp $
 */

#include <stdio.h>

char *int_ver="19980912";
char *ver="ircII 4.4A+ScrollZ v1.8i2 (12.09.98)+Cdcc v1.7";
char *chars="ABCDEFGHIJRSTUVWXYZ.*[]0123|_-{}/=+klmnopqabcdefghijrstuvwxyz456789KLMNOPQ!#$^?():'% ";

char tmpbuf[128];
char *tmp1,*tmp2,*tmp3,*tmp4;
int  i,j,l;

void main() {
    l=strlen(chars);
    tmp1=ver;
    tmp2=tmpbuf;
    tmp4=int_ver;
    while (*tmp1) {
        for (tmp3=chars,i=0;*tmp3;tmp3++,i++) if (*tmp3==*tmp4) break;
        if (!(*tmp3)) {
            printf("Character %c (%d/%X) is illegal!\n",*tmp4,*tmp4,*tmp4);
            exit();
        }
        for (tmp3=chars,j=0;*tmp3;tmp3++,j++) if (*tmp3==*tmp1) break;
        if (!(*tmp3)) {
            printf("Character %c (%d/%X) is wrong!\n",*tmp1,*tmp1,*tmp1);
            exit();
        }
        *tmp2=chars[(i+j)%l];
        tmp1++;
        tmp2++;
        tmp4++;
        if (!(*tmp4)) tmp4=int_ver;
    }
    *tmp2='\0';
    printf("[%s]\n",tmpbuf);
}
