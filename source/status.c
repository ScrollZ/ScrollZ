/*
 * status.c: handles the status line updating, etc for IRCII 
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
 * $Id: status.c,v 1.3 1998-09-28 16:43:38 f Exp $
 */

#include "irc.h"

#include "ircterm.h"
#include "status.h"
#include "server.h"
#include "vars.h"
#include "hook.h"
#include "input.h"
#include "edit.h"
#include "window.h"
#include "screen.h"
#include "mail.h"
#include "output.h"
#include "names.h"
#include "ircaux.h"
#include "translat.h"

/**************************** PATCHED by Flier ******************************/
#include "ctcp.h"
#include "myvars.h"

#ifdef WANTANSI
extern int vt100Decode _((char));
#endif
extern int  CheckChannel _((char *, char *));
extern void StripAnsi _((char *, char *, int));

extern time_t  start_time;
/****************************************************************************/

static	char	*convert_format _((char *, int));
static	char	*status_nickname _((Window *));
static	char	*status_query_nick _((Window *));
static	char	*status_right_justify _((Window *));
static	char	*status_chanop _((Window *));
static	char	*status_channel _((Window *));
static	char	*status_server _((Window *));
static	char	*status_mode _((Window *));
static	char	*status_umode _((Window *));
static	char	*status_insert_mode _((Window *));
static	char	*status_overwrite_mode _((Window *));
static	char	*status_away _((Window *));
static	char	*status_oper _((Window *));
/**************************** PATCHED by Flier ******************************/
/*static	char	*status_user0 _((Window *));
static	char	*status_user1 _((Window *));
static	char	*status_user2 _((Window *));
static	char	*status_user3 _((Window *));*/
/****************************************************************************/
static	char	*status_hold _((Window *));
static	char	*status_version _((Window *));
static	char	*status_clock _((Window *));
static	char	*status_hold_lines _((Window *));
static	char	*status_window _((Window *));
static	char	*status_mail _((Window *));
static	char	*status_refnum _((Window *));
static	char	*status_null_function _((Window *));
static	char	*status_notify_windows _((Window *));
static	char	*status_group _((Window *));
static	void	status_make_printable _((unsigned char *, int));
static	void	alarm_switch _((int));
static	char	*convert_sub_format _((char *, char));
/**************************** PATCHED by Flier ******************************/
static	char	*status_user00 _((Window *));
static	char	*status_user01 _((Window *));
static	char	*status_user02 _((Window *));
static	char	*status_user03 _((Window *));
static	char	*status_user04 _((Window *));
static	char	*status_user05 _((Window *));
static	char	*status_user06 _((Window *));
static	char	*status_user07 _((Window *));
static	char	*status_user08 _((Window *));
static	char	*status_user09 _((Window *));
static  char    *status_uptime _((Window *));
static  char    *status_lag _((Window *));
static  char    *status_lastjoin _((Window *));
static  char    *status_dcc _((Window *));
static  char    *status_channeltopic _((Window *));
static  char    *status_fullserver _((Window *));
/****** Coded by Zakath ******/
static	char	*status_packs _((Window *));
static	char	*status_dccgets _((Window *));
static	char	*status_dccsends _((Window *));
static	char    *status_autoget _((Window *));
static	char    *status_security _((Window *));
static	char    *status_frlist _((Window *));
static	char    *status_nhprot _((Window *));
static	char    *status_floodp _((Window *));
static	char    *status_ctcpcloak _((Window *));
static	char    *status_channelcount _((Window *));
/*****************************/
/****************************************************************************/

/*
 * Maximum number of "%" expressions in a status line format.  If you change
 * this number, you must manually change the sprintf() in make_status 
 */
#define MAX_FUNCTIONS 33

/* The format statements to build each portion of the status line */
static	char	*mode_format = (char *) 0;
static	char	*umode_format = (char *) 0;
/**************************** PATCHED by Flier ******************************/
/*static	char	*status_format[3] = {(char *) 0, (char *) 0, (char *) 0,};*/
static	char	*status_format[4] = {(char *) 0, (char *) 0, (char *) 0,};
/****************************************************************************/
static	char	*query_format = (char *) 0;
static	char	*clock_format = (char *) 0;
static	char	*hold_lines_format = (char *) 0;
static	char	*channel_format = (char *) 0;
static	char	*mail_format = (char *) 0;
static	char	*server_format = (char *) 0;
static	char	*notify_format = (char *) 0;
static	char	*group_format = (char *) 0;
/**************************** PATCHED by Flier ******************************/
static  char    *away_format = (char *) 0;
static  char    *channelcount_format = (char *) 0;
static  char    *uptime_format = (char *) 0;
/****************************************************************************/

/*
 * status_func: The list of status line function in the proper order for
 * display.  This list is set in convert_format() 
 */
/**************************** PATCHED by Flier ******************************/
/*static	char	*(*status_func[3][MAX_FUNCTIONS]) _((Window *));*/
static	char	*(*status_func[4][MAX_FUNCTIONS]) _((Window *));
/****************************************************************************/

/* func_cnt: the number of status line functions assigned */
/**************************** PATCHED by Flier ******************************/
/*static	int	func_cnt[3];*/
static	int	func_cnt[4];
/****************************************************************************/

static	int	alarm_hours,		/* hour setting for alarm in 24 hour time */
		alarm_minutes;		/* minute setting for alarm */

/* Stuff for the alarm */
static	struct itimerval clock_timer = { { 10L, 0L }, { 1L, 0L } };
static	struct itimerval off_timer = { { 0L, 0L }, { 0L, 0L } };

static	RETSIGTYPE alarmed _((void));

/* alarmed: This is called whenever a SIGALRM is received and the alarm is on */
static	RETSIGTYPE
alarmed()
{
	say("The time is %s", update_clock(GET_TIME));
	term_beep();
	term_beep();
	term_beep();
}

/*
 * alarm_switch: turns on and off the alarm display.  Sets the system timer
 * and sets up a signal to trap SIGALRMs.  If flag is 1, the alarmed()
 * routine will be activated every 10 seconds or so.  If flag is 0, the timer
 * and signal stuff are reset 
 */
