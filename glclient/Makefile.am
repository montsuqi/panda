if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = -lglade-panda-gnome $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = $(GTK_LIBS)
endif

INCLUDES	=		\
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(THE_FLAGS)			\
	$(XML_CFLAGS)			\
	$(GLADE_CFLAGS)			\
	$(PANDA_CFLAGS)

bin_PROGRAMS = glclient

glclient_LDADD	= 	\
	$(THE_LIBS) $(GLADE_LIBS) $(PANDA_LIBS)	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm	-lpthread \
	-L$(top_srcdir)/libs -lpandac


glclient_SOURCES	=	\
	client.c 	\
	protocol.c	protocol.h	\
	callbacks.c	callbacks.h	\
	widgetOPS.c	widgetOPS.h	\
	styleLex.c styleLex.h	\
	styleParser.c styleParser.h

