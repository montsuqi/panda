# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2006  ogochan
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
	$(POSTGRES_CFLAGS)		\
	$(OSEKI_CFLAGS)		\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

libpandadb_a_SOURCES	=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h
libpandadb_a_CFLAGS 	=	$(AM_CFLAGS)

DB_OBJECTS			=		\
	PostgreSQL.la			\
	Shell.la				\
	System.la				\
	Oseki.la				\
	NativeBLOB.la

PostgreSQL_la_LDFLAGS	=	-module -export-dynamic
PostgreSQL_la_SOURCES	=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h	\
	PostgreSQL.c
PostgreSQL_la_LIBADD	=	\
	$(top_builddir)/libs/net.lo		\
	$(top_builddir)/libs/socket.lo	\
	$(POSTGRES_LIBS)
PostgreSQL_la_DEPENDENCIES	=		\
	$(top_builddir)/defaults.h

Shell_la_LDFLAGS		=	-module -export-dynamic
Shell_la_SOURCES		=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h	\
	Shell.c
Shell_la_LIBADD	=

System_la_LDFLAGS		=	-module -export-dynamic
System_la_SOURCES		=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h	\
	System.c

NativeBLOB_la_LDFLAGS	=	-module -export-dynamic
NativeBLOB_la_SOURCES	=		\
	dbgroup.c	dbgroup.h		\
	redirect.c	redirect.h		\
	NativeBLOB.c
NativeBLOB_la_LIBADD	=		\
	$(top_builddir)/libs/blobreq.lo


Oseki_la_LDFLAGS	=	-module -export-dynamic
Oseki_la_SOURCES	=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h	\
	Oseki.c
Oseki_la_LIBADD		=	\
	$(OSEKI_LIBS)
