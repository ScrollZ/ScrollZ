#ifndef _blowfish_h_
#define _blowfish_h_

/*
 * Routines for Blowfish encryption
 *
 * $Id: blowfish.h,v 1.1 1998-09-10 17:31:12 f Exp $
 */

static unsigned int F(unsigned int);
static void BlowfishEncipher(unsigned int *, unsigned int *);
static void BlowfishInit(char *, int);
void EncryptString(char *, char *, char *, int);

#endif /* _blowfish_h_ */
