## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = pgserver

pgserver_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl	\
	-L$(top_srcdir)/datalib -lpandadata	\
	-L$(top_srcdir)/libs -lpandas

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/datalib	\
	$(POSTGRES_CFLAGS)	\
	$(FIXED_CFLAGS)		\
	$(GNOME_INCLUDEDIR)

pgserver_SOURCES	=	\
	pgserver.c	pgserver.h	\
	server.c	

