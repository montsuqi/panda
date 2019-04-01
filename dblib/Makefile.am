# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2009  ogochan
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

noinst_LIBRARIES = libpostgresqlutils.a
pkglib_LTLIBRARIES = $(DB_OBJECTS)
lib_LTLIBRARIES = libpandadb.la

libpandadb_la_LIBADD = 	\
	-luuid				\
	$(AMQP_LIBS)

libpandadbincludedir = $(pkgincludedir)
libpandadbinclude_HEADERS =	\
	dblib.h			\
	dbutils.h		\
	dbgroup.h		\
	monsys.h		\
	bytea.h			\
	msglib.h		\
	monpushevent.h

noinst_PROGRAMS		=

AM_CPPFLAGS 			=	\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)		\
	$(RUBY_CFLAGS)			\
	$(AMQP_CFLAGS)			\
	$(COMMON_CFLAGS)		\
	-DBIN_DIR=\"$(bindir)\"

libpandadb_la_CFLAGS = $(AM_CFLAGS)
libpandadb_la_LDFLAGS	=		\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpandadb_la_SOURCES	=			\
	dbgroup.c		dbgroup.h		\
	dbutils.c		dbutils.h		\
	redirect.c		redirect.h		\
	monsys.c		monsys.h		\
	dbops.c			dbops.h			\
	bytea.c			bytea.h			\
	msglib.c		msglib.h		\
	monpushevent.c	monpushevent.h

libpostgresqlutils_a_LIBADD = 


libpostgresqlutils_a_CFLAGS = $(AM_CFLAGS)
libpostgresqlutils_a_SOURCES = \
	PostgreSQLlib.c PostgreSQLlib.h	\
	PostgreSQLutils.c PostgreSQLutils.h \
	audit.c audit.h 

DB_OBJECTS			=		\
	PostgreSQL.la			\
	Shell.la				\
	System.la				\
	NativeBLOB.la			\
	MONBLOB.la				\
	XMLIO2.la				\
	MSGIO.la				\
	MSGARRAY.la				\
	PushEvent.la

PostgreSQL_la_LDFLAGS	=		\
	-avoid-version				\
	-module						\
	-export-dynamic
PostgreSQL_la_SOURCES	=			\
	PostgreSQLlib.c PostgreSQLlib.h \
	PostgreSQL.c
PostgreSQL_la_LIBADD	=				\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la	\
	$(POSTGRES_LIBS)
PostgreSQL_la_DEPENDENCIES	=			\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/defaults.h

Shell_la_LDFLAGS		=		\
	-avoid-version				\
	-module						\
	-export-dynamic
Shell_la_SOURCES		=	\
	Shell.c
Shell_la_LIBADD	= \
	$(top_builddir)/libs/libpanda.la \
	$(top_builddir)/dblib/libpandadb.la \
	-luuid

System_la_LDFLAGS		=		\
	-avoid-version				\
	-module						\
	-export-dynamic
System_la_SOURCES		=		\
	sysdata.c	sysdata.h		\
	System.c
System_la_LIBADD	=			\
	$(top_builddir)/libs/libpanda.la \
	$(top_builddir)/dblib/libpandadb.la

NativeBLOB_la_LDFLAGS	=		\
	-avoid-version				\
	-module						\
	-export-dynamic
NativeBLOB_la_SOURCES	=		\
	NativeBLOB.c
NativeBLOB_la_LIBADD	=		\
	$(top_builddir)/libs/libpanda.la \
	$(top_builddir)/dblib/libpandadb.la

MONBLOB_la_LDFLAGS	=		\
	-avoid-version				\
	-module						\
	-export-dynamic
MONBLOB_la_SOURCES	=		\
	MONBLOB.c
MONBLOB_la_LIBADD	=		\
	$(top_builddir)/libs/libpanda.la \
	$(top_builddir)/dblib/libpandadb.la

XMLIO2_la_LDFLAGS	=	\
	-avoid-version				\
	-module						\
	-export-dynamic
XMLIO2_la_SOURCES	=	\
	XMLIO2.c
XMLIO2_la_LIBADD			=			\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la

MSGIO_la_LDFLAGS	=	\
	-avoid-version				\
	-module						\
	-export-dynamic
MSGIO_la_SOURCES	=	\
	MSGIO.c
MSGIO_la_LIBADD			=			\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la

MSGARRAY_la_LDFLAGS	=	\
	-avoid-version				\
	-module						\
	-export-dynamic
MSGARRAY_la_SOURCES	=	\
	MSGARRAY.c
MSGARRAY_la_LIBADD			=			\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la

PushEvent_la_LDFLAGS	=				\
	-avoid-version						\
	-module								\
	-export-dynamic
PushEvent_la_SOURCES	=				\
	PushEvent.c
PushEvent_la_LIBADD			=			\
	$(top_builddir)/libs/libpanda.la	\
	$(top_builddir)/dblib/libpandadb.la
