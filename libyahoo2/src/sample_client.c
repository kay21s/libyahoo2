/*
 * sample yahoo client based on libyahoo2
 *
 * libyahoo2 is available at http://libyahoo2.sourceforge.net/
 *
 * $Revision$
 * $Date$
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

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <termios.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Get these from http://libyahoo2.sourceforge.net/ */
#include <yahoo2.h>
#include <yahoo2_callbacks.h>
#include <yahoo_connections.h>
#include "yahoo_util.h"

int fileno(FILE * stream);


#define MAX_PREF_LEN 255
#define MY_IP "192.168.1.1"

static int do_mail_notify = 0;
static int do_yahoo_debug = 0;
static int ignore_system = 0;
static int do_typing_notify = 1;
static int webcam_direction = YAHOO_WEBCAM_DOWNLOAD;
static time_t curTime = 0;
static time_t pingTimer = 0;
static time_t webcamTimer = 0;
static double webcamStart = 0;

static int poll_loop=1;

/* Exported to libyahoo2 */
char pager_host[MAX_PREF_LEN]="scs.yahoo.com";
char pager_port[MAX_PREF_LEN]="23";
char filetransfer_host[MAX_PREF_LEN]="filetransfer.msg.yahoo.com";
char filetransfer_port[MAX_PREF_LEN]="80";
char webcam_host[MAX_PREF_LEN]="webcam.yahoo.com";
char webcam_port[MAX_PREF_LEN]="5100";

static void register_callbacks();

typedef struct {
	char yahoo_id[255];
	char password[255];
	int id;
	int fd;
	int status;
	char *msg;
	
	char *webcam_key;
	char *webcam_user;
} yahoo_local_account;

typedef struct {
	char yahoo_id[255];
	char name[255];
	int status;
	int away;
	char *msg;
	char group[255];
} yahoo_account;

typedef struct {
	int id;
	char *label;
} yahoo_idlabel;

typedef struct {
	int id;
	char *who;
} yahoo_authorize_data;

typedef struct {
	int id;
	int fd;
} yahoo_connection;

yahoo_idlabel yahoo_status_codes[] = {
	{YAHOO_STATUS_AVAILABLE, "Available"},
	{YAHOO_STATUS_BRB, "BRB"},
	{YAHOO_STATUS_BUSY, "Busy"},
	{YAHOO_STATUS_NOTATHOME, "Not Home"},
	{YAHOO_STATUS_NOTATDESK, "Not at Desk"},
	{YAHOO_STATUS_NOTINOFFICE, "Not in Office"},
	{YAHOO_STATUS_ONPHONE, "On Phone"},
	{YAHOO_STATUS_ONVACATION, "On Vacation"},
	{YAHOO_STATUS_OUTTOLUNCH, "Out to Lunch"},
	{YAHOO_STATUS_STEPPEDOUT, "Stepped Out"},
	{YAHOO_STATUS_INVISIBLE, "Invisible"},
	{YAHOO_STATUS_IDLE, "Idle"},
	{YAHOO_STATUS_OFFLINE, "Offline"},
	{YAHOO_STATUS_CUSTOM, "[Custom]"},
	{YAHOO_STATUS_NOTIFY, "Notify"},
	{0, NULL}
};

char * yahoo_status_code(enum yahoo_status s)
{
	int i;
	for(i=0; yahoo_status_codes[i].label; i++)
		if(yahoo_status_codes[i].id == s)
			return yahoo_status_codes[i].label;
	return "Unknown";
}

#define YAHOO_DEBUGLOG ext_yahoo_log

int ext_yahoo_log(char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
	return 0;
}

#define LOG(x) if(do_yahoo_debug) { YAHOO_DEBUGLOG("%s:%d: ", __FILE__, __LINE__); \
	YAHOO_DEBUGLOG x; \
	YAHOO_DEBUGLOG("\n"); }

#define WARNING(x) if(do_yahoo_debug) { YAHOO_DEBUGLOG("%s:%d: warning: ", __FILE__, __LINE__); \
	YAHOO_DEBUGLOG x; \
	YAHOO_DEBUGLOG("\n"); }

#define print_message(x) { printf x; printf("\n"); }

static yahoo_local_account * ylad = NULL;
static yahoo_connection * ylab = NULL;
static yahoo_connection * ywcm = NULL;
static yahoo_connection * ycam = NULL;
static YList * buddies = NULL;

static int expired(time_t timer)
{
	if (timer && curTime >= timer)
		return 1;

	return 0;
}

static void rearm(time_t *timer, int seconds)
{
	time(timer);
	*timer += seconds;
}

static double get_time()
{
	struct timeval ct;
	gettimeofday(&ct, 0);

	/* return time in milliseconds */
	return (ct.tv_sec * 1E3 + ct.tv_usec / 1E3);
}

static int yahoo_ping_timeout_callback()
{
	print_message(("Sending a keep alive message"));
	yahoo_keepalive(ylad->id);
	rearm(&pingTimer, 600);
	return 1;
}

