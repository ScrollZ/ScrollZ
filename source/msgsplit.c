/*
 * msgsplit.c: automatic splitting of long PRIVMSG/NOTICE lines
 *
 * When a message exceeds the IRC protocol limit (512 bytes per line
 * including CRLF), this module splits it into multiple server commands,
 * preserving ANSI/mIRC color state across chunk boundaries.
 *
 * Copyright (c) 2026 ScrollZ contributors.
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
#include "server.h"
#include "msgsplit.h"
#include <string.h>

/*
 * IRC line limit: 512 bytes including trailing CR LF.
 * Usable payload = 512 - 2 (CRLF) = 510.
 * The server prepends ":nick!user@host " to relayed messages.
 * We estimate that prefix at PREFIX_ESTIMATE bytes.
 */
#define IRC_LINE_MAX		512
#define IRC_CRLF_LEN		2
#define PREFIX_ESTIMATE		80

/*
 * Color state tracked while walking the message.
 * ansi_active: nonzero when inside an ANSI 256-color span.
 * ansi_idx:    the palette index N from ESC[38;5;Nm.
 * mirc_fg/bg:  mIRC color codes (-1 = not set).
 * bold/under/reverse: toggle state.
 */
struct color_state {
	int	ansi_active;
	int	ansi_idx;
	int	mirc_fg;
	int	mirc_bg;
	int	bold;
	int	underline;
	int	reverse;
};

static	void	color_state_init _((struct color_state *));
static	int	color_state_active _((struct color_state *));
static	int	write_color_reopen _((struct color_state *, char *, int));
static	int	write_color_reset _((struct color_state *, char *, int));
static	int	is_ansi_start _((char *, int *));
static	int	is_ansi_reset _((char *, int *));
static	int	is_mirc_color _((char *, int *, int *, int *));

static void
color_state_init(cs)
	struct color_state	*cs;
{
	cs->ansi_active = 0;
	cs->ansi_idx = 0;
	cs->mirc_fg = -1;
	cs->mirc_bg = -1;
	cs->bold = 0;
	cs->underline = 0;
	cs->reverse = 0;
}

static int
color_state_active(cs)
	struct color_state	*cs;
{
	return cs->ansi_active || cs->mirc_fg >= 0 || cs->bold ||
	       cs->underline || cs->reverse;
}

/*
 * write_color_reopen: emit escape sequences to re-establish the color
 * state at the start of a continuation chunk.  Returns bytes written.
 */
static int
write_color_reopen(cs, buf, buflen)
	struct color_state	*cs;
	char			*buf;
	int			buflen;
{
	int	pos = 0;

	if (cs->bold && pos < buflen)
		buf[pos++] = BOLD_TOG;
	if (cs->underline && pos < buflen)
		buf[pos++] = UND_TOG;
	if (cs->reverse && pos < buflen)
		buf[pos++] = REV_TOG;
	if (cs->ansi_active && pos + 12 < buflen)
	{
		pos += snprintf(buf + pos, (size_t)(buflen - pos),
				"\033[38;5;%dm", cs->ansi_idx);
	}
	else if (cs->mirc_fg >= 0 && pos + 6 < buflen)
	{
		if (cs->mirc_bg >= 0)
			pos += snprintf(buf + pos, (size_t)(buflen - pos),
					"\003%d,%d", cs->mirc_fg, cs->mirc_bg);
		else
			pos += snprintf(buf + pos, (size_t)(buflen - pos),
					"\003%d", cs->mirc_fg);
	}
	return pos;
}

/*
 * write_color_reset: emit the minimal reset sequence to close active
 * color state at the end of a chunk.  Returns bytes written.
 */
static int
write_color_reset(cs, buf, buflen)
	struct color_state	*cs;
	char			*buf;
	int			buflen;
{
	int	pos = 0;

	if (cs->ansi_active && pos + 5 <= buflen)
	{
		memcpy(buf + pos, "\033[39m", 5);
		pos += 5;
	}
	if (cs->mirc_fg >= 0 && pos + 1 <= buflen)
		buf[pos++] = '\003';
	if ((cs->bold || cs->underline || cs->reverse) && pos + 1 <= buflen)
		buf[pos++] = ALL_OFF;
	return pos;
}

/*
 * is_ansi_start: check if `text` begins with ESC[38;5;Nm.
 * If so, sets *skip to the escape length and returns the color index.
 * Returns -1 if not a match.
 */
static int
is_ansi_start(text, skip)
	char	*text;
	int	*skip;
{
	int	idx = 0;
	int	pos = 0;

	if (text[0] != '\033' || text[1] != '[')
		return -1;
	pos = 2;
	if (text[pos] != '3' || text[pos + 1] != '8' ||
	    text[pos + 2] != ';' || text[pos + 3] != '5' ||
	    text[pos + 4] != ';')
		return -1;
	pos = 7;
	if (text[pos] < '0' || text[pos] > '9')
		return -1;
	while (text[pos] >= '0' && text[pos] <= '9')
	{
		idx = idx * 10 + (text[pos] - '0');
		pos++;
	}
	if (text[pos] != 'm')
		return -1;
	*skip = pos + 1;
	return idx;
}

