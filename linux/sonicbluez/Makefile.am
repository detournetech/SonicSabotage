AM_MAKEFLAGS = --no-print-directory

lib_LTLIBRARIES =

noinst_LIBRARIES =

noinst_LTLIBRARIES =

bin_PROGRAMS =

noinst_PROGRAMS =

dist_man_MANS =

dist_noinst_MANS =

CLEANFILES =

EXTRA_DIST =

libexec_PROGRAMS =

include_HEADERS =

AM_CFLAGS = $(WARNING_CFLAGS) $(MISC_CFLAGS)
AM_LDFLAGS = $(MISC_LDFLAGS)

noinst_LTLIBRARIES += gdbus/libgdbus-internal.la

gdbus_libgdbus_internal_la_SOURCES = gdbus/gdbus.h \
				gdbus/mainloop.c gdbus/watch.c \
				gdbus/object.c gdbus/client.c gdbus/polkit.c

AM_CFLAGS += @DBUS_CFLAGS@ @GLIB_CFLAGS@

bin_PROGRAMS += sonic

sonic_SOURCES = client/main.c \
                    client/ble_api.h client/ble_api.c \
                    client/app_api.h client/app_api.c \
                    client/display.h client/display.c \
                    client/agent.h client/agent.c \
                    client/gatt.h client/gatt.c \
                    client/uuid.h client/uuid.c \
					client/util.h client/util.c \
					client/wifi.h client/wifi.c

sonic_LDADD = gdbus/libgdbus-internal.la @GLIB_LIBS@ @DBUS_LIBS@ \
                -lreadline

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh mkinstalldirs test-driver


