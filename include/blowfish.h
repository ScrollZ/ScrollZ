#ifndef _blowfish_h_
#define _blowfish_h_

/*
 * Routines for Blowfish encryption
 *
 * $Id: blowfish.h,v 1.2 1998-11-15 20:13:57 f Exp $
 */

static unsigned int F(unsigned int);
static void BlowfishEncipher(unsigned int *, unsigned int *);
static void BlowfishDecipher(unsigned int *, unsigned int *);
static void BlowfishInit(char *, int);
void EncryptString(char *, char *, char *, int);
int  Base64Decode(char);
void DecryptString(char *, char *, char *, int);

#endif /* _blowfish_h_ */
