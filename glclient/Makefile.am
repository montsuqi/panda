INCLUDES	=		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(SSL_CFLAGS)			\
	$(XML_CFLAGS)			\
	$(GLADE_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)		\
	$(GTK_PANDA_CFLAGS)

bin_PROGRAMS = glclient

glclient_LDADD	= 						\
	-lm									\
	$(GLIB_LIBS)						\
	$(GLADE_LIBS)						\
	$(SSL_LIBS)							\
	$(GTK_PANDA_LIBS)					\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandagl.la
glclient_DEPENDENCIES	=				\
	$(top_srcdir)/libs/libpandamisc.la	\
	$(top_srcdir)/libs/libpandagl.la


glclient_SOURCES	=		\
	client.c 				\
	action.c	action.h	\
	protocol.c	protocol.h	\
	callbacks.c	callbacks.h	\
	widgetOPS.c	widgetOPS.h	\
	styleLex.c styleLex.h	\
	styleParser.c styleParser.h

