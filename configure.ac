AC_PREREQ(2.69)
AC_INIT(aclocal.m4)
AC_CANONICAL_SYSTEM
AC_DEFINE([FR_INSTALL_PREFIX], [], [Description])

AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

AM_OPTIONS_WXCONFIG
AC_PROG_CXX

AC_ARG_ENABLE(debug,
              [  --enable-debug          Enable debugging information],
              USE_DEBUG="$enableval", USE_DEBUG="no" CXXFLAGS+=" -DNDEBUG")


CFLAGS+=" -std=c11"
CXXFLAGS+=" -std=c++14"

if test $USE_DEBUG = yes ; then 
  DEBUG=1
  FINAL=0
  dnl Bakefile doesn't touch {C,CPP,CXX,LD}FLAGS in autoconf format, we
  dnl have to do it ourselves. (Incorrectly) assuming GCC here:
  CFLAGS="$CFLAGS -g"
else
  DEBUG=0
  FINAL=1
fi

# Check for wxWidgets
AM_PATH_WXCONFIG([3.0.0], [], [
        AC_MSG_ERROR([
           wxWidgets must be installed on your system
           but wx-config script couldn't be found.
     
           Please check that wx-config is in path, the directory
           where wxWidgets libraries are installed (returned by
           'wx-config --libs' command) is in LD_LIBRARY_PATH or
           equivalent variable and wxWidgets version is 2.8.0 or above.
        ])], [aui,stc,std])

AC_LANG_PUSH([C++])

# Check for Boost headers and libraries
BOOST_REQUIRE
BOOST_BIND
BOOST_FUNCTION
BOOST_SMART_PTR
BOOST_THREADS
BOOST_CHRONO


# Link to Firebird client library
case "${host}" in
    *-*-cygwin* | *-*-mingw32* )
        dnl Windows builds dynamically link to the Firebird client library
    ;;

    *-*-darwin* )
        dnl Mac OS X uses the Firebird framework
        LIBS="$LIBS /Library/Frameworks/Firebird.Framework/Firebird"
    ;;

    * )
        AC_SEARCH_LIBS([isc_attach_database], [fbclient fbembed], [],
          [AC_MSG_ERROR([
           Firebird client library not found
           Install Firebird server development files.
           In Debian/Ubuntu 'sudo apt-get install firebird2.5-dev'
          ])])
  esac


CPPFLAGS="$CPPFLAGS $WX_CXXFLAGS $BOOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $WX_LDFLAGS $WX_LIBS $BOOST_THREAD_LDFLAGS $BOOST_THREAD_LIBS $BOOST_CHRONO_LDFLAGS $BOOST_CHRONO_LIBS"


AC_BAKEFILE([m4_include(autoconf_inc.m4)])

if test $GCC_PCH = 1 ; then
   CPPFLAGS="$CPPFLAGS -DWX_PRECOMP"
fi

dnl -----------------------------------------------------------------------
dnl install prefix
dnl -----------------------------------------------------------------------

if test "x$prefix" != "xNONE"; then
    FR_PREFIX=$prefix
else
    FR_PREFIX=$ac_default_prefix
fi

AC_DEFINE_UNQUOTED(FR_INSTALL_PREFIX, "$FR_PREFIX")

dnl -----------------------------------------------------------------------
dnl wxWidgets include directories to find wx.rc
dnl -----------------------------------------------------------------------

WX_INCLUDES=""
for opt in $WX_CPPFLAGS
do
    case "$opt" in
        -I*)
            WX_INCLUDES="$WX_INCLUDES --include-dir `echo "$opt" | sed 's/^-I//'`" 
            ;;
    esac
done
AC_SUBST(WX_INCLUDES)



AC_CONFIG_HEADERS([ frconfig.h:frconfig.h.in ])
AC_CONFIG_FILES([ Makefile ])

AC_OUTPUT

