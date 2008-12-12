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

if GTK1CLIENT
gtk1bin	=	glclient
gtk1dir	=	gtk1
else
gtk1bin	=	
gtk1dir	=	
endif

if GTK2CLIENT
gtk2bin	=	glclient2
gtk2dir	=	gtk2
else
gtk2bin	=	
gtk2dir	=	
endif

SUBDIRS	=		\
	$(gtk1dir)	\
	$(gtk2dir)	

bin_PROGRAMS = 	\
	$(gtk1bin)	\
	$(gtk2bin)

base_sources =						\
	client.c		glclient.h		\
	protocol.c		protocol.h		\
	marshaller.c	marshaller.h	\
	interface.h

base_dependencies =								\
	$(top_builddir)/libs/libpandamisc.a			\
	$(top_builddir)/libs/libpandagl.a

base_CFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libs		\
	$(SSL_CFLAGS)				\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

base_LDADD =									\
	-lm											\
	$(top_builddir)/libs/libpandamisc.a			\
	$(top_builddir)/libs/libpandagl.a			\
	$(SSL_LIBS)									\
	$(MONDAI_LIBS)

#################################################
# glclient1
#################################################
	
glclient_SOURCES =		\
	$(base_sources)

glclient_DEPENDENCIES =							\
	$(base_dependencies)						\
	$(top_builddir)/glclient/gtk1/libuigtk1.la

glclient_CFLAGS =				\
	$(GTK1CLIENT_CFLAGS)		\
	$(base_CFLAGS)

glclient_LDADD =								\
	$(GLIB_LIBS)								\
	$(GTK1CLIENT_LIBS)							\
	$(base_LDADD)								\
	$(top_builddir)/glclient/gtk1/libuigtk1.la

#################################################
# glclient2
#################################################

glclient2_SOURCES =		\
	$(base_sources)

glclient2_DEPENDENCIES =						\
	$(top_builddir)/glclient/gtk2/libuigtk2.la	\
	$(base_dependencies)

glclient2_CFLAGS =				\
	$(GTK2CLIENT_CFLAGS)		\
	$(base_CFLAGS)

glclient2_LDADD =								\
	$(GTK2CLIENT_LIBS)							\
	$(top_builddir)/glclient/gtk2/libuigtk2.la	\
	$(base_LDADD)

AM_CPPFLAGS = 					\
	-DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = 					\
	testrc
