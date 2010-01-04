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
GIO_REQUIRED=2.22
LIBDAEMON_REQUIRED=0.13
LIBMPDCLIENT_REQUIRED=2.1

PKG_CHECK_MODULES([glib], [glib-2.0 >= $GLIB_REQUIRED],,
				  [AC_MSG_ERROR([mpdcron requires glib-$GLIB_REQUIRED or newer])])
PKG_CHECK_MODULES([libdaemon], [libdaemon >= $LIBDAEMON_REQUIRED],,
				  [AC_MSG_ERROR([mpdcron requires libdaemon-$LIBDAEMON_REQUIRED or newer])])
PKG_CHECK_MODULES([libmpdclient], [libmpdclient >= $LIBMPDCLIENT_REQUIRED],,
				  [AC_MSG_ERROR([mpdcron requires libmpdclient-$LIBMPDCLIENT_REQUIRED or newer])])
dnl }}}

dnl {{{ --enable-gmodule
AC_MSG_CHECKING([whether module support is wanted])
AC_ARG_ENABLE([gmodule],
			  [AS_HELP_STRING([--enable-gmodule],
							  [enable support for C modules (via GModule)])],
			  WANT_GMODULE="$enableval",
			  WANT_GMODULE="no")
AC_MSG_RESULT([$WANT_GMODULE])
AM_CONDITIONAL(HAVE_GMODULE, test x"$WANT_GMODULE" = x"yes")
if test x"$WANT_GMODULE" = x"yes"; then
	PKG_CHECK_MODULES([gmodule], [gmodule-2.0 >= $GLIB_REQUIRED],,
					  [AC_MSG_ERROR([mpdcron requires gmodule-$GLIB_REQUIRED or newer for module support])])
	AC_DEFINE([HAVE_GMODULE], 1, [Define for gmodule support])
fi
dnl }}}

dnl {{{ standard modules
ALL_STANDARD_MODULES="notification scrobbler stats"
AC_MSG_CHECKING([which standard modules are wanted])
AC_ARG_WITH([standard-modules],
			  [AS_HELP_STRING([--with-standard-modules=foo,bar,...
			   Build the specified standard modules:
			   all              Build all available standard modules
			   notification     Build the notification module
			   scrobbler        Build the scrobbler module
			   stats            Build the stats module])],
			   [standard_modules="`echo $with_standard_modules | tr ',' ' '`"],
			   [standard_modules=""])
standard_modules=`echo $standard_modules | sed -e "s,^all\$,$ALL_STANDARD_MODULES,"`
AC_MSG_RESULT([$standard_modules])
STANDARD_MODULES="$standard_modules"
AC_SUBST([STANDARD_MODULES])
WANT_NOTIFICATION=no
WANT_SCROBBLER=no
WANT_STATS=no
for m in $STANDARD_MODULES ; do
	if test x"$m" = x"notification" ; then
		if test x"$WANT_GMODULE" != x"yes"; then
			AC_MSG_WARN([GModule support is disabled, notification module won't be built])
		else
			# Notification requires notify-send
			AC_CHECK_PROGS(NOTIFY_SEND, notify-send, )
			if test x"$NOTIFY_SEND" = x; then
				AC_MSG_ERROR([notification standard module requires notify-send])
			else
				WANT_NOTIFICATION=yes
			fi
		fi
	fi
	if test x"$m" = x"scrobbler" ; then
		if test x"$WANT_GMODULE" != x"yes"; then
			AC_MSG_WARN([GModule support is disabled, scrobbler module won't be built])
		else
			# Scrobbler requires curl.
			PKG_CHECK_MODULES([libcurl], [libcurl], [WANT_SCROBBLER=yes],
							  AC_MSG_ERROR([scrobbler standard module requires curl]))
		fi
	fi
	if test x"$m" = x"stats" ; then
		if test x"$WANT_GMODULE" != x"yes"; then
			AC_MSG_WARN([GModule support is disabled, stats module won't be built])
		else
			# Stats module requires gio and sqlite3
			PKG_CHECK_MODULES([sqlite], [sqlite3], [WANT_STATS=yes],
							  AC_MSG_ERROR([stats standard module requires sqlite]))
			PKG_CHECK_MODULES([gio_unix], [gio-unix-2.0 >= $GIO_REQUIRED],
							  [HAVE_GIO_UNIX=yes],
							  [HAVE_GIO_UNIX=no])
			if test x"$HAVE_GIO_UNIX" = x"no" ; then
				PKG_CHECK_MODULES([gio], [gio-2.0 >= $GIO_REQUIRED], [WANT_STATS=yes],
								  [AC_MSG_ERROR([stats standard module requires gio-$GIO_REQUIRED or newer])])
				AC_DEFINE(HAVE_GIO_UNIX, 0, "Define for gio-unix")
			else
				AC_DEFINE(HAVE_GIO_UNIX, 1, "Define for gio-unix")
			fi
		fi
	fi
done
AM_CONDITIONAL([WANT_NOTIFICATION], test x"$WANT_NOTIFICATION" = x"yes")
AM_CONDITIONAL([WANT_SCROBBLER], test x"$WANT_SCROBBLER" = x"yes")
AM_CONDITIONAL([WANT_STATS], test x"$WANT_STATS" = x"yes")
AM_CONDITIONAL([HAVE_GIO_UNIX], test x"$HAVE_GIO_UNIX" = x"yes")
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
		  src/gmodule/Makefile
		  src/gmodule/notification/Makefile
		  src/gmodule/scrobbler/Makefile
		  src/gmodule/stats/Makefile
		  src/gmodule/stats/homescrape
		  data/Makefile
		  zsh-completion/Makefile
)
dnl }}}
