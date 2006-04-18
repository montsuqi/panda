#!/bin/sh

set -e

echo "Running libtoolize..."
libtoolize --automake --force --copy

echo "Running aclocal..."
aclocal-1.4

echo "Running autoheader..."
autoheader

echo "Running automake..."
automake-1.4 --copy --add-missing

echo "Running autoconf..."
autoconf

echo 'Done.  Run "./configure" now'
echo 'Usually configure option:'

echo 'CFLAGS="-g -Wall -Wunused" ./configure --enable-maintainer-mode --enable-opencobol --with-postgres=yes --with-ruby=/usr/bin/ruby --with-eruby=/usr/bin/eruby'
