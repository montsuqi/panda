## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = wfc
bin_PROGRAMS = wfccontrol

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	-I$(top_srcdir)/blob	\
	$(SSL_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

LDADD 			=						\
	-lm									\
	$(SSL_LIBS)							\
	$(GLIB_LIBS)						\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/blob/libpandablob.la
DEPENDENCIES	=						\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(top_builddir)/blob/libpandablob.la


wfc_SOURCES	=							\
	wfc.c		wfc.h						\
	mqthread.c		mqthread.h			\
	corethread.c	corethread.h		\
	blobthread.c	blobthread.h		\
	controlthread.c	controlthread.h		\
	blobserv.c		blobserv.h			\
	termthread.c	termthread.h

wfccontrol_SOURCES	=					\
	wfccontrol.c						\
	controlthread.h

