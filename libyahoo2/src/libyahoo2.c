/*
 * libyahoo2: libyahoo2.c
 *
 * Some code copyright (C) 2002, Philip S Tellis <philip . tellis AT gmx . net>
 *
 * Much of this code was taken and adapted from the yahoo module for
 * gaim released under the GNU GPL.  This code is also released under the 
 * GNU GPL.
 *
 * This code is derivitive of Gaim <http://gaim.sourceforge.net>
 * copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 *	       1998-1999, Adam Fritzler <afritz@marko.net>
 *	       1998-2002, Rob Flynn <rob@marko.net>
 *	       2000-2002, Eric Warmenhoven <eric@warmenhoven.org>
 *	       2001-2002, Brian Macke <macke@strangelove.net>
 *		    2001, Anand Biligiri S <abiligiri@users.sf.net>
 *		    2001, Valdis Kletnieks
 *		    2002, Sean Egan <bj91704@binghamton.edu>
 *		    2002, Toby Gray <toby.gray@ntlworld.com>
 *
 * This library also uses code from other libraries, namely:
 *     Portions from libfaim copyright 1998, 1999 Adam Fritzler
 *     <afritz@auk.cx>
 *     Portions of Sylpheed copyright 2000-2002 Hiroyuki Yamamoto
 *     <hiro-y@kcn.ne.jp>
 *
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include <sys/types.h>

#include <stdlib.h>
#include <ctype.h>

#include "md5.h"
#include "yahoo2.h"
#include "yahoo_connections.h"
#include "yahoo_httplib.h"
#include "yahoo_util.h"

#include "yahoo2_callbacks.h"
#include "yahoo_debug.h"

#ifdef USE_STRUCT_CALLBACKS
struct yahoo_callbacks *yc=NULL;

void yahoo_register_callbacks(struct yahoo_callbacks * tyc)
{
	yc = tyc;
}

#define YAHOO_CALLBACK(x)	yc->x
#else
#define YAHOO_CALLBACK(x)	x
#endif

int yahoo_log_message(char * fmt, ...)
{
	char out[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(out, sizeof(out), fmt, ap);
	va_end(ap);
	return YAHOO_CALLBACK(ext_yahoo_log)(out);
}

int yahoo_connect(char * host, int port)
{
	return YAHOO_CALLBACK(ext_yahoo_connect)(host, port);
}

extern char pager_host[];
extern char pager_port[];
static int fallback_ports[]={23, 25, 80, 5050, 0};
extern char filetransfer_host[];
extern char filetransfer_port[];
extern char webcam_host[];
extern char webcam_port[];

static char profile_url[] = "http://profiles.yahoo.com/";

enum yahoo_service { /* these are easier to see in hex */
	YAHOO_SERVICE_LOGON = 1,
	YAHOO_SERVICE_LOGOFF,
	YAHOO_SERVICE_ISAWAY,
	YAHOO_SERVICE_ISBACK,
	YAHOO_SERVICE_IDLE, /* 5 (placemarker) */
	YAHOO_SERVICE_MESSAGE,
	YAHOO_SERVICE_IDACT,
	YAHOO_SERVICE_IDDEACT,
	YAHOO_SERVICE_MAILSTAT,
	YAHOO_SERVICE_USERSTAT, /* 0xa */
	YAHOO_SERVICE_NEWMAIL,
	YAHOO_SERVICE_CHATINVITE,
	YAHOO_SERVICE_CALENDAR,
	YAHOO_SERVICE_NEWPERSONALMAIL,
	YAHOO_SERVICE_NEWCONTACT,
	YAHOO_SERVICE_ADDIDENT, /* 0x10 */
	YAHOO_SERVICE_ADDIGNORE,
	YAHOO_SERVICE_PING,
	YAHOO_SERVICE_GOTGROUPRENAME, /* < 1, 36(old), 37(new) */
	YAHOO_SERVICE_SYSMESSAGE = 0x14,
	YAHOO_SERVICE_PASSTHROUGH2 = 0x16,
	YAHOO_SERVICE_CONFINVITE = 0x18,
	YAHOO_SERVICE_CONFLOGON,
	YAHOO_SERVICE_CONFDECLINE,
	YAHOO_SERVICE_CONFLOGOFF,
	YAHOO_SERVICE_CONFADDINVITE,
	YAHOO_SERVICE_CONFMSG,
	YAHOO_SERVICE_CHATLOGON,
	YAHOO_SERVICE_CHATLOGOFF,
	YAHOO_SERVICE_CHATMSG = 0x20,
	YAHOO_SERVICE_GAMELOGON = 0x28,
	YAHOO_SERVICE_GAMELOGOFF,
	YAHOO_SERVICE_GAMEMSG = 0x2a,
	YAHOO_SERVICE_FILETRANSFER = 0x46,
	YAHOO_SERVICE_VOICECHAT = 0x4A,
	YAHOO_SERVICE_NOTIFY,
	YAHOO_SERVICE_VERIFY,
	YAHOO_SERVICE_P2PFILEXFER,
	YAHOO_SERVICE_PEERTOPEER = 0x4F,	/* Checks if P2P possible */
	YAHOO_SERVICE_WEBCAM,
	YAHOO_SERVICE_AUTHRESP = 0x54,
	YAHOO_SERVICE_LIST,
	YAHOO_SERVICE_AUTH = 0x57,
	YAHOO_SERVICE_ADDBUDDY = 0x83,
	YAHOO_SERVICE_REMBUDDY,
	YAHOO_SERVICE_IGNORECONTACT,	/* > 1, 7, 13 < 1, 66, 13, 0*/
	YAHOO_SERVICE_REJECTCONTACT,
	YAHOO_SERVICE_GROUPRENAME = 0x89, /* > 1, 65(new), 66(0), 67(old) */
	YAHOO_SERVICE_CHATONLINE = 0x96, /* > 109(id), 1, 6(abcde) < 0,1*/
	YAHOO_SERVICE_CHATGOTO,
	YAHOO_SERVICE_CHATJOIN,	/* > 1 104-room 129-1600326591 62-2 */
	YAHOO_SERVICE_CHATLEAVE,
	YAHOO_SERVICE_CHATEXIT = 0x9b,
	YAHOO_SERVICE_CHATLOGOUT = 0xa0,
	YAHOO_SERVICE_CHATPING,
	YAHOO_SERVICE_COMMENT = 0xa8
};

struct yahoo_pair {
	int key;
	char *value;
};

struct yahoo_packet {
	unsigned short int service;
	unsigned int status;
	unsigned int id;
	YList *hash;
};

static int last_id=0;

extern char *yahoo_crypt(char *, char *);

enum yahoo_log_level log_level = YAHOO_LOG_NONE;

int yahoo_set_log_level(enum yahoo_log_level level)
{
	enum yahoo_log_level l = log_level;
	log_level = level;
	return l;
}

/* Free a buddy list */
static void yahoo_free_buddies(YList * list)
{
	YList *l;

	for(l = list; l; l = l->next)
	{
		struct yahoo_buddy *bud = l->data;
		if(!bud)
			continue;

		FREE(bud->group);
		FREE(bud->id);
		FREE(bud->real_name);
		if(bud->yab_entry) {
			FREE(bud->yab_entry->fname);
			FREE(bud->yab_entry->lname);
			FREE(bud->yab_entry->nname);
			FREE(bud->yab_entry->id);
			FREE(bud->yab_entry->email);
			FREE(bud->yab_entry->hphone);
			FREE(bud->yab_entry->wphone);
			FREE(bud->yab_entry->mphone);
			FREE(bud->yab_entry);
		}
		FREE(bud);
		l->data = bud = NULL;
	}

	y_list_free(list);
}

/* Free an identities list */
static void yahoo_free_identities(YList * list)
{
	YList * l;
	for (l = list; l; l=l->next) {
		FREE(l->data);
		l->data=NULL;
	}

	y_list_free(list);
}

/* Free webcam data */
static void yahoo_free_webcam(struct webcam *wcm)
{
	if (wcm) {
		FREE(wcm->user);
		FREE(wcm->server);
		FREE(wcm->key);
		FREE(wcm->description);
		FREE(wcm->my_ip);
	}
	FREE(wcm);
}

static void yahoo_free_data(struct yahoo_data *yd)
{
	FREE(yd->user);
	FREE(yd->password);
	FREE(yd->cookie_y);
	FREE(yd->cookie_t);
	FREE(yd->cookie_c);
	FREE(yd->login_cookie);
	FREE(yd->login_id);

	FREE(yd->rxqueue);
	yd->rxlen = 0;

	yahoo_free_buddies(yd->buddies);
	yahoo_free_buddies(yd->ignore);
	yahoo_free_identities(yd->identities);
	yahoo_free_webcam(yd->wcm);

	FREE(yd);
}

#define YAHOO_PACKET_HDRLEN (4 + 2 + 2 + 2 + 2 + 4 + 4)

static struct yahoo_packet *yahoo_packet_new(enum yahoo_service service, 
		enum yahoo_status status, int id)
{
	struct yahoo_packet *pkt = y_new0(struct yahoo_packet, 1);

	pkt->service = service;
	pkt->status = status;
	pkt->id = id;

	return pkt;
}

static void yahoo_packet_hash(struct yahoo_packet *pkt, int key, const char *value)
{
	struct yahoo_pair *pair = y_new0(struct yahoo_pair, 1);
	pair->key = key;
	pair->value = strdup(value);
	pkt->hash = y_list_append(pkt->hash, pair);
}

static int yahoo_packet_length(struct yahoo_packet *pkt)
{
	YList *l;

	int len = 0;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		int tmp = pair->key;
		do {
			tmp /= 10;
			len++;
		} while (tmp);
		len += 2;
		len += strlen(pair->value);
		len += 2;
	}

	return len;
}

#define yahoo_put16(buf, data) ( \
		(*(buf) = (unsigned char)((data)>>8)&0xff), \
		(*((buf)+1) = (unsigned char)(data)&0xff),  \
		2)
#define yahoo_get16(buf) ((((*(buf))&0xff)<<8) + ((*((buf)+1)) & 0xff))
#define yahoo_put32(buf, data) ( \
		(*((buf)) = (unsigned char)((data)>>24)&0xff), \
		(*((buf)+1) = (unsigned char)((data)>>16)&0xff), \
		(*((buf)+2) = (unsigned char)((data)>>8)&0xff), \
		(*((buf)+3) = (unsigned char)(data)&0xff), \
		4)
