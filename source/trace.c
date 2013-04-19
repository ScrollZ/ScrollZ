/******************************************************************************
  ScrollZ tracing facility

  Copyright (C) Flier 2013
******************************************************************************/

#include "irc.h"
#include "vars.h"
#include "ircaux.h"
#include "server.h"
#include "trace.h"

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
    { 0                , NULL      }
};

char *BitsToTraceLevel(mask)
long mask;
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

/* Set trace mask */
void SetTrace(level)
char *level;
{
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
                    if (*str == '-') {
                        str++;
                        s = cmd + 1;
                        neg = 1;
                        len--;
                    }
                    else if (*str == '+') {
                        str++;
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
                        say("Unknown trace area: %s", str);
                }
                new_free(&cmd);
            }
            level = str;
        }
        level = rest;
    }
    TraceMask = mask;
    set_string_var(TRACE_VAR, BitsToTraceLevel(TraceMask));
}

char *FindArea(area)
long area;
{
    int i;

    for (i = 0; TraceAreas[i].value; i++) {
        if (area == TraceAreas[i].value) {
            return(TraceAreas[i].area);
        }
    }
    return("UNKNOWN");
}

#ifdef HAVE_STDARG_H
void Trace(long area, char *format, ...)
#else
void Trace(area, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
long area;
char *format;
char *arg1;
char *arg2;
char *arg3;
char *arg4;
char *arg5;
char *arg6;
char *arg7;
char *arg8;
char *arg9,
char *arg10;
#endif
{
#ifdef HAVE_STDARG_H
    va_list args;
#else
    char putbuf[mybufsize * 4];
#endif
    time_t timenow = time(NULL);
    char timestr[mybufsize/16];
    char *tracefile = OpenCreateFile("ScrollZ.trace", 1);
    char *areaname = FindArea(area);
    FILE *fp;

    if (!(TraceMask & area))
        return;

    if (!tracefile)
        return;

    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&timenow));

    fp = fopen(tracefile, "a");
    if (!fp)
        return;

    fprintf(fp, "%s [%s]: ", timestr, areaname);

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
}

void TraceServerInfo(fullinfo)
int fullinfo;
{
    int i;

    for (i = 0; i < number_of_servers; i++) {
        Trace(SZ_TRACE_SERVER, "server %d) %s:%d", i,
              EMPTY_STR(get_server_name(i)), server_list[i].port);
        Trace(SZ_TRACE_SERVER, "  nick: %s, away: %s, operator: %d",
              EMPTY_STR(server_list[i].nickname), EMPTY_STR(server_list[i].away),
              server_list[i].operator);
        Trace(SZ_TRACE_SERVER, "  version: %d, flags: %d",
              server_list[i].version, server_list[i].flags);
        Trace(SZ_TRACE_SERVER, "  umodeflags: %d, umodeflags2: %d",
              server_list[i].umodeflags, server_list[i].umodeflags2);
        Trace(SZ_TRACE_SERVER, "  connect time: %.24s",
              ctime(&server_list[i].ConnectTime));
#if defined(HAVE_SSL) || defined(HAVE_OPENSSL)
        Trace(SZ_TRACE_SERVER, "  enable ssl: %d",
              ctime(server_list[i].enable_ssl));
#endif
        if (fullinfo) {
          Trace(SZ_TRACE_SERVER, "  channels:");
          TraceChannelInfo(4, server_list[i].chan_list);
          Trace(SZ_TRACE_SERVER, "  pending channels:");
          TraceChannelInfo(4, server_list[i].ChanPendingList);
        }
    }
}

void TraceChannelInfo(indent, channels)
int indent;
ChannelList *channels;
{
    int i;
    char tmpbuf[mybufsize/8];
    ChannelList *chan;

    *tmpbuf = '\0';
    for (i = 0; i < indent; i++)
        strmcat(tmpbuf, " ", sizeof(tmpbuf));

    for (chan = channels; chan; chan = chan->next) {
        Trace(SZ_TRACE_SERVER, "%schannel %s, server %d, mode %lu (%s)",
                tmpbuf, EMPTY_STR(chan->channel), chan->server, chan->mode,
                EMPTY_STR(chan->s_mode));
        Trace(SZ_TRACE_SERVER, "%s  limit %d, key %s, connected %d",
                tmpbuf, chan->limit, EMPTY_STR(chan->key), chan->connected);
        Trace(SZ_TRACE_SERVER, "%s  status %d, banlist %d, wholist %d",
                tmpbuf, chan->status, chan->gotbans, chan->gotwho);
        Trace(SZ_TRACE_SERVER, "%s  window %d (%s)",
                tmpbuf,
                chan->window ? chan->window->refnum : -1,
                chan->window ? EMPTY_STR(chan->window->name) : empty_string);
        Trace(SZ_TRACE_SERVER, "%s  created at %s",
                tmpbuf, ctime(&chan->creationtime));
        Trace(SZ_TRACE_SERVER, "%s  nicklist:", tmpbuf);
        TraceNickListInfo(indent + 4, chan->nicks);
    }
}

void TraceNickListInfo(indent, nicks)
int indent;
NickList *nicks;
{
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
}