static int yahoo_webcam_timeout_callback(int id)
{
	unsigned char *image = NULL;
	unsigned int length = 0;
	unsigned int timestamp = get_time() - webcamStart;

	print_message(("Sending a webcam image"));
	yahoo_webcam_send_image(id, image, length, timestamp);
	rearm(&webcamTimer, 2);
	return 1;
}

YList * conferences = NULL;
typedef struct {
	int id;
	char * room_name;
	char * host;
	YList * members;
	int joined;
} conf_room;

static char * get_buddy_name(char * yid)
{
	YList * b;
	for (b = buddies; b; b = b->next) {
		yahoo_account * ya = b->data;
		if(!strcmp(yid, ya->yahoo_id))
			return ya->name&&*ya->name?ya->name:ya->yahoo_id;
	}

	return yid;
}

static conf_room * find_conf_room_by_name_and_id(int id, const char * name)
{
	YList * l;
	for(l = conferences; l; l=l->next) {
		conf_room * cr = l->data;
		if(cr->id == id && !strcmp(name, cr->room_name)) {
			return cr;
		}
	}

	return NULL;
}

void ext_yahoo_got_conf_invite(int id, char *who, char *room, char *msg, YList *members)
{
	conf_room * cr = y_new0(conf_room, 1);
	cr->room_name = strdup(room);
	cr->host = strdup(who);
	cr->members = members;
	cr->id = id;

	conferences = y_list_append(conferences, cr);

	print_message(("%s has invited you to a conference: %s\n"
				"with the message: %s",
				who, room, msg));

	for(; members; members=members->next)
		print_message(("\t%s", (char *)members->data));
	
}
void ext_yahoo_conf_userdecline(int id, char *who, char *room, char *msg)
{
	YList * l;
	conf_room * cr = find_conf_room_by_name_and_id(id, room);
	
	if(cr)
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who)) {
			cr->members = y_list_remove_link(cr->members, l);
			FREE(l->data);
			FREE(l);
			break;
		}
	}

	print_message(("%s declined the invitation to %s\n"
				"with the message: %s", who, room, msg));
}
void ext_yahoo_conf_userjoin(int id, char *who, char *room)
{
	conf_room * cr = find_conf_room_by_name_and_id(id, room);
	if(cr) {
	YList * l = NULL;
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who))
			break;
	}
	if(!l)
		cr->members = y_list_append(cr->members, strdup(who));
	}

	print_message(("%s joined the conference %s", who, room));

}
void ext_yahoo_conf_userleave(int id, char *who, char *room)
{
	YList * l;
	conf_room * cr = find_conf_room_by_name_and_id(id, room);
	
	if(cr)
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who)) {
			cr->members = y_list_remove_link(cr->members, l);
			FREE(l->data);
			FREE(l);
			break;
		}
	}

	print_message(("%s left the conference %s", who, room));
}
void ext_yahoo_conf_message(int id, char *who, char *room, char *msg, int utf8)
{
	char * umsg = msg;

	if(utf8)
		umsg = y_utf8_to_str(msg);

	who = get_buddy_name(who);

	print_message(("%s (in %s): %s", who, room, umsg));

	if(utf8)
		FREE(umsg);
}
void ext_yahoo_chat_userjoin(int id, char *who, char *room)
{
	conf_room * cr = find_conf_room_by_name_and_id(id, room);
	if(cr) {
	YList * l = NULL;
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who))
			break;
	}
	if(!l)
		cr->members = y_list_append(cr->members, strdup(who));
	}

	print_message(("%s joined the conference %s", who, room));

}
void ext_yahoo_chat_userleave(int id, char *who, char *room)
{
	YList * l;
	conf_room * cr = find_conf_room_by_name_and_id(id, room);
	
	if(cr)
	for(l = cr->members; l; l=l->next) {
		char * w = l->data;
		if(!strcmp(w, who)) {
			cr->members = y_list_remove_link(cr->members, l);
			FREE(l->data);
			FREE(l);
			break;
		}
	}

	print_message(("%s left the conference %s", who, room));
}
void ext_yahoo_chat_message(int id, char *who, char *room, char *msg, int utf8)
{
	char * umsg = msg;

	if(utf8)
		umsg = y_utf8_to_str(msg);

	who = get_buddy_name(who);

	print_message(("%s (in %s): %s", who, room, umsg));

	if(utf8)
		FREE(umsg);
}
void ext_yahoo_status_changed(int id, char *who, int stat, char *msg, int away)
{
	yahoo_account * ya=NULL;
	YList * b;
	for(b = buddies; b; b = b->next) {
		if(!strcmp(((yahoo_account *)b->data)->yahoo_id, who)) {
			ya = b->data;
			break;
		}
	}
	
	if(msg)
		print_message(("%s (%s) is now %s", ya?ya->name:who, who, msg))
	else
		print_message(("%s (%s) is now %s", ya?ya->name:who, who, 
					yahoo_status_code(stat)))

	if(ya) {
		ya->status = stat;
		ya->away = away;
		if(msg) {
			FREE(ya->msg);
			ya->msg = strdup(msg);
		}
	}
}

