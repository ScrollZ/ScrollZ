/*
 * names.c: This here is used to maintain a list of all the people currently
 * on your channel.  Seems to work 
 *
 * Written By Michael Sandrof
 *
 * Copyright (c) 1990 Michael Sandrof.
 * Copyright (c) 1991, 1992 Troy Rollo.
 * Copyright (c) 1992-1998 Matthew R. Green.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: names.c,v 1.31 2002-01-21 21:37:36 f Exp $
 */

#include "irc.h"

#include "ircaux.h"
#include "names.h"
#include "window.h"
#include "screen.h"
#include "server.h"
#include "lastlog.h"
#include "list.h"
#include "output.h"
#include "notify.h"
#include "vars.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"
#include "whowas.h"

#ifdef HAVETIMEOFDAY
#include <sys/time.h>
#include <unistd.h>
#endif

extern void ClearBans _((ChannelList *));
extern int  CheckChannel _((char *, char *));
extern struct friends *FindMatch _((char *, char *));
extern struct autobankicks *FindShit _((char *, char *));
extern NickList *CheckJoiners _((char *, char *, int , ChannelList *));
extern void AwaySave _((char *, int));
extern int  AddBan _((char *, char *, int, char *, int, time_t, ChannelList *));
extern int  RemoveBan _((char *, int, ChannelList *));
extern void CheckPermBans _((ChannelList *));
extern void HandleGotOps _((char *, ChannelList *));
#ifdef SORTED_NICKS
extern int  SortedCmp _((List *, List *));
#endif
extern int  HashFunc _((char *));
extern NickList *find_in_hash _((ChannelList *, char *));
extern Window *FindWindowByPtr _((Window *));
extern void UpdateChanLogFName _((ChannelList *));
extern void ChannelLogReport _((char *, ChannelList *));
extern void ChannelLogSave _((char *, ChannelList *));

extern NickList *tabnickcompl;
/****************************************************************************/

/* from names.h */
static	char	mode_str[] = MODE_STRING;

static	int	same_channel _((ChannelList *, char *));
static	void	free_channel _((ChannelList **));
static	void	show_channel _((ChannelList *));
/**************************** PATCHED by Flier ******************************/
/*static	void	clear_channel _((ChannelList *));
static	char	*recreate_mode _((ChannelList *));
static	int	decifer_mode _((char *, u_long *, int *, NickList **, char **));*/
void	clear_channel _((ChannelList *));
char	*recreate_mode _((ChannelList *));
static	int	decifer_mode _((char *, u_long *, int *, NickList **, char **, ChannelList *, char *, char *, char *, char *));
void    add_nick_to_hash _((ChannelList *, NickList *));
void    remove_nick_from_hash _((ChannelList *, NickList *));
/****************************************************************************/

/* clear_channel: erases all entries in a nick list for the given channel */
/**************************** PATCHED by Flier ******************************/
/*static	void*/
void
/****************************************************************************/
clear_channel(chan)
	ChannelList *chan;
{
	NickList *tmp,
		*next;

	for (tmp = chan->nicks; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->nick));
/**************************** PATCHED by Flier ******************************/
                new_free(&(tmp->userhost));
/****************************************************************************/
		new_free(&tmp);
	}
	chan->nicks = (NickList *) 0;
	chan->status &= ~CHAN_NAMES;
/**************************** PATCHED by Flier ******************************/
        ClearBans(chan);
/****************************************************************************/
}

/*
 * we need this to deal with !channels.
 */
static	int
same_channel(channel, chan2)
	ChannelList *channel;
	char	*chan2;
{
	size_t	len, len2;

	/* take the easy way out */
	if (*channel->channel != '!' && *chan2 != '!')
		return (!my_stricmp(channel->channel, chan2));

	/*
	 * OK, so what we have is channel = "!fooo" and chan2 = "!JUNKfoo".
	 */
	len = strlen(channel->channel);	
	len2 = strlen(chan2);	

	/* bail out on known stuff */
	if (len > len2)
		return (0);
	if (len == len2)
		return (!my_stricmp(channel->channel, chan2));

	/*
	 * replace the channel name if we are the same!
	 */
	if (!my_stricmp(channel->channel + 1, chan2 + 1 + (len2 - len)))
	{
		malloc_strcpy(&channel->channel, chan2);
		return (1);
	}
	return (0);
}

extern	ChannelList *
lookup_channel(channel, server, do_unlink)
	char	*channel;
	int	server;
	int	do_unlink;
{
 	ChannelList	*chan, *last = (ChannelList *) 0;

  	if (server == -1)
 		if ((server = primary_server) == -1)
 			return (ChannelList *) 0;
 	chan = server_list[server].chan_list;
	while (chan)
	{
		if (chan->server == server && same_channel(chan, channel))
		{
			if (do_unlink == CHAN_UNLINK)
			{
				if (last)
					last->next = chan->next;
				else
					server_list[server].chan_list = chan->next;
			}
			break;
		}
		last = chan;
		chan = chan->next;
	}
	return chan;
}

/*
 * add_channel: adds the named channel to the channel list.  If the channel
 * is already in the list, its attributes are modified accordingly with the
 * connected and copy parameters.
 */
void
add_channel(channel, server, connected, copy)
	char	*channel;
	int	server;
	int	connected;
	ChannelList *copy;
{
	ChannelList *new;
	int	do_add = 0;
/**************************** PATCHED by Flier ******************************/
        int     resetchan=0;
        WhowasChanList *whowaschan;
/****************************************************************************/

	/*
	 * avoid adding channel "0"
	 */
	if (channel[0] == '0' && channel[1] == '\0')
		return;

/**************************** PATCHED by Flier ******************************/
        if ((whowaschan=check_whowas_chan_buffer(channel,1))) {
            if ((new=lookup_channel(channel,server,CHAN_UNLINK))) {
                new_free(&(new->channel));
                new_free(&(new->s_mode));
                new_free(&(new->key));
                new_free(&(new->topicstr));
                new_free(&(new->topicwho));
                new_free(&(new->modelock));
                new_free(&(new->topiclock));
                ClearBans(new);
                new_free(&new);
            }
            new=whowaschan->channellist;
            new->next=(ChannelList *) 0;
            if ((new->window=is_bound(channel,server))==(Window *) 0)
                new->window=curr_scr_win;
            do_add=1;
            add_to_list((List **) &server_list[server].chan_list, (List *) new);
            new_free(&whowaschan);
        }
        else
/****************************************************************************/
	if ((new = lookup_channel(channel, server, CHAN_NOUNLINK)) == (ChannelList *) 0)
	{
		new = (ChannelList *) new_malloc(sizeof(ChannelList));
		new->channel = (char *) 0;
 		new->status = 0;
		new->key = (char *) 0;
		new->nicks = (NickList *) 0;
		new->s_mode = empty_string;
		malloc_strcpy(&new->channel, channel);
		new->mode = 0;
		new->limit = 0;
		if ((new->window = is_bound(channel, server)) == (Window *) 0)
			new->window = curr_scr_win;
		do_add = 1;
		add_to_list((List **) &server_list[server].chan_list, (List *) new);
/**************************** PATCHED by Flier ******************************/
                new->creationtime=time((time_t *) 0);
                new->modelock=(char *) 0;
                new->topiclock=(char *) 0;
                new->chanlogfpath=(char *) 0;
                resetchan=1;
/****************************************************************************/
	}
        else
        {
/**************************** Patched by Flier ******************************/
                if (!(new->nicks)) resetchan=1;
/****************************************************************************/
		if (new->connected != CHAN_LIMBO && new->connected != CHAN_JOINING)
			yell("--- add_channel: add_channel found channel not CHAN_LIMBO/JOINING: %s", new->channel);
        }
        if (do_add || (connected == CHAN_JOINED))
	{
		new->server = server;
/**************************** PATCHED by Flier ******************************/
                new->status=0;
                new->gotbans=0;
                new->gotwho=0;
		new->mode=0;
		new->limit=0;
		new_free(&new->key);
                new_free(&new->s_mode);
		new->s_mode=empty_string;
#ifdef HAVETIMEOFDAY
                gettimeofday(&(new->time),NULL);
#else
                new->time=time((time_t *) 0);
#endif
                if (resetchan) {
                    int i;

                    /* yay for ircII channel join handling, why is this called twice ? */
                    if (!connected) {
                        new->modelock=(char *) 0;
                        new->topiclock=(char *) 0;
                    }
                    new->pluso=0;     new->minuso=0;     new->plusb=0;
                    new->minusb=0;    new->topic=0;      new->kick=0;
                    new->pub=0;       new->servpluso=0;  new->servminuso=0;
                    new->servplusb=0; new->servminusb=0;
		    new->servplush=0; new->servminush=0;
		    new->plush=0;     new->minush=0;
                    new->AutoRejoin=AutoRejoin?CheckChannel(channel,AutoRejoinChannels):0;
                    new->MDopWatch=MDopWatch?CheckChannel(channel,MDopWatchChannels):0;
                    new->ShowFakes=ShowFakes?CheckChannel(channel,ShowFakesChannels):0;
                    new->KickOnFlood=KickOnFlood?CheckChannel(channel,KickOnFloodChannels):0;
                    new->KickWatch=KickWatch?CheckChannel(channel,KickWatchChannels):0;
                    new->NHProt=NHProt?CheckChannel(channel,NHProtChannels):0;
                    new->NickWatch=NickWatch?CheckChannel(channel,NickWatchChannels):0;
                    new->ShowAway=ShowAway?CheckChannel(channel,ShowAwayChannels):0;
                    new->KickOps=KickOps?CheckChannel(channel,KickOpsChannels):0;
                    new->KickOnBan=KickOnBan?CheckChannel(channel,KickOnBanChannels):0;
                    new->Bitch=Bitch?CheckChannel(channel,BitchChannels):0;
                    new->FriendList=FriendList?CheckChannel(channel,FriendListChannels):0;
#ifdef EXTRAS
                    new->IdleKick=IdleKick?CheckChannel(channel,IdleKickChannels):0;
                    if (new->IdleKick) new->IdleKick=IdleKick;
#endif
                    new->CompressModes=CompressModes?CheckChannel(channel,CompressModesChannels):0;
                    new->BKList=BKList?CheckChannel(channel,BKChannels):0;
                    new->ChanLog=ChanLog?CheckChannel(channel,ChanLogChannels):0;
                    if (new->ChanLog) UpdateChanLogFName(new);
                    new->TryRejoin=0;
                    new->banlist=NULL;
                    new->topicstr=NULL;
                    new->topicwho=NULL;
                    for (i=0;i<HASHTABLESIZE;i++) new->nickshash[i]=(struct hashstr *) 0;
                }
/****************************************************************************/
		clear_channel(new);
	}
	if (copy)
	{
		new->mode = copy->mode;
		new->limit = copy->limit;
		new->window = copy->window;
		malloc_strcpy(&new->key, copy->key);
	}
	new->connected = connected;
	if ((connected == CHAN_JOINED) && !is_current_channel(channel, server, 0))
	{
		int	flag = 1;
		Window	*tmp, *expected,
			*possible = (Window *) 0;

		expected = new->window;

		while ((tmp = traverse_all_windows(&flag)))
		{
/**************************** PATCHED by Flier ******************************/
                        if (tmp->name && !strcmp(tmp->name,"OV")) continue;
/****************************************************************************/
			if (tmp->server == server)
			{
				if (tmp == expected)
				{	
					set_channel_by_refnum(tmp->refnum, channel);
					new->window = tmp;
					update_all_status();
					return;
				}
				else if (!possible)
					possible = tmp;
			}
		}
		if (possible)
		{
			set_channel_by_refnum(possible->refnum, channel);
			new->window = possible;
			update_all_status();
			return;
		}
		set_channel_by_refnum(0, channel);
		new->window = curr_scr_win;
	}
	update_all_windows();
}

