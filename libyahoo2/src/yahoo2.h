/*
 * libyahoo2: yahoo2.h
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

#ifndef YAHOO2_H
#define YAHOO2_H

#ifdef __cplusplus
extern "C" {
#endif

#include "yahoo2_types.h"

/* returns the socket descriptor for a given connection. shouldn't be needed */
int  yahoo_get_fd(int id);

/* says how much logging to do */
/* see yahoo2_types.h for the different values */
int  yahoo_set_log_level(enum yahoo_log_level level);

/* these functions should be self explanatory */
/* who always means the buddy you're acting on */
/* id is the successful value returned by yahoo_login */


/* login returns a connection id used to identify the connection hereon */
/* initial is of type enum yahoo_status.  see yahoo2_types.h */
/* returns <= 0 on error - whatever ext_yahoo_connect returned */
int  yahoo_login(const char *username, const char *password, int initial);
void yahoo_logoff(int id);
/* reloads status of all buddies */
void yahoo_refresh(int id);
/* activates/deactivates an identity */
void yahoo_set_identity_status(int id, const char * identity, int active);
/* regets the entire buddy list from the server */
void yahoo_get_list(int id);
/* download buddy contact information from your yahoo addressbook */
void yahoo_get_yab(int id);
/* add/modify an address book entry.  if yab->dbid is set, it will */
/* modify that entry else it creates a new entry */
void yahoo_set_yab(int id, struct yab * yab);
void yahoo_keepalive(int id);

/* from is the identity you're sending from.  if NULL, the default is used */
/* utf8 is whether msg is a utf8 string or not. */
void yahoo_send_im(int id, const char *from, const char *who, const char *msg, int utf8);
/* if type is true, send typing notice, else send stopped typing notice */
void yahoo_send_typing(int id, const char *from, const char *who, int typ);

/* used to set away/back status. */
/* away says whether the custom message is an away message or a sig */
void yahoo_set_away(int id, enum yahoo_status state, const char *msg, int away);

void yahoo_add_buddy(int id, const char *who, const char *group);
void yahoo_remove_buddy(int id, const char *who, const char *group);
void yahoo_reject_buddy(int id, const char *who, const char *msg);
/* if unignore is true, unignore, else ignore */
void yahoo_ignore_buddy(int id, const char *who, int unignore);
void yahoo_change_buddy_group(int id, const char *who, const char *old_group, const char *new_group);
void yahoo_group_rename(int id, const char *old_group, const char *new_group);

void yahoo_conference_invite(int id, const char * from, YList *who, const char *room, const char *msg);
void yahoo_conference_addinvite(int id, const char * from, const char *who, const char *room, const YList * members, const char *msg);
void yahoo_conference_decline(int id, const char * from, YList *who, const char *room, const char *msg);
void yahoo_conference_message(int id, const char * from, YList *who, const char *room, const char *msg, int utf8);
void yahoo_conference_logon(int id, const char * from, YList *who, const char *room);
void yahoo_conference_logoff(int id, const char * from, YList *who, const char *room);

/* join room with specified roomname and roomid */
void yahoo_chat_logon(int id, const char *from, const char *room, const char *roomid);
/* Send message "msg" to room with specified roomname, msgtype is 1-normal message or 2-/me mesage */
void yahoo_chat_message(int id, const char *from, const char *room, const char *msg, const int msgtype, const int utf8);
/* Log off chat */
void yahoo_chat_logoff(int id, const char *from);

/* requests a key which is needed to connect to the webcam server */
/* who is the person who's webcam you would like to view */
void yahoo_webcam_get_key(int id, const char *who);

/* requests a key which is needed to connect to a webcam server for uploading */
void yahoo_webcam_get_upload_key(int id);

/* requests a webcam server to connect to */
/* who is the person who's webcam you would like to view */
void yahoo_webcam_get_server(int id, char *who);

/* requests a webcam server to connect to for uploading */
void yahoo_webcam_get_upload_server(int id);

/* connect to a webcam server */
/* the webcam struct should be filled with at least the key and the server */
/* when viewing a webcam the user should be filled */
/* when uploading the webcam description should be filled */
/* my_ip should be filled with your own ip, even if it's a private ip */
/* direction should be set to YAHOO_WEBCAM_DOWNLOAD or YAHOO_WEBCAM_UPLOAD */
/* conn_type should be one of the following 0=Dialup, 1=DSL/Cable, 2=T1/Lan */
void yahoo_webcam_connect(int id, struct yahoo_webcam *wcm);

/* sends an image when uploading */
/* image points to a JPEG-2000 image, lenght is the length of the image */
/* in bytes. The timestamp is the time in milliseconds since we started the */
/* webcam. */
void yahoo_webcam_send_image(int id, unsigned char *image, unsigned int length, unsigned int timestamp);

/* this function should be called if we want to allow a user to watch the */
/* webcam. Who is the user we want to accept. */
/* Accept user (accept = 1), decline user (accept = 0) */
void yahoo_webcam_accept_viewer(int id, const char* who, int accept);

/* send an invitation to a user to view your webcam */
void yahoo_webcam_invite(int id, const char *who);


/* returns a socket file descriptor to the upload stream. */
/* you should write your data to this stream when it returns */
int  yahoo_send_file(int id, const char *who, const char *msg, const char *name, long size);
/* returns a socket fd to a url for downloading a file. */
int yahoo_get_url_handle(int id, const char *url, char *filename, unsigned long *filesize);

/* these should be called when input is available on a fd */
/* registered by ext_yahoo_add_handler */
/* if these return negative values, errno may be set */
int  yahoo_read_ready(int id, int fd);
int  yahoo_write_ready(int id, int fd);

/* utility functions. these do not hit the server */
enum yahoo_status yahoo_current_status(int id);
const YList * yahoo_get_buddylist(int id);
const YList * yahoo_get_ignorelist(int id);
const YList * yahoo_get_identities(int id);
/* 'which' could be y, t, c or login.  This may change in later versions. */
const char  * yahoo_get_cookie(int id, const char *which);

/* returns the url used to get user profiles - you must append the user id */
/* as of now this is http://profiles.yahoo.com/ */
/* You'll have to do urlencoding yourself, but see yahoo_httplib.h first */
const char  * yahoo_get_profile_url( void );

#include "yahoo_httplib.h"

#ifdef __cplusplus
}
#endif

#endif
