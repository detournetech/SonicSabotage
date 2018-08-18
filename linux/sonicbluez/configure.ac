AC_PREREQ(2.60)
AC_INIT(sonic, 0.1)

AM_INIT_AUTOMAKE([foreign subdir-objects color-tests silent-rules
					tar-pax no-dist-gzip dist-xz])
AC_CONFIG_HEADERS(config.h)
AC_USE_SYSTEM_EXTENSIONS

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)

PKG_PROG_PKG_CONFIG

COMPILER_FLAGS

AC_LANG_C

AC_C_RESTRICT

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_PIE
AC_PROG_INSTALL
AC_PROG_MKDIR_P

m4_define([_LT_AC_TAGCONFIG], [])
m4_ifdef([AC_LIBTOOL_TAGS], [AC_LIBTOOL_TAGS([])])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

if (test "$USE_MAINTAINER_MODE" = "yes"); then
	AC_CHECK_PROG(enable_coverage, [lcov], [yes], [no])
	AC_CHECK_PROG(enable_dbus_run_session, [dbus-run-session], [yes])
	AC_CHECK_PROG(enable_valgrind, [valgrind], [yes])
	AC_CHECK_HEADERS(valgrind/memcheck.h)
fi
AM_CONDITIONAL(COVERAGE, test "${enable_coverage}" = "yes")
AM_CONDITIONAL(DBUS_RUN_SESSION, test "${enable_dbus_run_session}" = "yes")
AM_CONDITIONAL(VALGRIND, test "${enable_valgrind}" = "yes")

MISC_FLAGS

AC_ARG_ENABLE(threads, AC_HELP_STRING([--enable-threads],
		[enable threading support]), [enable_threads=${enableval}])

AC_CHECK_FUNC(signalfd, dummy=yes,
			AC_MSG_ERROR(signalfd support is required))

AC_CHECK_LIB(rt, clock_gettime, dummy=yes,
			AC_MSG_ERROR(realtime clock support is required))

AC_CHECK_LIB(pthread, pthread_create, dummy=yes,
			AC_MSG_ERROR(posix thread support is required))

AC_CHECK_LIB(dl, dlopen, dummy=yes,
			AC_MSG_ERROR(dynamic linking loader is required))

AC_CHECK_HEADERS(linux/types.h linux/if_alg.h)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.28, dummy=yes,
				AC_MSG_ERROR(GLib >= 2.28 is required))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

if (test "${enable_threads}" = "yes"); then
	AC_DEFINE(NEED_THREADS, 1, [Define if threading support is required])
	PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16, dummy=yes,
				AC_MSG_ERROR(GThread >= 2.16 is required))
	GLIB_CFLAGS="$GLIB_CFLAGS $GTHREAD_CFLAGS"
	GLIB_LIBS="$GLIB_LIBS $GTHREAD_LIBS"
fi

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.6, dummy=yes,
				AC_MSG_ERROR(D-Bus >= 1.6 is required))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

AC_ARG_WITH([dbusconfdir], AC_HELP_STRING([--with-dbusconfdir=DIR],
				[path to D-Bus configuration directory]),
					[path_dbusconfdir=${withval}])
if (test -z "${path_dbusconfdir}"); then
	AC_MSG_CHECKING([D-Bus configuration directory])
	path_dbusconfdir="`$PKG_CONFIG --variable=sysconfdir dbus-1`"
	if (test -z "${path_dbusconfdir}"); then
		AC_MSG_ERROR([D-Bus configuration directory is required])
	fi
	AC_MSG_RESULT([${path_dbusconfdir}])
fi
AC_SUBST(DBUS_CONFDIR, [${path_dbusconfdir}])

AC_ARG_WITH([dbussystembusdir], AC_HELP_STRING([--with-dbussystembusdir=DIR],
				[path to D-Bus system bus services directory]),
					[path_dbussystembusdir=${withval}])
if (test -z "${path_dbussystembusdir}"); then
	AC_MSG_CHECKING([D-Bus system bus services dir])
	path_dbussystembusdir="`$PKG_CONFIG --variable=system_bus_services_dir dbus-1`"
	if (test -z "${path_dbussystembusdir}"); then
		AC_MSG_ERROR([D-Bus system bus services directory is required])
	fi
	AC_MSG_RESULT([${path_dbussystembusdir}])
fi
AC_SUBST(DBUS_SYSTEMBUSDIR, [${path_dbussystembusdir}])

AC_ARG_WITH([dbussessionbusdir], AC_HELP_STRING([--with-dbussessionbusdir=DIR],
				[path to D-Bus session bus services directory]),
					[path_dbussessionbusdir=${withval}])
if (test -z "${path_dbussessionbusdir}"); then
	AC_MSG_CHECKING([D-Bus session bus services dir])
	path_dbussessionbusdir="`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`"
	if (test -z "${path_dbussessionbusdir}"); then
		AC_MSG_ERROR([D-Bus session bus services directory is required])
	fi
	AC_MSG_RESULT([${path_dbussessionbusdir}])
fi
AC_SUBST(DBUS_SESSIONBUSDIR, [${path_dbussessionbusdir}])

AC_ARG_ENABLE(backtrace, AC_HELP_STRING([--enable-backtrace],
		[compile backtrace support]), [enable_backtrace=${enableval}])

if (test "${enable_backtrace}" = "yes"); then
	AC_CHECK_HEADER(elfutils/libdwfl.h, dummy=yes,
			AC_MSG_ERROR(elfutils support is required))
	AC_DEFINE(HAVE_BACKTRACE_SUPPORT, 1,
			[Define to 1 if you have the backtrace support.])
	BACKTRACE_CFLAGS=""
	BACKTRACE_LIBS="-ldw"
	AC_SUBST(BACKTRACE_CFLAGS)
	AC_SUBST(BACKTRACE_LIBS)
fi

AC_ARG_ENABLE(client, AC_HELP_STRING([--disable-client],
		[disable command line client]), [enable_client=${enableval}])
AM_CONDITIONAL(CLIENT, test "${enable_client}" != "no")

if (test "${enable_client}" != "no"); then
        AC_CHECK_HEADERS(readline/readline.h, enable_readline=yes,
                AC_MSG_ERROR(readline header files are required))
fi
AM_CONDITIONAL(READLINE, test "${enable_readline}" = "yes")
AC_OUTPUT([Makefile])
