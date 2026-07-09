/*
 * colorhash.h: deterministic nick/channel colorizer
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

#ifndef __colorhash_h_
#define __colorhash_h_

/*
 * Buffer sizes for callers that declare local coloring buffers.
 * Nick: >= 30-char nick + 18-byte escape overhead.
 * Channel: up to ~220-char name + 18-byte escape overhead.
 */
#define COLORIZED_NICK_LEN	64
#define COLORIZED_CHANNEL_LEN	240

/* escape overhead: ESC[38;5;NNNm (max 12) + ESC[39m (5) + NUL (1) = 18 */
#define COLOR_ESCAPE_OVERHEAD	18

char	*colorize_nick     _((char *, char *, int));
char	*colorize_channel  _((char *, char *, int));
char	*colorize_and_pad  _((char *, int, char *, int, int, int));
char	*colorize_at_nicks _((char *, char *, int));

#endif /* __colorhash_h_ */
