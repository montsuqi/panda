## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = wfc

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(SSL_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

wfc_LDADD 		=						\
	-lm									\
	$(SSL_LIBS)							\
	$(GLIB_LIBS)						\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la


wfc_SOURCES	=							\
	wfc.c								\
	blob.c			blob.h				\
	mqthread.c		mqthread.h			\
	corethread.c	corethread.h		\
	termthread.c	termthread.h

wfc_DEPENDENCIES	=					\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la
