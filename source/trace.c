/******************************************************************************
  ScrollZ tracing facility

  Copyright (C) Flier 2013
******************************************************************************/

#include "irc.h"
#include "vars.h"
#include "ircaux.h"
#include "server.h"
#include "trace.h"

#ifdef SZTRACE
extern char *OpenCreateFile(char *, int);

static int TraceMask = 0;

static TraceArea TraceAreas[]= {
    { SZ_TRACE_SERVER  , "SERVER"  },
    { SZ_TRACE_CONNECT , "CONNECT" },
    { SZ_TRACE_JOIN    , "JOIN"    },
    { SZ_TRACE_PART    , "PART"    },
    { SZ_TRACE_CHANNEL , "CHANNEL" },
    { SZ_TRACE_IO      , "IO"      },
    { SZ_TRACE_WHOWAS  , "WHOWAS"  },
    { SZ_TRACE_NICK    , "NICK"    },
    { SZ_TRACE_WINDOW  , "WINDOW"  },
    { SZ_TRACE_LASTLOG , "LASTLOG" },
    { 0                , NULL      }
};

long GetTraceLevel(void)
{
    return(TraceMask);
}

void SetTraceLevel(long mask)
{
    TraceMask = mask;
}

char *
BitsToTraceLevel (long mask)
{
    int i;
    static char buf[mybufsize];

    if (mask == SZ_TRACE_ALL) strcpy(buf, "ALL");
    else if (mask == 0) strcpy(buf, "NONE");
    else {
        *buf = '\0';
        for (i = 0; TraceAreas[i].value; i++) {
            if (mask & TraceAreas[i].value) {
                strmcat(buf, TraceAreas[i].area, sizeof(buf));
                strmcat(buf, " ", sizeof(buf));
            }
        }
    }
    return(buf);
}
#endif

/* Set trace mask */
void 
SetTrace (char *level)
{
#ifdef SZTRACE
    int i, neg, len;
    long mask = 0;
    char *s;
    char *str;
    char *rest;

    while ((level = next_arg(level, &rest)) != NULL) {
        while (level) {
            if ((str = index(level, ',')) != NULL)
                *str++ = '\0';
            if ((len = strlen(level)) != 0) {
                char *cmd = NULL;

                malloc_strcpy(&cmd, level);
                upper(cmd);
                if (strncmp(cmd, "ALL", len) == 0) mask = SZ_TRACE_ALL;
                else if (strncmp(cmd, "NONE", len) == 0) mask = 0;
                else {
                    if (*level == '-') {
                        level++;
                        s = cmd + 1;
                        neg = 1;
                        len--;
                    }
                    else if (*level == '+') {
                        level++;
                        s = cmd + 1;
                        neg = 0;
                        len--;
                    }
                    else {
                        neg = 0;
                        s = cmd;
                    }
                    for (i = 0; TraceAreas[i].value; i++) {
                        if (!strncmp(s, TraceAreas[i].area, len)) {
                            if (neg) mask &= (SZ_TRACE_ALL ^ TraceAreas[i].value);
                            else mask |= TraceAreas[i].value;
                            break;
                        }
                    }
                    if (!TraceAreas[i].area)
                        say("Unknown trace area: %s", level);
                }
                new_free(&cmd);
            }
            level = str;
        }
        level = rest;
    }
    TraceMask = mask;
    set_string_var(TRACE_VAR, BitsToTraceLevel(TraceMask));
#else
    /* say("Trace facility was not enabled at compile time"); */
#endif
}

#ifdef SZTRACE
char *
FindArea (long area)
{
    int i;

    for (i = 0; TraceAreas[i].value; i++) {
        if (area == TraceAreas[i].value) {
            return(TraceAreas[i].area);
        }
    }
    return("UNKNOWN");
}
#endif

