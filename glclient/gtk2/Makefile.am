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

lib_LTLIBRARIES	=	libuigtk2.la

libuigtk2_la_LDFLAGS	=	\
	-version-info $(LTVERSION)

libuigtk2_la_SOURCES	=				\
	action.c		action.h		\
	bd_config.c		bd_config.h		\
	bd_component.c	bd_component.h	\
	bootdialog.c	bootdialog.h	\
	callbacks.c		callbacks.h		\
	dialogs.c		dialogs.h		\
	styleLex.c		styleLex.h		\
	styleParser.c	styleParser.h	\
	widgetOPS.c		widgetOPS.h		\
	toplevel.h						\
	interface.c

libuigtk2_la_CFLAGS	=			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libs		\
	-I$(top_srcdir)/glclient	\
	$(GTK2CLIENT_CFLAGS)		\
	$(MONDAI_CFLAGS)			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

libuigtk2_la_LIBADD	=					\
	-lm									\
	$(GTK2CLIENT_LIBS)					\
	$(MONDAI_LIBS)
