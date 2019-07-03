ACLOCAL_AMFLAGS = -I m4

AM_DISTCHECK_CONFIGURE_FLAGS = \
	CFLAGS="-Wall -g -O0" \
	--enable-tests \
	--disable-silent-rules \
	--disable-valgrind \
	$(NULL)

EXTRA_DIST = \
	build-aux/git-version-gen \
	build-aux/valgrind.sh \
	$(top_srcdir)/.version \
	autogen.sh \
	LICENSE \
	README.md \
	$(NULL)

BUILT_SOURCES = \
	$(top_srcdir)/.version \
	$(NULL)

bin_PROGRAMS = \
	dwire-gdb \
	$(NULL)

check_PROGRAMS = \
	$(NULL)

noinst_HEADERS = \
	src/debug.h \
	src/debugwire.h \
	src/error.h \
	src/gdbserver.h \
	src/serial.h \
	src/utils.h \
	$(NULL)

noinst_LTLIBRARIES = \
	libdwire_gdb.la \
	$(NULL)

dwire_gdb_SOURCES = \
	src/main.c \
	$(NULL)

dwire_gdb_LDADD = \
	libdwire_gdb.la \
	$(NULL)

libdwire_gdb_la_SOURCES = \
	src/debug.c \
	src/debugwire.c \
	src/error.c \
	src/gdbserver.c \
	src/serial.c \
	src/utils.c \
	$(NULL)


if USE_CMOCKA

check_PROGRAMS += \
	tests/check_utils \
	$(NULL)

tests_check_utils_SOURCES = \
	tests/check_utils.c \
	$(NULL)

tests_check_utils_CFLAGS = \
	$(CMOCKA_CFLAGS) \
	$(NULL)

tests_check_utils_LDFLAGS = \
	-no-install \
	$(NULL)

tests_check_utils_LDADD = \
	$(CMOCKA_LIBS) \
	libdwire_gdb.la \
	$(NULL)

if USE_LD_WRAP

check_PROGRAMS += \
	tests/check_error \
	tests/check_serial \
	$(NULL)

tests_check_error_SOURCES = \
	tests/check_error.c \
	$(NULL)

tests_check_error_CFLAGS = \
	$(CMOCKA_CFLAGS) \
	$(NULL)

tests_check_error_LDFLAGS = \
	-no-install \
	-Wl,--wrap=strerror \
	$(NULL)

tests_check_error_LDADD = \
	$(CMOCKA_LIBS) \
	libdwire_gdb.la \
	$(NULL)

tests_check_serial_SOURCES = \
	tests/check_serial.c \
	$(NULL)

tests_check_serial_CFLAGS = \
	$(CMOCKA_CFLAGS) \
	$(NULL)

tests_check_serial_LDFLAGS = \
	-no-install \
	-Wl,--wrap=open \
	-Wl,--wrap=close \
	-Wl,--wrap=ioctl \
	-Wl,--wrap=usleep \
	-Wl,--wrap=read \
	$(NULL)

tests_check_serial_LDADD = \
	$(CMOCKA_LIBS) \
	libdwire_gdb.la \
	$(NULL)

endif
endif


TESTS = \
	$(check_PROGRAMS) \
	$(NULL)


$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version


if USE_VALGRIND
valgrind: all
	$(MAKE) check TESTS_ENVIRONMENT=" \
		VALGRIND=$(VALGRIND) \
		/bin/bash -e $(top_srcdir)/build-aux/valgrind.sh"
endif

.PHONY: valgrind
