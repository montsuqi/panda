## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = glauth
bin_PROGRAMS = user

glauth_LDADD 		=					\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

user_LDADD 		=						\
	-lm									\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(FIXED_LIBS)						\
	$(MONDAI_LIBS)						\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la

INCLUDES 			= 					\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/include				\
	-I$(top_srcdir)/libs				\
	-I$(top_srcdir)/datalib				\
	$(SSL_CFLAGS)						\
	$(GLIB_CFLAGS)						\
	$(MONDAI_CFLAGS)					\
	$(FIXED_CFLAGS)

glauth_SOURCES	=	\
	glauth.c

user_SOURCES	=	\
	user.c

glauth_DEPENDENCIES	=					\
	$(top_builddir)/libs/libpanda.la		\
	$(top_builddir)/libs/libpandamisc.la	\
	$(top_builddir)/libs/libpandacom.la
user_DEPENDENCIES	=					\
	$(top_builddir)/libs/libpandamisc.la
