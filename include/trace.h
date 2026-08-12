/******************************************************************************
  ScrollZ tracing facility

  Copyright (C) Flier 2013
******************************************************************************/

#ifndef __trace_h_
# define __trace_h_

#define SZ_TRACE_SERVER    1<<0
#define SZ_TRACE_CONNECT   1<<1
#define SZ_TRACE_JOIN      1<<2
#define SZ_TRACE_PART      1<<3
#define SZ_TRACE_CHANNEL   1<<4
#define SZ_TRACE_IO        1<<5
#define SZ_TRACE_WHOWAS    1<<6
#define SZ_TRACE_NICK      1<<7
#define SZ_TRACE_WINDOW    1<<8
#define SZ_TRACE_LASTLOG   1<<9

#define SZ_TRACE_ALL       0xffffffff

typedef struct {
    long value;
    char *area;
} TraceArea;

void TraceServerInfo _((int, int));
void TraceChannelInfo _((int, ChannelList *));
void TraceNickListInfo _((int, NickList *));
void TraceWindowInfo _((int, Window *));
long GetTraceLevel _((void));
void SetTraceLevel _((long));
#ifdef HAVE_STDARG_H
void Trace _((long, char *, ...));
#else
void Trace _((long, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *, char *));
#endif

#endif
