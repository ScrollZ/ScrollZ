/*
 * sed.c: has the old (broken) encryption stuff.
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
 */

#ifndef LITE

static	void	sed_encrypt_str _((u_char **, int *, crypt_key *));
static	void	sed_decrypt_str _((u_char **, int *, crypt_key *));

/*
 * these are the old, broken crypt functions.  "cast.c" includes
 */
static	void
sed_encrypt_str(str, len, key)
	u_char	**str;
	int	*len;
	crypt_key	*key;
{
	int	key_len,
		key_pos,
		i;
	u_char	mix,
		tmp;

	key_len = strlen((char *) key->key);
	key_pos = 0;
	mix = 0;
	for (i = 0; i < *len; i++)
	{
		tmp = (*str)[i];
		(*str)[i] = mix ^ tmp ^ key->key[key_pos];
		mix ^= tmp;
		key_pos = (key_pos + 1) % key_len;
	}
	(*str)[i] = (char) 0;
}

static	void
sed_decrypt_str(str, len, key)
	u_char	**str;
	int	*len;
	crypt_key	*key;
{
	int	key_len,
		key_pos,
		i;
	u_char	mix,
		tmp;

	key_len = strlen((char *) key->key);
	key_pos = 0;
	/*    mix = key->key[key_len-1]; */
	mix = 0;
	for (i = 0; i < *len; i++)
	{
		tmp = mix ^ (*str)[i] ^ key->key[key_pos];
		(*str)[i] = tmp;
		mix ^= tmp;
		key_pos = (key_pos + 1) % key_len;
	}
	(*str)[i] = (char) 0;
}

#endif /* LITE */
