## Process this file with automake to produce Makefile.in

bin_PROGRAMS = copygen dbgen checkdir msgd
sbin_PROGRAMS = monitor
bin_SCRIPTS = recdefgen.rb recgen.rb
noinst_SCRIPTS = copy

copygen_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

dbgen_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

checkdir_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

monitor_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

msgd_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GLIB_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	-DSERVER_DIR=\"$(sbindir)\"

monitor_DEPENDENCIES	=				\
	$(top_builddir)/defaults.h			\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

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

EXTRA_DIST =					\
	recdefgen.rb				\
	recgen.rb				\
	copy
