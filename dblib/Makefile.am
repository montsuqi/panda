lib_LTLIBRARIES = libpandadb.la $(DB_OBJECTS)

libpandadb_la_LDFLAGS	=	\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpandadb_la_LIBADD	=

libpandadbincludedir = $(includedir)/panda
libpandadbinclude_HEADERS =	\
	dblib.h

INCLUDES 			=		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/blob	\
	$(POSTGRES_CFLAGS)		\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

libpandadb_la_SOURCES	=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h

DB_OBJECTS			=		\
	PostgreSQL.la			\
	Shell.la				\
	System.la

PostgreSQL_la_LDFLAGS	=	-module -export-dynamic
PostgreSQL_la_SOURCES	=	PostgreSQL.c
PostgreSQL_la_LIBADD	=	\
	$(POSTGRES_LIBS)
PostgreSQL_la_DEPENDENCIES	=		\
	$(top_srcdir)/defaults.h

Shell_la_LDFLAGS		=	-module -export-dynamic
Shell_la_SOURCES		=	Shell.c

System_la_LDFLAGS		=	-module -export-dynamic
System_la_SOURCES		=	System.c