/*
 * is_ansi_reset: check if `text` begins with ESC[39m.
 * Sets *skip to the escape length (5).  Returns 1 on match, 0 otherwise.
 */
static int
is_ansi_reset(text, skip)
	char	*text;
	int	*skip;
{
	if (text[0] == '\033' && text[1] == '[' && text[2] == '3' &&
	    text[3] == '9' && text[4] == 'm')
	{
		*skip = 5;
		return 1;
	}
	return 0;
}

/*
 * is_mirc_color: check if `text` starts with \003 followed by optional
 * digits (fg[,bg]).  Sets *fg, *bg (or -1), and *skip.
 * Returns 1 if a color code was found, 0 if just a bare \003 reset.
 */
static int
is_mirc_color(text, fg, bg, skip)
	char	*text;
	int	*fg;
	int	*bg;
	int	*skip;
{
	int	pos = 1;	/* past the \003 */
	int	val;

	*fg = -1;
	*bg = -1;
	if (text[pos] < '0' || text[pos] > '9')
	{
		*skip = 1;
		return 0;
	}
	val = text[pos++] - '0';
	if (text[pos] >= '0' && text[pos] <= '9')
		val = val * 10 + (text[pos++] - '0');
	*fg = val;
	if (text[pos] == ',')
	{
		pos++;
		if (text[pos] >= '0' && text[pos] <= '9')
		{
			val = text[pos++] - '0';
			if (text[pos] >= '0' && text[pos] <= '9')
				val = val * 10 + (text[pos++] - '0');
			*bg = val;
		}
	}
	*skip = pos;
	return 1;
}

/*
 * max_reopen_len: upper bound on bytes needed to re-establish color state.
 * ESC[38;5;NNNm = 12,  \003NN,NN = 6,  toggles = 3.
 */
#define MAX_REOPEN_LEN	16

/*
 * split_and_send: split `text` into chunks that fit within the IRC line
 * limit when sent as `command target :text\r\n`, and send each chunk
 * via send_to_server.
 *
 * command : "PRIVMSG" or "NOTICE"
 * target  : nick or channel (comma-separated list)
 * text    : the message body to split
 */
