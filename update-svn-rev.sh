#/bin/sh

if test -d .svn ; then
  ACTVERSION=`svn info | awk '/Revision/{print $2}'`
else
  ACTVERSION=""; 
fi

if test -f src/frsvnrev.h ; then
  HEADERVERSION=`awk '/FR_VERSION_SVN/{print $3}' src/frsvnrev.h`
else
  HEADERVERSION="";
fi

if test -n $ACTVERSION ; then
  if test -z $HEADERVERSION || test $ACTVERSION -gt $HEADERVERSION ; then
    echo "Writing svn revision $ACTVERSION to frsvnrev.h"
    echo "#define FR_VERSION_SVN $ACTVERSION" > src/frsvnrev.h;
  fi
elif test -n $HEADERVERSION ; then
  echo "Deleting svn revision from frsvnrev.h"
  echo "#undef FR_VERSION_SVN" > src/frsvnrev.h
fi
