#ifndef _cdcc_h_
#define _cdcc_h_

/*
 * Routines for Cdcc
 *
 * $Id: cdcc.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

/* This Mess, is the struct lists for cdcc.c */

/* My Holding List for sending Files */
typedef struct FilesStru
{
    struct FilesStru *next;               /* Pointer to next File */
    char   *path;                         /* Store the Path       */
    char   *file;                         /* Store Filename       */
    int    size;                          /* Store size of file   */
} Files;

/* Your Linked List of Offer Packs */
typedef struct PacksStru
{
    struct PacksStru *next;               /* Pointer to next Pack */
    char   *description;                  /* Description of Pack */
    int    totalfiles;                    /* Total Files in Pack */
    int    totalbytes;                    /* Total Bytes in Pack */
    int    gets;                          /* How many people have requested Pack */
    float  minspeed;                      /* Minimum DCC speed in kB/s */
    Files  *files;                        /* Pointer to List of Files in Pack */
} Packs;

/* Send Queue */
typedef struct QueueStru
{
    struct QueueStru *next;
    char   *file;
    char   *nick;
    int    flag;                          /* 1=send, 2=resend */
    int    server;
} FileQueue;

typedef struct CdccCommandStr
{
    char   *command;                      /* Cdcc command name */
    void   (*function)();                 /* Corresponding function */
} CdccCom;

#endif /* _cdcc_h_ */
