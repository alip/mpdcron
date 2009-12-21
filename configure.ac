dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT([src/cron-main.c])

VERSION_MAJOR=0
VERSION_MINOR=2
VERSION_FULL="$VERSION_MAJOR.$VERSION_MINOR"
VERSION="$VERSION_FULL"

AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_FULL])

AM_INIT_AUTOMAKE(mpdcron, [$VERSION_FULL])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl {{{ git revision
AC_MSG_CHECKING([for git head])
if test -d "${GIT_DIR:-${ac_top_srcdir:-./}/.git}" ; then
	GITHEAD=`git describe 2>/dev/null`
	if test -z ${GITHEAD} ; then
		GITHEAD=`git rev-parse --short HEAD`
	fi
	if test -n "`git diff-index -m --name-only HEAD`" ; then
		GITHEAD=${GITHEAD}-dirty
	fi
	if test -n "${GITHEAD}" ; then
		GITHEAD="-${GITHEAD}"
	fi
fi
AC_MSG_RESULT([$GITHEAD])
AC_SUBST([GITHEAD])
dnl }}}
dnl }}}

dnl {{{ toolchain checks
AC_PROG_CC
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR([mpdcron requires a C compiler that supports ISO C99!])
fi
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_SED
dnl }}}

dnl {{{ Make pkg-config work
PKG_PROG_PKG_CONFIG([0.9.0])
dnl }}}

dnl {{{ Check for libraries
GLIB_REQUIRED=2.18
LIBDAEMON_REQUIRED=0.13
LIBMPDCLIENT_REQUIRED=2.1

PKG_CHECK_MODULES([glib], [glib-2.0 >= $GLIB_REQUIRED],,
				  [AC_MSG_ERROR([mpdcron requires glib-$GLIB_REQUIRED or newer])])
PKG_CHECK_MODULES([libdaemon], [libdaemon >= $LIBDAEMON_REQUIRED],,
				  [AC_MSG_ERROR([mpdcron requires libdaemon-$LIBDAEMON_REQUIRED or newer])])
PKG_CHECK_MODULES([libmpdclient], [libmpdclient >= $LIBMPDCLIENT_REQUIRED],,
				  [AC_MSG_ERROR([mpdcron requires libmpdclient-$LIBMPDCLIENT_REQUIRED or newer])])
dnl }}}

dnl {{{ --enable-module
AC_MSG_CHECKING([whether module support is wanted])
AC_ARG_ENABLE([modules],
			  [AS_HELP_STRING([--enable-modules],
							  [enable support for modules (via GModule)])],
			  WANT_MODULE="$enableval",
			  WANT_MODULE="no")
AC_MSG_RESULT([$WANT_MODULE])
AM_CONDITIONAL(HAVE_MODULE, test x"$WANT_MODULE" = x"yes")
if test x"$WANT_MODULE" = x"yes"; then
	PKG_CHECK_MODULES([gmodule], [gmodule-2.0 >= $GLIB_REQUIRED],,
					  [AC_MSG_ERROR([mpdcron requires gmodule-$GLIB_REQUIRED or newer for module support])])
	AC_DEFINE([HAVE_MODULE], 1, [Define for module support])
dnl Standard module scrobbler requires curl.
	PKG_CHECK_MODULES([libcurl], [libcurl], found_curl=yes, found_curl=no)
else
	found_curl=no
fi
AM_CONDITIONAL([HAVE_CURL], test x"$found_curl" = x"yes")
dnl }}}

dnl {{{ Extra CFLAGS
MPDCRON_CFLAGS=
WANTED_CFLAGS="-Wall -W -Wextra -Wvla -Wformat=2 -Wformat-security -Wformat-nonliteral -Winit-self -Wfloat-equal -Wno-deprecated-declarations -Wmissing-declarations -Wmissing-noreturn -Wmissing-prototypes -Wredundant-decls  -Wshadow -Wpointer-arith -Wstrict-prototypes -Wcast-qual -Wwrite-strings -pedantic"
for flag in $WANTED_CFLAGS ; do
	AX_CHECK_COMPILER_FLAGS([$flag], [MPDCRON_CFLAGS="$MPDCRON_CFLAGS $flag"],)
done
AC_SUBST([MPDCRON_CFLAGS])
dnl }}}

dnl {{{ Output
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(
		  Makefile
		  src/Makefile
		  src/modules/Makefile
		  src/modules/scrobbler/Makefile
		  data/Makefile
)
dnl }}}
