## Process this file with automake to produce Makefile.in

bin_PROGRAMS = copygen dbgen checkdir
sbin_PROGRAMS = monitor
bin_SCRIPTS = recdefgen.rb

LDADD 		=							\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la


INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GLIB_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

monitor_DEPENDENCIES	=				\
	$(top_srcdir)/defaults.h			\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la

copygen_SOURCES	=	\
	copygen.c

#structgen_SOURCES	=	\
#	structgen.c

dbgen_SOURCES	=	\
	dbgen.c

checkdir_SOURCES	=	\
	checkdir.c
monitor_SOURCES	=	\
	monitor.c

install-exec-hook:
	$(LN_S) -f user $(bindir)/gluseradd
	$(LN_S) -f user $(bindir)/gluserdel
	$(LN_S) -f user $(bindir)/glusermod
