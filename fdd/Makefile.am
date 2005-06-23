# PANDA -- a simple transaction monitor
# Copyright (C) 2005  ogochan
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA

## Process this file with automake to produce Makefile.in

bin_PROGRAMS = fdw
sbin_PROGRAMS = fdd

LDADD 		=							\
	-lm									\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GLIB_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)

fdw_SOURCES		=	\
	fdw.c	fdd.h
fdd_SOURCES		=	\
	fdd.c	fdd.h
