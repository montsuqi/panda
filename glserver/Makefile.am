## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = glserver glauth

glserver_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl	\
	-L$(top_srcdir)/libs -lpandas
glauth_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl -lcrypt	\
	-L$(top_srcdir)/libs -lpandas

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(GNOME_INCLUDEDIR) \
	$(FIXED_CFLAGS)

glserver_SOURCES	=	\
	glserver.c	glserver.h	\
	server.c	

glauth_SOURCES	=	\
	glauth.c

