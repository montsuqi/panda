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
echo './configure --prefix=$HOME/local --enable-maintainer-mode --enable-opencobol --with-postgres=yes --with-ruby=/usr/bin/ruby CFLAGS="-g -Wall -Wunused"'
