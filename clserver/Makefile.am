## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = clserver

clserver_LDADD 		=	\
	-L/usr/lib -lglib -lm $(XML2_LIBS)	\
	-L$(top_srcdir)/libs -lpandas

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(XML2_CFLAGS)		\
	$(GNOME_INCLUDEDIR)

clserver_SOURCES	=	main.c
#	clserver.c	clserver.h	\
#	server.c	server.h