#define yahoo_get32(buf) ((((*(buf)   )&0xff)<<24) + \
			 (((*((buf)+1))&0xff)<<16) + \
			 (((*((buf)+2))&0xff)<< 8) + \
			 (((*((buf)+3))&0xff)))

static void yahoo_packet_read(struct yahoo_packet *pkt, unsigned char *data, int len)
{
	int pos = 0;

	while (pos + 1 < len) {
		char *key, *value = NULL;
		int accept;
		int x;

		struct yahoo_pair *pair = y_new0(struct yahoo_pair, 1);

		key = malloc(len + 1);
		x = 0;
		while (pos + 1 < len) {
			if (data[pos] == 0xc0 && data[pos + 1] == 0x80)
				break;
			key[x++] = data[pos++];
		}
		key[x] = 0;
		pos += 2;
		pair->key = strtol(key, NULL, 10);
		free(key);

		accept = x; 
		/* if x is 0 there was no key, so don't accept it */
		if (accept)
			value = malloc(len - pos + 1);
		x = 0;
		while (pos + 1 < len) {
			if (data[pos] == 0xc0 && data[pos + 1] == 0x80)
				break;
			if (accept)
				value[x++] = data[pos++];
		}
		if (accept)
			value[x] = 0;
		pos += 2;
		if (accept) {
			pair->value = strdup(value);
			FREE(value);
			pkt->hash = y_list_append(pkt->hash, pair);
			DEBUG_MSG(("Key: %d  \tValue: %s", pair->key, pair->value));
		} else {
			FREE(pair);
		}
	}
}

static void yahoo_packet_write(struct yahoo_packet *pkt, unsigned char *data)
{
	YList *l;
	int pos = 0;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		unsigned char buf[100];

		snprintf((char *)buf, sizeof(buf), "%d", pair->key);
		strcpy((char *)data + pos, (char *)buf);
		pos += strlen((char *)buf);
		data[pos++] = 0xc0;
		data[pos++] = 0x80;

		strcpy((char *)data + pos, pair->value);
		pos += strlen(pair->value);
		data[pos++] = 0xc0;
		data[pos++] = 0x80;
	}
}

static void yahoo_dump_unhandled(struct yahoo_packet *pkt)
{
	YList *l;

	NOTICE(("Service: 0x%02x\tStatus: %d", pkt->service, pkt->status));
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		NOTICE(("\t%d => %s", pair->key, pair->value));
	}
}


static void yahoo_packet_dump(unsigned char *data, int len)
{
	if(log_level >= YAHOO_LOG_DEBUG) {
		int i;
		for (i = 0; i < len; i++) {
			if ((i % 8 == 0) && i)
				YAHOO_CALLBACK(ext_yahoo_log)(" ");
			if ((i % 16 == 0) && i)
				YAHOO_CALLBACK(ext_yahoo_log)("\n");
			YAHOO_CALLBACK(ext_yahoo_log)("%02x ", data[i]);
		}
		YAHOO_CALLBACK(ext_yahoo_log)("\n");
		for (i = 0; i < len; i++) {
			if ((i % 8 == 0) && i)
				YAHOO_CALLBACK(ext_yahoo_log)(" ");
			if ((i % 16 == 0) && i)
				YAHOO_CALLBACK(ext_yahoo_log)("\n");
			if (isprint(data[i]))
				YAHOO_CALLBACK(ext_yahoo_log)(" %c ", data[i]);
			else
				YAHOO_CALLBACK(ext_yahoo_log)(" . ");
		}
		YAHOO_CALLBACK(ext_yahoo_log)("\n");
	}
}

static char base64digits[] = 	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz"
				"0123456789._";
static void to_y64(unsigned char *out, const unsigned char *in, int inlen)
/* raw bytes in quasi-big-endian order to base 64 string (NUL-terminated) */
{
	for (; inlen >= 3; inlen -= 3)
		{
			*out++ = base64digits[in[0] >> 2];
			*out++ = base64digits[((in[0]<<4) & 0x30) | (in[1]>>4)];
			*out++ = base64digits[((in[1]<<2) & 0x3c) | (in[2]>>6)];
			*out++ = base64digits[in[2] & 0x3f];
			in += 3;
		}
	if (inlen > 0)
		{
			unsigned char fragment;

			*out++ = base64digits[in[0] >> 2];
			fragment = (in[0] << 4) & 0x30;
			if (inlen > 1)
				fragment |= in[1] >> 4;
			*out++ = base64digits[fragment];
			*out++ = (inlen < 2) ? '-' 
					: base64digits[(in[1] << 2) & 0x3c];
			*out++ = '-';
		}
	*out = '\0';
}

static int yahoo_send_packet(struct yahoo_data *yd, struct yahoo_packet *pkt, int extra_pad)
{
	int pktlen = yahoo_packet_length(pkt);
	int len = YAHOO_PACKET_HDRLEN + pktlen;
	int ret;

	unsigned char *data;
	int pos = 0;

	if (yd->fd < 0)
		return -1;

	data = y_new0(unsigned char, len + 1);

	memcpy(data + pos, "YMSG", 4); pos += 4;
	pos += yahoo_put16(data + pos, 0x0900);
	pos += yahoo_put16(data + pos, 0x0000);
	pos += yahoo_put16(data + pos, pktlen + extra_pad);
	pos += yahoo_put16(data + pos, pkt->service);
	pos += yahoo_put32(data + pos, pkt->status);
	pos += yahoo_put32(data + pos, pkt->id);

	yahoo_packet_write(pkt, data + pos);

	yahoo_packet_dump(data, len);
	
	do {
		ret = write(yd->fd, data, len);
	} while(ret == -1 && errno==EINTR);

	LOG(("wrote packet"));

	FREE(data);

	return ret;
}

static void yahoo_packet_free(struct yahoo_packet *pkt)
{
	while (pkt->hash) {
		struct yahoo_pair *pair = pkt->hash->data;
		YList * l = pkt->hash;
		FREE(pair->value);
		FREE(pair);
		pkt->hash = y_list_remove_link(pkt->hash, pkt->hash);
		FREE(l);
	}
	FREE(pkt);
}

static int yahoo_send_data(struct yahoo_data *yd, char *data, int len)
{
	int ret;

	if (yd->fd < 0)
		return -1;

	yahoo_packet_dump((unsigned char*) data, len);

	do {
		ret = write(yd->fd, data, len);
	} while(ret == -1 && errno==EINTR);

	LOG(("wrote data"));

	return ret;
}

static void yahoo_close(struct yahoo_data *yd) 
{
	int id = yd->client_id;
	del_from_list(yd);

	if (yd->fd >= 0)
		close(yd->fd);
	FREE(yd->rxqueue);
	yd->rxlen = 0;
	yahoo_free_data(yd);
	if(id == last_id)
		last_id--;

}

static int is_same_bud(const void * a, const void * b) {
	const struct yahoo_buddy *subject = a;
	const struct yahoo_buddy *object = b;

	return strcmp(subject->id, object->id);
}

static YList * bud_str2list(char *rawlist)
{
	YList * l = NULL;

	char **lines;
	char **split;
	char **buddies;
	char **tmp, **bud;

	lines = y_strsplit(rawlist, "\n", -1);
	for (tmp = lines; *tmp; tmp++) {
		struct yahoo_buddy *newbud;

		split = y_strsplit(*tmp, ":", 2);
		if (!split)
			continue;
		if (!split[0] || !split[1]) {
			y_strfreev(split);
			continue;
		}
		buddies = y_strsplit(split[1], ",", -1);

		for (bud = buddies; bud && *bud; bud++) {
			newbud = y_new0(struct yahoo_buddy, 1);
			newbud->id = strdup(*bud);
			newbud->group = strdup(split[0]);

			if(y_list_find_custom(l, newbud, is_same_bud)) {
				FREE(newbud->id);
				FREE(newbud->group);
				FREE(newbud);
				continue;
			}

			newbud->real_name = NULL;

			l = y_list_append(l, newbud);

			NOTICE(("Added buddy %s to group %s", newbud->id, newbud->group));
		}

		y_strfreev(buddies);
		y_strfreev(split);
	}
	y_strfreev(lines);

	return l;
}

static char * getcookie(char *rawcookie)
{
	char * cookie=NULL;
	char * tmpcookie = strdup(rawcookie+2);
	char * cookieend = strchr(tmpcookie, ';');

	if(cookieend)
		*cookieend = '\0';

	cookie = strdup(tmpcookie);
	FREE(tmpcookie);
	cookieend=NULL;

	return cookie;
}

static char * getlcookie(char *cookie)
{
	char *tmp;
	char *tmpend;
	char *login_cookie = NULL;

	tmpend = strstr(cookie, "n=");
	if(tmpend) {
		tmp = strdup(tmpend+2);
		tmpend = strchr(tmp, '&');
		if(tmpend)
			*tmpend='\0';
		login_cookie = strdup(tmp);
		FREE(tmp);
	}

	return login_cookie;
}

static void yahoo_process_notify(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *msg = NULL;
	char *from = NULL;
	int stat = 0;
	int accept = 0;
	char *ind = NULL;
	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			from = pair->value;
		if (pair->key == 49)
			msg = pair->value;
		if (pair->key == 13)
			stat = atoi(pair->value);
		if (pair->key == 14)
			ind = pair->value;
		if (pair->key == 16) {	/* status == -1 */
			NOTICE((pair->value));
			return;
		}

	}

	if (!msg)
		return;
	
	if (!strncasecmp(msg, "TYPING", strlen("TYPING"))) 
		YAHOO_CALLBACK(ext_yahoo_typing_notify)(yd->client_id, from, stat);
	else if (!strncasecmp(msg, "GAME", strlen("GAME"))) 
		YAHOO_CALLBACK(ext_yahoo_game_notify)(yd->client_id, from, stat);
	else if (!strncasecmp(msg, "WEBCAMINVITE", strlen("WEBCAMINVITE"))) 
	{
		if (!strcmp(ind, " ")) {
			YAHOO_CALLBACK(ext_yahoo_webcam_invite)(yd->client_id, from);
		} else {
			accept = atoi(ind);
			/* accept the invitation (-1 = deny 1 = accept) */
			if (accept < 0)
				accept = 0;
			YAHOO_CALLBACK(ext_yahoo_webcam_invite_reply)(yd->client_id, from, accept);
		}
	}
	else
		LOG(("Got unknown notification: %s", msg));
}

