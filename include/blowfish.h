#ifndef _blowfish_h_
#define _blowfish_h_

/*
 * Routines for Blowfish encryption
 *
 * $Id: blowfish.h,v 1.3 1998-11-16 17:44:29 f Exp $
 */

static unsigned int F _((unsigned int));
static void BlowfishEncipher _((unsigned int *, unsigned int *));
static void BlowfishDecipher _((unsigned int *, unsigned int *));
static void BlowfishInit _((char *, int));
void EncryptString _((char *, char *, char *, int));
int  Base64Decode _((char));
void DecryptString _((char *, char *, char *, int));

#endif /* _blowfish_h_ */
