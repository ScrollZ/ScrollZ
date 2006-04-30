/*
 * translat.c:  Stuff for handling different encodings
 * and a digraph entry facility. Support an international IRC!
 *
 * <subliminal message> you start using utf-8 and
 * discard all legacy encodings</subliminal message>
 *
 * Joel Yliluoma.
 *
 * $Id: translat.c,v 1.7 2006-04-30 14:15:43 f Exp $
 *
 */

#include "irc.h"

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif /* HAVE_ICONV_H */

#include "vars.h"
#include "translat.h"
#include "ircaux.h"
#include "window.h"
#include "screen.h"
#include "output.h"

static	char	my_getarg(char **);

/* Globals */
char	digraph_changed = 0;

#ifdef HAVE_ICONV_OPEN
char  *irc_encoding     = NULL;
char  *display_encoding = NULL;
char  *input_encoding   = NULL;
#endif /* HAVE_ICONV_OPEN */


/*
 * dig_table_lo[] and dig_table_hi[] contain the character pair that
 * will result in the digraph in dig_table_di[].  To avoid searching
 * both tables, I take the lower character of the pair, and only
 * search dig_table_lo[].  Thus, dig_table_lo[] must always contain
 * the lower character of the pair.
 *
 * The digraph tables are based on those in the excellent editor Elvis,
 * with some additions for those, like me, who are used to VT320 or
 * VT420 terminals.
 */

#define	DiLo(x)	x,
#define	DiHi(x)
#define	DiDi(x)

/*
 * Digraph tables.  Note that, when adding a new digraph, the character
 * of the pair with the lowest value, *must* be in the DiLo column.
 * The higher of the pair goes in DiHi, and the digraph itself in DiDi.
 */

char	dig_table_lo[DIG_TABLE_SIZE] =
{
#include "digraph.inc"
	0
};


#undef	DiLo
#undef	DiHi
#undef	DiDi
#define	DiLo(x)
#define	DiHi(x)	x,
#define	DiDi(x)

char	dig_table_hi[DIG_TABLE_SIZE] =
{
#include "digraph.inc"
	0
};


#undef	DiLo
#undef	DiHi
#undef	DiDi
#define	DiLo(x)
#define	DiHi(x)
#define	DiDi(x)	x,

char	dig_table_di[DIG_TABLE_SIZE] =
{
#include "digraph.inc"
	0
};


/*
 * enter_digraph:  The BIND function ENTER_DIGRAPH.
 */
void
enter_digraph(key, str)
	u_int	key;
	char	*str;
{

	current_screen->digraph_hit = 1;  /* Just stuff away first character. */
}

/*
 * get_digraph:  Called by edit_char() when a digraph entry is activated.
 * Looks up a digraph given u_char c1 and the global u_char
 * current_screen->digraph_hit.
 */
char
get_digraph(ic1)
	u_int	ic1;
{
	int	i = 0;
	char	c,
		c2 = current_screen->digraph_first,
		c1 = (u_char)ic1;

	current_screen->digraph_hit = 0;
	if (c1 > c2)	/* Make sure we have the lowest one in c1. */
		c = c1,	c1 = c2, c2 = c;
	while (dig_table_lo[i])
	{	/* Find digraph and return it. */
		if ((dig_table_lo[i] == c1) && (dig_table_hi[i] == c2))
			return dig_table_di[i];
		i++;
	}
	return 0;		/* Failed lookup. */
}


/*
 * digraph:  The /DIGRAPH command with facilities.
 * This routine is *NOT* finished yet.
 */

