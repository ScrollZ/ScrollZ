/*
 *
 * Routines dealing with files within IRC II script language
 *
 * Written by Flier
 *
 * $Id: files.c,v 1.14 2001-05-09 17:20:41 f Exp $
 */

#include "irc.h"
#include "ircaux.h"
#include "mystructs.h"

#ifndef LITE

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

#endif /* LITE */
