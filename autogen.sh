#!/bin/sh

#ACLOCAL=`which aclocal-1.4 2> /dev/null || echo aclocal`
#AUTOHEADER=`which autoheader-2.13 2> /dev/null || echo autoheader`
#AUTOMAKE=`which automake-1.4 2> /dev/null || echo automake`
#AUTOCONF=`which autoconf-2.13 2> /dev/null || echo autoconf`

echo "Running libtoolize..."
libtoolize --automake --force --copy

echo "Running aclocal..."
#$ACLOCAL -I m4
aclocal

echo "Running autoheader..."
autoheader  2> /dev/null

echo "Running automake..."
#$AUTOMAKE --copy --add-missing
automake --copy --add-missing

echo "Running autoconf..."
autoconf

echo 'Done.  Run "configure --enable-maintainer-mode" now'
