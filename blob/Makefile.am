lib_LTLIBRARIES = libpandablob.la

libpandablob_la_LDFLAGS	=	\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpandablobincludedir = $(includedir)/panda
libpandablobinclude_HEADERS =	\
	blob.h

INCLUDES 			=		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)		\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GLIB_CFLAGS)

libpandablob_la_SOURCES	=	\
	blob.c			blob.h

libpandablob_la_DEPENDENCIES	=		\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la
