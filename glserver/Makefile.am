## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = glserver
bin_SCRIPTS = pstopng

glserver_LDFLAGS	=	-export-dynamic
glserver_LDADD 		=					\
	-lm -lmagic							\
	$(top_builddir)/libs/libpandas.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandagl.la	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						
glserver_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandas.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandagl.la
INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(GLIB_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	-DBIN_DIR=\"$(bindir)\"

glserver_SOURCES	=		\
	glserver.c	glserver.h	\
	glcomm.c	glcomm.h	\
	server.c
