lib_LTLIBRARIES = libpandadata.la

libpandadata_la_LDFLAGS	=	\
	-version-info $(LTVERSION)	\
	-export-dynamic

libpandaincludedir = $(includedir)/panda
libpandainclude_HEADERS = misc.h option.h LBSfunc.h value.h DDparser.h

INCLUDES 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/datalib	\
	$(GNOME_INCLUDEDIR) \
	$(FIXED_CFLAGS)

libpandadata_la_SOURCES	=		\
	misc.c		misc.h		\
	option.c	option.h	\
	LBSfunc.c	LBSfunc.h	\
	value.c		value.h		\
	SQLlex.c	SQLlex.h	\
	SQLparser.c	SQLparser.h	\
	DDparser.c	DDparser.h	\
	DDlex.c		DDlex.h