void Trace(long area, char *format, char *arg1, char *arg2, char *arg3, char *arg4, char *arg5, char *arg6, char *arg7, char *arg8, char *arg9, char *arg10)
{
#ifdef SZTRACE
#ifdef HAVE_STDARG_H
    va_list args;
#else
    char putbuf[mybufsize * 4];
#endif
    time_t timenow = time(NULL);
    char timestr[mybufsize/16];
    char *tracefile;
    char *areaname;
    char *srvname;
    FILE *fp;

    if (!(TraceMask & area))
        return;

    tracefile = OpenCreateFile("ScrollZ.trace", 1);
    if (!tracefile)
        return;

    areaname = FindArea(area);

    fp = fopen(tracefile, "a");
    if (!fp)
        return;

    if (parsing_server_index != -1) srvname = get_server_name(parsing_server_index);
    else if (from_server != -1) srvname = get_server_name(from_server);
    else srvname = "N/A";
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&timenow));
    fprintf(fp, "%s [%s] %s: ", timestr, areaname, srvname);

#ifdef HAVE_STDARG_H
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
#else
    snprintf(putbuf, sizeof(putbuf), format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
    fprintf(fp, "%s", putbuf);
#endif

    fprintf(fp, "\n");
    fclose(fp);
#endif
}

void 
TraceServerInfo (int indent, int fullinfo)
{
#ifdef SZTRACE
    int i;
    char tmpbuf[mybufsize/8];

    *tmpbuf = '\0';
    for (i = 0; i < indent; i++)
        strmcat(tmpbuf, " ", sizeof(tmpbuf));

    for (i = 0; i < number_of_servers; i++) {
        Trace(SZ_TRACE_SERVER, "%sserver %d) %s:%d", tmpbuf, i,
              EMPTY_STR(get_server_name(i)), server_list[i].port);
        Trace(SZ_TRACE_SERVER, "%s  nick: %s, away: %s, operator: %d",
              tmpbuf,
              EMPTY_STR(server_list[i].nickname), EMPTY_STR(server_list[i].away),
              server_list[i].operator);
        Trace(SZ_TRACE_SERVER, "%s  version: %d, flags: %d",
              tmpbuf, server_list[i].version, server_list[i].flags);
        Trace(SZ_TRACE_SERVER, "%s  umodeflags: %d, umodeflags2: %d",
              tmpbuf, server_list[i].umodeflags, server_list[i].umodeflags2);
        Trace(SZ_TRACE_SERVER, "%s  connect time: %.24s",
              tmpbuf, ctime(&server_list[i].ConnectTime));
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
        Trace(SZ_TRACE_SERVER, "%s  enable ssl: %d",
              tmpbuf, server_list[i].enable_ssl);
#endif
        if (fullinfo) {
          Trace(SZ_TRACE_SERVER, "%s  channels:", tmpbuf);
          TraceChannelInfo(indent + 4, server_list[i].chan_list);
          Trace(SZ_TRACE_SERVER, "%s  pending channels:", tmpbuf);
          TraceChannelInfo(indent + 4, server_list[i].ChanPendingList);
        }
    }
#endif
}

void TraceChannelInfo(indent, channels)
int indent;
ChannelList *channels;
{
#ifdef SZTRACE
    int i;
    char tmpbuf[mybufsize/8];
    ChannelList *chan;

    *tmpbuf = '\0';
    for (i = 0; i < indent; i++)
        strmcat(tmpbuf, " ", sizeof(tmpbuf));

    for (chan = channels; chan; chan = chan->next) {
        Trace(SZ_TRACE_SERVER, "%schannel %s (%p), server %d, mode %lu (%s)",
              tmpbuf, EMPTY_STR(chan->channel), chan->channel,
              chan->server, chan->mode, EMPTY_STR(chan->s_mode));
        Trace(SZ_TRACE_SERVER, "%s  limit %d, key %s, connected %d",
              tmpbuf, chan->limit, EMPTY_STR(chan->key), chan->connected);
        Trace(SZ_TRACE_SERVER, "%s  status %d, banlist %d, wholist %d",
              tmpbuf, chan->status, chan->gotbans, chan->gotwho);
        Trace(SZ_TRACE_SERVER, "%s  window %d (%s) (%p)",
              tmpbuf,
              chan->window ? chan->window->refnum : -1,
              chan->window ? EMPTY_STR(chan->window->name) : empty_string, chan->window);
        Trace(SZ_TRACE_SERVER, "%s  created at %s",
              tmpbuf, ctime(&chan->creationtime));
        Trace(SZ_TRACE_SERVER, "%s  nicklist:", tmpbuf);
        TraceNickListInfo(indent + 4, chan->nicks);
    }
#endif
}