static void yahoo_process_filetransfer(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *from=NULL;
	char *to=NULL;
	char *msg=NULL;
	char *url=NULL;
	long expires=0;

	char *service=NULL;

	char *filename=NULL;
	unsigned long filesize=0L;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			from = pair->value;
		if (pair->key == 5)
			to = pair->value;
		if (pair->key == 14)
			msg = pair->value;
		if (pair->key == 20)
			url = pair->value;
		if (pair->key == 38)
			expires = atol(pair->value);

		if (pair->key == 27)
			filename = pair->value;
		if (pair->key == 28)
			filesize = atol(pair->value);

		if (pair->key == 49)
			service = pair->value;
	}

	if(pkt->service == YAHOO_SERVICE_P2PFILEXFER) {
		if(strcmp("FILEXFER", service) != 0) {
			WARNING(("unhandled service 0x%02x", pkt->service));
			yahoo_dump_unhandled(pkt);
			return;
		}
	}

	if(msg) {
		char *tmp;
		tmp = strchr(msg, '\006');
		if(tmp)
			*tmp = '\0';
	}
	if(url && from)
		YAHOO_CALLBACK(ext_yahoo_got_file)(yd->client_id, from, url, expires, msg, filename, filesize);

}

static void yahoo_process_conference(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *msg = NULL;
	char *host = NULL;
	char *who = NULL;
	char *room = NULL;
	char *id = NULL;
	int  utf8 = 0;
	YList *members = NULL;
	YList *l;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 50)
			host = pair->value;
		
		if (pair->key == 52) {		/* invite */
			members = y_list_append(members, strdup(pair->value));
		}
		if (pair->key == 53)		/* logon */
			who = pair->value;
		if (pair->key == 54)		/* decline */
			who = pair->value;
		if (pair->key == 55)		/* unavailable (status == 2) */
			who = pair->value;
		if (pair->key == 56)		/* logoff */
			who = pair->value;

		if (pair->key == 57)
			room = pair->value;

		if (pair->key == 58)		/* join message */
			msg = pair->value;
		if (pair->key == 14)		/* decline/conf message */
			msg = pair->value;

		if (pair->key == 13)
			;
		if (pair->key == 16)		/* error */
			msg = pair->value;

		if (pair->key == 1)		/* my id */
			id = pair->value;
		if (pair->key == 3)		/* message sender */
			who = pair->value;

		if (pair->key == 97)
			utf8 = atoi(pair->value);
	}

	if(!room)
		return;

	if(host) {
		for(l = members; l; l = l->next) {
			char * w = l->data;
			if(!strcmp(w, host))
				break;
		}
		if(!l)
			members = y_list_append(members, strdup(host));
	}
	/* invite, decline, join, left, message -> status == 1 */

	switch(pkt->service) {
	case YAHOO_SERVICE_CONFINVITE:
		if(pkt->status == 2)
			;
		else if(members)
			YAHOO_CALLBACK(ext_yahoo_got_conf_invite)(yd->client_id, host, room, msg, members);
		else if(msg)
			YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, msg, 0);
		break;
	case YAHOO_SERVICE_CONFADDINVITE:
		if(pkt->status == 2)
			;
		else
			YAHOO_CALLBACK(ext_yahoo_got_conf_invite)(yd->client_id, host, room, msg, members);
		break;
	case YAHOO_SERVICE_CONFDECLINE:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_userdecline)(yd->client_id, who, room, msg);
		break;
	case YAHOO_SERVICE_CONFLOGON:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_userjoin)(yd->client_id, who, room);
		break;
	case YAHOO_SERVICE_CONFLOGOFF:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_userleave)(yd->client_id, who, room);
		break;
	case YAHOO_SERVICE_CONFMSG:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_conf_message)(yd->client_id, who, room, msg, utf8);
		break;
	}
}


static void yahoo_process_chat(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *msg = NULL;
	char *who = NULL;
	char *room = NULL;
	int  utf8 = 0;
	YList *l;
	
	yahoo_dump_unhandled(pkt);
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 109)		/* message sender */
			who = pair->value;

		if (pair->key == 117)		/* message */
			msg = pair->value;

		if (pair->key == 104)		/* Room name */
			room = pair->value;
	}

	if(!room)
		return;

	switch(pkt->service) {
	case YAHOO_SERVICE_CHATJOIN:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_chat_userjoin)(yd->client_id, who, room);
		break;
	case YAHOO_SERVICE_CHATEXIT:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_chat_userleave)(yd->client_id, who, room);
		break;
	case YAHOO_SERVICE_COMMENT:
		if(who)
			YAHOO_CALLBACK(ext_yahoo_chat_message)(yd->client_id, who, room, msg, utf8);
		break;
	}
}

static void yahoo_process_message(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	YList *l;
	YList * messages = NULL;

	struct m {
		int  i_31;
		int  i_32;
		char *to;
		char *from;
		long tm;
		char *msg;
		int  utf8;
	} *message = y_new0(struct m, 1);

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 1 || pair->key == 4)
			message->from = pair->value;
		else if (pair->key == 5)
			message->to = pair->value;
		else if (pair->key == 15)
			message->tm = strtol(pair->value, NULL, 10);
		else if (pair->key == 97)
			message->utf8 = atoi(pair->value);
			/* user message */  /* sys message */
		else if (pair->key == 14 || pair->key == 16)
			message->msg = pair->value;
		else if (pair->key == 31) {
			if(message->i_31) {
				messages = y_list_append(messages, message);
				message = y_new0(struct m, 1);
			}
			message->i_31 = atoi(pair->value);
		}
		else if (pair->key == 32)
			message->i_32 = atoi(pair->value);
		else
			LOG(("yahoo_process_message: status: %d, key: %d, value: %s",
					pkt->status, pair->key, pair->value));
	}

	messages = y_list_append(messages, message);

	for (l = messages; l; l=l->next) {
		message = l->data;
		if (pkt->service == YAHOO_SERVICE_SYSMESSAGE) {
			YAHOO_CALLBACK(ext_yahoo_system_message)(yd->client_id, message->msg);
		} else if (pkt->status <= 2 || pkt->status == 5) {
			YAHOO_CALLBACK(ext_yahoo_got_im)(yd->client_id, message->from, message->msg, message->tm, pkt->status, message->utf8);
		} else if (pkt->status == 0xffffffff) {
			YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, message->msg, 0);
		}
		free(message);
	}

	y_list_free(messages);
}


static void yahoo_process_status(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	YList *l;
	char *name = NULL;
	int state = 0;
	int away = 0;
	char *msg = NULL;
	
	if(pkt->service == YAHOO_SERVICE_LOGOFF && pkt->status == -1) {
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_DUPL, NULL);
		return;
	}

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;

		switch (pair->key) {
		case 0: /* we won't actually do anything with this */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 1: /* we don't get the full buddy list here. */
			if (!yd->logged_in) {
				yd->logged_in = TRUE;
				if(yd->current_status < 0)
					yd->current_status = yd->initial_status;
				YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_OK, NULL);
			}
			break;
		case 8: /* how many online buddies we have */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 7: /* the current buddy */
			name = pair->value;
			break;
		case 10: /* state */
			state = strtol(pair->value, NULL, 10);
			break;
		case 19: /* custom status message */
			msg = pair->value;
			break;
		case 47: /* is it an away message or not */
			away = atoi(pair->value);
			break;
		case 11: /* what is this? */
			NOTICE(("key %d:%s", pair->key, pair->value));
			break;
		case 17: /* in chat? */
			break;
		case 13: /* in pager? */
			if (pkt->service == YAHOO_SERVICE_LOGOFF || strtol(pair->value, NULL, 10) == 0) {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, YAHOO_STATUS_OFFLINE, NULL, 1);
				break;
			}
			if (state == YAHOO_STATUS_AVAILABLE) {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, state, NULL, 0);
			} else if (state == YAHOO_STATUS_CUSTOM) {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, state, msg, away);
			} else {
				YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, state, NULL, 1);
			}

			break;
		case 60: 
			/* sometimes going offline makes this 2, but invisible never sends it */
			NOTICE(("key %d:%s", pair->key, pair->value));
			 break;
		case 16: /* Custom error message */
			YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, pair->value, 0);
			break;
		default:
			WARNING(("unknown status key %d:%s", pair->key, pair->value));
			break;
		}
	}
}

static void yahoo_process_list(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	YList *l;

	if (!yd->logged_in) {
		yd->logged_in = TRUE;
		if(yd->current_status < 0)
			yd->current_status = yd->initial_status;
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, YAHOO_LOGIN_OK, NULL);
	}

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;

		switch(pair->key) {
		case 87: /* buddies */
			if(!yd->rawbuddylist)
				yd->rawbuddylist = strdup(pair->value);
			else {
				yd->rawbuddylist = y_string_append(yd->rawbuddylist, pair->value);
			}
			break;

		case 88: /* ignore list */
			if(!yd->ignorelist)
				yd->ignorelist = strdup("Ignore:");
			yd->ignorelist = y_string_append(yd->ignorelist, pair->value);
			break;

		case 89: /* identities */
			{
			char **identities = y_strsplit(pair->value, ",", -1);
			int i;
			for(i=0; identities[i]; i++)
				yd->identities = y_list_append(yd->identities, 
						strdup(identities[i]));
			y_strfreev(identities);
			}
			YAHOO_CALLBACK(ext_yahoo_got_identities)(yd->client_id, yd->identities);
			break;
		case 59: /* cookies add C cookie */
			if(yd->ignorelist) {
				yd->ignore = bud_str2list(yd->ignorelist);
				FREE(yd->ignorelist);
				YAHOO_CALLBACK(ext_yahoo_got_ignore)(yd->client_id, yd->ignore);
			}
			if(yd->rawbuddylist) {
				yd->buddies = bud_str2list(yd->rawbuddylist);
				FREE(yd->rawbuddylist);
				YAHOO_CALLBACK(ext_yahoo_got_buddies)(yd->client_id, yd->buddies);
			}

			if(pair->value[0]=='Y') {
				FREE(yd->cookie_y);
				FREE(yd->login_cookie);

				yd->cookie_y = getcookie(pair->value);
				yd->login_cookie = getlcookie(yd->cookie_y);

			} else if(pair->value[0]=='T') {
				FREE(yd->cookie_t);
				yd->cookie_t = getcookie(pair->value);

			} else if(pair->value[0]=='C') {
				FREE(yd->cookie_c);
				yd->cookie_c = getcookie(pair->value);
			} 
			if(yd->cookie_y && yd->cookie_t && yd->cookie_c)
				YAHOO_CALLBACK(ext_yahoo_got_cookies)(yd->client_id);

			break;
		case 3: /* my id */
		case 90: /* 1 */
		case 100: /* 0 */
		case 101: /* NULL */
		case 102: /* NULL */
		case 93: /* 86400/1440 */
			break;
		}
	}
}