void
split_and_send(command, target, text)
	char	*command;
	char	*target;
	char	*text;
{
	int	overhead;
	int	max_text;
	int	textlen;
	struct color_state	cs;
	char	chunk[IRC_LINE_MAX];
	int	chunk_pos;
	int	src;
	int	src_iter;
	int	content_start;
	int	last_space;
	int	last_space_src;
	int	need_reopen;
	struct color_state	chunk_start_cs;
	int	skip;
	int	idx;
	int	fg, bg;

	if (!command || !target || !text || !*text)
		return;

	/*
	 * overhead = len("PRIVMSG ") + len(target) + len(" :") + CRLF + prefix
	 *          = strlen(command) + 1 + strlen(target) + 2 + 2 + PREFIX_ESTIMATE
	 */
	overhead = (int) strlen(command) + 1 + (int) strlen(target) + 2 +
		   IRC_CRLF_LEN + PREFIX_ESTIMATE;
	max_text = IRC_LINE_MAX - overhead;
	/* keep at least MAX_REOPEN_LEN*3 so that, after a colour reopen of up
	 * to MAX_REOPEN_LEN bytes, the inner loop can still place payload and
	 * the walk always advances (else the outer loop cannot make progress) */
	if (max_text < MAX_REOPEN_LEN * 3)
		max_text = MAX_REOPEN_LEN * 3;

	textlen = (int) strlen(text);
	if (textlen <= max_text)
	{
		send_to_server("%s %s :%s", command, target, text);
		return;
	}

	color_state_init(&cs);
	src = 0;

	while (text[src])
	{
		chunk_pos = 0;
		src_iter = src;
		last_space = -1;
		last_space_src = -1;
		need_reopen = color_state_active(&cs);
		chunk_start_cs = cs;

		if (need_reopen)
		{
			chunk_pos = write_color_reopen(&cs, chunk,
						       (int) sizeof(chunk));
		}
		content_start = chunk_pos;

		while (text[src] && chunk_pos < max_text - MAX_REOPEN_LEN)
		{
			/* check for ANSI color start: ESC[38;5;Nm */
			if (text[src] == '\033')
			{
				idx = is_ansi_start(text + src, &skip);
				if (idx >= 0)
				{
					if (chunk_pos + skip >= max_text - MAX_REOPEN_LEN &&
					    chunk_pos > content_start)
						break;
					memcpy(chunk + chunk_pos, text + src, (size_t) skip);
					chunk_pos += skip;
					src += skip;
					cs.ansi_active = 1;
					cs.ansi_idx = idx;
					continue;
				}
				if (is_ansi_reset(text + src, &skip))
				{
					if (chunk_pos + skip >= max_text - MAX_REOPEN_LEN &&
					    chunk_pos > content_start)
						break;
					memcpy(chunk + chunk_pos, text + src, (size_t) skip);
					chunk_pos += skip;
					src += skip;
					cs.ansi_active = 0;
					continue;
				}
			}

			/* check for mIRC color code */
			if (text[src] == '\003')
			{
				if (is_mirc_color(text + src, &fg, &bg, &skip))
				{
					if (chunk_pos + skip >= max_text - MAX_REOPEN_LEN &&
					    chunk_pos > content_start)
						break;
					memcpy(chunk + chunk_pos, text + src, (size_t) skip);
					chunk_pos += skip;
					src += skip;
					cs.mirc_fg = fg;
					cs.mirc_bg = bg;
					continue;
				}
				else
				{
					/* bare \003 = color reset */
					chunk[chunk_pos++] = text[src++];
					cs.mirc_fg = -1;
					cs.mirc_bg = -1;
					continue;
				}
			}

			/* bold, underline, reverse toggles */
			if (text[src] == BOLD_TOG)
			{
				chunk[chunk_pos++] = text[src++];
				cs.bold = !cs.bold;
				continue;
			}
			if (text[src] == UND_TOG)
			{
				chunk[chunk_pos++] = text[src++];
				cs.underline = !cs.underline;
				continue;
			}
			if (text[src] == REV_TOG)
			{
				chunk[chunk_pos++] = text[src++];
				cs.reverse = !cs.reverse;
				continue;
			}
			if (text[src] == ALL_OFF)
			{
				chunk[chunk_pos++] = text[src++];
				color_state_init(&cs);
				continue;
			}

			/* track last word boundary for clean splitting */
			if (text[src] == ' ')
			{
				last_space = chunk_pos;
				last_space_src = src;
			}

			/* regular byte */
			chunk[chunk_pos++] = text[src++];
		}

		/* forward-progress backstop: if this iteration consumed no source
		 * (e.g. a colour escape larger than the remaining budget), copy one
		 * byte so the outer walk can never spin regardless of constants */
		if (src == src_iter && text[src])
			chunk[chunk_pos++] = text[src++];

		/*
		 * If there is more text and we found a word boundary in the
		 * last third of the chunk, split there instead of mid-word.
		 */
		if (text[src] && last_space >= 0 &&
		    last_space > (chunk_pos * 2 / 3))
		{
			chunk_pos = last_space;
			src = last_space_src + 1;

			/* recompute color state up to the split point by
			 * re-scanning from chunk start -- this is O(chunk)
			 * but chunks are small and correctness matters. */
			cs = chunk_start_cs;
			{
				int	i = 0;
				int	cs_skip;

				if (need_reopen)
				{
					char	tmp[MAX_REOPEN_LEN + 1];
					i = write_color_reopen(
							&chunk_start_cs,
							tmp,
							(int) sizeof(tmp));
				}
				while (i < chunk_pos)
				{
					if (chunk[i] == '\033')
					{
						idx = is_ansi_start(chunk + i,
								    &cs_skip);
						if (idx >= 0)
						{
							cs.ansi_active = 1;
							cs.ansi_idx = idx;
							i += cs_skip;
							continue;
						}
						if (is_ansi_reset(chunk + i,
								  &cs_skip))
						{
							cs.ansi_active = 0;
							i += cs_skip;
							continue;
						}
					}
					if (chunk[i] == '\003')
					{
						if (is_mirc_color(chunk + i,
								  &fg, &bg,
								  &cs_skip))
						{
							cs.mirc_fg = fg;
							cs.mirc_bg = bg;
							i += cs_skip;
							continue;
						}
						else
						{
							cs.mirc_fg = -1;
							cs.mirc_bg = -1;
							i++;
							continue;
						}
					}
					if (chunk[i] == BOLD_TOG)
					{
						cs.bold = !cs.bold;
						i++;
						continue;
					}
					if (chunk[i] == UND_TOG)
					{
						cs.underline = !cs.underline;
						i++;
						continue;
					}
					if (chunk[i] == REV_TOG)
					{
						cs.reverse = !cs.reverse;
						i++;
						continue;
					}
					if (chunk[i] == ALL_OFF)
					{
						color_state_init(&cs);
						i++;
						continue;
					}
					i++;
				}
			}
		}

		/* append color reset if color state is active */
		if (color_state_active(&cs) && text[src])
		{
			chunk_pos += write_color_reset(&cs, chunk + chunk_pos,
						       (int)(sizeof(chunk) - (size_t) chunk_pos));
		}

		chunk[chunk_pos] = '\0';
		send_to_server("%s %s :%s", command, target, chunk);
	}
}
