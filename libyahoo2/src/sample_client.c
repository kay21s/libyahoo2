/*
 * sample yahoo client based on libyahoo2
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

#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#include <yahoo2.h>
#include <yahoo2_callbacks.h>
#include "yahoo_util.h"

#define MAX_PREF_LEN 255

static int do_mail_notify = 0;
static int do_yahoo_debug = 0;
static int ignore_system = 0;
static int do_typing_notify = 1;

static int poll_fd=0;
static int poll_id=0;
static int poll_loop=1;

/* Exported to libyahoo2 */
char pager_host[MAX_PREF_LEN]="scs.yahoo.com";
char pager_port[MAX_PREF_LEN]="5050";
char filetransfer_host[MAX_PREF_LEN]="filetransfer.msg.yahoo.com";
char filetransfer_port[MAX_PREF_LEN]="80";

typedef struct {
	char yahoo_id[255];
	char password[255];
	int id;
	int fd;
	int status;
	char *status_message;
} yahoo_local_account;

typedef struct {
	int id;
	char *label;
} yahoo_idlabel;

typedef struct {
	int id;
	char *who;
} yahoo_authorize_data;

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
	{YAHOO_STATUS_TYPING, "Typing"},
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

int yahoo_ping_timeout_callback()
{
	yahoo_keepalive(poll_id);
	return 1;
}

void ext_yahoo_status_changed(int id, char *who, int stat, char *msg, int away)
{
	if(msg)
		print_message(("%s is now %s", who, msg))
	else
		print_message(("%s is now %s", who, 
					yahoo_status_code(stat)))
}

void ext_yahoo_got_buddies(int id, YList * buds)
{
}

void ext_yahoo_got_ignore(int id, YList * igns)
{
}

void ext_yahoo_got_im(int id, char *who, char *msg, long tm, int stat)
{
	if(stat == 2) {
		LOG(("Error sending message to %s", who));
		return;
	}

	if(!msg)
		return;

	if(tm) {
		char timestr[255];

		strncpy(timestr, ctime(&tm), sizeof(timestr));
		timestr[strlen(timestr) - 1] = '\0';

		print_message(("[Offline message at %s from %s]: %s", 
				timestr, who, msg));
	} else {
		print_message(("%s: %s", who, msg));
	}
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

void ext_yahoo_system_message(int id, char *msg)
{
	if(ignore_system)
		return;

	print_message(("Yahoo System Message: %s", msg));
}

int call_timeout=0;
void yahoo_logout()
{
	if (ylad->id <= 0) {
		return;
	}

	call_timeout=0;
	
	yahoo_logoff(ylad->id);

	ylad->status = YAHOO_STATUS_OFFLINE;
	ylad->id = 0;

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

	call_timeout=1;
}

void ext_yahoo_login_response(int id, int succ, char *url)
{
	char buff[1024];

	if(succ == YAHOO_LOGIN_OK) {
		ylad->status = yahoo_current_status(id);
		print_message(("logged in"));
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
	fprintf(stderr, "Yahoo Error: %s", err);
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
		if(ylad->status_message)
			yahoo_set_away(ylad->id, yahoo_state, ylad->status_message, 1);
		else
			yahoo_set_away(ylad->id, yahoo_state, "delta p * delta x too large", 1);
	} else
		yahoo_set_away(ylad->id, yahoo_state, NULL, 1);
}

int ext_yahoo_connect(char *host, int port)
{
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int servfd;
	int res;
	char **p;

	if(!(server = gethostbyname(host))) {
		WARNING(("failed to look up server (%s:%d)\n%d: %s", host, port,
					h_errno, hstrerror(h_errno)));
		return -1;
	}

	if((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		WARNING(("Socket create error (%d): %s", errno, 
					strerror(errno)));
		return -1;
	}

	LOG(("connecting to %s:%d\n", host, port));

	for (p = server->h_addr_list; *p; p++)
	{
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		memcpy(&serv_addr.sin_addr.s_addr, *p, server->h_length);
		serv_addr.sin_port = htons(port);

		res = -1;
		res = connect(servfd, (struct sockaddr *) &serv_addr, 
				sizeof(serv_addr));

		if(res == 0 ) {
			LOG(("connected"));
			return servfd;
		}
	}

	WARNING(("Could not connect to %s:%d\n%d:%s", host, port,
				errno, strerror(errno)));
	close(servfd);
	return -1;
}

/*************************************
 * Callback handling code starts here
 */
void yahoo_callback(int source)
{
	int ret=1;
	char buff[1024]={0};

	ret = yahoo_read_ready(poll_id, source);

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
	poll_fd=fd;
	poll_id=id;
}

void ext_yahoo_remove_handler(int id, int fd)
{
	poll_fd=0;
	poll_id=0;
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
	}

	if(!strcasecmp(cmd, "MSG")) {
		// send a message
		to = copy;
		tmp = strchr(copy, ' ');
		if(tmp) {
			*tmp = '\0';
			copy = tmp+1;
		}
		msg = copy;
		if(to && msg)
			yahoo_send_im(ylad->id, to, msg);
		FREE(start);
		return;
	} else if(!strcasecmp(cmd, "STA")) {
		if(isdigit(copy[0])) {
			state = (enum yahoo_status)atoi(copy);
			msg = NULL;
		} else {
			state = YAHOO_STATUS_CUSTOM;
			msg = copy;
		}

		yahoo_set_away(ylad->id, state, msg, 1);

		return;
	} else if(!strcasecmp(cmd, "OFF")) {
		// go offline
		FREE(start);
		poll_loop=0;
		return;
	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		FREE(start);
		return;
	}
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

	fd_set inp;
	struct timeval tv;

	ylad = y_new0(yahoo_local_account, 1);

	printf("Yahoo Id: ");
	scanf("%s", ylad->yahoo_id);
	printf("Password: ");
	scanf("%s", ylad->password);

	printf("Initial Status: ");
	scanf("%d", &status);

	printf("Log Level: ");
	scanf("%d", &log_level);

	yahoo_set_log_level(log_level);

	ext_yahoo_login(ylad, status);

	while(poll_loop) {
		FD_ZERO(&inp);
		FD_SET(0, &inp);
		FD_SET(poll_fd, &inp);
		tv.tv_sec=600000;
		tv.tv_usec=0;
		select(poll_fd + 1, &inp, NULL, NULL, &tv);

		if(call_timeout)		yahoo_ping_timeout_callback();
		if(FD_ISSET(0, &inp)) 		local_input_callback(0);
		if(FD_ISSET(poll_fd, &inp))	yahoo_callback(poll_fd);
	}

	yahoo_logout();

	return 0;
}

void ext_yahoo_got_conf_invite(int id, char *who, char *room, char *msg, YList *members)
{
}
void ext_yahoo_conf_userdecline(int id, char *who, char *room, char *msg)
{
}
void ext_yahoo_conf_userjoin(int id, char *who, char *room)
{
}
void ext_yahoo_conf_userleave(int id, char *who, char *room)
{
}
void ext_yahoo_conf_message(int id, char *who, char *room, char *msg)
{
}
void ext_yahoo_got_file(int id, char *who, char *url, long expires, char *msg, char *fname, unsigned long fesize)
{
}