void
digraph(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	char	c1,
		c2 = '\0',
		c3 = '\0';
	int	i;
	size_t	len;

	if ((arg = next_arg(args, &args)) && (*arg == '-'))
	{
		char	*cmd = (char *) 0;

		arg++;
		if ((len = strlen(arg)) == 0)
		{
			say("Unknown or missing flag.");
			return;
		}
		malloc_strcpy(&cmd, arg);
		lower(cmd);
		if (strncmp(cmd, "add", len) == 0)
		{
			/*
			 * Add a digraph to the table.
			 * I *know*.  This *is* a kludge.
			 */
			if ((i = strlen(dig_table_lo)) ==
					DIG_TABLE_SIZE - 1)
				say("Sorry, digraph table full.");
			else
			{
				while ((c1 = my_getarg(&args)) &&
				    (c2 = my_getarg(&args)) &&
				    (c3 = my_getarg(&args)))
				{
					/* Pass c1 to get_digraph() */
					current_screen->digraph_first = c1;	    
					if (get_digraph(c2) == 0)
					{
						dig_table_di[i] = c3;
						/* Make sure c1 <= c2 */
						if (c1 > c2)
						    c3 = c1, c1 = c2, c2 = c3;
						dig_table_lo[i] = c1;
						dig_table_hi[i] = c2;
						i++;
						dig_table_lo[i] =
							dig_table_hi[i] =
					 		dig_table_di[i] =
							(u_char) 0;
						digraph_changed = 1;
						say("Digraph added to table.");
					}
					else
					{
				say("Digraph already defined in table.");
				break;
					}
				}
				if (!c2 || !c3)
					say("Unknown or missing argument.");
			}

		}
		else if (strncmp(cmd, "remove", len) == 0)
		{

			/* Remove a digraph from the table. */
			if ((i = strlen(dig_table_lo)) == 0)
				say("Digraph table is already empty.");
			else
			{
				if ((c1 = my_getarg(&args)) &&
						(c2 = my_getarg(&args)))
				{
					i = 0;
					if (c1 > c2)
						c3 = c1, c1 = c2, c2 = c3;
					while (dig_table_lo[i])
					{
						if ((dig_table_lo[i] == c1) &&
						    (dig_table_hi[i] == c2))
					/*
					 * FIXME: strcpy() is not guaranteed for
					 * overlapping copying, but this one
					 * is high -> low. Ought to be fixed.
					 */
	/* re-indent this block - phone, jan 1993. */
				{
					strcpy(dig_table_lo + i, dig_table_lo + i + 1);
					strcpy(dig_table_hi + i, dig_table_hi + i + 1);
					strcpy(dig_table_di + i, dig_table_di + i + 1);
					digraph_changed = 1;
					put_it("Digraph removed from table.");
					return;
				}
	/* much better */
						i++;
					}
					say("Digraph not found.");
				}
			}
		}
		else if (strncmp(cmd, "clear", len) == 0)
		{

			/* Clear digraph table. */
			dig_table_lo[0] = dig_table_hi[0] = dig_table_di[0] =
				(u_char) 0;
			digraph_changed = 1;
			say("Digraph table cleared.");

		}
		else
			say("Unknown flag.");
	}
	else
	{

		/* Display digraph table. */
		u_char	buffer1[8];
		u_char	buffer2[192];

		say("Digraph table:");
		buffer2[0] = (u_char) 0;
		i = 0;
		while(dig_table_lo[i])
		{
			snprintf(CP(buffer1), sizeof buffer1, "%c%c %c   ", dig_table_lo[i],
			    dig_table_hi[i], dig_table_di[i]);
			strcat(buffer2, buffer1);
			if ((++i % 10) == 0)
			{
				put_it(CP(buffer2));
				buffer2[0] = (u_char) 0;
			}
		}
		if (buffer2[0])
			put_it(CP(buffer2));
		snprintf(CP(buffer2), sizeof buffer2, "%d digraphs listed.", i);
		say(CP(buffer2));
	}
}

static	char
my_getarg(args)
	char	**args;
{
	char *arg;

	arg = (char *)next_arg(*args, args);
	if (!args || !*args || !arg)
		return '\0';
	/* Don't trust isdigit() with 8 bits. */
	if ((*arg <= '9') && (*arg >= '0'))
	{
		u_char i = *arg & 0x0f;
		while ( *(++arg) )
			i = (i * 10) + (*arg & 0x0f);
		return i;
	}
	else if ( (*arg == '!') && (*(arg + 1)) )
		return *(arg + 1) | 0x80;
	return *arg;
}

void
save_digraphs(fp)
	FILE	*fp;
{
	if (digraph_changed)
	{

		int	i = 0;
		char	*command = "\nDIGRAPH -ADD ";

		fprintf(fp, "DIGRAPH -CLEAR");
		fprintf(fp, "%s", command);
		while(1)
		{
			fprintf(fp, "%d %d %d  ", dig_table_lo[i],
				dig_table_hi[i], dig_table_di[i]);
			if (!dig_table_lo[++i])
				break;
			if (!(i % 5))
				fprintf(fp, "%s", command);
		}
		fputc('\n', fp);

	}
}

