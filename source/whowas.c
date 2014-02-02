/*
 * whowas.c   a linked list buffer of people who have left your channel 
 * mainly used for ban prot and stats stuff.
 * Should even speed stuff up a bit too.
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 1995
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 *
 * Modified by Flier
 *
 * $Id: whowas.c,v 1.9 2002-01-23 18:48:10 f Exp $
 */

#include "irc.h"
#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "names.h"
#include "alias.h"
#include "output.h"
#include "numbers.h"
#include "status.h"
#include "screen.h"
#include "edit.h"
#include "config.h"
#include "list.h"

#include "whowas.h"
#include "myvars.h"
#include "trace.h"

#define whowas_userlist_max 150
#define whowas_reg_max 300
#define whowas_chan_max 20

extern int  CheckChannel _((char *, char *));

WhowasList        *whowas_userlist_list=(WhowasList *) 0;
WhowasList        *whowas_reg_list=(WhowasList *) 0;
WhowasChanList    *whowas_chan_list=(WhowasChanList *) 0;

static int whowas_userlist_count=0;
static int whowas_reg_count=0;
static int whowas_chan_count=0;

extern WhowasList *check_whowas_buffer(nick,userhost,channel,unlink)
char *nick;
char *userhost;
char *channel;
int  unlink;
{
    WhowasList *tmp;
    WhowasList *last=(WhowasList *) 0;

    Trace(SZ_TRACE_WHOWAS, "searching for %s!%s in %s (unlink = %d)",
          nick, userhost, channel, unlink);
    for (tmp=whowas_userlist_list;tmp;tmp=tmp->next) {
        if (!my_stricmp(tmp->nicklist->userhost,userhost) &&
            !my_stricmp(tmp->channel,channel)) {
            if (unlink) {
                if (last) last->next=tmp->next;
                else whowas_userlist_list=tmp->next;
                whowas_userlist_count--;
            }
            Trace(SZ_TRACE_WHOWAS, "entry found in whowas userlist list %d",
                  whowas_userlist_count);
            return(tmp);
        }
        last=tmp;
    }
    last=(WhowasList *) 0;
    for (tmp=whowas_reg_list;tmp;tmp=tmp->next) {
        if (!my_stricmp(tmp->nicklist->userhost,userhost) &&
            !my_stricmp(tmp->channel,channel)) {
            if (unlink) {
                if (last) last->next=tmp->next;
                else whowas_reg_list=tmp->next;
                whowas_reg_count--;
            }
            Trace(SZ_TRACE_WHOWAS, "entry found in whowas reg list %d",
                  whowas_reg_count);
            return(tmp);
        }
        last=tmp;
    }
    Trace(SZ_TRACE_WHOWAS, "entry not found");
    return((WhowasList *) 0);
}

void add_to_whowas_buffer(nicklist,channel)
NickList *nicklist;
char *channel;
{
    WhowasList *new;
    WhowasList **slot;

    if (!nicklist || !(nicklist->userhost)) return;
    Trace(SZ_TRACE_WHOWAS, "add %s!%s from %s",
          nicklist->nick, nicklist->userhost, channel);
    if (nicklist->frlist) {
        if (whowas_userlist_count>=whowas_userlist_max) {
            whowas_userlist_count-=
                remove_oldest_whowas(&whowas_userlist_list,0,
                                     (whowas_userlist_max+1)-whowas_userlist_count);
        }
        new=(WhowasList *) new_malloc(sizeof(WhowasList));
        new->channel=(char *) 0;
        new->nicklist=nicklist;
        new->nicklist->next=(NickList *) 0;
        malloc_strcpy(&(new->channel),channel);
        new->time=time(NULL);
        /* we've created it, now put it in order */
        for (slot=&whowas_userlist_list;*slot;slot=&(*slot)->next)
            if ((*slot)->time>new->time) break;
        new->next=*slot;
        *slot=new;
        whowas_userlist_count++;
        Trace(SZ_TRACE_WHOWAS, "added to whowas userlist list %d",
              whowas_userlist_count);
    }
    else {
        if (whowas_reg_count>=whowas_reg_max) {
            whowas_reg_count-=
                remove_oldest_whowas(&whowas_reg_list,0,
                                     (whowas_reg_max+1)-whowas_reg_count);
        }
        new=(WhowasList *) new_malloc(sizeof(WhowasList));
        new->channel=(char *) 0;
        new->nicklist=nicklist;
        new->nicklist->next=(NickList *) 0;
        malloc_strcpy(&(new->channel),channel);
        new->time=time(NULL);
        /* we've created it, now put it in order */
        for (slot=&whowas_reg_list;*slot;slot=&(*slot)->next)
            if ((*slot)->time>new->time) break;
        new->next=*slot;
        *slot=new;
        whowas_reg_count++;
        Trace(SZ_TRACE_WHOWAS, "added to whowas reg list %d",
              whowas_reg_count);
    }
}