static void yahoo_process_auth(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *seed = NULL;
	char *sn   = NULL;
	YList *l;
	
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 94)
			seed = pair->value;
		if (pair->key == 1)
			sn = pair->value;
	}
	
	if (!seed)
       		return;

	/* So, Yahoo has stopped supporting its older clients in India, and 
	 * undoubtedly will soon do so in the rest of the world.
	 * 
	 * The new clients use this authentication method.  I warn you in 
	 * advance, it's bizzare, convoluted, inordinately complicated.  
	 * It's also no more secure than crypt() was.  The only purpose this 
	 * scheme could serve is to prevent third part clients from connecting 
	 * to their servers.
	 *
	 * Sorry, Yahoo.
	 */
	{

	struct yahoo_packet *pack;
	
	md5_byte_t result[16];
	md5_state_t ctx;
	char *crypt_result;
	unsigned char *password_hash = malloc(25);
	unsigned char *crypt_hash = malloc(25);
	unsigned char *hash_string_p = malloc(50 + strlen(sn));
	unsigned char *hash_string_c = malloc(50 + strlen(sn));
	
	char checksum;
	
	int sv;
	
	unsigned char *result6 = malloc(25);
	unsigned char *result96 = malloc(25);

	sv = seed[15];
	sv = (sv % 8) % 5;

	md5_init(&ctx);
	md5_append(&ctx, (md5_byte_t *)yd->password, strlen(yd->password));
	md5_finish(&ctx, result);
	to_y64(password_hash, result, 16);
	
	md5_init(&ctx);
	crypt_result = yahoo_crypt(yd->password, "$1$_2S43d5f$");  
	md5_append(&ctx, (md5_byte_t *)crypt_result, strlen(crypt_result));
	md5_finish(&ctx, result);
	to_y64(crypt_hash, result, 16);

	switch (sv) {
	case 0:
		checksum = seed[seed[7] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, password_hash, yd->user, seed);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, crypt_hash, yd->user, seed);
		break;
	case 1:
		checksum = seed[seed[9] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, seed, password_hash);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, seed, crypt_hash);
		break;
	case 2:
		checksum = seed[seed[15] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, seed, password_hash, yd->user);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, seed, crypt_hash, yd->user);
		break;
	case 3:
		checksum = seed[seed[1] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, password_hash, seed);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, yd->user, crypt_hash, seed);
		break;
	case 4:
		checksum = seed[seed[3] % 16];
		snprintf((char *)hash_string_p, strlen(sn) + 50,
			"%c%s%s%s", checksum, password_hash, seed, yd->user);
		snprintf((char *)hash_string_c, strlen(sn) + 50,
			"%c%s%s%s", checksum, crypt_hash, seed, yd->user);
		break;
	}
		
	md5_init(&ctx);  
	md5_append(&ctx, (md5_byte_t *)hash_string_p, strlen((char *)hash_string_p));
	md5_finish(&ctx, result);
	to_y64(result6, result, 16);

	md5_init(&ctx);  
	md5_append(&ctx, (md5_byte_t *)hash_string_c, strlen((char *)hash_string_c));
	md5_finish(&ctx, result);
	to_y64(result96, result, 16);

	pack = yahoo_packet_new(YAHOO_SERVICE_AUTHRESP, yd->initial_status, 0);
	yahoo_packet_hash(pack, 0, yd->user);
	yahoo_packet_hash(pack, 6, (char *)result6);
	yahoo_packet_hash(pack, 96, (char *)result96);
	yahoo_packet_hash(pack, 1, yd->user);
		
	yahoo_send_packet(yd, pack, 0);
		
	FREE(result6);
	FREE(result96);
	FREE(password_hash);
	FREE(crypt_hash);
	FREE(hash_string_p);
	FREE(hash_string_c);

	yahoo_packet_free(pack);

	}
}

static void yahoo_process_auth_resp(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *login_id;
	char *handle;
	char *url=NULL;
	int  login_status=0;

	YList *l;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 0)
			login_id = pair->value;
		else if (pair->key == 1)
			handle = pair->value;
		else if (pair->key == 20)
			url = pair->value;
		else if (pair->key == 66)
			login_status = atoi(pair->value);
	}

	if(pkt->status == 0xffffffff) {
		YAHOO_CALLBACK(ext_yahoo_login_response)(yd->client_id, login_status, url);
	/*	yahoo_logoff(yd->client_id);*/
	}
}

static void yahoo_process_mail(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *email = NULL;
	char *subj = NULL;
	int count = 0;
	YList *l;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 9)
			count = strtol(pair->value, NULL, 10);
		else if (pair->key == 43)
			who = pair->value;
		else if (pair->key == 42)
			email = pair->value;
		else if (pair->key == 18)
			subj = pair->value;
		else
			LOG(("key: %d => value: %s", pair->key, pair->value));
	}

	if (who && email && subj) {
		char from[1024];
		snprintf(from, sizeof(from), "%s (%s)", who, email);
		YAHOO_CALLBACK(ext_yahoo_mail_notify)(yd->client_id, from, subj, count);
	} else if(count > 0)
		YAHOO_CALLBACK(ext_yahoo_mail_notify)(yd->client_id, NULL, NULL, count);
}

static void yahoo_process_contact(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *id = NULL;
	char *who = NULL;
	char *msg = NULL;
	char *name = NULL;
	long tm = 0L;
	int state = YAHOO_STATUS_AVAILABLE;
	int online = FALSE;
	int away = 0;

	YList *l;

	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 1)
			id = pair->value;
		else if (pair->key == 3)
			who = pair->value;
		else if (pair->key == 14)
			msg = pair->value;
		else if (pair->key == 7)
			name = pair->value;
		else if (pair->key == 10)
			state = strtol(pair->value, NULL, 10);
		else if (pair->key == 15)
			tm = strtol(pair->value, NULL, 10);
		else if (pair->key == 13)
			online = strtol(pair->value, NULL, 10);
		else if (pair->key == 47)
			away = strtol(pair->value, NULL, 10);
	}

	if (id)
		YAHOO_CALLBACK(ext_yahoo_contact_added)(yd->client_id, id, who, msg);
	else if (name)
		YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, name, state, msg, away);
	else if(pkt->status == 0x07)
		YAHOO_CALLBACK(ext_yahoo_rejected)(yd->client_id, who, msg);
}

static void yahoo_process_buddyadd(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *where = NULL;
	int status = 0;
	char *me = NULL;

	struct yahoo_buddy *bud=NULL;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 1)
			me = pair->value;
		if (pair->key == 7)
			who = pair->value;
		if (pair->key == 65)
			where = pair->value;
		if (pair->key == 66)
			status = strtol(pair->value, NULL, 10);
	}

	yahoo_dump_unhandled(pkt);

	if(!who)
		return;
	if(!where)
		where = "Unknown";

	bud = y_new0(struct yahoo_buddy, 1);
	bud->id = strdup(who);
	bud->group = strdup(where);
	bud->real_name = NULL;

	yd->buddies = y_list_append(yd->buddies, bud);

/*	YAHOO_CALLBACK(ext_yahoo_status_changed)(yd->client_id, who, status, NULL, (status==YAHOO_STATUS_AVAILABLE?0:1)); */
}

static void yahoo_process_buddydel(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *where = NULL;
	int unk_66 = 0;
	char *me = NULL;
	struct yahoo_buddy *bud;

	YList *buddy;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 1)
			me = pair->value;
		if (pair->key == 7)
			who = pair->value;
		if (pair->key == 65)
			where = pair->value;
		if (pair->key == 66)
			unk_66 = strtol(pair->value, NULL, 10);
	}
	
	bud = y_new0(struct yahoo_buddy, 1);
	bud->id = strdup(who);
	bud->group = strdup(where);

	buddy = y_list_find_custom(yd->buddies, bud, is_same_bud);

	FREE(bud->id);
	FREE(bud->group);
	FREE(bud);

	if(buddy) {
		bud = buddy->data;
		yd->buddies = y_list_remove_link(yd->buddies, buddy);

		FREE(bud->id);
		FREE(bud->group);
		FREE(bud->real_name);
		FREE(bud);

		bud=NULL;
	}
}

static void yahoo_process_ignore(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *who = NULL;
	int  status = 0;
	char *me = NULL;
	int  un_ignore = 0;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 0)
			who = pair->value;
		if (pair->key == 1)
			me = pair->value;
		if (pair->key == 13) /* 1 == ignore, 2 == unignore */ 
			un_ignore = strtol(pair->value, NULL, 10);
		if (pair->key == 66) 
			status = strtol(pair->value, NULL, 10);
	}


	/*
	 * status
	 * 	0  - ok
	 * 	2  - already in ignore list, could not add
	 * 	3  - not in ignore list, could not delete
	 * 	12 - is a buddy, could not add
	 */

/*	if(status)
		YAHOO_CALLBACK(ext_yahoo_error)(yd->client_id, status, who, 0);
*/	
}

static void yahoo_process_voicechat(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *who = NULL;
	char *me = NULL;
	char *room = NULL;
	char *voice_room = NULL;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 4)
			who = pair->value;
		if (pair->key == 5)
			me = pair->value;
		if (pair->key == 13)
			voice_room=pair->value;
		if (pair->key == 57) 
			room=pair->value;
	}

	NOTICE(("got voice chat invite from %s in %s", who, room));
	/* 
	 * send: s:0 1:me 5:who 57:room 13:1
	 * ????  s:4 5:who 10:99 19:-1615114531
	 * gotr: s:4 5:who 10:99 19:-1615114615
	 * ????  s:1 5:me 4:who 57:room 13:3room
	 * got:  s:1 5:me 4:who 57:room 13:1room
	 * rej:  s:0 1:me 5:who 57:room 13:3
	 * rejr: s:4 5:who 10:99 19:-1617114599
	 */
}

