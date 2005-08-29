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

noinst_LIBRARIES	=	libpandadb.a 
lib_LTLIBRARIES 	=	$(DB_OBJECTS)

libpandadb_a_LIBADD	=

libpandadbincludedir = $(includedir)/panda
libpandadbinclude_HEADERS =	\
	dblib.h

INCLUDES 			=		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/blob	\
	$(POSTGRES_CFLAGS)		\
	$(OSEKI_CFLAGS)		\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

libpandadb_a_SOURCES	=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h

DB_OBJECTS			=		\
	PostgreSQL.la			\
	Shell.la				\
	System.la				\
	Oseki.la				\
	NativeBLOB.la

PostgreSQL_la_LDFLAGS	=	-module -export-dynamic
PostgreSQL_la_SOURCES	=	PostgreSQL.c
PostgreSQL_la_LIBADD	=	\
	libpandadb.a		\
	$(POSTGRES_LIBS)
PostgreSQL_la_DEPENDENCIES	=		\
	$(top_srcdir)/defaults.h

Shell_la_LDFLAGS		=	-module -export-dynamic
Shell_la_SOURCES		=	Shell.c
Shell_la_LIBADD	=	\
	libpandadb.a

System_la_LDFLAGS		=	-module -export-dynamic
System_la_SOURCES		=	System.c
System_la_LIBADD	=	\
	libpandadb.a

NativeBLOB_la_LDFLAGS	=	-module -export-dynamic
NativeBLOB_la_SOURCES	=	NativeBLOB.c
NativeBLOB_la_LIBADD	=	\
	libpandadb.a

Oseki_la_LDFLAGS	=	-module -export-dynamic
Oseki_la_SOURCES	=	Oseki.c
Oseki_la_LIBADD		=	\
	libpandadb.a	\
	$(OSEKI_LIBS)
