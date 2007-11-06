#/bin/sh

FRDIR=`dirname $0`
SVNREVFILE=src/frsvnrev.h

if test -d $FRDIR/.svn ; then
  ACTVERSION=`svn info $FRDIR | awk '/Revision/{print $2}'`
else
  ACTVERSION=""; 
fi

if test -f $SVNREVFILE ; then
  HEADERVERSION=`awk '/FR_VERSION_SVN/{print $3}' $SVNREVFILE`
else
  HEADERVERSION="";
fi

if test -n $ACTVERSION ; then
  if test -z $HEADERVERSION || test $ACTVERSION -gt $HEADERVERSION ; then
    echo "Writing svn revision $ACTVERSION to $SVNREVFILE"
    echo "#define FR_VERSION_SVN $ACTVERSION" > $SVNREVFILE;
  fi
elif test -n $HEADERVERSION ; then
  echo "Deleting svn revision from $SVNREVFILE"
  echo "#undef FR_VERSION_SVN" > $SVNREVFILE
fi
