sbin_PROGRAMS =  dbredirector dbs


dbredirector_LDADD 		=				\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(POSTGRES_LIBS)					\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/dblib/libpandadb.la

dbs_LDADD 		=						\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandas.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/dblib/libpandadb.la

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(POSTGRES_CFLAGS)		\
	$(SSL_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

dbs_LDFLAGS	=	-export-dynamic
dbs_SOURCES	=	\
	dbs_main.c

dbs_DEPENDENCIES	=					\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/dblib/libpandadb.la

dbredirector_SOURCES	=	\
	dbredirector.c

dbredirector_DEPENDENCIES	=			\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/dblib/libpandadb.la