/**************************** PATCHED by Flier ******************************/
/* adds given nick to hash table */
void add_nick_to_hash(chan,nick)
ChannelList *chan;
NickList *nick;
{
    int i=HashFunc(nick->nick);
    struct hashstr *hashnew;
    struct hashstr *tmphash;

    if (find_in_hash(chan,nick->nick)) return;
    if ((hashnew=(struct hashstr *) new_malloc(sizeof(struct hashstr)))) {
        hashnew->next=(struct hashstr *) 0;
        hashnew->nick=nick;
        for (tmphash=chan->nickshash[i];tmphash && tmphash->next;) tmphash=tmphash->next;
        if (tmphash) tmphash->next=hashnew;
        else chan->nickshash[i]=hashnew;
    }
}
/****************************************************************************/

/*
 * add_to_channel: adds the given nickname to the given channel.  If the
 * nickname is already on the channel, nothing happens.  If the channel is
 * not on the channel list, nothing happens (although perhaps the channel
 * should be addded to the list?  but this should never happen) 
 */
/**************************** PATCHED by Flier ******************************/
/*void
add_to_channel(channel, nick, server, oper, voice)
	char	*channel;
	char	*nick;
	int	server;
	int	oper;
	int	voice;*/
ChannelList *add_to_channel(channel, nick, server, oper, halfop, voice, userhost,tmpchan)
	char	*channel;
	char	*nick;
	int	server;
	int	oper;
	int	halfop;
        int	voice;
        char    *userhost;
        ChannelList *tmpchan;
/****************************************************************************/
{
	NickList *new;
	ChannelList *chan;
	int	ishalfop = halfop;
	int	ischop = oper;
	int	hasvoice = voice;
/**************************** PATCHED by Flier ******************************/
        char    tmpbuf[mybufsize/4];
        time_t  timenow;
	NickList *tmp;
        WhowasList *whowas;
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
	/*if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))*/
        if (tmpchan) chan=tmpchan;
        else chan=lookup_channel(channel,server,CHAN_NOUNLINK);
        if (chan)
/****************************************************************************/
	{
		if (*nick == '+')
		{
			hasvoice = 1;
			nick++;
		}
		if (*nick == '%')
		{
			nick++;
			if (!my_stricmp(nick, get_server_nickname(server)))
				chan->status |= CHAN_HALFOP;

			ishalfop = 1;
		}
		if (*nick == '@')
		{
			nick++;
			if (!my_stricmp(nick, get_server_nickname(server)) && !((chan->status & CHAN_NAMES) && (chan->status & CHAN_MODE)))
			{
				char	*mode =  recreate_mode(chan);

				if (*mode)
				{
					int	old_server = from_server;

					from_server = server;
					send_to_server("MODE %s %s", chan->channel, mode);
					from_server = old_server;
				}

				chan->status |= CHAN_CHOP;
			}
			ischop = 1;
/**************************** PATCHED by Flier ******************************/
                        if (hasvoice) chan->status|=CHAN_VOICE;
/****************************************************************************/
		}

/**************************** PATCHED by Flier ******************************/
		/*if ((new = (NickList *) remove_from_list((List **) &(chan->nicks), nick)))
		{
                        new_free(&(new->userhost));
                        remove_nick_from_hash(chan,new);
			new_free(&(new->nick));
			new_free(&new);
		}
		new = (NickList *) new_malloc(sizeof(NickList));
		new->nick = (char *) 0;
		new->chanop = ischop;
		new->halfop = ishalfop;
		new->hasvoice = hasvoice;
		malloc_strcpy(&(new->nick), nick);
		add_to_list((List **) &(chan->nicks), (List *) new);*/
		tmp=(NickList *) remove_from_list((List **) &(chan->nicks),nick);
                if (userhost && (whowas=check_whowas_buffer(nick,userhost,channel,1))) {
                    new=whowas->nicklist;
                    new_free(&whowas);
                    sprintf(tmpbuf,"%s!%s",nick,userhost);
                    if (!(new->frlist && wild_match(new->frlist->userhost,tmpbuf) &&
                          CheckChannel(new->frlist->channels,channel)))
                        new->frlist=(struct friends *) FindMatch(tmpbuf,channel);
                    if (!(new->shitlist && wild_match(new->shitlist->userhost,tmpbuf) &&
                          CheckChannel(new->shitlist->channels,channel)))
                        new->shitlist=(struct autobankicks *) FindShit(tmpbuf,channel);
                }
                else {
                    new=(NickList *) new_malloc(sizeof(NickList));
                    new->nick=(char *) 0;
                    new->userhost=(char *) 0;
                    new->frlist=(struct friends *) 0;
                    new->shitlist=(struct autobankicks *) 0;
                    new->pluso=tmp?tmp->pluso:0;
                    new->minuso=tmp?tmp->minuso:0;
                    new->plusb=tmp?tmp->plusb:0;
                    new->minusb=tmp?tmp->minusb:0;
                    new->kick=tmp?tmp->kick:0;
                    new->nickc=tmp?tmp->nickc:0;
                    new->publics=tmp?tmp->publics:0;
                    if (userhost) {
                        sprintf(tmpbuf,"%s!%s",nick,userhost);
                        malloc_strcpy(&(new->userhost),userhost);
                        new->frlist=(struct friends *) FindMatch(tmpbuf,channel);
                        new->shitlist=(struct autobankicks *) FindShit(tmpbuf,channel);
                    }
                }
                if (tmp) {
                    new_free(&(tmp->userhost));
                    remove_nick_from_hash(chan,tmp);
                    new_free(&(tmp->nick));
                    new_free(&tmp);
                }
                malloc_strcpy(&(new->nick), nick);
                new->chanop=ischop;
		new->halfop=ishalfop;
		new->hasvoice=hasvoice;
                new->curo=0;
                new->curk=0;
                new->curn=0;
                timenow=time((time_t *) 0);
                new->deopt=timenow;
                new->kickt=timenow;
                new->nickt=timenow;
                new->lastmsg=timenow;
                new->deopp=0;
                new->kickp=0;
                new->nickp=0;
#ifdef SORTED_NICKS
                add_to_list_ext((List **) &(chan->nicks), (List *) new, SortedCmp);
#else
                add_to_list((List **) &(chan->nicks), (List *) new);
#endif
                add_nick_to_hash(chan,new);
/****************************************************************************/
	}
	notify_mark(nick, 1, 0);
/**************************** PATCHED by Flier ******************************/
        return(chan);
/****************************************************************************/
}


