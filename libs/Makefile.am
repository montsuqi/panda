lib_LTLIBRARIES	=	libpanda.la libpandas.la libpandacom.la libpandamisc.la panda.la

noinst_PROGRAMS	=	enum

panda_la_LDFLAGS	=	-module -export-dynamic
panda_la_SOURCES	=	panda.c
panda_la_DEPENDENCIES = libpanda.la
panda_la_LIBADD		=	\
	-L. -lpanda

libpanda_la_DEPENDENCIES = enum.h

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	$(GTK_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

enum_LDADD	= 	\
	$(GLADE_LIBS)			\
	-L/usr/lib -lglib -lm	\
	$(MONDAI_LIBS)

libpanda_la_SOURCES	=		\
	LDparser.c	LDparser.h	\
	LDlex.c		LDlex.h		\
	DIparser.c	DIparser.h	\
	DIlex.c		DIlex.h		\
	BDparser.c	BDparser.h	\
	BDlex.c		BDlex.h		\
	DBparser.c	DBparser.h	\
	DBlex.c		DBlex.h		\
	directory.c	directory.h	\
	wfcio.c		wfcio.h		\
	load.c		load.h

libpandas_la_SOURCES	=		\
	applications.c applications.h	\
	driver.c	driver.h	enum.h	\
	front.c		front.h				\
	authstub.c	authstub.h			\
	load.c		load.h

libpandas_la_DEPENDENCIES	=		\
	$(top_srcdir)/defaults.h

#libpandacominclude_HEADERS =	\
#	comm.h			\
#	comms.h			\
#	tcp.h			\
#	pty.h			\
#	queue.h
libpandacom_la_SOURCES	=		\
	net.c			net.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	tcp.c			tcp.h			\
	pty.c			pty.h			\
	queue.c			queue.h

enum.h:	enum
	./enum > enum.h

libpandamisc_la_SOURCES	=		\
	option.c		option.h	\
	message.c		message.h
