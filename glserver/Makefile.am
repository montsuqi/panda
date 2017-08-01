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

sbin_PROGRAMS = glserver

glserver_LDFLAGS	=	-export-dynamic
glserver_LDADD 		=					\
	-lm 								\
	$(top_builddir)/libs/libpanda.la	\
	$(MAGIC_LIBS)						\
	$(P11_LIBS)							\
	$(SSL_LIBS)							\
	$(COMMON_LIBS)

glserver_DEPENDENCIES	=				\
	$(top_builddir)/libs/libpanda.la
AM_CPPFLAGS 			= 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/libs	\
	$(P11_CFLAGS)			\
	$(SSL_CFLAGS)			\
	$(COMMON_CFLAGS)		\
	-DBIN_DIR=\"$(bindir)\"

glserver_SOURCES	=				\
	glserver.c		glserver.h		\
	http.c			http.h			\
	wfcio.c			wfcio.h			\
	sysdataio.c		sysdataio.h		\
	blobapi.c		blobapi.h
