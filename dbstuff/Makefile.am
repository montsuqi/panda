# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2008 ogochan
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

sbin_PROGRAMS =  dbredirector dbs dbmaster dbslave dblogger

dbredirector_LDADD 		=				\
	-lm									\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/dblib/libdblog.a	\
	$(top_builddir)/dblib/libpostgresqlutils.a	\
	$(top_builddir)/libs/libpanda.la	\
	$(POSTGRES_LIBS)					\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)						


dbs_LDADD 		=						\
	-lm									\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/dblib/libdblog.a	\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)						

INCLUDES 			= 					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/libs				\
	-I$(top_srcdir)/blob				\
	-I$(top_srcdir)/dblib				\
	$(POSTGRES_CFLAGS)					\
	$(P11_CFLAGS)						\
	$(SSL_CFLAGS)						\
	$(COMMON_CFLAGS)

dbs_LDFLAGS	=	-export-dynamic
dbs_SOURCES	=							\
	dbs_main.c	dbs_main.h

dbs_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/dblib/libdblog.a	\
	$(top_builddir)/libs/libpanda.la

dbredirector_LDFLAGS	=	-export-dynamic
dbredirector_SOURCES	=	\
	dbredirector.c	dbredirector.h

dbredirector_DEPENDENCIES	=			\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/dblib/libdblog.a	\
	$(top_builddir)/dblib/libpostgresqlutils.a	\
	$(top_builddir)/libs/libpanda.la


dblogger_LDADD 		= $(dbredirector_LDADD)
dblogger_LDFLAGS	= $(dbredirector_LDFLAGS)
dblogger_SOURCES	= $(dbredirector_SOURCES)
dblogger_DEPENDENCIES	= $(dbredirector_DEPENDENCIES)


dbmaster_LDADD 		= $(dbredirector_LDADD)
dbmaster_LDFLAGS	=	-export-dynamic
dbmaster_SOURCES	=	\
	dbmaster.c dbmaster_thread.c 
dbmaster_DEPENDENCIES	=			\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/dblib/libdblog.a	\
	$(top_builddir)/dblib/libpostgresqlutils.a	\
	$(top_builddir)/libs/libpanda.la

dbslave_LDADD 		= $(dbredirector_LDADD)
dbslave_LDFLAGS	=	-export-dynamic
dbslave_SOURCES	=	\
	dbslave.c dbslave_thread.c 
dbslave_DEPENDENCIES	=			\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/dblib/libdblog.a	\
	$(top_builddir)/dblib/libpostgresqlutils.a	\
	$(top_builddir)/libs/libpanda.la


EXTRA_DIST = 							\
	dbslib.rb							\
	testdbs.rb