void ext_yahoo_got_buddies(int id, YList * buds)
{
	while(buddies) {
		FREE(buddies->data);
		buddies = buddies->next;
		if(buddies)
			FREE(buddies->prev);
	}
	for(; buds; buds = buds->next) {
		yahoo_account *ya = y_new0(yahoo_account, 1);
		struct yahoo_buddy *bud = buds->data;
		strncpy(ya->yahoo_id, bud->id, 255);
		if(bud->real_name)
			strncpy(ya->name, bud->real_name, 255);
		strncpy(ya->group, bud->group, 255);
		ya->status = YAHOO_STATUS_OFFLINE;
		buddies = y_list_append(buddies, ya);

		print_message(("%s is %s", bud->id, bud->real_name));
	}
}

void ext_yahoo_got_ignore(int id, YList * igns)
{
}

void ext_yahoo_got_im(int id, char *who, char *msg, long tm, int stat, int utf8)
{
	char *umsg = msg;

	if(stat == 2) {
		LOG(("Error sending message to %s", who));
		return;
	}

	if(!msg)
		return;

	if(utf8)
		umsg = y_utf8_to_str(msg);
	
	who = get_buddy_name(who);

	if(tm) {
		char timestr[255];

		strncpy(timestr, ctime(&tm), sizeof(timestr));
		timestr[strlen(timestr) - 1] = '\0';

		print_message(("[Offline message at %s from %s]: %s", 
				timestr, who, umsg))
	} else {
		if(!strcmp(umsg, "<ding>")) 
			printf("\a");
		print_message(("%s: %s", who, umsg))
	}

	if(utf8)
		FREE(umsg);
}

void ext_yahoo_rejected(int id, char *who, char *msg)
{
	print_message(("%s has rejected you%s%s", who, 
				(msg?" with the message:\n":"."), 
				(msg?msg:"")));
}

void ext_yahoo_contact_added(int id, char *myid, char *who, char *msg)
{
	char buff[1024];

	snprintf(buff, sizeof(buff), "%s, the yahoo user %s has added you to their contact list", myid, who);
	if(msg) {
		strcat(buff, " with the following message:\n");
		strcat(buff, msg);
		strcat(buff, "\n");
	} else {
		strcat(buff, ".  ");
	}
	strcat(buff, "Do you want to allow this [Y/N]?");

/*	print_message((buff));
	scanf("%c", &choice);
	if(choice != 'y' && choice != 'Y')
		yahoo_reject_buddy(id, who, "Thanks, but no thanks.");
*/
}

void ext_yahoo_typing_notify(int id, char *who, int stat)
{
	if(stat && do_typing_notify)
		print_message(("%s is typing...", who));
}

void ext_yahoo_game_notify(int id, char *who, int stat)
{
}

void ext_yahoo_mail_notify(int id, char *from, char *subj, int cnt)
{
	char buff[1024] = {0};
	
	if(!do_mail_notify)
		return;

	if(from && subj)
		snprintf(buff, sizeof(buff), 
				"You have new mail from %s about %s\n", 
				from, subj);
	if(cnt) {
		char buff2[100];
		snprintf(buff2, sizeof(buff2), 
				"You have %d message%s\n", 
				cnt, cnt==1?"":"s");
		strcat(buff, buff2);
	}

	if(buff[0])
		print_message((buff));
}

void ext_yahoo_got_webcam_key(int id, char *key)
{
	print_message(("Got webcam key: %s", key));
	if (ylad->webcam_key) FREE(ylad->webcam_key);
	ylad->webcam_key = strdup(key);

	switch (webcam_direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			yahoo_webcam_get_server(id, ylad->webcam_user);
			break;
		case YAHOO_WEBCAM_UPLOAD:
			yahoo_webcam_get_upload_server(id);
			break;
	}
}

void ext_yahoo_got_webcam_server(int id, char *ip)
{
	struct webcam *wcm = NULL;

	if (!ip || !ip[0])
	{
		print_message(("User does not have their webcam on"));
		return;
	}

	print_message(("Got webcam server: %s", ip));
	wcm = y_new0(struct webcam, 1);
	wcm->key = strdup(ylad->webcam_key);
	wcm->server = strdup(ip);
	wcm->my_ip = strdup(MY_IP);

	switch (webcam_direction)
	{
		case YAHOO_WEBCAM_DOWNLOAD:
			wcm->direction = YAHOO_WEBCAM_DOWNLOAD;
			wcm->user = strdup(ylad->webcam_user);
			break;
		case YAHOO_WEBCAM_UPLOAD:
			wcm->direction = YAHOO_WEBCAM_UPLOAD;
			wcm->description = strdup("Philips Webcam");
			webcamStart = get_time();
			rearm(&webcamTimer, 2);
			break;
	}

	/* Connect to webcam server */
	yahoo_webcam_connect(ylad->id, wcm);
}

