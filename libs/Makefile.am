lib_LTLIBRARIES	=	libpanda.la libpandas.la libpandacom.la libpandagl.la libpandamisc.la panda.la

noinst_PROGRAMS	=	enum

panda_la_LDFLAGS	=	-module -export-dynamic
panda_la_SOURCES	=	panda.c
panda_la_DEPENDENCIES = libpanda.la
panda_la_LIBADD		=	\
	-L. -lpanda

libpandaincludedir = $(includedir)/panda
libpandainclude_HEADERS =	\
	enum.h
libpanda_la_DEPENDENCIES =	\
	enum.h

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/dblib	\
	-I$(top_srcdir)/blob	\
	$(GTK_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(FIXED_CFLAGS)			\
	$(MONDAI_CFLAGS)

enum_LDADD	= 	\
	$(GLADE_LIBS)			\
	-L/usr/lib -lglib -lm	\
	$(MONDAI_LIBS)

libpanda_la_SOURCES	=		\
	Dlex.h					\
	mhandler.c	mhandler.h	\
	DDparser.c	DDparser.h	\
	BDparser.c	BDparser.h	\
	LDparser.c	LDparser.h	\
	DBDparser.c	DBDparser.h	\
	DIparser.c	DIparser.h	\
	DBparser.c	DBparser.h	\
	SQLlex.c	SQLlex.h	\
	SQLparser.c	SQLparser.h	\
	directory.c	directory.h	\
	wfcio.c		wfcio.h		\
	load.c		load.h

libpandas_la_SOURCES	=		\
	applications.c applications.h	\
	DDparser.c	DDparser.h			\
	driver.c	driver.h	enum.h	\
	front.c		front.h				\
	comm.c			comm.h			\
	authstub.c	authstub.h			\
	load.c		load.h

libpandas_la_DEPENDENCIES	=		\
	$(top_srcdir)/defaults.h

#libpandacominclude_HEADERS =	\
#	comm.h			\
#	comms.h			\
#	socket.h		\
#	pty.h			\
#	queue.h

libpandacom_la_SOURCES	=			\
	auth.c			auth.h			\
	net.c			net.h			\
	comm.c			comm.h			\
	comms.c			comms.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	pty.c			pty.h			\
	queue.c			queue.h
libpandagl_la_SOURCES	=			\
	net.c			net.h			\
	socket.c		socket.h		\
	port.c			port.h			\
	queue.c			queue.h

enum.h:	enum
	./enum > enum.h

libpandamisc_la_SOURCES	=		\
	option.c		option.h	\
	message.c		message.h
