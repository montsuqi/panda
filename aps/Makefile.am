## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = aps dbstub
lib_LTLIBRARIES = MCPSUB.la $(HANDLER_OBJECTS)

libpandadbincludedir = $(includedir)/panda
libpandadbinclude_HEADERS =	\
	apslib.h

aps_LDFLAGS		=	-export-dynamic
dbstub_LDFLAGS	=	-export-dynamic

MCPSUB_la_LDFLAGS	=			\
	-module 					\
	-version-info $(LTVERSION)	\
	-export-dynamic

LDADD 		=							\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(OPENCOBOL_LIBS)					\
	$(RUBY_LIBS)						\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/dblib/libpandadb.la

MCPSUB_la_LIBADD	=

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/blob	\
	-I$(top_srcdir)/dblib	\
	$(OPENCOBOL_CFLAGS)		\
	$(RUBY_CFLAGS)		\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

HANDLER_OBJECTS	=	\
	dotCOBOL.la		\
	OpenCOBOL.la	\
	C.la			\
	Exec.la			\
	Resident.la		\
	Ruby.la

dotCOBOL_la_LDFLAGS		=	-module -export-dynamic
OpenCOBOL_la_LDFLAGS	=	-module -export-dynamic
C_la_LDFLAGS			=	-module -export-dynamic
Exec_la_LDFLAGS			=	-module -export-dynamic
Resident_la_LDFLAGS		=	-module -export-dynamic
Ruby_la_LDFLAGS			=   -module -export-dynamic

dotCOBOL_la_SOURCES		=	dotCOBOL.c
C_la_SOURCES			=	C.c
OpenCOBOL_la_SOURCES	=	OpenCOBOL.c
OpenCOBOL_la_LIBADD		=	\
	$(OPENCOBOL_LIBS)
Exec_la_SOURCES			=	Exec.c
Resident_la_SOURCES		=	Resident.c
Ruby_la_SOURCES			=	Ruby.c
Ruby_la_LIBADD			=	$(RUBY_LIBS)

aps_SOURCES	=	\
	aps_main.c	aps_main.h				\
	apsio.c		apsio.h					\
	handler.c	handler.h

aps_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/dblib/libpandadb.la

dbstub_SOURCES	=	\
	dbstub_main.c						\
	handler.c		handler.h

dbstub_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	MCPSUB.la							\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/dblib/libpandadb.la

MCPSUB_la_SOURCES	=			\
	MCPSUB.c				\
	OpenCOBOL.h
