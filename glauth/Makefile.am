# PANDA -- a simple transaction monitor
# Copyright (C) 2005-2008  ogochan
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA
## Process this file with automake to produce Makefile.in

sbin_PROGRAMS = glauth user

glauth_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

user_LDADD 		=						\
	-lm									\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

INCLUDES 			= 					\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/include				\
	-I$(top_srcdir)/libs				\
	-I$(top_srcdir)/datalib				\
	$(P11_CFLAGS)						\
	$(SSL_CFLAGS)						\
	$(GLIB_CFLAGS)						\
	$(MONDAI_CFLAGS)

glauth_SOURCES	=	\
	glauth.c

user_SOURCES	=	\
	user.c

glauth_DEPENDENCIES	=					\
	$(top_builddir)/libs/libpanda.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a
user_DEPENDENCIES	=					\
	$(top_builddir)/libs/libpandamisc.a

install-exec-hook:
	cd $(DESTDIR)$(sbindir)/ && $(LN_S) -f user$(EXEEXT) gluseradd$(EXEEXT)
	cd $(DESTDIR)$(sbindir)/ && $(LN_S) -f user$(EXEEXT) gluserdel$(EXEEXT)
	cd $(DESTDIR)$(sbindir)/ && $(LN_S) -f user$(EXEEXT) glusermod$(EXEEXT)