static void yahoo_process_webcam_key(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	char *me = NULL;
	char *key = NULL;

	YList *l;
	for (l = pkt->hash; l; l = l->next) {
		struct yahoo_pair *pair = l->data;
		if (pair->key == 5)
			me = pair->value;
		if (pair->key == 61) 
			key=pair->value;
	}

	if (me && key)
		YAHOO_CALLBACK(ext_yahoo_got_webcam_key)(yd->client_id, key);
}

static void yahoo_packet_process(struct yahoo_data *yd, struct yahoo_packet *pkt)
{
	DEBUG_MSG(("yahoo_packet_process: 0x%02x", pkt->service));
	switch (pkt->service)
	{
	case YAHOO_SERVICE_USERSTAT:
	case YAHOO_SERVICE_LOGON:
	case YAHOO_SERVICE_LOGOFF:
	case YAHOO_SERVICE_ISAWAY:
	case YAHOO_SERVICE_ISBACK:
	case YAHOO_SERVICE_GAMELOGON:
	case YAHOO_SERVICE_GAMELOGOFF:
	case YAHOO_SERVICE_IDACT:
	case YAHOO_SERVICE_IDDEACT:
		yahoo_process_status(yd, pkt);
		break;
	case YAHOO_SERVICE_NOTIFY:
		yahoo_process_notify(yd, pkt);
		break;
	case YAHOO_SERVICE_MESSAGE:
	case YAHOO_SERVICE_GAMEMSG:
	case YAHOO_SERVICE_SYSMESSAGE:
		yahoo_process_message(yd, pkt);
		break;
	case YAHOO_SERVICE_NEWMAIL:
		yahoo_process_mail(yd, pkt);
		break;
	case YAHOO_SERVICE_NEWCONTACT:
		yahoo_process_contact(yd, pkt);
		break;
	case YAHOO_SERVICE_LIST:
		yahoo_process_list(yd, pkt);
		break;
	case YAHOO_SERVICE_AUTH:
		yahoo_process_auth(yd, pkt);
		break;
	case YAHOO_SERVICE_AUTHRESP:
		yahoo_process_auth_resp(yd, pkt);
		break;
	case YAHOO_SERVICE_CONFINVITE:
	case YAHOO_SERVICE_CONFADDINVITE:
	case YAHOO_SERVICE_CONFDECLINE:
	case YAHOO_SERVICE_CONFLOGON:
	case YAHOO_SERVICE_CONFLOGOFF:
	case YAHOO_SERVICE_CONFMSG:
		yahoo_process_conference(yd, pkt);
		break;
	case YAHOO_SERVICE_CHATONLINE:
	case YAHOO_SERVICE_CHATGOTO:
	case YAHOO_SERVICE_CHATJOIN:
	case YAHOO_SERVICE_CHATLEAVE:
	case YAHOO_SERVICE_CHATEXIT:
	case YAHOO_SERVICE_CHATLOGOUT:
	case YAHOO_SERVICE_CHATPING:
	case YAHOO_SERVICE_COMMENT:
		yahoo_process_chat(yd, pkt);
		break;
	case YAHOO_SERVICE_P2PFILEXFER:
	case YAHOO_SERVICE_FILETRANSFER:
		yahoo_process_filetransfer(yd, pkt);
		break;
	case YAHOO_SERVICE_ADDBUDDY:
		yahoo_process_buddyadd(yd, pkt);
		break;
	case YAHOO_SERVICE_REMBUDDY:
		yahoo_process_buddydel(yd, pkt);
		break;
	case YAHOO_SERVICE_IGNORECONTACT:
		yahoo_process_ignore(yd, pkt);
		break;
	case YAHOO_SERVICE_VOICECHAT:
		yahoo_process_voicechat(yd, pkt);
		break;
	case YAHOO_SERVICE_WEBCAM:
		yahoo_process_webcam_key(yd, pkt);
		break;
	case YAHOO_SERVICE_IDLE:
	case YAHOO_SERVICE_MAILSTAT:
	case YAHOO_SERVICE_CHATINVITE:
	case YAHOO_SERVICE_CALENDAR:
	case YAHOO_SERVICE_NEWPERSONALMAIL:
	case YAHOO_SERVICE_ADDIDENT:
	case YAHOO_SERVICE_ADDIGNORE:
	case YAHOO_SERVICE_PING:
	case YAHOO_SERVICE_GOTGROUPRENAME:
	case YAHOO_SERVICE_GROUPRENAME:
	case YAHOO_SERVICE_PASSTHROUGH2:
	case YAHOO_SERVICE_CHATLOGON:
	case YAHOO_SERVICE_CHATLOGOFF:
	case YAHOO_SERVICE_CHATMSG:
	case YAHOO_SERVICE_REJECTCONTACT:
	case YAHOO_SERVICE_PEERTOPEER:
		WARNING(("unhandled service 0x%02x", pkt->service));
		yahoo_dump_unhandled(pkt);
		break;
	default:
		WARNING(("unknown service 0x%02x", pkt->service));
		yahoo_dump_unhandled(pkt);
		break;
	}
}

static struct yahoo_packet * yahoo_getdata(struct yahoo_data * yd)
{
	struct yahoo_packet *pkt;
	int pos = 0;
	int pktlen;

	if(!yd)
		return NULL;

	DEBUG_MSG(("rxlen is %d", yd->rxlen));
	if (yd->rxlen < YAHOO_PACKET_HDRLEN) {
		DEBUG_MSG(("len < YAHOO_PACKET_HDRLEN"));
		return NULL;
	}

	pos += 4; /* YMSG */
	pos += 2;
	pos += 2;

	pktlen = yahoo_get16(yd->rxqueue + pos); pos += 2;
	DEBUG_MSG(("%d bytes to read, rxlen is %d", 
			pktlen, yd->rxlen));

	if (yd->rxlen < (YAHOO_PACKET_HDRLEN + pktlen)) {
		DEBUG_MSG(("len < YAHOO_PACKET_HDRLEN + pktlen"));
		return NULL;
	}

	LOG(("reading packet"));
	yahoo_packet_dump(yd->rxqueue, YAHOO_PACKET_HDRLEN + pktlen);

	pkt = yahoo_packet_new(0, 0, 0);

	pkt->service = yahoo_get16(yd->rxqueue + pos); pos += 2;
	pkt->status = yahoo_get32(yd->rxqueue + pos); pos += 4;
	DEBUG_MSG(("Yahoo Service: 0x%02x Status: %d", pkt->service,
		       pkt->status));
	pkt->id = yahoo_get32(yd->rxqueue + pos); pos += 4;

	yd->id = pkt->id;

	yahoo_packet_read(pkt, yd->rxqueue + pos, pktlen);

