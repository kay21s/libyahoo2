dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/libyahoo2.c)
AM_INIT_AUTOMAKE([libyahoo2], [0.6cvs])
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_ARG_WITH([struct-callbacks], [AC_HELP_STRING([--with-struct-callbacks],
[use a callback structure instead of callback functions])])
if test "$with_callback_struct" = "yes"; then
	AC_DEFINE(USE_STRUCT_CALLBACKS, 1, 
	[Define if you want to use a callback structure instead of callback functions])
fi

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

AM_PATH_GLIB(1.2.0, [AC_DEFINE(HAVE_GLIB, 1, [Define if you have glib 1.2.0 or higher])])
CFLAGS="$CFLAGS -Wall"

AC_OUTPUT([Makefile src/Makefile])
