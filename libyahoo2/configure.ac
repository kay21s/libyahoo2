dnl Process this file with autoconf to produce a configure script.
AC_INIT(libyahoo2, 0.7.7, libyahoo2-users@lists.sourceforge.net)
AC_PREREQ(2.53)
AC_CONFIG_SRCDIR(src/libyahoo2.c)
AM_INIT_AUTOMAKE([1.6 dist-bzip2])

AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

CFLAGS="$CFLAGS -Wall -ansi -pedantic"

dnl ######################################################################
dnl # libtool versioning
dnl ######################################################################
dnl #
dnl # +1 :  0 : +1   == new interface that does not break old one.
dnl # +1 :  0 :  0   == removed/changed an interface. Breaks old apps.
dnl #  ? : +1 :  ?   == internal changes that doesn't break anything.
dnl #
dnl # CURRENT : REVISION : AGE
YAHOO2_CURRENT=10	# 1 - < 0.6
			# 2  - pre utf8 (0.6.2)
			# 3  - pre webcam/chat support (0.6.3)
			# 4  - 0.7.0
			# 5  - 0.7.1-2 - yahoo_init_with_attributes
			# 6  - yahoo_get_log_level() (0.7.2)
			# 7  - contact search, add/remove handler (0.7.3)
			# 8  - remove_handler takes client id (0.7.4)
			# 9  - add_buddy takes buddy add message (0.7.5)
			# 10 - Windows support (0.7.6)
YAHOO2_REVISION=1
YAHOO2_AGE=0
YAHOO2_VERSION="$YAHOO2_CURRENT:$YAHOO2_REVISION:$YAHOO2_AGE"
AC_SUBST(YAHOO2_VERSION)

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_LIBTOOL
AC_PROG_INSTALL

# Checks for libraries.
enable_sample_client="yes"
AC_SEARCH_LIBS(socket, socket, ,
	[AC_SEARCH_LIBS(socket, socket, LIBS="$LIBS -lnsl", 
		enable_sample_client="no", -lnsl)])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM

AC_CHECK_TYPE([uint8_t],,
[AC_DEFINE([uint8_t], [unsigned char],
[Define to `unsigned char' if not defined.])])
AC_CHECK_TYPE([uint32_t],,
[AC_DEFINE([uint32_t], [unsigned int],
[Define to `unsigned int' if not defined.])])
AC_CHECK_TYPE([uint64_t],,
[AC_DEFINE([uint64_t], [unsigned long long],
[Define to `unsigned long long' if not defined.])])

dnl Checks for library functions.
AC_CHECK_FUNCS(strerror)


# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strchr strcspn strdup strncasecmp strrchr strstr strtol])

AC_ARG_WITH([struct-callbacks], [AC_HELP_STRING([--with-struct-callbacks],
[use a callback structure instead of callback functions])])
if test "$with_struct_callbacks" = "yes"; then
	AC_DEFINE(USE_STRUCT_CALLBACKS, 1, 
	[Define if you want to use a callback structure instead of callback functions])
fi

AC_ARG_ENABLE([sample-client], [AC_HELP_STRING([--disable-sample-client],
[do not compile the sample client program])])
AM_CONDITIONAL(SAMPLE_CLIENT, test "$enable_sample_client" != "no")

YAHOOPKGREQ=""
AC_ARG_WITH([glib], [AC_HELP_STRING([--with-glib], [Which version of glib to use: 0/no = none, 1 = glib 1.2, 2 = glib 2, yes = auto.  Default is auto.])])
if test "$with_glib" = "1"; then
	AM_PATH_GLIB(1.2.8, [
		AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib support])
		YAHOOPKGREQ="glib >= 1.2.8"
	])
elif test "$with_glib" = "2"; then
	AM_PATH_GLIB_2_0(2.0.0, [
		AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib support])
		YAHOOPKGREQ="glib-2.0"
	])
elif test "$with_glib" != "no" -a "$with_glib" != "0"; then
	AM_PATH_GLIB_2_0(2.0.0, [
		AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib support])
		YAHOOPKGREQ="glib-2.0"
	], [AM_PATH_GLIB(1.2.8, [
		AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib support])
		YAHOOPKGREQ="glib >= 1.2.8"
		]
	)]
	)
fi

AC_SUBST(YAHOOPKGREQ)
AC_SUBST_FILE(CHANGELOG)
CHANGELOG=ChangeLog
AC_SUBST(DATE)
DATE=`date +"%Y%m%d"`

AC_CONFIG_FILES([
	Makefile libyahoo2.pc libyahoo2.spec 
	src/Makefile 
  src/windows/Makefile
  src/windows/vc6/Makefile
  src/windows/vc7/Makefile
  src/windows/vc8/Makefile
  src/windows/vc9/Makefile
	doc/Makefile
])

AC_OUTPUT
