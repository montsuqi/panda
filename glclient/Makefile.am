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

bin_PROGRAMS = 						\
	glclient

glclient_SOURCES =					\
	client.c		glclient.h		\
	bootdialog.c	bootdialog.h	\
	bd_component.c	bd_component.h	\
	bd_config.c		bd_config.h		\
	action.c		action.h		\
	protocol.c		protocol.h		\
	callbacks.c		callbacks.h		\
	widgetOPS.c		widgetOPS.h		\
	styleLex.c		styleLex.h		\
	styleParser.c	styleParser.h	\
	dialogs.c		dialogs.h		\
	fileEntry.c		fileEntry.h

glclient_DEPENDENCIES =					\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandagl.a

INCLUDES =						\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libs		\
	$(SSL_CFLAGS)				\
	$(XML_CFLAGS)				\
	$(GLADE_CFLAGS)				\
	$(MONDAI_CFLAGS)			\
	$(GTK_PANDA_CFLAGS)			\
	$(GTK_CFLAGS)				\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

glclient_LDADD =						\
	-lm									\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandagl.a	\
	$(GLIB_LIBS)						\
	$(GTK_LIBS)							\
	$(GLADE_LIBS)						\
	$(SSL_LIBS)							\
	$(GTK_PANDA_LIBS)					\
	$(MONDAI_LIBS)

noinst_PROGRAMS = testdialogs testfileentry

test: testdialogs testfileentry

testdialogs_LDADD =				\
	$(GTK_LIBS) 				\
	$(GLADE_LIBS) 				\
	dialogs.o

testfileentry_LDADD =			\
	$(GTK_LIBS) 				\
	$(GLADE_LIBS) 				\
	$(MONDAI_LIBS) 				\
	dialogs.o 					\
	fileEntry.o 

AM_CPPFLAGS = 					\
	-DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = 					\
	testrc
