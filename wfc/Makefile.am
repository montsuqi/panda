## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = wfc

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/datalib	\
	$(POSTGRES_CFLAGS)	\
	$(GNOME_INCLUDEDIR) \
	$(FIXED_CFLAGS)

wfc_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl -lpthread $(POSTGRES_LIBS) \
	-L$(top_srcdir)/libs -li18n 	\
	$(top_srcdir)/datalib/libpandadata.la	\
	$(top_srcdir)/libs/libpanda.la	

wfc_SOURCES	=	\
	wfc.c			\
	mqthread.c		mqthread.h		\
	corethread.c	corethread.h	\
	termthread.c	termthread.h
