## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = aps dbstub
lib_LTLIBRARIES = MCPSUB.la $(LANG_OBJECTS)

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
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(OPENCOBOL_LIBS)					\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/dblib/libpandadb.la

MCPSUB_la_LIBADD	=

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(OPENCOBOL_CFLAGS)		\
	$(SSL_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

LANG_OBJECTS	=	\
	dotCOBOL.la		\
	OpenCOBOL.la	\
	C.la			\
	Exec.la

dotCOBOL_la_LDFLAGS		=	-module -export-dynamic
OpenCOBOL_la_LDFLAGS	=	-module -export-dynamic
C_la_LDFLAGS			=	-module -export-dynamic
Exec_la_LDFLAGS			=	-module -export-dynamic

dotCOBOL_la_SOURCES		=	dotCOBOL.c
C_la_SOURCES			=	C.c
OpenCOBOL_la_SOURCES	=	OpenCOBOL.c
OpenCOBOL_la_LIBADD		=	\
	$(OPENCOBOL_LIBS)
Exec_la_SOURCES			=	Exec.c

aps_SOURCES	=	\
	aps_main.c	aps_main.h				\
	apsio.c		apsio.h					\
	loader.c	loader.h				\
	handler.c	handler.h

aps_DEPENDENCIES	=					\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/dblib/libpandadb.la

dbstub_SOURCES	=	\
	dbstub_main.c						\
	loader.c		loader.h			\
	handler.c		handler.h

dbstub_DEPENDENCIES	=					\
	$(top_srcdir)/defaults.h			\
	MCPSUB.la							\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/dblib/libpandadb.la

MCPSUB_la_SOURCES	=	\
	MCPSUB.c

