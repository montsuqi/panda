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
lib_LTLIBRARIES = MCPSUB.la MONFUNC.la $(HANDLER_OBJECTS)

libpandadbincludedir = $(includedir)/panda
libpandadbinclude_HEADERS =	\
	apslib.h


MCPSUB_la_LDFLAGS	=			\
	-module 					\
	-version-info $(LTVERSION)	\
	-export-dynamic
MONFUNC_la_LDFLAGS	=			\
	-module 					\
	-version-info $(LTVERSION)	\
	-export-dynamic

LDADD 		=							\
	-lm									\
	$(top_builddir)/dblib/libpandadb.a  \
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

MCPSUB_la_LIBADD	=
MONFUNC_la_LIBADD	=

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(OPENCOBOL_CFLAGS)		\
	$(OPENCOBOL23_CFLAGS)	\
	$(OPENCOBOL08_CFLAGS)	\
	$(RUBY_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

OpenCOBOLla = 
if OPENCOBOL
OpenCOBOLla += OpenCOBOL.la
endif
if OPENCOBOL23
OpenCOBOLla += OpenCOBOL23.la
endif
if OPENCOBOL08
OpenCOBOLla += OpenCOBOL08.la
endif

HANDLER_OBJECTS	=	\
	dotCOBOL.la		\
	C.la			\
	Exec.la			\
	Resident.la		\
	Ruby.la			\
	$(OpenCOBOLla)

dotCOBOL_la_LDFLAGS		=	-module -export-dynamic
if OPENCOBOL
OpenCOBOL_la_LDFLAGS	=	-module -export-dynamic
endif
if OPENCOBOL23
OpenCOBOL23_la_LDFLAGS	=	-module -export-dynamic
endif
if OPENCOBOL08
OpenCOBOL08_la_LDFLAGS	=	-module -export-dynamic
endif
C_la_LDFLAGS			=	-module -export-dynamic
Exec_la_LDFLAGS			=	-module -export-dynamic
Resident_la_LDFLAGS		=	-module -export-dynamic
Ruby_la_LDFLAGS			=   -module -export-dynamic

dotCOBOL_la_SOURCES		=	dotCOBOL.c

C_la_SOURCES			=	\
	C.c						\
	apslib.c

if OPENCOBOL23
OpenCOBOL23_la_SOURCES	=	OpenCOBOL.c
OpenCOBOL23_la_LIBADD		=	\
	$(OPENCOBOL23_LIBS)
endif
if OPENCOBOL08
OpenCOBOL08_la_SOURCES	=	OpenCOBOL.c
OpenCOBOL08_la_LIBADD		=	\
	$(OPENCOBOL08_LIBS)
endif
if OPENCOBOL
OpenCOBOL_la_SOURCES	=	\
	OpenCOBOL.c
OpenCOBOL_la_LIBADD		=	$(OPENCOBOL_LIBS)
endif

Ruby_la_SOURCES			=	\
	Ruby.c					\
	apslib.c
Ruby_la_LIBADD			=	$(RUBY_LIBS)

Exec_la_SOURCES			=	Exec.c
Exec_la_LIBADD			=	\
	$(top_builddir)/libs/comm.lo \
	$(top_builddir)/libs/comms.lo

Resident_la_SOURCES		=	Resident.c

aps_LDFLAGS	=			\
	-export-dynamic
aps_SOURCES	=	\
	aps_main.c	aps_main.h				\
	apsio.c		apsio.h					\
	handler.c	handler.h
aps_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

dbstub_LDFLAGS	=			\
	-export-dynamic
dbstub_SOURCES	=	\
	dbstub_main.c						\
	handler.c		handler.h
dbstub_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	MCPSUB.la							\
	$(top_builddir)/dblib/libpandadb.a	\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

MCPSUB_la_SOURCES	=			\
	MCPSUB.c					\
	OpenCOBOL.h					\
	apslib.c
MONFUNC_la_SOURCES	=			\
	MONFUNC.c					\
	OpenCOBOL.h					\
	apslib.c

EXTRA_DIST = apslib.rb