/*
 * recreate_mode: converts the bitmap representation of a channels mode into
 * a string 
 */
/**************************** PATCHED by Flier ******************************/
/*static	char	**/
char	*
/****************************************************************************/
recreate_mode(chan)
	ChannelList *chan;
{
	int	mode_pos = 0,
		mode;
	static	char	*s;
	char	buffer[BIG_BUFFER_SIZE+1];

	buffer[0] = '\0';	 /* paranoia */
	s = buffer;
	mode = chan->mode;
	while (mode)
	{
		if (mode % 2)
			*s++ = mode_str[mode_pos];
		mode /= 2;
		mode_pos++;
	}
	if (chan->key && !get_int_var(HIDE_CHANNEL_KEYS_VAR))
	{
		*s++ = ' ';
		strcpy(s, chan->key);
		s += strlen(chan->key);
	}
	if (chan->limit)
		snprintf(s, sizeof(buffer) - (s - buffer), " %d", chan->limit);
	else
		*s = '\0';

	malloc_strcpy(&chan->s_mode, buffer);
	return chan->s_mode;
}

/*
 * decifer_mode: This will figure out the mode string as returned by mode
 * commands and convert that mode string into a one byte bit map of modes 
 */
/**************************** PATCHED by Flier ******************************/
/*static	int
decifer_mode(mode_string, mode, chop, nicks, key)
	char	*mode_string;
	u_long	*mode;
	char	*chop;
	NickList **nicks;
	char	**key;*/
