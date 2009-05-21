dnl Process this file with autoconf to produce a configure script

AC_INIT([eom], [0.2])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([shave])

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

EOM_MODULES='clutter-0.9 glib-2.0 gio-2.0'
EOM_SAMPLE_DIR="`pwd`/images"

AC_ARG_ENABLE(gesture,
  AC_HELP_STRING([--disable-gesture],
                 [disable gesture support]),,
  enable_gesture=yes)

if test x$enable_gesture = xyes; then
    AC_DEFINE(HAVE_GESTURE,1,[Build with gesture support])
    EOM_MODULES="$EOM_MODULES clutter-gesture"
fi

AC_ARG_ENABLE(gamma,
    AC_HELP_STRING([--disable-gamma],
                   [disable gamma support]),,
    enable_gamma=yes)

if test x$enable_gamma = xyes; then
    AC_DEFINE(HAVE_GAMMA,1,[Build with gamma support])
fi

AC_DEFINE_UNQUOTED(EOM_SAMPLE_DIR, "$EOM_SAMPLE_DIR", [sample image dir])

PKG_CHECK_MODULES(DEPS, $EOM_MODULES)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

SHAVE_INIT([shave], [enable])

AC_CONFIG_FILES([
    shave/shave
    shave/shave-libtool
    Makefile
    src/Makefile
    tidy/Makefile
])

dnl End the configure script.
AC_OUTPUT
