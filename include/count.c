/*
 * Replacement for ircII's flex crap by Flier
 */

#include <stdio.h>
#include <string.h>

int main() {
    int  count = 0;
    char buffer[512]; /* should be more than enough */
    char *tmpstr;
    FILE *fp;

    while ((fgets(buffer, 128, stdin))) {
        int len = strlen(buffer);

        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[len - 1] = '\0';
            len--;
        }
        /* valid lines start with #define */
        if (!strncmp(buffer, "#define", 7) && (tmpstr = strchr(buffer, '$')) &&
            *(tmpstr + 1) == '\0') {
            *tmpstr = '\0';
            fprintf(stdout, "%s%d\n", buffer, count);
            count++;
        }
	else fprintf(stdout, "%s\n", buffer);
    }
    return(0);
}
