## Process this file with automake to produce Makefile.in

bin_PROGRAMS = wfc

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(GNOME_INCLUDEDIR)

wfc_LDADD 		=	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm -ldl -lpthread $(POSTGRES_LIBS)	\
	-L$(top_srcdir)/libs -lpanda -li18n

wfc_SOURCES	=	\
	wfc.c			\
	mqthread.c		mqthread.h		\
	corethread.c	corethread.h	\
	termthread.c	termthread.h
