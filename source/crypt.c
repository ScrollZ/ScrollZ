/*
 * crypt.c: handles some encryption of messages stuff. 
 *
 * Written By Michael Sandrof
 *
 * Copyright (c) 1990 Michael Sandrof.
 * Copyright (c) 1991, 1992 Troy Rollo.
 * Copyright (c) 1992-2003 Matthew R. Green.
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
IRCII_RCSID("@(#)$Id: crypt.c,v 1.13 2003-01-08 20:00:54 f Exp $");

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

#ifndef LITE

/**************************** Patched by Flier ******************************/
#define my_strlen(x)    strlen((char *) x)
#define my_strcpy(x, y) strcpy((char *) x, (char *) y)
/****************************************************************************/

static	void	add_to_crypt _((u_char *, u_char *, CryptFunc, CryptFunc, u_char *));
static	int	remove_crypt _((u_char *));
static	u_char	*do_crypt _((u_char *, crypt_key *, int, u_char **));

#ifdef HAVE_DEV_RANDOM
static	int	crypt_dev_random_byte _((void));
#define GET_RANDOM_BYTE	crypt_dev_random_byte()
#else
/* gotta use the sucky one */
#define GET_RANDOM_BYTE	(random() & 255)
#endif

#include "cast.c"
#if 0
#include "rijndael.c"
#endif
#include "sed.c"

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
	u_char	*nick;
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
	u_char	*nick;
	u_char	*keystr;
	CryptFunc enc;
	CryptFunc dec;
	u_char	*type;
{
	Crypt	*new;

	if ((new = (Crypt *) remove_from_list((List **) &crypt_list, nick)) != NULL)
	{
		new_free(&(new->nick));
		bzero(new->key->key, strlen((char *) new->key->key));		/* wipe it out */
		new_free(&(new->key->key));
		new_free(&(new->key->cookie));
		new_free(&(new->key));
		new_free(&new);
	}
	new = (Crypt *) new_malloc(sizeof(Crypt));
	new->key = (crypt_key *) new_malloc(sizeof(*new->key));
	new->nick = (u_char *) 0;
	new->key->key = (u_char *) 0;
	new->key->type = type;
	malloc_strcpy((char **) &(new->nick), nick);
	malloc_strcpy((char **) &(new->key->key), keystr);
	new->key->crypt = enc;
	new->key->decrypt = dec;
	new->key->cookie = NULL;
	add_to_list((List **) &crypt_list, (List *) new);
}

/*
 * remove_crypt: removes the given nickname from the crypt_list, returning 0
 * if successful, and 1 if not (because the nickname wasn't in the list) 
 */
