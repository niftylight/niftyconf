# niftyconf configure.ac
# v0.1 - Daniel Hiepler (2010-2012)



# versioning
m4_define([niftyconf_major_version], [0])
m4_define([niftyconf_minor_version], [0])
m4_define([niftyconf_micro_version], [1])


m4_define(niftyconf_datestamp,
          m4_esyscmd(date +%Y%m%d | tr -d '\n\r'))
          
m4_define([niftyconf_version],
          [niftyconf_major_version.niftyconf_minor_version.niftyconf_micro_version])

m4_define(niftyconf_full_version,
          niftyconf_version[-]niftyconf_datestamp)

          
          

# ---------------
#  autoconf init
# ---------------

AC_INIT([Niftyconf],
        [niftyconf_version],
        [http://github.com/niftylight/niftyconf/issues],
        [niftyconf],
        [http://wiki.niftylight.de/Niftyconf])
        
AC_PREREQ([2.64])
AC_COPYRIGHT([
  Copyright (C) 2002-2012 Daniel Hiepler
])


AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE([1.11 no-define -Wno-portability tar-ustar no-dist-gzip dist-xz])
 
# Support silent build rules. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# --------------
#  dependencies
# --------------

# Check for programs
AC_PROG_CC
AC_HEADER_STDC
AM_PATH_GLIB_2_0

# minimum versions
NIFTYLED_REQUIRED=0.1.0


# ---------------------
#  check for packages
# ---------------------
PKG_CHECK_MODULES(NIFTYCONF, 
[
        gtk+-2.0 
        gmodule-2.0 
        gconf-2.0 
        niftyled >= $NIFTYLED_REQUIRED
])
AC_SUBST(NIFTYCONF_CFLAGS)
AC_SUBST(NIFTYCONF_LIBS)




# -----------------------
#  ./configure arguments
# -----------------------

# enable debug
AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--disable-debug],
                 [compile without debug code]),
    enable_debug=$enableval, enable_debug=yes )

if test x$enable_debug = xyes; then
  AC_DEFINE(ENABLE_DEBUG, [], [debugging code enabled])
  CFLAGS="-g -O0"
fi

# enable gprof
AC_ARG_ENABLE(gprof,
            AS_HELP_STRING([--enable-gprof=@<:@no/yes@:>@],
            [build with support for gprof]),,
            [enable_gprof=no])
if test "x$enable_gprof" != "xno" ; then
    CFLAGS="$CFLAGS -pg"
    LDFLAGS="$LDFLAGS -pg"
fi



# ---------------
#  compile flags
# ---------------
WARNING_CFLAGS="-Wall -Wreturn-type -Wswitch -Wuninitialized -Wparentheses -Wpointer-arith -Winline"
CFLAGS="-std=c99 $CFLAGS"
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)



AC_CONFIG_FILES([
   Makefile
   data/niftyconf.desktop.in
   data/Makefile
   data/icons/Makefile
   src/Makefile
])
AC_OUTPUT



echo "
Configure summary:

        Version.....................:  niftyconf_version
        Build.......................:  niftyconf_datestamp
	Compiler....................:  ${CC}
	Compiler Flags..............:  ${CFLAGS} ${WARNING_CFLAGS}
	Linker Flags................:  ${LDFLAGS}
	Prefix......................:  ${prefix}
"