int remove_oldest_whowas(list,timet,count)
WhowasList **list;
time_t timet;
int count;
{
    WhowasList *tmp=(WhowasList *) 0;
    time_t t;
    int total=0;

    /* if no ..count.. then remove ..time.. links */
    Trace(SZ_TRACE_WHOWAS, "remove %d oldest entri(es) from whowas %s list",
          count, *list == whowas_userlist_list ? "userlist" : "reg");
    if (!count) {
        t=time(NULL);
        while (*list && ((*list)->time+timet)<=t) {
            tmp=*list;
            Trace(SZ_TRACE_WHOWAS, "remove %s!%s from %s",
                  tmp->nicklist->nick, tmp->nicklist->userhost, tmp->channel);
            new_free(&(tmp->nicklist->nick));
            new_free(&(tmp->nicklist->userhost));
            new_free(&(tmp->nicklist));
            new_free(&(tmp->channel));
            *list=tmp->next;
            new_free(&tmp);
            total++;
        }
    }
    else {
        while (*list && count) {
            tmp=*list;
            Trace(SZ_TRACE_WHOWAS, "remove %s!%s from %s",
                  tmp->nicklist->nick, tmp->nicklist->userhost, tmp->channel);
            new_free(&(tmp->nicklist->nick));
            new_free(&(tmp->nicklist->userhost));
            new_free(&(tmp->nicklist));
            new_free(&(tmp->channel));
            *list=tmp->next;
            new_free(&tmp);
            total++;
            count--;
        }
    }
    Trace(SZ_TRACE_WHOWAS, "removed %d entri(es)", total);
    return(total);
}

void clean_whowas_list() {
    whowas_userlist_count-=remove_oldest_whowas(&whowas_userlist_list,25*60,0);
    whowas_reg_count-=remove_oldest_whowas(&whowas_reg_list,15*60,0);
}

/* Used to rehash whowas listings for new users */
void synch_whowas_adduser(added)
struct friends *added;
{
    WhowasList *tmp;
    char user[mybufsize/2];

    Trace(SZ_TRACE_WHOWAS, "sync whowas with new userlist entry %s for %s",
          added->userhost, added->channels);
    for (tmp=whowas_userlist_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist) continue;
        if (CheckChannel(tmp->channel,added->channels)) {
            snprintf(user,sizeof(user),"%s!%s",tmp->nicklist->nick,tmp->nicklist->userhost);
            if (wild_match(added->userhost,user)) {
                tmp->nicklist->frlist=added;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s updated in whowas userlist list",
                      user);
            }
        }
    }
    for (tmp=whowas_reg_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist) continue;
        if (CheckChannel(tmp->channel,added->channels)) {
            snprintf(user,sizeof(user),"%s!%s",tmp->nicklist->nick,tmp->nicklist->userhost);
            if (wild_match(added->userhost,user)) {
                tmp->nicklist->frlist=added;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s updated in whowas reg list",
                      user);
            }
        }
    }
}

/* Used to rehash whowas listings for removed userlist entries */
void synch_whowas_unuser(entry)
struct friends *entry;
{
    WhowasList *tmp;

    Trace(SZ_TRACE_WHOWAS, "sync whowas with removed userlist entry %s for %s",
          entry->userhost, entry->channels);
    for (tmp=whowas_userlist_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist || !(tmp->nicklist->frlist)) continue;
        if (!my_stricmp(tmp->nicklist->frlist->userhost,entry->userhost) &&
            !my_stricmp(tmp->nicklist->frlist->channels,entry->channels)) {
                tmp->nicklist->frlist=(struct friends *) 0;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s!%s updated in whowas userlist list",
                      tmp->nicklist->nick, tmp->nicklist->userhost);
        }
    }
    for (tmp=whowas_reg_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist || !(tmp->nicklist->frlist)) continue;
        if (!my_stricmp(tmp->nicklist->frlist->userhost,entry->userhost) &&
            !my_stricmp(tmp->nicklist->frlist->channels,entry->channels)) {
                tmp->nicklist->frlist=(struct friends *) 0;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s!%s updated in whowas reg list",
                      tmp->nicklist->nick, tmp->nicklist->userhost);
        }
    }
}

