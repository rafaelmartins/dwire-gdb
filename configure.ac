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

LT_INIT

AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], [
  AC_MSG_ERROR([no C99 compiler found, dwire-gdb requires a C99 compiler.])
])

PKG_PROG_PKG_CONFIG

AC_ARG_ENABLE([valgrind], AS_HELP_STRING([--disable-valgrind],
              [ignore presence of valgrind]))
AS_IF([test "x$enable_valgrind" != "xno"], [
  AC_PATH_PROG([valgrind], [valgrind])
  AS_IF([test "x$ac_cv_path_valgrind" = "x"], [
    have_valgrind=no
  ], [
    have_valgrind=yes
  ])
])
AS_IF([test "x$have_valgrind" = "xyes"], , [
  AS_IF([test "x$enable_valgrind" = "xyes"], [
    AC_MSG_ERROR([valgrind requested but not found])
  ])
])
AM_CONDITIONAL([USE_VALGRIND], [test "x$have_valgrind" = "xyes"])
VALGRIND="$ac_cv_path_valgrind"
AC_SUBST(VALGRIND)

TESTS="disabled"
AC_ARG_ENABLE([tests], AS_HELP_STRING([--disable-tests],
              [disable unit tests, ignoring presence of cmocka]))
AS_IF([test "x$enable_tests" != "xno"], [
  PKG_CHECK_MODULES([CMOCKA], [cmocka], [
    have_cmocka=yes
    AC_MSG_CHECKING([whether the linker supports -wrap])
    save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -Wl,-wrap,exit"
    AC_RUN_IFELSE([
      AC_LANG_PROGRAM([
        [void __wrap_exit(int s){__real_exit(0);}]
      ], [
        [exit(1);]
      ])
    ], [
      have_ld_wrap=yes
    ], [
      have_ld_wrap=no
    ], [
      have_ld_wrap=no
    ])
    AC_MSG_RESULT([$have_ld_wrap])
    AS_IF([test "x$have_ld_wrap" = "xyes"],, [
      AC_MSG_WARN([linker does not supports -wrap. tests requiring it will be disabled])
    ])
    LDFLAGS="$save_LDFLAGS"
  ], [
    have_cmocka=no
  ])
  AS_IF([test "x$have_cmocka" = "xyes"], [
    TESTS="enabled"
    AS_IF([test "x$have_ld_wrap" = "xyes"], [
      TESTS="$TESTS (with -wrap)"
    ])
  ], [
    TESTS="disabled"
    AS_IF([test "x$enable_tests" = "xyes"], [
      AC_MSG_ERROR([tests requested but cmocka was not found])
    ], [
      AC_MSG_WARN([cmocka was not found, disabling tests])
    ])
  ])
])
AM_CONDITIONAL([USE_CMOCKA], [test "x$have_cmocka" = "xyes"])
AM_CONDITIONAL([USE_LD_WRAP], [test "x$have_ld_wrap" = "xyes"])

AC_CONFIG_FILES([
  Makefile
])
AC_OUTPUT

AS_ECHO("
        ====== ${PACKAGE_STRING} ======

        prefix:       ${prefix}
        exec_prefix:  ${exec_prefix}
        bindir:       ${bindir}

        compiler:     ${CC}
        cxxflags:     ${CFLAGS}
        ldflags:      ${LDFLAGS}

        tests:        ${TESTS}

        valgrind:     ${VALGRIND}
")
