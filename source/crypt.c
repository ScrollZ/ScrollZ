/*
 * crypt.c: handles some encryption of messages stuff. 
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

#include "irc.h"
IRCII_RCSID("@(#)$Id: crypt.c,v 1.5 1999-03-04 22:06:05 f Exp $");

#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "list.h"
#include "ctcp.h"
#include "output.h"
#include "newio.h"

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

static	void	add_to_crypt _((char *, char *, CryptFunc, CryptFunc, char *));
static	int	remove_crypt _((char *));
static	char	*do_crypt _((char *, crypt_key *, int, char **));

#ifdef USE_CAST
#include "cast.c"
#endif

#ifdef USE_SED
#include "sed.c"
#endif

#define CRYPT_BUFFER_SIZE (IRCD_BUFFER_SIZE - 50)	/* Make this less than
							 * the trasmittable
							 * buffer */
/*
 * Crypt: the crypt list structure,  consists of the nickname, and the
 * encryption key 
 */
typedef struct	CryptStru
{
	struct	CryptStru *next;
	char	*nick;
	crypt_key	*key;
}	Crypt;

/* crypt_list: the list of nicknames and encryption keys */
static	Crypt	*crypt_list = (Crypt *) 0;

/*
 * add_to_crypt: adds the nickname and key pair to the crypt_list.  If the
 * nickname is already in the list, then the key is changed the the supplied
 * key. 
 */
static	void
add_to_crypt(nick, keystr, enc, dec, type)
	char	*nick;
	char	*keystr;
	CryptFunc enc;
	CryptFunc dec;
	char	*type;
{
	Crypt	*new;

	if ((new = (Crypt *) remove_from_list((List **) &crypt_list, nick)) != NULL)
	{
		new_free(&(new->nick));
		bzero(new->key->key, strlen(new->key->key));		/* wipe it out */
		new_free(&(new->key->key));
		new_free(&(new->key));
		new_free(&new);
	}
	new = (Crypt *) new_malloc(sizeof(Crypt));
	new->key = (crypt_key *) new_malloc(sizeof(*new->key));
	new->nick = (char *) 0;
	new->key->key = (char *) 0;
	new->key->type = type;
	malloc_strcpy(&(new->nick), nick);
	malloc_strcpy(&(new->key->key), keystr);
	new->key->crypt = enc;
	new->key->decrypt = dec;
	add_to_list((List **) &crypt_list, (List *) new);
}

/*
 * remove_crypt: removes the given nickname from the crypt_list, returning 0
 * if successful, and 1 if not (because the nickname wasn't in the list) 
 */
