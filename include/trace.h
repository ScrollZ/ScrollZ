/******************************************************************************
  ScrollZ tracing facility

  Copyright (C) Flier 2013
******************************************************************************/

#define SZ_TRACE_SERVER    1<<0
#define SZ_TRACE_CONNECT   1<<1
#define SZ_TRACE_JOIN      1<<2
#define SZ_TRACE_PART      1<<3
#define SZ_TRACE_CHANNEL   1<<4
#define SZ_TRACE_IO        1<<5

#define SZ_TRACE_ALL       0xffffffff

typedef struct {
    long value;
    char *area;
} TraceArea;