static int
decifer_mode(mode_string,mode,chop,nicks,key,chan,from,userhost,nethacks,servmodes)
char	*mode_string;
u_long	*mode;
int	*chop;
NickList **nicks;
char	**key;
ChannelList *chan;
char    *from;
char    *userhost;
char    *nethacks;
char    *servmodes;
/****************************************************************************/
{
	char	*limit = 0;
/**************************** PATCHED by Flier ******************************/
        /*char	*person;*/
        char	*person = NULL;
/****************************************************************************/
	int	add = 0;
	int	limit_set = 0;
	int	limit_reset = 0;
	char	*rest,
		*the_key;
	NickList *ThisNick;
 	u_long	value = 0;
/**************************** PATCHED by Flier ******************************/
        char *mynick = get_server_nickname(parsing_server_index);
        int  check = from ? 1 : 0;
        int  gotops = 0;
        int  isitme = check ? !my_stricmp(from, mynick) : 0;
        int  isserver = check ? (strchr(from, '.') != NULL) : 0;
        int  max = get_int_var(MAX_MODES_VAR);
        int  deopped = 0;
        int  servadd = -1;
        int  compadd = -1;
        int  minusban = 0;
	int  hadops = !HAS_OPS(*chop);
        int  privs;
        int  isprot = 0;
        int  count = 0;
        int  whowasdone = 0;
        int  server = parsing_server_index;
        int  exception;
        char *arg;
        char tmpbuf[mybufsize / 2];
        char nhdeop[mybufsize / 4];
        char modebuf[mybufsize / 32];
        char servline[mybufsize / 2];
        char lastdeop[mybufsize / 4];
        char lastvoice[mybufsize / 4];
        char compmode[mybufsize / 32];
        char compline[mybufsize / 2];
        char tmpbufmode[mybufsize / 2];
        char tmporigmode[mybufsize / 2];
        char *origmode = mode_string;
        char *compmodeadd = compmode;
        char *servmodeadd = servmodes;
        time_t timenow = time(NULL);
        NickList *joiner = NULL;
        NickList *tmpjoiner = NULL;
        WhowasList *whowas;

        if (check) {
            if (isserver) userhost = NULL;
            *nethacks = '\0';
            *servmodeadd = '\0';
            *tmpbufmode = '\0';
            *servline = '\0';
            *lastdeop = '\0';
            *lastvoice = '\0';
            *nhdeop = '\0';
            *compline = '\0';
            *compmodeadd = '\0';
            tmpjoiner = CheckJoiners(from, chan->channel, server, chan);
            if (tmpjoiner) {
                if (tmpjoiner->curo == 0) tmpjoiner->deopp = 0;
                if (timenow-tmpjoiner->deopt >= MDopTimer) {
                    tmpjoiner->curo = 0;
                    tmpjoiner->deopp = 0;
                    tmpjoiner->deopt = timenow;
                }
                if (tmpjoiner->userhost && tmpjoiner->frlist)
                    isprot = (tmpjoiner->frlist->privs) & (FLPROT | FLGOD);
            }
            if (!(chan->CompressModes)) strcpy(tmporigmode, mode_string);
        }
/****************************************************************************/

	if (!(mode_string = next_arg(mode_string, &rest)))
		return -1;
	for (; *mode_string; mode_string++)
	{
/**************************** PATCHED by Flier ******************************/
                if (*mode_string != 'o' && *mode_string != '+' && *mode_string != '-') {
                    if (check && isserver) {
                        if (servadd != add) {
                            if (add) *servmodeadd ++= '+';
                            else *servmodeadd ++= '-';
                            servadd = add;
                        }
                        *servmodeadd ++= *mode_string;
                    }
                }
                if (check && chan->CompressModes) {
                    if (*mode_string == 'a' || *mode_string == 'i' || *mode_string == 'k' ||
                        *mode_string == 'l' || *mode_string == 'm' || *mode_string == 'n' ||
                        *mode_string == 'p' || *mode_string == 'q' || *mode_string == 's' ||
                        *mode_string == 't' || *mode_string == 'c' || *mode_string == 'R' ||
                        *mode_string == 'I') {
                        if (compadd != add) {
                            if (add) *compmodeadd ++= '+';
                            else *compmodeadd ++= '-';
                            compadd = add;
                        }
                        *compmodeadd ++= *mode_string;
                    }
                }
/****************************************************************************/
                switch (*mode_string)
		{
		case '+':
			add = 1;
			value = 0;
			break;
		case '-':
			add = 0;
			value = 0;
			break;
		case 'a':
			value = MODE_ANONYMOUS;
			break;
		case 'c':
			value = MODE_COLOURLESS;
			break;
		case 'i':
			value = MODE_INVITE;
			break;
		case 'k':
			value = MODE_KEY;
			the_key = next_arg(rest, &rest);
			if (add)
				malloc_strcpy(key, the_key);
			else
				new_free(key);
/**************************** PATCHED by Flier ******************************/
                        if (!the_key) the_key = empty_string;
                        if (check && isserver) {
                            strcat(servline, the_key);
                            strcat(servline, " ");
                        }
                        if (check && chan->CompressModes) {
                            strcat(compline, the_key);
                            strcat(compline, " ");
                        }
/****************************************************************************/
			break;
		case 'l':
			value = MODE_LIMIT;
			if (add)
			{
				limit_set = 1;
				if (!(limit = next_arg(rest, &rest)))
					limit = empty_string;
				else if (0 == strncmp(limit, zero, 1))
					limit_reset = 1, limit_set = 0, add = 0;
			}
			else
				limit_reset = 1;
/**************************** PATCHED by Flier ******************************/
                        if (check && isserver && add) {
                            strcat(servline, limit);
                            strcat(servline, " ");
                        }
                        if (check && chan->CompressModes) {
                            if (add) {
                                strcat(compline, limit);
                                strcat(compline, " ");
                            }
                        }
/****************************************************************************/
			break;
		case 'm':
			value = MODE_MODERATED;
			break;
/**************************** Patched by Flier ******************************/
                        /* half-op code by braneded */
		case 'h':
			if ((person = next_arg(rest, &rest)) && !my_stricmp(person, mynick)) {
				if (add) {
					/* we can only have one of ohv on a hybrid7 server.
				   	   +v, +h, -h != +v */
					if (get_server_version(server) == Server2_11)
						*chop &= ~CHAN_VOICE;

					*chop |= CHAN_HALFOP;
					if (check && hadops) gotops = 1;
				}
				else
					*chop &= ~CHAN_HALFOP;
			}
			ThisNick = find_in_hash(chan, person);
			if (!person) person = empty_string;
			if (check && chan->CompressModes && ThisNick) {
				if ((add && !(ThisNick->halfop)) ||
				    (!add && ThisNick->halfop)) {
				    if (compadd != add) {
					if (add) *compmodeadd ++= '+';
					else *compmodeadd ++= '-';
					compadd = add;
				    }
				    *compmodeadd ++= *mode_string;
				    strcat(compline, person);
				    strcat(compline, " ");
				}
			}
			if (isserver) {
			    if (!add) {
				if (servadd != add) {
				    if (add) *servmodeadd ++= '+';
				    else *servmodeadd ++= '-';
				    servadd = add;
                            	}
				*servmodeadd ++= *mode_string;
				strcat(servline, person);
				strcat(servline, " ");
			    }
                        }
			if (ThisNick) {
				ThisNick->halfop = add;
				if (add && get_server_version(server) == Server2_11)
					ThisNick->hasvoice = 0;
			}
			if (check && tmpjoiner) {
				if (add) tmpjoiner->plush++;
				else tmpjoiner->minush++;
			}
			if (isserver) {
				if (add) {
					chan->servplush++;
					strcat(servline, person);
					strcat(servline, " ");
				} else chan->servminush++;
			}
			if (add) chan->plush++;
			else chan->minush++;
			break;
/****************************************************************************/
		case 'o':
/**************************** PATCHED by Flier ******************************/
 			/*if ((person = next_arg(rest, &rest)) && !my_stricmp(person, get_server_nickname(from_server))) {
                                if (add) {
					*chop |= CHAN_CHOP;
				else
					*chop &= ~CHAN_CHOP;
 			}*/
                        if ((person = next_arg(rest, &rest)) && !my_stricmp(person, mynick)) {
                                if (add) {
					/* we can only have one of ohv on a hybrid7 server.
					   +h, +o, -o != +h */
					if (get_server_version(server) == Server2_11)
						*chop &= ~(CHAN_HALFOP | CHAN_VOICE);

					*chop |= CHAN_CHOP;
                                        if (check && hadops) gotops = 1;
                                }
				else
					*chop &= ~CHAN_CHOP;
 			}
/****************************************************************************/
/**************************** PATCHED by Flier ******************************/
                        /*ThisNick = (NickList *) list_lookup((List **) nicks, person, !USE_WILDCARDS, !REMOVE_FROM_LIST);*/
                        ThisNick = find_in_hash(chan, person);
                        if (!person) person = empty_string;
                        if (check) {
                            if (tmpjoiner) {
                                if (add) tmpjoiner->pluso++;
                                else {
                                    tmpjoiner->minuso++;
                                    tmpjoiner->curo++;
                                }
                            }
                            if (isserver) {
                                if (!add) {
                                    if (servadd != add) {
                                        if (add) *servmodeadd ++= '+';
                                        else *servmodeadd ++= '-';
                                        servadd = add;
                                    }
                                    *servmodeadd ++= *mode_string;
                                    strcat(servline, person);
                                    strcat(servline, " ");
                                }
                                else {
                                    if (chan->NHProt && chan->FriendList) {
                                        if (!(ThisNick && ThisNick->frlist && ThisNick->frlist->privs)) {
                                            strcat(nhdeop, " -o ");
                                            strcat(nhdeop, person);
                                        }
                                    }
                                    strcat(nethacks, person);
                                    strcat(nethacks, " ");
                                }
                                if (add) chan->servpluso++;
                                else chan->servminuso++;
                            }
                            if (check && chan->CompressModes && ThisNick) {
                                if ((add && !(ThisNick->chanop)) ||
                                    (!add && ThisNick->chanop)) {
                                    if (compadd != add) {
                                        if (add) *compmodeadd ++= '+';
                                        else *compmodeadd ++= '-';
                                        compadd = add;
                                    }
                                    *compmodeadd ++= *mode_string;
                                    strcat(compline, person);
                                    strcat(compline, " ");
                                }
                            }
                            if (add) {
                                chan->pluso++;
                                minusban = 1;
                                if (ThisNick && chan->BKList) {
                                    if ((ThisNick->shitlist && (ThisNick->shitlist->shit) & SLDEOP)
                                        || (chan->Bitch && !isitme && !(isprot & FLGOD) &&
                                            (!ThisNick->frlist || !((ThisNick->frlist->privs) & (FLOP | FLAUTOOP | FLINSTANT))) &&
                                            my_stricmp(mynick, ThisNick->nick))) {
                                        count++;
                                        strcat(modebuf, "-o");
                                        strcat(tmpbufmode, " ");
                                        strcat(tmpbufmode, ThisNick->nick);
                                    }
                                }
                            }
                            else {
                                chan->minuso++;
                                if (!isitme && ThisNick && ThisNick->userhost &&
                                    ThisNick->chanop && chan->FriendList) {
                                    if (ThisNick->frlist) privs = ThisNick->frlist->privs;
                                    else privs = 0;
                                    if ((privs & (FLPROT | FLGOD)) && (privs & FLOP)
                                        && my_stricmp(from, ThisNick->nick)
                                        && !CheckChannel(lastdeop, ThisNick->nick)
                                        && !(!(privs & FLGOD) && (isprot & FLGOD))) {
                                        if (away_set || LogOn || (chan && chan->ChanLog)) {
                                            sprintf(tmpbuf,"%s (%s) has been deopped on channel %s by %s",
                                                    ThisNick->nick, ThisNick->userhost,
                                                    chan->channel, from);
                                            if (!isserver) {
                                                strcat(tmpbuf, " (");
                                                strcat(tmpbuf, userhost);
                                                strcat(tmpbuf, ")");
                                            }
                                            if (away_set || LogOn) AwaySave(tmpbuf, SAVEPROT);
                                            if (chan && chan->ChanLog) ChannelLogSave(tmpbuf, chan);
                                        }
                                        if (*chop & CHAN_CHOP) {
                                            if (!isserver && (privs & FLGOD) && !(isprot & FLGOD)) {
                                                count++;
                                                strcat(modebuf, "-o");
                                                strcat(tmpbufmode, " ");
                                                strcat(tmpbufmode, from);
                                                deopped = 1;
                                            }
                                            if (count == max) {
                                                send_to_server("MODE %s %s %s",
                                                               chan->channel, modebuf,
                                                               tmpbufmode);
                                                *tmpbufmode = '\0';
                                                *modebuf = '\0';
                                                count = 0;
                                            }
                                            if ((privs & FLGOD) || ((privs & FLPROT) && !(isprot & FLGOD))) {
                                                count++;
                                                strcat(modebuf, "+o");
                                                strcat(tmpbufmode, " ");
                                                strcat(tmpbufmode, ThisNick->nick);
                                            }
                                        }
                                        if (*lastdeop) strcat(lastdeop, ",");
                                        strcat(lastdeop, ThisNick->nick);
                                    }
                                }
                            }
                        }
                        if (ThisNick && add && !(ThisNick->chanop)) {
                            ThisNick->curo = 0;
                            ThisNick->deopp = 0;
                            ThisNick->deopt = timenow;
                        }
/****************************************************************************/
			if (ThisNick)
			{
				ThisNick->chanop = add;

				if (add && get_server_version(server) == Server2_11)
					ThisNick->halfop = ThisNick->hasvoice = 0;
			}
			break;
		case 'n':
			value = MODE_MSGS;
			break;
		case 'p':
			value = MODE_PRIVATE;
			break;
		case 'q':
			value = MODE_QUIET;
			break;
 		case 'r':
 			value = MODE_REOP;
 			break;
		case 's':
			value = MODE_SECRET;
			break;
		case 't':
			value = MODE_TOPIC;
			break;
		case 'v':
/**************************** PATCHED by Flier ******************************/
			/*person = next_arg(rest, &rest);
			ThisNick = (NickList *) list_lookup((List **) nicks, person, !USE_WILDCARDS, !REMOVE_FROM_LIST);
			if (ThisNick)
				ThisNick->hasvoice = add;
			break;*/
			if ((person = next_arg(rest, &rest)) && !my_stricmp(person, mynick))
                        {
				if (add)
					*chop |= CHAN_VOICE;
				else
					*chop &= ~CHAN_VOICE;
                        }
                        ThisNick = find_in_hash(chan, person);
                        if (!person) person = empty_string;
                        if (check && isserver) {
                            strcat(servline, person);
                            strcat(servline, " ");
                        }
                        if (check && chan->CompressModes && ThisNick) {
                            if ((add && !(ThisNick->hasvoice)) ||
                                (!add && ThisNick->hasvoice)) {
                                if (compadd != add) {
                                    if (add) *compmodeadd ++= '+';
                                    else *compmodeadd ++= '-';
                                    compadd = add;
                                }
                                *compmodeadd ++= *mode_string;
                                strcat(compline, person);
                                strcat(compline, " ");
                            }
                        }
                        if (ThisNick) {
                            ThisNick->hasvoice = add;
                            if (ThisNick->frlist) privs = ThisNick->frlist->privs;
                            else privs = 0;
                            if (check && chan->FriendList && (privs & FLPROT) &&
                                (privs & FLVOICE) && HAS_OPS(*chop) && !isserver &&
                                !add && !isitme && my_stricmp(from, ThisNick->nick) &&
                                !CheckChannel(lastvoice, ThisNick->nick)) {
                                send_to_server("MODE %s +v %s", chan->channel,
                                               ThisNick->nick);
                                if (*lastdeop) strcat(lastvoice, ",");
                                strcat(lastvoice, ThisNick->nick);
                            }
                        }
			break;
/****************************************************************************/
		case 'b':
		case 'e':
/**************************** PATCHED by Flier ******************************/
			/*(void) next_arg(rest, &rest);*/
                        if (!(person = next_arg(rest, &rest))) person = empty_string;
                        if (check) {
                            if (*mode_string == 'e') exception = 1;
                            else exception = 0;
                            if (tmpjoiner && !exception) {
                                if (add) tmpjoiner->plusb++;
                                else tmpjoiner->minusb++;
                            }
                            if (isserver) {
                                strcat(servline, person);
                                strcat(servline, " ");
                                if (!exception) {
                                    if (add) chan->servplusb++;
                                    else chan->servminusb++;
                                }
                            }
                            if (add) {
                                if (!exception) chan->plusb++;
                                if (userhost) sprintf(tmpbuf, "%s!%s", from, userhost);
                                else strcpy(tmpbuf, from);
                                if (AddBan(person, chan->channel, server, tmpbuf, exception, timenow, chan)) {
                                    if (chan->CompressModes) {
                                        if (compadd != add) {
                                            *compmodeadd ++= '+';
                                            compadd = add;
                                        }
                                        *compmodeadd ++= *mode_string;
                                        strcat(compline, person);
                                        strcat(compline, " ");
                                    }
                                }
                                if (!exception && !isitme && chan->FriendList) {
                                    for (joiner = chan->nicks; joiner; joiner = joiner->next) {
                                        if (joiner->frlist) privs = joiner->frlist->privs;
                                        else privs = 0;
                                        if ((privs & (FLPROT | FLGOD)) && (privs & FLUNBAN) && 
                                            !(!(privs & FLGOD) && (isprot & FLGOD))) {
                                            if (joiner->userhost)
                                                sprintf(tmpbuf, "%s!%s", joiner->nick, joiner->userhost);
                                            else strcpy(tmpbuf, joiner->nick);
                                            if (wild_match(person, tmpbuf)) {
                                                if (*chop & CHAN_CHOP) {
                                                    if (!deopped && !(isprot & FLGOD) &&
                                                        !isserver && (privs & FLGOD)) {
                                                        count++;
                                                        strcat(modebuf, "-o");
                                                        strcat(tmpbufmode, " ");
                                                        strcat(tmpbufmode, from);
                                                        deopped = 1;
                                                    }
                                                    if (count == max) {
                                                        send_to_server("MODE %s %s %s",
                                                                       chan->channel,
                                                                       modebuf,
                                                                       tmpbufmode);
                                                        *tmpbufmode = '\0';
                                                        *modebuf = '\0';
                                                        count = 0;
                                                    }
                                                    if ((privs & FLGOD) ||
                                                        ((privs & FLPROT) && !(isprot & FLGOD))) {
                                                        count++;
                                                        strcat(modebuf, "-b");
                                                        strcat(tmpbufmode, " ");
                                                        strcat(tmpbufmode, person);
                                                    }
                                                }
                                                if (away_set || LogOn || (chan && chan->ChanLog)) {
                                                    sprintf(tmpbuf, "%cBan%c on mask %s on channel %s by %s",
                                                            bold, bold, person,
                                                            chan->channel, from);
                                                    if (!isserver) {
                                                        strcat(tmpbuf, " (");
                                                        strcat(tmpbuf, userhost);
                                                        strcat(tmpbuf, ")");
                                                    }
                                                    if (away_set || LogOn) AwaySave(tmpbuf, SAVEPROT);
                                                    if (chan && chan->ChanLog) ChannelLogSave(tmpbuf, chan);
                                                }
                                                break;
                                            }
                                        }
                                    }
                                    if (!joiner) {
                                        whowas = whowas_userlist_list;
                                        if (!whowas) {
                                            whowas = whowas_reg_list;
                                            whowasdone = 1;
                                        }
                                        if (!whowas) whowasdone = 2;
                                        while (whowasdone < 2) {
                                            if (!whowas && !whowasdone) {
                                                whowas = whowas_reg_list;
                                                whowasdone = 1;
                                                continue;
                                            }
                                            if (!whowas && whowasdone == 1) {
                                                whowasdone = 2;
                                                continue;
                                            }
                                            if (!my_stricmp(whowas->channel, chan->channel)) {
                                                if (whowas->nicklist->frlist)
                                                    privs = whowas->nicklist->frlist->privs;
                                                else privs = 0;
                                                if (privs & (FLGOD | FLPROT) && (privs & FLUNBAN) &&
                                                    !(!(privs & FLGOD) && (isprot & FLGOD))) {
                                                    if (whowas->nicklist->userhost)
                                                        sprintf(tmpbuf, "%s!%s",
                                                                whowas->nicklist->nick,
                                                                whowas->nicklist->userhost);
                                                    else strcpy(tmpbuf, whowas->nicklist->nick);
                                                    if (wild_match(person, tmpbuf)) {
                                                        if (*chop & CHAN_CHOP) {
                                                            count++;
                                                            strcat(modebuf, "-b");
                                                            strcat(tmpbufmode, " ");
                                                            strcat(tmpbufmode, person);
                                                            if (count == max) {
                                                                send_to_server("MODE %s %s %s",
                                                                               chan->channel,
                                                                               modebuf,
                                                                               tmpbufmode);
                                                                *tmpbufmode = '\0';
                                                                *modebuf = '\0';
                                                                count = 0;
                                                            }
                                                            if (!deopped && !isprot && !isserver &&
                                                                (privs & FLGOD)) {
                                                                count++;
                                                                strcat(modebuf, "-o");
                                                                strcat(tmpbufmode, " ");
                                                                strcat(tmpbufmode, from);
                                                            }
                                                            deopped = 1;
                                                        }
                                                        if (away_set || LogOn || (chan && chan->ChanLog)) {
                                                            sprintf(tmpbuf,
                                                                    "%cBan%c on mask %s on channel %s by %s",
                                                                    bold, bold, person,
                                                                    chan->channel, from);
                                                            if (!isserver) {
                                                                strcat(tmpbuf, " (");
                                                                strcat(tmpbuf, userhost);
                                                                strcat(tmpbuf, ")");
                                                            }
                                                            if (away_set || LogOn) AwaySave(tmpbuf, SAVEPROT);
                                                            if (chan && chan->ChanLog)
                                                                ChannelLogSave(tmpbuf, chan);
                                                        }
                                                        break;
                                                    }
                                                }
                                            }
                                            whowas = whowas->next;
                                        }
                                    }
                                }
                            }
                            else {
                                if (!exception) chan->minusb++;
                                minusban = 1;
                                if (RemoveBan(person, exception, chan)) {
                                    if (chan->CompressModes) {
                                        if (compadd != add) {
                                            *compmodeadd ++= '-';
                                            compadd = add;
                                        }
                                        *compmodeadd ++= *mode_string;
                                        strcat(compline, person);
                                        strcat(compline, " ");
                                    }
                                }
                            }
                        }
/****************************************************************************/
			break;
/**************************** PATCHED by Flier ******************************/
 		/*case 'e':*/
                /* we handle e properly together with b above */
/****************************************************************************/
 		case 'I':
 		case 'O': /* this is a weird special case */
/**************************** Patched by Flier ******************************/
  			/*(void) next_arg(rest, &rest);*/
  			arg = next_arg(rest, &rest);
                        if (arg && check && chan->CompressModes) {
                            strcat(compline, arg);
                            strcat(compline, " ");
                        }
/****************************************************************************/
  			break;
		case 'R':
			value = MODE_REGONLY;
			break;
		}
		if (add)
			*mode |= value;
		else
			*mode &= ~value;
/**************************** PATCHED by Flier ******************************/
                if (check && count == max) {
                    if (HAS_OPS(*chop)) send_to_server("MODE %s %s %s", chan->channel,
                                                        modebuf, tmpbufmode);
                    *tmpbufmode = '\0';
                    *modebuf = '\0';
                    count = 0;
                }
/****************************************************************************/
	}
/**************************** PATCHED by Flier ******************************/
        if (tmpjoiner && tmpjoiner->frlist) privs = tmpjoiner->frlist->privs;
        else privs = 0;
        if (chan->FriendList && chan->MDopWatch && tmpjoiner && !isitme &&
            !(privs & (FLGOD | FLPROT))) {
            if (tmpjoiner->curo >= DeopSensor && tmpjoiner->curo < DeopSensor * 2) {
                if (!(tmpjoiner->deopp)) {
                    if (!deopped && !isserver && (*chop & CHAN_CHOP)) {
                        count++;
                        strcat(modebuf, "-o");
                        strcat(tmpbufmode, " ");
                        strcat(tmpbufmode, from);
                    }
                    deopped = 1;
#ifdef WANTANSI
                    sprintf(tmpbuf, "%sMass deop%s detected on %s%s%s by %s%s%s",
                            CmdsColors[COLWARNING].color1, Colors[COLOFF],
                            CmdsColors[COLWARNING].color4, chan->channel, Colors[COLOFF],
                            CmdsColors[COLWARNING].color2, from, Colors[COLOFF]);
#else
                    sprintf(tmpbuf,"%cMass deop%c detected on %s by %s",
                            bold, bold, chan->channel, from);
#endif
                    say("%s", tmpbuf);
                    if (away_set || LogOn || (chan && chan->ChanLog)) {
                        if (!isserver) {
                            strcat(tmpbuf, " (");
                            strcat(tmpbuf, userhost);
                            strcat(tmpbuf, ")");
                        }
                        if (away_set || LogOn) AwaySave(tmpbuf, SAVEMASS);
                        if (chan && chan->ChanLog) ChannelLogSave(tmpbuf, chan);
                    }
                }
                tmpjoiner->deopp = 1;
            }
            else if (tmpjoiner->curo >= DeopSensor * 2) {
                if (tmpjoiner->deopp < 2) {
                    if (!deopped && !isserver && chan->KickOnFlood && (*chop & CHAN_CHOP))
                        send_to_server("KICK %s %s :Deop flood detected", chan->channel, from);
                    deopped = 1;
#ifdef WANTANSI
                    sprintf(tmpbuf, "%sDeop flood%s detected on %s%s%s by %s%s%s",
                            CmdsColors[COLWARNING].color1, Colors[COLOFF],
                            CmdsColors[COLWARNING].color4, chan->channel, Colors[COLOFF],
                            CmdsColors[COLWARNING].color2, from, Colors[COLOFF]);
#else
                    sprintf(tmpbuf,"%cDeop flood%c detected on %s by %s",
                            bold, bold, chan->channel, from);
#endif
                    say("%s", tmpbuf);
                    if (away_set || LogOn || (chan && chan->ChanLog)) {
                        sprintf(tmpbuf, "%cDeop flood%c detected on %s by %s",
                                bold, bold, chan->channel, from);
                        if (!isserver) {
                            strcat(tmpbuf, " (");
                            strcat(tmpbuf, userhost);
                            strcat(tmpbuf, ")");
                        }
                        if (away_set || LogOn) AwaySave(tmpbuf, SAVEMASS);
                        if (chan && chan->ChanLog) ChannelLogSave(tmpbuf, chan);
                    }
                }
                tmpjoiner->deopp = 2;
            }
        }
        if (check) {
            if (HAS_OPS(*chop) && count)
                send_to_server("MODE %s %s %s", chan->channel, modebuf, tmpbufmode);
            if ((chan->status & CHAN_CHOP) && chan->NHProt && *nhdeop)
                send_to_server("MODE %s %s", chan->channel, nhdeop);
            *servmodeadd = '\0';
            if (*servmodes) {
                if (*servline) {
                    strcat(servmodes, " ");
                    strcat(servmodes, servline);
                }
                if (servmodes[strlen(servmodes) - 1] == ' ')
                    servmodes[strlen(servmodes) - 1] = '\0';
            }
            if (!chan->gotbans || !chan->gotwho) gotops = 0;
            if (!isitme && minusban && HAS_OPS(*chop) && chan->gotbans && chan->gotwho && !gotops && chan->BKList)
                CheckPermBans(chan);
            if (!isitme && gotops && (chan->FriendList || chan->BKList))
                HandleGotOps(mynick, chan);
            if (chan->CompressModes) {
                *compmodeadd = '\0';
                if (*compmode) sprintf(origmode, "%s %s", compmode, compline);
                else *origmode = '\0';
                if (*origmode && origmode[strlen(origmode) - 1] == ' ')
                    origmode[strlen(origmode) - 1] = '\0';
            }
            else strcpy(origmode, tmporigmode);
        }
/****************************************************************************/
	if (limit_set)
		return (atoi(limit));
	else if (limit_reset)
		return(0);
	else
		return(-1);
}

