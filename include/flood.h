/*
 * flood.h: header file for flood.c
 *
 * $Id: flood.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

#ifndef __flood_h_
#define __flood_h_

	int	check_flooding _((char *, int, char *));

#define MSG_FLOOD 0
#define PUBLIC_FLOOD 1
#define NOTICE_FLOOD 2
#define WALL_FLOOD 3
#define WALLOP_FLOOD 4
/**************************** PATCHED by Flier ******************************/
/*#define NUMBER_OF_FLOODS 5*/
#define CTCP_FLOOD 5
#define NUMBER_OF_FLOODS 6
/****************************************************************************/

#endif /* __flood_h_ */
