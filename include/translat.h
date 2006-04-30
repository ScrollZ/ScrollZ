/*
 * Global stuff for translation tables.
 *
 * Tomten, tomten@solace.hsh.se / tomten@lysator.liu.se
 *
 * $Id: translat.h,v 1.4 2006-04-30 14:15:43 f Exp $
 */

#ifndef __translat_h_
# define __translat_h_

#include "defs.h"

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

	void	enter_digraph _((u_int, char *));
	char	get_digraph _((u_int));
	void	digraph _((char *, char *, char *));
	void	save_digraphs _((FILE *));

#ifndef HAVE_ICONV_OPEN
	typedef void *iconv_t;
#endif

typedef struct mb_data
{
	unsigned input_bytes;    /* How many bytes we ate */
	unsigned output_bytes;   /* How many bytes we produced */
	unsigned num_columns;    /* How many columns does this result take */
#ifdef HAVE_ICONV_OPEN
	iconv_t conv_in;
	iconv_t conv_out;
	const char* enc;
#endif
} mb_data;

	/* Returns true(1)/false(0) whether the given unival is printable */
	char displayable_unival(unsigned unival, iconv_t conv_out);

	/* Sequence the given unicode value to the given utf-8 buffer */
	void utf8_sequence(unsigned unival, u_char* utfbuf);
	
	/* Return the unicode value of the first character in the given utf-8 string */
	unsigned calc_unival(const u_char *);
	/* Guess the width of the given unicode value in columns */
	unsigned calc_unival_width(unsigned unival);
	/* Calculate the length of the unicode sequence beginning at given pos */
	unsigned calc_unival_length(const u_char* );

	void set_irc_encoding(char *);
	void set_input_encoding(char *);
	void set_display_encoding(char *);

	void mbdata_init(struct mb_data* d, const char* enc);
	void mbdata_done(struct mb_data* d);
	void decode_mb(u_char *ptr, u_char *dest, mb_data *data);

# define DIG_TABLE_SIZE 256
extern	char	dig_table_lo[DIG_TABLE_SIZE];
extern	char	dig_table_hi[DIG_TABLE_SIZE];
extern	char	dig_table_di[DIG_TABLE_SIZE];

extern	char	digraph_hit;
extern	u_char	digraph_first;

#ifdef HAVE_ICONV_OPEN
extern char  *irc_encoding;
extern char  *display_encoding;
extern char  *input_encoding;
#endif

#endif /* __translat_h_ */