/*
 * get_channel_mode: returns the current mode string for the given channel
 */
char	*
get_channel_mode(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *tmp;

	if ((tmp = lookup_channel(channel, server, CHAN_NOUNLINK)) && (tmp->status & CHAN_MODE))
		return recreate_mode(tmp);
	return empty_string;
}

/*
 * update_channel_mode: This will modify the mode for the given channel
 * according the the new mode given.  
 */
void
/**************************** PATCHED by Flier ******************************/
/*update_channel_mode(channel, server, mode)
	char	*channel;
	int	server;
	char	*mode;*/
update_channel_mode(channel,server,mode,from,userhost,nethacks,servmodes,tmpchan)
char *channel;
int   server;
char *mode;
char *from;
char *userhost;
char *nethacks;
char *servmodes;
ChannelList *tmpchan;
/****************************************************************************/
{
	ChannelList *tmp;
	int	limit;

/**************************** PATCHED by Flier ******************************/
        if (tmpchan) tmp=tmpchan;
        else tmp=lookup_channel(channel, server, CHAN_NOUNLINK);
	/*if ((tmp = lookup_channel(channel, server, CHAN_NOUNLINK)) &&*/
	if (tmp &&
                /*(limit = decifer_mode(mode, &(tmp->mode), &(tmp->chop), &(tmp->nicks), &(tmp->key))) != -1)*/
                (limit=decifer_mode(mode,&(tmp->mode),&(tmp->status),&(tmp->nicks),
                                    &(tmp->key),tmp,from,userhost,nethacks,servmodes))!=-1)
/****************************************************************************/
		tmp->limit = limit;
}

