/******************************************************************************
  ScrollZ tracing facility

  Copyright (C) Flier 2013
******************************************************************************/

#include "irc.h"
#include "vars.h"
#include "ircaux.h"
#include "trace.h"


static int TraceMask = 0;

static TraceArea TraceAreas[]= {
    { SZ_TRACE_SERVER  , "SERVER"  },
    { SZ_TRACE_CONNECT , "CONNECT" },
    { SZ_TRACE_JOIN    , "JOIN"    },
    { SZ_TRACE_PART    , "PART"    },
    { SZ_TRACE_CHANNEL , "CHANNEL" },
    { SZ_TRACE_IO      , "IO" },
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
            if ((len = strlen(str)) != 0) {
                char *cmd = NULL;

                malloc_strcpy(&cmd, str);
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
