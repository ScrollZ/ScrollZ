/*
 * Written By Douglas A. Lewis <dalewis@cs.Buffalo.EDU>
 *
 * This file is in the public domain.
 *
 * $Id: reg.c,v 1.3 1999-02-15 21:20:10 f Exp $
 */

#include "irc.h"

#include "ircaux.h"

static	int	_wild_match _((u_char *, u_char *));

#define RETURN_FALSE -1
#define RETURN_TRUE count

u_char lower_tab[256] = 
{
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95,
 96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
144,145,145,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

#undef tolower		/* don't want previous version. */
#define tolower(x) lower_tab[x]

/*
 * So I don't make the same mistake twice: We don't need to check for '\\'
 * in the input (string to check) because it doesn't have a special meaning
 * in that context.
 *
 * We don't need to check for '\\' when we come accross a * or % .. The rest
 * of the routine will do that for us.
 */

static	int
_wild_match(mask, string)
	u_char	*mask,
		*string;
{
	u_char	*m = mask,
		*n = string,
		*ma = NULL,
		*na = NULL,
		*mp = NULL,
		*np = NULL;
	int	just = 0,
 		pcount = 0,
 		acount = 0,
		count = 0;

	for (;;)
	{
		if (*m == '*')
		{
			ma = ++m;
			na = n;
			just = 1;
			mp = NULL;
			acount = count;
		}
		else if (*m == '%')
		{
			mp = ++m;
			np = n;
			pcount = count;
		}
		else if (*m == '?')
		{
			m++;
			if (!*n++)
				return RETURN_FALSE;
		}
		else
		{
			if (*m == '\\')
			{
				m++;
				/* Quoting "nothing" is a bad thing */
				if (!*m)
					return RETURN_FALSE;
			}
			if (!*m)
			{
				/*
				 * If we are out of both strings or we just
				 * saw a wildcard, then we can say we have a
				 * match
				 */
				if (!*n)
					return RETURN_TRUE;
				if (just)
					return RETURN_TRUE;
				just = 0;
				goto not_matched;
			}
			/*
			 * We could check for *n == NULL at this point, but
			 * since it's more common to have a character there,
			 * check to see if they match first (m and n) and
			 * then if they don't match, THEN we can check for
			 * the NULL of n
			 */
			just = 0;
			if (tolower(*m) == tolower(*n))
			{
				m++;
				if (*n == ' ')
					mp = NULL;
				count++;
				n++;
			}
			else
			{

	not_matched:

				/*
				 * If there are no more characters in the
				 * string, but we still need to find another
				 * character (*m != NULL), then it will be
				 * impossible to match it
				 */
				if (!*n)
					return RETURN_FALSE;
				if (mp)
				{
					m = mp;
					if (*np == ' ')
					{
						mp = NULL;
						goto check_percent;
					}
					n = ++np;
					count = pcount;
				}
				else
	check_percent:

				if (ma)
				{
					m = ma;
					n = ++na;
					count = acount;
				}
				else
					return RETURN_FALSE;
			}
		}
	}
}

#if 0
int 
match (char *pattern, char *string)
{
/* -1 on false >= 0 on true */
  return ((_wild_match(pattern, string)>=0)?1:0);
}
#endif

int 
wild_match (char *pattern, char *str)
{
	/* assuming a -1 return of false */
	return _wild_match((u_char *) pattern, (u_char *) str) + 1;
}
