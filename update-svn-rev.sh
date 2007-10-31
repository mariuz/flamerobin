#/bin/sh

FRDIR=`dirname $0`
if test -d $FRDIR/.svn ; then
  ACTVERSION=`svn info $FRDIR | awk '/Revision/{print $2}'`
else
  ACTVERSION=""; 
fi

if test -f frsvnrev.h ; then
  HEADERVERSION=`awk '/FR_VERSION_SVN/{print $3}' frsvnrev.h`
else
  HEADERVERSION="";
fi

if test -n $ACTVERSION ; then
  if test -z $HEADERVERSION || test $ACTVERSION -gt $HEADERVERSION ; then
    echo "Writing svn revision $ACTVERSION to frsvnrev.h"
    echo "#define FR_VERSION_SVN $ACTVERSION" > frsvnrev.h;
  fi
elif test -n $HEADERVERSION ; then
  echo "Deleting svn revision from frsvnrev.h"
  echo "#undef FR_VERSION_SVN" > frsvnrev.h
fi
