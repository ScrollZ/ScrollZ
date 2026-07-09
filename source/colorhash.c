/*
 * colorhash.c: deterministic nick/channel colorizer using FNV-1a hash
 *              and a curated xterm-256 palette.
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
#include "vars.h"
#include "ircaux.h"
#include "myvars.h"
#include "colorhash.h"

/*
 * Curated xterm-256 palette for dark terminals.  72 entries from the
 * color cube (indices 16-231) spread evenly across all twelve hue
 * families, each chosen for >= 3:1 WCAG contrast on black, dark grey,
 * and Solarized Dark backgrounds with saturation >= 0.35.  A dark
 * background is assumed, which admits the bright warm end of the cube:
 * yellows, ambers, limes and vivid greens carry the highest luminance
 * here and give the hash far more room to separate nicks.  The basic
 * 16 colors, the grayscale ramp 232-255, and washed-out low-saturation
 * tones are excluded.  Perceptual spread verified via CIELAB.
 */
#ifdef WANTANSI
static const unsigned char nick_palette[] = {
	167, 174, 196, 203, 204,		/* reds */
	130, 180, 202, 209, 214, 215,		/* oranges */
	106, 136, 143, 190, 220, 221, 228,	/* ambers/yellows */
	 70,  82, 107, 154, 156, 191,		/* chartreuse/lime */
	 34,  40,  46,  71,  83, 120,		/* greens */
	 35,  41,  47,  49,  72, 122,		/* spring/mint */
	 31,  36,  44,  45,  50,  73, 123,	/* teals/cyans */
	 33,  39,  67,  68,  74,  81,		/* azures */
	 63,  69, 104, 105,			/* blues */
	 97,  99, 129, 134, 140, 141,		/* violets */
	163, 164, 165, 171, 176, 213,		/* purples */
	168, 175, 197, 198, 199, 205, 206	/* magentas/pinks */
};
#define PALETTE_SIZE	(sizeof(nick_palette) / sizeof(nick_palette[0]))
#endif /* WANTANSI */

/* COLOR_ESCAPE_OVERHEAD is now in colorhash.h */

#ifdef WANTANSI
static	unsigned char	nick_color_index _((char *));
#endif /* WANTANSI */

/*
 * nick_color_index: FNV-1a 32-bit hash over case-folded name, reduced
 * modulo palette size.  COLORIZE_WARP folds in 4 extra bytes to shift
 * every assignment deterministically; warp==0 is a no-op.
 */
#ifdef WANTANSI
static unsigned char
nick_color_index(name)
	char	*name;
{
	unsigned int	hash = 2166136261u;
	unsigned char	*p;
	int	warp;
	unsigned int	w;
	int	i;

	for (p = (unsigned char *) name; *p; p++)
	{
		hash ^= (unsigned int) (unsigned char) tolower((int) *p);
		hash *= 16777619u;
	}
	warp = get_int_var(COLORIZE_WARP_VAR);
	if (warp != 0)
	{
		w = (unsigned int) warp;
		for (i = 0; i < 4; i++)
		{
			hash ^= (w >> (i * 8)) & 0xffU;
			hash *= 16777619u;
		}
	}
	return nick_palette[hash % PALETTE_SIZE];
}
#endif /* WANTANSI */

/*
 * colorize_nick: wrap nick with deterministic xterm-256 fg color escape.
 * Falls back to a plain copy when COLORIZE_NICKS is OFF, WANTANSI is
 * not compiled, or the name would not fit with escape overhead.
 */
char *
colorize_nick(name, buf, buflen)
	char	*name;
	char	*buf;
	int	buflen;
{
	if (buflen <= 0)
		return buf;
	buf[0] = '\0';
	if (!name || !*name)
		return buf;
#ifdef WANTANSI
	if (get_int_var(COLORIZE_NICKS_VAR))
	{
		if ((int) strlen(name) + COLOR_ESCAPE_OVERHEAD <= buflen)
		{
			unsigned char idx = nick_color_index(name);
			snprintf(buf, (size_t) buflen,
				 "\033[38;5;%um%s\033[39m",
				 (unsigned int) idx, name);
			return buf;
		}
	}
#endif /* WANTANSI */
	strmcpy(buf, name, buflen);
	return buf;
}

/*
 * colorize_channel: wrap channel name with deterministic xterm-256 fg
 * color escape.  Falls back to a plain copy when COLORIZE_CHANNELS is
 * OFF, WANTANSI is not compiled, or the name would not fit with escape
 * overhead.
 */
char *
colorize_channel(name, buf, buflen)
	char	*name;
	char	*buf;
	int	buflen;
{
	if (buflen <= 0)
		return buf;
	buf[0] = '\0';
	if (!name || !*name)
		return buf;
#ifdef WANTANSI
	if (get_int_var(COLORIZE_CHANNELS_VAR))
	{
		if ((int) strlen(name) + COLOR_ESCAPE_OVERHEAD <= buflen)
		{
			unsigned char idx = nick_color_index(name);
			snprintf(buf, (size_t) buflen,
				 "\033[38;5;%um%s\033[39m",
				 (unsigned int) idx, name);
			return buf;
		}
	}
#endif /* WANTANSI */
	strmcpy(buf, name, buflen);
	return buf;
}