/* Used to rehash whowas listings for new shitlist entries */
void synch_whowas_addshit(added)
struct autobankicks *added;
{
    WhowasList *tmp;
    char user[BIG_BUFFER_SIZE+1];

    Trace(SZ_TRACE_WHOWAS, "sync whowas with new shitlist entry %s for %s",
          added->userhost, added->channels);
    for (tmp=whowas_userlist_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist) continue;
        if (CheckChannel(tmp->channel,added->channels)) {
            snprintf(user,sizeof(user),"%s!%s",tmp->nicklist->nick,tmp->nicklist->userhost);
            if (wild_match(added->userhost,user)) {
                tmp->nicklist->shitlist=added;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s updated in whowas userlist list",
                      user);
            }
        }
    }
    for (tmp=whowas_reg_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist) continue;
        if (CheckChannel(tmp->channel,added->channels)) {
            snprintf(user,sizeof(user),"%s!%s",tmp->nicklist->nick,tmp->nicklist->userhost);
            if (wild_match(added->userhost,user)) {
                tmp->nicklist->shitlist=added;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s updated in whowas reg list",
                      user);
            }
        }
    }
}

/* Used to rehash whowas listings for removed shitlist entries */
void synch_whowas_unshit(entry)
struct autobankicks *entry;
{
    WhowasList *tmp;

    Trace(SZ_TRACE_WHOWAS, "sync whowas with removed shitlist entry %s for %s",
          entry->userhost, entry->channels);
    for (tmp=whowas_userlist_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist || !(tmp->nicklist->shitlist)) continue;
        if (!my_stricmp(tmp->nicklist->shitlist->userhost,entry->userhost) &&
            !my_stricmp(tmp->nicklist->shitlist->channels,entry->channels)) {
                tmp->nicklist->shitlist=(struct autobankicks *) 0;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s!%s updated in whowas userlist list",
                      tmp->nicklist->nick, tmp->nicklist->userhost);
            }
    }
    for (tmp=whowas_reg_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist || !(tmp->nicklist->shitlist)) continue;
        if (!my_stricmp(tmp->nicklist->shitlist->userhost,entry->userhost) &&
            !my_stricmp(tmp->nicklist->shitlist->channels,entry->channels)) {
                tmp->nicklist->shitlist=(struct autobankicks *) 0;
                Trace(SZ_TRACE_WHOWAS,
                      "entry for %s!%s updated in whowas reg list",
                      tmp->nicklist->nick, tmp->nicklist->userhost);
            }
    }
}


/* BELOW THIS MARK IS THE CHANNEL WHOWAS STUFF */

extern WhowasChanList *check_whowas_chan_buffer(channel,unlink)
char *channel;
int unlink;
{
    WhowasChanList *tmp;
    WhowasChanList *last=(WhowasChanList *) 0;

    Trace(SZ_TRACE_WHOWAS, "searching for channel %s (unlink=%d)",
          channel, unlink);
    for (tmp=whowas_chan_list;tmp;tmp=tmp->next) {
        if (!my_stricmp(tmp->channellist->channel,channel)) {
            if (unlink) {
                if (last) last->next=tmp->next;
                else whowas_chan_list=tmp->next;
                whowas_chan_count--;
            }
            Trace(SZ_TRACE_WHOWAS, "entry found %d",
                  whowas_chan_count);
            return(tmp);
        }
        last=tmp;
    }
    Trace(SZ_TRACE_WHOWAS, "entry not found");
    return((WhowasChanList *) 0);
}

int add_to_whowas_chan_buffer(channel)
ChannelList *channel;
{
    WhowasChanList *new;
    WhowasChanList **slot;

    Trace(SZ_TRACE_WHOWAS, "add channel %s window %p", channel->channel, channel->window);
    if (check_whowas_chan_buffer(channel->channel,0)) return(0);
    if (whowas_chan_count>=whowas_chan_max) {
        whowas_chan_count-=
            remove_oldest_chan_whowas(&whowas_chan_list,0,
                                      (whowas_chan_max+1)-whowas_chan_count);
    }
    new=(WhowasChanList *) new_malloc(sizeof(WhowasChanList));
    new->channellist=channel;
    new->time=time(NULL);
    /* we've created it, now put it in order */
    for (slot=&whowas_chan_list;*slot;slot=&(*slot)->next)
        if ((*slot)->time>new->time) break;
    new->next=*slot;
    *slot=new;
    whowas_chan_count++;
    Trace(SZ_TRACE_WHOWAS, "added %d", whowas_chan_count);
    return(1);
}

