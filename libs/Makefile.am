if GNOME_SUPPORT
THE_FLAGS = $(GNOME_INCLUDEDIR)
THE_LIBS  = -lglade-gnome -lglade $(GNOME_LIBDIR) $(GNOMEUI_LIBS)
else
THE_FLAGS = $(GTK_CFLAGS)
THE_LIBS  = -lglade $(GTK_LIBS)
endif

lib_LTLIBRARIES = libpanda.la

enum_LDADD	= 	\
	$(THE_LIBS) $(GLADE_LIBS)	$(PANDA_LIBS)	\
	-L/usr/lib -lglib -lm
noinst_PROGRAMS	=	enum

noinst_LIBRARIES = libi18n.a libpandac.a libpandas.a libpandat.a


libpanda_la_DEPENDENCIES = enum.h

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(GNOME_INCLUDEDIR)



libpanda_la_SOURCES	=		\
	misc.c		misc.h		\
	option.c	option.h	\
	comm.c		comm.h		\
	LBSfunc.c	LBSfunc.h	\
	value.c		value.h		\
	SQLlex.c	SQLlex.h	\
	SQLparser.c	SQLparser.h	\
	DDparser.c	DDparser.h	\
	DDlex.c		DDlex.h		\
	LDparser.c	LDparser.h	\
	LDlex.c		LDlex.h		\
	DIparser.c	DIparser.h	\
	DIlex.c		DIlex.h		\
	BDparser.c	BDparser.h	\
	BDlex.c		BDlex.h		\
	directory.c	directory.h	\
	driver.c	driver.h	\
	tcp.c		tcp.h		\
	pty.c		pty.h		\
	queue.c		queue.h		\
	enum.h

libpandac_a_SOURCES	=		\
	misc.c		misc.h		\
	option.c	option.h	\
	comm.c		comm.h		\
	LBSfunc.c	LBSfunc.h	\
	value.c		value.h		\
	driver.c	driver.h	\
	tcp.c		tcp.h		\
	queue.c		queue.h		\
	enum.h

libpandas_a_SOURCES	=		\
	misc.c		misc.h		\
	option.c	option.h	\
	comm.c		comm.h		\
	LBSfunc.c	LBSfunc.h	\
	value.c		value.h		\
	SQLlex.c	SQLlex.h	\
	SQLparser.c	SQLparser.h	\
	DDparser.c	DDparser.h	\
	DDlex.c		DDlex.h		\
	applications.c applications.h	\
	pandaIO.c	pandaIO.h	\
	driver.c	driver.h	\
	tcp.c		tcp.h		\
	pty.c		pty.h		\
	auth.c		auth.h		\
	authstub.c	authstub.h	\
	front.c	front.h			\
	wfcio.c		wfcio.h		\
	enum.h	\
	demo.c	demo.h	\
	demo2.c	\
	demo3.c	\
	demo4.c

libpandat_a_SOURCES	=		\
	misc.c		misc.h		\
	option.c	option.h	\
	comm.c		comm.h		\
	LBSfunc.c	LBSfunc.h	\
	value.c		value.h		\
	SQLlex.c	SQLlex.h	\
	SQLparser.c	SQLparser.h	\
	DDparser.c	DDparser.h	\
	DDlex.c		DDlex.h		\
	LDparser.c	LDparser.h	\
	LDlex.c		LDlex.h		\
	DIparser.c	DIparser.h	\
	DIlex.c		DIlex.h		\
	BDparser.c	BDparser.h	\
	BDlex.c		BDlex.h		\
	directory.c	directory.h	\
	driver.c	driver.h	\
	tcp.c		tcp.h		\
	pty.c		pty.h		\
	auth.c		auth.h		\
	authstub.c	authstub.h	\
	front.c	front.h			\
	wfcio.c		wfcio.h		\
	enum.h

libi18n_a_SOURCES	=		\
	i18n_res.c i18n_rec.h	\
	i18n_codeset.c	i18n_codeset.h	\
	i18n_code.c	i18n_code.h	\
	i18n_ctype.c	\
	i18n_strings.c	i18n_struct.h	i18n_const.h

enum.h:	enum
	./enum > enum.h