	yd->rxlen -= YAHOO_PACKET_HDRLEN + pktlen;
	DEBUG_MSG(("rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	if (yd->rxlen>0) {
		unsigned char *tmp = y_memdup(yd->rxqueue + YAHOO_PACKET_HDRLEN 
				+ pktlen, yd->rxlen);
		FREE(yd->rxqueue);
		yd->rxqueue = tmp;
		DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	} else {
		DEBUG_MSG(("freed rxqueue == %p", yd->rxqueue));
		FREE(yd->rxqueue);
	}

	return pkt;
}

static void yahoo_yab_read(struct yab *yab, unsigned char *d, int len)
{
	char *st, *en;
	char *data = (char *)d;
	data[len]='\0';

	NOTICE(("Got yab: %s", data));
	st = strstr(data, "userid=\"") + strlen("userid=\"");
	en = strchr(st, '"');
	*en++ = '\0';

	yab->id = yahoo_xmldecode(st);

	st = strstr(en, "fname=\"");
	if(st) {
		st += strlen("fname=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->fname = yahoo_xmldecode(st);
	}

	st = strstr(en, "lname=\"");
	if(st) {
		st += strlen("lname=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->lname = yahoo_xmldecode(st);
	}

	st = strstr(en, "nname=\"");
	if(st) {
		st += strlen("nname=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->nname = yahoo_xmldecode(st);
	}

	st = strstr(en, "email=\"");
	if(st) {
		st += strlen("email=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->email = yahoo_xmldecode(st);
	}

	st = strstr(en, "hphone=\"");
	if(st) {
		st += strlen("hphone=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->hphone = yahoo_xmldecode(st);
	}

	st = strstr(en, "wphone=\"");
	if(st) {
		st += strlen("wphone=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->wphone = yahoo_xmldecode(st);
	}

	st = strstr(en, "mphone=\"");
	if(st) {
		st += strlen("mphone=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->mphone = yahoo_xmldecode(st);
	}

	st = strstr(en, "dbid=\"");
	if(st) {
		st += strlen("dbid=\"");
		en = strchr(st, '"'); *en++ = '\0';
		yab->dbid = atoi(st);
	}
}

static struct yab * yahoo_getyab(struct yahoo_data *yd)
{
	struct yab *yab = NULL;
	int pos = 0, end=0;

	if(!yd)
		return NULL;

	DEBUG_MSG(("rxlen is %d", yd->rxlen));

	/* start with <record */
	while(pos < yd->rxlen-strlen("<record")+1 
			&& memcmp(yd->rxqueue + pos, "<record", strlen("<record")))
		pos++;

	if(pos >= yd->rxlen-1)
		return NULL;

	end = pos+2;
	/* end with /> */
	while(end < yd->rxlen-strlen("/>")+1 && memcmp(yd->rxqueue + end, "/>", strlen("/>")))
	       	end++;

	if(end >= yd->rxlen-1)
		return NULL;

	yab = y_new0(struct yab, 1);
	yahoo_yab_read(yab, yd->rxqueue + pos, end+2-pos);
	

	yd->rxlen -= end+1;
	DEBUG_MSG(("rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	if (yd->rxlen>0) {
		unsigned char *tmp = y_memdup(yd->rxqueue + end + 1, yd->rxlen);
		FREE(yd->rxqueue);
		yd->rxqueue = tmp;
		DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	} else {
		DEBUG_MSG(("freed rxqueue == %p", yd->rxqueue));
		FREE(yd->rxqueue);
	}


	return yab;
}

static char * yahoo_getwebcam_master(struct yahoo_data *yd)
{
	unsigned int pos=0;
	unsigned int len=0;
	char *server;

	if(!yd)
		return NULL;

	DEBUG_MSG(("rxlen is %d", yd->rxlen));

	len = yd->rxqueue[pos++];
	if (yd->rxlen < len)
		return NULL;

	pos += 3; /* skip next 3 bytes */

	server =  y_memdup(yd->rxqueue+pos, 16);
	pos += 16;

	/* skip rest of the data */

	yd->rxlen -= len;
	DEBUG_MSG(("rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	if (yd->rxlen>0) {
		unsigned char *tmp = y_memdup(yd->rxqueue + pos, yd->rxlen);
		FREE(yd->rxqueue);
		yd->rxqueue = tmp;
		DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	} else {
		DEBUG_MSG(("freed rxqueue == %p", yd->rxqueue));
		FREE(yd->rxqueue);
	}

	return server;
}

static int yahoo_get_webcam_data(struct yahoo_data *yd)
{
	unsigned char reason=0;
	unsigned int pos=0;
	unsigned int begin=0;
	unsigned int end=0;
	unsigned char header_len=0;
	char *who;
	int connect=0;

	if(!yd->wcm || !yd->rxlen)
		return -1;

	DEBUG_MSG(("rxlen is %d", yd->rxlen));

	/* if we are not reading part of image then read header */
	if (!yd->wcm->to_read)
	{
		header_len=yd->rxqueue[pos++];
		yd->wcm->packet_type=0;

		if (yd->rxlen < header_len)
			return 0;

		if (header_len >= 8)
		{
			reason = yd->rxqueue[pos++];
			/* next 2 bytes should always be 05 00 */
			pos += 2;
			yd->wcm->data_size = yahoo_get32(yd->rxqueue + pos);
			pos += 4;
			yd->wcm->to_read = yd->wcm->data_size;
		}
		if (header_len >= 13)
		{
			yd->wcm->packet_type = yd->rxqueue[pos++];
			yd->wcm->timestamp = yahoo_get32(yd->rxqueue + pos);
			pos += 4;
		}

		/* skip rest of header */
		pos = header_len;
	}

	begin = pos;
	pos += yd->wcm->to_read;
	if (pos > yd->rxlen) pos = yd->rxlen;

	/* if it is not an image then make sure we have the whole packet */
	if (yd->wcm->packet_type != 0x02) {
		if ((pos - begin) != yd->wcm->data_size) {
			yd->wcm->to_read = 0;
			return 0;
		} else {
			yahoo_packet_dump(yd->rxqueue + begin, pos - begin);
		}
	}

	DEBUG_MSG(("packet type %.2X, data length %d", yd->wcm->packet_type,
		yd->wcm->data_size));

	/* find out what kind of packet we got */
	switch (yd->wcm->packet_type)
	{
		case 0x00: /* user requests to view webcam */
			if (yd->wcm->data_size) {
				end = begin;
				while (end <= yd->rxlen &&
					yd->rxqueue[end++] != 13);
				if (end > begin)
				{
					who = y_memdup(yd->rxqueue + begin, end - begin);
					who[end - begin - 1] = 0;
					YAHOO_CALLBACK(ext_yahoo_webcam_viewer)
						(yd->client_id, who + 2, 2);
					FREE(who);
				}
			}
			break;
		case 0x01: /* status packets?? */
			/* timestamp contains status info */
			/* 00 00 00 01 = we have data?? */
			break;
		case 0x02: /* image data */
			YAHOO_CALLBACK(ext_yahoo_got_webcam_image)
				(yd->client_id, yd->rxqueue + begin,
				yd->wcm->data_size, pos - begin,
				yd->wcm->timestamp);
			break;
		case 0x05: /* response packets when uploading */
			if (!yd->wcm->data_size) {
				YAHOO_CALLBACK(ext_yahoo_webcam_data_request)
					(yd->client_id, yd->wcm->timestamp);
			}
			break;
		case 0x07: /* connection is closing */
			/* second byte in header is reason */
			/* 01 = user closed connection */
			/* 0F = user disconnected us */
			break;
		case 0x0C: /* user connected */
		case 0x0D: /* user disconnected */
			if (yd->wcm->data_size) {
				who = y_memdup(yd->rxqueue + begin, pos - begin + 1);
				who[pos - begin] = 0;
				if (yd->wcm->packet_type == 0x0C)
					connect=1;
				else
					connect=0;
				YAHOO_CALLBACK(ext_yahoo_webcam_viewer)
					(yd->client_id, who, connect);
				FREE(who);
			}
			break;
		case 0x13: /* user data */
			/* i=user_ip (ip of the user we are viewing) */
			break;
		case 0x17: /* ?? */
			break;
	}
	yd->wcm->to_read -= pos - begin;

	yd->rxlen -= pos;
	DEBUG_MSG(("rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	if (yd->rxlen>0) {
		unsigned char *tmp = y_memdup(yd->rxqueue + pos, yd->rxlen);
		FREE(yd->rxqueue);
		yd->rxqueue = tmp;
		DEBUG_MSG(("new rxlen == %d, rxqueue == %p", yd->rxlen, yd->rxqueue));
	} else {
		DEBUG_MSG(("freed rxqueue == %p", yd->rxqueue));
		FREE(yd->rxqueue);
	}

	/* If we read a complete packet return success */
	if (!yd->wcm->to_read)
		return 1;

	return 0;
}

int yahoo_write_ready(int id, int fd)
{
	return 1;
}

static void yahoo_process_pager_connection(struct yahoo_data *yd)
{
	struct yahoo_packet *pkt;
	int id = yd->client_id;

	while (find_conn_by_id(id) && (pkt = yahoo_getdata(yd)) != NULL) {

		yahoo_packet_process(yd, pkt);

		yahoo_packet_free(pkt);
	}
}

static void yahoo_process_ft_connection(struct yahoo_data *yd)
{
}

static void yahoo_process_yab_connection(struct yahoo_data *yd)
{
	struct yab *yab;
	YList *buds;
	int changed=0;

	while((yab = yahoo_getyab(yd)) != NULL) {
		changed=1;
		for(buds = yd->buddies; buds; buds=buds->next) {
			struct yahoo_buddy * bud = buds->data;
			if(!strcmp(bud->id, yab->id)) {
				bud->yab_entry = yab;
				if(yab->nname) {
					bud->real_name = strdup(yab->nname);
				} else if(yab->fname && yab->lname) {
					bud->real_name = y_new0(char, 
							strlen(yab->fname)+
							strlen(yab->lname)+2
							);
					sprintf(bud->real_name, "%s %s",
							yab->fname, yab->lname);
				} else if(yab->fname) {
					bud->real_name = strdup(yab->fname);
				}
				break; /* for */
			}
		}
	}

	if(changed)
		YAHOO_CALLBACK(ext_yahoo_got_buddies)(yd->client_id, yd->buddies);
}

static void yahoo_process_webcam_master_connection(struct yahoo_data *yd)
{
	char* server;

	server = yahoo_getwebcam_master(yd);

	if (server)
	{
		YAHOO_CALLBACK(ext_yahoo_got_webcam_server)(yd->client_id, server);
		FREE(server);
	}
}

static void yahoo_process_webcam_connection(struct yahoo_data *yd)
{
	int id = yd->client_id;

	/* as long as we still have packets available keep processing them */
	while (find_conn_by_id(id) && yahoo_get_webcam_data(yd) == 1);
}

static void (*yahoo_process_connection[])(struct yahoo_data *) = {
	yahoo_process_pager_connection,
	yahoo_process_ft_connection,
	yahoo_process_yab_connection,
	yahoo_process_webcam_master_connection,
	yahoo_process_webcam_connection
};

int yahoo_read_ready(int id, int fd)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	char buf[1024];
	int len;

	DEBUG_MSG(("callback"));

	if(!yd)
		return -2;
	
	do {
		len = read(fd, buf, sizeof(buf));
	} while(len == -1 && errno == EINTR);

	if (len <= 0) {
		int e = errno;
		DEBUG_MSG(("len == %d (<= 0)", len));

		yd->current_status = -1;
		YAHOO_CALLBACK(ext_yahoo_remove_handler)(id, fd);
		if(yd->type == YAHOO_CONNECTION_YAB)
			yd->buddies = NULL;
		yahoo_close(yd);

		/* no need to return an error, because we've already fixed it */
		if(len == 0)
			return 1;

		errno=e;
		return -1;
	}

	yd->rxqueue = realloc(yd->rxqueue, len + yd->rxlen);
	memcpy(yd->rxqueue + yd->rxlen, buf, len);
	yd->rxlen += len;

	yahoo_process_connection[yd->type](yd);

	return len;
}

int yahoo_login(const char *username, const char *password, int initial)
{
	struct yahoo_data *yd;
	struct yahoo_packet *pkt;
	int fd;
	int i;

	fd = YAHOO_CALLBACK(ext_yahoo_connect)(pager_host, atoi(pager_port));

	for(i=0; fd <=0 && fallback_ports[i]; i++) {
		fd = YAHOO_CALLBACK(ext_yahoo_connect)(pager_host, 
				fallback_ports[i]);
	}

	if(fd <= 0)
		return fd;

	yd = y_new0(struct yahoo_data, 1);
	yd->fd = fd;

	yd->user = strdup(username);
	yd->password = strdup(password);

	yd->initial_status = initial;
	yd->current_status = -1;

	yd->client_id = ++last_id;

	add_to_list(yd, yd->fd);

	pkt = yahoo_packet_new(YAHOO_SERVICE_AUTH, YAHOO_STATUS_AVAILABLE, 0);

	yahoo_packet_hash(pkt, 1, username);
	NOTICE(("Sending initial packet"));
	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);

	YAHOO_CALLBACK(ext_yahoo_add_handler)(yd->client_id, yd->fd, YAHOO_INPUT_READ);

	return yd->client_id;
}


int yahoo_get_fd(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return 0;
	else
		return yd->fd;
}

void yahoo_send_im(int id, const char *from, const char *who, const char *what, int utf8)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_MESSAGE, YAHOO_STATUS_OFFLINE, yd->id);

	if(from && strcmp(from, yd->user))
		yahoo_packet_hash(pkt, 0, yd->user);
	yahoo_packet_hash(pkt, 1, from?from:yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 14, what);

	if(utf8)
		yahoo_packet_hash(pkt, 97, "1");

	yahoo_packet_hash(pkt, 63, ";0");	/* imvironment name; or ;0 */
	yahoo_packet_hash(pkt, 64, "0");


	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_send_typing(int id, const char *from, const char *who, int typ)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_NOTIFY, YAHOO_STATUS_NOTIFY, yd->id);

	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 4, from?from:yd->user);
	yahoo_packet_hash(pkt, 14, " ");
	yahoo_packet_hash(pkt, 13, typ ? "1" : "0");
	yahoo_packet_hash(pkt, 49, "TYPING");

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_set_away(int id, enum yahoo_status state, const char *msg, int away)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;
	int service;
	char s[4];

	if(!yd)
		return;

	if (msg) {
		yd->current_status = YAHOO_STATUS_CUSTOM;
	} else {
		yd->current_status = state;
	}

	if (yd->current_status == YAHOO_STATUS_AVAILABLE)
		service = YAHOO_SERVICE_ISBACK;
	else
		service = YAHOO_SERVICE_ISAWAY;
	pkt = yahoo_packet_new(service, yd->current_status, yd->id);
	snprintf(s, sizeof(s), "%d", yd->current_status);
	yahoo_packet_hash(pkt, 10, s);
	if (yd->current_status == YAHOO_STATUS_CUSTOM) {
		yahoo_packet_hash(pkt, 19, msg);
		yahoo_packet_hash(pkt, 47, away?"1":"0");
	}

	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_logoff(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	if(yd->current_status != -1) {

		pkt = yahoo_packet_new(YAHOO_SERVICE_LOGOFF, YAHOO_STATUS_AVAILABLE, yd->id);
		yd->current_status = -1;

		if (pkt) {
			yahoo_send_packet(yd, pkt, 0);
			yahoo_packet_free(pkt);
		}
	}

	YAHOO_CALLBACK(ext_yahoo_remove_handler)(id, yd->fd);
	yahoo_close(yd);
}

void yahoo_get_list(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_LIST, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	if (pkt) {
		yahoo_send_packet(yd, pkt, 0);
		yahoo_packet_free(pkt);
	}
}

void yahoo_get_yab(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_data *nyd;
	char url[1024];
	char buff[1024];

	if(!yd)
		return;

	nyd = y_new0(struct yahoo_data, 1);
	nyd->id = yd->id;
	nyd->client_id = ++last_id;
	nyd->type = YAHOO_CONNECTION_YAB;
	nyd->buddies = yd->buddies;

	snprintf(url, 1024, "http://insider.msg.yahoo.com/ycontent/?ab2=0");

	snprintf(buff, sizeof(buff), "Y=%s; T=%s",
			yd->cookie_y, yd->cookie_t);

	nyd->fd = yahoo_http_get(url, buff);

	add_to_list(nyd, nyd->fd);

	YAHOO_CALLBACK(ext_yahoo_add_handler)(nyd->client_id, nyd->fd, YAHOO_INPUT_READ);

}

void yahoo_set_yab(int id, struct yab * yab)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_data *nyd;
	char url[1024];
	char buff[1024];
	char *temp;
	int size = sizeof(url)-1;

	if(!yd)
		return;

	nyd = y_new0(struct yahoo_data, 1);
	nyd->id = yd->id;
	nyd->client_id = ++last_id;
	nyd->type = YAHOO_CONNECTION_YAB;
	nyd->buddies = yd->buddies;

	strncpy(url, "http://insider.msg.yahoo.com/ycontent/?addab2=0", size);

	if(yab->dbid) {
		/* change existing yab */
		char tmp[32];
		strncat(url, "&ee=1&ow=1&id=", size - strlen(url));
		snprintf(tmp, sizeof(tmp), "%d", yab->dbid);
		strncat(url, tmp, size - strlen(url));
	}

	if(yab->fname) {
		strncat(url, "&fn=", size - strlen(url));
		temp = yahoo_urlencode(yab->fname);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->lname) {
		strncat(url, "&ln=", size - strlen(url));
		temp = yahoo_urlencode(yab->lname);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	strncat(url, "&yid=", size - strlen(url));
	temp = yahoo_urlencode(yab->id);
	strncat(url, temp, size - strlen(url));
	free(temp);
	if(yab->nname) {
		strncat(url, "&nn=", size - strlen(url));
		temp = yahoo_urlencode(yab->nname);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->email) {
		strncat(url, "&e=", size - strlen(url));
		temp = yahoo_urlencode(yab->email);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->hphone) {
		strncat(url, "&hp=", size - strlen(url));
		temp = yahoo_urlencode(yab->hphone);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->wphone) {
		strncat(url, "&wp=", size - strlen(url));
		temp = yahoo_urlencode(yab->wphone);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	if(yab->mphone) {
		strncat(url, "&mp=", size - strlen(url));
		temp = yahoo_urlencode(yab->mphone);
		strncat(url, temp, size - strlen(url));
		free(temp);
	}
	strncat(url, "&pp=0", size - strlen(url));

	snprintf(buff, sizeof(buff), "Y=%s; T=%s",
			yd->cookie_y, yd->cookie_t);

	nyd->fd = yahoo_http_get(url, buff);

	add_to_list(nyd, nyd->fd);

	YAHOO_CALLBACK(ext_yahoo_add_handler)(nyd->client_id, nyd->fd, YAHOO_INPUT_READ);
}

void yahoo_set_identity_status(int id, const char * identity, int active)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(active?YAHOO_SERVICE_IDACT:YAHOO_SERVICE_IDDEACT,
			YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 3, identity);
	if (pkt) {
		yahoo_send_packet(yd, pkt, 0);
		yahoo_packet_free(pkt);
	}
}

void yahoo_refresh(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_USERSTAT, YAHOO_STATUS_AVAILABLE, yd->id);
	if (pkt) {
		yahoo_send_packet(yd, pkt, 0);
		yahoo_packet_free(pkt);
	}
}

void yahoo_keepalive(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt=NULL;
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_PING, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_add_buddy(int id, const char *who, const char *group)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;

	if(!yd)
		return;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_ADDBUDDY, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, group);
	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_remove_buddy(int id, const char *who, const char *group)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_REMBUDDY, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, group);
	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_reject_buddy(int id, const char *who, const char *msg)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;

	if(!yd)
		return;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_REJECTCONTACT, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 14, msg);
	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_ignore_buddy(int id, const char *who, int unignore)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;

	if(!yd)
		return;

	if (!yd->logged_in)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_IGNORECONTACT, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 13, unignore?"2":"1");
	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_change_buddy_group(int id, const char *who, const char *old_group, const char *new_group)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_ADDBUDDY, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, new_group);
	yahoo_packet_hash(pkt, 14, " ");

	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);

	pkt = yahoo_packet_new(YAHOO_SERVICE_REMBUDDY, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 7, who);
	yahoo_packet_hash(pkt, 65, old_group);
	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_group_rename(int id, const char *old_group, const char *new_group)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt = NULL;

	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_GROUPRENAME, YAHOO_STATUS_AVAILABLE, yd->id);
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 65, old_group);
	yahoo_packet_hash(pkt, 67, new_group);

	yahoo_send_packet(yd, pkt, 0);
	yahoo_packet_free(pkt);
}

