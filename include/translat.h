/*
 * Global stuff for translation tables.
 *
 * Tomten, tomten@solace.hsh.se / tomten@lysator.liu.se
 *
 * $Id: translat.h,v 1.2 1999-02-15 21:18:54 f Exp $
 */

#ifndef __translat_h_
# define __translat_h_

	void	set_translation _((char *));
	void	enter_digraph _((u_int, char *));
 	unsigned char	get_digraph _((u_int));
	void	digraph _((char *, char *, char *));
	void	save_digraphs _((FILE *));

extern	unsigned char	transToClient[256];
extern	unsigned char	transFromClient[256];
extern	char	translation;

# define DIG_TABLE_SIZE 256
extern	unsigned char	dig_table_lo[DIG_TABLE_SIZE];
extern	unsigned char	dig_table_hi[DIG_TABLE_SIZE];
extern	unsigned char	dig_table_di[DIG_TABLE_SIZE];

extern	char	digraph_hit;
extern	unsigned char	digraph_first;

#endif /* __translat_h_ */