/*
 * is_channel_mode: returns the logical AND of the given mode with the
 * channels mode.  Useful for testing a channels mode 
 */
int
is_channel_mode(channel, mode, server_index)
	char	*channel;
	int	mode;
	int	server_index;
{
	ChannelList *tmp;

	if ((tmp = lookup_channel(channel, server_index, CHAN_NOUNLINK)))
		return (tmp->mode & mode);
	return 0;
}

static	void
free_channel(channel)
	ChannelList **channel;
{
/**************************** PATCHED by Flier ******************************/
	/*clear_channel(*channel);
	new_free(&(*channel)->channel);
	new_free(&(*channel)->key);
 	new_free(&(*channel)->s_mode);
        new_free(&(*channel));*/

        NickList *nick,*tmpnick;

        ChannelLogReport("ended", *channel);
        (*channel)->mode=0;
        (*channel)->limit=0;
        new_free(&((*channel)->key));
        new_free(&((*channel)->s_mode));
        new_free(&((*channel)->topicstr));
        new_free(&((*channel)->topicwho));
        ClearBans((*channel));
        for (nick=(*channel)->nicks;nick;) {
            tmpnick=nick;
            nick=nick->next;
            add_to_whowas_buffer(tmpnick,(*channel)->channel);
        }
        (*channel)->nicks=(NickList *) 0;
        if (!(add_to_whowas_chan_buffer((*channel)))) {
            clear_channel(*channel);
            new_free(&(*channel)->channel);
            new_free(&(*channel));
        }
/****************************************************************************/
}

