INCLUDES	=		\
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/datalib	\
	-I$(top_srcdir)/libs	\
	$(THE_FLAGS)			\
	$(XML_CFLAGS)			\
	$(GLADE_CFLAGS)			\
	$(PANDA_CFLAGS)

bin_PROGRAMS = glclient

glclient_LDADD	= 	\
	$(GLADE_LIBS) $(PANDA_LIBS)	\
	-L/usr/lib -lglib $(FIXED_LIBS) -lm	-lpthread \
	-L$(top_srcdir)/libs -lpandac	\
	$(top_srcdir)/datalib/libpandadata.la


glclient_SOURCES	=	\
	client.c 	\
	protocol.c	protocol.h	\
	callbacks.c	callbacks.h	\
	widgetOPS.c	widgetOPS.h	\
	styleLex.c styleLex.h	\
	styleParser.c styleParser.h