static	void
alarm_switch(flag)
	int	flag;
{
	static	int	alarm_on = 0;

	if (flag)
	{
		if (!alarm_on)
		{
			setitimer(ITIMER_REAL, &clock_timer,
				(struct itimerval *) 0);
			(void) MY_SIGNAL(SIGALRM, alarmed, 0);
			alarm_on = 1;
		}
	}
	else if (alarm_on)
	{
		setitimer(ITIMER_REAL, &off_timer, (struct itimerval *) 0);
		(void) MY_SIGNAL(SIGALRM, (sigfunc *)SIG_IGN, 0);
		alarm_on = 0;
	}
}

/*
 * set_alarm: given an input string, this checks it's validity as a clock
 * type time thingy.  It accepts two time formats.  The first is the HH:MM:XM
 * format where HH is between 1 and 12, MM is between 0 and 59, and XM is
 * either AM or PM.  The second is the HH:MM format where HH is between 0 and
 * 23 and MM is between 0 and 59.  This routine also looks for one special
 * case, "OFF", which sets the alarm string to null 
 */
void
set_alarm(str)
	char	*str;
{
	char	hours[10],
		minutes[10],
		merid[3];
	char	time_str[10];
	int	c,
		h,
		m,
		min_hours,
		max_hours;

	if (str == (char *) 0)
	{
		alarm_switch(0);
		return;
	}
	if (!my_stricmp(str, var_settings[OFF]))
	{
		set_string_var(CLOCK_ALARM_VAR, (char *) 0);
		alarm_switch(0);
		return;
	}
	
	c = sscanf(str, " %2[^:]:%2[^paPA]%2s ", hours, minutes, merid);
	switch (c)
	{
	case 2:
		min_hours = 0;
		max_hours = 23;
		break;
	case 3:
		min_hours = 1;
		max_hours = 12;
		upper(merid);
		break;
	default:
		say("CLOCK_ALARM: Bad time format.");
		set_string_var(CLOCK_ALARM_VAR, (char *) 0);
		return;
	}
	
	h = atoi(hours);
	m = atoi(minutes);
	if (h >= min_hours && h <= max_hours && isdigit(hours[0]) &&
		(isdigit(hours[1]) || hours[1] == (char) 0))
	{
		if (m >= 0 && m <= 59 && isdigit(minutes[0]) &&
				isdigit(minutes[1]))
		{
			alarm_minutes = m;
			alarm_hours = h;
			if (max_hours == 12)
			{
				if (merid[0] != 'A')
				{
					if (merid[0] == 'P')
					{
						if (h != 12)
							alarm_hours += 12;
					}
					else
					{
	say("CLOCK_ALARM: alarm time must end with either \"AM\" or \"PM\"");
	set_string_var(CLOCK_ALARM_VAR, (char *) 0);
					}
				}
				else
				{
					if (h == 12)
						alarm_hours = 0;
				}
				if (merid[1] == 'M')
				{
					sprintf(time_str, "%02d:%02d%s", h, m,
						merid);
					set_string_var(CLOCK_ALARM_VAR,
						time_str);
				}
				else
				{
	say("CLOCK_ALARM: alarm time must end with either \"AM\" or \"PM\"");
	set_string_var(CLOCK_ALARM_VAR, (char *) 0);
				}
			}
			else
			{
				sprintf(time_str, "%02d:%02d", h, m);
				set_string_var(CLOCK_ALARM_VAR, time_str);
			}
		}
		else
		{
	say("CLOCK_ALARM: alarm minutes value must be between 0 and 59.");
	set_string_var(CLOCK_ALARM_VAR, (char *) 0);
		}
	}
	else
	{
		say("CLOCK_ALARM: alarm hour value must be between %d and %d.",
			min_hours, max_hours);
		set_string_var(CLOCK_ALARM_VAR, (char *) 0);
	}
}

/* update_clock: figures out the current time and returns it in a nice format */
char	*
update_clock(flag)
	int	flag;
{
	static	char	time_str[10];
	static	int	min = -1,
			hour = -1;
	struct tm	*time_val;
	char	*merid;
	time_t	t;

	t = time(0);
	time_val = localtime(&t);
	if (get_string_var(CLOCK_ALARM_VAR))
	{
		if ((time_val->tm_hour == alarm_hours) &&
				(time_val->tm_min == alarm_minutes))
			alarm_switch(1);
		else
			alarm_switch(0);
	}
	if (flag == RESET_TIME || time_val->tm_min != min || time_val->tm_hour != hour)
	{
		int	tmp_hour,
			tmp_min,
			server;

		tmp_hour = time_val->tm_hour;
		tmp_min = time_val->tm_min;

		if (get_int_var(CLOCK_24HOUR_VAR))
			merid = empty_string;
		else
		{
			if (time_val->tm_hour < 12)
/**************************** PATCHED by Flier ******************************/
				/*merid = "AM";*/
#ifdef TDF
                                merid="am";
#else
				merid="AM";
#endif
/****************************************************************************/
			else
/**************************** PATCHED by Flier ******************************/
				/*merid = "PM";*/
#ifdef TDF
                                merid="pm";
#else
				merid="PM";
#endif
/****************************************************************************/
			if (time_val->tm_hour > 12)
				time_val->tm_hour -= 12;
			else if (time_val->tm_hour == 0)
				time_val->tm_hour = 12;
		}
		server = from_server;
		from_server = primary_server;
		sprintf(time_str, "%02d:%02d%s", time_val->tm_hour, time_val->tm_min, merid);
		if (tmp_min != min || tmp_hour != hour)
		{
			hour = tmp_hour;
			min = tmp_min;
			do_hook(TIMER_LIST, "%s", time_str);
		}
		do_hook(IDLE_LIST, "%ld", (t - idle_time) / 60L);
		from_server = server;
		return (time_str);
	}
	if (flag == GET_TIME)
		return(time_str);
	else
		return ((char *) 0);
}

/*ARGSUSED*/
void
reset_clock(unused)
	char	*unused;
{
	update_clock(RESET_TIME);
	update_all_status();
}

