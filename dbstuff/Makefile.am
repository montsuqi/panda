sbin_PROGRAMS =  dbredirector dbs

dbredirector_LDADD 		=				\
	-lm									\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/dblib/libpandadb.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(POSTGRES_LIBS)					

dbs_LDADD 		=						\
	-lm									\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandas.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/dblib/libpandadb.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/blob	\
	-I$(top_srcdir)/dblib	\
	$(POSTGRES_CFLAGS)		\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

dbs_LDFLAGS	=	-export-dynamic
dbs_SOURCES	=	\
	dbs_main.c	dbs_main.h

dbs_DEPENDENCIES	=					\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/dblib/libpandadb.la

dbredirector_LDFLAGS	=	-export-dynamic
dbredirector_SOURCES	=	\
	dbredirector.c

dbredirector_DEPENDENCIES	=			\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/dblib/libpandadb.la

