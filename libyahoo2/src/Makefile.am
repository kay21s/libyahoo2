lib_LTLIBRARIES = libyahoo2.la

EXTRA_DIST = md5.h yahoo_connections.h yahoo_debug.h \
	sample_client.c sample_makefile yahoo_util.h

libyahoo2_la_SOURCES = libyahoo2.c crypt.c md5.c yahoo_connections.c \
	yahoo_httplib.c yahoo_util.c

libyahoo2_la_LDFLAGS = -module

include_HEADERS = yahoo2.h yahoo2_callbacks.h yahoo2_types.h \
	yahoo_httplib.h
