sbin_PROGRAMS =  dbredirector dbs

dbredirector_LDADD 		=	\
	-L/usr/lib -lglib -lm -ldl -lpthread	\
	$(FIXED_LIBS)		\
	$(POSTGRES_LIBS) \
	$(OPENCOBOL_LIBS)	\
	$(top_srcdir)/libs/libpanda.la	\
	$(top_srcdir)/datalib/libpandadata.la	\
	$(top_srcdir)/dblib/libpandadb.la

dbs_LDADD 		=	\
	-L/usr/lib -lglib -lm -ldl -lpthread	\
	$(FIXED_LIBS)		\
	$(top_srcdir)/libs/libpanda.la	\
	$(top_srcdir)/datalib/libpandadata.la	\
	$(top_srcdir)/dblib/libpandadb.la

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/datalib	\
	-I$(top_srcdir)/dblib	\
	$(POSTGRES_CFLAGS)	\
	$(OPENCOBOL_CFLAGS)	\
	$(FIXED_CFLAGS)		\
	$(GNOME_INCLUDEDIR)

dbs_SOURCES	=	\
	dbs_main.c

dbredirector_SOURCES	=	\
	dbredirector.c
