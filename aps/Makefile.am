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

sbin_PROGRAMS = aps dbstub
pkglib_LTLIBRARIES = MONFUNC.la $(HANDLER_OBJECTS)


libpandadbincludedir = $(pkgincludedir)
libpandadbinclude_HEADERS =	\
	apslib.h

MONFUNC_la_LDFLAGS	=			\
	-module 					\
	-avoid-version				\
	-export-dynamic

LDADD 		=							\
	-lm									\
	$(top_builddir)/dblib/libpandadb.la  \
	$(top_builddir)/libs/libpanda.la	\
	$(COMMON_LIBS)						\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(OPENCOBOL_LIBS)

MONFUNC_la_LIBADD	=

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(OPENCOBOL_CFLAGS)		\
	$(P11_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(COMMON_CFLAGS)

OpenCOBOLla = 
OpenCOBOLla += OpenCOBOL.la

HANDLER_OBJECTS	=	\
	C.la			\
	Exec.la			\
	Resident.la		\
	OpenCOBOL.la

OpenCOBOL_la_LDFLAGS	=	-module -export-dynamic -avoid-version
C_la_LDFLAGS			=	-module -export-dynamic -avoid-version
Exec_la_LDFLAGS			=	-module -export-dynamic -avoid-version
Resident_la_LDFLAGS		=	-module -export-dynamic -avoid-version

C_la_SOURCES			=	\
	C.c						\
	apslib.c

OpenCOBOL_la_SOURCES	=	\
	MONFUNC.c				\
	MONFUNC.h				\
	OpenCOBOL.c
OpenCOBOL_la_LIBADD		=	$(OPENCOBOL_LIBS)

Exec_la_SOURCES			=	Exec.c
Exec_la_LIBADD			=	\
	$(top_builddir)/libs/libpanda.la

Resident_la_SOURCES		=	Resident.c

aps_LDFLAGS	=			\
	-export-dynamic
aps_SOURCES	=	\
	aps_main.c	aps_main.h				\
	apsio.c		apsio.h					\
	handler.c	handler.h
aps_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la

dbstub_LDFLAGS	=			\
	-export-dynamic
dbstub_SOURCES	=	\
	dbstub_main.c						\
	handler.c		handler.h
dbstub_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.la	\
	$(top_builddir)/libs/libpanda.la

MONFUNC_la_SOURCES	=			\
	MONFUNC.c					\
	MONFUNC.h					\
	apslib.c

EXTRA_DIST = apslib.rb