void ext_yahoo_got_webcam_image(int id, unsigned char *image, unsigned int image_size, unsigned int real_size, unsigned int timestamp)
{
	static unsigned char *cur_image = NULL;
	static unsigned int cur_image_len = 0;
	static unsigned int image_num = 0;
	FILE* f_image;
	char fname[1024];

	/* copy image part to cur_image */
	if (real_size)
	{
		if (!cur_image) cur_image = y_new0(unsigned char, image_size);
		memcpy(cur_image + cur_image_len, image, real_size);
		cur_image_len += real_size;
	}

	if (image_size == cur_image_len)
	{
		print_message(("Received a image update at %d (%d bytes)",
			 timestamp, image_size));

		/* if we recieved an image then write it to file */
		if (image_size)
		{
			sprintf(fname, "images/%s_%.3d.jpc", ylad->webcam_user,				image_num++);

			if ((f_image = fopen(fname, "w")) != NULL) {
				fwrite(cur_image, image_size, 1, f_image);
				fclose(f_image);
			} else {
				printf("Error writing to %s\n", fname);
			}
			FREE(cur_image);
			cur_image_len = 0;
			if (image_num > 999) image_num = 0;
		}
	}
}

void ext_yahoo_webcam_invite(int id, char *from)
{
	print_message(("Got a webcam invitation from %s", from));
}

void ext_yahoo_webcam_invite_reply(int id, char *from, int accept)
{
	if(accept) {
		print_message(("%s accepted webcam invitation...", from));
	} else {
		print_message(("%s declined webcam invitation...", from));
	}
}

void ext_yahoo_system_message(int id, char *msg)
{
	if(ignore_system)
		return;

	print_message(("Yahoo System Message: %s", msg));
}

void yahoo_logout()
{
	if (ylad->id <= 0) {
		return;
	}

	pingTimer=0;

	while(conferences) {
		YList * n = conferences->next;
		conf_room * cr = conferences->data;
		if(cr->joined)
			yahoo_conference_logoff(ylad->id, NULL, cr->members, cr->room_name);
		FREE(cr->room_name);
		FREE(cr->host);
		while(cr->members) {
			YList *n = cr->members->next;
			FREE(cr->members->data);
			FREE(cr->members);
			cr->members=n;
		}
		FREE(cr);
		FREE(conferences);
		conferences = n;
	}
	
	yahoo_logoff(ylad->id);

	ylad->status = YAHOO_STATUS_OFFLINE;
	ylad->id = 0;

	poll_loop=0;
	print_message(("logged_out"));
}

void ext_yahoo_login(yahoo_local_account * ylad, int login_mode)
{
	LOG(("ext_yahoo_login"));

	ylad->id = yahoo_login(ylad->yahoo_id, ylad->password, login_mode);
	ylad->status = YAHOO_STATUS_OFFLINE;

	if (ylad->id <= 0) {
		print_message(("Could not connect to Yahoo server.  Please verify that you are connected to the net and the pager host and port are correctly entered."));
		return;
	}

	rearm(&pingTimer, 600);
}

void ext_yahoo_login_response(int id, int succ, char *url)
{
	char buff[1024];

	if(succ == YAHOO_LOGIN_OK) {
		ylad->status = yahoo_current_status(id);
		print_message(("logged in"));
		yahoo_get_yab(id);
		return;
		
	} else if(succ == YAHOO_LOGIN_PASSWD) {

		snprintf(buff, sizeof(buff), "Could not log into Yahoo service.  Please verify that your username and password are correctly typed.");

	} else if(succ == YAHOO_LOGIN_LOCK) {
		
		snprintf(buff, sizeof(buff), "Could not log into Yahoo service.  Your account has been locked.\nVisit %s to reactivate it.", url);

	} else if(succ == YAHOO_LOGIN_DUPL) {
		snprintf(buff, sizeof(buff), "You have been logged out of the yahoo service, possibly due to a duplicate login.");
	}

	ylad->status = YAHOO_STATUS_OFFLINE;
	print_message((buff));
	yahoo_logout();
	poll_loop=0;
}

void ext_yahoo_error(int id, char *err, int fatal)
{
	fprintf(stdout, "Yahoo Error: ");
	fprintf(stdout, "%s", err);
	/*
	switch(num) {
		case E_CONFNOTAVAIL:
			fprintf(stdout, "%s is not available for the conference", err);
			break;
		case E_IGNOREDUP:
			fprintf(stdout, "%s is already ignored", err);
			break;
		case E_IGNORENONE:
			fprintf(stdout, "%s is not in the ignore list", err);
			break;
		case E_IGNORECONF:
			fprintf(stdout, "%s is in buddy list - cannot ignore ", err);
			break;
	}
	*/
	fprintf(stdout, "\n");
	if(fatal)
		yahoo_logout();
}

