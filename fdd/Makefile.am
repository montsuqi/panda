## Process this file with automake to produce Makefile.in

bin_PROGRAMS = fdw
sbin_PROGRAMS = fdd

LDADD 		=							\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GLIB_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)

fdw_SOURCES		=	\
	fdw.c	fdd.h
fdd_SOURCES		=	\
	fdd.c	fdd.h
