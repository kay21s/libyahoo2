lib_LTLIBRARIES = libyahoo2.la
noinst_PROGRAMS = yahoo

EXTRA_DIST = md5.h yahoo_connections.h yahoo_debug.h \
	sample_client.c sample_makefile yahoo_util.h

CFLAGS = @CFLAGS@ $(GLIB_CFLAGS)
LIBS = @LIBS@ $(GLIB_LIBS)

libyahoo2_la_SOURCES = libyahoo2.c crypt.c md5.c yahoo_connections.c \
	yahoo_httplib.c yahoo_util.c yahoo_list.c

libyahoo2_la_LDFLAGS = -module -version-info $(YAHOO2_VERSION)

pkginclude_HEADERS = yahoo2.h yahoo2_callbacks.h yahoo2_types.h \
	yahoo_httplib.h yahoo_list.h

yahoo_SOURCES = sample_client.c
yahoo_LDADD = libyahoo2.la
yahoo_LDFLAGS = -static
