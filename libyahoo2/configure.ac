dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)

AC_INIT(libyahoo2, 0.6.3)
RELEASE="1"

AC_CONFIG_SRCDIR(config.h.in)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

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
YAHOO2_CURRENT=3	# version 1 was pre 0.6
			# version 2 was pre utf8 (0.6.2)
YAHOO2_REVISION=0
YAHOO2_AGE=0
YAHOO2_VERSION="$YAHOO2_CURRENT:$YAHOO2_REVISION:$YAHOO2_AGE"
AC_SUBST(YAHOO2_VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strchr strcspn strdup strncasecmp strrchr strstr strtol])

AC_ARG_WITH([struct-callbacks], [AC_HELP_STRING([--with-struct-callbacks],
[use a callback structure instead of callback functions])])
if test "$with_struct_callbacks" = "yes"; then
	AC_DEFINE(USE_STRUCT_CALLBACKS, 1, 
	[Define if you want to use a callback structure instead of callback functions])
fi

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

if echo "$RELEASE" | grep '\.' >/dev/null 2>&1; then
	RELEASE="$RELEASE$DATE"
fi
AC_SUBST(RELEASE)

AC_CONFIG_FILES([
	Makefile libyahoo2.pc libyahoo2.spec 
	src/Makefile 
	doc/Makefile
])

AC_OUTPUT
