#!/bin/sh

echo "Running libtoolize..."
libtoolize --automake --force --copy

echo "Running gettextize..."
gettextize --force --copy --intl
