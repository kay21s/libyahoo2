dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)

AC_INIT(libyahoo2, 0.6.2cvs)
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
YAHOO2_CURRENT=1
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

AC_CHECK_TYPES(u_char)

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
AM_PATH_GLIB_2_0(2.0.0, [
	AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib support])
	YAHOOPKGREQ="glib-2.0"
	], [AM_PATH_GLIB(1.2.8, [
		AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib support])
		YAHOOPKGREQ="glib >= 1.2.8"
		]
	)]
)
AC_SUBST(YAHOOPKGREQ)

AC_CONFIG_FILES([Makefile libyahoo2.pc src/Makefile doc/Makefile])

AC_OUTPUT