void yahoo_set_current_state(int yahoo_state)
{
	if (ylad->status == YAHOO_STATUS_OFFLINE && yahoo_state != YAHOO_STATUS_OFFLINE) {
		ext_yahoo_login(ylad, yahoo_state);
		return;
	} else if (ylad->status != YAHOO_STATUS_OFFLINE && yahoo_state == YAHOO_STATUS_OFFLINE) {
		yahoo_logout();
		return;
	}

	ylad->status = yahoo_state;
	if(yahoo_state == YAHOO_STATUS_CUSTOM) {
		if(ylad->msg)
			yahoo_set_away(ylad->id, yahoo_state, ylad->msg, 1);
		else
			yahoo_set_away(ylad->id, yahoo_state, "delta p * delta x too large", 1);
	} else
		yahoo_set_away(ylad->id, yahoo_state, NULL, 1);
}

int ext_yahoo_connect(char *host, int port)
{
	struct sockaddr_in serv_addr;
	static struct hostent *server;
	static char last_host[256];
	int servfd;
	char **p;

	if(last_host[0] || strcasecmp(last_host, host)!=0) {
		if(!(server = gethostbyname(host))) {
			WARNING(("failed to look up server (%s:%d)\n%d: %s", 
						host, port,
						h_errno, strerror(h_errno)));
			return -1;
		}
		strncpy(last_host, host, 255);
	}

	if((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		WARNING(("Socket create error (%d): %s", errno, strerror(errno)));
		return -1;
	}

	LOG(("connecting to %s:%d", host, port));

	for (p = server->h_addr_list; *p; p++)
	{
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy(&serv_addr.sin_addr.s_addr, *p, server->h_length);
		serv_addr.sin_port = htons(port);

		LOG(("trying %s", inet_ntoa(serv_addr.sin_addr)));
		if(connect(servfd, (struct sockaddr *) &serv_addr, 
					sizeof(serv_addr)) == -1) {
			if(errno!=ECONNREFUSED && errno!=ETIMEDOUT && 
					errno!=ENETUNREACH) {
				break;
			}
		} else {
			LOG(("connected"));
			return servfd;
		}
	}

	WARNING(("Could not connect to %s:%d\n%d:%s", host, port, errno, 
				strerror(errno)));
	close(servfd);
	return -1;
}

/*************************************
 * Callback handling code starts here
 */
void yahoo_callback(int id)
{
	int ret=1;
	char buff[1024]={0};
	struct yahoo_data *yd = find_conn_by_id(id);

	if (!yd)
		return;

	ret = yahoo_read_ready(id, yd->fd);

	if(ret == -1)
		snprintf(buff, sizeof(buff), 
			"Yahoo read error (%d): %s", errno, strerror(errno));
	else if(ret == 0)
		snprintf(buff, sizeof(buff), 
			"Yahoo read error: Server closed socket");

	if(buff[0])
		print_message((buff));
}

void ext_yahoo_add_handler(int id, int fd, yahoo_input_condition cond)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	switch (yd->type)
	{
		case YAHOO_CONNECTION_PAGER:
			ylad->fd=fd;
			ylad->id=id;
			break;
		case YAHOO_CONNECTION_YAB:
			ylab->fd=fd;
			ylab->id=id;
			break;
		case YAHOO_CONNECTION_WEBCAM_MASTER:
			ywcm->fd=fd;
			ywcm->id=id;
			break;
		case YAHOO_CONNECTION_WEBCAM:
			ycam->fd=fd;
			ycam->id=id;
			break;
	}
}

void ext_yahoo_remove_handler(int id, int fd)
{
	struct yahoo_data *yd = find_conn_by_id(id);
	if (!yd)
		return;

	switch (yd->type)
	{
		case YAHOO_CONNECTION_PAGER:
			ylad->fd=0;
			ylad->id=0;
			break;
		case YAHOO_CONNECTION_YAB:
			ylab->fd=0;
			ylab->id=0;
			break;
		case YAHOO_CONNECTION_WEBCAM_MASTER:
			ywcm->fd=0;
			ywcm->id=0;
			break;
		case YAHOO_CONNECTION_WEBCAM:
			ycam->fd=0;
			ycam->id=0;
			break;
	}
	del_from_list(yd);
}
/*
 * Callback handling code ends here
 ***********************************/

