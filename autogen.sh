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
