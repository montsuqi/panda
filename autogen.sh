#!/bin/sh

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

echo 'Done.  Run "configure --enable-maintainer-mode --with-postgres=yes --with-shell=yes --with-ruby=/usr/bin/ruby --with-opencobol" now'