static void process_commands(char *line)
{
	char *cmd, *to, *msg;

	char *tmp, *start;
	char *copy = strdup(line);

	enum yahoo_status state;

	start = cmd = copy;
	tmp = strchr(copy, ' ');
	if(tmp) {
		*tmp = '\0';
		copy = tmp+1;
	} else {
		copy = NULL;
	}

	if(!strncasecmp(cmd, "MSG", strlen("MSG"))) {
		/* send a message */
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;
		if(to && msg) {
			if(!strcmp(msg, "\a"))
				yahoo_send_im(ylad->id, NULL, to, "<ding>", 0);
			else {
				msg = y_str_to_utf8(msg);
				yahoo_send_im(ylad->id, NULL, to, msg, 1);
				FREE(msg);
			}
		}
	} else if(!strncasecmp(cmd, "CMS", strlen("CMS"))) {
		/* send a message */
		conf_room * cr;
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;
		cr = find_conf_room_by_name_and_id(ylad->id, to);
		if(!cr) {
			print_message(("no such room: %s", copy));
			goto end_parse;
		}
		if(msg)
			yahoo_conference_message(ylad->id, NULL, cr->members, to, msg, 0);
	} else if(!strncasecmp(cmd, "CLS", strlen("CLS"))) {
		YList * l;
		if(copy) {
			conf_room * cr = find_conf_room_by_name_and_id(ylad->id, copy);
			if(!cr) {
				print_message(("no such room: %s", copy));
				goto end_parse;
			}
			print_message(("Room: %s", copy));
			for(l = cr->members; l; l=l->next) {
				print_message(("%s", (char *)l->data))
			}
		} else {
			print_message(("All Rooms:"));
			for(l = conferences; l; l=l->next) {
				conf_room * cr = l->data;
				print_message(("%s", cr->room_name));
			}
		}

	} else if(!strncasecmp(cmd, "CCR", strlen("CCR"))) {
		conf_room * cr = y_new0(conf_room, 1);
		while((tmp = strchr(copy, ' ')) != NULL) {
			*tmp = '\0';
			if(!cr->room_name)
				cr->room_name = strdup(copy);
			else
				cr->members = y_list_append(cr->members,
						strdup(copy));
			copy = tmp+1;
		}
		cr->members = y_list_append(cr->members, strdup(copy));

		if(!cr->room_name || !cr->members) {
			FREE(cr);
		} else {
			cr->id = ylad->id;
			cr->joined = 1;
			conferences = y_list_append(conferences, cr);
			yahoo_conference_invite(ylad->id, NULL, cr->members, cr->room_name, "Join my conference");
			cr->members = y_list_append(cr->members,strdup(ylad->yahoo_id));
		}
	} else if(!strncasecmp(cmd, "CIN", strlen("CIN"))) {
		conf_room * cr;
		char * room=copy;
		YList * l1, *l = NULL;

		while((tmp = strchr(copy, ' ')) != NULL) {
			*tmp = '\0';
			copy = tmp+1;
			l = y_list_append(l, copy);
		}

		cr = find_conf_room_by_name_and_id(ylad->id, room);
		if(!cr) {
			print_message(("no such room: %s", room));
			y_list_free(l);
			goto end_parse;
		}

		for(l1 = l; l1; l1=l1->next) {
			char * w = l1->data;
			yahoo_conference_addinvite(ylad->id, NULL, w, room, cr->members, "Join my conference");
			cr->members = y_list_append(cr->members, strdup(w));
		}
		y_list_free(l);

	} else if(!strncasecmp(cmd, "CLN", strlen("CLN"))) {
		conf_room * cr = find_conf_room_by_name_and_id(ylad->id, copy);
		YList * l;
		if(!cr) {
			print_message(("no such room: %s", copy));
			goto end_parse;
		}

		cr->joined = 1;
		for(l = cr->members; l; l=l->next) {
			char * w = l->data;
			if(!strcmp(w, ylad->yahoo_id))
				break;
		}
		if(!l)
			cr->members = y_list_append(cr->members, strdup(ylad->yahoo_id));
		yahoo_conference_logon(ylad->id, NULL, cr->members, copy);

	} else if(!strncasecmp(cmd, "CLF", strlen("CLF"))) {
		conf_room * cr = find_conf_room_by_name_and_id(ylad->id, copy);
		
		if(!cr) {
			print_message(("no such room: %s", copy));
			goto end_parse;
		}

		yahoo_conference_logoff(ylad->id, NULL, cr->members, copy);

		conferences = y_list_remove(conferences, cr);
		FREE(cr->room_name);
		FREE(cr->host);
		while(cr->members) {
			YList *n = cr->members->next;
			FREE(cr->members->data);
			FREE(cr->members);
			cr->members=n;
		}
		FREE(cr);

	} else if(!strncasecmp(cmd, "CDC", strlen("CDC"))) {
		conf_room * cr;
		char * room = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
			msg = copy;
		} else {
			msg = "Thanks, but no thanks!";
		}
		
		cr = find_conf_room_by_name_and_id(ylad->id, room);
		if(!cr) {
			print_message(("no such room: %s", room));
			goto end_parse;
		}

		yahoo_conference_decline(ylad->id, NULL, cr->members, room,msg);

		conferences = y_list_remove(conferences, cr);
		FREE(cr->room_name);
		FREE(cr->host);
		while(cr->members) {
			YList *n = cr->members->next;
			FREE(cr->members->data);
			FREE(cr->members);
			cr->members=n;
		}
		FREE(cr);


	} else if(!strncasecmp(cmd, "CHJ", strlen("CHJ"))) {
		char *roomid, *roomname;
/* Linux, FreeBSD, Solaris:1 */
/* 1600326591 */
		roomid = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		roomname = copy;
		if(roomid && roomname) {
			yahoo_chat_logon(ylad->id, NULL, roomname, roomid);
		}

	} else if(!strncasecmp(cmd, "CHM", strlen("CHM"))) {
		char *msg, *roomname;
		roomname = copy;
		tmp = strstr(copy, "  ");
		if(tmp) {
			*tmp = '\0';
			copy = tmp+2;
		}
		msg = copy;
		if(roomname && msg) {
			yahoo_chat_message(ylad->id, NULL, roomname, msg);
		}

	} else if(!strncasecmp(cmd, "CHX", strlen("CHX"))) {
		yahoo_chat_logoff(ylad->id, NULL);
	} else if(!strncasecmp(cmd, "STA", strlen("STA"))) {
		if(isdigit(copy[0])) {
			state = (enum yahoo_status)atoi(copy);
			copy = strchr(copy, ' ');
			if(state == 99) {
				if(copy)
					msg = copy;
				else
					msg = "delta x * delta p too large";
			} else
				msg = NULL;
		} else {
			state = YAHOO_STATUS_CUSTOM;
			msg = copy;
		}

		yahoo_set_away(ylad->id, state, msg, 1);

	} else if(!strncasecmp(cmd, "OFF", strlen("OFF"))) {
		/* go offline */
		poll_loop=0;
	} else if(!strncasecmp(cmd, "IDS", strlen("IDS"))) {
		/* print identities */
		const YList * ids = yahoo_get_identities(ylad->id);
		printf("Identities: ");
		for(; ids; ids = ids->next)
			printf("%s, ", (char *)ids->data);
		printf("\n");
	} else if(!strncasecmp(cmd, "AID", strlen("AID"))) {
		/* activate identity */
		yahoo_set_identity_status(ylad->id, copy, 1);
	} else if(!strncasecmp(cmd, "DID", strlen("DID"))) {
		/* deactivate identity */
		yahoo_set_identity_status(ylad->id, copy, 0);
	} else if(!strncasecmp(cmd, "LST", strlen("LST"))) {
		YList * b = buddies;
		for(; b; b=b->next) {
			yahoo_account * ya = b->data;
			if(ya->status == YAHOO_STATUS_OFFLINE)
				continue;
			if(ya->msg)
				print_message(("%s (%s) is now %s", ya->name, ya->yahoo_id, 
							ya->msg))
			else
				print_message(("%s (%s) is now %s", ya->name, ya->yahoo_id, 
						yahoo_status_code(ya->status)))
		}
	} else if(!strncasecmp(cmd, "NAM", strlen("NAM"))) {
		struct yab * yab;
		
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;

		if(to && msg) {
			yab = y_new0(struct yab, 1);
			yab->id = to;
			yab->fname = msg;
			yahoo_set_yab(ylad->id, yab);
			FREE(yab);
		}
	} else if(!strncasecmp(cmd, "WCAM", strlen("WCAM"))) {
		if (copy)
		{
			printf("Viewing webcam (%s)\n", copy);
			if (ylad->webcam_user) FREE(ylad->webcam_user);
			ylad->webcam_user = strdup(copy);
			webcam_direction = YAHOO_WEBCAM_DOWNLOAD;
			yahoo_webcam_get_key(ylad->id, copy);
		} else {
			printf("Starting webcam\n");
			webcam_direction = YAHOO_WEBCAM_UPLOAD;
			yahoo_webcam_get_upload_key(ylad->id);
		}
	} else if(!strncasecmp(cmd, "WINV", strlen("WINV"))) {
		printf("Inviting %s to view webcam\n", copy);
		yahoo_webcam_invite(ylad->id, copy);
	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
	}

