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
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/blob/libpandablob.la
DEPENDENCIES	=						\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la	\
	$(top_srcdir)/blob/libpandablob.la


wfc_SOURCES	=							\
	wfc.c								\
	mqthread.c		mqthread.h			\
	corethread.c	corethread.h		\
	controlthread.c	controlthread.h		\
	termthread.c	termthread.h
wfccontrol_SOURCES	=					\
	wfccontrol.c						\
	controlthread.h

