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

bin_PROGRAMS = 	\
	glclient2

base_sources =						\
	client.c		glclient.h		\
	desktop.c		desktop.h		\
	protocol.c		protocol.h		\
	marshaller.c	marshaller.h	\
	widgetcache.c	widgetcache.h	\
	print.c			print.h			\
	utils.c			utils.h			\
	action.c		action.h		\
	bd_config.c		bd_config.h		\
	bd_component.c	bd_component.h	\
	bootdialog.c	bootdialog.h	\
	callbacks.c		callbacks.h		\
	dialogs.c		dialogs.h		\
	download.c		download.h		\
	styleLex.c		styleLex.h		\
	styleParser.c	styleParser.h	\
	notify.c		notify.h		\
	printservice.c	printservice.h	\
	widgetOPS.c		widgetOPS.h		\
	toplevel.h

base_dependencies =								\
	$(top_builddir)/libs/libpandamisc.a			\
	$(top_builddir)/libs/libpandagl.a

base_CFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libs		\
	$(COMMON_CFLAGS)			\
	$(SSL_CFLAGS)				\
	$(P11_CFLAGS)				\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

base_LDADD =									\
	-lm											\
	$(top_builddir)/libs/libpandamisc.a			\
	$(top_builddir)/libs/libpandagl.a			\
	$(COMMON_LIBS)								\
	$(P11_LIBS)									\
	$(SSL_LIBS)			

#################################################
# glclient2
#################################################

glclient2_SOURCES =		\
	$(base_sources)

glclient2_DEPENDENCIES =						\
	$(base_dependencies)

glclient2_CFLAGS =			\
	$(base_CFLAGS)			\
	$(GLCLIENT_CFLAGS)

glclient2_LDADD =								\
	$(GLCLIENT_LIBS)							\
	$(base_LDADD)

AM_CPPFLAGS = 					\
	-DLOCALEDIR=\"$(localedir)\"\
	-DGLCLIENT_DATADIR=\"$(datadir)/panda-client\"