int remove_oldest_chan_whowas(list,timet,count)
WhowasChanList **list;
time_t timet;
int count;
{
    WhowasChanList *tmp=(WhowasChanList *) 0;
    time_t t;
    int total=0;

    /* if no ..count.. then remove ..time.. links */
    Trace(SZ_TRACE_WHOWAS, "remove %d oldest channel entri(es)", count);
    if (!count) {
        t = time(NULL);
        while (*list && ((*list)->time+timet)<=t) {
            tmp=*list;
            Trace(SZ_TRACE_WHOWAS, "remove %s", tmp->channellist->channel);
            new_free(&(tmp->channellist->channel));
            new_free(&(tmp->channellist->s_mode));
            new_free(&(tmp->channellist->key));
#ifdef EXTRAS
            new_free(&(tmp->channellist->modelock));
            new_free(&(tmp->channellist->topiclock));
#endif
            new_free(&(tmp->channellist->topicstr));
            new_free(&(tmp->channellist->topicwho));
            tmp->channellist->nicks=(NickList *) 0;
            tmp->channellist->banlist=(struct bans *) 0;
            new_free(&(tmp->channellist));
            *list=tmp->next;
            new_free(&tmp);
            total++;
        }
    }
    else {
        while (*list && count) {
            tmp=*list;
            Trace(SZ_TRACE_WHOWAS, "remove %s", tmp->channellist->channel);
            new_free(&(tmp->channellist->channel));
            new_free(&(tmp->channellist->s_mode));
            new_free(&(tmp->channellist->key));
#ifdef EXTRAS
            new_free(&(tmp->channellist->modelock));
            new_free(&(tmp->channellist->topiclock));
#endif
            new_free(&(tmp->channellist->topicstr));
            new_free(&(tmp->channellist->topicwho));
            tmp->channellist->nicks=(NickList *) 0;
            tmp->channellist->banlist=(struct bans *) 0;
            new_free(&(tmp->channellist));
            *list=tmp->next;
            new_free(&tmp);
            total++;
            count--;
        }
    }
    Trace(SZ_TRACE_WHOWAS, "removed %d entri(es)", total);
    return(total);
}

void clean_whowas_chan_list() {
    whowas_chan_count-=remove_oldest_chan_whowas(&whowas_chan_list,24*60*60,0);
}

/* Clean up memory used by whowas lists */
void CleanUpWhowas() {
    Trace(SZ_TRACE_WHOWAS, "clean up whowas");
    remove_oldest_whowas(&whowas_userlist_list,0,whowas_userlist_max);
    remove_oldest_whowas(&whowas_reg_list,0,whowas_reg_max);
    remove_oldest_chan_whowas(&whowas_chan_list,0,whowas_chan_max);
}

void swap_whowas_chan_win_ptr(Window *v_window, Window *window) {
    WhowasChanList *tmp;

    for (tmp = whowas_chan_list; tmp; tmp = tmp->next) {
        if (tmp->channellist->window == v_window) {
            tmp->channellist->window = window;
            Trace(SZ_TRACE_WHOWAS, "swap window pointer for channel %s: %p -> %p",
                  tmp->channellist->channel, v_window, window);
        }
        else if (tmp->channellist->window == window) {
            tmp->channellist->window = v_window;
            Trace(SZ_TRACE_WHOWAS, "swap window pointer for channel %s: %p -> %p",
                  tmp->channellist->channel, window, v_window);
        }
    }
}

/*
void show_whowas() {
    int count=1;
    WhowasList *tmp;

    say("Scanning whowas userlist, flag thinks there is %d entries",whowas_userlist_count);
    for (tmp=whowas_userlist_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist) say("%d: NO NICKLIST!!! BUG!",count);
        else say("%d: %s %s",count,tmp->nicklist->nick,tmp->nicklist->userhost);
        if (!tmp->channel) say("%d: NO CHANNEL!!! BUG!",count);
        else say("%d: %s",count,tmp->channel);
        count++;
    }
    count=1;
    say("Scanning whowas reg, flag thinks there is %d entries",whowas_reg_count);
    for (tmp=whowas_reg_list;tmp;tmp=tmp->next) {
        if (!tmp->nicklist) say("%d: NO NICKLIST!!! BUG!",count);
        else say("%d: %s %s",count,tmp->nicklist->nick,tmp->nicklist->userhost);
        if (!tmp->channel) say("%d: NO CHANNEL!!! BUG!",count);
        else say("%d: %s",count,tmp->channel);
        count++;
        }
}*/
