## Process this file with automake to produce Makefile.in

bin_PROGRAMS = copygen dbgen checkdir msgd
sbin_PROGRAMS = monitor
bin_SCRIPTS = recdefgen.rb recgen.rb

copygen_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la
dbgen_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la
checkdir_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la
monitor_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpanda.la		\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandacom.la
msgd_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
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

copygen_SOURCES		=	\
	copygen.c

#structgen_SOURCES	=	\
#	structgen.c

dbgen_SOURCES		=	\
	dbgen.c

checkdir_SOURCES	=	\
	checkdir.c
monitor_SOURCES		=	\
	monitor.c
msgd_SOURCES		=	\
	msgd.c

install-exec-hook:
	$(LN_S) -f user $(bindir)/gluseradd
	$(LN_S) -f user $(bindir)/gluserdel
	$(LN_S) -f user $(bindir)/glusermod
