## Process this file with automake to produce Makefile.in

bin_PROGRAMS = copygen dbgen user checkdir monitor
#bin_PROGRAMS = copygen structgen dbgen user checkdir monitor

LDADD 		=	\
	-L/usr/lib -lglib -lm \
	$(FIXED_LIBS)  -ldl	\
	-L$(top_srcdir)/libs -lpandat
user_LDADD 		=	\
	-L/usr/lib -lglib -lm \
	$(FIXED_LIBS)  -ldl	\
	-L$(top_srcdir)/libs -lpandat -lcrypt 

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(GNOME_INCLUDEDIR)

copygen_SOURCES	=	\
	copygen.c

#structgen_SOURCES	=	\
#	structgen.c

dbgen_SOURCES	=	\
	dbgen.c

user_SOURCES	=	\
	user.c

checkdir_SOURCES	=	\
	checkdir.c
monitor_SOURCES	=	\
	monitor.c

install-exec-hook:
	cd	$(bindir)
	$(LN_S) -f user gluseradd
	$(LN_S) -f user gluserdel
	$(LN_S) -f user glusermod
	$(LN_S) -f user glauth