static	int
remove_crypt(nick)
	char	*nick;
{
	Crypt	*tmp;

	if ((tmp = (Crypt *) list_lookup((List **) &crypt_list, nick, !USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
	{
		new_free(&tmp->nick);
		bzero(tmp->key->key, strlen(tmp->key->key));		/* wipe it out */
		new_free(&tmp->key->key);
		new_free(&tmp->key);
		new_free(&tmp);
		return (0);
	}
	return (1);
}

/*
 * is_crypted: looks up nick in the crypt_list and returns the encryption key
 * if found in the list.  If not found in the crypt_list, null is returned. 
 */
crypt_key *
is_crypted(nick)
	char	*nick;
{
	Crypt	*tmp;

	if (!crypt_list)
		return NULL;
	if ((tmp = (Crypt *) list_lookup((List **) &crypt_list, nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		return (tmp->key);
	return NULL;
}

/*
 * encrypt_cmd: the ENCRYPT command.  Adds the given nickname and key to the
 * encrypt list, or removes it, or list the list, you know. 
 */
/*ARGSUSED*/
void
encrypt_cmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	CryptFunc enc = NULL, dec = NULL;
	char	*type = NULL;
	char	*nick,
		*keystr;

restart:
	if ((nick = next_arg(args, &args)) != NULL)
	{
		if (0)
			;
#ifdef USE_CAST
		else if (my_strnicmp(nick, "-cast", 5) == 0)
		{
			enc = cast_encrypt_str;
			dec = cast_decrypt_str;
			type = CAST_STRING;
			goto restart;
		}
#endif

#ifdef USE_SED
		else if (my_strnicmp(nick, "-sed", 5) == 0)
		{
			enc = sed_encrypt_str;
			dec = sed_decrypt_str;
			type = SED_STRING;
			goto restart;
		}
#endif
		else
		{
			enc = DEFAULT_CRYPTER;
			dec = DEFAULT_DECRYPTER;
			type = DEFAULT_CRYPTYPE;
		}

		if ((keystr = next_arg(args, &args)) != NULL)
		{
			add_to_crypt(nick, keystr, enc, dec, type);
			say("%s added to the %s crypt", nick, type);
		}
		else
		{
			if (remove_crypt(nick))
				say("No such nickname in the crypt: %s", nick);
			else
				say("%s removed from the crypt", nick);
		}
	}
	else
	{
		if (crypt_list)
		{
			Crypt	*tmp;

			say("The crypt:");
			for (tmp = crypt_list; tmp; tmp = tmp->next)
				put_it("%s with key %s type %s", tmp->nick, tmp->key->key, tmp->key->type);
		}
		else
			say("The crypt is empty");
	}
}

static	char	*
do_crypt(str, key, flag, type)
	char	*str;
	crypt_key *key;
	int	flag;
	char	**type;
{
	int	in[2],
		out[2];
	size_t	c;
	char	lbuf[CRYPT_BUFFER_SIZE + 1];
	char	*ptr = (char *) 0,
		*encrypt_program;

#ifndef _Windows
	encrypt_program = get_string_var(ENCRYPT_PROGRAM_VAR);
	if (encrypt_program)
	{
#ifdef DAEMON_UID
		if (DAEMON_UID == getuid())
		{
			say("ENCRYPT_PROGRAM not available from daemon mode");
			return (char *) 0;
		}
#endif /* DAEMON_ID */
		in[0] = in[1] = -1;
		out[0] = out[1] = -1;
		if (access(encrypt_program, X_OK))
		{
			say("Unable to execute encryption program: %s", encrypt_program);
			return ((char *) 0);
		}
		c = strlen(str);
		if (!flag)
			ptr = ctcp_unquote_it(str, &c);
		else 
			malloc_strcpy(&ptr, str);
		if (pipe(in) || pipe(out))
		{
			say("Unable to start encryption process: %s", strerror(errno));
			if (in[0] != -1)
			{
				new_close(in[0]);
				new_close(in[1]);
			}
			if (out[0] != -1)
			{
				new_close(out[0]);
				new_close(out[1]);
			}
		}
		switch (fork())
		{
		case -1:
			say("Unable to start encryption process: %s", strerror(errno));
			return ((char *) 0);
		case 0:
			MY_SIGNAL(SIGINT, (sigfunc *) SIG_IGN, 0);
			dup2(out[1], 1);
			dup2(in[0], 0);
			new_close(out[0]);
			new_close(out[1]);
			new_close(in[0]);
			new_close(in[1]);
			setgid(getgid());
			setuid(getuid());
			if (get_int_var(OLD_ENCRYPT_PROGRAM_VAR))
				execl(encrypt_program, encrypt_program, key->key, NULL);
			else
				execl(encrypt_program, encrypt_program, NULL);
			exit(0);
		default:
			new_close(out[1]);
			new_close(in[0]);
			if (get_int_var(OLD_ENCRYPT_PROGRAM_VAR) == 0)
				write(in[1], key->key, strlen(key->key));
			write(in[1], ptr, c);
			new_close(in[1]);
			c = read(out[0], lbuf, CRYPT_BUFFER_SIZE);
			wait(NULL);
			lbuf[c] = (char) 0;
			new_close(out[0]);
			break;
		}
		new_free(&ptr);
		if (flag)
			ptr = ctcp_quote_it(lbuf, strlen(lbuf));
		else
			malloc_strcpy(&ptr, lbuf);
	}
	else
#endif /* _Windows */
	{
		c = strlen(str);
		if (flag)
		{
			key->crypt(&str, (int *)&c, key);
			ptr = ctcp_quote_it(str, c);
		}
		else
		{
			ptr = ctcp_unquote_it(str, &c);
			key->decrypt(&ptr, (int *)&c, key);
		}
	}
	if (type)
		*type = key->type;
	return (ptr);
}

/*
 * crypt_msg: Executes the encryption program on the given string with the
 * given key.  If flag is true, the string is encrypted and the returned
 * string is ready to be sent over irc.  If flag is false, the string is
 * decrypted and the returned string should be readable.
 */
char	*
crypt_msg(str, key, flag)
	char	*str;
	crypt_key *key;
	int	flag;
{
	static	char	lbuf[CRYPT_BUFFER_SIZE + 1];
	char	*ptr,
		*rest,
		*type;
	int	on = 1;

	if (flag)
	{
		*lbuf = (char) 0;
		while ((rest = index(str, '\005')) != NULL)
		{
			*(rest++) = (char) 0;
			if (on && *str && (ptr = do_crypt(str, key, flag, &type)))
			{
				sprintf(lbuf, "%c%.30s ", CTCP_DELIM_CHAR, type);
				strmcat(lbuf, ptr, CRYPT_BUFFER_SIZE);
				strmcat(lbuf, CTCP_DELIM_STR, CRYPT_BUFFER_SIZE);
				new_free(&ptr);
			}
			else
				strmcat(lbuf, str, CRYPT_BUFFER_SIZE);
			on = !on;
			str = rest;
		}
		if (on && (ptr = do_crypt(str, key, flag, &type)))
		{
			sprintf(lbuf, "%c%.30s ", CTCP_DELIM_CHAR, type);
			strmcat(lbuf, ptr, CRYPT_BUFFER_SIZE);
			strmcat(lbuf, CTCP_DELIM_STR, CRYPT_BUFFER_SIZE);
			new_free(&ptr);
		}
		else
			strmcat(lbuf, str, CRYPT_BUFFER_SIZE);
	}
	else
	{
		if ((ptr = do_crypt(str, key, flag, &type)) != NULL)
		{
			strmcpy(lbuf, ptr, CRYPT_BUFFER_SIZE);
			new_free(&ptr);
		}
		else
			strmcat(lbuf, str, CRYPT_BUFFER_SIZE);
	}
	return (lbuf);
}