char
displayable_unival(unsigned unival, iconv_t conv_out)
{
	/* First rule out control characters */
	if((unival >= 0x00 && unival < 0x20) ||
	   (unival >= 0x80 && unival < 0xA0) ||
	   (unival == 0x7F) ||
	   (unival >= 0xFFF0 && unival <= 0xFFFF))
		return 0;
	
	/* Range 0x80..0x9F is used in some character sets (such as cp850),
	 * but they are assigned different positions in unicode.
	 * The univals we handle here are unicode positions.
	 * In unicode, 0x80..0x9F are not used because some
	 * american programs might still blindly assume
	 * 7-bitness and take those as control characters.
	 * 0x7F is delete/backspace.
	 * 0xFFF0..0xFFFF is the unicode control range.
	 * It contains a signature token, an illegal
	 * character token and so on.
	 */

#ifdef HAVE_ICONV_OPEN
	if (conv_out)
	{
		u_char utfbuf[8],*utfptr;
		u_char outbuf[256],*outptr;
		size_t utfspace, outspace;
		size_t retval;

		/* Now sequence the character to buffer
		 * and let iconv say whether it can displayed.
		 */
		utf8_sequence(unival, utfbuf);
		
		utfptr = utfbuf;
		outptr = outbuf;
		utfspace = strlen(utfbuf);
		outspace = sizeof outbuf;
		
		/* reset the converter */
		iconv(conv_out, NULL, 0, NULL, 0);
		
		/*	*outptr = '\0'; */
		retval = iconv(conv_out,
		               (iconv_const char**)&utfptr, &utfspace,
		               (char **)&outptr, &outspace);

		/*
		*outptr = '\0';
		fprintf(stderr, "CHK: '%s' -> '%s', retval=%d, errno=%d\n",
			utfbuf, outbuf,
			retval, errno);
		*/
		return retval != (size_t)-1;
	}
#endif /* HAVE_ICONV_OPEN */
	return 1;
}

unsigned
calc_unival_width(unsigned unival)
{
	/* FIXME: Should we use some kind of database here?
	 * FIXME: Combining marks support is completely untested
	 */
	
	/* chinese, japanese, korean */
	if(unival >= 0x3000 && unival < 0xFF00)
		return 2;
	/* combining diacritical marks */
	if(unival >= 0x0300 && unival < 0x0400)
		return 0;
	/* combining diacritical marks for symbols */
	if(unival >= 0x20D0 && unival < 0x2100)
		return 0;
	/* combining half-marks */
	if(unival >= 0xFE20 && unival < 0xFE30)
		return 0;
	/* everything else */
	return 1;
}

unsigned
calc_unival_length(const u_char* str)
{
	/* Returns the number of bytes taken by
	 * the given utf-8 code
	 */
	static const char sizes[16] =
	{ 1,1,1,1,1,1,1,1,
	  0,0,0,0,2,2,3,4 };
	return sizes[*str >> 4];
	/* 1-byte (0..7F):
	 * 0 1 2 3 4 5 6 7
	 * 2-byte (80..7FF):
	 *         C D
	 * 3-byte (800..FFFF):
	 *             E
	 * 4-byte (10000..1FFFFF):
	 *               F
	 * invalid:
	 * 8 9 A B (they can not begin a sequence)
	 *
	 * If utf8 is some day extended to use 5-byte
	 * codings, you need to double the sizes[] size
	 * and shift str by 3 instead of 4.
	 * You'd also need to modify
	 *  utf8_sequence() and calc_unival().
	 *
	 * Today, it seems unlikely that these encodings
	 * will be needed in practical applications such as
	 * an irc client. Many programs (such as Microsoft IE)
	 * don't even support 4-byte encodings.
	 * 2-3 -byte encodings are in daily use everywhere.
	 */
}

unsigned
calc_unival(const u_char *utfbuf)
{
	/* This function does the reverse of utf8_sequence(). */
	switch (calc_unival_length(utfbuf))
	{
		case 1:
		default:
			return ((utfbuf[0] & 127));
		case 2:
			return ((utfbuf[0] & 31) << 6)
			     | ((utfbuf[1] & 63));
		case 3:
			return ((utfbuf[0] & 15) << 12)
			     | ((utfbuf[1] & 63) << 6)
			     | ((utfbuf[2] & 63));
		case 4:
			return ((utfbuf[0] &  7) << 16)
			     | ((utfbuf[1] & 63) << 12)
			     | ((utfbuf[2] & 63) << 6)
			     | ((utfbuf[3] & 63));
	}
}

