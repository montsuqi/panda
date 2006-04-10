# PANDA -- a simple transaction monitor
# Copyright (C) 2005  ogochan
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

bin_PROGRAMS = mon htcproc mce
sbin_PROGRAMS = htserver

htserver_LDFLAGS	=	-export-dynamic
htserver_LDADD 		=					\
	-lm									\
	$(top_builddir)/libs/libpandas.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(SSL_LIBS)							\
	$(GLIB_LIBS)						\
	$(MONDAI_LIBS)

mon_LDADD 		=						\
	-lm									\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

htcproc_LDADD 	=						\
	-lm									\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)

INCLUDES 			= \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(POSTGRES_CFLAGS)	\
	$(SSL_CFLAGS)		\
	$(MONDAI_CFLAGS)	\
	$(XSLT_CFLAGS)		\
	$(GLIB_CFLAGS)

htserver_SOURCES	=	\
	htserver.c	htserver.h	\
	server.c	\
	trid.c		trid.h
htserver_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandas.a		\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

mon_SOURCES	=	\
	cgi.c		cgi.h		\
	multipart.c	multipart.h	\
	HTClex.c	HTClex.h	\
	HTCparser.c	HTCparser.h	\
	eruby.c		eruby.h		\
	tags.c		tags.h		\
	exec.c		exec.h		\
	mon.c		mon.h

mon_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

htcproc_SOURCES	=	\
	cgi.c		cgi.h		\
	multipart.c	multipart.h	\
	HTClex.c	HTClex.h	\
	HTCparser.c	HTCparser.h	\
	eruby.c		eruby.h		\
	tags.c		tags.h		\
	exec.c		exec.h		\
	htcproc.c

htcproc_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a

mce_LDADD 		=						\
	-lm									\
	$(top_builddir)/libs/libpandamisc.a	\
	$(top_builddir)/libs/libpandacom.a	\
	$(GLIB_LIBS)						\
	$(SSL_LIBS)							\
	$(MONDAI_LIBS)						\
	$(XML2_LIBS)						\
	$(XSLT_LIBS)

mcd_INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/libs		\
	$(XML2_CFLAGS)				\
	$(XSLT_CFLAGS)				\
	$(MONDAI_CFLAGS)			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

mce_SOURCES	=	\
	mce.c
