/*
 * mail.c: Ok, so I gave in.  I added mail checking.  So sue me. 
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
 * $Id: mail.c,v 1.8 2001-01-22 18:19:01 f Exp $
 */

#include "irc.h"
#include "newio.h"
/**************************** PATCHED by Flier *****************************/
#include "myvars.h"
#include "mystructs.h"
#include "parse.h"
#include "ircaux.h"
#include "screen.h"
#include "vars.h"
#include "ircterm.h"
/***************************************************************************/

#if defined(HAVE_DIRENT_H) || defined(_POSIX_SOURCE)
# include <dirent.h>
# define NLENGTH(d) (strlen((d)->d_name)
#else /* DIRENT || _POSIX_SOURCE */
# define dirent direct
# define NLENGTH(d) ((d)->d_namlen)
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H || _POSIX_VERSION */

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef ESIX
# include <lan/net_types.h>
#endif /* !ESIX */

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */

#include <sys/stat.h>

#include "mail.h"
#include "lastlog.h"
#include "hook.h"
#include "vars.h"
#include "ircaux.h"
#include "output.h"
#include "window.h"

static	char	*mail_path = (char *) 0;

static	void	init_mail _((void));

/* init_mail: this initialized the path to the users mailbox */
static	void
init_mail()
{
	char	*tmp_mail_path;

	if (mail_path)
		return; /* why do it 2000 times?  -lynx */

#ifdef UNIX_MAIL
	if ((tmp_mail_path = getenv("MAIL")) != NULL)
		malloc_strcpy(&mail_path, tmp_mail_path);
	else
	{
		malloc_strcpy(&mail_path, UNIX_MAIL);
		malloc_strcat(&mail_path, "/");
		malloc_strcat(&mail_path, username);
	}
#else
# ifdef AMS_MAIL
	malloc_strcpy(&mail_path, my_path);
	malloc_strcat(&mail_path, "/");
	malloc_strcat(&mail_path, AMS_MAIL);
# endif /* AMS_MAIL */
#endif /* UNIX_MAIL */
}

#ifdef AMS_MAIL
/*
 * count_files: counts all the visible files in the specified directory and
 * returns that number as the function value 
 */
static	u_int
count_files(dir_name, lasttime)
	char	*dir_name;
	time_t	lasttime;
{
	DIR	*dir;
	struct	direct	*dirbuf;
	unsigned int	cnt;
	int	fd;
	char	LetterName[BIG_BUFFER_SIZE+1];
	struct	stat	LetterInfo;
	static	int	VirginProgram = 1;
	int	lastlog_level;

	if ((dir = opendir(dir_name)) == (DIR *) 0)
		return (0);
	cnt = 0;
	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);		/* XXX should delete this */
	while ((dirbuf = readdir(dir)) != (struct direct *) 0)
	{
		if (*(dirbuf->d_name) != '.')
		{
			cnt++;
			sprintf(LetterName, "%s/%s", dir_name, dirbuf->d_name);
			stat_file(LetterName, &LetterInfo);
			if (get_int_var(MAIL_VAR) == 2 && LetterInfo.st_ctime>lasttime && !VirginProgram)
			{
				if ((fd = open(LetterName, O_RDONLY)) == -1)
					say("Unable to check headers on new mail");
				else
				{
/**************************** PATCHED by Flier *****************************/
					/*while (dgets(LetterName,BIG_BUFFER_SIZE, fd, (char *) 0) > 0 && *LetterName != '\n' &&
					    *LetterName != '\0')*/
					while (dgets(LetterName,BIG_BUFFER_SIZE, fd, (char *) 0, 0) > 0 && *LetterName != '\n' &&
					    *LetterName != '\0')
/***************************************************************************/
					{
						LetterName[strlen(LetterName) - - 1] = '\0';
 						if (!my_strnicmp(LetterName, "From", 4) || !my_strnicmp(LetterName, "Subject:", 8))
							say("%s", LetterName);
					}
					new_close(fd);
				}
			}
		}
	}
end:
	VirginProgram = 0;
	closedir(dir);
 	restore_message_from();
	set_lastlog_msg_level(lastlog_level);
	return (cnt);
}
#endif /* AMS_MAIL */

/*
 * check_mail_status: returns 0 if mail status has not changed, 1 if mail
 * status has changed 
 */
int
check_mail_status()
{

#if defined(AMS_MAIL) || defined(UNIX_MAIL)
	struct	stat	stat_buf;
	static	time_t	old_stat = 0L;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
		return 0;
#endif /*DAEMON_UID*/

	if (!get_int_var(MAIL_VAR))
	{
		old_stat = 0L;
		return (0);
	}
	init_mail();
	if (stat_file(mail_path, &stat_buf) == -1)
		return (0);
	if (stat_buf.st_ctime > old_stat)
	{
		old_stat = stat_buf.st_ctime;
		return (1);
	}
#endif /* defined(AMS_MAIL) || defined(UNIX_MAIL) */
	return (0);
}

