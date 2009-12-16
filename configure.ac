dnl vim: set sw=4 sts=4 ts=4 noet ft=config foldmethod=marker foldmarker={{{,}}} :

dnl {{{ program, version
AC_PREREQ(2.59)
AC_INIT([src/main.c])

VERSION_MAJOR=0
VERSION_MINOR=2
VERSION_FULL="$VERSION_MAJOR.$VERSION_MINOR"
VERSION="$VERSION_FULL"

AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_FULL])

AM_INIT_AUTOMAKE(mpdhooker, [$VERSION_FULL])
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
fi
AC_MSG_RESULT([$GITHEAD])
AC_SUBST([GITHEAD])
dnl }}}
dnl }}}

dnl {{{ toolchain checks
AC_PROG_CC
AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
	AC_MSG_ERROR([mpdhooker requires a C compiler that supports ISO C99!])
fi
AC_PROG_INSTALL
AC_PROG_MAKE_SET
dnl }}}

dnl {{{ Make pkg-config work
PKG_PROG_PKG_CONFIG([0.9.0])
dnl }}}

dnl {{{ Check for libraries
PKG_CHECK_MODULES([libdaemon], [libdaemon >= 0.13],,
				  [AC_MSG_ERROR([mpdhooker requires libdaemon-0.13 or newer])])
PKG_CHECK_MODULES([libmpdclient], [libmpdclient >= 2.1],,
				  [AC_MSG_ERROR([mpdhooker requires libmpdclient-2.1 or newer])])
PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.18],,
				  [AC_MSG_ERROR([mpdhooker requires glib-2.18 or newer])])
dnl }}}

dnl {{{ Extra CFLAGS
MPDHOOKER_CFLAGS=
WANTED_CFLAGS="-Wall -W -Wextra -Wvla -Wformat=2 -Wformat-security -Wformat-nonliteral -Wlogical-op -Winit-self -Wpointer-arith -Wfloat-equal -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wshadow -Wcast-align -Winline -pedantic"
for flag in $WANTED_CFLAGS ; do
	AX_CHECK_COMPILER_FLAGS([$flag], [MPDHOOKER_CFLAGS="$MPDHOOKER_CFLAGS $flag"],)
done
AC_SUBST([MPDHOOKER_CFLAGS])
dnl }}}

dnl {{{ Output
AM_CONFIG_HEADER(config.h)
AC_OUTPUT(
		  Makefile
		  src/Makefile
)
dnl }}}
