lib_LTLIBRARIES	=	$(MONPLS_OBJECTS)
MONPLS_OBJECTS	=	\
	demo.la		\
	demo2.la	\
	demo3.la	\
	demo4.la
demo_la_LDFLAGS		=	-module -export-dynamic
demo2_la_LDFLAGS	=	-module -export-dynamic
demo3_la_LDFLAGS	=	-module -export-dynamic
demo4_la_LDFLAGS	=	-module -export-dynamic
demo_la_SOURCES		=	demo.c
demo2_la_SOURCES	=	demo2.c
demo3_la_SOURCES	=	demo3.c
demo4_la_SOURCES	=	demo4.c

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GTK_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(MONDAI_CFLAGS)
