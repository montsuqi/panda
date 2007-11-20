#!/bin/sh

set -e

LIBTOOLIZE=libtoolize
ACLOCAL=aclocal-1.10
AUTOHEADER=autoheader2.50
AUTOMAKE=automake-1.10
AUTOCONF=autoconf2.50

echo "Running libtoolize..."
$LIBTOOLIZE --automake --force --copy

echo "Running aclocal..."
$ACLOCAL -I ./m4

echo "Running autoheader..."
#$AUTOHEADER
autoheader

echo "Running automake..."
$AUTOMAKE --copy --add-missing --force

echo "Running autoconf..."
#$AUTOCONF
autoconf

echo 'Done.  Run "./configure" now'
echo 'Usually configure option:'

echo 'CFLAGS="-g -Wall -Wunused" ./configure --enable-maintainer-mode --enable-opencobol --with-postgres=yes --with-ruby=/usr/bin/ruby --with-eruby=/usr/bin/eruby'
