dnl Process this file with autoconf to produce a configure script

AC_INIT([eom], [0.6.2])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([shave])

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

EOM_MODULES='clutter-1.0 glib-2.0 gio-2.0'
EOM_SAMPLE_DIR="`pwd`/images"
PICTABLE_MODULES="$EOM_MODULES clutter-box2d-0.10 clutter-gst-0.10"

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug], [enable debug]),
              [enable_debug=$enableval],
              [enable_debug=no])

AC_ARG_ENABLE(gesture,
  AC_HELP_STRING([--disable-gesture],
                 [disable gesture support]),,
  enable_gesture=yes)

AC_ARG_ENABLE(twofinger,
  AC_HELP_STRING([--enable-twofinger],
                 [enable two finger gesture support]),
  enable_twofinger=$enableval,
  enable_twofinger=no)

AC_ARG_ENABLE(no-motion,
  AC_HELP_STRING([--enable-no-motion],
                 [no motion event]),no_motion=yes,)

AC_ARG_ENABLE(gamma,
    AC_HELP_STRING([--disable-gamma],
                   [disable gamma support]),,
    enable_gamma=yes)

if test x$enable_gesture = xyes; then
    AC_DEFINE(HAVE_GESTURE,1,[Build with gesture support])
    EOM_MODULES="$EOM_MODULES clutter-gesture"
    PICTABLE_MODULES="$PICTABLE_MODULES clutter-gesture"
fi

if test x$enable_twofinger = xyes; then
    AC_DEFINE(HAVE_TWO_FINGER_SLIDE,1,[Build with 2 finger slide support])
fi

if test x$enable_gamma = xyes; then
    AC_DEFINE(HAVE_GAMMA,1,[Build with gamma support])
fi

if test x$no_motion = xyes; then
    AC_DEFINE(HAVE_DISABLE_MOTION,1,[Build without motion support])
fi

if test x$enable_debug = xyes; then
  AC_DEFINE(HAVE_DEBUG,1,[Build with debug support])
  DEFAULT_COMPILE_FLAGS="-g -O0 -Wall"
else
  DEFAULT_COMPILE_FLAGS=""
fi

AC_DEFINE_UNQUOTED(EOM_SAMPLE_DIR, "$EOM_SAMPLE_DIR", [sample image dir])

PKG_CHECK_MODULES(DEPS, $EOM_MODULES)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

PKG_CHECK_MODULES(PTDEP, $PICTABLE_MODULES)
AC_SUBST(PTDEP_CFLAGS)
AC_SUBST(PTDEP_LIBS)

AC_SUBST(DEFAULT_COMPILE_FLAGS)

SHAVE_INIT([shave], [enable])

AC_CONFIG_FILES([
    shave/shave
    shave/shave-libtool
    Makefile
    src/Makefile
    tidy/Makefile
    physics/Makefile
])

dnl End the configure script.
AC_OUTPUT
