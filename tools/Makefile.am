# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2008  ogochan
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

bin_PROGRAMS = pandadb copygen dbgen checkdir $(dbsyncbin)
sbin_PROGRAMS = monitor msgd
bin_SCRIPTS = recdefgen.rb recgen.rb
noinst_SCRIPTS = copy

copygen_LDFLAGS	=			\
	-export-dynamic

copygen_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

dbgen_LDFLAGS	=			\
	-export-dynamic
dbgen_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

pandadb_LDFLAGS	=			\
	-export-dynamic
pandadb_LDADD 		=					\
	-lm									\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

checkdir_LDFLAGS	=			\
	-export-dynamic
checkdir_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

monitor_LDFLAGS	=			\
	-export-dynamic
monitor_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

msgd_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la	\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(P11_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(COMMON_CFLAGS)		\
	$(POSTGRES_CFLAGS)		\
	-DSERVER_DIR=\"$(sbindir)\"

monitor_DEPENDENCIES	=				\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la

copygen_SOURCES		=	\
	copygen.c

dbgen_SOURCES		=	\
	dbgen.c

pandadb_SOURCES		=	\
	pandadb.c

checkdir_SOURCES	=	\
	checkdir.c
monitor_SOURCES		=	\
	monitor.c
msgd_SOURCES		=	\
	msgd.c

if POSTGRESQL
dbsyncbin = dbsync

dbsync_SOURCES	= \
	$(top_builddir)/defaults.h			\
	dbsync.c

dbsync_LDADD			=		\
	-lm		\
	$(top_builddir)/dblib/libpandadb.la			\
	$(top_builddir)/dblib/libpostgresqlutils.a	\
	$(top_builddir)/libs/libpanda.la			\
	$(P11_LIBS)									\
	$(SSL_LIBS)									\
	$(COMMON_LIBS)								\
	$(POSTGRES_LIBS)			

dbsync_LDFLAGS	=	-export-dynamic

dbsync_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la			\
	$(top_builddir)/dblib/libpostgresqlutils.a	

else
dbsyncbin = 
dbsync_SOURCES	=
endif


EXTRA_DIST =				\
	recdefgen.rb			\
	recgen.rb				\
	copy
