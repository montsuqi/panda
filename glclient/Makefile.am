bin_PROGRAMS = 					\
	glclient

glclient_SOURCES =				\
	client.c	glclient.h		\
	bootdialog.c	bootdialog.h		\
	bd_config.c	bd_config.h		\
	action.c	action.h		\
	protocol.c	protocol.h		\
	callbacks.c	callbacks.h		\
	widgetOPS.c	widgetOPS.h		\
	styleLex.c styleLex.h			\
	styleParser.c styleParser.h

glclient_DEPENDENCIES =				\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandagl.la

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/libs			\
	$(SSL_CFLAGS)				\
	$(XML_CFLAGS)				\
	$(GLADE_CFLAGS)				\
	$(FIXED_CFLAGS)				\
	$(MONDAI_CFLAGS)			\
	$(GTK_PANDA_CFLAGS)			\
	$(GTK_CFLAGS)				\
        -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

glclient_LDADD =				\
	-lm					\
	$(GLIB_LIBS)				\
	$(GTK_LIBS)				\
	$(GLADE_LIBS)				\
	$(SSL_LIBS)				\
	$(GTK_PANDA_LIBS)			\
	$(FIXED_LIBS)				\
	$(MONDAI_LIBS)				\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandagl.la