/*
 * remove_channel: removes the named channel from the
 * server_list[server].chan_list.  If the channel is not on the
 * server_list[server].chan_list, nothing happens.  If the channel was
 * the current channel, this will select the top of the
 * server_list[server].chan_list to be the current_channel, or 0 if the
 * list is empty. 
 */
void
remove_channel(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *tmp;

	if (channel)
	{
		int refnum = -1;

		if ((tmp = lookup_channel(channel, server, CHAN_NOUNLINK)))
		{
			tmp = lookup_channel(channel, server, CHAN_UNLINK);
			if (tmp->window)
				refnum = tmp->window->refnum;
			free_channel(&tmp);
		}

		(void)is_current_channel(channel, server, refnum);
	}
	else
	{
		ChannelList *next;

		for (tmp = server_list[server].chan_list; tmp; tmp = next)
		{
			next = tmp->next;
			free_channel(&tmp);
		}
		server_list[server].chan_list = (ChannelList *) 0;
	}
	update_all_windows();
}

/**************************** PATCHED by Flier ******************************/
/*
 * purge given channel from memory, for use on numeric 405
 */
void
PurgeChannel(channel,server)
char *channel;
int  server;
{
    ChannelList *tmp;

    if ((tmp=lookup_channel(channel,server,CHAN_UNLINK))) {
        new_free(&(tmp->key));
        new_free(&(tmp->s_mode));
        new_free(&(tmp->topicstr));
        new_free(&(tmp->topicwho));
        new_free(&(tmp->channel));
        /* clear_channel removes bans */
        clear_channel(tmp);
        new_free(&tmp);
    }
}

/* removes given nick's entry from hash table */
void remove_nick_from_hash(chan,tmp)
ChannelList *chan;
NickList *tmp;
{
    int i=HashFunc(tmp->nick);
    struct hashstr *tmphash;
    struct hashstr *prevhash=NULL;

    if (!chan || !tmp || !(tmp->nick)) return;
    for (tmphash=chan->nickshash[i];tmphash;) {
        if (tmphash->nick==tmp) break;
        prevhash=tmphash;
        tmphash=tmphash->next;
    }
    if (prevhash) prevhash->next=tmphash->next;
    else if (tmphash) chan->nickshash[i]=tmphash->next;
    new_free(&tmphash);
}
/****************************************************************************/

/*
 * remove_from_channel: removes the given nickname from the given channel. If
 * the nickname is not on the channel or the channel doesn't exist, nothing
 * happens. 
 */
void
remove_from_channel(channel, nick, server)
	char	*channel;
	char	*nick;
	int	server;
{
	ChannelList *chan;
	NickList *tmp;

	if (channel)
	{
		if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		{
			if ((tmp = (NickList *) list_lookup((List **) &(chan->nicks), nick, !USE_WILDCARDS, REMOVE_FROM_LIST)))
			{
/**************************** PATCHED by Flier ******************************/
				/*new_free(&(tmp->nick));
				new_free(&tmp);*/
                                remove_nick_from_hash(chan,tmp);
                                add_to_whowas_buffer(tmp,channel);
/****************************************************************************/
			}
		}
	}
	else
	{
		for (chan = server_list[server].chan_list; chan; chan = chan->next)
		{
			if ((tmp = (NickList *) list_lookup((List **) &(chan->nicks), nick, !USE_WILDCARDS, REMOVE_FROM_LIST)))
			{
/**************************** PATCHED by Flier ******************************/
				/*new_free(&(tmp->nick));
				new_free(&tmp);*/
                                remove_nick_from_hash(chan,tmp);
                                add_to_whowas_buffer(tmp,chan->channel);
/****************************************************************************/
			}
		}
	}
/**************************** PATCHED by Flier ******************************/
        update_all_status();
/****************************************************************************/
}

/*
 * rename_nick: in response to a changed nickname, this looks up the given
 * nickname on all you channels and changes it the new_nick 
 */
void
rename_nick(old_nick, new_nick, server)
	char	*old_nick,
		*new_nick;
	int	server;
{
	ChannelList *chan;
	NickList *tmp;
/**************************** PATCHED by Flier ******************************/
        char tmpbuf[mybufsize/8];
/****************************************************************************/

	for (chan = server_list[server].chan_list; chan; chan = chan->next)
	{
		if ((chan->server == server) != 0)
		{
/**************************** PATCHED by Flier ******************************/
			/*if ((tmp = (NickList *) list_lookup((List **) &chan->nicks, old_nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)))*/
#ifdef SORTED_NICKS
                        if ((tmp=(NickList *) list_lookup((List **) &chan->nicks,
                                                          old_nick,!USE_WILDCARDS,REMOVE_FROM_LIST)) != NULL)
#else
                        if ((tmp=find_in_hash(chan,old_nick)))
#endif
/****************************************************************************/
			{
/**************************** PATCHED by Flier ******************************/
                                remove_nick_from_hash(chan,tmp);
/****************************************************************************/
				new_free(&tmp->nick);
				malloc_strcpy(&tmp->nick, new_nick);
/**************************** PATCHED by Flier ******************************/
#ifdef SORTED_NICKS
                                add_to_list_ext((List **) &(chan->nicks), (List *) tmp,
                                                SortedCmp);
#endif
                                add_nick_to_hash(chan,tmp);
                                if (tmp->userhost) {
                                    sprintf(tmpbuf,"%s!%s",tmp->nick,tmp->userhost);
                                    tmp->frlist=(struct friends *) FindMatch(tmpbuf,chan->channel);
                                }
/****************************************************************************/
			}
		}
	}
}

/*
 * is_on_channel: returns true if the given nickname is in the given channel,
 * false otherwise.  Also returns false if the given channel is not on the
 * channel list. 
 */
int
is_on_channel(channel, server, nick)
	char	*channel;
	int	server;
	char	*nick;
{
	ChannelList *chan;

	chan = lookup_channel(channel, server, CHAN_NOUNLINK);
	if (chan && (chan->connected == CHAN_JOINED)
	/* channel may be "surviving" from a server disconnect/reconnect,
					   make sure it's connected -Sol */
/**************************** PATCHED by Flier ******************************/
	/*&& list_lookup((List **) &(chan->nicks), nick, !USE_WILDCARDS, !REMOVE_FROM_LIST))*/
	&& find_in_hash(chan,nick))
/****************************************************************************/
		return 1;
	return 0;
}

int
is_chanop(channel, nick)
	char	*channel;
	char	*nick;
{
	ChannelList *chan;
	NickList *Nick;

	if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)) &&
		(chan->connected == CHAN_JOINED) &&
		/* channel may be "surviving" from a disconnect/connect
						   check here too -Sol */
/**************************** PATCHED by Flier ******************************/
			/*(Nick = (NickList *) list_lookup((List **) &(chan->nicks),
		nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)) && Nick->chanop)*/
			(Nick=find_in_hash(chan,nick)) && (Nick->chanop || Nick->halfop))
/****************************************************************************/
		return 1;
	return 0;
}

int
has_voice(channel, nick, server)
	char	*channel;
	char	*nick;
	int	server;
{
	ChannelList *chan;
	NickList *Nick;

	if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)) &&
		(chan->connected == CHAN_JOINED) &&
		/* channel may be "surviving" from a disconnect/connect
						   check here too -Sol */
/**************************** PATCHED by Flier ******************************/
			/*(Nick = (NickList *) list_lookup((List **) &(chan->nicks),
		nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)) && (Nick->chanop || Nick->hasvoice))*/
                (Nick = find_in_hash(chan, nick)) && Nick->hasvoice)
/****************************************************************************/
		return 1;
	return 0;
}