void yahoo_conference_addinvite(int id, const char * from, const char *who, const char *room, const YList * members, const char *msg)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFADDINVITE, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 51, who);
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 58, msg);
	yahoo_packet_hash(pkt, 13, "0");
	for(; members; members = members->next) {
		yahoo_packet_hash(pkt, 52, (char *)members->data);
		yahoo_packet_hash(pkt, 53, (char *)members->data);
	}
	/* 52, 53 -> other members? */

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_invite(int id, const char * from, YList *who, const char *room, const char *msg)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFINVITE, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 50, yd->user);
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 52, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 58, msg);
	yahoo_packet_hash(pkt, 13, "0");

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_logon(int id, const char *from, YList *who, const char *room)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFLOGON, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 3, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_decline(int id, const char * from, YList *who, const char *room, const char *msg)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFDECLINE, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 3, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 14, msg);

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_logoff(int id, const char * from, YList *who, const char *room)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFLOGOFF, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 3, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_conference_message(int id, const char * from, YList *who, const char *room, const char *msg, int utf8)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CONFMSG, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	for(; who; who = who->next) {
		yahoo_packet_hash(pkt, 53, (char *)who->data);
	}
	yahoo_packet_hash(pkt, 57, room);
	yahoo_packet_hash(pkt, 14, msg);

	if(utf8)
		yahoo_packet_hash(pkt, 97, "1");

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_chat_logon(int id, const char *from, const char *room, const char *roomid)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CHATONLINE, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 109, yd->user);
	yahoo_packet_hash(pkt, 6, "abcde");

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);

	pkt = yahoo_packet_new(YAHOO_SERVICE_CHATJOIN, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 104, room);
	yahoo_packet_hash(pkt, 129, roomid);
	yahoo_packet_hash(pkt, 62, "2"); /* ??? */

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}


void yahoo_chat_message(int id, const char *from, const char *room, const char *msg)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_COMMENT, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));
	yahoo_packet_hash(pkt, 104, room);
	yahoo_packet_hash(pkt, 117, msg);
	yahoo_packet_hash(pkt, 124, "1");

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}


void yahoo_chat_logoff(int id, const char *from)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_CHATLOGOUT, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, (from?from:yd->user));

	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_webcam_get_key(int id, const char *who)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_WEBCAM, YAHOO_STATUS_AVAILABLE, yd->id);

	yahoo_packet_hash(pkt, 1, yd->user);
	if (who != NULL)
		yahoo_packet_hash(pkt, 5, who);
	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}

void yahoo_webcam_get_upload_key(int id)
{
	yahoo_webcam_get_key(id, NULL);
}

void yahoo_webcam_get_server(int id, char *who)
{
	char *data = NULL;
	char *packet = NULL;
	unsigned char magic_nr[] = {0, 1, 0};
	unsigned char header_len = 8;
	unsigned int len = 0;
	unsigned int pos = 0;
	struct yahoo_data *nyd;
	struct yahoo_data *yd = find_conn_by_id(id);

	nyd = y_new0(struct yahoo_data, 1);
	nyd->id = yd->id;
	nyd->client_id = ++last_id;
	nyd->type = YAHOO_CONNECTION_WEBCAM_MASTER;

	nyd->fd = YAHOO_CALLBACK(ext_yahoo_connect)(webcam_host, atoi(webcam_port));

	add_to_list(nyd, nyd->fd);

	/* send initial packet */
	if (who)
		data = strdup("<RVWCFG>");
	else
		data = strdup("<RUPCFG>");
	yahoo_send_data(nyd, data, strlen(data));
	FREE(data);

	/* send data */
	if (who)
	{
		data = strdup("g=");
		data = y_string_append(data, who);
		data = y_string_append(data, "\r\n");
	} else {
		data = strdup("f=1\r\n");
	}
	len = strlen(data);
	packet = y_new0(char, header_len + len);
	packet[pos++] = header_len;
	memcpy(packet + pos, magic_nr, sizeof(magic_nr));
	pos += sizeof(magic_nr);
	pos += yahoo_put32(packet + pos, len);
	memcpy(packet + pos, data, len);
	pos += len;
	yahoo_send_data(nyd, packet, pos);
	FREE(packet);
	FREE(data);

	YAHOO_CALLBACK(ext_yahoo_add_handler)(nyd->client_id, nyd->fd, YAHOO_INPUT_READ);
}

