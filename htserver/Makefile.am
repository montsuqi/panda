## Process this file with automake to produce Makefile.in

bin_PROGRAMS = htserver mon

htserver_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl -lpthread	\
	-L$(top_srcdir)/libs -lpandas
mon_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl -lpthread	\
	-L$(top_srcdir)/libs -lpandas

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(GNOME_INCLUDEDIR)

htserver_SOURCES	=	\
	htserver.c	htserver.h	\
	server.c	\
	trid.c		trid.h

mon_SOURCES	=	\
	HTClex.c	HTClex.h	\
	HTCparser.c	HTCparser.h	\
	tags.c		tags.h	\
	exec.c		exec.h	\
	mon.c		mon.h
