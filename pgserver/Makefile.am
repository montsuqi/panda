## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = pgserver

pgserver_LDFLAGS	=	-export-dynamic
pgserver_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpandas.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la


INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(GLIB_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

pgserver_SOURCES	=	\
	pgserver.c	pgserver.h	\
	server.c	
pgserver_DEPENDENCIES	=				\
	$(top_srcdir)/libs/libpandas.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la