void
utf8_sequence(unsigned unival, u_char* utfbuf)
{
	/* This function does the reverse of calc_unival(). */
	/* The output buffer should have 5 bytes of space.  */
	u_char *utfptr = utfbuf;
	if (unival < 0x80)                 /* <=7 bits */
		*utfptr++ = (char)unival;
	else
	{
		if (unival < 0x800)            /* <=11 bits */
			*utfptr++ = (char)(0xC0 + (unival>>6));
		else
		{
			if (unival < 0x10000)      /* <=16 bits */
				*utfptr++ = (char)(0xE0 + (unival>>12));
			else                      /* <=21 bits */
			{
				*utfptr++ = (char)(0xF0 + (unival>>18));
				*utfptr++ = (char)(0x80 + ((unival>>12)&63));
			}
			*utfptr++ = (char)(0x80 + ((unival>>6)&63));
		}
		*utfptr++ = (char)(0x80 + (unival&63));
	}
	/* Last put a zero-terminator. */
	*utfptr = '\0';
/*	
	fprintf(stderr, "utf8-seq %X: %02X %02X (%s)\n",
	    unival, utfbuf[0], utfbuf[1], utfbuf);
*/
}

void
mbdata_init(struct mb_data *d, const char *enc)
{
	bzero(d, sizeof(*d));

#ifdef HAVE_ICONV_OPEN
	d->enc = enc;
	if (!d->conv_in && !d->conv_out && d->enc && display_encoding)
	{
		/* New encoding, reinitialize converters */
		
		if (!d->conv_in)
			d->conv_in = iconv_open("UTF-8", d->enc);
		if (!d->conv_out)
			d->conv_out = iconv_open(display_encoding, "UTF-8");
		
		if (d->conv_in == (iconv_t)(-1))
		{
			iconv_close(d->conv_in);
			d->conv_in = NULL;
		}
		if (d->conv_out == (iconv_t)(-1))
		{
			iconv_close(d->conv_out);
			d->conv_out = NULL;
		}
	}
#endif /* HAVE_ICONV_OPEN */
}

void
mbdata_done(struct mb_data* d)
{
#ifdef HAVE_ICONV_OPEN
	if (d->conv_in)
		iconv_close(d->conv_in);
	if (d->conv_out)
		iconv_close(d->conv_out);
#endif /* HAVE_ICONV_OPEN */
	bzero(d, sizeof(*d));
}

