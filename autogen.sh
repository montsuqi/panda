#!/bin/sh

set -e

echo "Running libtoolize..."
libtoolize --automake --force --copy

echo "Running aclocal..."
aclocal

echo "Running autoheader..."
autoheader

echo "Running automake..."
automake --copy --add-missing

echo "Running autoconf..."
autoconf

echo 'Done.  Run "./configure" now'
echo 'Usually configure option:'
echo 'CFLAGS="-g -Wall -Wunused" ./configure --enable-maintainer-mode --prefix=$HOME/local --enable-opencobol --with-postgres=yes --with-ruby=/usr/bin/ruby'
