/*
 * Replacement for ircII's flex crap by Flier
 */

#include <stdio.h>
#include <string.h>

int main() {
    int  count=0;
    char buffer[128]; /* should be more than enough */
    char *tmpstr;
    FILE *fp;

    while ((fgets(buffer,128,stdin))) {
        /* valid lines start with #define */
        if (!strncmp(buffer,"#define",7) && (tmpstr=strstr(buffer,"$\n")) &&
            *(tmpstr+2)=='\0') {
            *tmpstr='\0';
            fprintf(stdout,"%s%d\n",buffer,count);
            count++;
        }
	else fprintf(stdout,"%s",buffer);
    }
    return(0);
}
