lib_LTLIBRARIES = libpandadb.la

libpandadb_la_LDFLAGS	=	\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpandadb_la_LIBADD	=	\
	$(POSTGRES_LIBS)

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/datalib	\
	$(POSTGRES_CFLAGS)	\
	$(FIXED_CFLAGS)		\
	$(GNOME_INCLUDEDIR)

libpandadb_la_SOURCES	=	\
	dbgroup.c	dbgroup.h	\
	redirect.c	redirect.h	\
	Postgres.c	Postgres.h	\
	Shell.c		Shell.h

