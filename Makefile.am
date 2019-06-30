ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh \
	$(top_srcdir)/.version \
	$(NULL)

BUILT_SOURCES = \
	$(top_srcdir)/.version \
	$(NULL)

bin_PROGRAMS = \
	dwire-gdb \
	$(NULL)

noinst_HEADERS = \
	src/debug.h \
	src/debugwire.h \
	src/error.h \
	src/gdbserver.h \
	src/serial.h \
	src/utils.h \
	$(NULL)

dwire_gdb_SOURCES = \
	src/main.c \
	src/debug.c \
	src/debugwire.c \
	src/error.c \
	src/gdbserver.c \
	src/serial.c \
	src/utils.c \
	$(NULL)

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
