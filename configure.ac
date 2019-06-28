AC_PREREQ([2.69])

AC_INIT([dwire-gdb], m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [https://github.com/rafaelmartins/dwire-gdb], [dwire-gdb],
        [https://dwire-gdb.rgm.io/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.13 foreign dist-bzip2 dist-xz dist-zip subdir-objects
                  serial-tests -Wall -Wno-extra-portability -Werror])
AC_CONFIG_HEADERS([config.h])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], [
  AC_MSG_ERROR([no C99 compiler found, dwire-gdb requires a C99 compiler.])
])

AC_CONFIG_FILES([
  Makefile
])
AC_OUTPUT

AS_ECHO("
        ====== ${PACKAGE_STRING} ======

        prefix:              ${prefix}
        exec_prefix:         ${exec_prefix}
        bindir:              ${bindir}

        compiler:            ${CC}
        cxxflags:            ${CFLAGS}
        ldflags:             ${LDFLAGS}
")
