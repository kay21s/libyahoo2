/*
 * libyahoo2: yahoo_httplib.h
 *
 * Copyright (C) 2002, Philip S Tellis <philip . tellis AT gmx . net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef YAHOO_HTTPLIB_H
#define YAHOO_HTTPLIB_H

#include "yahoo2_types.h"

int yahoo_tcp_readline(char *ptr, int maxlen, int fd);
char *yahoo_urlencode(const char *instr);
char *yahoo_urldecode(const char *instr);
int yahoo_http_post(char *url, struct yahoo_data *yd, long size);
int yahoo_http_get(char *url, struct yahoo_data *yd);
int yahoo_get_url_fd(char *url, struct yahoo_data *yd, 
		char *filename, unsigned long *filesize);

#endif
