## Process this file with automake to produce Makefile.in

bin_PROGRAMS = copygen dbgen checkdir
sbin_PROGRAMS = user monitor
bin_SCRIPTS = recdefgen.rb
#bin_PROGRAMS = copygen structgen dbgen user checkdir monitor

LDADD 		=	\
	-L/usr/lib -lglib -lm \
	$(FIXED_LIBS)  -ldl	\
	-L$(top_srcdir)/datalib -lpandad	\
	-L$(top_srcdir)/libs -lpandat
dbgen_LDADD 		=	\
	-L/usr/lib -lglib -lm \
	$(FIXED_LIBS)  -ldl	\
	-L$(top_srcdir)/datalib -lpandad	\
	-L$(top_srcdir)/libs -lpandat
user_LDADD 		=	\
	-L/usr/lib -lglib -lm \
	$(FIXED_LIBS)  -ldl	\
	-L$(top_srcdir)/datalib -lpandad	\
	-L$(top_srcdir)/libs -lpandat -lcrypt 

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	-I$(top_srcdir)/datalib	\
	$(GNOME_INCLUDEDIR) \
	$(FIXED_CFLAGS)

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
	$(LN_S) -f user $(bindir)/gluseradd
	$(LN_S) -f user $(bindir)/gluserdel
	$(LN_S) -f user $(bindir)/glusermod
