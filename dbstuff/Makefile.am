# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2007 ogochan
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

sbin_PROGRAMS =  dbredirector dbs

dbredirector_LDADD 		=				\
	-lm									\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandacom.a	\
	$(top_builddir)/libs/libpandamisc.a	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(POSTGRES_LIBS)					

dbs_LDADD 		=						\
	-lm									\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandas.a	\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						

INCLUDES 			= 					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/libs				\
	-I$(top_srcdir)/blob				\
	-I$(top_srcdir)/dblib				\
	$(POSTGRES_CFLAGS)					\
	$(SSL_CFLAGS)						\
	$(MONDAI_CFLAGS)					\
	$(GLIB_CFLAGS)

dbs_LDFLAGS	=	-export-dynamic
dbs_SOURCES	=							\
	dbs_main.c	dbs_main.h

dbs_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/libs/libpandamisc.a

dbredirector_LDFLAGS	=	-export-dynamic
dbredirector_SOURCES	=	\
	dbredirector.c

dbredirector_DEPENDENCIES	=			\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a

EXTRA_DIST = 							\
	dbslib.rb							\
	testdbs.rb