void
decode_mb(ptr, dest, data)
	u_char   *ptr;      /* Source, encoded in whatever */
	u_char   *dest;	    /* Target, encoded in utf-8 - NULL is allowed */
	mb_data  *data;	    /* Populated with data*/
{
#ifdef HAVE_ICONV_OPEN
	/* If iconv has now been initialized, use it. */
	if (data->conv_in && data->conv_out)
	{
		/* Task:
		 *  Eat input byte by byte
		 *  Until either
		 *	- the input is exhausted
		 *	- conv_in creates a character
		 *  When conv_in creates a character,
		 *	- feed the character to conv_out
		 *	- if conv_out says dame desu yo
		 *	- we have an invalid character
		 *	- otherwise, analyze the unicode value
		 *	  - For values 0000..001F: add 40, invert (invalid)
		 *	  - For values 0080..009F: dec 40, invert (invalid)
		 *	  - For values 3000..FEFF: (CJK) width=2
		 */
		u_char utfbuf[8], *utfptr = utfbuf;
		size_t utfspace = sizeof(utfbuf);
		unsigned unival;
		int error = 0;
		size_t retval = 0;
		
		data->input_bytes  = 0;
		data->output_bytes = 0;
		data->num_columns  = 0;
		
		*utfptr = '\0';
		
		while (*ptr != '\0')
		{
			unsigned gave;
			size_t is = 1;
			u_char *cptr, *cutfptr;

		retry:
			gave = is;
			cptr = (char *)ptr;
			cutfptr = (char *)utfptr;
			retval = iconv(data->conv_in,
			               (iconv_const char**)&ptr, &is,
			               (char **)&utfptr, &utfspace);
		
			data->input_bytes  += gave-is;
		
			if (retval == (size_t)-1)
			{
				switch (errno)
				{
				case EINVAL:
					/* We didn't give enough bytes. Must give more */
					is = gave;
					if (ptr[is] != '\0')
					{
						++is;
						goto retry;
					}
					/* It's an undecodable input. */
					error = 1;
					data->input_bytes = 1;
					++ptr;
					goto endloop;
				case EILSEQ:
					/* Ignore invalid byte, continue loop. */
					error = 1;
					if (*ptr != '\0')
					{
						++ptr;
						++data->input_bytes;
					}
					continue;
				}
			}
		
			if (utfptr > utfbuf)
			{
				/* An UTF-8 character was created! */
				data->output_bytes += utfptr-utfbuf;
				*utfptr = '\0';
			endloop:
				break;
			}
			break;
		}
		
		if (data->output_bytes == 0 && !error)
		{
			/* Nothing was produced, no errors. */
			return;
		}
		
		unival = 0;
		
		if (data->output_bytes > 0)
		{
			/* Calculate the unicode value of the utf8 character */
			unival = calc_unival(utfbuf);
		}
		
		if (!displayable_unival(unival, data->conv_out))
		{
			/* The character could not be expressed in display encoding
			 * or would be a control character
			 */
			data->num_columns  = data->input_bytes;
			data->output_bytes = data->input_bytes;
			if (data->output_bytes > 0)
				data->output_bytes += 2;
			if (dest)
			{
				unsigned n = data->input_bytes;
				if (n > 0)
				{
					ptr -= n;
					*dest++ = REV_TOG;
					/* we assume ascii always works */
					while (n-- > 0)
						*dest++ = (*ptr++ & 127) | 64;
					*dest++ = REV_TOG;
				}
			}
			return;
		}

		data->num_columns = calc_unival_width(unival);
		
		if (dest)
		{
			memcpy(dest, utfbuf, data->output_bytes);
		}
		return;
	}
#endif /* HAVE_ICONV_OPEN */
	/* No usable iconv (maybe csets were invalid), assume ISO-8859-1 in */
	data->input_bytes  = 1;
	data->num_columns  = 1;
	
	if (!displayable_unival(*ptr, NULL))
	{
		data->output_bytes = 3;
		if (dest)
		{
			*dest++ = REV_TOG;
			*dest++ = (*ptr & 127) | 64;
			*dest++ = REV_TOG;
		}
	}
	else
	{
		unsigned unival = *ptr;

		if (unival < 0x80)
			data->output_bytes = 1;
		else if (unival < 0x800)
			data->output_bytes = 2;
		else if (unival < 0x10000)
			data->output_bytes = 3;
		else
			data->output_bytes = 4;
		if (dest)
			utf8_sequence(unival, dest);
	}
}

void
set_irc_encoding(char *enc)
{
#ifdef HAVE_ICONV_OPEN
	iconv_t test = iconv_open("UTF-8", enc);

	if (test != NULL && test != (iconv_t)(-1))
	{
		iconv_close(test);
		malloc_strcpy(&irc_encoding, enc);
	}
	else
		say("IRC_ENCODING value %s is not supported by this system", enc);
#else
	say("IRC_ENCODING has no effect - this version was compiled without iconv support");
#endif /* HAVE_ICONV_OPEN */
}

void
set_display_encoding(char *enc)
{
#ifdef HAVE_ICONV_OPEN
	iconv_t test = iconv_open(enc, "UTF-8");

	if (test != NULL && test != (iconv_t)(-1))
	{
		iconv_close(test);
		malloc_strcpy(&display_encoding, enc);
	}
	else
		say("DISPLAY_ENCODING value %s is not supported by this system", enc);
#else
	say("DISPLAY_ENCODING has no effect - this version was compiled without iconv support");
#endif /* HAVE_ICONV_OPEN */
}

void
set_input_encoding(char *enc)
{
#ifdef HAVE_ICONV_OPEN
	iconv_t test = iconv_open("UTF-8", enc);

	if (test != NULL && test != (iconv_t)(-1))
	{
		iconv_close(test);
		malloc_strcpy(&input_encoding, enc);
	}
	else
		say("INPUT_ENCODING value %s is not supported by this system", enc);
#else
	say("INPUT_ENCODING has no effect - this version was compiled without iconv support");
#endif /* HAVE_ICONV_OPEN */
}
