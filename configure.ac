############
# niftyconf configure.ac
# v0.5 - (c) Daniel Hiepler (2010-2014)


# --------------------------------
#    initialization
# --------------------------------

# package info
AC_INIT([niftyconf], 
		m4_esyscmd([./version --short]),
        [http://github.com/niftylight/niftyconf/issues],
        [niftyconf],
        [http://wiki.niftylight.de/AC_PACKAGE_NAME])
AC_COPYRIGHT([Copyright (C) 2002-2014 Daniel Hiepler])        

# for cross compiling
AC_CANONICAL_SYSTEM

# autotools initialization
AC_LANG(C)
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 color-tests dist-bzip2 tar-ustar parallel-tests silent-rules subdir-objects -Wno-portability -Wall])




# --------------------------------
#    further info on the package
# --------------------------------
# infos
PACKAGE_DESCRIPTION="niftyled setup configuration GUI"
PACKAGE_DESCRIPTION_LONG="GUI to create a niftyled setup file"

AC_SUBST([PACKAGE_DESCRIPTION], [${PACKAGE_DESCRIPTION}], [package description])
AC_SUBST([PACKAGE_DESCRIPTION_LONG], [${PACKAGE_DESCRIPTION_LONG}], [long package description])
AC_DEFINE_UNQUOTED(PACKAGE_DESCRIPTION, "${PACKAGE_DESCRIPTION}", [package description])
AC_DEFINE_UNQUOTED(PACKAGE_DESCRIPTION_LONG, "${PACKAGE_DESCRIPTION_LONG}", [long package description])

# versioning (s. version script)
AC_SUBST([PACKAGE_MAJOR_VERSION], m4_esyscmd([./version --major]), [major version])
AC_SUBST([PACKAGE_MINOR_VERSION], m4_esyscmd([./version --minor]), [minor version])
AC_SUBST([PACKAGE_MICRO_VERSION], m4_esyscmd([./version --micro]), [micro version])
AC_SUBST([PACKAGE_GIT_VERSION], m4_esyscmd([./version --git]), [git version])
AC_SUBST([PACKAGE_PREFS_VERSION], m4_esyscmd([./version --prefs]), [prefs version])
AC_SUBST([DEBIAN_REVISION], m4_esyscmd([./version --debian]), [debian revision])

         
# --------------------------------
#    checks for programs
# --------------------------------
AM_PATH_GLIB_2_0

dnl change this to 0.28 as soon as it's widely used. Then also delete m4/pkg.m4
PKG_PROG_PKG_CONFIG([0.25])
if test -z "${PKG_CONFIG}"; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/ or check your distribution.])
fi

          

# --------------------------------
#    checks for libraries
# --------------------------------
# minimum versions
NIFTYLED_REQUIRED=0.1.0
GTK_REQUIRED=2.2


# ---------------------
#  check for packages
# ---------------------
PKG_CHECK_MODULES(NIFTYCONF, 
[
        cairo
        gtk+-2.0 >= $GTK_REQUIRED
        gmodule-2.0 
        niftyled >= $NIFTYLED_REQUIRED
])
AC_SUBST(NIFTYCONF_CFLAGS)
AC_SUBST(NIFTYCONF_LIBS)


# --------------------------------
#    checks for header files
# --------------------------------
AC_HEADER_STDC


# --------------------------------
#    checks for types
# --------------------------------


# --------------------------------
#    checks for structures
# --------------------------------


# --------------------------------
#    checks for compiler characteristics
# --------------------------------


# --------------------------------
#    checks for library functions
# --------------------------------


# --------------------------------
#    checks for system services
# --------------------------------


# --------------------------------
#   compile process
# --------------------------------
AC_PROG_CC_C99
AM_PROG_CC_C_O
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


# --------------------------------
# Arguments for configure-script
# --------------------------------
AC_ARG_ENABLE(
        debug,
		AS_HELP_STRING([--enable-debug], [enable debugging, default: no]),
		[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
		esac],
		[debug=false])
AM_CONDITIONAL(ENABLE_DEBUG, test x$debug = xtrue)

AC_ARG_ENABLE(
        gprof,
		AS_HELP_STRING([--enable-gprof], [enable profiling, default: no]),
		[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-gprof]) ;;
		esac],
		[debug=false])
AM_CONDITIONAL(ENABLE_GPROF, test x$gprof = xtrue)



# --------------------------------
# Output
# --------------------------------
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
   Makefile
   data/niftyconf.desktop
   data/Makefile
   data/icons/Makefile
   src/Makefile
   src/version.c
   src/version.h
   dist/debian/changelog
   dist/debian/control.tmpl
   dist/debian/rules.tmpl
   dist/debian/watch
])
AC_OUTPUT



echo -e "
Configure summary:
\tPackage.....................:  ${PACKAGE_STRING}
\tDescription.................:  ${PACKAGE_DESCRIPTION}
\tRevision....................:  ${PACKAGE_GIT_VERSION}
\tPreferences version.........:  ${PACKAGE_PREFS_VERSION}
\tURL.........................:  ${PACKAGE_URL}
\tBugreports..................:  ${PACKAGE_BUGREPORT}
\tCapture mechanisms..........:  ${CAPTURE}

\tInstall prefix..............:  ${prefix}
\tC compiler..................:  ${CC}
\tSystem CFLAGS...............:  ${CFLAGS}
\tSystem CXXFLAGS.............:  ${CXXFLAGS}
\tSystem LDFLAGS..............:  ${LDFLAGS}
"
