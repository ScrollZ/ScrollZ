/*
 *
 * Routines dealing with files within IRC II script language
 *
 * Written by Flier
 *
 * $Id: files.c,v 1.11 2000-08-21 18:41:40 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "mystructs.h"

char    *chars="ABCDEFGHIJ.*[]0123|abcdefghijrstuvwxyzKLMNOPQ!#$^?():'_-{}/=+klmnopq456789RSTUVWXYZ% ";
#ifdef IPCHECKING
char    global_track[256]={102,56,53,112,108,118,52,115,32,112,37,75,108,53,106,98,101,
        58,100,82,36,46,83,99,103,105,89,40,58,33,53,97,63,83,35,102,35,81,70,94,78,
        0,103,70,105,115,81,127,74,108,41,77,58,43,114,123,99,70,124,66,84,120,27,104,
        103,13,118,90,46,99,51,31,73,26,102,50,13,55,49,88,35,90,37,93,5,23,88,105,94,
        84,43,50,77,70,27,52,84,17,14,2,116,65,33,61,92,7,112,105,62,33,65,97,124,103,
        62,1,126,23,106,92,107,22,15,56,92,42,108,48,59,123,50,47,60,84,108,24,91,92,
        2,26,126,67,123,122,42,58,123,41,81,102,5,60,124,20,117,88,62,97,9,121,92,59,
        40,25,15,21,49,107,113,51,5,111,119,0,105,33,58,101,74,11,75,80,72,71,100,61,
        31,35,30,40,28,123,100,69,20,115,90,69,94,75,121,99,59,112,100,36,17,30,9,92,
        42,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0
        };
#endif

/*
   open(FILENAME <type>)
        <type> is R for read, W for write, A for append, R is default.
        Returns fd of opened file, -1 on error
   read (fd)
        Returns line for given fd, as long as fd is
        opened via the open() call, -1 on error
   write (fd text)
        Writes the text to the file pointed to by fd.
        Returns the number of bytes written, -1 on error
   close (fd)
        closes file for given fd
        Returns 0 on OK, -1 on error
   eof (fd)
        Returns 1 if fd is at EOF, 0 if not. -1 on error
*/

struct FILE___ {
	FILE *file;
	struct FILE___ *next;
};
typedef struct FILE___ File;

static File *FtopEntry=(File *) 0;

File *NewFile()
{
    File *tmp=FtopEntry;
    File *tmpfile=(File *) new_malloc(sizeof(File));

    if (FtopEntry==(File *) 0) FtopEntry=tmpfile;
    else {
        while (tmp->next) tmp=tmp->next;
        tmp->next=tmpfile;
    }
    tmpfile->file=NULL;
    return(tmpfile);
}

void RemoveFile(file)
File *file;
{
    File *tmp=FtopEntry;

    if (file==FtopEntry) FtopEntry=file->next;
    else {
        while (tmp->next && tmp->next!=file) tmp=tmp->next;
        if (tmp->next) tmp->next=tmp->next->next;
    }
    if (file->file) fclose(file->file);
    new_free((char **)&file);
}

int OpenFileRead(filename)
char *filename;
{
    char *expand=NULL;
    FILE *file;

    if (!(expand=expand_twiddle(filename))) malloc_strcpy(&expand,filename);
    file=fopen(expand,"r");
    new_free(&expand);
    if (file) {
        File *nfs=NewFile();
        nfs->file=file;
        nfs->next=(File *) 0;
        return(fileno(file));
    }
    else return(-1);
}

int OpenFileWrite(filename,type)
char *filename;
char *type;
{
    char *expand=NULL;
    FILE *file;

    if (!(expand=expand_twiddle(filename))) malloc_strcpy(&expand,filename);
    *type=tolower(*type);
    file=fopen(expand,type);
    new_free(&expand);
    if (file) {
        File *nfs=NewFile();
        nfs->file=file;
        nfs->next=(File *) 0;
        return(fileno(file));
    }
    else return(-1);
}

File *LookupFile(fd)
int fd;
{
    File *ptr=FtopEntry;

    while (ptr) {
        if (fileno(ptr->file)==fd) return(ptr);
        else ptr=ptr->next;
    }
    return((File *) 0);
}

int FileWrite(fd,stuff)
int  fd;
char *stuff;
{
    int  result;
    File *ptr=LookupFile(fd);

    if (!ptr) return(-1);
    else {
        result=fprintf(ptr->file,"%s\n",stuff);
        fflush(ptr->file);
        return(result);
    }
}

char *FileRead(fd)
int fd;
{
    File *ptr=LookupFile(fd);
    char *tmpstr=(char *) 0;
    char *result;
    char tmpbuf[mybufsize];

    if (!ptr) strcpy(tmpbuf,"-1");
    else {
        *tmpbuf='\0';
        result=fgets(tmpbuf,sizeof(tmpbuf),ptr->file);
        if (!result) strcpy(tmpbuf,"-1");
        else if (strlen(tmpbuf) && tmpbuf[strlen(tmpbuf)-1]=='\n')
            tmpbuf[strlen(tmpbuf)-1]='\0';
    } 
    malloc_strcpy(&tmpstr,tmpbuf);
    return(tmpstr);
}

int FileEof(fd)
int fd;
{
    File *ptr=LookupFile(fd);

    if (!ptr) return(-1);
    else return(feof(ptr->file));
}

int FileClose(fd)
int fd;
{
    File *ptr=LookupFile (fd);

    if (!ptr) return(-1);
    else RemoveFile(ptr);
    return(0);
}
