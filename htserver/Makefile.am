## Process this file with automake to produce Makefile.in

bin_PROGRAMS = mon htcproc mce
sbin_PROGRAMS = htserver

htserver_LDFLAGS	=	-export-dynamic
htserver_LDADD 		=					\
	-lm									\
	$(SSL_LIBS)							\
	$(GLIB_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpandas.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

mon_LDADD 		=						\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

htcproc_LDADD 	=						\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(SSL_CFLAGS)		\
	$(MONDAI_CFLAGS)	\
	$(GLIB_CFLAGS)

htserver_SOURCES	=	\
	htserver.c	htserver.h	\
	server.c	\
	trid.c		trid.h
htserver_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandas.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

mon_SOURCES	=	\
	cgi.c		cgi.h		\
	multipart.c	multipart.h	\
	HTClex.c	HTClex.h	\
	HTCparser.c	HTCparser.h	\
	tags.c		tags.h		\
	exec.c		exec.h		\
	htc.c		htc.h		\
	mon.c		mon.h

mon_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

htcproc_SOURCES	=	\
	cgi.c		cgi.h		\
	multipart.c	multipart.h	\
	HTClex.c	HTClex.h	\
	HTCparser.c	HTCparser.h	\
	tags.c		tags.h		\
	exec.c		exec.h		\
	htc.c		htc.h		\
	htcproc.c

htcproc_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

mce_LDADD 		=						\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(XML2_LIBS)						\
	$(XSLT_LIBS)						\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la
mcd_INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libs		\
	$(XML2_CFLAGS)				\
	$(XSLT_CFLAGS)				\
	$(MONDAI_CFLAGS)			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

mce_SOURCES	=	\
	mce.c