/*
 * colorize_and_pad: colorize the plain name (correct hash), then
 * right-pad with spaces to reach field_width visible characters.
 * is_nick selects nick vs channel colouring and the matching SET
 * variable; truncate cuts the visible text to field_width (matching a
 * stock %-N.Ns format) while a clear truncate matches a plain %-Ns
 * (pad only).  The full name is always hashed, so a truncated field
 * keeps the same colour as the full name shown elsewhere.
 */
char *
colorize_and_pad(name, field_width, buf, buflen, is_nick, truncate)
	char	*name;
	int	field_width;
	char	*buf;
	int	buflen;
	int	is_nick;
	int	truncate;
{
	int	vw, pad, clen;
	char	cbuf[COLORIZED_CHANNEL_LEN];

	if (buflen <= 0)
		return buf;
	buf[0] = '\0';
	if (!name)
		return buf;
	vw = (int) strlen(name);
	if (truncate && vw > field_width)
	{
		char trunc[COLORIZED_CHANNEL_LEN - COLOR_ESCAPE_OVERHEAD];
		int tlen = field_width < (int) sizeof(trunc) - 1
			 ? field_width : (int) sizeof(trunc) - 1;

		if (tlen < 0)
			tlen = 0;
		memcpy(trunc, name, (size_t) tlen);
		trunc[tlen] = '\0';
#ifdef WANTANSI
		if (get_int_var(is_nick ? COLORIZE_NICKS_VAR
					 : COLORIZE_CHANNELS_VAR) &&
		    tlen + COLOR_ESCAPE_OVERHEAD <= (int) sizeof(cbuf))
			snprintf(cbuf, sizeof(cbuf), "\033[38;5;%um%s\033[39m",
				 (unsigned int) nick_color_index(name), trunc);
		else
#endif /* WANTANSI */
			strmcpy(cbuf, trunc, sizeof(cbuf));
		vw = tlen;
	}
	else if (is_nick)
		colorize_nick(name, cbuf, (int) sizeof(cbuf));
	else
		colorize_channel(name, cbuf, (int) sizeof(cbuf));
	clen = (int) strlen(cbuf);
	if (clen >= buflen)
		clen = buflen - 1;
	pad = (field_width > vw) ? field_width - vw : 0;
	if (clen + pad >= buflen)
		pad = buflen - clen - 1;
	if (pad < 0)
		pad = 0;
	memcpy(buf, cbuf, (size_t) clen);
	if (pad > 0)
		memset(buf + clen, ' ', (size_t) pad);
	buf[clen + pad] = '\0';
	return buf;
}


#ifdef WANTANSI
static int
is_nick_char(c)
	int	c;
{
	if (isalnum(c))
		return 1;
	switch (c)
	{
	case '[': case ']': case '\\': case '^':
	case '{': case '|': case '}': case '-': case '_':
		return 1;
	}
	return 0;
}
#endif /* WANTANSI */

/*
 * colorize_at_nicks: scan text for @word patterns where @ is preceded
 * by whitespace (or at start of string) and wrap the word portion with
 * nick colorization.  Returns buf.
 */
char *
colorize_at_nicks(text, buf, buflen)
	char	*text;
	char	*buf;
	int	buflen;
{
	if (buflen <= 0)
		return buf;
	buf[0] = '\0';
	if (!text || !*text)
	{
		if (text)
			strmcpy(buf, text, buflen);
		return buf;
	}
#ifdef WANTANSI
	if (get_int_var(COLORIZE_NICKS_VAR))
	{
		char	*src = text;
		char	*dst = buf;
		int	remain = buflen - 1;
		int	at_boundary = 1;

		while (*src && remain > 0)
		{
			if (*src == '@' && at_boundary)
			{
				char	*start = src + 1;
				char	*end = start;
				int	nlen;
				char	word[64];
				char	colored[COLORIZED_NICK_LEN];
				int	clen;

				while (*end && is_nick_char(
						(unsigned char) *end))
					end++;
				nlen = (int)(end - start);
				if (nlen > 0 &&
				    nlen < (int) sizeof(word))
				{
					memcpy(word, start, (size_t) nlen);
					word[nlen] = '\0';
					colorize_nick(word, colored,
						(int) sizeof(colored));
					clen = (int) strlen(colored);
					if (1 + clen <= remain)
					{
						*dst++ = '@';
						remain--;
						memcpy(dst, colored,
							(size_t) clen);
						dst += clen;
						remain -= clen;
						src = end;
						at_boundary = 0;
						continue;
					}
				}
			}
			at_boundary = (*src == ' ' || *src == '\t');
			if (remain > 0)
			{
				*dst++ = *src++;
				remain--;
			}
		}
		*dst = '\0';
		return buf;
	}
#endif /* WANTANSI */
	strmcpy(buf, text, buflen);
	return buf;
}