end_parse:
	FREE(start);
}

static void local_input_callback(int source)
{
	char line[1024] = {0};
	int i;
	char c;
	i=0; c=0;
	do {
		if(read(source, &c, 1) <= 0)
			c='\0';
		if(c == '\r')
			continue;
		if(c == '\n')
			break;
		if(c == '\b') {
			if(!i)
				continue;
			c = '\0';
			i--;
		}
		if(c) {
			line[i++] = c;
			line[i]='\0';
		}
	} while(i<1023 && c != '\n');

	if(line[0])
		process_commands(line);
}

int main(int argc, char * argv[])
{
	int status;
	int log_level;
	int lfd=0;

	fd_set inp;
	struct timeval tv;


	int fd_stdin = fileno(stdin);

	ylad = y_new0(yahoo_local_account, 1);
	ylab = y_new0(yahoo_connection, 1);
	ywcm = y_new0(yahoo_connection, 1);
	ycam = y_new0(yahoo_connection, 1);

	printf("Yahoo Id: ");
	scanf("%s", ylad->yahoo_id);
	printf("Password: ");
	{
		tcflag_t oflags;
		struct termios term;
		tcgetattr(fd_stdin, &term);
		oflags = term.c_lflag;
		term.c_lflag = oflags & ~(ECHO | ECHOK | ICANON);
		term.c_cc[VTIME] = 1;
		tcsetattr(fd_stdin, TCSANOW, &term);
		
		scanf("%s", ylad->password);

		term.c_lflag = oflags;
		term.c_cc[VTIME] = 0;
		tcsetattr(fd_stdin, TCSANOW, &term);
	}
	printf("\n");

	printf("Initial Status: ");
	scanf("%d", &status);

	printf("Log Level: ");
	scanf("%d", &log_level);
	do_yahoo_debug=log_level;

	register_callbacks();
	yahoo_set_log_level(log_level);

	ext_yahoo_login(ylad, status);

	while(poll_loop) {
		FD_ZERO(&inp);
		FD_SET(fd_stdin, &inp);
		if (ylad->fd) FD_SET(ylad->fd, &inp);
		if (ylab->fd) FD_SET(ylab->fd, &inp);
		if (ywcm->fd) FD_SET(ywcm->fd, &inp);
		if (ycam->fd) FD_SET(ycam->fd, &inp);
		tv.tv_sec=1;
		tv.tv_usec=0;
		lfd=0;
		if(lfd < ylad->fd) lfd=ylad->fd;
		if(lfd < ylab->fd) lfd=ylab->fd;
		if(lfd < ywcm->fd) lfd=ywcm->fd;
		if(lfd < ycam->fd) lfd=ycam->fd;
		select(lfd + 1, &inp, NULL, NULL, &tv);
		time(&curTime);

		if(expired(pingTimer))		yahoo_ping_timeout_callback();
		if(expired(webcamTimer))	yahoo_webcam_timeout_callback(ycam->id);
		if(FD_ISSET(fd_stdin, &inp))	local_input_callback(0);
		if(ylad->fd && FD_ISSET(ylad->fd, &inp)) yahoo_callback(ylad->id);
		if(ylab->fd && FD_ISSET(ylab->fd, &inp)) yahoo_callback(ylab->id);
		if(ywcm->fd && FD_ISSET(ywcm->fd, &inp)) yahoo_callback(ywcm->id);
		if(ycam->fd && FD_ISSET(ycam->fd, &inp)) yahoo_callback(ycam->id);
	}

	yahoo_logout();

	FREE(ylad);
	FREE(ylab);
	FREE(ywcm);
	FREE(ycam);

	return 0;
}

