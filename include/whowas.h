/*
 * whowas.h: header file for whowas.c
 *
 * Written by Scott H Kilau
 *
 * CopyRight(c) 1995
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 *
 * $Id: whowas.h,v 1.2 1998-09-25 20:23:55 f Exp $
 *
 * Modified by Flier
 */

#ifndef _whowas_h_
#define _whowas_h_

#include "irc.h"
#include "window.h"
#include "names.h"

/* WhowasList: structure for the whowas buffer linked list */
typedef	struct	whowaslist_stru
{
    struct    whowaslist_stru *next;  /* pointer to next whowas entry */
    time_t    time;                   /* time added on list */
    char      *channel;               /* channel they left from */
    /*char    *server1;*/             /* server 1 of split */
    /*char    *server2;*/             /* server 2 of split */
    NickList *nicklist;               /* NickList entry */
} WhowasList;

/* WhowasChanList: structure for the whowas channel buffer linked list */
typedef	struct	whowaschanlist_stru
{
    struct whowaschanlist_stru *next;  /* pointer to next whowas chan entry */
    int refnum;                        /* to rejoin in correct window */
    time_t time;                       /* time added on list */
    ChannelList *channellist;          /* ChannelList entry */
} WhowasChanList;

extern WhowasList *whowas_userlist_list;
extern WhowasList *whowas_reg_list;
extern WhowasChanList *whowas_chan_list;

extern WhowasList *check_whowas_buffer _((char *, char *, char *, int));
extern void add_to_whowas_buffer _((NickList *, char */*, char *, char **/));
extern int remove_oldest_whowas _((WhowasList **, time_t, int));
extern void clean_whowas_list _((void));
extern void synch_whowas_adduser _((struct friends *));
extern void synch_whowas_unuser _((struct friends *));
extern void synch_whowas_addshit _((struct autobankicks *));
extern void synch_whowas_unshit _((struct autobankicks *));
extern WhowasChanList *check_whowas_chan_buffer _((char *, int)); 
extern int add_to_whowas_chan_buffer _((ChannelList *));
extern int remove_oldest_chan_whowas _((WhowasChanList **, time_t, int));
extern void clean_whowas_chan_list _((void));

#endif /* _whowas_h_ */