static	int
remove_crypt(nick)
	u_char	*nick;
{
	Crypt	*tmp;

	if ((tmp = (Crypt *) list_lookup((List **) &crypt_list, nick, !USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
	{
		new_free(&tmp->nick);
		bzero(tmp->key->key, strlen((char *) tmp->key->key));		/* wipe it out */
		new_free(&tmp->key->key);
		new_free(&tmp->key->cookie);
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
	u_char	*nick;
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
	u_char	*command,
		*args,
		*subargs;
{
	/* XXX this is getting really, really gross */
	CryptFunc enc = DEFAULT_CRYPTER, dec = DEFAULT_DECRYPTER;
	u_char	*type = DEFAULT_CRYPTYPE;
	u_char	*nick;
	int	showkeys = 0;

restart:
	if ((nick = next_arg((char *) args, (char **) &args)) != NULL)
	{
 		size_t len = strlen((char *) nick);

 		if (my_strnicmp((char *) nick, "-showkeys", len) == 0)
 		{
 			showkeys = 1;
 			goto restart;
 		}
 		else if (my_strnicmp((char *) nick, "-cast", len) == 0)
		{
			enc = cast_encrypt_str;
			dec = cast_decrypt_str;
			type = CAST_STRING;
			goto restart;
		}
#if 0
		else if (my_strnicmp(nick, UP("-rijndael"), len) == 0)
		{
			enc = rijndael_encrypt_str;
			dec = rijndael_decrypt_str;
			type = RIJNDAEL_STRING;
			goto restart;
		}
#endif
		else if (my_strnicmp((char *) nick, "-sed", len) == 0)
		{
			enc = sed_encrypt_str;
			dec = sed_decrypt_str;
			type = SED_STRING;
			goto restart;
		}

		if (args && *args)
		{
			add_to_crypt(nick, args, enc, dec, type);
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
				if (showkeys)
					put_it("%s with key \"%s\" type %s", tmp->nick, tmp->key->key, tmp->key->type);
				else
					put_it("%s type %s", tmp->nick, tmp->key->type);
		}
		else
			say("The crypt is empty");
	}
}

static	u_char	*
do_crypt(str, key, flag, type)
	u_char	*str;
	crypt_key *key;
	int	flag;
	u_char	**type;
{
	int	in[2],
		out[2];
	size_t	c;
	u_char	lbuf[CRYPT_BUFFER_SIZE + 1];
	u_char	*ptr = (char *) 0,
		*crypt_program,
		*encrypt_program,
		*decrypt_program,
		*crypt_str;

#ifndef _Windows
	encrypt_program = get_string_var(ENCRYPT_PROGRAM_VAR);
	decrypt_program = get_string_var(DECRYPT_PROGRAM_VAR);
	if ((flag && encrypt_program) || (!flag && decrypt_program))
	{
#ifdef DAEMON_UID
		if (DAEMON_UID == getuid())
		{
			say("ENCRYPT_PROGRAM not available from daemon mode");
			return (u_char *) 0;
		}
#endif /* DAEMON_ID */
		in[0] = in[1] = -1;
		out[0] = out[1] = -1;
		if (flag)
		{
			crypt_str = CP("encryption");
			crypt_program = encrypt_program;
		}
		else
		{
			crypt_str = CP("decryption");
			crypt_program = decrypt_program;
		}
		if (access(CP(crypt_program), X_OK))
		{
			say("Unable to execute %s program: %s", crypt_str, crypt_program);
			return ((u_char *) 0);
		}
		c = strlen((char *) str);
		if (!flag)
			ptr = ctcp_unquote_it(str, &c);
		else 
			malloc_strcpy((char **) &ptr, str);
		if (pipe(in) || pipe(out))
		{
			say("Unable to start %s process: %s", crypt_str, strerror(errno));
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
			say("Unable to start %s process: %s", crypt_str, strerror(errno));
			return ((u_char *) 0);
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
				execl(CP(crypt_program), CP(crypt_program), key->key, NULL);
			else
				execl(CP(crypt_program), CP(crypt_program), NULL);
			exit(0);
		default:
			new_close(out[1]);
			new_close(in[0]);
			if (get_int_var(OLD_ENCRYPT_PROGRAM_VAR) == 0)
			{
				write(in[1], key->key, strlen((char *) key->key));
				write(in[1], "\n", 1);
			}
			write(in[1], ptr, c);
			new_close(in[1]);
			c = read(out[0], lbuf, CRYPT_BUFFER_SIZE);
			wait(NULL);
			lbuf[c] = (u_char) 0;
			new_close(out[0]);
			break;
		}
		new_free(&ptr);
		if (flag)
			ptr = ctcp_quote_it(lbuf, strlen((char *) lbuf));
		else
			malloc_strcpy((char **) &ptr, lbuf);
	}
	else
#endif /* _Windows */
	{
		c = strlen((char *) str);
		if (flag)
		{
			if (key->crypt(key, &str, (int *)&c) != 0)
			{
				yell("--- do_crypt(): crypto encrypt failed");
				return 0;
			}
			ptr = ctcp_quote_it(str, c);
		}
		else
		{
			ptr = ctcp_unquote_it(str, &c);
			if (key->decrypt(key, &ptr, (int *)&c) != 0)
			{
				yell("--- do_crypt(): crypto decrypt failed");
				return 0;
			}
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
u_char	*
crypt_msg(str, key, flag)
	u_char	*str;
	crypt_key *key;
	int	flag;
{
	static	u_char	lbuf[CRYPT_BUFFER_SIZE + 1];
	u_char	*ptr,
		*rest,
		*type;
	int	on = 1;

	if (flag)
	{
		*lbuf = (u_char) 0;
		while ((rest = index((char *) str, '\005')) != NULL)
		{
			*(rest++) = (u_char) 0;
			if (on && *str && (ptr = do_crypt(str, key, flag, &type)))
			{
				snprintf(CP(lbuf), sizeof lbuf, "%c%.30s ", CTCP_DELIM_CHAR, type);
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
			snprintf(CP(lbuf), sizeof lbuf, "%c%.30s ", CTCP_DELIM_CHAR, type);
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

#ifdef HAVE_DEV_RANDOM
static RETSIGTYPE alarmer _((void));

static RETSIGTYPE
alarmer()
{

}

static int
crypt_dev_random_byte()
{
	static	int	devrndfd = -1;
	u_char	c;

	if (devrndfd == -1)
	{
		devrndfd = open(DEV_RANDOM_PATH, O_RDONLY);

		if (devrndfd == -1)
		{
			yell("--- HELP!!!!rypt_dev_random_byte: can not open %s: %s",
			    DEV_RANDOM_PATH, strerror(errno));
			yell("--- using random()");
			devrndfd = -2;
		}
	}
	if (devrndfd == -2)
		goto do_random_instead;

	alarm(2);
	(void)MY_SIGNAL(SIGALRM, (sigfunc *) alarmer, 0);
	if (read(devrndfd, &c, 1) != 1)
	{
		alarm(0);
		(void)MY_SIGNAL(SIGALRM, (sigfunc *) SIG_DFL, 0);
		/* if we were just interrupted, don't bail on /dev/random */
		if (errno == EINTR)
		{
			yell("--- crypt_dev_random_byte: timeout, using random()");
			goto do_random_instead;
		}
		yell("--- HELP!  crypt_dev_random_byte: read of one byte on %s failed: %s",
		    DEV_RANDOM_PATH, strerror(errno));
		yell("--- using random()");
		devrndfd = -2;
		goto do_random_instead;
	}
	alarm(0);
	(void)MY_SIGNAL(SIGALRM, (sigfunc *) SIG_DFL, 0);
	return ((int)c);

do_random_instead:
	return (random() & 255);
}
#endif

#endif /* LITE */
