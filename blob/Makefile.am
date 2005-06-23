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

lib_LTLIBRARIES =	libpandablob.la
noinst_PROGRAMS	=	testblob

libpandablob_la_LDFLAGS	=	\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpandablobincludedir = $(includedir)/panda
libpandablobinclude_HEADERS =	\
	blob_v1.h					\
	blob_v2.h
	blob.h

INCLUDES 			=		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)		\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

libpandablob_la_SOURCES	=			\
	blob_v1.c		blob_v1.h		\
					blob_v2.h		\
	table.c			table.h			\
	blob.c			blob.h

libpandablob_la_DEPENDENCIES	=		\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

testblob_LDADD	=				\
	-lm					\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/blob/libpandablob.la	\
	$(GLIB_LIBS)				\
	$(SSL_LIBS)				\
	$(MONDAI_LIBS)				

testblob_DEPENDENCIES	=			\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/blob/libpandablob.la