/*
 * convert_sub_format: This is used to convert the formats of the
 * sub-portions of the status line to a format statement specially designed
 * for that sub-portions.  convert_sub_format looks for a single occurence of
 * %c (where c is passed to the function). When found, it is replaced by "%s"
 * for use is a sprintf.  All other occurences of % followed by any other
 * character are left unchanged.  Only the first occurence of %c is
 * converted, all subsequence occurences are left unchanged.  This routine
 * mallocs the returned string. 
 */
static	char	*
#ifdef __STDC__
convert_sub_format(char *format, char c)
#else
convert_sub_format(format, c)
	char	*format;
	char	c;
#endif
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	static	char	bletch[] = "%% ";
	char	*ptr = (char *) 0;
	int	dont_got_it = 1;

	if (format == (char *) 0)
		return ((char *) 0);
	*buffer = (char) 0;
	while (format)
	{
		if ((ptr = (char *) index(format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat(buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			if ((*ptr == c) && dont_got_it)
			{
				dont_got_it = 0;
				strmcat(buffer, "%s", BIG_BUFFER_SIZE);
			}
			else
			{
				bletch[2] = *ptr;
				strmcat(buffer, bletch, BIG_BUFFER_SIZE);
			}
			ptr++;
		}
		else
			strmcat(buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	malloc_strcpy(&ptr, buffer);
	return (ptr);
}

static	char	*
convert_format(format, k)
	char	*format;
	int	k;
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*ptr,
		*malloc_ptr = (char *) 0; 
	int	*cp;
	
	*buffer = (char) 0;
	while (format)
	{
		if ((ptr = (char *) index(format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat(buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			cp = &func_cnt[k];
			if (*cp < MAX_FUNCTIONS)
			{
				switch (*(ptr++))
				{
				case '%':
					strmcat(buffer, "%", BIG_BUFFER_SIZE);
					break;
				case 'N':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_nickname;
					break;
				case '>':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_right_justify;
					break;
				case 'G':
					new_free(&group_format);
					group_format =
		convert_sub_format(get_string_var(STATUS_GROUP_VAR), 'G');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_group;
					break;
				case 'Q':
					new_free(&query_format);
					query_format =
		convert_sub_format(get_string_var(STATUS_QUERY_VAR), 'Q');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_query_nick;
					break;
				case 'F':
					new_free(&notify_format);
					notify_format = 
		convert_sub_format(get_string_var(STATUS_NOTIFY_VAR), 'F');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_notify_windows;
					break;
				case '@':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_chanop;
					break;
				case 'C':
					new_free(&channel_format);
					channel_format =
		convert_sub_format(get_string_var(STATUS_CHANNEL_VAR), 'C');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_channel;
					break;
				case 'S':
					new_free(&server_format);
					server_format =
		convert_sub_format(get_string_var(STATUS_SERVER_VAR), 'S');
					strmcat(buffer,"%s",BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_server;
					break;
				case '+':
					new_free(&mode_format);
					mode_format =
		convert_sub_format(get_string_var(STATUS_MODE_VAR), '+');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_mode;
					break;
				case '#':
					new_free(&umode_format);
					umode_format =
		convert_sub_format(get_string_var(STATUS_UMODE_VAR), '#');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_umode;
					break;
				case 'M':
					new_free(&mail_format);
					mail_format =
		convert_sub_format(get_string_var(STATUS_MAIL_VAR), 'M');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_mail;
					break;
				case 'I':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_insert_mode;
					break;
				case 'O':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_overwrite_mode;
					break;
				case 'A':
/**************************** PATCHED by Flier ******************************/
					/*strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_away;
					break;*/
					new_free(&away_format);
					away_format =
                convert_sub_format(get_string_var(STATUS_AWAY_VAR), 'A');
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                status_away;
                                        break;
/****************************************************************************/
				case 'V':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_version;
					break;
				case 'R':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_refnum;
					break;
				case 'T':
					new_free(&clock_format);
					clock_format =
		convert_sub_format(get_string_var(STATUS_CLOCK_VAR), 'T');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_clock;
					break;
/**************************** PATCHED by Flier ******************************/
				/*case 'U':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_user0;
					break;*/
/****************************************************************************/
				case 'H':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_hold;
					break;
				case 'B':
					new_free(&hold_lines_format);
					hold_lines_format =
		convert_sub_format(get_string_var(STATUS_HOLD_LINES_VAR), 'B');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_hold_lines;
					break;
				case '*':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_oper;
					break;
				case 'W':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_window;
					break;
/**************************** PATCHED by Flier ******************************/
				/*case 'X':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_user1;
					break;
				case 'Y':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_user2;
					break;
				case 'Z':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_user3;
					break;*/
                                case '1':
                                        new_free(&uptime_format);
                                        uptime_format =
                convert_sub_format(get_string_var(STATUS_UPTIME_VAR), '1');
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                status_uptime;
                                        break;
                                case '2':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_lag;
                                        break;
                                case '3':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_lastjoin;
                                        break;
                                case 'J':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] =
						status_dcc;
                                        break;
                                case 'U':
                                        new_free(&channelcount_format);
                                        channelcount_format =
                convert_sub_format(get_string_var(STATUS_CHANNELCOUNT_VAR), 'U');
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                status_channelcount;
                                        break;
/****** Coded by Zakath ******/
                                case '4':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                status_packs;
                                        break;
                                case '5':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                status_dccsends;
                                        break;
                                case '6':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                status_dccgets;
                                        break;
                                case '7':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_autoget;
                                        break;
                                case '8':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_security;
                                        break;
                                case '9':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_channeltopic;
                                        break;
                                case 'D':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_frlist;
                                        break;
                                case 'E':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_nhprot;
                                        break;
                                case 'P':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_floodp;
                                        break;
                                case 'L':
                                        strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                        status_func[k][(*cp)++] =
                                                 status_ctcpcloak;
                                        break;
                                case '!': /* %!## Status_user format by Zakath */
                                          /* Thx to Sheik & Flier for help */
                                    switch (*(ptr++)) {
                                        case '0' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user00;
                                            break;
                                        case '1' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user01;
                                            break;
                                        case '2' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user02;
                                            break;
                                        case '3' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user03;
                                            break;
                                        case '4' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user04;
                                            break;
                                        case '5' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user05;
                                            break;
                                        case '6' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user06;
                                            break;
                                        case '7' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user07;
                                            break;
                                        case '8' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user08;
                                            break;
                                        case '9' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_user09;
                                            break;
                                        case 'S' :
                                            strmcat(buffer, "%s", BIG_BUFFER_SIZE);
                                            status_func[k][(*cp)++] =
                                                status_fullserver;
                                            break;
                                        default:
                                            ptr++;
                                            break;
                                    }
                                    break;
                                default:
                                    ptr++;
                                    break;
/****************************************************************************/
		/* no default..?? - phone, jan 1993 */
		/* empty is a good default -lynx, mar 93 */
				}
			}
			else
				ptr++;
		}
		else
			strmcat(buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	/* this frees the old str first */
	malloc_strcpy(&malloc_ptr, buffer);
	return (malloc_ptr);
}

void
build_status(format)
	char	*format;
{
	int	i, k;

/**************************** PATCHED by Flier ******************************/
	/*for (k = 0; k < 3; k++)*/
	for (k = 0; k < 4; k++)
/****************************************************************************/
	{
		new_free(&status_format[k]);
		func_cnt[k] = 0;
		switch (k)
		{
		case 0 : 
			format = get_string_var(STATUS_FORMAT_VAR);
			break;
			
		case 1 : 
			format = get_string_var(STATUS_FORMAT1_VAR);
			break;
			
		case 2 : 
			format = get_string_var(STATUS_FORMAT2_VAR);
			break;
/**************************** PATCHED by Flier ******************************/
		case 3 :
			format = get_string_var(STATUS_FORMAT3_VAR);
			break;
/****************************************************************************/
		}
		if (format != NULL)	/* convert_format mallocs for us */
			status_format[k] = convert_format(format, k);
		for (i = func_cnt[k]; i < MAX_FUNCTIONS; i++)
			status_func[k][i] = status_null_function;
	}
	update_all_status();
}

/**************************** PATCHED by Flier ******************************/
#ifdef CELE
void
Cquick_status(format,qstat)
char *format;
int qstat;
{
    int	i,k;

    for (k=0;k<4;k++) {
        new_free(&status_format[k]);
        func_cnt[k]=0;
        if (qstat) { /* Turn QuickStat ON */
            switch (k) {
                case 0:
                    format=get_string_var(STATUS_FORMAT_VAR);
                    break;
                case 1:
                    format=get_string_var(STATUS_FORMAT3_VAR);
                    break;
                case 2:
                    format=get_string_var(STATUS_FORMAT1_VAR);
                    break;
                case 3:
                    format=get_string_var(STATUS_FORMAT2_VAR);
                    break;
            }
        } else {  /* Turn QuickStat OFF */
            switch (k) {
                case 0:
                    format=get_string_var(STATUS_FORMAT_VAR);
                    break;
                case 1:
                    format=get_string_var(STATUS_FORMAT2_VAR);
                    break;
                case 2:
                    format=get_string_var(STATUS_FORMAT3_VAR);
                    break;
                case 3:
                    format=get_string_var(STATUS_FORMAT1_VAR);
                    break;
            }
        }
        if (format) /* convert_format mallocs for us */
            status_format[k]=convert_format(format, k);
        for (i=func_cnt[k];i<MAX_FUNCTIONS;i++)
            status_func[k][i]=status_null_function;
    }
}
#endif /*CELE*/
/****************************************************************************/

void
make_status(window)
	Window	*window;
{
	int	i,
	len,
	k,
	RJustifyPos = -1,
	RealPosition;
	u_char	buffer[BIG_BUFFER_SIZE + 1];
	char	*func_value[MAX_FUNCTIONS];
/**************************** PATCHED by Flier ******************************/
        int     change=1;
/****************************************************************************/

/**************************** PATCHED by Flier ******************************/
        /*for (k = 0 ; k < ((window->double_status) ? 2 : 1); k++)*/
        for (k = 0 ; k < (window->double_status)+1; k++)
/****************************************************************************/
	{
		int l;

		if (k)
/**************************** PATCHED by Flier ******************************/
			/*l = 2;*/
                        l = k+1;
/****************************************************************************/
		else if (window->double_status)
			l = 1;
		else
			l = 0;

		if (!dumb && status_format[l])
		{
			for (i = 0; i < MAX_FUNCTIONS; i++)
				func_value[i] = (status_func[l][i]) (window);
/**************************** PATCHED by Flier ******************************/
			/*buffer[0] = REV_TOG;*/
                        buffer[0]=get_int_var(STATUS_REVERSE_VAR) ? REV_TOG : ALL_OFF;
/****************************************************************************/
			sprintf((char *) buffer+1, status_format[l],
				func_value[0], func_value[1], func_value[2],
				func_value[3], func_value[4], func_value[5],
				func_value[6], func_value[7], func_value[8],
				func_value[9], func_value[10], func_value[11],
				func_value[12], func_value[13], func_value[14],
				func_value[15], func_value[16], func_value[17],
				func_value[18], func_value[19], func_value[20],
				func_value[21], func_value[22], func_value[23],
				func_value[24], func_value[25], func_value[26],
				func_value[27], func_value[28], func_value[29],
				func_value[30], func_value[31],func_value[32]);
			for (i = 0; i < MAX_FUNCTIONS; i++)
				new_free(&(func_value[i]));
			
			/*  Patched 26-Mar-93 by Aiken
			 *  make_window now right-justifies everything 
			 *  after a %>
			 *  it's also more efficient.
			 */
			
			RealPosition = 0;
			RJustifyPos = -1;
			for (i = 0; buffer[i]; i++)
				/* formfeed is a marker for left/right border*/
				if (buffer[i] == '\f')
				{
					RJustifyPos = i;
				}
/**************************** PATCHED by Flier ******************************/
				/*else if (buffer[i] != REV_TOG
					 && buffer[i] != UND_TOG 
					 && buffer[i] != ALL_OFF 
					 && buffer[i] != BOLD_TOG)*/
#ifdef WANTANSI
                                else if (buffer[i] != REV_TOG && buffer[i] != UND_TOG &&
                                         buffer[i] != ALL_OFF && buffer[i] != BOLD_TOG
                                         && !vt100Decode(buffer[i]))
#else
                                else if (buffer[i] != REV_TOG && buffer[i] != UND_TOG &&
                                         buffer[i] != ALL_OFF && buffer[i] != BOLD_TOG)
#endif
/****************************************************************************/
				{
/**************************** PATCHED by Flier ******************************/
					/*if (RealPosition == CO)*/
					if (RealPosition == CO-1)
/****************************************************************************/
					{
						buffer[i] = '\0';
						break;
					}
					RealPosition++;
				}
			
			/* note that i points to the nul, 
			   RealPosition is vis.chars */
			
			if (RJustifyPos == -1)
			{
				RJustifyPos = i;
			}
			else
			{
				/* get rid of the marker */
				strcpy((char *) &buffer[RJustifyPos], 
					(char *) &buffer[RJustifyPos+1]);
				i--;
			}
			
			if (get_int_var(FULL_STATUS_LINE_VAR))
			{
				int	diff;
				u_char	c;
				
				if (RJustifyPos == 0)
					c = ' ';
				else
					c = buffer[RJustifyPos - 1];
				
				diff = CO - RealPosition;
				
				for ( ; i >= RJustifyPos; i--)
					buffer[i + diff] = buffer[i];
				
				for (i++ ; diff > 0 ; diff--, i++)
					buffer[i] = c;
			}

/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                        strcat(buffer,"[0m");
#endif
/****************************************************************************/

			len = strlen((char *) buffer);
			buffer[len] = ALL_OFF;
			buffer[len+1] =  '\0';

			status_make_printable(buffer, len);

	      /*
	       * Thanks to Max Bell (mbell@cie.uoregon.edu) for info about TVI
	       * terminals and the sg terminal capability 
	       */
			RealPosition = 0;
/**************************** PATCHED by Flier ******************************/
			/*if (window->status_line[k] && (SG == -1))
			{
				for (i = 0; buffer[i] && window->status_line[k][i]; i++)
				{
					if ((char) buffer[i] != window->status_line[k][i])
						break;
					if (buffer[i] != REV_TOG 
					    && buffer[i] != UND_TOG
					    && buffer[i] != ALL_OFF
					    && buffer[i] != BOLD_TOG)
						RealPosition++;
				}
			}
			else
				i = 0;
			
			if ((len = strlen((char *) buffer + i)) 
			     || buffer[i] || window->status_line[k] 
			     || window->status_line[k][i])*/
#ifdef WANTANSI
                        i=0;
                        if (window->status_line[k])
                            change=strcmp(buffer,window->status_line[k])!=0?1:0;
#else
                        if (window->status_line[k] && (SG == -1))
                        {
                            for (i = 0; buffer[i] && window->status_line[k][i]; i++)
                            {
                                if (buffer[i] != window->status_line[k][i])
                                    break;
                                if (buffer[i] != REV_TOG && buffer[i] != UND_TOG &&
                                    buffer[i] != ALL_OFF && buffer[i] != BOLD_TOG)
                                    RealPosition++;
                            }
                        }
                        else
                            i=0;
#endif
                        if (change && ((len = strlen(buffer + i)) || buffer[i] ||
                                          window->status_line[k] || window->status_line[k][i]))
/****************************************************************************/
			{
				Screen *old_current_screen;
				
				old_current_screen = current_screen;
				set_current_screen(window->screen);
				term_move_cursor(RealPosition, window->bottom
						 + k);
/*
				term_move_cursor(RealPosition, window->bottom
						 + k - window->double_status);
*/
				output_line(buffer, NULL, i);
				cursor_in_display();
				if (term_clear_to_eol())
					term_space_erase(len);
				malloc_strcpy(&window->status_line[k],
					      (char *) buffer);
				set_current_screen(old_current_screen);
			}
		}
	}
	cursor_to_input();
}

static	char	*
status_nickname(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (!window->current_channel) &&
	    (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
		malloc_strcpy(&ptr, get_server_nickname(window->server));
	return (ptr);
}

static	char	*
status_server(window)
	Window	*window;
{
	char	*ptr = NULL,
		*rest,
		*name;

	if (connected_to_server != 1)
	{
		if (window->server != -1)
		{
			if (server_format)
			{
				name = get_server_name(window->server);
				rest = (char *) index(name, '.');
				if (rest != NULL && my_strnicmp(name, "irc", 3) != 0)
				{
					if (is_number(name))
						sprintf(buffer, server_format, name);
					else
					{
						*rest = '\0';
						sprintf(buffer, server_format, name);
						*rest = '.';
					}
				}
				else
					sprintf(buffer, server_format, name);
			}
			else
				*buffer = '\0';
		}
		else
			strcpy(buffer, " No Server");
	}
	else
		*buffer = '\0';
	malloc_strcpy(&ptr, buffer);
	return (ptr);
}

static	char	*
status_group(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if (window->server_group && group_format)
	{
		sprintf(buffer, group_format, find_server_group_name(window->server_group));
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_query_nick(window)
	Window	*window;
{
	char	*ptr = (char *) 0;
/**************************** PATCHED by Flier ******************************/
        int     buflen=get_int_var(CHANNEL_NAME_WIDTH_VAR);
        char    tmpbuf[mybufsize/4+1];
/****************************************************************************/

	if (window->query_nick && query_format)
	{
/**************************** PATCHED by Flier ******************************/
		/*sprintf(buffer, query_format, window->query_nick);*/
                if (buflen>mybufsize/4) buflen=mybufsize/4;
		strmcpy(tmpbuf,window->query_nick,buflen);
		sprintf(buffer,query_format,tmpbuf);
/****************************************************************************/
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_right_justify(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	malloc_strcpy(&ptr, "\f");
	return (ptr);
}

static	char	*
status_notify_windows(window)
	Window	*window;
{
	char	refnum[10];
	int	doneone = 0;
	char	*ptr = (char *) 0;
	int	flag = 1;
	char	buf2[81];

	if (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    window == window->screen->current_window)
	{
		*buf2='\0';
		while ((window = traverse_all_windows(&flag)) != NULL)
		{
			if (window->miscflags & WINDOW_NOTIFIED)
			{
				if (!doneone)
				{
					doneone++;
					sprintf(refnum, "%d", window->refnum);
				}
				else
					sprintf(refnum, ",%d", window->refnum);
				strmcat(buf2, refnum, 81);
			}
		}
	}
	if (doneone && notify_format)
	{
		sprintf(buffer, notify_format, buf2);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return ptr;
}

static	char	*
status_clock(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if ((get_int_var(CLOCK_VAR) && clock_format)  &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
	{
		sprintf(buffer, clock_format, update_clock(GET_TIME));
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_mode(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*mode;

	if (window->current_channel && chan_is_connected(window->current_channel, window->server))
	{
		mode = get_channel_mode(window->current_channel,window->server);
		if (mode && *mode && mode_format)
		{
			sprintf(buffer, mode_format, mode);
			malloc_strcpy(&ptr, buffer);
			return (ptr);
		}
	}
	malloc_strcpy(&ptr, empty_string);
	return (ptr);
}


static	char	*
status_umode(window)
	Window	*window;
{
	char	*ptr = (char *) 0;
/**************************** PATCHED by Flier ******************************/
        /*char	localbuf[10];*/
        char	localbuf[32];
        int     i;
/****************************************************************************/
	char	*c;

	if (connected_to_server == 0)
		malloc_strcpy(&ptr, empty_string);
	else if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
	{
		c = localbuf;
/**************************** PATCHED by Flier ******************************/
                for (i=0;i<26;i++)
                    if (get_server_umode_flag(window->server,'a'+i)) *c++='a'+i;
/****************************************************************************/
		*c++ = '\0';
		if (*localbuf != '\0' && umode_format)
		{
			sprintf(buffer, umode_format, localbuf);
			malloc_strcpy(&ptr, buffer);
		}
		else
			malloc_strcpy(&ptr, empty_string);
	}
	return (ptr);
}

static	char	*
status_chanop(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*text;
/**************************** PATCHED by Flier ******************************/
        ChannelList *chan;
/****************************************************************************/

	if (window->current_channel &&
	    chan_is_connected(window->current_channel, window->server) &&
	    get_channel_oper(window->current_channel, window->server) &&
	    (text = get_string_var(STATUS_CHANOP_VAR)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
/**************************** PATCHED by Flier ******************************/
	if (window->current_channel &&
            chan_is_connected(window->current_channel, window->server)) {
            chan=lookup_channel(window->current_channel,window->server,0);
            if (chan && ((chan->status)&CHAN_VOICE) && !((chan->status)&CHAN_CHOP)) {
                text=get_string_var(STATUS_CHANOP_VAR);
		malloc_strcpy(&ptr,text);
                if ((text=index(ptr,'@'))) *text='+';
                else malloc_strcpy(&ptr,empty_string);
            }
        }
/****************************************************************************/
	return (ptr);
}


static	char	*
status_hold_lines(window)
	Window	*window;
{
	char	*ptr = (char *) 0;
	int	num;
	char	localbuf[40];

	num = window->held_lines - window->held_lines%10;
	if (num)
	{
		sprintf(localbuf, "%d", num);
		sprintf(buffer, hold_lines_format, localbuf);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_channel(window)
	Window	*window;
{
	int	num;
	char	*s, *ptr,
		channel[IRCD_BUFFER_SIZE + 1];

	s = window->current_channel;
	if (s && chan_is_connected(s, window->server))
	{
		if (get_int_var(HIDE_PRIVATE_CHANNELS_VAR) &&
		    is_channel_mode(window->current_channel,
				MODE_PRIVATE | MODE_SECRET,
				window->server))
			ptr = "*private*";
		else
			ptr = window->current_channel;
		strmcpy(channel, ptr, IRCD_BUFFER_SIZE);
		if ((num = get_int_var(CHANNEL_NAME_WIDTH_VAR)) &&
		    ((int) strlen(channel) > num))
			channel[num] = (char) 0;
		/* num = atoi(channel); */
		ptr = (char *) 0;
		sprintf(buffer, channel_format, channel);
		malloc_strcpy(&ptr, buffer);
	}
	else
	{
		ptr = (char *) 0;
		malloc_strcpy(&ptr, empty_string);
	}
	return (ptr);
}

static	char	*
status_mail(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*number;

	if ((get_int_var(MAIL_VAR) && (number = check_mail()) && mail_format) &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
	{
		sprintf(buffer, mail_format, number);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_insert_mode(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	text = empty_string;
	if (get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
	{
		if ((text = get_string_var(STATUS_INSERT_VAR)) == (char *) 0)
			text = empty_string;
	}
	malloc_strcpy(&ptr, text);
	return (ptr);
}

static	char	*
status_overwrite_mode(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	text = empty_string;
	if (!get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
	{
	    if ((text = get_string_var(STATUS_OVERWRITE_VAR)) == (char *) 0)
		text = empty_string;
	}
	malloc_strcpy(&ptr, text);
	return (ptr);
}

static	char	*
status_away(window)
	Window	*window;
{
/**************************** PATCHED by Flier ******************************/
	/*char	*ptr = (char *) 0,
	*text;*/
        char	*ptr = (char *) 0;
        char    buf[10];
/****************************************************************************/

	if (connected_to_server == 0)
		malloc_strcpy(&ptr, empty_string);
	else if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
	{
/**************************** PATCHED by Flier ******************************/
		/*if (server_list[window->server].away &&
				(text = get_string_var(STATUS_AWAY_VAR)))
			malloc_strcpy(&ptr, text);
		else
			malloc_strcpy(&ptr, empty_string);*/
                if (server_list[window->server].away && away_format) {
                    sprintf(buf,"%d",AwayMsgNum);
                    sprintf(buffer,away_format,buf);
                    malloc_strcpy(&ptr, buffer);
                }
		else malloc_strcpy(&ptr, empty_string);
/****************************************************************************/
	}
	return (ptr);
}

/**************************** PATCHED by Flier ******************************/
/*static	char	*
status_user0(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if ((text = get_string_var(STATUS_USER_VAR)) &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static  char    *
status_user1(window)
	Window  *window;
{
        char    *ptr = (char *) 0,
        *text;

        if ((text = get_string_var(STATUS_USER1_VAR)) &&
            (get_int_var(SHOW_STATUS_ALL_VAR) ||
            (window == window->screen->current_window)))
                malloc_strcpy(&ptr, text);
        else
                malloc_strcpy(&ptr, empty_string);
        return (ptr);
}

static  char    *
status_user2(window)
	Window  *window;
{
        char    *ptr = (char *) 0,
        *text;

        if ((text = get_string_var(STATUS_USER2_VAR)) &&
            (get_int_var(SHOW_STATUS_ALL_VAR) ||
            (window == window->screen->current_window)))
                malloc_strcpy(&ptr, text);
        else
                malloc_strcpy(&ptr, empty_string);
        return (ptr);
}

static  char    *
status_user3(window)
	Window  *window;
{
        char    *ptr = (char *) 0,
        *text;

        if ((text = get_string_var(STATUS_USER3_VAR)) &&
            (get_int_var(SHOW_STATUS_ALL_VAR) ||
            (window == window->screen->current_window)))
                malloc_strcpy(&ptr, text);
        else
                malloc_strcpy(&ptr, empty_string);
        return (ptr);
}*/
/****************************************************************************/

static	char	*
status_hold(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if (window->held && (text = get_string_var(STATUS_HOLD_VAR)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_oper(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if (!connected_to_server)
		malloc_strcpy(&ptr, empty_string);
	else if (get_server_operator(window->server) &&
			(text = get_string_var(STATUS_OPER_VAR)) &&
			(get_int_var(SHOW_STATUS_ALL_VAR) ||
			connected_to_server != 1 || 
			(window->screen->current_window == window)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_window(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if ((text = get_string_var(STATUS_WINDOW_VAR)) &&
	    (number_of_windows() > 1) && (window->screen->current_window == window))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_refnum(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if (window->name)
		malloc_strcpy(&ptr, window->name);
	else
	{
		sprintf(buffer, "%u", window->refnum);
		malloc_strcpy(&ptr, buffer);
	}
	return (ptr);
}

static	char	*
status_version(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
	{
		malloc_strcpy(&ptr, irc_version);
	}
	return (ptr);
}


static	char	*
status_null_function(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

/*
 * pass an already allocated char array with n bits, and this
 * gets rid of nasty unprintables.
 */
static	void
status_make_printable(str, n)
	unsigned char	*str;
	int n;
{
	unsigned char	*s;
	int	pos;
	char	buffer[BIG_BUFFER_SIZE];

	if (!str || !*str)
		return;

	bzero(buffer, BIG_BUFFER_SIZE);
	for (pos = 0, s = str; s && pos < BIG_BUFFER_SIZE && pos < n; s++)
	{
		if (translation)
			*s = transToClient[*s];
/**************************** PATCHED by Flier ******************************/
#ifdef WANTANSI
                if (vt100Decode(*s)) {
                    buffer[pos++]=*s;
                    continue;
                }
#endif
/****************************************************************************/
		if (*s < 32)
		{
			switch(*s)
			{
			case UND_TOG:
			case ALL_OFF:
			case REV_TOG:
			case BOLD_TOG:
				buffer[pos++] = *s;
				break;
			default:
				buffer[pos++] = REV_TOG;
				buffer[pos++] = (*s & 0x7f) | 0x40;
				buffer[pos++] = REV_TOG;
				break;
			}
		}
		else if ((u_char) 0x7f == *s)
		{
			buffer[pos++] = REV_TOG;
			buffer[pos++] = '?';
			buffer[pos++] = REV_TOG;
		}
		else
			buffer[pos++] = *s;
	}
	buffer[pos] = '\0';
	strncpy((char *) str, buffer, pos);
}

/**************************** PATCHED by Flier ******************************/
/* by Zakath */
static char *status_user00(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user01(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER1_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user02(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER2_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user03(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER3_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user04(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER4_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user05(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER5_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user06(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER6_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user07(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER7_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user08(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER8_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_user09(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *text;

    if ((text=get_string_var(STATUS_USER9_VAR)) &&
        (get_int_var(SHOW_STATUS_ALL_VAR) ||
         (window==window->screen->current_window)))
        malloc_strcpy(&ptr,text);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_lag(window)
Window	*window;
{
    char *ptr=(char *) 0;
#if defined(HAVETIMEOFDAY) && defined(CELE)
    char lagbuf[8];
#endif

    if (get_int_var(SHOW_STATUS_ALL_VAR) || current_screen->current_window==window) {
#if defined(HAVETIMEOFDAY) && defined(CELE)
        sprintf(lagbuf,"%06d",LagTimer.tv_usec);
        lagbuf[3]='\0';
        sprintf(buffer,"%d.%s",LagTimer.tv_sec,lagbuf);
#else
        sprintf(buffer,"%02d",LagTimer);
#endif
        malloc_strcpy(&ptr,buffer);
    }
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_lastjoin(window)
Window	*window;
{
    char *ptr=(char *) 0;

    if (LastJoin &&
        (get_int_var(SHOW_STATUS_ALL_VAR) || current_screen->current_window==window))
        malloc_strcpy(&ptr,LastJoin);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_dcc(window)
Window	*window;
{
    char *ptr=(char *) 0;

    if (CurrentDCC &&
        (get_int_var(SHOW_STATUS_ALL_VAR) || current_screen->current_window==window))
        malloc_strcpy(&ptr,CurrentDCC);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_channelcount(window)
Window	*window;
{
    int	ops=0;
    int nonops=0;
    int tmpvar=0;
    char *ptr=(char *) 0;
    char *tmpstr1=(char *) 0;
    char *tmpstr2=(char *) 0;
    char *channel=window->current_channel;
    char tmpbuf[mybufsize/4];
    NickList *tmp;
    ChannelList	*chan;

    if (!channel || !chan_is_connected(channel, window->server) ||
        !channelcount_format || !get_string_var(STATUS_CHANNELCOUNT_VAR) ||
        (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window))
        malloc_strcpy(&ptr,empty_string);
    else if ((chan=lookup_channel(channel,curr_scr_win->server,0))==NULL)
        malloc_strcpy(&ptr,empty_string);
    else {
        for (tmp=chan->nicks;tmp;tmp=tmp->next)
            if (tmp->chanop) ops++;
            else nonops++;
        for (tmpstr1=channelcount_format,tmpstr2=tmpbuf;*tmpstr1;tmpstr1++)
            if (*tmpstr1=='%') {
                tmpstr1++;
                if (*tmpstr1=='%') tmpstr1++;
                if (*tmpstr1=='o' || *tmpstr1=='O') tmpvar=ops;
                else if (*tmpstr1=='n' || *tmpstr1=='N') tmpvar=nonops;
                else if (*tmpstr1=='t' || *tmpstr1=='T') tmpvar=ops+nonops;
                else tmpvar=-1;
                if (tmpvar>=0) {
                    sprintf(buffer,"%d",tmpvar);
                    *tmpstr2='\0';
                    strcat(tmpstr2,buffer);
                    tmpstr2+=strlen(buffer);
                }
                else *tmpstr2++=*tmpstr1;
            }
            else *tmpstr2++=*tmpstr1;
        *tmpstr2='\0';
        malloc_strcpy(&ptr,tmpbuf);
    }
    return(ptr);
}

static char *status_channeltopic(window)
Window	*window;
{
    char *ptr=(char *) 0;
    char *channel=window->current_channel;
    char topicbuf[mybufsize];
    ChannelList	*chan;

    if (!channel || !chan_is_connected(channel,window->server) ||
        (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window))
        malloc_strcpy(&ptr,empty_string);
    else if ((chan=lookup_channel(channel,curr_scr_win->server,0))==NULL)
        malloc_strcpy(&ptr,empty_string);
    else {
        StripAnsi(chan->topicstr?chan->topicstr:empty_string,topicbuf,1);
        malloc_strcpy(&ptr,topicbuf);
    }
    return(ptr);
}

static char *status_fullserver(window)
Window	*window;
{
    char *ptr=(char *) 0;

    if (curr_scr_win->server!=-1 &&
        (get_int_var(SHOW_STATUS_ALL_VAR) || current_screen->current_window==window))
        malloc_strcpy(&ptr,server_list[curr_scr_win->server].itsname);
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

static char *status_uptime(window)
Window	*window;
{
    int tmpvar;
    char *ptr=(char *) 0;
    char *tmpstr1;
    char *tmpstr2;
    char tmpbuf[mybufsize/4];
    time_t timediff;

    if (get_int_var(SHOW_STATUS_ALL_VAR) || current_screen->current_window==window) {
        timediff=time((time_t *) 0)-start_time;
        for (tmpstr1=uptime_format,tmpstr2=tmpbuf;*tmpstr1;tmpstr1++) {
            if (*tmpstr1=='%') {
                tmpstr1++;
                if (*tmpstr1=='%') tmpstr1++;
                if (*tmpstr1=='d' || *tmpstr1=='D') tmpvar=timediff/86400;
                else if (*tmpstr1=='h' || *tmpstr1=='H') tmpvar=(timediff/3600)%24;
                else if (*tmpstr1=='m' || *tmpstr1=='M') tmpvar=(timediff/60)%60;
                else tmpvar=-1;
                if (tmpvar>=0) {
                    sprintf(buffer,"%02d",tmpvar);
                    *tmpstr2='\0';
                    strcat(tmpstr2,buffer);
                    tmpstr2+=strlen(buffer);
                }
                else *tmpstr2++=*tmpstr1;
            }
            else *tmpstr2++=*tmpstr1;
        }
        *tmpstr2='\0';
        malloc_strcpy(&ptr,tmpbuf);
    }
    else malloc_strcpy(&ptr,empty_string);
    return(ptr);
}

/****** Coded by Zakath ******/
static char *status_ctcpcloak(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else if (CTCPCloaking==0) malloc_strcpy(&ptr,"k");
    else if (CTCPCloaking==1) malloc_strcpy(&ptr,"K");
    else malloc_strcpy(&ptr,"H");
    return(ptr);
}

static char *status_floodp(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else if (FloodProt>1) malloc_strcpy(&ptr,"M");
    else if (FloodProt) malloc_strcpy(&ptr,"P");
    else malloc_strcpy(&ptr,"p");
    return(ptr);
}

static char *status_nhprot(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else if (NHProt) {
        if (NHDisp==0) malloc_strcpy(&ptr,"Nq");
        else if (NHDisp==1) malloc_strcpy(&ptr,"Nm");
        else malloc_strcpy(&ptr,"Nf");
    }
    else {
        if (NHDisp==0) malloc_strcpy(&ptr,"nq");
        else if (NHDisp==1) malloc_strcpy(&ptr,"nm");
        else malloc_strcpy(&ptr,"nf");
    }
    return(ptr);
}

static char *status_frlist(window)
Window *window;
{
    char *ptr=(char *) 0;
    char *channel;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else if (FriendList && (channel=get_channel_by_refnum(0)) &&
            CheckChannel(FriendListChannels,channel)) malloc_strcpy(&ptr,"F");
    else malloc_strcpy(&ptr,"f");
    return(ptr);
}

static char *status_packs(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else {
        sprintf(buffer,"%d",CdccPackNum);
        malloc_strcpy(&ptr,buffer);
    }
    return(ptr);
}

static char *status_dccsends(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else {
        sprintf(buffer,"%d",CdccSendNum);
        malloc_strcpy(&ptr,buffer);
    }
    return(ptr);
}

static char *status_dccgets(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else {
        sprintf(buffer,"%d",CdccRecvNum);
        malloc_strcpy(&ptr,buffer);
    }
    return(ptr);
}

static char *status_autoget(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else if (AutoGet) malloc_strcpy(&ptr,"A");
    else malloc_strcpy(&ptr,"a");
    return(ptr);
}

static char *status_security(window)
Window *window;
{
    char *ptr=(char *) 0;

    if (!get_int_var(SHOW_STATUS_ALL_VAR) && current_screen->current_window!=window)
        malloc_strcpy(&ptr,empty_string);
    else if (Security) malloc_strcpy(&ptr,"S");
    else malloc_strcpy(&ptr,"s");
    return(ptr);
}
/*****************************/
/****************************************************************************/