void TraceNickListInfo(indent, nicks)
int indent;
NickList *nicks;
{
#ifdef SZTRACE
    int i;
    char tmpbuf[mybufsize/8];
    NickList *nick;

    *tmpbuf = '\0';
    for (i = 0; i < indent; i++)
        strmcat(tmpbuf, " ", sizeof(tmpbuf));

    for (nick = nicks; nick; nick = nick->next) {
        Trace(SZ_TRACE_SERVER, "%snick %s (%s)",
              tmpbuf, EMPTY_STR(nick->nick), nick->userhost);
        Trace(SZ_TRACE_SERVER, "%s  op: %d, voice: %d, halfop: %d",
              tmpbuf, nick->chanop, nick->hasvoice, nick->halfop);
        Trace(SZ_TRACE_SERVER, "%s  frlist: %d %d/%s (%s)",
              tmpbuf,
              nick->frlist ? nick->frlist->number : -1,
              nick->frlist ? nick->frlist->privs : -1,
              nick->frlist ? nick->frlist->userhost : empty_string,
              nick->frlist ? nick->frlist->channels : empty_string);
        Trace(SZ_TRACE_SERVER, "%s  shitlist: %d/%s (%s)",
              tmpbuf,
              nick->shitlist ? nick->shitlist->shit : -1,
              nick->shitlist ? nick->shitlist->userhost : empty_string,
              nick->shitlist ? nick->shitlist->channels : empty_string);
    }
#endif
}

void TraceWindowInfo(indent, window)
int indent;
Window *window;
{
#ifdef SZTRACE
    int i, flag = 1;
    Window *tmp;
    char tmpbuf[mybufsize/2];
    struct channels *bound_chan;

    *tmpbuf = '\0';
    for (i = 0; i < indent; i++)
        strmcat(tmpbuf, " ", sizeof(tmpbuf));

    do {
        if (window) tmp = window;
        else tmp = traverse_all_windows(&flag);

        if (!tmp)
            break;

        Trace(SZ_TRACE_WINDOW, "%swindow %d (%s) (%p)",
              tmpbuf, tmp->refnum, EMPTY_STR(tmp->name), tmp);
        Trace(SZ_TRACE_WINDOW, "%s  server: %d, top: %d, bottom: %d, line_cnt: %d",
              tmpbuf, tmp->server, tmp->top, tmp->bottom, tmp->line_cnt);
        Trace(SZ_TRACE_WINDOW, "%s  scroll: %d, display_size: %d, visible: %d",
              tmpbuf, tmp->scroll, tmp->display_size, tmp->visible);
        Trace(SZ_TRACE_WINDOW, "%s  update: %d, miscflags: %d, double_status: %d",
              tmpbuf, tmp->update, tmp->miscflags, tmp->double_status);
        Trace(SZ_TRACE_WINDOW, "%s  prompt: %s", tmpbuf, EMPTY_STR(tmp->prompt));
        Trace(SZ_TRACE_WINDOW, "%s  current_channel: %s",
              tmpbuf, EMPTY_STR(tmp->current_channel));
        for (bound_chan = tmp->bound_chans; bound_chan; bound_chan = bound_chan->next) {
            if (bound_chan == tmp->bound_chans)
                Trace(SZ_TRACE_WINDOW, "%s  bound channels:", tmpbuf);
            Trace(SZ_TRACE_WINDOW, "%s    %s", tmpbuf, bound_chan->channel);
        }
        Trace(SZ_TRACE_WINDOW, "%s  query_nick: %s, window_level: %d, hold_mode: %d",
              tmpbuf, EMPTY_STR(tmp->query_nick), tmp->window_level, tmp->hold_mode);
        Trace(SZ_TRACE_WINDOW, "%s  lastlog_level: %d, lastlog_size: %d",
              tmpbuf, tmp->lastlog_level, tmp->lastlog_size);
        Trace(SZ_TRACE_WINDOW, "%s  notify_level: %d",
              tmpbuf, tmp->notify_level);
        Trace(SZ_TRACE_WINDOW, "%s  logfile: %s, log: %d",
              tmpbuf, EMPTY_STR(tmp->logfile), tmp->log);
        Trace(SZ_TRACE_WINDOW, "%s  server_group: %d, sticky: %d",
              tmpbuf, tmp->server_group, tmp->sticky);

        if (tmp == window) tmp = NULL;
    } while (tmp);
#endif
}
