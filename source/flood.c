/*
 * flood.c: handle channel flooding. 
 *
 * This attempts to give you some protection from flooding.  Basically, it keeps
 * track of how far apart (timewise) messages come in from different people.
 * If a single nickname sends more than 3 messages in a row in under a
 * second, this is considered flooding.  It then activates the ON FLOOD with
 * the nickname and type (appropriate for use with IGNORE). 
 *
 * Thanks to Tomi Ollila <f36664r@puukko.hut.fi> for this one. 
 *
 * $Id: flood.c,v 1.5 1999-10-03 09:24:25 f Exp $
 */

#include "irc.h"

#include "hook.h"
#include "ircaux.h"
#include "ignore.h"
#include "flood.h"
#include "vars.h"
#include "output.h"

/**************************** PATCHED by Flier ******************************/
#include "myvars.h"

extern void HandleFlood _((char *, char *, char *));
/****************************************************************************/

static	char	*ignore_types[NUMBER_OF_FLOODS] =
{
	"MSG",
	"PUBLIC",
	"NOTICE",
	"WALL",
/**************************** PATCHED by Flier ******************************/
        /*"WALLOP"*/
        "WALLOP",
        "CTCP"
/****************************************************************************/
};

typedef struct flood_stru
{
 	char	*nick;
	int	type;
	char	flood;
	long	cnt;
	time_t	start;
}	Flooding;

static	Flooding *flood = (Flooding *) 0;

/*
 * check_flooding: This checks for message flooding of the type specified for
 * the given nickname.  This is described above.  This will return 0 if no
 * flooding took place, or flooding is not being monitored from a certain
 * person.  It will return 1 if flooding is being check for someone and an ON
 * FLOOD is activated. 
 *
 * NOTE:  flood's are never freed.
 */
int
check_flooding(nick, type, line)
	char	*nick;
	int	type;
	char	*line;
{
	static	int	users = 0,
			pos = 0;
	int	i;
	time_t	flood_time,
	diff;
	Flooding *tmp;
/**************************** PATCHED by Flier ******************************/
        float   rate,after,current=0.0;
/****************************************************************************/

	if (users != get_int_var(FLOOD_USERS_VAR))
	{
		if ((users = get_int_var(FLOOD_USERS_VAR)) == 0)
			return(1);
		if (flood)
			flood = (Flooding *) new_realloc((char *) flood, sizeof(Flooding) * users);
		else
			flood = (Flooding *) new_malloc(sizeof(Flooding) *
				users);
		for (i = 0; i < users; i++)
		{
 			flood[i].nick = 0;
			flood[i].cnt = 0;
			flood[i].flood = 0;
		}
	}
	if (users == 0)
		return (1);
        for (i = 0; i < users; i++)
	{
 		if (flood[i].nick && *(flood[i].nick))
		{
			if ((my_stricmp(nick, flood[i].nick) == 0) &&
					(type == flood[i].type))
				break;
		}
	}
	flood_time = time(0);
        if (i == users)
	{
		tmp = &(flood[pos]);
		pos = (pos + 1) % users;
 		tmp->nick = 0;
 		malloc_strcpy(&tmp->nick, nick);
		tmp->type = type;
		tmp->cnt = 1;
		tmp->start = flood_time;
		tmp->flood = 0;
		return (1);
	}
	else
		tmp = &(flood[i]);
	tmp->cnt++;
	diff = flood_time - tmp->start;
/**************************** PATCHED by Flier ******************************/
	/*if (tmp->cnt > get_int_var(FLOOD_AFTER_VAR))*/
        after=(float) get_int_var(FLOOD_AFTER_VAR);
        rate=(float) get_int_var(FLOOD_RATE_VAR);
        if (tmp->cnt>after)
/****************************************************************************/
	{
/**************************** PATCHED by Flier ******************************/
		/*if ((diff == 0) || (tmp->cnt / diff) >
				get_int_var(FLOOD_RATE_VAR))*/
                if (diff!=0) current=(float) tmp->cnt/(float) diff;
                else current=(float) tmp->cnt;
                if (current>=(after/rate))
/****************************************************************************/
		{
			if (tmp->flood == 0)
                        {
				if (get_int_var(FLOOD_WARNING_VAR))
/**************************** PATCHED by Flier ******************************/
					/*say("%s flooding detected from %s",
						ignore_types[type], nick);*/
                                   HandleFlood(nick,FromUserHost,ignore_types[type]);
/****************************************************************************/
				tmp->flood = 1;
			}
			return (do_hook(FLOOD_LIST, "%s %s %s", nick,
				ignore_types[type],line));
		}
		else
		{
			tmp->flood = 0;
			tmp->cnt = 1;
			tmp->start = flood_time;
		}
	}
	return (1);
}

/**************************** PATCHED by Flier ******************************/
/* Clean up all allocated memory */
void CleanUpFlood() {
    int i,users;

    users=get_int_var(FLOOD_USERS_VAR);
    for (i=0;i<users;i++) new_free(&(flood[i].nick));
    new_free(&flood);
}
/****************************************************************************/