/*
 * check_mail: This here thing counts up the number of pieces of mail and
 * returns it as static string.  If there are no mail messages, null is
 * returned. 
 */
char	*
check_mail()
{
#if !defined(AMS_MAIL) && !defined(UNIX_MAIL)
	return	(char *) 0;
#else
	static	unsigned int	cnt = 0;
	static	time_t	old_stat = 0L;
	static	char	ret_str[8];
	struct	stat	stat_buf;
	unsigned int	new_cnt = 0;
	char	tmp[8];
	static	int	VirginProgram = 1;  /* It's its first time */
	int	lastlog_level;
	char	buffer[BIG_BUFFER_SIZE+1];
#ifdef UNIX_MAIL
	int	des;
	int	blanks = 1;
	int	reset_blanks = 0;
#endif /* UNIX_MAIL */

#ifdef DAEMON_UID
	if (getuid()==DAEMON_UID)
		return ((char *) 0);
#endif

	init_mail();
#ifdef UNIX_MAIL
	if (stat_file(mail_path, &stat_buf) == -1)
		return ((char *) 0);
	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
 	save_message_from();
	message_from((char *) 0, LOG_CURRENT);		/* XXX should delete this */
	if (stat_buf.st_ctime > old_stat)
	{
		old_stat = stat_buf.st_ctime;
		if ((des = open(mail_path, O_RDONLY, 0)) >= 0)
		{
			new_cnt = 0;
/**************************** PATCHED by Flier *****************************/
			/*while (dgets(buffer, BIG_BUFFER_SIZE, des,(char *) 0)>0)*/
			while (dgets(buffer, BIG_BUFFER_SIZE, des,(char *) 0, 0)>0)
/***************************************************************************/
			{
				if (buffer[0] == '\n') {
					blanks++;
					continue;
				}
				else
					reset_blanks = 1;
				if (!strncmp(MAIL_DELIMITER, buffer, sizeof(MAIL_DELIMITER) - 1) && blanks)
				{
					new_cnt++;
					if (new_cnt > cnt && !VirginProgram && get_int_var(MAIL_VAR) == 2)
					{
/**************************** PATCHED by Flier *****************************/
						/*while (dgets(buffer, BIG_BUFFER_SIZE, des, (char *) 0) > 0 && *buffer != '\0' && *buffer != '\n')*/
						while (dgets(buffer, BIG_BUFFER_SIZE, des, (char *) 0, 0) > 0 && *buffer != '\0' && *buffer != '\n')
/***************************************************************************/
						{
							buffer[strlen(buffer)-1] = '\0';
							if (!strncmp(buffer, "From:", 5) || !strncmp(buffer, "Subject:", 8))
								say("%s", buffer);
						}
					}
				}
				if (reset_blanks)
					reset_blanks = blanks = 0;
			}
			VirginProgram=0;
			new_close(des);
		}
#else
# ifdef AMS_MAIL
		if (stat_file(mail_path, &stat_buf) == -1)
		{
			set_lastlog_msg_level(lastlog_level);
			return ((char *) 0);
		}
		if (stat_buf.st_ctime > old_stat)
		{
			new_cnt = count_files(mail_path, old_stat);
			old_stat = stat_buf.st_ctime;
		}
# endif /* AMS_MAIL */
#endif /* UNIX_MAIL */
		/* yeeeeack */
		if (new_cnt > cnt)
		{
                        sprintf(tmp, "%d", new_cnt - cnt);
                        sprintf(buffer, "%d", new_cnt);
			if (do_hook(MAIL_LIST, "%s %s", tmp, buffer) && get_int_var(MAIL_VAR) == 1)
/**************************** PATCHED by Flier *****************************/
				/*say("You have new email.");*/
                        {
#ifdef CELECOSM
                            say("%sYou have %s new email(s) - %s total.%s",
                                CmdsColors[COLCELE].color3,tmp,buffer,Colors[COLOFF]);
#else
                            say("You have new email.");
#endif
                            if (get_int_var(BEEP_ON_MAIL_VAR))
                                term_beep();
                        }
/***************************************************************************/
		}
		cnt = new_cnt;
	}
	set_lastlog_msg_level(lastlog_level);
 	restore_message_from();
	if (cnt && (cnt < 65536))
	{
		sprintf(ret_str, "%d", cnt);
		return (ret_str);
	}
	else
		return ((char *) 0);
#endif /* !defined(AMS_MAIL) && !defined(UNIX_MAIL) */
}