void yahoo_webcam_get_upload_server(int id)
{
	yahoo_webcam_get_server(id, NULL);
}

void yahoo_webcam_connect(int id, struct webcam *wcm)
{
	char conn_type[100];
	char *data=NULL;
	char *packet=NULL;
	unsigned char magic_nr[] = {1, 0, 0, 0, 1};
	unsigned header_len=0;
	unsigned int len=0;
	unsigned int pos=0;
	struct yahoo_data *nyd;
	struct yahoo_data *yd = find_conn_by_id(id);

	if (!wcm || !wcm->server || !wcm->key)
		return;

	nyd = y_new0(struct yahoo_data, 1);
	nyd->id = yd->id;
	nyd->client_id = ++last_id;
	nyd->type = YAHOO_CONNECTION_WEBCAM;

	/* copy webcam data to new connection */
	nyd->wcm = y_new0(struct webcam, 1);
	nyd->wcm->direction = wcm->direction;
	nyd->wcm->server = strdup(wcm->server);
	nyd->wcm->key = strdup(wcm->key);
	if (wcm->my_ip) nyd->wcm->my_ip = strdup(wcm->my_ip);
	if (wcm->user) nyd->wcm->user = strdup(wcm->user);
	if (wcm->description) nyd->wcm->description = strdup(wcm->description);

	nyd->fd = YAHOO_CALLBACK(ext_yahoo_connect)(wcm->server, atoi(webcam_port));

	add_to_list(nyd, nyd->fd);

       	/* send initial packet */
	switch (wcm->direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			data = strdup("<REQIMG>");
			break;
		case YAHOO_WEBCAM_UPLOAD:	
			data = strdup("<SNDIMG>");
			break;
		default:
			return;
	}
	yahoo_send_data(nyd, data, strlen(data));
	FREE(data);

	/* send data */
	switch (wcm->direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			header_len = 8;
			data = strdup("a=2\r\nc=us\r\ne=21\r\nu=");
			data = y_string_append(data, yd->user);
			data = y_string_append(data, "\r\nt=");
			data = y_string_append(data, wcm->key);
			data = y_string_append(data, "\r\ni=");
			data = y_string_append(data, wcm->my_ip);
			data = y_string_append(data, "\r\ng=");
			data = y_string_append(data, wcm->user);
			data = y_string_append(data, "\r\no=w-2-5-1\r\np=");
			sprintf(conn_type, "%d", wcm->conn_type);
			data = y_string_append(data, conn_type);
			data = y_string_append(data, "\r\n");
			break;
		case YAHOO_WEBCAM_UPLOAD:
			header_len = 13;
			data = strdup("a=2\r\nc=us\r\nu=");
			data = y_string_append(data, yd->user);
			data = y_string_append(data, "\r\nt=");
			data = y_string_append(data, wcm->key);
			data = y_string_append(data, "\r\ni=");
			data = y_string_append(data, wcm->my_ip);
			data = y_string_append(data, "\r\no=w-2-5-1\r\np=");
			sprintf(conn_type, "%d", wcm->conn_type);
			data = y_string_append(data, conn_type);
			data = y_string_append(data, "\r\nb=");
			data = y_string_append(data, wcm->description);
			data = y_string_append(data, "\r\n");
			break;
	}

	len = strlen(data);
	packet = y_new0(char, header_len + len);
	packet[pos++] = header_len;
	packet[pos++] = 0;
	switch (wcm->direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			packet[pos++] = 1;
			packet[pos++] = 0;
			break;
		case YAHOO_WEBCAM_UPLOAD:
			packet[pos++] = 5;
			packet[pos++] = 0;
			break;
	}

	pos += yahoo_put32(packet + pos, len);
	if (wcm->direction == YAHOO_WEBCAM_UPLOAD)
	{
		memcpy(packet + pos, magic_nr, sizeof(magic_nr));
		pos += sizeof(magic_nr);
	}
	memcpy(packet + pos, data, len);
	yahoo_send_data(nyd, packet, header_len + len);
	FREE(packet);
	FREE(data);

	YAHOO_CALLBACK(ext_yahoo_add_handler)(nyd->client_id, nyd->fd, YAHOO_INPUT_READ);
}

void yahoo_webcam_send_image(int id, unsigned char *image, unsigned int length, unsigned int timestamp)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	char *packet;
	unsigned char header_len = 13;
	unsigned int pos = 0;

	if (!yd)
		return;

	packet = y_new0(char, header_len + length);
	packet[pos++] = header_len;
	packet[pos++] = 0;
	packet[pos++] = 5; /* version byte?? */
	packet[pos++] = 0;
	pos += yahoo_put32(packet + pos, length);
	packet[pos++] = 2; /* packet type, image */
	pos += yahoo_put32(packet + pos, timestamp);
	if (length) memcpy(packet + pos, image, length);
	yahoo_send_data(yd, packet, header_len + length);
	FREE(packet);
}

void yahoo_webcam_accept_viewer(int id, const char* who, int accept)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	char *packet = NULL;
	char *data = NULL;
	unsigned char header_len = 13;
	unsigned int pos = 0;
	unsigned int len = 0;

	if (!yd)
		return;

	data = strdup("u=");
	data = y_string_append(data, (char*)who);
	data = y_string_append(data, "\r\n");
	len = strlen(data);

	packet = y_new0(char, header_len + len);
	packet[pos++] = header_len;
	packet[pos++] = 0;
	packet[pos++] = 5; /* version byte?? */
	packet[pos++] = 0;
	pos += yahoo_put32(packet + pos, len);
	packet[pos++] = 0; /* packet type */
	pos += yahoo_put32(packet + pos, accept);
	memcpy(packet + pos, data, len);
	yahoo_send_data(yd, packet, header_len + len);
}

void yahoo_webcam_invite(int id, const char *who)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_packet *pkt;
		
	if(!yd)
		return;

	pkt = yahoo_packet_new(YAHOO_SERVICE_NOTIFY, YAHOO_STATUS_NOTIFY, yd->id);

	yahoo_packet_hash(pkt, 49, "WEBCAMINVITE");
	yahoo_packet_hash(pkt, 14, " ");
	yahoo_packet_hash(pkt, 13, "0");
	yahoo_packet_hash(pkt, 1, yd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_send_packet(yd, pkt, 0);

	yahoo_packet_free(pkt);
}
	

int yahoo_send_file(int id, const char *who, const char *msg, const char *name, long size)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	struct yahoo_data *nyd;
	struct yahoo_packet *pkt = NULL;
	char size_str[10];
	long content_length=0;
	unsigned char buff[1024];
	char url[255];

	if(!yd)
		return -1;

	nyd = y_new0(struct yahoo_data, 1);
	nyd->id = yd->id;
	nyd->client_id = ++last_id;
	nyd->user = strdup(yd->user);
	nyd->cookie_y = strdup(yd->cookie_y);
	nyd->cookie_t = strdup(yd->cookie_t);
	nyd->type = YAHOO_CONNECTION_FT;

	pkt = yahoo_packet_new(YAHOO_SERVICE_FILETRANSFER, YAHOO_STATUS_AVAILABLE, nyd->id);

	snprintf(size_str, sizeof(size_str), "%ld", size);

	yahoo_packet_hash(pkt, 0, nyd->user);
	yahoo_packet_hash(pkt, 5, who);
	yahoo_packet_hash(pkt, 14, msg);
	yahoo_packet_hash(pkt, 27, name);
	yahoo_packet_hash(pkt, 28, size_str);

	content_length = YAHOO_PACKET_HDRLEN + yahoo_packet_length(pkt);

	snprintf(url, sizeof(url), "http://%s:%s/notifyft", 
			filetransfer_host, filetransfer_port);
	snprintf((char *)buff, sizeof(buff), "Y=%s; T=%s",
			nyd->cookie_y, nyd->cookie_t);
	nyd->fd = yahoo_http_post(url, (char *)buff, content_length + 4 + size);

	add_to_list(nyd, nyd->fd);

	yahoo_send_packet(nyd, pkt, 8);
	yahoo_packet_free(pkt);

	snprintf((char *)buff, sizeof(buff), "29");
	buff[2] = 0xc0;
	buff[3] = 0x80;
	
	write(nyd->fd, buff, 4);

/*	YAHOO_CALLBACK(ext_yahoo_add_handler)(nyd->client_id, nyd->fd, YAHOO_INPUT_READ); */

	return nyd->fd;

	/*
	while(yahoo_tcp_readline(buff, sizeof(buff), nyd->fd) > 0) {
		if(!strcmp(buff, ""))
			break;
	}

	yahoo_close(nyd);
	*/
}


enum yahoo_status yahoo_current_status(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return YAHOO_STATUS_OFFLINE;
	return yd->current_status;
}

const YList * yahoo_get_buddylist(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	return yd->buddies;
}

const YList * yahoo_get_ignorelist(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	return yd->ignore;
}

const YList * yahoo_get_identities(int id)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	return yd->identities;
}

const char * yahoo_get_cookie(int id, const char *which)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return NULL;
	if(!strncasecmp(which, "y", 1))
		return yd->cookie_y;
	if(!strncasecmp(which, "t", 1))
		return yd->cookie_t;
	if(!strncasecmp(which, "c", 1))
		return yd->cookie_c;
	if(!strncasecmp(which, "login", 5))
		return yd->login_cookie;
	return NULL;
}

int yahoo_get_url_handle(int id, const char *url, char *filename, unsigned long *filesize)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if(!yd)
		return 0;

	return yahoo_get_url_fd(url, yd, filename, filesize);
}

const char * yahoo_get_profile_url( void )
{
	return profile_url;
}