static	void
show_channel(chan)
	ChannelList *chan;
{
	NickList *tmp;
	int	buffer_len,
		len;
	char	*nicks = (char *) 0;
	char	*s;
	char	buffer[BIG_BUFFER_SIZE+1];

	s = recreate_mode(chan);
	*buffer = (char) 0;
	buffer_len = 0;
	for (tmp = chan->nicks; tmp; tmp = tmp->next)
	{
		len = strlen(tmp->nick);
		if (buffer_len + len >= (BIG_BUFFER_SIZE / 2))
		{
			malloc_strcpy(&nicks, buffer);
			say("\t%s +%s (%s): %s", chan->channel, s, get_server_name(chan->server), nicks);
			*buffer = (char) 0;
			buffer_len = 0;
		}
		strmcat(buffer, tmp->nick, BIG_BUFFER_SIZE);
		strmcat(buffer, " ", BIG_BUFFER_SIZE);
		buffer_len += len + 1;
	}
	malloc_strcpy(&nicks, buffer);
	say("\t%s +%s (%s): %s", chan->channel, s, get_server_name(chan->server), nicks);
	new_free(&nicks);
}

/* list_channels: displays your current channel and your channel list */
void
list_channels()
{
	ChannelList *tmp;
	int	server,
		no = 1;
	int	first;

	if (connected_to_server < 1)
	{
		say("You are not connected to a server, use /SERVER to connect.");
		return;
	}
	if (get_channel_by_refnum(0))
		say("Current channel %s", get_channel_by_refnum(0));
	else
		say("No current channel for this window");
	first = 1;
	for (tmp = server_list[get_window_server(0)].chan_list; tmp; tmp = tmp->next)
	{
		if (tmp->connected != CHAN_JOINED)
			continue;
		if (first)
			say("You are on the following channels:");
		show_channel(tmp);
		first = 0;
		no = 0;
	}

	if (connected_to_server > 1)
	{
		for (server = 0; server < number_of_servers; server++)
		{
			if (server == get_window_server(0))
				continue;
			first = 1;
			for (tmp = server_list[server].chan_list; tmp; tmp = tmp->next)
			{
				if (tmp->connected != CHAN_JOINED)
					continue;
				if (first)
					say("Other servers:");
				show_channel(tmp);
				first = 0;
				no = 0;
			}
		}
	}
	if (no)
		say("You are not on any channels");
}

void
switch_channels(key, ptr)
 	u_int	key;
	char *	ptr;
{
	ChannelList *	tmp;
	char *	s;

	if (server_list[from_server].chan_list)
	{
		if (get_channel_by_refnum(0))
		{
			if ((tmp = lookup_channel(get_channel_by_refnum(0), from_server, CHAN_NOUNLINK)))
			{
				for (tmp = tmp->next; tmp; tmp = tmp->next)
				{
					s = tmp->channel;
					if (!is_current_channel(s, from_server, 0) && !(is_bound(s, from_server) && curr_scr_win != tmp->window))
					{
						set_channel_by_refnum(0, s);
						update_all_windows();
/**************************** PATCHED by Flier ******************************/
                                                tabnickcompl=NULL;
/****************************************************************************/
						return;
					}
				}
			}
		}
		for (tmp = server_list[from_server].chan_list; tmp; tmp = tmp->next)
		{
			s = tmp->channel;
			if (!is_current_channel(s, from_server, 0) && !(is_bound(s, from_server) && curr_scr_win != tmp->window))
			{
				set_channel_by_refnum(0, s);
				update_all_windows();
/**************************** PATCHED by Flier ******************************/
                                tabnickcompl=NULL;
/****************************************************************************/
				return;
			}
		}
	}
}

void
change_server_channels(old, new)
	int	old,
		new;
{
	ChannelList *tmp;

	if (new == old)
		return;
	if (old > -1)
	{
		for (tmp = server_list[old].chan_list; tmp ;tmp = tmp->next)
			tmp->server = new;
		server_list[new].chan_list = server_list[old].chan_list;
	}
	else
		server_list[new].chan_list = (ChannelList *) 0;
}

void
clear_channel_list(server)
	int	server;
{
	ChannelList *tmp,
		*next;
	Window		*ptr;
	int		flag = 1;

	while ((ptr = traverse_all_windows(&flag)))
		if (ptr->server == server && ptr->current_channel)
			new_free(&ptr->current_channel);
	
	for (tmp = server_list[server].chan_list; tmp; tmp = next)
	{
		next = tmp->next;
/**************************** PATCHED by Flier ******************************/
		/*free_channel(&tmp);*/
                remove_channel(tmp->channel,tmp->server);
/****************************************************************************/
	}
	server_list[server].chan_list = (ChannelList *) 0;
	return;
}

/*
 * reconnect_all_channels: used after you get disconnected from a server, 
 * clear each channel nickname list and re-JOINs each channel in the 
 * channel_list ..  
 */
void
reconnect_all_channels(server)
	int	server;
{
	ChannelList *tmp = (ChannelList *) 0;
	char	*mode, *chan;
/**************************** PATCHED by Flier ******************************/
	ChannelList *next;
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
        /*for (tmp = server_list[server].chan_list; tmp; tmp = tmp->next)*/
        for (tmp = server_list[server].chan_list;tmp;tmp=next)
/****************************************************************************/
	{
/**************************** PATCHED by Flier ******************************/
                next=tmp->next;
/****************************************************************************/
		mode = recreate_mode(tmp);
		chan = tmp->channel;
		if (get_server_version(server) >= Server2_8)
			send_to_server("JOIN %s%s%s", tmp->channel, tmp->key ? " " : "", tmp->key ? tmp->key : "");
		else
			send_to_server("JOIN %s%s%s", tmp->channel, mode ? " " : "", mode ? mode : "");
		tmp->connected = CHAN_JOINING;
	}
}

char	*
what_channel(nick, server)
	char	*nick;
	int	server;
{
	ChannelList *tmp;

	if (curr_scr_win->current_channel && is_on_channel(curr_scr_win->current_channel, curr_scr_win->server, nick))
		return curr_scr_win->current_channel;

	for (tmp = server_list[from_server].chan_list; tmp; tmp = tmp->next)
/**************************** PATCHED by Flier ******************************/
		/*if (list_lookup((List **) &(tmp->nicks), nick, !USE_WILDCARDS, !REMOVE_FROM_LIST))*/
		if (find_in_hash(tmp,nick))
/****************************************************************************/
			return tmp->channel;

	return (char *) 0;
}

char	*
walk_channels(nick, init, server)
	int	init;
	char	*nick;
	int	server;
{
	static	ChannelList *tmp = (ChannelList *) 0;

	if (init)
		tmp = server_list[server].chan_list;
	else if (tmp)
		tmp = tmp->next;
	for (;tmp ; tmp = tmp->next)
/**************************** PATCHED by Flier ******************************/
		/*if ((tmp->server == from_server) && (list_lookup((List **) &(tmp->nicks), nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)))*/
		if ((tmp->server == from_server) && (find_in_hash(tmp,nick)))
/****************************************************************************/
			return (tmp->channel);
	return (char *) 0;
}

int
get_channel_oper(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *chan;

	if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		return (chan->status & CHAN_CHOP);
	else
		return 1;
}

extern	void
set_channel_window(window, channel, server)
	Window	*window;
	char	*channel;
	int	server;
{
	ChannelList	*tmp;

	if (!channel)
		return;
	for (tmp = server_list[server].chan_list; tmp; tmp = tmp->next)
		if (!my_stricmp(channel, tmp->channel))
		{
			tmp->window = window;
			return;
		}
}

extern	char	*
create_channel_list(window)
	Window	*window;
{
	ChannelList	*tmp;
	char	*value = (char *) 0;
	char	buffer[BIG_BUFFER_SIZE+1];

	*buffer = '\0';
	for (tmp = server_list[window->server].chan_list; tmp; tmp = tmp->next)
	{
		strcat(buffer, tmp->channel);
		strcat(buffer, " ");
	}
	malloc_strcpy(&value, buffer);

	return value;
}

extern void
channel_server_delete(server)
	int     server;
{
	ChannelList     *tmp;
	int	i;

	for (i = server + 1; i < number_of_servers; i++)
		for (tmp = server_list[i].chan_list ; tmp; tmp = tmp->next)
			if (tmp->server >= server)
				tmp->server--;
}

extern	int
chan_is_connected(channel, server)
	char *	channel;
	int	server;
{
	ChannelList *	cp = lookup_channel(channel, server, CHAN_NOUNLINK);

	if (!cp)
		return 0;

	return (cp->connected == CHAN_JOINED);
}

void
mark_not_connected(server)
	int	server;
{
	ChannelList	*tmp;

	for (tmp = server_list[server].chan_list; tmp; tmp = tmp->next)
	{
		tmp->status = 0;
		tmp->connected = CHAN_LIMBO;
	}
}
