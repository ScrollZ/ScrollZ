/*
 * crypt.h: header for crypt.c 
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
 * @(#)$Id: crypt.h,v 1.2 1999-03-04 22:06:01 f Exp $
 */

#ifndef __crypt_h_
#define __crypt_h_

#define	CTCP_SHUTUP	0
#define	CTCP_VERBOSE	1
#define	CTCP_NOREPLY	2

/*
 * crypt interface
 */

typedef struct crypt_key crypt_key;

typedef void (*CryptFunc) _((char **, int *, crypt_key *));

struct crypt_key
{
	char		*key;
	char		*type;
	CryptFunc	crypt;
	CryptFunc	decrypt;
};

/*
 * function interfaces we have
 */
char		*crypt_msg _((char *, crypt_key *, int));
void		encrypt_cmd _((char *, char *, char *));
crypt_key	*is_crypted _((char *));

/*
 * the broken old `ctcp sed' crap.  we retain this but make it
 * optional..
 */
#ifndef NO_USE_SED
#define USE_SED
#endif

/*
 * a crypter must define a `<CRYPTER>_CTCP_ENTRY', which is added to
 * CRYPTO_CTCP_ENTRIES, and a default crypter/decrypter functions.
 * should let the user somehow set these so that one of them can 
 * for to be the default.
 */
#ifdef USE_CAST
# define CAST_STRING	"CAST128ED"
# define CAST_CTCP_ENTRY { CAST_STRING, "contains CAST-128 strongly encrypted data", \
		CTCP_SHUTUP | CTCP_NOREPLY, do_crypto },
# ifndef DEFAULT_CRYPTER
#  define DEFAULT_CRYPTER cast_encrypt_str
#  define DEFAULT_DECRYPTER cast_decrypt_str
#  define DEFAULT_CRYPTYPE CAST_STRING
# endif
#else
# define CAST_CTCP_ENTRY
#endif

#ifdef USE_SED
# define SED_STRING	"SED"
# define SED_CTCP_ENTRY { SED_STRING, "contains simple weekly encrypted data", \
		CTCP_SHUTUP | CTCP_NOREPLY, do_crypto },
# ifndef DEFAULT_CRYPTER
#  define DEFAULT_CRYPTER sed_encrypt_str
#  define DEFAULT_DECRYPTER sed_decrypt_str
#  define DEFAULT_CRYPTYPE SED_STRING
# endif
#else
# define SED_CTCP_ENTRY
#endif

#define	CRYPTO_CTCP_ENTRIES \
	CAST_CTCP_ENTRY \
	SED_CTCP_ENTRY

#endif /* _crypt_h_ */
