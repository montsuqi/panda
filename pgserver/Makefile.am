## Process this file with automake to produce Makefile.in

bin_PROGRAMS = pgserver

pgserver_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl	\
	-L$(top_srcdir)/libs -lpandas

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(GNOME_INCLUDEDIR)

pgserver_SOURCES	=	\
	pgserver.c	pgserver.h	\
	server.c	