void ext_yahoo_got_file(int id, char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize)
{
}

void ext_yahoo_got_identities(int id, YList * ids)
{
}

static void register_callbacks()
{
#ifdef USE_STRUCT_CALLBACKS
	static struct yahoo_callbacks yc;

	yc.ext_yahoo_login_response = ext_yahoo_login_response;
	yc.ext_yahoo_got_buddies = ext_yahoo_got_buddies;
	yc.ext_yahoo_got_ignore = ext_yahoo_got_ignore;
	yc.ext_yahoo_got_identities = ext_yahoo_got_identities;
	yc.ext_yahoo_status_changed = ext_yahoo_status_changed;
	yc.ext_yahoo_got_im = ext_yahoo_got_im;
	yc.ext_yahoo_got_conf_invite = ext_yahoo_got_conf_invite;
	yc.ext_yahoo_conf_userdecline = ext_yahoo_conf_userdecline;
	yc.ext_yahoo_conf_userjoin = ext_yahoo_conf_userjoin;
	yc.ext_yahoo_conf_userleave = ext_yahoo_conf_userleave;
	yc.ext_yahoo_conf_message = ext_yahoo_conf_message;
	yc.ext_yahoo_chat_userjoin = ext_yahoo_chat_userjoin;
	yc.ext_yahoo_chat_userleave = ext_yahoo_chat_userleave;
	yc.ext_yahoo_chat_message = ext_yahoo_chat_message;
	yc.ext_yahoo_got_file = ext_yahoo_got_file;
	yc.ext_yahoo_contact_added = ext_yahoo_contact_added;
	yc.ext_yahoo_rejected = ext_yahoo_rejected;
	yc.ext_yahoo_typing_notify = ext_yahoo_typing_notify;
	yc.ext_yahoo_game_notify = ext_yahoo_game_notify;
	yc.ext_yahoo_mail_notify = ext_yahoo_mail_notify;
	yc.ext_yahoo_system_message = ext_yahoo_system_message;
	yc.ext_yahoo_error = ext_yahoo_error;
	yc.ext_yahoo_log = ext_yahoo_log;
	yc.ext_yahoo_add_handler = ext_yahoo_add_handler;
	yc.ext_yahoo_remove_handler = ext_yahoo_remove_handler;
	yc.ext_yahoo_connect = ext_yahoo_connect;

	yahoo_register_callbacks(&yc);
	
#endif
}

